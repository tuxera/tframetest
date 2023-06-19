/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include "profile.h"
#include "frame.h"
#include "tester.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	profile_t prof;
	frame_t *frm;
	test_result_t res;

	prof = profile_get_by_name("2K-24bit");

	frm = frame_gen(prof);
	if (!frm) {
		fprintf(stderr, "Can't allocate frame\n");
		return 1;
	}

	printf("Profile name: %s\n", frm->profile.name);
	res = tester_run_write(".", frm, 2000);

	printf("Result:\n");
	printf(" frames: %lu\n", res.frames_written);
	printf(" bytes : %lu\n", res.bytes_written);
	printf(" time  : %lu\n", res.time_taken_ns);
	printf(" fps   : %lf\n", (double)res.frames_written * SEC_IN_NS
			/ res.time_taken_ns);
	printf(" MiB/s : %lf\n", (double)(res.bytes_written * SEC_IN_NS)
			/ (1024 * 1024)
			/ res.time_taken_ns);

	return 0;
}
