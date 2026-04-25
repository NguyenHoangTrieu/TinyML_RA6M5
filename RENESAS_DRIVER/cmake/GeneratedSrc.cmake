# Source files and build target for TESTING_2

file(GLOB_RECURSE Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/**/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/**/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/**/*.cc
)

# Loại bỏ các file test, benchmark và source code của third_party khỏi quá trình biên dịch
list(FILTER Source_Files EXCLUDE REGEX ".*_test\\.cc$")
list(FILTER Source_Files EXCLUDE REGEX ".*_test\\.c$")
list(FILTER Source_Files EXCLUDE REGEX ".*benchmark.*")
list(FILTER Source_Files EXCLUDE REGEX ".*third_party.*")
list(FILTER Source_Files EXCLUDE REGEX ".*examples.*")
list(FILTER Source_Files EXCLUDE REGEX ".*testing.*")
list(FILTER Source_Files EXCLUDE REGEX ".*testdata.*")
list(FILTER Source_Files EXCLUDE REGEX ".*test_data_generation.*")
list(FILTER Source_Files EXCLUDE REGEX ".*integration_tests.*")
list(FILTER Source_Files EXCLUDE REGEX ".*experimental.*")
list(FILTER Source_Files EXCLUDE REGEX ".*tools.*")
list(FILTER Source_Files EXCLUDE REGEX ".*signal.*")
list(FILTER Source_Files EXCLUDE REGEX ".*python.*")

# Loại bỏ các thư mục rác chứa platform-specific code (tránh xung đột driver và header)
list(FILTER Source_Files EXCLUDE REGEX ".*(arc_custom|arc_emsdp|arc_mli|bluepill|ceva|chre|cortex_m_corstone_300|cortex_m_generic|ethos_u|hexagon|riscv32_generic|xtensa|cmsis_nn|models).*")

# Tách riêng source files của TFLite để compile với -w (tắt warning)
file(GLOB_RECURSE TFLite_Source_Files
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/**/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/**/*.cc
)
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*_test\\.cc$")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*_test\\.c$")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*benchmark.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*third_party.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*examples.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*testing.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*testdata.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*test_data_generation.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*integration_tests.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*experimental.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*tools.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*signal.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*python.*")
list(FILTER TFLite_Source_Files EXCLUDE REGEX ".*(arc_custom|arc_emsdp|arc_mli|bluepill|ceva|chre|cortex_m_corstone_300|cortex_m_generic|ethos_u|hexagon|riscv32_generic|xtensa|cmsis_nn|models).*")

# Loại TFLite sources ra khỏi Source_Files (sẽ add riêng với flag -w)
list(FILTER Source_Files EXCLUDE REGEX ".*TensorFlowLite.*")

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
    ${TFLite_Source_Files}
)

# Tắt toàn bộ warning cho các file TFLite (third-party, không cần sửa)
foreach(tflite_src ${TFLite_Source_Files})
    set_source_files_properties(${tflite_src} PROPERTIES COMPILE_FLAGS "-w")
endforeach()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/third_party/flatbuffers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Middleware/TensorFlowLite/third_party/gemmlowp
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
