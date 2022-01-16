/*
  Copyright (C) 2021 Christian Jaeger, <ch@christianjaeger.ch>
  Published under the terms of the MIT License, see the LICENSE file.
*/

#ifndef TESTINFRA_H_
#define TESTINFRA_H_


typedef struct {
    int successes;
    int failures;
    int errors;
} TestStatistics;


static
void teststatistics_print(TestStatistics *stats) {
    printf("test results: %i success(es), %i failure(s), %i error(s)\n",
           stats->successes, stats->failures, stats->errors);
}

static
int teststatistics_issues(TestStatistics *stats) {
    return stats->failures + stats->errors;
}

#endif /* TESTINFRA_H_ */
