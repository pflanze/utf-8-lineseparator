/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef TEST_BUFFEREDSTREAM_H_
#define TEST_BUFFEREDSTREAM_H_

#include "testinfra.h"
#include "BufferedStream.h"


static
Result_Unit test_BufferedStream_1(TestStatistics *stats) {
    BEGIN_PROPAGATE(Unit);
#define TBUFSIZ 90000
    unsigned char buf[TBUFSIZ];
    {
        uint64_t n0, n1;
        n0 = 1;
        n1 = 1;
        for (uint64_t i = 0; i < TBUFSIZ; i++) {
            uint64_t n = n0 + n1 - (i >> 8);
            buf[i] = n;
            n0 = n1;
            n1 = n;
        }
    }

    Result_BufferedStream rs = open_BufferedStream(
        literal_String(".test.out"), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    PROPAGATEL_Result(rs, Unit, rs);

    {
        Result_Option_u8 rmc0 = BufferedStream_getc(&rs.ok);
        // getc on write-only file handle must fail:
        TEST_ASSERT(Result_is_Err(rmc0));
        if (Result_is_Err(rmc0)) {
            TEST_ASSERT(0 == strcmp(rmc0.err.str,
                                    "getc: stream was not opened for input"));
        }
        Result_release(rmc0);
    }

    {
        Result_Unit ru;
        for (int i = 0; i < TBUFSIZ; i++) {
            ru = BufferedStream_putc(&rs.ok, buf[i]);
            PROPAGATEL_Result(ru, Unit, ru);
        }

        {
            Result_Unit ru2 = BufferedStream_close(&rs.ok);
            PROPAGATEL_Result(ru2, Unit, ru2);

            // Now read back and compare:

            {
                Result_BufferedStream rs2 = open_r_BufferedStream(
                    literal_String(".test.out"));
                PROPAGATEL_Result(rs2, Unit, rs2);

                Result_Option_u8 rmc;
                for (int i = 0; i < TBUFSIZ; i++) {
                    rmc = BufferedStream_getc(&rs2.ok);
                    PROPAGATEL_Result(rmc, Unit, rmc);
                    if (rmc.ok.is_none) {
                        RETURNL(rmc, Error(Unit, literal_String(
                                               "bug: file is too small")));
                    } else {
                        if (! (rmc.ok.value == buf[i])) {
                            RETURNL(rmc, Error(Unit, literal_String(
                                                   "rmc.ok.value == buf[i] failed")));
                        }
                    }
                }
                rmc = BufferedStream_getc(&rs2.ok);
                PROPAGATEL_Result(rmc, Unit, rmc);
                if (! rmc.ok.is_none) {
                    RETURNL(rmc, Error(Unit, literal_String(
                                           "bug: file is too large")));
                }

                {
                    Result_Unit ru3 = BufferedStream_close(&rs2.ok);
                    PROPAGATEL_Result(ru3, Unit, ru3);

                    RETURN(Ok(Unit, {}));
                ru3:
                    Result_release(ru3);
                }
            rmc:
                Result_release(rmc);
                BufferedStream_close(&rs2.ok);
                BufferedStream_release(&rs2.ok);
            rs2:
                Result_release(rs2);
            }
        ru2:
            Result_release(ru2);
        }
    ru:
        Result_release(ru);
    }
rs:
    BufferedStream_close(&rs.ok); // no need to check the result here
    BufferedStream_release(&rs.ok);
    Result_release(rs);
    END_PROPAGATE;
}


#define CHECK(e)                                        \
    r = e;                                              \
    if (Result_is_Err(r)) {                         \
        TEST_FAILURE_("%s", r.err.str);             \
        Result_release(r);                              \
    }


static
void test_BufferedStream(TestStatistics *stats) {

    Result_Unit r;

    CHECK(test_BufferedStream_1(stats));
}

#endif /* TEST_BUFFEREDSTREAM_H_ */
