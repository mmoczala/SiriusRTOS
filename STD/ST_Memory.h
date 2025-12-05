/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Memory.h - Memory Management
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_MEMORY_H
#define ST_MEMORY_H
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

/* Enable Memory Management functions by default */
#ifndef ST_USE_MEMORY
  #define ST_USE_MEMORY                 1
#endif

/* Enable safe stMemoryFree execution when invalid pointers are specified */
#ifndef ST_USE_SAFE_MEMORY_FREE
  #define ST_USE_SAFE_MEMORY_FREE       1
#endif

/* Enable stMemGetInfo function by default */
#ifndef ST_GET_MEMORY_INFO_FUNC
  #define ST_GET_MEMORY_INFO_FUNC       1
#endif

/* Enable stMemExpand function by default */
#ifndef ST_MEMORY_EXPAND_FUNC
  #define ST_MEMORY_EXPAND_FUNC         1
#endif

/* Binary Search Trees must be enabled for Memory Management */
#if ((ST_USE_MEMORY) && (!ST_USE_BSTREE))
  #error ST_USE_BSTREE must be set to 1 for Memory Management
#endif

/* The stBSTreeExchange function must be enabled for Memory Management */
#if ((ST_USE_MEMORY) && (!ST_BSTREE_EXCHANGE_FUNC))
  #error ST_BSTREE_EXCHANGE_FUNC must be set to 1 for Memory Management
#endif


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_MEMORY)

    BOOL stMemoryInit(PVOID MemoryPool, SIZE MemorySize);

    PVOID stMemoryAlloc(PVOID MemoryPool, SIZE Size);
    BOOL stMemoryFree(PVOID MemoryPool, PVOID Ptr);

    #if (ST_GET_MEMORY_INFO_FUNC)
      void stMemoryGetInfo(PVOID MemoryPool, ULONG *TotalMemory,
        ULONG *FreeMemory);
    #endif

    #if (ST_MEMORY_EXPAND_FUNC)
      BOOL stMemoryExpand(PVOID MemoryPool, PVOID MemoryAddress,
        SIZE MemorySize);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_MEMORY_H */
/***************************************************************************/
