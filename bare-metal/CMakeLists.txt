
#   Part of the Raspberry-Pi Bare Metal Tutorials
#   Copyright (c) 2015, Brian Sidebotham
#   All rights reserved.

#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:

#   1. Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.

#   2. Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.

#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.

# CMake build environment for the Valvers Raspberry-Pi bare metal tutorials

# CMake 2.8.10 Documentation: http://www.cmake.org/cmake/help/v2.8.10/cmake.html

cmake_minimum_required( VERSION 2.8 )

# Mark the language as C so that CMake doesn't try to test the C++
# cross-compiler's ability to compile a simple program because that will fail
project( tube-client C ASM )

# Add any C compiler flags here. The C flags concerned with processor tuning
# are in the toolchain files toolchain-arm-none-eabi-bcm2835.cmake, etc.
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostartfiles" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPROFILING" )



# Set the linker flags so that we use our "custom" linker script
set( CMAKE_EXE_LINKER_FLAGS "-Wl,-T,${PROJECT_SOURCE_DIR}/rpi.ld" )

add_executable( tube-client
    tube-client.c
# 32016 Co Pro
    copro-32016.c
    copro-32016.h
    ../../NS32016/32016.c
    ../../NS32016/32016.h
    ../../NS32016/Decode.c
    ../../NS32016/Decode.h
    ../../NS32016/mem32016.c
    ../../NS32016/mem32016.h
    ../../NS32016/NSDis.c
    ../../NS32016/pandora/PandoraV0_61.h
    ../../NS32016/pandora/PandoraV2_00.h
    ../../NS32016/Trap.c
    ../../NS32016/Trap.h
    ../../NS32016/Profile.c
    ../../NS32016/Profile.h
# 80186 Co Pro
    copro-80186.c
    copro-80186.h
    ../cpu80186/cpu80186.c
    ../cpu80186/cpu80186.h
    ../cpu80186/mem80186.c
    ../cpu80186/mem80186.h
    ../cpu80186/iop80186.c
    ../cpu80186/iop80186.h
    ../cpu80186/Client86_v1_01.h
    ../cpu80186/i386dasm.c
# ARM2 Co Pro
    mame/arm.c
    mame/arm.h
    copro-arm2.c
    copro-arm2.h
    tuberom_arm.c
    tuberom_arm.h
# ARM11 Co Pro
    copro-arm.h
    copro-arm.c
    armc-cstartup.c
    armc-cstubs.c
    armc-start.S
    rpi-armtimer.c
    rpi-armtimer.h
    rpi-aux.c
    rpi-aux.h
    rpi-base.h
    rpi-gpio.c
    rpi-gpio.h
    rpi-interrupts.c
    rpi-interrupts.h
    rpi-systimer.c
    rpi-systimer.h
	spi.c
	spi.h
	debug.h
	swi.c
	swi.h
	tube-env.h
	tube-lib.h
	tube-lib.c
	tube-commands.h
	tube-commands.c
	tube-swi.h
	tube-swi.c
	tube-exception.c
	tube-isr.h
	tube-isr.c
# DARM DIsassembler Core
    darm/armv7.c
    darm/armv7-tbl.c
    darm/armv7-tbl.h
    darm/darm.c
    darm/darm.h
    darm/darm-internal.h
    darm/darm-tbl.c
    darm/darm-tbl.h
    darm/thumb2.c
    darm/thumb2-decoder.c
    darm/thumb2.h
    darm/thumb2-tbl.c
    darm/thumb2-tbl.h
    darm/thumb.c
    darm/thumb-tbl.c
    darm/thumb-tbl.h
# Shared
    tuberom_6502.h
    tuberom_6502.c
# 6502 Co Processor using 65tube Implementation
    copro-65tube.h
    copro-65tube.c
    copro-65tubeasm.S
# 6502 Co Processor using BeebDroid ARM 6502 Implementation
    copro-6502.h
    copro-6502.c
    copro-6502asm.S
# 6502 Co Processor using lib6502 Implementation
    copro-lib6502.h
    copro-lib6502.c
    lib6502.c
    lib6502.h
)

target_link_libraries (tube-client m)

add_custom_command(
    TARGET tube-client POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ./tube-client -O binary ./kernel.img
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Convert the ELF output file to a binary image" )
