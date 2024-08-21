#!/usr/bin/env sh

alias get_rv1126_='export PATH="/sdk/rv1126_rv1109_linux_release_20211022/buildroot/output/firefly_rv1126_rv1109/host/bin":"/usr/lib/python3.11/site-packages/":$PATH; export LD_LIBRARY_PATH="/sdk/rv1126_rv1109_linux_release_20211022/buildroot/output/firefly_rv1126_rv1109/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib":$LD_LIBRARY_PATH'

get_rv1126_; make -j 7 dep
