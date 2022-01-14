# Design

Also see [Approach](approach.md).

## Abstractions

### Type-parameterized containers

Today's languages with static type systems usually have parameterized
types. E.g. representing an optional value (in function arguments or
return types or as part of other data structures) in various
languages:

  - Python: `Optional[int]`, `Optional[str]`
  - Haskell: `Maybe Int`, `Maybe [Char]`
  - C++: `optional<int>`, `optional<string>`

C does not have that feature. There are two standard approaches for
such containers, one is to use a single type definition with a pointer
of unknown type and then using unsafe type casting:

    struct Maybe {
        void *maybe_value;
        // (no need for separate switch to indicate whether a 
        // value is contained, as the pointer can be NULL.) 
    };

    int main() {
        int *myvalp = malloc(sizeof(int));
        *myvalp = 42;
        struct Maybe mf = { (void *)&myvalp };
        int *myvalpalias = (int *)mf.maybe_value;

        const char *mystr = "foo";
        struct Maybe ms = { (void *)mystr };
        const char *mystralias = (const char *)ms.maybe_value;
    }

The now often required heap allocations are slow and the handling of
the pointers and the unsafe casts are sources of memory safety bugs.

The other is defining the container for every contained type used.

    struct Maybe_int {
        bool is_nothing;
        int value;
    };

    struct Maybe_str {
        bool is_nothing;
        const char *value;
    };

    int main() {
        int myval = 42;
        struct Maybe_int mf = { false, myval };
        int myvalcopy = mf.value;

        const char *mystr = "foo";
        struct Maybe_str ms = { false, mystr };
        const char *mystralias = ms.value;
    }

This is type and memory safe, but redefining the container type
manually is getting tedious fast. But C has macros, which this project
is taking advantage of (the include here is referring to
[maybe.h](../maybe.h)):

    #include "maybe.h"

    DEFTYPE_Maybe_(int);
    typedef const char* str;
    DEFTYPE_Maybe_(str);
    
    int main() {
        int myval = 42;
        // Maybe_int mf = { false, myval };
        //  -- or, hiding the implementation: --
        Maybe_int mf = Just(int) myval ENDJust;
        int myvalcopy = mf.value;

        const char *mystr = "foo";
        Maybe_str ms = Just(str) mystr ENDJust;
        const char *mystralias = ms.value;
    }

When it seems unclear what code the macros generate, compile via the
compiler's `-E` flag (todo: generate expanded files as part of the
build by default).

Since the `DEFTYPE_Maybe_` macro creates a new type name by simply
appending the name of the type given as its argument to
`Maybe_`, the argument must not contain anything other than word
characters (`\w+`). For this reason, `typedef` is consistently used to
create type names that do not contain spaces or other punctuation,
e.g. generating the punctuation-less type name `str` above, but also
why `DEFTYPE_Maybe_` defines `Maybe_int` as a typedef so that the
`struct ` prefix isn't needed, so that the `Maybe_int` type itself be
used as a parameter for another DEFTYPE style macro.

The Maybe types need to be checked via their `.is_nothing` attribute
whether they actually contain a value.

### Result

The file [result.h](../result.h)) defines the parametrized Result
type. A Result can contain a value similar to a Maybe, but instead of
containing nothing in the alternate case, they contain an
error. Currently this is always simply a string; i.e. whenever a
function can return an error, it returns a type derived via
`DEFTYPE_Result_`, and if an error is to be returned at runtime, it is
a string describing the error condition.

Note that C does not have exceptions (except for `longjmp` on some
systems but that shouldn't/can't be used as it prevents cleanup
actions). Returning values is the only way execution can leave a
function without terminating the program. (`exit` (`man 3 exit` and
`man 2 exit`) will exit a program cleanly without returing from the
calling function, and `abort`, `assert` (`man 3 ...`), as well as the
macros `DIE` and `DIE_` from [util.h](../util.h), will exit via
program termination, basically like uncaught exceptions.)

#### String

Standard C strings are simply pointers to an array of 8-bit characters
(bytes), with a 0-byte at the end indicating the end of the
string. They can be allocated from the heap, as constants (indefinite
life time), or on the stack with a dynamically scoped life time. In
the first case they have to be `free`d, not so in the latter two
cases. Since a receiver of a string doesn't necessarily know
statically which case it is dealing with, the strings would either
*always* have to be heap-allocated (imposing overhead), or the
knowledge about which case it is dealing with has to be included at
runtime when passing that string.

The type `String` from [string.h](../string.h) is used for the latter approach.
The attribute `needs_freeing` embodies the knowledge about which case it
is representing. The `string_release(String s)` function releases the
memory associated with the string, *iff* needed (i.e. `needs_freeing`
is true). A `String` is created by simply passing the boolean along
with the pointer to the array. This would create a `String`
representing a string constant:

    (String) { false, "some constant string" }

There's a macro to write this in a shorter, more expressive way:

    String_literal("some constant string")

#### Ownership handling, work in progress

The `needs_freeing` approach is also currently being (mis)used for
ownership handling:

If a function foo expects a `String` as an argument and assumes that
it is becoming the owner of that string (implying it will have to
release it when done with it), but the caller of foo wants to continue
using the string afterwards, it can simply pass a new `String` that
sets the `needs_freeing` field to `false`. When foo calls
`string_release`, it will then not actually free it and the caller can
continue to use it.

(This is not a very solid approach (as foo might assume that it can
use the string for longer than its caller allows) and more thinking
should go into it. It works for the time being, and the sanitizers
together with the extensive tests will catch any issues, but possibly
a better approach should be used. This is part of the learning curve
towards what Rust is doing.)

#### Error propagation

When a function is returning an error via Result, the macros
`PROPAGATE_Result` or `PROPAGATEL_Result` can be used to propagate it
up the call chain; meaning, to stop the evaluation of the current
function and return the error from the current function.

`PROPAGATE_Result` can be used if the function does not need to clean
up anything. OTOH if there are things that need to be cleaned up
(e.g. calling some `.._release` functions), then those cleanup actions
should be put at the end of the function (in the reverse order of the
allocations of the things that need to be cleaned up), with labels,
and then the label is passed as the first argument to
`PROPAGATEL_Result`. An example can be seen in [test.c](../test.c) in
`buf_to_utf8_codepoint`.

Similarly, in C it is not possible to directly `return` from a
function when a cleanup action has to be done; to ease this case, the
`RETURNL`, `RETURN` macros are provided; to make them work,
`BEGINRETURN` has to precede their use and defines the type to return,
and `ENDRETURN` needs to put at the end of the function to actually
`return` the value that `RETURNL` or `RETURN` have stored for the
return.

## Leakcheck

For precise checking of memory handling errors, the address sanitizer
(ASAN) does a very good job. The drawback is that it shouldn't be used
in production code, only during development and testing.

Also, ASAN doesn't run while AFL++ does its fuzz testing, since the
program doesn't actually exit.

[leakcheck.h](../leakcheck.h) provides a simple and cheap means of
testing for missed memory deallocations both in production and as part
of AFL++ testing. The header file should be included after system
headers but before any headers that should be tested for memory
leaks. It redefines `malloc` and `free` as well as a few others like
`strdup` to count allocations and deallocations; `leakcheck_verify`
should be called at the end of the program to verify that they
have balanced each other out.

## Monkey testing

[Monkey testing](https://en.wikipedia.org/wiki/Monkey_testing) means
to inject artificial failures (or bogus inputs) into IO operations, to
verify that the program handles them gracefully. `make runafl` already
passes it bogus data, but no IO failures yet. [monkey.h](../monkey.h)
provides a way to inject the failures, by siphoning off some of the
fuzzing input from AFL++.

Sadly, the siphoning quickly reduces the chance for AFL++ finding
problems in the other places. Maybe it's especially bad when some
region of inputs has no effect (as is the case when the part reserved
for IO calls isn't actually used up), or maybe it's just that
increased complexity (more code paths for AFL++ to cater to) quickly
leads to exploding search times, or rather, the genetic algorithm
getting stuck and not finding paths anymore (just restarting fresh
many times seems to yield more benefits than letting it run for a long
time). I still need to figure out the best approach to handle
this. Separating parts of the app will be the important approach, but
you'd still want to introduce IO failures in parallel to the main
input feed. Thus: todo.

