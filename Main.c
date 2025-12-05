/****************************************************************************
*
*  SiriusRTOS
*  Main.c - The main source file
*  Version 1.00
*
*  Copyright 2009 by SpaceShadow
*  All rights reserved!
*
***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include <stdio.h>
#include <conio.h>
#include "OS_API.h"


/****************************************************************************
 *
 *  Configuration Constants
 *
 ***************************************************************************/

/* Number of worker threads to spawn */
#define TASK_COUNT                      8

/* Duration in milliseconds that the worker task simulates active CPU load */
#define JOB_ITER_TIME_MS                750


/****************************************************************************
 *
 *  Name:
 *    gotoxy
 *
 *  Description:
 *    Places cursor in desired position on console using ANSI escape
 *    sequences. This mimics the legacy gotoxy() function from the Borland
 *    conio.h library but is compatible with modern terminal emulators.
 *
 *  Parameters:
 *    x - Horizontal location of the cursor (1-based index)
 *    y - Vertical location of the cursor (1-based index)
 *
 ***************************************************************************/

void gotoxy(int x, int y)
{
  printf("\033[%d;%dH", y, x);
}


/****************************************************************************
 *
 *  Name:
 *    Task
 *
 *  Description:
 *    Example worker thread function. It simulates a specific workload by 
 *    burning CPU cycles for a fixed duration, then sleeps. 
 *
 *  Parameters:
 *    Arg - Task index (passed as void pointer)
 *
 *  Return:
 *    Returns success status.
 *
 ***************************************************************************/

ERROR Task(PVOID Arg)
{
  INDEX i;
  BOOL PrevLockState;
  INDEX ThreadId = (INDEX) Arg;

  /* Run the simulation loop 10 times */
  for(i = 0; i < 10; i++)
  {
    /* Lock interrupts (arLock) to prevent context switches while printing.
     * This ensures ANSI codes and  text don't get interleaved with other
     * threads. */
    PrevLockState = arLock();
    
    /* Update visual progress bar for this specific thread ID */
    gotoxy(30 + i, 4 + ThreadId);
    printf("#");
    
    arRestore(PrevLockState);
 
    /* Keep the CPU busy for JOB_ITER_TIME_MS. */
    TIME startTime = arGetTickCount();
    while (arGetTickCount() - startTime < JOB_ITER_TIME_MS)
    {
      /* Prevent the compiler from optimizing away this "useless" empty loop.
       * Without this memory barrier, the compiler might delete the loop, 
       * causing the task to finish instantly instead of simulating work.
       */
      #if defined(_MSC_VER)
        _ReadWriteBarrier();
      #else
        __asm__ __volatile__("" ::: "memory");
      #endif
    }
 
    /* Sleep formula: Base sleep + (ThreadId * 1000).
     * This staggers the tasks. Higher ThreadIDs sleep significantly longer 
     * between work bursts */
    osSleep(1000 - JOB_ITER_TIME_MS + 1000 * ThreadId);
  }

  return ERR_NO_ERROR;
}


/****************************************************************************
 *
 *  Name:
 *    Main
 *
 *  Description:
 *    The System Monitor Task. It initializes the UI, spawns worker threads,
 *    and periodically calculates/displays CPU usage statistics.
 *
 *  Parameters:
 *    Arg - Unused
 *
 *  Return:
 *    Returns success status.
 *
 ***************************************************************************/

ERROR Main(PVOID Arg)
{
  INDEX i;
  HANDLE Tasks[TASK_COUNT];

  /* Array to store previous CPU usage to optimize screen redraws */
  INDEX PrevCPUUsage[TASK_COUNT + 2];

  /* Draw the static table frame for Task ID, CPU Usage, and Progress */
  printf("\n");
  printf("\t+------+-----------+------------+\n");
  printf("\t| Task | CPU Usage | Progress   |\n");
  printf("\t+------+-----------+------------+\n");
  
  /* Print rows for each worker task */
  for(i = 0; i < TASK_COUNT; i++)
    printf("\t| %4i |           |            |\n", i);
    
  /* Print rows for Statistics (this) and Idle task */
  printf("\t| Stat |           |            |\n");
  printf("\t| Idle |           |            |\n");
  printf("\t+------+-----------+------------+\n");

  /* Spawns worker tasks with increasing IDs (1 to TASK_COUNT) */
  for(i = 0; i < TASK_COUNT; i++)
    Tasks[i] = osCreateTask(Task, (PVOID) (i + 1), 0, 1, FALSE);

  /* Initialize usage history with 0xFF to force an initial screen update */
  stMemSet(PrevCPUUsage, 0xFF, sizeof(PrevCPUUsage));

  /* Main Monitoring Loop */
  while(TRUE)
  {
    BOOL Success;
    INDEX CPUUsage[TASK_COUNT + 2];
    INDEX CPUTime, TotalTime;
  
    /* 1. Calculate Worker Task CPU usage */
    for(i = 0; i < TASK_COUNT; i++)
    {
      BOOL Success = osGetTaskStat(Tasks[i], &CPUTime, &TotalTime);
      
      /* Calculate usage in Basis Points (0.01%) for integer precision.
       * If TotalTime is valid, (CPUTime * 10000) / TotalTime.
       * If invalid, set to MAX_INDEX (displayed as '-'). */
      CPUUsage[i] = Success && TotalTime ? CPUTime * 10000 / TotalTime : (INDEX) ~((INDEX) 0);
    }
  
    /* 2. Calculate Monitor (Self) Task CPU usage */
    Success = osGetTaskStat(osGetTaskHandle(), &CPUTime, &TotalTime);
    CPUUsage[TASK_COUNT + 0] = Success && TotalTime ? CPUTime * 10000 / TotalTime : (INDEX) ~((INDEX) 0);
  
    /* 3. Calculate Idle Task CPU usage */
    /* Idle usage is the inverse of load: 100% - BusyTime */
    osGetSystemStat(&CPUTime, &TotalTime);
    CPUUsage[TASK_COUNT + 1] = TotalTime ? 10000 - CPUTime * 10000 / TotalTime : (INDEX) ~((INDEX) 0);
  
    /* 4. Check for changes */
    BOOL HasChanged = FALSE;
    for(i = 0; i < TASK_COUNT + 2; i++)
      if (CPUUsage[i] != PrevCPUUsage[i])
      {
        stMemCpy(PrevCPUUsage, CPUUsage, sizeof(PrevCPUUsage));
        HasChanged = TRUE;
        break;
      }
  
    /* 5. Update Display (only if statistics changed) */
    for(i = 0; HasChanged && i < TASK_COUNT + 2; i++)
    {
      /* Position cursor at the "CPU Usage" column for the specific row */
      gotoxy(18, 5 + i);
      
      /* Convert Basis Points back to percentage float: 1234 -> 12.34 % */
      if (CPUUsage[i] >= 0 && CPUUsage[i] <= 10000)
        printf("%7.2f %%", CPUUsage[i] / 100.0f);
      else
        printf("        - ");
    }
  
    /* Wait 200ms before next sample */
    osSleep(200);
  }

  /* Unreachable code, but required to silence compiler warnings */
  return ERR_NO_ERROR;
}


/****************************************************************************
 *
 *  Name:
 *    main
 *
 *  Description:
 *    Standard C entry point. Initializes the Hardware Abstraction Layer (AR),
 *    Standard Library (ST), and Operating System (OS) before starting the
 *    scheduler.
 *
 *  Return:
 *    Returns zero.
 *
 ***************************************************************************/

int main(void)
{
  /* Subsystem Initialization */
  arInit(); /* Architecture/Hardware init */
  stInit(); /* Standard library init */
  osInit(); /* OS Kernel init */

  /* Create the System Monitor task (Main) */
  osCreateTask(Main, NULL, 0, 0, FALSE);

  /* Transfer control to the OS Scheduler (Does not return) */
  osStart();

  /* Deinitialization (Only reached if OS shuts down) */
  osDeinit();
  arDeinit();
  return 0;
}


/***************************************************************************/
