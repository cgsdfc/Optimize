#! /usr/bin/env python3

"""Tokenize a program"""
import re
from tokenize import TokenInfo
from tokenize import ISTERMINAL as is_terminal
from tokenize import ISNONTERMINAL as is_nonterminal

from simplecompiler.compiler.Symbol import *

__all__ = [ "is_terminal", "is_nonterminal", "tokenize", "print_tokens" ]

def group(*args): return '({})'.format('|'.join(args))


def make_string():
    return '[{}-{}]'.format(bytes([32, 33, 35]).decode(), bytes([126]).decode())


String = r'"{}*"'.format(make_string())
Char = r"'[+\-*/_a-zA-Z0-9]'"
Name = r'[_a-zA-Z][_a-zA-Z0-9]*'
Number = r'\d+'
Operators = group(r'[+\-*/]', r'[=!]?=', r'[<>]=?')
Bracket = '[][(){}]'
Special = group(r'[;:,]', r'\r?\n')
Whitespace = r'[ \f\t]*'
PseudoToken = re.compile(
    Whitespace + group(String, Number, Char, Name, Operators, Bracket, Special))
Blank = re.compile(r'[ \t\f]*(?:[\r\n]|$)')


def _tokenize(readline):
    """Tokenize lines from ``readline()``, which should be the ``__next__``
    attribute of an opened file in text mode.

    >>> with open(file) as f:
        ... return list(tokenize(f.__next__))
    """
    numchars = '0123456789'
    lnum = 0
    readline = iter(readline, '')

    while True:
        try:
            line = next(readline)
            lnum += 1
        except StopIteration:
            break

        pos, max = 0, len(line)
        if Blank.match(line):
            continue
        while pos < max:
            psmat = PseudoToken.match(line, pos)
            if psmat:
                start, end = psmat.span(1)
                spos, epos, pos = (lnum, start), (lnum, end), end
                if start == end:
                    continue
                token, initial = line[start: end], line[start]
                if initial in numchars:
                    yield TokenInfo(NUMBER, token, spos, epos, line)
                elif initial in '\r\n':
                    continue
                elif initial in ("\"", "'"):
                    yield TokenInfo(CHAR if initial == "'" else STRING,
                                    token, spos, epos, line)
                elif initial.isidentifier():
                    yield TokenInfo(NAME, token.lower(), spos, epos, line)
                else:
                    yield TokenInfo(OP, token, spos, epos, line)
            else:
                yield TokenInfo(ERRORTOKEN, line[pos],
                                (lnum, pos), (lnum, pos+1), line)
                pos += 1
    yield TokenInfo(ENDMARKER, '', (lnum, 0), (lnum, 0), '')


def tokenize(input):
    return list(_tokenize(input.__next__))

def print_tokens(tokens, output):
    for token in tokens:
        token_range = "%d,%d-%d,%d:" % (token.start + token.end)
        print("%-20s%-15s%-15r" %
              (token_range, tok_name[token.type], token.string), file=output)
