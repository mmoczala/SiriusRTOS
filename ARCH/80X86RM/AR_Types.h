/****************************************************************************
 *
 *  SiriusRTOS
 *  AR_Types.h - Standard type definitions (80x86 Real Mode Port)
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef AR_TYPES_H
#define AR_TYPES_H
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

/* Disable 64-bit integers by default */
#ifndef AR_USE_64_BIT
  #define AR_USE_64_BIT                 0
#endif

/* Configure the CPU as Little Endian by default */
#ifndef AR_LENDIAN_CPU
  #define AR_LENDIAN_CPU                1
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* NULL pointer definition */
#ifndef NULL
  #ifdef __cplusplus
    #define NULL                        0UL
  #else
    #define NULL                        ((PVOID) 0UL)
  #endif
#endif

/* Boolean constants */
#define FALSE                           ((BOOL) 0)
#define TRUE                            ((BOOL) 1)

/* Index constants */
#define AR_UNDEFINED_INDEX              ((INDEX) 0xFFFF)

/* Time constants */
#define AR_TIME_IGNORE                  ((TIME) 0UL)
#define AR_TIME_INFINITE                ((TIME) 0xFFFFFFFFUL)

/* Memory alignment */
#define AR_MEMORY_ALIGNMENT             ((SIZE) 4UL)

/* Compiler-specific keywords */
#define INLINE
#define FAR                             __far
#define NEAR                            __near
#define CALLBACK                        __far __stdcall


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* 8-bit integer types */
typedef signed char INT8;
typedef unsigned char UINT8;

/* 16-bit integer types */
typedef signed int INT16;
typedef unsigned int UINT16;

/* 32-bit integer types */
typedef signed long INT32;
typedef unsigned long UINT32;

/* 64-bit integer types */
#if (AR_USE_64_BIT)
  typedef signed long long INT64;
  typedef unsigned long long UINT64;
#endif

/* Longest integer types */
#if (AR_USE_64_BIT)
  typedef INT64 LONG;
  typedef UINT64 ULONG;
#else
  typedef INT32 LONG;
  typedef UINT32 ULONG;
#endif

/* Extended types */
typedef int BOOL;
typedef UINT16 INDEX;
typedef UINT16 SIZE;
typedef UINT32 TIME;
typedef void FAR *PVOID;
typedef char FAR *PSTR;


/***************************************************************************/
#endif /* AR_TYPES_H */
/***************************************************************************/
