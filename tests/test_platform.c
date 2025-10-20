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

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "platform.h"

#define NAME_MAX 512

typedef struct test_platform_file_t {
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

static inline platform_handle_t
test_platform_open(const char *fname, platform_open_flags_t flags, int mode)
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

static inline platform_off_t test_platform_seek(platform_handle_t handle,
						platform_off_t offs,
						platform_seek_flags_t whence)
{
	test_platform_file_t *f;

	if (!handle || handle > file_cnt)
		return 0;

	f = &files[handle - 1];
	if (!f->size || !f->data)
		return 0;

	switch (whence) {
	case PLATFORM_SEEK_SET:
		f->pos = offs;
		break;
	case PLATFORM_SEEK_CUR:
		f->pos += offs;
		break;
	case PLATFORM_SEEK_END:
		if (f->size > 0)
			f->pos = f->size - 1;
		else
			f->pos = 0;
		break;
	}

	if (f->pos < 0)
		f->pos = 0;
	if (f->pos >= f->size) {
		if (f->size > 0)
			f->pos = f->size - 1;
		else
			f->pos = 0;
	}

	return f->pos;
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

int test_platform_thread_create(uint64_t *thread_id, void *(*start)(void *),
				void *arg)
{
	return -1;
}

int test_platform_thread_cancel(uint64_t thread_id)
{
	return -1;
}

int test_platform_thread_join(uint64_t thread_id, void **retval)
{
	return -1;
}

static platform_t test_platform = {
	.open = test_platform_open,
	.close = test_platform_close,
	.write = test_platform_write,
	.read = test_platform_read,
	.seek = test_platform_seek,

	.usleep = test_platform_usleep,
	.stat = test_platform_stat,

	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = posix_memalign,
	.free = free,

	.thread_create = test_platform_thread_create,
	.thread_cancel = test_platform_thread_cancel,
	.thread_join = test_platform_thread_join,
};

const platform_t *test_platform_get(void)
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

static int ignore_printf = 0;

void test_ignore_printf(int val)
{
	ignore_printf = val;
}

int __real_puts(const char *s);
int __wrap_puts(const char *s)
{
	if (ignore_printf)
		return 0;
	return __real_puts(s);
}

int __real_putchar(int c);
int __wrap_putchar(int c)
{
	if (ignore_printf)
		return 0;
	return __real_putchar(c);
}

int __wrap_printf(const char *fmt, ...)
{
	va_list ap;

	if (ignore_printf)
		return 0;
	va_start(ap, fmt);
	return vprintf(fmt, ap);
}
