/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Flags.c - Flags object management functions
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
#if (OS_USE_FLAGS)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Flags object descriptor */
struct TFlagsObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_FLAGS_FUNC)
    struct TObjectName Name;
  #endif
};


/****************************************************************************
 *
 *  Name:
 *    osCreateFlags
 *
 *  Description:
 *    Creates a flags object where each bit represents a binary flag state.
 *    The number of available flags depends on the size of the INDEX type.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    InitialState - Initial state of the flags.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateFlags(SYSNAME Name, INDEX InitialState)
{
  struct TFlagsObject FAR *FlagsObject;
  struct TSysObject FAR *Object;

  /* Allocate memory for the flags object */
  FlagsObject = (struct TFlagsObject FAR *) osMemAlloc(sizeof(*FlagsObject));
  if(!FlagsObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &FlagsObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) FlagsObject, Object, OS_OBJECT_TYPE_FLAGS))
  {
    osMemFree(FlagsObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_FLAGS_FUNC)
    if(!osRegisterName(Object, &FlagsObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the object */
  Object->Signal.Signaled = InitialState;

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_FLAGS_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenFlags
 *
 *  Description:
 *    Opens an existing flags object.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenFlags(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_FLAGS);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_FLAGS_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetFlags
 *
 *  Description:
 *    Returns the current state of the specified flags object.
 *
 *  Parameters:
 *    Handle - Handle of the flags object.
 *    State - Pointer to a variable that receives the flags state.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osGetFlags(HANDLE Handle, INDEX *State)
{
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_FLAGS);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Read signal state */
  *State = Object->Signal.Signaled;

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osSetFlags
 *
 *  Description:
 *    Sets specified flags (logical OR).
 *
 *  Parameters:
 *    Handle - Handle of the flags object.
 *    Mask - Bitmask of the flags to set.
 *    Changed - Pointer to a variable that receives the change status.
 *    Each bit set to 1 represents a flag that was actually changed.
 *    Can be NULL if this information is not needed.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSetFlags(HANDLE Handle, INDEX Mask, INDEX *Changed)
{
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_FLAGS);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Determine changed bits */
  if(Changed)
    *Changed = (INDEX) ((Object->Signal.Signaled ^ Mask) & Mask);

  /* Set the flags */
  osUpdateSignalState(&Object->Signal,
    (INDEX) (Object->Signal.Signaled | Mask));

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osResetFlags
 *
 *  Description:
 *    Resets specified flags (logical AND NOT).
 *
 *  Parameters:
 *    Handle - Handle of the flags object.
 *    Mask - Bitmask of the flags to reset.
 *    Changed - Pointer to a variable that receives the change status.
 *    Each bit set to 1 represents a flag that was actually changed.
 *    Can be NULL if this information is not needed.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osResetFlags(HANDLE Handle, INDEX Mask, INDEX *Changed)
{
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_FLAGS);
  if(!Object)
    return FALSE;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Determine changed bits */
  if(Changed)
    *Changed = (INDEX) ((~Object->Signal.Signaled ^ Mask) & Mask);

  /* Reset the flags */
  osUpdateSignalState(&Object->Signal,
    (INDEX) (Object->Signal.Signaled & ~Mask));

  /* Leave critical section */
  arRestore(PrevLockState);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_USE_FLAGS */
/***************************************************************************/


/***************************************************************************/
