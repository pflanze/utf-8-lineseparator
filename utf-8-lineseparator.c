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

#include "result.h"
#include "maybe.h"
#include "shorttypenames.h" /* u8 u32 */
#include "string.h"
#include "util.h"
#include "mem.h"
#include "env.h"
#include "buffer.h"



DEFTYPE_Maybe_(u32);
DEFTYPE_Result_(Maybe_u32);

// XXX TODO: handle Byte order mark?

static
Result_Maybe_u32 get_unicodechar(BufferedStream *in) {
    // https://en.wikipedia.org/wiki/Utf-8#Encoding
#define EBUFSIZ 256
    u32 codepoint;
    Result_Maybe_u8 b1 = bufferedstream_getc(in);
    PROPAGATE_Result(Maybe_u32, b1);
    if (b1.ok.is_nothing) {
        return Ok(Maybe_u32) Nothing(u32) ENDOk;
    }
    codepoint = b1.ok.value;
    if ((b1.ok.value & 128) == 0) {
        // 1 byte
    } else {
        int numbytes;
        if        ((b1.ok.value & 0b11100000) == 0b11000000) {
            numbytes = 2;
        } else if ((b1.ok.value & 0b11110000) == 0b11100000) {
            numbytes = 3;
        } else if ((b1.ok.value & 0b11111000) == 0b11110000) {
            numbytes = 4;
        } else {
            return Error(
                Maybe_u32, false,
                "invalid start byte decoding UTF-8");
        }
        for (int i = 1; i < numbytes; i++) {
            Result_Maybe_u8 b = bufferedstream_getc(in);
            PROPAGATE_Result(Maybe_u32, b);
            if (b.ok.is_nothing) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "premature EOF decoding UTF-8 (byte #%i)",
                         i+1);
                return Error(Maybe_u32, true, xstrdup(msg));
            }
            if ((b.ok.value & 0b11000000) != 0b10000000) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "invalid continuation byte decoding UTF-8 (byte #%i)",
                         i+1);
                return Error(Maybe_u32, true, xstrdup(msg));
            }
            codepoint |= ((b.ok.value & 0b00111111) << (i * 6 + 1));
        }
    }
    if (codepoint <= 0x10FFFF) {
        return Ok(Maybe_u32) Just(u32) codepoint ENDJust ENDOk;
    } else {
        char msg[EBUFSIZ];
        snprintf(msg, EBUFSIZ,
                 "invalid unicode codepoint (%ui)",
                 codepoint);
        return Error(Maybe_u32, true, xstrdup(msg));
    }
#undef EBUFSIZ
}

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
        if (result_is_failure(c)) {
            int64_t linecount = LFcount + CRcount + CRLFcount;
            const char *questionable =
                (linecount == MAX3(LFcount, CRcount, CRLFcount)) ? "false" : "true";
            printf("{ \"type\": \"utf-8-failure\", \"failure\": \"%s\", \"character_position\": %li, \"line\": %li, \"column\": %li, \"line_questionable\": %s }\n",
                   c.failure.str, // XXX Needs to be converted to json string
                   charcount + 1,
                   linecount + 1,
                   column + 1,
                   questionable);
            result_release(c);
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


#if FUZZ
// See
// https://github.com/AFLplusplus/AFLplusplus/blob/stable/utils/persistent_mode/persistent_demo_new.c
__AFL_FUZZ_INIT();
# pragma clang optimize off
// # pragma GCC   optimize("O0")
#endif

int main(int argc, const char**argv) {
#if FUZZ
    if (env("FUZZ")) {
        // Execution for AFL fuzz testing
        __AFL_INIT();
        unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
        while (__AFL_LOOP(1000)) {
            ssize_t len = __AFL_FUZZ_TESTCASE_LEN;

            BufferedStream in = buffer_to_BufferedStream(
                (Buffer) { .length = len, .readpos = 0, .size = len,
                    .array = buf, .needs_freeing = false },
                (String) { false, "AFL buffer" });

            int res = report(&in);
            fprintf(stderr, "report returned with exit code %i\n", res);
            bufferedstream_release(&in);
        }
        return 0;
    } else {
#endif
        // Normal execution
        if (argc == 1) {
            BufferedStream in =
                fd_BufferedStream(0,
                                  STREAM_DIRECTION_IN,
                                  (String) { false, "STDIN" },
                                  false);
            int res = report(&in);
            Result_Unit r = bufferedstream_close(&in);
            if (result_is_failure(r)) {
                // XX should this have the path in the message,
                // already? Should there be a
                // bufferedstream_error_message method?
                fprintf(stderr, "close: %s", r.failure.str);
                res = 1; // OK?
            }
            result_release(r);
            bufferedstream_release(&in);

            return res;
        } else if (argc == 2) {
            const char *path = argv[1];
            Result_BufferedStream r_in =
                open_BufferedStream((String) { false, path },
                                    O_RDONLY);
            if (result_is_failure(r_in)) {
                // XX should this have the path in the message,
                // already? Should there be a
                // bufferedstream_error_message method?
                fprintf(stderr, "open: %s", r_in.failure.str);
                result_release(r_in);
                return 1;
            }

            int res = report(&r_in.ok);
            Result_Unit r = bufferedstream_close(&r_in.ok);
            if (result_is_failure(r)) {
                fprintf(stderr, "close: %s", r.failure.str);
                res = 1; // OK?
            }
            result_release(r);
            bufferedstream_release(&r_in.ok);
            result_release(r_in);
            // (XX should result_release magically call release on .ok, too?)

            return res;
        } else {
            fprintf(stderr,
                    "Usage: %s [file]\n"
                    "  Verify proper UTF-8 encoding and report usage of CR and LF\n"
                    "  characters in <file> if given, otherwise of STDIN.\n",
                    argv[0]);
            return 1;
        }
#if FUZZ
    }
#endif
}
