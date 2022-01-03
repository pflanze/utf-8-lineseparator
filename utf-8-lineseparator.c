/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#undef _GNU_SOURCE
#define _POSIX_C_SOURCE 202112L
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

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

#define MAX2(a,b)                               \
    ((a) < (b) ? (b) : (a))

#define MAX3(a,b,c)                             \
    ((a) < (b) ? MAX2(b, c) : MAX2(a, c))

static
void die_outofmemory() {
    fprintf(stderr, "Out of memory, aborting\n");
    abort();
}

static
char *xstrdup(const char *str) {
    char *res= strdup(str);
    if (!res) die_outofmemory();
    return res;
}

static
int perror_str(const char *fmt, const char *arg1) {
#define EBUFSIZ 256
    char msg[EBUFSIZ];
    strerror_r(errno, msg, EBUFSIZ);
    int res = fprintf(stderr, fmt, arg1);
    if (res < 0) return res;
    return fprintf(stderr, ": %s\n", msg);
#undef EBUFSIZ
}


typedef struct {
    bool needs_freeing;
    const char* str;
} String;

static
void string_release(String s) {
    if (s.needs_freeing) {
        free((void*)s.str);
    }
}

static
int perror_string(const char *fmt, String str /* taking ownership */) {
    int res = perror_str(fmt, str.str);
    string_release(str);
    return res;
}

static
String string_quote_sh(const char *str) {
#define QBUFSIZ 1024
    char out[QBUFSIZ];
    int out_i = 0;
#define PUSH(c)                                     \
    if (out_i < (QBUFSIZ-3-1-1)) {                  \
        out[out_i++] = c;                           \
    } else {                                        \
        goto push_error;                            \
    }

    size_t len = strlen(str);
    PUSH('\'');
    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\'') {
            PUSH('\'');
            PUSH('\\');
            PUSH('\'');
            PUSH('\'');
        } else {
            PUSH(c);
        }
    }
    PUSH('\'');
    goto finish;
push_error:
    out[out_i++] = '\'';
    out[out_i++] = '.';
    out[out_i++] = '.';
    out[out_i++] = '.';
finish:
    out[out_i++] = '\0';
    return (String) { true, xstrdup(out) };
#undef QBUFSIZ
}


#define DEFTYPE_Result_(T)                      \
    typedef struct {                            \
        String failure;                         \
        T ok;                                   \
    } Result_##T;

#define Error(T, needs_freeing, str)                            \
    (Result_##T) { (String) { needs_freeing, str }, (T){} }
#define Ok(T)                                           \
    (Result_##T) { (String) { false, NULL }, (T)
#define ENDOk }

#define result_is_success(v) (!((v).failure.str))
#define result_is_failure(v) (!!((v).failure.str))
// #define result_failure_str(v) ((v).failure.str)
#define result_release(v)                       \
    string_release((v).failure)
#define result_print_failure(fmt, v)            \
    fprintf(stderr, fmt, (v).failure.str)

#define PROPAGATE_Result(T, r)                          \
    if (result_is_failure(r)) {                         \
        return (Result_##T) { (r).failure, (T){} };     \
    }



#define DEFTYPE_Maybe_(T)                       \
    typedef struct {                            \
        bool is_nothing;                        \
        T value;                                \
    } Maybe_##T;

#define Nothing(T)                              \
    (Maybe_##T) { true, default_##T }
#define Just(T)                                 \
    (Maybe_##T) { false,
#define ENDJust  }


typedef uint8_t u8;
#define default_u8 0
typedef uint32_t u32;
#define default_u32 0


DEFTYPE_Maybe_(u8);
DEFTYPE_Result_(Maybe_u8);

static
Result_Maybe_u8 getc_Result(FILE *in) {
    int c = getc(in);
    if (c == EOF) {
        if (errno) {
#define EBUFSIZ 256
            char msg[EBUFSIZ];
            strerror_r(errno, msg, EBUFSIZ);
            return Error(Maybe_u8, true, xstrdup(msg));
#undef EBUFSIZ
        }
        return Ok(Maybe_u8) Nothing(u8) ENDOk;
    } else {
        return Ok(Maybe_u8) Just(u8) c ENDJust ENDOk;
    }
}

DEFTYPE_Maybe_(u32);
DEFTYPE_Result_(Maybe_u32);

// XXX TODO: handle Byte order mark?

static
Result_Maybe_u32 get_unicodechar(FILE *in) {
    // https://en.wikipedia.org/wiki/Utf-8#Encoding
#define EBUFSIZ 256
    u32 codepoint;
    Result_Maybe_u8 b1 = getc_Result(in);
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
            Result_Maybe_u8 b = getc_Result(in);
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
int report(const char* instr, FILE* in) {
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
        ssize_t len;
        unsigned char *buf;
        __AFL_INIT();
        buf = __AFL_FUZZ_TESTCASE_BUF;
        while (__AFL_LOOP(1000)) {
            len = __AFL_FUZZ_TESTCASE_LEN;

            int res = report("STDIN", in);
            fprintf(stderr, "report returned with exit code %i\n", res);
        }
        return 0;
    } else {
#endif
        // Normal execution
        if (argc == 1) {
            return report("STDIN", stdin);
        } else if (argc == 2) {
            const char *path = argv[1];
            FILE *in = fopen(path, "r");
            if (!in) {
                perror_string("open(%s)", string_quote_sh(path));
                return 1;
            }
            String quotedpath = string_quote_sh(path);
            int res = report(quotedpath.str, in);
            string_release(quotedpath);
            if (fclose(in) != 0) {
                perror_string("close(%s)", string_quote_sh(path));
                return 1;
            }
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
