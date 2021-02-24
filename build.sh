#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./ServerWorkDir}

mkdir -p $BUILD_DIR/logDir \
    && cd $BUILD_DIR \
    && cmake $SOURCE_DIR \
    && make $*
