/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_ATmega.c - ATmega Port
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "AR_API.h"
#include "ST_API.h"


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

TPreemptiveProc arPreemptiveHandler;
struct TTaskContext arPreemptiveContext;
struct TTaskContext arCurrTaskContext;
static TIME arSystemTime;


/****************************************************************************
 *
 *  Name:
 *    arInit
 *
 *  Description:
 *    Initializes the platform-specific hardware interface.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arInit(void)
{
  /* Initialize global variables */
  arPreemptiveHandler = NULL;
  arPreemptiveContext.StackAddress = NULL;
  arSystemTime = 0;

  /* Disable all interrupts for Timer/Counter0 and Timer/Counter1 */
  TIMSK &= ~(_BV(OCIE0) | _BV(TOIE0) | _BV(TICIE1) | _BV(OCIE1A) |
    _BV(OCIE1B) | _BV(TOIE1));
  ETIMSK &= ~_BV(OCIE1C);

  /* Configure Timer/Counter0 to provide the cyclic interrupt for
     preemption. Set prescaler to 64 (for 16.384 MHz clock) to provide
     a 1 ms interval for timer counter overflow. */
  ASSR = 0;
  TCNT0 = 0;
  TCCR0 = _BV(CS02);

  /* Configure Timer/Counter1 to provide a system timer with 1 ms
     precision. Set prescaler to 64 (for 16.384 MHz clock). */
  TCCR1A = 0;
  TCCR1B = _BV(CS21) | _BV(CS20);
  TCCR1C = 0;
  TCNT1 = 0;

  /* Clear all Timer0 and Timer1 interrupt flags.
     [!] Logic Warning: On AVR, flags are usually cleared by writing '1'.
     Using Read-Modify-Write (&= ~) may fail or clear other pending flags. */
  TIFR &= ~(_BV(OCF0) | _BV(TOV0) | _BV(ICF1) | _BV(OCF1A) | _BV(OCF1B) |
    _BV(TOV1));

  /* Enable interrupt timer counter overflow for both Timer0 and Timer1 */
  TIMSK |= _BV(TOIE0) | _BV(TOV1);

  /* Success */
  return TRUE;
}


/***************************************************************************/
#if (AR_USE_DEINIT)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arDeinit
 *
 * Description:
 * Deinitializes the platform hardware interface.
 *
 ***************************************************************************/

void arDeinit(void)
{
  /* Disable all interrupts for Timer/Counter0 and Timer/Counter1 */
  TIMSK &= ~(_BV(OCIE0) | _BV(TOIE0) | _BV(TICIE1) | _BV(OCIE1A) |
    _BV(OCIE1B) | _BV(TOIE1));
  ETIMSK &= ~_BV(OCIE1C);

  /* Disable Timer/Counter0 */
  ASSR = 0;
  TCNT0 = 0;
  TCCR0 = 0;

  /* Disable Timer/Counter1 */
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT1 = 0;

  /* Clear all Timer0 and Timer1 interrupt flags. */
  TIFR &= ~(_BV(OCF0) | _BV(TOV0) | _BV(ICF1) | _BV(OCF1A) | _BV(OCF1B) |
    _BV(TOV1));

  /* Release used memory resources */
  if(arPreemptiveContext.StackAddress)
    stMemFree(arPreemptiveContext.StackAddress);
}


/***************************************************************************/
#endif /* AR_USE_DEINIT */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arLock
 *
 *  Description:
 *    Disables global interrupts.
 *
 *  Return:
 *    The previous state of the interrupt enable flag.
 *
 ***************************************************************************/

BOOL arLock(void)
{
  UINT16 OldFlagState;

  /* Save the state of the interrupt enable flag and disable interrupts */
  OldFlagState = SREG >> 7;
  cli();

  /* Return the previous state */
  return OldFlagState;
}


/****************************************************************************
 *
 *  Name:
 *    arRestore
 *
 *  Description:
 *    Restores the global interrupt enable flag state.
 *
 *  Parameters:
 *    PreviousLockState - The previous state of the interrupt enable flag.
 *
 ***************************************************************************/

void arRestore(BOOL PreviousLockState)
{
  /* Restore the interrupt enable flag (interrupts are disabled after
     the corresponding call to arLock) */
  if(PreviousLockState)
    sei();
}


/****************************************************************************
 *
 *  Name:
 *    TIMER1_OVF_vect
 *
 *  Description:
 *    Timer/Counter1 interrupt service routine.
 *
 ***************************************************************************/

ISR(TIMER1_OVF_vect)
{
  /* Update the timer tick counter */
  arSystemTime += 0x100;

  /* Clear the interrupt flag. */
  TIFR &= ~(_BV(TOV1));
}


/****************************************************************************
 *
 *  Name:
 *    arGetTickCount
 *
 *  Description:
 *    Returns the total number of system timer ticks elapsed since startup.
 *
 *  Return:
 *    Total number of timer ticks.
 *
 ***************************************************************************/

TIME arGetTickCount(void)
{
  /* Check for pending overflow (race condition handling) */
  if(TIFR & _BV(TOV1))
  {
    arSystemTime += 0x100;
    TIFR &= ~(_BV(TOV1));
  }

  /* Return system time (Upper byte from variable, Lower byte from HW) */
  return arSystemTime + (TCNT1 >> 8);
}


/****************************************************************************
 *
 *  Name:
 *    TIMER0_OVF_vect
 *
 *  Description:
 *    Timer/Counter0 interrupt service routine. Aliased to the arYield
 *    function to implement preemption.
 *
 ***************************************************************************/

/* Configure arYield to be executed every millisecond */
ISR_ALIAS(TIMER0_OVF_vect, arYield);


/****************************************************************************
 *
 *  Name:
 *    arSetPreemptiveHandler
 *
 *  Description:
 *    Sets the handler for the preemptive procedure (scheduler).
 *
 *  Parameters:
 *    PreemptiveProc - Pointer to the preemptive callback function (or NULL
 *      to disable).
 *    StackSize - Stack size expected for the preemptive procedure call.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arSetPreemptiveHandler(TPreemptiveProc PreemptiveProc, SIZE StackSize)
{
  UINT8 *Stack;

  /* Remove previous handler */
  arPreemptiveHandler = NULL;

  /* Release used memory resources */
  if(arPreemptiveContext.StackAddress)
    stMemFree(arPreemptiveContext.StackAddress);

  /* Prepare new context for the scheduler */
  if(PreemptiveProc)
  {
    /* Allocate memory for the scheduler stack */
    arPreemptiveContext.StackAddress = stMemAlloc(StackSize);
    if(!arPreemptiveContext.StackAddress)
      return FALSE;

    /* Calculate the pointer to the top of the stack */
    Stack = (UINT8 FAR *) (PVOID)
      &((UINT8 FAR *) arPreemptiveContext.StackAddress)[StackSize & 0xFFFE];

    /* Assign the updated context */
    arPreemptiveContext.TaskContext = (PVOID) Stack;

    /* Set the new handler */
    arPreemptiveHandler = PreemptiveProc;
  }

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arCreateTaskContext
 *
 *  Description:
 *    Initializes the execution context for a new task.
 *    WARNING: The TaskStartupProc must not return; it should contain an
 *    infinite loop or explicitly terminate the task.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure.
 *    TaskStartupProc - Pointer to the task entry function.
 *    StackSize - Size of the task stack in bytes.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arCreateTaskContext(struct TTaskContext FAR *TaskContext,
  TTaskStartupProc TaskStartupProc, SIZE StackSize)
{
  UINT8 Counter, *Stack;

  /* Verify minimal stack size requirement */
  if(StackSize < (SIZE) 33)
  {
    stSetLastError(ERR_TOO_SMALL_STACK_SIZE);
    return FALSE;
  }

  /* Allocate memory for the task stack */
  TaskContext->StackAddress = stMemAlloc(StackSize);
  if(!TaskContext->StackAddress)
    return FALSE;

  /* Calculate the pointer to the top of the stack (aligned) */
  Stack = (UINT8 FAR *) (PVOID)
    &((UINT8 FAR *) TaskContext->StackAddress)[StackSize & 0xFFFE];

  /* Push Task Startup Procedure Address (Return Address).
     AVR Stack grows down. High byte pushed first (lower address)?
     Wait: PUSH decrements SP.
     Standard RET pops High then Low? Or Low then High?
     Code pushes Low byte, then High byte. */
  *(--Stack) = (UINT8) (((UINT16) TaskStartupProc) & 0xFF); /* Low */
  *(--Stack) = (UINT8) (((UINT16) TaskStartupProc) >> 8);   /* High */

  /* Default register values (R0, SREG, and R1..R31) */
  *(--Stack) = 0x00; /* R0 */
  *(--Stack) = 0x80; /* SREG (Interrupts Enabled) */

  Counter = 31; /* R31 down to R1 */
  while(Counter--)
    *(--Stack) = 0x00;

  /* Save the updated stack pointer to the context structure */
  TaskContext->TaskContext = (PVOID) (Stack - 1);
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arReleaseTaskContext
 *
 *  Description:
 *    Releases the resources associated with a task context.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arReleaseTaskContext(struct TTaskContext FAR *TaskContext)
{
  /* Free the allocated stack memory */
  return stMemFree(TaskContext->StackAddress);
}


/****************************************************************************
 *
 *  Name:
 *    arSavePower
 *
 *  Description:
 *    Enters low-power mode when the system is idle.
 *
 ***************************************************************************/

void arSavePower(void)
{
  /* Switch CPU to Idle Mode */
  sleep_cpu();
}


/***************************************************************************/
