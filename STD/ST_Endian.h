/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Endian.h - Endianness Conversion Support
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_ENDIAN_H
#define ST_ENDIAN_H
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

/* Default to Little Endian CPU architecture */
#ifndef AR_LENDIAN_CPU
  #define AR_LENDIAN_CPU                1
#endif

/* Enable endianness conversion macros by default */
#ifndef ST_USE_ENDIANS
  #define ST_USE_ENDIANS                1
#endif

/* Disable runtime endianness check (stIsLEndianCPU) by default */
#ifndef ST_IS_LENDIAN_CPU_FUNC
  #define ST_IS_LENDIAN_CPU_FUNC        0
#endif


/****************************************************************************
 *
 *  Macros
 *
 ***************************************************************************/

#if (ST_USE_ENDIANS)

  /* Endianness conversion macros */
  #if (AR_LENDIAN_CPU)

    /* Little Endian CPU: No conversion for LE data, Swap for BE data */
    #define stLEToCPU16(Value)            ((UINT16) (Value))
    #define stBEToCPU16(Value)            stSwap16((UINT16) (Value))

    #define stLEToCPU32(Value)            ((UINT32) (Value))
    #define stBEToCPU32(Value)            stSwap32((UINT32) (Value))

    #if (AR_USE_64_BIT)
      #define stLEToCPU64(Value)          ((UINT64) (Value))
      #define stBEToCPU64(Value)          stSwap64((UINT64) (Value))
    #endif

  #else

    /* Big Endian CPU: Swap for LE data, No conversion for BE data */
    #define stLEToCPU16(Value)            stSwap16((UINT16) (Value))
    #define stBEToCPU16(Value)            ((UINT16) (Value))

    #define stLEToCPU32(Value)            stSwap32((UINT32) (Value))
    #define stBEToCPU32(Value)            ((UINT32) (Value))

    #if (AR_USE_64_BIT)
      #define stLEToCPU64(Value)          stSwap64((UINT64) (Value))
      #define stBEToCPU64(Value)          ((UINT64) (Value))
    #endif

  #endif

  /* CPU-to-Protocol definitions (symmetric to Protocol-to-CPU) */
  #define stCPUToLE16                     stLEToCPU16
  #define stCPUToBE16                     stBEToCPU16
  #define stCPUToLE32                     stLEToCPU32
  #define stCPUToBE32                     stBEToCPU32

  #if (AR_USE_64_BIT)
    #define stCPUToLE64                   stLEToCPU64
    #define stCPUToBE64                   stBEToCPU64
  #endif

#endif


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_ENDIANS)

    INLINE UINT16 stSwap16(UINT16 Value);
    INLINE UINT32 stSwap32(UINT32 Value);

    #if (AR_USE_64_BIT)
      INLINE UINT64 stSwap64(UINT64 Value);
    #endif

    #if (ST_IS_LENDIAN_CPU_FUNC)
      INLINE BOOL stIsLEndianCPU(void);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_ENDIAN_H */
/***************************************************************************/
