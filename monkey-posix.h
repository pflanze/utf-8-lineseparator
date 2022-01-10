/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

/*
  Monkey'ed versions of POSIX routines.
 */

// Now unused, since using io layer instead.

#ifndef MONKEY_POSIX_H_
#define MONKEY_POSIX_H_


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "monkey.h"


DEF_MONKEYWRAPPERSTATE(monkey_open__2_state);

static
int monkey_open__2(const char *pathname, int flags) {
#if AFL
    if (monkey_open__2_state.waits--) {
#endif
        return open(pathname, flags);
#if AFL
    } else {
        monkeywrapperstate_refresh_both(&monkey_open__2_state);
        errno = monkey_open__2_state.other; // XX OK?
        return -1;
    }
#endif
}


#endif /* MONKEY_POSIX_H_ */
