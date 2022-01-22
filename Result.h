/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef RESULT_H_
#define RESULT_H_

#include "String.h"


#define Result(T) Result_##T

#define DEFTYPE_Result(T)                       \
    typedef struct {                            \
        String failure;                         \
        T ok;                                   \
    } Result(T)

#define Error(T, string)                        \
    (Result(T)) { string, default_##T }
#define Ok(T, val)                              \
    (Result(T)) { noString, val }

#define Result_is_Ok(v) (!((v).failure.str))
#define Result_is_Err(v) (!!((v).failure.str))
// #define Result_failure_str(v) ((v).failure.str)

// We don't release the .ok part here as that one may have changed
// ownership in the mean time!
#define Result_release(v)                       \
    String_release((v).failure)
#define Result_print_failure(fmt, v)            \
    fprintf(stderr, fmt, (v).failure.str)

#define PROPAGATE_Result(T, r)                                          \
    if (Result_is_Err(r)) {                                         \
        /* (r).failure.needs_freeing = false;                           \
           after the next line, but usually deallocated anyway */       \
        return (Result(T)) { (r).failure, default_##T };                \
    }

/*

  Label based cleanup handling:

    Result_Sometype foo() {
        BEGIN_PROPAGATE(Sometype);
        ...
        if (bar) RETURNL(l1, val1);
        Result_foo x = givingx();
        PROPAGATEL_Result(l2, Sometype, x);
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

#define RETURNL(label, val)                      \
    __return = val;                              \
    goto label;

#define RETURN(val)                             \
    __return = val;

#define END_PROPAGATE                               \
    return __return;

#define PROPAGATEL_Result(label, T, r)                                  \
    if (Result_is_Err(r)) {                                         \
        __return = (Result(T)) { (r).failure, default_##T };            \
        (r).failure.needs_freeing = false;                              \
        goto label;                                                     \
    }


#endif /* RESULT_H_ */
