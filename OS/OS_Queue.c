/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Queue.c - Queue object management functions
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


/***************************************************************************/
#if (OS_USE_QUEUE)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Mutex protection flag */
#if (OS_QUEUE_PROTECT_MUTEX)
  #define OS_QUEUE_MODE_MASK_0          (OS_IPC_PROTECT_MUTEX)
#else
  #define OS_QUEUE_MODE_MASK_0          0x00
#endif

/* Event protection flag */
#if (OS_QUEUE_PROTECT_EVENT)
  #define OS_QUEUE_MODE_MASK_1          ((OS_QUEUE_MODE_MASK_0) | \
                                        (OS_IPC_PROTECT_EVENT))
#else
  #define OS_QUEUE_MODE_MASK_1          (OS_QUEUE_MODE_MASK_0)
#endif

/* Wait on read completion flag */
#if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
  #define OS_QUEUE_MODE_MASK_2          ((OS_QUEUE_MODE_MASK_1) | \
                                        (OS_IPC_WAIT_IF_EMPTY))
#else
  #define OS_QUEUE_MODE_MASK_2          (OS_QUEUE_MODE_MASK_1)
#endif

/* Wait on write completion flag */
#if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
  #define OS_QUEUE_MODE_MASK_3          ((OS_QUEUE_MODE_MASK_2) | \
                                        (OS_IPC_WAIT_IF_FULL))
#else
  #define OS_QUEUE_MODE_MASK_3          (OS_QUEUE_MODE_MASK_2)
#endif

/* Direct read-write flag */
#if (OS_QUEUE_ALLOW_DIRECT_RW)
  #define OS_QUEUE_MODE_MASK_4          ((OS_QUEUE_MODE_MASK_3) | \
                                        (OS_IPC_DIRECT_READ_WRITE))
#else
  #define OS_QUEUE_MODE_MASK_4          (OS_QUEUE_MODE_MASK_3)
#endif

/* Available mode flags for the queue object */
#define OS_QUEUE_MODE_MASK              (OS_QUEUE_MODE_MASK_4)


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Queue object descriptor */
struct TQueueObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_QUEUE_FUNC)
    struct TObjectName Name;
  #endif

  /* Mode flags */
  UINT8 Mode;

  /* Queue configuration */
  SIZE MessageSize;
  INDEX MaxCount;
  INDEX Offset;

  /* Queue access synchronization */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    struct TSignal WrSync;
    struct TSignal RdSync;
  #endif

  #if (OS_QUEUE_PROTECT_MUTEX)
    struct TCriticalSection WrCS;
    struct TCriticalSection RdCS;
  #endif

  /* Waiting for incoming data */
  #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
    struct TSignal SyncOnEmpty;
  #endif

  /* Waiting for buffer space */
  #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
    struct TSignal SyncOnFull;
  #endif
};


/***************************************************************************/
#if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osQueueLock
 *
 *  Description:
 *    Locks the specified queue object for read or write.
 *
 *  Parameters:
 *    QueueObject - Pointer to the queue object.
 *    Signal - Signal to lock.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

static BOOL osQueueLock(struct TQueueObject FAR *QueueObject,
  struct TSignal FAR *Signal, TIME Timeout)
{
  #if (OS_QUEUE_PROTECT_MUTEX)
    ERROR LastErrorCode;
  #endif

  /* Lock the queue */
  switch(QueueObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Wait for event */
    #if (OS_QUEUE_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        return osWaitFor(Signal, Timeout);
    #endif

    /* Wait for mutex */
    #if (OS_QUEUE_PROTECT_MUTEX)
      case OS_IPC_PROTECT_MUTEX:

        /* Save last error code */
        LastErrorCode = osGetLastError();

        /* Wait for mutex */
        if(osWaitFor(Signal, Timeout))
          return TRUE;

        /* Treat acquiring abandoned critical section as a valid process.
           Restore previous last error code in this case. */
        else if(osGetLastError() == ERR_WAIT_ABANDONED)
        {
          osSetLastError(LastErrorCode);
          return TRUE;
        }

        /* Failure during acquiring the critical section */
        return FALSE;
    #endif
  }

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osQueueUnlock
 *
 *  Description:
 *    Unlocks the specified queue object.
 *
 *  Parameters:
 *    QueueObject - Pointer to the queue object.
 *    Signal - Signal to unlock.
 *
 ***************************************************************************/

static void osQueueUnlock(struct TQueueObject FAR *QueueObject,
  struct TSignal FAR *Signal)
{
  /* Unlock the queue */
  switch(QueueObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Release event */
    #if (OS_QUEUE_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        osUpdateSignalState(Signal, TRUE);
        break;
    #endif

    /* Release mutex */
    #if (OS_QUEUE_PROTECT_MUTEX)
      case OS_IPC_PROTECT_MUTEX:
        osReleaseCS(Signal->CS, osCurrentTask, 1, NULL);
        break;
    #endif
  }
}


/***************************************************************************/
#endif /* OS_QUEUE_PROTECT_EVENT || OS_QUEUE_PROTECT_MUTEX */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osQueueWrite
 *
 *  Description:
 *    Stores data in the specified queue.
 *
 *  Parameters:
 *    QueueObject - Pointer to the queue object.
 *    Buffer - Pointer to the buffer with data to write.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully sent, or zero on failure.
 *
 ***************************************************************************/

static SIZE osQueueWrite(struct TQueueObject FAR *QueueObject,
  PVOID Buffer, SIZE Size, TIME Timeout)
{
  BOOL PrevLockState, Success;
  SIZE DataOffset;

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
    BOOL WaitOnFullCompleted;
  #endif

  #if (OS_QUEUE_ALLOW_DIRECT_RW)
    struct TTask FAR *Task;
    struct TWaitAssoc FAR *WaitAssoc;
  #endif

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL PrevISRState;
  #endif

  /* Mark unused parameter */
  #if (!((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX) || \
    (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)))
    AR_UNUSED_PARAM(Timeout);
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Determine the protection method */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((QueueObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);
    if(!ProtectByInt)
    {
      /* Operation can be performed only by a task when synchronization
         other than by interrupt disabling is used */
      if(!osCurrentTask || osInISR)
      {
        osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
        return 0;
      }

      /* Wait for write access synchronization */
      if(!osQueueLock(QueueObject, &QueueObject->WrSync, Timeout))
        return 0;
    }
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* This loop is executed once and it is used to simplify code structure */
  Success = FALSE;
  while(1)
  {
    /* Direct read-write when some task is waiting for read completion */
    #if (OS_QUEUE_ALLOW_DIRECT_RW)
      if(QueueObject->Mode & OS_IPC_DIRECT_READ_WRITE)
      {
        /* Get first waiting task */
        WaitAssoc = (struct TWaitAssoc FAR *)
          stBSTreeGetFirst(&QueueObject->SyncOnEmpty.WaitingTasks);
        if(WaitAssoc)
        {
          Task = WaitAssoc->Task;

          /* Block a task for IPC operation and make it not waiting. The
             task is blocked, so during osTerminateTask call for current
             task, the WaitExitCode may be set. The blocked task will
             then be resumed and finish the wait function with the
             specified error code. */
          Task->BlockingFlags |= OS_BLOCK_FLAG_IPC;
          osCurrentTask->IPCBlockingTask = Task;
          osMakeNotWaiting(Task);

          /* Unlock the queue */
          if(!ProtectByInt)
            osQueueUnlock(QueueObject, &QueueObject->WrSync);

          /* Leave critical section */
          arRestore(PrevLockState);

          /* Determine the message size */
          if(Size > Task->IPCSize)
            Size = Task->IPCSize;
          else
            Task->IPCSize = Size;

          /* Copy data directly to waiting task buffer */
          stMemCpy(Task->IPCBuffer, Buffer, Size);

          /* Enter critical section */
          PrevLockState = arLock();

          /* Resume blocked task */
          Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_IPC;
          Task->IPCDRWCompletion = TRUE;
          osCurrentTask->IPCBlockingTask = NULL;
          osMakeReady(Task);

          /* Leave critical section */
          arRestore(PrevLockState);

          /* Return the number of bytes transferred */
          return Size;
        }
      }
    #endif

    /* This variable is used to avoid signal state changing */
    #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
      WaitOnFullCompleted = FALSE;
    #endif

    /* When buffer is full */
    if(QueueObject->Object.Signal.Signaled >= QueueObject->MaxCount)
    {
      /* Waiting for buffer space */
      #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)

        /* Exit when not enabled or called not by task */
        if(!(QueueObject->Mode & OS_IPC_WAIT_IF_FULL) ||
          !osCurrentTask || osInISR)
        {
          osSetLastError(ERR_QUEUE_IS_FULL);
          break;
        }

        /* Setup for direct read-write */
        #if (OS_QUEUE_ALLOW_DIRECT_RW)
          osCurrentTask->IPCDRWCompletion = FALSE;
          osCurrentTask->IPCBuffer = Buffer;
          osCurrentTask->IPCSize = Size;
        #endif

        /* Wait when buffer is full */
        if(!osWaitFor(&QueueObject->SyncOnFull, Timeout))
          break;

        /* Exit on direct read-write operation completion */
        #if (OS_QUEUE_ALLOW_DIRECT_RW)
          if(!QueueObject->MaxCount &&
            (QueueObject->Mode & OS_IPC_DIRECT_READ_WRITE))
            if(osCurrentTask->IPCDRWCompletion)
            {
              Size = osCurrentTask->IPCSize;
              break;
            }
        #endif

        /* Disallow modification of signal state */
        WaitOnFullCompleted = TRUE;

      /* Exit when buffer is full */
      #else
        osSetLastError(ERR_QUEUE_IS_FULL);
        break;
      #endif
    }

    /* Calculate offset in buffer to write data */
    DataOffset = AR_MEMORY_ALIGN_UP(sizeof(struct TQueueObject)) +
      (SIZE) ((QueueObject->Object.Signal.Signaled + QueueObject->Offset) %
      QueueObject->MaxCount) * QueueObject->MessageSize;

    /* Leave critical section */
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        arRestore(PrevLockState);
    #endif

    /* Check maximal size */
    if(Size > QueueObject->MessageSize)
      Size = QueueObject->MessageSize;

    /* Copy data */
    stMemCpy(&((UINT8 FAR *) QueueObject)[DataOffset], Buffer, Size);

    /* Enter critical section */
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        PrevLockState = arLock();
    #endif

    /* Begin delaying scheduler execution */
    #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
      (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      PrevISRState = osEnterISR();
    #endif

    /* Update main signal */
    osUpdateSignalState(&QueueObject->Object.Signal,
      QueueObject->Object.Signal.Signaled + 1);

    #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
      if(QueueObject->Mode & OS_IPC_WAIT_IF_EMPTY)
        osUpdateSignalState(&QueueObject->SyncOnEmpty,
          QueueObject->SyncOnEmpty.Signaled + 1);
    #endif

    #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
      if(!WaitOnFullCompleted)
        if(QueueObject->Mode & OS_IPC_WAIT_IF_FULL)
          osUpdateSignalState(&QueueObject->SyncOnFull,
            QueueObject->SyncOnFull.Signaled - 1);
    #endif

    /* Set success flag to TRUE when code was executed successfully */
    Success = TRUE;
    break;
  }

  /* Release mutex or auto-reset event */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(!ProtectByInt)
      osQueueUnlock(QueueObject, &QueueObject->WrSync);
  #endif

  /* Execute delayed scheduler */
  #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
    (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(Success)
      osLeaveISR(PrevISRState);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return number of successfully transmitted bytes */
  return Success ? Size : 0;
}


/****************************************************************************
 *
 *  Name:
 *    osQueueRead
 *
 *  Description:
 *    Reads data from the specified queue.
 *
 *  Parameters:
 *    QueueObject - Pointer to the queue object.
 *    Buffer - Pointer to the buffer that obtains data.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully received, or zero on failure.
 *
 ***************************************************************************/

static SIZE osQueueRead(struct TQueueObject FAR *QueueObject,
  PVOID *Buffer, SIZE Size, TIME Timeout)
{
  BOOL PrevLockState, Success;
  SIZE DataOffset;

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
    BOOL WaitOnEmptyCompleted;
  #endif

  #if (OS_QUEUE_ALLOW_DIRECT_RW)
    struct TTask FAR *Task;
    struct TWaitAssoc FAR *WaitAssoc;
  #endif

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL PrevISRState;
  #endif

  /* Mark unused parameter */
  #if (!((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX) || \
    (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)))
    AR_UNUSED_PARAM(Timeout);
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Determine the protection method */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((QueueObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);
    if(!ProtectByInt)
    {
      /* Operation can be performed only by a task when synchronization
         other than by interrupt disabling is used */
      if(!osCurrentTask || osInISR)
      {
        osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
        return 0;
      }

      /* Wait for read access synchronization */
      if(!osQueueLock(QueueObject, &QueueObject->RdSync, Timeout))
        return 0;
    }
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* This loop is executed once and it is used to simplify code structure */
  Success = FALSE;
  while(1)
  {
    /* Direct read-write when some task is waiting for write completion */
    #if (OS_QUEUE_ALLOW_DIRECT_RW)
      if(!QueueObject->Object.Signal.Signaled &&
        (QueueObject->Mode & OS_IPC_DIRECT_READ_WRITE))
      {
        /* Get first waiting task */
        WaitAssoc = (struct TWaitAssoc FAR *)
          stBSTreeGetFirst(&QueueObject->SyncOnFull.WaitingTasks);
        if(WaitAssoc)
        {
          Task = WaitAssoc->Task;

          /* Block a task for IPC operation and make it not waiting. The
             task is blocked, so during osTerminateTask call for current
             task, the WaitExitCode may be set. The blocked task will
             then be resumed and finish the wait function with the
             specified error code. */
          Task->BlockingFlags |= OS_BLOCK_FLAG_IPC;
          osCurrentTask->IPCBlockingTask = Task;
          osMakeNotWaiting(Task);

          /* Unlock the queue */
          if(!ProtectByInt)
            osQueueUnlock(QueueObject, &QueueObject->RdSync);

          /* Leave critical section */
          arRestore(PrevLockState);

          /* Determine the message size */
          if(Size > Task->IPCSize)
            Size = Task->IPCSize;
          else
            Task->IPCSize = Size;

          /* Copy data directly from waiting task buffer */
          stMemCpy(Buffer, Task->IPCBuffer, Size);

          /* Enter critical section */
          PrevLockState = arLock();

          /* Resume blocked task */
          Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_IPC;
          Task->IPCDRWCompletion = TRUE;
          osCurrentTask->IPCBlockingTask = NULL;
          osMakeReady(Task);

          /* Leave critical section */
          arRestore(PrevLockState);

          /* Return the number of bytes transferred */
          return Size;
        }
      }
    #endif

    /* This variable is used to avoid signal state changing */
    #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
      WaitOnEmptyCompleted = FALSE;
    #endif

    /* When buffer is empty */
    if(!QueueObject->Object.Signal.Signaled)
    {
      /* Waiting for buffer */
      #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)

        /* Exit when not enabled or called not by task */
        if(!(QueueObject->Mode & OS_IPC_WAIT_IF_EMPTY) ||
          !osCurrentTask || osInISR)
        {
          osSetLastError(ERR_QUEUE_IS_EMPTY);
          break;
        }

        /* Setup for direct read-write */
        #if (OS_QUEUE_ALLOW_DIRECT_RW)
          osCurrentTask->IPCDRWCompletion = FALSE;
          osCurrentTask->IPCBuffer = Buffer;
          osCurrentTask->IPCSize = Size;
        #endif

        /* Wait when buffer is empty */
        if(!osWaitFor(&QueueObject->SyncOnEmpty, Timeout))
          break;

        /* Exit on direct read-write operation completion */
        #if (OS_QUEUE_ALLOW_DIRECT_RW)
          if(QueueObject->Mode & OS_IPC_DIRECT_READ_WRITE)
            if(osCurrentTask->IPCDRWCompletion)
            {
              Size = osCurrentTask->IPCSize;
              break;
            }
        #endif

        /* Disallow modification of signal state */
        WaitOnEmptyCompleted = TRUE;

      /* Exit when buffer is empty */
      #else
        osSetLastError(ERR_QUEUE_IS_EMPTY);
        break;
      #endif
    }

    /* Calculate offset in buffer to read data */
    DataOffset = AR_MEMORY_ALIGN_UP(sizeof(struct TQueueObject)) +
      ((SIZE) QueueObject->Offset) * QueueObject->MessageSize;

    /* Remove message, before interrupts are restored (avoids problem
       occurring during task termination) */
    QueueObject->Offset = (QueueObject->Offset + 1) % QueueObject->MaxCount;

    /* Leave critical section */
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        arRestore(PrevLockState);
    #endif

    /* Check maximal size */
    if(Size > QueueObject->MessageSize)
      Size = QueueObject->MessageSize;

    /* Copy data */
    stMemCpy(Buffer, &((UINT8 FAR *) QueueObject)[DataOffset], Size);

    /* Enter critical section */
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        PrevLockState = arLock();
    #endif

    /* Begin delaying scheduler execution */
    #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
      (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      PrevISRState = osEnterISR();
    #endif

    /* Update main signal */
    osUpdateSignalState(&QueueObject->Object.Signal,
      QueueObject->Object.Signal.Signaled - 1);

    #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
      if(!WaitOnEmptyCompleted)
        if(QueueObject->Mode & OS_IPC_WAIT_IF_EMPTY)
          osUpdateSignalState(&QueueObject->SyncOnEmpty,
            QueueObject->SyncOnEmpty.Signaled - 1);
    #endif

    #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
      if(QueueObject->Mode & OS_IPC_WAIT_IF_FULL)
        osUpdateSignalState(&QueueObject->SyncOnFull,
          QueueObject->SyncOnFull.Signaled + 1);
    #endif

    /* Set success flag to TRUE when code was executed successfully */
    Success = TRUE;
    break;
  }

  /* Release mutex or auto-reset event */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(!ProtectByInt)
      osQueueUnlock(QueueObject, &QueueObject->RdSync);
  #endif

  /* Execute delayed scheduler */
  #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
    (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(Success)
      osLeaveISR(PrevISRState);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return number of successfully transmitted bytes */
  return Success ? Size : 0;
}


/****************************************************************************
 *
 *  Name:
 *    osQueueIOCtrl
 *
 *  Description:
 *    Processes device IO control codes for queue objects.
 *
 *  Parameters:
 *    Object - Pointer to the system object.
 *    ControlCode - Device IO control code.
 *    Buffer - Pointer to the data buffer.
 *    BufferSize - Size of the buffer.
 *    IORequest - Pointer to the structure with additional settings.
 *
 *  Return:
 *    Value specific to the specified device IO control code.
 *
 ***************************************************************************/

static INDEX osQueueIOCtrl(struct TSysObject FAR *Object,
  INDEX ControlCode, PVOID Buffer, SIZE BufferSize,
  struct TIORequest *IORequest)
{
  struct TQueueObject FAR *QueueObject;
  SIZE BytesTransferred;

  /* Obtain queue descriptor */
  QueueObject = (struct TQueueObject FAR *) Object->ObjectDesc;

  /* Execute specific operation */
  switch(ControlCode)
  {
    /* Read from the queue */
    case DEV_IO_CTL_READ:
      BytesTransferred = osQueueRead(QueueObject, Buffer, BufferSize,
        IORequest ? IORequest->Timeout : OS_INFINITE);
      if(IORequest)
        IORequest->NumberOfBytesTransferred = BytesTransferred;
      return BytesTransferred != 0;

    /* Write to the queue */
    case DEV_IO_CTL_WRITE:
      BytesTransferred = osQueueWrite(QueueObject, Buffer, BufferSize,
        IORequest ? IORequest->Timeout : OS_INFINITE);
      if(IORequest)
        IORequest->NumberOfBytesTransferred = BytesTransferred;
      return BytesTransferred != 0;
  }

  /* Not supported device IO control code */
  osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osCreateQueue
 *
 *  Description:
 *    Creates a queue object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    Mode - Mode flags. Specifying values other than those listed below
 *      will cause function failure. Features are available only when
 *      enabled.
 *
 *      Protection Method (select one, defaults to Int Ctrl):
 *      OS_IPC_PROTECT_INT_CTRL - Protection by interrupt disabling.
 *      OS_IPC_PROTECT_EVENT - Protection by auto-reset event.
 *      OS_IPC_PROTECT_MUTEX - Protection by mutex.
 *
 *      Data Transfer Flags:
 *      OS_IPC_WAIT_IF_EMPTY - Enables waiting for read completion.
 *      OS_IPC_WAIT_IF_FULL - Enables waiting for write completion.
 *      OS_IPC_DIRECT_READ_WRITE - Enables direct read-write feature (must
 *      be used with OS_IPC_WAIT_IF_EMPTY and/or OS_IPC_WAIT_IF_FULL).
 *
 *    MaxCount - Maximal number of messages.
 *    MessageSize - Size of the message.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateQueue(SYSNAME Name, UINT8 Mode, INDEX MaxCount,
  SIZE MessageSize)
{
  struct TQueueObject FAR *QueueObject;
  struct TSysObject FAR *Object;
  SIZE QueueDescSize;
  BOOL InvalidParam;

  /* Get size of the queue object descriptor */
  QueueDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TQueueObject));

  /* Check parameters */
  InvalidParam = (Mode & ((UINT8) ~OS_QUEUE_MODE_MASK)) || !MessageSize ||
    (MaxCount > ((((SIZE) (-1)) - QueueDescSize) / MessageSize));

  /* Direct read-write feature (can be used only with OS_IPC_WAIT_IF_EMPTY
     and/or OS_IPC_WAIT_IF_FULL flag) */
  #if (OS_QUEUE_ALLOW_DIRECT_RW)
    if(Mode & OS_IPC_DIRECT_READ_WRITE)
      if(!(Mode & (OS_IPC_WAIT_IF_EMPTY | OS_IPC_WAIT_IF_FULL)))
        InvalidParam = TRUE;
  #endif

  /* MaxCount must be greater than zero when direct read-write is not
     enabled */
  #if (OS_QUEUE_ALLOW_DIRECT_RW)
    if(!(Mode & OS_IPC_DIRECT_READ_WRITE) && !MaxCount)
      InvalidParam = TRUE;
  #else
    if(!MaxCount)
      InvalidParam = TRUE;
  #endif

  /* Check the protection method */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if((Mode & OS_IPC_PROTECTION_MASK) == OS_IPC_PROTECT_MUTEX)
      InvalidParam = TRUE;
  #endif

  /* Return when some parameter is invalid */
  if(InvalidParam)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object */
  QueueObject = (struct TQueueObject FAR *)
    osMemAlloc(QueueDescSize + MaxCount * MessageSize);
  if(!QueueObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &QueueObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) QueueObject, Object, OS_OBJECT_TYPE_QUEUE))
  {
    osMemFree(QueueObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_QUEUE_FUNC)
    if(!osRegisterName(Object, &QueueObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the queue */
  Object->Signal.Signaled = 0;
  Object->Flags |= OS_OBJECT_FLAG_USES_IO_DEINIT;
  Object->DeviceIOCtrl = osQueueIOCtrl;
  QueueObject->Mode = Mode;
  QueueObject->MaxCount = MaxCount;
  QueueObject->MessageSize = MessageSize;

  /* Setup the auto-reset event / mutex for protection */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if((Mode & OS_IPC_PROTECTION_MASK) != OS_IPC_PROTECT_INT_CTRL)
    {
      /* Setup signal for write protection */
      QueueObject->WrSync.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      QueueObject->WrSync.Signaled = (INDEX) TRUE;
      stBSTreeInit(&QueueObject->WrSync.WaitingTasks, osWaitAssocCmp);

      /* Setup signal for read protection */
      QueueObject->RdSync.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      QueueObject->RdSync.Signaled = (INDEX) TRUE;
      stBSTreeInit(&QueueObject->RdSync.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        QueueObject->WrSync.CS = NULL;
        QueueObject->RdSync.CS = NULL;
      #endif

      /* Multiple signals associated with object */
      #if (OS_ALLOW_OBJECT_DELETION)
        Object->Signal.NextSignal = &QueueObject->WrSync;
        QueueObject->WrSync.NextSignal = &QueueObject->RdSync;
        QueueObject->RdSync.NextSignal = NULL;
      #endif
    }
  #endif

  /* Register the mutex for protection */
  #if (OS_QUEUE_PROTECT_MUTEX)
    if((Mode & OS_IPC_PROTECTION_MASK) == OS_IPC_PROTECT_MUTEX)
    {
      osRegisterCS(&QueueObject->WrSync, &QueueObject->WrCS, 1, 1, TRUE);
      osRegisterCS(&QueueObject->RdSync, &QueueObject->RdCS, 1, 1, TRUE);
    }
  #endif

  /* Setup signal used for waiting for incoming data */
  #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
    if(Mode & OS_IPC_WAIT_IF_EMPTY)
    {
      QueueObject->SyncOnEmpty.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      QueueObject->SyncOnEmpty.Signaled = 0;
      stBSTreeInit(&QueueObject->SyncOnEmpty.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        QueueObject->SyncOnEmpty.CS = NULL;
      #endif

      /* Multiple signals associated with the object */
      #if (OS_ALLOW_OBJECT_DELETION)
        QueueObject->SyncOnEmpty.NextSignal = Object->Signal.NextSignal;
        Object->Signal.NextSignal = &QueueObject->SyncOnEmpty;
      #endif
    }
  #endif

  /* Setup signal used for waiting for outgoing data */
  #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
    if(Mode & OS_IPC_WAIT_IF_FULL)
    {
      QueueObject->SyncOnFull.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      QueueObject->SyncOnFull.Signaled = MaxCount;
      stBSTreeInit(&QueueObject->SyncOnFull.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        QueueObject->SyncOnFull.CS = NULL;
      #endif

      /* Multiple signals associated with the object */
      #if (OS_ALLOW_OBJECT_DELETION)
        QueueObject->SyncOnFull.NextSignal = Object->Signal.NextSignal;
        Object->Signal.NextSignal = &QueueObject->SyncOnFull;
      #endif
    }
  #endif

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_QUEUE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenQueue
 *
 *  Description:
 *    Opens an existing queue object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenQueue(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_QUEUE);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_QUEUE_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_QUEUE_POST_PEND_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osQueuePost
 *
 *  Description:
 *    Appends data to the specified queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Buffer - Pointer to the buffer with data.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osQueuePost(HANDLE Handle, PVOID Buffer)
{
  struct TSysObject FAR *Object;
  struct TQueueObject FAR *QueueObject;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_QUEUE);
  if(!Object)
    return FALSE;

  /* Write data to queue */
  QueueObject = (struct TQueueObject FAR *) Object->ObjectDesc;
  return osQueueWrite(QueueObject, Buffer, QueueObject->MessageSize,
    OS_INFINITE) != 0;
}


/****************************************************************************
 *
 *  Name:
 *    osQueuePend
 *
 *  Description:
 *    Reads and then removes data from the specified queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Buffer - Pointer to the buffer for data.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osQueuePend(HANDLE Handle, PVOID Buffer)
{
  struct TSysObject FAR *Object;
  struct TQueueObject FAR *QueueObject;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_QUEUE);
  if(!Object)
    return FALSE;

  /* Write data to queue */
  QueueObject = (struct TQueueObject FAR *) Object->ObjectDesc;
  return osQueueRead(QueueObject, Buffer, QueueObject->MessageSize,
    OS_INFINITE) != 0;
}


/***************************************************************************/
#endif /* OS_QUEUE_POST_PEND_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_QUEUE_PEEK_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osQueuePeek
 *
 *  Description:
 *    Reads data from the specified queue but does not remove it.
 *    If data is not in the queue, the function returns an error.
 *    Function fails when direct read-write is enabled and the maximal
 *    number of messages is set to 0.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Buffer - Pointer to the buffer for data.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osQueuePeek(HANDLE Handle, PVOID Buffer)
{
  struct TSysObject FAR *Object;
  struct TQueueObject FAR *QueueObject;
  BOOL PrevLockState;
  SIZE DataOffset;

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_QUEUE);
  if(!Object)
    return FALSE;

  /* Get queue object pointer */
  QueueObject = (struct TQueueObject FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Return if queue is empty */
  if(!Object->Signal.Signaled)
  {
    arRestore(PrevLockState);
    osSetLastError(ERR_QUEUE_IS_EMPTY);
    return FALSE;
  }

  /* Determine the protection method */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((QueueObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);
    if(!ProtectByInt)
    {
      /* Operation can be performed only by a task when synchronization
         other than by interrupt disabling is used */
      if(!osCurrentTask || osInISR)
      {
        osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
        return FALSE;
      }

      /* Wait for read access synchronization */
      if(!osQueueLock(QueueObject, &QueueObject->RdSync, OS_INFINITE))
        return FALSE;
    }
  #endif

  /* Return if queue is empty */
  if(!Object->Signal.Signaled)
  {
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        osQueueUnlock(QueueObject, &QueueObject->RdSync);
    #endif

    arRestore(PrevLockState);
    osSetLastError(ERR_QUEUE_IS_EMPTY);
    return FALSE;
  }

  /* Leave critical section */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(!ProtectByInt)
      arRestore(PrevLockState);
  #endif

  /* Calculate offset in buffer to read data */
  DataOffset = AR_MEMORY_ALIGN_UP(sizeof(struct TQueueObject)) +
    ((SIZE) QueueObject->Offset) * QueueObject->MessageSize;

  /* Copy data */
  stMemCpy(Buffer, &((UINT8 FAR *) QueueObject)[DataOffset],
    QueueObject->MessageSize);

  /* Leave critical section */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(ProtectByInt)
      arRestore(PrevLockState);
    else
      osQueueUnlock(QueueObject, &QueueObject->RdSync);
  #else
    arRestore(PrevLockState);
  #endif

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_QUEUE_PEEK_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_QUEUE_CLEAR_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osClearQueue
 *
 *  Description:
 *    Clears the specified queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osClearQueue(HANDLE Handle)
{
  struct TSysObject FAR *Object;
  struct TQueueObject FAR *QueueObject;
  BOOL PrevLockState;

  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
    (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    BOOL PrevISRState;
  #endif

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_QUEUE);
  if(!Object)
    return FALSE;

  /* Get queue object pointer */
  QueueObject = (struct TQueueObject FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Return if queue is empty */
  if(!Object->Signal.Signaled)
  {
    arRestore(PrevLockState);
    osSetLastError(ERR_QUEUE_IS_EMPTY);
    return FALSE;
  }

  /* Determine the protection method */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((QueueObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);
    if(!ProtectByInt)
    {
      /* Operation can be performed only by a task when synchronization
         other than by interrupt disabling is used */
      if(!osCurrentTask || osInISR)
      {
        osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
        return FALSE;
      }

      /* Wait for read access synchronization */
      if(!osQueueLock(QueueObject, &QueueObject->RdSync, OS_INFINITE))
        return FALSE;
    }
  #endif

  /* Return if queue is empty */
  if(!Object->Signal.Signaled)
  {
    #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
      if(!ProtectByInt)
        osQueueUnlock(QueueObject, &QueueObject->RdSync);
    #endif

    arRestore(PrevLockState);
    osSetLastError(ERR_QUEUE_IS_EMPTY);
    return FALSE;
  }

  /* Remove all messages */
  QueueObject->Offset = (QueueObject->Offset + Object->Signal.Signaled) %
    QueueObject->MaxCount;

  /* Begin delaying scheduler execution */
  #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
    (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    PrevISRState = osEnterISR();
  #endif

  /* Update main signal */
  osUpdateSignalState(&QueueObject->Object.Signal, 0);

  #if (OS_QUEUE_ALLOW_WAIT_IF_EMPTY)
    if(QueueObject->Mode & OS_IPC_WAIT_IF_EMPTY)
      osUpdateSignalState(&QueueObject->SyncOnEmpty, 0);
  #endif

  #if (OS_QUEUE_ALLOW_WAIT_IF_FULL)
    if(QueueObject->Mode & OS_IPC_WAIT_IF_FULL)
      osUpdateSignalState(&QueueObject->SyncOnFull, QueueObject->MaxCount);
  #endif

  /* Release mutex or auto-reset event */
  #if ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    if(!ProtectByInt)
      osQueueUnlock(QueueObject, &QueueObject->RdSync);
  #endif

  /* Execute delayed scheduler */
  #if ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL) || \
    (OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX))
    osLeaveISR(PrevISRState);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_QUEUE_CLEAR_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* OS_USE_QUEUE */
/***************************************************************************/


/***************************************************************************/
