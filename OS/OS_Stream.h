/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Stream.h - Stream object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_STREAM_H
#define OS_STREAM_H
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

/* Enable Stream object management by default */
#ifndef OS_USE_STREAM
  #define OS_USE_STREAM                 1
#elif (((OS_USE_STREAM) != 0) && ((OS_USE_STREAM) != 1))
  #error OS_USE_STREAM must be either 0 or 1
#endif

/* Enable osOpenStream (lookup by name) by default */
#ifndef OS_OPEN_STREAM_FUNC
  #define OS_OPEN_STREAM_FUNC           (OS_USE_STREAM)
#elif (((OS_OPEN_STREAM_FUNC) != 0) && ((OS_OPEN_STREAM_FUNC) != 1))
  #error OS_OPEN_STREAM_FUNC must be either 0 or 1
#elif (((OS_OPEN_STREAM_FUNC) != 0) && !(OS_USE_STREAM))
  #error OS_OPEN_STREAM_FUNC must be 0 when OS_USE_STREAM is 0
#endif

/* Enable stream protection by auto-reset event */
#ifndef OS_STREAM_PROTECT_EVENT
  #define OS_STREAM_PROTECT_EVENT       (OS_USE_STREAM)
#elif (((OS_STREAM_PROTECT_EVENT) != 0) && ((OS_STREAM_PROTECT_EVENT) != 1))
  #error OS_STREAM_PROTECT_EVENT must be either 0 or 1
#elif (((OS_STREAM_PROTECT_EVENT) != 0) && !(OS_USE_STREAM))
  #error OS_STREAM_PROTECT_EVENT must be 0 when OS_USE_STREAM is 0
#endif

/* Enable stream protection by mutex */
#ifndef OS_STREAM_PROTECT_MUTEX
  #define OS_STREAM_PROTECT_MUTEX       (OS_USE_STREAM)
#elif (((OS_STREAM_PROTECT_MUTEX) != 0) && ((OS_STREAM_PROTECT_MUTEX) != 1))
  #error OS_STREAM_PROTECT_MUTEX must be either 0 or 1
#elif (((OS_STREAM_PROTECT_MUTEX) != 0) && !(OS_USE_STREAM))
  #error OS_STREAM_PROTECT_MUTEX must be 0 when OS_USE_STREAM is 0
#endif

/* Enable stream waiting for read completion (Blocking Read) */
#ifndef OS_STREAM_ALLOW_WAIT_IF_EMPTY
  #define OS_STREAM_ALLOW_WAIT_IF_EMPTY (OS_USE_STREAM)
#elif (((OS_STREAM_ALLOW_WAIT_IF_EMPTY) != 0) && \
  ((OS_STREAM_ALLOW_WAIT_IF_EMPTY) != 1))
  #error OS_STREAM_ALLOW_WAIT_IF_EMPTY must be either 0 or 1
#elif (((OS_STREAM_ALLOW_WAIT_IF_EMPTY) != 0) && !(OS_USE_STREAM))
  #error OS_STREAM_ALLOW_WAIT_IF_EMPTY must be 0 when OS_USE_STREAM is 0
#endif

/* Enable stream waiting for write completion (Blocking Write) */
#ifndef OS_STREAM_ALLOW_WAIT_IF_FULL
  #define OS_STREAM_ALLOW_WAIT_IF_FULL  (OS_USE_STREAM)
#elif (((OS_STREAM_ALLOW_WAIT_IF_FULL) != 0) && \
  ((OS_STREAM_ALLOW_WAIT_IF_FULL) != 1))
  #error OS_STREAM_ALLOW_WAIT_IF_FULL must be either 0 or 1
#elif (((OS_STREAM_ALLOW_WAIT_IF_FULL) != 0) && !(OS_USE_STREAM))
  #error OS_STREAM_ALLOW_WAIT_IF_FULL must be 0 when OS_USE_STREAM is 0
#endif

/* Enable stream direct read-write method (Zero-Copy IPC) */
#ifndef OS_STREAM_ALLOW_DIRECT_RW
  #define OS_STREAM_ALLOW_DIRECT_RW     (OS_USE_STREAM)
#elif (((OS_STREAM_ALLOW_DIRECT_RW) != 0) && \
  ((OS_STREAM_ALLOW_DIRECT_RW) != 1))
  #error OS_STREAM_ALLOW_DIRECT_RW must be either 0 or 1
#elif (((OS_STREAM_ALLOW_DIRECT_RW) != 0) && \
  (!(OS_STREAM_ALLOW_WAIT_IF_EMPTY) || !(OS_STREAM_ALLOW_WAIT_IF_FULL)))
  #error OS_STREAM_ALLOW_DIRECT_RW requires blocking read and write \
    (WAIT_IF_EMPTY and WAIT_IF_FULL) to be enabled
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_STREAM_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Device I/O Control function */
#if ((OS_USE_STREAM) && !defined(OS_USE_DEVICE_IO_CTRL))
  #define OS_USE_DEVICE_IO_CTRL         1
#endif

/* Enable Critical Section objects if Mutex protection is used */
#if ((OS_USE_STREAM) && !defined(OS_USE_CSEC_OBJECTS) && \
  (OS_STREAM_PROTECT_MUTEX))
  #define OS_USE_CSEC_OBJECTS           1
#endif

/* Enable Multiple Signals support if complex synchronization is used */
#if ((OS_USE_STREAM) && !defined(OS_USE_MULTIPLE_SIGNALS) && \
  ((OS_STREAM_PROTECT_EVENT) || (OS_STREAM_PROTECT_MUTEX) || \
  (OS_STREAM_ALLOW_WAIT_IF_EMPTY) || (OS_STREAM_ALLOW_WAIT_IF_FULL)))
  #define OS_USE_MULTIPLE_SIGNALS       1
#endif

/* Enable Direct Read-Write IPC feature */
#if ((OS_USE_STREAM) && !defined(OS_USE_IPC_DIRECT_RW) && \
  (OS_STREAM_ALLOW_DIRECT_RW))
  #define OS_USE_IPC_DIRECT_RW          1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_STREAM           9


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_STREAM)

    HANDLE osCreateStream(SYSNAME Name, UINT8 Mode, SIZE BufferSize);

    #if (OS_OPEN_STREAM_FUNC)
      HANDLE osOpenStream(SYSNAME Name);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_STREAM_H */
/***************************************************************************/
