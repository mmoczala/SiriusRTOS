/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Event.c - Event object management functions
 *  Version 1.00
 *
 *  Copyright 2009 by SpaceShadow
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
#if (OS_USE_EVENT)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Event object descriptor */
struct TEventObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_EVENT_FUNC)
    struct TObjectName Name;
  #endif
};


/****************************************************************************
 *
 *  Name:
 *    osCreateEvent
 *
 *  Description:
 *    Creates an event object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    InitialState - Initial state of the event (TRUE - event is set/signaled).
 *    ManualReset - Determines the reset behavior:
 *      TRUE:  Manual Reset. When signaled, it remains signaled until
 *      osResetEvent is called. All waiting tasks are released.
 *      FALSE: Auto Reset. When signaled, it releases only one waiting task
 *      and automatically resets the event to the non-signaled state.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateEvent(SYSNAME Name, BOOL InitialState, BOOL ManualReset)
{
  struct TEventObject FAR *EventObject;
  struct TSysObject FAR *Object;

  /* Allocate memory for the object */
  EventObject = (struct TEventObject FAR *) osMemAlloc(sizeof(*EventObject));
  if(!EventObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &EventObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) EventObject, Object, OS_OBJECT_TYPE_EVENT))
  {
    osMemFree(EventObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_EVENT_FUNC)
    if(!osRegisterName(Object, &EventObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the object */
  Object->Signal.Signaled = (INDEX) InitialState;
  if(!ManualReset)
    Object->Signal.Flags |= OS_SIGNAL_FLAG_DEC_ON_RELEASE;

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_EVENT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenEvent
 *
 *  Description:
 *    Opens an existing event object by name.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenEvent(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_EVENT);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_EVENT_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSetEvent
 *
 *  Description:
 *    Sets the event state to signaled.
 *
 *  Parameters:
 *    Handle - Handle of the event object.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osSetEvent(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_EVENT);
  if(!Object)
    return FALSE;

  /* Make object signaled */
  osUpdateSignalState(&Object->Signal, (INDEX) TRUE);

  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osResetEvent
 *
 *  Description:
 *    Resets the event state to non-signaled.
 *
 *  Parameters:
 *    Handle - Handle of the event object.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osResetEvent(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_EVENT);
  if(!Object)
    return FALSE;

  /* Make object non-signaled */
  osUpdateSignalState(&Object->Signal, (INDEX) FALSE);

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* OS_USE_EVENT */
/***************************************************************************/


/***************************************************************************/
