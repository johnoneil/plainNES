#!/usr/bin/env bash
set -e

# allow custom cmake, make if desired
CMAKE="${CMAKE:-cmake}"
MAKE="${MAKE:-make}"

# Build defaults to debug
# Define an env variable BUILD=Release or other to override
BUILD="${BUILD:-Debug}"
PLATFORM=linux

# Fail if we don't have CMakeLists.txt in current dir
CMAKELISTS=${PWD}/CMakeLists.txt
if [ ! -f "$CMAKELISTS" ]; then
    echo "$CMAKELISTS could not be found."
    exit -1
fi

BUILD_DIR=${PWD}/build/${PLATFORM}/${BUILD}
mkdir -p ${BUILD_DIR}
${CMAKE} -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD}
pushd ${BUILD_DIR}
${MAKE} VERBOSE=${VERBOSE}
