/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_BSTree.h - Binary Search Tree (AVL Implementation)
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/***************************************************************************/
#ifndef ST_BSTREE_H
#define ST_BSTREE_H
/***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "AR_API.h"


/****************************************************************************
 *
 *  Default configuration
 *
 ***************************************************************************/

/* Enable Binary Search Tree management by default */
#ifndef ST_USE_BSTREE
  #define ST_USE_BSTREE                 1
#elif (((ST_USE_BSTREE) != 0) && ((ST_USE_BSTREE) != 1))
  #error ST_USE_BSTREE must be either 0 or 1
#endif

/* Enable stBSTreeGetFirst (O(1) minimum lookup) by default */
#ifndef ST_BSTREE_FIRST_FUNC
  #define ST_BSTREE_FIRST_FUNC         1
#elif (((ST_BSTREE_FIRST_FUNC) != 0) && ((ST_BSTREE_FIRST_FUNC) != 1))
  #error ST_BSTREE_FIRST_FUNC must be either 0 or 1
#elif (((ST_BSTREE_FIRST_FUNC) != 0) && ((ST_USE_BSTREE) != 1))
  #error ST_BSTREE_FIRST_FUNC must be 0 when ST_USE_BSTREE is 0
#endif

/* Enable stBSTreeGetNext (in-order successor) by default */
#ifndef ST_BSTREE_NEXT_FUNC
  #define ST_BSTREE_NEXT_FUNC         1
#elif (((ST_BSTREE_NEXT_FUNC) != 0) && ((ST_BSTREE_NEXT_FUNC) != 1))
  #error ST_BSTREE_NEXT_FUNC must be either 0 or 1
#elif (((ST_BSTREE_NEXT_FUNC) != 0) && ((ST_USE_BSTREE) != 1))
  #error ST_BSTREE_NEXT_FUNC must be 0 when ST_USE_BSTREE is 0
#endif

/* Enable stBSTreeExchange (node replacement) by default */    
#ifndef ST_BSTREE_EXCHANGE_FUNC
  #define ST_BSTREE_EXCHANGE_FUNC         1
#elif (((ST_BSTREE_EXCHANGE_FUNC) != 0) && ((ST_BSTREE_EXCHANGE_FUNC) != 1))
  #error ST_BSTREE_EXCHANGE_FUNC must be either 0 or 1
#elif (((ST_BSTREE_EXCHANGE_FUNC) != 0) && ((ST_USE_BSTREE) != 1))
  #error ST_BSTREE_EXCHANGE_FUNC must be 0 when ST_USE_BSTREE is 0
#endif


/****************************************************************************
 *
 *  Type definitions
 *
 ***************************************************************************/

/* User-defined comparison function type for tree nodes */
typedef int (* FAR TBSTreeCmpFunc) (PVOID Item1, PVOID Item2);

/* Binary Search Tree node structure */
struct TBSTreeNode
{
  PVOID Data;

  int Balance;
  struct TBSTreeNode FAR *Parent;
  struct TBSTreeNode FAR *Left;
  struct TBSTreeNode FAR *Right;
};

/* Binary Search Tree control structure */
struct TBSTree
{
  struct TBSTreeNode FAR *Root;
  TBSTreeCmpFunc CmpFunc;

  #if (ST_BSTREE_FIRST_FUNC)
    struct TBSTreeNode FAR *Min;
  #endif            
};


/****************************************************************************
 *
 *  Functions
 *
 ***************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

  #if (ST_USE_BSTREE)

    void stBSTreeInit(struct TBSTree FAR *BSTree, TBSTreeCmpFunc CmpFunc);

    BOOL stBSTreeInsert(struct TBSTree FAR *BSTree,
      struct TBSTreeNode FAR *Node, struct TBSTreeNode FAR **Existing,
      PVOID Data);

    void stBSTreeRemove(struct TBSTree FAR *BSTree,
      struct TBSTreeNode FAR *Node);

    PVOID stBSTreeSearch(struct TBSTree FAR *BSTree, PVOID Data);

    #if (ST_BSTREE_FIRST_FUNC)
      INLINE PVOID stBSTreeGetFirst(struct TBSTree FAR *BSTree);
    #endif

    #if (ST_BSTREE_NEXT_FUNC)
      INLINE PVOID stBSTreeGetNext(struct TBSTree FAR *BSTree, PVOID Data);
    #endif

    #if (ST_BSTREE_EXCHANGE_FUNC)
      void stBSTreeExchange(struct TBSTree FAR *BSTree,
        struct TBSTreeNode FAR *CurNode, struct TBSTreeNode FAR *NewNode);
    #endif

  #endif

#ifdef __cplusplus
  };
#endif


/***************************************************************************/
#endif /* ST_BSTREE_H */
/***************************************************************************/
