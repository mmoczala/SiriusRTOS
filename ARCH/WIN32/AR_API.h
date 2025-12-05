/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_API.h - Architecture API
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef AR_API_H
#define AR_API_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "Config.h"
#include "AR_Types.h"
#include "AR_WinIntf.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable arDeinit function by default */
#ifndef AR_USE_DEINIT
  #define AR_USE_DEINIT                 1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Defines the resolution of the system tick counter (ticks per second) */
#define AR_TICKS_PER_SECOND             1000UL


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Function callbacks */
typedef void (CALLBACK * TPreemptiveProc)(struct TTaskContext
  FAR *TaskContext);
  
typedef void (CALLBACK * TTaskStartupProc)(void);


/****************************************************************************
 *
 *  Macros
 *
 ***************************************************************************/

/* Marks the specified parameter as unused to suppress compiler warnings */
#define AR_UNUSED_PARAM(Param)          ((void) Param)

/* Aligns the specified size upward to the nearest alignment boundary */
#define AR_MEMORY_ALIGN_UP(Value) ((SIZE) \
  (((Value) + ((AR_MEMORY_ALIGNMENT) - 1)) & ~((AR_MEMORY_ALIGNMENT) - 1)))


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  BOOL arInit(void);

  #if (AR_USE_DEINIT)
    void arDeinit(void);
  #endif

  BOOL arLock(void);
  void arRestore(BOOL PreviousLockState);

  TIME arGetTickCount(void);

  BOOL arSetPreemptiveHandler(TPreemptiveProc PreemptiveProc,
    SIZE StackSize);

  void arYield(void);

  BOOL arCreateTaskContext(struct TTaskContext FAR *TaskContext,
    TTaskStartupProc TaskStartupProc, SIZE StackSize);
  BOOL arReleaseTaskContext(struct TTaskContext FAR *TaskContext);

  void arSavePower(void);

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* AR_API_H */
/***************************************************************************/
