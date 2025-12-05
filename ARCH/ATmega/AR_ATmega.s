/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_ATmega.s - ATmega Port (Assembly)
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Variable and functions import/export
 *
 ***************************************************************************/

.extern arPreemptiveHandler
.extern arCurrTaskContext
.extern arPreemptiveContext

.global arYield


/***************************************************************************/
.text
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arYield
 *
 *  Description:
 *    Voluntarily yields execution of the current task.
 *
 ***************************************************************************/

.func arYield
arYield:

  /* Save r0 register (part of task context) */
  push r0

  /* Save SREG (Status Register, 0x3F) */
  in r0, 0x3F
  push r0

  /* Disable global interrupts (critical section) */
  cli

  /* Save general purpose registers r1..r31 */
  push r1
  push r2
  push r3
  push r4
  push r5
  push r6
  push r7
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15
  push r16
  push r17
  push r18
  push r19
  push r20
  push r21
  push r22
  push r23
  push r24
  push r25
  push r26
  push r27
  push r28
  push r29
  push r30
  push r31

  /* Save the current task stack pointer (SP) to the task context.
     0x3D = SPL, 0x3E = SPH. */
  in r24, 0x3D
  in r25, 0x3E
  sts arCurrTaskContext + 1, r25
  sts arCurrTaskContext, r24

  /* Switch to the dedicated scheduler stack (Preemptive Context).
     This prevents the scheduler from consuming the task's stack space. */
  lds r24, arPreemptiveContext
  lds r25, arPreemptiveContext + 1
  out 0x3E, r25
  out 0x3D, r24

  /* Execute the preemptive handler (Scheduler).
     Pass the address of arCurrTaskContext in r24:r25 (AVR ABI). */
  lds r30, arPreemptiveHandler
  lds r31, arPreemptiveHandler + 1
  ldi r24, lo8(arCurrTaskContext)
  ldi r25, hi8(arCurrTaskContext)
  icall

  /* Switch to the new task stack pointer (restored from context) */
  lds r24, arCurrTaskContext
  lds r25, arCurrTaskContext + 1
  out 0x3E, r25
  out 0x3D, r24

  /* Restore general purpose registers r31..r1 */
  pop r31
  pop r30
  pop r29
  pop r28
  pop r27
  pop r26
  pop r25
  pop r24
  pop r23
  pop r22
  pop r21
  pop r20
  pop r19
  pop r18
  pop r17
  pop r16
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop r7
  pop r6
  pop r5
  pop r4
  pop r3
  pop r2
  pop r1

  /* Restore SREG (Status Register) */
  pop r0
  out 0x3F, r0

  /* Restore r0 register */
  pop r0

  /* Return from interrupt (RETI enables global interrupts).
     Note: Although arYield is called via CALL, the task context mimics
     an interrupt frame, so RETI is used to resume execution. */
  reti

.endfunc


/***************************************************************************/
.end
/***************************************************************************/
