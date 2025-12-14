/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_AT91a.s - AT91SAM7S64 Port (Assembly)
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

/* Mode flags */
.equ FLAG_MODE_SUPERVISOR,              0x13
.equ FLAG_MODE_MASK,                    0x1F

/* Other flags */
.equ FLAG_IRQ_DISABLE,                  0x80
.equ FLAG_FIQ_DISABLE,                  0x40
.equ FLAG_THUMB_ENABLE,                 0x20

/* AT91SAM7S registers */
.equ AT91_PITC_PIVR,                    0xFFFFFD38
.equ AT91_AIC_EOICR,                    0xFFFFF130


/****************************************************************************
 *
 *  Variable and functions import/export
 *
 ***************************************************************************/

.extern arPreemptiveHandler
.extern arCurrTaskContext

.global arLock
.global arRestore
.global arYield
.global arIRQPreemption


/***************************************************************************/
.text
.arm
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arLock
 *
 *  Description:
 *    Disables normal (IRQ) interrupts. The calling task must be in
 *    Supervisor Mode.
 *
 *  Return:
 *    Previous state of the interrupt flag (1 if enabled, 0 if disabled).
 *
 ***************************************************************************/

.func arLock
arLock:

  /* Read Current Program Status Register (CPSR) and save it on the stack */
  mrs r0, cpsr
  stmfd sp!, { r0 }

  /* Disable normal interrupts by setting the I flag in the CPSR */
  orr r0, r0, #FLAG_IRQ_DISABLE
  msr cpsr_c, r0

  /* Return 1 if normal interrupts were previously enabled */
  ldmfd sp!, { r0 }
  mvn r0, r0, lsr #7
  and r0, r0, #1

  /* Return */
  bx lr

.endfunc


/****************************************************************************
 *
 *  Name:
 *    arRestore
 *
 *  Description:
 *    Restores the normal interrupt flag state. The calling task must be in
 *    Supervisor Mode.
 *
 *  Parameters:
 *    PreviousLockState - Previous state of the interrupt flag.
 *
 ***************************************************************************/

.func arRestore
arRestore:

  /* Save register R0 */
  stmfd sp!, { r0 }

  /* Enable normal interrupts if PreviousLockState is non-zero */
  cmp r0, #0
  mrsne r0, cpsr
  bicne r0, r0, #FLAG_IRQ_DISABLE
  msrne cpsr_c, r0

  /* Restore register R0 */
  ldmfd sp!, { r0 }

  /* Return */
  bx lr

.endfunc


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

  /* The Link Register (LR) must be stored manually when a context switch
     is requested via software (yield). Two copies are saved:
     1. The original LR value.
     2. The return address for the PC (same as LR in this case). */
  stmfd sp!, { lr }
  stmfd sp!, { lr }

  arIRQYield:

  /* Save the current task context (R0-R12) on the stack */
  stmfd sp!, { r0-r12 }

  /* Save the CPSR. If we are currently in Thumb mode (indicated by Bit 0
     of LR), set the Thumb bit in the saved CPSR to ensure correct return. */
  mrs r0, cpsr
  tst lr, #1
  orrne r0, #FLAG_THUMB_ENABLE
  stmfd sp!, { r0 }

  /* Disable normal interrupts (the previous state was stored implicitly
     in the saved CPSR) */
  mrs r0, cpsr
  orr r0, #FLAG_IRQ_DISABLE
  msr cpsr_c, r0

  /* Save the current task stack pointer */
  ldr r0, =arCurrTaskContext
  str sp, [r0]

  /* Execute the preemption handler (scheduler) */
  ldr r1, =arPreemptiveHandler
  ldr r1, [r1]
  cmp r1, #0
  beq arSkipSPChanging

    /* Call the scheduler */
    mov lr, pc
    bx r1

    /* Load the new task stack pointer */
    ldr r0, =arCurrTaskContext
    ldr sp, [r0]

  arSkipSPChanging:

  /* Restore the new task context */
  ldmfd sp!, { r0 }
  msr cpsr_cxsf, r0 /* Restore CPSR */
  ldmfd sp!, { r0-r12, lr, pc } /* Restore registers and return */

.endfunc


/****************************************************************************
 *
 *  Name:
 *    arIRQPreemption
 *
 *  Description:
 *    Preempts the current task to perform a context switch. This routine
 *    is called from the IRQ Interrupt Service Routine.
 *
 ***************************************************************************/

.func arIRQPreemption
arIRQPreemption:

  /* Reserve space on the stack for a single register. This slot will be
     used to pass the return address during the mode switch. */
  sub sp, #4

  /* Save R0 and R1 on the IRQ stack */
  stmfd sp!, { r0-r1 }

  /* Read the Periodic Interval Value Register (clears the interrupt) */
  ldr r0, =AT91_PITC_PIVR
  ldr r0, [r0]

  /* Acknowledge the interrupt in the AIC (End of Interrupt Command) */
  ldr r0, =AT91_AIC_EOICR
  str r0, [r0]

  /* Copy IRQ-banked registers to general purpose registers:
     R0 = IRQ Stack Pointer
     R1 = Return Address (LR_irq - 4) */
  mov r0, sp
  sub r1, lr, #4

  /* Clean up the IRQ stack. R0 and R1 values are preserved in registers.
     We remove the space used by the stored R0-R1 (8 bytes) and the
     reserved slot (4 bytes). */
  add sp, #12

  /* Switch CPU mode back to Supervisor (or the previous privileged mode)
     to perform the context save. We push the address of 'arRestoreCPUMode'
     onto the stack and load it into the PC while restoring the SPSR to
     the CPSR (^ suffix). */
  ldr lr, =arRestoreCPUMode
  stmfd sp!, { lr }
  ldmfd sp!, { pc }^

  arRestoreCPUMode:

  /* We are now in Supervisor Mode.
     Store the task return address (calculated earlier in R1) and the
     original Link Register on the stack. */
  stmfd sp!, { r1 }
  stmfd sp!, { lr }

  /* Restore the original R0 and R1 values from the pointer to the old
     IRQ stack (stored in R0). */
  ldr r1, [r0, #4]
  ldr r0, [r0]

  /* Jump to the common context switch logic in arYield */
  b arIRQYield

.endfunc


/***************************************************************************/
.end
/***************************************************************************/
