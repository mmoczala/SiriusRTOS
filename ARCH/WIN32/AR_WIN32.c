/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_WIN32.c - Windows 32-bit port
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
 *  Global variables
 *
 ***************************************************************************/

static TPreemptiveProc arPreemptiveProc;


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
  arPreemptiveProc = NULL;

  /* Initialize the platform interface */
  if(!arWin32Init())
  {
    stSetLastError(ERR_CAN_NOT_INIT_ARCHITECTURE);
    return FALSE;
  }

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
 *  Description:
 *    Deinitializes the platform hardware interface.
 *
 ***************************************************************************/

void arDeinit(void)
{
  /* Deinitialize the platform interface */
  arWin32Deinit();
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
 *    Disables global interrupts to protect critical sections.
 *
 *  Return:
 *    The previous state of the interrupt enable flag.
 *
 ***************************************************************************/

BOOL arLock(void)
{
  /* Disable interrupts */
  return (BOOL) arWin32Lock();
}


/****************************************************************************
 *
 *  Name:
 *    arRestore
 *
 *  Description:
 *    Restores the global interrupt enable flag to a previous state.
 *
 *  Parameters:
 *    PreviousLockState - The previous state of the interrupt enable flag.
 *
 ***************************************************************************/

void arRestore(BOOL PreviousLockState)
{
  /* Restore the interrupt enable flag state */
  arWin32Restore((int) PreviousLockState);
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
  /* Return the tick counter value */
  return (TIME) arWin32GetTickCount();
}


/****************************************************************************
 *
 *  Name:
 *    arPreemptiveHandler
 *
 *  Description:
 *    Wrapper function that executes the registered preemptive callback.
 *
 *  Parameters:
 *    TaskContext - Pointer to the current task context structure.
 *
 ***************************************************************************/

static void arPreemptiveHandler(struct TTaskContext FAR *TaskContext)
{
  /* Execute the registered preemption handler */
  if(arPreemptiveProc)
    arPreemptiveProc(TaskContext);
}


/****************************************************************************
 *
 *  Name:
 *    arSetPreemptiveHandler
 *
 *  Description:
 *    Registers the system preemption handler (scheduler callback).
 *
 *  Parameters:
 *    PreemptiveProc - Pointer to the preemptive callback function (or NULL
 *       to disable).
 *    StackSize - Stack size required for the preemptive call.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arSetPreemptiveHandler(TPreemptiveProc PreemptiveProc, SIZE StackSize)
{
  /* Cache the preemptive handler pointer */
  arPreemptiveProc = PreemptiveProc;

  /* Register the wrapper function with the platform layer */
  if(!arWin32SetPreemptiveHandler(PreemptiveProc ?
    arPreemptiveHandler : NULL, (unsigned long) StackSize))
  {
    stSetLastError(ERR_CAN_NOT_SET_PREEMPT_HANDLER);
    return FALSE;
  }

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arYield
 *
 *  Description:
 *    Voluntarily yields the CPU to the next scheduled task.
 *
 ***************************************************************************/

void arYield(void)
{
  /* Yield execution to the platform scheduler */
  arWin32Yield();
}


/****************************************************************************
 *
 *  Name:
 *    arTaskStartupProc
 *
 *  Description:
 *    Wrapper function for the task entry point.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure.
 *
 ***************************************************************************/

static void arTaskStartupProc(struct TTaskContext FAR *TaskContext)
{
  /* Execute the task entry point stored in the Arg field */
  ((TTaskStartupProc) TaskContext->Arg)();
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
 *    TaskContext - Pointer to the task context structure to initialize.
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
  /* Store the entry point in the context argument field */
  TaskContext->Arg = TaskStartupProc;

  /* Initialize the platform-specific context */
  if(!arWin32CreateTaskContext(TaskContext, arTaskStartupProc,
    (unsigned long) StackSize))
  {
    stSetLastError(ERR_CAN_NOT_CREATE_TASK_CONTEXT);
    return FALSE;
  }

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arReleaseTaskContext
 *
 *  Description:
 *    Releases resources associated with a task context.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure to release.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL arReleaseTaskContext(struct TTaskContext FAR *TaskContext)
{
  /* Release the platform-specific context resources */
  if(!arWin32ReleaseTaskContext(TaskContext))
  {
    stSetLastError(ERR_CAN_NOT_REL_TASK_CONTEXT);
    return FALSE;
  }

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arSavePower
 *
 *  Description:
 *    Enters low-power mode when the system is idle. Execution resumes upon
 *    the next interrupt.
 *
 ***************************************************************************/

void arSavePower(void)
{
  /* Enter power-save mode */
  arWin32SavePower();
}


/***************************************************************************/
