#!/bin/sh

# see http://stackoverflow.com/a/3915420/318790
function realpath { echo $(cd $(dirname "$1"); pwd)/$(basename "$1"); }
__FILE__=`realpath "$0"`
__DIR__=`dirname "${__FILE__}"`

BASEDIR_PATH="$1"
TARGET_URL="http://downloads.xiph.org/releases/opus/opus-1.1.2.tar.gz"
TARGET_PATH="${BASEDIR_PATH}/src"

# download
function download() {
    "${__DIR__}/download.sh" "$1" "$2" #--no-cache
}

# build
function build() {
    ARCH="$1"
    PREFIX="${BASEDIR_PATH}/build/${ARCH}"
    LOG="${PREFIX}/build.log"

    pushd . > /dev/null
    cd "${TARGET_PATH}"

    if [ -d "${PREFIX}" ]; then
        rm -rf ${PREFIX}
    fi
    mkdir -p ${PREFIX}

    echo "Builing for ${ARCH}..."

    SDKNAME="iphoneos"
    HOST="arm-apple-darwin"
    if [ "${ARCH}" == "i386" ] || [ "${ARCH}" == "x86_64" ]; then
        SDKNAME="iphonesimulator"
    fi
    CC=$(xcrun --sdk ${SDKNAME} -f clang)
    CXX=$(xcrun --sdk ${SDKNAME} -f clang++)
    SYSROOT="$(xcrun -sdk ${SDKNAME} --show-sdk-path)"
    CFLAGS="-isysroot ${SYSROOT} -arch ${ARCH} -fPIE -miphoneos-version-min=7.0 -O2"
    CXXFLAGS=$CFLAGS

    export CC CXX CFLAGS CXXFLAGS

    ./configure --disable-shared --enable-static --with-pic \
        --disable-extra-programs --disable-doc --prefix="${PREFIX}" --host="${HOST}" > "${LOG}"

    make -j4 >> "${LOG}"
    make install  >> "${LOG}"
    make clean  >> "${LOG}"

    popd > /dev/null
}

download ${TARGET_URL} ${TARGET_PATH}

build armv7 && build armv7s && build arm64 && build i386 && build x86_64

shopt -s extglob
mkdir -p "${BASEDIR_PATH}/lib"
xcrun -sdk iphoneos lipo -create "${BASEDIR_PATH}"/build/*/lib/libopus.a -output "${BASEDIR_PATH}/lib/libopus.a"

if [ ! -d "${BASEDIR_PATH}/include" ]; then
    cp -R "${BASEDIR_PATH}/build/armv7/include" "${BASEDIR_PATH}/include"
fi
