/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_CountSem.c - Counting semaphore object management functions
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
#if (OS_USE_CNT_SEM)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Counting semaphore object descriptor */
struct TCountSemObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_CNT_SEM_FUNC)
    struct TObjectName Name;
  #endif

  /* Maximum semaphore counter value */
  INDEX MaxSignaled;
};


/****************************************************************************
 *
 *  Name:
 *    osCreateCountSem
 *
 *  Description:
 *    Creates a counting semaphore object. A counting semaphore is a fast
 *    semaphore implementation that does not use priority inheritance.
 *    It should not be used for controlling critical sections.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    InitialCount - The initial count for the counting semaphore object.
 *    MaxCount - The maximum count for the counting semaphore object.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateCountSem(SYSNAME Name, INDEX InitialCount, INDEX MaxCount)
{
  struct TCountSemObject FAR *CountSemObject;
  struct TSysObject FAR *Object;

  /* The maximum count cannot be less than the initial count */
  if((MaxCount < InitialCount) || !MaxCount)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  /* Allocate memory for the object */
  CountSemObject =
    (struct TCountSemObject FAR *) osMemAlloc(sizeof(*CountSemObject));
  if(!CountSemObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &CountSemObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) CountSemObject, Object,
    OS_OBJECT_TYPE_COUNT_SEM))
  {
    osMemFree(CountSemObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_CNT_SEM_FUNC)
    if(!osRegisterName(Object, &CountSemObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the object */
  Object->Signal.Flags |= OS_SIGNAL_FLAG_DEC_ON_RELEASE;
  Object->Signal.Signaled = InitialCount;
  CountSemObject->MaxSignaled = MaxCount;

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_CNT_SEM_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenCountSem
 *
 *  Description:
 *    Opens an existing counting semaphore object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenCountSem(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_COUNT_SEM);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_CNT_SEM_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseCountSem
 *
 *  Description:
 *    Releases ownership of a specified counting semaphore object (increments
 *    the count).
 *
 *  Parameters:
 *    Handle - Handle of the counting semaphore object.
 *    ReleaseCount - The value to add to the semaphore counter.
 *      If this value is too large (exceeds MaxCount), the semaphore will
 *      not be released and an error occurs. Must be greater than zero.
 *    PrevCount - Pointer to a variable that receives the counter value
 *      before releasing (can be NULL if not needed).
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osReleaseCountSem(HANDLE Handle, INDEX ReleaseCount, INDEX *PrevCount)
{
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Check parameters */
  if(!ReleaseCount)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_COUNT_SEM);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Check parameters against maximum allowed signal count */
  if(ReleaseCount > (((struct TCountSemObject FAR *)
    Object->ObjectDesc)->MaxSignaled - Object->Signal.Signaled))
  {
    arRestore(PrevLockState);
    osSetLastError(ERR_OBJECT_CAN_NOT_BE_RELEASED);
    return FALSE;
  }

  /* Get previous counter value */
  if(PrevCount)
    *PrevCount = Object->Signal.Signaled;

  /* Release counting semaphore */
  osUpdateSignalState(&Object->Signal,
    Object->Signal.Signaled + ReleaseCount);

  /* Leave critical section */
  arRestore(PrevLockState);
  return TRUE;
}


/***************************************************************************/
#endif /* OS_USE_CNT_SEM */
/***************************************************************************/


/***************************************************************************/
