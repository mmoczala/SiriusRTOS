/****************************************************************************
 *
 *  SiriusRTOS
 *  AT91Init.c - AT91SAM7S Low-level CPU Initialization
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

#include "AT91SAM7S64.h"


/****************************************************************************
 *
 *  Function prototypes
 *
 ***************************************************************************/

void cpLowLevelInit(void);


/****************************************************************************
 *
 *  Name:
 *    cpLowLevelInit
 *
 *  Description:
 *    Performs low-level hardware initialization.
 *    Configures the Watchdog, Flash wait states, Main Oscillator, PLL,
 *    and Master Clock (MCK).
 *
 ***************************************************************************/

void cpLowLevelInit(void)
{
  /* Disable the Watchdog Timer */
  *AT91C_WDTC_WDMR = AT91C_WDTC_WDDIS;

  /* Configure Flash Mode: 1 Wait State (2 cycles) and FMCN setup */
  *AT91C_MC_FMR = AT91C_MC_FWS_1FWS | (0x32 << 16);

  /* Enable the Main Oscillator and set startup time (48 slow clock cycles) */
  *AT91C_CKGR_MOR = AT91C_CKGR_MOSCEN | (0x06 << 8);

  /* Wait for the Main Oscillator to stabilize */
  while(!(*AT91C_PMC_SR & AT91C_PMC_MOSCS));

  /* Configure the PLL (Phase Locked Loop) settings */
  *AT91C_CKGR_PLLR =
    (AT91C_CKGR_DIV & 14) |
    (AT91C_CKGR_PLLCOUNT & (28 << 8)) |
    (AT91C_CKGR_MUL & (72 << 16));

  /* Wait for the PLL to lock and the Master Clock to be ready */
  while(!(*AT91C_PMC_SR & AT91C_PMC_LOCK));
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY));

  /* Set Master Clock Prescaler: Divide PLL clock by 2 */
  *AT91C_PMC_MCKR = AT91C_PMC_PRES_CLK_2;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY));

  /* Switch Master Clock source to PLL */
  *AT91C_PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY));
}


/***************************************************************************/
