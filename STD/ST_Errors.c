/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Errors.c - Error Code Management
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

#if (OS_USED)
  #include "OS_API.h"
#endif


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

#if !(OS_USED)
  static ERROR stLastErrorCode;
#endif


/****************************************************************************
 *
 *  Name:
 *    stSetLastError
 *
 *  Description:
 *    Sets the last error code for the current execution context.
 *    If the Operating System is enabled, this delegates to the OS task
 *    storage; otherwise, it uses a global variable (bare-metal mode).
 *
 *  Parameters:
 *    ErrorCode - The error code to set.
 *
 ***************************************************************************/

INLINE void stSetLastError(ERROR ErrorCode)
{
  /* Store the error code based on the system configuration */
  #if (OS_USED)
    osSetLastError(ErrorCode);
  #else
    stLastErrorCode = ErrorCode;
  #endif
}


/****************************************************************************
 *
 *  Name:
 *    stGetLastError
 *
 *  Description:
 *    Retrieves the last error code set by the current execution context.
 *
 *  Return:
 *    The last recorded error code.
 *
 ***************************************************************************/

INLINE ERROR stGetLastError(void)
{
  /* Retrieve the error code based on the system configuration */
  #if (OS_USED)
    return osGetLastError();
  #else
    return stLastErrorCode;
  #endif
}


/***************************************************************************/
