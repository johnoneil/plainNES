set -e

# Run debug by default
# Define an env variable BUILD=Release or other to override
BUILD="${BUILD:-Debug}"
TESTS_DIR="${ROM:-test}"

BUILD_DIR=build/${BUILD}
EXE=${BUILD_DIR}/test/ROMtests

# Make sure we can find our binary
if [ ! -f "$EXE" ]; then
    echo Could not find binary ${EXE}
    exit 1
fi
# Make sure we can find our test roms
if [ ! -d "${TESTS_DIR}/roms" ]; then
    echo Could not find test roms at ${TESTS_DIR}
    exit 1
fi

echo going to dir ${TESTS_DIR}
cd "${TESTS_DIR}"
../${EXE}