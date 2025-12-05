;****************************************************************************
;
;  SiriusRTOS
;  AR_x86RMa.asm - 80x86 Real Mode Port (Assembly)
;  Version 1.00
;
;  Copyright 2010 by SpaceShadow
;  All rights reserved!
;
;****************************************************************************


.186

  _DATA SEGMENT BYTE PUBLIC 'DATA'

    EXTRN _arPreemptiveHandler : DWORD
    EXTRN _arPreemptiveContext : DWORD
    EXTRN _arCurrTaskContext : DWORD
    EXTRN _arPreviousPITHandler : DWORD

  _DATA ENDS


  _TEXT SEGMENT BYTE PUBLIC 'CODE'
    ASSUME cs:_TEXT
    PUBLIC _arPITInit, _arPITDeinit, _arDisableInt, _arEnableInt,
      _arIntState, _arForceInterrupt, _arGetDS, _arGetES


;****************************************************************************
;
;  Name:
;    arPITInit
;
;  Description:
;    Initializes the Programmable Interval Timer (PIT) and hooks the
;    timer interrupt vector.
;
;****************************************************************************
    _arPITInit:

      ; Save registers on the stack
      push ax
      push es
      pushf

      ; Disable interrupts (save previous state on stack via pushf)
      cli

      ; Configure PIT Channel 0
      ; 0x36 = Channel 0, Access lo/hi byte, Mode 3 (Square Wave), Binary
      mov al, 0x36
      out 0x43, al

      ; Set Frequency to 1 kHz (1.19318 MHz / 1000 = ~1193 = 0x04A9)
      mov al, 0xA9
      out 0x40, al
      mov al, 0x04
      out 0x40, al

      ; Set the default preemptive handler stub
      mov word ptr _arPreemptiveHandler, offset _arDefPreemptiveHandler
      mov word ptr _arPreemptiveHandler[2], seg _arDefPreemptiveHandler

      ; Save the current Interrupt 0x1C handler (User Timer Tick)
      ; Vector 0x1C address = 0x1C * 4 = 0x0070
      xor ax, ax
      mov es, ax
      mov ax, word ptr es:[0x0070]
      mov word ptr _arPreviousPITHandler, ax
      mov ax, word ptr es:[0x0072]
      mov word ptr _arPreviousPITHandler[2], ax

      ; Set new Interrupt 0x1C handler
      mov word ptr es:[0x0070], offset _arDefInterruptHandler
      mov word ptr es:[0x0072], seg _arDefInterruptHandler

      ; Restore registers from the stack
      popf
      pop es
      pop ax

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arPITDeinit
;
;  Description:
;    Restores the previous timer interrupt handler.
;    [!] Logic Warning: Does not restore PIT frequency to 18.2 Hz.
;
;****************************************************************************
    _arPITDeinit:

      ; Save registers on the stack
      push ax
      push es
      pushf

      ; Disable interrupts
      cli

      ; Restore the previous Interrupt 0x1C handler
      xor ax, ax
      mov es, ax
      mov ax, word ptr _arPreviousPITHandler
      mov word ptr es:[0x0070], ax
      mov ax, word ptr _arPreviousPITHandler[2]
      mov word ptr es:[0x0072], ax

      ; Restore registers from the stack
      popf
      pop es
      pop ax

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arDisableInt
;
;  Description:
;    Disables maskable interrupts.
;
;****************************************************************************
    _arDisableInt:

      ; Disable interrupts
      cli

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arEnableInt
;
;  Description:
;    Enables maskable interrupts.
;
;****************************************************************************
    _arEnableInt:

      ; Enable interrupts
      sti

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arIntState
;
;  Description:
;    Returns the state of the interrupt enable flag (IF).
;
;  Return:
;    1 if interrupts are enabled, 0 otherwise.
;
;****************************************************************************
    _arIntState:

      ; Read interrupt enable flag state via FLAGS register
      pushf
      pop ax
      shr ax, 9
      and ax, 0x0001

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arDefInterruptHandler
;
;  Description:
;    Default timer interrupt service routine. Performs context switch.
;    Hooked to Vector 0x1C.
;
;****************************************************************************
    _arDefInterruptHandler:

      ; Disable interrupts
      cli

      ; Save registers on the current task stack
      pusha
      push ds
      push es

      ; Initialize Data Segment (DS) for the kernel
      mov ax, SEG _arCurrTaskContext
      mov ds, ax

      ; Save current task context (Stack Pointer)
      ; Format: Offset (SP), then Segment (SS)
      mov ax, ss
      mov word ptr _arCurrTaskContext, sp
      mov word ptr _arCurrTaskContext[2], ax

      ; Switch to the dedicated scheduler stack (Preemptive Context)
      mov sp, word ptr _arPreemptiveContext
      mov ax, word ptr _arPreemptiveContext[2]
      mov ss, ax

      ; Call the preemptive handler (Scheduler)
      ; Push arguments (Far Pointer to _arCurrTaskContext)
      push seg _arCurrTaskContext
      push offset _arCurrTaskContext
      call dword ptr _arPreemptiveHandler

      ; Restore the new task context (Switch Stack)
      mov sp, word ptr _arCurrTaskContext
      mov ax, word ptr _arCurrTaskContext[2]
      mov ss, ax

      ; Acknowledge Interrupt (EOI) to Master PIC
      ; [!] Logic Warning: Vector 0x1C is called by the BIOS Int 0x08
      ; handler, which already handles EOI. Sending EOI here is redundant
      ; and potentially dangerous (Double EOI).
      mov al, 0x20
      out 0x20, al

      ; Restore registers from the new task stack
      pop es
      pop ds
      popa

      ; Return from Interrupt
      iret


;****************************************************************************
;
;  Name:
;    arDefPreemptiveHandler
;
;  Description:
;    Default stub for the preemptive handler.
;
;****************************************************************************
    _arDefPreemptiveHandler:

      ; Return (Far) and pop 4 bytes (arguments) from stack
      retf 0x0004


;****************************************************************************
;
;  Name:
;    arForceInterrupt
;
;  Description:
;    Simulates a timer interrupt to force a context switch.
;
;****************************************************************************
    _arForceInterrupt:

      ; Simulate interrupt frame (Push Flags, Push CS) and Call
      pushf
      push cs
      call _arDefInterruptHandler

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arGetDS
;
;  Description:
;    Returns the current Data Segment (DS) selector.
;
;****************************************************************************
    _arGetDS:

      ; Load DS to AX
      mov ax, ds

      ; Return (Far)
      retf


;****************************************************************************
;
;  Name:
;    arGetES
;
;  Description:
;    Returns the current Extra Segment (ES) selector.
;
;****************************************************************************
    _arGetES:

      ; Load ES to AX
      mov ax, es

      ; Return (Far)
      retf


  _TEXT ENDS
END


;****************************************************************************
