# File checker tool and framework for safe and somewhat Rust-like coding in C

This provides a tool to efficiently check text files for valid UTF-8
use, and to report which line separators they use. It is about 50x
faster than a corresponding Python program. It is currently used in
[gnqc](https://git.genenetwork.org/jgart/gnqc), and could easily be
extended to take on more functionality.

But this project also provides a "framework" (tooling infrastructure
and libraries) for writing programs in a manner that is a bit more
like one would work with Rust, and that uses verification tooling to
try to make it fully safe, primarily by using
[AFL++](https://aflplus.plus/) in combination with the sanitizers
(ASAN, UBSAN). Even a coverage-guided fuzzer like AFL++ doesn't
guarantee to find all bugs (and in fact given large enough software is
pretty much guaranteed not to), but, by limiting the scope of the
program exposed to fuzzing (currently the whole of
`utf-8-lineseparator` is small enough to qualify) by giving access to
subsets of the program (similar to unit testing) and verifying via
coverage tooling that no part remains untested, we hope to scale this
up to larger programs.  See our [approach](docs/approach.md) to
development and security for further details.

## Note about CSV

[CSV](https://en.wikipedia.org/wiki/Comma-separated_values) files can
contain line breaks embedded within cells; those might be different
ones than the line separator used to separate
rows. `utf-8-lineseparator` is reporting separate counts of all 3 line
separators (CR, LF, CRLF), thus it could report non-zero numbers for
multiple of those in such cases, without this meaning that the file is
broken. `utf-8-lineseparator` does not currently attempt to parse the
file as CSV.

([gnqc](https://git.genenetwork.org/jgart/gnqc) currently disallows
line separators inside cells, thus such a case can't happen in files
deemed valid in that project.)

## Dependencies

Just tooling, so far:

- (GNU) make, bash for the scripts

- For normal builds: gcc or clang (but Clang builds a faster binary);
  by default `cc` is used, whatever this links to, set `export
  CC=clang` (or gcc) to choose the compiler before running make.

- For using the AFL fuzzing targets (`runafl`, `runaflnosan`):
  [AFL++](https://aflplus.plus/) (`afl++-clang` in Debian).

- For using the coverage related targets (`runaflcov`,
  `utf-8-lineseparator.cov`): LLVM tooling (e.g. `llvm-11` in Debian
  Bullseye)

- For the `checkgdb` target: `gdbrun` from
  [chj-bin](https://github.com/pflanze/chj-bin) (could be simplified
  or replaced, I'm just used to it).

- For the `make expand` target: clang-format (package of the same name
  in Debian).

## Build

The production executable is `utf-8-lineseparator`, built via `make`.

Have a look at the [Makefile](Makefile) for the variables that can be
set, notable `CC` (gcc and clang are known to work).

For some testing, run `make check`.

Proper extensive testing is done via `make runafl`. More documentation
has to be written about this; generated test cases from AFL should be
added to the test suite run by `make check` (todo).

## More information

* [Approach](docs/approach.md) to see how C is used usefully
* [Design](docs/design.md) to help into coding on the project
* [TODO](docs/TODO.md) for open issues

