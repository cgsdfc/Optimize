#! /usr/bin/env python3

"""Ast generation"""

import sys
import asdl
import util
import abc

from pprint import pprint
from itertools import chain


def camal_case(name):
    """Convert snake_case to camal_case, respecting the original
    camals in ``name``
    """
    # uppercase the first letter, keep the rest unchanged
    return ''.join(c[0].upper() + c[1:] for c in name.split('_'))


def is_simple(sum):
    """Return True if a sum is a simple.

    A sum is simple if its types have no fields, e.g.
    unaryop = Invert | Not | UAdd | USub
    """
    return not any(t.fields for t in sum.types)


# Represented the cpp types we are dealing with.

class CppType(metaclass=abc.ABCMeta):
    """Base type for a cpp type that goes into output"""

    def const_ref(self, name):
        return 'const {}&'.format(name)

    def ptr(self, name):
        return name + '*'

    @abc.abstractproperty
    def as_member(self):
        """Form in member declaration"""

    @abc.abstractproperty
    def as_argument(self):
        """Form in arguments type"""

    def __repr__(self):
        # taking subclass's name
        return "{}({!r})".format(self.__class__.__name__, self.name)


class AstNode(CppType):
    """AstNode represents subclasses of AST"""

    def __init__(self, name):
        self.name = camal_case(name)

    @property
    def as_member(self):
        return self.ptr(self.name)

    as_argument = as_member


class Sequence(CppType):
    """A list of item dealing with * in asdl"""

    impl = 'std::vector<{}>'

    def __init__(self, elemtype):
        self.elemtype = elemtype

    @property
    def as_member(self):
        return self.impl.format(self.elemtype.as_member)

    @property
    def as_argument(self):
        return self.const_ref(self.as_member)

    name = as_member


class Optional(CppType):
    """A optional item dealing with ? in asdl"""

    impl = 'std::optional<{}>'

    def __init__(self, elemtype):
        self.elemtype = elemtype

    @property
    def as_member(self):
        if isinstance(self.elemtype, AstNode):
            # nullptr serves a great optional
            return self.elemtype.as_member
        else:
            return self.impl.format(self.elemtype.as_member)

    as_argument = as_member
    name = as_member


class Primitive(CppType):
    """Primitive types in both asdl and cpp"""

    strimpl = 'std::string'
    asdl2cpp = {
        'identifier': strimpl,
        'string': strimpl,
        'location': 'Location',
        'int': 'int',
    }

    def __init__(self, name):
        self.name = self.asdl2cpp[name]

    @property
    def as_member(self):
        return self.name

    @property
    def as_argument(self):
        if self.name == 'int':
            # too verbose to say "const int&"
            return self.as_member
        else:
            # larger types
            return self.const_ref(self.name)


class Enum(CppType):
    """Representing ``enum class`` of cpp"""

    def __init__(self, name):
        self.name = camal_case(name) + 'Kind'

    @property
    def as_member(self):
        # enums are just ints
        return self.name

    as_argument = as_member


class Emittor:
    """Base class for all other emittor"""

    def emit_forward(self, e):
        """Emit a forward declaration which comes first in the header"""

    def emit_definition(self, e):
        """Emit a definition of struct or class (goes into header)"""

    def emit_implementaion(self, e):
        """Emit an implementation (goes into .cpp)"""
        # for future use


class AstNodeEmittor(Emittor):
    """An AstNode is part of the Ast tree."""

    format_header = "void Format(std::ostream &os) const override {"

    def __init__(self, name, members):
        """``name`` cpp name
        ``members`` a list of (CppType, name)
        """
        self.name = name
        self.members = members

    def emit_members(self, e, depth=1):
        """Emit data member declaration"""
        for type, name in self.members:
            e.emit("{} {};".format(type.as_member, name), depth)

    def emit_constructor(self, e, depth=1):
        """Emit a constructor definition"""
        e.emit("{name}({parameters}): {init} {{}}".format(
            name=self.name,
            parameters=", ".join("{} {}".format(type.as_argument, name)
                for type, name in self.members),
            init=", ".join("{0}({0})".format(name)
                for _, name in self.members),
        ), depth)

    def emit_formatter(self, e):
        """Emit Format() method"""
        e.emit(self.format_header, 1)
        e.emit("os << \"{}(\" <<".format(self.name), 2)
        e.emit(" << \", \" << ".join("\"{0}=\" << {0}".format(name)
            for _, name in self.members), 2)
        e.emit("<< \")\";", 2)
        e.emit("}", 1)

    def emit_forward(self, class_or_struct, e):
        e.emit("{} {};".format(class_or_struct, self.name))


class ProductStructEmittor(AstNodeEmittor):
    """A ``struct`` representing ``asdl.Product``"""

    def emit_struct(self, e):
        e.emit("struct {}: public {} {{".format(self.name, 'AST'))
        self.emit_members(e)
        self.emit_constructor(e)
        self.emit_formatter(e)
        e.emit("};")

    def emit_definition(self, e):
        self.emit_struct(e)

    def emit_forward(self, e):
        super().emit_forward('struct', e)


class ConcreteNodeEmittor(AstNodeEmittor):
    """A ``class`` representing a concrete AstNode"""

    subclass_kind_header = "int SubclassKind() const override {"

    def __init__(self, name, members, base):
        super().__init__(name, members)
        self.base = base

    def emit_definition(self, e):
        """Emit the class definition for this ConcreteNode"""
        e.emit("class {}: public {} {{".format(self.name, self.base))
        e.emit("public:")
        self.emit_members(e)
        self.emit_constructor(e)
        if self.base != 'AST':
            self.emit_subclass_kind(e)
        self.emit_formatter(e)
        e.emit("};")

    def emit_subclass_kind(self, e):
        e.emit(self.subclass_kind_header, 1)
        e.emit("return {enum}::{val};".format(enum=self.base, val=self.name), 2)
        e.emit("}", 1)

    def emit_forward(self, e):
        super().emit_forward('class', e)


class EnumEmittor(Emittor):
    """Enum provides common base for types that have an enum or is an enum."""

    def __init__(self, name, members):
        self.name = name
        self.members = members

    @property
    def formatted_members(self):
        return "{" + ", ".join(self.members) + "};"


class AbstractNodeEmittor(EnumEmittor):
    """Emit an AbstractNode, which cannot be instantiated, but rather provides a base for all its
    children and scope for the enum to tell its children apart.

    Expr, Stmt and Decl are examples of AbstractNode.
    Their definitions look like:

    class Expr: public AST {
    public:
        enum { ... };
    };
    """

    base_class = 'AST'
    subclass_kind_header = "virtual int SubclassKind() const = 0;"
    class_header = "class {}: public {} {{"

    def emit_definition(self, e):
        e.emit(self.class_header.format(self.name, self.base_class))
        e.emit("public:")
        e.emit("enum " + self.formatted_members, 1)
        e.emit(self.subclass_kind_header, 1)
        e.emit("};")

    def emit_forward(self, e):
        e.emit("class {};".format(self.name))


class SimpleEnumEmittor(EnumEmittor):
    """Enum representing enum class"""

    # enum_value to string
    format_header = "inline std::ostream &operator<<(std::ostream &os, {name} val) {{"

    def emit_formatter(self, e):
        e.emit(self.format_header.format(name=self.name))
        e.emit("switch (val) {", 1)
        for member in self.members:
            e.emit(f"case {self.name}::{member}: os << \"{self.name}::{member}\";", 1)
        e.emit("}", 1)
        e.emit("return os;", 1)
        e.emit("}")

    def emit_forward(self, e):
        e.emit("enum class {} {}".format(self.name, self.formatted_members))
        self.emit_formatter(e)
        e.emit("")


class TypeVisitor(asdl.VisitorBase):
    """A visitor that map types from asdl to cpp."""

    def __init__(self):
        super().__init__()
        # hard coded known types
        self.typemap = {
            'identifier': Primitive('identifier'),
            'string': Primitive('string'),
            'location': Primitive('location'),
            'int': Primitive('int'),
        }

    def visitModule(self, mod):
        for dfn in mod.dfns:
            self.visit(dfn)

    def visitType(self, type):
        self.visit(type.value, str(type.name))

    def visitSum(self, sum, name):
        n_branches = len(sum.types)
        if n_branches > 1:
            if is_simple(sum): # simple sum becomes enum class
                self.typemap[name] = Enum(name)
            else: # complex sum becomes AbstractNode
                self.typemap[name] = AstNode(name)
        else:
            # if it has only one constructor,
            # map it to its solo constructor.
            # pure enum is not likely to be solo.
            self.typemap[name] = AstNode(name)

        for t in sum.types:
            self.visit(t)

    def visitConstructor(self, cons):
        if cons.fields:
            # constructor with fields becomes ConcreteNode
            self.typemap[cons.name] = AstNode(cons.name)

    def visitProduct(self, prod, name):
        # Product becomes ProductStruct
        self.typemap[name] = AstNode(name)



class EmittorVisitor(asdl.VisitorBase):
    """A visitor that build emittors for various asdl constructs"""

    def __init__(self, typemap):
        super().__init__()
        self.typemap = typemap
        # emit string2enum functions: some enums represent strings, like operator
        # some are not, like expr_context
        self.string2enum = String2enumEmittor()

    def visitModule(self, mod):
        for dfn in mod.dfns:
            yield from self.visit(dfn)
        yield self.string2enum

    def visitType(self, type):
        yield from self.visit(type.value, str(type.name))

    def visitSum(self, sum, name):
        n_branches = len(sum.types)
        if n_branches > 1:
            cpp_name = self.typemap[name].name
            members = self.make_enum_members(sum)
            if is_simple(sum): # enum class
                self.string2enum.enums.append((name, cpp_name))
                yield SimpleEnumEmittor(cpp_name, members)
            else:
                yield AbstractNodeEmittor(cpp_name, members)

        for type in sum.types:
            yield from self.visit(type, name, n_branches, sum.attributes)


    def make_enum_members(self, sum):
        """Make enum members from ``sum``."""
        return [cons.name for cons in sum.types]

    def make_class_members(self, fields, attributes):
        """Core function for making members for all class-based AstNode
        (as opposed to enum-based). asdl.opt becomes Optional, asdl.seq
        becomes Sequence.
        """

        def field_type(f):
            type = self.typemap[f.type]
            if f.seq: return Sequence(type)
            if f.opt: return Optional(type)
            return type

        return [(field_type(f), f.name) for f in chain(fields, attributes)]

    def visitConstructor(self, cons, name, n_branches, attributes):
        if cons.fields:
            base = self.typemap[name].name if n_branches > 1 else 'AST'
            name = self.typemap[cons.name].name
            members = self.make_class_members(cons.fields, attributes)
            yield ConcreteNodeEmittor(name, members, base)

    def visitProduct(self, prod, name):
        members = self.make_class_members(prod.fields, prod.attributes)
        yield ProductStructEmittor(self.typemap[name].name, members)


def generate_code(mod, output):
    """Emit cpp code from ``mod`` to ``output``"""
    # currently all written to a header
    v = TypeVisitor()
    v.visit(mod)
    # collect all Emittors
    cpp_types = list(EmittorVisitor(v.typemap).visit(mod))

    with open(output, 'w') as f:
        f.write(Header)
        e = util.Emittor(f)
        for c in cpp_types:
            c.emit_forward(e)
            e.emit("")
        e.emit("")

        for c in cpp_types:
            c.emit_definition(e)
            e.emit("")
        f.write(Tailer)


Header = """#ifndef AST_H
#define AST_H

#include "tokenize.h"

#include <vector>
#include <iostream>
#include <optional>

class AST {
public:
    virtual void Format(std::ostream &os) const = 0;
};

inline std::ostream &operator<<(std::ostream &os, const AST *ast) {
    ast->Format(os);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const AST &ast) {
    return os << &ast;
}

inline std::ostream &operator<<(std::ostream &os, const std::optional<std::string> &s) {
    os << s.value_or("None");
    return os;
}

template<class T>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    os << "[";
    for (auto b = v.begin(), e = v.end(); b != e; ++b) {
        os << *b;
        if (b != e - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

"""

Tailer = """
#endif"""


class String2enumEmittor(Emittor):
    """Emit functions that convert a c-string to enum value"""

    func_header = "inline {enum} {funcname}(const String &s) {{"

    # make sure these names match those in asdl!
    operator = {
        "+": "Add",
        "-": "Sub",
        "*": "Mult",
        "/": "Div",
        "==": "Eq",
        "!=": "NotEq",
        "<": "Lt",
        "<=": "LtE",
        ">": "Gt",
        ">=": "GtE",
    }

    unaryop = {
        "+": "UAdd",
        "-": "USub",
    }

    basic_type = {
        "int": "Int",
        "char": "Character",
        "void": "Void",
    }

    def __init__(self):
        # to be filled by EmittorVisitor
        self.enums = []

    def emit_func(self, table, enum, e):
        e.emit(self.func_header.format(
            enum=enum,
            funcname='String2' + enum,
        ))

        for op, val in table.items():
            e.emit("if (s == \"{op}\") return {enum}::{val};".format(
                op=op, enum=enum, val=val), 1)
            e.emit("")
        # fail with an assert
        e.emit("assert(false && \"not a member of {}\");".format(enum), 1)
        e.emit("}")

    def emit_definition(self, e):
        for attr, enum in self.enums:
            try:
                table = getattr(self, attr)
            except AttributeError:
                pass
            else:
                self.emit_func(table, enum, e)
                e.emit("")


def main():
    import argparse
    import json

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', dest='config', type=argparse.FileType(),
            help='configure file', required=1)
    args = parser.parse_args()
    config = json.load(args.config)

    mod = asdl.parse(config['asdl'])
    if not asdl.check(mod):
        return 1

    generate_code(mod, config['AST.h'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
