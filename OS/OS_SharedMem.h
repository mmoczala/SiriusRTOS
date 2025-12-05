/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_SharedMem.h - Shared Memory object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_SHARED_MEM_H
#define OS_SHARED_MEM_H
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

/* Enable Shared Memory objects by default */
#ifndef OS_USE_SHARED_MEM
  #define OS_USE_SHARED_MEM             1
#elif (((OS_USE_SHARED_MEM) != 0) && ((OS_USE_SHARED_MEM) != 1))
  #error OS_USE_SHARED_MEM must be either 0 or 1
#endif

/* Enable osOpenSharedMemory (lookup by name) by default */
#ifndef OS_OPEN_SH_MEM_FUNC
  #define OS_OPEN_SH_MEM_FUNC           (OS_USE_SHARED_MEM)
#elif (((OS_OPEN_SH_MEM_FUNC) != 0) && ((OS_OPEN_SH_MEM_FUNC) != 1))
  #error OS_OPEN_SH_MEM_FUNC must be either 0 or 1
#elif (((OS_OPEN_SH_MEM_FUNC) != 0) && !(OS_USE_SHARED_MEM))
  #error OS_OPEN_SH_MEM_FUNC must be 0 when OS_USE_SHARED_MEM is 0
#endif

/* Enable osGetSharedMemoryAddress by default */
#ifndef OS_GET_SH_MEM_ADDRESS_FUNC
  #define OS_GET_SH_MEM_ADDRESS_FUNC    (OS_USE_SHARED_MEM)
#elif (!OS_USE_SHARED_MEM)
  #error OS_USE_SHARED_MEM must be set to 1 when \
     osGetSharedMemoryAddress is enabled
#elif (((OS_GET_SH_MEM_ADDRESS_FUNC) != 0) && \
  ((OS_GET_SH_MEM_ADDRESS_FUNC) != 1))
  #error OS_GET_SH_MEM_ADDRESS_FUNC must be either 0 or 1
#endif

/* Enable Shared memory protection by mutex by default */
#ifndef OS_SH_MEM_PROTECT_MUTEX
  #define OS_SH_MEM_PROTECT_MUTEX       (OS_USE_SHARED_MEM)
#elif (((OS_SH_MEM_PROTECT_MUTEX) != 0) && ((OS_SH_MEM_PROTECT_MUTEX) != 1))
  #error OS_SH_MEM_PROTECT_MUTEX must be either 0 or 1
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_SH_MEM_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif

/* Enable Critical Section objects if Mutex protection is used */
#if ((OS_USE_SHARED_MEM) && (OS_SH_MEM_PROTECT_MUTEX) && \
  !defined(OS_USE_CSEC_OBJECTS))
  #define OS_USE_CSEC_OBJECTS           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_SHARED_MEM       7


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_SHARED_MEM)

    HANDLE osCreateSharedMemory(SYSNAME Name, UINT8 Mode, PVOID *Address,
      ULONG Size);

    #if (OS_OPEN_SH_MEM_FUNC)
      HANDLE osOpenSharedMemory(SYSNAME Name, PVOID *Address);
    #endif

    #if (OS_GET_SH_MEM_ADDRESS_FUNC)
      PVOID osGetSharedMemoryAddress(HANDLE Handle);
    #endif

    BOOL osReleaseSharedMemory(HANDLE Handle);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_SHARED_MEM_H */
/***************************************************************************/
