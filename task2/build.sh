#!/bin/bash

set -e
set -x

export CONAN_SYSREQUIRES_MODE=enabled

rm -rf build
mkdir build

# conan install .. --build missing
conan install . -s build_type=Debug --install-folder=build --build missing

cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
