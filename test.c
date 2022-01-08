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


DEFTYPE_Result_(u32);

static
Result_u32 buf_to_utf8_codepoint(const unsigned char *inbuf,
                                 size_t inlen) {
    BufferedStream in = buffer_to_BufferedStream(
        (Buffer) {
            .length = inlen,
            .size = inlen,
            .readpos = 0,
            .array = (unsigned char*)inbuf,
            // ^ XX provide ConstBuffer instead?
            .needs_freeing = false
        },
        (String) {
            false,
            "buf"
        });
    Result_Maybe_u32 rmc = get_unicodechar(&in);
    PROPAGATE_Result(u32, rmc);
    if (rmc.ok.is_nothing) {
        result_release(rmc);
        return Error(u32, false, "premature EOF");
    }
    Result_Maybe_u32 rmc2 = get_unicodechar(&in);
    PROPAGATE_Result(u32, rmc2);
    if (! rmc2.ok.is_nothing) {
        result_release(rmc2);
        result_release(rmc);
        return Error(u32, false, "left-over data after character");
    }

    u32 res = rmc.ok.value;
    bufferedstream_close(&in);
    bufferedstream_release(&in);
    return Ok(u32) res ENDOk;
}

static
void t_utf8_equal_codepoint(u32 codepoint,
                            const unsigned char *buf,
                            size_t buflen,
                            int line,
                            int *successes,
                            int *failures,
                            int *errors) {
    Result_u32 rc =
        buf_to_utf8_codepoint(buf, buflen);
    if (result_is_failure(rc)) {
        WARN_("*** Error running test on line %i: %s",
              line,
              rc.failure.str);
        (*errors)++;
    } else {
        if (rc.ok == codepoint) {
            (*successes)++;
        } else {
            WARN_("*** Test failed on line %i: expected %u, got %u",
                  line,
                  codepoint,
                  rc.ok);
            (*failures)++;
        }
    }
    result_release(rc);
}


#define T_UTF8_EQUAL_CODEPOINT(codepoint, ...)                          \
    {                                                                   \
        const unsigned char buf[] = { __VA_ARGS__ };                    \
        t_utf8_equal_codepoint(codepoint, buf, sizeof(buf), __LINE__,   \
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

    T_UTF8_EQUAL_CODEPOINT(228, 0xc3, 0xa4);
    T_UTF8_EQUAL_CODEPOINT(0x10FFF0, 0xf4, 0x8f, 0xbf, 0xb0);

    printf("test results: %i success(es), %i failure(s), %i error(s)\n",
           successes, failures, errors);

    leakcheck_verify(false);
    int issues = failures + errors;
    return (issues ? 1 : 0);
}

