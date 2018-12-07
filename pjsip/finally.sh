#!/bin/bash

set -e
set -x

# see http://stackoverflow.com/a/3915420/318790
function realpath { echo $(cd $(dirname "$1"); pwd)/$(basename "$1"); }
__FILE__=`realpath "$0"`
__DIR__=`dirname "${__FILE__}"`
PJ="${__DIR__}/build/pjproject"

mkdir -p "${PJ}/include"

for project in pjlib pjlib-util pjmedia pjnath pjsip; do
	rsync -r "${PJ}/src/${project}/include" "build/pjproject/"
done

shopt -s extglob
mkdir -p "${PJ}/lib"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjlib/lib/"libpj-@(arm64|armv7|armv7s|i386|x86_64)-*                 -output "${PJ}/lib/libpj.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjlib-util/lib"/libpjlib-util-@(arm64|armv7|armv7s|i386|x86_64)-*    -output "${PJ}/lib/libpjlib-util.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjmedia/lib/"libpjmedia-@(arm64|armv7|armv7s|i386|x86_64)-*          -output "${PJ}/lib/libpjmedia.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjmedia/lib/"libpjmedia-audiodev-@(arm64|armv7|armv7s|i386|x86_64)-* -output "${PJ}/lib/libpjmedia-audiodev.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjmedia/lib/"libpjmedia-codec-@(arm64|armv7|armv7s|i386|x86_64)-*    -output "${PJ}/lib/libpjmedia-codec.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjmedia/lib/"libpjmedia-videodev-@(arm64|armv7|armv7s|i386|x86_64)-* -output "${PJ}/lib/libpjmedia-videodev.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjnath/lib/"libpjnath-@(arm64|armv7|armv7s|i386|x86_64)-*            -output "${PJ}/lib/libpjnath.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjsip/lib/"libpjsip-@(arm64|armv7|armv7s|i386|x86_64)-*              -output "${PJ}/lib/libpjsip.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjsip/lib/"libpjsip-simple-@(arm64|armv7|armv7s|i386|x86_64)-*       -output "${PJ}/lib/libpjsip-simple.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjsip/lib/"libpjsip-ua-@(arm64|armv7|armv7s|i386|x86_64)-*           -output "${PJ}/lib/libpjsip-ua.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjsip/lib/"libpjsua-@(arm64|armv7|armv7s|i386|x86_64)-*              -output "${PJ}/lib/libpjsua.a"
xcrun -sdk iphoneos lipo -create "${PJ}/src/pjsip/lib/"libpjsua2-@(arm64|armv7|armv7s|i386|x86_64)-*             -output "${PJ}/lib/libpjsua2.a"
for lib in g7221codec gsmcodec ilbccodec resample speex srtp webrtc; do
	xcrun -sdk iphoneos lipo -create "${PJ}/src/third_party/lib/"lib${lib}-@(arm64|armv7|armv7s|i386|x86_64)-*   -output "${PJ}/lib/lib${lib}.a"
done
