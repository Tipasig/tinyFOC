##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#
TOOLCHAIN ?= ARMCC
#TOOLCHAIN ?= ARMCLANG

# Compiler options here.
ifeq ($(USE_OPT),)
  ifeq ($(TOOLCHAIN),ARMCC)
    USE_OPT = -O3 -Otime
  else
    USE_OPT = -O3 -Ofast -g
  endif
  #USE_OPT = -O1
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT =
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
ifneq ($(TOOLCHAIN),ARMCC)
  USE_LTO = no
endif
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif
ifeq ($(V),1)
  USE_VERBOSE_COMPILE = yes
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = yes
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# don't enable EX9 or IFC in sflash version
ifeq ($(USE_EX9),)
  USE_EX9 = no
endif
ifeq ($(USE_IFC),)
  USE_IFC = no
endif

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x600
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

# Enables the use of FPU on Cortex-M4 (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = no
endif

# PY32F003: CM0+@32MHz
# STM32F407: CM4@168MHz
ifeq ($(MCU_NAME),)
  MCU_NAME = STM32F407xx
endif
mcu_lcase = $(shell echo $(MCU_NAME) | tr A-Z a-z)

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = libtinyfoc

# Imported source files and paths
TINYFOCINC = \
    ./st/Drivers/CMSIS/Core/Include \
    ./st/Drivers/CMSIS/Device/ST/STM32F4xx/Include
TINYFOCSRC = \
    src_c/libmathq15.c \
    src/tinyfoc_math_q15.c \
    src/tinyfoc_foc_q15.c \
    src/tinyfoc_smc_q15.c

# Define linker script file here
#LDSCRIPT= py32f003.sct

# C sources
CSRC = $(STARTUPSRC) \
       $(TINYFOCSRC) \

# C++ sources
CPPSRC =

# List ASM source files here
ASMSRC = $(STARTUPASM)

INCDIR = $(STARTUPINC) \
         $(TINYFOCINC)

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

CPU  = Cortex-M0+

ifeq ($(OS),Windows_NT)
toolchain = $(TOOLCHAIN)
else
toolchain = $(shell echo $(TOOLCHAIN) | tr A-Z a-z)
endif

TRGT = C:/Keil_v5/ARM/$(TOOLCHAIN)/bin/
#TRGT = "D:/Program Files/ARMCC/sw/ARMCompiler5.06u4/bin/"
CC   = $(TRGT)$(toolchain)
CPPC = $(TRGT)$(toolchain)
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)armlink
ELF  = $(TRGT)fromelf
AS   = $(TRGT)armasm
AR   = $(TRGT)armar -r
HEX  = $(ELF) --i32
BIN  = $(ELF) --bin

# ARM-specific options here
AOPT =

# Define C warning options here: -std=c99
ifeq ($(TOOLCHAIN),ARMCC)
CWARN = --c99 --diag_suppress A1609
else
CWARN = -std=c99 -Wnonportable-include-path
endif

# Define C++ warning options here
CPPWARN =

#
# Compiler settings
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS = -DARM_MATH_CM4 -DUSE_FULL_LL_DRIVER \
        -D$(MCU_NAME) -DCMSIS_DEV_HDR=\"$(mcu_lcase).h\" \
        -DCCMRAM -DNDEBUG

# Define ASM defines here
UADEFS = -D$(MCU_NAME)

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR = #$(EXE)/lib/$(toolchain)

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

RULESPATH = ./toolchain/$(toolchain)
include $(RULESPATH)/rules.mk

##############################################################################
# MISRA check rule, requires PCLint and the setup files, not provided.
#
misra:
	@lint-nt -v -w3 $(DEFS) pclint/co-gcc.lnt pclint/au-misra3.lnt pclint/waivers.lnt $(IINCDIR) $(CSRC) &> misra.txt
