#!/bin/sh

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
    --exec_prefix=$INSTALL_DIR \
    --prefix=$INSTALL_DIR \
    --host=arm-linux-gnueabihf --build=x86_64

