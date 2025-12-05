/****************************************************************************
 *
 *  AR_x86RM.c - 80x86 real mode port
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

/* Preemptive procedure handler */
static TPreemptiveProc arDefPreemptiveHandler;
TPreemptiveProc arPreemptiveHandler;
struct TTaskContext arPreemptiveContext;
struct TTaskContext arCurrTaskContext;
void FAR *arPreviousPITHandler;

/* Programmable Interval Timer */
volatile static TIME FAR *TickCount;


/****************************************************************************
 *
 *  Assemble exported functions
 *
 ***************************************************************************/

void CALLBACK arPITInit(void);
void CALLBACK arPITDeinit(void);
void CALLBACK arDisableInt(void);
void CALLBACK arEnableInt(void);
UINT16 CALLBACK arIntState(void);
void CALLBACK arForceInterrupt(void);
UINT16 CALLBACK arGetDS(void);
UINT16 CALLBACK arGetES(void);


/****************************************************************************
 *
 *  Name:
 *    arInit
 *
 *  Description:
 *    Initializes platform interface.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL arInit(void)
{
  /* Preemptive procedure stack is not assigned jet */
  arPreemptiveContext.StackAddress = NULL;

  /* Initialize Programmable Interval Timer */
  arPITInit();

  /* Initialize global variables */
  TickCount = (TIME FAR *) 0x0000046CUL;
  *TickCount = 0UL;

  /* Return success */
  return TRUE;
}


/***************************************************************************/
#if AR_USE_DEINIT
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    arDeinit
 *
 *  Description:
 *    Deinitializes platform interface.
 *
 ***************************************************************************/

void arDeinit(void)
{
  /* Deinitialize Programmable Interval Timer */
  arPITDeinit();

  /* Release preemptive prcedure stack */
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
 *    Disables interrupts.
 *
 *  Return:
 *    Previous state of the interrupts enable flag.
 *
 ***************************************************************************/

BOOL arLock(void)
{
  UINT16 OldFlagState;

  /* Remember state of the interrupts enable flag and disable interrupts */
  OldFlagState = arIntState();
  arDisableInt();

  /* Return state of the interrupts enable flag */
  return OldFlagState;
}


/****************************************************************************
 *
 *  Name:
 *    arRestore
 *
 *  Description:
 *    Restores interrupts enable flag state.
 *
 *  Parameters:
 *    PreviousLockState - Previous state of the interrupts enable flag.
 *
 ***************************************************************************/

void arRestore(BOOL PreviousLockState)
{
  /* Restore interrupts enable flag (interrupts are disabled after
     coresponding call of arLock function) */
  if(PreviousLockState)
    arEnableInt();
}


/****************************************************************************
 *
 *  Name:
 *    arGetTickCount
 *
 *  Description:
 *    Function returns total number of the Programmable Interval Timer
 *    ticks counted from the timer start.
 *
 *  Return:
 *    Total number of the Programmable Interval Timer ticks.
 *
 ***************************************************************************/

TIME arGetTickCount(void)
{
  BOOL PrevLockState;
  TIME CurrentTime;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Read timer counter value */
  CurrentTime = *TickCount;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return timer counter value */
  return *TickCount;
}


/****************************************************************************
 *
 *  Name:
 *    arPrepareContext
 *
 *  Description:
 *    Function creates a stack for specified context.
 *
 *  Parameters:
 *    TaskContext - Pointer to task context to prepare.
 *    StackSize - Size of the stack.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

static BOOL arPrepareContext(struct TTaskContext FAR *TaskContext,
  SIZE StackSize)
{
  UINT16 FAR *StackAddress;

  /* Allocate memory for stack, with additional 15 bytes for adjustment */
  TaskContext->StackAddress = stMemAlloc((SIZE) (StackSize + (SIZE) 15));
  if(!TaskContext->StackAddress)
    return FALSE;

  /* Adjust stack start address to 16 bytes. In upper 16 bits of stack
     pointer store stack segment and in lower 16 bits store stack size
     (pointer will points to top of the stack) */
  TaskContext->TaskContext =
    (PVOID) ((UINT8 FAR *) TaskContext->StackAddress + 15UL);
  StackAddress = (UINT16 FAR *) &TaskContext->TaskContext;
  StackAddress[1] += (UINT16) (StackAddress[0] >> 4);
  StackAddress[0] = (UINT16) StackSize;
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arSetPreemptiveHandler
 *
 *  Description:
 *    Sets handler of the preemptive procedure. Function allocates a memory
 *    for individual context of preemptive procedure.
 *
 *  Parameters:
 *    PreemptiveProc - Handler of the preemptive procedure (or NULL to
 *      disable previous).
 *    StackSize - Stack size expected for peemptive procedure call.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL arSetPreemptiveHandler(TPreemptiveProc PreemptiveProc, SIZE StackSize)
{
  BOOL PrevLockState;

  /* Save oryginal (default) preemptive handler */
  if(!arPreemptiveContext.StackAddress)
    arDefPreemptiveHandler = arPreemptiveHandler;

  /* Uninitialize handler if NULL is specified */
  else
  {
    /* Enter critical section */
    PrevLockState = arLock();

    /* Release preemptive procedure context (stack) if already defined */
    arPreemptiveHandler = arDefPreemptiveHandler;
    stMemFree(arPreemptiveContext.StackAddress);
    arPreemptiveContext.StackAddress = NULL;
    arPreemptiveHandler = NULL;

    /* Leave critical section */
    arRestore(PrevLockState);    

    /* Return with success */
    return TRUE;
  }

  /* Create context for preemptive procedure */
  if(!arPrepareContext(&arPreemptiveContext, StackSize))
    return FALSE;

  /* Set handler of the preemptive procedure */
  arPreemptiveHandler = PreemptiveProc;
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arYield
 *
 *  Description:
 *    Function voluntary yields execution of the current task.
 *
 ***************************************************************************/

void arYield(void)
{
  /* Current task voluntary yields its execution */
  arForceInterrupt();
}


/****************************************************************************
 *
 *  Name:
 *    arCreateTaskContext
 *
 *  Description:
 *    Function creates a task context. The task startup procedure passed
 *    to this functions can not be never ended. It should contain infinite
 *    loop or the task must be terminated before ending.
 *
 *  Parameters:
 *    TaskContext - Pointer to task context structure.
 *    TaskStartupProc - Task startup code.
 *    StackSize - Size of the task stack.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL arCreateTaskContext(struct TTaskContext FAR *TaskContext,
  TTaskStartupProc TaskStartupProc, SIZE StackSize)
{
  UINT16 FAR *Stack;
  UINT16 RegSP;


  /* Check minimal task stack size (expected for context dump) */
  if(StackSize < (SIZE) 26)
  {
    stSetLastError(ERR_TO_SMALL_STACK_SIZE);
    return FALSE;
  }

  /* Prepare task context (create stack) */
  if(!arPrepareContext(TaskContext, StackSize))
    return FALSE;


  /* Fill task stack with default values */
  Stack = (UINT16 FAR *) TaskContext->TaskContext;

  /* Default flags register value */
  *(--Stack) = 0x0202;

  /* Task startup procedure address */
  Stack -= 2;
  *((TTaskStartupProc *) Stack) = TaskStartupProc;

  /* Default register values (AX, CX, DX, BX, SP, BP, SI, DI, DS, ES) */
  RegSP = (UINT16) Stack;
  *(--Stack) = 0x0000;
  *(--Stack) = 0x0000;
  *(--Stack) = 0x0000;
  *(--Stack) = 0x0000;
  *(--Stack) = RegSP;
  *(--Stack) = 0x0000;
  *(--Stack) = 0x0000;
  *(--Stack) = 0x0000;
  *(--Stack) = arGetDS();
  *(--Stack) = arGetES();


  /* Assign updated task context */
  TaskContext->TaskContext = (PVOID) Stack;
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    arReleaseTaskContext
 *
 *  Description:
 *    Function releases a task context.
 *
 *  Parameters:
 *    TaskContext - Pointer to task context structure.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL arReleaseTaskContext(struct TTaskContext FAR *TaskContext)
{
  /* Release context stack */
  return stMemFree(TaskContext->StackAddress);
}


/****************************************************************************
 *
 *  Name:
 *    arSavePower
 *
 *  Description:
 *    Function is called every time, when CPU is not used by any task. It
 *    allows switching the CPU to power-save mode, which resumes the CPU
 *    on every interrupt.
 *
 ***************************************************************************/

void arSavePower(void)
{
}


/***************************************************************************/
