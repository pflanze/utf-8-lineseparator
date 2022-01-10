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
    size_t readpos;
    /* const */ size_t size;
    unsigned char *array;
    bool needs_freeing;
} Buffer;

static
void buffer_release(Buffer *s) {
    if (s->needs_freeing) free(s->array);
    s->array = NULL;
}

static
Maybe_u8 buffer_getc(Buffer *in) {
    if (in->readpos < in->length) {
        return Just(u8) in->array[in->readpos++] ENDJust;
    } else {
        return Nothing(u8);
    }
}


#endif /* BUFFER_H_ */
