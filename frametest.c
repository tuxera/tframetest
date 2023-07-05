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
	const char *path;

	size_t threads;
	size_t frames;

	unsigned int csv : 1;
	unsigned int no_csv_header : 1;
} opts_t;

typedef struct thread_info_t {
	size_t id;
	pthread_t thread;
	const opts_t *opts;
	test_result_t res;
	size_t start_frame;
	size_t frames;
} thread_info_t;

void print_frames_stat(const test_result_t *res, int csv)
{
	uint64_t min = SIZE_MAX;
	uint64_t max = 0;
	uint64_t total = 0;
	size_t i;

	if (!res->completion) {
		if (csv)
			printf(",,,");
		return;
	}

	for (i = 0; i < res->frames_written; i++) {
		if (res->completion[i] < min)
			min = res->completion[i];
		if (res->completion[i] > max)
			max = res->completion[i];
		total += res->completion[i];
	}
	if (csv) {
		printf("%lu,", min);
		printf("%lf,", (double)total / res->frames_written);
		printf("%lu,", max);
	} else {
		printf("Completion times:\n");
		printf(" min   : %lf ms\n", (double)min / SEC_IN_MS);
		printf(" avg   : %lf ms\n", (double)total / res->frames_written
				/ SEC_IN_MS);
		printf(" max   : %lf ms\n", (double)max / SEC_IN_MS);
	}
}

void print_results(const char *tcase, const test_result_t *res)
{
	if (!res)
		return;
	if (!res->time_taken_ns)
		return;

	printf("Results %s:\n", tcase);
	printf(" frames: %lu\n", res->frames_written);
	printf(" bytes : %lu\n", res->bytes_written);
	printf(" time  : %lu\n", res->time_taken_ns);
	printf(" fps   : %lf\n", (double)res->frames_written * SEC_IN_NS
			/ res->time_taken_ns);
	printf(" B/s   : %lf\n", (double)res->bytes_written * SEC_IN_NS
			/ res->time_taken_ns);
	printf(" MiB/s : %lf\n", (double)res->bytes_written * SEC_IN_NS
			/ (1024 * 1024)
			/ res->time_taken_ns);
	print_frames_stat(res, 0);
}

void print_header_csv(void)
{
	printf(";case,profile,threads,frames,bytes,time,fps,bps,mibps,"
			"fmin,favg,fmax\n");
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
	printf("%lu,", opts->threads);
	printf("%lu,", res->frames_written);
	printf("%lu,", res->bytes_written);
	printf("%lu,", res->time_taken_ns);
	printf("%lf,", (double)res->frames_written * SEC_IN_NS
			/ res->time_taken_ns);
	printf("%lf,", (double)res->bytes_written * SEC_IN_NS
			/ res->time_taken_ns);
	printf("%lf,", (double)res->bytes_written * SEC_IN_NS
			/ (1024 * 1024)
			/ res->time_taken_ns);
	print_frames_stat(res, 1);
	printf("\n");
}

void *run_write_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	info->res = tester_run_write(info->opts->path, info->opts->frm,
			info->start_frame, info->frames);

	return NULL;
}

void *run_read_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	info->res = tester_run_read(info->opts->path, info->opts->frm,
			info->start_frame, info->frames);

	return NULL;
}

void calculate_frame_range(thread_info_t *threads, const opts_t *opts)
{
	size_t i;
	uint64_t start_frame;
	uint64_t frames_per_thread;
	uint64_t frames_left;

	frames_per_thread = opts->frames / opts->threads;
	frames_left = opts->frames % opts->threads;
	start_frame = 0;
	for (i = 0; i < opts->threads; i++) {
		threads[i].start_frame = start_frame;
		threads[i].frames = frames_per_thread;
		if (frames_left) {
			++threads[i].frames;
			--frames_left;
		}
		start_frame += threads[i].frames;
	}
}

int run_test_threads(const char *tst, const opts_t *opts, void *(*tfunc)(void*))
{
	size_t i;
	int res;
	thread_info_t *threads;
	test_result_t tres = {0};
	uint64_t start;

	threads = calloc(opts->threads, sizeof(*threads));
	if (!threads)
		return 1;

	calculate_frame_range(threads, opts);

	start = tester_start();
	for (i = 0; i < opts->threads; i++) {
		int res;

		threads[i].id = i;
		threads[i].opts = opts;
		res = pthread_create(&threads[i].thread, NULL,
				tfunc, (void*)&threads[i]);
		if (res) {
			size_t j;
			void *ret;

			for (j = 0; j < i; j++)
				pthread_cancel(threads[j].thread);
			for (j = 0; j < i; j++)
				pthread_join(threads[j].thread, &ret);
			free(threads);
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

#if 0
		print_results(&threads[i].res);
#endif
		if (test_result_aggregate(&tres, &threads[i].res))
		    res = 1;
		result_free(&threads[i].res);
	}
	tres.time_taken_ns = tester_stop(start);
	if (!res) {
		if (opts->csv)
			print_results_csv(tst, opts, &tres);
		else
			print_results(tst, &tres);
	}
	result_free(&tres);
	free(threads);
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
	if ((opts->mode & TEST_WRITE) && opts->profile.prof == PROF_INVALID) {
		fprintf(stderr, "No test profile found!\n");
		return 1;
	}
	if (opts->mode & TEST_WRITE)
		opts->frm = frame_gen(opts->profile);
	else if (opts->mode & TEST_READ) {
		if (!opts->frm) {
			opts->frm = tester_get_frame_read(opts->path);
		}
		if (!opts->frm) {
			fprintf(stderr, "Can't allocate frame\n");
			return 1;
		}
		opts->profile = opts->frm->profile;
	}
	if (!opts->csv)
		printf("Profile: %s\n", opts->profile.name);

	if (opts->csv && !opts->no_csv_header)
		print_header_csv();

	if (opts->mode & TEST_WRITE) {
		if (!opts->frm) {
			fprintf(stderr, "Can't allocate frame\n");
			return 1;
		}
		run_test_threads("write", opts, &run_write_test_thread);
	}
	if (opts->mode & TEST_READ) {
		run_test_threads("read", opts, &run_read_test_thread);
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
		printf("     %lux%lu, %lu bits, %luB header\n",
				prof.width, prof.height,
				prof.bytes_per_pixel * 8,
				prof.header_size);
	}
}

struct long_opt_desc {
	const char *name;
	const char *desc;
};
static struct option long_opts[] = {
	{ "write", required_argument, 0, 'w' },
	{ "read", no_argument, 0, 'r' },
	{ "profile", required_argument, 0, 'p' },
	{ "list-profiles", no_argument, 0, 'l' },
	{ "threads", required_argument, 0, 't' },
	{ "num-frames", required_argument, 0, 'n' },
	{ "csv", no_argument, 0, 'c' },
	{ "no-csv-header", no_argument, 0, 0 },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};
static size_t long_opts_cnt = sizeof(long_opts) / sizeof(long_opts[0]);
static struct long_opt_desc long_opt_descs[] = {
	{ "write", "Perform write tests"},
	{ "read", "Perform read tests"},
	{ "profile", "Select frame profile to use"},
	{ "list-profiles", "List available profiles"},
	{ "threads", "Use number of threads (default 1)"},
	{ "num-frames", "Write number of frames (default 1800)" },
	{ "csv", "Output results in CSV format" },
	{ "no-csv-header", "Do not print CSV header" },
	{ "help", "Display this help" },
	{ 0, 0 },
};

#define DESC_POS 30
void usage(const char *name)
{
	size_t i;

	fprintf(stderr, "Usage: %s [options] path\n", name);
	fprintf(stderr, "Options:\n");
	for (i = 0; i < long_opts_cnt; i++) {
		int p = 8;

		if (!long_opts[i].name)
			break;
		if (long_opts[i].val)
			fprintf(stderr, "    -%c, ", long_opts[i].val);
		else
			fprintf(stderr, "        ");

		p += strlen(long_opts[i].name);
		fprintf(stderr, "--%s", long_opts[i].name);

		if (p < DESC_POS)
			p = DESC_POS - p;
		else
			p = 1;
		fprintf(stderr, "%*s%s\n", p, " ",
				long_opt_descs[i].desc);
	}
}

int main(int argc, char **argv)
{
	opts_t opts = {0};

	opts.threads = 1;
	opts.frames = 1800;
	while (1) {
		int opt_index = 0;
		int c;

		c = getopt_long(argc, argv, "rw:p:lt:n:hc",
				long_opts, &opt_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (!strcmp(long_opts[opt_index].name, "no-csv-header"))
				opts.no_csv_header = 1;
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		case 'c':
			opts.csv = 1;
			break;
		case 'w':
			if (opt_parse_write(&opts, optarg)) {
				if (opt_parse_profile(&opts, optarg)) {
					/* Could not parse profile, just skip */
				}
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
	if (optind < argc) {
		int i;

		for (i = optind; i < argc; i++) {
			if (!opts.path)
				opts.path = argv[i];
			else {
				printf("Unknown option: %s\n", argv[i]);
				return 1;
			}
		}
	}
	if (!opts.path) {
		usage(argv[0]);
		return 1;
	}

	return run_tests(&opts);
}
