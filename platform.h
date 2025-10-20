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

#ifndef FRAMETEST_PLATFORM_H
#define FRAMETEST_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t platform_handle_t;
typedef int64_t platform_off_t;

typedef enum platform_open_flags_t {
	PLATFORM_OPEN_READ = 1 << 0,
	PLATFORM_OPEN_WRITE = 1 << 1,
	PLATFORM_OPEN_CREATE = 1 << 2,
	PLATFORM_OPEN_TRUNC = 1 << 3,
	PLATFORM_OPEN_DIRECT = 1 << 4,
} platform_open_flags_t;

typedef enum platform_seek_flags_t {
	PLATFORM_SEEK_SET = 1,
	PLATFORM_SEEK_CUR = 2,
	PLATFORM_SEEK_END = 3,
} platform_seek_flags_t;

typedef struct platform_stat_t {
	uint64_t dev;
	uint64_t rdev;
	uint64_t ino;
	uint64_t mode;
	uint64_t nlink;
	uint64_t uid;
	uint64_t gid;
	uint64_t size;
	uint64_t blksize;
	uint64_t blocks;
} platform_stat_t;

typedef struct platform_t {
	platform_handle_t (*open)(const char *fname,
				  platform_open_flags_t flags, int mode);
	int (*close)(platform_handle_t handle);
	size_t (*write)(platform_handle_t handle, const char *buf, size_t size);
	size_t (*read)(platform_handle_t handle, char *buf, size_t size);
	platform_off_t (*seek)(platform_handle_t handle, platform_off_t offs,
			       platform_seek_flags_t whence);
	int (*usleep)(uint64_t usec);
	int (*stat)(const char *fname, platform_stat_t *statbuf);

	void *(*calloc)(size_t nmemb, size_t size);
	void *(*malloc)(size_t size);
	int (*aligned_alloc)(void **res, size_t align, size_t size);
	void (*free)(void *mem);

	int (*thread_create)(uint64_t *thread_id, void *(*start)(void *),
			     void *arg);
	int (*thread_cancel)(uint64_t thread_id);
	int (*thread_join)(uint64_t thread_id, void **retval);

	void *priv;
} platform_t;

const platform_t *platform_get(void);

#endif
