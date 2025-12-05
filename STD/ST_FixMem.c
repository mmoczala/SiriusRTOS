/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_FixMem.c - Fixed Size Memory Management
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
#if (ST_USE_FIXMEM)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Fixed Memory Pool descriptor */
struct TFixedMemDesc
{
  SIZE BlockSize;       /* Size of a single block (aligned) */
  SIZE BlockCount;      /* Total capacity in blocks */
  UINT8 FAR *MemSpace;  /* Pointer to the start of the data area */

  SIZE Count;           /* Number of blocks allocated from the linear area */
  UINT8 FAR *FirstFree; /* Head of the LIFO free list (recycled blocks) */
};


/****************************************************************************
 *
 *  Name:
 *    stFixedMemInit
 *
 *  Description:
 *    Initializes a specified memory buffer for fixed-size memory allocation.
 *    The allocated block addresses are aligned according to the
 *    AR_MEMORY_ALIGNMENT constant, relative to the base MemoryAddress.
 *
 *    Note: These functions do not set error codes (stSetLastError) and do
 *    not provide internal synchronization. The caller must ensure thread
 *    safety if the pool is shared between tasks.
 *
 *  Parameters:
 *    MemoryAddress - Pointer to the raw memory buffer.
 *    MemorySize - Total size of the buffer in bytes.
 *    BlockSize - Desired size of a single memory block.
 *
 *  Return:
 *    TRUE on success, FALSE on failure (e.g., buffer too small).
 *
 ***************************************************************************/

BOOL stFixedMemInit(PVOID MemoryAddress, SIZE MemorySize, SIZE BlockSize)
{
  struct TFixedMemDesc FAR *MemDesc;
  SIZE HeaderSize;

  /* Align values to architecture requirements */
  BlockSize = AR_MEMORY_ALIGN_UP(BlockSize);
  HeaderSize = AR_MEMORY_ALIGN_UP(sizeof(struct TFixedMemDesc));

  /* Block size must be large enough to hold a pointer (for the free list) */
  if(BlockSize < sizeof(UINT8 FAR *))
    return FALSE;

  /* Check memory size (must hold at least the header and one block) */
  if(HeaderSize + BlockSize > MemorySize)
    return FALSE;

  /* Initialize the memory descriptor at the start of the buffer */
  MemDesc = (struct TFixedMemDesc FAR *) MemoryAddress;
  MemDesc->BlockSize = BlockSize;
  MemDesc->BlockCount = (MemorySize - HeaderSize) / BlockSize;
  MemDesc->MemSpace = &((UINT8 FAR *) MemoryAddress)[HeaderSize];
  MemDesc->Count = 0;
  MemDesc->FirstFree = NULL;

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    stFixedMemAlloc
 *
 *  Description:
 *    Allocates a free memory block from the specified pool.
 *    It prioritizes reusing previously freed blocks (from the free list)
 *    before carving new blocks from the uninitialized area.
 *
 *  Parameters:
 *    MemoryAddress - Pointer to the initialized memory pool buffer.
 *
 *  Return:
 *    Pointer to the allocated memory block, or NULL if the pool is full.
 *
 ***************************************************************************/

PVOID stFixedMemAlloc(PVOID MemoryAddress)
{
  struct TFixedMemDesc FAR *MemDesc;
  PVOID RetAddress;

  /* Get pointer to the memory descriptor */
  MemDesc = (struct TFixedMemDesc FAR *) MemoryAddress;

  /* Strategy 1: Reuse a block from the free list */
  if(MemDesc->FirstFree)
  {
    RetAddress = MemDesc->FirstFree;

    /* Pop the block from the list (the block contains the 'next' pointer) */
    MemDesc->FirstFree = *((UINT8 FAR **) (PVOID) MemDesc->FirstFree);
  }

  /* Strategy 2: Allocate a new block from the linear space */
  else
  {
    /* Check if the linear space is exhausted */
    if(MemDesc->Count >= MemDesc->BlockCount)
      RetAddress = NULL;

    /* Allocate the next available block */
    else
    {
      RetAddress = &((UINT8 FAR *) MemDesc->MemSpace)[
        MemDesc->Count * MemDesc->BlockSize];
      MemDesc->Count++;
    }
  }

  /* Return the address of the allocated block */
  return RetAddress;
}


/****************************************************************************
 *
 *  Name:
 *    stFixedMemFree
 *
 *  Description:
 *    Releases a previously allocated memory block back to the pool.
 *    The block is added to the internal free list for future reuse.
 *
 *  Parameters:
 *    MemoryAddress - Pointer to the initialized memory pool buffer.
 *    Address - Pointer to the memory block to release.
 *
 *  Return:
 *    TRUE on success, FALSE if the address is out of the pool's range.
 *
 ***************************************************************************/

BOOL stFixedMemFree(PVOID MemoryAddress, PVOID Address)
{
  struct TFixedMemDesc FAR *MemDesc;

  /* Get pointer to the memory descriptor */
  MemDesc = (struct TFixedMemDesc FAR *) MemoryAddress;

  /* Validate that the address falls within the pool's data area */
  if((Address < ((PVOID) MemDesc->MemSpace)) || (Address > ((PVOID)
    &MemDesc->MemSpace[MemDesc->BlockCount * MemDesc->BlockSize])))
    return FALSE;

  /* Push the block onto the 'FirstFree' LIFO list */
  /* Store the current head pointer inside the block being freed */
  *((UINT8 FAR **) Address) = MemDesc->FirstFree;

  /* Update the head pointer to point to this block */
  MemDesc->FirstFree = (UINT8 FAR *) Address;
  return TRUE;
}


/***************************************************************************/
#endif /* ST_USE_FIXMEM */
/***************************************************************************/


/***************************************************************************/
