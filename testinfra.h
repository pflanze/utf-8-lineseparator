/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef TESTINFRA_H_
#define TESTINFRA_H_

#include "util.h" /* XSTR */


typedef struct {
    int successes;
    int failures;
    int errors;
} TestStatistics;


static
void TestStatistics_print(TestStatistics *stats) {
    printf("test results: %i success(es), %i failure(s), %i error(s)\n",
           stats->successes, stats->failures, stats->errors);
}

static
int TestStatistics_issues(TestStatistics *stats) {
    return stats->failures + stats->errors;
}


// Helper macros: these expect `TestStatistics *stats` to be in scope.

#define TEST_SUCCESS                            \
    stats->successes++

#define TEST_FAILURE_(fmt, ...)                 \
    WARN_("*** Test failed: " fmt " at %s:%i",  \
          __VA_ARGS__, __FILE__, __LINE__);     \
    stats->failures++

#define TEST_FAILURE(msg)                       \
    TEST_FAILURE_("%s", msg)

#define TEST_ERROR_(fmt, ...)                           \
    WARN_("*** Error running test: " fmt " at %s:%i",   \
          __VA_ARGS__, __FILE__, __LINE__);             \
    stats->errors++

#define TEST_ERROR(msg)                                 \
    TEST_ERROR_("%s", msg)


#define TEST_ASSERT(expr)                       \
    if (expr) {                                 \
        TEST_SUCCESS;                           \
    } else {                                    \
        TEST_FAILURE_("%s", XSTR(expr));        \
    }


#endif /* TESTINFRA_H_ */
