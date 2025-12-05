/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Core.c - Operating System Core interface
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
 *  Definitions
 *
 ***************************************************************************/

#if ((OS_LOWEST_USED_PRIORITY) < 1)
  #define OS_PRIORITY_COUNT             1
#elif ((OS_LOWEST_USED_PRIORITY) < 2)
  #define OS_PRIORITY_COUNT             2
#elif ((OS_LOWEST_USED_PRIORITY) < 4)
  #define OS_PRIORITY_COUNT             4
#elif ((OS_LOWEST_USED_PRIORITY) < 8)
  #define OS_PRIORITY_COUNT             8
#elif ((OS_LOWEST_USED_PRIORITY) < 16)
  #define OS_PRIORITY_COUNT             16
#elif ((OS_LOWEST_USED_PRIORITY) < 32)
  #define OS_PRIORITY_COUNT             32
#elif ((OS_LOWEST_USED_PRIORITY) < 64)
  #define OS_PRIORITY_COUNT             64
#elif ((OS_LOWEST_USED_PRIORITY) < 128)
  #define OS_PRIORITY_COUNT             128
#else
  #define OS_PRIORITY_COUNT             256
#endif


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

/* Internal system memory pool */
#if ((OS_INTERNAL_MEMORY_SIZE) > 0UL)
  PVOID osMemoryPool[OS_INTERNAL_MEMORY_SIZE];
#endif

/* Fixed size memory pool 0 */
#if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL0_SIZE) > 0UL))
  static UINT8 osFixMemPool0[OS_FIX_POOL0_SIZE];
#endif

/* Fixed size memory pool 1 */
#if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL1_SIZE) > 0UL))
  static UINT8 osFixMemPool1[OS_FIX_POOL1_SIZE];
#endif

/* Fixed size memory pool 2 */
#if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL2_SIZE) > 0UL))
  static UINT8 osFixMemPool2[OS_FIX_POOL2_SIZE];
#endif

/* Fixed size memory pool 3 */
#if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL3_SIZE) > 0UL))
  static UINT8 osFixMemPool3[OS_FIX_POOL3_SIZE];
#endif

/* Last error code (used when operating system is not running or code is
   executed from an ISR) */
static ERROR osLastErrorCode;

/* ISR Flag */
BOOL osInISR;
static BOOL osYieldAfterISR;

/* Operating system caller context */
#if (OS_STOP_FUNC)
  static BOOL osSaveCallerAndStart, osRestoreCallerAndStop;
  static struct TTaskContext osCallerContext;
#endif

/* Time notification */
#if (OS_USE_TIME_OBJECTS)
  static struct TPQueue osTimeNotifyQueue;
  static struct TTimeNotify FAR *osTimeNotify[OS_LOWEST_USED_PRIORITY + 1];
  static TIME osTimeNotifyArr[OS_PRIORITY_COUNT +
    OS_LOWEST_USED_PRIORITY + 2];
#endif

/* System names */
#if (OS_USE_OBJECT_NAMES)
  static struct TBSTree osSysNames;
#endif

/* Deferred signalization queue */
static struct TBSTree osDeferredSignal;

/* Ready to run tasks queue */
static struct TPQueue osTaskPQueue;

/* Idle task pointer */
#if ((OS_DEINIT_FUNC) || (OS_USE_STATISTICS))
  static struct TTask FAR *osIdleTask;
#endif

/* Current task pointer */
struct TTask FAR *osCurrentTask;

/* Last time quantum assign time */
TIME osLastQuantumTime;
INDEX osLastQuantumIndex;

/* Total CPU usage information */
#if (OS_USE_STATISTICS)
  TIME osCPUUsageTime;
  INDEX osCPUUsage;
  static TIME osCPUCalcTime;
  static INDEX osCPUCalc;
#endif

/* System object deinitialization list */
#if (OS_DEINIT_FUNC)
  static struct TSysObject FAR *osFirstObject;
#endif


/****************************************************************************
 *
 *  Function prototypes
 *
 ***************************************************************************/

static BOOL osAcquire(struct TSignal FAR *Signal, BOOL OnCheck);
static struct TCSAssoc FAR *osFindCSAssoc(struct TCriticalSection *CS,
  struct TTask *Task);


/****************************************************************************
 *
 *  System memory management
 *
 ***************************************************************************/


/***************************************************************************/
#if (OS_USE_FIXMEM_POOLS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMemAlloc
 *
 *  Description:
 *    Allocates a new memory block in the defined fixed size memory pool.
 *
 *  Parameters:
 *    Size - Size of the memory to be allocated.
 *
 *  Return:
 *    Address of the newly allocated memory block or NULL on failure.
 *
 ***************************************************************************/

PVOID osMemAlloc(SIZE Size)
{
  BOOL PrevLockState;
  PVOID Ptr;

  /* Enter critical section */
  PrevLockState = arLock();

  while(true)
  {
    /* Allocate data in fixed size memory pool 0 */
    #if ((OS_FIX_POOL0_SIZE) > 0UL)
      if(Size < (OS_FIX_POOL0_ITEM_SIZE))
      {
        Ptr = stFixedMemAlloc(osFixMemPool0);
        if(Ptr)
          break;
      }
    #endif

    /* Allocate data in fixed size memory pool 1 */
    #if ((OS_FIX_POOL1_SIZE) > 0UL)
      if(Size < (OS_FIX_POOL1_ITEM_SIZE))
      {
        Ptr = stFixedMemAlloc(osFixMemPool1);
        if(Ptr)
          break;
      }
    #endif

    /* Allocate data in fixed size memory pool 2 */
    #if ((OS_FIX_POOL2_SIZE) > 0UL)
      if(Size < (OS_FIX_POOL2_ITEM_SIZE))
      {
        Ptr = stFixedMemAlloc(osFixMemPool2);
        if(Ptr)
          break;
      }
    #endif

    /* Allocate data in fixed size memory pool 3 */
    #if ((OS_FIX_POOL3_SIZE) > 0UL)
      if(Size < (OS_FIX_POOL3_ITEM_SIZE))
      {
        Ptr = stFixedMemAlloc(osFixMemPool3);
        if(Ptr)
          break;
      }
    #endif

    /* Expected memory size is higher than specified in configuration */
    osSetLastError(ERR_WRONG_OS_FIXMEM_CONFIG);
    Ptr = NULL;
    break;
  }

  /* Leave critical section */
  arRestore(PrevLockState);
  return Ptr;
}


/***************************************************************************/
#if (OS_ALLOW_OBJECT_DELETION)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMemFree
 *
 *  Description:
 *    Releases an allocated memory block.
 *
 *  Parameters:
 *    Ptr - Address of the memory block to be released.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osMemFree(PVOID Ptr)
{
  BOOL PrevLockState;
  BOOL Success;
  Success = FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Try to release data from fixed size memory pool 0 */
  #if ((OS_FIX_POOL0_SIZE) > 0UL)
    Success = stFixedMemFree(osFixMemPool0, Ptr);
  #endif

  /* Try to release data from fixed size memory pool 1 */
  #if ((OS_FIX_POOL1_SIZE) > 0UL)
    if(!Success)
      Success = stFixedMemFree(osFixMemPool1, Ptr);
  #endif

  /* Try to release data from fixed size memory pool 2 */
  #if ((OS_FIX_POOL2_SIZE) > 0UL)
    if(!Success)
      Success = stFixedMemFree(osFixMemPool2, Ptr);
  #endif

  /* Try to release data from fixed size memory pool 3 */
  #if ((OS_FIX_POOL3_SIZE) > 0UL)
    if(!Success)
      Success = stFixedMemFree(osFixMemPool3, Ptr);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Set error code when memory was not released */
  if(!Success)
    osSetLastError(ERR_INVALID_MEMORY_BLOCK);

  return Success;
}


/***************************************************************************/
#endif /* OS_ALLOW_OBJECT_DELETION */
/***************************************************************************/


/***************************************************************************/
#endif /* OS_USE_FIXMEM_POOLS */
/***************************************************************************/


/****************************************************************************
 *
 *  Error management
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSetLastError
 *
 *  Description:
 *    Sets the last error code. If the operating system is not running
 *    or the function is called from an ISR, the last error code is stored
 *    in the global variable; otherwise, it is stored for the calling task.
 *
 *  Parameters:
 *    ErrorCode - Error code.
 *
 ***************************************************************************/

void osSetLastError(ERROR ErrorCode)
{
  /* Set last error code */
  if(!osCurrentTask || osInISR)
    osLastErrorCode = ErrorCode;
  else
    osCurrentTask->LastErrorCode = ErrorCode;
}


/****************************************************************************
 *
 *  Name:
 *    osGetLastError
 *
 *  Description:
 *    Returns the last error code.
 *
 *  Return:
 *    Error code.
 *
 ***************************************************************************/

ERROR osGetLastError(void)
{
  /* Get last error code */
  return (!osCurrentTask || osInISR) ? osLastErrorCode :
    osCurrentTask->LastErrorCode;
}


/****************************************************************************
 *
 *  ISR section management
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osEnterISR
 *
 *  Description:
 *    Begins a section of code executed in an ISR. From this moment,
 *    scheduler execution will be delayed until the corresponding osLeaveISR
 *    is called. Execution of osEnterISR and osLeaveISR functions can be
 *    nested.
 *
 *  Return:
 *    Value expected for the further osLeaveISR function call.
 *
 ***************************************************************************/

INLINE BOOL osEnterISR(void)
{
  register BOOL OldISR;
  register BOOL PrevLockState;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Begin section of code executed in the ISR */
  OldISR = osInISR;
  if(!OldISR)
  {
    osYieldAfterISR = FALSE;
    osInISR = TRUE;
  }

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return previous osInISR value */
  return OldISR;
}


/****************************************************************************
 *
 *  Name:
 *    osLeaveISR
 *
 *  Description:
 *    Ends a section of code executed in an ISR. At this moment, the
 *    delayed scheduler procedure will be executed.
 *
 *  Parameters:
 *    PrevISRState - Value returned by the corresponding call of the
 *    osEnterISR function.
 *
 ***************************************************************************/

INLINE void osLeaveISR(BOOL PrevISRState)
{
  register BOOL PrevLockState;

  if(!PrevISRState)
  {
    /* Enter critical section */
    PrevLockState = arLock();

    /* End section of code executed in the ISR and execute delayed scheduler
       procedure */
    osInISR = FALSE;
    if(osYieldAfterISR)
      arYield();

    /* Leave critical section */
    arRestore(PrevLockState);
  }
}


/****************************************************************************
 *
 *  Name:
 *    osYield
 *
 *  Description:
 *    Voluntarily yields execution of the current task. If an ISR is
 *    processing, scheduler execution will be delayed.
 *
 ***************************************************************************/

static void osYield(void)
{
  /* Execute arYield function only when not called from an ISR */
  if(osInISR)
    osYieldAfterISR = TRUE;
  else
    arYield();
}


/****************************************************************************
 *
 *  Time notification management
 *
 ***************************************************************************/


/***************************************************************************/
#if (OS_USE_TIME_OBJECTS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osTimeNotifyCmp
 *
 *  Description:
 *    Compares two time notification descriptors.
 *
 *  Parameters:
 *    Item1 - Pointer to first time notification descriptor.
 *    Item2 - Pointer to second time notification descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

static int osTimeNotifyCmp(PVOID Item1, PVOID Item2)
{
  register struct TTimeNotify FAR *T1;
  register struct TTimeNotify FAR *T2;
  register int Cmp;

  /* Compare items */
  T1 = Item1;
  T2 = Item2;

  Cmp = ((int) T1->Priority) - ((int) T2->Priority);
  if(Cmp != 0)
    return Cmp;

  if(T1->Time < T2->Time)
    return -1;
  if(T1->Time > T2->Time)
    return 1;

  /* Items are equal */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osUpdateTimeNotify
 *
 *  Description:
 *    Internal function that updates the array used for searching time
 *    notifications.
 *
 *  Parameters:
 *    TimeNotify - Time notification descriptor pointer.
 *    Priority - Priority of the time notification.
 *
 ***************************************************************************/

static void osUpdateTimeNotify(struct TTimeNotify FAR *TimeNotify,
  int Priority)
{
  TIME Time, TimeOp;

  /* Update array */
  osTimeNotify[Priority] = TimeNotify;

  /* Update time tree */
  Time = TimeNotify ? TimeNotify->Time : OS_INFINITE;
  Priority += OS_PRIORITY_COUNT;
  osTimeNotifyArr[Priority] = Time;
  do
  {
    TimeOp = osTimeNotifyArr[Priority ^ 1];
    if(Time > TimeOp)
      Time = TimeOp;

    Priority >>= 1;
    if(osTimeNotifyArr[Priority] == Time)
      break;

    osTimeNotifyArr[Priority] = Time;
  }
  while(Priority > 1);
}


/****************************************************************************
 *
 *  Name:
 *    osInitTimeWaiting
 *
 *  Description:
 *    Initializes the time notification descriptor.
 *
 *  Parameters:
 *    TimeNotify - Time notification descriptor pointer.
 *
 ***************************************************************************/

void osInitTimeNotify(struct TTimeNotify FAR *TimeNotify)
{
  /* Initialize time notification descriptor */
  TimeNotify->Registered = FALSE;
  TimeNotify->Task = NULL;
  TimeNotify->Signal = NULL;
}


/****************************************************************************
 *
 *  Name:
 *    osRegisterTimeNotify
 *
 *  Description:
 *    Registers a time notification descriptor. Function must be called
 *    from inside a critical section.
 *
 *  Parameters:
 *    TimeNotify - Time notification descriptor pointer.
 *    Time - Notification time.
 *
 ***************************************************************************/

void osRegisterTimeNotify(struct TTimeNotify FAR *TimeNotify,
  TIME Time)
{
  /* Unregister object if it is already registered */
  if(TimeNotify->Registered)
     osUnregisterTimeNotify(TimeNotify);

  /* Define item */
  TimeNotify->Registered = TRUE;
  TimeNotify->Time = Time;

  /* Obtain the time notification descriptor priority */
  if(TimeNotify->Task)
    TimeNotify->Priority = TimeNotify->Task->Priority;
  else
  {
    struct TWaitAssoc FAR *WaitAssoc;

    WaitAssoc = (struct TWaitAssoc FAR *)
      stBSTreeGetFirst(&TimeNotify->Signal->WaitingTasks);
    TimeNotify->Priority = (UINT8)
      (WaitAssoc ? WaitAssoc->Task->Priority : OS_LOWEST_PRIORITY);
  }

  /* Add time notification descriptor into the queue */
  stPQueueInsert(&osTimeNotifyQueue, &TimeNotify->Item, TimeNotify);

  /* Update time notification array */
  if(Time < osTimeNotifyArr[OS_PRIORITY_COUNT + TimeNotify->Priority])
    osUpdateTimeNotify(TimeNotify, TimeNotify->Priority);
}


/****************************************************************************
 *
 *  Name:
 *    osUnregisterTimeNotify
 *
 *  Description:
 *    Unregisters a time notification descriptor. Function must be called
 *    from inside a critical section.
 *
 *  Parameters:
 *    TimeNotify - Time notification descriptor pointer.
 *
 ***************************************************************************/

void osUnregisterTimeNotify(struct TTimeNotify FAR *TimeNotify)
{
  UINT8 Priority;
  struct TBSTreeNode FAR *BSTreeNode;
  struct TTimeNotify FAR *Tmp;

  /* Return if object is not registered */
  if(!TimeNotify->Registered)
    return;

  /* Get time notification descriptor priority */
  Priority = TimeNotify->Priority;

  /* Remove item from the queue */
  stPQueueRemove(&osTimeNotifyQueue, &TimeNotify->Item);
  TimeNotify->Registered = FALSE;

  /* Find the minimal value with the same priority */
  TimeNotify = NULL;
  BSTreeNode = (struct TBSTreeNode FAR *) osTimeNotifyQueue.Tree.Root;
  while(BSTreeNode)
  {
    Tmp = (struct TTimeNotify FAR *) BSTreeNode->Data;

    if(Priority == Tmp->Priority)
      TimeNotify = Tmp;

    if(Priority <= Tmp->Priority)
      BSTreeNode = Tmp->Item.Node.Left;

    else
      BSTreeNode = Tmp->Item.Node.Right;
  }

  /* Update time notification array */
  osUpdateTimeNotify(TimeNotify, Priority);
}


/****************************************************************************
 *
 *  Name:
 *    osGetTimeNotify
 *
 *  Description:
 *    Returns a pointer to a registered time notification descriptor.
 *    The priority and time must be less than or equal to those specified
 *    in the function parameters. If this condition is not met, the function
 *    returns NULL.
 *    Function must be called from inside a critical section.
 *
 *  Parameters:
 *    Priority - Minimal priority value.
 *    Time - Maximal time value.
 *
 *  Return:
 *    Time notification descriptor.
 *
 ***************************************************************************/

static struct TTimeNotify FAR *osGetTimeNotify(UINT8 Priority, TIME Time)
{
  int i, Offset, BaseOffset;

  /* Find time notification descriptor that meets specified conditions */
  Offset = 0;
  BaseOffset = 1;
  for(i = 1; i < OS_PRIORITY_COUNT; i <<= 1)
  {
    /* Check time of the node */
    if(osTimeNotifyArr[BaseOffset + Offset] > Time)
      return NULL;

    /* Child position */
    BaseOffset += i;
    Offset <<= 1;

    /* Choose left or right child */
    if(osTimeNotifyArr[BaseOffset + Offset] > Time)
      Offset++;

    /* Check priority */
    if(Offset > (Priority / ((OS_PRIORITY_COUNT >> 1) / i)))
      return NULL;
  }

  /* Return pointer to time notification descriptor */
  return osTimeNotify[Offset];
}


/***************************************************************************/
#endif  /* OS_USE_TIME_OBJECTS */
/***************************************************************************/


/****************************************************************************
 *
 *  System object opening, naming and handling
 *
 ***************************************************************************/


/***************************************************************************/
#if (OS_ALLOW_OBJECT_DELETION)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osObjectByHandleCmp
 *
 *  Description:
 *    Compares two object descriptors by their handle.
 *
 *  Parameters:
 *    Item1 - Pointer to first object descriptor.
 *    Item2 - Pointer to second object descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

int osObjectByHandleCmp(PVOID Item1, PVOID Item2)
{
  /* Compare handle of the objects */
  if(((struct TSysObject FAR *) Item1)->Handle <
    ((struct TSysObject FAR *) Item2)->Handle)
    return -1;

  if(((struct TSysObject FAR *) Item1)->Handle >
    ((struct TSysObject FAR *) Item2)->Handle)
    return 1;

  /* Items are equal */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osOpenObject
 *
 *  Description:
 *    Opens the specified system object. The opened object becomes a child
 *    of the calling task and cannot be deleted until all tasks that opened
 *    the object close it using the osCloseHandle function. If the object
 *    is already opened by the calling task, the function returns
 *    immediately with success.
 *
 *  Parameters:
 *    Object - Pointer to system object descriptor.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

static BOOL osOpenObject(struct TSysObject FAR *Object)
{
  struct TBSTreeNode FAR *Node;
  BOOL PrevLockState, Success;

  /* Allocate memory for child descriptor */
  Node = (struct TBSTreeNode FAR *) osMemAlloc(sizeof(*Node));
  if(!Node)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Insert to task child tree */
  Success = stBSTreeInsert(&osCurrentTask->Childs, Node, NULL, Object);
  if(Success)
    Object->OwnerCount++;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* If the object is already opened, the node descriptor is not used */
  if(!Success)
    osMemFree(Node);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_ALLOW_OBJECT_DELETION */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osTaskCmp
 *
 *  Description:
 *    Compares two tasks by their priorities. When priorities are equal,
 *    it also compares the time quantum assign time.
 *
 *  Parameters:
 *    Item1 - Pointer to first task descriptor.
 *    Item2 - Pointer to second task descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

static int osTaskCmp(PVOID Item1, PVOID Item2)
{
  register struct TTask FAR *Task1;
  register struct TTask FAR *Task2;
  register int Cmp;

  /* Get task pointers */
  Task1 = (struct TTask FAR *) Item1;
  Task2 = (struct TTask FAR *) Item2;

  /* Compare task priorities */
  Cmp = ((int) Task1->Priority) - ((int) Task2->Priority);
  if(Cmp != 0)
    return Cmp;

  /* Compare time quantum assign time when priorities are equal */
  if(Task1->LastQuantumTime < Task2->LastQuantumTime)
    return -1;

  if(Task1->LastQuantumTime > Task2->LastQuantumTime)
    return 1;

  if(Task1->LastQuantumIndex < Task2->LastQuantumIndex)
    return -1;

  if(Task1->LastQuantumIndex > Task2->LastQuantumIndex)
    return 1;

  /* Tasks are equal */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osWaitAssocCmp
 *
 *  Description:
 *    Compares two wait associations by task priorities. When priorities
 *    are equal, it also compares the time quantum assign time.
 *
 *  Parameters:
 *    Item1 - Pointer to first association descriptor.
 *    Item2 - Pointer to second association descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

int osWaitAssocCmp(PVOID Item1, PVOID Item2)
{
  /* Compare association by tasks */
  return osTaskCmp(
    ((struct TWaitAssoc FAR *) Item1)->Task,
    ((struct TWaitAssoc FAR *) Item2)->Task);
}


/****************************************************************************
 *
 *  Name:
 *    osRegisterObject
 *
 *  Description:
 *    Registers a new system object. The registered object is not ready to
 *    use, and the OS_OBJECT_FLAG_READY_TO_USE flag should be set after
 *    full object initialization and configuration. The registered object
 *    is in the signaled state by default.
 *
 *  Parameters:
 *    ObjectDesc - Pointer to specific object descriptor.
 *    Object - Pointer to system object descriptor.
 *    Type - Object type.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osRegisterObject(PVOID ObjectDesc, struct TSysObject FAR *Object,
  UINT8 Type)
{
  /* System object type */
  Object->Type = Type;

  /* Initial flags (object is not ready to use) */
  Object->Flags = 0;

  /* Setup the default signal */
  Object->Signal.Flags = 0;
  Object->Signal.Signaled = (INDEX) TRUE;
  stBSTreeInit(&Object->Signal.WaitingTasks, osWaitAssocCmp);

  /* Object signalization descriptor */
  #if (OS_USE_SYSTEM_IO_CTRL)
    Object->Signal.Object = Object;
  #endif

  /* Critical section descriptor */
  #if (OS_USE_CSEC_OBJECTS)
    Object->Signal.CS = NULL;
  #endif

  /* Multiple signals associated with object */
  #if ((OS_USE_MULTIPLE_SIGNALS) && (OS_ALLOW_OBJECT_DELETION))
    Object->Signal.NextSignal = NULL;
  #endif

  /* Object name descriptor pointer */
  #if (OS_USE_OBJECT_NAMES)
    Object->Name = NULL;
  #endif

  /* Device IO control function for specified object */
  #if (OS_USE_DEVICE_IO_CTRL)
    Object->DeviceIOCtrl = NULL;
  #endif

  /* Add object to system object deinitialization list */
  #if (OS_DEINIT_FUNC)
    Object->PrevObject = NULL;
    Object->NextObject = osFirstObject;
    osFirstObject->PrevObject = Object;
    osFirstObject = Object;
  #endif

  /* Pointer to specific object descriptor */
  Object->ObjectDesc = ObjectDesc;

  /* Try to assign new handle for the object */
  if(!stHandleAlloc(&Object->Handle, Object, 0, Type))
    return FALSE;

  /* Open created object */
  #if (OS_ALLOW_OBJECT_DELETION)
    if(!osCurrentTask || osInISR)
      Object->OwnerCount = 1;
    else
    {
      Object->OwnerCount = 0;
      if(!osOpenObject(Object))
      {
        stHandleRelease(Object->Handle);
        return FALSE;
      }
    }
  #endif

  /* Return with success */
  return TRUE;
}

/***************************************************************************/
#if (OS_USE_OBJECT_NAMES)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osObjectByNameCmp
 *
 *  Description:
 *    Compares two name descriptors by object name.
 *
 *  Parameters:
 *    Item1 - Pointer to first name descriptor.
 *    Item2 - Pointer to second name descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

static int osObjectByNameCmp(PVOID Item1, PVOID Item2)
{
  /* Compare string names */
  #if ((OS_SYS_OBJECT_MAX_NAME_LEN) > 0UL)

    return stStrNICmp(((struct TObjectName FAR *) Item1)->Name,
      ((struct TObjectName FAR *) Item2)->Name, OS_SYS_OBJECT_MAX_NAME_LEN);

  /* Compare integer names */
  #else

    if(((struct TObjectName FAR *) Item1)->Name <
      ((struct TObjectName FAR *) Item2)->Name)
      return -1;

    if(((struct TObjectName FAR *) Item1)->Name >
      ((struct TObjectName FAR *) Item2)->Name)
      return 1;

    return 0;

  #endif
}


/****************************************************************************
 *
 *  Name:
 *    osRegisterName
 *
 *  Description:
 *    Registers a new name descriptor for the specified object.
 *
 *  Parameters:
 *    Object - Pointer to system object descriptor.
 *    ObjectName - Pointer to name descriptor.
 *    Name - Object name to register.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osRegisterName(struct TSysObject FAR *Object,
  struct TObjectName FAR *ObjectName, SYSNAME Name)
{
  BOOL PrevLockState, Success;

  /* Assign empty string, if NULL is specified */
  if(Name == NULL)
    Name = "";

  /* Return with success, when name is not specified */
  if(Name[0] == '\0')
    return TRUE;

  /* Assign name */
  #if ((OS_SYS_OBJECT_MAX_NAME_LEN) > 0UL)
    stStrNCpy(ObjectName->Name, Name, OS_SYS_OBJECT_MAX_NAME_LEN);
  #else
    ObjectName->Name = Name;
  #endif

  /* Assign object descriptor pointer */
  ObjectName->Object = Object;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Register name descriptor in the system */
  Success = stBSTreeInsert(&osSysNames, &ObjectName->Node, NULL, ObjectName);
  if(Success)
    Object->Name = ObjectName;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return status */
  if(!Success)
    osSetLastError(ERR_OBJECT_ALREADY_EXISTS);

  return Success;
}


/****************************************************************************
 *
 *  Name:
 *    osOpenNamedObject
 *
 *  Description:
 *    Opens the specified system object by its name. When the specified
 *    type is different from OS_OBJECT_TYPE_IGNORE and the object type,
 *    the function will fail.
 *
 *  Parameters:
 *    Name - Object name.
 *    Type - Type of the object (or OS_OBJECT_TYPE_IGNORE to ignore).
 *
 *  Return:
 *    Pointer to opened system object or NULL on failure.
 *
 ***************************************************************************/

struct TSysObject FAR *osOpenNamedObject(SYSNAME Name, UINT8 Type)
{
  struct TSysObject FAR *Object;
  struct TObjectName FAR *ObjectName;
  struct TObjectName TmpName;
  BOOL PrevLockState;

  /* Object can be opened only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return NULL;
  }

  /* Prepare temporary name descriptor */
  #if ((OS_SYS_OBJECT_MAX_NAME_LEN) > 0UL)
    stStrNCpy(TmpName.Name, Name, OS_SYS_OBJECT_MAX_NAME_LEN);
  #else
    TmpName.Name = Name;
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* Find object by name */
  ObjectName =
    (struct TObjectName FAR *) stBSTreeSearch(&osSysNames, &TmpName);
  Object = ObjectName ? ObjectName->Object : NULL;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* [!] NOTE: Deleting the object at this moment will cause an error.
     Ensure the object remains valid until opened. */

  /* Object not found */
  while(Object)
  {
    /* Check object type */
    if(Object->Type != Type)
      break;

    /* Return with error if object is not ready to use yet */
    if(!(Object->Flags & OS_OBJECT_FLAG_READY_TO_USE))
      break;

    /* Open object */
    #if (OS_ALLOW_OBJECT_DELETION)
      return osOpenObject(Object) ? Object : NULL;
    #else
      return Object;
    #endif
  }

  /* Object opening failure */
  osSetLastError(ERR_OBJECT_CAN_NOT_BE_OPENED);
  return NULL;
}


/***************************************************************************/
#endif /* OS_USE_OBJECT_NAMES */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetObjectByHandle
 *
 *  Description:
 *    Returns a pointer to the system object descriptor specified by the
 *    handle. The function fails if the specified type is different from
 *    OS_OBJECT_TYPE_IGNORE and the object type.
 *
 *  Parameters:
 *    Handle - Object handle.
 *    Type - Object type or OS_OBJECT_TYPE_IGNORE.
 *
 *  Return:
 *    Pointer to system object descriptor or NULL on failure.
 *
 ***************************************************************************/

struct TSysObject FAR *osGetObjectByHandle(HANDLE Handle, UINT8 Type)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  if(!stGetHandleInfo(Handle, (PVOID *) &Object, Type))
    return NULL;

  while(TRUE)
  {
    /* Is the object ready to use? */
    if(!(Object->Flags & OS_OBJECT_FLAG_READY_TO_USE))
      break;

    /* Check object type */
    if(Type != OS_OBJECT_TYPE_IGNORE)
      if(Object->Type != Type)
        break;

    /* Return object pointer */
    return Object;
  }

  /* Failure, object cannot be used */
  osSetLastError(ERR_INVALID_HANDLE);
  return NULL;
}


/***************************************************************************/
#if (OS_OPEN_BY_HANDLE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenByHandle
 *
 *  Description:
 *    Opens the specified system object by its handle.
 *
 *  Parameters:
 *    Handle - Object handle.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osOpenByHandle(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  if(!stGetHandleInfo(Handle, (PVOID *) &Object, ST_HANDLE_TYPE_IGNORE))
    return FALSE;

  /* [!] NOTE: Deleting the object at this moment will cause an error. */

  /* Try to open the object */
  return osOpenObject(Object);
}


/***************************************************************************/
#endif /* OS_OPEN_BY_HANDLE_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_ALLOW_OBJECT_DELETION)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osDeleteObject
 *
 *  Description:
 *    Deletes the specified system object. Before deletion, the object is
 *    marked as not ready to use. Corresponding deinit IO control code is
 *    performed (when defined). If the deleting object is a task, it also
 *    releases the task context. Finally, the memory of the specific
 *    object descriptor is released.
 *
 *  Parameters:
 *    Object - Pointer to system object descriptor.
 *
 ***************************************************************************/

void osDeleteObject(struct TSysObject FAR *Object)
{
  /* Mark as not ready to use */
  Object->Flags &= (UINT8) ~OS_OBJECT_FLAG_READY_TO_USE;

  /* Perform device IO control code for deinitialization */
  #if (OS_USE_DEVICE_IO_CTRL)
    if(Object->Flags & OS_OBJECT_FLAG_USES_IO_DEINIT)
      Object->DeviceIOCtrl(Object, DEV_IO_CTL_DEINIT, NULL, 0, NULL);
  #endif

  /* Remove object from system object deinitialization list */
  #if (OS_DEINIT_FUNC)
    if(Object->PrevObject)
      Object->PrevObject->NextObject = Object->NextObject;
    else
      osFirstObject = Object->NextObject;
    if(Object->NextObject)
      Object->NextObject->PrevObject = Object->PrevObject;
  #endif

  /* Unregister object name from the system */
  #if (OS_USE_OBJECT_NAMES)
    if(Object->Name)
      stBSTreeRemove(&osSysNames, &Object->Name->Node);
  #endif

  /* Release task context if object is a task */
  if(Object->Type == OS_OBJECT_TYPE_TASK)
    arReleaseTaskContext(&((struct TTask FAR *)
      Object->ObjectDesc)->TaskContext);

  /* Release object handle */
  stHandleRelease(Object->Handle);

  /* Release memory allocated for the object descriptor */
  osMemFree(Object);
}


/****************************************************************************
 *
 *  Name:
 *    osCloseObject
 *
 *  Description:
 *    Closes the system object for the specified task. When the closed
 *    object has no more owners, it will be deleted. If the object is a
 *    task, it will be deleted only when it is terminated.
 *
 *  Parameters:
 *    Handle - Object handle.
 *    Task - Object parent.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

static BOOL osCloseObject(struct TSysObject FAR *Object,
  struct TTask FAR *Task)
{
  struct TBSTreeNode FAR *Node;

  #if (OS_USE_CSEC_OBJECTS)
    struct TCSAssoc FAR *CSAssoc;
    struct TCriticalSection FAR *CS;

    #if (OS_USE_MULTIPLE_SIGNALS)
      struct TSignal FAR *Signal;
    #endif
  #endif

  /* Find corresponding child */
  Node = Task->Childs.Root;
  while(Node)
  {
    int Cmp;

    /* Compare objects by their handles */
    Cmp = osObjectByHandleCmp(Object, (struct TSysObject FAR *) Node->Data);
    if(!Cmp)
      break;

    Node = (Cmp < 0) ? Node->Left : Node->Right;
  }

  /* Return if specified object is not a child of the task */
  if(!Node)
  {
    osSetLastError(ERR_INVALID_HANDLE);
    return FALSE;
  }

  /* Remove a child */
  stBSTreeRemove(&Task->Childs, Node);
  osMemFree(Node);

  /* Release each abandoned critical section associated with specified
     object */
  #if (OS_USE_CSEC_OBJECTS)
    #if (OS_USE_MULTIPLE_SIGNALS)
      Signal = &Object->Signal;
      while(TRUE)
      {
        CS = Signal->CS;
    #else
        CS = Object->Signal.CS;
    #endif

        /* Release abandoned critical section (if owned) */
        if(CS)
        {
          CSAssoc = osFindCSAssoc(CS, Task);
          if(CSAssoc)
          {
            CS->Signal->Flags |= OS_SIGNAL_FLAG_ABANDONED;
            osReleaseCS(CS, Task, CSAssoc->Count, NULL);
          }
        }

    #if (OS_USE_MULTIPLE_SIGNALS)

        /* Check next signal */
        Signal = Signal->NextSignal;
        if(!Signal)
          break;
      }
    #endif
  #endif

  /* Decrease object owner counter. When it reaches zero, the object
     is not owned (opened) by any task and should be deleted */
  Object->OwnerCount--;
  if(!Object->OwnerCount)
  {
    /* When object is a task, delete it only when it is terminated */
    if(Object->Type == OS_OBJECT_TYPE_TASK)
    {
      if(((struct TTask FAR *) Object->ObjectDesc)->BlockingFlags &
        OS_BLOCK_FLAG_TERMINATED)
        osDeleteObject(Object);
    }

    /* Delete object */
    else
      osDeleteObject(Object);
  }

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osCloseHandle
 *
 *  Description:
 *    Closes the system object for the current task. When the closed object
 *    has no more owners, it will be deleted. If the object is a task, it
 *    will be deleted only when it is terminated.
 *
 *  Parameters:
 *    Handle - Object handle.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osCloseHandle(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Can be performed only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_IGNORE);
  if(!Object)
    return FALSE;

  /* Close specified object */
  return osCloseObject(Object, osCurrentTask);
}


/***************************************************************************/
#endif /* OS_ALLOW_OBJECT_DELETION */
/***************************************************************************/


/***************************************************************************/
#if ((OS_ALLOW_OBJECT_DELETION) || (OS_USE_CSEC_OBJECTS))
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseTaskResources
 *
 *  Description:
 *    Releases each owned critical section and marks it as abandoned. It also
 *    closes all opened objects. When a closed object has no more owners,
 *    it will be deleted. When the closing object is a task, it will be
 *    deleted only when it is terminated.
 *
 *  Parameters:
 *    Task - A pointer to task descriptor.
 *
 ***************************************************************************/

void osReleaseTaskResources(struct TTask FAR *Task)
{
  #if (OS_ALLOW_OBJECT_DELETION)
    struct TSysObject FAR *Object;
  #endif

  #if (OS_USE_CSEC_OBJECTS)
    struct TCSAssoc FAR *CSAssoc;
  #endif

  /* Release owned critical sections */
  #if (OS_USE_CSEC_OBJECTS)
    while(TRUE)
    {
      /* Get first critical section with the highest priority */
      CSAssoc = (struct TCSAssoc FAR *) stPQueueGet(&Task->OwnedCS);
      if(!CSAssoc)
        break;

      /* Release abandoned critical section */
      CSAssoc->CS->Signal->Flags |= OS_SIGNAL_FLAG_ABANDONED;
      osReleaseCS(CSAssoc->CS, Task, CSAssoc->Count, NULL);
    }
  #endif

  /* Close each child (opened objects) */
  #if (OS_ALLOW_OBJECT_DELETION)
    while(TRUE)
    {
      Object = (struct TSysObject FAR *) stBSTreeGetFirst(&Task->Childs);
      if(!Object)
        break;

      /* Close object */
      osCloseObject(Object, osCurrentTask);
    }
  #endif
}


/***************************************************************************/
#endif /* OS_ALLOW_OBJECT_DELETION || OS_USE_CSEC_OBJECTS */
/***************************************************************************/


/****************************************************************************
 *
 *  Deferred signalization queue management
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSignalCmp
 *
 *  Description:
 *    Compares two signals by their priorities. The signal priority is
 *    the highest priority of all the tasks waiting for the signal.
 *
 *  Parameters:
 *    Item1 - Pointer to first signal descriptor.
 *    Item2 - Pointer to second signal descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

static int osSignalCmp(PVOID Item1, PVOID Item2)
{
  /* Compare specified signals by priority of waiting tasks */
  return osTaskCmp(
    ((struct TWaitAssoc FAR *) stBSTreeGetFirst(&((struct TSignal FAR *)
      Item1)->WaitingTasks))->Task,
    ((struct TWaitAssoc FAR *) stBSTreeGetFirst(&((struct TSignal FAR *)
      Item2)->WaitingTasks))->Task);
}

/****************************************************************************
 *
 *  Name:
 *    osSignalUpdated
 *
 *  Description:
 *    Puts the specified signal in the valid position of the deferred
 *    signalization list or removes it if necessary.
 *
 *  Parameters:
 *    Signal - Pointer to signal descriptor.
 *
 *  Return:
 *    Pointer to the first task waiting for the signal or NULL when no
 *    task is waiting for the signal.
 *
 ***************************************************************************/

static struct TTask FAR *osSignalUpdated(struct TSignal FAR *Signal)
{
  struct TWaitAssoc FAR *WaitAssoc;

  /* Remove signal from deferred signalization list */
  if(Signal->Flags & OS_SIGNAL_FLAG_DEFERRED)
  {
    stBSTreeRemove(&osDeferredSignal, &Signal->DeferredSgn);
    Signal->Flags &= (UINT8) ~OS_SIGNAL_FLAG_DEFERRED;
  }

  /* Insert signal into deferred signalization list if necessary */
  WaitAssoc = (struct TWaitAssoc FAR *)
    stBSTreeGetFirst(&Signal->WaitingTasks);
  if(Signal->Signaled && WaitAssoc)
  {
    stBSTreeInsert(&osDeferredSignal, &Signal->DeferredSgn, NULL, Signal);
    Signal->Flags |= OS_SIGNAL_FLAG_DEFERRED;
  }

  /* [!] ISSUE: Check when this is called. There might be a bug if this is
     called when the priority is the same, as it might change the position
     in the queue incorrectly. */

  /* Pointer to first task waiting for signal */
  return WaitAssoc ? WaitAssoc->Task : NULL;
}


/****************************************************************************
 *
 *  Scheduling
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osRoundRobinTaskCmp
 *
 *  Description:
 *    Compares two tasks only by their priorities.
 *
 *  Parameters:
 *    Item1 - Pointer to first task descriptor.
 *    Item2 - Pointer to second task descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

static int osRoundRobinTaskCmp(PVOID Item1, PVOID Item2)
{
  /* Compare tasks by their priorities only */
  return ((int) ((struct TTask FAR *) Item1)->Priority) -
    ((int) ((struct TTask FAR *) Item2)->Priority);
}


/****************************************************************************
 *
 *  Name:
 *    osSchedCmp
 *
 *  Description:
 *    Compares the current task with another specified task. If the
 *    priority is higher, the current task pointer is overwritten with
 *    the specified task and the reason code is set.
 *
 *  Parameters:
 *    Reason - Pointer to current reason index.
 *    NewReason - New reason index.
 *    Task - New task pointer (may be NULL).
 *
 ***************************************************************************/

static void osSchedCmp(INDEX *Reason, INDEX NewReason,
  struct TTask FAR *Task)
{
  /* Overwrite current task pointer, when new task has higher priority
     or time quantum assign time */
  if(Task)
    if(osTaskCmp(osCurrentTask, Task) >= 0)
    {
      *Reason = NewReason;
      osCurrentTask = Task;
    }
}


/****************************************************************************
 *
 *  Name:
 *    osMakeReady
 *
 *  Description:
 *    Makes a task ready to run if it is not blocked.
 *
 *  Parameters:
 *    Task - Pointer to task descriptor.
 *
 ***************************************************************************/

void osMakeReady(struct TTask FAR *Task)
{
  /* Do not make ready if some blocking flag is set or task is already
     ready to run */
  if((Task->Object.Flags & OS_OBJECT_FLAG_READY_TO_RUN) ||
    Task->BlockingFlags)
    return;

  /* Make task ready */
  Task->Object.Flags |= OS_OBJECT_FLAG_READY_TO_RUN;
  stPQueueInsert(&osTaskPQueue, &Task->ReadyTask, Task);

  /* Reset time quanta counter */
  #if (OS_USE_TIME_QUANTA)
    Task->TimeQuantumCounter = Task->MaxTimeQuantum;
  #endif

  /* Reschedule if new task has higher priority */
  if(osCurrentTask)
    if(osCurrentTask->Priority > Task->Priority)
      osYield();
}


/****************************************************************************
 *
 *  Name:
 *    osMakeNotReady
 *
 *  Description:
 *    Removes the specified task from the ready to run queue.
 *
 *  Parameters:
 *    Task - Pointer to task descriptor.
 *
 ***************************************************************************/

void osMakeNotReady(struct TTask FAR *Task)
{
  /* Skip if task is not ready to run */
  if(!(Task->Object.Flags & OS_OBJECT_FLAG_READY_TO_RUN))
    return;

  /* Remove task from the ready to run tasks queue */
  stPQueueRemove(&osTaskPQueue, &Task->ReadyTask);
  Task->Object.Flags &= (UINT8) ~OS_OBJECT_FLAG_READY_TO_RUN;

  /* Call scheduler when current task is specified */
  if(Task == osCurrentTask)
    osYield();
}


/****************************************************************************
 *
 *  Name:
 *    osScheduler
 *
 *  Description:
 *    The scheduler determines which task gets the next slice of CPU time.
 *    Priority-based scheduling allows running ready tasks with the
 *    highest priority only. When several tasks can run, the scheduler
 *    uses a Round-Robin algorithm, activating the task that hasn't run
 *    for the longest time first.
 *
 *  Parameters:
 *    TaskContext - Pointer to current task context descriptor.
 *
 ***************************************************************************/

static void CALLBACK osScheduler(struct TTaskContext FAR *TaskContext)
{
  struct TWaitAssoc FAR *WaitAssoc;
  struct TSignal FAR *Signal;
  TIME CurrentTime;
  INDEX Reason;

  #if (OS_USE_TIME_OBJECTS)
    struct TTimeNotify FAR *TimeNotify;
  #endif

  /* Return immediately if ISR is processing */
  if(osInISR)
  {
    osYieldAfterISR = TRUE;
    return;
  }

  /* If the operating system is not running */
  if(!osCurrentTask)
  {
    #if (OS_STOP_FUNC)

      /* If osStart was called, save caller context */
      if(osSaveCallerAndStart)
        osCallerContext = *TaskContext;

      /* Else exit immediately */
      else
        return;

    #else

      /* Exit immediately */
      return;

    #endif
  }

  /* Save context of the preempted task */
  if(osCurrentTask)
    osCurrentTask->TaskContext = *TaskContext;

  /* If osStop was called, restore caller context */
  #if (OS_STOP_FUNC)
    if(osRestoreCallerAndStop)
    {
      *TaskContext = osCallerContext;
      osCurrentTask = NULL;
      return;
    }
  #endif

  /* Get current time */
  CurrentTime = arGetTickCount();

  /* Set a new current task pointer based on Round-Robin scheduling
     and time quanta */
  Reason = OS_SCHED_READY_TO_RUN;
  #if (OS_USE_TIME_QUANTA)
    osCurrentTask = (struct TTask FAR *) stPQueueGet(&osTaskPQueue);
    if(!osCurrentTask->TimeQuantumCounter)
    {
      osCurrentTask->TimeQuantumCounter = osCurrentTask->MaxTimeQuantum;
      stPQueueRotate(&osTaskPQueue, NULL, TRUE);
      osCurrentTask = (struct TTask FAR *) stPQueueGet(&osTaskPQueue);
    }
  #else
    osCurrentTask = (struct TTask FAR *) stPQueueGet(&osTaskPQueue);
  #endif

  /* Time notification */
  #if (OS_USE_TIME_OBJECTS)
    TimeNotify = osGetTimeNotify(osCurrentTask->Priority, CurrentTime);
    if(TimeNotify)
    {
      if(TimeNotify->Task)
        osSchedCmp(&Reason, OS_SCHED_TIME_NOTIFICATION, TimeNotify->Task);
      else
      {
        TimeNotify->Signal->Signaled = (INDEX) TRUE;
        osSignalUpdated(TimeNotify->Signal);
        osUnregisterTimeNotify(TimeNotify);
      }
    }
  #endif

  /* Deferred signalization */
  Signal = (struct TSignal FAR *) stBSTreeGetFirst(&osDeferredSignal);
  if(Signal)
  {
    WaitAssoc = (struct TWaitAssoc FAR *)
      stBSTreeGetFirst(&Signal->WaitingTasks);
    osSchedCmp(&Reason, OS_SCHED_DEFERRED_SIGNALIZATION, WaitAssoc->Task);
  }

  /* Perform operation depending on task release reason */
  switch(Reason)
  {
    #if (OS_USE_TIME_OBJECTS)
      case OS_SCHED_TIME_NOTIFICATION:
        osUnregisterTimeNotify(TimeNotify);
        osCurrentTask->WaitExitCode = ERR_WAIT_TIMEOUT;
        osCurrentTask->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_SLEEP;
        break;
    #endif

    case OS_SCHED_DEFERRED_SIGNALIZATION:
      osAcquire(Signal, FALSE);
      break;
  }

  /* Insert task at the beginning of the ready to run task queue */
  if(Reason != OS_SCHED_READY_TO_RUN)
  {
    /* Make task not waiting */
    if(osCurrentTask->BlockingFlags & OS_BLOCK_FLAG_WAITING)
      osMakeNotWaiting(osCurrentTask);

    /* Insert item at the queue beginning */
    stPQueueInsert(&osTaskPQueue, &osCurrentTask->ReadyTask, osCurrentTask);
    stPQueueRotate(&osTaskPQueue, NULL, FALSE);
    osCurrentTask->Object.Flags |= OS_OBJECT_FLAG_READY_TO_RUN;

    /* Restart time quanta counter */
    #if (OS_USE_TIME_QUANTA)
      osCurrentTask->TimeQuantumCounter = osCurrentTask->MaxTimeQuantum;
    #endif
  }

  /* Round Robin fashion scheduling with time quanta */
  #if (OS_USE_TIME_QUANTA)
    osCurrentTask->TimeQuantumCounter--;
  #else
    stPQueueRotate(&osTaskPQueue, TRUE);
  #endif

  /* Update last time quantum assign time */
  if(osLastQuantumTime != CurrentTime)
  {
    osLastQuantumTime = CurrentTime;
    osLastQuantumIndex = 0;
  }

  /* Set the last time quantum assign time */
  osCurrentTask->LastQuantumTime = osLastQuantumTime;
  osCurrentTask->LastQuantumIndex = osLastQuantumIndex++;

  /* Task statistics (CPU usage) */
  #if (OS_USE_STATISTICS)
    if(CurrentTime >= (osCPUCalcTime + OS_STAT_SAMPLE_RATE))
    {
      osCPUUsageTime = osCPUCalcTime;
      osCPUUsage = osCPUCalc;
      osCPUCalcTime = CurrentTime;
      osCPUCalc = 1;
    }
    else
      osCPUCalc++;

    if(osCurrentTask->CPUCalcTime != osCPUCalcTime)
    {
      osCurrentTask->CPUUsageTime = osCurrentTask->CPUCalcTime;
      osCurrentTask->CPUUsage = osCurrentTask->CPUCalc;
      osCurrentTask->CPUCalcTime = osCPUCalcTime;
      osCurrentTask->CPUCalc = 1;
    }
    else
      osCurrentTask->CPUCalc++;
  #endif

  /* Restore task context */
  *TaskContext = osCurrentTask->TaskContext;
}


/****************************************************************************
 *
 *  Critical section management
 *
 ***************************************************************************/


/***************************************************************************/
#if (OS_USE_CSEC_OBJECTS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osCSAssocCmp
 *
 *  Description:
 *    Compares two critical sections by their priorities.
 *
 *  Parameters:
 *    Item1 - Pointer to first critical section association descriptor.
 *    Item2 - Pointer to second critical section association descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

int osCSAssocCmp(PVOID Item1, PVOID Item2)
{
  struct TWaitAssoc FAR *WaitAssoc1;
  struct TWaitAssoc FAR *WaitAssoc2;

  /* Return a difference between priorities of specified critical section
     associations */
  WaitAssoc1 = (struct TWaitAssoc FAR *) stBSTreeGetFirst(
    &((struct TCSAssoc FAR *) Item1)->CS->Signal->WaitingTasks);
  WaitAssoc2 = (struct TWaitAssoc FAR *) stBSTreeGetFirst(
    &((struct TCSAssoc FAR *) Item2)->CS->Signal->WaitingTasks);

  return
    (WaitAssoc1 ? (int) WaitAssoc1->Task->Priority : OS_LOWEST_PRIORITY) -
    (WaitAssoc2 ? (int) WaitAssoc2->Task->Priority : OS_LOWEST_PRIORITY);
}


/****************************************************************************
 *
 *  Name:
 *    osCSPtrCmp
 *
 *  Description:
 *    Compares two critical sections by their addresses.
 *
 *  Parameters:
 *    Item1 - Pointer to first critical section association descriptor.
 *    Item2 - Pointer to second critical section association descriptor.
 *
 *  Return:
 *    - < 0 if Item1 is less than Item2
 *    - 0 if Item1 is the same as Item2
 *    - > 0 if Item1 is greater than Item2
 *
 ***************************************************************************/

int osCSPtrCmp(PVOID Item1, PVOID Item2)
{
  /* Compare objects */
  if(((struct TCSAssoc FAR *) Item1)->CS <
    ((struct TCSAssoc FAR *) Item2)->CS)
    return -1;

  if(((struct TCSAssoc FAR *) Item1)->CS >
    ((struct TCSAssoc FAR *) Item2)->CS)
    return 1;

  /* Items are equal */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osCSAssocAlloc
 *
 *  Description:
 *    Allocates a new critical section association descriptor for the
 *    specified critical section. This function does not control error
 *    situations; they should never happen.
 *
 *  Parameters:
 *    CS - Address of the critical section descriptor.
 *
 *  Return:
 *    Address of the newly allocated critical section association
 *    descriptor.
 *
 ***************************************************************************/

static struct TCSAssoc FAR *osCSAssocAlloc(struct TCriticalSection *CS)
{
  struct TCSAssoc FAR *CSAssoc;

  /* Allocate a new critical section association descriptor */
  if(CS->FirstFree)
  {
    CSAssoc = CS->FirstFree;
    CS->FirstFree = CSAssoc->Next;
  }
  else
    CSAssoc = &CS->TasksInCS[CS->Count++];

  /* Add to list of allocated critical section association descriptors */
  CSAssoc->Prev = NULL;
  CSAssoc->Next = CS->FirstAllocated;
  if(CS->FirstAllocated)
    CS->FirstAllocated->Prev = CSAssoc;
  CS->FirstAllocated = CSAssoc;

  /* Pointer to newly allocated critical section association descriptor */
  return CSAssoc;
}


/****************************************************************************
 *
 *  Name:
 *    osCSAssocFree
 *
 *  Description:
 *    Releases the specified critical section association descriptor for
 *    the specified critical section.
 *
 *  Parameters:
 *    CS - Address of the critical section descriptor.
 *    CSAssoc - Pointer to critical section association descriptor to
 *      release.
 *
 ***************************************************************************/

static void osCSAssocFree(struct TCriticalSection *CS,
  struct TCSAssoc FAR *CSAssoc)
{
  /* Remove from list of allocated critical section association
     descriptors */
  if(!CSAssoc->Prev)
    CS->FirstAllocated = CSAssoc->Next;
  else
    CSAssoc->Prev->Next = CSAssoc->Next;
  if(CSAssoc->Next)
    CSAssoc->Next->Prev = CSAssoc->Prev;

  /* Add to list of free critical section association descriptors */
  CSAssoc->Next = CS->FirstFree;
  CS->FirstFree = CSAssoc;
}


/****************************************************************************
 *
 *  Name:
 *    osRegisterCS
 *
 *  Description:
 *    Registers a critical section descriptor.
 *
 *  Parameters:
 *    Signal - Pointer to the signal descriptor.
 *    CS - Pointer to the critical section descriptor.
 *    InitialCount - The initial count for the critical section.
 *    MaxCount - The maximum count for the critical section.
 *    MutualExclusion - Boolean flag for mutual exclusion mode.
 *
 ***************************************************************************/

void osRegisterCS(struct TSignal FAR *Signal,
  struct TCriticalSection FAR *CS, INDEX InitialCount, INDEX MaxCount,
  BOOL MutualExclusion)
{
  BOOL PrevLockState;

  /* Define signal */
  Signal->Flags = (UINT8)
    (OS_SIGNAL_FLAG_DEC_ON_RELEASE | OS_SIGNAL_FLAG_CRITICAL_SECTION |
    (MutualExclusion ? OS_SIGNAL_FLAG_MUTUAL_EXCLUSION : 0));
  Signal->Signaled = InitialCount;
  Signal->CS = CS;
  
  /* [!] NOTE: This might be redundant as it was already initialized */
  stBSTreeInit(&Signal->WaitingTasks, osWaitAssocCmp);

  /* Critical section descriptor */
  CS->Signal = Signal;
  CS->MaxSignaled = MaxCount;
  CS->PriorityPath.Task = NULL;
  CS->PriorityPath.CS = CS;
  CS->FirstFree = NULL;
  CS->Count = 0;

  /* When critical section is owned at creation, the corresponding
     association must be defined */
  if(InitialCount != MaxCount)
  {
    struct TCSAssoc FAR *CSAssoc;

    /* Define association */
    CSAssoc = osCSAssocAlloc(CS);
    CSAssoc->CS = CS;
    CSAssoc->Task = osCurrentTask;
    CSAssoc->Count = MaxCount - InitialCount;

    /* Enter critical section */
    PrevLockState = arLock();

    /* Assign association to task */
    stPQueueInsert(&osCurrentTask->OwnedCS, &CSAssoc->Item, CSAssoc);
    stBSTreeInsert(&osCurrentTask->OwnedCSPtr, &CSAssoc->Node, NULL,
      CSAssoc);

    /* Leave critical section */
    arRestore(PrevLockState);
  }
}


/****************************************************************************
 *
 *  Name:
 *    osPriorityPath
 *
 *  Description:
 *    Executes the priority path algorithm for the specified object.
 *    Function fails if a deadlock occurs. After detecting a deadlock,
 *    the Task must stop waiting and the osPriorityPath function must be
 *    called for each critical section that the task previously awaited.
 *
 *  Parameters:
 *    Priority - Pointer to priority path descriptor.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osPriorityPath(struct TPriorityPath FAR *Priority)
{
  struct TPriorityPath FAR *FirstPriority;
  struct TPriorityPath FAR *LastPriority;
  struct TPriorityPath FAR *NewPriority;
  struct TCriticalSection *CS;
  struct TCSAssoc *CSAssoc;
  struct TTask FAR *Task;

  /* Begin priority path */
  FirstPriority = Priority;
  LastPriority = Priority;
  Priority->Next = NULL;

  /* Analyze priority path */
  while(Priority)
  {
    /* Append any critical section that the task is waiting for into
       the priority path */
    Task = Priority->Task;
    if(Task)
    {
      if(Task->BlockingFlags & OS_BLOCK_FLAG_WAITING)
      {
        #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
          INDEX i;
          for(i = 0; i < Task->WaitingCount; i++)
          {
            CS = Task->WaitingFor[i].Signal->CS;
        #else
            CS = Task->WaitingFor[0].Signal->CS;
        #endif

            if(CS)
            {
              NewPriority = &CS->PriorityPath;
              NewPriority->Next = NULL;
              LastPriority->Next = NewPriority;
              LastPriority = NewPriority;
            }

        #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
          }
        #endif
      }
    }

    /* Append all tasks owning this critical section */
    else
    {
      CS = Priority->CS;
      CSAssoc = CS->FirstAllocated;
      while(CSAssoc)
      {
        Task = CSAssoc->Task;

        /* Stop immediately when deadlock has been detected */
        if(Task == FirstPriority->Task)
          return FALSE;

        /* Update task priority */
        osChangeTaskPriority(Task, Task->AssignedPriority);

        /* Add at the end of the priority path all critical sections that
           the task awaits */
        NewPriority = &Task->PriorityPath;
        NewPriority->Next = NULL;
        LastPriority->Next = NewPriority;
        LastPriority = NewPriority;

        /* For each task owning this critical section, update queue of
           critical sections, ordered by priorities of tasks waiting for
           critical section */
        stPQueueRemove(&Task->OwnedCS, &CSAssoc->Item);
        stPQueueInsert(&Task->OwnedCS, &CSAssoc->Item, CSAssoc);

        /* Check next association descriptor */
        CSAssoc = CSAssoc->Next;
      }

      /* Tasks waiting for a critical section have updated priority */
      osSignalUpdated(CS->Signal);
    }

    /* Check next object in the priority path */
    Priority = Priority->Next;
  }

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osFindCSAssoc
 *
 *  Description:
 *    Returns a pointer to the critical section association descriptor
 *    associating the specified task and critical section.
 *
 *  Parameters:
 *    CS - Pointer to critical section descriptor.
 *    Task - Pointer to task descriptor.
 *
 *  Return:
 *    Pointer to found association descriptor or NULL if not exists.
 *
 ***************************************************************************/

static struct TCSAssoc FAR *osFindCSAssoc(struct TCriticalSection *CS,
  struct TTask *Task)
{
  struct TBSTreeNode FAR *BSTreeNode;
  struct TCSAssoc FAR *CSAssoc = NULL;

  /* Optimize search when only one task can own a critical section.
     [!] NOTE: Optimization depends on MaxSignaled, not Count, as
     positions are variable (0: NULL, 1: first, ...). */
  if(CS->MaxSignaled == 1)
  {
    CSAssoc = &CS->TasksInCS[0];
    if(CSAssoc->Task != Task)
      return NULL;
  }

  /* Find existing association */
  BSTreeNode = (struct TBSTreeNode FAR *) Task->OwnedCSPtr.Root;
  while(BSTreeNode)
  {
    CSAssoc = (struct TCSAssoc FAR *) BSTreeNode->Data;
    if(CS == CSAssoc->CS)
      break;

    BSTreeNode = (CS <= CSAssoc->CS) ?
      CSAssoc->Node.Left : CSAssoc->Node.Right;
  }

  /* Return pointer to found association descriptor */
  return BSTreeNode ? CSAssoc : NULL;
}


/***************************************************************************/
#endif /* OS_USE_CSEC_OBJECTS */
/***************************************************************************/


/***************************************************************************/
#if (OS_MODIFIABLE_TASK_PRIO)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osChangeTaskPriority
 *
 *  Description:
 *    Changes task priority and updates system queues ordered by task priority.
 *
 *  Parameters:
 *    Task - Pointer to task descriptor.
 *    Priority - New task priority.
 *
 *  Return:
 *    TRUE when inherited priority has been changed, otherwise FALSE.
 *
 ***************************************************************************/

BOOL osChangeTaskPriority(struct TTask FAR *Task, UINT8 Priority)
{
  BOOL IsHigher;
  struct TWaitAssoc FAR *WaitAssoc;
  struct TSignal FAR *Signal;

  #if (OS_USE_CSEC_OBJECTS)
    struct TCSAssoc FAR *CSAssoc;
  #endif

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX i;
  #endif

  /* Calculate expected value of inherited priority */
  #if (OS_USE_CSEC_OBJECTS)
    CSAssoc = (struct TCSAssoc FAR *) stPQueueGet(&Task->OwnedCS);
    if(CSAssoc)
    {
      WaitAssoc = (struct TWaitAssoc FAR *)
        stBSTreeGetFirst(&CSAssoc->CS->Signal->WaitingTasks);
      if(WaitAssoc)
        if(Priority > WaitAssoc->Task->Priority)
          Priority = WaitAssoc->Task->Priority;
    }
  #endif

  /* Skip if already set */
  if(Task->Priority == Priority)
    return FALSE;

  /* Assign new priority */
  IsHigher = (BOOL) (Priority < Task->Priority);
  Task->Priority = Priority;

  /* Rearrange task queue. If new priority is higher than current, a task
     will be stored at the beginning of the queue to be processed
     immediately, otherwise at the end. */
  if(Task->Object.Flags & OS_OBJECT_FLAG_READY_TO_RUN)
  {
    stPQueueRemove(&osTaskPQueue, &Task->ReadyTask);
    stPQueueInsert(&osTaskPQueue, &Task->ReadyTask, Task);
    if(IsHigher)
      stPQueueRotate(&osTaskPQueue, NULL, FALSE);

    /* [!] ISSUE: Is rotate correct? Should it be for a node on
       another level? */
  }

  /* Update time notification */
  #if (OS_USE_TIME_OBJECTS)
    if(Task->WaitTimeout.Registered)
    {
      osUnregisterTimeNotify(&Task->WaitTimeout);
      osRegisterTimeNotify(&Task->WaitTimeout, Task->WaitTimeout.Time);

      /* [!] ISSUE: Rotate as above? */
      /* [!] ISSUE: What about the timer if waiting for it? */
    }
  #endif

  /* Rearrange waiting queue */
  if(Task->BlockingFlags & OS_BLOCK_FLAG_WAITING)
  {
    #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
      for(i = 0; i < Task->WaitingCount; i++)
      {
        WaitAssoc = &Task->WaitingFor[i];
        Signal = WaitAssoc->Signal;
        stBSTreeRemove(&Signal->WaitingTasks, &WaitAssoc->Node);
        stBSTreeInsert(&Signal->WaitingTasks, &WaitAssoc->Node,
           NULL, WaitAssoc);
        osSignalUpdated(Signal);

        /* [!] TODO: Time Notify timer updates... */
      }
    #else
      WaitAssoc = &Task->WaitingFor[0];
      Signal = WaitAssoc->Signal;
      stBSTreeRemove(&Signal->WaitingTasks, &WaitAssoc->Node);
      stBSTreeInsert(&Signal->WaitingTasks, &WaitAssoc->Node,
        NULL, WaitAssoc);
      osSignalUpdated(Signal);

      /* [!] TODO: Time Notify timer updates... */
    #endif
  }

  /* Priority has been changed */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osRescheduleIfHigherPriority
 *
 *  Description:
 *    Calls scheduler if some task has priority higher than the priority
 *    of the current task.
 *
 ***************************************************************************/

void osRescheduleIfHigherPriority(void)
{
  struct TSignal FAR *Signal;
  struct TWaitAssoc FAR *WaitAssoc;

  #if (OS_USE_TIME_OBJECTS)
    struct TTimeNotify FAR *TimeNotify;
  #endif

  while(TRUE)
  {
    /* Check first task in queue */
    if(osCurrentTask->Priority >
      ((struct TTask FAR *) stPQueueGet(&osTaskPQueue))->Priority)
      break;

    /* Time notification */
    #if (OS_USE_TIME_OBJECTS)
      TimeNotify = osGetTimeNotify(osCurrentTask->Priority,
        osLastQuantumTime);
      if(TimeNotify)
      {
        if(TimeNotify->Task)
        {
          if(osCurrentTask->Priority > TimeNotify->Task->Priority)
            break;
        }
        else
        {
          WaitAssoc = (struct TWaitAssoc FAR *)
            stBSTreeGetFirst(&TimeNotify->Signal->WaitingTasks);
          if(WaitAssoc)
            if(osCurrentTask->Priority > WaitAssoc->Task->Priority)
              break;
        }
      }
    #endif

    /* Deferred signalization */
    Signal = (struct TSignal FAR *) stBSTreeGetFirst(&osDeferredSignal);
    if(Signal)
    {
      WaitAssoc =
        (struct TWaitAssoc FAR *) stBSTreeGetFirst(&Signal->WaitingTasks);
      if(WaitAssoc)
        if(osCurrentTask->Priority > WaitAssoc->Task->Priority)
          break;
    }

    /* Each task has priority lower or equal to priority of the current
       task */
    return;
  }

  /* Reschedule, because some task has higher priority than current */
  osYield();
}


/***************************************************************************/
#endif /* OS_MODIFIABLE_TASK_PRIO */
/***************************************************************************/


/****************************************************************************
 *
 *  Synchronization
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osAcquire
 *
 *  Description:
 *    Tries to acquire the signal.
 *
 *  Parameters:
 *    Signal - Pointer to the signal descriptor.
 *    OnCheck - If TRUE, the task will not acquire the signal when it has
 *      the "decrement on release" flag set and the task priority is lower
 *      or equal to the priority of the signal. This queues signal acquire
 *      requests.
 *
 *  Return:
 *    TRUE if object was acquired, otherwise FALSE.
 *
 ***************************************************************************/

static BOOL osAcquire(struct TSignal FAR *Signal, BOOL OnCheck)
{
  struct TWaitAssoc FAR *WaitAssoc;

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX i;
  #endif

  #if (OS_USE_CSEC_OBJECTS)
    struct TCSAssoc FAR *CSAssoc;
  #endif

  /* Check signalization state */
  #if (OS_USE_SYSTEM_IO_CTRL)
    if(Signal->Flags & OS_SIGNAL_FLAG_USES_IO_SYSTEM)
    {
      if(!Signal->Object->DeviceIOCtrl(Signal->Object,
        OS_IO_CTL_GET_SIGNAL_STATE, NULL, 0, NULL))
        return FALSE;
    }
    else
  #endif

  if(!Signal->Signaled)
  {
    /* Mutexes are always signaled for owning task */
    #if (OS_USE_CSEC_OBJECTS)
      if(Signal->Flags & OS_SIGNAL_FLAG_MUTUAL_EXCLUSION)
        if(Signal->CS->TasksInCS[0].Task == osCurrentTask)
          return TRUE;
    #endif

    /* All other objects cannot be acquired when not signaled */
    return FALSE;
  }

  /* Decrement signal counter when the "decrement on release" is set */
  if(Signal->Flags & OS_SIGNAL_FLAG_DEC_ON_RELEASE)
  {
    /* Check signal priority */
    if(OnCheck)
    {
      WaitAssoc = (struct TWaitAssoc FAR *)
        stBSTreeGetFirst(&Signal->WaitingTasks);
      if(WaitAssoc)
        if(WaitAssoc->Task->Priority <= osCurrentTask->Priority)
          return FALSE;
    }

    /* Decrement signal state */
    Signal->Signaled--;
    osSignalUpdated(Signal);
  }

  /* Define association between the task and a critical section */
  #if (OS_USE_CSEC_OBJECTS)
    if(Signal->Flags & OS_SIGNAL_FLAG_CRITICAL_SECTION)
    {
      /* Try to find existing association */
      CSAssoc = osFindCSAssoc(Signal->CS, osCurrentTask);
      if(CSAssoc)
        CSAssoc->Count++;

      /* Define new association */
      else
      {
        CSAssoc = osCSAssocAlloc(Signal->CS);
        CSAssoc->CS = Signal->CS;
        CSAssoc->Task = osCurrentTask;
        CSAssoc->Count = 1;

        /* Assign association to task */
        stPQueueInsert(&osCurrentTask->OwnedCS, &CSAssoc->Item, CSAssoc);
        stBSTreeInsert(&osCurrentTask->OwnedCSPtr, &CSAssoc->Node, NULL,
          CSAssoc);
      }

      /* Is the "abandon flag" set? */
      if(Signal->Flags & OS_SIGNAL_FLAG_ABANDONED)
      {
        Signal->Flags &= (UINT8) ~OS_SIGNAL_FLAG_ABANDONED;
        osCurrentTask->WaitExitCode = ERR_WAIT_ABANDONED;
      }
    }
  #endif

  /* Notify object acquiring */
  #if (OS_USE_SYSTEM_IO_CTRL)
    if(Signal->Flags & OS_SIGNAL_FLAG_USES_IO_SYSTEM)
      Signal->Object->DeviceIOCtrl(Signal->Object, OS_IO_CTL_WAIT_ACQUIRE,
        NULL, 0, NULL);
  #endif

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osMakeNotWaiting
 *
 *  Description:
 *    Exits from the wait state.
 *
 *  Parameters:
 *    Task - Pointer to task that should exit from the wait state.
 *
 ***************************************************************************/

void osMakeNotWaiting(struct TTask FAR *Task)
{
  struct TWaitAssoc FAR *WaitAssoc;
  struct TSignal FAR *Signal;

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX i;
  #endif

  /* Remove waiting flags */
  Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_WAITING;

  /* Remove task from waiting queue of each signal */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    for(i = 0; i < Task->WaitingCount; i++)
    {
      WaitAssoc = &Task->WaitingFor[i];
  #else
      WaitAssoc = &Task->WaitingFor[0];
  #endif

      Signal = WaitAssoc->Signal;
      stBSTreeRemove(&Signal->WaitingTasks, &WaitAssoc->Node);
      osSignalUpdated(Signal);

      /* [!] TODO: Unregister timer by OS_IO_CTL_WAIT_UPDATE... */

      /* Update priority path */
      #if (OS_USE_CSEC_OBJECTS)
        if(Signal->CS)
          osPriorityPath(&Signal->CS->PriorityPath);
      #endif

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    }
  #endif
}


/****************************************************************************
 *
 *  Name:
 *    osMakeWaiting
 *
 *  Description:
 *    Makes the current task wait for specified signals. The task will
 *    wait until the signal is in the signaled state or the specified
 *    timeout elapses. To exit from the wait state, osMakeNotWaiting must
 *    be called.
 *
 *  Parameters:
 *    Timeout - Timeout value in time units.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

static BOOL osMakeWaiting(TIME Timeout)
{
  BOOL PrevLockState;
  struct TWaitAssoc FAR *WaitAssoc;
  struct TSignal FAR *Signal;

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX i;
  #endif

  /* Check timeout value */
  #if !(OS_USE_TIME_OBJECTS)
    if((Timeout != OS_IGNORE) && (Timeout != OS_INFINITE))
    {
      osSetLastError(ERR_INVALID_PARAMETER);
      return FALSE;
    }
  #endif

  /* Obtain pointers of descriptors */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) == 1)
    WaitAssoc = &osCurrentTask->WaitingFor[0];
    Signal = WaitAssoc->Signal;
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* [!] NOTE: Will be cleared already */
  osCurrentTask->WaitExitCode = ERR_NO_ERROR;

  /* Check object signalization */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    for(i = 0; i < osCurrentTask->WaitingCount; i++)
      if(osAcquire(osCurrentTask->WaitingFor[i].Signal, TRUE))
      {
        arRestore(PrevLockState);
        osCurrentTask->WaitingIndex = i;

        /* Return with error on acquiring abandoned critical section */
        #if (OS_USE_CSEC_OBJECTS)
          if(osCurrentTask->WaitExitCode)
          {
            osSetLastError(osCurrentTask->WaitExitCode);
            return FALSE;
          }
        #endif

        return TRUE;
      }
  #else
    if(osAcquire(Signal, TRUE))
    {
      arRestore(PrevLockState);

      /* Return with error on acquiring abandoned critical section */
      #if (OS_USE_CSEC_OBJECTS)
        if(osCurrentTask->WaitExitCode)
        {
          osSetLastError(osCurrentTask->WaitExitCode);
          return FALSE;
        }
      #endif

      return TRUE;
    }
  #endif

  /* Return immediately if timeout value is set to OS_IGNORE (or 0) */
  if(Timeout == OS_IGNORE)
  {
    arRestore(PrevLockState);
    osSetLastError(ERR_WAIT_TIMEOUT);
    return FALSE;
  }

  /* Set the waiting flag (interrupts are disabled, so code will be
     continued uninterrupted) */
  osCurrentTask->BlockingFlags |= OS_BLOCK_FLAG_WAITING;

  /* Append waiting tasks to waiting queue of specified signals */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    for(i = 0; i < osCurrentTask->WaitingCount; i++)
    {
      WaitAssoc = &osCurrentTask->WaitingFor[i];
      Signal = WaitAssoc->Signal;

      stBSTreeInsert(&Signal->WaitingTasks, &WaitAssoc->Node, NULL,
        WaitAssoc);
      osSignalUpdated(Signal);

      #if (OS_USE_SYSTEM_IO_CTRL)
        if(Signal->Flags & OS_SIGNAL_FLAG_USES_IO_SYSTEM)
          Signal->Object->DeviceIOCtrl(Signal->Object, OS_IO_CTL_WAIT_START,
            NULL, 0, NULL);
      #endif
    }
  #else
    stBSTreeInsert(&Signal->WaitingTasks, &WaitAssoc->Node, NULL, WaitAssoc);
    osSignalUpdated(Signal);

    #if (OS_USE_SYSTEM_IO_CTRL)
      if(Signal->Flags & OS_SIGNAL_FLAG_USES_IO_SYSTEM)
        Signal->Object->DeviceIOCtrl(Signal->Object, OS_IO_CTL_WAIT_START,
          NULL, 0, NULL);
    #endif
  #endif

  /* Set the timeout */
  #if (OS_USE_TIME_OBJECTS)
    if(Timeout != OS_INFINITE)
    {
      TIME CurrentTime;
      CurrentTime = arGetTickCount();

      /* Control time overflow */
      Timeout = ((OS_INFINITE - CurrentTime) <= Timeout) ?
        OS_INFINITE : (CurrentTime + Timeout);

      osRegisterTimeNotify(&osCurrentTask->WaitTimeout, Timeout);
    }
  #endif

  /* Update priority path for waiting task */
  #if (OS_USE_CSEC_OBJECTS)
    if(!osPriorityPath(&osCurrentTask->PriorityPath))
    {
      /* Exit wait state when deadlock occurs */
      osMakeNotWaiting(osCurrentTask);
      arRestore(PrevLockState);
      osSetLastError(ERR_WAIT_DEADLOCK);
      return FALSE;
    }
  #endif

  /* Begin waiting and reschedule. This function will return when one of the
     signals is in the signaled state or specified timeout interval
     elapses. */
  /* [!] NOTE: already cleared above */
  osCurrentTask->WaitExitCode = ERR_NO_ERROR;
  osMakeNotReady(osCurrentTask);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Set the last error code when error occurs */
  /* [!] ISSUE: Remove ifdefs? */
  #if ((OS_USE_WAITING_WITH_TIME_OUT) || (OS_USE_CSEC_OBJECTS) || \
    (OS_USE_IPC_DIRECT_RW))
    if(osCurrentTask->WaitExitCode != ERR_NO_ERROR)
    {
      osSetLastError(osCurrentTask->WaitExitCode);
      return FALSE;
    }
  #endif

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osUpdateSignalState
 *
 *  Description:
 *    Changes object signalization state.
 *
 *  Parameters:
 *    Signal - Pointer to signal descriptor.
 *    Signaled - New signal value.
 *
 ***************************************************************************/

void osUpdateSignalState(struct TSignal FAR *Signal, INDEX Signaled)
{
  BOOL PrevLockState, NeedUpdate;
  struct TTask FAR *Task;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Is signal updating mandatory? */
  NeedUpdate = ((BOOL) (Signal->Signaled > 0)) ^ ((BOOL) (Signaled > 0));

  /* Change signalization state */
  Signal->Signaled = Signaled;

  /* Update signal when it is mandatory */
  /* [!] CHECK: It is ok here before calling osSignalUpdated - verify
     elsewhere! */
  if(NeedUpdate)
  {
    Task = osSignalUpdated(Signal);

    /* When signal is in the signaled state and priority of task waiting for
       signal is higher than priority of the current task, reschedule
       immediately */
    if(Signaled && Task)
      if(Task->Priority < osCurrentTask->Priority)
        osYield();
  }

  /* Leave critical section */
  arRestore(PrevLockState);
}


/***************************************************************************/
#if (OS_USE_CSEC_OBJECTS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseCS
 *
 *  Description:
 *    Releases a specified critical section.
 *
 *  Parameters:
 *    CS - Pointer to critical section descriptor.
 *    Task - Pointer to task descriptor.
 *    ReleaseCount - The value which will be added to critical section
 *      counter. If it is too large, the critical section will not be
 *      released and an error occurs. Must be greater than zero.
 *    PrevCount - Pointer to variable that receives the counter value
 *      before releasing (may be NULL if not expected).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osReleaseCS(struct TCriticalSection FAR *CS, struct TTask FAR *Task,
  INDEX ReleaseCount, INDEX *PrevCount)
{
  BOOL PrevLockState;
  struct TCSAssoc FAR *CSAssoc;

  /* Operation can be performed only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return FALSE;
  }

  /* Enter critical section (disable interrupts) */
  PrevLockState = arLock();

  /* Fail if ReleaseCount is incorrect */
  CSAssoc = osFindCSAssoc(CS, Task);
  if(CSAssoc)
    if(ReleaseCount > CSAssoc->Count)
      CSAssoc = NULL;

  /* Fail if association is not defined */
  if(!CSAssoc)
  {
    arRestore(PrevLockState);
    osSetLastError(ERR_OBJECT_CAN_NOT_BE_RELEASED);
    return FALSE;
  }

  /* Subtract the ReleaseCount from the counter value */
  CSAssoc->Count -= ReleaseCount;

  /* Remove association when counter is zero */
  if(!CSAssoc->Count)
  {
    stPQueueRemove(&Task->OwnedCS, &CSAssoc->Item);
    stBSTreeRemove(&Task->OwnedCSPtr, &CSAssoc->Node);
    osCSAssocFree(CS, CSAssoc);

    /* Update task priority. The task is not waiting for any signal, so
       priority path is already valid. */
    osChangeTaskPriority(Task, Task->AssignedPriority);
  }

  /* Save previous counter value */
  if(PrevCount)
    *PrevCount = CS->Signal->Signaled;

  /* Release critical section */
  osUpdateSignalState(CS->Signal, CS->Signal->Signaled + ReleaseCount);

  /* Leave critical section (restore interrupts) */
  arRestore(PrevLockState);
  return TRUE;
}


/***************************************************************************/
#endif /* OS_USE_CSEC_OBJECTS */
/***************************************************************************/


/***************************************************************************/
#if (OS_USE_MULTIPLE_SIGNALS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osWaitFor
 *
 *  Description:
 *    Switches the task into a wait state until the specified signal is in
 *    the signaled state or the specified timeout interval elapses.
 *
 *  Parameters:
 *    Signal - Pointer to signal descriptor.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osWaitFor(struct TSignal FAR *Signal, TIME Timeout)
{
  /* Prepare association descriptor */
  osCurrentTask->WaitingFor[0].Signal = Signal;
  osCurrentTask->WaitingFor[0].Task = osCurrentTask;

  /* Number of objects that the task is waiting for */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    osCurrentTask->WaitingCount = 1;
  #endif

  /* Start waiting for specified object */
  return osMakeWaiting(Timeout);
}


/***************************************************************************/
#endif /* OS_USE_MULTIPLE_SIGNALS */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSleep
 *
 *  Description:
 *    Suspends current task execution for the specified amount of time.
 *    When time is set to OS_IGNORE, task execution will not be suspended,
 *    but the task voluntarily yields its execution.
 *
 *  Parameters:
 *    Time - Amount of time for which execution is to be suspended,
 *      specified in time units.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSleep(TIME Time)
{
  #if (OS_USE_TIME_OBJECTS)
    BOOL PrevLockState;
  #endif

  /* Operation can be performed only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return FALSE;
  }

  /* Voluntarily yield execution when time is set to OS_IGNORE */
  if(Time == OS_IGNORE)
  {
    #if (OS_USE_TIME_QUANTA)
      osCurrentTask->TimeQuantumCounter = 0;
    #endif

    osYield();
  }
  else
  {
    /* Sleep for specified amount of time */
    #if (OS_USE_TIME_OBJECTS)

      TIME CurrentTime;
      CurrentTime = arGetTickCount();

      /* Control time overflow */
      Time = ((OS_INFINITE - CurrentTime) <= Time) ?
        OS_INFINITE : (CurrentTime + Time);

      /* Enter critical section */
      PrevLockState = arLock();

      /* Suspend task execution for specified amount of time */
      osCurrentTask->BlockingFlags |= OS_BLOCK_FLAG_SLEEP;
      osRegisterTimeNotify(&osCurrentTask->WaitTimeout, Time);
      osMakeNotReady(osCurrentTask);

      /* Leave critical section */
      arRestore(PrevLockState);

    /* Return immediately with error when timeouts are disabled */
    #else
      osSetLastError(ERR_INVALID_PARAMETER);
      return FALSE;
    #endif
  }

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osWaitForObject
 *
 *  Description:
 *    Switches task into wait state until the specified object is in the
 *    signaled state or the specified timeout interval elapses.
 *
 *  Parameters:
 *    Handle - Object handle.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osWaitForObject(HANDLE Handle, TIME Timeout)
{
  struct TSysObject FAR *Object;

  /* Operation can be performed only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_IGNORE);
  if(!Object)
    return FALSE;

  /* Prepare association descriptor */
  osCurrentTask->WaitingFor[0].Signal = &Object->Signal;
  osCurrentTask->WaitingFor[0].Task = osCurrentTask;

  /* Number of objects that the task is waiting for */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    osCurrentTask->WaitingCount = 1;
  #endif

  /* Start waiting for specified object */
  return osMakeWaiting(Timeout);
}


/***************************************************************************/
#if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osWaitForObjects
 *
 *  Description:
 *    Switches task into wait state until at least one of the specified
 *    objects is in the signaled state or the specified timeout interval
 *    elapses.
 *
 *  Parameters:
 *    Handles - Array of object handles.
 *    Count - Number of handles in array.
 *    ObjectIndex - Pointer to variable that will receive a reason index.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osWaitForObjects(HANDLE *Handles, INDEX Count, TIME Timeout,
  INDEX *ObjectIndex)
{
  INDEX i;
  BOOL Success;
  struct TSysObject FAR *Object;
  struct TWaitAssoc FAR *WaitAssoc;

  /* Check parameters */
  if((Count <= 0) || (Count > OS_MAX_WAIT_FOR_OBJECTS))
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Operation can be performed only by a task */
  if(!osCurrentTask || osInISR)
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return FALSE;
  }

  /* Prepare association descriptors */
  for(i = 0; i < Count; i++)
  {
    /* Get object by handle */
    Object = osGetObjectByHandle(Handles[i], OS_OBJECT_TYPE_IGNORE);
    if(!Object)
      return FALSE;

    /* Prepare association descriptor */
    WaitAssoc = &osCurrentTask->WaitingFor[i];
    WaitAssoc->Signal = &Object->Signal;
    WaitAssoc->Task = osCurrentTask;
    WaitAssoc->Index = i;
  }

  /* Number of objects that the task is waiting for */
  osCurrentTask->WaitingCount = Count;

  /* Start waiting for specified objects */
  Success = osMakeWaiting(Timeout);

  /* Set the index of the object that caused osMakeWaiting function exit */
  if(Success && ObjectIndex)
    *ObjectIndex = osCurrentTask->WaitingIndex;

  /* Return completion status */
  return Success;
}


/***************************************************************************/
#endif /* OS_MAX_WAIT_FOR_OBJECTS */
/***************************************************************************/


/****************************************************************************
 *
 *  Interprocess communication
 *
 ***************************************************************************/


/***************************************************************************/
#if (OS_READ_WRITE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osRead
 *
 *  Description:
 *    Reads data from a specified system object.
 *
 *  Parameters:
 *    Handle - Object handle.
 *    Buffer - Pointer to buffer to receive data.
 *    Size - Number of bytes to read.
 *    IORequest - Pointer to IORequest structure (may be NULL).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osRead(HANDLE Handle, PVOID Buffer, SIZE Size,
  struct TIORequest *IORequest)
{
  #if (OS_USE_DEVICE_IO_CTRL)

    struct TSysObject FAR *Object;

    /* Get object by handle */
    Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_IGNORE);
    if(!Object)
      return FALSE;

    /* Is the device IO control function implemented for this object? */
    if(!Object->DeviceIOCtrl)
    {
      osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
      return FALSE;
    }

    /* Read data from the object */
    return (BOOL) Object->DeviceIOCtrl(Object, DEV_IO_CTL_READ, Buffer, Size,
      IORequest);

  #else

    /* Mark unused parameters */
    AR_UNUSED_PARAM(Handle);
    AR_UNUSED_PARAM(Buffer);
    AR_UNUSED_PARAM(Size);
    AR_UNUSED_PARAM(IORequest);

    /* This function cannot be used with any available system object */
    osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
    return FALSE;

  #endif
}


/****************************************************************************
 *
 *  Name:
 *    osRead
 *
 *  Description:
 *    Writes data to the specified system object.
 *
 *  Parameters:
 *    Handle - Object handle.
 *    Buffer - Pointer to buffer with data to write.
 *    Size - Number of bytes to write.
 *    IORequest - Pointer to IORequest structure (may be NULL).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osWrite(HANDLE Handle, PVOID Buffer, SIZE Size,
  struct TIORequest *IORequest)
{
  #if (OS_USE_DEVICE_IO_CTRL)

    struct TSysObject FAR *Object;

    /* Get object by handle */
    Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_IGNORE);
    if(!Object)
      return FALSE;

    /* Is the device IO control function implemented for this object? */
    if(!Object->DeviceIOCtrl)
    {
      osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
      return FALSE;
    }

    /* Write data to the object */
    return (BOOL) Object->DeviceIOCtrl(Object, DEV_IO_CTL_WRITE, Buffer, Size,
      IORequest);

  #else

    /* Mark unused parameters */
    AR_UNUSED_PARAM(Handle);
    AR_UNUSED_PARAM(Buffer);
    AR_UNUSED_PARAM(Size);
    AR_UNUSED_PARAM(IORequest);

    /* This function cannot be used with any available system object */
    osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
    return FALSE;

  #endif
}


/***************************************************************************/
#endif /* OS_READ_WRITE_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  General system management
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osIdleTaskProc
 *
 *  Description:
 *    System idle task. The task runs whenever the CPU is not used by other
 *    tasks. It allows switching the CPU to power-save mode, which resumes
 *    the CPU on every interrupt.
 *
 ***************************************************************************/

static void CALLBACK osIdleTaskProc(void)
{
  /* Call arSavePower function to reduce power consumption */
  while(1)
    arSavePower();
}


/****************************************************************************
 *
 *  Name:
 *    osInit
 *
 *  Description:
 *    Initializes the operating system.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osInit(void)
{
  #if (OS_USE_TIME_OBJECTS)
    int i;
  #endif

  #if (!((OS_DEINIT_FUNC) || (OS_USE_STATISTICS)))
    struct TTask FAR *osIdleTask;
  #endif

  /* Initialize internal system memory pool */
  #if (!(OS_USE_FIXMEM_POOLS) && ((OS_INTERNAL_MEMORY_SIZE) > 0))
    if(!stMemoryInit(osMemoryPool, OS_INTERNAL_SYS_MEM_SIZE))
      return FALSE;
  #endif

  /* Initialize fixed size memory pool 0 */
  #if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL0_SIZE) > 0))
    if(!stFixedMemInit(osFixMemPool0, OS_FIX_POOL0_SIZE,
      OS_FIX_POOL0_ITEM_SIZE))
      return FALSE;
  #endif

  /* Initialize fixed size memory pool 1 */
  #if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL1_SIZE) > 0))
    if(!stFixedMemInit(osFixMemPool1, OS_FIX_POOL1_SIZE,
      OS_FIX_POOL1_ITEM_SIZE))
      return FALSE;
  #endif

  /* Initialize fixed size memory pool 2 */
  #if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL2_SIZE) > 0))
    if(!stFixedMemInit(osFixMemPool2, OS_FIX_POOL2_SIZE,
      OS_FIX_POOL2_ITEM_SIZE))
      return FALSE;
  #endif

  /* Initialize fixed size memory pool 3 */
  #if ((OS_USE_FIXMEM_POOLS) && ((OS_FIX_POOL3_SIZE) > 0))
    if(!stFixedMemInit(osFixMemPool3, OS_FIX_POOL3_SIZE,
      OS_FIX_POOL3_ITEM_SIZE))
      return FALSE;
  #endif

  /* Last error code set to no error */
  osLastErrorCode = ERR_NO_ERROR;

  /* Not in the ISR */
  osInISR = FALSE;

  /* Initialize osStart and osStop functions */
  #if (OS_STOP_FUNC)
    osSaveCallerAndStart = FALSE;
    osRestoreCallerAndStop = FALSE;
  #endif

  /* Initialize time notification */
  #if (OS_USE_TIME_OBJECTS)
    stPQueueInit(&osTimeNotifyQueue, osTimeNotifyCmp);

    for(i = 1; i < OS_PRIORITY_COUNT + OS_LOWEST_USED_PRIORITY + 2; i++)
      osTimeNotifyArr[i] = OS_INFINITE;

    for(i = 0; i <= OS_LOWEST_USED_PRIORITY; i++)
      osTimeNotify[i] = NULL;
  #endif

  /* Initialize binary search tree for registered system object names */
  #if (OS_USE_OBJECT_NAMES)
    stBSTreeInit(&osSysNames, osObjectByNameCmp);
  #endif

  /* Initialize deferred signalization queue */
  stBSTreeInit(&osDeferredSignal, osSignalCmp);

  /* Initialize ready to run task queue */
  stPQueueInit(&osTaskPQueue, osRoundRobinTaskCmp);

  /* Current task pointer (NULL means that operating system is stopped) */
  osCurrentTask = NULL;

  /* Last time quantum assign time */
  osLastQuantumTime = 0;
  osLastQuantumIndex = 0;

  /* Initialize system statistics */
  #if (OS_USE_STATISTICS)
    osCPUUsageTime = OS_INFINITE;
    osCPUUsage = 0;
    osCPUCalcTime = arGetTickCount();
    osCPUCalc = 0;
  #endif

  /* Prepare idle task */
  osIdleTask = (struct TTask FAR *) osMemAlloc(sizeof(*osIdleTask));
  if(!osIdleTask)
    return FALSE;

  stMemSet(osIdleTask, 0x00, sizeof(*osIdleTask));

  osIdleTask->Object.Type = OS_OBJECT_TYPE_TASK;

  #if (OS_ALLOW_OBJECT_DELETION)
    osIdleTask->Object.OwnerCount = 1;
  #endif

  osIdleTask->Object.ObjectDesc = osIdleTask;

  osIdleTask->Priority = OS_LOWEST_PRIORITY;
  #if (OS_USE_CSEC_OBJECTS)
    osIdleTask->AssignedPriority = OS_LOWEST_PRIORITY;
  #endif

  #if (OS_USE_TIME_QUANTA)
    osIdleTask->MaxTimeQuantum = 1;
  #endif

  #if (OS_USE_STATISTICS)
    osIdleTask->CPUUsageTime = OS_INFINITE;
    osIdleTask->CPUCalcTime = osCPUUsageTime;
  #endif

  /* Initialize system object deinitialization list */
  #if (OS_DEINIT_FUNC)
    osFirstObject = &osIdleTask->Object;
  #endif

  /* Create idle task context */
  if(!arCreateTaskContext(&osIdleTask->TaskContext, osIdleTaskProc,
    OS_IDLE_STACK_SIZE))
  {
    osMemFree(osIdleTask);
    return FALSE;
  }

  /* Make idle task ready to run */
  osMakeReady(osIdleTask);

  /* Set scheduler function as preemption handler */
  if(!arSetPreemptiveHandler(osScheduler, OS_STACK_SIZE))
  {
    arReleaseTaskContext(&osIdleTask->TaskContext);
    osMemFree(osIdleTask);
    return FALSE;
  }

  /* Initialization success */
  return TRUE;
}


/***************************************************************************/
#if (OS_DEINIT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osDeinit
 *
 *  Description:
 *    Deinitializes the operating system. Function may be executed only when
 *    the operating system is not running. It removes all resources allocated
 *    by the operating system (not by tasks) and calls the deinit IO function
 *    for each object (if defined).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osDeinit(void)
{
  struct TBSTreeNode FAR *Node;
  struct TSysObject FAR *Object;
  struct TTask FAR *Task;

  /* Operating System cannot be running at deinitialization */
  if(osCurrentTask || osInISR)
  {
    osSetLastError(ERR_OS_CAN_NOT_BE_RUNNING);
    return FALSE;
  }

  /* Deinitialization of the system objects */
  while(osFirstObject)
  {
    Object = osFirstObject;
    osFirstObject = osFirstObject->NextObject;

    /* Deleting object is a task */
    if(Object->Type == OS_OBJECT_TYPE_TASK)
    {
      Task = (struct TTask FAR *) Object->ObjectDesc;

      /* Release task children */
      #if (!(OS_USE_FIXMEM_POOLS) && !(OS_INTERNAL_MEMORY_SIZE))
        while(TRUE)
        {
          Node = (struct TBSTreeNode FAR *) Task->Childs.Min;
          if(!Node)
            break;

          stBSTreeRemove(&Task->Childs, Node);
          osMemFree(Node);
        }
      #endif

      /* Release task context */
      arReleaseTaskContext(&Task->TaskContext);
    }

    /* Call deinit IO */
    #if (OS_USE_DEVICE_IO_CTRL)
      if(Object->Flags & OS_OBJECT_FLAG_USES_IO_DEINIT)
        Object->DeviceIOCtrl(Object, DEV_IO_CTL_DEINIT, NULL, 0, NULL);
    #endif

    /* Release system object descriptor */
    #if (!(OS_USE_FIXMEM_POOLS) || ((OS_INTERNAL_MEMORY_SIZE) <= 0))
      osMemFree(Object);
    #endif
  }

  /* Restore default clock handler */
  return arSetPreemptiveHandler(NULL, 0);
}


/***************************************************************************/
#endif /* OS_DEINIT_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osStart
 *
 *  Description:
 *    Starts the operating system execution. All tasks created before the
 *    function call will be executed. The function will return with success
 *    when osStop is called. Before calling osStart, osInit must be
 *    executed.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osStart(void)
{
  /* Operating system is already running */
  if(osCurrentTask)
  {
    osSetLastError(ERR_OS_ALREADY_RUNNING);
    return FALSE;
  }

  /* Start operating system */
  #if (OS_STOP_FUNC)
    osRestoreCallerAndStop = FALSE;
    osSaveCallerAndStart = TRUE;
  #else
    osCurrentTask = (struct TTask FAR *) stPQueueGet(&osTaskPQueue);
  #endif

  /* Execute task scheduler to start operating system immediately */
  osYield();
  return TRUE;
}


/***************************************************************************/
#if (OS_STOP_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osStop
 *
 *  Description:
 *    Stops the operating system execution. It does not terminate any task
 *    and does not release any system resources. This operation returns to
 *    the osStart function execution point. osStart returns with TRUE
 *    (successful operating system execution). To continue execution of
 *    the operating system, call osStart again. To permanently exit and
 *    release system resources (all created tasks and objects), use
 *    osDeinit when the operating system is stopped.
 *
 ***************************************************************************/

void osStop(void)
{
  /* Stop the operating system execution */
  osSaveCallerAndStart = FALSE;
  osRestoreCallerAndStop = TRUE;

  /* Execute task scheduler to restore the caller */
  osYield();
}


/***************************************************************************/
#endif /* OS_STOP_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_GET_SYSTEM_STAT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetSystemStat
 *
 *  Description:
 *    Returns current CPU usage. This is the total CPU time minus the idle
 *    task CPU usage. The percentage of CPU usage can be calculated from
 *    the formula: 100 * CPUTime / TotalTime.
 *
 *  Parameters:
 *    CPUTime - Pointer to variable that receives CPU usage.
 *    TotalTime - Pointer to variable that receives total CPU time.
 *
 ***************************************************************************/

void osGetSystemStat(INDEX *CPUTime, INDEX *TotalTime)
{
  /* Get system usage */
  if(osIdleTask->CPUUsageTime == osCPUUsageTime)
    *CPUTime = osCPUUsage - osIdleTask->CPUUsage;
  else if(osIdleTask->CPUCalcTime == osCPUUsageTime)
    *CPUTime = osCPUUsage - osIdleTask->CPUCalc;
  else
    *CPUTime = osCPUUsage;
  *TotalTime = osCPUUsage;
}


/***************************************************************************/
#endif /* OS_GET_SYSTEM_STAT_FUNC */
/***************************************************************************/


/***************************************************************************/

