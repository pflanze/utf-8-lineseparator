/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef UNICODE_H_
#define UNICODE_H_


#include "shorttypenames.h" /* u8 u32 */
#include "BufferedStream.h"
#include "Result.h"
#include "Option.h"


DEFTYPE_Option(u32);
#define default_Option_u32
DEFTYPE_Result(Option_u32);


static
Result_Option_u32 get_unicodechar(BufferedStream *in) {
    // https://en.wikipedia.org/wiki/Utf-8#Encoding
#define EBUFSIZ 256
    u32 codepoint;
    Result_Option_u8 b1 = BufferedStream_getc(in);
    PROPAGATE_Result(Option_u32, b1);
    if (b1.ok.is_none) {
        return Ok(Option_u32, None(u32));
    }
    if ((b1.ok.value & 128) == 0) {
        // 1 byte
        codepoint = b1.ok.value;
    } else {
        int numbytes;
        if        ((b1.ok.value & 0b11100000) == 0b11000000) {
            numbytes = 2;
            codepoint = b1.ok.value & 0b11111;
        } else if ((b1.ok.value & 0b11110000) == 0b11100000) {
            numbytes = 3;
            codepoint = b1.ok.value & 0b1111;
        } else if ((b1.ok.value & 0b11111000) == 0b11110000) {
            numbytes = 4;
            codepoint = b1.ok.value & 0b111;
        } else {
            return Err(Option_u32, literal_String(
                             "invalid start byte decoding UTF-8"));
        }
        for (int i = 1; i < numbytes; i++) {
            Result_Option_u8 b = BufferedStream_getc(in);
            PROPAGATE_Result(Option_u32, b);
            if (b.ok.is_none) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "premature EOF decoding UTF-8 (byte #%i)",
                         i+1);
                return Err(Option_u32, copy_String(msg));
            }
            if ((b.ok.value & 0b11000000) != 0b10000000) {
                char msg[EBUFSIZ];
                snprintf(msg, EBUFSIZ,
                         "invalid continuation byte decoding UTF-8 (byte #%i)",
                         i+1);
                return Err(Option_u32, copy_String(msg));
            }
            codepoint <<= 6;
            codepoint |= (b.ok.value & 0b00111111);
        }
    }
    if (codepoint <= 0x10FFFF) {
        return Ok(Option_u32, Some(u32, codepoint));
    } else {
        char msg[EBUFSIZ];
        snprintf(msg, EBUFSIZ,
                 "invalid unicode codepoint (%u, 0x%x)",
                 codepoint, codepoint);
        return Err(Option_u32, copy_String(msg));
    }
#undef EBUFSIZ
}


#endif /* UNICODE_H_ */
