/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#define _POSIX_C_SOURCE 202112L

#include "leakcheck.h"

#include "testinfra.h"
#include "test_unicode.h"
#include "test_bufferedstream.h"


int main() {
    TestStatistics stats = {};

    test_unicode(&stats);
    test_bufferedstream(&stats);

    teststatistics_print(&stats);
    leakcheck_verify(false);
    // ^ don't say true, as this will run under SAN and thus this will
    // lead to a better report
    return teststatistics_issues(&stats) ? 1 : 0;
}

