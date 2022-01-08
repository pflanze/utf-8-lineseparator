/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef UNICODE_H_
#define UNICODE_H_


#include "mem.h"
#include "shorttypenames.h" /* u8 u32 */
#include "bufferedstream.h"
#include "result.h"
#include "maybe.h"


DEFTYPE_Maybe_(u32);
#define default_Maybe_u32
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
        assert(codepoint < 0x10FFF0);
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


#endif /* UNICODE_H_ */
