#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

typedef struct test_platform_file_t
{
	int fd;
	int free;
	const char *name;
	size_t pos;
	size_t size;
	char *data;
} test_platform_file_t;

static test_platform_file_t *files = NULL;
static size_t file_cnt = 0;

static inline platform_handle_t test_platform_open(const char *fname,
	platform_open_flags_t flags, int mode)
{
	size_t idx = 0;

	if (file_cnt > 0) {
		size_t i;

		for (i = 0; i < file_cnt; i++) {
			if (strcmp(files[i].name, fname) == 0) {
				idx = files[i].fd;
				break;
			}
		}
		if (!idx) {
			for (i = 0; i < file_cnt; i++) {
				if (files[i].free) {
					idx = files[i].fd;
					break;
				}
			}
		}
	}
	if (!idx) {
		test_platform_file_t *tmp = NULL;
		++file_cnt;
		tmp = realloc(files, sizeof(*tmp) * file_cnt);
		if (!tmp) {
			--file_cnt;
			return 0;
		}
		idx = file_cnt;
		files = tmp;
		files[idx - 1].size = 0;
		files[idx - 1].data = NULL;
	}

	files[idx - 1].fd = idx;
	files[idx - 1].name = fname;
	files[idx - 1].pos = 0;
	files[idx - 1].free = 0;

	return idx;
}

static inline int test_platform_close(platform_handle_t f)
{
	if (!f || f > file_cnt)
		return 1;

	files[f - 1].free = 1;

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

static platform_t test_platform = {
	.open = test_platform_open,
	.close = test_platform_close,
	.write = test_platform_write,
	.read = test_platform_read,
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
