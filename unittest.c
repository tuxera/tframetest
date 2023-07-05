#include <stdio.h>

#define RUN_TEST(NAME)\
	extern int test_ ## NAME (void);\
	printf("Running %s...\n", #NAME);\
	if (test_ ## NAME ()) { return 1;}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	RUN_TEST(frame);

	printf("Done.\n");
	return 0;
}
