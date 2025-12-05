/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_CountSem.h - Counting semaphore object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_COUNT_SEM_H
#define OS_COUNT_SEM_H
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

/* Enable Counting Semaphore objects by default */
#ifndef OS_USE_CNT_SEM
  #define OS_USE_CNT_SEM                1
#elif (((OS_USE_CNT_SEM) != 0) && ((OS_USE_CNT_SEM) != 1))
  #error OS_USE_CNT_SEM must be either 0 or 1
#endif

/* Enable osOpenCountSem (lookup by name) by default */
#ifndef OS_OPEN_CNT_SEM_FUNC
  #define OS_OPEN_CNT_SEM_FUNC          (OS_USE_CNT_SEM)
#elif (((OS_OPEN_CNT_SEM_FUNC) != 0) && ((OS_OPEN_CNT_SEM_FUNC) != 1))
  #error OS_OPEN_CNT_SEM_FUNC must be either 0 or 1
#elif (((OS_OPEN_CNT_SEM_FUNC) != 0) && !(OS_USE_CNT_SEM))
  #error OS_OPEN_CNT_SEM_FUNC must be 0 when OS_USE_CNT_SEM is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_CNT_SEM_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_COUNT_SEM        4


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_CNT_SEM)

    HANDLE osCreateCountSem(SYSNAME Name, INDEX InitialCount,
      INDEX MaxCount);

    #if (OS_OPEN_CNT_SEM_FUNC)
      HANDLE osOpenCountSem(SYSNAME Name);
    #endif

    BOOL osReleaseCountSem(HANDLE Handle, INDEX ReleaseCount,
      INDEX *PrevCount);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_COUNT_SEM_H */
/***************************************************************************/
