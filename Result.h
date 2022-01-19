/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef RESULT_H_
#define RESULT_H_

#include "String.h"


#define DEFTYPE_Result_(T)                      \
    typedef struct {                            \
        String failure;                         \
        T ok;                                   \
    } Result_##T;

#define Error(T, string)                        \
    (Result_##T) { string, default_##T }
#define Ok(T)                                                   \
    (Result_##T) { noString, (T)
#define ENDOk }

#define Result_is_success(v) (!((v).failure.str))
#define Result_is_failure(v) (!!((v).failure.str))
// #define Result_failure_str(v) ((v).failure.str)

// We don't release the .ok part here as that one may have changed
// ownership in the mean time!
#define Result_release(v)                       \
    String_release((v).failure)
#define Result_print_failure(fmt, v)            \
    fprintf(stderr, fmt, (v).failure.str)

#define PROPAGATE_Result(T, r)                                          \
    if (Result_is_failure(r)) {                                         \
        /* (r).failure.needs_freeing = false;                           \
           after the next line, but usually deallocated anyway */       \
        return (Result_##T) { (r).failure, default_##T };               \
    }

/*

  Label based cleanup handling:

    Result_Sometype foo() {
        BEGINRETURN(Result_Sometype);
        ...
        if (bar) RETURNL(l1, val1);
        Result_foo x = givingx();
        PROPAGATEL_Result(l2, Sometype, x);
        RETURN(val2);
     l2:
        Result_release(x);
     l1:
        cleanup1();
        ENDRETURN;
    }
  
*/
#define BEGINRETURN(T)                          \
    T __return;
#define RETURNL(label, val)                      \
    __return = val;                              \
    goto label;
#define RETURN(val)                             \
    __return = val;
#define ENDRETURN                               \
    return __return;

#define PROPAGATEL_Result(label, T, r)                                  \
    if (Result_is_failure(r)) {                                         \
        __return = (Result_##T) { (r).failure, default_##T };           \
        (r).failure.needs_freeing = false;                              \
        goto label;                                                     \
    }


#endif /* RESULT_H_ */
