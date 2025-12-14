/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_ARMHooks.c - ARM Architecture Hooks
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

#include "AR_API.h"
#include "ST_API.h"


/****************************************************************************
 *
 *  Name:
 *    arARMInitHook
 *
 *  Description:
 *    Initializes the platform-specific hardware interface.
 *
 *  Parameters:
 *    IRQHandler - Pointer to the IRQ handler function used to implement
 *      preemption.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arARMInitHook(void (* IRQHandler)(void))
{
  /* TO DO: Write your code here */
  /* ... */
}


/****************************************************************************
 *
 *  Name:
 *    arARMDeinitHook
 *
 *  Description:
 *    Deinitializes the platform-specific hardware interface.
 *
 ***************************************************************************/

void arARMDeinitHook(void)
{
  /* TO DO: Write your code here */
  /* ... */
}


/****************************************************************************
 *
 *  Name:
 *    arGetTickCountHook
 *
 *  Description:
 *    Returns the total number of system timer ticks elapsed since startup.
 *
 *  Return:
 *    Total number of timer ticks.
 *
 ***************************************************************************/

TIME arGetTickCountHook(void)
{
  /* TO DO: Write your code here */
  /* ... */
}


/****************************************************************************
 *
 *  Name:
 *    arIRQHandlerHook
 *
 *  Description:
 *    Called every time a preemption interrupt occurs. This function should be
 *    used to acknowledge/clear the interrupt source.
 *    WARNING: This function executes in ARM IRQ Mode. Be careful with stack
 *    usage. Registers other than R0-R3 must be saved/restored if used.
 *
 ***************************************************************************/

void arIRQHandlerHook(void)
{
  /* TO DO: Write your code here */
  /* ... */
}


/****************************************************************************
 *
 *  Name:
 *    arSavePowerHook
 *
 *  Description:
 *    Called whenever the CPU is idle (not used by any task). It allows
 *    switching the CPU to a low-power mode, which should resume execution
 *    upon the next interrupt.
 *
 ***************************************************************************/

void arSavePowerHook(void)
{
  /* TO DO: Write your code here */
  /* ... */
}


/***************************************************************************/
