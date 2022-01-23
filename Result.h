/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef RESULT_H_
#define RESULT_H_

#include "String.h"
#include "macro-util.h"


#define Result(T) XCAT(Result_,T)

#define DEFTYPE_Result(T)                       \
    typedef struct {                            \
        bool is_err;                            \
        union {                                 \
            String err;                         \
            T ok;                               \
        };                                      \
    } Result(T)

#define Err(T, string)                          \
    (Result(T)) { .is_err = true, .err = string }
#define Ok(T, val)                              \
    (Result(T)) { .is_err = false, .ok = val }

#define Result_is_Ok(v) (!((v).is_err))
#define Result_is_Err(v) ((v).is_err)
// #define Result_failure_str(v) ((v).err.str)

// We don't release the .ok part here as that one may have changed
// ownership in the mean time!
#define Result_release(v)                       \
    if (Result_is_Err(v)) String_release((v).err)
#define Result_print_failure(fmt, v)            \
    fprintf(stderr, fmt, (v).err.str)

#define PROPAGATE_return(T, r)                                          \
    if (Result_is_Err(r)) {                                             \
        /* (r).err.needs_freeing = false;                               \
           after the next line, but usually deallocated anyway */       \
        return Err(T, (r).err);                                         \
    }


/*

  Label based cleanup handling:

    Result_Sometype foo() {
        BEGIN_PROPAGATE(Sometype);
        ...
        if (bar) RETURN_goto(l1, val1);
        Result_foo x = givingx();
        PROPAGATE_goto(l2, Sometype, x);
        RETURN(Ok(Sometype, val2));
     l2:
        Result_release(x);
     l1:
        cleanup1();
        END_PROPAGATE;
    }
  
*/

#define BEGIN_PROPAGATE(T)                      \
    Result(T) __return;

#define RETURN_goto(label, val)                 \
    __return = val;                             \
    goto label;

#define RETURN(val)                             \
    __return = val;

#define END_PROPAGATE                           \
    return __return;

#define PROPAGATE_goto(label, T, r)                                     \
    if (Result_is_Err(r)) {                                             \
        __return = Err(T, (r).err);                                     \
        (r).err.needs_freeing = false;                                  \
        goto label;                                                     \
    }


/*

  Block based cleanup handling:


    BEGIN_PROPAGATE(Bar);

    if_Ok(Bar, result) {
        ....
        RETURN(Ok(Bar, { }));
    }
    // other cleanup than the result

    END_PROPAGATE;

  Or

    BEGIN_PROPAGATE(Bar);

    if_let_Ok(Bar, var, expr) {
        ....
        RETURN(Ok(Bar, { }));
    }
    // other cleanup than the result

    END_PROPAGATE;

*/

#define if_Ok(T, expr)                                  \
    __typeof__(expr) __if_Ok_result = expr;             \
    if (Result_is_Err(__if_Ok_result)) {                \
        RETURN(Err(T, __if_Ok_result.err));             \
        Result_release(__if_Ok_result);                 \
    } else 

// ^ XX Bummer: the whole if_Ok should automatically be wrapped in a
// new block, but can't (or does the MACRO{ } trick work here? TODO)

/*
#define if_let_Ok(T, var, expr)                         \
    __typeof__(expr) __if_Ok_result = expr;             \
    if (Result_is_Err(__if_Ok_result)) {                \
        RETURN(Err(T, __if_Ok_result.err));             \
        Result_release(__if_Ok_result);                 \
    } else {                                            \
    __typeof__(__if_Ok_result.ok) var =                 \
        __if_Ok_result.ok;                              \

The problem is that we'd need a ENDif_let_Ok to close off
the opening brace.

So...:
*/

#define if_let_Ok(T, var, expr)                         \
    __typeof__(expr) __if_Ok_result = expr;             \
    __typeof__(__if_Ok_result.ok) var =                 \
        __if_Ok_result.ok;                              \
    if (Result_is_Err(__if_Ok_result)) {                \
        RETURN(Err(T, __if_Ok_result.err));             \
        Result_release(__if_Ok_result);                 \
    } else

// XX Bummer.


#endif /* RESULT_H_ */
