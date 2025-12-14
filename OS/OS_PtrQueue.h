/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_PtrQueue.h - Pointer queue object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_PTR_QUEUE_H
#define OS_PTR_QUEUE_H
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

/* Enable Pointer Queue objects by default */
#ifndef OS_USE_PTR_QUEUE
  #define OS_USE_PTR_QUEUE              1
#elif (((OS_USE_PTR_QUEUE) != 0) && ((OS_USE_PTR_QUEUE) != 1))
  #error OS_USE_PTR_QUEUE must be either 0 or 1
#endif

/* Enable osOpenPtrQueue (lookup by name) by default */
#ifndef OS_OPEN_PTR_QUEUE_FUNC
  #define OS_OPEN_PTR_QUEUE_FUNC        (OS_USE_PTR_QUEUE)
#elif (((OS_OPEN_PTR_QUEUE_FUNC) != 0) && ((OS_OPEN_PTR_QUEUE_FUNC) != 1))
  #error OS_OPEN_PTR_QUEUE_FUNC must be either 0 or 1
#elif (((OS_OPEN_PTR_QUEUE_FUNC) != 0) && !(OS_USE_PTR_QUEUE))
  #error OS_OPEN_PTR_QUEUE_FUNC must be 0 when OS_USE_PTR_QUEUE is 0
#endif

/* Enable osPtrQueuePeek by default */
#ifndef OS_PTR_QUEUE_PEEK_FUNC
  #define OS_PTR_QUEUE_PEEK_FUNC        (OS_USE_PTR_QUEUE)
#elif (((OS_PTR_QUEUE_PEEK_FUNC) != 0) && ((OS_PTR_QUEUE_PEEK_FUNC) != 1))
  #error OS_PTR_QUEUE_PEEK_FUNC must be either 0 or 1
#elif (((OS_PTR_QUEUE_PEEK_FUNC) != 0) && !(OS_USE_PTR_QUEUE))
  #error OS_PTR_QUEUE_PEEK_FUNC must be 0 when OS_USE_PTR_QUEUE is 0
#endif

/* Enable osClearPtrQueue by default */
#ifndef OS_PTR_QUEUE_CLEAR_FUNC
  #define OS_PTR_QUEUE_CLEAR_FUNC       (OS_USE_PTR_QUEUE)
#elif (((OS_PTR_QUEUE_CLEAR_FUNC) != 0) && ((OS_PTR_QUEUE_CLEAR_FUNC) != 1))
  #error OS_PTR_QUEUE_CLEAR_FUNC must be either 0 or 1
#elif (((OS_PTR_QUEUE_CLEAR_FUNC) != 0) && !(OS_USE_PTR_QUEUE))
  #error OS_PTR_QUEUE_CLEAR_FUNC must be 0 when OS_USE_PTR_QUEUE is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_PTR_QUEUE_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_PTR_QUEUE        8


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_PTR_QUEUE)

    HANDLE osCreatePtrQueue(SYSNAME Name, INDEX MaxCount);

    #if (OS_OPEN_PTR_QUEUE_FUNC)
      HANDLE osOpenPtrQueue(SYSNAME Name);
    #endif

    BOOL osPtrQueuePost(HANDLE Handle, PVOID Ptr);
    BOOL osPtrQueuePend(HANDLE Handle, PVOID *Ptr);

    #if (OS_PTR_QUEUE_PEEK_FUNC)
      BOOL osPtrQueuePeek(HANDLE Handle, PVOID *Ptr);
    #endif

    #if (OS_PTR_QUEUE_CLEAR_FUNC)
      BOOL osClearPtrQueue(HANDLE Handle);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_PTR_QUEUE_H */
/***************************************************************************/
