# tframetest

Tool to test and benchmark writing/reading media frames to/from a disk.

This tool is planned to do about the same as closed source binary tool
`frametest` found on various places from the net, for example from here:
[https://support.dvsus.com/hc/en-us/articles/212925466-How-to-use-frametest](https://support.dvsus.com/hc/en-us/articles/212925466-How-to-use-frametest).

We aim that tframetest would be almost drop-in replacement for
the original tool, but there are some differences for example in the supported
options, functionality, way of working, and in the output.


## Compiling

To compile for Linux install a C compiler and issue:

	make

This should build `tframetest` binary into `build` folder.

To build for FreeBSD you need to use `gmake` instead since the Makefile is not
compatible with FreeBSD `make`.

To build Windows binary there's script `build_win.sh` or the `make win` target.
One can cross-compile the tool in a Linux system.
We have tested `i686-w64-mingw32-gcc` and `x86_64-w64-mingw32-gcc` available
in Ubuntu/Debian repository as `mingw-w64`.
The default build for Windows is 32-bit but 64-bit build is possible as well:

	sudo apt install mingw-w64 zip
	./build_win.sh
	CROSS=x86_64-w64-mingw32- ./build_win.sh


## Usage

The basic usage is to write first some frames, and then to read them.
Most used options are: frame size, number of frames, and number of threads.
To write 1000 of 2k sized frames with 4 threads do:

	mkdir tst
	build/tframetest -w 2k -n 1000 -t 4 tst

To perform read test for those freshly written frames:

	build/tframetest -r -n 1000 -t 4 tst

There's more options available, please see the help for more info:

	build/tframetest --help


## License

This program is distributed under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version. See the included file [COPYING](COPYING).
