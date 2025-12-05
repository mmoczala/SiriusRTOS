/****************************************************************************
 *
 *  SiriusRTOS
 *  Main.c - Application Entry Point (Example)
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

#include "OS_API.h"


/****************************************************************************
 *
 *  Application Entry Point
 *
 ***************************************************************************/

int main(void)
{
  /* Hardware Abstraction Layer Initialization */
  arInit();

  /* Standard Library Initialization */
  stInit();

  /* Operating System Initialization */
  osInit();

  /* ... Application Code ... */

  /* Perform System Shutdown */
  osDeinit();
  arDeinit();
  return 0;
}


/***************************************************************************/
