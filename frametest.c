/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "frame.h"
#include "tester.h"

void print_results(const test_result_t *res)
{
	if (!res)
		return;

	printf("Result:\n");
	printf(" frames: %lu\n", res->frames_written);
	printf(" bytes : %lu\n", res->bytes_written);
	printf(" time  : %lu\n", res->time_taken_ns);
	printf(" fps   : %lf\n", (double)res->frames_written * SEC_IN_NS
			/ res->time_taken_ns);
	printf(" MiB/s : %lf\n", (double)(res->bytes_written * SEC_IN_NS)
			/ (1024 * 1024)
			/ res->time_taken_ns);
}

int run_tests(void)
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
	res = tester_run_write(".", frm, 200);

	print_results(&res);

	return 0;
}

enum TestMode {
	TEST_WRITE = 1 << 0,
	TEST_READ  = 1 << 1,
};

typedef struct opts_t {
	enum TestMode mode;

	enum ProfileType prof;
	size_t write_size;
	profile_t profile;
} opts_t;

int opt_parse_write(opts_t *opt, const char *arg)
{
	char *endp = NULL;

	if (!strcmp(arg, "sd") || !strcmp(arg, "SD")) {
		opt->prof = PROF_SD;
		return 0;
	} else if (!strcmp(arg, "hd") || !strcmp(arg, "HD")) {
		opt->prof = PROF_HD;
		return 0;
	} else if (!strcmp(arg, "fullhd") || !strcmp(arg, "FULLHD")) {
		opt->prof = PROF_FULLHD;
		return 0;
	} else if (!strcmp(arg, "2k") || !strcmp(arg, "2K")) {
		opt->prof = PROF_2K;
		return 0;
	} else if (!strcmp(arg, "4k") || !strcmp(arg, "4K")) {
		opt->prof = PROF_4K;
		return 0;
	} else if (!strcmp(arg, "8k") || !strcmp(arg, "8K")) {
		opt->prof = PROF_8K;
		return 0;
	}

	opt->write_size = strtoll(arg, &endp, 10);
	if (endp && *endp == 0 && opt->write_size)
		return 0;

	return 1;
}

int opt_parse_profile(opts_t *opt, const char *arg)
{
	profile_t prof;

	if (!arg)
		return 1;

	prof = profile_get_by_name(arg);
	if (prof.prof == PROF_INVALID)
		return 1;
	opt->profile = prof;
	return 0;
}

void list_profiles(void)
{
	size_t cnt = profile_count();
	size_t i;

	printf("Profiles:\n");
	for (i = 1; i < cnt; i++) {
		profile_t prof = profile_get_by_index(i);

		printf("  %s\n", prof.name);
	}
}

int main(int argc, char **argv)
{
	static struct option long_opts[] = {
		{ "write", required_argument, 0, 'w' },
		{ "read", no_argument, 0, 'r' },
		{ "profile", required_argument, 0, 'p' },
		{ "list-profiles", no_argument, 0, 'l' },
		{ 0, 0, 0, 0 },
	};
	opts_t opts = {0};

	while (1) {
		int c;
		int opt_index;

		c = getopt_long(argc, argv, "rw:p:l",
				long_opts, &opt_index);
		printf("da: %c %d, %d\n", c, c, opt_index);
		if (c == -1)
			break;
		switch (c) {
		case 'w':
			if (opt_parse_write(&opts, optarg)) {
				fprintf(stderr, "Invalid argument for option "
						"%c: %s\n", c, optarg);
				return 1;
			}
			opts.mode |= TEST_WRITE;
			break;
		case 'r':
			opts.mode |= TEST_READ;
			break;
		case 'p':
			if (opt_parse_profile(&opts, optarg)) {
				fprintf(stderr, "Invalid argument for option "
						"%c: %s\n", c, optarg);
				return 1;
			}
			break;
		case 'l':
			list_profiles();
			return 0;
		default:
			printf("GOT: %c\n", c);
			break;
		}
	}
	printf("mode:%s%s\n",
		opts.mode & TEST_WRITE ? " write" : "",
		opts.mode & TEST_READ ? " read" : "");
	printf("opt1: %lu\n", opts.write_size);
	printf("opt2: %u\n", (unsigned int)opts.prof);

	return 0;

	return run_tests();
}
