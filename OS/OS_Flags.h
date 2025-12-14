/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Flags.h - Flags object management functions
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_FLAGS_H
#define OS_FLAGS_H
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

/* Enable Flag objects by default */
#ifndef OS_USE_FLAGS
  #define OS_USE_FLAGS                  1
#elif (((OS_USE_FLAGS) != 0) && ((OS_USE_FLAGS) != 1))
  #error OS_USE_FLAGS must be either 0 or 1
#endif

/* Enable osOpenFlags (lookup by name) by default */
#ifndef OS_OPEN_FLAGS_FUNC
  #define OS_OPEN_FLAGS_FUNC            (OS_USE_FLAGS)
#elif (((OS_OPEN_FLAGS_FUNC) != 0) && ((OS_OPEN_FLAGS_FUNC) != 1))
  #error OS_OPEN_FLAGS_FUNC must be either 0 or 1
#elif (((OS_OPEN_FLAGS_FUNC) != 0) && !(OS_USE_FLAGS))
  #error OS_OPEN_FLAGS_FUNC must be 0 when OS_USE_FLAGS is 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Enable named object support if open function is used */
#if ((OS_OPEN_FLAGS_FUNC) && !defined(OS_USE_OBJECT_NAMES))
  #define OS_USE_OBJECT_NAMES           1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define OS_OBJECT_TYPE_FLAGS            12


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (OS_USE_FLAGS)

    HANDLE osCreateFlags(SYSNAME Name, INDEX InitialState);

    #if (OS_OPEN_FLAGS_FUNC)
      HANDLE osOpenFlags(SYSNAME Name);
    #endif

    BOOL osGetFlags(HANDLE Handle, INDEX *State);
    BOOL osSetFlags(HANDLE Handle, INDEX Mask, INDEX *Changed);
    BOOL osResetFlags(HANDLE Handle, INDEX Mask, INDEX *Changed);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_FLAGS_H */
/***************************************************************************/
