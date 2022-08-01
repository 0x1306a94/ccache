#!/bin/sh

XCODE_DIR=$PWD/xcode_project

mkdir -p $XCODE_DIR

cd $XCODE_DIR

# cmake -G "Xcode" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ../
cmake -G "Xcode" -DCMAKE_BUILD_TYPE=RelWithDebInfo ../

cd ..