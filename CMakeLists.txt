cmake_minimum_required(VERSION 3.15)

# Cross compilation toolchain setup
set(CMAKE_CROSSCOMPILING true)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv) 

set(RV64_CROSS_PREFIX riscv64-unknown-elf-)
find_program(RV64_COMPILER ${RV64_CROSS_PREFIX}gcc)
find_program(RV64_OBJDUMP  ${RV64_CROSS_PREFIX}objdump)
find_program(RV64_OBJCOPY  ${RV64_CROSS_PREFIX}objcopy)

mark_as_advanced(RV64_COMPILER)
mark_as_advanced(RV64_OBJDUMP)
mark_as_advanced(RV64_OBJCOPY)

set(CMAKE_C_COMPILER ${RV64_COMPILER})
set(CMAKE_CXX_COMPILER ${RV64_COMPILER})
set(CMAKE_ASM_COMPILER ${RV64_COMPILER})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Actual project specification
project(xvisor-guest C ASM)

# The submodule where the FreeRTOS-Kernel lives
set(FREERTOS_KERNEL_PATH "./kernel")

# Xvisor provides a quite bare RISCV core
set(FREERTOS_RISCV_EXTENSION "RISCV_no_extensions")

# Provide either the default stack size or respect external overrides
if (NOT DEFINED STACK_SIZE)
    set(STACK_SIZE 300 CACHE STRING "Stack region size in bytes")
else()
    set(STACK_SIZE ${STACK_SIZE} CACHE STRING "Stack region size in bytes")
endif()

if (DEFINED FREERTOS_SMP AND FREERTOS_SMP STREQUAL "1")
    message(STATUS "Build FreeRTOS for SMP")
    # Adding the following configurations to build SMP template port
    set(SMP_FLAGS
        "-DconfigNUMBER_OF_CORES=2 -DconfigUSE_PASSIVE_IDLE_HOOK=0")
endif()

# Select the heap port.  values between 1-4 will pick a heap.
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)

# Select the RISC-V GCC port
set(FREERTOS_PORT "GCC_RISC_V_GENERIC" CACHE STRING "" FORCE)

if (DEFINED DEBUG AND DEBUG STREQUAL "1")
    message(STATUS "Debugging Build")
    set(C_OPTIMIZATION
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-ggdb3> $<$<COMPILE_LANG_AND_ID:C,GNU>:-Og>)
else()
    set(C_OPTIMIZATION
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-O2>)
endif()

set(ARCH_FLAGS
    $<$<COMPILE_LANGUAGE:C,ASM>:-march=rv64imafdc_zicsr_zifencei>
    $<$<COMPILE_LANGUAGE:C,ASM>:-mabi=lp64d>
    $<$<COMPILE_LANGUAGE:C,ASM>:-mcmodel=medany>
)

set(GENERAL_FLAGS
    $<$<COMPILE_LANGUAGE:C,ASM>:-fmessage-length=0>
    $<$<COMPILE_LANGUAGE:C,ASM>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C,ASM>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C,ASM>:-fno-builtin-printf>
)


# Add the freertos_config for FreeRTOS-Kernel
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config
    INTERFACE
    "./"
)

# Adding the FreeRTOS-Kernel subdirectory
add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)

########################################################################
# Overall Compile Options
# Note the compile option strategy is to error on everything and then
# Per library opt-out of things that are warnings/errors.
# This ensures that no matter what strategy for compilation you take, the
# builds will still occur.
#
# Only tested with GNU and Clang.
# Other options are https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html#variable:CMAKE_%3CLANG%3E_COMPILER_ID
# Naming of compilers translation map:
#
#   FreeRTOS    | CMake
#   -------------------
#   CCS         | ?TBD?
#   GCC         | GNU, Clang, *Clang Others?
#   IAR         | IAR
#   Keil        | ARMCC
#   MSVC        | MSVC # Note only for MinGW?
#   Renesas     | ?TBD?

target_compile_options(freertos_kernel PRIVATE
    ${ARCH_FLAGS}
    ${GENERAL_FLAGS}
    ${SMP_FLAGS}
    ${C_OPTIMIZATION}
    
    ### Gnu/Clang C Options
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wpedantic>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
    #$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wconversion>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>

    # Suppressions required to build clean with clang.
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-unused-macros>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-padded>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-missing-variable-declarations>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-covered-switch-default>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-cast-align> )

add_executable(${PROJECT_NAME}
    start.S
    vector.S
    ns16550.c
    riscv-virt.c
    main_blinky.c
    main.c
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${ARCH_FLAGS}
    ${GENERAL_FLAGS}
    ${SMP_FLAGS}
    ${C_OPTIMIZATION}
    $<$<COMPILE_LANGUAGE:C,ASM>:-march=rv64imafdc_zicsr_zifencei>
    $<$<COMPILE_LANGUAGE:C,ASM>:-mabi=lp64d>
    $<$<COMPILE_LANGUAGE:C,ASM>:-mcmodel=medany>
    $<$<COMPILE_LANGUAGE:C,ASM>:-fmessage-length=0>
    $<$<COMPILE_LANGUAGE:C,ASM>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C,ASM>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C,ASM>:-fno-builtin-printf>
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wpedantic>
    $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>

    # Suppressions required to build clean with clang.
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-unused-macros>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-padded>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-missing-variable-declarations>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-covered-switch-default>
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-cast-align>
)


target_link_libraries(${PROJECT_NAME} freertos_kernel freertos_config)

target_link_options(${PROJECT_NAME} PRIVATE
   ${ARCH_FLAGS}
   $<$<C_COMPILER_ID:GNU>:-T${CMAKE_SOURCE_DIR}/vm-guest.ld>
   $<$<C_COMPILER_ID:GNU>:-nostartfiles>
   $<$<C_COMPILER_ID:GNU>:LINKER:--gc-sections>
   $<$<C_COMPILER_ID:GNU>:LINKER:--defsym=__stack_size=${STACK_SIZE}>
   $<$<C_COMPILER_ID:GNU>:LINKER:-Map=${PROJECT_NAME}.map>
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${RV64_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${RV64_OBJDUMP} -d ${CMAKE_BINARY_DIR}/${PROJECT_NAME} > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dump
)    

