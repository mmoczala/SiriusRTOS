/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Handle.c - Handle management
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
#if (ST_USE_HANDLE)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

/* Handle flags used internally */
#define HANDLE_FLAG_ALLOCATED           0x80
#define HANDLE_FLAG_FREE                0x40
#define HANDLE_TYPE_MASK                0x3F


/***************************************************************************/
#if ((ST_MAX_HANDLE_COUNT) > 0UL)
/***************************************************************************/


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

static struct THandle stHandleArr[ST_MAX_HANDLE_COUNT];
static struct THandle FAR *stFirstFreeHandle;
static HANDLE stHighestUsedHandles;


/****************************************************************************
 *
 *  Name:
 *    stHandleInit
 *
 *  Description:
 *    Initializes the Handle Management module.
 *
 ***************************************************************************/

void stHandleInit(void)
{
  /* Initialize global variables */
  stFirstFreeHandle = NULL;
  stHighestUsedHandles = 0UL;
}


/***************************************************************************/
#endif /* ST_MAX_HANDLE_COUNT > 0UL */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stHandleAlloc
 *
 *  Description:
 *    Allocates a new handle and an optional memory buffer for the object
 *    pointed to by the handle. If Object is NULL and ObjectSize is zero,
 *    the function allocates a handle without an associated memory buffer.
 *
 *  Parameters:
 *    Handle - Pointer to a variable that will receive the allocated handle
 *      value.
 *    Object - If ObjectSize is zero, this parameter points to the object
 *      associated with the new handle. If ObjectSize is non-zero, this
 *      parameter may point to a variable that will receive the address of
 *      the newly allocated memory (can be NULL in both cases).
 *    ObjectSize - If zero, the address specified in the Object parameter
 *      is associated with the new handle. If greater than zero, a new
 *      memory buffer of the specified size is allocated and associated
 *      with this handle.
 *    Type - The type of the handle.
 *
 *  Return:
 *    Pointer to the handle descriptor on success, or NULL on failure.
 *
 ***************************************************************************/

struct THandle FAR * stHandleAlloc(HANDLE FAR *Handle, PVOID Object,
  SIZE ObjectSize, UINT8 Type)
{
  struct THandle FAR *HandleDesc;
  PVOID ObjPtr;


  /* Allocate handle descriptor in the static handles array */
  #if ((ST_MAX_HANDLE_COUNT) > 0UL)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      BOOL PrevLockState;
      PrevLockState = arLock();
    #endif

    /* Get pointer to a free handle descriptor */
    HandleDesc = stFirstFreeHandle;
    if(HandleDesc)
      stFirstFreeHandle = (struct THandle FAR *) HandleDesc->Object;

    /* Allocate a new handle descriptor from the high-water mark */
    else
      if(stHighestUsedHandles < ST_MAX_HANDLE_COUNT)
        HandleDesc = &stHandleArr[stHighestUsedHandles++];

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Cannot assign a new handle */
    if(!HandleDesc)
    {
      stSetLastError(ERR_CAN_NOT_ASSIGN_NEW_HANDLE);
      return NULL;
    }

  /* Allocate handle descriptor in dynamic memory */
  #else

    /* Allocate memory for the handle descriptor */
    HandleDesc = (struct THandle FAR *)
      stMemAlloc(AR_MEMORY_ALIGN_UP(sizeof(struct THandle)) +
      (ObjectSize ? sizeof(PVOID) : ObjectSize));

    /* Cannot assign a new handle */
    if(!HandleDesc)
      return NULL;

  #endif


  /* Setup the handle descriptor */
  HandleDesc->Flags = (UINT8) (Type & HANDLE_TYPE_MASK);

  #if (ST_USE_OWNER_COUNTER)
    HandleDesc->OwnerCount = 0;
  #endif

  #if (USE_DEVICE_IO_CTL_FUNC)
    HandleDesc->DeviceIOCtl = NULL;
  #endif


  /* Set pointer to the object associated with the new handle */
  #if ((ST_MAX_HANDLE_COUNT) > 0UL)

    /* Allocate memory for the object descriptor if size is requested */
    if(ObjectSize)
    {
      ObjPtr = stMemAlloc(ObjectSize);
      if(!ObjPtr)
      {
        /* Enter critical section (required only in multitasking) */
        #if (OS_USED)
          PrevLockState = arLock();
        #endif

        /* Release the allocated handle descriptor on failure */
        HandleDesc->Flags = HANDLE_FLAG_FREE;
        HandleDesc->Object = stFirstFreeHandle;
        stFirstFreeHandle = HandleDesc;

        /* Leave critical section */
        #if (OS_USED)
          arRestore(PrevLockState);
        #endif

        /* Return with failure */
        return NULL;
      }

      /* Mark that the handle points to allocated memory */
      HandleDesc->Flags |= HANDLE_FLAG_ALLOCATED;

      /* Set pointer to the object for the caller */
      if(Object)
        *(PVOID FAR *) Object = ObjPtr;
    }

    else
        ObjPtr = Object;

    /* Set pointer to the object internally */
    HandleDesc->Object = ObjPtr;

    /* Set handle value */
    *Handle = (HANDLE) (1 + ((HANDLE) (HandleDesc - stHandleArr)));

  #else

    /* Object definition area calculation */
    ObjPtr = (PVOID) &((UINT8 FAR *) HandleDesc)[
        AR_MEMORY_ALIGN_UP(sizeof(struct THandle))];

    /* Set pointer to the object associated with the handle */
    if(!ObjectSize)
      *(PVOID FAR *) ObjPtr = Object;

    /* Mark that the handle points to allocated memory */
    else
    {
      HandleDesc->Flags |= HANDLE_FLAG_ALLOCATED;

      /* Set pointer to the object for the caller */
      if(Object)
        *(PVOID FAR *) Object = ObjPtr;
    }

    /* Set handle value */
    *Handle = (HANDLE) HandleDesc;

  #endif

  /* Return pointer to the handle descriptor */
  return HandleDesc;
}


/****************************************************************************
 *
 *  Name:
 *    stHandleRelease
 *
 *  Description:
 *    Releases the specified handle and frees associated resources.
 *
 *  Parameters:
 *    Handle - The handle to release.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stHandleRelease(HANDLE Handle)
{
  /* Release handle descriptor */
  #if ((ST_MAX_HANDLE_COUNT) > 0UL)

    struct THandle FAR *HandleDesc;
    PVOID Object;

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      BOOL PrevLockState;
      PrevLockState = arLock();
    #endif

    /* Release handle when valid */
    if((Handle != NULL_HANDLE) && (Handle <= stHighestUsedHandles))
    {
      HandleDesc = &stHandleArr[Handle - 1];
      if(HandleDesc->Flags != HANDLE_FLAG_FREE)
      {
        /* Get pointer to the object to be released */
        if(HandleDesc->Flags & HANDLE_FLAG_ALLOCATED)
          Object = HandleDesc->Object;
        else
          Object = NULL;

        /* Release handle logic */
        HandleDesc->Flags = HANDLE_FLAG_FREE;
        HandleDesc->Object = stFirstFreeHandle;
        stFirstFreeHandle = (struct THandle FAR *) HandleDesc;

        /* Leave critical section */
        #if (OS_USED)
          arRestore(PrevLockState);
        #endif

        /* Release memory resources, if allocated */
        if(Object)
          return stMemFree(Object);

        /* Return with success */
        return TRUE;
      }
    }

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Failure: Invalid handle specified */
    stSetLastError(ERR_INVALID_HANDLE);
    return FALSE;

  #else

    /* Release memory allocated for the handle descriptor */
    if(Handle != NULL_HANDLE)
      return stMemFree(Handle);

    /* Failure: Invalid handle specified */
    stSetLastError(ERR_INVALID_HANDLE);
    return FALSE;

  #endif
}


/****************************************************************************
 *
 *  Name:
 *    stGetHandlePtr
 *
 *  Description:
 *    Provides access to handle-specific information.
 *
 *  Parameters:
 *    Handle - The handle to query.
 *    Object - Pointer to a variable that will receive a pointer to the
 *      object the handle points to. Can be NULL if not needed.
 *    Type - The required type of the handle. Function fails if the handle
 *      type does not match this value. Use ST_HANDLE_TYPE_IGNORE to skip
 *      type checking.
 *
 *  Return:
 *    Pointer to the handle descriptor on success, or NULL on failure.
 *
 ***************************************************************************/

struct THandle FAR * stGetHandleInfo(HANDLE Handle, PVOID *Object,
  UINT8 Type)
{
  struct THandle FAR *HandleDesc;

  /* Release handle descriptor */
  #if ((ST_MAX_HANDLE_COUNT) > 0UL)

    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      BOOL PrevLockState;
      PrevLockState = arLock();
    #endif

    /* Validate handle */
    if((Handle != NULL_HANDLE) && (Handle <= stHighestUsedHandles))
    {
      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* Continue handle validation */
      HandleDesc = &stHandleArr[Handle - 1];
      if(HandleDesc->Flags != HANDLE_FLAG_FREE)
        if((Type == ST_HANDLE_TYPE_IGNORE) ||
          ((HandleDesc->Flags & HANDLE_TYPE_MASK) == Type))
        {
          /* Obtain pointer to the object that the handle points to */
          if(Object)
            *Object = HandleDesc->Object;

          /* Return pointer to the handle descriptor */
          return HandleDesc;
        }
    }

    /* Failure: Invalid handle specified */
    stSetLastError(ERR_INVALID_HANDLE);
    return NULL;

  #else

    /* Validate handle */
    if(Handle != NULL_HANDLE)
    {
      HandleDesc = (struct THandle FAR *) Handle;
      
      /* [!] BUG: The logic below seems inverted or incorrect compared to
         the static allocation path. It enters the block if types DO NOT
         match? */
      if((Type != ST_HANDLE_TYPE_IGNORE) ||
        ((HandleDesc->Flags & HANDLE_TYPE_MASK) != Type))
      {
        /* Pointer to the object that the handle points to */
        if(Object)
          if(HandleDesc->Flags & HANDLE_FLAG_ALLOCATED)
            *Object = (PVOID) &((UINT8 FAR *) HandleDesc)[
              AR_MEMORY_ALIGN_UP(sizeof(struct THandle))];
          else
            *Object = *(PVOID FAR *) (PVOID) &((UINT8 FAR *) HandleDesc)[
              AR_MEMORY_ALIGN_UP(sizeof(struct THandle))];

        /* Return pointer to the handle descriptor */
        return HandleDesc;
      }
    }

    /* Failure: Invalid handle specified */
    stSetLastError(ERR_INVALID_HANDLE);
    return NULL;

  #endif
}


/***************************************************************************/
#if (ST_IO_CTL_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stIOCtrl
 *
 *  Description:
 *    Executes an IO control function for an object specified by its handle.
 *
 *  Parameters:
 *    Handle - Handle of the object.
 *    IOCtl - IO control code.
 *    Buffer - Pointer to the buffer (usage depends on IO control code).
 *    BufferSize - Size of the buffer (usage depends on IO control code).
 *
 *  Return:
 *    Returns the specific IO control code result.
 *    Returns 0 on failure (if IO control is not defined).
 *
 ***************************************************************************/

INDEX stIOCtrl(HANDLE Handle, INDEX IOCtl, PVOID Buffer, SIZE BufferSize)
{
  struct THandle FAR *HandleInfo;

  /* Get handle information */
  HandleInfo = stGetHandleInfo(Handle, NULL, ST_HANDLE_TYPE_IGNORE);

  /* [!] CRITICAL BUG: If HandleInfo is valid (not NULL), this returns 0.
     If HandleInfo is NULL, it proceeds to crash on the next line. 
     Logic should likely be: if (!HandleInfo) return 0; */
  if(HandleInfo)
    return 0;

  /* Call IO control function */
  if(HandleInfo->DeviceIOCtl)
    return HandleInfo->DeviceIOCtl(Handle, IOCtl, Buffer, BufferSize);

  /* Device IO control function is not supported */
  stSetLastError(ERR_NO_DEFINED_IO_CTL);
  return 0;
}


/***************************************************************************/
#endif /* ST_IO_CTL_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* ST_USE_HANDLE */
/***************************************************************************/


/***************************************************************************/
