/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_API.h - Standard library API
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_API_H
#define ST_API_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

/* Global configuration settings */
#include "Config.h"

/* Platform-dependent abstraction layer */
#include "AR_API.h"

/* Standard library modules */
#include "ST_BSTree.h"
#include "ST_PQueue.h"
#include "ST_Endian.h"
#include "ST_Errors.h"
#include "ST_Handle.h"
#include "ST_DevMan.h"
#include "ST_Memory.h"
#include "ST_FixMem.h"
#include "ST_CLIB.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Operating System is disabled by default */
#ifndef OS_USED
  #define OS_USED                       0
#endif


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  BOOL stInit(void);

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_API_H */
/***************************************************************************/
