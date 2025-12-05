/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Semaphore.c - Semaphore object management functions
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
#if (OS_USE_SEMAPHORE)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Semaphore object descriptor */
struct TSemaphoreObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_SEMAPHORE_FUNC)
    struct TObjectName Name;
  #endif

  /* Critical section descriptor */
  struct TCriticalSection CS;
};


/****************************************************************************
 *
 *  Name:
 *    osCreateSemaphore
 *
 *  Description:
 *    Creates a semaphore object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    InitialCount - The initial count for the semaphore object.
 *    MaxCount - The maximum count for the semaphore object.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateSemaphore(SYSNAME Name, INDEX InitialCount, INDEX MaxCount)
{
  struct TSemaphoreObject FAR *SemObject;
  struct TSysObject FAR *Object;

  /* The maximum count cannot be less than the initial count */
  if((MaxCount < InitialCount) || !MaxCount)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Semaphore can be owned only by a task */
  if((InitialCount > 0) && (!osCurrentTask || osInISR))
  {
    osSetLastError(ERR_ALLOWED_ONLY_FOR_TASKS);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object.
     Allocation size includes space for the critical section associations. */
  SemObject = (struct TSemaphoreObject FAR *) osMemAlloc(
    sizeof(*SemObject) + (MaxCount - 1) * sizeof(struct TCSAssoc));
  if(!SemObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &SemObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) SemObject, Object, OS_OBJECT_TYPE_SEMAPHORE))
  {
    osMemFree(SemObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_SEMAPHORE_FUNC)
    if(!osRegisterName(Object, &SemObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Register critical section descriptor */
  osRegisterCS(&Object->Signal, &SemObject->CS, InitialCount,
    MaxCount, FALSE);

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_SEMAPHORE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenSemaphore
 *
 *  Description:
 *    Opens an existing semaphore object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenSemaphore(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_SEMAPHORE);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_SEMAPHORE_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseSemaphore
 *
 *  Description:
 *    Releases ownership of a specified semaphore object (increments count).
 *
 *  Parameters:
 *    Handle - Handle of the semaphore object.
 *    ReleaseCount - The amount to add to the semaphore counter.
 *      If this value is too large (exceeds MaxCount), the semaphore is
 *      not released and an error occurs. Must be greater than zero.
 *    PrevCount - Pointer to a variable that receives the counter value
 *      before releasing (can be NULL if not needed).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osReleaseSemaphore(HANDLE Handle, INDEX ReleaseCount, INDEX *PrevCount)
{
  struct TSysObject FAR *Object;

  /* Check parameters */
  if(!ReleaseCount)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_SEMAPHORE);
  if(!Object)
    return FALSE;

  /* Release critical section */
  return osReleaseCS(Object->Signal.CS, osCurrentTask, ReleaseCount,
    PrevCount);
}


/***************************************************************************/
#endif /* OS_USE_SEMAPHORE */
/***************************************************************************/


/***************************************************************************/
