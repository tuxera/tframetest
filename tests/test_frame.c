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

#include "frame.c"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include "profile.h"
#include "unittest.h"
#include "frame.h"

static profile_t default_test_profile = {
	"SD-32bit-cmp", PROF_SD, 720, 480, 4, 0
};

profile_t profile_get_by_name(const char *name)
{
	(void)name;
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

size_t profile_count(void)
{
	return 1;
}

size_t profile_size(const profile_t *profile)
{
	(void)profile;
	return default_test_profile.width * default_test_profile.height *
		       default_test_profile.bytes_per_pixel +
	       default_test_profile.header_size;
}

profile_t profile_get_by_frame_size(size_t header_size, size_t size)
{
	(void)header_size;
	(void)size;
	return default_test_profile;
}

void test_setup(void **state)
{
	*state = (void *)test_platform_get();
}

void test_teardown(void **state)
{
	test_platform_finalize();
}

static frame_t *gen_default_frame(const platform_t *platform)
{
	profile_t prof;

	prof = profile_get_by_index(1);
	return frame_gen(platform, prof);
}

int test_frame_gen(void **state)
{
	const platform_t *platform = *state;
	frame_t *frm;

	frm = gen_default_frame(platform);
	TEST_ASSERT(frm);

	frame_destroy(platform, frm);

	return 0;
}

int test_frame_fill(void **state)
{
	const platform_t *platform = *state;
	frame_t *frm;
	size_t i;

	frm = gen_default_frame(platform);
	TEST_ASSERT(frm);

	/* Default fill with 't' */
	for (i = 0; i < frm->size; i++)
		TEST_ASSERT_EQI(i, ((unsigned char *)frm->data)[i], 't');

	/* Test fill works */
	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	for (i = 0; i < frm->size; i++)
		TEST_ASSERT_EQI(i, ((unsigned char *)frm->data)[i], 0x42);

	frame_destroy(platform, frm);

	return 0;
}

int test_frame_write_read(void **state)
{
	const platform_t *platform = *state;
	frame_t *frm;
	frame_t *frm_read;
	size_t fw;
	int fd;

	frm = gen_default_frame(platform);
	frm_read = gen_default_frame(platform);
	TEST_ASSERT(frm);
	TEST_ASSERT(frm_read);

	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	TEST_ASSERT_EQ(((unsigned char *)frm->data)[0], 0x42);
	TEST_ASSERT_EQ(((unsigned char *)frm_read->data)[0], 't');

	fd = platform->open("tst1",
			    PLATFORM_OPEN_WRITE | PLATFORM_OPEN_CREATE |
				    PLATFORM_OPEN_DIRECT,
			    0666);
	fw = frame_write(platform, fd, frm);
	TEST_ASSERT_EQ(fw, frm->size);
	platform->close(fd);

	TEST_ASSERT_EQ(((unsigned char *)frm_read->data)[0], 't');

	fd = platform->open("tst1", PLATFORM_OPEN_READ | PLATFORM_OPEN_DIRECT,
			    0666);
	fw = frame_read(platform, fd, frm_read);
	TEST_ASSERT_EQ(fw, frm->size);

	TEST_ASSERT_EQ(((unsigned char *)frm->data)[0], 0x42);
	TEST_ASSERT_EQ(((unsigned char *)frm_read->data)[0], 0x42);
	platform->close(fd);

	frame_destroy(platform, frm);
	frame_destroy(platform, frm_read);

	return 0;
}

int test_frame_from_file(void **state)
{
	const platform_t *platform = *state;
	frame_t *frm;
	int fd;

	frm = frame_from_file(platform, "tst1", 0);
	TEST_ASSERT(!frm);

	frm = gen_default_frame(platform);
	fd = platform->open("tst2",
			    PLATFORM_OPEN_WRITE | PLATFORM_OPEN_CREATE |
				    PLATFORM_OPEN_DIRECT,
			    0666);
	(void)frame_write(platform, fd, frm);
	platform->close(fd);
	frame_destroy(platform, frm);

	frm = frame_from_file(platform, "tst2", 0);
	TEST_ASSERT(frm);
	frame_destroy(platform, frm);

	frm = gen_default_frame(platform);
	fd = platform->open("tst3",
			    PLATFORM_OPEN_WRITE | PLATFORM_OPEN_CREATE |
				    PLATFORM_OPEN_DIRECT,
			    0666);
	(void)frame_write(platform, fd, NULL);
	platform->close(fd);
	frame_destroy(platform, frm);

	frm = frame_from_file(platform, "tst3", 0);
	TEST_ASSERT(frm);

	frame_destroy(platform, frm);

	return 0;
}

int test_frame(void)
{
	TEST_INIT();

	TESTF(frame_gen, test_setup, test_teardown);
	TESTF(frame_fill, test_setup, test_teardown);
	TESTF(frame_write_read, test_setup, test_teardown);
	TESTF(frame_from_file, test_setup, test_teardown);

	TEST_END();
}

TEST_MAIN(frame)
