#!/bin/sh


if [[ ${INSTALL_DIR} == ""   ]]
then
    echo "Set INSTALL_DIR."
    echo "exit ..."
    exit 4
fi
echo "INSTALL_DIR=$INSTALL_DIR"



./configure \
    --enable-libwebrtc-aec3 \
    --exec_prefix=$INSTALL_DIR \
    --prefix=$INSTALL_DIR
