#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include "profile.h"
#include "unittest.h"
#include "frame.h"

static const platform_t *frame_platform = NULL;

static frame_t *gen_default_frame(void)
{
	profile_t prof;

	prof = profile_get_by_index(1);
	return frame_gen(frame_platform, prof);
}

int test_frame_gen(void)
{
	frame_t *frm;

	frm = gen_default_frame();
	TEST_ASSERT(frm);

	frame_destroy(frame_platform, frm);

	return 0;
}

int test_frame_fill(void)
{
	frame_t *frm;
	size_t i;

	frm = gen_default_frame();
	TEST_ASSERT(frm);

	/* Default fill with 't' */
	for (i = 0; i < frm->size; i++)
		TEST_ASSERT_EQ(((unsigned char*)frm->data)[i], 't');

	/* Test fill works */
	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	for (i = 0; i < frm->size; i++)
		TEST_ASSERT_EQ(((unsigned char*)frm->data)[i], 0x42);

	frame_destroy(frame_platform, frm);

	return 0;
}

int test_frame_write_read(void)
{
	frame_t *frm;
	frame_t *frm_read;
	size_t fw;
	int fd;

	frm = gen_default_frame();
	frm_read = gen_default_frame();
	TEST_ASSERT(frm);
	TEST_ASSERT(frm_read);

	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 0x42);
	TEST_ASSERT_EQ(((unsigned char*)frm_read->data)[0], 't');

	fd = frame_platform->open("tst1", PLATFORM_OPEN_WRITE |
			PLATFORM_OPEN_CREATE |
			PLATFORM_OPEN_DIRECT, 0666);
	fw = frame_write(frame_platform, fd, frm);
	TEST_ASSERT_EQ(fw, frm->size);
	frame_platform->close(fd);

	TEST_ASSERT_EQ(((unsigned char*)frm_read->data)[0], 't');

	fd = frame_platform->open("tst1", PLATFORM_OPEN_READ |
			PLATFORM_OPEN_DIRECT, 0666);
	fw = frame_read(frame_platform, fd, frm_read);
	TEST_ASSERT_EQ(fw, frm->size);

	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 0x42);
	TEST_ASSERT_EQ(((unsigned char*)frm_read->data)[0], 0x42);
	frame_platform->close(fd);

	frame_destroy(frame_platform, frm);
	frame_destroy(frame_platform, frm_read);

	return 0;
}

int test_frame(void)
{
	TEST_INIT();

	frame_platform = test_platform_get();

	TEST(frame_gen);
	TEST(frame_fill);
	TEST(frame_write_read);

	test_platform_finalize();

	TEST_END();
}
