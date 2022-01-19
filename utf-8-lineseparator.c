/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 202112L
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "leakcheck.h"

#include "String.h"
#include "util.h"
#include "env.h"
#include "BufferedStream.h"
#include "unicode.h"



static
int report(BufferedStream* in /* borrowed */) {
    int64_t charcount = 0;
    int64_t LFcount = 0;
    int64_t CRcount = 0;
    int64_t CRLFcount = 0;
    int64_t column = 0;
    bool last_was_CR = false;
    while (1) {
        Result_Maybe_u32 c = get_unicodechar(in);
        if (Result_is_failure(c)) {
            int64_t linecount = LFcount + CRcount + CRLFcount;
            const char *questionable =
                (linecount == MAX3(LFcount, CRcount, CRLFcount)) ? "false" : "true";
            printf("{ \"type\": \"utf-8-failure\", \"failure\": \"%s\", \"character_position\": %li, \"line\": %li, \"column\": %li, \"line_questionable\": %s }\n",
                   c.failure.str, // XXX Needs to be converted to json string
                   charcount + 1,
                   linecount + 1,
                   column + 1,
                   questionable);
            Result_release(c);
            return 0;
        }
        if (c.ok.is_nothing) {
            break;
        }
        charcount++;
        if (c.ok.value == '\r') {
            if (last_was_CR) {
                CRcount++;
            }
            // the new one will be counted in the next iteration
            last_was_CR = true;
            column = 0;
        } else if (c.ok.value == '\n') {
            if (last_was_CR) {
                CRLFcount++;
            } else {
                LFcount++;
            }
            last_was_CR = false;
            column = 0;
        } else {
            if (last_was_CR) {
                CRcount++;
            }
            last_was_CR = false;
            column++;
        }
    }
    if (last_was_CR) {
        CRcount++;
    }
    printf("{ \"type\": \"linecount\", \"charcount\": %li, \"LFcount\": %li, \"CRcount\": %li, \"CRLFcount\": %li }\n",
           charcount, LFcount, CRcount, CRLFcount);
    return 0;
}


#if AFL
// See
// https://github.com/AFLplusplus/AFLplusplus/blob/stable/utils/persistent_mode/persistent_demo_new.c
__AFL_FUZZ_INIT();
# pragma clang optimize off
// # pragma GCC   optimize("O0")
#endif

// XX configurable?
#define MONKEY_INIT_LEN 0

int main(int argc, const char**argv) {
#if AFL
    if (env("AFL")) {
        // Execution for AFL fuzz testing
        __AFL_INIT();
        unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
        while (__AFL_LOOP(1000000)) {
            ssize_t len = __AFL_FUZZ_TESTCASE_LEN;
            if (len < MONKEY_INIT_LEN) continue;
            monkey_init(buf, MONKEY_INIT_LEN);
            unsigned char *bufrest = buf + MONKEY_INIT_LEN;

            BufferedStream in = Buffer_to_BufferedStream(
                Buffer_from_array(false,
                                  bufrest,
                                  len),
                STREAM_DIRECTION_IN,
                literal_String("AFL buffer"));

            int res = report(&in);
            fprintf(stderr, "report returned with exit code %i\n", res);
            BufferedStream_close(&in);
            BufferedStream_release(&in);

            monkey_release();

            leakcheck_verify(true);
        }
        return 0;
    } else {
#endif
        // Normal execution
        if (argc == 1) {
            BufferedStream in =
                fd_BufferedStream(0,
                                  STREAM_DIRECTION_IN,
                                  literal_String("STDIN"),
                                  false);
            int res = report(&in);
            Result_Unit r = BufferedStream_close(&in);
            if (Result_is_failure(r)) {
                // XX should this have the path in the message,
                // already? Should there be a
                // BufferedStream_error_message method?
                fprintf(stderr, "close: %s", r.failure.str);
                res = 1; // OK?
            }
            Result_release(r);
            BufferedStream_release(&in);

            leakcheck_verify(false);
            return res;
        } else if (argc == 2) {
            const char *path = argv[1];
            Result_BufferedStream r_in =
                open_r_BufferedStream(borrowing_String(path));
            if (Result_is_failure(r_in)) {
                // XX should this have the path in the message,
                // already? Should there be a
                // BufferedStream_error_message method?
                fprintf(stderr, "open: %s", r_in.failure.str);
                Result_release(r_in);
                leakcheck_verify(false);
                return 1;
            }

            int res = report(&r_in.ok);
            Result_Unit r = BufferedStream_close(&r_in.ok);
            if (Result_is_failure(r)) {
                fprintf(stderr, "close: %s", r.failure.str);
                res = 1; // OK?
            }
            Result_release(r);
            BufferedStream_release(&r_in.ok);
            Result_release(r_in);

            leakcheck_verify(false);
            return res;
        } else {
            fprintf(stderr,
                    "Usage: %s [file]\n"
                    "  Verify proper UTF-8 encoding and report usage of CR and LF\n"
                    "  characters in <file> if given, otherwise of STDIN.\n",
                    argv[0]);
            leakcheck_verify(false);
            return 1;
        }
#if AFL
    }
#endif
}
