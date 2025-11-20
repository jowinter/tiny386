CMAKE_MINIMUM_REQUIRED(VERSION 3.18)
PROJECT(Nano386)

ENABLE_LANGUAGE(C)
ENABLE_LANGUAGE(ASM)

# -------------------------------------------------------------------------------------------------
# Toolchain specific flags (warnings, language standard)
#

ADD_LIBRARY(Nano386_Toolchain INTERFACE)
TARGET_COMPILE_OPTIONS(Nano386_Toolchain INTERFACE -Wall -std=c99 -ffreestanding)

OPTION(NANO386_PEDANTIC_WARNINGS "Enable pedantic warnings during compilation (-pedantic)" OFF)
IF (NANO386_PEDANTIC_WARNINGS)
	TARGET_COMPILE_OPTIONS(Nano386_Toolchain -pedantic)
ENDIF ()

OPTION(NANO386_WARNINGS_AS_ERRORS "Treat warnings as errors (-Werror)" OFF)
IF (NANO386_WARNINGS_AS_ERRORS)
	TARGET_COMPILE_OPTIONS(Nano386_Toolchain -Werror)
ENDIF ()

# -------------------------------------------------------------------------------------------------
# Platform definitions (compiler and link options specific to the platform)
#

ADD_LIBRARY(Nano386_Platform INTERFACE)

TARGET_COMPILE_OPTIONS(Nano386_Platform	INTERFACE -mthumb)
TARGET_LINK_OPTIONS(Nano386_Platform	INTERFACE -mthumb --specs=nano.specs)
TARGET_LINK_LIBRARIES(Nano386_Platform  INTERFACE Nano386_Toolchain -lm)

TARGET_COMPILE_DEFINITIONS(Nano386_Platform INTERFACE NANO386_PLATFORM=1)

OPTION (NANO386_GNU_SOURCE "Build with -D_GNU_SOURCE=1" ON)
IF (NANO386_GNU_SOURCE)
	# strdup in <string.h> and friends ...
	TARGET_COMPILE_DEFINITIONS(Nano386_Platform INTERFACE NANO386_GNU_SOURCE=1 _GNU_SOURCE=1)
ENDIF ()

OPTION (NANO386_SEMIHOSTING "Enable ARM semihosting" ON)
IF (NANO386_SEMIHOSTING)
	MESSAGE ("-- Enabling semihosting support (via rdimon.specs)")
	TARGET_COMPILE_DEFINITIONS(Nano386_Platform INTERFACE NANO386_SEMIHOSTING=1)
	TARGET_LINK_OPTIONS(Nano386_Platform INTERFACE --specs=rdimon.specs)
ELSE ()
	MESSAGE ("-- Semihosting support is disabled")
	TARGET_LINK_OPTIONS(Nano386_Platform INTERFACE --specs=nosys.specs)
ENDIF ()

# -------------------------------------------------------------------------------------------------
# Nano386 emulator core (aka. the retargetable part of tiny386)
#

ADD_LIBRARY(Nano386_Emulator OBJECT)

TARGET_SOURCES(Nano386_Emulator
	PRIVATE
		../crt.c
		../ini.c
		../i386.c
		../fpu.c
		../i8259.c
		../i8254.c
		../ide.c
		../vga.c
		../i8042.c
		../misc.c
		../fmopl.c
		../adlib.c
		../i8257.c
		../sb16.c
		../pcspk.c
		../pci.c
		../pc.c
)

OPTION (NANO386_PLATFORM_ENABLE_NE2000 "Platform: Enable the NE2000 network interface card" OFF)
IF (NANO386_PLATFORM_ENABLE_NE2000)
	TARGET_SOURCES(Nano386_Emulator PRIVATE ../ne2000.c)
	TARGET_COMPILE_DEFINITIONS(Nano386_Emulator PUBLIC NANO386_PLATFORM_ENABLE_NE2000=1)
ELSE ()
	TARGET_COMPILE_DEFINITIONS(Nano386_Emulator PUBLIC NANO386_PLATFORM_ENABLE_NE2000=0)
ENDIF ()

TARGET_INCLUDE_DIRECTORIES(Nano386_Emulator
	PUBLIC
		..
)

TARGET_COMPILE_OPTIONS(Nano386_Emulator
	PUBLIC
		# TODO: Strengthen incrementally ;)
		-Wno-unused-function
		-Wno-unused-variable
		-Wno-unused-but-set-variable
		-Wno-discarded-qualifiers
		-Wno-parentheses
		-Wno-format
)

TARGET_LINK_LIBRARIES(Nano386_Emulator
	PUBLIC
		Nano386_Platform
)

# -------------------------------------------------------------------------------------------------
# Nano386 host application
#

ADD_EXECUTABLE(Nano386)

TARGET_SOURCES(Nano386
	PRIVATE
		src/nano386.c
)

TARGET_LINK_LIBRARIES(Nano386
	PRIVATE
		Nano386_Emulator
		Nano386_Platform
)