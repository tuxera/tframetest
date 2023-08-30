#!/bin/bash

set -eu

VERSION=${1:-$(date +%Y%m%d-%H%M%S)}
CROSS="${CROSS:-i686-w64-mingw32-}"
SRC_D="${CROSS}"
if [ "${SRC_D: -1}" == "-" ]; then
	SRC_D="${SRC_D::-1}"
fi
export CC="${CROSS}gcc"
export STRIP="${CROSS}strip"
export AR="${CROSS}ar"

LIB_SRC_PATH="/usr/${SRC_D}/lib"
LIBS=(libwinpthread-1.dll)

if ! "${CC}" -v > /dev/null 2>&1 ; then
	echo "Please install ${CC}"
	exit 1
fi
echo "Compiler ${CC}"

make clean all

# Package to zip file
DST="tframetest-win-${CROSS}${VERSION}"
cleanup() {
	if [ "${DST}" != "" ] && [ -d "${DST}" ]; then
		rm -rf "${DST:?}"
	fi
}

trap cleanup EXIT
mkdir -p "${DST}"
cp tframetest.exe "${DST}/"
"${STRIP}" "${DST}/tframetest.exe"

for lib in "${LIBS[@]}"; do
	cp "${LIB_SRC_PATH}/${lib}" "${DST}/"
done
zip -r "${DST}.zip" "${DST}"
rm -rf "${DST:?}"
