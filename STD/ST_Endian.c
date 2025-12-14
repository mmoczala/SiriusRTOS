/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Endian.c - Endianness Conversion Implementation
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "ST_API.h"


/***************************************************************************/
#if (ST_USE_ENDIANS)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stSwap16
 *
 *  Description:
 *    Swaps the byte order of a 16-bit value (Little Endian <-> Big Endian).
 *
 *  Parameters:
 *    Value - The 16-bit input value to convert.
 *
 *  Return:
 *    The value with swapped byte order.
 *
 ***************************************************************************/

INLINE UINT16 stSwap16(UINT16 Value)
{
  return (UINT16) ((Value >> 8) | (Value << 8));
}


/****************************************************************************
 *
 *  Name:
 *    stSwap32
 *
 *  Description:
 *    Swaps the byte order of a 32-bit value (Little Endian <-> Big Endian).
 *
 *  Parameters:
 *    Value - The 32-bit input value to convert.
 *
 *  Return:
 *    The value with swapped byte order.
 *
 ***************************************************************************/

INLINE UINT32 stSwap32(UINT32 Value)
{
  return (UINT32) ((Value >> 24) | (Value << 24) |
    ((Value >> 8) & 0x0000FF00UL) | ((Value << 8) & 0x00FF0000UL));
}


/***************************************************************************/
#if (AR_USE_64_BIT)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stSwap64
 *
 *  Description:
 *    Swaps the byte order of a 64-bit value (Little Endian <-> Big Endian).
 *
 *  Parameters:
 *    Value - The 64-bit input value to convert.
 *
 *  Return:
 *    The value with swapped byte order.
 *
 ***************************************************************************/

INLINE UINT64 stSwap64(UINT64 Value)
{
  register UINT8 tmp;
  register UINT8 NEAR *ptr = (UINT8 NEAR *) &Value;

  tmp = ptr[0];  ptr[0] = ptr[7];  ptr[7] = tmp;
  tmp = ptr[1];  ptr[1] = ptr[6];  ptr[6] = tmp;
  tmp = ptr[2];  ptr[2] = ptr[5];  ptr[5] = tmp;
  tmp = ptr[3];  ptr[3] = ptr[4];  ptr[4] = tmp;

  return Value;
}


/***************************************************************************/
#endif /* AR_USE_64_BIT */
/***************************************************************************/


/***************************************************************************/
#if (ST_IS_LENDIAN_CPU_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stIsLEndianCPU
 *
 *  Description:
 *    Determines the endianness of the host CPU at runtime.
 *
 *  Return:
 *    TRUE if the CPU architecture is Little Endian, FALSE otherwise.
 *
 ***************************************************************************/

INLINE BOOL stIsLEndianCPU(void)
{
  volatile UINT16 Data = 0x0001;
  return (BOOL) (*(UINT8 NEAR *) &Data);
}


/***************************************************************************/
#endif /* ST_IS_LENDIAN_CPU_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* ST_USE_ENDIANS */
/***************************************************************************/


/***************************************************************************/
