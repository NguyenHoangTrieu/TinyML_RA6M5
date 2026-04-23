# TESTING_2 — RA6M5 Baremetal CMake/Ninja Project

Bare-metal driver development and testing project for the **Renesas RA6M5** (R7FA6M5BH) on an EK-RA6M5 board.  
Build system: **CMake + Ninja**. Toolchain: **Arm GNU GCC** (arm-none-eabi).

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Install ARM GCC Toolchain](#2-install-arm-gcc-toolchain)
3. [Install CMake](#3-install-cmake)
4. [Install Ninja](#4-install-ninja)
5. [Install J-Link (optional, for flashing)](#5-install-j-link-optional-for-flashing)
6. [Set Environment Variables](#6-set-environment-variables)
7. [Build from Command Line](#7-build-from-command-line)
8. [Build with VS Code](#8-build-with-vs-code)
9. [Flash to Target](#9-flash-to-target)
10. [Project Structure](#10-project-structure)
11. [Adding New Source Files to CMake](#11-adding-new-source-files-to-cmake)

---

## 1. Prerequisites

| Tool | Minimum version | Purpose |
|------|----------------|---------|
| Arm GNU Toolchain (`arm-none-eabi-gcc`) | 12.2 | Cross-compile for Cortex-M33 |
| CMake | 3.16 | Build configuration |
| Ninja | 1.11 | Fast incremental build |
| J-Link (SEGGER) | V7.x | Flash binary to board |
| VS Code + CMake Tools extension | latest | IDE support (optional) |

---

## 2. Install ARM GCC Toolchain

### Windows
1. Download **Arm GNU Toolchain** from  
   https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads  
   Choose the **Windows x86_64** installer for `arm-none-eabi`.

2. Run the installer. Default path:  
   `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\<version>\bin`

3. Tick **"Add to PATH"** during installation, or add manually (see §6).

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
```
Or install the latest release manually:
```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz
tar -xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -C /opt
```

---

## 3. Install CMake

### Windows
Download the msi installer from https://cmake.org/download/ (choose `cmake-x.xx.x-windows-x86_64.msi`).  
During install, select **"Add CMake to the system PATH"**.

### Linux
```bash
sudo apt install cmake
# or for the latest version:
pip install cmake
```

Verify:
```bash
cmake --version
```

---

## 4. Install Ninja

### Windows
Option A – scoop (recommended):
```powershell
scoop install ninja
```
Option B – manual:  
1. Download `ninja-win.zip` from https://github.com/ninja-build/ninja/releases  
2. Extract `ninja.exe` to a folder already on your PATH (e.g. `C:\tools\`).

### Linux
```bash
sudo apt install ninja-build
```

Verify:
```bash
ninja --version
```

---

## 5. Install J-Link (optional, for flashing)

Download **J-Link Software and Documentation Pack** from  
https://www.segger.com/downloads/jlink/

Install and note the binary location:
- **Windows**: `C:\Program Files\SEGGER\JLink_VXxx\JLink.exe`
- **Linux**: `/usr/bin/JLinkExe`

---

## 6. Set Environment Variables

The toolchain path is read from `ARM_GCC_TOOLCHAIN_PATH` (CMake kit) **or** `ARM_TOOLCHAIN_PATH` (CLI/batch).

### Windows — permanent (System variables)
```
ARM_GCC_TOOLCHAIN_PATH = C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.2 Rel1\bin
```
Open **System Properties → Advanced → Environment Variables → New** (System variables).

### Windows — temporary (current session)
```cmd
set ARM_GCC_TOOLCHAIN_PATH=C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.2 Rel1\bin
```

### Linux
```bash
export ARM_GCC_TOOLCHAIN_PATH=/opt/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-eabi/bin
# Add to ~/.bashrc or ~/.zshrc to make permanent
```

---

## 7. Build from Command Line

### Windows
```cmd
:: Debug build (default)
build_and_flash.bat

:: Release build
build_and_flash.bat Release

:: Debug build + flash to board via J-Link
build_and_flash.bat Debug flash

:: Release build + flash
build_and_flash.bat Release flash
```

> **Flash argument**: passing `flash` as the second argument generates a temporary `flash.jlink` script and calls J-Link to erase + program the device via SWD at 4 MHz. The script file is deleted automatically after flashing. J-Link path is configured at the top of `build_and_flash.bat`.

### Linux / macOS
```bash
chmod +x build_and_flash.sh

# Debug build
./build_and_flash.sh

# Release build
./build_and_flash.sh Release

# Debug build + flash
./build_and_flash.sh Debug flash

# Release build + flash
./build_and_flash.sh Release flash
```

### Manual CMake steps
```bash
mkdir build && cd build

cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DARM_TOOLCHAIN_PATH="<path-to-toolchain-bin>" \
    ..

ninja
```

Output files are in `build/`:
- `TESTING_2.elf` — ELF for GDB debugging
- `TESTING_2.srec` — S-record for flashing
- `TESTING_2.map` — linker map
- `compile_commands.json` — IntelliSense/clangd database

---

## 8. Build with VS Code

1. Open the workspace: **File → Open Workspace from File → `TESTING_2.code-workspace`**

2. Install recommended extensions when prompted:
   - **CMake Tools** (`ms-vscode.cmake-tools`)
   - **C/C++** (`ms-vscode.cpptools`)

3. Set your active **CMake Kit** (bottom status bar):  
   Choose `ARM GCC - Ninja`.

4. Click **Build** (status bar) or press `Ctrl+Shift+P` → `CMake: Build`.

> **Tip**: set the `ARM_GCC_TOOLCHAIN_PATH` system environment variable so VS Code picks it up automatically without editing any files.

---

## 9. Flash to Target

### Via build script (recommended)
```cmd
build_and_flash.bat Debug flash
```

### Via J-Link Commander (manual)
Create a script file `flash.jlink`:
```
device R7FA6M5BH
si SWD
speed 4000
connect
erase
loadfile build/TESTING_2.srec
r
g
exit
```
Then run:
```cmd
"C:\Program Files\SEGGER\JLink_V910\JLink.exe" -CommandFile flash.jlink
```

### Via Renesas Flash Programmer (RFP)
1. Open RFP and create a new project (Renesas RA MCU family).
2. Select **R7FA6M5BH**, interface **JTAG/SWD**.
3. Load `build/TESTING_2.srec` and click **Start**.

---

## 10. Project Structure

```
TESTING_2/
├── CMakeLists.txt          Main CMake entry point
├── Config.cmake            Tool paths (RASC exe path)
├── memory_regions.ld       Memory map for R7FA6M5BH
├── fsp_gen.ld              FSP linker sections
├── bsp_linker_info.h       Linker info types (BSP)
├── build_and_flash.bat     Windows build + flash script
├── build_and_flash.sh      Linux/macOS build + flash script
├── cmake/
│   ├── gcc.cmake           ARM GCC toolchain file
│   ├── GeneratedCfg.cmake  Compiler/linker flags
│   ├── GeneratedSrc.cmake  Source file list + CMake targets
│   └── prebuild.cmake      Optional RASC pre-build step
├── script/
│   └── fsp.ld              Linker script (includes memory_regions + fsp_gen)
├── ra/                     Renesas FSP HAL sources (BSP + drivers)
├── ra_cfg/                 FSP configuration headers (generated by RASC)
├── ra_gen/                 FSP generated sources (generated by RASC)
├── Driver/
│   ├── Include/            Custom bare-metal driver headers
│   │   ├── CGC.h           Clock generation circuit
│   │   ├── GPIO.h          General-purpose I/O
│   │   ├── IIC.h           I²C (RIIC)
│   │   ├── LPM.h           Low-power mode / Module stop
│   │   ├── RWP.h           Register write protection (PRCR)
│   │   └── SCI.h           Serial communication interface (UART/SPI/I2C)
│   └── Source/             Custom driver implementations
└── src/
    ├── hal_entry.c         Application entry (LED blink test result)
    ├── hal_warmstart.c     BSP warm start hook (pin init)
    └── test/
        ├── test_runner.h   Minimal test framework header
        ├── test_runner.c   Test framework implementation
        ├── test_cases.h    Test registration prototypes
        ├── test_gpio.c     GPIO driver unit tests
        └── test_rwp.c      RWP driver unit tests
```

---

## LED Test Result Interpretation

After flashing, the **blue LED (LED1, P400)** indicates test results:

| Pattern | Meaning |
|---------|---------|
| Slow blink (500 ms on/off) | All tests **passed** |
| Fast blink (100 ms on/off) | One or more tests **failed** |
---

## 11. Adding New Source Files to CMake

The build system uses **`cmake/GeneratedSrc.cmake`** as the single place where source files and include directories are managed. You never need to edit `CMakeLists.txt` for day-to-day file additions.

### How source discovery works

`GeneratedSrc.cmake` uses `file(GLOB_RECURSE ...)` to automatically pick up every `.c` / `.cpp` file inside the two main source trees:

```cmake
file(GLOB_RECURSE Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Source/*.c
)
```

This means:

| Location | Auto-picked? |
|---|---|
| `src/` and all subdirectories | Yes |
| `Driver/Source/` and all subdirectories | Yes |
| Anywhere else | No — must be added explicitly |

### Case 1 — New `.c` file inside `src/` or `Driver/Source/`

Just create the file. CMake will find it on the next configure run.  
Re-run the build script or press **Re-configure** in VS Code CMake Tools — no cmake file change needed.

```
src/
    my_new_module.c      ← picked up automatically
Driver/Source/
    MY_PERIPH.c          ← picked up automatically
```

### Case 2 — New `.c` file **outside** the two glob trees

Add it explicitly to `cmake/GeneratedSrc.cmake` by appending it to `Source_Files` **before** the `add_executable` call:

```cmake
file(GLOB_RECURSE Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Source/*.c
)

# Add files that live outside the glob trees:
list(APPEND Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/my_lib/foo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/my_lib/bar.c
)
```

### Case 3 — New header-only directory

If your new header file lives in a directory that is not already on the include path, add the directory to `target_include_directories` in `GeneratedSrc.cmake`:

```cmake
target_include_directories(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ra/arm/CMSIS_6/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/my_lib/Include   # ← add new include dir here
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/
)
```

### Case 4 — New driver module (`.h` + `.c` pair)

The recommended pattern:

1. Place the header in `Driver/Include/MY_PERIPH.h`
2. Place the implementation in `Driver/Source/MY_PERIPH.c`
3. No CMake change needed — both directories are already covered.

```
Driver/
    Include/
        MY_PERIPH.h     ← include path already configured
    Source/
        MY_PERIPH.c     ← auto-globbed by GeneratedSrc.cmake
```

### After any `cmake/GeneratedSrc.cmake` change

A full **re-configure** is required (Ninja alone is not enough):

```cmd
:: Windows — re-configure + build
build_and_flash.bat Debug
```

The build script always deletes the `build/` directory and re-runs `cmake`, so it handles this automatically.

> **Note**: `file(GLOB_RECURSE ...)` does **not** re-run automatically when you add a new file without re-configuring CMake. Always re-run the build script (or CMake configure) after adding a new file so it is included in the build.