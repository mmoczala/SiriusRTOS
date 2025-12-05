/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Mailbox.h - Mailbox object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_MAILBOX_H
#define OS_MAILBOX_H
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

/* Enable Mailbox objects by default */
#ifndef OS_USE_MAILBOX
  #define OS_USE_MAILBOX                1
#elif (((OS_USE_MAILBOX) != 0) && ((OS_USE_MAILBOX) != 1))
  #error OS_USE_MAILBOX must be either 0 or 1
#endif

/* Enable osOpenMailbox (lookup by name) by default */
#ifndef OS_OPEN_MBOX_FUNC
  #define OS_OPEN_MBOX_FUNC             (OS_USE_MAILBOX)
#elif (((OS_OPEN_MBOX_FUNC) != 0) && ((OS_OPEN_MBOX_FUNC) != 1))
  #error OS_OPEN_MBOX_FUNC must be either 0 or 1
#elif (((OS_OPEN_MBOX_FUNC) != 0) && !(OS_USE_MAILBOX))
  #error OS_OPEN_MBOX_FUNC must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable osMailboxPost and osMailboxPend by default */
#ifndef OS_MBOX_POST_PEND_FUNC
  #define OS_MBOX_POST_PEND_FUNC        (OS_USE_MAILBOX)
#elif (((OS_MBOX_POST_PEND_FUNC) != 0) && ((OS_MBOX_POST_PEND_FUNC) != 1))
  #error OS_MBOX_POST_PEND_FUNC must be either 0 or 1
#elif (((OS_MBOX_POST_PEND_FUNC) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_POST_PEND_FUNC must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable osMailboxPeek by default */
#ifndef OS_MBOX_PEEK_FUNC
  #define OS_MBOX_PEEK_FUNC             (OS_USE_MAILBOX)
#elif (((OS_MBOX_PEEK_FUNC) != 0) && ((OS_MBOX_PEEK_FUNC) != 1))
  #error OS_MBOX_PEEK_FUNC must be either 0 or 1
#elif (((OS_MBOX_PEEK_FUNC) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_PEEK_FUNC must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable osClearMailbox by default */
#ifndef OS_MBOX_CLEAR_FUNC
  #define OS_MBOX_CLEAR_FUNC            (OS_USE_MAILBOX)
#elif (((OS_MBOX_CLEAR_FUNC) != 0) && ((OS_MBOX_CLEAR_FUNC) != 1))
  #error OS_MBOX_CLEAR_FUNC must be either 0 or 1
#elif (((OS_MBOX_CLEAR_FUNC) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_CLEAR_FUNC must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable osGetMailboxInfo by default */
#ifndef OS_MBOX_GET_INFO_FUNC
  #define OS_MBOX_GET_INFO_FUNC         (OS_USE_MAILBOX)
#elif (((OS_MBOX_GET_INFO_FUNC) != 0) && ((OS_MBOX_GET_INFO_FUNC) != 1))
  #error OS_MBOX_GET_INFO_FUNC must be either 0 or 1
#elif (((OS_MBOX_GET_INFO_FUNC) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_GET_INFO_FUNC must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable Mailbox protection by auto-reset event */
#ifndef OS_MBOX_PROTECT_EVENT
  #define OS_MBOX_PROTECT_EVENT         (OS_MBOX_PEEK_FUNC)
#elif (((OS_MBOX_PROTECT_EVENT) != 0) && ((OS_MBOX_PROTECT_EVENT) != 1))
  #error OS_MBOX_PROTECT_EVENT must be either 0 or 1
#elif (((OS_MBOX_PROTECT_EVENT) != 0) && !(OS_MBOX_PEEK_FUNC))
  #error OS_MBOX_PROTECT_EVENT must be 0 when OS_MBOX_PEEK_FUNC is 0
#elif (((OS_MBOX_PROTECT_EVENT) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_PROTECT_EVENT must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable Mailbox protection by mutex */
#ifndef OS_MBOX_PROTECT_MUTEX
  #define OS_MBOX_PROTECT_MUTEX         (OS_MBOX_PEEK_FUNC)
#elif (((OS_MBOX_PROTECT_MUTEX) != 0) && ((OS_MBOX_PROTECT_MUTEX) != 1))
  #error OS_MBOX_PROTECT_MUTEX must be either 0 or 1
#elif (((OS_MBOX_PROTECT_MUTEX) != 0) && !(OS_MBOX_PEEK_FUNC))
  #error OS_MBOX_PROTECT_MUTEX must be 0 when OS_MBOX_PEEK_FUNC is 0
#elif (((OS_MBOX_PROTECT_MUTEX) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_PROTECT_MUTEX must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable Mailbox waiting for read completion (Blocking Read) */
#ifndef OS_MBOX_ALLOW_WAIT_IF_EMPTY
  #define OS_MBOX_ALLOW_WAIT_IF_EMPTY   (OS_USE_MAILBOX)
#elif (((OS_MBOX_ALLOW_WAIT_IF_EMPTY) != 0) && \
  ((OS_MBOX_ALLOW_WAIT_IF_EMPTY) != 1))
  #error OS_MBOX_ALLOW_WAIT_IF_EMPTY must be either 0 or 1
#elif (((OS_MBOX_ALLOW_WAIT_IF_EMPTY) != 0) && !(OS_USE_MAILBOX))
  #error OS_MBOX_ALLOW_WAIT_IF_EMPTY must be 0 when OS_USE_MAILBOX is 0
#endif

/* Enable Mailbox direct read-write method (Zero-Copy IPC) */
#ifndef OS_MBOX_ALLOW_DIRECT_RW
  #define OS_MBOX_ALLOW_DIRECT_RW       (OS_USE_MAILBOX)
#elif (((OS_MBOX_ALLOW_DIRECT_RW) != 0) && ((OS_MBOX_ALLOW_DIRECT_RW) != 1))
  #error OS_MBOX_ALLOW_DIRECT_RW must be either 0 or 1
#elif (((OS_MBOX_ALLOW_DIRECT_RW) != 0) && !(OS_MBOX_ALLOW_WAIT_IF_EMPTY))
  #error OS_MBOX_ALLOW_DIRECT_RW requires OS_MBOX_ALLOW_WAIT_IF_EMPTY \
    to be enabled
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_MBOX_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Device I/O Control function */
#if ((OS_USE_MAILBOX) && !defined(OS_USE_DEVICE_IO_CTRL))
  #define OS_USE_DEVICE_IO_CTRL         1
#endif

/* Enable Critical Section objects if Mutex protection is used */
#if ((OS_USE_MAILBOX) && !defined(OS_USE_CSEC_OBJECTS) && \
  (OS_MBOX_PROTECT_MUTEX))
  #define OS_USE_CSEC_OBJECTS           1
#endif

/* Enable Multiple Signals support if complex synchronization is used */
#if ((OS_USE_MAILBOX) && !defined(OS_USE_MULTIPLE_SIGNALS) && \
  ((OS_MBOX_PROTECT_EVENT) || (OS_MBOX_PROTECT_MUTEX) || \
  (OS_MBOX_ALLOW_WAIT_IF_EMPTY)))
  #define OS_USE_MULTIPLE_SIGNALS       1
#endif

/* Enable Direct Read-Write IPC feature */
#if ((OS_USE_MAILBOX) && !defined(OS_USE_IPC_DIRECT_RW) && \
  (OS_MBOX_ALLOW_DIRECT_RW))
  #define OS_USE_IPC_DIRECT_RW          1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_MAILBOX          11


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_MAILBOX)

    HANDLE osCreateMailbox(SYSNAME Name, UINT8 Mode);

    #if (OS_OPEN_MBOX_FUNC)
      HANDLE osOpenMailbox(SYSNAME Name);
    #endif

    #if (OS_MBOX_POST_PEND_FUNC)
      SIZE osMailboxPost(HANDLE Handle, PVOID Buffer, SIZE Size);
      SIZE osMailboxPend(HANDLE Handle, PVOID Buffer, SIZE Size);
    #endif

    #if (OS_MBOX_PEEK_FUNC)
      SIZE osMailboxPeek(HANDLE Handle, PVOID Buffer, SIZE Size);
    #endif

    #if (OS_MBOX_CLEAR_FUNC)
      BOOL osClearMailbox(HANDLE Handle);
    #endif

    #if (OS_MBOX_GET_INFO_FUNC)
      BOOL osGetMailboxInfo(HANDLE Handle, SIZE *NextSize, INDEX *Count);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_MAILBOX_H */
/***************************************************************************/
