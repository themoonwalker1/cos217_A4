/*--------------------------------------------------------------------*/
/* checkerFT.c                                                        */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include "checkerFT.h"
#include "dynarray.h"
#include "path.h"

/*--------------------------------------------------------------------*/

/** Helper Functions **/

/*
   Returns a combined DynArray_T of the child FILES and DIRECTORIES
   nodes under oNNode.

   ** Returned DynArray_T must be freed by the caller! **
*/
static DynArray_T CheckerFT_combineChildren(NodeFT_T oNNode) {
    DynArray_T oDChildren;
    size_t ulIndex1, ulIndex2;
    NodeFT_T oNTempNode = NULL;

    assert(oNNode != NULL);

    if (NodeFT_isFile(oNNode) == TRUE) {
        return DynArray_new(0);
    }

    oDChildren = DynArray_new(NodeFT_getNumChildren(oNNode, FALSE) +
                              NodeFT_getNumChildren(oNNode, TRUE));

    /* add Files */
    for (ulIndex1 = 0;
         ulIndex1 < NodeFT_getNumChildren(oNNode, TRUE); ulIndex1++) {
        NodeFT_getChild(oNNode, ulIndex1, TRUE, &oNTempNode);
        (void) DynArray_set(oDChildren, ulIndex1, oNTempNode);
    }

    /* add Directories */
    for (ulIndex2 = 0;
         ulIndex2 < NodeFT_getNumChildren(oNNode, FALSE); ulIndex2++) {
        NodeFT_getChild(oNNode, ulIndex2, FALSE, &oNTempNode);
        (void) DynArray_set(oDChildren, ulIndex1 + ulIndex2,
                            oNTempNode);
    }

    return oDChildren;
}

/*
   Checks whether oNPrevChild is "less than or equal to" oNChild. Return
   TRUE if it is "less than or equal to", otherwise return FALSE.
   Precondition: oNPrevChild and oNChild are not NULL.
*/
static boolean
CheckerFT_sortedSiblings(NodeFT_T oNPrevChild, NodeFT_T oNChild) {

    assert(oNPrevChild != NULL);
    assert(oNChild != NULL);

    /* not in lexicographic order */
    if (Path_compareString(NodeFT_getPath(oNChild),
                           Path_getPathname(NodeFT_getPath(
                                   oNPrevChild))) < 0) {
        fprintf(stderr,
                "Children are not in lexicographic order: (%s) (%s)\n",
                Path_getPathname(NodeFT_getPath(oNPrevChild)),
                Path_getPathname(NodeFT_getPath(oNChild)));
        return FALSE;
    }

    /* duplicate children */
    if (Path_compareString(NodeFT_getPath(oNChild),
                           Path_getPathname(NodeFT_getPath(
                                   oNPrevChild))) == 0) {
        fprintf(stderr,
                "Siblings have non-unique paths: (%s) (%s)\n",
                Path_getPathname(NodeFT_getPath(oNPrevChild)),
                Path_getPathname(NodeFT_getPath(oNChild)));
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

    /* Node must be valid */
    /* If not, pass that failure back up immediately */
    if (!CheckerFT_Node_isValid(oNNode))
        return FALSE;

    /* Recur on every child of oNNode */
    oDChildren = CheckerFT_combineChildren(oNNode);
    for (ulIndex = 0;
         ulIndex < DynArray_getLength(oDChildren); ulIndex++) {

        NodeFT_T oNChild = NULL;
        NodeFT_T oNPrevChild = NULL;

        /* less children than expected */
        if ((oNChild = DynArray_get(oDChildren, ulIndex)) == NULL) {
            fprintf(stderr,
                    "getNumChildren claims more children than "
                    "getChild returns\n");
            DynArray_free(oDChildren);
            return FALSE;
        }

        /* check if nodes are sorted lexicographically */
        if (ulIndex > 0) {
            if ((oNPrevChild = DynArray_get(oDChildren, ulIndex - 1)) ==
                NULL) {
                fprintf(stderr,
                        "Failed to retrieve the "
                        "previous sibling node\n");
                DynArray_free(oDChildren);
                return FALSE;
            }

            if (NodeFT_isFile(oNPrevChild) == NodeFT_isFile(oNChild) &&
                !CheckerFT_sortedSiblings(oNPrevChild, oNChild)) {
                DynArray_free(oDChildren);
                return FALSE;
            }
        }

        /* if recurring down one subtree results in a failed check
           farther down, passes the failure back up immediately */
        if (!CheckerFT_treeCheck(oNChild)) {
            DynArray_free(oDChildren);
            return FALSE;
        }
    }
    DynArray_free(oDChildren);

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

    DynArray_free(oDChildren);
    return count;
}

/*--------------------------------------------------------------------*/

boolean CheckerFT_Node_isValid(NodeFT_T oNNode) {
    NodeFT_T oNParent = NULL;
    NodeFT_T oNChild = NULL;
    Path_T oPNPath, oPPPath, oSPath;
    size_t ulIndex;
    DynArray_T oDChildren;

    /* A NULL pointer is not a valid node */
    if (oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    /* Node is root, so cannot check parent or sibling invariants */
    oNParent = NodeFT_getParent(oNNode);
    if (oNParent == NULL) {
        return TRUE;
    }

    oPNPath = NodeFT_getPath(oNNode);
    oPPPath = NodeFT_getPath(oNParent);

    /* Parent's path must be the longest possible proper prefix of the
       node's path */
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
            DynArray_free(oDChildren);
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
            DynArray_free(oDChildren);
            return FALSE;
        }
    }
    DynArray_free(oDChildren);

    return TRUE;
}

boolean CheckerFT_isValid(boolean bIsInitialized, NodeFT_T oNRoot,
                          size_t ulCount) {
    size_t temp;

    /* Global Invariants for Hierarchy */
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

    /* ulCount represents the actual number of nodes in the tree */
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

/*--------------------------------------------------------------------*/