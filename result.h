/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef RESULT_H_
#define RESULT_H_

#include "string.h"


#define DEFTYPE_Result_(T)                      \
    typedef struct {                            \
        String failure;                         \
        T ok;                                   \
    } Result_##T;

#define Error(T, needs_freeing, str)                            \
    (Result_##T) { (String) { needs_freeing, str }, default_##T }
#define string_Error(T, string)                                 \
    (Result_##T) { string, default_##T }
#define Ok(T)                                                   \
    (Result_##T) { (String) { false, NULL }, (T)
#define ENDOk }

#define result_is_success(v) (!((v).failure.str))
#define result_is_failure(v) (!!((v).failure.str))
// #define result_failure_str(v) ((v).failure.str)
#define result_release(v)                       \
    string_release((v).failure)
#define result_print_failure(fmt, v)            \
    fprintf(stderr, fmt, (v).failure.str)

#define PROPAGATE_Result(T, r)                                  \
    if (result_is_failure(r)) {                                 \
        return (Result_##T) { (r).failure, default_##T };       \
    }


#endif /* RESULT_H_ */
