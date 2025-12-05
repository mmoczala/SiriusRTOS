/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_DevMan.h - Device Driver Manager
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_DEVMAN_H
#define ST_DEVMAN_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"
#include "ST_Handle.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable Device Driver Manager functions by default */
#ifndef ST_USE_DEVMAN
  #define ST_USE_DEVMAN                 1
#elif (((ST_USE_DEVMAN) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_USE_DEVMAN must be either 0 or 1
#endif

/* Enable dedicated Plug and Play (PnP) management task by default */
#ifndef ST_PNP_MULTITASKING
  #define ST_PNP_MULTITASKING           1
#elif (((ST_PNP_MULTITASKING) != 0) && ((ST_PNP_MULTITASKING) != 1))
  #error ST_PNP_MULTITASKING must be either 0 or 1
#elif (((ST_PNP_MULTITASKING) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_PNP_MULTITASKING must be 0 when ST_USE_DEVMAN is 0
#elif (((ST_PNP_MULTITASKING) != 0) && (!defined(OS_USED)))
  #error ST_PNP_MULTITASKING must be 0 when Operating System is not used
#elif (((ST_PNP_MULTITASKING) != 0) && ((OS_USED) != 1))
  #error ST_PNP_MULTITASKING must be 0 when Operating System is not used
#endif

/* The highest priority (0) is assigned to the PnP main task */
#ifndef ST_PNP_TASK_PRIORITY
  #define ST_PNP_TASK_PRIORITY          0
#elif (((ST_PNP_TASK_PRIORITY) < 0) || (ST_PNP_TASK_PRIORITY > 254))
  #error ST_PNP_TASK_PRIORITY must be in range from 0 to 254
#endif

/* The default stack size is assigned to the PnP main task */
#ifndef ST_PNP_TASK_STACK_SIZE
  #define ST_PNP_TASK_STACK_SIZE        0
#elif ((ST_PNP_TASK_STACK_SIZE) < 0)
  #error ST_PNP_TASK_STACK_SIZE must be greater than or equal to 0
#endif

/* Disable fixed-size memory for device management by default */
#ifndef ST_DEVMAN_FIXMEM_SIZE
  #define ST_DEVMAN_FIXMEM_SIZE         0
#elif ((ST_DEVMAN_FIXMEM_SIZE) < 0)
  #error ST_DEVMAN_FIXMEM_SIZE must be greater than or equal to 0
#elif (((ST_DEVMAN_FIXMEM_SIZE) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_DEVMAN_FIXMEM_SIZE must be 0 when ST_USE_DEVMAN is 0
#endif

/* Enable stDeinitDevMan function by default */
#ifndef ST_DEVMAN_DEINIT_FUNC
  #define ST_DEVMAN_DEINIT_FUNC         1
#elif (((ST_DEVMAN_DEINIT_FUNC) != 0) && ((ST_DEVMAN_DEINIT_FUNC) != 1))
  #error ST_DEVMAN_DEINIT_FUNC must be either 0 or 1
#elif (((ST_DEVMAN_DEINIT_FUNC) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_DEVMAN_DEINIT_FUNC must be 0 when ST_USE_DEVMAN is 0
#endif

/* Enable device change notification functions by default */
#ifndef ST_DEVICE_NOTIFY_FUNC
  #define ST_DEVICE_NOTIFY_FUNC         1
#elif (((ST_DEVICE_NOTIFY_FUNC) != 0) && ((ST_DEVICE_NOTIFY_FUNC) != 1))
  #error ST_DEVICE_NOTIFY_FUNC must be either 0 or 1
#elif (((ST_DEVICE_NOTIFY_FUNC) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_DEVICE_NOTIFY_FUNC must be 0 when ST_USE_DEVMAN is 0
#endif

/* Enable Find Device functions by default */
#ifndef ST_FIND_DEVICE_FUNC
  #define ST_FIND_DEVICE_FUNC           1
#elif (((ST_FIND_DEVICE_FUNC) != 0) && ((ST_FIND_DEVICE_FUNC) != 1))
  #error ST_FIND_DEVICE_FUNC must be either 0 or 1
#elif (((ST_FIND_DEVICE_FUNC) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_FIND_DEVICE_FUNC must be 0 when ST_USE_DEVMAN is 0
#endif

/* Enable Power Mode functions by default */
#ifndef ST_POWER_MODE_FUNC
  #define ST_POWER_MODE_FUNC            1
#elif (((ST_POWER_MODE_FUNC) != 0) && ((ST_POWER_MODE_FUNC) != 1))
  #error ST_POWER_MODE_FUNC must be either 0 or 1
#elif (((ST_POWER_MODE_FUNC) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_POWER_MODE_FUNC must be 0 when ST_USE_DEVMAN is 0
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Power mode states */
#define ST_POWER_STATE_NOT_DEFINED      0
#define ST_POWER_STATE_POWERED          1
#define ST_POWER_STATE_POWERING_UP      2
#define ST_POWER_STATE_SUSPENDING       3
#define ST_POWER_STATE_SUSPENDED        4

/* Device classes */
#define ST_DEV_CLASS_NULL               0x0000
#define ST_DEV_CLASS_VOLUME             0x0001


/****************************************************************************
 *
 *  I/O Control code definitions
 *
 ***************************************************************************/

/* System I/O control codes */
#define IOCTL_OS_GET_SIGNAL_STATE       0x0001
#define IOCTL_OS_WAIT_ACQUIRE           0x0002
#define IOCTL_OS_WAIT_BEGIN             0x0003
#define IOCTL_OS_WAIT_UPDATE            0x0004
#define IOCTL_OS_WAIT_FAILURE           0x0005

/* Driver I/O control codes */
#define IOCTL_DRV_INIT                  0x0020
#define IOCTL_DRV_DEINIT                0x0021
#define IOCTL_DRV_RUN                   0x0022
#define IOCTL_DRV_WORK_TIMEOUT          0x0023
#define IOCTL_DRV_DEVICE_ATTACH         0x0024
#define IOCTL_DRV_DEVICE_DETACH         0x0025
#define IOCTL_DRV_POWER_UP              0x0026
#define IOCTL_DRV_POWER_DOWN            0x0027

/* Device I/O control codes */
#define IOCTL_INIT                      0x0030
#define IOCTL_DEINIT                    0x0031
#define IOCTL_RECOVERY                  0x0032
#define IOCTL_READ                      0x0033
#define IOCTL_WRITE                     0x0034
#define IOCTL_SEEK                      0x0035
#define IOCTL_POWER_UP                  0x0036
#define IOCTL_POWER_DOWN                0x0037


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Device search context structure */
struct TDevSearch
{
  /* Private system information */
  INDEX SearchClass;

  /* Device information */
  INDEX Class;
  HANDLE Handle;
};


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_DEVMAN)

    void stDevManInit(void);

    #if (ST_DEVMAN_DEINIT_FUNC)
      void stDevManDeinit(void);
    #endif

    BOOL stDevManRun(void);

    HANDLE stCreateDriver(TDeviceIOCtl DeviceIOCtl, PVOID DriverContext);

    HANDLE stAttachDevice(HANDLE DriverHandle, INDEX Class,
      PVOID DeviceContext);
    BOOL stDetachDevice(HANDLE Handle);

    #if (ST_DEVICE_NOTIFY_FUNC)
      HANDLE stRegDevNotification(HANDLE DriverHandle, INDEX Class);
      BOOL stUnregDevNotification(HANDLE Handle);
    #endif

    #if (ST_FIND_DEVICE_FUNC)
      HANDLE stFindFirstDevice(struct TDevSearch *DevSearch, INDEX Class);
      HANDLE stFindNextDevice(struct TDevSearch *DevSearch);
    #endif

    #if (ST_POWER_MODE_FUNC)
      void stPowerUp(void);
      void stPowerDown(void);
      INDEX stPowerStatus(void);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_DEVMAN_H */
/***************************************************************************/
