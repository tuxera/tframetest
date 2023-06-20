/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include <getopt.h>
#include <pthread.h>
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
	printf(" B/s   : %lf\n", (double)(res->bytes_written * SEC_IN_NS)
			/ res->time_taken_ns);
	printf(" MiB/s : %lf\n", (double)(res->bytes_written * SEC_IN_NS)
			/ (1024 * 1024)
			/ res->time_taken_ns);
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

	frame_t *frm;

	size_t threads;
	size_t frames;
} opts_t;

typedef struct thread_info_t {
	size_t id;
	pthread_t thread;
	const opts_t *opts;
	test_result_t res;
} thread_info_t;

void *run_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	size_t frames;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	frames = info->opts->frames / info->opts->threads;
	if (info->id == 0)
		frames += info->opts->frames % info->opts->threads;
	info->res = tester_run_write(".", info->opts->frm, frames);

	return NULL;
}

int run_test_threads(const opts_t *opts)
{
	size_t i;
	int res;
	thread_info_t *threads;
	test_result_t tres = {0};

	threads = calloc(opts->threads, sizeof(*threads));
	if (!threads)
		return 1;

	for (i = 0; i < opts->threads; i++) {
		int res;

		threads[i].id = i;
		threads[i].opts = opts;
		res = pthread_create(&threads[i].thread, NULL,
				&run_test_thread, (void*)&threads[i]);
		if (res) {
			size_t j;
			void *ret;

			for (j = 0; j < i; j++)
				pthread_cancel(threads[j].thread);
			for (j = 0; j < i; j++)
				pthread_join(threads[j].thread, &ret);
			return 1;
		}
	}

	res = 0;
	for (i = 0; i < opts->threads; i++) {
		void *ret;

		if (pthread_join(threads[i].thread, &ret))
			res = 1;
		if (ret)
			res = 1;

		if (test_result_aggregate(&tres, &threads[i].res))
		    res = 1;
	}
	if (!res)
		print_results(&tres);
	return res;
}

int run_tests(opts_t *opts)
{

	if (!opts)
		return 1;

	if (opts->profile.prof == PROF_INVALID &&
			opts->prof != PROF_INVALID) {
		opts->profile = profile_get_by_type(opts->prof);
	}
	if (opts->profile.prof == PROF_INVALID && opts->write_size) {
		opts->profile.prof = PROF_CUSTOM;
		opts->profile.name = "custom";

		/* Faking the size */
		opts->profile.width = opts->write_size;
		opts->profile.bytes_per_pixel = 1;
		opts->profile.height = 1;
		opts->profile.header_size = 0;
	}
	if (opts->profile.prof == PROF_INVALID) {
		fprintf(stderr, "No test profile found!\n");
		return 1;
	}
	printf("Profile: %s\n", opts->profile.name);

	opts->frm = frame_gen(opts->profile);
	if (!opts->frm) {
		fprintf(stderr, "Can't allocate frame\n");
		return 1;
	}

	if (opts->mode & TEST_WRITE) {
		if (opts->threads == 1) {
			test_result_t res;

			res = tester_run_write(".", opts->frm, opts->frames);
			print_results(&res);
		} else
			run_test_threads(opts);
	}
	frame_destroy(opts->frm);

	return 0;
}

int opt_parse_write(opts_t *opt, const char *arg)
{
	char *endp = NULL;
	size_t sz;

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

	sz = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0 || !sz)
		return 1;
	opt->write_size = sz;

	return 0;
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

int opt_parse_threads(opts_t *opt, const char *arg)
{
	char *endp = NULL;
	size_t th;

	if (!arg)
		return 1;

	th = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0 || !th)
		return 1;
	opt->threads = th;

	return 0;
}

int opt_parse_num_frames(opts_t *opt, const char *arg)
{
	char *endp = NULL;
	size_t th;

	if (!arg)
		return 1;

	th = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0 || !th)
		return 1;
	opt->frames = th;

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
		{ "threads", required_argument, 0, 't' },
		{ "num-frames", required_argument, 0, 'n' },
		{ 0, 0, 0, 0 },
	};
	opts_t opts = {0};

	opts.threads = 1;
	opts.frames = 1800;
	while (1) {
		int c;
		int opt_index;

		c = getopt_long(argc, argv, "rw:p:lt:n:",
				long_opts, &opt_index);
		if (c == -1)
			break;
		switch (c) {
		case 'w':
			if (opt_parse_write(&opts, optarg)) {
				if (opt_parse_profile(&opts, optarg)) {
					/* Could not parse profile, just skip */
				}
#if 0
				fprintf(stderr, "Invalid argument for option "
						"%c: %s\n", c, optarg);
				return 1;
#endif
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
		case 't':
			if (opt_parse_threads(&opts, optarg)) {
				fprintf(stderr, "Invalid argument for option "
						"%c: %s\n", c, optarg);
				return 1;
			}
			break;
		case 'n':
			if (opt_parse_num_frames(&opts, optarg)) {
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

	return run_tests(&opts);
}
