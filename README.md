# File checker tool and framework for safe and somewhat Rust-like coding in C

This provides a tool to efficiently check text (CSV) files for valid
UTF-8 use, and to report which line endings they use. It is about 50x
faster than a corresponding Python program. It is currently used in
[gnqc](https://git.genenetwork.org/jgart/gnqc), and could easily be
extended to take on more functionality.

But this project also provides a "framework" (tooling infrastructure
and libraries) for writing programs in a manner that is a bit more
like one would work Rust, and that uses verification tooling to try to
make it fully safe, primarily via [AFL++](https://aflplus.plus/). Note
that even a coverage-guided fuzzer doesn't guarantee to find all bugs
(and in fact in large enough software is pretty much guaranteed to not
do that), but, by limiting the scope of the program exposed to fuzzing
(currently the whole ofutf-8-lineseparator` is small enough to
qualify) by giving choosable access to subsets (like unit testing) and
verifying via coverage tooling that no part remains untested, we hope
to scale this up to larger programs.  The project also takes a number
of further approaches to minimize the potential issues. See our
[approach](docs/approach.md) for further details.

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

- For the `runtestsgdb` target: `gdbrun` from
  [chj-bin](https://github.com/pflanze/chj-bin) (could be simplified
  or replaced, I'm just used to it).

## Build

The production executable is `utf-8-lineseparator`, built via `make`.

Have a look at the [Makefile](Makefile) for the variables that can be
set, notable `CC` (gcc and clang are known to work).

For some testing, run `make runtests`.

Proper extensive testing is done via `make runafl`. More documentation
has to be written about this; generated test cases from AFL should be
added to the test suite run by `make runtests` (todo).

## More information

* [Approach](docs/approach.md) to see how C is used usefully
* [Design](docs/design.md) to help into coding on the project
* [TODO](docs/TODO.md) for open issues

