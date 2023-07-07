#ifndef FRAMETEST_PLATFORM_H
#define FRAMETEST_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t platform_handle_t;

typedef enum platform_open_flags_t {
	PLATFORM_OPEN_READ   = 1 << 0,
	PLATFORM_OPEN_WRITE  = 1 << 1,
	PLATFORM_OPEN_CREATE = 1 << 2,
	PLATFORM_OPEN_TRUNC  = 1 << 3,
	PLATFORM_OPEN_DIRECT = 1 << 4,
} platform_open_flags_t;

typedef enum platform_seek_flags_t {
	PLATFORM_SEEK_SET = 1,
	PLATFORM_SEEK_CUR = 2,
	PLATFORM_SEEK_END = 3,
} platform_seek_flags_t;

typedef struct platform_t {
	platform_handle_t (*open)(const char *fname,
		platform_open_flags_t flags, int mode);
	int (*close)(platform_handle_t handle);
	size_t (*write)(platform_handle_t handle, const char *buf, size_t size);
	size_t (*read)(platform_handle_t handle, char *buf, size_t size);
#if 0
	off_t (*seek)(platform_handle_t handle, off_t offs,
			platform_seek_flags_t whence);
#endif
	int (*usleep)(uint64_t usec);
#if 0
	int (*stat)(const char *fname, platform_stat_t *statbuf);
#endif

	void * (*calloc)(size_t nmemb, size_t size);
	void * (*malloc)(size_t size);
	int (*aligned_alloc)(void **res, size_t align, size_t size);
	void (*free)(void *mem);

	void *priv;
} platform_t;

const platform_t *platform_get(void);

#endif