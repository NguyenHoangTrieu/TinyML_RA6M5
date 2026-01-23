#!/usr/bin/env bash

set -e

# Paths
ARM_TOOLCHAIN_PATH="/opt/arm-gnu-toolchain/arm-none-eabi/bin"
JLINK_PATH="/usr/bin/JLinkExe"

# Toolchain check
if [[ ! -f "${ARM_TOOLCHAIN_PATH}/arm-none-eabi-gcc" ]]; then
    echo "ERROR: Toolchain not found at ${ARM_TOOLCHAIN_PATH}" >&2
    exit 1
fi

# Export toolchain
export PATH="${ARM_TOOLCHAIN_PATH}:${PATH}"

# Clean build
rm -rf build
mkdir build
cd build

# Configure
cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DARM_TOOLCHAIN_PATH="${ARM_TOOLCHAIN_PATH}" \
    ..

# Build
ninja

cd ..

# Generate JLink script
cat > flash.jlink <<EOF
device R7FA6M5BH
si SWD
speed 1000
connect
erase
loadfile build/TinyML_RA6M5.srec
r
g
exit
EOF

# Flash
"${JLINK_PATH}" -CommandFile flash.jlink

# Cleanup
rm -f flash.jlink
echo "Build and flash completed"
