/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_WinIntf.h - Windows Interface (Win32 Port)
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef AR_WIN_INTF_H
#define AR_WIN_INTF_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "Config.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Defines the context switch interval in milliseconds. Default value: 1 ms */
#ifndef AR_WIN32_CTX_SWITCH_INTERVAL
  #define AR_WIN32_CTX_SWITCH_INTERVAL  1
#elif (AR_WIN32_CTX_SWITCH_INTERVAL) < 1
  #error AR_WIN32_CTX_SWITCH_INTERVAL must be greater than zero
#endif


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

struct TTaskContext;

/* Function callbacks */
typedef void (* TWin32Proc)(struct TTaskContext *TaskContext);

/* Task context structure */
struct TTaskContext
{
  void *Arg;
  TWin32Proc TaskStartupProc;

  int InterruptEnable;
  int DelayedContextSwitch;

  void *hTask;
  void *YieldCtrl;
};


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  int arWin32Init(void);
  void arWin32Deinit(void);

  int arWin32Lock(void);
  void arWin32Restore(int PrevLockState);

  unsigned long arWin32GetTickCount(void);

  int arWin32SetPreemptiveHandler(TWin32Proc PreemptiveProc,
    unsigned long StackSize);
  void arWin32Yield(void);

  int arWin32CreateTaskContext(struct TTaskContext *TaskContext,
    TWin32Proc TaskStartupProc, unsigned long StackSize);
  int arWin32ReleaseTaskContext(struct TTaskContext *TaskContext);

  void arWin32SavePower(void);

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* AR_WIN_INTF_H */
/***************************************************************************/
