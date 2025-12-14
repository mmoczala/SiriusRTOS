/****************************************************************************
 *
 *  SiriusRTOS
 *  AT91Startup.s - AT91SAM7S Startup Code / Boot Sequence
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* ARM Processor Mode Bits (CPSR) */
.equ FLAG_MODE_USER,                    0x10
.equ FLAG_MODE_FIQ,                     0x11
.equ FLAG_MODE_IRQ,                     0x12
.equ FLAG_MODE_SUPERVISOR,              0x13
.equ FLAG_MODE_ABORT,                   0x17
.equ FLAG_MODE_UNDEFINED,               0x1B
.equ FLAG_MODE_SYSTEM,                  0x1F

/* State and Interrupt Flags */
.equ FLAG_IRQ_DISABLE,                  0x80
.equ FLAG_FIQ_DISABLE,                  0x40
.equ FLAG_THUMB_ENABLE,                 0x20

/* Stack Size Configuration (in bytes) */
.equ STACK_TOP,                         0x00204000
.equ STACK_UNDEFINED_SIZE,              0x00000000
.equ STACK_ABORT_SIZE,                  0x00000000
.equ STACK_FIQ_SIZE,                    0x00000000
.equ STACK_IRQ_SIZE,                    0x00000014
.equ STACK_SUPERVISOR_SIZE,             0x00000100
.equ STACK_SYSTEM_SIZE,                 0x00000000


/***************************************************************************/
.text
.arm
/***************************************************************************/


/****************************************************************************
 *
 * Exception Vectors
 *
 ***************************************************************************/

.section .vect, "ax"

  /* Reset Vector: Jump to the reset handler */
  b cpResetHandler

  /* Undefined Instruction Vector: Trap (Infinite Loop) */
  sub pc, #-0x0008

  /* Software Interrupt Vector: Trap (Infinite Loop) */
  sub pc, #-0x0008

  /* Prefetch Abort Vector: Trap (Infinite Loop) */
  sub pc, #-0x0008

  /* Data Abort Vector: Trap (Infinite Loop) */
  sub pc, #-0x0008

  /* Reserved Vector: Trap (Infinite Loop) */
  sub pc, #-0x0008

  /* IRQ Vector: Jump to the handler provided by the AIC */
  ldr pc, [pc, #-0x0F20]

  /* FIQ Vector: Jump to the handler provided by the AIC */
  ldr pc, [pc, #-0x0F20]


/****************************************************************************
 *
 * Reset Handler
 *
 ***************************************************************************/

.section .init, "ax"
.func cpResetHandler
  cpResetHandler:


/****************************************************************************
 *
 * Stack Initialization
 *
 ***************************************************************************/

  /* Load the top of the physical RAM */
  ldr r0, =STACK_TOP

  /* Switch to Undefined Mode and set its Stack Pointer */
  msr cpsr_c, #FLAG_MODE_UNDEFINED | FLAG_IRQ_DISABLE | FLAG_FIQ_DISABLE
  mov sp, r0
  sub r0, r0, #STACK_UNDEFINED_SIZE

  /* Switch to Abort Mode and set its Stack Pointer */
  msr cpsr_c, #FLAG_MODE_ABORT | FLAG_IRQ_DISABLE | FLAG_FIQ_DISABLE
  mov sp, r0
  sub r0, r0, #STACK_ABORT_SIZE

  /* Switch to FIQ Mode and set its Stack Pointer */
  msr cpsr_c, #FLAG_MODE_FIQ | FLAG_IRQ_DISABLE | FLAG_FIQ_DISABLE
  mov sp, r0
  sub r0, r0, #STACK_FIQ_SIZE

  /* Switch to IRQ Mode and set its Stack Pointer */
  msr cpsr_c, #FLAG_MODE_IRQ | FLAG_IRQ_DISABLE | FLAG_FIQ_DISABLE
  mov sp, r0
  sub r0, r0, #STACK_IRQ_SIZE

  /* Switch to System Mode and set its Stack Pointer (Shared with User) */
  msr cpsr_c, #FLAG_MODE_SYSTEM | FLAG_IRQ_DISABLE | FLAG_FIQ_DISABLE
  mov sp, r0
  sub r0, r0, #STACK_SYSTEM_SIZE

  /* Switch to Supervisor Mode and set its Stack Pointer */
  msr cpsr_c, #FLAG_MODE_SUPERVISOR
  mov sp, r0

  /* Define the Stack Limit for the current mode */
  sub sl, sp, #STACK_SUPERVISOR_SIZE


/****************************************************************************
 *
 *  Low-Level Hardware Initialization
 *
 ***************************************************************************/

  /* Branch to the C function for PLL and Clock setup */
  ldr r0, =cpLowLevelInit
  ldr lr, =cpLowLevelInitExit
  bx r0
  cpLowLevelInitExit:


/****************************************************************************
 *
 *  Data Section Initialization
 *
 ***************************************************************************/

  /* Copy the initialized data section from Flash to RAM */
  ldr r1, =_etext
  ldr r2, =_data
  ldr r3, =_edata
  cpDataRel:
  cmp r2, r3
    ldrlo r0, [r1], #4
    strlo r0, [r2], #4
  blo cpDataRel


/****************************************************************************
 *
 *  BSS Section Initialization
 *
 ***************************************************************************/

  /* Zero-fill the uninitialized data section */
  mov r0, #0
  ldr r1, =__bss_start__
  ldr r2, =__bss_end__
  cpBSSClear:
    cmp r1, r2
    strlo r0, [r1], #4
  blo cpBSSClear


/****************************************************************************
 *
 *  Main Application Entry
 *
 ***************************************************************************/

  /* Initialize arguments (argc = 0, argv = NULL) */
  mov r0, #0
  mov r1, #0
  mov r2, #0

  /* Reset the Frame Pointer */
  mov fp, #0

  /* Reset the Thumb Frame Pointer */
  mov r7, #0

  /* Jump to the main() function */
  ldr r10, =main
  ldr lr, =cpMainExit
  bx r10

  /* Catch return from main (Infinite Loop) */
  cpMainExit:
  b cpMainExit

.endfunc


/***************************************************************************/
.end
/***************************************************************************/
