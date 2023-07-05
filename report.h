#ifndef FRAMETEST_REPORT_H
#define FRAMETEST_REPORT_H

#include "tester.h"
#include "frametest.h"

extern void print_header_csv(const opts_t *opts);
extern void print_results_csv(const char *tcase, const opts_t *opts,
		const test_result_t *res);
extern void print_results(const char *tcase, const opts_t *opts,
		const test_result_t *res);

#endif
