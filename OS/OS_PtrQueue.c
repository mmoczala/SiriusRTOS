/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_PtrQueue.c - Pointer queue object management functions
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
#if (OS_USE_PTR_QUEUE)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Pointer queue object descriptor */
struct TPtrQueueObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_PTR_QUEUE_FUNC)
    struct TObjectName Name;
  #endif

  /* Pointer queue buffer */
  INDEX MaxCount;
  INDEX Count;
  INDEX Offset;
  PVOID Data[1];
};


/****************************************************************************
 *
 *  Name:
 *    osCreatePtrQueue
 *
 *  Description:
 *    Creates a pointer queue object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    MaxCount - Maximum number of messages (pointers) the queue can hold.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreatePtrQueue(SYSNAME Name, INDEX MaxCount)
{
  struct TPtrQueueObject FAR *PtrQueueObject;
  struct TSysObject FAR *Object;

  /* Check parameters */
  if(!MaxCount || (MaxCount > ((((SIZE) (-1)) -
    sizeof(*PtrQueueObject)) / sizeof(PVOID) + 1)))
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object */
  PtrQueueObject = (struct TPtrQueueObject FAR *)
    osMemAlloc(sizeof(*PtrQueueObject) + (MaxCount - 1) * sizeof(PVOID));
  if(!PtrQueueObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &PtrQueueObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) PtrQueueObject, Object,
    OS_OBJECT_TYPE_PTR_QUEUE))
  {
    osMemFree(PtrQueueObject);
    return NULL_HANDLE;
  }

  /* Setup the object */
  Object->Signal.Signaled = (INDEX) FALSE;
  PtrQueueObject->MaxCount = MaxCount;
  PtrQueueObject->Count = 0;
  PtrQueueObject->Offset = 0;

  /* Register name descriptor */
  #if (OS_OPEN_PTR_QUEUE_FUNC)
    if(!osRegisterName(Object, &PtrQueueObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_PTR_QUEUE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenPtrQueue
 *
 *  Description:
 *    Opens an existing pointer queue object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenPtrQueue(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_PTR_QUEUE);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_PTR_QUEUE_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osPtrQueuePost
 *
 *  Description:
 *    Appends a pointer to the specified queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Ptr - Pointer to be stored in the queue.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osPtrQueuePost(HANDLE Handle, PVOID Ptr)
{
  struct TPtrQueueObject FAR *PtrQueueObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState, Success;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_PTR_QUEUE);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Store message in the queue */
  PtrQueueObject = (struct TPtrQueueObject FAR *) Object->ObjectDesc;
  Success = PtrQueueObject->Count < PtrQueueObject->MaxCount;
  if(Success)
  {
    INDEX Offset, Tmp;

    /* Calculate offset for the new message.
       Logic equivalent to: (Offset + Count) % MaxCount */
    Offset = PtrQueueObject->Offset;
    Tmp = (INDEX) (PtrQueueObject->MaxCount - Offset);
    if(PtrQueueObject->Count < Tmp)
      Offset += PtrQueueObject->Count;
    else
      Offset = (INDEX) (PtrQueueObject->Count - Tmp);

    /* Store message in the queue */
    PtrQueueObject->Data[Offset] = Ptr;
    PtrQueueObject->Count++;

    /* Make object signaled */
    osUpdateSignalState(&Object->Signal, TRUE);
  }

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Set last error code at failure situation */
  if(!Success)
    osSetLastError(ERR_PTR_QUEUE_IS_FULL);

  /* Return with success */
  return Success;
}


/****************************************************************************
 *
 *  Name:
 *    osPtrQueuePend
 *
 *  Description:
 *    Receives and removes a pointer from the specified queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Ptr - Pointer to a variable that receives the message.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osPtrQueuePend(HANDLE Handle, PVOID *Ptr)
{
  struct TPtrQueueObject FAR *PtrQueueObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState, Success;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_PTR_QUEUE);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Get message from the queue and remove it */
  PtrQueueObject = (struct TPtrQueueObject FAR *) Object->ObjectDesc;
  Success = PtrQueueObject->Count > 0;
  if(Success)
  {
    *Ptr = PtrQueueObject->Data[PtrQueueObject->Offset];

    /* Remove message */
    PtrQueueObject->Count--;
    PtrQueueObject->Offset++;
    if(PtrQueueObject->Offset >= PtrQueueObject->MaxCount)
      PtrQueueObject->Offset = 0;
  }

  /* Change object signalization */
  osUpdateSignalState(&Object->Signal, PtrQueueObject->Count > 0);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Set last error code at failure situation */
  if(!Success)
    osSetLastError(ERR_PTR_QUEUE_IS_EMPTY);

  /* Return with success */
  return Success;
}


/***************************************************************************/
#if (OS_PTR_QUEUE_PEEK_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osPtrQueuePeek
 *
 *  Description:
 *    Retrieves a pointer from the specified queue but does not remove it.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *    Ptr - Pointer to a variable that receives the message.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osPtrQueuePeek(HANDLE Handle, PVOID *Ptr)
{
  struct TPtrQueueObject FAR *PtrQueueObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState, Success;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_PTR_QUEUE);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Get message from the queue */
  PtrQueueObject = (struct TPtrQueueObject FAR *) Object->ObjectDesc;
  Success = PtrQueueObject->Count > 0;
  if(Success)
    *Ptr = PtrQueueObject->Data[PtrQueueObject->Offset];

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Set last error code at failure situation */
  if(!Success)
    osSetLastError(ERR_PTR_QUEUE_IS_EMPTY);

  /* Return with success */
  return Success;
}


/***************************************************************************/
#endif /* OS_PTR_QUEUE_PEEK_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_PTR_QUEUE_CLEAR_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osClearPtrQueue
 *
 *  Description:
 *    Clears the specified pointer queue.
 *
 *  Parameters:
 *    Handle - Handle of the queue.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osClearPtrQueue(HANDLE Handle)
{
  struct TPtrQueueObject FAR *PtrQueueObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_PTR_QUEUE);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Clear the queue */
  PtrQueueObject = (struct TPtrQueueObject FAR *) Object->ObjectDesc;
  PtrQueueObject->Count = 0;
  PtrQueueObject->Offset = 0;

  /* Make object non-signaled */
  osUpdateSignalState(&Object->Signal, FALSE);

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_PTR_QUEUE_CLEAR_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* OS_USE_PTR_QUEUE */
/***************************************************************************/


/***************************************************************************/
