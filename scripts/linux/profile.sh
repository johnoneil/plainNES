#!/usr/bin/env bash
#set -e

# Run Release by default
# Define an env variable BUILD=Debug or other to override
BUILD="${BUILD:-Release}"
ROM="${ROM:-test/roms/legend.of.zelda.nes}"
VALGRIND="${VALGRIND:-valgrind}"
KCACHEGRIND="${KCACHEGRIND:-kcachegrind}"
VALGRIND_OUTFILE="${VALGRIND_OUTFILE:-callgrind.output}"
PLATFORM=linux

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

${VALGRIND} --tool=callgrind --callgrind-out-file="${VALGRIND_OUTFILE}" ${EXE} ${ROM}
${KCACHEGRIND} ${VALGRIND_OUTFILE}




