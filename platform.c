#ifdef __linux__
/* For O_DIRECT */
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"

#ifdef _WIN32
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
	int oflags = generic_resolve_flags(flags);

	return open(fname, oflags, mode);
}

static inline int win_close(platform_handle_t handle)
{
	return close(handle);
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
#else
static inline platform_handle_t generic_open(const char *fname,
	platform_open_flags_t flags, int mode)
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
#endif

static platform_t default_platform = {
#if defined(_WIN32)
	.open = win_open,
	.close = win_close,
	.write = win_write,
	.read = win_read,
	.usleep = win_usleep,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = win_aligned_alloc,
	.free = free,
#else
	.open = generic_open,
	.close = generic_close,
	.write = generic_write,
	.read = generic_read,
	.usleep = generic_usleep,
	.calloc = calloc,
	.malloc = malloc,
	.aligned_alloc = posix_memalign,
	.free = free,
#endif
};


const platform_t * platform_get(void)
{
	return &default_platform;
}
