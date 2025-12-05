/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_API.h - Operating System API
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_API_H
#define OS_API_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

/* Global configuration */
#include "Config.h"

/* Architecture dependent code */
#include "AR_API.h"

/* Standard libraries */
#include "ST_API.h"

/* Tasks */
#include "OS_Task.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable system object deletion by default. When disabled, features
   responsible for object opening, closing, and deletion are excluded from
   compilation to reduce code size. */
#ifndef OS_ALLOW_OBJECT_DELETION
  #define OS_ALLOW_OBJECT_DELETION      1
#elif (((OS_ALLOW_OBJECT_DELETION) != 0) && \
  ((OS_ALLOW_OBJECT_DELETION) != 1))
  #error OS_ALLOW_OBJECT_DELETION must be either 0 or 1
#endif

/* Maximum length of the system object name. If greater than zero, the
   name is a string with the specified maximum length. If zero, integers
   are used as names. By default, 8-character strings are used. */
#ifndef OS_SYS_OBJECT_MAX_NAME_LEN
  #define OS_SYS_OBJECT_MAX_NAME_LEN    8UL
#elif ((OS_SYS_OBJECT_MAX_NAME_LEN) < 0L)
  #error OS_SYS_OBJECT_MAX_NAME_LEN must be greater or equal to 0
#endif

/* Enable osOpenByHandle by default when object deletion is enabled.
   This function is only necessary when objects can be deleted. */
#ifndef OS_OPEN_BY_HANDLE_FUNC
  #define OS_OPEN_BY_HANDLE_FUNC        (OS_ALLOW_OBJECT_DELETION)
#elif (((OS_OPEN_BY_HANDLE_FUNC) != 0) && ((OS_OPEN_BY_HANDLE_FUNC) != 1))
  #error OS_OPEN_BY_HANDLE_FUNC must be either 0 or 1
#elif ((OS_OPEN_BY_HANDLE_FUNC) && !(OS_ALLOW_OBJECT_DELETION))
  #error OS_OPEN_BY_HANDLE_FUNC cannot be enabled when \
    OS_ALLOW_OBJECT_DELETION is 0
#endif

/* Maximum number of objects that a task can await simultaneously.
   When set to 1, only osWaitForObject is available. When greater,
   osWaitForObjects is also enabled. Default is 1. */
#ifndef OS_MAX_WAIT_FOR_OBJECTS
  #define OS_MAX_WAIT_FOR_OBJECTS       1UL
#elif ((OS_MAX_WAIT_FOR_OBJECTS) < 1UL)
  #error OS_MAX_WAIT_FOR_OBJECTS must be greater than zero
#endif

/* Enable osRead and osWrite functions by default */
#ifndef OS_READ_WRITE_FUNC
  #define OS_READ_WRITE_FUNC            1
#elif (((OS_READ_WRITE_FUNC) != 0) && ((OS_READ_WRITE_FUNC) != 1))
  #error OS_READ_WRITE_FUNC must be either 0 or 1
#endif

/* Enable osStop function by default */
#ifndef OS_STOP_FUNC
  #define OS_STOP_FUNC                  1
#elif (((OS_STOP_FUNC) != 0) && ((OS_STOP_FUNC) != 1))
  #error OS_STOP_FUNC must be either 0 or 1
#endif

/* Enable osDeinit function by default when object deletion and stop
   functions are enabled. */
#ifndef OS_DEINIT_FUNC
  #define OS_DEINIT_FUNC                ((OS_ALLOW_OBJECT_DELETION) && \
                                        (OS_STOP_FUNC))
#elif (((OS_DEINIT_FUNC) != 0) && ((OS_DEINIT_FUNC) != 1))
  #error OS_DEINIT_FUNC must be either 0 or 1
#elif (OS_DEINIT_FUNC)
  #if !(OS_STOP_FUNC)
    #error OS_DEINIT_FUNC cannot be enabled when OS_STOP_FUNC is 0
  #elif !(OS_ALLOW_OBJECT_DELETION)
    #error OS_DEINIT_FUNC cannot be enabled when OS_ALLOW_OBJECT_DELETION \
      is 0
  #endif
#endif

/* Enable osGetSystemStat function by default */
#ifndef OS_GET_SYSTEM_STAT_FUNC
  #define OS_GET_SYSTEM_STAT_FUNC       1
#elif (((OS_GET_SYSTEM_STAT_FUNC) != 0) && ((OS_GET_SYSTEM_STAT_FUNC) != 1))
  #error OS_GET_SYSTEM_STAT_FUNC must be either 0 or 1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Time constants */
#define OS_IGNORE                       AR_TIME_IGNORE
#define OS_INFINITE                     AR_TIME_INFINITE

/* IPC mode flags */
#define OS_IPC_PROTECTION_MASK          0x03
#define OS_IPC_PROTECT_INT_CTRL         0x00
#define OS_IPC_PROTECT_EVENT            0x01
#define OS_IPC_PROTECT_MUTEX            0x02
#define OS_IPC_WAIT_IF_EMPTY            0x04
#define OS_IPC_WAIT_IF_FULL             0x08
#define OS_IPC_DIRECT_READ_WRITE        0x10


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Object name type definition */
#if ((OS_SYS_OBJECT_MAX_NAME_LEN) <= 0UL)
  typedef INDEX SYSNAME;
#else
  typedef PSTR SYSNAME;
#endif

/* IO request information structure */
struct TIORequest
{
  TIME Timeout;
  SIZE NumberOfBytesTransferred;
};


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  BOOL osInit(void);

  #if (OS_DEINIT_FUNC)
    BOOL osDeinit(void);
  #endif

  BOOL osStart(void);

  #if (OS_STOP_FUNC)
    void osStop(void);
  #endif

  INLINE BOOL osEnterISR(void);
  INLINE void osLeaveISR(BOOL PrevISRState);

  void osSetLastError(ERROR ErrorCode);
  ERROR osGetLastError(void);

  #if (OS_GET_SYSTEM_STAT_FUNC)
    void osGetSystemStat(INDEX *CPUTime, INDEX *TotalTime);
  #endif

  #if (OS_OPEN_BY_HANDLE_FUNC)
    BOOL osOpenByHandle(HANDLE Handle);
  #endif

  #if (OS_ALLOW_OBJECT_DELETION)
    BOOL osCloseHandle(HANDLE Handle);
  #endif

  BOOL osSleep(TIME Time);
  BOOL osWaitForObject(HANDLE Handle, TIME Timeout);

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    BOOL osWaitForObjects(HANDLE *Handles, INDEX Count, TIME Timeout,
      INDEX *ObjectIndex);
  #endif

  #if (OS_READ_WRITE_FUNC)
    BOOL osRead(HANDLE Handle, PVOID Buffer, SIZE Size,
      struct TIORequest *IORequest);
    BOOL osWrite(HANDLE Handle, PVOID Buffer, SIZE Size,
      struct TIORequest *IORequest);
  #endif

#ifdef __cplusplus
  };
#endif


/****************************************************************************
 *
 *  Includes - System objects
 *
 ***************************************************************************/

/* Synchronization */
#include "OS_Mutex.h"
#include "OS_Semaphore.h"
#include "OS_CountSem.h"
#include "OS_Event.h"
#include "OS_Timer.h"

/* Interprocess communication */
#include "OS_SharedMem.h"
#include "OS_PtrQueue.h"
#include "OS_Stream.h"
#include "OS_Queue.h"
#include "OS_Mailbox.h"
#include "OS_Flags.h"


/***************************************************************************/
#endif /* OS_API_H */
/***************************************************************************/
