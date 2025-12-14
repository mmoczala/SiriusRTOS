/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Handle.h - Handle management
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_HANDLE_H
#define ST_HANDLE_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable handle management functionality by default */
#ifndef ST_USE_HANDLE
  #define ST_USE_HANDLE                 1
#endif

/* Maximum number of handles that can be allocated */
#ifndef ST_MAX_HANDLE_COUNT
  #define ST_MAX_HANDLE_COUNT           10UL
#elif ((ST_MAX_HANDLE_COUNT) < 0L)
  #error ST_MAX_HANDLE_COUNT must be greater than or equal to zero
#endif

/* Enable the handle owner counter if an operating system is present */
#ifndef ST_USE_OWNER_COUNTER
  #ifdef OS_ALLOW_OBJECT_DELETION
    #define ST_USE_OWNER_COUNTER        (OS_ALLOW_OBJECT_DELETION)
  #elif defined(OS_USED)
    #define ST_USE_OWNER_COUNTER        (OS_USED)
  #endif
#elif (((ST_USE_OWNER_COUNTER) != 0) && ((ST_USE_OWNER_COUNTER) != 1))
  #error Value of ST_USE_OWNER_COUNTER must be 0 or 1
#endif

/* Enable the stIOCtrl function by default */
#ifndef ST_IO_CTL_FUNC
  #define ST_IO_CTL_FUNC                1
#elif (((ST_IO_CTL_FUNC) != 0) && ((ST_IO_CTL_FUNC) != 1))
  #error ST_IO_CTL_FUNC must be either 0 or 1
#elif (((ST_IO_CTL_FUNC) != 0) && ((ST_USE_DEVMAN) != 1))
  #error ST_IO_CTL_FUNC must be 0 when ST_USE_DEVMAN is 0
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* NULL handle definition */
#if ((ST_MAX_HANDLE_COUNT) == 0UL)
  #define NULL_HANDLE                   NULL
#else
  #define NULL_HANDLE                   ((HANDLE) 0UL)
#endif

/* Predefined handle types */
#define ST_HANDLE_TYPE_IGNORE           0x40
#define ST_HANDLE_TYPE_DRIVER           0x00
#define ST_HANDLE_TYPE_DEVICE           0x01
#define ST_HANDLE_TYPE_NOTIFY           0x02
#define ST_HANDLE_TYPE_TASK             0x03
#define ST_HANDLE_TYPE_MUTEX            0x04
#define ST_HANDLE_TYPE_SEMAPHORE        0x05
#define ST_HANDLE_TYPE_EVENT            0x06
#define ST_HANDLE_TYPE_COUNT_SEM        0x07
#define ST_HANDLE_TYPE_TIMER            0x08
#define ST_HANDLE_TYPE_SHARED_MEM       0x09
#define ST_HANDLE_TYPE_PTR_QUEUE        0x0A
#define ST_HANDLE_TYPE_STREAM           0x0B
#define ST_HANDLE_TYPE_QUEUE            0x0C
#define ST_HANDLE_TYPE_MAILBOX          0x0D
#define ST_HANDLE_TYPE_FLAGS            0x0E


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Handle type definition */
#if ((ST_MAX_HANDLE_COUNT) == 0UL)
  typedef PVOID HANDLE;
#elif ((ST_MAX_HANDLE_COUNT) < 65535UL)
  typedef UINT16 HANDLE;
#else
  typedef UINT32 HANDLE;
#endif

/* Device I/O control function pointer type */
typedef INDEX (FAR * TDeviceIOCtl)(HANDLE Handle, INDEX IOCtlCode,
  PVOID Buffer, SIZE Size);

/* Handle descriptor structure */
struct THandle
{
  UINT8 Flags;

  #if ((ST_MAX_HANDLE_COUNT) > 0UL)
    PVOID Object;
  #endif

  #if (ST_USE_OWNER_COUNTER)
    INDEX OwnerCount;
  #endif

  #if (ST_IO_CTL_FUNC)
    TDeviceIOCtl DeviceIOCtl;
  #endif
};


/****************************************************************************
 *
 * Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_HANDLE)

    #if ((ST_MAX_HANDLE_COUNT) > 0UL)
      void stHandleInit(void);
    #endif

    struct THandle FAR * stHandleAlloc(HANDLE FAR *Handle, PVOID Object,
      SIZE ObjectSize, UINT8 Type);

    BOOL stHandleRelease(HANDLE Handle);

    struct THandle FAR * stGetHandleInfo(HANDLE Handle, PVOID *Object,
      UINT8 Type);

    #if (ST_IO_CTL_FUNC)
      INDEX stIOCtrl(HANDLE Handle, INDEX IOCtl, PVOID Buffer,
        SIZE BufferSize);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_HANDLE_H */
/***************************************************************************/
