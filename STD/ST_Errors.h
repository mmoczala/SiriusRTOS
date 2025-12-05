/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Errors.h - System Error Code Definitions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_ERRORS_H
#define ST_ERRORS_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"


/****************************************************************************
 *
 *  Error code definitions
 *
 ***************************************************************************/

/* General error codes */
#define ERR_NO_ERROR                    ((ERROR) 0x0000UL)
#define ERR_INVALID_PARAMETER           ((ERROR) 0x0001UL)
#define ERR_NOT_IMPLEMENTED             ((ERROR) 0x0002UL)

/* Memory Management error codes */
#define ERR_NOT_ENOUGH_MEMORY           ((ERROR) 0x0010UL)
#define ERR_INVALID_MEMORY_BLOCK        ((ERROR) 0x0011UL)

/* Handle Management error codes */
#define ERR_CAN_NOT_ASSIGN_NEW_HANDLE   ((ERROR) 0x0020UL)
#define ERR_INVALID_HANDLE              ((ERROR) 0x0021UL)
#define ERR_NO_DEFINED_IO_CTL           ((ERROR) 0x0022UL)

/* Device Driver Management error codes */
#define ERR_DEVMAN_NOT_RUNNING          ((ERROR) 0x0030UL)
#define ERR_DEVMAN_ALREADY_RUNNING      ((ERROR) 0x0031UL)
#define ERR_DEVICE_NOT_FOUND            ((ERROR) 0x0032UL)
#define ERR_NOTIFY_ALREADY_USED         ((ERROR) 0x0033UL)

/* Architecture specific error codes */
#define ERR_CAN_NOT_INIT_ARCHITECTURE   ((ERROR) 0x0080UL)
#define ERR_CAN_NOT_SET_PREEMPT_HANDLER ((ERROR) 0x0081UL)
#define ERR_CAN_NOT_CREATE_TASK_CONTEXT ((ERROR) 0x0082UL)
#define ERR_CAN_NOT_REL_TASK_CONTEXT    ((ERROR) 0x0083UL)
#define ERR_TOO_SMALL_STACK_SIZE        ((ERROR) 0x0084UL)

/* Operating System error codes */
#define ERR_OS_ALREADY_RUNNING          ((ERROR) 0x0100UL)
#define ERR_OS_CAN_NOT_BE_RUNNING       ((ERROR) 0x0101UL)
#define ERR_WRONG_OS_FIXMEM_CONFIG      ((ERROR) 0x0102UL)
#define ERR_ALLOWED_ONLY_FOR_TASKS      ((ERROR) 0x0103UL)
#define ERR_OBJECT_ALREADY_EXISTS       ((ERROR) 0x0104UL)
#define ERR_OBJECT_CAN_NOT_BE_OPENED    ((ERROR) 0x0105UL)
#define ERR_OBJECT_CAN_NOT_BE_RELEASED  ((ERROR) 0x0106UL)
#define ERR_TASK_NOT_TERMINATED         ((ERROR) 0x0107UL)
#define ERR_TASK_TERMINATED_BY_OTHER    ((ERROR) 0x0108UL)
#define ERR_WAIT_TIMEOUT                ((ERROR) 0x0109UL)
#define ERR_WAIT_ABANDONED              ((ERROR) 0x010AUL)
#define ERR_WAIT_DEADLOCK               ((ERROR) 0x010BUL)
#define ERR_INVALID_DEVICE_IO_CTL       ((ERROR) 0x010CUL)
#define ERR_TIMER_NOT_STARTED           ((ERROR) 0x010DUL)
#define ERR_PTR_QUEUE_IS_FULL           ((ERROR) 0x010EUL)
#define ERR_PTR_QUEUE_IS_EMPTY          ((ERROR) 0x010FUL)
#define ERR_DATA_TRANSFER_FAILURE       ((ERROR) 0x0110UL)
#define ERR_STREAM_IS_FULL              ((ERROR) 0x0111UL)
#define ERR_STREAM_IS_EMPTY             ((ERROR) 0x0112UL)
#define ERR_QUEUE_IS_FULL               ((ERROR) 0x0113UL)
#define ERR_QUEUE_IS_EMPTY              ((ERROR) 0x0114UL)
#define ERR_MAILBOX_IS_EMPTY            ((ERROR) 0x0115UL)


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Error code type declaration */
typedef UINT16 ERROR;


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  INLINE void stSetLastError(ERROR ErrorCode);
  INLINE ERROR stGetLastError(void);

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_ERRORS_H */
/***************************************************************************/
