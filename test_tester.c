#define _XOPEN_SOURCE 500
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "unittest.h"
#include "tester.h"

#define SLEEP_TIME 1000UL

static const platform_t *tester_platform = NULL;

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

static frame_t *gen_default_frame(void)
{
	profile_t prof;

	prof = profile_get_by_index(1);
	return frame_gen(tester_platform, prof);
}

static int tester_run_write_read_with(test_mode_t mode, size_t fps)
{
	const size_t frames = 5;
	test_result_t res;
	test_result_t res_read;
	platform_handle_t f;
	frame_t *frm;
	frame_t *frm_res;

	frm = gen_default_frame();
	TEST_ASSERT(frm);

	f = tester_platform->open("./frame000000.tst", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_EQ(f, -1);

	res = tester_run_write(tester_platform, ".", frm, 0, frames, fps,
			mode);

	TEST_ASSERT_EQ(res.frames_written, frames);
	TEST_ASSERT_EQ(res.bytes_written, frames * frm->size);
	TEST_ASSERT(res.completion);

	result_free(tester_platform, &res);

	f = tester_platform->open("./frame000000.tst", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_NE(f, -1);
	tester_platform->close(f);

	frm_res = tester_get_frame_read(tester_platform, ".");
	TEST_ASSERT(frm_res);

	res_read = tester_run_read(tester_platform, ".", frm_res, 0, frames,
			fps, mode);
	TEST_ASSERT_EQ(res_read.frames_written, frames);
	TEST_ASSERT_EQ(res_read.bytes_written, frames * frm_res->size);
	TEST_ASSERT(res_read.completion);

	result_free(tester_platform, &res_read);

	return 0;
}

int test_tester_run_write_read(void)
{
	return tester_run_write_read_with(TEST_MODE_NORM, 0);
}

int test_tester_run_write_read_reverse(void)
{
	return tester_run_write_read_with(TEST_MODE_REVERSE, 0);
}

int test_tester_run_write_read_random(void)
{
	return tester_run_write_read_with(TEST_MODE_RANDOM, 0);
}

int test_tester_run_write_read_fps(void)
{
	uint64_t start;
	uint64_t time;
	int res;

	start = tester_start();
	res = tester_run_write_read_with(TEST_MODE_NORM, 40);
	time = tester_stop(start);

	/* 5 frames, write and read totals 10 frames, 40 fps == 0.25 second */
	TEST_ASSERT(time >= (SEC_IN_NS / 4));

	return res;
}

int test_tester_result_aggregate(void)
{
	test_result_t a = {0};
	test_result_t b = {0};
	test_result_t c = {0};

	a.frames_written = 100;
	a.bytes_written = 424242;
	a.time_taken_ns = 0xfefefefe;

	b.frames_written = 42;
	b.bytes_written = 111111;
	b.time_taken_ns = 0x01010101;

	TEST_ASSERT(!test_result_aggregate(&c, &a));
	TEST_ASSERT_EQ(a.frames_written, 100);
	TEST_ASSERT_EQ(a.frames_written, c.frames_written);
	TEST_ASSERT_EQ(a.bytes_written, c.bytes_written);
	TEST_ASSERT_EQ(a.time_taken_ns, c.time_taken_ns);

	TEST_ASSERT(!test_result_aggregate(&c, &b));
	TEST_ASSERT_EQ(a.frames_written + b.frames_written, c.frames_written);
	TEST_ASSERT_EQ(a.bytes_written + b.bytes_written, c.bytes_written);
	TEST_ASSERT_EQ(a.time_taken_ns + b.time_taken_ns, c.time_taken_ns);

	return 0;
}

int test_tester(void)
{
	TEST_INIT();

	tester_platform = test_platform_get();

	TEST(tester_start_stop);
	TEST(tester_run_write_read);
	test_platform_finalize();
	TEST(tester_run_write_read_reverse);
	test_platform_finalize();
	TEST(tester_run_write_read_random);
	test_platform_finalize();
	TEST(tester_result_aggregate);
	TEST(tester_run_write_read_fps);
	test_platform_finalize();

	test_platform_finalize();

	TEST_END();
}
