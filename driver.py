# Copyright 2004-2005 Elemental Security, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

# Modifications:
# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Parser driver.

This provides a high-level interface to parse a file into a syntax tree.

"""

__author__ = "Guido van Rossum <guido@python.org>"

__all__ = ["Driver"]

# Python imports
import io
import os
import logging
import sys

# Local imports
from tokenizer import tokenize
from parser import ParseError, Parser


class Driver(object):

    def __init__(self, grammar, start=None, logger=None):
        if logger is None:
            logger = logging.getLogger()
        self.grammar = grammar
        self.logger = logger
        self.parser = Parser(grammar, start)

    def parse_tokens(self, tokens, debug=False):
        """Parse a series of tokens and return the syntax tree."""
        # XXX Move the prefix computation into a wrapper around tokenize.
        lineno = 1
        column = 0
        type = value = start = end = line_text = None
        prefix = ""
        for quintuple in tokens:
            type, value, start, end, line_text = quintuple
            if start != (lineno, column):
                s_lineno, s_column = start
                if lineno < s_lineno:
                    prefix += "\n" * (s_lineno - lineno)
                    lineno = s_lineno
                    column = 0
                if column < s_column:
                    prefix += line_text[column:s_column]
                    column = s_column
            if debug:
                self.logger.debug("%s %r (prefix=%r)", type, value, prefix)
            if self.parser.addtoken(type, value, (prefix, start)):
                if debug:
                    self.logger.debug("Stop.")
                break
            prefix = ""
            lineno, column = end
            if value.endswith("\n"):
                lineno += 1
                column = 0
        else:
            # We never broke out -- EOF is too soon (how can this happen???)
            raise ParseError("incomplete input",
                                   type, value, (prefix, start))
        return self.parser.rootnode

    def parse_stream(self, stream, debug=False):
        """Parse a stream and return the syntax tree."""
        tokens = tokenize(stream.__next__)
        return self.parse_tokens(tokens, debug)

    def parse_file(self, filename, debug=False):
        """Parse a file and return the syntax tree."""
        with open(filename) as stream:
            return self.parse_stream(stream, debug)

    def parse_string(self, text, debug=False):
        """Parse a string and return the syntax tree."""
        tokens = tokenize(io.StringIO(text).readline)
        return self.parse_tokens(tokens, debug)
