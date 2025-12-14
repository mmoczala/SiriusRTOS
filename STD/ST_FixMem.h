/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_FixMem.h - Fixed Size Memory Management
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_FIXMEM_H
#define ST_FIXMEM_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable Fixed Size Memory Management functions by default */
#ifndef ST_USE_FIXMEM
  #define ST_USE_FIXMEM                 1
#endif


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_FIXMEM)

    BOOL stFixedMemInit(PVOID MemoryAddress, SIZE MemorySize,
      SIZE BlockSize);

    PVOID stFixedMemAlloc(PVOID MemoryAddress);
    BOOL stFixedMemFree(PVOID MemoryAddress, PVOID Address);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_FIXMEM_H */
/***************************************************************************/
