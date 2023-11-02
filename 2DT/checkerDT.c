/* checkerDT.c */
/* Author: */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"

/* Helper function to check if a node is not a NULL pointer */
static boolean isNotNull(Node_T oNNode) {
    if (oNNode == NULL) {
        (void)fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }
    return TRUE;
}

/* Helper function to check if parent's path is a proper prefix of the node's path */
static boolean hasProperParentPath(Node_T oNNode) {
    Node_T oNParent = Node_getParent(oNNode);
    if (oNParent != NULL) {
        Path_T oPNPath = Node_getPath(oNNode);
        Path_T oPPPath = Node_getPath(oNParent);

        if (Path_getSharedPrefixDepth(oPNPath, oPPPath) != Path_getDepth(oPNPath) - 1) {
            (void)fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                    Path_getPathname(oPPPath),
                    Path_getPathname(oPNPath));
            return FALSE;
        }
    }
    return TRUE;
}

/* Helper function to check if siblings have unique paths */
static boolean hasUniqueSiblings(Node_T oNNode) {
    Node_T oNParent = Node_getParent(oNNode);
    size_t index;

    for (index = 0; index < Node_getNumChildren(oNParent); index++) {
        Node_T oNChild;
        if (Node_getChild(oNParent, index, &oNChild) != SUCCESS) {
            (void)fprintf(stderr, "Failed to retrieve a sibling node\n");
            return FALSE;
        }

        if (oNChild != oNNode) {
            Path_T oSPath = Node_getPath(oNChild);
            if (Path_comparePath(oSPath, Node_getPath(oNNode)) == 0) {
                (void)fprintf(stderr, "Siblings have non-unique paths: (%s) (%s)\n",
                        Path_getPathname(Node_getPath(oNNode)),
                        Path_getPathname(oSPath));
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* Main validation function */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
    return isNotNull(oNNode) &&
           hasProperParentPath(oNNode) &&
           hasUniqueSiblings(oNNode);
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
    size_t ulIndex;

    if (oNNode != NULL) {
        /* Sample check on each node: node must be valid */
        /* If not, pass that failure back up immediately */
        if (!CheckerDT_Node_isValid(oNNode))
            return FALSE;

        /* Check if children are in sorted order */
        for (ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
            Node_T oNChild = NULL;
            int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

            if (iStatus != SUCCESS) {
                (void)fprintf(stderr,
                        "getNumChildren claims more children than "
                        "getChild returns\n");
                return FALSE;
            }

            /* Check if the child is in the correct position (sorted order) */
            if (ulIndex > 0) {
                Node_T oNPrevChild = NULL;
                int iPrevStatus = Node_getChild(oNNode, ulIndex - 1, &oNPrevChild);

                if (iPrevStatus != SUCCESS) {
                    (void)fprintf(stderr, "Failed to retrieve the previous sibling node\n");
                    return FALSE;
                }

                /* Compare the paths of the current child and the previous child */
                if (Path_compareString(Node_getPath(oNChild), Path_getPathname(Node_getPath(oNPrevChild))) <= 0) {
                    (void)fprintf(stderr, "Children are not in sorted order\n");
                    return FALSE;
                }
            }

            /* If recurring down one subtree results in a failed check
               farther down, pass the failure back up immediately */
            if (!CheckerDT_treeCheck(oNChild))
                return FALSE;
        }
    }

    return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
    if (!bIsInitialized) {
        if (ulCount != 0) {
            (void)fprintf(stderr, "Not initialized, but count is not 0\n");
            return FALSE;
        }
        if (oNRoot != NULL) {
            (void)fprintf(stderr, "Not initialized, but root is not null\n");
            return FALSE;
        }
    } else {
        if (ulCount != 0) {
            if (oNRoot == NULL) {
                (void)fprintf(stderr, "Initialized and non-empty, "
                                "but root is still null\n");
                return FALSE;
            }
        } else {
            if (oNRoot != NULL) {
                (void)fprintf(stderr, "Initialized and empty, "
                                "but root is not null\n");
            }
        }
    }

    /* Now checks invariants recursively at each node from the root. */
    return CheckerDT_treeCheck(oNRoot);
}
