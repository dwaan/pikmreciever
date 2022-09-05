#!/usr/bin/sh

# Get submodule
git submodule update --init

# Create build folder
mkdir build
cd build

# Initialized cmake
cmake ..