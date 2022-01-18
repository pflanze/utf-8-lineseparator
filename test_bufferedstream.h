/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef TEST_BUFFEREDSTREAM_H_
#define TEST_BUFFEREDSTREAM_H_

#include "testinfra.h"
#include "bufferedstream.h"


static
Result_Unit test_bufferedstream_1(TestStatistics *stats) {
    BEGINRETURN(Result_Unit);
#define TBUFSIZ 9000
    unsigned char buf[TBUFSIZ];
    for (int i = 0; i < TBUFSIZ; i++) {
        buf[i] = i;
    }

    Result_BufferedStream rs = open_BufferedStream(
        String_literal(".test.out"), O_WRONLY | O_CREAT, 0666);
    PROPAGATEL_Result(rs, Unit, rs);

    Result_Maybe_u8 rmc0 = bufferedstream_getc(&rs.ok);
    if (result_is_failure(rmc0)) {
        TEST_ASSERT(0 == strcmp(rmc0.failure.str,
                                "getc: stream was not opened for input"));
    } else {
        TEST_FAILURE("getc on write-only file handle did not fail");
    }
    result_release(rmc0);

    Result_Unit ru;
    for (int i = 0; i < TBUFSIZ; i++) {
        ru = bufferedstream_putc(&rs.ok, buf[i]);
        PROPAGATEL_Result(ru, Unit, ru);
    }

    Result_Unit ru2 = bufferedstream_close(&rs.ok);
    PROPAGATEL_Result(ru2, Unit, ru2);

    // Now read back and compare:

    Result_BufferedStream rs2 = open_r_BufferedStream(
        String_literal(".test.out"));
    PROPAGATEL_Result(rs2, Unit, rs2);

    Result_Maybe_u8 rmc;
    for (int i = 0; i < TBUFSIZ; i++) {
        rmc = bufferedstream_getc(&rs2.ok);
        PROPAGATEL_Result(rmc, Unit, rmc);
        if (rmc.ok.is_nothing) {
            TEST_FAILURE("bug: file is too short");
        } else {
            TEST_ASSERT(rmc.ok.value == buf[i]);
            // ^ XX that reports tons of success. return on fail!
        }
    }
    rmc = bufferedstream_getc(&rs2.ok);
    PROPAGATEL_Result(rmc, Unit, rmc);
    if (! rmc.ok.is_nothing) {
        TEST_FAILURE("bug: file is too long");
    }

    Result_Unit ru3 = bufferedstream_close(&rs2.ok);
    PROPAGATEL_Result(ru3, Unit, ru3);

    RETURN(Ok(Unit) {} ENDOk);
ru3:
    result_release(ru3);
rmc:
    result_release(rmc);
    bufferedstream_close(&rs2.ok);
    bufferedstream_release(&rs2.ok);
rs2:
    result_release(rs2);
ru2:
    result_release(ru2);
ru:
    result_release(ru);
rs:
    bufferedstream_close(&rs.ok); // no need to check the result here
    bufferedstream_release(&rs.ok);
    result_release(rs);
    ENDRETURN;
}


#define CHECK(e)                                        \
    r = e;                                              \
    if (result_is_failure(r)) {                         \
        TEST_FAILURE_("%s", r.failure.str);             \
        result_release(r);                              \
    }


static
void test_bufferedstream(TestStatistics *stats) {

    Result_Unit r;

    CHECK(test_bufferedstream_1(stats));
}

#endif /* TEST_BUFFEREDSTREAM_H_ */
