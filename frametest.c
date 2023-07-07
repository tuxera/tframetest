/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "profile.h"
#include "frame.h"
#include "tester.h"
#include "histogram.h"
#include "frametest.h"
#include "report.h"
#include "platform.h"

typedef struct thread_info_t {
	size_t id;
	uint64_t thread;

	const platform_t *platform;
	const opts_t *opts;
	test_result_t res;

	size_t start_frame;
	size_t frames;
	size_t fps;
} thread_info_t;

void *run_write_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;
	info->res = tester_run_write(info->platform, info->opts->path,
			info->opts->frm, info->start_frame, info->frames,
			info->fps, mode);

	return NULL;
}

void *run_read_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_MODE_NORM;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_MODE_REVERSE;
	else if (info->opts->random)
		mode = TEST_MODE_RANDOM;
	info->res = tester_run_read(info->platform, info->opts->path,
			info->opts->frm, info->start_frame, info->frames,
			info->fps, mode);

	return NULL;
}

void calculate_frame_range(thread_info_t *threads, const opts_t *opts)
{
	size_t i;
	uint64_t start_frame;
	uint64_t frames_per_thread;
	uint64_t frames_left;
	uint64_t fps_per_thread;
	uint64_t fps_left;

	frames_per_thread = opts->frames / opts->threads;
	frames_left = opts->frames % opts->threads;

	fps_per_thread = opts->fps / opts->threads;
	fps_left = opts->fps % opts->threads;

	start_frame = 0;

	for (i = 0; i < opts->threads; i++) {
		threads[i].start_frame = start_frame;
		threads[i].frames = frames_per_thread;
		threads[i].fps = fps_per_thread;
		if (frames_left) {
			++threads[i].frames;
			--frames_left;
		}
		if (fps_left) {
			++threads[i].fps;
			--fps_left;
		}
		start_frame += threads[i].frames;
	}
}

int run_test_threads(const platform_t *platform, const char *tst,
		const opts_t *opts, void *(*tfunc)(void*))
{
	size_t i;
	int res;
	thread_info_t *threads;
	test_result_t tres = {0};
	uint64_t start;

	threads = platform->calloc(opts->threads, sizeof(*threads));
	if (!threads)
		return 1;

	calculate_frame_range(threads, opts);

	start = tester_start();
	for (i = 0; i < opts->threads; i++) {
		int res;

		threads[i].id = i;
		threads[i].platform = platform;
		threads[i].opts = opts;
		res = platform->thread_create(&threads[i].thread,
				tfunc, (void*)&threads[i]);
		if (res) {
			size_t j;
			void *ret;

			for (j = 0; j < i; j++)
				platform->thread_cancel(threads[j].thread);
			for (j = 0; j < i; j++)
				platform->thread_join(threads[j].thread, &ret);
			platform->free(threads);
			return 1;
		}
	}

	res = 0;
	for (i = 0; i < opts->threads; i++) {
		void *ret;

		if (platform->thread_join(threads[i].thread, &ret))
			res = 1;
		if (ret)
			res = 1;

#if 0
		print_results(&threads[i].res);
#endif
		if (test_result_aggregate(&tres, &threads[i].res))
		    res = 1;
		result_free(platform, &threads[i].res);
	}
	tres.time_taken_ns = tester_stop(start);
	if (!res) {
		if (opts->csv)
			print_results_csv(tst, opts, &tres);
		else {
			print_results(tst, opts, &tres);
			if (opts->histogram)
				print_histogram(&tres);
		}
	}
	result_free(platform, &tres);
	platform->free(threads);
	return res;
}

int run_tests(opts_t *opts)
{
	const platform_t *platform = NULL;

	if (!opts)
		return 1;

	platform = platform_get();
	if (opts->profile.prof == PROF_INVALID &&
			opts->prof != PROF_INVALID) {
		opts->profile = profile_get_by_type(opts->prof);
	}
	if (opts->mode & TEST_EMPTY)
		opts->profile = profile_get_by_name("empty");
	else if (opts->profile.prof == PROF_INVALID && opts->write_size) {
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
	opts->profile.header_size = (opts->mode & TEST_EMPTY)
			? 0 : opts->header_size;
	if (opts->mode & TEST_WRITE)
		opts->frm = frame_gen(platform, opts->profile);
	else if (opts->mode & TEST_READ) {
		if (!opts->frm) {
			opts->frm = tester_get_frame_read(platform, opts->path);
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
		print_header_csv(opts);

	if (opts->mode & TEST_WRITE) {
		if (!opts->frm) {
			fprintf(stderr, "Can't allocate frame\n");
			return 1;
		}
		run_test_threads(platform, "write", opts,
				&run_write_test_thread);
	}
	if (opts->mode & TEST_READ) {
		run_test_threads(platform, "read", opts, &run_read_test_thread);
	}
	frame_destroy(platform, opts->frm);

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

static inline int parse_arg_size_t(const char *arg, size_t *res, int zero_ok)
{
	char *endp = NULL;
	size_t val;

	if (!arg || !res)
		return 1;

	val = strtoll(arg, &endp, 10);
	if (!endp || *endp != 0)
		return 1;
	if (!zero_ok && !val)
		return 1;

	*res = val;

	return 0;
}

int opt_parse_threads(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->threads, 0);
}

int opt_parse_num_frames(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->frames, 0);
}

int opt_parse_limit_fps(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->fps, 0);
}

int opt_parse_header_size(opts_t *opt, const char *arg)
{
	return parse_arg_size_t(arg, &opt->header_size, 1);
}

void list_profiles(void)
{
	size_t cnt = profile_count();
	size_t i;

	printf("Profiles:\n");
	for (i = 1; i < cnt; i++) {
		profile_t prof = profile_get_by_index(i);

		printf("  %s\n", prof.name);
		printf("     %zux%zu, %zu bits, %zuB header\n",
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
	{ "empty", no_argument, 0, 'e' },
	{ "list-profiles", no_argument, 0, 'l' },
	{ "threads", required_argument, 0, 't' },
	{ "num-frames", required_argument, 0, 'n' },
	{ "fps", required_argument, 0, 'f' },
	{ "reverse", no_argument, 0, 'v' },
	{ "random", no_argument, 0, 'm' },
	{ "csv", no_argument, 0, 'c' },
	{ "no-csv-header", no_argument, 0, 0 },
	{ "header", required_argument, 0, 0 },
	{ "times", no_argument, 0, 0 },
	{ "frametimes", no_argument, 0, 0 },
	{ "histogram", no_argument, 0, 0 },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 },
};
static size_t long_opts_cnt = sizeof(long_opts) / sizeof(long_opts[0]);
static struct long_opt_desc long_opt_descs[] = {
	{ "write", "Perform write tests, size/profile as parameter"},
	{ "read", "Perform read tests"},
	{ "empty", "Perform write tests with empty frames"},
	{ "list-profiles", "List available profiles"},
	{ "threads", "Use number of threads (default 1)"},
	{ "num-frames", "Write number of frames (default 1800)" },
	{ "fps", "Limit frame rate to frames per second" },
	{ "reverse", "Access files in reverse order" },
	{ "random", "Access files in random order" },
	{ "csv", "Output results in CSV format" },
	{ "no-csv-header", "Do not print CSV header" },
	{ "header", "Frame header size (default 64k)" },
	{ "times", "Show breakdown of completion times (open/io/close)" },
	{ "frametimes", "Show detailed timings of every frames in CSV format" },
	{ "histogram", "Show histogram of completion times at the end" },
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
	int c = 0;
	int opt_index = 0;

	srand(time(NULL));
	opts.threads = 1;
	opts.frames = 1800;
	opts.header_size = 65536;
	while (1) {

		c = getopt_long(argc, argv, "rw:elt:n:f:vmhc",
				long_opts, &opt_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (!strcmp(long_opts[opt_index].name, "no-csv-header"))
				opts.no_csv_header = 1;
			if (!strcmp(long_opts[opt_index].name, "histogram"))
				opts.histogram = 1;
			if (!strcmp(long_opts[opt_index].name, "times"))
				opts.times = 1;
			if (!strcmp(long_opts[opt_index].name, "frametimes"))
				opts.frametimes = 1;
			if (!strcmp(long_opts[opt_index].name, "header")) {
				if (opt_parse_header_size(&opts, optarg))
					goto invalid_long;
			}
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		case 'c':
			opts.csv = 1;
			break;
		case 'v':
			opts.reverse = 1;
			break;
		case 'm':
			opts.random = 1;
			break;
		case 'w':
			if (opt_parse_write(&opts, optarg)) {
				if (opt_parse_profile(&opts, optarg)) {
					/* Could not parse profile, just skip */
				}
			}
			opts.mode |= TEST_WRITE;
			break;
		case 'e':
			opts.mode |= TEST_WRITE;
			opts.mode |= TEST_EMPTY;
			break;
		case 'r':
			opts.mode |= TEST_READ;
			break;
		case 't':
			if (opt_parse_threads(&opts, optarg))
				goto invalid_short;
			break;
		case 'n':
			if (opt_parse_num_frames(&opts, optarg))
				goto invalid_short;
			break;
		case 'f':
			if (opt_parse_limit_fps(&opts, optarg))
				goto invalid_short;
			break;
		case 'l':
			list_profiles();
			return 0;
		default:
			printf("Invalid option: %c\n", c);
			return 1;
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
	if (opts.random && opts.reverse) {
		printf("ERROR: --random and --reverse are mutually exclusive, "
		       "please define only one.\n");
		usage(argv[0]);
		return 1;
	}
	if (!opts.path) {
		usage(argv[0]);
		return 1;
	}

	return run_tests(&opts);

invalid_long:
	fprintf(stderr, "Invalid argument for option --%s: %s\n",
			long_opts[opt_index].name, optarg);
	return 1;

invalid_short:
	fprintf(stderr, "Invalid argument for option " "-%c: %s\n", c, optarg);
	return 1;
}
