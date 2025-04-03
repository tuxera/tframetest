#!/bin/bash

set -eu

VERSION=${1:-$(date +%Y%m%d-%H%M%S)}
CROSS="${CROSS:-i686-w64-mingw32-}"
SRC_D="${CROSS}"
BUILD_FOLDER=${BUILD_FOLDER:-$(pwd)/build}
if [ "${SRC_D: -1}" == "-" ]; then
	SRC_D="${SRC_D::-1}"
fi
export CC="${CROSS}gcc"
export STRIP="${CROSS}strip"
export AR="${CROSS}ar"

LIBS=(libwinpthread-1.dll)
# Paths are hardcoded for now
LIB_SRC_PATH="/usr/${SRC_D}/lib"
LIB_LICENSE_PATH="/usr/share/doc/mingw-w64-common/copyright"
LICENSE_PATH="COPYING"

if ! "${CC}" -v > /dev/null 2>&1 ; then
	echo "Please install ${CC}"
	exit 1
fi
echo "Compiler ${CC}"

make clean all BUILD_FOLDER="${BUILD_FOLDER}"

# Package to zip file
DST="tframetest-win-${CROSS}${VERSION}"
cleanup() {
	if [ "${DST}" != "" ] && [ -d "${DST}" ]; then
		rm -rf "${DST:?}"
	fi
}

trap cleanup EXIT
mkdir -p "${DST}"
cp "${BUILD_FOLDER}/tframetest.exe" "${DST}/"
"${STRIP}" "${DST}/tframetest.exe"

for lib in "${LIBS[@]}"; do
	cp "${LIB_SRC_PATH}/${lib}" "${DST}/"
done
cp "${LIB_LICENSE_PATH}" "${DST}/copying-mingw.txt"
cp "${LICENSE_PATH}" "${DST}/"
rm -f "${DST}.zip"
zip -r "${DST}.zip" "${DST}"
rm -rf "${DST:?}"
