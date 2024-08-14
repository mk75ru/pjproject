#!/bin/sh 

#export CROSS_COMPILE_HOST=arm-buildroot-linux-gnueabihf
export CROSS_COMPILE_HOST=arm-linux-gnueabihf
export SDK_DIR="/sdk/rv1126_rv1109_linux_release_20211022/buildroot/output/firefly_rv1126_rv1109"
export SDK_SYSROOT_DIR="${SDK_DIR}/host/${CROSS_COMPILE_HOST}/sysroot"
export INSTALL_DIR="/work/pjproject.arm/install-rv1126"
./configure-arm-without_video.sh
