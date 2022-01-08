/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef BUFFER_H_
#define BUFFER_H_

#include <unistd.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "maybe.h"
#include "result.h"
#include "io.h"

typedef struct { } Unit;


DEFTYPE_Maybe_(u8);
DEFTYPE_Result_(Maybe_u8);
DEFTYPE_Result_(Unit);
#define default_Unit {}

typedef struct {
    size_t length;
    size_t readpos;
    const size_t size;
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


typedef struct {
} _BufferStream;

#define FD_NOTHING -1

typedef struct {
    int maybe_fd; // FD_NOTHING == closed
    bool is_exhausted; // saw EOF
    String maybe_failure; // error we saw
} _FileStream;


#define STREAM_TYPE_BUFFERSTREAM 1
#define STREAM_TYPE_FILESTREAM 2

typedef struct {
    Buffer buffer;
    bool is_closed; // in which case buffer's readpos == length == 0
    bool has_path; // whether a path is given in maybe_path_or_name
    String maybe_path_or_name;
    const uint8_t stream_type;
    union {
        _BufferStream bufferstream;
        _FileStream filestream;
    };
} BufferedStream;

#define STREAM_DIRECTION_IN 1
/* #define STREAM_DIRECTION_OUT 2 */
/* #define STREAM_DIRECTION_INOUT 3 */

UNUSED static
String /* owned by receiver */ bufferedstream_name_sh(BufferedStream *s) {
    assert(s->maybe_path_or_name.str);
    if (s->has_path) {
        return string_quote_sh(s->maybe_path_or_name.str);
    } else {
        return (String) { false, s->maybe_path_or_name.str };
    }
}


UNUSED static
BufferedStream buffer_to_BufferedStream(Buffer s /* owned */,
                                        String name /* owned */) {
    return (BufferedStream) {
        .buffer = s,
        .is_closed = false,
        .has_path = false,
        .maybe_path_or_name = name,
        .stream_type = STREAM_TYPE_BUFFERSTREAM,
        .bufferstream = {}
    };
}

static
BufferedStream fd_BufferedStream(int fd,
                                 int direction,
                                 String maybe_path_or_name /* owned */,
                                 bool is_path) {
    assert(fd >= 0);
    assert(direction == STREAM_DIRECTION_IN); // for now
#define BSIZ 4096
    unsigned char *buf = xmalloc(BSIZ);
    return (BufferedStream) {
        (Buffer) { .length = 0, .readpos = 0, .size = BSIZ,
            .array = buf, .needs_freeing = true },
        .is_closed = false,
        .has_path = is_path,
        .maybe_path_or_name = maybe_path_or_name,
        .stream_type = STREAM_TYPE_FILESTREAM,
        .filestream = (_FileStream) {
            .maybe_fd = fd,
            .is_exhausted = false,
            .maybe_failure = noString
        }
    };
#undef BSIZ
}

DEFTYPE_Result_(BufferedStream);

static
Result_BufferedStream open_BufferedStream(String path /* owned */,
                                          int flags) {
    assert(flags == O_RDONLY); // for now
#define BSIZ 4096
    unsigned char *buf = xmalloc(BSIZ);
    int fd = open(path.str, flags);
    if (fd < 0) {
        int err = errno;
        string_release(path);
        free(buf);
        return string_Error(BufferedStream, strerror_String(err));
    }
    return Ok(BufferedStream) (BufferedStream) {
        (Buffer) { .length = 0, .readpos = 0, .size = BSIZ,
            .array = buf, .needs_freeing = true },
        .is_closed = false,
        .has_path = true,
        .maybe_path_or_name = path,
        .stream_type = STREAM_TYPE_FILESTREAM,
        .filestream = (_FileStream) {
            .maybe_fd = fd,
            .is_exhausted = false,
            .maybe_failure = noString
        }
    } ENDOk;
#undef BSIZ
}


static
void bufferedstream_release(BufferedStream *s) {
    buffer_release(&s->buffer);
    if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
        // nothing
    }
    else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
        string_release(s->filestream.maybe_failure);
    }
    else {
        DIE("invalid stream_type");
    }
}

UNUSED static
void bufferedstream_free(BufferedStream *s) {
    bufferedstream_release(s);
    free(s);
}

static
Result_Unit bufferedstream_close(BufferedStream *s) {
    if (s->is_closed) {
        return Error(Unit, false, "stream is already closed");
    }
    s->is_closed = true;
    s->buffer.length = 0;
    s->buffer.readpos = 0;
    if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
        // nothing else to do
        return Ok(Unit) {} ENDOk;
    }
    else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
        // First check and return on previous failure? No! Need to
        // really call close when desired. (But we have to check
        // whether we already closed the fd, as the same fd could have
        // been re-used in the mean time, but that has been done above
        // via `is_closed` already.)
        assert(s->filestream.maybe_fd != FD_NOTHING);
        int fd = s->filestream.maybe_fd;
    retry: 
        if (close(fd) < 0) {
            int err = errno;
            if (err == EINTR) {
                goto retry;
            }
            // Clear `maybe_fd`? Store the failure? XX This is a bit
            // unclear!
            s->filestream.maybe_fd = FD_NOTHING;
            s->filestream.maybe_failure = strerror_String(err);
            return string_Error(Unit, s->filestream.maybe_failure);
        }
        s->filestream.maybe_fd = FD_NOTHING;
        return Ok(Unit) {} ENDOk;
    }
    else {
        DIE("invalid stream_type");
    }
}

static
Result_Maybe_u8 bufferedstream_getc(BufferedStream *in) {
    if (in->is_closed)
        return Error(Maybe_u8, false, "stream is closed");
    Maybe_u8 c = buffer_getc(&in->buffer);
    if (maybe_is_just(c)) {
        return Ok(Maybe_u8) c ENDOk;
    } else {
        if (in->stream_type == STREAM_TYPE_BUFFERSTREAM) {
            // there's nothing to do with in->bufferstream, simply:
            return Ok(Maybe_u8) Nothing(u8) ENDOk;
        }
        else if (in->stream_type == STREAM_TYPE_FILESTREAM) {
            if (in->filestream.is_exhausted) {
                return Ok(Maybe_u8) Nothing(u8) ENDOk;
            } else if (in->filestream.maybe_failure.str) {
                // return previously seen failure (OK?)
                return (Result_Maybe_u8) {
                    in->filestream.maybe_failure,
                    Nothing(u8)
                };
            } else {
                // replenish the buffer
                assert(in->filestream.maybe_fd != FD_NOTHING);
                int fd = in->filestream.maybe_fd;
            retry: {
                    int n = read(fd, in->buffer.array, in->buffer.size);
                    if (n < 0) {
                        int err = errno;
                        if (err == EINTR) {
                            goto retry;
                        }
                        in->filestream.maybe_failure = strerror_String(err);
                        /* return (Result_Maybe_u8) { */
                        /*     in->filestream.maybe_failure, */
                        /*     Nothing(u8) */
                        /* }; */
                    } else if (n == 0) {
                        // EOF
                        in->filestream.is_exhausted = true;
                        //return Ok(Maybe_u8) Nothing(u8) ENDOk;
                    } else {
                        in->buffer.length = n;
                        in->buffer.readpos = 0;
                    }
                    return bufferedstream_getc(in);
                }
            }
        }
        else {
            DIE("invalid stream_type");
        }
    }
}

#endif /* BUFFER_H_ */
