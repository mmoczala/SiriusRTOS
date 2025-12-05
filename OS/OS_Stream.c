/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Stream.c - Stream object management functions
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
#if (OS_USE_STREAM)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Mutex protection flag */
#if (OS_STREAM_PROTECT_MUTEX)
  #define OS_STREAM_MODE_MASK_0         (OS_IPC_PROTECT_MUTEX)
#else
  #define OS_STREAM_MODE_MASK_0         0x00
#endif

/* Event protection flag */
#if (OS_STREAM_PROTECT_EVENT)
  #define OS_STREAM_MODE_MASK_1         ((OS_STREAM_MODE_MASK_0) | \
                                         (OS_IPC_PROTECT_EVENT))
#else
  #define OS_STREAM_MODE_MASK_1         (OS_STREAM_MODE_MASK_0)
#endif

/* Wait on read completion flag */
#if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)
  #define OS_STREAM_MODE_MASK_2         ((OS_STREAM_MODE_MASK_1) | \
                                         (OS_IPC_WAIT_IF_EMPTY))
#else
  #define OS_STREAM_MODE_MASK_2         (OS_STREAM_MODE_MASK_1)
#endif

/* Wait on write completion flag */
#if (OS_STREAM_ALLOW_WAIT_IF_FULL)
  #define OS_STREAM_MODE_MASK_3         ((OS_STREAM_MODE_MASK_2) | \
                                         (OS_IPC_WAIT_IF_FULL))
#else
  #define OS_STREAM_MODE_MASK_3         (OS_STREAM_MODE_MASK_2)
#endif

/* Direct read-write flag */
#if (OS_STREAM_ALLOW_DIRECT_RW)
  #define OS_STREAM_MODE_MASK_4         ((OS_STREAM_MODE_MASK_3) | \
                                         (OS_IPC_DIRECT_READ_WRITE))
#else
  #define OS_STREAM_MODE_MASK_4         (OS_STREAM_MODE_MASK_3)
#endif

/* Available mode flags for the stream object */
#define OS_STREAM_MODE_MASK             (OS_STREAM_MODE_MASK_4)


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Stream object descriptor */
struct TStreamObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_STREAM_FUNC)
    struct TObjectName Name;
  #endif

  /* Mode flags */
  UINT8 Mode;

  /* Stream configuration */
  SIZE BufferSize;
  SIZE Offset;
  SIZE Length;

  /* Stream access synchronization */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    struct TSignal WrSync;
    struct TSignal RdSync;
  #endif

  #if (OS_STREAM_PROTECT_MUTEX)
    struct TCriticalSection WrCS;
    struct TCriticalSection RdCS;
  #endif

  /* Waiting for incoming data */
  #if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)
    struct TSignal SyncOnEmpty;
  #endif

  /* Waiting for buffer space */
  #if (OS_STREAM_ALLOW_WAIT_IF_FULL)
    struct TSignal SyncOnFull;
  #endif
};


/***************************************************************************/
#if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osStreamLock
 *
 *  Description:
 *    Locks the specified stream object for read or write.
 *
 *  Parameters:
 *    StreamObject - Pointer to the stream object.
 *    Signal - Signal to lock.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

static BOOL osStreamLock(struct TStreamObject FAR *StreamObject,
  struct TSignal FAR *Signal, TIME Timeout)
{
  #if (OS_STREAM_PROTECT_MUTEX)
    ERROR LastErrorCode;
  #endif

  /* Lock the stream */
  switch(StreamObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Wait for event */
    #if (OS_STREAM_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        return osWaitFor(Signal, Timeout);
    #endif

    /* Wait for mutex */
    #if (OS_STREAM_PROTECT_MUTEX)
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
 *    osStreamUnlock
 *
 *  Description:
 *    Unlocks the specified stream object.
 *
 *  Parameters:
 *    StreamObject - Pointer to the stream object.
 *    Signal - Signal to unlock.
 *
 ***************************************************************************/

static void osStreamUnlock(struct TStreamObject FAR *StreamObject,
  struct TSignal FAR *Signal)
{
  /* Unlock the stream */
  switch(StreamObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Release event */
    #if (OS_STREAM_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        osUpdateSignalState(Signal, TRUE);
        break;
    #endif

    /* Release mutex */
    #if (OS_STREAM_PROTECT_MUTEX)
      case OS_IPC_PROTECT_MUTEX:
        osReleaseCS(Signal->CS, osCurrentTask, 1, NULL);
        break;
    #endif
  }
}


/***************************************************************************/
#endif /* OS_STREAM_PROTECT_EVENT || OS_STREAM_PROTECT_MUTEX */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osStreamWrite
 *
 *  Description:
 *    Writes data to the specified stream.
 *
 *  Parameters:
 *    StreamObject - Pointer to the stream object.
 *    Buffer - Pointer to the buffer containing data to write.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully sent, or zero on failure.
 *    If the function writes less than the specified number of bytes and
 *    wait for write completion is disabled, it sets the last error code
 *    to ERR_STREAM_IS_FULL.
 *
 ***************************************************************************/

static SIZE osStreamWrite(struct TStreamObject FAR *StreamObject,
  PVOID Buffer, SIZE Size, TIME Timeout)
{
  BOOL PrevLockState;
  SIZE StreamDescSize, NumBytesWritten, BytesToCopy, DataOffset;

  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  #if (OS_STREAM_ALLOW_DIRECT_RW)
    struct TTask FAR *Task;
    struct TWaitAssoc FAR *WaitAssoc;
  #endif

  /* Mark unused parameter */
  #if (!((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX) \
    || (OS_STREAM_ALLOW_WAIT_IF_FULL)))
    AR_UNUSED_PARAM(Timeout);
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Get size of the stream object descriptor */
  StreamDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TStreamObject));

  /* Determine the protection method */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((StreamObject->Mode & OS_IPC_PROTECTION_MASK) ==
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
      if(!osStreamLock(StreamObject, &StreamObject->WrSync, Timeout))
        return 0;
    }
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* Data transmission */
  NumBytesWritten = 0;
  while(Size > 0)
  {
    /* Direct read-write when some task is waiting for read completion */
    #if (OS_STREAM_ALLOW_DIRECT_RW)
      if(StreamObject->Mode & OS_IPC_DIRECT_READ_WRITE)
      {
        /* Get first waiting task */
        WaitAssoc = (struct TWaitAssoc FAR *)
          stBSTreeGetFirst(&StreamObject->SyncOnEmpty.WaitingTasks);
        if(WaitAssoc)
        {
          Task = WaitAssoc->Task;

          /* Block a task for IPC operation and make it not waiting. The
             task is blocked, so during osTerminateTask call for current
             task, the WaitExitCode may be set. The blocked task will then
             be resumed and finish the wait function with the specified
             error code. */
          Task->BlockingFlags |= OS_BLOCK_FLAG_IPC;
          osCurrentTask->IPCBlockingTask = Task;
          osMakeNotWaiting(Task);

          /* Leave critical section */
          if(!ProtectByInt)
            arRestore(PrevLockState);

          /* Determine the data size */
          BytesToCopy = Task->IPCSize;
          if(BytesToCopy > Size)
            BytesToCopy = Size;

          /* Copy data directly to waiting task buffer */
          stMemCpy(Task->IPCBuffer, &((UINT8 FAR *) Buffer)[NumBytesWritten],
            BytesToCopy);

          /* Number of written bytes */
          NumBytesWritten += BytesToCopy;
          Size -= BytesToCopy;

          /* Enter critical section */
          if(!ProtectByInt)
            PrevLockState = arLock();

          /* Resume blocked task */
          Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_IPC;
          Task->IPCDRWCompletion = TRUE;
          Task->IPCSize = BytesToCopy;
          osCurrentTask->IPCBlockingTask = NULL;
          osMakeReady(Task);

          /* Continue data transmission */
          continue;
        }
      }
    #endif

    /* When stream buffer is full */
    if(StreamObject->Length >= StreamObject->BufferSize)
    {
      /* Waiting for buffer */
      #if (OS_STREAM_ALLOW_WAIT_IF_FULL)

        /* Exit when not enabled or called not by task */
        if(!(StreamObject->Mode & OS_IPC_WAIT_IF_FULL) ||
          !osCurrentTask || osInISR)
        {
          osSetLastError(ERR_STREAM_IS_FULL);
          break;
        }

        /* Setup for direct read-write */
        #if (OS_STREAM_ALLOW_DIRECT_RW)
          osCurrentTask->IPCDRWCompletion = FALSE;
          osCurrentTask->IPCBuffer =
            &((UINT8 FAR *) Buffer)[NumBytesWritten];
          osCurrentTask->IPCSize = Size;
        #endif

        /* Wait when buffer is full */
        if(!osWaitFor(&StreamObject->SyncOnFull, Timeout))
          break;

        /* Exit on direct read-write operation completion */
        #if (OS_STREAM_ALLOW_DIRECT_RW)
          if(StreamObject->Mode & OS_IPC_DIRECT_READ_WRITE)
            if(osCurrentTask->IPCDRWCompletion)
            {
              /* Number of written bytes */
              NumBytesWritten += osCurrentTask->IPCSize;
              Size -= osCurrentTask->IPCSize;

              /* Continue data transmission */
              continue;
            }
        #endif

      /* Exit when buffer is full */
      #else
        osSetLastError(ERR_STREAM_IS_FULL);
        break;
      #endif
    }

    /* Calculate data offset */
    DataOffset = StreamObject->BufferSize - StreamObject->Offset;
    if(DataOffset > StreamObject->Length)
      DataOffset = StreamObject->Offset + StreamObject->Length;
    else
      DataOffset = StreamObject->Length - DataOffset;

    /* Calculate number of bytes to copy */
    BytesToCopy = StreamObject->BufferSize - DataOffset;
    if(BytesToCopy > (StreamObject->BufferSize - StreamObject->Length))
      BytesToCopy = StreamObject->BufferSize - StreamObject->Length;
    if(BytesToCopy > Size)
      BytesToCopy = Size;

    /* Leave critical section */
    if(!ProtectByInt)
      arRestore(PrevLockState);

    /* Copy data */
    stMemCpy(&((UINT8 FAR *) StreamObject)[StreamDescSize + DataOffset],
      &((UINT8 FAR *) Buffer)[NumBytesWritten], BytesToCopy);

    /* Number of written bytes */
    NumBytesWritten += BytesToCopy;
    Size -= BytesToCopy;

    /* Enter critical section */
    if(!ProtectByInt)
      PrevLockState = arLock();

    /* Number of bytes stored in the stream buffer */
    StreamObject->Length += BytesToCopy;

    /* Update main signal */
    osUpdateSignalState(&StreamObject->Object.Signal,
      (BOOL) (StreamObject->Length > 0));

    #if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)
      if(StreamObject->Mode & OS_IPC_WAIT_IF_EMPTY)
        osUpdateSignalState(&StreamObject->SyncOnEmpty,
          (BOOL) (StreamObject->Length > 0));
    #endif

    #if (OS_STREAM_ALLOW_WAIT_IF_FULL)
      if(StreamObject->Mode & OS_IPC_WAIT_IF_FULL)
        osUpdateSignalState(&StreamObject->SyncOnFull,
          (BOOL) (StreamObject->Length < StreamObject->BufferSize));
    #endif
  }

  /* Release mutex or auto-reset event */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    if(!ProtectByInt)
      osStreamUnlock(StreamObject, &StreamObject->WrSync);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return number of successfully transmitted bytes */
  return NumBytesWritten;
}


/****************************************************************************
 *
 *  Name:
 *    osStreamRead
 *
 *  Description:
 *    Reads data from the specified stream.
 *
 *  Parameters:
 *    StreamObject - Pointer to the stream object.
 *    Buffer - Pointer to the buffer that obtains data.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully received, or zero on failure.
 *    If the function reads less than the specified number of bytes and
 *    wait for read completion is disabled, it sets the last error code
 *    to ERR_STREAM_IS_EMPTY.
 *
 ***************************************************************************/

static SIZE osStreamRead(struct TStreamObject FAR *StreamObject,
  PVOID *Buffer, SIZE Size, TIME Timeout)
{
  BOOL PrevLockState;
  SIZE StreamDescSize, NumBytesRead, BytesToCopy, DataOffset;

  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  #if (OS_STREAM_ALLOW_DIRECT_RW)
    struct TTask FAR *Task;
    struct TWaitAssoc FAR *WaitAssoc;
  #endif

  /* Mark unused parameter */
  #if (!((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX) \
    || (OS_STREAM_ALLOW_WAIT_IF_EMPTY)))
    AR_UNUSED_PARAM(Timeout);
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Get size of the stream object descriptor */
  StreamDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TStreamObject));

  /* Determine the protection method */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    ProtectByInt = (BOOL) ((StreamObject->Mode & OS_IPC_PROTECTION_MASK) ==
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
      if(!osStreamLock(StreamObject, &StreamObject->RdSync, Timeout))
        return 0;
    }
  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* Data transmission */
  NumBytesRead = 0;
  while(Size > 0)
  {
    /* Direct read-write when some task is waiting for write completion */
    #if (OS_STREAM_ALLOW_DIRECT_RW)
      if(!StreamObject->Length &&
        (StreamObject->Mode & OS_IPC_DIRECT_READ_WRITE))
      {
        /* Get first waiting task */
        WaitAssoc = (struct TWaitAssoc FAR *)
          stBSTreeGetFirst(&StreamObject->SyncOnFull.WaitingTasks);
        if(WaitAssoc)
        {
          Task = WaitAssoc->Task;

          /* Block a task for IPC operation and make it not waiting. The
             task is blocked, so during osTerminateTask call for current
             task, the WaitExitCode may be set. The blocked task will then
             be resumed and finish the wait function with the specified
             error code. */
          Task->BlockingFlags |= OS_BLOCK_FLAG_IPC;
          osCurrentTask->IPCBlockingTask = Task;
          osMakeNotWaiting(Task);

          /* Leave critical section */
          if(!ProtectByInt)
            arRestore(PrevLockState);

          /* Determine the data size */
          BytesToCopy = Task->IPCSize;
          if(BytesToCopy > Size)
            BytesToCopy = Size;

          /* Copy data directly to waiting task buffer */
          stMemCpy(&((UINT8 FAR *) Buffer)[NumBytesRead], Task->IPCBuffer,
            BytesToCopy);

          /* Number of read bytes */
          NumBytesRead += BytesToCopy;
          Size -= BytesToCopy;

          /* Enter critical section */
          if(!ProtectByInt)
            PrevLockState = arLock();

          /* Resume blocked task */
          Task->BlockingFlags &= (UINT8) ~OS_BLOCK_FLAG_IPC;
          Task->IPCDRWCompletion = TRUE;
          Task->IPCSize = BytesToCopy;
          osCurrentTask->IPCBlockingTask = NULL;
          osMakeReady(Task);

          /* Continue data transmission */
          continue;
        }
      }
    #endif

    /* When stream buffer is empty */
    if(!StreamObject->Length)
    {
      /* Waiting for buffer */
      #if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)

        /* Exit when not enabled or called not by task */
        if(!(StreamObject->Mode & OS_IPC_WAIT_IF_EMPTY) ||
          !osCurrentTask || osInISR)
        {
          osSetLastError(ERR_STREAM_IS_EMPTY);
          break;
        }

        /* Setup for direct read-write */
        #if (OS_STREAM_ALLOW_DIRECT_RW)
          osCurrentTask->IPCDRWCompletion = FALSE;
          osCurrentTask->IPCBuffer =
            &((UINT8 FAR *) Buffer)[NumBytesRead];
          osCurrentTask->IPCSize = Size;
        #endif

        /* Wait when buffer is empty */
        if(!osWaitFor(&StreamObject->SyncOnEmpty, Timeout))
          break;

        /* Exit on direct read-write operation completion */
        #if (OS_STREAM_ALLOW_DIRECT_RW)
          if(StreamObject->Mode & OS_IPC_DIRECT_READ_WRITE)
            if(osCurrentTask->IPCDRWCompletion)
            {
              /* Number of written bytes */
              NumBytesRead += osCurrentTask->IPCSize;
              Size -= osCurrentTask->IPCSize;

              /* Continue data transmission */
              continue;
            }
        #endif

      /* Exit when buffer is empty */
      #else
        osSetLastError(ERR_STREAM_IS_EMPTY);
        break;
      #endif
    }

    /* Calculate data offset */
    DataOffset = StreamObject->Offset;

    /* Calculate number of bytes to copy */
    BytesToCopy = StreamObject->BufferSize - DataOffset;
    if(BytesToCopy > StreamObject->Length)
      BytesToCopy = StreamObject->Length;
    if(BytesToCopy > Size)
      BytesToCopy = Size;

    /* Leave critical section */
    if(!ProtectByInt)
      arRestore(PrevLockState);

    /* Copy data */
    stMemCpy(&((UINT8 FAR *) Buffer)[NumBytesRead],
      &((UINT8 FAR *) StreamObject)[StreamDescSize + DataOffset],
      BytesToCopy);

    /* Number of written bytes */
    NumBytesRead += BytesToCopy;
    Size -= BytesToCopy;

    /* Enter critical section */
    if(!ProtectByInt)
      PrevLockState = arLock();

    /* Number of bytes stored in the stream buffer */
    StreamObject->Length -= BytesToCopy;
    StreamObject->Offset =
      (StreamObject->Offset + BytesToCopy) % StreamObject->BufferSize;

    /* Update main signal */
    osUpdateSignalState(&StreamObject->Object.Signal,
      (BOOL) (StreamObject->Length > 0));

    #if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)
      if(StreamObject->Mode & OS_IPC_WAIT_IF_EMPTY)
        osUpdateSignalState(&StreamObject->SyncOnEmpty,
          (BOOL) (StreamObject->Length > 0));
    #endif

    #if (OS_STREAM_ALLOW_WAIT_IF_FULL)
      if(StreamObject->Mode & OS_IPC_WAIT_IF_FULL)
        osUpdateSignalState(&StreamObject->SyncOnFull,
          (BOOL) (StreamObject->Length < StreamObject->BufferSize));
    #endif
  }

  /* Release mutex or auto-reset event */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    if(!ProtectByInt)
      osStreamUnlock(StreamObject, &StreamObject->RdSync);
  #endif

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return number of successfully transmitted bytes */
  return NumBytesRead;
}


/****************************************************************************
 *
 *  Name:
 *    osStreamIOCtrl
 *
 *  Description:
 *    Processes device IO control codes for stream objects.
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

static INDEX osStreamIOCtrl(struct TSysObject FAR *Object,
  INDEX ControlCode, PVOID Buffer, SIZE BufferSize,
  struct TIORequest *IORequest)
{
  struct TStreamObject FAR *StreamObject;
  SIZE BytesTransferred;

  /* Obtain stream descriptor */
  StreamObject = (struct TStreamObject FAR *) Object->ObjectDesc;

  /* Execute specific operation */
  switch(ControlCode)
  {
    /* Read from the stream */
    case DEV_IO_CTL_READ:
      BytesTransferred = osStreamRead(StreamObject, Buffer, BufferSize,
        IORequest ? IORequest->Timeout : OS_INFINITE);
      if(IORequest)
        IORequest->NumberOfBytesTransferred = BytesTransferred;
      return BytesTransferred != 0;

    /* Write to the stream */
    case DEV_IO_CTL_WRITE:
      BytesTransferred = osStreamWrite(StreamObject, Buffer, BufferSize,
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
 *    osCreateStream
 *
 *  Description:
 *    Creates a stream object.
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
 *   BufferSize - Size of the Stream buffer.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateStream(SYSNAME Name, UINT8 Mode, SIZE BufferSize)
{
  struct TStreamObject FAR *StreamObject;
  struct TSysObject FAR *Object;
  SIZE StreamDescSize;
  BOOL InvalidParam;

  /* Get size of the stream object descriptor */
  StreamDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TStreamObject));

  /* Check parameters */
  InvalidParam = (Mode & ((UINT8) ~OS_STREAM_MODE_MASK)) ||
    (BufferSize > (((SIZE) (-1)) - StreamDescSize));

  /* Direct read-write feature (can be used only with OS_IPC_WAIT_IF_EMPTY
     and/or OS_IPC_WAIT_IF_FULL flag) */
  #if (OS_STREAM_ALLOW_DIRECT_RW)
    if(Mode & OS_IPC_DIRECT_READ_WRITE)
      if(!(Mode & (OS_IPC_WAIT_IF_EMPTY | OS_IPC_WAIT_IF_FULL)))
        InvalidParam = TRUE;
  #endif

  /* BufferSize must be higher than zero when direct read-write is not
     enabled */
  #if (OS_STREAM_ALLOW_DIRECT_RW)
    if(!(Mode & OS_IPC_DIRECT_READ_WRITE) && !BufferSize)
      InvalidParam = TRUE;
  #else
    if(!BufferSize)
      InvalidParam = TRUE;
  #endif

  /* Check the protection method */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    if((Mode & OS_IPC_PROTECTION_MASK) == OS_IPC_PROTECTION_MASK)
      InvalidParam = TRUE;
  #endif

  /* Return when some parameter is invalid */
  if(InvalidParam)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object */
  StreamObject = (struct TStreamObject FAR *)
    osMemAlloc(StreamDescSize + BufferSize);
  if(!StreamObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &StreamObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) StreamObject, Object, OS_OBJECT_TYPE_STREAM))
  {
    osMemFree(StreamObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_STREAM_FUNC)
    if(!osRegisterName(Object, &StreamObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the stream */
  Object->Signal.Signaled = 0;
  Object->Flags |= OS_OBJECT_FLAG_USES_IO_DEINIT;
  Object->DeviceIOCtrl = osStreamIOCtrl;
  StreamObject->Mode = Mode;
  StreamObject->BufferSize = BufferSize;
  StreamObject->Offset = 0;
  StreamObject->Length = 0;

  /* Setup the auto-reset event / mutex for protection */
  #if ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX))
    if((Mode & OS_IPC_PROTECTION_MASK) != OS_IPC_PROTECT_INT_CTRL)
    {
      /* Setup signal for write protection */
      StreamObject->WrSync.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      StreamObject->WrSync.Signaled = (INDEX) TRUE;
      stBSTreeInit(&StreamObject->WrSync.WaitingTasks, osWaitAssocCmp);

      /* Setup signal for read protection */
      StreamObject->RdSync.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      StreamObject->RdSync.Signaled = (INDEX) TRUE;
      stBSTreeInit(&StreamObject->RdSync.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        StreamObject->WrSync.CS = NULL;
        StreamObject->RdSync.CS = NULL;
      #endif

      /* Multiple signals associated with object */
      #if (OS_ALLOW_OBJECT_DELETION)
        Object->Signal.NextSignal = &StreamObject->WrSync;
        StreamObject->WrSync.NextSignal = &StreamObject->RdSync;
        StreamObject->RdSync.NextSignal = NULL;
      #endif
    }
  #endif

  /* Register the mutex for protection */
  #if (OS_STREAM_PROTECT_MUTEX)
    if((Mode & OS_IPC_PROTECTION_MASK) == OS_IPC_PROTECT_MUTEX)
    {
      osRegisterCS(&StreamObject->WrSync, &StreamObject->WrCS, 1, 1, TRUE);
      osRegisterCS(&StreamObject->RdSync, &StreamObject->RdCS, 1, 1, TRUE);
    }
  #endif

  /* Setup signal used for waiting for incoming data */
  #if (OS_STREAM_ALLOW_WAIT_IF_EMPTY)
    if(Mode & OS_IPC_WAIT_IF_EMPTY)
    {
      StreamObject->SyncOnEmpty.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      StreamObject->SyncOnEmpty.Signaled = (INDEX) FALSE;
      stBSTreeInit(&StreamObject->SyncOnEmpty.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        StreamObject->SyncOnEmpty.CS = NULL;
      #endif

      /* Multiple signals associated with the object */
      #if (OS_ALLOW_OBJECT_DELETION)
        StreamObject->SyncOnEmpty.NextSignal = Object->Signal.NextSignal;
        Object->Signal.NextSignal = &StreamObject->SyncOnEmpty;
      #endif
    }
  #endif

  /* Setup signal used for waiting for outgoing data */
  #if (OS_STREAM_ALLOW_WAIT_IF_FULL)
    if(Mode & OS_IPC_WAIT_IF_FULL)
    {
      StreamObject->SyncOnFull.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      StreamObject->SyncOnFull.Signaled = (INDEX) TRUE;
      stBSTreeInit(&StreamObject->SyncOnFull.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        StreamObject->SyncOnFull.CS = NULL;
      #endif

      /* Multiple signals associated with the object */
      #if (OS_ALLOW_OBJECT_DELETION)
        StreamObject->SyncOnFull.NextSignal = Object->Signal.NextSignal;
        Object->Signal.NextSignal = &StreamObject->SyncOnFull;
      #endif
    }
  #endif

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_STREAM_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenStream
 *
 *  Description:
 *    Opens an existing stream object by name.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenStream(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_STREAM);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_STREAM_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* OS_USE_STREAM */
/***************************************************************************/


/***************************************************************************/
