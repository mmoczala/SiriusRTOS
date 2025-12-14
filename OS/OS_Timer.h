/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Timer.h - Timer object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_TIMER_H
#define OS_TIMER_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "OS_API.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable Timer object management by default */
#ifndef OS_USE_TIMER
  #define OS_USE_TIMER                  1
#elif (((OS_USE_TIMER) != 0) && ((OS_USE_TIMER) != 1))
  #error OS_USE_TIMER must be either 0 or 1
#endif

/* Enable osOpenTimer (lookup by name) by default */
#ifndef OS_OPEN_TIMER_FUNC
  #define OS_OPEN_TIMER_FUNC            (OS_USE_TIMER)
#elif (((OS_OPEN_TIMER_FUNC) != 0) && ((OS_OPEN_TIMER_FUNC) != 1))
  #error OS_OPEN_TIMER_FUNC must be either 0 or 1
#elif (((OS_OPEN_TIMER_FUNC) != 0) && !(OS_USE_TIMER))
  #error OS_OPEN_TIMER_FUNC must be 0 when OS_USE_TIMER is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_TIMER_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable time-managed object support (Kernel time tick integration) */
#if ((OS_USE_TIMER) && !defined(OS_USE_TIME_OBJECTS))
  #define OS_USE_TIME_OBJECTS           1
#endif

/* Enable System I/O Control codes */
#if ((OS_USE_TIMER) && !defined(OS_OBJ_USES_IO_SIGNAL))
  #define OS_USE_SYSTEM_IO_CTRL         1
#endif

/* Enable Device I/O Control function */
#if ((OS_USE_TIMER) && !defined(OS_USE_DEVICE_IO_CTRL))
  #define OS_USE_DEVICE_IO_CTRL         1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_TIMER            6


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_TIMER)

    HANDLE osCreateTimer(SYSNAME Name, BOOL ManualReset);

    #if (OS_OPEN_TIMER_FUNC)
      HANDLE osOpenTimer(SYSNAME Name);
    #endif

    BOOL osSetTimer(HANDLE Handle, TIME Interval, INDEX PassCount);
    BOOL osResetTimer(HANDLE Handle);
    BOOL osCancelTimer(HANDLE Handle);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_TIMER_H */
/***************************************************************************/
