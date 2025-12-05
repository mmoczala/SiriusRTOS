/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Mutex.c - Mutual exclusion semaphore object management functions
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
#if (OS_USE_MUTEX)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Mutex object descriptor */
struct TMutexObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_MUTEX_FUNC)
    struct TObjectName Name;
  #endif

  /* Critical section descriptor */
  struct TCriticalSection CS;
};


/****************************************************************************
 *
 *  Name:
 *    osCreateMutex
 *
 *  Description:
 *    Creates a mutex object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    InitialOwner - If TRUE, the calling task becomes the owner of the
 *      created mutex. To release ownership, use the osReleaseMutex
 *      function.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateMutex(SYSNAME Name, BOOL InitialOwner)
{
  struct TMutexObject FAR *MutexObject;
  struct TSysObject FAR *Object;

  /* Mutex can only be owned by a task */
  if(InitialOwner && (!osCurrentTask || osInISR))
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object */
  MutexObject = (struct TMutexObject FAR *) osMemAlloc(sizeof(*MutexObject));
  if(!MutexObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &MutexObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) MutexObject, Object, OS_OBJECT_TYPE_MUTEX))
  {
    osMemFree(MutexObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_MUTEX_FUNC)
    if(!osRegisterName(Object, &MutexObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Register critical section descriptor.
     If InitialOwner is TRUE, initial count is 0 (locked).
     If InitialOwner is FALSE, initial count is 1 (unlocked). */
  osRegisterCS(&Object->Signal, &MutexObject->CS, !InitialOwner, 1, TRUE);

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_MUTEX_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenMutex
 *
 *  Description:
 *    Opens an existing mutex object by name.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenMutex(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_MUTEX);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_MUTEX_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseMutex
 *
 *  Description:
 *    Releases ownership of the specified mutex object.
 *
 *  Parameters:
 *    Handle - Handle of the mutex object.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osReleaseMutex(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_MUTEX);
  if(!Object)
    return FALSE;

  /* Release critical section */
  return osReleaseCS(Object->Signal.CS, osCurrentTask, 1, NULL);
}


/***************************************************************************/
#endif /* OS_USE_MUTEX */
/***************************************************************************/


/***************************************************************************/
