# tframetest

Tool to test and benchmark writing/reading media frames to/from a disk.

This tool is planned to about the same as closed source binary too `frametest`
found on various places from the net, for example from here:
[https://support.dvsus.com/hc/en-us/articles/212925466-How-to-use-frametest](https://support.dvsus.com/hc/en-us/articles/212925466-How-to-use-frametest)


## Compiling

To compile for Linux install a C compiler and issue:

	make

It should build `tframetest` binary.

To build for FreeBSD you need to use `gmake` instead since the Makefile is not
compatible with FreeBSD `make`.

To build Windows binary there's script `build_win.sh`.
One can cross-compile the tool in a Linux system.
We have tested `i686-w64-mingw32-gcc` and `x86_64-w64-mingw32-gcc` available
in Ubuntu/Debian repository as `mingw-w64`.
The default build for Windows is 32-bit but 64-bit build is possible:

	sudo apt install mingw-w64 zip
	./build_win.sh
	CROSS=x86_64-w64-mingw32- ./build_win.sh

## Usage

The basic usage is to first write some frames, and then to read them.
Most used options are: frame size, number of frames, and number of threads.
To write 1000 of 2k sized frames with 4 threads do:

	mkdir tst
	./tframetest -w 2k -n 1000 -t 4 tst

To perform read test for those freshly written frames:

	./tframetest -r -n 1000 -t 4 tst

There's more options available, please see the help for more info:

	./tframetest --help
