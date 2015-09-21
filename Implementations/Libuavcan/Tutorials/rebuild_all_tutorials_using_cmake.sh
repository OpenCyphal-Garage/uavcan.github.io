#!/bin/bash

echo ">>> Purging build directories..."
rm -rf */build &> /dev/null

set -e

for cmake_script in */CMakeLists.txt ; do
    d=`dirname $cmake_script`

    echo ">>> Creating $d/build..."
    mkdir $d/build
    cd $d/build

    echo ">>> Running CMake..."
    cmake ..

    echo ">>> Running make..."
    make -j8

    echo ">>> Tutorial '$d' built successfully"
    cd ../..
done

echo ">>> All builds succeeded"
