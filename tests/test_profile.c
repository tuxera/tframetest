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

#include "profile.c"
#include <stdio.h>
#include <string.h>
#include "profile.h"
#include "unittest.h"

int test_profile_count(void)
{
	size_t cnt;

	cnt = profile_count();

	TEST_ASSERT_NE(cnt, 0);
	TEST_ASSERT(cnt > 0);
	TEST_ASSERT(cnt < 0x100);

	return 0;
}

int test_profile_get_empty(void)
{
	profile_t prof;

	prof = profile_get_by_name("empty");

	TEST_ASSERT_EQ(prof.prof, PROF_CUSTOM);
	TEST_ASSERT_EQ_STR(prof.name, "empty");
	TEST_ASSERT_EQ(prof.width, 0);
	TEST_ASSERT_EQ(prof.height, 0);
	TEST_ASSERT_EQ(prof.bytes_per_pixel, 0);

	return 0;
}

int test_profile_get_by_name(void)
{
	profile_t prof;

	prof = profile_get_by_name("2K-24bit");

	TEST_ASSERT_EQ(prof.prof, PROF_2K);
	TEST_ASSERT_NE(prof.width, 0);
	TEST_ASSERT_NE(prof.height, 0);
	TEST_ASSERT_NE(prof.bytes_per_pixel, 0);

	return 0;
}

int test_profile_get_by_type(void)
{
	profile_t prof;
	profile_t prof2;

	prof = profile_get_by_type(PROF_INVALID);
	TEST_ASSERT_EQ(prof.prof, PROF_INVALID);

	prof = profile_get_by_type(PROF_4K);
	TEST_ASSERT_EQ(prof.prof, PROF_4K);
	TEST_ASSERT_NE(prof.width, 0);
	TEST_ASSERT_NE(prof.height, 0);
	TEST_ASSERT_NE(prof.bytes_per_pixel, 0);

	prof2 = profile_get_by_type(PROF_SD);
	TEST_ASSERT_EQ(prof2.prof, PROF_SD);
	TEST_ASSERT_NE(prof2.width, 0);
	TEST_ASSERT_NE(prof2.height, 0);
	TEST_ASSERT_NE(prof2.bytes_per_pixel, 0);

	/* 4k profile should be bigger than SD */
	TEST_ASSERT(prof.width > prof2.width);
	TEST_ASSERT(prof.height > prof2.height);

	return 0;
}

int test_profile_get_by_index(void)
{
	profile_t prof;

	/* First one should be invalid */
	prof = profile_get_by_index(0);
	TEST_ASSERT_EQ(prof.prof, PROF_INVALID);

	/* Second one should NOT be invalid */
	prof = profile_get_by_index(1);
	TEST_ASSERT_NE(prof.prof, PROF_INVALID);

	/* Out of count should be invalid */
	prof = profile_get_by_index(profile_count() + 1);
	TEST_ASSERT_EQ(prof.prof, PROF_INVALID);

	return 0;
}

int test_profile(void)
{
	TEST_INIT();

	TEST(profile_count);
	TEST(profile_get_empty);
	TEST(profile_get_by_name);
	TEST(profile_get_by_type);
	TEST(profile_get_by_index);

	TEST_END();
}

TEST_MAIN(profile)
