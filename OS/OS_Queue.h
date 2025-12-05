/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Queue.h - Queue object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_QUEUE_H
#define OS_QUEUE_H
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

/* Enable Queue object management by default */
#ifndef OS_USE_QUEUE
  #define OS_USE_QUEUE                  1
#elif (((OS_USE_QUEUE) != 0) && ((OS_USE_QUEUE) != 1))
  #error OS_USE_QUEUE must be either 0 or 1
#endif

/* Enable osOpenQueue (lookup by name) by default */
#ifndef OS_OPEN_QUEUE_FUNC
  #define OS_OPEN_QUEUE_FUNC            (OS_USE_QUEUE)
#elif (((OS_OPEN_QUEUE_FUNC) != 0) && ((OS_OPEN_QUEUE_FUNC) != 1))
  #error OS_OPEN_QUEUE_FUNC must be either 0 or 1
#elif (((OS_OPEN_QUEUE_FUNC) != 0) && !(OS_USE_QUEUE))
  #error OS_OPEN_QUEUE_FUNC must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable osQueuePost and osQueuePend by default */
#ifndef OS_QUEUE_POST_PEND_FUNC
  #define OS_QUEUE_POST_PEND_FUNC       (OS_USE_QUEUE)
#elif (((OS_QUEUE_POST_PEND_FUNC) != 0) && ((OS_QUEUE_POST_PEND_FUNC) != 1))
  #error OS_QUEUE_POST_PEND_FUNC must be either 0 or 1
#elif (((OS_QUEUE_POST_PEND_FUNC) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_POST_PEND_FUNC must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable osQueuePeek by default */
#ifndef OS_QUEUE_PEEK_FUNC
  #define OS_QUEUE_PEEK_FUNC            (OS_USE_QUEUE)
#elif (((OS_QUEUE_PEEK_FUNC) != 0) && ((OS_QUEUE_PEEK_FUNC) != 1))
  #error OS_QUEUE_PEEK_FUNC must be either 0 or 1
#elif (((OS_QUEUE_PEEK_FUNC) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_PEEK_FUNC must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable osClearQueue by default */
#ifndef OS_QUEUE_CLEAR_FUNC
  #define OS_QUEUE_CLEAR_FUNC           (OS_USE_QUEUE)
#elif (((OS_QUEUE_CLEAR_FUNC) != 0) && ((OS_QUEUE_CLEAR_FUNC) != 1))
  #error OS_QUEUE_CLEAR_FUNC must be either 0 or 1
#elif (((OS_QUEUE_CLEAR_FUNC) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_CLEAR_FUNC must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable Queue protection by auto-reset event */
#ifndef OS_QUEUE_PROTECT_EVENT
  #define OS_QUEUE_PROTECT_EVENT        (OS_USE_QUEUE)
#elif (((OS_QUEUE_PROTECT_EVENT) != 0) && ((OS_QUEUE_PROTECT_EVENT) != 1))
  #error OS_QUEUE_PROTECT_EVENT must be either 0 or 1
#elif (((OS_QUEUE_PROTECT_EVENT) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_PROTECT_EVENT must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable Queue protection by mutex */
#ifndef OS_QUEUE_PROTECT_MUTEX
  #define OS_QUEUE_PROTECT_MUTEX        (OS_USE_QUEUE)
#elif (((OS_QUEUE_PROTECT_MUTEX) != 0) && ((OS_QUEUE_PROTECT_MUTEX) != 1))
  #error OS_QUEUE_PROTECT_MUTEX must be either 0 or 1
#elif (((OS_QUEUE_PROTECT_MUTEX) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_PROTECT_MUTEX must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable Queue waiting for read completion (Blocking Read) */
#ifndef OS_QUEUE_ALLOW_WAIT_IF_EMPTY
  #define OS_QUEUE_ALLOW_WAIT_IF_EMPTY  (OS_USE_QUEUE)
#elif (((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) != 0) && \
  ((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) != 1))
  #error OS_QUEUE_ALLOW_WAIT_IF_EMPTY must be either 0 or 1
#elif (((OS_QUEUE_ALLOW_WAIT_IF_EMPTY) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_ALLOW_WAIT_IF_EMPTY must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable Queue waiting for write completion (Blocking Write) */
#ifndef OS_QUEUE_ALLOW_WAIT_IF_FULL
  #define OS_QUEUE_ALLOW_WAIT_IF_FULL   (OS_USE_QUEUE)
#elif (((OS_QUEUE_ALLOW_WAIT_IF_FULL) != 0) && \
  ((OS_QUEUE_ALLOW_WAIT_IF_FULL) != 1))
  #error OS_QUEUE_ALLOW_WAIT_IF_FULL must be either 0 or 1
#elif (((OS_QUEUE_ALLOW_WAIT_IF_FULL) != 0) && !(OS_USE_QUEUE))
  #error OS_QUEUE_ALLOW_WAIT_IF_FULL must be 0 when OS_USE_QUEUE is 0
#endif

/* Enable Queue direct read-write method (Zero-Copy IPC) */
#ifndef OS_QUEUE_ALLOW_DIRECT_RW
  #define OS_QUEUE_ALLOW_DIRECT_RW      (OS_USE_QUEUE)
#elif (((OS_QUEUE_ALLOW_DIRECT_RW) != 0) && \
  ((OS_QUEUE_ALLOW_DIRECT_RW) != 1))
  #error OS_QUEUE_ALLOW_DIRECT_RW must be either 0 or 1
#elif (((OS_QUEUE_ALLOW_DIRECT_RW) != 0) && \
  (!(OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || !(OS_QUEUE_ALLOW_WAIT_IF_FULL)))
  #error OS_QUEUE_ALLOW_DIRECT_RW requires blocking read and write \
    (WAIT_IF_EMPTY and WAIT_IF_FULL) to be enabled
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_QUEUE_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Device I/O Control function */
#if ((OS_USE_QUEUE) && !defined(OS_USE_DEVICE_IO_CTRL))
  #define OS_USE_DEVICE_IO_CTRL         1
#endif

/* Enable Critical Section objects if Mutex protection is used */
#if ((OS_USE_QUEUE) && !defined(OS_USE_CSEC_OBJECTS) && \
  (OS_QUEUE_PROTECT_MUTEX))
  #define OS_USE_CSEC_OBJECTS           1
#endif

/* Enable Multiple Signals support if complex synchronization is used */
#if ((OS_USE_QUEUE) && !defined(OS_USE_MULTIPLE_SIGNALS) && \
  ((OS_QUEUE_PROTECT_EVENT) || (OS_QUEUE_PROTECT_MUTEX) || \
  (OS_QUEUE_ALLOW_WAIT_IF_EMPTY) || (OS_QUEUE_ALLOW_WAIT_IF_FULL)))
  #define OS_USE_MULTIPLE_SIGNALS       1
#endif

/* Enable Direct Read-Write IPC feature */
#if ((OS_USE_QUEUE) && !defined(OS_USE_IPC_DIRECT_RW) && \
  (OS_QUEUE_ALLOW_DIRECT_RW))
  #define OS_USE_IPC_DIRECT_RW          1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_QUEUE            10


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_QUEUE)

    HANDLE osCreateQueue(SYSNAME Name, UINT8 Mode, INDEX MaxCount,
      SIZE MessageSize);

    #if (OS_OPEN_QUEUE_FUNC)
      HANDLE osOpenQueue(SYSNAME Name);
    #endif

    #if (OS_QUEUE_POST_PEND_FUNC)
      BOOL osQueuePost(HANDLE Handle, PVOID Buffer);
      BOOL osQueuePend(HANDLE Handle, PVOID Buffer);
    #endif

    #if (OS_QUEUE_PEEK_FUNC)
      BOOL osQueuePeek(HANDLE Handle, PVOID Buffer);
    #endif

    #if (OS_QUEUE_CLEAR_FUNC)
      BOOL osClearQueue(HANDLE Handle);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_QUEUE_H */
/***************************************************************************/
