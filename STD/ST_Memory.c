/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_Memory.c - Memory Management
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
#if (ST_USE_MEMORY)
/***************************************************************************/


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Memory Block descriptor */
struct TMemoryBlock
{
  SIZE Size;

  struct TMemoryBlock FAR *Prev;
  struct TMemoryBlock FAR *Next;

  struct TBSTreeNode Node;
  struct TMemoryBlock FAR *PrevDup;
  struct TMemoryBlock FAR *NextDup;
};

/* Memory Pool descriptor */
struct TMemoryPool
{
  struct TBSTree FreeBlocks;

  #if (ST_USE_SAFE_MEMORY_FREE)
    struct TBSTree OccupiedBlocks;
  #endif

  SIZE TotalSize;

  #if (ST_GET_MEMORY_INFO_FUNC)
    SIZE FreeSize;
  #endif

  #if (ST_MEMORY_EXPAND_FUNC)
    struct TMemoryPool FAR *NextMemoryPool;
  #endif
};


/***************************************************************************/
#if (ST_USE_SAFE_MEMORY_FREE)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stOccupiedCmp
 *
 *  Description:
 *    Compares two memory blocks by their addresses.
 *
 *  Parameters:
 *    Item1 - Pointer to the first memory block.
 *    Item2 - Pointer to the second memory block.
 *
 *  Return:
 *    Returned value is:
 *    - less than zero (-1) if Item1 is less than Item2,
 *    - equal to zero (0) if Item1 is the same as Item2,
 *    - greater than zero (1) if Item1 is greater than Item2.
 *
 ***************************************************************************/

static int stOccupiedCmp(PVOID Item1, PVOID Item2)
{
  /* Compare memory blocks by addresses */
  if(Item1 < Item2)
    return -1;

  if(Item1 > Item2)
    return 1;

  return 0;
}


/***************************************************************************/
#endif /* ST_USE_SAFE_MEMORY_FREE */
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stFreeCmp
 *
 *  Description:
 *    Compares two memory blocks by their size.
 *    Note: If compared items are equal (same size), the BST insertion
 *    function will return failure (duplicate). In this case, the caller
 *    will append the block to the duplicate list of the existing node.
 *
 *  Parameters:
 *    Item1 - Pointer to the first (new) memory block.
 *    Item2 - Pointer to the second (current) memory block.
 *
 *  Return:
 *    Returned value is:
 *    - less than zero (-1) if Item1 is smaller than Item2,
 *    - equal to zero (0) if Item1 is the same size as Item2,
 *    - greater than zero (1) if Item1 is larger than Item2.
 *
 ***************************************************************************/

static int stFreeCmp(PVOID Item1, PVOID Item2)
{
  /* Compare memory blocks by size */
  if(((struct TMemoryBlock FAR *) Item1)->Size <
    ((struct TMemoryBlock FAR *) Item2)->Size)
    return -1;

  if(((struct TMemoryBlock FAR *) Item1)->Size >
    ((struct TMemoryBlock FAR *) Item2)->Size)
    return 1;

  /* Items are equal */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    stInsertFreeBlock
 *
 *  Description:
 *    Inserts a memory block definition into the tree of free blocks.
 *    If a block with the specified size already exists in the tree,
 *    the new block is added to the duplicated blocks chain.
 *
 *  Parameters:
 *    BSTree - Pointer to the binary search tree descriptor.
 *    Block - Pointer to the memory block descriptor.
 *
 ***************************************************************************/

static void stInsertFreeBlock(struct TBSTree FAR *BSTree,
  struct TMemoryBlock FAR *Block)
{
  struct TBSTreeNode FAR *Existing;
  struct TMemoryBlock FAR *Curr;

  /* Insert block definition into the tree of free blocks */
  if(stBSTreeInsert(BSTree, &Block->Node, &Existing, Block))
  {
    Block->NextDup = NULL;
    Block->PrevDup = NULL;
  }
  else
  {
    /* Duplicate found; link into the duplicate chain */
    Curr = (struct TMemoryBlock FAR *) Existing->Data;

    Block->NextDup = Curr->NextDup;
    Block->PrevDup = Curr;

    if(Curr->NextDup)
      Curr->NextDup->PrevDup = Block;
    Curr->NextDup = Block;
  }
}


/****************************************************************************
 *
 *  Name:
 *    stMemoryInit
 *
 *  Description:
 *    Initializes the Memory Management functions for the specified
 *    memory pool. The memory pool cannot be used until this function
 *    finalizes its initialization.
 *
 *  Parameters:
 *    MemoryPool - Memory pool start address.
 *    MemorySize - Total size of the memory pool.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stMemoryInit(PVOID MemoryPool, SIZE MemorySize)
{
  SIZE MemoryPoolDescSize, FreeSize;
  struct TMemoryPool FAR *MemPool;
  struct TMemoryBlock FAR *MemoryBlock;


  /* Calculate size of the memory pool descriptor located at the
     beginning of the specified memory address. Locating the descriptor
     here also prevents memory allocation at the NULL address (if the pool
     starts at 0), which is reserved. */
  MemoryPoolDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TMemoryPool));

  /* Check memory size */
  if(MemorySize < MemoryPoolDescSize)
  {
    stSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Calculate size of the free memory available after alignment */
  FreeSize = (MemorySize & ((SIZE) ~((AR_MEMORY_ALIGNMENT) - 1))) -
    MemoryPoolDescSize;

  /* Initialize memory pool descriptor */
  MemPool = (struct TMemoryPool FAR *) MemoryPool;
  stBSTreeInit(&MemPool->FreeBlocks, stFreeCmp);

  #if (ST_USE_SAFE_MEMORY_FREE)
    stBSTreeInit(&MemPool->OccupiedBlocks, stOccupiedCmp);
  #endif

  MemPool->TotalSize = MemorySize;

  #if (ST_GET_MEMORY_INFO_FUNC)
    MemPool->FreeSize = FreeSize;
  #endif

  #if (ST_MEMORY_EXPAND_FUNC)
    MemPool->NextMemoryPool = NULL;
  #endif

  /* Define the initial single large free memory block */
  MemoryBlock = (struct TMemoryBlock FAR *)
    (PVOID) &((UINT8 FAR *) MemoryPool)[MemoryPoolDescSize];
  MemoryBlock->Size = FreeSize;
  MemoryBlock->PrevDup = NULL;
  MemoryBlock->NextDup = NULL;
  MemoryBlock->Prev = NULL;
  MemoryBlock->Next = NULL;
  
  stBSTreeInsert(&MemPool->FreeBlocks, &MemoryBlock->Node,
    NULL, MemoryBlock);


  /* Return with success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    stMemoryAlloc
 *
 *  Description:
 *    Allocates a new memory block. The allocated memory block address
 *    will be aligned to the value specified in ST_MALLOC_ALIGNMENT
 *    relative to the memory pool start address.
 *
 *  Parameters:
 *    MemoryPool - Memory pool start address.
 *    Size - Size of the memory to be allocated.
 *
 *  Return:
 *    Address of the newly allocated block of memory, or NULL on failure.
 *
 ***************************************************************************/

PVOID stMemoryAlloc(PVOID MemoryPool, SIZE Size)
{
  SIZE NewSize, BlockDescSize;
  struct TMemoryPool FAR *MemPool;
  struct TMemoryBlock FAR *Block;
  struct TMemoryBlock FAR *NextDup;
  struct TMemoryBlock FAR *NewFree;
  struct TBSTreeNode FAR *Node;

  /* Check parameters */
  if(!Size)
  {
    stSetLastError(ERR_INVALID_PARAMETER);
    return NULL;
  }

  /* Calculate the new size with additional bytes required for the memory
     block definition. Size variable overflow is controlled. */
  BlockDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TMemoryBlock));
  NewSize = BlockDescSize + AR_MEMORY_ALIGN_UP(Size);
  if(NewSize < Size)
  {
    stSetLastError(ERR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  Size = NewSize;

  /* Memory pool cast */
  MemPool = (struct TMemoryPool FAR *) MemoryPool;


  /* Check each memory pool (if expansion is used) */
  while(TRUE)
  {
    /* Enter critical section (required only in multitasking) */
    #if (OS_USED)
      BOOL PrevLockState;
      PrevLockState = arLock();
    #endif

    /* Find free memory block with the smallest possible size (Best Fit) */
    Block = NULL;
    Node = MemPool->FreeBlocks.Root;
    while(Node)
    {
      if(((struct TMemoryBlock FAR *) Node->Data)->Size < Size)
        Node = Node->Right;
      else
      {
        Block = (struct TMemoryBlock FAR *) Node->Data;
        if(Block->Size > Size)
          Node = Node->Left;
        else
          break;
      }
    }

    /* Free block not found in this pool */
    if(!Block)
    {
      /* Get next memory pool address if defined */
      #if (ST_MEMORY_EXPAND_FUNC)
        MemPool = MemPool->NextMemoryPool;
      #endif

      /* Leave critical section */
      #if (OS_USED)
        arRestore(PrevLockState);
      #endif

      /* Check in another memory pool */
      #if (ST_MEMORY_EXPAND_FUNC)
        if(MemPool)
          continue;
      #endif

      /* No more memory pools, free block not found anywhere */
      break;
    }

    /* Remove block from free blocks. If a block with the same size exists
       (duplicate), use the duplicate first to avoid tree rebalancing. */
    NextDup = Block->NextDup;
    if(NextDup)
    {
      Block->NextDup = NextDup->NextDup;
      if(NextDup->NextDup)
        NextDup->NextDup->PrevDup = Block;

      Block = NextDup;
    }
    else
      stBSTreeRemove(&MemPool->FreeBlocks, &Block->Node);

    /* If the free block is big enough to store the requested size plus
       another minimal block, split it. */
    if(Block->Size > (Size + BlockDescSize + AR_MEMORY_ALIGNMENT))
    {
      /* Define new free block from the remainder */
      NewFree = (struct TMemoryBlock FAR *)
        (PVOID) &((UINT8 FAR *) Block)[Size];
      NewFree->Size = Block->Size - Size;
      NewFree->Prev = Block;
      NewFree->Next = Block->Next;

      /* Update pointers */
      if(Block->Next)
        Block->Next->Prev = NewFree;
      Block->Next = NewFree;

      /* Update the size of the allocated block (for info only) */
      #if (ST_GET_MEMORY_INFO_FUNC)
        Block->Size = Size;
      #endif

      /* Insert the new free block definition back into the tree */
      stInsertFreeBlock(&MemPool->FreeBlocks, NewFree);
    }


    /* Block is now occupied, store it in the occupied blocks tree */
    #if (ST_USE_SAFE_MEMORY_FREE)
      stBSTreeInsert(&MemPool->OccupiedBlocks, &Block->Node, NULL, Block);
    #endif

    /* Update free size statistics */
    #if (ST_GET_MEMORY_INFO_FUNC)
      MemPool->FreeSize -= Block->Size;
    #endif

    /* Mark block as occupied (Size = 0 is the marker for occupied blocks) */
    Block->Size = 0;

    /* Leave critical section */
    #if (OS_USED)
      arRestore(PrevLockState);
    #endif

    /* Return address of the allocated block (after the header) */
    return (PVOID) (((UINT8 FAR *) Block) + BlockDescSize);
  }

  /* Cannot allocate memory for new block */
  stSetLastError(ERR_NOT_ENOUGH_MEMORY);
  return NULL;
}


/****************************************************************************
 *
 *  Name:
 *    stMemoryFree
 *
 *  Description:
 *    Releases an allocated memory block.
 *
 *  Parameters:
 *    MemoryPool - Memory pool start address.
 *    Ptr - Address of the memory block to be released.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stMemoryFree(PVOID MemoryPool, PVOID Ptr)
{
  BOOL CheckPrev;
  SIZE BlockDescSize;
  struct TMemoryPool FAR *MemPool;
  struct TMemoryBlock FAR *Block;
  struct TMemoryBlock FAR *Merge;
  struct TMemoryBlock FAR *Tmp;

  #if (OS_USED)
    BOOL PrevLockState;
  #endif


  /* Memory pool pointer */
  MemPool = (struct TMemoryPool FAR *) MemoryPool;

  /* Determine in which memory pool the specified block is located */
  #if (ST_MEMORY_EXPAND_FUNC)
    while(TRUE)
    {
      /* Check that pointer is located in this memory pool */
      if(Ptr > ((PVOID) MemPool))
        if(Ptr < ((PVOID) &((UINT8 FAR *) MemPool)[MemPool->TotalSize]))
          break;

      /* Check next memory pool */
      MemPool = MemPool->NextMemoryPool;

      /* Return if no more memory pools are defined */
      if(!MemPool)
      {
        stSetLastError(ERR_INVALID_MEMORY_BLOCK);
        return FALSE;
      }
    }
  #endif


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif


  /* Size of memory block definition */
  BlockDescSize = AR_MEMORY_ALIGN_UP(sizeof(struct TMemoryBlock));

  /* Find specified memory block structure by its address */
  Block = (struct TMemoryBlock FAR *)
    (PVOID) (((UINT8 FAR *) Ptr) - BlockDescSize);

  #if (ST_USE_SAFE_MEMORY_FREE)
    if(Block != (struct TMemoryBlock FAR *) stBSTreeSearch(
      &MemPool->OccupiedBlocks, Block))
    {
      stSetLastError(ERR_INVALID_MEMORY_BLOCK);
      return FALSE;
    }

    /* Remove occupied block from the occupied set */
    stBSTreeRemove(&MemPool->OccupiedBlocks, &Block->Node);
  #endif

  /* Mark block as free by calculating its actual size */
  Block->Size = (SIZE) ((Block->Next ? ((UINT8 FAR *) Block->Next) :
    &((UINT8 FAR *) MemPool)[MemPool->TotalSize]) - ((UINT8 FAR *) Block));

  #if (ST_GET_MEMORY_INFO_FUNC)
    MemPool->FreeSize += Block->Size;
  #endif

  /* Merge next and previous blocks if they are free.
     The loop below passes exactly two times to check both neighbors. */
  CheckPrev = FALSE;
  while(TRUE)
  {
    Merge = CheckPrev ? Block->Prev : Block->Next;
    if(Merge)
      /* Check if the neighbor is free (Free blocks have Size > 0) */
      if(Merge->Size)
      {
        /* Remove the neighbor block from the Free Tree/List before merging */
        Tmp = Merge->NextDup;
        if(!Merge->PrevDup)
        {
          if(!Tmp)
            stBSTreeRemove(&MemPool->FreeBlocks, &Merge->Node);
          else
          {
            stBSTreeExchange(&MemPool->FreeBlocks, &Merge->Node, &Tmp->Node);
            Tmp->PrevDup = NULL;
          }
        }
        else
        {
          Merge->PrevDup->NextDup = Tmp;
          if(Tmp)
           Tmp->PrevDup = Merge->PrevDup;
        }

        /* Update pointers and merge logic */
        if(CheckPrev)
        {
          Tmp = Block;
          Block = Merge;
          Merge = Tmp;
        }

        /* Physically merge the blocks */
        if(Merge->Next)
          Merge->Next->Prev = Block;
        Block->Next = Merge->Next;
        Block->Size += Merge->Size;
      }

    /* Check previous block in the next iteration */
    if(CheckPrev)
      break;
    CheckPrev = TRUE;
  }

  /* Store the new (possibly merged) free block into the Free Tree */
  stInsertFreeBlock(&MemPool->FreeBlocks, Block);


  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif

  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#if (ST_GET_MEMORY_INFO_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stMemoryGetInfo
 *
 *  Description:
 *    Returns information on specified memory capacities.
 *
 *  Parameters:
 *    MemoryPool - Memory pool start address.
 *    TotalMemory - Pointer to receive the total size of the memory.
 *    FreeMemory - Pointer to receive the size of the free memory.
 *
 ***************************************************************************/

void stMemoryGetInfo(PVOID MemoryPool, ULONG *TotalMemory,
  ULONG *FreeMemory)
{
  struct TMemoryPool FAR *MemPool;

  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    BOOL PrevLockState;

    PrevLockState = arLock();
  #endif


  /* Memory pool pointer */
  MemPool = (struct TMemoryPool FAR *) MemoryPool;

  /* Capacities of the first memory pool */
  if(TotalMemory)
    *TotalMemory = MemPool->TotalSize;

  if(FreeMemory)
    *FreeMemory = MemPool->FreeSize;

  /* Capacities of the additional memory pools */
  #if (ST_MEMORY_EXPAND_FUNC)
    while(TRUE)
    {
      MemPool = MemPool->NextMemoryPool;
      if(!MemPool)
        break;

      if(TotalMemory)
        *TotalMemory += MemPool->TotalSize;

      if(FreeMemory)
        *FreeMemory += MemPool->FreeSize;
    }
  #endif


  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif
}


/***************************************************************************/
#endif /* ST_GET_MEMORY_INFO_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (ST_MEMORY_EXPAND_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stMemoryExpand
 *
 *  Description:
 *    Allows connecting multiple memory pools into one larger pool managed
 *    by the Memory Management functions.
 *    For example, if 1MB of memory space at address 0x00100000 was defined
 *    at initialization, stMemoryExpand allows adding additional memory
 *    blocks (e.g., at 0x00200000).
 *    Note: The memory located at NULL (0x00000000) can only be defined
 *    by the stMemoryInit function.
 *
 *  Parameters:
 *    MemoryPool - Memory pool start address (existing).
 *    MemoryAddress - Address of the new memory pool to attach (cannot be NULL).
 *    MemorySize - Total size of the new memory pool.
 *
 *  Return:
 *    TRUE on success or FALSE on failure.
 *
 ***************************************************************************/

BOOL stMemoryExpand(PVOID MemoryPool, PVOID MemoryAddress, SIZE MemorySize)
{
  #if (OS_USED)
    BOOL PrevLockState;
  #endif

  /* Memory address cannot be NULL */
  if(!MemoryAddress)
  {
    stSetLastError(ERR_INVALID_PARAMETER);
    return FALSE;
  }

  /* Initialize the new memory pool */
  if(!stMemoryInit(MemoryAddress, MemorySize))
    return FALSE;


  /* Enter critical section (required only in multitasking) */
  #if (OS_USED)
    PrevLockState = arLock();
  #endif

  /* Insert new memory pool definition into the linked list */
  ((struct TMemoryPool FAR *) MemoryAddress)->NextMemoryPool =
    ((struct TMemoryPool FAR *) MemoryPool)->NextMemoryPool;
  ((struct TMemoryPool FAR *) MemoryPool)->NextMemoryPool =
    ((struct TMemoryPool FAR *) MemoryAddress);

  /* Leave critical section */
  #if (OS_USED)
    arRestore(PrevLockState);
  #endif


  /* Return with success */
  return TRUE;
}


/***************************************************************************/
#endif /* ST_MEMORY_EXPAND_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* ST_USE_MEMORY */
/***************************************************************************/


/***************************************************************************/
