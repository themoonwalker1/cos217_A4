/*--------------------------------------------------------------------*/
/* checkerFT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerFT.h"
#include "dynarray.h"
#include "path.h"

static void CheckerFT_addSubDirectory(NodeFT_T oNNode, DynArray_T oDChildren, boolean bIsFile) {
    size_t ulIndex;
    NodeFT_T oNTempNode;

    assert(oNNode != NULL);
    assert(oDChildren != NULL);

    for (ulIndex = 0;
         ulIndex < NodeFT_getNumChildren(oNNode, bIsFile); ulIndex++) {
        NodeFT_getChild(oNNode, ulIndex, bIsFile, &oNTempNode);
        DynArray_add(oDChildren, oNTempNode);
    }
}

static DynArray_T CheckerFT_combineChildren(NodeFT_T oNNode) {
    DynArray_T oDChildren = DynArray_new(0);

    assert(oNNode != NULL);

    if (NodeFT_isFile(oNNode) == TRUE) {
        return oDChildren;
    }

    CheckerFT_addSubDirectory(oNNode, oDChildren, FALSE);
    CheckerFT_addSubDirectory(oNNode, oDChildren, TRUE);

    return oDChildren;
}


/* see checkerFT.h for specification */
boolean CheckerFT_Node_isValid(NodeFT_T oNNode) {
    NodeFT_T oNParent = NULL;
    NodeFT_T oNChild = NULL;
    Path_T oPNPath, oPPPath, oSPath;
    size_t ulIndex;
    DynArray_T oDChildren;

    /* Sample check: a NULL pointer is not a valid node */
    if (oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    oNParent = NodeFT_getParent(oNNode);
    if (oNParent == NULL) {
        return TRUE;
    }

    oPNPath = NodeFT_getPath(oNNode);
    oPPPath = NodeFT_getPath(oNParent);

    /* Sample check: parent's path must be the longest possible
       proper prefix of the node's path */

    if (Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
        Path_getDepth(oPNPath) - 1) {
        fprintf(stderr,
                "P-C nodes don't have P-C paths: (%s) (%s)\n",
                Path_getPathname(oPPPath),
                Path_getPathname(oPNPath));
        return FALSE;
    }

    oDChildren = CheckerFT_combineChildren(oNParent);
    /* Check if siblings have unique paths */
    for (ulIndex = 0;
         ulIndex < DynArray_getLength(oDChildren); ulIndex++) {
        if ((oNChild = DynArray_get(oDChildren, ulIndex)) == NULL) {
            fprintf(stderr,
                    "Failed to retrieve a sibling node\n");
            return FALSE;
        }

        /* Compare path of the current sibling with oNNode */
        if (oNChild != oNNode
            && !Path_comparePath(oSPath = NodeFT_getPath(oNChild),
                                 oPNPath)) {
            fprintf(stderr,
                    "Siblings have non-unique paths: "
                    "(%s) (%s)\n",
                    Path_getPathname(oPNPath),
                    Path_getPathname(oSPath));
            return FALSE;
        }
    }


    return TRUE;
}

/* Checks whether oNPrevChild is "less than or equal to" oNChild. Return
   TRUE if it is "less than or equal to", otherwise return FALSE.
   Precondition: oNPrevChild and oNChild are not NULL. */
static boolean
CheckerFT_sortedSiblings(NodeFT_T oNPrevChild, NodeFT_T oNChild) {

    assert(oNPrevChild != NULL);
    assert(oNChild != NULL);

    if (Path_compareString(NodeFT_getPath(oNChild),
                           Path_getPathname(NodeFT_getPath(
                                   oNPrevChild))) <= 0) {
        fprintf(stderr,
                "Children are not in sorted order\n");
        return FALSE;
    }
    return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerFT_treeCheck(NodeFT_T oNNode) {
    size_t ulIndex;
    DynArray_T oDChildren;

    if (oNNode == NULL) {
        return TRUE;
    }

    /* Sample check on each node: node must be valid */
    /* If not, pass that failure back up immediately */
    if (!CheckerFT_Node_isValid(oNNode))
        return FALSE;

    /* Recur on every child of oNNode */
    oDChildren = CheckerFT_combineChildren(oNNode);
    for (ulIndex = 0;
         ulIndex < DynArray_getLength(oDChildren); ulIndex++) {

        NodeFT_T oNChild = NULL;
        NodeFT_T oNPrevChild = NULL;

        if ((oNChild = DynArray_get(oDChildren, ulIndex)) == NULL) {
            fprintf(stderr,
                    "getNumChildren claims more children than "
                    "getChild returns\n");
            return FALSE;
        }

        if (ulIndex > 0) {
            if ((oNPrevChild = DynArray_get(oDChildren, ulIndex)) == NULL) {
                fprintf(stderr,
                        "Failed to retrieve the "
                        "previous sibling node\n");
                return FALSE;
            }

            /* Check if child is in the correct position (sorted order) */
            if (NodeFT_isFile(oNPrevChild) == NodeFT_isFile(oNChild) && !CheckerFT_sortedSiblings(oNPrevChild, oNChild))
                return FALSE;
        }
        /* if recurring down one subtree results in a failed check
           farther down, passes the failure back up immediately */
        if (!CheckerFT_treeCheck(oNChild))
            return FALSE;
    }
    return TRUE;
}


/* Counts and returns the actual number of nodes existing in the tree
   with root oNNode. */
static size_t CheckerFT_countNodes(NodeFT_T oNNode) {
    size_t count = 1;
    size_t ulIndex;
    DynArray_T oDChildren;

    if (oNNode == NULL) {
        return 0;
    }

    oDChildren = CheckerFT_combineChildren(oNNode);

    for (ulIndex = 0;
         ulIndex < DynArray_getLength(oDChildren); ulIndex++) {
        NodeFT_T oNChild;
        if ((oNChild = DynArray_get(oDChildren, ulIndex)) != NULL) {
            count += CheckerFT_countNodes(oNChild);
        }
    }
    return count;
}

/* see checkerFT.h for specification */
boolean CheckerFT_isValid(boolean bIsInitialized, NodeFT_T oNRoot,
                          size_t ulCount) {
    size_t temp;

    if (!bIsInitialized) {
        if (ulCount != 0) {
            fprintf(stderr, "Not initialized, but count is not 0\n");
            return FALSE;
        }
        if (oNRoot != NULL) {
            fprintf(stderr,
                    "Not initialized, but root is not null\n");
            return FALSE;
        }
    } else {
        if (ulCount != 0 && oNRoot == NULL) {
            fprintf(stderr,
                    "Initialized and non-empty, "
                    "but root is still null\n");
            return FALSE;
        } else if (ulCount == 0 && oNRoot != NULL) {
            fprintf(stderr,
                    "Initialized and empty, but root is not null\n");
            return FALSE;
        }
    }

    if (ulCount != (temp = CheckerFT_countNodes(oNRoot))) {
        fprintf(stderr,
                "Actual node count (%lu) does not "
                "match the expected count (%lu)\n",
                (unsigned long) temp, (unsigned long) ulCount);
        return FALSE;
    }

    /* Now checks invariants recursively at each node from the root. */
    return CheckerFT_treeCheck(oNRoot);
}


