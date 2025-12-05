/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Task.h - Task management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_TASK_H
#define OS_TASK_H
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

/* Enable osGetTaskHandle by default */
#ifndef OS_GET_TASK_HANDLE_FUNC
  #define OS_GET_TASK_HANDLE_FUNC       1
#elif (((OS_GET_TASK_HANDLE_FUNC) != 0) && ((OS_GET_TASK_HANDLE_FUNC) != 1))
  #error OS_GET_TASK_HANDLE_FUNC must be either 0 or 1
#endif

/* Enable osGetTaskExitCode by default */
#ifndef OS_TASK_EXIT_CODE_FUNC
  #define OS_TASK_EXIT_CODE_FUNC        1
#elif (((OS_TASK_EXIT_CODE_FUNC) != 0) && ((OS_TASK_EXIT_CODE_FUNC) != 1))
  #error OS_TASK_EXIT_CODE_FUNC must be either 0 or 1
#endif

/* Enable osTerminateTask by default */
#ifndef OS_TERMINATE_TASK_FUNC
  #define OS_TERMINATE_TASK_FUNC        1
#elif (((OS_TERMINATE_TASK_FUNC) != 0) && ((OS_TERMINATE_TASK_FUNC) != 1))
  #error OS_TERMINATE_TASK_FUNC must be either 0 or 1
#endif

/* Enable osSuspendTask and osResumeTask by default */
#ifndef OS_SUSP_RES_TASK_FUNC
  #define OS_SUSP_RES_TASK_FUNC         1
#elif (((OS_SUSP_RES_TASK_FUNC) != 0) && ((OS_SUSP_RES_TASK_FUNC) != 1))
  #error OS_SUSP_RES_TASK_FUNC must be either 0 or 1
#endif

/* Enable osGetTaskPriority and osSetTaskPriority by default */
#ifndef OS_TASK_PRIORITY_FUNC
  #define OS_TASK_PRIORITY_FUNC         1
#elif (((OS_TASK_PRIORITY_FUNC) != 0) && ((OS_TASK_PRIORITY_FUNC) != 1))
  #error OS_TASK_PRIORITY_FUNC must be either 0 or 1
#endif

/* Enable osGetTaskQuantum and osSetTaskQuantum by default */
#ifndef OS_TASK_QUANTUM_FUNC
  #define OS_TASK_QUANTUM_FUNC          1
#elif (((OS_TASK_QUANTUM_FUNC) != 0) && ((OS_TASK_QUANTUM_FUNC) != 1))
  #error OS_TASK_QUANTUM_FUNC must be either 0 or 1
#endif

/* Enable osGetTaskStat by default */
#ifndef OS_GET_TASK_STAT_FUNC
  #define OS_GET_TASK_STAT_FUNC         1
#elif (((OS_GET_TASK_STAT_FUNC) != 0) && ((OS_GET_TASK_STAT_FUNC) != 1))
  #error OS_GET_TASK_STAT_FUNC must be either 0 or 1
#endif

/* Default task stack size is 512 bytes */
#ifndef OS_DEFAULT_TASK_STACK_SIZE
  #define OS_DEFAULT_TASK_STACK_SIZE    512UL
#elif ((OS_DEFAULT_TASK_STACK_SIZE) <= 0UL)
  #error OS_DEFAULT_TASK_STACK_SIZE must be greater than zero
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable modifiable task priority if the set function is used */
#if ((OS_TASK_PRIORITY_FUNC) && !defined(OS_MODIFIABLE_TASK_PRIO))
  #define OS_MODIFIABLE_TASK_PRIO       1
#endif

/* Enable time quanta if the quantum functions are used */
#if ((OS_TASK_QUANTUM_FUNC) && !defined(OS_USE_TIME_QUANTA))
  #define OS_USE_TIME_QUANTA            1
#endif

/* Enable task CPU usage statistics if osGetTaskStat is used */
#if ((OS_GET_TASK_STAT_FUNC) && !defined(OS_USE_STATISTICS))
  #define OS_USE_STATISTICS             1
#endif


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Task entry point procedure type */
typedef ERROR (* TTaskProc)(PVOID Arg);


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  HANDLE osCreateTask(TTaskProc TaskProc, PVOID Arg, SIZE StackSize,
    UINT8 Priority, BOOL Suspended);

  void osExitTask(ERROR ExitCode);

  #if (OS_GET_TASK_HANDLE_FUNC)
    HANDLE osGetTaskHandle(void);
  #endif

  #if (OS_TASK_EXIT_CODE_FUNC)
    BOOL osGetTaskExitCode(HANDLE Handle, ERROR *ExitCode);
  #endif

  #if (OS_TERMINATE_TASK_FUNC)
    BOOL osTerminateTask(HANDLE Handle);
  #endif

  #if (OS_SUSP_RES_TASK_FUNC)
    BOOL osSuspendTask(HANDLE Handle);
    BOOL osResumeTask(HANDLE Handle);
  #endif

  #if (OS_TASK_PRIORITY_FUNC)
    BOOL osGetTaskPriority(HANDLE Handle, UINT8 *Priority);
    BOOL osSetTaskPriority(HANDLE Handle, UINT8 Priority);
  #endif

  #if (OS_TASK_QUANTUM_FUNC)
    BOOL osGetTaskQuantum(HANDLE Handle, UINT8 *Quantum);
    BOOL osSetTaskQuantum(HANDLE Handle, UINT8 Quantum);
  #endif

  #if (OS_GET_TASK_STAT_FUNC)
    BOOL osGetTaskStat(HANDLE Handle, INDEX *CPUTime, INDEX *TotalTime);
  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_TASK_H */
/***************************************************************************/
