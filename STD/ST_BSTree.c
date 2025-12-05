/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_BSTree.c - Binary Search Tree (AVL Implementation)
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
#if (ST_USE_BSTREE)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stBSTreeInit
 *
 *  Description:
 *    Initializes the binary search tree structure.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    CmpFunc - Pointer to the user-defined comparison function.
 *
 ***************************************************************************/

void stBSTreeInit(struct TBSTree FAR *BSTree, TBSTreeCmpFunc CmpFunc)
{
  /* Initialize the tree control structure */
  BSTree->Root = NULL;
  BSTree->CmpFunc = CmpFunc;

  /* Initialize the Minimum value cache if enabled */
  #if (ST_BSTREE_FIRST_FUNC)
    BSTree->Min = NULL;
  #endif
}


/****************************************************************************
 *
 *  Name:
 *    stSTreeRotateLeft
 *
 *  Description:
 *    Performs a left rotation on a specified node to restore AVL balance.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Root - The root node of the subtree to be rotated.
 *
 ***************************************************************************/

static void stBSTreeRotateLeft(struct TBSTree FAR *BSTree,
  struct TBSTreeNode FAR *Root)
{
  struct TBSTreeNode FAR *Pivot;
  Pivot = Root->Right;

  /* Update balance factors for the rotated nodes */
  Root->Balance -= 1 + ((Pivot->Balance > 0) ? Pivot->Balance : 0);
  Pivot->Balance -= 1 - ((Root->Balance < 0) ? Root->Balance : 0);

  /* Relink the parent to point to the new subtree root (Pivot) */
  if(!Root->Parent)
    BSTree->Root = Pivot;
  else
    if(Root->Parent->Left == Root)
      Root->Parent->Left = Pivot;
    else
      Root->Parent->Right = Pivot;

  /* Adjust internal links to complete the rotation */
  Pivot->Parent = Root->Parent;
  Root->Parent = Pivot;
  Root->Right = Pivot->Left;
  if(Pivot->Left)
    Pivot->Left->Parent = Root;
  Pivot->Left = Root;
}


/****************************************************************************
 *
 *  Name:
 *    stSTreeRotateRight
 *
 *  Description:
 *    Performs a right rotation on a specified node to restore AVL balance.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Root - The root node of the subtree to be rotated.
 *
 ***************************************************************************/

static void stBSTreeRotateRight(struct TBSTree FAR *BSTree,
  struct TBSTreeNode FAR *Root)
{
  struct TBSTreeNode FAR *Pivot;
  Pivot = Root->Left;

  /* Update balance factors for the rotated nodes */
  Root->Balance += 1 - ((Pivot->Balance < 0) ? Pivot->Balance : 0);
  Pivot->Balance += 1 + ((Root->Balance > 0) ? Root->Balance : 0);

  /* Relink the parent to point to the new subtree root (Pivot) */
  if(!Root->Parent)
    BSTree->Root = Pivot;
  else
    if(Root->Parent->Left == Root)
      Root->Parent->Left = Pivot;
    else
      Root->Parent->Right = Pivot;

  /* Adjust internal links to complete the rotation */
  Pivot->Parent = Root->Parent;
  Root->Parent = Pivot;
  Root->Left = Pivot->Right;
  if(Pivot->Right)
    Pivot->Right->Parent = Root;
  Pivot->Right = Root;
}


/****************************************************************************
 *
 *  Name:
 *    stBSTreeInsert
 *
 *  Description:
 *    Inserts data into the binary search tree. Fails if the data already
 *    exists.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Node - Pointer to the new node structure to be inserted.
 *    Existing - Output parameter. If a duplicate is found, the address of
 *      the conflicting node is stored here. Pass NULL if not needed.
 *    Data - Pointer to the user data.
 *
 *  Return:
 *    TRUE on success, FALSE on failure (duplicate found).
 *
 ***************************************************************************/

BOOL stBSTreeInsert(struct TBSTree FAR *BSTree,
  struct TBSTreeNode FAR *Node, struct TBSTreeNode FAR **Existing,
  PVOID Data)
{
  struct TBSTreeNode FAR *Parent;

  #if (ST_BSTREE_FIRST_FUNC)
    BOOL IsNewMin;
  #endif

  /* Initialize the new node structure */
  Node->Balance = 0;
  Node->Data = Data;
  Node->Left = NULL;
  Node->Right = NULL;

  /* Handle the empty tree case (inserting the first node) */
  if(!BSTree->Root)
  {
    Node->Parent = NULL;
    BSTree->Root = Node;

    #if (ST_BSTREE_FIRST_FUNC)
      BSTree->Min = Node;
    #endif

    return TRUE;
  }

  /* Optimization: Use a flag to identify the new minimum immediately,
     avoiding the need for redundant comparisons later. */
  #if (ST_BSTREE_FIRST_FUNC)
    IsNewMin = TRUE;
  #endif

  /* Traverse the tree to find the correct insertion point */
  Parent = BSTree->Root;
  while(TRUE)
  {
    /* Compare keys */
    int CmpResult = BSTree->CmpFunc(Data, Parent->Data);

    /* Duplicate keys are not allowed; handle the collision */
    if(!CmpResult)
    {
      /* Return the pointer to the existing node via the output parameter */
      if(Existing)
        *Existing = Parent;

      return FALSE;
    }

    /* Target is smaller: move to the left child */
    if(CmpResult < 0)
    {
      if(!Parent->Left)
      {
        /* Update the cached minimum pointer if this new node is the
           new smallest value */
        #if (ST_BSTREE_FIRST_FUNC)
          if(IsNewMin)
            BSTree->Min = Node;
        #endif

        Parent->Left = Node;
        break;
      }

      Parent = Parent->Left;
    }

    /* Target is larger: move to the right child */
    else
    {
      if(!Parent->Right)
      {
        Parent->Right = Node;
        break;
      }

      Parent = Parent->Right;

      /* If we move right, the new node cannot be the global minimum
         (it is larger than the current parent) */
      #if (ST_BSTREE_FIRST_FUNC)
        IsNewMin = FALSE;
      #endif
    }
  }

  /* Link the new node to its parent */
  Node->Parent = Parent;

  /* Backtrack up the tree to restore AVL balance */
  while(TRUE)
  {
    /* Move up to the parent */
    Parent = Node->Parent;
    if(!Parent)
      break;

    /* Update the parent's balance factor */
    Parent->Balance += (Parent->Left == Node) ? -1 : 1;
    if(!Parent->Balance)
      break;

    /* Perform rotation if the balance factor violates AVL rules */
    if(Parent->Balance > 1)
    {
      if(Parent->Right->Balance == -1)
        stBSTreeRotateRight(BSTree, Node);
      stBSTreeRotateLeft(BSTree, Parent);
      break;
    }

    else if(Parent->Balance < -1)
    {
      if(Parent->Left->Balance == 1)
        stBSTreeRotateLeft(BSTree, Node);
      stBSTreeRotateRight(BSTree, Parent);
      break;
    }

    /* Continue propagating changes up the tree */
    Node = Parent;
  }

  /* Success */
  return TRUE;
}


/****************************************************************************
 *
 *  Name:
 *    stBSTreeRemove
 *
 *  Description:
 *    Removes the specified node from the tree. The node must exist in the
 *    tree; otherwise, the structure may become corrupted.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Node - Pointer to the node to be removed.
 *
 ***************************************************************************/

void stBSTreeRemove(struct TBSTree FAR *BSTree,
  struct TBSTreeNode FAR *Node)
{
  struct TBSTreeNode FAR *Parent;
  struct TBSTreeNode FAR *Child;
  struct TBSTreeNode FAR *Max;

  /* Update the minimum value cache if we are removing the current minimum.
     Since the minimum node has no left child by definition:
     1. If it has a right child, that child becomes the new minimum.
     2. Otherwise, its parent becomes the new minimum. */
  #if (ST_BSTREE_FIRST_FUNC)
    if(Node == BSTree->Min)
      BSTree->Min = Node->Right ? Node->Right : Node->Parent;
  #endif

  /* Case 1: The node has a left child.
     We must find the in-order predecessor to replace the deleted node. */
  if(Node->Left)
  {
    /* Find the predecessor (the rightmost node in the left subtree) */
    Max = Node->Left;
    while(Max->Right)
      Max = Max->Right;

    /* Replace the node to be removed with its predecessor */
    Parent = Node->Parent;
    if(!Parent)
      BSTree->Root = Max;
    else
      if(Parent->Left == Node)
        Parent->Left = Max;
      else
        Parent->Right = Max;

    /* Special Case: The predecessor is the immediate left child of the
       node being removed. */
    if(Max->Parent == Node)
    {
      /* The predecessor moves up, effectively taking over the balance factor
         plus the weight of the right subtree. */
      Parent = Max;
      Parent->Balance = Node->Balance + 1;
    }

    /* General Case: The predecessor is deeper in the left subtree. */
    else
    {
      /* The predecessor is leaving its current position. Update its old
         parent's balance (it effectively lost a right child). */
      Parent = Max->Parent;
      Parent->Balance--;

      /* The predecessor may have had a left child. Move that child up
         to the predecessor's old position. */
      Child = Max->Left;
      Parent->Right = Child;
      if(Child)
        Child->Parent = Parent;

      /* Adopt the balance factor of the removed node. */
      Max->Balance = Node->Balance;

      /* Link the left subtree of the removed node to the new position. */
      Child = Node->Left;
      Max->Left = Child;
      if(Child)
        Child->Parent = Max;
    }

    /* Link the right subtree of the removed node to the new position. */
    Child = Node->Right;
    Max->Right = Child;
    if(Child)
      Child->Parent = Max;

    /* Update the parent pointer of the replacement node. */
    Max->Parent = Node->Parent;
  }

  /* Case 2: The node has no left child.
     Simply replace the node with its right child (or NULL). */
  else
  {
    /* Identify the replacement child (if any) */
    Parent = Node->Parent;
    Child = Node->Right;
    if(Child)
      Child->Parent = Parent;

    /* Relink the parent to the child. The parent's balance will change,
       triggering a rebalance. */
    if(!Parent)
      BSTree->Root = Child;
    else
      if(Parent->Left == Node)
      {
        Parent->Left = Child;
        Parent->Balance++;
      }
      else
      {
        Parent->Right = Child;
        Parent->Balance--;
      }
  }

  /* Backtrack up the tree to restore AVL balance */
  Child = Parent;
  while(Child)
  {
    /* Save parent pointer for next iteration */
    Parent = Child->Parent;

    /* If balance is within limits (+/-1), the height change didn't
       violate AVL rules. Stop. */
    if((Child->Balance == -1) || (Child->Balance == 1))
      break;

    /* Perform rotation if necessary */
    if(Child->Balance > 1)
    {
      if(Child->Right->Balance == -1)
        stBSTreeRotateRight(BSTree, Child->Right);
      stBSTreeRotateLeft(BSTree, Child);
      Child = Child->Parent;
    }

    else if(Child->Balance < -1)
    {
      if(Child->Left->Balance == 1)
        stBSTreeRotateLeft(BSTree, Child->Left);
      stBSTreeRotateRight(BSTree, Child);
      Child = Child->Parent;
    }

    /* Check stop condition again after rotation */
    if((Child->Balance == -1) || (Child->Balance == 1))
      break;

    /* Propagate balance change to the parent */
    if(Parent)
      Parent->Balance += ((Parent->Left == Child) ? 1 : -1);

    /* Continue up the tree */
    Child = Parent;
  }
}


/****************************************************************************
 *
 *  Name:
 *    stBSTreeSearch
 *
 *  Description:
 *    Searches for a specific value in the tree.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Data - Pointer to the data key to search for.
 *
 *  Return:
 *    Pointer to the found data, or NULL if not found.
 *
 ***************************************************************************/

PVOID stBSTreeSearch(struct TBSTree FAR *BSTree, PVOID Data)
{
  struct TBSTreeNode FAR *Node;

  /* Start search from the root */
  Node = BSTree->Root;
  while(Node)
  {
    /* Compare keys */
    int CmpResult = BSTree->CmpFunc(Data, Node->Data);

    /* Match found */
    if(!CmpResult)
      return Node->Data;

    /* Continue search in the appropriate subtree */
    Node = (CmpResult < 0) ? Node->Left : Node->Right;
  }

  /* Not found */
  return NULL;
}


/***************************************************************************/
#if (ST_BSTREE_FIRST_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stBSTreeGetFirst
 *
 *  Description:
 *    Returns a pointer to the smallest (first) value in the tree.
 *    This is an O(1) operation due to the cached Min pointer.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *
 *  Return:
 *    Pointer to the data, or NULL if the tree is empty.
 *
 ***************************************************************************/

INLINE PVOID stBSTreeGetFirst(struct TBSTree FAR *BSTree)
{
  /* Return the cached minimal element */
  return BSTree->Min ? BSTree->Min->Data : NULL;
}


/***************************************************************************/
#endif /* ST_BSTREE_FIRST_FUNC */
/***************************************************************************/


/***************************************************************************/
#if (ST_BSTREE_NEXT_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stBSTreeGetNext
 *
 *  Description:
 *    Returns the value immediately following the specified data in sort
 *    order (the in-order successor).
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    Data - Pointer to the reference data.
 *
 *  Return:
 *    Pointer to the successor data, or NULL if not found / no successor.
 *
 ***************************************************************************/

PVOID stBSTreeGetNext(struct TBSTree FAR *BSTree, PVOID Data)
{
  struct TBSTreeNode FAR *Node;

  /* Traverse to find the reference node */
  Node = BSTree->Root;
  while(Node)
  {
    /* Compare keys */
    int CmpResult = BSTree->CmpFunc(Data, Node->Data);

    /* Match found */
    if(!CmpResult)
    {
      /* Successor logic:
         [!] ISSUE? Ensure this logic matches traversal requirements.
         If right child exists, usually we want the smallest node in
         that right subtree. */
      if(Node->Right)
        while(Node->Left)
          Node = Node->Left;
      else
        Node = Node->Parent;

      /* Return the node data */
      return Node ? Node->Data : NULL;
    }

    /* Continue search */
    Node = (CmpResult < 0) ? Node->Left : Node->Right;
  }

  /* Node not found */
  return NULL;
}


/***************************************************************************/
#endif /* ST_BSTREE_NEXT_FUNC */
/***************************************************************************/



/***************************************************************************/
#if (ST_BSTREE_EXCHANGE_FUNC)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stBSTreeExchange
 *
 *  Description:
 *    Replaces the physical node structure 'CurNode' with 'NewNode' in the
 *    tree, preserving all connections.
 *    WARNING: Nodes must contain identical data keys. Otherwise, the tree
 *    structure will be corrupted.
 *
 *  Parameters:
 *    BSTree - Pointer to the tree control block.
 *    CurNode - Pointer to the current node in the tree.
 *    NewNode - Pointer to the new node structure to replace it with.
 *
 ***************************************************************************/

void stBSTreeExchange(struct TBSTree FAR *BSTree,
  struct TBSTreeNode FAR *CurNode, struct TBSTreeNode FAR *NewNode)
{
  struct TBSTreeNode FAR *Ptr;

  /* Copy balance factor */
  NewNode->Balance = CurNode->Balance;

  /* Update parent linkage */
  Ptr = CurNode->Parent;
  NewNode->Parent = Ptr;
  if(!Ptr)
    BSTree->Root = NewNode;
  else
    if(Ptr->Left == CurNode)
      Ptr->Left = NewNode;
    else
      Ptr->Right = NewNode;

  /* Update left child linkage */
  Ptr = CurNode->Left;
  NewNode->Left = Ptr;
  if(Ptr)
    Ptr->Parent = NewNode;

  /* Update right child linkage */
  Ptr = CurNode->Right;
  NewNode->Right = Ptr;
  if(Ptr)
    Ptr->Parent = NewNode;

  /* Update the Minimum cache if necessary */
  #if (ST_BSTREE_FIRST_FUNC)
    if(BSTree->Min == CurNode)
      BSTree->Min = NewNode;
  #endif
}


/***************************************************************************/
#endif /* ST_BSTREE_EXCHANGE_FUNC */
/***************************************************************************/


/***************************************************************************/
#endif /* ST_USE_BSTREE */
/***************************************************************************/


/***************************************************************************/
