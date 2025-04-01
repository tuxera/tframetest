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

#ifdef __linux__
/* For O_DIRECT */
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"

#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(_WIN32) || defined(__APPLE__)
/* Faking O_DIRECT for now... */
#ifndef O_DIRECT
#define O_DIRECT 0
#endif
#endif

int generic_resolve_flags(platform_open_flags_t flags)
{
	int oflags = 0;

	if ((flags & (PLATFORM_OPEN_READ | PLATFORM_OPEN_WRITE)) ==
	    (PLATFORM_OPEN_READ | PLATFORM_OPEN_WRITE))
		oflags |= O_RDWR;
	else if (flags & (PLATFORM_OPEN_WRITE))
		oflags |= O_WRONLY;
	else if (flags & (PLATFORM_OPEN_READ))
		oflags |= O_RDONLY;

	if (flags & (PLATFORM_OPEN_CREATE))
		oflags |= O_CREAT;
	if (flags & (PLATFORM_OPEN_TRUNC))
		oflags |= O_TRUNC;
	if (flags & (PLATFORM_OPEN_DIRECT))
		oflags |= O_DIRECT;

	return oflags;
}

#if defined(_WIN32)
static inline platform_handle_t win_open(const char *fname,
					 platform_open_flags_t flags, int mode)
{
	unsigned int access = 0;
	unsigned int creat = 0;
	unsigned int oflags = 0;
	HANDLE h;

	if (flags & PLATFORM_OPEN_WRITE)
		access |= GENERIC_WRITE;
	if (flags & PLATFORM_OPEN_READ)
		access |= GENERIC_READ;

	if (flags & PLATFORM_OPEN_CREATE)
		creat = CREATE_ALWAYS;
	else
		creat = OPEN_EXISTING;

	if (flags & (PLATFORM_OPEN_DIRECT)) {
		oflags |= FILE_FLAG_NO_BUFFERING;
		oflags |= FILE_FLAG_WRITE_THROUGH;
	}

	h = CreateFile(fname, access, 0, NULL, creat, oflags, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	return _open_osfhandle((size_t)h, 0);
}

static inline int win_close(platform_handle_t handle)
{
	return _close(handle);
}

static inline size_t win_write(platform_handle_t handle, const char *buf,
			       size_t size)
{
	return write(handle, buf, size);
}

static inline size_t win_read(platform_handle_t handle, char *buf, size_t size)
{
	return read(handle, buf, size);
}

int win_aligned_alloc(void **res, size_t align, size_t size)
{
	void *tmp;

	tmp = _aligned_malloc(size, align);
	if (!tmp)
		return 1;
	*res = tmp;

	return 0;
}

static inline int win_usleep(uint64_t us)
{
	return usleep((useconds_t)us);
}

static inline int win_stat(const char *fname, platform_stat_t *st)
{
	struct stat sb;
	int res;

	res = stat(fname, &sb);
	if (res)
		return res;
	if (!st)
		return res;

	st->dev = sb.st_dev;
	st->rdev = sb.st_rdev;
	st->ino = sb.st_ino;
	st->mode = sb.st_mode;
	st->uid = sb.st_uid;
	st->gid = sb.st_gid;
	st->size = sb.st_size;

	return res;
}

int win_thread_create(uint64_t *thread_id, void *(*start)(void *), void *arg)
{
	return pthread_create((pthread_t *)thread_id, NULL, start, arg);
}

int win_thread_cancel(uint64_t thread_id)
{
	return pthread_cancel((pthread_t)thread_id);
}

int win_thread_join(uint64_t thread_id, void **retval)
{
	return pthread_join((pthread_t)thread_id, retval);
}
#else
static inline platform_handle_t
generic_open(const char *fname, platform_open_flags_t flags, int mode)
{
	int oflags = generic_resolve_flags(flags);

	return open(fname, oflags, mode);
}

static inline int generic_close(platform_handle_t handle)
{
	return close(handle);
}

static inline size_t generic_write(platform_handle_t handle, const char *buf,
				   size_t size)
{
	return write(handle, buf, size);
}

static inline size_t generic_read(platform_handle_t handle, char *buf,
				  size_t size)
{
	return read(handle, buf, size);
}

static inline int generic_usleep(uint64_t us)
{
	return usleep((useconds_t)us);
}

static inline int generic_stat(const char *fname, platform_stat_t *st)
{
	struct stat sb;
	int res;

	res = stat(fname, &sb);
	if (res)
		return res;
	if (!st)
		return res;

	st->dev = sb.st_dev;
	st->rdev = sb.st_rdev;
	st->ino = sb.st_ino;
	st->mode = sb.st_mode;
	st->uid = sb.st_uid;
	st->gid = sb.st_gid;
	st->size = sb.st_size;
	st->blksize = sb.st_blksize;
	st->blocks = sb.st_blocks;

	return res;
}

int generic_thread_create(uint64_t *thread_id, void *(*start)(void *),
			  void *arg)
{
	return pthread_create((pthread_t *)thread_id, NULL, start, arg);
}

int generic_thread_cancel(uint64_t thread_id)
{
	return pthread_cancel((pthread_t)thread_id);
}

int generic_thread_join(uint64_t thread_id, void **retval)
{
	return pthread_join((pthread_t)thread_id, retval);
}

#endif

static platform_t default_platform = {
#if defined(_WIN32)
	.open = win_open,
	.close = win_close,
	.write = win_write,
	.read = win_read,
	.usleep = win_usleep,
	.stat = win_stat,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = win_aligned_alloc,
	.free = free,

	.thread_create = win_thread_create,
	.thread_cancel = win_thread_cancel,
	.thread_join = win_thread_join,
#else
	.open = generic_open,
	.close = generic_close,
	.write = generic_write,
	.read = generic_read,
	.usleep = generic_usleep,
	.stat = generic_stat,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = posix_memalign,
	.free = free,

	.thread_create = generic_thread_create,
	.thread_cancel = generic_thread_cancel,
	.thread_join = generic_thread_join,
#endif
};

const platform_t *platform_get(void)
{
	return &default_platform;
}
