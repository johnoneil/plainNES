#!/usr/bin/env bash
set -e

# Run debug by default
# Define an env variable BUILD=Release or other to override
BUILD="${BUILD:-Debug}"
PLATFORM=linux
ROM="${ROM:-test/roms/legend.of.zelda.nes}"

BUILD_DIR=build/${PLATFORM}/${BUILD}
EXE=${BUILD_DIR}/bin/plainNES

# Make sure we can find our binary
if [ ! -f "$EXE" ]; then
    echo Could not find binary ${EXE}
    exit -1
fi
# Make sure we can find our rom
if [ ! -f "$ROM" ]; then
    echo Could not find rom ${ROM}
    exit -1
fi

${EXE} ${ROM}


