/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_Core.h - Operating System Core interface
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef OS_CORE_H
#define OS_CORE_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "OS_API.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* The highest used priority is set to 15 by default */
#ifndef OS_LOWEST_USED_PRIORITY
  #define OS_LOWEST_USED_PRIORITY       15
#elif ((OS_LOWEST_USED_PRIORITY) < 0) || (OS_LOWEST_USED_PRIORITY > 254)
  #error OS_LOWEST_USED_PRIORITY must be between 0 and 254
#endif

/* Using timeouts is enabled by default */
#ifndef OS_USE_WAITING_WITH_TIME_OUT
  #define OS_USE_WAITING_WITH_TIME_OUT  1
#elif (((OS_USE_WAITING_WITH_TIME_OUT) != 0) && \
  ((OS_USE_WAITING_WITH_TIME_OUT) != 1))
  #error OS_USE_WAITING_WITH_TIME_OUT must be 0 or 1
#endif

/* Statistics sampling rate. By default set to 100 time units */
#ifndef OS_STAT_SAMPLE_RATE
  #define OS_STAT_SAMPLE_RATE           100UL
#elif ((OS_STAT_SAMPLE_RATE) < 1UL)
  #error OS_STAT_SAMPLE_RATE must be greater than or equal to 1
#endif

/* Using fixed-size memory allocation for system objects is disabled by
   default */
#ifndef OS_USE_FIXMEM_POOLS
  #define OS_USE_FIXMEM_POOLS           0
#elif (((OS_USE_FIXMEM_POOLS) != 0) && ((OS_USE_FIXMEM_POOLS) != 1))
  #error OS_USE_FIXMEM_POOLS must be either 0 or 1
#elif ((OS_USE_FIXMEM_POOLS) && !(ST_USE_FIXMEM))
  #error ST_USE_FIXMEM must be set to 1 when OS_USE_FIXMEM_POOLS is 1
#endif

/* Define up to four fixed-size memory pools */
#if (OS_USE_FIXMEM_POOLS)

  /* Size of the fixed memory pool 0 is 1024 bytes by default */
  #ifndef OS_FIX_POOL0_SIZE
    #define OS_FIX_POOL0_SIZE           1024UL
  #elif ((OS_FIX_POOL0_SIZE) < 0UL)
    #error OS_FIX_POOL0_SIZE must be greater than or equal to 0
  #endif

  /* Block size of the fixed memory pool 0 is 32 bytes by default */
  #ifndef OS_FIX_POOL0_ITEM_SIZE
    #define OS_FIX_POOL0_ITEM_SIZE      32UL
  #elif ((OS_FIX_POOL0_ITEM_SIZE) <= 0UL)
    #error OS_FIX_POOL0_ITEM_SIZE must be greater than 0
  #endif

  /* Size of the fixed memory pool 1 is 1024 bytes by default */
  #ifndef OS_FIX_POOL1_SIZE
    #define OS_FIX_POOL1_SIZE           1024UL
  #elif ((OS_FIX_POOL1_SIZE) < 0UL)
    #error OS_FIX_POOL1_SIZE must be greater than or equal to 0
  #endif

  /* Block size of the fixed memory pool 1 is 128 bytes by default */
  #ifndef OS_FIX_POOL1_ITEM_SIZE
    #define OS_FIX_POOL1_ITEM_SIZE      128UL
  #elif ((OS_FIX_POOL1_ITEM_SIZE) <= 0UL)
    #error OS_FIX_POOL1_ITEM_SIZE must be greater than 0
  #endif

  /* Size of the fixed memory pool 2 is 1024 bytes by default */
  #ifndef OS_FIX_POOL2_SIZE
    #define OS_FIX_POOL2_SIZE           1024UL
  #elif ((OS_FIX_POOL2_SIZE) < 0UL)
    #error OS_FIX_POOL2_SIZE must be greater than or equal to 0
  #endif

  /* Block size of the fixed memory pool 2 is 256 bytes by default */
  #ifndef OS_FIX_POOL2_ITEM_SIZE
    #define OS_FIX_POOL2_ITEM_SIZE      256UL
  #elif ((OS_FIX_POOL2_ITEM_SIZE) <= 0UL)
    #error OS_FIX_POOL2_ITEM_SIZE must be greater than 0
  #endif

  /* Size of the fixed memory pool 3 is 4096 bytes by default */
  #ifndef OS_FIX_POOL3_SIZE
    #define OS_FIX_POOL3_SIZE           4096UL
  #elif ((OS_FIX_POOL3_SIZE) < 0UL)
    #error OS_FIX_POOL3_SIZE must be greater than or equal to 0
  #endif

  /* Block size of the fixed memory pool 3 is 1024 bytes by default */
  #ifndef OS_FIX_POOL3_ITEM_SIZE
    #define OS_FIX_POOL3_ITEM_SIZE      1024UL
  #elif ((OS_FIX_POOL3_ITEM_SIZE) <= 0UL)
    #error OS_FIX_POOL3_ITEM_SIZE must be greater than 0
  #endif

  /* Check fixed memory pools configuration */
  #if (((OS_FIX_POOL0_ITEM_SIZE) > (OS_FIX_POOL1_ITEM_SIZE)) || \
    ((OS_FIX_POOL1_ITEM_SIZE) > (OS_FIX_POOL2_ITEM_SIZE)) || \
    ((OS_FIX_POOL2_ITEM_SIZE) > (OS_FIX_POOL3_ITEM_SIZE)))
    #error Block size of the following fixed memory pools must always \
      be greater than the previous
  #endif

#endif

/* Internal system memory is disabled by default */
#ifndef OS_INTERNAL_MEMORY_SIZE
  #define OS_INTERNAL_MEMORY_SIZE       0UL
#elif (OS_USE_FIXMEM_POOLS)
  #error OS_INTERNAL_MEMORY_SIZE must be set to 0 when \
    OS_USE_FIXMEM_POOLS is set to 1
#elif ((OS_INTERNAL_MEMORY_SIZE) < 0UL)
  #error OS_INTERNAL_MEMORY_SIZE must be greater than or equal to 0
#endif

/* Size of the operating system stack */
#ifndef OS_STACK_SIZE
  #define OS_STACK_SIZE                 0x200UL
#elif ((OS_STACK_SIZE) <= 0UL)
  #error OS_STACK_SIZE must be greater than 0
#endif

/* Size of the idle task stack */
#ifndef OS_IDLE_STACK_SIZE
  #define OS_IDLE_STACK_SIZE            0x60UL
#elif ((OS_IDLE_STACK_SIZE) <= 0UL)
  #error OS_IDLE_STACK_SIZE must be greater than 0
#endif


/****************************************************************************
 *
 *  System configuration
 *
 ***************************************************************************/

/* Using system object names is disabled by default, but will be enabled
   automatically when it is necessary */
#ifndef OS_USE_OBJECT_NAMES
  #define OS_USE_OBJECT_NAMES           0
#endif

/* System uses multiple signals (disabled by default, but will be enabled
   automatically when it is necessary) */
#ifndef OS_USE_MULTIPLE_SIGNALS
  #define OS_USE_MULTIPLE_SIGNALS       0
#endif

/* System supports critical section objects (disabled by default, but will
   be enabled automatically when it is necessary) */
#ifndef OS_USE_CSEC_OBJECTS
  #define OS_USE_CSEC_OBJECTS           0
#endif

/* System supports time signalization objects (disabled by default, but
   will be enabled automatically when it is necessary) */
#ifndef OS_USE_TIME_OBJECTS
  #define OS_USE_TIME_OBJECTS           (OS_USE_WAITING_WITH_TIME_OUT)
#endif

/* System supports a direct read-write method for IPC objects (disabled by
   default, but will be enabled automatically when it is necessary) */
#ifndef OS_USE_IPC_DIRECT_RW
  #define OS_USE_IPC_DIRECT_RW          0
#endif

/* Dynamic task priority change (disabled by default, but will be enabled
   automatically when it is necessary) */
#ifndef OS_MODIFIABLE_TASK_PRIO
  #define OS_MODIFIABLE_TASK_PRIO       (OS_USE_CSEC_OBJECTS)
#endif

/* System uses time quanta (disabled by default, but will be enabled
   automatically when it is necessary) */
#ifndef OS_USE_TIME_QUANTA
  #define OS_USE_TIME_QUANTA            0
#endif

/* System provides CPU usage statistics (disabled by default, but will
   be enabled automatically when it is necessary) */
#ifndef OS_USE_STATISTICS
  #define OS_USE_STATISTICS             (OS_GET_SYSTEM_STAT_FUNC)
#endif

/* Using system IO control code (disabled by default, but will be enabled
   automatically when it is necessary) */
#ifndef OS_USE_SYSTEM_IO_CTRL
  #define OS_USE_SYSTEM_IO_CTRL         0
#endif

/* Using device IO control function (disabled by default, but will be
   enabled automatically when it is necessary) */
#ifndef OS_USE_DEVICE_IO_CTRL
  #define OS_USE_DEVICE_IO_CTRL         (OS_USE_SYSTEM_IO_CTRL)
#endif


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* System object types */
#define OS_OBJECT_TYPE_IGNORE           0x40
#define OS_OBJECT_TYPE_TASK             1

/* System object flags */
#define OS_OBJECT_FLAG_READY_TO_USE     0x01
#define OS_OBJECT_FLAG_READY_TO_RUN     0x02
#define OS_OBJECT_FLAG_USES_IO_DEINIT   0x04

/* Signal flags */
#define OS_SIGNAL_FLAG_DEFERRED         0x01
#define OS_SIGNAL_FLAG_USES_IO_SYSTEM   0x02
#define OS_SIGNAL_FLAG_DEC_ON_RELEASE   0x04
#define OS_SIGNAL_FLAG_CRITICAL_SECTION 0x08
#define OS_SIGNAL_FLAG_MUTUAL_EXCLUSION 0x10
#define OS_SIGNAL_FLAG_ABANDONED        0x20

/* Task blocking flags */
#define OS_BLOCK_FLAG_SLEEP             0x01
#define OS_BLOCK_FLAG_WAITING           0x02
#define OS_BLOCK_FLAG_IPC               0x04
#define OS_BLOCK_FLAG_SUSPENDED         0x10
#define OS_BLOCK_FLAG_TERMINATING       0x20
#define OS_BLOCK_FLAG_TERMINATED        0x40

/* Reserved system priorities */
#define OS_LOWEST_PRIORITY              255

/* Scheduling reason */
#define OS_SCHED_TIME_NOTIFICATION      1
#define OS_SCHED_DEFERRED_SIGNALIZATION 2
#define OS_SCHED_READY_TO_RUN           3

/* System IO control codes */
#define OS_IO_CTL_GET_SIGNAL_STATE      0x00
#define OS_IO_CTL_WAIT_ACQUIRE          0x01
#define OS_IO_CTL_WAIT_START            0x02
#define OS_IO_CTL_WAIT_UPDATE           0x03
#define OS_IO_CTL_WAIT_FAILURE          0x04

/* Device IO control codes */
#define DEV_IO_CTL_DEINIT               0x11
#define DEV_IO_CTL_READ                 0x12
#define DEV_IO_CTL_WRITE                0x13


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* System descriptors */
#if (OS_USE_OBJECT_NAMES)
  struct TObjectName;
#endif

struct TSignal;
struct TWaitAssoc;

#if (OS_USE_TIME_OBJECTS)
  struct TTimeNotify;
#endif

#if (OS_USE_CSEC_OBJECTS)
  struct TCSAssoc;
  struct TPriorityPath;
  struct TCriticalSection;
#endif

struct TSysObject;
struct TTask;


/* Device IO control function type */
typedef INDEX (FAR * TDeviceIOCtrl)(struct TSysObject FAR *Object,
  INDEX ControlCode, PVOID Buffer, SIZE BufferSize,
  struct TIORequest FAR *IORequest);


#if (OS_USE_OBJECT_NAMES)

  /* Name descriptor */
  struct TObjectName
  {
    /* System object name (string or integer value) */
    #if ((OS_SYS_OBJECT_MAX_NAME_LEN) > 0UL)
      char Name[OS_SYS_OBJECT_MAX_NAME_LEN];
    #else
      SYSNAME Name;
    #endif

    /* System object descriptor pointer */
    struct TSysObject FAR *Object;

    /* Binary search tree node for object names */
    struct TBSTreeNode Node;
  };

#endif


/* Signal descriptor */
struct TSignal
{
  /* Signal flags */
  UINT8 Flags;

  /* System object state */
  INDEX Signaled;

  /* Tree used as a priority queue of tasks waiting for this signal */
  struct TBSTree WaitingTasks;

  /* Tree node descriptor for deferred signalization */
  struct TBSTreeNode DeferredSgn;

  /* System object descriptor pointer */
  #if (OS_USE_SYSTEM_IO_CTRL)
    struct TSysObject FAR *Object;
  #endif

  /* Critical section descriptor pointer */
  #if (OS_USE_CSEC_OBJECTS)
    struct TCriticalSection FAR *CS;
  #endif

  #if ((OS_USE_MULTIPLE_SIGNALS) && (OS_ALLOW_OBJECT_DELETION))
    struct TSignal FAR *NextSignal;
  #endif
};


/* Wait association descriptor */
struct TWaitAssoc
{
  /* Relation between task and signal descriptor */
  struct TSignal FAR *Signal;
  struct TTask FAR *Task;

  /* Binary search tree node descriptor */
  struct TBSTreeNode Node;

  /* Index of signal causing exiting from waiting state */
  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX Index;
  #endif
};


#if (OS_USE_TIME_OBJECTS)

  /* Time notification descriptor */
  struct TTimeNotify
  {
    /* Time and priority */
    UINT8 Priority; /* [!] NOTE: Is this still needed? */
    TIME Time;

    /* Information of time notification descriptor registration */
    BOOL Registered;

    /* Controlled object */
    struct TTask FAR *Task;
    struct TSignal FAR *Signal;

    /* Priority queue item descriptor */
    struct TPQueueItem Item;
  };

#endif


#if (OS_USE_CSEC_OBJECTS)

  /* Critical section association descriptor */
  struct TCSAssoc
  {
    /* Relation between task and signal descriptor */
    struct TCriticalSection FAR *CS;
    struct TTask FAR *Task;

    /* Priority queue item descriptor used by queue of critical sections
       owned by a task */
    struct TPQueueItem Item;

    /* Binary search tree node descriptor for searching critical sections
       by addresses */
    struct TBSTreeNode Node;

    /* Critical section own counter */
    INDEX Count;

    /* Pointer to previous and next association descriptor */
    struct TCSAssoc FAR *Prev;
    struct TCSAssoc FAR *Next;
  };


  /* Priority inheritance path descriptor */
  struct TPriorityPath
  {
    /* System object descriptor pointer */
    struct TTask FAR *Task;
    struct TCriticalSection FAR *CS;

    /* Priority inheritance path pointer */
    struct TPriorityPath FAR *Next;
  };


  /* Critical section object descriptor */
  struct TCriticalSection
  {
    /* Object signalization descriptor pointer */
    struct TSignal FAR *Signal;

    /* Maximal possible signalization value */
    INDEX MaxSignaled;

    /* Structure used by priority inheritance path algorithm */
    struct TPriorityPath PriorityPath;

    /* Variables used for controlling the allocation of critical section
       association descriptors */
    struct TCSAssoc FAR *FirstFree;
    struct TCSAssoc FAR *FirstAllocated;
    INDEX Count;

    /* List of the critical section owners */
    struct TCSAssoc TasksInCS[1];
  };

#endif


/* System object descriptor */
struct TSysObject
{
  /* System object type */
  UINT8 Type;

  /* System object handle */
  HANDLE Handle;

  /* System object flags */
  UINT8 Flags;

  /* Number of object owners */
  #if (OS_ALLOW_OBJECT_DELETION)
    INDEX OwnerCount;
  #endif

  /* Object signalization descriptor */
  struct TSignal Signal;

  /* Object name descriptor pointer */
  #if (OS_USE_OBJECT_NAMES)
    struct TObjectName FAR *Name;
  #endif

  /* Device IO control function for specified object */
  #if (OS_USE_DEVICE_IO_CTRL)
    TDeviceIOCtrl DeviceIOCtrl;
  #endif

  /* System object deinitialization list pointers */
  #if (OS_DEINIT_FUNC)
    struct TSysObject FAR *PrevObject;
    struct TSysObject FAR *NextObject;
  #endif

  /* Pointer to specific object descriptor */
  PVOID ObjectDesc;
};


/* Task descriptor */
struct TTask
{
  /* System object descriptor */
  struct TSysObject Object;

  /* Task context */
  struct TTaskContext TaskContext;

  /* Task startup function and its argument */
  TTaskProc TaskProc;
  PVOID Arg;

  /* Priority queue item descriptor for queue of ready to run tasks */
  struct TPQueueItem ReadyTask;

  /* Task priority */
  UINT8 Priority;

  /* Variables used by priority inheritance path algorithm */
  #if (OS_USE_CSEC_OBJECTS)
    UINT8 AssignedPriority;
    struct TPriorityPath PriorityPath;
  #endif

  /* Last time quantum assign time */
  TIME LastQuantumTime;
  INDEX LastQuantumIndex;

  /* Task time quanta */
  #if (OS_USE_TIME_QUANTA)
    UINT8 MaxTimeQuantum;
    UINT8 TimeQuantumCounter;
  #endif

  /* Task state */
  UINT8 BlockingFlags;

  /* List of the objects which the task is waiting */
  struct TWaitAssoc WaitingFor[OS_MAX_WAIT_FOR_OBJECTS];

  #if ((OS_MAX_WAIT_FOR_OBJECTS) > 1)
    INDEX WaitingCount;
    INDEX WaitingIndex;
  #endif

  /* Waiting exit code */
  ERROR WaitExitCode; /* [!] TODO: Add ifdef as in os_core.h: 2967 */

  /* Waiting timeout */
  #if (OS_USE_WAITING_WITH_TIME_OUT)
    struct TTimeNotify WaitTimeout;
  #endif

  /* Direct read-write feature for IPC */
  #if (OS_USE_IPC_DIRECT_RW)
    SIZE IPCSize;
    PVOID IPCBuffer;
    BOOL IPCDRWCompletion;
    struct TTask FAR *IPCBlockingTask;
  #endif

  /* Lists of the owned critical sections, ordered by its priorities
     and addresses */
  #if (OS_USE_CSEC_OBJECTS)
    struct TPQueue OwnedCS;
    struct TBSTree OwnedCSPtr;
  #endif

  /* Task children (created or opened system objects) */
  #if (OS_ALLOW_OBJECT_DELETION)
    struct TBSTree Childs;
  #endif

  /* CPU usage */
  #if (OS_USE_STATISTICS)
    TIME CPUUsageTime;
    INDEX CPUUsage;
    TIME CPUCalcTime;
    INDEX CPUCalc;
  #endif

  /* Last error code */
  ERROR LastErrorCode;
};


/****************************************************************************
 *
 *  Variables
 *
 ***************************************************************************/

/* ISR Flag */
extern BOOL osInISR;

/* Current task pointer */
extern struct TTask FAR *osCurrentTask;

/* Last time quantum assign time */
extern TIME osLastQuantumTime;
extern INDEX osLastQuantumIndex;

/* Total CPU usage */
#if (OS_USE_STATISTICS)
  extern TIME osCPUUsageTime;
  extern INDEX osCPUUsage;
#endif


/****************************************************************************
 *
 *  Macros
 *
 ***************************************************************************/

/* Memory management */
#if !(OS_USE_FIXMEM_POOLS)
  #if ((OS_INTERNAL_MEMORY_SIZE) > 0)

    extern UINT8 osMemoryPool[OS_INTERNAL_MEMORY_SIZE];

    #define osMemAlloc(Size) stMemoryAlloc(stMemoryPool, (SIZE) (Size))
    #define osMemFree(Ptr) stMemoryFree(stMemoryPool, (PVOID) (Ptr))

  #else

    #define osMemAlloc(Size) stMemAlloc((SIZE) (Size))
    #define osMemFree(Ptr) stMemFree((PVOID) (Ptr))

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

  #if (OS_USE_FIXMEM_POOLS)
    PVOID osMemAlloc(SIZE Size);
  #endif

  #if (OS_ALLOW_OBJECT_DELETION)
    int osObjectByHandleCmp(PVOID Item1, PVOID Item2);
  #endif

  int osWaitAssocCmp(PVOID Item1, PVOID Item2);
  BOOL osRegisterObject(PVOID ObjectDesc, struct TSysObject FAR *Object,
    UINT8 Type);

  #if (OS_USE_OBJECT_NAMES)
     BOOL osRegisterName(struct TSysObject FAR *Object,
      struct TObjectName FAR *ObjectName, SYSNAME Name);
  #endif

  /* [!] TODO: Add register signal function to automatically append to
     the end of the signal list */

  #if (OS_USE_CSEC_OBJECTS)
    int osCSAssocCmp(PVOID Item1, PVOID Item2);
    int osCSPtrCmp(PVOID Item1, PVOID Item2);
    void osRegisterCS(struct TSignal FAR *Signal,
      struct TCriticalSection FAR *CS, INDEX InitialCount, INDEX MaxCount,
      BOOL MutualExclusion);
  #endif

  #if (OS_ALLOW_OBJECT_DELETION)
    void osDeleteObject(struct TSysObject FAR *Object);
  #endif

  #if (OS_USE_OBJECT_NAMES)
    struct TSysObject FAR *osOpenNamedObject(SYSNAME Name, UINT8 Type);
  #endif

  struct TSysObject FAR *osGetObjectByHandle(HANDLE Handle, UINT8 Type);

  #if ((OS_ALLOW_OBJECT_DELETION) || (OS_USE_CSEC_OBJECTS))
    void osReleaseTaskResources(struct TTask FAR *Task);
  #endif

  void osMakeReady(struct TTask FAR *Task);
  void osMakeNotReady(struct TTask FAR *Task);

  void osMakeNotWaiting(struct TTask FAR *Task);

  #if (OS_USE_MULTIPLE_SIGNALS)
    BOOL osWaitFor(struct TSignal FAR *Signal, TIME Timeout);
  #endif

  void osUpdateSignalState(struct TSignal FAR *Signal, INDEX Signaled);

  #if (OS_USE_CSEC_OBJECTS)
    BOOL osPriorityPath(struct TPriorityPath FAR *Priority);
    BOOL osReleaseCS(struct TCriticalSection FAR *CS,
      struct TTask FAR *Task, INDEX ReleaseCount, INDEX *PrevCount);
  #endif

  #if (OS_USE_TIME_OBJECTS)
    void osInitTimeNotify(struct TTimeNotify FAR *TimeNotify);
    void osRegisterTimeNotify(struct TTimeNotify FAR *TimeNotify,
      TIME Time);
    void osUnregisterTimeNotify(struct TTimeNotify FAR *TimeNotify);
  #endif

  #if (OS_MODIFIABLE_TASK_PRIO)
    BOOL osChangeTaskPriority(struct TTask FAR *Task, UINT8 Priority);
    void osRescheduleIfHigherPriority(void);
  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* OS_CORE_H */
/***************************************************************************/
