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

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include "frametest.h"

enum CompletionStat {
	COMP_FRAME = 0,
	COMP_OPEN,
	COMP_IO,
	COMP_CLOSE,
};

static void print_stat_about(const test_result_t *res, const char *label,
			     enum CompletionStat stat, int csv)
{
	uint64_t min = SIZE_MAX;
	uint64_t max = 0;
	uint64_t total = 0;
	size_t i;

	for (i = 0; i < res->frames_written; i++) {
		size_t val;

		switch (stat) {
		case COMP_OPEN:
			val = res->completion[i].open;
			val -= res->completion[i].start;
			break;
		case COMP_IO:
			val = res->completion[i].io;
			val -= res->completion[i].open;
			break;
		case COMP_CLOSE:
			val = res->completion[i].close;
			val -= res->completion[i].io;
			break;
		default:
		case COMP_FRAME:
			val = res->completion[i].frame;
			val -= res->completion[i].start;
			break;
		}

		if (val < min)
			min = val;
		if (val > max)
			max = val;
		total += val;
	}
	if (csv) {
		printf("%" PRIu64 ",", min);
		printf("%lf,", (double)total / res->frames_written);
		printf("%" PRIu64 ",", max);
	} else {
		printf("%s:\n", label);
		printf(" min   : %lf ms\n", (double)min / SEC_IN_MS);
		printf(" avg   : %lf ms\n",
		       (double)total / res->frames_written / SEC_IN_MS);
		printf(" max   : %lf ms\n", (double)max / SEC_IN_MS);
	}
}

static void print_frames_stat(const test_result_t *res, const opts_t *opts)
{
	if (!res->completion) {
		if (opts->csv)
			printf(",,,");
		return;
	}

	if (opts->csv) {
		print_stat_about(res, "", COMP_FRAME, 1);
		if (opts->times) {
			print_stat_about(res, "", COMP_OPEN, 1);
			print_stat_about(res, "", COMP_IO, 1);
			print_stat_about(res, "", COMP_CLOSE, 1);
		}
	} else {
		print_stat_about(res, "Completion times", COMP_FRAME, 0);
		if (opts->times) {
			print_stat_about(res, "Open times", COMP_OPEN, 0);
			print_stat_about(res, "I/O times", COMP_IO, 0);
			print_stat_about(res, "Close times", COMP_CLOSE, 0);
		}
	}
}

static void print_frame_times(const test_result_t *res, const opts_t *opts)
{
	if (!opts->frametimes)
		return;
	size_t i;

	printf("frame,start,open,io,close,frame\n");
	for (i = 0; i < res->frames_written; i++) {
		printf("%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
		       ",%" PRIu64 "\n",
		       i, res->completion[i].start, res->completion[i].open,
		       res->completion[i].io, res->completion[i].close,
		       res->completion[i].frame);
	}
}

void print_results(const char *tcase, const opts_t *opts,
		   const test_result_t *res)
{
	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("Results %s:\n", tcase);
	printf(" frames: %" PRIu64 "\n", res->frames_written);
	printf(" bytes : %" PRIu64 "\n", res->bytes_written);
	printf(" time  : %" PRIu64 "\n", res->time_taken_ns);
	printf(" fps   : %lf\n",
	       (double)res->frames_written * SEC_IN_NS / res->time_taken_ns);
	printf(" B/s   : %lf\n",
	       (double)res->bytes_written * SEC_IN_NS / res->time_taken_ns);
	printf(" MiB/s : %lf\n", (double)res->bytes_written * SEC_IN_NS /
					 (1024 * 1024) / res->time_taken_ns);
	print_frames_stat(res, opts);
	print_frame_times(res, opts);
}

void print_header_csv(const opts_t *opts)
{
	const char *extra = "";

	if (opts->times)
		extra = ",omin,oavg,omax,iomin,ioavg,iomax,cmin,cavg,cmax";

	printf("case,profile,threads,frames,bytes,time,fps,bps,mibps,"
	       "fmin,favg,fmax%s\n",
	       extra);
}

void print_results_csv(const char *tcase, const opts_t *opts,
		       const test_result_t *res)
{
	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("\"%s\",", tcase);
	printf("\"%s\",", opts->profile.name);
	printf("%zu,", opts->threads);
	printf("%" PRIu64 ",", res->frames_written);
	printf("%" PRIu64 ",", res->bytes_written);
	printf("%" PRIu64 ",", res->time_taken_ns);
	printf("%lf,",
	       (double)res->frames_written * SEC_IN_NS / res->time_taken_ns);
	printf("%lf,",
	       (double)res->bytes_written * SEC_IN_NS / res->time_taken_ns);
	printf("%lf,", (double)res->bytes_written * SEC_IN_NS / (1024 * 1024) /
			       res->time_taken_ns);
	print_frames_stat(res, opts);
	printf("\n");
	print_frame_times(res, opts);
}
