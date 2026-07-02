#!/usr/bin/env bash
# -------------------------------------------------------
# TESTING_2 - RA6M5 Baremetal CMake/Ninja Build Script
# -------------------------------------------------------

set -e

# ---- Toolchain paths (edit for your system) ----
# Default: auto-detect arm-none-eabi-gcc from PATH (e.g. apt-installed
# gcc-arm-none-eabi puts it in /usr/bin). Override with ARM_TOOLCHAIN_PATH
# env var if installed elsewhere (e.g. a manually unpacked toolchain).
if [[ -z "${ARM_TOOLCHAIN_PATH:-}" ]]; then
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        ARM_TOOLCHAIN_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
    else
        ARM_TOOLCHAIN_PATH="/opt/arm-gnu-toolchain/arm-none-eabi/bin"
    fi
fi
# Must be exported: cmake/gcc.cmake also reads this as an OS env var
# during its internal try_compile ABI-detection sub-build, which does
# not inherit -D cache variables from the outer configure.
export ARM_TOOLCHAIN_PATH
JLINK_PATH="${JLINK_PATH:-/usr/bin/JLinkExe}"

# ---- Build type: Debug (default) or Release ----
# Usage: ./build_and_flash.sh [Debug|Release]
BUILD_TYPE="${1:-Debug}"

# ---- Validate toolchain ----
if [[ ! -f "${ARM_TOOLCHAIN_PATH}/arm-none-eabi-gcc" ]]; then
    echo "[ERROR] ARM GCC toolchain not found at: ${ARM_TOOLCHAIN_PATH}" >&2
    echo "Install it or set ARM_TOOLCHAIN_PATH env variable." >&2
    exit 1
fi

# ---- Clean and create build directory ----
rm -rf build
mkdir build
cd build

# ---- Configure ----
echo "[INFO] Configuring with CMake (BUILD_TYPE=${BUILD_TYPE})..."
cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc.cmake \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DARM_TOOLCHAIN_PATH="${ARM_TOOLCHAIN_PATH}" \
    ..

# ---- Build ----
echo "[INFO] Building with Ninja..."
ninja

cd ..
echo "[INFO] Build succeeded: build/TESTING_2.elf / build/TESTING_2.srec"

# ---- Flash via J-Link ----
if [[ ! -f "${JLINK_PATH}" ]]; then
    echo "[ERROR] J-Link not found at: ${JLINK_PATH}" >&2
    exit 1
fi

echo "[INFO] Flashing via J-Link..."
cat > flash.jlink <<EOF
device R7FA6M5BH
si SWD
speed 4000
connect
erase
loadfile build/TESTING_2.srec
r
g
exit
EOF

"${JLINK_PATH}" -CommandFile flash.jlink
rm -f flash.jlink
echo "[INFO] Flash complete."

echo "Done!"
