#!/bin/sh

if [[ ${CROSS_COMPILE_HOST} == ""   ]]
then
    echo "Set CROSS_COMPILE_HOST."
    echo "exit ..."
    exit 1
fi


if [[ ${SDK_DIR} == ""   ]]
then
    echo "Set SDK_DIR."
    echo "exit ..."
    exit 1
fi


if [[ ${SDK_SYSROOT_DIR} == ""   ]]
then
    echo "Set SDK_SYSROOT_DIR."
    echo "exit ..."
    exit 1
fi

if [[ ${INSTALL_DIR} == ""   ]]
then
    echo "Set INSTALL_DIR."
    echo "exit ..."
    exit 2
fi

export PATH="${SDK_DIR}/host/bin":$PATH
export LD_LIBRARY_PATH="${SDK_SYSROOT_DIR}/usr/lib":$LD_LIBRARY_PATH
export CFLAGS+="-I${SDK_SYSROOT_DIR}/usr/include -Wno-format"
export CPPFLAGS+="-I${SDK_SYSROOT_DIR}/usr/include -Wno-format"
#export CC=arm-linux-gnueabihf-gcc
#export NM=arm-linux-gnueabihf-nm
#export AR=arm-linux-gnueabihf-ar

./configure \
    --enable-libwebrtc-aec3 \
    --with-sdl=${SDK_SYSROOT_DIR} \
    --with-ffmpeg=${SDK_SYSROOT_DIR} \
    --with-vpx=${SDK_SYSROOT_DIR} \
    --with-openh264=${SDK_SYSROOT_DIR} \
    --exec_prefix=$INSTALL_DIR \
    --prefix=$INSTALL_DIR \
    --host=${CROSS_COMPILE_HOST} --build=x86_64

