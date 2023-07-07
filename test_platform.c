#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "platform.h"

#define NAME_MAX 512

typedef struct test_platform_file_t
{
	int fd;
	int open;
	char name[NAME_MAX];
	size_t pos;
	size_t size;
	char *data;
} test_platform_file_t;

static test_platform_file_t *files = NULL;
static size_t file_cnt = 0;

static inline size_t test_platform_find_file(const char *fname,
		test_platform_file_t **file)
{
	size_t i;

	for (i = 0; i < file_cnt; i++) {
		if (!strcmp(files[i].name, fname)) {
			if (file)
				*file = &files[i];
			return files[i].fd;
		}
	}

	return 0;
}

static inline platform_handle_t test_platform_open(const char *fname,
	platform_open_flags_t flags, int mode)
{
	size_t idx = 0;

	idx = test_platform_find_file(fname, NULL);
	/* In case it's not CREATE and we didn't find file, return err */
	if (!idx && !(mode & (PLATFORM_OPEN_CREATE)))
		return -1;

	if (!idx) {
		test_platform_file_t *tmp = NULL;

		++file_cnt;
		tmp = realloc(files, sizeof(*tmp) * file_cnt);
		if (!tmp) {
			--file_cnt;
			return -1;
		}
		idx = file_cnt;
		files = tmp;
		files[idx - 1].size = 0;
		files[idx - 1].data = NULL;
	}

	files[idx - 1].fd = idx;
	memcpy(files[idx - 1].name, fname, NAME_MAX);
	files[idx - 1].name[NAME_MAX - 1] = 0;
	files[idx - 1].pos = 0;
	files[idx - 1].open = 1;

	return idx;
}

static inline int test_platform_close(platform_handle_t f)
{
	if (!f || f > file_cnt)
		return 1;

	files[f - 1].open = 0;

	return 0;
}

static inline size_t test_platform_write(platform_handle_t handle,
		const char *buf, size_t size)
{
	test_platform_file_t *f;
	size_t new_size;
	char *tmp;

	if (!handle || handle > file_cnt)
		return 0;

	f = &files[handle - 1];
	new_size = f->size + size;
	tmp = realloc(f->data, new_size);
	if (!tmp)
		return 0;
	memmove(tmp + f->size, buf, size);
	f->data = tmp;
	f->size = new_size;
	f->pos = new_size;

	return size;
}

static inline size_t test_platform_read(platform_handle_t handle, char *buf,
		size_t size)
{
	test_platform_file_t *f;
	size_t cnt;

	if (!handle || handle > file_cnt)
		return 0;

	f = &files[handle - 1];
	if (!f->size || !f->data)
		return 0;

	cnt = size;
	if (f->pos + cnt >= f->size)
		cnt = f->size - f->pos;
	if (!cnt)
		return 0;

	memmove(buf, f->data + f->pos, cnt);
	f->pos += cnt;

	return cnt;
}

static inline int test_platform_usleep(uint64_t us)
{
	return usleep((useconds_t)us);
}

static inline int test_platform_stat(const char *fname, platform_stat_t *st)
{
	test_platform_file_t *f = NULL;
	size_t idx;

	idx = test_platform_find_file(fname, &f);
	if (!idx)
		return -1;

	memset(st, 0, sizeof(*st));
	st->size = f->size;
	st->ino = f->fd;
	st->nlink = 1;
	st->blksize = 512;
	st->blksize = (f->size + 511) & ~511;

	return 0;
}

static platform_t test_platform = {
	.open = test_platform_open,
	.close = test_platform_close,
	.write = test_platform_write,
	.read = test_platform_read,

	.usleep = test_platform_usleep,
	.stat = test_platform_stat,

	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = posix_memalign,
	.free = free,
};


const platform_t * test_platform_get(void)
{
	return &test_platform;
}

void test_platform_finalize(void)
{
	size_t i;

	if (files) {
		for (i = 0; i < file_cnt; i++) {
			if (files[i].data)
				free(files[i].data);
		}
		free(files);
		files = NULL;
		file_cnt = 0;
	}
}
