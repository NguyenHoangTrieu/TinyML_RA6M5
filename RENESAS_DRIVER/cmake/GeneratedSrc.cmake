# Source files and build target for TESTING_2

file(GLOB_RECURSE Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/**/*.c
)

# RTOS Kernel sources — C implementation and Cortex-M33 assembly port
file(GLOB_RECURSE Kernel_C_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/Kernel/src/*.c
)
file(GLOB_RECURSE Kernel_ASM_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/Kernel/port/*.S
)

set(ALL_FILES ${Source_Files} ${Kernel_C_Files} ${Kernel_ASM_Files})

add_executable(${PROJECT_NAME}.elf
    ${ALL_FILES}
)

target_compile_options(${PROJECT_NAME}.elf
    PRIVATE
    $<$<CONFIG:Debug>:${RASC_DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${RASC_RELEASE_FLAGS}>
    $<$<CONFIG:MinSizeRel>:${RASC_MIN_SIZE_RELEASE_FLAGS}>
    $<$<CONFIG:RelWithDebInfo>:${RASC_RELEASE_WITH_DEBUG_INFO}>
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE $<$<COMPILE_LANGUAGE:C>:${RASC_CMAKE_C_FLAGS}>)
target_compile_options(${PROJECT_NAME}.elf PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${RASC_CMAKE_CXX_FLAGS}>)

target_link_options(${PROJECT_NAME}.elf PRIVATE $<$<LINK_LANGUAGE:C>:${RASC_CMAKE_EXE_LINKER_FLAGS}>)
target_link_options(${PROJECT_NAME}.elf PRIVATE $<$<LINK_LANGUAGE:CXX>:${RASC_CMAKE_EXE_LINKER_FLAGS}>)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${RASC_CMAKE_DEFINITIONS})

target_include_directories(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ra/arm/CMSIS_6/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/Kernel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/AHT20
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/
)

target_link_directories(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/script
)

target_link_libraries(${PROJECT_NAME}.elf
    PRIVATE
)

# Post-build: generate .srec file
add_custom_command(
    TARGET ${PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O srec ${PROJECT_NAME}.elf ${PROJECT_NAME}.srec
    COMMENT "Generating S-record: ${PROJECT_NAME}.srec"
)
