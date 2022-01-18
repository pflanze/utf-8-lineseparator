/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef BUFFEREDSTREAM_H_
#define BUFFEREDSTREAM_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
/* open: */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* /open */
#include <errno.h>
#include <assert.h>

#include "io.h"
#include "shorttypenames.h"
#include "maybe.h"
#include "result.h"
#include "buffer.h"
#include "util.h"

#include "monkey.h"


const size_t bufferedstream_buffersize = 4096*4;


typedef struct { } Unit;
#define default_Unit {}

DEFTYPE_Result_(Maybe_u8);
DEFTYPE_Result_(Unit);


typedef struct {
} _BufferStream;

#define FD_NOTHING -1

typedef struct {
    int maybe_fd; // FD_NOTHING == closed
    bool is_exhausted; // saw EOF
    String maybe_failure; // error we saw
} _FileStream;


#define STREAM_DIRECTION_IN 1
#define STREAM_DIRECTION_OUT 2
#define STREAM_DIRECTION_INOUT 3
static
void assert_direction(uint8_t direction) {
    assert((direction >= STREAM_DIRECTION_IN)
           && (direction <= STREAM_DIRECTION_INOUT));
}


#define STREAM_TYPE_BUFFERSTREAM 1
#define STREAM_TYPE_FILESTREAM 2

typedef struct {
    Buffer buffer;
    bool is_closed; // in which case buffer's pos == length == 0
    bool has_path; // whether a path is given in maybe_path_or_name
    String maybe_path_or_name;
    const uint8_t direction;
    const uint8_t stream_type;
    union {
        _BufferStream bufferstream;
        _FileStream filestream;
    };
} BufferedStream;

#define default_BufferedStream (BufferedStream){}

UNUSED static
String /* owned by receiver */ bufferedstream_name_sh(BufferedStream *s) {
    assert(s->maybe_path_or_name.str);
    if (s->has_path) {
        return string_quote_sh(s->maybe_path_or_name.str);
    } else {
        return String_copy(s->maybe_path_or_name.str);
    }
}


UNUSED static
BufferedStream buffer_to_BufferedStream(Buffer b /* owned */,
                                        uint8_t direction,
                                        String name /* owned */) {
    assert_direction(direction);
    buffer_assert(&b);
    if ((direction & STREAM_DIRECTION_OUT) && b.size == 0) {
        /* return Error(Unit, String_literal( */
        /*                  "can't use buffer of size 0 for writing")); */
        // Treat this as a bug, to keep the return type non-Result, hmm?
        DIE("can't use buffer of size 0 for writing");
    }

    return (BufferedStream) {
        .buffer = b,
        .is_closed = false,
        .has_path = false,
        .maybe_path_or_name = name,
        .direction = direction,
        .stream_type = STREAM_TYPE_BUFFERSTREAM,
        .bufferstream = {}
    };
}

UNUSED static
BufferedStream fd_BufferedStream(int fd,
                                 uint8_t direction,
                                 String maybe_path_or_name /* owned */,
                                 bool is_path) {
    assert(fd >= 0);
    assert_direction(direction);
    
    return (BufferedStream) {
        .buffer = Buffer_from_buf(true,
                                  xmalloc(bufferedstream_buffersize),
                                  bufferedstream_buffersize),
        .is_closed = false,
        .has_path = is_path,
        .maybe_path_or_name = maybe_path_or_name,
        .direction = direction,
        .stream_type = STREAM_TYPE_FILESTREAM,
        .filestream = (_FileStream) {
            .maybe_fd = fd,
            .is_exhausted = false,
            .maybe_failure = noString
        }
    };
}

DEFTYPE_Result_(BufferedStream);

UNUSED static
Result_BufferedStream open_BufferedStream(String path /* owned */,
                                          int flags,
                                          mode_t mode) {
    // The flags are 0, 1, 2 on Linux, but ? HACKY.
    const int flags_directions = flags & (O_RDONLY | O_WRONLY | O_RDWR);
    uint8_t direction;
    if (flags_directions == O_RDWR) {
        direction = STREAM_DIRECTION_INOUT;
    } else if (flags_directions == O_WRONLY) {
        direction = STREAM_DIRECTION_OUT;
    } else if (flags_directions == O_RDONLY) {
        direction = STREAM_DIRECTION_IN;
    } else {
        DIE("invalid flags");
    }
    
    unsigned char *buf = xmalloc(bufferedstream_buffersize);
    int fd = open(path.str, flags, mode);
    if (fd < 0) {
        int err = errno;
        string_release(path);
        free(buf);
        return Error(BufferedStream, strerror_String(err));
    }
    return Ok(BufferedStream) (BufferedStream) {
        .buffer = Buffer_from_buf(true,
                                  buf,
                                  bufferedstream_buffersize),
        .is_closed = false,
        .has_path = true,
        .maybe_path_or_name = path,
        .direction = direction,
        .stream_type = STREAM_TYPE_FILESTREAM,
        .filestream = (_FileStream) {
            .maybe_fd = fd,
            .is_exhausted = false,
            .maybe_failure = noString
        }
    } ENDOk;
}

UNUSED static
Result_BufferedStream open_r_BufferedStream(String path /* owned */) {
    return open_BufferedStream(path, O_RDONLY, 0);
}


static
void bufferedstream_release(BufferedStream *s) {
    assert(s->is_closed);
    assert_direction(s->direction); // paranoia (to catch all usage
                                    // patterns)

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
Result_Unit _bufferedstream_filestream_flush_unsafe(BufferedStream *s) {
    assert(s->buffer.size > 0); // otherwise it would loop endlessly
    int fd = s->filestream.maybe_fd;
retry: {
        int n = write(fd,
                      slice_start(s->buffer.slice),
                      slice_length(s->buffer.slice));
        if (n < 0) {
            int err = errno;
            if (err == EINTR) {
                goto retry;
            }
            s->filestream.maybe_failure = strerror_String(err);
            return Error(Unit, {});
        } else if (n == slice_length(s->buffer.slice)) {
            // done
            s->buffer.slice.startpos = 0;
            s->buffer.slice.endpos = 0;
            return Ok(Unit) {} ENDOk;
        } else {
            // partial write
            s->buffer.slice.startpos += n;
            assert(s->buffer.slice.startpos <=
                   s->buffer.slice.endpos);
            goto retry;
        }
    }
}

static
Result_Unit bufferedstream_flush(BufferedStream *s) {
    if (s->is_closed) {
        return Error(Unit, String_literal("flush: stream is closed"));
    }
    if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
        return Ok(Unit) {} ENDOk;
    }
    else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
        assert(s->filestream.maybe_fd != FD_NOTHING);
        // Turn startpos from putc writing pos into write writing pos
        // (this is hacky) (it is being turned back to putc writing
        // pos by _bufferedstream_filestream_flush_unsafe resetting
        // the slice to positions 0,0):
        s->buffer.slice.startpos = 0;
        return _bufferedstream_filestream_flush_unsafe(s);
    }
    else {
        DIE("invalid stream_type");
    }
}

static
Result_Unit bufferedstream_close(BufferedStream *s) {
    if (s->is_closed) {
        return Error(Unit, String_literal("close: stream is already closed"));
    }
    BEGINRETURN(Result_Unit);
    if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
        RETURN(Ok(Unit) {} ENDOk);
    }
    else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
        // First check and return on previous failure? No! Need to
        // really call close when desired. (But we have to check
        // whether we already closed the fd, as the same fd could have
        // been re-used in the mean time, but that has been done above
        // via `is_closed` already.)
        if (s->direction & STREAM_DIRECTION_OUT) {
            Result_Unit r = bufferedstream_flush(s);
            PROPAGATE_Result(Unit, r);
        }
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
            RETURN(Error(Unit, s->filestream.maybe_failure));
        } else {
            s->filestream.maybe_fd = FD_NOTHING;
            RETURN(Ok(Unit) {} ENDOk);
        }
    }
    else {
        DIE("invalid stream_type");
    }
    // on RETURN:
    s->is_closed = true;
    s->buffer.slice.startpos = 0;
    s->buffer.slice.endpos = 0;
    ENDRETURN;
}

static
Result_Maybe_u8 bufferedstream_getc(BufferedStream *s) {
    if (s->is_closed) {
        return Error(Maybe_u8, String_literal("getc: stream is closed"));
    }
    if (! (s->direction & STREAM_DIRECTION_IN)) {
        return Error(Maybe_u8, String_literal(
                         "getc: stream was not opened for input"));
    }
    
    Maybe_u8 c = buffer_getc(&s->buffer);
    if (maybe_is_just(c)) {
        return Ok(Maybe_u8) c ENDOk;
    } else {
        if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
            // there's nothing to do with s->bufferstream, simply:
            return Ok(Maybe_u8) Nothing(u8) ENDOk;
        }
        else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
            if (s->filestream.is_exhausted) {
                return Ok(Maybe_u8) Nothing(u8) ENDOk;
            } else if (s->filestream.maybe_failure.str) {
                // return previously seen failure (OK?)
                return (Result_Maybe_u8) {
                    s->filestream.maybe_failure,
                    Nothing(u8)
                };
            } else {
                // replenish the buffer
                assert(s->filestream.maybe_fd != FD_NOTHING);
                int fd = s->filestream.maybe_fd;
            retry: {
                    int n = read(fd, s->buffer.slice.data, s->buffer.size);
                    if (n < 0) {
                        int err = errno;
                        if (err == EINTR) {
                            goto retry;
                        }
                        s->filestream.maybe_failure = strerror_String(err);
                        /* return (Result_Maybe_u8) { */
                        /*     s->filestream.maybe_failure, */
                        /*     Nothing(u8) */
                        /* }; */
                    } else if (n == 0) {
                        // EOF
                        s->filestream.is_exhausted = true;
                        //return Ok(Maybe_u8) Nothing(u8) ENDOk;
                    } else {
                        s->buffer.slice.startpos = 0;
                        s->buffer.slice.endpos = n;
                    }
                    return bufferedstream_getc(s);
                }
            }
        }
        else {
            DIE("invalid stream_type");
        }
    }
}

static
Result_Unit bufferedstream_putc(BufferedStream *s, unsigned char c) {
    if (s->is_closed) {
        return Error(Unit, String_literal("putc: stream is closed"));
    }
    if (! (s->direction & STREAM_DIRECTION_OUT)) {
        return Error(Unit, String_literal(
                         "getc: stream was not opened for output"));
    }

    if (buffer_putc(&s->buffer, c)) {
        return Ok(Unit) {} ENDOk;
    } else {
        if (s->stream_type == STREAM_TYPE_BUFFERSTREAM) {
            // XX include name in message, right?
            return Error(Unit, String_literal("putc to buffer: out of space"));
        }
        else if (s->stream_type == STREAM_TYPE_FILESTREAM) {
            Result_Unit r = bufferedstream_flush(s);
            PROPAGATE_Result(Unit, r);
            return bufferedstream_putc(s, c);
        }
        else {
            DIE("invalid stream_type");
        }
    }
}

#endif /* BUFFEREDSTREAM_H_ */
