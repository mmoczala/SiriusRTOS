/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Task.c - Task management functions
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

#include "OS_Core.h"


/****************************************************************************
 *
 *  Name:
 *    osExecuteCurrentTask
 *
 *  Description:
 *    Task startup procedure. Executes the task startup function with the
 *    specified argument. When the task function finishes, the returned
 *    value is used as the error code passed to osExitTask.
 *    This procedure will never return, as the task is terminated before
 *    the function scope ends.
 *
 ***************************************************************************/

static void CALLBACK osExecuteCurrentTask(void)
{
  /* Execute the task and terminate when it ends */
  osExitTask(osCurrentTask->TaskProc(osCurrentTask->Arg));
}


/****************************************************************************
 *
 *  Name:
 *    osCreateTask
 *
 *  Description:
 *    Creates a new task.
 *
 *  Parameters:
 *    TaskProc - Address of the task startup function.
 *    Arg - Argument passed to the task startup function.
 *    StackSize - Task stack size (if 0, the default size is assigned).
 *    Priority - Task priority.
 *    Suspended - Determines whether the created task should start suspended.
 *
 *  Return:
 *    Handle of the newly created task or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateTask(TTaskProc TaskProc, PVOID Arg, SIZE StackSize,
  UINT8 Priority, BOOL Suspended)
{
  struct TTask FAR *Task;
  BOOL PrevLockState;

  /* Mark unused parameters */
  #if !(OS_SUSP_RES_TASK_FUNC)
    AR_UNUSED_PARAM(Suspended);
  #endif

  /* Check parameters */
  if(Priority > OS_LOWEST_USED_PRIORITY)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Allocate memory for the task object */
  Task = (struct TTask FAR *) osMemAlloc(sizeof(*Task));
  if(!Task)
    return NULL_HANDLE;

  /* Register new system object */
  if(!osRegisterObject((PVOID) Task, &Task->Object, OS_OBJECT_TYPE_TASK))
  {
    osMemFree(Task);
    return NULL_HANDLE;
  }

  /* [!] NOTE: Terminating the current task at this moment will cause an
     error during the osDeleteObject call from osReleaseTaskResources. */

  /* Create task context */
  if(!arCreateTaskContext(&Task->TaskContext, osExecuteCurrentTask,
    StackSize ? StackSize : OS_DEFAULT_TASK_STACK_SIZE))
  {
    stSetLastError(ERR_CAN_NOT_CREATE_TASK_CONTEXT);

    /* [!] ISSUE: osDeleteObject may fail if object deletion is disabled */
    osDeleteObject(&Task->Object);
    return NULL_HANDLE;
  }

  /* Task is not in the signaled state until termination */
  Task->Object.Signal.Signaled = (INDEX) FALSE;

  /* Task startup function and its argument */
  Task->TaskProc = TaskProc;
  Task->Arg = Arg;

  /* Task priority */
  Task->Priority = Priority;
  #if (OS_USE_CSEC_OBJECTS)
    Task->AssignedPriority = Priority;
    Task->PriorityPath.Task = Task;
    Task->PriorityPath.CS = NULL;
  #endif

  /* Last time quantum assign time */
  Task->LastQuantumTime = osLastQuantumTime;
  Task->LastQuantumIndex = osLastQuantumIndex++;

  /* Default number of the time quanta assigning to the task */
  #if (OS_USE_TIME_QUANTA)
    Task->MaxTimeQuantum = 1;
  #endif

  /* Task state (suspended or not) */
  #if (OS_SUSP_RES_TASK_FUNC)
    Task->BlockingFlags = (UINT8) (Suspended ? OS_BLOCK_FLAG_SUSPENDED : 0);
  #else
    Task->BlockingFlags = 0;
  #endif

  /* Time notification for task sleeping or wait timeout */
  #if (OS_USE_WAITING_WITH_TIME_OUT)
    osInitTimeNotify(&Task->WaitTimeout);
    Task->WaitTimeout.Task = Task;
  #endif

  /* Direct read-write for IPC */
  #if (OS_MBOX_ALLOW_DIRECT_RW)
    Task->IPCBlockingTask = NULL;
  #endif

  /* Binary search trees for owned critical sections */
  #if (OS_USE_CSEC_OBJECTS)
    stPQueueInit(&Task->OwnedCS, osCSAssocCmp);
    stBSTreeInit(&Task->OwnedCSPtr, osCSPtrCmp);
  #endif

  /* Binary search tree for task children (created or opened objects) */
  #if (OS_ALLOW_OBJECT_DELETION)
    stBSTreeInit(&Task->Childs, osObjectByHandleCmp);
  #endif

  /* CPU usage */
  #if (OS_USE_STATISTICS)
    Task->CPUUsageTime = OS_INFINITE;
    Task->CPUUsage = 0;
    Task->CPUCalcTime = osCPUUsageTime;
    Task->CPUCalc = 0;
  #endif

  /* Last error code */
  Task->LastErrorCode = ERR_NO_ERROR;

  /* Mark task object as ready to use */
  Task->Object.Flags |= OS_OBJECT_FLAG_READY_TO_USE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Make task ready to run. If priority of new task is higher, it will
     run immediately */
  osMakeReady(Task);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return handle of the new task */
  return Task->Object.Handle;
}


/****************************************************************************
 *
 *  Name:
 *    osExitTask
 *
 *  Description:
 *    Terminates the current task.
 *
 *  Parameters:
 *    ExitCode - Task exit code.
 *
 *  Return:
 *    None.
 *
 ***************************************************************************/

void osExitTask(ERROR ExitCode)
{
  /* Skip if operating system is not running */
  if(!osCurrentTask || osInISR)
    return;

  /* Release all system objects created or opened by current task */
  #if ((OS_ALLOW_OBJECT_DELETION) || (OS_USE_CSEC_OBJECTS))
    osReleaseTaskResources(osCurrentTask);
  #endif

  /* Enter critical section (leaving critical section is not necessary) */
  arLock();

  /* Mark current task as terminated and set exit code */
  osCurrentTask->LastErrorCode = ExitCode;
  osCurrentTask->BlockingFlags |= OS_BLOCK_FLAG_TERMINATED;

  /* Notify that task has changed to signaled state */
  osUpdateSignalState(&osCurrentTask->Object.Signal, TRUE);

  /* Remove it from ready-to-run tasks queue. When this is done, the task
     will no longer run */
  osMakeNotReady(osCurrentTask);
}


/***************************************************************************/
#if (OS_GET_TASK_HANDLE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetTaskHandle
 *
 *  Description:
 *    Returns the handle of the current task.
 *
 *  Return:
 *    Handle of the current task or NULL_HANDLE when operating system is not
 *    running or function is called from an ISR.
 *
 ***************************************************************************/

HANDLE osGetTaskHandle(void)
{
  /* Return handle of the current task */
  return (osCurrentTask && !osInISR) ?
    osCurrentTask->Object.Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_GET_TASK_HANDLE_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_TASK_EXIT_CODE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetTaskExitCode
 *
 *  Description:
 *    Retrieves the task termination code.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    ExitCode - Pointer to store the task termination error code.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetTaskExitCode(HANDLE Handle, ERROR *ExitCode)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Check termination flag */
  if(!(Task->BlockingFlags & OS_BLOCK_FLAG_TERMINATED))
  {
    stSetLastError(ERR_TASK_NOT_TERMINATED);
    return FALSE;
  }

  /* Set the task termination code */
  *ExitCode = Task->LastErrorCode;
  return TRUE;
}


/***************************************************************************/
#endif /* OS_TASK_EXIT_CODE_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_TERMINATE_TASK_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osTerminateTask
 *
 *  Description:
 *    Terminates the specified task.
 *
 *  Parameters:
 *    Handle - Task handle.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osTerminateTask(HANDLE Handle)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Use osExitTask for current task termination */
  if(osCurrentTask && !osInISR)
    if(Handle == osCurrentTask->Object.Handle)
      osExitTask(ERR_TASK_TERMINATED_BY_OTHER);

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Mark current task as terminating. This will suspend the terminating
     task execution to avoid resource use while they are being released.
     There is no way to exit the task from this state. After resources
     are released, the task termination will be continued. */
  Task->BlockingFlags |= OS_BLOCK_FLAG_TERMINATING;
  osMakeNotReady(Task);

  /* Make task not waiting */
  if(Task->BlockingFlags & OS_BLOCK_FLAG_WAITING)
    osMakeNotWaiting(Task);

  /* Make not sleeping */
  if(Task->BlockingFlags & OS_BLOCK_FLAG_SLEEP)
    osUnregisterTimeNotify(&Task->WaitTimeout);

  /* Release task blocked during direct read-write operation */
  #if (OS_USE_IPC_DIRECT_RW)
    if(Task->IPCBlockingTask)
    {
      Task->IPCBlockingTask->WaitExitCode = ERR_DATA_TRANSFER_FAILURE;
      /* ... osMakeNotWaiting(Task->IPCBlockingTask); */
      Task->IPCBlockingTask->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_IPC;
      osMakeReady(Task->IPCBlockingTask);
    }
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Release all owned critical sections and close all created and opened
     system objects */
  #if ((OS_ALLOW_OBJECT_DELETION) || (OS_USE_CSEC_OBJECTS))
    osReleaseTaskResources(Task);
  #endif

  /* Mark current task as terminated and set exit code */
  Task->LastErrorCode = ERR_TASK_TERMINATED_BY_OTHER;
  Task->BlockingFlags |= OS_BLOCK_FLAG_TERMINATED;

  /* Notify that task has changed to signaled state */
  osUpdateSignalState(&osCurrentTask->Object.Signal, TRUE);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_TERMINATE_TASK_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_SUSP_RES_TASK_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSuspendTask
 *
 *  Description:
 *    Suspends the specified task execution.
 *
 *  Parameters:
 *    Handle - Task handle.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSuspendTask(HANDLE Handle)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* [!] NOTE: Task cannot be BLOCKED when it is suspending... */

  /* Mark task as suspended */
  Task->BlockingFlags |= OS_BLOCK_FLAG_SUSPENDED;
  osMakeNotReady(Task);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osResumeTask
 *
 *  Description:
 *    Resumes the specified task.
 *
 *  Parameters:
 *    Handle - Task handle.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osResumeTask(HANDLE Handle)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Mark task as resumed (non-suspended). If it has higher priority than
     current task, it will run immediately */
  Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_SUSPENDED;
  osMakeReady(Task);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_SUSP_RES_TASK_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_TASK_PRIORITY_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetTaskPriority
 *
 *  Description:
 *    Returns the priority of the specified task. The returned priority is
 *    the assigned (not inherited) priority value.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    Priority - Pointer to variable that receives the task priority.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetTaskPriority(HANDLE Handle, UINT8 *Priority)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task priority */
  #if (OS_USE_CSEC_OBJECTS)
    *Priority = ((struct TTask FAR *) Object->ObjectDesc)->AssignedPriority;
  #else
    *Priority = ((struct TTask FAR *) Object->ObjectDesc)->Priority;
  #endif

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osSetTaskPriority
 *
 *  Description:
 *    Sets the priority of the specified task. If the new priority is higher
 *    than the priority of the current task, the specified task will run
 *    immediately.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    Priority - New task priority.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSetTaskPriority(HANDLE Handle, UINT8 Priority)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Check parameters */
  if(Priority > OS_LOWEST_USED_PRIORITY)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Set the task priority */
  #if (OS_USE_CSEC_OBJECTS)
    Task->AssignedPriority = Priority;
    if(osChangeTaskPriority(Task, Priority))
    {
      /* [!] ISSUE: osPriorityPath logic needs verification regarding
         automatic switching. */
      osPriorityPath(&Task->PriorityPath);
      osRescheduleIfHigherPriority();
    }
  #else
    if(osChangeTaskPriority(Task, Priority))
      osRescheduleIfHigherPriority();
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_TASK_PRIORITY_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_TASK_QUANTUM_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetTaskQuantum
 *
 *  Description:
 *    Returns the number of time quanta assigned to the specified task.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    Quantum - Pointer to variable that receives task time quanta.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetTaskQuantum(HANDLE Handle, UINT8 *Quantum)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get the task time quanta */
  *Quantum = ((struct TTask FAR *) Object->ObjectDesc)->MaxTimeQuantum;

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osSetTaskQuantum
 *
 *  Description:
 *    Sets the number of time quanta assigned to the specified task.
 *    The new value is applied immediately and affects the time quanta
 *    counter if the task is currently executing.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    Quantum - New task time quanta value.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSetTaskQuantum(HANDLE Handle, UINT8 Quantum)
{
  struct TTask FAR *Task;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;
  UINT8 Diff;

  /* Check parameters */
  if(!Quantum)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Update task time quanta */
  Diff = (UINT8) (Task->MaxTimeQuantum - Task->TimeQuantumCounter);
  Task->MaxTimeQuantum = Quantum;
  Task->TimeQuantumCounter =
    (UINT8) ((Quantum > Diff) ? (Quantum - Diff) : 0);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_TASK_QUANTUM_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_GET_TASK_STAT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetTaskStat
 *
 *  Description:
 *    Returns the CPU usage for the specified task. The percentage CPU
 *    usage of the task can be calculated using the formula:
 *    100 * CPUTime / TotalTime.
 *
 *  Parameters:
 *    Handle - Task handle.
 *    CPUTime - Pointer to variable that receives task CPU usage time.
 *    TotalTime - Pointer to variable that receives total CPU usage time.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetTaskStat(HANDLE Handle, INDEX *CPUTime, INDEX *TotalTime)
{
  struct TSysObject FAR *Object;
  struct TTask FAR *Task;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TASK);
  if(!Object)
    return FALSE;

  /* Get task pointer */
  Task = (struct TTask FAR *) Object->ObjectDesc;

  /* Get task CPU usage */
  if(Task->CPUUsageTime == osCPUUsageTime)
    *CPUTime = Task->CPUUsage;
  else if(Task->CPUCalcTime == osCPUUsageTime)
    *CPUTime = Task->CPUCalc;
  else
    *CPUTime = 0;
  *TotalTime = osCPUUsage;

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_GET_TASK_STAT_FUNC */
/***************************************************************************/


/***************************************************************************/
