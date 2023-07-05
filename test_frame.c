#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include "profile.h"
#include "unittest.h"
#include "frame.h"

static frame_t *__gen_default_frame(void)
{
	profile_t prof;

	prof = profile_get_by_index(1);
	return frame_gen(prof);
}

int test_frame_gen(void)
{
	frame_t *frm;

	frm = __gen_default_frame();
	TEST_ASSERT(frm);

	frame_destroy(frm);

	return 0;
}

int test_frame_fill(void)
{
	frame_t *frm;

	frm = __gen_default_frame();
	TEST_ASSERT(frm);

	/* Default fill with 't' */
	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 't');

	/* Test fill works */
	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 0x42);

	frame_destroy(frm);

	return 0;
}

int test_frame_write_read(void)
{
	frame_t *frm;
	frame_t *frm_read;
	pid_t pid;
	int fd[2];
	int res = 0;

	frm = __gen_default_frame();
	frm_read = __gen_default_frame();
	TEST_ASSERT(frm);
	TEST_ASSERT(frm_read);

	TEST_ASSERT_EQ(frame_fill(frm, 0x42), frm->size);
	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 0x42);
	TEST_ASSERT_EQ(((unsigned char*)frm_read->data)[0], 't');

	/* Create pipe for read / write */
	TEST_ASSERT_EQ(pipe(fd), 0);

	pid = fork();
	TEST_ASSERT_NE(pid, -1);

	if (pid == 0) {
		close(fd[1]);

		TEST_ASSERT_EQ(frame_read(fd[0], frm_read), frm->size);
		close(fd[0]);

		if (((unsigned char*)frm_read->data)[0] == 0x42)
			_exit(EXIT_SUCCESS);
		_exit(EXIT_FAILURE);
	} else {
		int status;

		close(fd[0]);

		TEST_ASSERT_EQ(frame_write(fd[1], frm), frm->size);

		close(fd[1]);

		waitpid(pid, &status, 0);
		if (status != 0)
			res = 1;
	}
	TEST_ASSERT_EQ(((unsigned char*)frm->data)[0], 0x42);

	frame_destroy(frm);
	frame_destroy(frm_read);

	return res;
}

int test_frame(void)
{
	TEST_INIT();

	TEST(frame_gen);
	TEST(frame_fill);
	TEST(frame_write_read);

	TEST_END();
}
