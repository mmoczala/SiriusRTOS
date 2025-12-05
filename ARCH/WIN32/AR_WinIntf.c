/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_WinIntf.c - Windows Interface (Win32 Port)
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

#include <windows.h>
#include "AR_WinIntf.h"

/* Disable warning 8080 for Borland C Compiler */
#ifdef __BORLANDC__
  #pragma option -w-8080
#endif


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

static BOOL volatile arDeinitialize;
static TWin32Proc volatile arPreemptiveProc;

struct TTaskContext arCurrentTaskContext;
static HANDLE arSyncMutex;
static HANDLE arInterruptEnableFlag;
static HANDLE arInterruptThread;
static HANDLE arForceContextSwitchEvent;
static HANDLE arContextSwitchThread;


/****************************************************************************
 *
 *  Macros
 *
 ***************************************************************************/

/* Macro returns the event state (TRUE if signaled, FALSE otherwise). */
#define GET_EVENT_STATE(Event) \
  (WaitForSingleObject(Event, IGNORE) == WAIT_OBJECT_0)


/****************************************************************************
 *
 *  Name:
 *    arInterruptThreadProc
 *
 *  Description:
 *    Thread that generates a cyclic interrupt simulation. It forces
 *    a context switch at regular intervals.
 *
 *  Parameters:
 *    Arg - Unused parameter.
 *
 *  Return:
 *    Always returns 0.
 *
 ***************************************************************************/

static DWORD WINAPI arInterruptThreadProc(PVOID Arg)
{
  HANDLE Handles[3];
  LARGE_INTEGER liDueTime;

  /* Mark unused parameters */
  (void) Arg;

  /* Create a periodic timer */
  Handles[0] = CreateWaitableTimer(NULL, FALSE, NULL);

  /* Start periodic timer. QuadPart is in 100ns units. Negative value
     indicates relative time. 1ms = 10,000 units. */
  liDueTime.QuadPart = AR_WIN32_CTX_SWITCH_INTERVAL * -10000;
  SetWaitableTimer(Handles[0], &liDueTime, AR_WIN32_CTX_SWITCH_INTERVAL,
    NULL, NULL, FALSE);

  /* Prepare handles for the wait loop */
  Handles[1] = arInterruptEnableFlag;
  Handles[2] = arSyncMutex;

  while(!arDeinitialize)
  {
    /* Wait for: Timer Tick AND Interrupts Enabled AND Mutex Access.
       [!] Logic Warning: This wait will fail when handles are closed in
       arWin32Deinit. The return value is not checked, relying on the loop
       condition or crash/exit behavior for termination. */
    WaitForMultipleObjects(3, Handles, TRUE, INFINITE);

    /* Force a context switch */
    SetEvent(arForceContextSwitchEvent);

    /* Release synchronization mutex */
    ReleaseMutex(arSyncMutex);
  }

  /* Close local handles */
  CloseHandle(Handles[0]);
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    arContextSwitchThreadProc
 *
 *  Description:
 *    Thread that performs the actual context switch whenever the
 *    arForceContextSwitchEvent is set. It acts as the core scheduler entry.
 *
 *  Parameters:
 *    Arg - Unused parameter.
 *
 *  Return:
 *    Always returns 0.
 *
 ***************************************************************************/

static DWORD WINAPI arContextSwitchThreadProc(PVOID Arg)
{
  HANDLE Handles[2];

  /* Mark unused parameters */
  (void) Arg;

  /* Prepare handles for the wait loop */
  Handles[0] = arForceContextSwitchEvent;
  Handles[1] = arSyncMutex;

  while(!arDeinitialize)
  {
    /* Wait for: Context Switch Request AND Mutex Access.
       [!] Logic Warning: See arInterruptThreadProc regarding shutdown. */
    WaitForMultipleObjects(2, Handles, TRUE, INFINITE);

    /* Save the current task context (Suspend the actual Windows thread) */
    SuspendThread((HANDLE) arCurrentTaskContext.hTask);

    /* Save interrupt enable state for the current task */
    arCurrentTaskContext.InterruptEnable =
      GET_EVENT_STATE(arInterruptEnableFlag);

    /* Execute the Preemption Handler (Scheduler call) */
    if(arPreemptiveProc)
      arPreemptiveProc(&arCurrentTaskContext);

    /* Restore interrupt enable flag for the current task (now the new task) */
    if(arCurrentTaskContext.InterruptEnable)
      SetEvent(arInterruptEnableFlag);
    else
      ResetEvent(arInterruptEnableFlag);

    /* Restore the new task context (Resume the target Windows thread) */
    ResumeThread((HANDLE) arCurrentTaskContext.hTask);

    /* Signal yield operation finalization */
    SetEvent((HANDLE) arCurrentTaskContext.YieldCtrl);

    /* Release synchronization mutex */
    ReleaseMutex(arSyncMutex);
  }

  /* Return with success */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32Init
 *
 *  Description:
 *    Initializes the platform-specific interface components.
 *
 *  Return:
 *    TRUE (1) on success, FALSE (0) on failure.
 *
 ***************************************************************************/

int arWin32Init(void)
{
  HANDLE hProcess;

  /* Initialize global variables */
  arDeinitialize = FALSE;
  arPreemptiveProc = NULL;

  /* Define the current thread as the initial system context */
  hProcess = GetCurrentProcess();
  if(!DuplicateHandle(hProcess, GetCurrentThread(), hProcess,
    (HANDLE *) &arCurrentTaskContext.hTask, 0, TRUE, DUPLICATE_SAME_ACCESS))
  {
    return 0;
  }

  /* Create event for controlling yield operations */
  arCurrentTaskContext.YieldCtrl = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(!arCurrentTaskContext.YieldCtrl)
  {
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Create mutex for global synchronization */
  arSyncMutex = CreateMutex(NULL, TRUE, NULL);
  if(!arSyncMutex)
  {
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Create event for interrupt enable control.
     Manual Reset = TRUE, Initial State = FALSE (Interrupts Disabled) */
  arInterruptEnableFlag = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(!arInterruptEnableFlag)
  {
    CloseHandle(arSyncMutex);
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Create event for forcing context switching.
     Auto Reset = FALSE. */
  arForceContextSwitchEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if(!arForceContextSwitchEvent)
  {
    CloseHandle(arInterruptEnableFlag);
    CloseHandle(arSyncMutex);
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Create the interrupt generation thread */
  arInterruptThread = CreateThread(NULL, 0, arInterruptThreadProc,
    NULL, 0, NULL);
  if(!arInterruptThread)
  {
    CloseHandle(arForceContextSwitchEvent);
    CloseHandle(arInterruptEnableFlag);
    CloseHandle(arSyncMutex);
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Create the context switching thread */
  arContextSwitchThread = CreateThread(NULL, 0, arContextSwitchThreadProc,
    NULL, 0, NULL);
  if(!arContextSwitchThread)
  {
    CloseHandle(arInterruptThread);
    CloseHandle(arForceContextSwitchEvent);
    CloseHandle(arInterruptEnableFlag);
    CloseHandle(arSyncMutex);
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
    return 0;
  }

  /* Set high priority for the scheduler and release the initial mutex lock */
  SetThreadPriority(arContextSwitchThread, THREAD_PRIORITY_ABOVE_NORMAL);
  ReleaseMutex(arSyncMutex);

  /* Initialization success */
  return 1;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32Deinit
 *
 *  Description:
 *    Deinitializes the platform interface and releases resources.
 *
 ***************************************************************************/

void arWin32Deinit(void)
{
  /* Signal deinitialization start */
  arDeinitialize = TRUE;
  arPreemptiveProc = NULL;

  /* Close context switching thread handle */
  if(arContextSwitchThread)
    CloseHandle(arContextSwitchThread);

  /* Close interrupt generating thread handle */
  if(arInterruptThread)
    CloseHandle(arInterruptThread);

  /* Close context switch event */
  if(arForceContextSwitchEvent)
    CloseHandle(arForceContextSwitchEvent);

  /* Close interrupt enable flag */
  if(arInterruptEnableFlag)
    CloseHandle(arInterruptEnableFlag);

  /* Close synchronization mutex */
  if(arSyncMutex)
    CloseHandle(arSyncMutex);

  /* Close yield control event */
  if(arCurrentTaskContext.YieldCtrl)
    CloseHandle((HANDLE) arCurrentTaskContext.YieldCtrl);

  /* Close the duplicated handle of the current thread */
  if(arCurrentTaskContext.hTask)
    CloseHandle((HANDLE) arCurrentTaskContext.hTask);
}


/****************************************************************************
 *
 *  Name:
 *    arWin32Lock
 *
 *  Description:
 *    Disables "interrupts" by resetting the interrupt enable event.
 *
 *  Return:
 *    The previous state of the interrupt enable flag (1 if enabled, 0 otherwise).
 *
 ***************************************************************************/

int arWin32Lock(void)
{
  int WaitResult;

  /* Enter critical section */
  WaitForSingleObject(arSyncMutex, INFINITE);

  /* Check if a context switch was requested while we were running.
     The GET_EVENT_STATE macro (via WaitForSingleObject) will automatically
     reset the auto-reset event if it was set. We save this state to
     process it later in arWin32Restore. */
  arCurrentTaskContext.DelayedContextSwitch =
    GET_EVENT_STATE(arForceContextSwitchEvent);

  /* Capture current interrupt enable status */
  WaitResult = GET_EVENT_STATE(arInterruptEnableFlag);

  /* Disable interrupts by resetting the event */
  ResetEvent(arInterruptEnableFlag);

  /* Leave critical section */
  ReleaseMutex(arSyncMutex);

  /* Return previous state */
  return WaitResult;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32Restore
 *
 *  Description:
 *    Restores the interrupt enable flag to the specified state.
 *
 *  Parameters:
 *    PreviousLockState - The previous state of the interrupt enable flag.
 *
 ***************************************************************************/

void arWin32Restore(int PrevLockState)
{
  /* Restore the state of the interrupt flag if it was previously enabled */
  if(PrevLockState)
  {
    /* Enter critical section */
    WaitForSingleObject(arSyncMutex, INFINITE);

    /* Enable interrupts */
    SetEvent(arInterruptEnableFlag);

    /* If a context switch was delayed during the lock, force it now */
    if(arCurrentTaskContext.DelayedContextSwitch)
      SetEvent(arForceContextSwitchEvent);

    /* Leave critical section */
    ReleaseMutex(arSyncMutex);
  }
}


/****************************************************************************
 *
 *  Name:
 *    arWin32Yield
 *
 *  Description:
 *    Voluntarily yields execution of the current task.
 *
 ***************************************************************************/

void arWin32Yield(void)
{
  /* Enter critical section */
  WaitForSingleObject(arSyncMutex, INFINITE);

  /* Reset the yield control event to prepare for synchronization */
  ResetEvent((HANDLE) arCurrentTaskContext.YieldCtrl);

  /* Force a context switch */
  SetEvent(arForceContextSwitchEvent);

  /* Leave critical section */
  ReleaseMutex(arSyncMutex);

  /* Block until the yield operation is completed (scheduler resumes us) */
  WaitForSingleObject((HANDLE) arCurrentTaskContext.YieldCtrl, INFINITE);
}


/****************************************************************************
 *
 *  Name:
 *    arWin32SavePower
 *
 *  Description:
 *    Called when the CPU is idle. Switches to power-save mode (suspends
 *    execution) until an interrupt occurs.
 *
 ***************************************************************************/

void arWin32SavePower(void)
{
  /* Enter critical section */
  WaitForSingleObject(arSyncMutex, INFINITE);

  /* Reset yield control, but DO NOT force a context switch immediately.
     We rely on the interrupt thread to eventually trigger the switch. */
  ResetEvent((HANDLE) arCurrentTaskContext.YieldCtrl);

  /* Leave critical section */
  ReleaseMutex(arSyncMutex);

  /* Halt execution until the timer interrupt triggers a context switch
     and the scheduler eventually resumes this thread. */
  WaitForSingleObject((HANDLE) arCurrentTaskContext.YieldCtrl, INFINITE);
}


/****************************************************************************
 *
 *  Name:
 *    arWin32GetTickCount
 *
 *  Description:
 *    Returns the total number of system timer ticks elapsed.
 *
 *  Return:
 *    Total number of timer ticks.
 *
 ***************************************************************************/

unsigned long arWin32GetTickCount(void)
{
  /* Return system tick count */
  return GetTickCount();
}


/****************************************************************************
 *
 *  Name:
 *    arWin32SetPreemptiveHandler
 *
 *  Description:
 *    Sets the handler for the preemptive procedure (scheduler).
 *
 *  Parameters:
 *    PreemptiveProc - Pointer to the handler function (or NULL to disable).
 *    StackSize - Stack size expected for the handler call.
 *
 *  Return:
 *    TRUE (1) on success, FALSE (0) on failure.
 *
 ***************************************************************************/

int arWin32SetPreemptiveHandler(TWin32Proc PreemptiveProc,
  unsigned long StackSize)
{
  /* Stack size validation */
  if(!StackSize)
    return 0;

  /* Enter critical section */
  WaitForSingleObject(arSyncMutex, INFINITE);

  /* Register the preemptive procedure */
  arPreemptiveProc = PreemptiveProc;

  /* Leave critical section */
  ReleaseMutex(arSyncMutex);

  /* Success */
  return 1;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32TaskStartup
 *
 *  Description:
 *    Wrapper thread procedure that executes the task startup code.
 *
 *  Parameters:
 *    Arg - Unused parameter.
 *
 *  Return:
 *    Always returns 0.
 *
 ***************************************************************************/

static DWORD WINAPI arWin32TaskStartup(LPVOID Arg)
{
  /* Mark unused parameters */
  (void) Arg;

  /* Execute the task startup procedure using the global context.
     Note: The scheduler must ensure arCurrentTaskContext matches this
     thread before resuming it. */
  arCurrentTaskContext.TaskStartupProc(&arCurrentTaskContext);
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32CreateTaskContext
 *
 *  Description:
 *    Creates a new task context (thread).
 *    WARNING: The TaskStartupProc must not return. It should contain an
 *    infinite loop or the task must be terminated explicitly.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure.
 *    TaskStartupProc - Pointer to the task entry function.
 *    StackSize - Size of the task stack.
 *
 *  Return:
 *    TRUE (1) on success, FALSE (0) on failure.
 *
 ***************************************************************************/

int arWin32CreateTaskContext(struct TTaskContext *TaskContext,
  TWin32Proc TaskStartupProc, unsigned long StackSize)
{
  /* Enable interrupts by default for the new task */
  TaskContext->InterruptEnable = 1;

  /* Initialize task context fields */
  TaskContext->TaskStartupProc = TaskStartupProc;

  /* Create the thread in SUSPENDED state */
  TaskContext->hTask = CreateThread(NULL, StackSize, arWin32TaskStartup,
    (LPVOID) 0, CREATE_SUSPENDED, NULL);
  if(!TaskContext->hTask)
    return 0;

  /* Create event for controlling yield synchronization */
  TaskContext->YieldCtrl = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(!TaskContext->YieldCtrl)
  {
    TerminateThread((HANDLE) TaskContext->hTask, 0);
    CloseHandle((HANDLE) TaskContext->hTask);
    return 0;
  }

  /* Success */
  return 1;
}


/****************************************************************************
 *
 *  Name:
 *    arWin32ReleaseTaskContext
 *
 *  Description:
 *    Releases a task context and associated system resources.
 *
 *  Parameters:
 *    TaskContext - Pointer to the task context structure.
 *
 *  Return:
 *    TRUE (1) on success, FALSE (0) on failure.
 *
 ***************************************************************************/

int arWin32ReleaseTaskContext(struct TTaskContext *TaskContext)
{
  /* Enter critical section */
  WaitForSingleObject(arSyncMutex, INFINITE);

  /* Terminate the thread and close handles */
  TerminateThread((HANDLE) TaskContext->hTask, 0);
  CloseHandle((HANDLE) TaskContext->hTask);
  CloseHandle((HANDLE) TaskContext->YieldCtrl);

  /* Leave critical section */
  ReleaseMutex(arSyncMutex);

  /* Success */
  return 1;
}


/***************************************************************************/
