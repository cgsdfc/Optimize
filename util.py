import os
import subprocess
import tempfile
import sys

TABSIZE = 4
MAX_COL = 80

def reflow_lines(s, depth):
    """Reflow the line s indented depth tabs.

    Return a sequence of lines where no line extends beyond MAX_COL
    when properly indented.  The first line is properly indented based
    exclusively on depth * TABSIZE.  All following lines -- these are
    the reflowed lines generated by this function -- start at the same
    column as the first character beyond the opening { in the first
    line.
    """
    size = MAX_COL - depth * TABSIZE
    if len(s) < size:
        return [s]

    lines = []
    cur = s
    padding = ""
    while len(cur) > size:
        i = cur.rfind(' ', 0, size)
        # XXX this should be fixed for real
        if i == -1 and 'GeneratorExp' in cur:
            i = size + 3
        assert i != -1, "Impossible line %d to reflow: %r" % (size, s)
        lines.append(padding + cur[:i])
        if len(lines) == 1:
            # find new size based on brace
            j = cur.find('{', 0, i)
            if j >= 0:
                j += 2 # account for the brace and the space after it
                size -= j
                padding = " " * j
            else:
                j = cur.find('(', 0, i)
                if j >= 0:
                    j += 1 # account for the paren (no space after it)
                    size -= j
                    padding = " " * j
        cur = cur[i+1:]
    else:
        lines.append(padding + cur)
    return lines


def indented_lines(lines, depth=0):
    """Use it in a template.

    template = Template('''
    enum {
        $constants
    };
    ''')

    template.substitute(constants=indented_lines(iterable, 1))
    """

    indented = map(lambda l: (" " * TABSIZE * depth) + l, lines)
    return "\n".join(indented)


class Emittor:
    "Base class to do emission with reflow."

    def __init__(self, file, *args):
        super().__init__(*args)
        self.file = file

    def emit(self, line, depth=0):
        lines = reflow_lines(line, depth)
        for line in lines:
            if line:
                line = (" " * TABSIZE * depth) + line
            self.file.write(line + "\n")

    def emit_raw(self, string):
        lines = string.split("\n")
        for l in lines:
            self.emit(l)


class ChainOfVisitors:
    def __init__(self, *visitors):
        self.visitors = visitors

    def visit(self, object):
        for v in self.visitors:
            v.visit(object)


def format_code(code_string, dest, external_formatter=None):
    """Format C++ code in ``code_string`` using ``external_formatter`` and
    write it to ``dest``.
    external_formatter is the program to use, default to clang-format.
    """
    if external_formatter is None:
        external_formatter = "clang-format"

    fd, temp = tempfile.mkstemp()
    with open(temp, 'w') as f:
        f.write(code_string)

    try:
        formatted = subprocess.check_output([external_formatter, temp])
        with open(dest, 'wb') as f:
            f.write(formatted)
    finally:
        os.remove(temp)


def error(msg, loc):
    print("Error in line {} column {}: {}".format(
        loc[0], loc[1], msg), file=sys.stderr)


def double_qoute(s):
    """Add double quotes to s

    >>> double_qoute('program')
    '"program"'
    """

    return '"' + s + '"'

def get_args(fields, attrs):
    """Return a list of names from fields and attrs"""
    # field name is optional
    from itertools import chain
    args = []
    unnamed = {}
    for f in chain(fields, attrs):
        if f.name is None:
            name = f.type
            c = unnamed[name] = unnamed.get(name, 0) + 1
            if c > 1:
                name = "name%d" % (c - 1)
        else:
            name = f.name
        args.append(name)
    return args
