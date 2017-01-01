# CFG Checker

*CFG Checker* searches for ambiguities in [context-free grammars](https://en.wikipedia.org/wiki/Context-free_grammar). It is only semi-decidable to determine whether an arbitrary context-free grammar is [ambiguous](https://en.wikipedia.org/wiki/Ambiguous_grammar). The best we can do is generate all derivations in a breadth-first fashion and look for two which produce the same sentential form. So that's exactly what CFG Checker does. If the input grammar is ambiguous, CFG Checker will eventually find a minimal ambiguous sentential form. If the grammar is unambiguous, CFG Checker will either reach this conclusion or loop forever.

## Getting started

Here's how it works. You specify the grammar as a series of production rules:

```cfg
expression = number | sum
sum = expression + expression
```

Then you run CFG Checker on it:

```sh
$ cfg-checker example.cfg
....
Found a sentential form with two different derivations:

  expression + expression + expression

Derivation 1:

  0: expression
  1: sum
  2: expression + expression
  3: expression + sum
  4: expression + expression + expression

Derivation 2:

  0: expression
  1: sum
  2: expression + expression
  3: sum + expression
  4: expression + expression + expression
```

If it doesn't finish within a few seconds, the grammar is probably unambiguous unless you gave it some pathological input.

## Grammar syntax

Each line is a production rule. A production rule specifies that a symbol may be rewritten as one of several alternatives separated by pipes (`|`). An alternative is a (possibly empty) sequence of symbols separated by whitespace. Consider this example:

```cfg
S = a S a | b S b | c
```

This rule says `S` can be rewritten to `a S a`, `b S b`, or `c`.

A symbol can be any non-whitespace string except a pipe (`|`). All of the following are valid symbols: `東京`, `foo`, `=`, `+`, `"`, `(`, `)`, etc.

The first rule in the grammar determines the start symbol.

## How to build and install

Just run `make` from the root of this repository. A binary called `cfg-checker` will be produced. You can run `make install` to install it to `/usr/local/bin`, or `make install PREFIX=/custom/path` to change the installation prefix.

CFG Checker is written in C++ and has no dependencies.

## License

Copyright (c) 2017 Stephan Boyer

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
