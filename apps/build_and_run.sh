#!/bin/bash

app=$1

# Exit if any command fails
set -e

# Build the project
echo "Building $app ..."

cd $1 && LIBRARY_PATH=../../bindings/c cargo build && cd ../

# Check if build succeeded and run the project
if [ $? -eq 0 ]; then
    echo "Build succeeded. Running the project..."
    LD_LIBRARY_PATH=../bindings/c/ LD_PRELOAD=../cacao/build/libcacao.so ./$app/target/debug/$app $@
else
    echo "Build failed."
fi