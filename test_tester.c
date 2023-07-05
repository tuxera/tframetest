#define _XOPEN_SOURCE 500
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "unittest.h"
#include "tester.h"

#define SLEEP_TIME 1000UL

int test_tester_start_stop(void)
{
	uint64_t start;
	uint64_t time;
	uint64_t end;

	start = tester_start();
	usleep(SLEEP_TIME);
	time = tester_stop(start);
	end = tester_start();

	TEST_ASSERT(end > start);
	TEST_ASSERT(time != 0);
	TEST_ASSERT(time >= SLEEP_TIME);

	return 0;
}

int test_tester_run_write(void)
{
	/*
	 * TODO tester_run_write() needs path and I/O access to files.
	 * We should fake those OR create a temporary folder.
	 */
	return 0;
}

int test_tester(void)
{
	TEST_INIT();

	TEST(tester_start_stop);
	TEST(tester_run_write);

	TEST_END();
}
