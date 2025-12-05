/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Semaphore.h - Semaphore object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_SEMAPHORE_H
#define OS_SEMAPHORE_H
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

/* Enable Semaphore objects by default */
#ifndef OS_USE_SEMAPHORE
  #define OS_USE_SEMAPHORE              1
#elif (((OS_USE_SEMAPHORE) != 0) && ((OS_USE_SEMAPHORE) != 1))
  #error OS_USE_SEMAPHORE must be either 0 or 1
#endif

/* Enable osOpenSemaphore (lookup by name) by default */
#ifndef OS_OPEN_SEMAPHORE_FUNC
  #define OS_OPEN_SEMAPHORE_FUNC        (OS_USE_SEMAPHORE)
#elif (((OS_OPEN_SEMAPHORE_FUNC) != 0) && ((OS_OPEN_SEMAPHORE_FUNC) != 1))
  #error OS_OPEN_SEMAPHORE_FUNC must be either 0 or 1
#elif (((OS_OPEN_SEMAPHORE_FUNC) != 0) && !(OS_USE_SEMAPHORE))
  #error OS_OPEN_SEMAPHORE_FUNC must be 0 when OS_USE_SEMAPHORE is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_SEMAPHORE_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Critical Section objects if Semaphores are used */
#if ((OS_USE_SEMAPHORE) && !defined(OS_USE_CSEC_OBJECTS))
  #define OS_USE_CSEC_OBJECTS           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_SEMAPHORE        3


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_SEMAPHORE)

    HANDLE osCreateSemaphore(SYSNAME Name, INDEX InitialCount,
      INDEX MaxCount);

    #if (OS_OPEN_SEMAPHORE_FUNC)
      HANDLE osOpenSemaphore(SYSNAME Name);
    #endif

    BOOL osReleaseSemaphore(HANDLE Handle, INDEX ReleaseCount,
      INDEX *PrevCount);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_SEMAPHORE_H */
/***************************************************************************/
