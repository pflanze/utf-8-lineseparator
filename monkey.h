/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

/*
  Monkey testing, i.e. feeding fuzzing inputs via various IO
  calls. Meant to be used with (AFL/)AFL++.
 */

#ifndef MONKEY_H_
#define MONKEY_H_

#include "string.h"


// move globals to .c once created

#if AFL

Buffer monkey_buf;

typedef struct {
    u8 waits; // number of calls before the next failure is injected
    u8 other; // additional state, e.g. num calls until getting EINTR
} MonkeyWrapperState;

#define MONKEY_MAX_MONKEYWRAPPERSTATE_INSTANCES 256
MonkeyWrapperState* monkey_monkeywrapperstates[
    MONKEY_MAX_MONKEYWRAPPERSTATE_INSTANCES];
size_t monkey_monkeywrapperstates_i = 0;

static
void monkey_register_monkeywrapperstate(MonkeyWrapperState *st) {
    if (monkey_monkeywrapperstates_i < MONKEY_MAX_MONKEYWRAPPERSTATE_INSTANCES) {
        monkey_monkeywrapperstates[
            monkey_monkeywrapperstates_i++] = st;
    } else {
        DIE("out of space for MonkeyWrapperState instances, "
            "please increase MONKEY_MAX_MONKEYWRAPPERSTATE_INSTANCES");
    }
}

static
MonkeyWrapperState new_MonkeyWrapperState() {
    Maybe_u8 mc = buffer_getc(&monkey_buf);
    if (mc.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    Maybe_u8 mc2 = buffer_getc(&monkey_buf);
    if (mc2.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    return (MonkeyWrapperState) {
        .waits = mc.value,
        .other = mc2.value,
    };
}

static
void monkey_init(unsigned char *buf /* borrowed */,
                 size_t len) {
    monkey_buf = (Buffer) {
        .length = len,
        .readpos = 0,
        .size = len,
        .array = xmemcpy(buf, len),
        .needs_freeing = true
    };
    for (size_t i = 0; i < monkey_monkeywrapperstates_i; i++) {
        *(monkey_monkeywrapperstates[i]) = new_MonkeyWrapperState();
    }
}

static
void monkey_release() {
    buffer_release(&monkey_buf);
}



// https://stackoverflow.com/questions/2521927/initializing-a-global-struct-in-c
#define DEF_MONKEYWRAPPERSTATE(var)                     \
    MonkeyWrapperState var;                             \
    static __attribute__((constructor))                 \
    void monkeywrapper_init_##var() {                   \
        monkey_register_monkeywrapperstate(&var);       \
    }


static
void monkeywrapperstate_refresh_both(MonkeyWrapperState *s) {
    Maybe_u8 mc = buffer_getc(&monkey_buf);
    if (mc.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    s->waits = mc.value;
    Maybe_u8 mc2 = buffer_getc(&monkey_buf);
    if (mc2.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    s->other = mc2.value;
}    

static
void monkeywrapperstate_refresh_waits(MonkeyWrapperState *s) {
    Maybe_u8 mc = buffer_getc(&monkey_buf);
    if (mc.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    s->waits = mc.value;
}

static
void monkeywrapperstate_refresh_other(MonkeyWrapperState *s) {
    Maybe_u8 mc2 = buffer_getc(&monkey_buf);
    if (mc2.is_nothing) {
        DIE("not enough fuzzer input for monkey");
    }
    s->other = mc2.value;
}


#else

#define DEF_MONKEYWRAPPERSTATE(var)

#endif /* AFL */

#endif /* MONKEY_H_ */
