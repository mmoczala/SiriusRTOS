/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_CLIB.h - Standard C Library Abstraction
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_CLIB_H
#define ST_CLIB_H
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

/* Disable standard C library memory management (malloc/free) by default */
#ifndef ST_USE_MALLOC_CLIB
  #define ST_USE_MALLOC_CLIB            0
#endif

/* Disable standard C library memory block operations (memcpy, etc.) by 
   default */
#ifndef ST_USE_MEMORY_CLIB
  #define ST_USE_MEMORY_CLIB            0
#endif

/* Disable standard C library string operations (strcpy, etc.) by default */
#ifndef ST_USE_STRING_CLIB
  #define ST_USE_STRING_CLIB            0
#endif


/* Internal Memory Management Configuration */
#if !ST_USE_MALLOC_CLIB

  /* The internal memory module (ST_Memory) is required when the standard 
     CLIB allocator is disabled */
  #if !(ST_USE_MEMORY)
    #error ST_USE_MEMORY must be set to 1 when alternative C library \
    memory management functions are used
  #endif

  /* Enable static buffering for the memory pool */
  #ifndef ST_MALLOC_BUFFERED
    #define ST_MALLOC_BUFFERED          1
  #endif

  /* Ensure a base address is provided if static buffering is disabled */
  #ifndef ST_MALLOC_BASE_ADDRESS
    #if !(ST_MALLOC_BUFFERED)
      #error Memory base address must be specified in \
        ST_MALLOC_BASE_ADDRESS when buffered memory is not used
    #endif
  #endif

  /* Define the memory pool size.
     - Buffered: Defaults to 8 KB.
     - Unbuffered: The size must be explicitly specified by the user. */
  #ifndef ST_MALLOC_SIZE
    #if (ST_MALLOC_BUFFERED)
      #define ST_MALLOC_SIZE            ((SIZE) 8192UL)
    #else
      #error Memory size must be specified in ST_MALLOC_SIZE when \
        buffered memory is not used
    #endif
  #elif ((ST_MALLOC_SIZE) <= 0UL)
    #error Memory size must be greater than zero
  #endif

#endif


/****************************************************************************
 *
 *  Memory Allocation Wrapper
 *
 ***************************************************************************/

#if (ST_USE_MALLOC_CLIB)

  PVOID stMemAlloc(SIZE Size);
  BOOL stMemFree(PVOID Ptr);

#else

  #include "ST_Memory.h"

  #if (ST_MALLOC_BUFFERED)
    extern UINT8 stMemoryPool[ST_MALLOC_SIZE];
  #endif

  #define stMemAlloc(Size) stMemoryAlloc((PVOID) stMemoryPool, (SIZE) (Size))
  #define stMemFree(Ptr) stMemoryFree((PVOID) stMemoryPool, (PVOID) (Ptr))

#endif


/****************************************************************************
 *
 *  Memory Block Operations
 *
 ***************************************************************************/

#if (ST_USE_MEMORY_CLIB)

  #include <memory.h>

  #define stMemCmp memcmp
  #define stMemCpy memcpy
  #define stMemMove memmove
  #define stMemSet memset

#else

  #define stMemMove stMemCpy

  #ifdef __cplusplus
    extern "C" {
  #endif

    int stMemCmp(const PVOID s1, const PVOID s2, SIZE n);
    PVOID stMemCpy(PVOID dest, const PVOID src, SIZE n);
    PVOID stMemSet(PVOID s, UINT8 c, SIZE n);

  #ifdef __cplusplus
    };
  #endif

#endif


/****************************************************************************
 *
 *  String Operations
 *
 ***************************************************************************/

#if (ST_USE_STRING_CLIB)

  #include <string.h>

  #define stStrLen strlen

  #define stStrCpy strcpy
  #define stStrNCpy strncpy

  #define stStrCat strcat
  #define stStrNCat strncat

  #define stStrCmp strcmp
  #define stStrNCmp strncmp
  #define stStrICmp stricmp
  #define stStrNICmp strnicmp

  #define stStrUpr strupr

#else

  #ifdef __cplusplus
    extern "C" {
  #endif

    int stStrLen(const PSTR s);

    PSTR stStrCpy(PSTR dest, const PSTR src);
    PSTR stStrNCpy(PSTR dest, const PSTR src, SIZE maxlen);

    PSTR stStrCat(PSTR dest, const PSTR src);
    PSTR stStrNCat(PSTR dest, const PSTR src, SIZE maxlen);

    int stStrCmp(const PSTR s1, const PSTR s2);
    int stStrNCmp(const PSTR s1, const PSTR s2, SIZE maxlen);
    int stStrICmp(const PSTR s1, const PSTR s2);
    int stStrNICmp(const PSTR s1, const PSTR s2, SIZE maxlen);

    PSTR stStrUpr(PSTR s);

  #ifdef __cplusplus
    };
  #endif

#endif


/***************************************************************************/
#endif /* ST_CLIB_H */
/***************************************************************************/
