#****************************************************************************
#
#  SiriusRTOS
#  Makefile - Build Configuration for AT91SAM7S, WinARM
#  Version 1.00
#
#  Copyright 2010 by SpaceShadow
#  All rights reserved!
#
#****************************************************************************


#****************************************************************************
#
#  Target Configuration
#
#****************************************************************************

# Target CPU and Output Filename
CPU_TYPE = AT91SAM7S64
OUTPUT_FILE = AT91


#****************************************************************************
#
#  Source Files and Directories
#
#****************************************************************************

# Assembler Source Files (ARM Mode)
SRC_ASM_ARM += AT91Startup.s
SRC_ASM_ARM += ARCH/AT91SAM7S64/AR_AT91a.s

# Assembler Source Files (Thumb Mode)
SRC_ASM_THUMB +=

# C Source Files (ARM Mode)
SRC_C_ARM += ARCH/AT91SAM7S64/AR_AT91.c
SRC_C_ARM += STD/ST_BSTree.c
SRC_C_ARM += STD/ST_PQueue.c
SRC_C_ARM += STD/ST_Endian.c
SRC_C_ARM += STD/ST_Errors.c
SRC_C_ARM += STD/ST_Handle.c
SRC_C_ARM += STD/ST_DevMan.c
SRC_C_ARM += STD/ST_Memory.c
SRC_C_ARM += STD/ST_FixMem.c
SRC_C_ARM += STD/ST_CLIB.c
SRC_C_ARM += STD/ST_Init.c
SRC_C_ARM += OS/OS_Core.c
SRC_C_ARM += OS/OS_Task.c
SRC_C_ARM += OS/OS_Mutex.c
SRC_C_ARM += OS/OS_Semaphore.c
SRC_C_ARM += OS/OS_CountSem.c
SRC_C_ARM += OS/OS_Event.c
SRC_C_ARM += OS/OS_Timer.c
SRC_C_ARM += OS/OS_SharedMem.c
SRC_C_ARM += OS/OS_PtrQueue.c
SRC_C_ARM += OS/OS_Stream.c
SRC_C_ARM += OS/OS_Queue.c
SRC_C_ARM += OS/OS_Mailbox.c
SRC_C_ARM += OS/OS_Flags.c
SRC_C_ARM += AT91Init.c
SRC_C_ARM += Main.c

# C Source Files (Thumb Mode)
SRC_C_THUMB +=

# Include Paths
INCLUDE_DIR += ARCH/AT91SAM7S64
INCLUDE_DIR += STD
INCLUDE_DIR += OS


#****************************************************************************
#
#  Assembler Flags
#
#****************************************************************************

# Core Flags (CPU Architecture and Interworking)
AFLAGS = -mcpu=arm7tdmi -mthumb-interwork -I. -x assembler-with-cpp

# Define the specific CPU variant
AFLAGS += -DCPU_$(CPU_TYPE)


#****************************************************************************
#
#  Compiler Flags
#
#****************************************************************************

# Core Flags
CFLAGS = -mcpu=arm7tdmi -mthumb-interwork

# Language Standard (GNU99)
CFLAGS += -std=gnu99

# Define the specific CPU variant
CFLAGS += -DCPU_$(CPU_TYPE)

# Optimization Level
CFLAGS += -O1

# Warning Configuration
CFLAGS += -W -Wall -Wcast-align -Wimplicit -Wpointer-arith -Wredundant-decls
CFLAGS += -Wreturn-type -Wshadow -Wunused -Wcast-qual -Wmissing-declarations
CFLAGS += -Wnested-externs -Wmissing-prototypes -Wstrict-prototypes -Wswitch

# Include Path Expansion
CFLAGS += -I. $(patsubst %,-I%,$(INCLUDE_DIR))

# Debug: Generate assembly listings (optional)
# CFLAGS += -Wa,-adhlns=$(subst $(suffix $<),.lst,$<)


#****************************************************************************
#
#  Linker Flags
#
#****************************************************************************

# Core Flags (No standard startup files)
LFLAGS += -nostartfiles

# Define the specific CPU variant
LFLAGS += -DCPU_$(CPU_TYPE)

# Linker Script
LFLAGS += -T./$(CPU_TYPE).ld

# Map File Generation
LFLAGS += -Wl,-Map=$(OUTPUT_FILE).map,--cref

# Additional Libraries
# LFLAGS += -lc -lgcc -lm -lstdc


#****************************************************************************
#
#  Build Rules
#
#****************************************************************************

# Object File Definitions
OBJ_ASM_ARM = $(SRC_ASM_ARM:.s=.o)
OBJ_ASM_THUMB = $(SRC_ASM_THUMB:.s=.o)
OBJ_C_ARM = $(SRC_C_ARM:.c=.o)
OBJ_C_THUMB = $(SRC_C_THUMB:.c=.o)

# Main Target: Build the binary and cleanup intermediate files
build: $(OBJ_ASM_ARM) $(OBJ_ASM_THUMB) $(OBJ_C_ARM) $(OBJ_C_THUMB)
	arm-elf-gcc -mthumb $(CFLAGS) $(OBJ_ASM_ARM) $(OBJ_ASM_THUMB)\
		$(OBJ_C_ARM) $(OBJ_C_THUMB) --output $(OUTPUT_FILE).elf $(LFLAGS)
	arm-elf-objcopy -O binary $(OUTPUT_FILE).elf $(OUTPUT_FILE).bin
	rm -f $(OUTPUT_FILE).elf
	rm -f $(OBJ_ASM_ARM)
	rm -f $(OBJ_ASM_THUMB)
	rm -f $(OBJ_C_ARM)
	rm -f $(OBJ_C_THUMB)

# Rule: Assemble ARM Sources
$(OBJ_ASM_ARM) : %.o : %.s
	arm-elf-gcc -c $(AFLAGS) $< -o $@

# Rule: Assemble Thumb Sources
$(OBJ_ASM_THUMB) : %.o : %.s
	arm-elf-gcc -c -mthumb $(AFLAGS) $< -o $@

# Rule: Compile ARM Sources
$(OBJ_C_ARM) : %.o : %.c
	arm-elf-gcc -c $(CFLAGS) $< -o $@

# Rule: Compile Thumb Sources
$(OBJ_C_THUMB) : %.o : %.c
	arm-elf-gcc -c -mthumb $(CFLAGS) $< -o $@

.PHONY: build