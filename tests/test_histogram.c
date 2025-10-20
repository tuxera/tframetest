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

#include "unittest.h"
#include "histogram.h"
#include "histogram.c"

int test_histogram_time_bucket(void)
{
	size_t bucket;

	bucket = time_get_bucket(0);
	TEST_ASSERT_EQ(bucket, 0);

	bucket = time_get_bucket(1);
	TEST_ASSERT_EQ(bucket, 0);

	bucket = time_get_bucket(200001);
	TEST_ASSERT_EQ(bucket, 1);

	bucket = time_get_bucket(2000001);
	TEST_ASSERT_EQ(bucket, 4);

	bucket = time_get_bucket(20000001);
	TEST_ASSERT_EQ(bucket, 7);

	bucket = time_get_bucket(200000001);
	TEST_ASSERT_EQ(bucket, 10);

	bucket = time_get_bucket(2000000001);
	TEST_ASSERT_EQ(bucket, 12);

	bucket = time_get_bucket(20000000001);
	TEST_ASSERT_EQ(bucket, 12);

	bucket = time_get_bucket(200000000001);
	TEST_ASSERT_EQ(bucket, 12);

	return 0;
}

int test_histogram_time_sub_bucket(void)
{
	size_t bucket;
	size_t assertcnt;

	EXPECT_ASSERTS(0);
	bucket = time_get_sub_bucket(0, 1);
	TEST_ASSERT_EQ(bucket, 0);

	bucket = time_get_sub_bucket(0, 40001);
	TEST_ASSERT_EQ(bucket, 1);

	bucket = time_get_sub_bucket(0, 80001);
	TEST_ASSERT_EQ(bucket, 2);

	bucket = time_get_sub_bucket(0, 120001);
	TEST_ASSERT_EQ(bucket, 3);

	bucket = time_get_sub_bucket(0, 199999);
	TEST_ASSERT_EQ(bucket, 4);

	/* This should cause and error */
	EXPECT_ASSERTS(1);
	bucket = time_get_sub_bucket(0, 2000000);
	/* Fallback is the last sub bucket */
	TEST_ASSERT_EQ(bucket, 4);

	//assertcnt = unittest_asserts;
	EXPECT_ASSERTS(1);
	bucket = time_get_sub_bucket(1, 0);
	/* Fallback is the first sub bucket */
	TEST_ASSERT_EQ(bucket, 0);

	assertcnt = unittest_asserts;
	bucket = time_get_sub_bucket(4, 2000200);
	TEST_ASSERT_EQ(assertcnt, unittest_asserts);

	return 0;
}

void gen_completions(test_result_t *res)
{
	size_t i;

	res->completion =
		malloc(sizeof(*res->completion) * res->frames_written);
	for (i = 0; i < res->frames_written; i++) {
		res->completion[i].start = 1;
		res->completion[i].open = 2;
		res->completion[i].io = (i * 1) * (i * 1) * (i * 1) * 100UL;
		res->completion[i].close = res->completion[i].io + 1;
		res->completion[i].frame = res->completion[i].close + 2;
	}
}

int test_histogram_collect_cnts(void)
{
	test_result_t res = {
		.frames_written = 100,
	};
	uint64_t cnts[SUB_BUCKET_CNT * (buckets_cnt + 1)] = { 0 };
	uint64_t expected[SUB_BUCKET_CNT * (buckets_cnt + 1)] = {
		8, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 1,
		1, 1, 2, 2, 2, 2, 1, 3, 2, 2, 1, 2, 3, 2, 3, 2, 2, 5,
		5, 4, 4, 3, 5, 4, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	size_t sbcnt;
	size_t i;
	size_t framecnt;

	gen_completions(&res);
	hist_collect_cnts(&res, cnts);
	framecnt = 0;
	sbcnt = hist_cnts();
	for (i = 0; i < sbcnt; i++)
		framecnt += cnts[i];
	TEST_ASSERT_EQ(framecnt, res.frames_written);

	for (i = 0; i < sbcnt; i++)
		TEST_ASSERT_EQI(i, cnts[i], expected[i]);

	free(res.completion);

	return 0;
}

int test_histogram_cnts_max()
{
	test_result_t res = {
		.frames_written = 100,
	};
	uint64_t cnts[SUB_BUCKET_CNT * (buckets_cnt + 1)] = { 0 };
	uint64_t max;
	size_t sbcnt;

	gen_completions(&res);
	hist_collect_cnts(&res, cnts);

	sbcnt = hist_cnts();
	max = hist_cnts_max(cnts, sbcnt);
	TEST_ASSERT_EQ(max, 8);

	free(res.completion);

	return 0;
}

int test_histogram_print()
{
	test_completion_t compl = {
		.start = 1,
		.open = 10,
		.io = 20,
		.close = 30,
		.frame = 1,
	};
	test_result_t res = {
		.frames_written = 1,
	};

	test_ignore_printf(1);

	print_histogram(&res);

	res.completion = &compl ;
	print_histogram(&res);

	test_ignore_printf(0);

	return 0;
}

int test_histogram(void)
{
	TEST_INIT();

	TEST(histogram_time_bucket);
	TEST(histogram_time_sub_bucket);
	TEST(histogram_collect_cnts);
	TEST(histogram_cnts_max);
	TEST(histogram_print);

	TEST_END();
}

TEST_MAIN(histogram)
