/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_PQueue.h - Priority Queue
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_PQUEUE_H
#define ST_PQUEUE_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"
#include "ST_BSTree.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Priority Queue functions are enabled by default when Binary Search Trees
   are also enabled */
#ifndef ST_USE_PQUEUE
  #define ST_USE_PQUEUE                 (ST_USE_BSTREE)
#elif (((ST_USE_PQUEUE) != 0) && ((ST_USE_PQUEUE) != 1))
  #error Value of ST_USE_PQUEUE must be 0 or 1
#endif

/* Binary Search Trees must be enabled for the Priority Queue */
#if ((ST_USE_PQUEUE) && !(ST_USE_BSTREE))
  #error ST_USE_BSTREE must be set to 1 for Priority Queue
#endif

/* The stBSTreeGetFirst function must be enabled for the Priority Queue */
#if ((ST_USE_PQUEUE) && !(ST_BSTREE_FIRST_FUNC))
  #error ST_BSTREE_FIRST_FUNC must be set to 1 for Priority Queue
#endif

/* The stBSTreeExchange function must be enabled for the Priority Queue */
#if ((ST_USE_PQUEUE) && !(ST_BSTREE_EXCHANGE_FUNC))
  #error ST_BSTREE_EXCHANGE_FUNC must be set to 1 for Priority Queue
#endif


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* Priority Queue item comparison function type */
typedef TBSTreeCmpFunc TPQueueCmpFunc;

/* Priority Queue item descriptor */
struct TPQueueItem
{
  struct TBSTreeNode Node;
  struct TPQueueItem FAR *Prev;
  struct TPQueueItem FAR *Next;
};

/* Priority Queue control structure */
struct TPQueue
{
  struct TBSTree Tree;
};


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_PQUEUE)

    void stPQueueInit(struct TPQueue FAR *PQueue, TPQueueCmpFunc CmpFunc);

    void stPQueueInsert(struct TPQueue FAR *PQueue,
      struct TPQueueItem FAR *Item, PVOID Data);
    void stPQueueRemove(struct TPQueue FAR *PQueue,
      struct TPQueueItem FAR *Item);

    INLINE PVOID stPQueueGet(struct TPQueue FAR *PQueue);
    void stPQueueRotate(struct TPQueue FAR *PQueue,
      struct TPQueueItem FAR *Item, BOOL Forward);

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_PQUEUE_H */
/***************************************************************************/
