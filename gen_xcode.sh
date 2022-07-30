#!/bin/sh

XCODE_DIR=$PWD/xcode_project

mkdir -p $XCODE_DIR

cd $XCODE_DIR

cmake -G "Xcode" ../

cd ..