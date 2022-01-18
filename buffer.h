/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>
#include <stdbool.h>

#include "shorttypenames.h"
#include "maybe.h"
#include "slice.h"


DEFTYPE_Maybe_(u8);
#define default_Maybe_u8 {}

DEFTYPE_Slice_(u8);


typedef struct {
    // slightly mis-using Slice here
    Slice_u8 slice; // the unused (read or unwritten) data
    /* const */ size_t size; // the size of the data in slice
    bool needs_freeing; // whether the data in slice needs to be freed
} Buffer;

static
void buffer_assert(Buffer *s) {
    assert(s->slice.endpos <= s->size);
    assert(s->slice.startpos <= s->slice.endpos);
    assert(s->slice.data);
}

static
void buffer_release(Buffer *b) {
    if (b->needs_freeing) free(b->slice.data);
}

static
Maybe_u8 buffer_getc(Buffer *b) {
    if (slice_is_empty(b->slice)) {
        return Nothing(u8);
    } else {
        return Just(u8) slice_get_unsafe(b->slice) ENDJust;
    }
}

// Returns true if done, false if buffer is full.
static
bool buffer_putc(Buffer *b, unsigned char c) {
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
