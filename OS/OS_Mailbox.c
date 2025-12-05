/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Mailbox.c - Mailbox object management functions
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
#if (OS_USE_MAILBOX)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Mutex protection flag */
#if (OS_MBOX_PROTECT_MUTEX)
  #define OS_MBOX_MODE_MASK_0           (OS_IPC_PROTECT_MUTEX)
#else
  #define OS_MBOX_MODE_MASK_0           0x00
#endif

/* Event protection flag */
#if (OS_MBOX_PROTECT_EVENT)
  #define OS_MBOX_MODE_MASK_1           ((OS_MBOX_MODE_MASK_0) | \
                                        (OS_IPC_PROTECT_EVENT))
#else
  #define OS_MBOX_MODE_MASK_1           (OS_MBOX_MODE_MASK_0)
#endif

/* Wait on read completion flag */
#if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
  #define OS_MBOX_MODE_MASK_2           ((OS_MBOX_MODE_MASK_1) | \
                                        (OS_IPC_WAIT_IF_EMPTY))
#else
  #define OS_MBOX_MODE_MASK_2           (OS_MBOX_MODE_MASK_1)
#endif

/* Direct read-write flag */
#if (OS_MBOX_ALLOW_DIRECT_RW)
  #define OS_MBOX_MODE_MASK_3           ((OS_MBOX_MODE_MASK_2) | \
                                        (OS_IPC_DIRECT_READ_WRITE))
#else
  #define OS_MBOX_MODE_MASK_3           (OS_MBOX_MODE_MASK_2)
#endif

/* Available mode flags for the mailbox object */
#define OS_MBOX_MODE_MASK               (OS_MBOX_MODE_MASK_3)

/* Size of the mailbox message descriptor */
#define OS_MBOX_MSG_DESC_SIZE            (AR_MEMORY_ALIGN_UP( \
                                         sizeof(struct TMailboxMsg)))


/****************************************************************************
 *
 *  Macros
 *
 ***************************************************************************/

/* Macro returns data offset in the specified message */
#define OS_MBOX_MSG_DATA(Msg)           ((PVOID) &((UINT8 FAR *) \
                                        MailboxMsg)[OS_MBOX_MSG_DESC_SIZE])


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Mailbox message descriptor */
struct TMailboxMsg
{
  /* Pointer to next message */
  struct TMailboxMsg *NextMessage;

  /* Message data size */
  SIZE Size;
};

/* Mailbox object descriptor */
struct TMailboxObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_MBOX_FUNC)
    struct TObjectName Name;
  #endif

  /* Message list */
  struct TMailboxMsg *FirstMessage;
  struct TMailboxMsg *LastMessage;

  /* Mode flags */
  UINT8 Mode;

  /* Mailbox access synchronization */
  #if (OS_MBOX_PEEK_FUNC)
    BOOL PrevLockState;

    #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))
      struct TSignal Sync;
    #endif

    #if (OS_MBOX_PROTECT_MUTEX)
      struct TCriticalSection CS;
    #endif
  #endif

  /* Waiting for incoming data */
  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    struct TSignal SyncOnEmpty;
  #endif
};


/***************************************************************************/
#if ((OS_MBOX_PEEK_FUNC) && \
  ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMailboxLock
 *
 *  Description:
 *    Locks the specified mailbox object.
 *
 *  Parameters:
 *    MailboxObject - Pointer to the mailbox object.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

static BOOL osMailboxLock(struct TMailboxObject FAR *MailboxObject,
  TIME Timeout)
{
  #if (OS_MBOX_PROTECT_MUTEX)
    ERROR LastErrorCode;
  #endif

  /* Lock the mailbox */
  switch(MailboxObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Disable interrupts */
    case OS_IPC_PROTECT_INT_CTRL:
      MailboxObject->PrevLockState = arLock();
      break;

    /* Wait for event */
    #if (OS_MBOX_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        return osWaitFor(&MailboxObject->Sync, Timeout);
    #endif

    /* Wait for mutex */
    #if (OS_MBOX_PROTECT_MUTEX)
      case OS_IPC_PROTECT_MUTEX:

        /* Save last error code */
        LastErrorCode = osGetLastError();

        /* Wait for mutex */
        if(osWaitFor(&MailboxObject->Sync, Timeout))
          return TRUE;

        /* Treat acquiring abandoned critical section as valid process.
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
 *    osMailboxUnlock
 *
 *  Description:
 *    Unlocks the specified mailbox object.
 *
 *  Parameters:
 *    MailboxObject - Pointer to the mailbox object.
 *
 ***************************************************************************/

static void osMailboxUnlock(struct TMailboxObject FAR *MailboxObject)
{
  /* Unlock the mailbox */
  switch(MailboxObject->Mode & OS_IPC_PROTECTION_MASK)
  {
    /* Restore interrupts */
    case OS_IPC_PROTECT_INT_CTRL:
      arRestore(MailboxObject->PrevLockState);
      break;

    /* Release event */
    #if (OS_MBOX_PROTECT_EVENT)
      case OS_IPC_PROTECT_EVENT:
        osUpdateSignalState(&MailboxObject->Sync, TRUE);
        break;
    #endif

    /* Release mutex */
    #if (OS_MBOX_PROTECT_MUTEX)
      case OS_IPC_PROTECT_MUTEX:
        osReleaseCS(&MailboxObject->CS, osCurrentTask, 1, NULL);
        break;
    #endif
  }
}


/***************************************************************************/
#endif /* OS_MBOX_PEEK_FUNC &&
  (OS_MBOX_PROTECT_EVENT || OS_MBOX_PROTECT_MUTEX) */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMailboxWrite
 *
 *  Description:
 *    Sends data to the specified mailbox.
 *
 *  Parameters:
 *    MailboxObject - Pointer to the mailbox object.
 *    Buffer - Pointer to the buffer with data to write.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully sent, or zero on failure.
 *
 ***************************************************************************/

static SIZE osMailboxWrite(struct TMailboxObject FAR *MailboxObject,
  PVOID Buffer, SIZE Size, TIME Timeout)
{
  struct TMailboxMsg FAR *MailboxMsg;
  BOOL PrevLockState;

  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    BOOL PrevISRState;
  #endif

  #if (OS_MBOX_ALLOW_DIRECT_RW)
    struct TTask FAR *Task;
    struct TWaitAssoc FAR *WaitAssoc;
  #endif

  /* Mark unused parameter */
  AR_UNUSED_PARAM(Timeout);

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Direct read-write */
  #if (OS_MBOX_ALLOW_DIRECT_RW)
    if(MailboxObject->Mode & OS_IPC_DIRECT_READ_WRITE)
    {
      /* Enter critical section */
      PrevLockState = arLock();

      /* Get first waiting task */
      WaitAssoc = (struct TWaitAssoc FAR *)
        stBSTreeGetFirst(&MailboxObject->SyncOnEmpty.WaitingTasks);
      if(WaitAssoc)
      {
        Task = WaitAssoc->Task;

        /* Block a task for IPC operation and make it not waiting. The task is
           blocked, so during osTerminateTask call for current task, the
           WaitExitCode may be set. The blocked task will be resumed and
           finishes the wait function with the specified error code. */
        Task->BlockingFlags |= OS_BLOCK_FLAG_IPC;
        osCurrentTask->IPCBlockingTask = Task;
        osMakeNotWaiting(Task);

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

      /* Leave critical section */
      arRestore(PrevLockState);
    }
  #endif

  /* Allocate memory for new message */
  MailboxMsg = (struct TMailboxMsg FAR *)
    stMemAlloc(OS_MBOX_MSG_DESC_SIZE + Size);
  if(!MailboxMsg)
    return 0;

  /* Setup message (and copy data) */
  MailboxMsg->NextMessage = NULL;
  MailboxMsg->Size = Size;
  stMemCpy(OS_MBOX_MSG_DATA(MailboxMsg), Buffer, Size);

  /* Enter critical section and begin delaying scheduler execution */
  PrevLockState = arLock();

  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    PrevISRState = osEnterISR();
  #endif

  /* Store message in the mailbox */
  if(!MailboxObject->FirstMessage)
    MailboxObject->FirstMessage = MailboxMsg;
  else
    MailboxObject->LastMessage->NextMessage = MailboxMsg;
  MailboxObject->LastMessage = MailboxMsg;

  /* Update signal state, which also contains a message counter */
  osUpdateSignalState(&MailboxObject->Object.Signal,
    (INDEX) (MailboxObject->Object.Signal.Signaled + 1));

  /* Release single task waiting for read completion */
  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    osUpdateSignalState(&MailboxObject->SyncOnEmpty,
      MailboxObject->Object.Signal.Signaled);
  #endif

  /* Execute delayed scheduler and leave critical section */
  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    osLeaveISR(PrevISRState);
  #endif

  arRestore(PrevLockState);

  /* Return number of bytes written to mailbox */
  return Size;
}


/****************************************************************************
 *
 *  Name:
 *    osMailboxRead
 *
 *  Description:
 *    Receives and removes data from the specified mailbox. If the mailbox
 *    is empty, it will wait until a new message appears only if the
 *    OS_IPC_WAIT_IF_EMPTY flag is set; otherwise, the function will fail.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *    Buffer - Pointer to the buffer that obtains data.
 *    Size - Size of the data buffer.
 *    Timeout - Timeout value.
 *
 *  Return:
 *    Number of bytes successfully received, or zero on failure.
 *
 ***************************************************************************/

static SIZE osMailboxRead(struct TMailboxObject FAR *MailboxObject,
  PVOID *Buffer, SIZE Size, TIME Timeout)
{
  struct TMailboxMsg FAR *MailboxMsg = NULL;
  BOOL PrevLockState, ForceReturn;

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
    BOOL ProtectByInt;
  #endif

  /* Mark unused parameters to avoid warning messages */
  #if !((OS_MBOX_ALLOW_WAIT_IF_EMPTY) && (OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
    AR_UNUSED_PARAM(Timeout);
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))

    /* Determine the protection method */
    ProtectByInt = (BOOL) ((MailboxObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);

    /* Operation can be performed only by a task when synchronization other
       than by interrupt disabling is used */
    if(!ProtectByInt && (!osCurrentTask || osInISR))
    {
      osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
      return 0;
    }

  #endif

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))

    /* Lock the mailbox and enter critical section */
    if(!osMailboxLock(MailboxObject, Timeout))
      return 0;

    if(!ProtectByInt)
      PrevLockState = arLock();

  #else

    /* Enter critical section */
    PrevLockState = arLock();

  #endif

  /* If the mailbox is empty */
  ForceReturn = !MailboxObject->FirstMessage;
  if(ForceReturn)
  {
    /* Set error if waiting for incoming message is disabled */
    #if !(OS_MBOX_ALLOW_WAIT_IF_EMPTY)

      osSetLastError(ERR_MAILBOX_IS_EMPTY);
      Size = 0;

    /* Wait for incoming message */
    #else

      while(1)
      {
        /* Set error if waiting for incoming message is disabled */
        if(!(MailboxObject->Mode & OS_IPC_WAIT_IF_EMPTY))
        {
          osSetLastError(ERR_MAILBOX_IS_EMPTY);
          Size = 0;
          break;
        }

        /* Waiting can be performed only by a task */
        if(!osCurrentTask || osInISR)
        {
          osSetLastError(ERR_MAILBOX_IS_EMPTY);
          Size = 0;
          break;
        }

        /* Setup for direct read-write */
        #if (OS_MBOX_ALLOW_DIRECT_RW)
          osCurrentTask->IPCDRWCompletion = FALSE;
          osCurrentTask->IPCBuffer = Buffer;
          osCurrentTask->IPCSize = Size;
        #endif

        /* Wait for incoming message */
        if(!osWaitFor(&MailboxObject->SyncOnEmpty, Timeout))
        {
          Size = 0;
          break;
        }

        /* Return, if direct read-write was performed */
        #if (OS_MBOX_ALLOW_DIRECT_RW)
          if(MailboxObject->Mode & OS_IPC_DIRECT_READ_WRITE)
            if(osCurrentTask->IPCDRWCompletion)
            {
              Size = osCurrentTask->IPCSize;
              break;
            }
        #endif

        /* Waiting completed successfully, message must be read */
        ForceReturn = FALSE;
        break;
      }

    #endif
  }

  /* Extract message from the mailbox */
  if(!ForceReturn)
  {
    /* Remove first message */
    MailboxMsg = MailboxObject->FirstMessage;
    if (MailboxMsg)
        MailboxObject->FirstMessage = MailboxMsg->NextMessage;

    /* Change signal state */
    osUpdateSignalState(&MailboxObject->Object.Signal,
      (INDEX) (MailboxObject->Object.Signal.Signaled - 1));
  }

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))

    /* Leave critical section and unlock the mailbox */
    if(!ProtectByInt)
      arRestore(PrevLockState);
    osMailboxUnlock(MailboxObject);

  #else

    /* Leave critical section */
    arRestore(PrevLockState);

  #endif

  /* Copy data */
  if(!ForceReturn && MailboxMsg)
  {
    /* Determine maximal number of bytes that can be copied */
    if(Size > MailboxMsg->Size)
      Size = MailboxMsg->Size;

    /* Copy data */
    stMemCpy(Buffer, OS_MBOX_MSG_DATA(MailboxMsg), Size);

    /* Release memory allocated for the message */
    stMemFree(MailboxMsg);
  }

  /* Return number of successfully transferred bytes */
  return Size;
}


/****************************************************************************
 *
 *  Name:
 *    osMailboxIOCtrl
 *
 *  Description:
 *    Processes device IO control codes for mailbox objects.
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

static INDEX osMailboxIOCtrl(struct TSysObject FAR *Object,
  INDEX ControlCode, PVOID Buffer, SIZE BufferSize,
  struct TIORequest *IORequest)
{
  struct TMailboxObject FAR *MailboxObject;
  struct TMailboxMsg FAR *MailboxMsg;
  SIZE BytesTransferred;

  /* Obtain mailbox descriptor */
  MailboxObject = (struct TMailboxObject FAR *) Object->ObjectDesc;

  /* Execute specific operation */
  switch(ControlCode)
  {
    /* Read from the mailbox */
    case DEV_IO_CTL_READ:
      BytesTransferred = osMailboxRead(MailboxObject, Buffer, BufferSize,
        IORequest ? IORequest->Timeout : OS_INFINITE);
      if(IORequest)
        IORequest->NumberOfBytesTransferred = BytesTransferred;
      return (INDEX) (BytesTransferred != 0);

    /* Write to the mailbox */
    case DEV_IO_CTL_WRITE:
      BytesTransferred = osMailboxWrite(MailboxObject, Buffer, BufferSize,
        IORequest ? IORequest->Timeout : OS_INFINITE);
      if(IORequest)
        IORequest->NumberOfBytesTransferred = BytesTransferred;
      return (INDEX) (BytesTransferred != 0);

    /* Mailbox deinitialization. Releases all memory resources allocated
       for messages. It does not need to be locked by critical section,
       because the mailbox has already been marked as not ready to use. */
    #if (OS_ALLOW_OBJECT_DELETION)
      case DEV_IO_CTL_DEINIT:
        MailboxMsg = MailboxObject->FirstMessage;
        while(MailboxMsg)
        {
          MailboxObject->FirstMessage = MailboxMsg->NextMessage;
          stMemFree(MailboxMsg);
          MailboxMsg = MailboxObject->FirstMessage;
        }
        return 1;
    #endif
  }

  /* Not supported device IO control code */
  osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osCreateMailbox
 *
 *  Description:
 *    Creates a mailbox object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    Mode - Mode flags. Specifying values other than those listed below
 *      will cause function failure. Features are available only when enabled.
 *
 *      Protection Method (select one, defaults to Int Ctrl):
 *      OS_IPC_PROTECT_INT_CTRL - Protection by interrupt disabling.
 *      OS_IPC_PROTECT_EVENT - Protection by auto-reset event.
 *      OS_IPC_PROTECT_MUTEX - Protection by mutex.
 *
 *      Data Transfer Flags:
 *      OS_IPC_WAIT_IF_EMPTY - Enables waiting for read completion.
 *      OS_IPC_DIRECT_READ_WRITE - Enables direct read-write feature (can
 *      be used only with OS_IPC_WAIT_IF_EMPTY flag).
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateMailbox(SYSNAME Name, UINT8 Mode)
{
  struct TMailboxObject FAR *MailboxObject;
  struct TSysObject FAR *Object;
  BOOL InvalidParam;

  /* Check the mode flags */
  InvalidParam = FALSE;
  if(Mode & ((UINT8) ~OS_MBOX_MODE_MASK))
    InvalidParam = TRUE;

  /* Direct read-write feature (can be used only with OS_IPC_WAIT_IF_EMPTY
     flag) */
  #if (OS_QUEUE_ALLOW_DIRECT_RW)
    if(Mode & OS_IPC_DIRECT_READ_WRITE)
      if(!(Mode & OS_IPC_WAIT_IF_EMPTY))
        InvalidParam = TRUE;
  #endif

  /* Check the protection method */
  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
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
  MailboxObject =
    (struct TMailboxObject FAR *) osMemAlloc(sizeof(*MailboxObject));
  if(!MailboxObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &MailboxObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) MailboxObject, Object,
    OS_OBJECT_TYPE_MAILBOX))
  {
    osMemFree(MailboxObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_MBOX_FUNC)
    if(!osRegisterName(Object, &MailboxObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the mailbox */
  Object->Signal.Signaled = 0;
  Object->Flags |= OS_OBJECT_FLAG_USES_IO_DEINIT;
  Object->DeviceIOCtrl = osMailboxIOCtrl;
  MailboxObject->FirstMessage = NULL;
  MailboxObject->Mode = Mode;

  #if (OS_MBOX_PEEK_FUNC)

    /* Setup the auto-reset event / mutex for protection */
    #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))
      if((Mode & OS_IPC_PROTECTION_MASK) != OS_IPC_PROTECT_INT_CTRL)
      {
        MailboxObject->Sync.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
        MailboxObject->Sync.Signaled = (INDEX) TRUE;
        stBSTreeInit(&MailboxObject->Sync.WaitingTasks, osWaitAssocCmp);

        #if (OS_USE_CSEC_OBJECTS)
          MailboxObject->Sync.CS = NULL;
        #endif

        /* Multiple signals associated with object */
        #if (OS_ALLOW_OBJECT_DELETION)
          Object->Signal.NextSignal = &MailboxObject->Sync;
          MailboxObject->Sync.NextSignal = NULL;
        #endif
      }
    #endif

    /* Register the mutex for protection */
    #if (OS_MBOX_PROTECT_MUTEX)
      if((Mode & OS_IPC_PROTECTION_MASK) == OS_IPC_PROTECT_MUTEX)
        osRegisterCS(&MailboxObject->Sync, &MailboxObject->CS, 1, 1, TRUE);
    #endif

  #endif

  /* Setup signal used for waiting for incoming data */
  #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
    if(Mode & OS_IPC_WAIT_IF_EMPTY)
    {
      MailboxObject->SyncOnEmpty.Flags = OS_SIGNAL_FLAG_DEC_ON_RELEASE;
      MailboxObject->SyncOnEmpty.Signaled = 0;
      stBSTreeInit(&MailboxObject->SyncOnEmpty.WaitingTasks, osWaitAssocCmp);

      #if (OS_USE_CSEC_OBJECTS)
        MailboxObject->SyncOnEmpty.CS = NULL;
      #endif

      /* Multiple signals associated with the object */
      #if (OS_ALLOW_OBJECT_DELETION)
        MailboxObject->SyncOnEmpty.NextSignal = Object->Signal.NextSignal;
        Object->Signal.NextSignal = &MailboxObject->SyncOnEmpty;
      #endif
    }
  #endif

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_MBOX_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenMailbox
 *
 *  Description:
 *    Opens an existing mailbox object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenMailbox(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_MAILBOX);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_MBOX_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_MBOX_POST_PEND_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMailboxPost
 *
 *  Description:
 *    Sends data to the specified mailbox.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *    Buffer - Pointer to the buffer with data.
 *    Size - Size of the data buffer.
 *
 *  Return:
 *    Number of bytes successfully sent, or zero on failure.
 *
 ***************************************************************************/

SIZE osMailboxPost(HANDLE Handle, PVOID Buffer, SIZE Size)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MAILBOX);
  if(!Object)
    return 0;

  /* Write data to mailbox */
  return osMailboxWrite((struct TMailboxObject FAR *) Object->ObjectDesc,
    Buffer, Size, OS_INFINITE);
}


/****************************************************************************
 *
 *  Name:
 *    osMailboxPend
 *
 *  Description:
 *    Receives and removes data from the specified mailbox. If the mailbox
 *    is empty, the function will fail.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *    Buffer - Pointer to the buffer that obtains data.
 *    Size - Size of the data buffer.
 *
 *  Return:
 *    Number of bytes successfully received, or zero on failure.
 *
 ***************************************************************************/

SIZE osMailboxPend(HANDLE Handle, PVOID Buffer, SIZE Size)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MAILBOX);
  if(!Object)
    return 0;

  /* Read data from mailbox */
  return osMailboxRead((struct TMailboxObject FAR *) Object->ObjectDesc,
    Buffer, Size, OS_INFINITE);
}


/***************************************************************************/
#endif /* OS_MBOX_POST_PEND_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_MBOX_PEEK_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osMailboxPeek
 *
 *  Description:
 *    Retrieves data from the specified mailbox but does not remove it. If
 *    the mailbox is empty, the function will fail.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *    Buffer - Pointer to the buffer that obtains data.
 *    Size - Size of the data buffer.
 *
 *  Return:
 *    Number of bytes successfully received, or zero on failure.
 *
 ***************************************************************************/

SIZE osMailboxPeek(HANDLE Handle, PVOID Buffer, SIZE Size)
{
  struct TMailboxObject FAR *MailboxObject;
  struct TMailboxMsg FAR *MailboxMsg;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))
    BOOL ProtectByInt;
  #endif

  /* Check parameters */
  if(!Size)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return 0;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MAILBOX);
  if(!Object)
    return 0;

  /* Obtain mailbox descriptor */
  MailboxObject = (struct TMailboxObject FAR *) Object->ObjectDesc;

  #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))

    /* Determine the protection method */
    ProtectByInt = (BOOL) ((MailboxObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);

    /* Operation can be performed only by a task when synchronization other
       than by interrupt disabling is used */
    if(!ProtectByInt && (!osCurrentTask || osInISR))
    {
      osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
      return 0;
    }

  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* Exit if mailbox is empty */
  if(!MailboxObject->FirstMessage)
  {
    arRestore(PrevLockState);
    stSetLastError(ERR_MAILBOX_IS_EMPTY);
    return 0;
  }

  /* Lock the mailbox when method of synchronization other than by interrupts
     disabling is used */
  #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))
    if(!ProtectByInt)
    {
      /* Lock the mailbox (It will return with success) */
      osMailboxLock(MailboxObject, OS_INFINITE);

      /* Exit if mailbox is empty */
      if(!MailboxObject->FirstMessage)
      {
        arRestore(PrevLockState);
        osMailboxUnlock(MailboxObject);
        stSetLastError(ERR_MAILBOX_IS_EMPTY);
        return 0;
      }

      /* Leave critical section */
      arRestore(PrevLockState);
    }
  #endif

  /* Get pointer of the first message */
  MailboxMsg = MailboxObject->FirstMessage;

  /* Determine maximal number of bytes that can be copied */
  if(Size > MailboxMsg->Size)
    Size = MailboxMsg->Size;

  /* Copy data */
  stMemCpy(Buffer, OS_MBOX_MSG_DATA(MailboxMsg), Size);

  /* Unlock the mailbox */
  #if ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX))
    if(ProtectByInt)
      arRestore(PrevLockState);
    else
      osMailboxUnlock(MailboxObject);
  #else
    arRestore(PrevLockState);
  #endif

  /* Return number of received bytes */
  return Size;
}


/***************************************************************************/
#endif /* OS_MBOX_PEEK_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_MBOX_CLEAR_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osClearMailbox
 *
 *  Description:
 *    Clears the specified mailbox.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osClearMailbox(HANDLE Handle)
{
  struct TMailboxObject FAR *MailboxObject;
  struct TMailboxMsg FAR *MailboxMsgToRemove;
  struct TMailboxMsg FAR *MailboxMsg;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
    BOOL ProtectByInt;
  #endif

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MAILBOX);
  if(!Object)
    return FALSE;

  /* Obtain mailbox descriptor */
  MailboxObject = (struct TMailboxObject FAR *) Object->ObjectDesc;

  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))

    /* Determine the protection method */
    ProtectByInt = (BOOL) ((MailboxObject->Mode & OS_IPC_PROTECTION_MASK) ==
      OS_IPC_PROTECT_INT_CTRL);

    /* Operation can be performed only by a task when synchronization other
       than by interrupt disabling is used */
    if(!ProtectByInt && (!osCurrentTask || osInISR))
    {
      osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
      return FALSE;
    }

  #endif

  /* Enter critical section */
  PrevLockState = arLock();

  /* Exit with success when mailbox is empty */
  if(!MailboxObject->FirstMessage)
  {
    arRestore(PrevLockState);
    return TRUE;
  }

  /* Lock the mailbox when method of synchronization other than by interrupts
     disabling is used. It will return with success */
  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
    if(!ProtectByInt)
      osMailboxLock(MailboxObject, OS_INFINITE);
  #endif

  /* Obtain pointer to first message */
  MailboxMsg = MailboxObject->FirstMessage;
  if(MailboxMsg)
  {
    /* Clear the mailbox */
    MailboxObject->FirstMessage = NULL;

    /* Make object non-signaled */
    osUpdateSignalState(&MailboxObject->Object.Signal, 0);

    #if (OS_MBOX_ALLOW_WAIT_IF_EMPTY)
      osUpdateSignalState(&MailboxObject->SyncOnEmpty, 0);
    #endif
  }

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Unlock the mailbox */
  #if ((OS_MBOX_PEEK_FUNC) && \
    ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX)))
    osMailboxUnlock(MailboxObject);
  #endif

  /* Release all memory resources allocated for messages */
  while(MailboxMsg)
  {
    MailboxMsgToRemove = MailboxMsg;
    MailboxMsg = MailboxMsgToRemove->NextMessage;
    stMemFree(MailboxMsgToRemove);
  }

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_MBOX_CLEAR_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_MBOX_GET_INFO_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetMailboxInfo
 *
 *  Description:
 *    Retrieves information about the specified mailbox.
 *
 *  Parameters:
 *    Handle - Handle of the mailbox.
 *    NextSize - Pointer to a variable that receives the size of the next
 *    message to read (may be NULL if not needed).
 *    Count - Pointer to a variable that receives the number of messages
 *      in the mailbox (may be NULL if not needed).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetMailboxInfo(HANDLE Handle, SIZE *NextSize, INDEX *Count)
{
  struct TMailboxObject FAR *MailboxObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MAILBOX);
  if(!Object)
    return FALSE;

  /* Obtain mailbox descriptor */
  MailboxObject = (struct TMailboxObject FAR *) Object->ObjectDesc;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Obtain number of messages information */
  if(Count)
    *Count = MailboxObject->Object.Signal.Signaled;

  /* Obtain size of the next message information */
  if(NextSize)
    *NextSize = MailboxObject->FirstMessage ?
      MailboxObject->FirstMessage->Size : 0;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_MBOX_GET_INFO_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* OS_USE_MAILBOX */
/***************************************************************************/


/***************************************************************************/
