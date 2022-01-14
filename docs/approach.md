# Approach

## Language choice

The aim of this project is for `utf-8-lineseparator` and possibly the
remaining code in the [gnqc](https://git.genenetwork.org/jgart/gnqc)
project to become fast (fast enough for checking in real time as a
file is being uploaded even over a relatively fast connection like 100
mbps), and to be correct and error free, as well as to be secure
(there's already a high attack surface via running untrusted code in
Linux containers on the server, so the required security is a bit
lower than it might otherwise be, but still there should be no
security hole if possible). Also, it might prove beneficial to run the
checker on the client instead of the server, and WASM can enable the
desired performance there, thus compilation to WASM might be useful.

The most suitable langugage for such a task seems to be Rust, its
features allow for a level of performance (equal to C or C++ code
that's written in a performance-conscious way) that just about allows
to match the 100 mbps uload speed target, while being secure by
default. The drawback of Rust is that it's a complex language that
requires time to learn. This means that finishing reimplementing
[gnqc](https://git.genenetwork.org/jgart/gnqc) in Rust would be months
down the road.

Alternatives are C, C++, Zig, and possibly some garbage collected
languages (D, Nim). (Languages like OCaml or Common Lisp do not allow
the kind of memory optimization that the former languages do, may be
too slow for that reason, and are less suited to run in the browser.)

[gnqc](https://git.genenetwork.org/jgart/gnqc) is not actually complex
from a memory handling perspective, there's no need for a garbage
collector. C and C++ are well-known, and C has the advantage of being
a simple language, and after macro expansion (via e.g. `gcc -E`) the
code one sees is the code that the machine executes, which allows to
inspect higher level abstractions instead of having to learn them well
(which is what one would have to do in C++).

So, using C has the advantage that it's a language that many people
already know, and is otherwise the easiest of (C, C++, Rust) to learn
and inspect should that be needed, and still it allows to program in a
way similar to Rust, which means it can serve as a learning step
towards Rust, should the organisation decide to move towards that
language in the future. More about how this project aims for
similarity with Rust see in "Similarity with Rust" at the end of this
page.

What C does not offer is inherent memory safety (C++ and Zig don't
fully offer it either (C++ has pointer wrappers to make heap
allocations safe but that has a large overhead over stack
allocation)), which is bad from a security perspective. To make
security problems unlikely, this project takes the following
approaches:

  - coding style:

      - avoid using heap allocations and pointers when possible
        (allocate structs on the stack and pass and return them by
        value; modern compilers optimize this usage well)
      - have a clear ownership/borrowing approach (like in Rust, but
        not compiler-verified; this should make it easy to learn the
        principles and thus be an investment towards learning Rust),
        via:
          1. API documentation
          2. tracking of heap allocations via `needs_freeing` flags
             (todo: rename to `owned`?)
      - not using unsafe type casting, instead:
          - using macros to generate parameterized types for
            containers (e.g. `Maybe` and `Result`)
          - using type tags with unions and wrappers around them that
            prevent invalid use
      - limit the use of arrays to library functionality, or static
        sizes
      - use `assert` to encode any kind of assumption taken by the
        programmer while coding
      - separate concerns into individually testable code (functional
        style where possible) and write tests for all of them (see
        [`test.c`](../test.c), todo: use a unit testing library,
        probably [chj-ctest](https://github.com/pflanze/chj-ctest))

  - verification: to catch any issues that coding style didn't prevent
    (because it wasn't being followed accidentally, or because it was
    library work that can't follow it)

      - use the sanitizers in gcc and clang (address sanitizer,
        undefined behaviour sanitizers) when running tests and AFL++
        (see next point)
      - use [AFL++](https://aflplus.plus/) to try to find any issues
        (done: `runafl` make target and scripts like
        [`aflcollect`](../bin/aflcollect); todo: automate iterative
        re-tries and merging of AFL++'s generated tests, write-up of a
        how-to, implement testing of individual pieces like unit
        testing does (to allow for full coverage even as app
        complexity grows))
      - use coverage tooling to verify that there's no untested code
        left after running the test suite and AFL++ testing (mostly
        done, todo: how-to write-up)
      - add the test cases that AFL++ found to the test suite (making
        the critical cases fast to run during `make runtests`, and
        retaining cases that AFL++ might not find again on the fly)
        (todo)

  - runtime protection in production, only as a last resort
    (verification should already catch all the issues):

      - use those sanitizers which are low overhead and safe to use in
        production code (todo; ASAN is said to not be safe for such
        use, UBSAN is)
      - use the other security enhancement features offered by the
        compilers and the kernel (stack protector,
        [ASLR](https://en.wikipedia.org/wiki/Address_space_layout_randomization),
        and more, todo (just compiler flags, perhaps manual or
        automatic verification that they are active))

This, together with instructions for new developers on how to work on
this code base (todo: write instruction/checklist page), should pretty
much eliminate the risk for security holes due to memory safety
issues. Also, a nice side benefit of the AFL++ based testing will be
catching bugs in general, not just memory related ones.

(Further security layers, like maybe lighttpd's chroot support, could
be used if there's still doubt about the security aspect. Or use of
Linux's [seccomp](https://en.wikipedia.org/wiki/Seccomp) could be
integrated.)

## Speed

utf-8-lineseparator (compiled with Clang, which is a bit faster than
compiled with gcc) runs about 50 times faster than
[python-crlf](https://github.com/pflanze/python-crlf.git), its Python
precursor. A similar level of performance improvement can be expected
from porting the rest in
[gnqc](https://git.genenetwork.org/jgart/gnqc). This matches the
expected level of performance needed for real-time handling at 100
mbps upload speeds.

## Future work

The following could be implemented in steps:

 1. Auto-convert non-LF line endings to LF ones instead of just
    reporting them.
 1. Auto-detect and -convert non-UTF-8 charset encodings to UTF-8
    instead of just reporting them.
 1. Port the checks in [gnqc](https://git.genenetwork.org/jgart/gnqc)
      - CSV parsing probably via
        [libcsv](https://github.com/rgamble/libcsv) ([in
        Debian](https://packages.debian.org/bullseye/libcsv3))
      - implement as a CGI (or
        [SCGI](https://en.wikipedia.org/wiki/Simple_Common_Gateway_Interface),
        or FastCGI) handler, behind lighttpd, Apache or Nginx (CGI
        would be preferable since it's the simplest and the startup
        overhead is negligible in this case, but Nginx doesn't support
        it)
      - give the check results as JSON (which is probably what's
        needed for real-time use, both when running inside the browser
        via WASM, or when read from the server via JavaScript)

## Similarities with Rust

This project consciously tries to do make some things somewhat more
similar to how they are done in Rust (both because they seem to be
good design decisions, and because it may ease the learning process
towards Rust, or maybe even a potential future porting of code to
Rust):

  - function and variable names start with a lowercase initial (there
    are currently some exceptions, todo clean up?), type names which
    are not C built-ins with an uppercase initial. I.e. any identifier
    that starts with an uppercase initial (but is not all-caps, which
    is reserved for macros, and is not part of the macro
    infrastructure for a type like Result(1)), can be assumed to be a
    compound type (struct type).
    
  - C is already a bit like Rust in that it only uses return values to
    indicate errors, and abort for abnormal exits (called panics in
    Rust). C traditionally uses small integer values as error codes
    (or overloads integer return values to mean successful values when
    positive and errors when negative, but then usually encoding the
    particular error value in `errno`), which is pretty ugly, error
    prone and complicated/inflexible. Rust uses the `Result` type, and
    this project emulates Rust by defining its own parameterized
    `Result` type, too (although it simplifies by just providing
    strings for the error case). Also like Rust, it tries to make
    error propagation simpler: Rust uses the `&` character to signify
    propagation, this project uses the `PROPAGATE*` macros for the
    same. Sure, it's way uglier but the best we can do in C.

  - as mentioned earlier, we avoid allocating things on the heap,
    instead opting to pass by copy (compilers optimize this so that
    often/usually no copy has to be done); which is also how Rust
    works when passing ownership. But we do pass pointers in cases
    where a struct type has to be borrowed (at least when doing so
    mutably); the type we have to write is `const Foo *p` when in Rust
    it's `p: &Foo`, and `Foo *p` when in Rust it's `p: &mut Foo`. In
    both languages we write `&p` to pass a variable from a caller to
    such a function. (Currently the project also uses comments with
    annotations like `/* owned */` in non-pointer cases to make it
    clear the struct changes ownership; the rules to use for this are
    to be done, todo.)

(1) For more details see [Design](design.md).
