/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef ENV_H_
#define ENV_H_

#include <stdlib.h>
#include <stdbool.h>
#include "util.h"


UNUSED static
bool env(const char* name) {
    const char *val = getenv(name);
    if (val) {
        if (*val) {
            return (val[0] == '1') && (val[1] == 0);
        } else {
            return false;
        }
    } else {
        return false;
    }
}

#endif /* ENV_H_ */
