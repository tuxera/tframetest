/*
 * This file is part of tframetest.
 *
 * Copyright (c) 2023-2025 Tuxera Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "tester.c"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "unittest.h"
#include "tester.h"
#include "timing.h"
#include "frame.h"

#define SLEEP_TIME 1000UL

size_t frame_write(const platform_t *platform, platform_handle_t f,
		   frame_t *frame)
{
	(void)platform;
	(void)f;
	(void)frame;
	return sizeof(*frame);
}

size_t frame_read(const platform_t *platform, platform_handle_t f,
		  frame_t *frame)
{
	(void)platform;
	(void)f;
	(void)frame;
	return sizeof(*frame);
}

frame_t *frame_from_file(const platform_t *platform, const char *fname,
			 size_t header_size)
{
	(void)platform;
	(void)fname;
	return (frame_t *)calloc(1, sizeof(frame_t));
}

frame_t *frame_gen(const platform_t *platform, profile_t profile)
{
	(void)platform;
	(void)profile;
	return (frame_t *)calloc(1, sizeof(frame_t));
}

void frame_destroy(const platform_t *platform, frame_t *frame)
{
	(void)platform;
	free(frame);
}

profile_t profile_get_by_index(size_t idx)
{
	profile_t p = { "SD-32bit-cmp", PROF_SD, 720, 480, 4, 0 };
	(void)idx;
	return p;
}

static uint64_t monotonic_fake_time = 0;
uint64_t timing_time(void)
{
	return ++monotonic_fake_time;
}

int usleep(useconds_t usec)
{
	monotonic_fake_time += usec;
	return 0;
}

void test_setup(void **state)
{
	const platform_t *frame_platform = test_platform_get();
	*state = (void *)frame_platform;
}

void test_teardown(void **state)
{
	test_platform_finalize();
}

int test_timing_start_elapsed(void)
{
	uint64_t start;
	uint64_t time;
	uint64_t end;

	start = timing_start();
	usleep(SLEEP_TIME);
	time = timing_elapsed(start);
	end = timing_start();

	TEST_ASSERT(end > start);
	TEST_ASSERT(time != 0);
	TEST_ASSERT(time >= SLEEP_TIME);

	return 0;
}

static frame_t *gen_default_frame(const platform_t *platform)
{
	profile_t prof;

	prof = profile_get_by_index(1);
	return frame_gen(platform, prof);
}

static int tester_run_write_read_with(const platform_t *platform,
				      test_mode_t mode, size_t fps)
{
	const size_t frames = 5;
	test_result_t res;
	test_result_t res_read;
	platform_handle_t f;
	frame_t *frm;
	frame_t *frm_res;

	frm = gen_default_frame(platform);
	TEST_ASSERT(frm);

	f = platform->open("./frame000000.tst", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_EQ(f, -1);

	res = tester_run_write(platform, ".", frm, 0, frames, fps, mode,
			       TEST_FILES_MULTIPLE);

	TEST_ASSERT_EQ(res.frames_written, frames);
	TEST_ASSERT_EQ(res.bytes_written, frames * frm->size);
	TEST_ASSERT(res.completion);

	result_free(platform, &res);

	f = platform->open("./frame000000.tst", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_NE(f, -1);
	platform->close(f);

	frm_res = tester_get_frame_read(platform, ".", 0);
	TEST_ASSERT(frm_res);

	res_read = tester_run_read(platform, ".", frm_res, 0, frames, fps, mode,
				   TEST_FILES_MULTIPLE);
	TEST_ASSERT_EQ(res_read.frames_written, frames);
	TEST_ASSERT_EQ(res_read.bytes_written, frames * frm_res->size);
	TEST_ASSERT(res_read.completion);

	result_free(platform, &res_read);

	frame_destroy(platform, frm);
	frame_destroy(platform, frm_res);
	return 0;
}

int test_tester_run_write_read(void **state)
{
	return tester_run_write_read_with(*state, TEST_MODE_NORM, 0);
}

int test_tester_run_write_read_reverse(void **state)
{
	return tester_run_write_read_with(*state, TEST_MODE_REVERSE, 0);
}

int test_tester_run_write_read_random(void **state)
{
	return tester_run_write_read_with(*state, TEST_MODE_RANDOM, 0);
}

int test_tester_run_write_read_fps(void **state)
{
	uint64_t start;
	uint64_t time;
	int res;

	start = timing_start();
	res = tester_run_write_read_with(*state, TEST_MODE_NORM, 40);
	time = timing_elapsed(start);

	/* 5 frames, write and read totals 10 frames, 40 fps == 0.25 second */
	TEST_ASSERT(time >= (SEC_IN_NS / 4));

	return res;
}

int test_tester_run_write_read_single_file(void **state)
{
	const platform_t *platform = *state;
	const size_t frames = 5;
	test_result_t res;
	test_result_t res_read;
	platform_handle_t f;
	frame_t *frm;
	frame_t *frm_res;

	frm = gen_default_frame(platform);
	TEST_ASSERT(frm);

	f = platform->open("./single", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_EQ(f, -1);

	res = tester_run_write(platform, "./single", frm, 0, frames, 0,
			       TEST_MODE_NORM, TEST_FILES_SINGLE);
	TEST_ASSERT_EQ(res.frames_written, frames);
	TEST_ASSERT_EQ(res.bytes_written, frames * frm->size);
	TEST_ASSERT(res.completion);

	result_free(platform, &res);

	f = platform->open("./single", PLATFORM_OPEN_READ, 0);
	TEST_ASSERT_NE(f, -1);
	platform->close(f);

	frm_res = gen_default_frame(platform);

	res_read = tester_run_read(platform, ".", frm_res, 0, frames, 0,
				   TEST_MODE_NORM, TEST_FILES_SINGLE);
	TEST_ASSERT_EQ(res_read.frames_written, frames);
	TEST_ASSERT_EQ(res_read.bytes_written, frames * frm_res->size);
	TEST_ASSERT(res_read.completion);

	result_free(platform, &res_read);

	frame_destroy(platform, frm);
	frame_destroy(platform, frm_res);

	return 0;
}

int test_tester_result_aggregate(void)
{
	test_result_t a = { 0 };
	test_result_t b = { 0 };
	test_result_t c = { 0 };

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

	TEST(timing_start_elapsed);
	TESTF(tester_run_write_read, test_setup, test_teardown);
	TESTF(tester_run_write_read_reverse, test_setup, test_teardown);
	TESTF(tester_run_write_read_random, test_setup, test_teardown);
	TESTF(tester_run_write_read_single_file, test_setup, test_teardown);
	TEST(tester_result_aggregate);
	TESTF(tester_run_write_read_fps, test_setup, test_teardown);

	TEST_END();
}

TEST_MAIN(tester)
