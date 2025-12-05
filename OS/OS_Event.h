/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Event.h - Event object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_EVENT_H
#define OS_EVENT_H
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

/* Enable Event object management by default */
#ifndef OS_USE_EVENT
  #define OS_USE_EVENT                  1
#elif (((OS_USE_EVENT) != 0) && ((OS_USE_EVENT) != 1))
  #error OS_USE_EVENT must be either 0 or 1
#endif

/* Enable osOpenEvent (lookup by name) by default */
#ifndef OS_OPEN_EVENT_FUNC
  #define OS_OPEN_EVENT_FUNC            (OS_USE_EVENT)
#elif (((OS_OPEN_EVENT_FUNC) != 0) && ((OS_OPEN_EVENT_FUNC) != 1))
  #error OS_OPEN_EVENT_FUNC must be either 0 or 1
#elif (((OS_OPEN_EVENT_FUNC) != 0) && !(OS_USE_EVENT))
  #error OS_OPEN_EVENT_FUNC must be 0 when OS_USE_EVENT is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_EVENT_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_EVENT            5


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_EVENT)

    HANDLE osCreateEvent(SYSNAME Name, BOOL InitialState, BOOL ManualReset);

    #if (OS_OPEN_EVENT_FUNC)
      HANDLE osOpenEvent(SYSNAME Name);
    #endif

    BOOL osSetEvent(HANDLE Handle);
    BOOL osResetEvent(HANDLE Handle);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_EVENT_H */
/***************************************************************************/
