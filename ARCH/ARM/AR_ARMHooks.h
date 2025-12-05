/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_ARMHooks.h - ARM Architecture Hooks
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef AR_ARM_HOOKS_H
#define AR_ARM_HOOKS_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "Config.h"
#include "AR_Types.h"


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Defines the resolution of the system tick counter (ticks per second) */
#define AR_TICKS_PER_SECOND             1000UL

/* Memory alignment definition */
#define AR_MEMORY_ALIGNMENT             ((SIZE) 4UL)


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  BOOL arARMInitHook(void (* IRQHandler)(void));
  void arARMDeinitHook(void);

  TIME arGetTickCountHook(void);
  void arIRQHandlerHook(void);
  void arSavePowerHook(void);

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* AR_ARM_HOOKS_H */
/***************************************************************************/
