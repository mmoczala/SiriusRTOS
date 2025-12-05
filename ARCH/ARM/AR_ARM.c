/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_ARM.c - Generic ARM port
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
 *  Definitions
 *
 ***************************************************************************/

/* Processor State Definitions */
#define FLAG_MODE_SUPERVISOR            0x00000013
#define FLAG_THUMB_MODE                 0x00000020


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

TPreemptiveProc arPreemptiveHandler;
struct TTaskContext arCurrTaskContext;


/****************************************************************************
 *
 *  Assemble exported functions
 *
 ***************************************************************************/

void arIRQPreemption(void);


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
  /* Reset the preemption handler */
  arPreemptiveHandler = NULL;

  /* Initialize platform */
  return arARMInitHook(arIRQPreemption);
}


/***************************************************************************/
#if (AR_USE_DEINIT)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arDeinit
 *
 *  Description:
 *    Deinitializes the platform hardware interface.
 *
 ***************************************************************************/

void arDeinit(void)
{
  /* Deinitialize platform */
  arARMDeinitHook();
}


/***************************************************************************/
#endif /* AR_USE_DEINIT */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arGetTickCount
 *
 *  Description:
 *    Returns the total number of Real-Time Timer (RTT) ticks elapsed since
 *    startup.
 *
 *  Return:
 *    Total number of RTT ticks.
 *
 ***************************************************************************/

TIME arGetTickCount(void)
{
  /* Returns total number of the Real-Time Timer ticks */
  return arGetTickCountHook();
}


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
 *    StackSize - Stack size expected for the preemptive call.
 *      NOTE: This value is ignored on ARM processors. Interrupt handlers
 *      are executed in Supervisor or IRQ Mode, which have dedicated stacks
 *      defined at startup.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arSetPreemptiveHandler(TPreemptiveProc PreemptiveProc, SIZE StackSize)
{
  /* Mark parameters as unused */
  AR_UNUSED_PARAM(StackSize);

  /* Register the preemptive handler */
  arPreemptiveHandler = PreemptiveProc;
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
  UINT32 FAR *Stack;

  /* Verify minimal stack size requirement */
  if(StackSize < (SIZE) 64)
  {
    stSetLastError(ERR_TOO_SMALL_STACK_SIZE);
    return FALSE;
  }

  /* Allocate memory for the task stack */
  TaskContext->StackAddress = stMemAlloc(StackSize);
  if(!TaskContext->StackAddress)
    return FALSE;

  /* Calculate the initial stack pointer (aligned to 4 bytes) */
  Stack = (UINT32 FAR *) (PVOID)
    (((UINT32) TaskContext->StackAddress) + (StackSize & 0xFFFFFFFC));

  /* Store the task entry point */
  *((TTaskStartupProc *) (--Stack)) = TaskStartupProc;

  /* Initialize general purpose registers (R0 to R12 and LR) to zero */
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;
  *(--Stack) = 0x00000000;

  /* Initialize the CPSR (Current Program Status Register)
     Default: Supervisor Mode, IRQ and FIQ enabled.
     If the startup address is odd, enable Thumb Mode. */
  *(--Stack) = FLAG_MODE_SUPERVISOR |
    ((((UINT32) TaskStartupProc) & 1) ? FLAG_THUMB_MODE : 0);

  /* Save the updated stack pointer to the context structure */
  TaskContext->TaskContext = (PVOID) Stack;
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
 *    Enters low-power mode (Idle) when the system is idle.
 *    The CPU clock is disabled until the next interrupt, but data transfers
 *    from other bus masters (e.g., DMA) continue.
 *
 ***************************************************************************/

void arSavePower(void)
{
  /* Switches CPU to Idle Mode. */
  arSavePowerHook();
}


/***************************************************************************/
