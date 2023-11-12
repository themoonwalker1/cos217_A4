/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"


/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
    Node_T oNParent = NULL;
    Node_T oNChild = NULL;
    Path_T oPNPath, oPPPath, oSPath;
    size_t index;

    /* Sample check: a NULL pointer is not a valid node */
    if (oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    oNParent = Node_getParent(oNNode);
    if (oNParent == NULL) {
        return TRUE;
    }

    oPNPath = Node_getPath(oNNode);
    oPPPath = Node_getPath(oNParent);

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

    /* Check if siblings have unique paths */
    for (index = 0;
         index < Node_getNumChildren(oNParent); index++) {
        if (Node_getChild(oNParent, index, &oNChild) != SUCCESS) {
            fprintf(stderr,
                    "Failed to retrieve a sibling node\n");
            return FALSE;
        }

        /* Compare path of the current sibling with oNNode */
        if (oNChild != oNNode
            && !Path_comparePath(oSPath = Node_getPath(oNChild),
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
CheckerDT_sortedSiblings(Node_T oNPrevChild, Node_T oNChild) {

    assert(oNPrevChild != NULL);
    assert(oNChild != NULL);

    if (Path_compareString(Node_getPath(oNChild),
                           Path_getPathname(Node_getPath(
                                   oNPrevChild))) <= 0) {
        fprintf(stderr,
                "Children are not in sorted order:\n\t- %s\n\t- %s\n",
                Node_toString(oNPrevChild), Node_toString(oNChild));
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
static boolean CheckerDT_treeCheck(Node_T oNNode) {
    size_t ulIndex;

    if (oNNode == NULL) {
        return TRUE;
    }

    /* Sample check on each node: node must be valid */
    /* If not, pass that failure back up immediately */
    if (!CheckerDT_Node_isValid(oNNode))
        return FALSE;

    /* Recur on every child of oNNode */
    for (ulIndex = 0;
         ulIndex < Node_getNumChildren(oNNode); ulIndex++) {

        Node_T oNChild = NULL;
        Node_T oNPrevChild = NULL;

        if (Node_getChild(oNNode, ulIndex, &oNChild) != SUCCESS) {
            fprintf(stderr,
                    "getNumChildren claims more children than "
                    "getChild returns\n");
            return FALSE;
        }

        if (Node_getChild(oNNode, ulIndex - 1, &oNPrevChild) !=
            SUCCESS) {
            fprintf(stderr,
                    "Failed to retrieve the "
                    "previous sibling node\n");
            return FALSE;
        }

        /* Check if child is in the correct position (sorted order) */
        if (ulIndex > 0 &&
            !CheckerDT_sortedSiblings(oNPrevChild, oNChild))
            return FALSE;

        /* if recurring down one subtree results in a failed check
           farther down, passes the failure back up immediately */
        if (!CheckerDT_treeCheck(oNChild))
            return FALSE;
    }
    return TRUE;
}


/* Counts and returns the actual number of nodes existing in the tree
   with root oNNode. */
static size_t CheckerDT_countNodes(Node_T oNNode) {
    size_t count = 1;
    size_t ulIndex;

    if (oNNode == NULL) {
        return 0;
    }

    for (ulIndex = 0;
         ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
        Node_T oNChild = NULL;
        int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
        if (iStatus == SUCCESS) {
            count += CheckerDT_countNodes(oNChild);
        }
    }
    return count;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
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

    if (ulCount != (temp = CheckerDT_countNodes(oNRoot))) {
        fprintf(stderr,
                "Actual node count (%lu) does not "
                "match the expected count (%lu)\n",
                (unsigned long) temp, (unsigned long) ulCount);
        return FALSE;
    }

    /* Now checks invariants recursively at each node from the root. */
    return CheckerDT_treeCheck(oNRoot);
}


