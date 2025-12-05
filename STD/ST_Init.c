/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Init.c - Standard Library initialization
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


/****************************************************************************
 *
 *  Name:
 *    stInit
 *
 *  Description:
 *    Initializes the standard library.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stInit(void)
{
  /* Initialize the error code management module */
  stSetLastError(ERR_NO_ERROR);

  /* Initialize the Handle Management module */
  #if ((ST_MAX_HANDLE_COUNT) > 0UL)
    stHandleInit();
  #endif

  /* Initialize the alternative C library memory manager */
  #if !ST_USE_MALLOC_CLIB
    #if ST_MALLOC_BUFFERED
      return stMemoryInit(stMemoryPool, ST_MALLOC_SIZE);
    #else
      return stMemoryInit(ST_MALLOC_BASE_ADDRESS, ST_MALLOC_SIZE);
    #endif
  #endif

  return TRUE;
}


/***************************************************************************/
