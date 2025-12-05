/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Mutex.h - Mutual exclusion semaphore object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_MUTEX_H
#define OS_MUTEX_H
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

/* Enable Mutex objects by default */
#ifndef OS_USE_MUTEX
  #define OS_USE_MUTEX                  1
#elif (((OS_USE_MUTEX) != 0) && ((OS_USE_MUTEX) != 1))
  #error OS_USE_MUTEX must be either 0 or 1
#endif

/* Enable osOpenMutex (lookup by name) by default */
#ifndef OS_OPEN_MUTEX_FUNC
  #define OS_OPEN_MUTEX_FUNC            (OS_USE_MUTEX)
#elif (((OS_OPEN_MUTEX_FUNC) != 0) && ((OS_OPEN_MUTEX_FUNC) != 1))
  #error OS_OPEN_MUTEX_FUNC must be either 0 or 1
#elif (((OS_OPEN_MUTEX_FUNC) != 0) && !(OS_USE_MUTEX))
  #error OS_OPEN_MUTEX_FUNC must be 0 when OS_USE_MUTEX is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_MUTEX_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Critical Section objects if Mutexes are used */
#if ((OS_USE_MUTEX) && !defined(OS_USE_CSEC_OBJECTS))
  #define OS_USE_CSEC_OBJECTS           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_MUTEX            2


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_MUTEX)

    HANDLE osCreateMutex(SYSNAME Name, BOOL InitialOwner);

    #if (OS_OPEN_MUTEX_FUNC)
      HANDLE osOpenMutex(SYSNAME Name);
    #endif

    BOOL osReleaseMutex(HANDLE Handle);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_MUTEX_H */
/***************************************************************************/
