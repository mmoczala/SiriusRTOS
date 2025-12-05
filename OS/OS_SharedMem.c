/****************************************************************************
 *
 *  SiriusRTOS
 *  OS_SharedMem.c - Shared memory object management functions
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

#include "OS_Core.h"


/***************************************************************************/
#if (OS_USE_SHARED_MEM)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Shared memory object descriptor */
struct TShMemObject
{
  /* System object descriptor */
  struct TSysObject Object;

  /* System object name descriptor */
  #if (OS_OPEN_SH_MEM_FUNC)
    struct TObjectName Name;
  #endif

  /* Critical section descriptor */
  #if (OS_SH_MEM_PROTECT_MUTEX)
    struct TCriticalSection CS;
    UINT8 Mode;
  #endif
};


/****************************************************************************
 *
 *  Name:
 *    osCreateSharedMemory
 *
 *  Description:
 *    Creates a shared memory object. Use osWaitForObject to synchronize
 *    access to the shared memory.
 *
 *  Parameters:
 *    Name - Name of the object.
 *    Mode - Mode flags. Specifying values other than the following will
 *      cause function failure:
 *      OS_IPC_PROTECT_EVENT - Protected by auto-reset event.
 *      OS_IPC_PROTECT_MUTEX - Protected by mutex.
 *    Address - Pointer to a variable that receives the memory address
 *      (can be NULL if not needed).
 *    Size - Expected size of the shared memory.
 *
 *  Return:
 *    Handle of the created object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osCreateSharedMemory(SYSNAME Name, UINT8 Mode, PVOID *Address,
  ULONG Size)
{
  struct TShMemObject FAR *ShMemObject;
  struct TSysObject FAR *Object;

  /* Check parameters */
  #if (OS_SH_MEM_PROTECT_MUTEX)
    if(((Mode != OS_IPC_PROTECT_EVENT) && (Mode != OS_IPC_PROTECT_MUTEX))
      || !Size)
    {
      osSetLastError(ERR_INVALID_PARAMETER);
      return NULL_HANDLE;
    }
  #else
    if(!Size || (Mode != OS_IPC_PROTECT_EVENT))
    {
      osSetLastError(ERR_INVALID_PARAMETER);
      return NULL_HANDLE;
    }
  #endif

  /* Allocate memory for the object */
  ShMemObject = (struct TShMemObject FAR *)
    osMemAlloc(AR_MEMORY_ALIGN_UP(sizeof(*ShMemObject)) + Size);
  if(!ShMemObject)
    return NULL_HANDLE;

  /* Pointer to system object descriptor */
  Object = &ShMemObject->Object;

  /* Register new system object */
  if(!osRegisterObject((PVOID) ShMemObject, Object,
    OS_OBJECT_TYPE_SHARED_MEM))
  {
    osMemFree(ShMemObject);
    return NULL_HANDLE;
  }

  /* Register name descriptor */
  #if (OS_OPEN_SH_MEM_FUNC)
    if(!osRegisterName(Object, &ShMemObject->Name, Name))
    {
      osDeleteObject(Object);
      return NULL_HANDLE;
    }

  /* Mark unused parameters to avoid warning messages */
  #else
    AR_UNUSED_PARAM(Name);
  #endif

  /* Shared memory protection method */
  #if (OS_SH_MEM_PROTECT_MUTEX)
    ShMemObject->Mode = Mode;

    /* Register critical section descriptor */
    if(ShMemObject->Mode == OS_IPC_PROTECT_MUTEX)
      osRegisterCS(&Object->Signal, &ShMemObject->CS, 1, 1, TRUE);

    /* Setup the event */
    else
      Object->Signal.Flags |= OS_SIGNAL_FLAG_DEC_ON_RELEASE;

  /* Setup the event, when only this method is available */
  #else
    Object->Signal.Flags |= OS_SIGNAL_FLAG_DEC_ON_RELEASE;
  #endif

  /* Obtain address of the memory buffer */
  if(Address)
    *Address = &(((UINT8 FAR *) ShMemObject)[AR_MEMORY_ALIGN_UP(
      sizeof(*ShMemObject))]);

  /* Mark object as ready to use and return its handle */
  Object->Flags |= OS_OBJECT_FLAG_READY_TO_USE;
  return Object->Handle;
}


/***************************************************************************/
#if (OS_OPEN_SH_MEM_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osOpenSharedMemory
 *
 *  Description:
 *    Opens an existing shared memory object. Use osWaitForObject to
 *    synchronize access to the shared memory.
 *
 *  Parameters:
 *    Name - Name of the existing object.
 *    Address - Pointer to a variable that receives the memory address
 *      (can be NULL if not needed).
 *
 *  Return:
 *    Handle of the object or NULL_HANDLE on failure.
 *
 ***************************************************************************/

HANDLE osOpenSharedMemory(SYSNAME Name, PVOID *Address)
{
  struct TSysObject FAR *Object;

  /* Open named object */
  Object = osOpenNamedObject(Name, OS_OBJECT_TYPE_SHARED_MEM);
  if(!Object)
    return NULL_HANDLE;

  /* Obtain address of the memory */
  if(Address)
    *Address = &(((UINT8 FAR *) Object->ObjectDesc)[
      AR_MEMORY_ALIGN_UP(sizeof(struct TShMemObject))]);

  /* Return handle of the opened object or NULL_HANDLE on failure */
  return Object->Handle;
}


/***************************************************************************/
#endif /* OS_OPEN_SH_MEM_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (OS_GET_SH_MEM_ADDRESS_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osGetSharedMemoryAddress
 *
 *  Description:
 *    Returns the memory address of the specified shared memory object.
 *
 *  Parameters:
 *    Handle - Handle of the shared memory object.
 *
 *  Return:
 *    Address of the shared memory or NULL on failure.
 *
 ***************************************************************************/

PVOID osGetSharedMemoryAddress(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_SHARED_MEM);

  /* Address of the shared memory or NULL on failure */
  return !Object ? NULL : &(((UINT8 FAR *) Object->ObjectDesc)[
    AR_MEMORY_ALIGN_UP(sizeof(struct TShMemObject))]);
}


/***************************************************************************/
#endif /* OS_GET_SH_MEM_ADDRESS_FUNC */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    osReleaseSharedMemory
 *
 *  Description:
 *    Releases ownership of a specified shared memory object.
 *
 *  Parameters:
 *    Handle - Handle of the shared memory object.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL osReleaseSharedMemory(HANDLE Handle)
{
  struct TSysObject FAR *Object;

  /* Get object by handle */
  Object = osGetObjectByHandle(Handle, OS_OBJECT_TYPE_SHARED_MEM);
  if(!Object)
    return FALSE;

  #if (OS_SH_MEM_PROTECT_MUTEX)

    /* Release critical section */
    if(((struct TShMemObject FAR *)
      Object->ObjectDesc)->Mode == OS_IPC_PROTECT_MUTEX)
      return osReleaseCS(Object->Signal.CS, osCurrentTask, 1, NULL);

    /* Make release auto-reset event signaled */
    else
    {
      osUpdateSignalState(&Object->Signal, (INDEX) TRUE);
      return TRUE;
    }

  /* Make release auto-reset event signaled */
  #else
    osUpdateSignalState(&Object->Signal, (INDEX) TRUE);
    return TRUE;
  #endif
}


/***************************************************************************/
#endif /* OS_USE_SHARED_MEM */
/***************************************************************************/


/***************************************************************************/
