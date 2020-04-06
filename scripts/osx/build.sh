#!/usr/bin/env bash
set -e

# This creates and builds an xcode project at <root>/build/osx

# allow custom cmake, make if desired
CMAKE="${CMAKE:-cmake}"

# Build defaults to debug
# Define an env variable BUILD=Release or other to override
BUILD="${BUILD:-Debug}"
PLATFORM=osx

# Fail if we don't have CMakeLists.txt in current dir
CMAKELISTS=${PWD}/CMakeLists.txt
if [ ! -f "$CMAKELISTS" ]; then
    echo "$CMAKELISTS could not be found."
    exit -1
fi

BUILD_DIR=${PWD}/build/${PLATFORM}
mkdir -p ${BUILD_DIR}
${CMAKE} -G Xcode -S . -B ${BUILD_DIR}

# xcodebuild -list -project build/osx/Debug/plainNES.xcodeproj
# Information about project "plainNES":
#     Targets:
#         ALL_BUILD
#         NES
#         ROMtests
#         ZERO_CHECK
#         glad
#         plainNES

#     Build Configurations:
#         Debug
#         Release
#         MinSizeRel
#         RelWithDebInfo

#     If no build configuration is specified and -scheme is not passed then "Debug" is used.

#     Schemes:
#         ALL_BUILD
#         glad
#         NES
#         plainNES
#         ROMtests
#         ZERO_CHECK
# for more info: https://developer.apple.com/library/archive/technotes/tn2339/_index.html

cd ${BUILD_DIR}
xcodebuild -scheme ALL_BUILD build

