/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#define _POSIX_C_SOURCE 202112L

#include "leakcheck.h"

#include "unicode.h"
#include "bufferedstream.h"
#include "buffer.h"
#include "result.h"
#include "env.h"


DEFTYPE_Result_(u32);


static
Result_u32 buf_to_utf8_codepoint(const unsigned char *inbuf,
                                 size_t inlen) {
    BEGINRETURN(Result_u32);
    BufferedStream in = buffer_to_BufferedStream(
        (Buffer) {
            .length = inlen,
            .size = inlen,
            .readpos = 0,
            .array = (unsigned char*)inbuf,
            // ^ XX provide ConstBuffer instead?
            .needs_freeing = false
        },
        String_literal("buf"));
    Result_Maybe_u32 rmc = get_unicodechar(&in);
    PROPAGATEL_Result(_rmc, u32, rmc);
    if (rmc.ok.is_nothing) {
        RETURNL(_rmc, Error(u32, String_literal("premature EOF")));
    }
    Result_Maybe_u32 rmc2 = get_unicodechar(&in);
    PROPAGATEL_Result(_rmc2, u32, rmc2);
    if (! rmc2.ok.is_nothing) {
        RETURNL(_rmc2, Error(u32, String_literal(
                                 "left-over data after character")));
    }

    RETURN(Ok(u32) rmc.ok.value ENDOk);
_rmc2:
    result_release(rmc2);
_rmc:
    result_release(rmc);
    bufferedstream_close(&in);
    bufferedstream_release(&in);
    ENDRETURN;
}

static
void t_utf8_equal_codepoint(u32 codepoint,
                            const unsigned char *buf,
                            size_t buflen,
                            const char *sourcefile,
                            int sourceline,
                            int *successes,
                            int *failures,
                            int *errors) {
    Result_u32 rc =
        buf_to_utf8_codepoint(buf, buflen);
    if (result_is_failure(rc)) {
        WARN_("*** Error running test: %s at %s:%i",
              rc.failure.str,
              sourcefile,
              sourceline);
        (*errors)++;
    } else {
        if (rc.ok == codepoint) {
            (*successes)++;
        } else {
            WARN_("*** Test failed: expected %u, got %u (0x%x) at %s:%i",
                  codepoint,
                  rc.ok,
                  rc.ok,
                  sourcefile,
                  sourceline);
            (*failures)++;
        }
    }
    result_release(rc);
}


#define T_UTF8_EQUAL_CODEPOINT(codepoint, ...)                          \
    {                                                                   \
        const unsigned char buf[] = { __VA_ARGS__ };                    \
        t_utf8_equal_codepoint(codepoint, buf, sizeof(buf),             \
                               __FILE__, __LINE__,                      \
                               &successes, &failures, &errors);         \
    }


int main() {
    int successes = 0;
    int failures = 0;
    int errors = 0;

    /* failing tests to verify test program
    T_UTF8_EQUAL_CODEPOINT(123, 133);
    T_UTF8_EQUAL_CODEPOINT(123, 43,43,55);
    T_UTF8_EQUAL_CODEPOINT(123, 43);
    */

    T_UTF8_EQUAL_CODEPOINT(13, 0x0d);
    T_UTF8_EQUAL_CODEPOINT(228, 0xc3, 0xa4);
    T_UTF8_EQUAL_CODEPOINT(231, 0xc3, 0xa7);
    T_UTF8_EQUAL_CODEPOINT(0x20AC, 0xE2, 0x82, 0xAC);
    T_UTF8_EQUAL_CODEPOINT(0xD55C, 0xED, 0x95, 0x9C);
    T_UTF8_EQUAL_CODEPOINT(0x10348, 0xF0, 0x90, 0x8D, 0x88);
    if (! env("EXHAUSTIVE")) {
        T_UTF8_EQUAL_CODEPOINT(0x10FFF0, 0xf4, 0x8f, 0xbf, 0xb0);
    } else {
        // Just trying to crash it.
        // Missing 5th and 6th bytes (too costly).
        FOR_RANGE (i3, 0, 256) {
            WARN_("i3 = %i", i3);
            FOR_RANGE (i2, 0, 256) {
                FOR_RANGE (i1, 0, 256) {
                    FOR_RANGE (i0, 0, 256) {
                        const unsigned char buf[] = { i0, i1, i2, i3 };
                        Result_u32 rc = buf_to_utf8_codepoint(
                            buf, sizeof(buf));
                        // if (result_is_success(rc)) ...
                        result_release(rc);
                    }
                }
            }
        }
    }

    printf("test results: %i success(es), %i failure(s), %i error(s)\n",
           successes, failures, errors);

    leakcheck_verify(false);
    int issues = failures + errors;
    return (issues ? 1 : 0);
}

