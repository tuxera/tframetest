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

#ifndef assert
#include <assert.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include "tester.h"

/* Histogram buckets in ns */
static uint64_t buckets[] = {
	0,	   200000,    500000,	  1000000,  2000000,
	5000000,   10000000,  20000000,	  50000000, 100000000,
	200000000, 500000000, 1000000000,
};
static char *bucket_label[] = {
	" 0 ", ".2 ", ".5 ", " 1 ", " 2 ", " 5 ", "10 ",
	"20 ", "50 ", "100", "200", "500", ">1s", "ovf",
};
#define buckets_cnt (sizeof(buckets) / sizeof(*buckets))
#define SUB_BUCKET_CNT 5
#define HISTOGRAM_HEIGHT 10

static inline size_t time_get_bucket(uint64_t time)
{
	size_t i;

	for (i = 0; i < buckets_cnt; i++) {
		if (time <= buckets[i])
			return i > 0 ? i - 1 : i;
	}

	return buckets_cnt - 1;
}

static inline size_t time_get_sub_bucket(size_t bucket, uint64_t time)
{
	uint64_t min;
	uint64_t max;

	min = buckets[bucket];
	if (bucket + 1 < buckets_cnt)
		max = buckets[bucket + 1];
	else
		max = min;

	if (max - min == 0)
		return 0;
	assert(time >= min && time < max);
	if (time < min)
		return 0;
	if (time > max)
		return SUB_BUCKET_CNT - 1;

	time = time - min;

	return (time * SUB_BUCKET_CNT) / (max - min + 1);
}

static inline void hist_collect_cnts(const test_result_t *res, uint64_t *cnts)
{
	size_t i;

	/*
	 * Categorize the completion times in buckets and sub buckets.
	 * Every bucket has SUB_BUCKET_CNT sub buckets, so divide the result
	 * into proper one.
	 */
	for (i = 0; i < res->frames_written; i++) {
		size_t frametime =
			res->completion[i].frame - res->completion[i].start;
		size_t b = time_get_bucket(frametime);
		size_t sb = time_get_sub_bucket(b, frametime);

		++cnts[b * SUB_BUCKET_CNT + sb];
	}
}

static inline uint64_t hist_cnts_max(uint64_t *cnts, size_t sbcnt)
{
	uint64_t max = 0;
	size_t i;

	for (i = 0; i < sbcnt; i++) {
		if (cnts[i] > max)
			max = cnts[i];
	}

	return max;
}

static inline size_t hist_cnts(void)
{
	return SUB_BUCKET_CNT * buckets_cnt;
}

void print_histogram(const test_result_t *res)
{
	uint64_t cnts[SUB_BUCKET_CNT * (buckets_cnt + 1)] = { 0 };
	uint64_t max;
	size_t i, j;
	size_t sbcnt;

	if (!res->completion)
		return;

	sbcnt = hist_cnts();
	hist_collect_cnts(res, cnts);

	/* Resolve the maximum value for scale */
	max = hist_cnts_max(cnts, sbcnt);

	printf("\nCompletion times:\n");
	for (j = 0; j < HISTOGRAM_HEIGHT; j++) {
		printf("|");
		for (i = 0; i < sbcnt; i++) {
			uint64_t th;
			uint64_t rth;

			th = cnts[i] * HISTOGRAM_HEIGHT / (max + 1);
			rth = HISTOGRAM_HEIGHT - 1 - th;

			if (j > rth)
				printf("*");
			else if (cnts[i] && j == HISTOGRAM_HEIGHT - 1)
				printf("*");
			else
				printf(" ");
		}
		printf("\n");
	}
	printf("|");
	for (i = 0; i < sbcnt; i++) {
		if (i % SUB_BUCKET_CNT == 0)
			printf("|");
		else
			printf("-");
	}
	printf("\n");
	for (i = 0; i < sbcnt; i++) {
		size_t b;

		b = i / SUB_BUCKET_CNT;
		if (i % SUB_BUCKET_CNT == 0)
			printf("%s", bucket_label[b]);
		else if (i % SUB_BUCKET_CNT >= 3)
			printf(" ");
	}
	printf("\n");
}
