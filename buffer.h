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


DEFTYPE_Maybe_(u8);
#define default_Maybe_u8 {}

typedef struct {
    size_t length;
    size_t pos;
    /* const */ size_t size;
    unsigned char *array;
    bool needs_freeing;
} Buffer;

static
void buffer_release(Buffer *b) {
    if (b->needs_freeing) free(b->array);
    b->array = NULL;
}

static
Maybe_u8 buffer_getc(Buffer *b) {
    if (b->pos < b->length) {
        return Just(u8) b->array[b->pos++] ENDJust;
    } else {
        return Nothing(u8);
    }
}

// Returns true if done, false if buffer is full.
static
bool buffer_putc(Buffer *b, unsigned char c) {
    size_t newpos = b->pos + 1;
    if (newpos < b->size) {
        b->array[newpos] = c;
        b->pos = newpos;
        // XX is this bad design? Have 2 kinds of buffers, or?:
        size_t newminlen = newpos + 1;
        if (b->length < newminlen) {
            b->length = newminlen;
        }
        return true;
    } else {
        return false;
    }
}


#endif /* BUFFER_H_ */
