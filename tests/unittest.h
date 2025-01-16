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

#ifndef FRAMETEST_UNITTEST_H
#define FRAMETEST_UNITTEST_H

extern unsigned long unittest_asserts;
#define assert(X) if (!(X)) {\
		if (!unittest_asserts)\
		printf("ASSERT: %sx @%s:%u\n", #X, __FILE__, __LINE__);\
		--unittest_asserts; }
#define INIT_ASSERT() unsigned long unittest_asserts = 0;
#define EXPECT_ASSERTS(X) unittest_asserts = (X);

#include "platform.h"

#define TEST_INIT() unsigned long ok = 0; unsigned long tests = 0;
#define TEST(X) ++tests; if (!test_ ## X ()) ++ok; else { \
	printf(" FAILED: %s\n", #X); return 1; }
#define TESTF(X, S, T) { void *state=NULL; S(&state); ++tests; \
	if (!test_ ## X (&state)) { ++ok; T(&state); } else { \
	printf(" FAILED: %s\n", #X); T(&state); return 1; }}
#define TEST_END() return tests == ok ? 0 : 1;

#define TEST_ASSERT(X) if (!(X)) { \
	printf(" ASSERT: %s @%s:%u\n", #X, __FILE__, __LINE__); return 1; }
#define TEST_ASSERT_EQI(I, X, Y) if ((X) != (Y)) {\
	printf(" ASSERT: %zu. %s == %s @%s:%u\n", (size_t)(I), #X, #Y, \
		__FILE__, __LINE__); return 1; }
#define TEST_ASSERT_EQ(X, Y) if ((X) != (Y)) {\
	printf(" ASSERT: %s == %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }
#define TEST_ASSERT_NE(X, Y) if ((X) == (Y)) {\
	printf(" ASSERT: %s != %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }
#define TEST_ASSERT_EQ_STR(X, Y) if (strcmp(X, Y)) {\
	printf(" ASSERT: %s == %s @%s:%u\n", #X, #Y, \
		__FILE__, __LINE__); return 1; }

const platform_t * test_platform_get(void);
void test_platform_finalize(void);
void test_ignore_printf(int val);

#define RUN_TEST(NAME)\
	extern int test_ ## NAME (void);\
	printf("Testing %s...\n", #NAME);\
	if (test_ ## NAME ()) { return 1;}

#define TEST_MAIN(NAME) \
INIT_ASSERT()\
int main(int argc, char **argv)\
{\
	(void)argc;\
	(void)argv;\
	RUN_TEST(NAME);\
}

#endif
