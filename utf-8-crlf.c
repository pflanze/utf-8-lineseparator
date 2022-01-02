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


typedef struct {
    bool needs_free;
    const char* str;
} String;

#define DEFTYPE_Result_(T)                      \
    typedef struct {                            \
        String failure;                         \
        T ok;                                   \
    } Result_##T;

#define Error(T, needs_free, str)                               \
    (Result_##T) { (String) { needs_free, str }, (T){} }
#define Ok(T)                                           \
    (Result_##T) { (String) { false, NULL }, (T)
#define EndOk }

#define result_is_success(v) (!((v).failure.str))
#define result_is_failure(v) (!!((v).failure.str))
// #define result_failure_str(v) ((v).failure.str)
#define result_release(v)                       \
    if ((v).failure.needs_free) {               \
        free((void*)(v).failure.str);           \
    }
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
#define EndJust  }


typedef uint8_t u8;
#define default_u8 0
typedef uint32_t u32;
#define default_u32 0


DEFTYPE_Maybe_(u8);
DEFTYPE_Result_(Maybe_u8);

Result_Maybe_u8 getc_Result(FILE *in) {
    int c = getc(in);
    if (c == EOF) {
        if (errno) {
#define EBUFSIZ 256
            char msg[EBUFSIZ];
            strerror_r(errno, msg, EBUFSIZ);
            // Error-check strerr_r and strdup, or leave as is?
            return Error(Maybe_u8, true, strdup(msg));
#undef EBUFSIZ
        }
        return Ok(Maybe_u8) Nothing(u8) EndOk;
    } else {
        return Ok(Maybe_u8) Just(u8) c EndJust EndOk;
    }
}

DEFTYPE_Maybe_(u32);
DEFTYPE_Result_(Maybe_u32);

// XXX TODO: handle Byte order mark?

Result_Maybe_u32 get_unicodechar(FILE *in) {
    // https://en.wikipedia.org/wiki/Utf-8#Encoding
#define EBUFSIZ 256
    u32 codepoint;
    Result_Maybe_u8 b1 = getc_Result(in);
    PROPAGATE_Result(Maybe_u32, b1);
    if (b1.ok.is_nothing) {
        return Ok(Maybe_u32) Nothing(u32) EndOk;
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
                "invalid start byte decoding UTF-8 (byte 1)");
        }
        for (int i = 1; i < numbytes; i++) {
            Result_Maybe_u8 b = getc_Result(in);
            PROPAGATE_Result(Maybe_u32, b);
            if (b.ok.is_nothing) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "premature EOF decoding UTF-8 (byte %i)",
                         i+1);
                return Error(Maybe_u32, true, strdup(msg));
            }
            if ((b.ok.value & 0b11000000) != 0b10000000) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "invalid continuation byte decoding UTF-8 (byte %i)",
                         i+1);
                return Error(Maybe_u32, true, strdup(msg));
            }
            codepoint |= ((b.ok.value & 0b00111111) << (i * 6 + 1));
        }
    }
    if (codepoint <= 0x10FFFF) {
        return Ok(Maybe_u32) Just(u32) codepoint EndJust EndOk;
    } else {
        char msg[EBUFSIZ];
        snprintf(msg, EBUFSIZ,
                 "invalid unicode codepoint (%ui)",
                 codepoint);
        return Error(Maybe_u32, true, strdup(msg));
    }
#undef EBUFSIZ
}


int main(int argc, const char**argv) {
    const char *path = argv[1];
    FILE *in = fopen(path, "r");
    if (!in) {
        perror("open");
        return 1;
    }
    int64_t charcount = 0;
    int64_t LFcount = 0;
    int64_t CRcount = 0;
    int64_t CRLFcount = 0;
    bool last_was_CR = false;
    while (1) {
        Result_Maybe_u32 c = get_unicodechar(in);
        if (result_is_failure(c)) {
            fprintf(stderr, "utf-8-crlf(%s): %s\n", path, c.failure.str);
            result_release(c);
            fclose(in);
            return 1;
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
        } else if (c.ok.value == '\n') {
            if (last_was_CR) {
                CRLFcount++;
            } else {
                LFcount++;
            }
            last_was_CR = false;
        } else {
            if (last_was_CR) {
                CRcount++;
            }
            last_was_CR = false;
        }
    }
    if (last_was_CR) {
        CRcount++;
    }
    if (fclose(in) != 0) {
        perror("close");
        return 1;
    }
    printf("{ charcount: %li, LFcount: %li, CRcount: %li, CRLFcount: %li }\n",
           charcount, LFcount, CRcount, CRLFcount);
    return 0;
}
