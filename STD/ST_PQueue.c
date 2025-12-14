/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_PQueue.c - Priority Queue
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
#if (ST_USE_PQUEUE)
/***************************************************************************/


/****************************************************************************
 *
 *  Definitions
 *
 ***************************************************************************/

#define ST_PQUEUE_NOT_TREE_NODE         100


/****************************************************************************
 *
 *  Name:
 *    stPQueueInit
 *
 *  Description:
 *    Initializes a priority queue.
 *
 *  Parameters:
 *    PQueue - Pointer to the priority queue descriptor.
 *    CmpFunc - Priority queue comparison function pointer.
 *
 ***************************************************************************/

void stPQueueInit(struct TPQueue FAR *PQueue, TPQueueCmpFunc CmpFunc)
{
  /* Initialize the tree descriptor */
  stBSTreeInit(&PQueue->Tree, (TBSTreeCmpFunc) CmpFunc);
}


/****************************************************************************
 *
 *  Name:
 *    stPQueueInsert
 *
 *  Description:
 *    Inserts the specified data into the priority queue.
 *    If an item with the same priority (key) already exists, the new item
 *    is appended to the list of items with that priority (FIFO order).
 *
 *  Parameters:
 *    PQueue - Pointer to the Priority Queue descriptor.
 *    Item - Pointer to the Priority Queue item descriptor.
 *    Data - Pointer to the data.
 *
 ***************************************************************************/

void stPQueueInsert(struct TPQueue FAR *PQueue, struct TPQueueItem FAR *Item,
  PVOID Data)
{
  struct TBSTreeNode FAR *ExistingNode;

  /* Insert data into the tree */
  if(stBSTreeInsert(&PQueue->Tree, &Item->Node, &ExistingNode, Data))
  {
    /* If unique, initialize the circular list */
    Item->Prev = Item;
    Item->Next = Item;
  }

  /* If data already exists, append it to the queue (duplicate key) */
  else
  {
    struct TPQueueItem FAR *ExistingItem;

    /* Pointer to the tree node (already existing item) */
    ExistingItem = (struct TPQueueItem FAR *) ExistingNode;

    /* Update pointers to insert at the end of the circular list */
    Item->Prev = ExistingItem->Prev;
    Item->Next = ExistingItem;
    ExistingItem->Prev->Next = Item;
    ExistingItem->Prev = Item;

    /* Mark this item as not being a primary tree node */
    Item->Node.Balance = ST_PQUEUE_NOT_TREE_NODE;
  }
}


/****************************************************************************
 *
 *  Name:
 *    stPQueueRemove
 *
 *  Description:
 *    Removes the specified item from the priority queue.
 *
 *  Parameters:
 *    PQueue - Pointer to the priority queue descriptor.
 *    Item - Pointer to the priority queue item descriptor to remove.
 *
 ***************************************************************************/

void stPQueueRemove(struct TPQueue FAR *PQueue, struct TPQueueItem FAR *Item)
{
  /* Unlink the item from the circular list */
  Item->Prev->Next = Item->Next;
  Item->Next->Prev = Item->Prev;

  /* Check if the item is the primary node in the BST */
  if(Item->Node.Balance != ST_PQUEUE_NOT_TREE_NODE)
  {
    /* If the item is the only one in the queue with this priority,
       remove the node from the tree entirely */
    if(Item->Next == Item)
      stBSTreeRemove(&PQueue->Tree, &Item->Node);

    /* Otherwise, transfer the tree node identity to the next item */
    else
    {
      stBSTreeExchange(&PQueue->Tree, &Item->Node, &Item->Next->Node);

      /* Mark the removed item (which now holds the old balance)
         as not a BST node */
      Item->Node.Balance = ST_PQUEUE_NOT_TREE_NODE;
    }
  }
}


/****************************************************************************
 *
 *  Name:
 *    stPQueueGet
 *
 *  Description:
 *    Returns a pointer to the first item (highest priority) in the
 *    priority queue.
 *
 *  Parameters:
 *    PQueue - Pointer to the priority queue descriptor.
 *
 *  Return:
 *    Pointer to the data, or NULL if the priority queue is empty.
 *
 ***************************************************************************/

INLINE PVOID stPQueueGet(struct TPQueue FAR *PQueue)
{
  /* Returns pointer to the first item in the priority queue */
  return stBSTreeGetFirst(&PQueue->Tree);
}


/****************************************************************************
 *
 *  Name:
 *    stPQueueRotate
 *
 *  Description:
 *    Rotates the queue at a specific priority level.
 *    This effectively moves the first item of a priority group to the end,
 *    or the last item to the beginning, by swapping the Tree Node identity.
 *
 *  Parameters:
 *    PQueue - Pointer to the priority queue descriptor.
 *    Item - Pointer to the beginning item of the group to be rotated.
 *      If NULL, the highest priority group (Tree.Min) is rotated.
 *    Forward - If TRUE, removes the first item and puts it at the end
 *      of the queue. Otherwise, removes the last item and puts it at
 *      the beginning.
 *
 ***************************************************************************/

void stPQueueRotate(struct TPQueue FAR *PQueue, struct TPQueueItem FAR *Item,
  BOOL Forward)
{
  /* Obtain pointer to the first item (highest priority) if not specified */
  if(!Item)
    Item = (struct TPQueueItem FAR *) PQueue->Tree.Min;

  /* Perform rotation if the item exists */
  if(Item)
    if(Item->Next != Item)
    {
      /* Exchange tree node identity with the neighbor */
      stBSTreeExchange(&PQueue->Tree, &Item->Node,
        (Forward ? &Item->Next->Node : &Item->Prev->Node));

      /* Mark the old head item as not a BST node anymore */
      Item->Node.Balance = ST_PQUEUE_NOT_TREE_NODE;
    }
}


/***************************************************************************/
#endif /* ST_USE_PQUEUE */
/***************************************************************************/


/***************************************************************************/
