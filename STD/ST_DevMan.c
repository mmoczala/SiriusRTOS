/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_DevMan.c - Device Driver Manager
 *  Version 1.00
 *
 *  Copyright (c) 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/

/*
 * [!] WARNING: EXPERIMENTAL MODULE
 *
 * This module was created significantly later than the core system.
 * It is likely incomplete and has NOT been verified.
 * The logic below contains placeholders, unimplemented paths, and potentially
 * unstable mechanisms. Use with extreme caution and expect to perform
 * significant debugging and implementation work.
 */


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "ST_API.h"

/* Include Operating System when used */
#if (ST_PNP_MULTITASKING)
  #include "OS_API.h"
#endif


/***************************************************************************/
#if (ST_USE_DEVMAN)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Device Driver Manager list item types */
#define ST_PNP_TYPE_NOTUSED             0
#define ST_PNP_TYPE_QUEUED              1
#define ST_PNP_TYPE_PROCESSING          2

/* Device states used by change notification list */
#define ST_DEVMAN_FLAG_ATTACH           0x01
#define ST_DEVMAN_FLAG_DETACH           0x02
#define ST_DEVMAN_FLAG_DETACHING        0x04

/* Internal Power States (Transient) */
#define ST_POWER_STATE_POWERING         0x10
#define ST_POWER_STATE_SUSPENDING_INT   0x11


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

#if (ST_PNP_MULTITASKING)

  /* Device Driver Manager list item */
  struct TDevManListItem
  {
    UINT8 Type;
    struct TDevManListItem *Prev, *Next;

    HANDLE Handle;
    INDEX IoCtl;
  };

#endif

/* Device driver structure */
struct TDriver
{
  TDeviceIOCtl DeviceIOCtl;
  INDEX IOCtl;
  struct TDriver FAR *NextIOCtl;

  #if (ST_POWER_MODE_FUNC)
    HANDLE Handle;
    struct TDriver FAR *NextPower;
  #endif

  #if (ST_PNP_MULTITASKING)
    struct TDevManListItem Item;
  #endif
};

/* Device structure */
struct TDevice
{
  HANDLE DriverHandle;
  PVOID DeviceContext;
  
  #if (ST_FIND_DEVICE_FUNC)
    HANDLE Handle;
    struct TBSTreeNode Node;
  #endif

  #if ((ST_FIND_DEVICE_FUNC) || (ST_DEVICE_NOTIFY_FUNC))
    INDEX Class;
  #endif

  #if (ST_PNP_MULTITASKING) || (ST_DEVICE_NOTIFY_FUNC)
    UINT8 Flags;
    struct TDevice FAR *NextIOCtl;
  #endif
  
  #if (ST_PNP_MULTITASKING)
    struct TDevManListItem Item;
  #endif

  INDEX Index; /* [!] Unused in some configs, preserved from legacy */
};

#if (ST_DEVICE_NOTIFY_FUNC)

  /* Device change notification structure */
  struct TNotify
  {
    HANDLE DriverHandle;
    INDEX Class;

    struct TBSTreeNode Node;
  };

#endif

#if ((ST_PNP_WORKERS) > 0)

  /* Device driver worker information structure */
  struct TWorkerInfo
  {
    HANDLE WorkerHandle;

    int State;
    TIME StateStartTime;

    TDeviceIOCtl DeviceIOCtl;
    HANDLE Handle;
    INDEX IOCtl;
    HANDLE Data;

    BOOL DriverInitSuccess;
  };

#endif


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

/* Device Driver Manager main task termination condition */
#if ((ST_PNP_MULTITASKING) && (ST_DEVMAN_DEINIT_FUNC))
  static volatile BOOL stStopDevManTask;
#endif

/* Device Driver Manager main task */
#if (ST_PNP_MULTITASKING)
  static HANDLE stDevManTaskHandle;
  struct TDevManListItem *stDevManListFirst, *stDevManListLast;
#else
  static BOOL stDevManRunExecuted;
#endif

/* Device driver workers */
#if ((ST_PNP_WORKERS) > 0)
  static INDEX stDevManWorkerCount;
  static HANDLE stDevManWorkerHandles[ST_PNP_WORKERS];
  static TWorkerInfo *stDevManWorkerInfos[ST_PNP_WORKERS];
#endif

/* Device tree ordered by device class and index */
#if (ST_FIND_DEVICE_FUNC)
  struct TBSTree stDevices;
#endif

/* Device change notification tree */
#if (ST_DEVICE_NOTIFY_FUNC)
  struct TBSTree stNotifies;
#endif

/* Device change notification list */
#if (ST_PNP_MULTITASKING) || (ST_DEVICE_NOTIFY_FUNC)
  struct TDevice FAR *stFirstDeviceChange;
  struct TDevice FAR *stLastDeviceChange;
#endif

/* Device driver IOCtl list */
struct TDriver FAR *stFirstDriverIOCtl;
struct TDriver FAR *stLastDriverIOCtl;

/* Device IOCtl list */
#if (ST_PNP_MULTITASKING)
  struct TDevice FAR *stFirstDeviceIOCtl;
  struct TDevice FAR *stLastDeviceIOCtl;
#endif

/* Fixed size memory pool for device manager objects */
#if ((ST_DEVMAN_FIXMEM_SIZE) > 0)
  UINT8 stDevManFixMemPool[ST_DEVMAN_FIXMEM_SIZE];
#endif

/* Power mode management */
#if (ST_POWER_MODE_FUNC)
  static INDEX stCurrPowerStatus;
  static struct TDriver FAR *stPoweredDrivers;
  static struct TDriver FAR *stSuspendedDrivers;
#endif


/****************************************************************************
 *
 * Internal Functions (Forward Declarations)
 *
 ***************************************************************************/

#if (ST_FIND_DEVICE_FUNC)
static int stDeviceCmp(PVOID Item1, PVOID Item2)
{
  /* Compare devices by class */
  if(((struct TDevice FAR *) Item1)->Class <
    ((struct TDevice FAR *) Item2)->Class)
    return -1;

  if(((struct TDevice FAR *) Item1)->Class >
    ((struct TDevice FAR *) Item2)->Class)
    return 1;

  /* Compare devices by handle */
  if(((struct TDevice FAR *) Item1)->Handle <
    ((struct TDevice FAR *) Item2)->Handle)
    return -1;

  if(((struct TDevice FAR *) Item1)->Handle >
    ((struct TDevice FAR *) Item2)->Handle)
    return 1;

  /* Items are equal */
  return 0;
}
#endif

#if (ST_DEVICE_NOTIFY_FUNC)
static int stNotifyCmp(PVOID Item1, PVOID Item2)
{
  /* Compare notifies by class */
  if(((struct TNotify FAR *) Item1)->Class <
    ((struct TNotify FAR *) Item2)->Class)
    return -1;

  if(((struct TNotify FAR *) Item1)->Class >
    ((struct TNotify FAR *) Item2)->Class)
    return 1;

  /* Compare notifies by driver handle */
  if(((struct TNotify FAR *) Item1)->DriverHandle <
    ((struct TNotify FAR *) Item2)->DriverHandle)
    return -1;

  if(((struct TNotify FAR *) Item1)->DriverHandle >
    ((struct TNotify FAR *) Item2)->DriverHandle)
    return 1;

  /* Items are equal */
  return 0;
}
#endif

#if (ST_PNP_MULTITASKING)
static ERROR stDevManMainTask(PVOID Arg);
#endif


/****************************************************************************
 *
 *  Name:
 *    stDevManInit
 *
 *  Description:
 *    Initializes the Device Driver Manager.
 *
 ***************************************************************************/

void stDevManInit(void)
{
  /* Initialize global variables */
  #if (ST_PNP_MULTITASKING)
    stDevManTaskHandle = NULL_HANDLE;
    stDevManListFirst = NULL;
    stDevManListLast = NULL;
  #else
    stDevManRunExecuted = FALSE;
  #endif

  #if ((ST_PNP_WORKERS) > 0)
    stDevManWorkerCount = 0;
  #endif
  
  #if (ST_FIND_DEVICE_FUNC)
    stBSTreeInit(&stDevices, stDeviceCmp);
  #endif

  #if (ST_DEVICE_NOTIFY_FUNC)
    stBSTreeInit(&stNotifies, stNotifyCmp);
  #endif

  #if (ST_PNP_MULTITASKING) || (ST_DEVICE_NOTIFY_FUNC)
    stFirstDeviceChange = NULL;
    stLastDeviceChange = NULL;
  #endif

  stFirstDriverIOCtl = NULL;
  stLastDriverIOCtl = NULL;

  #if (ST_PNP_MULTITASKING)
    stFirstDeviceIOCtl = NULL;
    stLastDeviceIOCtl = NULL;
  #endif

  #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)
    /* [!] TODO: Initialize fixed memory pool (stFixedMemInit is missing) */
    /* stFixedMemInit(stDevManFixMemPool, ST_DEVMAN_FIXMEM_SIZE, ...); */
  #endif

  #if (ST_POWER_MODE_FUNC)
    stCurrPowerStatus = ST_POWER_STATE_POWERED;
    stPoweredDrivers = NULL;
    stSuspendedDrivers = NULL;
  #endif
}


/***************************************************************************/
#if (ST_DEVMAN_DEINIT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stDevManDeinit
 *
 *  Description:
 *    Deinitializes the Device Driver Manager.
 *
 ***************************************************************************/

void stDevManDeinit(void)
{
  /* Multitasking version */
  #if (ST_PNP_MULTITASKING)

    /* Stop all tasks when created */
    if(stDevManTaskHandle)
    {
      /* Stop Device Driver Manager main and worker tasks */
      stStopDevManTask = TRUE;
      osResumeTask(stDevManTaskHandle);

      /* [!] TODO: Wait for task termination and cleanup resources */
    }

  #endif
}


/***************************************************************************/
#endif /* ST_DEVMAN_DEINIT_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stDevManRun
 *
 *  Description:
 *    Runs the Device Driver Manager.
 *    When a dedicated task for Plug and Play support is enabled, this routine
 *    creates and starts it.
 *    When not enabled, the routine calls IOCTL_DRV_RUN for each driver created
 *    before the call of this routine.
 *    This routine may be executed only once.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stDevManRun(void)
{
  /* Multitasking version */
  #if (ST_PNP_MULTITASKING)

    /* Fail when already running */
    if(stDevManTaskHandle)
    {
      stSetLastError(ERR_DEVMAN_ALREADY_RUNNING);
      return FALSE;
    }

    /* Initialize global variables */
    stStopDevManTask = FALSE;

    /* Create Device Driver Manager main task */
    stDevManTaskHandle = osCreateTask(stDevManMainTask, NULL,
      ST_PNP_TASK_STACK_SIZE, ST_PNP_TASK_PRIORITY, FALSE);
    if(!stDevManTaskHandle)
      return FALSE;

  #else

    /* [!] TODO: Implement Single-Tasking Logic:
       - for each driver:
         - call IOCTL_DRV_INIT
         - if success:
           - call IOCTL_DRV_RUN
    */

    stDevManRunExecuted = TRUE;

  #endif

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#if (ST_PNP_MULTITASKING)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stDevManMainTask
 *
 *  Description:
 *    Device Driver Manager main task. Performs device or device driver
 *    IOCtl functions.
 *
 *  Parameters:
 *    Arg - Not used by Device Driver Manager task.
 *
 *  Return:
 *    Returns with ERR_NO_ERROR.
 *
 ***************************************************************************/

static ERROR stDevManMainTask(PVOID Arg)
{
  /* Mark unused parameters */
  AR_UNUSED_PARAM(Arg);


  /* Main task loop */
  while(!stStopDevManTask)
  {
    /* [!] TODO: Implement PnP Task Logic:
       - If available IOCtl:
         - Process IOCtl (or delegate to worker)
         - Continue
       - If power Up/Down requested:
         - Build IOCtl list
         - Continue
       - If Attach/Detach requested:
         - Build IOCtl list
         - Continue
       - If no work pending:
         - Suspend task execution
    */


    /* Suspend task execution */
    osSuspendTask(osGetTaskHandle());
  }


  /* Return with no error */
  return ERR_NO_ERROR;
}


/***************************************************************************/
#endif /* ST_PNP_MULTITASKING */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stCreateDriver
 *
 *  Description:
 *    Creates a driver instance.
 *
 *  Parameters:
 *    DeviceIOCtl - Driver IO Control routine pointer.
 *    DriverContext - Driver specific value passed in call of IOCTL_DRV_INIT,
 *      IOCTL_DRV_DEINIT and IOCTL_DRV_RUN IOCtl.
 *
 *  Return:
 *    Handle of the created driver or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE stCreateDriver(TDeviceIOCtl DeviceIOCtl, PVOID DriverContext)
{
  struct TDriver FAR *Driver;
  HANDLE Handle;

  #if (OS_USED)
    BOOL PrevLockState;
  #endif


  /* Allocate handle and memory buffer */
  #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Allocate memory for the object */
    Driver = (struct TDriver FAR *) stFixedMemAlloc(stDevManFixMemPool);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Not enough memory to finalize */
    if(!Driver)
    {
      stSetLastError(ERR_NOT_ENOUGH_MEMORY);
      return NULL_HANDLE;
    }

    /* Allocate handle for object */
    if(!stHandleAlloc(&Handle, Driver, 0, ST_HANDLE_TYPE_DRIVER))
    {
      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif

      /* Release object memory */
      stFixedMemFree(stDevManFixMemPool, Driver);

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      return NULL_HANDLE;
    }

  #else

    /* Allocate handle and memory for the object */
    if(!stHandleAlloc(&Handle, &Driver, sizeof(struct TDriver),
      ST_HANDLE_TYPE_DRIVER))
      return NULL_HANDLE;

  #endif

  /* Initialize Driver structure */
  Driver->DeviceIOCtl = DeviceIOCtl;


  /* Setup driver power management */
  #if (ST_POWER_MODE_FUNC)

    Driver->Handle = Handle;

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Insert driver into suspended or powered drivers list, depending on
       current power state */
    if(stCurrPowerStatus == ST_POWER_STATE_POWERED ||
      stCurrPowerStatus == ST_POWER_STATE_POWERING_UP)
    {
      Driver->NextPower = stPoweredDrivers;
      stPoweredDrivers = Driver;
    }
    else
    {
      Driver->NextPower = stSuspendedDrivers;
      stSuspendedDrivers = Driver;
    }

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

  #endif


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Device driver initialization */
  #if (ST_PNP_MULTITASKING)

    /* Add driver to IOCtls list to initialize it */
    Driver->IOCtl = IOCTL_DRV_INIT;
    Driver->NextIOCtl = NULL;
    if(stLastDriverIOCtl)
      stLastDriverIOCtl->NextIOCtl = Driver;
    else
      stFirstDriverIOCtl = Driver;
    stLastDriverIOCtl = Driver;

    /* Requires resuming execution of Plug and Play task to process
       device driver creation */
    if(stDevManTaskHandle != NULL_HANDLE)
      osResumeTask(stDevManTaskHandle);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

  #else

    /* Insert into drivers initialization if Run not yet executed */
    if(!stDevManRunExecuted)
    {
      Driver->IOCtl = IOCTL_DRV_INIT;
      Driver->NextIOCtl = NULL;
      if(stLastDriverIOCtl)
        stLastDriverIOCtl->NextIOCtl = Driver;
      else
        stFirstDriverIOCtl = Driver;
      stLastDriverIOCtl = Driver;

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif
    }

    /* Initialize driver immediately by calling corresponding IOCtls */
    else
    {
      /* Leave critical section first to allow IOCTL to run */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* [!] Note: sizeof(DriverContext) is size of pointer (4/8 bytes).
         Ensure this is intended. */
      if(stIOCtrl(Handle, IOCTL_DRV_INIT, DriverContext, sizeof(DriverContext)))
        stIOCtrl(Handle, IOCTL_DRV_RUN, DriverContext, sizeof(DriverContext));
    }

  #endif


  /* Return device driver handle on success */
  return Handle;
}


/****************************************************************************
 *
 *  Name:
 *    stAttachDevice
 *
 *  Description:
 *    Creates a device instance.
 *    A calling task should not be terminated until this function finishes,
 *    otherwise it may fail to release all resources.
 *
 *  Parameters:
 *    DriverHandle - Handle of the device driver that created the device.
 *    Class - Class of the device.
 *    DeviceContext - Driver specific value passed in call of IOCTL_INIT
 *      and IOCTL_DEINIT IOCtl.
 *
 *  Return:
 *    Handle of the created device or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE stAttachDevice(HANDLE DriverHandle, INDEX Class, PVOID DeviceContext)
{
  struct TDevice FAR *Device;
  HANDLE Handle;

  #if (!(ST_PNP_MULTITASKING) && (ST_DEVICE_NOTIFY_FUNC))
    HANDLE DeviceHandle;
    struct TNotify FAR *Notify;
    struct TNotify CurrentNotify;
  #endif

  #if (OS_USED)
    BOOL PrevLockState;
  #endif


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Device manager must be already executed */
  #if (ST_PNP_MULTITASKING)

    if(stDevManTaskHandle == NULL_HANDLE)
    {
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif
      stSetLastError(ERR_DEVMAN_NOT_RUNNING);
      return NULL_HANDLE;
    }

  #else

    if(!stDevManRunExecuted)
    {
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif
      stSetLastError(ERR_DEVMAN_NOT_RUNNING);
      return NULL_HANDLE;
    }

  #endif

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif

  /* Check parameters */
  if(Class == ST_DEV_CLASS_NULL)
  {
    stSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  if(!stGetHandleInfo(DriverHandle, NULL, ST_HANDLE_TYPE_DRIVER))
    return NULL_HANDLE;


  /* Allocate handle and memory buffer */
  #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Allocate memory for the object */
    Device = (struct TDevice FAR *) stFixedMemAlloc(stDevManFixMemPool);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Not enough memory to finalize */
    if(!Device)
    {
      stSetLastError(ERR_NOT_ENOUGH_MEMORY);
      return NULL_HANDLE;
    }

    /* Allocate handle for object */
    if(!stHandleAlloc(&Handle, Device, 0, ST_HANDLE_TYPE_DEVICE))
    {
      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif

      /* Release object memory */
      stFixedMemFree(stDevManFixMemPool, Device);

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      return NULL_HANDLE;
    }

  #else

    /* Allocate handle and memory for the object */
    if(!stHandleAlloc(&Handle, &Device, sizeof(struct TDevice),
      ST_HANDLE_TYPE_DEVICE))
      return NULL_HANDLE;

  #endif


  /* Setup device */
  Device->DriverHandle = DriverHandle;
  Device->DeviceContext = DeviceContext;

  #if (ST_FIND_DEVICE_FUNC)
    Device->Handle = Handle;
  #endif

  #if ((ST_FIND_DEVICE_FUNC) || (ST_DEVICE_NOTIFY_FUNC))
    Device->Class = Class;
  #endif


  /* Device Plug and Play mechanisms */
  #if (ST_PNP_MULTITASKING)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Device is ready to be initialized */
    Device->Flags = ST_DEVMAN_FLAG_ATTACH;

    #if (ST_DEVICE_NOTIFY_FUNC)

      /* Insert into device change notification list */
      Device->NextIOCtl = NULL;
      if(stLastDeviceChange)
        stLastDeviceChange->NextIOCtl = Device;
      else
        stFirstDeviceChange = Device;
      stLastDeviceChange = Device;

    #else

      /* Add device to IOCtl list */
      Device->NextIOCtl = NULL;
      if(stLastDeviceIOCtl)
        stLastDeviceIOCtl->NextIOCtl = Device;
      else
        stFirstDeviceIOCtl = Device;
      stLastDeviceIOCtl = Device;

    #endif

    /* Requires resuming execution of Plug and Play task to process
       device change */
    osResumeTask(stDevManTaskHandle);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

  #else

    /* Call IOCtrl for device initialization */
    stIOCtrl(Handle, IOCTL_INIT, DeviceContext, sizeof(DeviceContext));

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Insert into search tree */
    #if (ST_FIND_DEVICE_FUNC)
      stBSTreeInsert(&stDevices, &Device->Node, NULL, Device);
    #endif

    #if (ST_DEVICE_NOTIFY_FUNC)

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* Notify device drivers on device attach */
      CurrentNotify.Class = Device->Class;
      CurrentNotify.DriverHandle = NULL_HANDLE;
      while(TRUE)
      {
        /* Enter critical section (required only in multitasking) */
        #if (OS_USED)
          PrevLockState = arLock();
        #endif

        /* Find next notify */
        Notify = (struct TNotify FAR *) stBSTreeGetNext(&stNotifies,
          &CurrentNotify);

        /* No more drivers to notify or Class mismatch */
        if(!Notify)
          break;

        if(Notify->Class != CurrentNotify.Class)
          break;

        /* Assign driver handle */
        CurrentNotify.DriverHandle = Notify->DriverHandle;

        /* Leave critical section */
        #if (OS_USED)
          arRestore(PrevLockState);
        #endif

        /* Call IOCtl for device attach. Device handle is copied to
           prevent handle modification by the driver. */
        DeviceHandle = Handle;
        stIOCtrl(CurrentNotify.DriverHandle, IOCTL_DRV_DEVICE_ATTACH,
          &DeviceHandle, sizeof(DeviceHandle));
      }

    #else
      /* Leave critical section if Notify func not used */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif
    #endif

  #endif


  /* Return device handle on success */
  return Handle;
}


/****************************************************************************
 *
 *  Name:
 *    stDetachDevice
 *
 *  Description:
 *    Removes a device specified by handle.
 *    When Plug and Play is not managed by a dedicated task
 *    (ST_PNP_MULTITASKING is 0), a calling task should not be terminated
 *    until this function finishes.
 *
 *  Parameters:
 *    Handle - Handle of the device to remove.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stDetachDevice(HANDLE Handle)
{
  struct TDevice FAR *Device;

  #if (!(ST_PNP_MULTITASKING) && (ST_DEVICE_NOTIFY_FUNC))
    HANDLE DeviceHandle;
    struct TNotify FAR *Notify;
    struct TNotify CurrentNotify;
  #endif

  #if (OS_USED)
    BOOL PrevLockState;
  #endif


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Get pointer to device object structure by its handle */
  if(!stGetHandleInfo(Handle, (PVOID) &Device, ST_HANDLE_TYPE_DEVICE))
  {
    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    return FALSE;
  }

  /* Exit when device is already marked as detaching */
  /* [!] Note: Flags usage depends on ST_PNP_MULTITASKING or ST_DEVICE_NOTIFY_FUNC */
  #if (ST_PNP_MULTITASKING) || (ST_DEVICE_NOTIFY_FUNC)
    if(Device->Flags & ST_DEVMAN_FLAG_DETACHING)
    {
      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      return TRUE;
    }

    /* Mark device as detaching */
    Device->Flags |= ST_DEVMAN_FLAG_DETACHING;
  #endif

  /* Remove from search tree */
  #if (ST_FIND_DEVICE_FUNC)
    stBSTreeRemove(&stDevices, &Device->Node);
  #endif


  /* Device Plug and Play mechanisms */
  #if (ST_PNP_MULTITASKING)

    #if (ST_DEVICE_NOTIFY_FUNC)

      /* Insert into device change notification list when it is not
         already done */
      if(!(Device->Flags & ST_DEVMAN_FLAG_ATTACH))
      {
        Device->NextIOCtl = NULL;
        if(stLastDeviceChange)
          stLastDeviceChange->NextIOCtl = Device;
        else
          stFirstDeviceChange = Device;
        stLastDeviceChange = Device;
      }

    #else

      /* Add device to IOCtl list if it is not already done */
      if(!(Device->Flags & ST_DEVMAN_FLAG_ATTACH))
      {
        Device->NextIOCtl = NULL;
        if(stLastDeviceIOCtl)
          stLastDeviceIOCtl->NextIOCtl = Device;
        else
          stFirstDeviceIOCtl = Device;
        stLastDeviceIOCtl = Device;
      }

    #endif

    /* Mark device as ready to detach */
    Device->Flags |= ST_DEVMAN_FLAG_DETACH;

    /* Requires resuming execution of Plug and Play task to process
       device change */
    osResumeTask(stDevManTaskHandle);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

  #else

    #if (ST_DEVICE_NOTIFY_FUNC)

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* Notify device drivers on device detach */
      CurrentNotify.Class = Device->Class;
      CurrentNotify.DriverHandle = NULL_HANDLE;
      while(TRUE)
      {
        /* Enter critical section (required only in multitasking) */
        #if (OS_USED)
          PrevLockState = arLock();
        #endif

        /* Find next notify */
        Notify = (struct TNotify FAR *) stBSTreeGetNext(&stNotifies,
          &CurrentNotify);

        /* No more drivers to notify */
        if(!Notify)
          break;

        if(Notify->Class != CurrentNotify.Class)
          break;

        /* Assign driver handle */
        CurrentNotify.DriverHandle = Notify->DriverHandle;

        /* Leave critical section */
        #if (OS_USED)
          arRestore(PrevLockState);
        #endif

        /* Call IOCtl for device detach */
        DeviceHandle = Handle;
        stIOCtl(CurrentNotify.DriverHandle, IOCTL_DRV_DEVICE_DETACH,
          &DeviceHandle, sizeof(DeviceHandle));
      }

    #else
      /* Leave critical section if Notify func not used */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif
    #endif

    /* Call IOCtl for device deinitialization */
    stIOCtl(Handle, IOCTL_DEINIT, Device->DeviceContext,
      sizeof(Device->DeviceContext));

    /* Release handle */
    stHandleRelease(Handle);

    /* Release memory */
    #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif

      /* Release memory */
      stFixedMemFree(stDevManFixMemPool, Device);

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

    #endif

  #endif


  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#if (ST_DEVICE_NOTIFY_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stRegDevNotification
 *
 *  Description:
 *    Enables device change notification for the specified driver.
 *
 *  Parameters:
 *    DriverHandle - Handle of the device driver.
 *    Class - Device class to monitor.
 *
 *  Return:
 *    Handle to be used in stUnregDevNotification function call to disable
 *    device change notification, or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE stRegDevNotification(HANDLE DriverHandle, INDEX Class)
{
  struct TNotify FAR *Notify;
  HANDLE Handle;

  #if (OS_USED)
    BOOL PrevLockState;
  #endif


  /* Check parameters */
  if(Class == ST_DEV_CLASS_NULL)
  {
    stSetLastError(ERR_INVALID_PARAMETER);
    return NULL_HANDLE;
  }

  if(!stGetHandleInfo(DriverHandle, NULL, ST_HANDLE_TYPE_DRIVER))
    return NULL_HANDLE;


  /* Allocate handle and memory buffer */
  #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Allocate memory for the object */
    Notify = (struct TNotify FAR *) stFixedMemAlloc(stDevManFixMemPool);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Not enough memory to finalize */
    if(!Notify)
    {
      stSetLastError(ERR_NOT_ENOUGH_MEMORY);
      return NULL_HANDLE;
    }

    /* Allocate handle for object */
    if(!stHandleAlloc(&Handle, Notify, 0, ST_HANDLE_TYPE_NOTIFY))
    {
      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif

      /* Release object memory */
      stFixedMemFree(stDevManFixMemPool, Notify);

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      return NULL_HANDLE;
    }

  #else

    /* Allocate handle and memory for the object */
    if(!stHandleAlloc(&Handle, &Notify, sizeof(struct TNotify),
      ST_HANDLE_TYPE_NOTIFY))
      return NULL_HANDLE;

  #endif


  /* Initialize structure fields */
  Notify->DriverHandle = DriverHandle;
  Notify->Class = Class;


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Try to add new notify into the list */
  if(!stBSTreeInsert(&stNotifies, &Notify->Node, NULL, Notify))
  {
    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Release memory */
    #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif

      /* Release object memory */
      stFixedMemFree(stDevManFixMemPool, Notify);

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

    #endif

    /* Release handle */
    stHandleRelease(Handle);

    /* Registration failure (likely duplicate), return with NULL_HANDLE */
    /* [!] Check ERR_NOTIFY_ALREADY_USED def in ST_API.h */
    stSetLastError(ERR_NOTIFY_ALREADY_USED);
    return NULL_HANDLE;
  }

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif


  /* Return handle of the allocated object */
  return Handle;
}


/****************************************************************************
 *
 *  Name:
 *    stUnregDevNotification
 *
 *  Description:
 *    Disables device change notification for the specified driver.
 *
 *  Parameters:
 *    Handle - Handle returned by stRegDevNotification.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stUnregDevNotification(HANDLE Handle)
{
  struct TNotify FAR *Notify;

  #if (OS_USED)
    BOOL PrevLockState;
  #endif

  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Get pointer to device change notification object structure */
  if(!stGetHandleInfo(Handle, (PVOID) &Notify, ST_HANDLE_TYPE_NOTIFY))
  {
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    return FALSE;
  }

  /* Remove node from device change notification tree */
  stBSTreeRemove(&stNotifies, &Notify->Node);

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif

  /* Release handle */
  stHandleRelease(Handle);

  /* Release memory */
  #if ((ST_DEVMAN_FIXMEM_SIZE) > 0)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      PrevLockState = arLock();
    #endif

    /* Release memory */
    stFixedMemFree(stDevManFixMemPool, Notify);

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

  #endif

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* ST_DEVICE_NOTIFY_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (ST_FIND_DEVICE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stFindFirstDevice
 *
 *  Description:
 *    Returns handle of the first device specified by its class, or
 *    handle of the first available device when class is ST_DEV_CLASS_NULL.
 *
 *  Parameters:
 *    DevSearch - Device search structure.
 *    Class - Function will search for all devices of specified class or
 *    all available devices when ST_DEV_CLASS_NULL is specified.
 *
 *  Return:
 *    Handle of existing device or NULL_HANDLE when no available devices.
 *
 ***************************************************************************/

HANDLE stFindFirstDevice(struct TDevSearch *DevSearch, INDEX Class)
{
  /* Fill device information structure */
  DevSearch->SearchClass = Class;
  DevSearch->Class = Class;
  DevSearch->Handle = NULL_HANDLE;

  /* Find first device by using stFindNextDevice function */
  return stFindNextDevice(DevSearch);
}


/****************************************************************************
 *
 *  Name:
 *    stFindNextDevice
 *
 *  Description:
 *    Continues device search begun by stFindFirstDevice function call.
 *
 *  Parameters:
 *    DevSearch - Device search structure, used in call of stFindFirstDevice.
 *
 *  Return:
 *    Handle of next existing device or NULL_HANDLE when there are no more
 *    available devices.
 *
 ***************************************************************************/

HANDLE stFindNextDevice(struct TDevSearch *DevSearch)
{
  struct TDevice CurrentDevice;
  struct TDevice FAR *NextDevice;
  HANDLE DeviceHandle;

  #if (OS_USED)
    BOOL PrevLockState;
  #endif

  /* No handle is assigned when device is not found */
  DeviceHandle = NULL_HANDLE;

  /* Current device information */
  CurrentDevice.Class = DevSearch->Class;
  CurrentDevice.Handle = DevSearch->Handle;

  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Find the next device */
  NextDevice = (struct TDevice FAR *) stBSTreeGetNext(&stDevices,
    &CurrentDevice);
    
  if(NextDevice)
    if((DevSearch->SearchClass == NextDevice->Class) ||
      (DevSearch->SearchClass == ST_DEV_CLASS_NULL))
    {
      /* Extract device information */
      DevSearch->Class = NextDevice->Class;
      DevSearch->Handle = NextDevice->Handle;
      DeviceHandle = NextDevice->Handle;
    }

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif

  /* Device has not been found */
  if(!DeviceHandle)
    stSetLastError(ERR_DEVICE_NOT_FOUND);

  /* Return handle of the found device */
  return DeviceHandle;
}


/***************************************************************************/
#endif /* ST_FIND_DEVICE_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (ST_POWER_MODE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stSetPowerMode
 *
 *  Description:
 *    Function switches devices to power down or power up mode.
 *    It also suspends or resumes execution of the Plug and Play task and
 *    calls the corresponding IO control for each available driver.
 *
 *  Parameters:
 *    PowerUp - TRUE to switch to power up mode, FALSE for power down mode.
 *
 ***************************************************************************/

static void stSetPowerMode(BOOL PowerUp)
{
  static INDEX stNewPowerStatus;

  #if !(ST_PNP_MULTITASKING)
    struct TDriver FAR *Driver;
  #endif

  #if (OS_USED)
    BOOL PrevLockState;
  #endif

  /* Power change mode */
  stNewPowerStatus =
    PowerUp ? ST_POWER_STATE_POWERING : ST_POWER_STATE_SUSPENDING_INT;

  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Enter to corresponding power change mode */
  stCurrPowerStatus = stNewPowerStatus;

  #if (ST_PNP_MULTITASKING)

    /* Requires resuming execution of Plug and Play task to process
       power change */
    if(stDevManTaskHandle != NULL_HANDLE)
      osResumeTask(stDevManTaskHandle);

  #else

    /* Call corresponding power IO control for each device driver, until
       another call causes switching to another power mode. */
    while(stCurrPowerStatus == stNewPowerStatus)
    {
      /* Get driver pointer */
      Driver = PowerUp ? stSuspendedDrivers : stPoweredDrivers;

      /* No more drivers, enter to powered or suspended mode */
      if(!Driver)
      {
        stCurrPowerStatus =
          PowerUp ? ST_POWER_STATE_POWERED : ST_POWER_STATE_SUSPENDED;
        break;
      }

      /* Move driver from suspended into powered drivers list */
      if(PowerUp)
      {
        stSuspendedDrivers = Driver->NextPower;
        Driver->NextPower = stPoweredDrivers;
        stPoweredDrivers = Driver;
      }

      /* Move driver from powered into suspended drivers list */
      else
      {
        stPoweredDrivers = Driver->NextPower;
        Driver->NextPower = stSuspendedDrivers;
        stSuspendedDrivers = Driver;
      }

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* Call corresponding power IO control for current device driver */
      stIOCtrl(Driver->Handle,
        PowerUp ? IOCTL_DRV_POWER_UP : IOCTL_DRV_POWER_DOWN, NULL, 0);

      /* Enter critical section (required only in multitasking) */
      #if (OS_USED)
        PrevLockState = arLock();
      #endif
    }

  #endif

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif
}


/****************************************************************************
 *
 *  Name:
 *    stPowerUp
 *
 *  Description:
 *    Switches devices to power up mode.
 *
 ***************************************************************************/

void stPowerUp(void)
{
  /* Enter power up mode */
  stSetPowerMode(TRUE);
}


/****************************************************************************
 *
 *  Name:
 *    stPowerDown
 *
 *  Description:
 *    Switches devices to power down mode.
 *
 ***************************************************************************/

void stPowerDown(void)
{
  /* Enter power down mode */
  stSetPowerMode(FALSE);
}


/****************************************************************************
 *
 *  Name:
 *    stPowerStatus
 *
 *  Description:
 *    Returns the current power status.
 *
 *  Return:
 *    Returns current power status.
 *
 ***************************************************************************/

INDEX stPowerStatus(void)
{
  /* Return current power status */
  return stCurrPowerStatus;
}


/***************************************************************************/
#endif /* ST_POWER_MODE_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* ST_USE_DEVMAN */
/***************************************************************************/


/***************************************************************************/
