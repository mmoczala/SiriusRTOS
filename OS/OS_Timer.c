/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Timer.c - Timer object management functions
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
#if (OS_USE_TIMER)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Timer object descriptor */
struct TTimerObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_TIMER_FUNC)
    struct TObjectName Name;
  #endif

  /* Time notification descriptor */
  struct TTimeNotify TimeNotify;

  /* Timer specific variables */
  BOOL ManualReset;
  BOOL Running;
  BOOL IsPeriodical;
  INDEX PassCount;
  TIME Interval;
  TIME StartupTime;
  TIME SignalTime;
};


/****************************************************************************
 *
 *  Name:
 *    osUpdateTimer     
 *
 *  Description:
 *    Updates the time notification for the specified timer.
 *
 *  Parameters:
 *    TimerObject - Pointer to the timer object.
 *
 ***************************************************************************/

static void osUpdateTimer(struct TTimerObject FAR *TimerObject)
{
  /* Register or update the timer notification if running and tasks are
     waiting */
  if(TimerObject->Running &&
    stBSTreeGetFirst(&TimerObject->Object.Signal.WaitingTasks))
  {
    osRegisterTimeNotify(&TimerObject->TimeNotify,
      TimerObject->SignalTime);
  }

  /* Unregister timer if stopped or no tasks are waiting */
  else
    osUnregisterTimeNotify(&TimerObject->TimeNotify);
}


/****************************************************************************
 *
 *  Name:
 *    osRestartTimer
 *
 *  Description:
 *    Restarts the specified timer.
 *
 *  Parameters:
 *    TimerObject - Pointer to the timer object.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

static BOOL osRestartTimer(struct TTimerObject FAR *TimerObject)
{
  BOOL PrevLockState;
  TIME CurrentTime, PassCount;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Check if the timer is running */
  if(!TimerObject->Running)
  {
    arRestore(PrevLockState);
    return FALSE;
  }

  /* Get the current time */
  CurrentTime = arGetTickCount();

  /* Calculate the number of elapsed intervals for auto-reset timers */
  if(!TimerObject->ManualReset)
    PassCount = (CurrentTime - TimerObject->StartupTime) /
      TimerObject->Interval;

  /* Check stop condition */
  if(!TimerObject->IsPeriodical)
  {
    /* Stop condition for manual-reset timer */
    if(TimerObject->ManualReset)
    {
      if(TimerObject->SignalTime <= CurrentTime)
      {
        TimerObject->PassCount--;
        if(!TimerObject->PassCount)
          TimerObject->Running = FALSE;
      }
    }

    /* Stop condition for auto-reset timer */
    else
    {
      if(((INDEX) PassCount) >= TimerObject->PassCount)
        TimerObject->Running = FALSE;
    }
  }

  /* If the timer is still running after checks */
  if(TimerObject->Running)
  {
    /* Update the pass counter of periodical auto-reset timer */
    if(!TimerObject->ManualReset && !TimerObject->IsPeriodical)
    {
      TimerObject->PassCount -= PassCount;
      if(TimerObject->SignalTime <= CurrentTime)
      {
        /* Drift correction: add elapsed intervals to startup time */
        TimerObject->StartupTime += PassCount * TimerObject->Interval;
        CurrentTime = TimerObject->StartupTime;
      }
      else
        TimerObject->StartupTime = CurrentTime;
    }

    /* Calculate the end time of the current cycle */
    TimerObject->SignalTime =
      (TimerObject->Interval >= (OS_INFINITE - CurrentTime)) ?
      OS_INFINITE : (CurrentTime + TimerObject->Interval);

    /* Set auto-reset flag state when timer is a running auto-reset timer */
    TimerObject->Object.Signal.Flags |= OS_SIGNAL_FLAG_DEC_ON_RELEASE;
  }

  /* Timer is stopped now */
  else
  {
    /* Clear auto-reset flag state when timer is stopped */
    TimerObject->Object.Signal.Flags &=
      (UINT8) ~OS_SIGNAL_FLAG_DEC_ON_RELEASE;

    /* Stopped timer is always in the signaled state */
    osUpdateSignalState(&TimerObject->Object.Signal, TRUE);
  }

  /* Update the timer notification */
  osUpdateTimer(TimerObject);

  /* Leave critical section */
  arRestore(PrevLockState);
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osStopTimer
 *
 *  Description:
 *    Stops the specified timer.
 *
 *  Parameters:
 *    TimerObject - Pointer to the timer object.
 *
 ***************************************************************************/

static void osStopTimer(struct TTimerObject FAR *TimerObject)
{
  BOOL PrevLockState;

  /* Enter critical section */
  PrevLockState = arLock();

  /* Stop the timer */
  TimerObject->Running = FALSE;
  osUnregisterTimeNotify(&TimerObject->TimeNotify);

  /* Update signalization state */
  osUpdateSignalState(&TimerObject->Object.Signal, TRUE);

  /* Leave critical section */
  arRestore(PrevLockState);
}


/****************************************************************************
 *
 *  Name:
 *    osTimerlIOCtrl
 *
 *  Description:
 *    Processes device IO control codes for timer objects.
 *
 *  Parameters:
 *    Object - Pointer to the system object.
 *    ControlCode - Device IO control code.
 *    Buffer - Not used by timer objects.
 *    BufferSize - Not used by timer objects.
 *    IORequest - Not used by timer objects.
 *
 *  Return:
 *    Value specific to the ControlCode.
 *
 ***************************************************************************/

static INDEX osTimerIOCtrl(struct TSysObject FAR *Object,
  INDEX ControlCode, PVOID Buffer, SIZE BufferSize,
  struct TIORequest *IORequest)
{
  struct TTimerObject FAR *TimerObject;

  #if (OS_ALLOW_OBJECT_DELETION)
    BOOL PrevLockState;
  #endif

  /* Mark unused parameters */
  AR_UNUSED_PARAM(Buffer);
  AR_UNUSED_PARAM(BufferSize);
  AR_UNUSED_PARAM(IORequest);

  /* Obtain timer descriptor */
  TimerObject = (struct TTimerObject FAR *) Object->ObjectDesc;

  /* Execute specific operation */
  switch(ControlCode)
  {
    /* Determine object signalization state */
    case OS_IO_CTL_GET_SIGNAL_STATE:

      /* Check timer signalization when it is running */
      if(TimerObject->Running)
      {
        TimerObject->Object.Signal.Signaled =
          (BOOL) (TimerObject->SignalTime <= arGetTickCount());
        return (INDEX) TimerObject->Object.Signal.Signaled;
      }

      /* The timer is signaled when it is stopped */
      return (INDEX) TRUE;

    /* Waiting completion */
    case OS_IO_CTL_WAIT_ACQUIRE:

      /* Restart auto-reset timer upon successful wait acquisition */
      if(TimerObject->Running && !TimerObject->ManualReset)
        osRestartTimer(TimerObject);

      /* Operation success */
      return (INDEX) TRUE;

    /* New task started waiting, priority changed, or wait failure */
    case OS_IO_CTL_WAIT_START:
    case OS_IO_CTL_WAIT_UPDATE:
    case OS_IO_CTL_WAIT_FAILURE:

      /* Update the timer notification logic */
      osUpdateTimer(TimerObject);

      /* Operation success */
      return (INDEX) TRUE;

    /* Deinitialize the timer */
    #if (OS_ALLOW_OBJECT_DELETION)

      case DEV_IO_CTL_DEINIT:

        /* Enter critical section */
        PrevLockState = arLock();

        /* Unregister time notification (if registered) */
        osUnregisterTimeNotify(&TimerObject->TimeNotify);

        /* Leave critical section */
        arRestore(PrevLockState);
        return (INDEX) TRUE;

    #endif
  }

  /* Not supported device IO control code */
  osSetLastError(ERR_INVALID_DEVICE_IO_CTL);
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    osCreateTimer
 *
 *  Description:
 *    Creates a timer object.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    ManualReset - Determines the reset behavior:
 *      TRUE:  Manual Reset. When signaled, it remains signaled until
 *      osResetTimer is called. All waiting tasks are released.
 *      FALSE: Auto Reset. When signaled, it releases one waiting task
 *      and automatically resets.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateTimer(SYSNAME Name, BOOL ManualReset)
{
  struct TTimerObject FAR *TimerObject;
  struct TSysObject FAR *Object;

  /* Allocate memory for the object */
  TimerObject = (struct TTimerObject FAR *) osMemAlloc(sizeof(*TimerObject));
  if(!TimerObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &TimerObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) TimerObject, Object, OS_OBJECT_TYPE_TIMER))
  {
    osMemFree(TimerObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_TIMER_FUNC)
    if(!osRegisterName(Object, &TimerObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Setup the object */
  Object->DeviceIOCtrl = osTimerIOCtrl;
  Object->Flags |= OS_OBJECT_FLAG_USES_IO_DEINIT;
  Object->Signal.Flags |= OS_SIGNAL_FLAG_USES_IO_SYSTEM;
  osInitTimeNotify(&TimerObject->TimeNotify);
  TimerObject->TimeNotify.Signal = &Object->Signal;
  TimerObject->ManualReset = ManualReset;
  TimerObject->Running = FALSE;

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_TIMER_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenTimer
 *
 *  Description:
 *    Opens an existing timer object by name.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenTimer(SYSNAME Name)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_TIMER);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object ? Object->Handle : NULL_HANDLE;
}


/***************************************************************************/
#endif /* OS_OPEN_TIMER_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osSetTimer
 *
 *  Description:
 *    Configures and starts the timer. After calling this function, the
 *    timer is in the non-signaled state and will switch to the signaled
 *    state after 'n' time units.
 *
 *  Parameters:
 *    Handle - Handle of the timer object.
 *    Interval - Time units before the timer is signaled. Fails if zero.
 *    PassCount - Number of timer passes. If zero, timer is periodical.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL osSetTimer(HANDLE Handle, TIME Interval, INDEX PassCount)
{
  struct TTimerObject FAR *TimerObject;
  struct TSysObject FAR *Object;
  BOOL PrevLockState;

  /* Check parameters */
  if(!Interval)
  {
    osSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TIMER);
  if(!Object)
    return FALSE;

  /* Obtain timer descriptor */
  TimerObject = (struct TTimerObject FAR *) Object->ObjectDesc;

  /* Stop timer (if it is working) */
  osStopTimer(TimerObject);

  /* Enter critical section */
  PrevLockState = arLock();

  /* Set up the timer */
  TimerObject->Running = TRUE;
  TimerObject->Interval = Interval;
  TimerObject->PassCount = PassCount;
  TimerObject->IsPeriodical = (PassCount == 0);
  TimerObject->StartupTime = arGetTickCount();
  TimerObject->SignalTime =
    (TimerObject->Interval >= (OS_INFINITE - TimerObject->StartupTime)) ?
    OS_INFINITE : (TimerObject->StartupTime + TimerObject->Interval);

  /* Start timer */
  osRestartTimer(TimerObject);

  /* Leave critical section */
  arRestore(PrevLockState);
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osResetTimer
 *
 *  Description:
 *    Resets (re-triggers) the timer without stopping it.
 *    If signaled, the timer switches to non-signaled.
 *    If not running, the function fails.
 *
 *  Parameters:
 *    Handle - Handle of the timer object.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL osResetTimer(HANDLE Handle)
{
  struct TSysObject FAR *Object;
  struct TTimerObject FAR *TimerObject;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TIMER);
  if(!Object)
    return FALSE;

  /* Get the timer object descriptor */
  TimerObject = (struct TTimerObject FAR *) Object->ObjectDesc;

  /* Restart timer */
  if(!osRestartTimer(TimerObject))
  {
    osSetLastError(ERR_TIMER_NOT_STARTED);
    return FALSE;
  }

  /* Update the timer notification when success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    osCancelTimer
 *
 *  Description:
 *    Resets and stops the timer. A stopped timer is in the signaled state.
 *
 *  Parameters:
 *    Handle - Handle of the timer object.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL osCancelTimer(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_TIMER);
  if(!Object)
    return FALSE;

  /* Stop timer */
  osStopTimer((struct TTimerObject FAR *) Object->ObjectDesc);
  return TRUE;
}


/***************************************************************************/
#endif /* OS_USE_TIMER */
/***************************************************************************/


/***************************************************************************/
