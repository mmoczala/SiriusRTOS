/****************************************************************************
 *
 *  SiriusRTOS
 *  AT91SAM7S64.ld - Linker Script / Flash Workspace Definition
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/* Memory Region Definitions */
MEMORY
{
  CODE (rx) : ORIGIN = 0x00100000, LENGTH = 0x00010000
  DATA (rw) : ORIGIN = 0x00200000, LENGTH = 0x00003EEC
  STACK (rw) : ORIGIN = 0x00203EEC, LENGTH = 0x00000114
}


/* Section Definitions */
SECTIONS
{
  /* Text Section: Executable code and vector table */
  .text :
  {
    KEEP(*(.vect))
    KEEP(*(.init))
    *(.glue_7t .glue_7)
  } > CODE = 0 . = ALIGN(4);

  /* Read-Only Data Section (Constants) */
  .rodata :
  {
    *(.rodata)
  } > CODE . = ALIGN(4);

  _etext = .;
  PROVIDE(etext = .);

  /* Initialized Data Section
     Stored in Flash (AT), but copied to RAM at runtime */
  .data : AT(_etext)
  {
    _data = .;
    *(.data)
  } > DATA . = ALIGN(4);

  _edata = .;
  PROVIDE(edata = .);

  /* Uninitialized Data Section (BSS)
     Zero-initialized at startup */
  .bss :
  {
    __bss_start__ = .;
    *(.bss)
    *(COMMON)
  } > DATA . = ALIGN(4);
  __bss_end__ = .;

  /* Reserved Stack Space */
  .rsv_for_stack :
  {
    *(.rsv_ram_top)
  } > STACK . = ALIGN(4);

  /* Global End-of-Heap Symbol */
  _end = .;
  PROVIDE(end = .);
}
