#! /usr/bin/env python3
"""Ast generation"""

from operator import attrgetter
from string import Template
from itertools import chain

import asdl
from util import double_qoute

TAB = "    "

def is_simple(sum):
    """Return True if a sum is a simple.

    A sum is simple if its types have no fields, e.g.
    unaryop = Invert | Not | UAdd | USub
    """
    return not any(t.fields for t in sum.types)


class PythonType:
    """Base class for Python Ast types"""

    def __init__(self, name):
        self.name = name


class Enum(PythonType):
    template = Template("""
$name = Enum("$name", "$members")
""")

    def __init__(self, name, values):
        super().__init__(name)
        self.values = values

    def __str__(self):
        return self.template.substitute(
            name=self.name,
            members=" ".join(self.values),
        )

class ConcreteNode(PythonType):
    template = Template("""
class $name($base):
    __slots__ = ($slots)

    def __init__(self, $args):
        $inits
""")

    init = Template("""self.$name = $name""")

    def __init__(self, name, base, members):
        super().__init__(name)
        self.base = base
        self.members = members

    def __str__(self):
        return self.template.substitute(
            name=self.name,
            base=self.base,
            slots=''.join(map(lambda s: double_qoute(s) + ',', self.members)),
            args=", ".join(self.members),
            inits=("\n" + TAB * 2).join(
                map(lambda n: self.init.substitute(name=n), self.members)
            )
        )

class AbstractNode(PythonType):
    template = Template("""
class $name(AST):
    pass
""")

    def __str__(self):
        return self.template.substitute(name=self.name)

class LeafNode(PythonType):
    template = Template("""
$name = namedtuple("$name", "$members")
""")

    def __init__(self, name, members):
        super().__init__(name)
        self.members = members

    def __str__(self):
        return self.template.substitute(
            name=self.name,
            members=" ".join(self.members),
        )


class TypeVisitor(asdl.VisitorBase):

    def __init__(self):
        super().__init__()

    def visitModule(self, mod):
        for dfn in mod.dfns:
            yield from self.visit(dfn)

    def visitType(self, type):
        yield from self.visit(type.value, type.name)

    def visitSum(self, sum, name):
        # Enum
        if is_simple(sum):
            yield Enum(name, [cons.name for cons in sum.types])

        # LeafNode
        elif len(sum.types) == 1: # direct subclass of AST -- LeafNode
            cons = sum.types[0]
            members = [ f.name for f in chain(cons.fields, sum.attributes)]
            yield ConcreteNode(cons.name, 'AST', members)
        else:
            # AbstractNode
            yield AbstractNode(name)

            # ConcreteNode
            for cons in sum.types:
                yield ConcreteNode(cons.name, name,
                        [f.name for f in chain(cons.fields, sum.attributes)])


    def visitProduct(self, prod, name):
        # LeafNode
        yield LeafNode(name,
                [f.name for f in chain(prod.fields, prod.attributes)])


class ImplTemplate:
    impl = Template("""# Automatically Generated File
from enum import Enum
from collections import namedtuple

class AST:
    '''Base class of AST'''

    def __repr__(self):
        return "{name}({data})".format(
            name=self.__class__.__name__,
            data=", ".join("{}={}".format(
                name,
                getattr(self, name)
            ) for name in self.__slots__)
        )

    @property
    def _fields(self):
        return self.__slots__

    def __iter__(self):
        for name in self.__slots__:
            child = getattr(self, name)
            if isinstance(child, list):
                yield from child
            else:
                yield child

$definitions

# Hard coded mapping from string to their ASTs
string2operator = {
    "+": operator.Add,
    "-": operator.Sub,
    "*": operator.Mult,
    "/": operator.Div,
    "==": operator.Eq,
    "!=": operator.NotEq,
    "<": operator.Lt,
    "<=": operator.LtE,
    ">": operator.Gt,
    ">=": operator.GtE,
}

string2unaryop = {
    "+": unaryop.UAdd,
    "-": unaryop.USub,
}

string2basic_type = {
    "int": basic_type.Int,
    "char": basic_type.Character,
    "void": basic_type.Void,
}
""")

    def substitute(self, types):
        return self.impl.substitute(
            definitions="\n".join(map(str, types))
        )


def main():
    import argparse
    import json

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', dest='config', type=argparse.FileType(),
            help='configure file', required=1)
    parser.add_argument('--dump_module', action='store_true', default=False,
            help='Dump the asdl module and exit')
    args = parser.parse_args()
    config = json.load(args.config)

    mod = asdl.parse(config['AST']['asdl'])
    if args.dump_module:
        print(mod)
        return 0
    if not asdl.check(mod):
        return 1

    with open(config['AST']['AST.py'], 'w') as f:
        types = TypeVisitor().visit(mod)
        f.write(ImplTemplate().substitute(types))
    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
