/*
 * Copyright (c) 2023 Tuxera Inc. All rights reserved.
 */

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "profile.h"
#include "frame.h"
#include "tester.h"

enum TestMode {
	TEST_WRITE = 1 << 0,
	TEST_READ  = 1 << 1,
	TEST_EMPTY = 1 << 2,
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
	size_t fps;
	size_t header_size;

	unsigned int reverse : 1;
	unsigned int random : 1;
	unsigned int csv : 1;
	unsigned int no_csv_header : 1;
	unsigned int histogram : 1;
} opts_t;

typedef struct thread_info_t {
	size_t id;
	pthread_t thread;

	const opts_t *opts;
	test_result_t res;

	size_t start_frame;
	size_t frames;
	size_t fps;
} thread_info_t;

/* Histogram buckets in ns */
static uint64_t buckets[] = {
                 0,
	    200000,
	    500000,
	   1000000,
	   2000000,
	   5000000,
	  10000000,
	  20000000,
	  50000000,
	 100000000,
	 200000000,
	 500000000,
	1000000000,
};
static char *bucket_label[] = {
	" 0 ",
	".2 ",
	".5 ",
	" 1 ",
	" 2 ",
	" 5 ",
	"10 ",
	"20 ",
	"50 ",
	"100",
	"200",
	"500",
	">1s",
	"ovf",
};
#define buckets_cnt (sizeof(buckets) / sizeof(*buckets))
#define SUB_BUCKET_CNT 5
#define HISTOGRAM_HEIGHT 10

static inline size_t time_get_bucket(uint64_t time)
{
	size_t i;

	for (i = 0; i < buckets_cnt; i++) {
		if (time <= buckets[i])
			return i - 1;
	}

	return buckets_cnt;
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
	if (time < min)
		return 0;
	if (time > max)
		return SUB_BUCKET_CNT - 1;

	time = time - min;

	return (time * SUB_BUCKET_CNT) / (max - min);
}

void print_histogram(const test_result_t *res)
{
	uint64_t cnts[SUB_BUCKET_CNT * (buckets_cnt + 1)] = {0};
	size_t i, j;
	uint64_t max;
	uint64_t sbcnt;

	if (!res->completion)
		return;

	sbcnt = SUB_BUCKET_CNT * buckets_cnt;
	/*
	 * Categorize the completion times in buckets and sub buckets.
	 * Every bucket has SUB_BUCKET_CNT sub buckets, so divide the result
	 * into proper one.
	 */
	for (i = 0; i < res->frames_written; i++) {
		size_t b = time_get_bucket(res->completion[i]);
		size_t sb = time_get_sub_bucket(b, res->completion[i]);

		++cnts[b * SUB_BUCKET_CNT + sb];
	}

	/* Maximum value for scale */
	max = 0;
	for (i = 0; i < sbcnt; i++) {
		if (cnts[i] > max)
			max = cnts[i];
	}

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
	test_mode_t mode = TEST_NORM;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_REVERSE;
	else if (info->opts->random)
		mode = TEST_RANDOM;
	info->res = tester_run_write(info->opts->path, info->opts->frm,
			info->start_frame, info->frames, info->fps, mode);

	return NULL;
}

void *run_read_test_thread(void *arg)
{
	thread_info_t *info = (thread_info_t *)arg;
	test_mode_t mode = TEST_NORM;

	if (!arg)
		return NULL;
	if (!info->opts)
		return NULL;

	if (info->opts->reverse)
		mode = TEST_REVERSE;
	else if (info->opts->random)
		mode = TEST_RANDOM;
	info->res = tester_run_read(info->opts->path, info->opts->frm,
			info->start_frame, info->frames, info->fps, mode);

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
		else {
			print_results(tst, &tres);
			if (opts->histogram)
				print_histogram(&tres);
		}
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
