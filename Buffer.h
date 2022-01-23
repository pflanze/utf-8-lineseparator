/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>
#include <stdbool.h>

#include "shorttypenames.h"
#include "Option.h"
#include "LSlice.h"
#include "util.h" /* UNUSED */


DEFTYPE_Option(u8);
#define default_Option_u8 {}

DEFTYPE_LSlice_(u8);


typedef struct {
    // slightly mis-using LSlice here
    LSlice_u8 slice; // the unused (read or unwritten) data
    /* const */ size_t size; // the size of the data in slice
    bool needs_freeing; // whether the data in slice needs to be freed
} Buffer;

static
void Buffer_assert(Buffer *s) {
    assert(s->slice.endpos <= s->size);
    assert(s->slice.startpos <= s->slice.endpos);
    assert(s->slice.data);
}

static UNUSED
Buffer Buffer_from_array(bool needs_freeing, unsigned char *array, size_t size) {
    return (Buffer) {
        .slice = (LSlice_u8) {
            .startpos = 0,
            .endpos = size,
            .data = array
        },
        .size = size,
        .needs_freeing = needs_freeing
    };
}

// Same as Buffer_from_array but initializes the active content length
// to 0, i.e. buf is only used for reading into / writing to, not as
// data source.
static UNUSED
Buffer Buffer_from_buf(bool needs_freeing, unsigned char *buf, size_t size) {
    return (Buffer) {
        .slice = (LSlice_u8) {
            .startpos = 0,
            .endpos = 0,
            .data = buf
        },
        .size = size,
        .needs_freeing = needs_freeing
    };
}

static
void Buffer_release(Buffer *b) {
    if (b->needs_freeing) free(b->slice.data);
}

static
Option(u8) Buffer_getc(Buffer *b) {
    if (LSlice_is_empty(b->slice)) {
        return None(u8);
    } else {
        return Some(u8, LSlice_get_unsafe(b->slice));
    }
}

// Returns true if done, false if buffer is full.
static
bool Buffer_putc(Buffer *b, unsigned char c) {
    size_t startpos = b->slice.startpos;
    if (startpos < b->size) {
        b->slice.data[startpos] = c;
        size_t newpos = b->slice.startpos + 1;
        b->slice.startpos = newpos;
        // XX is this bad design? Have 2 kinds of buffers, or?:
        if (b->slice.endpos < newpos) {
            b->slice.endpos = newpos;
        }
        return true;
    } else {
        return false;
    }
}


#endif /* BUFFER_H_ */
