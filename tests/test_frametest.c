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

#define main testmain
#include "frametest.c"
#undef main
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include "timing.h"
#include "unittest.h"

static uint64_t monotonic_fake_time = 0;
uint64_t timing_time(void)
{
	return ++monotonic_fake_time;
}

static profile_t default_test_profile = {
	"2K-24bit", PROF_2K, 2048, 1080, 3, 0
};
#define DEFAULT_PROF_SIZE                                           \
	(default_test_profile.width * default_test_profile.height * \
	 default_test_profile.bytes_per_pixel)
static profile_t test_profile_empty = { "empty", PROF_CUSTOM, 0, 0, 0, 0 };
static profile_t test_profile_invalid = { "invalid", PROF_INVALID, 0, 0, 0, 0 };

profile_t profile_get_by_name(const char *name)
{
	if (strcmp(name, "empty") == 0)
		return test_profile_empty;
	return default_test_profile;
}

profile_t profile_get_by_type(enum ProfileType prof)
{
	(void)prof;
	return default_test_profile;
}

profile_t profile_get_by_index(size_t idx)
{
	(void)idx;
	return default_test_profile;
}

profile_t profile_get_by_frame_size(size_t header_size, size_t size)
{
	(void)header_size;
	(void)size;
	return test_profile_invalid;
}

size_t profile_count(void)
{
	return 1;
}

frame_t *frame_gen(const platform_t *platform, profile_t profile)
{
	frame_t *res;

	(void)platform;

	res = (frame_t *)calloc(1, sizeof(frame_t));
	if (res == NULL)
		return NULL;
	res->profile = profile;
	res->size = profile.width * profile.height * profile.bytes_per_pixel;
	res->size += profile.header_size;

	return res;
}

void frame_destroy(const platform_t *platform, frame_t *frame)
{
	(void)platform;
	free(frame);
}

frame_t *tester_get_frame_read(const platform_t *platform, const char *path,
			       size_t frame_size)
{
	(void)platform;
	(void)path;
	(void)frame_size;
	return frame_gen(platform, default_test_profile);
}

test_result_t tester_run_write(const platform_t *platform, const char *path,
			       frame_t *frame, size_t start_frame,
			       size_t frames, size_t fps, test_mode_t mode,
			       test_files_t files)
{
	test_result_t res = { 0 };

	(void)platform;
	(void)path;
	(void)frame;
	(void)start_frame;
	(void)frames;
	(void)fps;
	(void)mode;
	(void)files;

	return res;
}

test_result_t tester_run_read(const platform_t *platform, const char *path,
			      frame_t *frame, size_t start_frame, size_t frames,
			      size_t fps, test_mode_t mode, test_files_t files)
{
	test_result_t res = { 0 };

	(void)platform;
	(void)path;
	(void)frame;
	(void)start_frame;
	(void)frames;
	(void)fps;
	(void)mode;
	(void)files;

	return res;
}

void print_results(const char *tcase, const opts_t *opts,
		   const test_result_t *res)
{
	(void)tcase;
	(void)opts;
	(void)res;
}

void print_header_csv(const opts_t *opts)
{
	(void)opts;
}

void print_results_csv(const char *tcase, const opts_t *opts,
		       const test_result_t *res)
{
	(void)tcase;
	(void)opts;
	(void)res;
}

void print_histogram(const test_result_t *res)
{
	(void)res;
}

const platform_t *platform_get(void)
{
	return test_platform_get();
}

void test_setup(void **state)
{
	*state = (void *)test_platform_get();
}

void test_teardown(void **state)
{
	test_platform_finalize();
}

int test_run_resolve_opts_null(void **state)
{
	TEST_ASSERT_EQ(run_resolve_opts(NULL, NULL), 1);
	return 0;
}

int test_run_resolve_opts_null_opts(void **state)
{
	const platform_t *platform = *state;

	TEST_ASSERT_EQ(run_resolve_opts(platform, NULL), 1);

	return 0;
}

int test_run_resolve_opts_prof(void **state)
{
	const platform_t *platform = *state;
	opts_t opts = {
		.prof = default_test_profile.prof,
	};

	TEST_ASSERT_EQ(run_resolve_opts(platform, &opts), 0);
	TEST_ASSERT_EQ(opts.profile.prof, default_test_profile.prof);
	TEST_ASSERT(strcmp(opts.profile.name, "empty") != 0);
	TEST_ASSERT(!opts.frm);

	return 0;
}

int test_run_resolve_opts_empty(void **state)
{
	const platform_t *platform = *state;
	opts_t opts = {
		.mode = TEST_EMPTY,
	};

	TEST_ASSERT_EQ(run_resolve_opts(platform, &opts), 0);
	TEST_ASSERT_EQ(opts.profile.prof, PROF_CUSTOM);
	TEST_ASSERT_EQ(strcmp(opts.profile.name, "empty"), 0);
	TEST_ASSERT(!opts.frm);

	return 0;
}

int test_run_resolve_opts_write_prof(void **state)
{
	const platform_t *platform = *state;
	opts_t opts = {
		.prof = default_test_profile.prof,
		.mode = TEST_WRITE,
	};

	TEST_ASSERT_EQ(run_resolve_opts(platform, &opts), 0);
	TEST_ASSERT_EQ(opts.profile.prof, default_test_profile.prof);
	TEST_ASSERT(strcmp(opts.profile.name, "empty") != 0);
	TEST_ASSERT(opts.frm);
	TEST_ASSERT_EQ(opts.frm->size, DEFAULT_PROF_SIZE);

	return 0;
}

int test_run_resolve_opts_write_size(void **state)
{
	const platform_t *platform = *state;
	opts_t opts = {
		.prof = PROF_INVALID,
		.write_size = 8192,
		.mode = TEST_WRITE,
	};

	TEST_ASSERT_EQ(run_resolve_opts(platform, &opts), 0);
	TEST_ASSERT_EQ(opts.profile.prof, PROF_CUSTOM);
	TEST_ASSERT(strcmp(opts.profile.name, "empty") != 0);
	TEST_ASSERT(opts.frm);
	TEST_ASSERT_EQ(opts.frm->size, 8192);

	return 0;
}

int test_run_resolve_opts_write_single_size(void **state)
{
	const platform_t *platform = *state;
	opts_t opts = {
		.prof = PROF_INVALID,
		.single_file = 1,
		.frame_size = 8192,
		.mode = TEST_WRITE,
	};

	TEST_ASSERT_EQ(run_resolve_opts(platform, &opts), 0);
	TEST_ASSERT_EQ(opts.profile.prof, PROF_CUSTOM);
	TEST_ASSERT(strcmp(opts.profile.name, "empty") != 0);
	TEST_ASSERT(opts.frm);
	TEST_ASSERT_EQ(opts.frm->size, 8192);

	return 0;
}

int test_frametest(void)
{
	TEST_INIT();

	TESTF(run_resolve_opts_null, test_setup, test_teardown);
	TESTF(run_resolve_opts_null_opts, test_setup, test_teardown);
	TESTF(run_resolve_opts_prof, test_setup, test_teardown);
	TESTF(run_resolve_opts_empty, test_setup, test_teardown);
	TESTF(run_resolve_opts_write_prof, test_setup, test_teardown);
	TESTF(run_resolve_opts_write_size, test_setup, test_teardown);
	TESTF(run_resolve_opts_write_single_size, test_setup, test_teardown);

	TEST_END();
}

TEST_MAIN(frametest)
