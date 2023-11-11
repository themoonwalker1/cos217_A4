/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "nodeFT.h"
#include "checkerFT.h"

/*--------------------------------------------------------------------*/

/*
   A node in a FT that can represent either a directory or file. If the
   node represents a directory, then the subdirectories are populated.
   If the node represents a file, then contents and size are populated.
*/
struct NodeFT {
    /** Common Variables **/
    /* the object corresponding to a node's absolute path */
    Path_T oPPath;
    /* a node's parent */
    NodeFT_T oNParent;
    /* indicates whether a node is a file (TRUE) or a directory (FALSE) */
    boolean bIsFile;

    /** Directory Variables **/
    /* subdirectory of a node that stores files lexicographically */
    DynArray_T oDFiles;
    /* subdirectory of a node that stores directories
       lexicographically */
    DynArray_T oDDirs;

    /** File Variables **/
    /* pointer to a file contents of the node - can be null */
    void *pvContents;
    /* length of a file contents of the node - can be zero */
    size_t ulFileLength;
};

/*--------------------------------------------------------------------*/

/** HELPER FUNCTIONS **/
/*
   Compares the string representation of the absolute path of oNFirst
   with string pcSecond (which represents another Node's path).

   Returns:
   * <0 if oNFirst is "less than" pcSecond
   * 0 if oNFirst is "equal to" pcSecond
   * >0 if oNFirst is "greater than" pcSecond

   Precondition:
   * oNFirst cannot be NULL
   * pcSecond cannot be NULL
*/
static int NodeFT_compareString(const NodeFT_T oNFirst,
                                const char *pcSecond) {
    assert(oNFirst != NULL);
    assert(pcSecond != NULL);

    return Path_compareString(oNFirst->oPPath, pcSecond);
}

/*
   Retrieves the "correct" subdirectory of oNNode based on the value of
   bIsFile.

   Returns:
   * The FILES subdirectory of oNNode when bIsFile is TRUE
   * The DIRS subdirectory of oNNode when bIsFile is FALSE

   Preconditions:
   * oNNode cannot be NULL
   * oNNode is a directory node
*/
static DynArray_T NodeFT_getChildDynArray(NodeFT_T oNNode,
                                          boolean bIsFile) {
    assert(oNNode != NULL);
    assert(NodeFT_isFile(oNNode) == FALSE);

    if (bIsFile == TRUE)
        return oNNode->oDFiles;

    return oNNode->oDDirs;
}

/*
   Links child oNChild into the correct subdirectory of oNParent at
   ulIndex.

   Returns:
   * SUCCESS when oNChild is correctly and successfully inserted
   * MEMORY_ERROR if memory allocation fails for inserting oNChild

   Precondition:
   * oNParent cannot be NULL
   * oNChild cannot be NULL
   * oNParent is a directory node
*/
static int NodeFT_addChild(NodeFT_T oNParent, NodeFT_T oNChild) {
    DynArray_T oDChildren;
    size_t ulIndex = 0;

    assert(oNParent != NULL);
    assert(oNChild != NULL);
    assert(NodeFT_isFile(oNParent) == FALSE);


    oDChildren = NodeFT_getChildDynArray(oNParent, oNChild->bIsFile);

    DynArray_bsearch(
            oDChildren, (char *) Path_getPathname(oNChild->oPPath),
            &ulIndex,
            (int (*)(const void *, const void *)) NodeFT_compareString);

    if (DynArray_addAt(oDChildren, ulIndex, oNChild))
        return SUCCESS;

    return MEMORY_ERROR;
}

/*--------------------------------------------------------------------*/

int NodeFT_new(NodeFT_T oNParent, Path_T oPPath, void *pvContents,
               size_t ulLength, boolean bIsFile,
               NodeFT_T *poNResult) {
    struct NodeFT *psNew;
    Path_T oPParentPath = NULL;
    Path_T oPNewPath = NULL;
    size_t ulParentDepth;
    size_t ulIndex;
    int iStatus;

    assert(oPPath != NULL);
    assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));

    /* allocate space for a new node */
    psNew = malloc(sizeof(struct NodeFT));
    if (psNew == NULL) {
        *poNResult = NULL;
        *poNResult = NULL;
        return MEMORY_ERROR;
    }

    /* set the new node's path */
    iStatus = Path_dup(oPPath, &oPNewPath);
    if (iStatus != SUCCESS) {
        free(psNew);
        *poNResult = NULL;
        return iStatus;
    }
    psNew->oPPath = oPNewPath;

    /* validate and set the new node's parent */
    if (oNParent != NULL) {
        size_t ulSharedDepth;

        oPParentPath = oNParent->oPPath;
        ulParentDepth = Path_getDepth(oPParentPath);
        ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                  oPParentPath);
        /* parent must be an ancestor of child */
        if (ulSharedDepth < ulParentDepth) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return CONFLICTING_PATH;
        }

        /* parent must be exactly one level up from child */
        if (Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NO_SUCH_PATH;
        }

        /* parent must not already have child with this path */
        if (NodeFT_hasFile(oNParent, oPPath, &ulIndex)) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return ALREADY_IN_TREE;
        }

        /* parent must not already have child with this path */
        if (NodeFT_hasDir(oNParent, oPPath, &ulIndex)) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return ALREADY_IN_TREE;
        }
    } else {
        /* new node must be root */
        /* can only create one "level" at a time */
        if (Path_getDepth(psNew->oPPath) != 1) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NO_SUCH_PATH;
        }
    }

    psNew->oNParent = oNParent;

    if (bIsFile == FALSE) {
        /* initialize the new node */
        psNew->oDDirs = DynArray_new(0);
        if (psNew->oDDirs == NULL) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return MEMORY_ERROR;
        }

        /* initialize the new node */
        psNew->oDFiles = DynArray_new(0);
        if (psNew->oDFiles == NULL) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return MEMORY_ERROR;
        }

        psNew->bIsFile = FALSE;
        psNew->pvContents = NULL;
        psNew->ulFileLength = 0;
    } else {
        psNew->oDFiles = NULL;
        psNew->oDDirs = NULL;
        psNew->bIsFile = TRUE;
        psNew->pvContents = pvContents;
        psNew->ulFileLength = ulLength;
    }

    /* Link into parent's children list */
    if (oNParent != NULL) {
        iStatus = NodeFT_addChild(oNParent, psNew);
        if (iStatus != SUCCESS) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return iStatus;
        }
    }

    *poNResult = psNew;

    assert(oNParent == NULL || CheckerFT_Node_isValid(oNParent));
    assert(CheckerFT_Node_isValid(*poNResult));

    return SUCCESS;
}

size_t NodeFT_free(NodeFT_T oNNode) {
    size_t ulIndex;
    size_t ulCount = 0;

    assert(oNNode != NULL);
    assert(CheckerFT_Node_isValid(oNNode));

    /* remove from parent's list */
    if (oNNode->oNParent != NULL) {
        if (DynArray_bsearch(
                NodeFT_getChildDynArray(oNNode->oNParent,
                                        oNNode->bIsFile),
                oNNode, &ulIndex,
                (int (*)(const void *, const void *)) NodeFT_compare)
                )
            (void) DynArray_removeAt(
                    NodeFT_getChildDynArray(oNNode->oNParent,
                                            oNNode->bIsFile),
                    ulIndex);
    }

    if (oNNode->bIsFile == FALSE) {
        /* recursively remove Files */
        while (DynArray_getLength(oNNode->oDFiles) != 0) {
            ulCount += NodeFT_free(DynArray_get(oNNode->oDFiles, 0));
        }

        /* recursively remove directories */
        while (DynArray_getLength(oNNode->oDDirs) != 0) {
            ulCount += NodeFT_free(DynArray_get(oNNode->oDDirs, 0));
        }

        DynArray_free(oNNode->oDFiles);
        DynArray_free(oNNode->oDDirs);
    }

    /* remove path */
    Path_free(oNNode->oPPath);

    /* finally, free the struct node */
    free(oNNode);
    ulCount++;
    return ulCount;

}

boolean
NodeFT_hasFile(NodeFT_T oNParent, Path_T oPPath, size_t *pulChildId) {
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(pulChildId != NULL);

    return (DynArray_bsearch(oNParent->oDFiles,
                             (char *) Path_getPathname(oPPath),
                             pulChildId,
                             (int (*)(const void *,
                                      const void *)) NodeFT_compareString));
}

boolean
NodeFT_hasDir(NodeFT_T oNParent, Path_T oPPath, size_t *pulChildId) {
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(pulChildId != NULL);

    return (DynArray_bsearch(oNParent->oDDirs,
                             (char *) Path_getPathname(oPPath),
                             pulChildId,
                             (int (*)(const void *,
                                      const void *)) NodeFT_compareString));
}

Path_T NodeFT_getPath(NodeFT_T oNNode) {
    assert(oNNode != NULL);
    return oNNode->oPPath;
}

size_t NodeFT_getNumChildren(NodeFT_T oNParent, boolean bIsFile) {
    assert(oNParent != NULL);
    assert(NodeFT_isFile(oNParent) == FALSE);

    if (bIsFile == TRUE)
        return DynArray_getLength(oNParent->oDFiles);
    return DynArray_getLength(oNParent->oDDirs);
}

int NodeFT_getChild(NodeFT_T oNParent, size_t ulChildId,
                    boolean bIsFile, NodeFT_T *poNResult) {
    assert(oNParent != NULL);
    assert(poNResult != NULL);

    if (ulChildId >= NodeFT_getNumChildren(oNParent, bIsFile)) {
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }
    *poNResult = DynArray_get(
            NodeFT_getChildDynArray(oNParent, bIsFile), ulChildId);
    return SUCCESS;
}

NodeFT_T NodeFT_getParent(NodeFT_T oNNode) {
    assert(oNNode != NULL);

    return oNNode->oNParent;
}

void *NodeFT_getContents(NodeFT_T oNNode) {
    assert(oNNode != NULL);
    assert(oNNode->bIsFile == TRUE);

    return oNNode->pvContents;
}

void *NodeFT_setContents(NodeFT_T oNNode, void *pvContents,
                         size_t ulNewLength) {
    void *pvPrevContents;

    assert(oNNode != NULL);
    assert(oNNode->bIsFile == TRUE);

    pvPrevContents = oNNode->pvContents;
    oNNode->pvContents = pvContents;
    oNNode->ulFileLength = ulNewLength;

    return pvPrevContents;
}

size_t NodeFT_getFileSize(NodeFT_T oNNode) {
    assert (oNNode != NULL);

    return oNNode->ulFileLength;
}

boolean NodeFT_isFile(NodeFT_T oNNode) {
    assert(oNNode != NULL);

    return oNNode->bIsFile;
}

int NodeFT_compare(NodeFT_T oNFirst, NodeFT_T oNSecond) {
    assert(oNFirst != NULL);
    assert(oNSecond != NULL);

    return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *NodeFT_toString(NodeFT_T oNNode) {
    char *copyPath;

    assert(oNNode != NULL);

    copyPath = malloc(Path_getStrLength(NodeFT_getPath(oNNode)) + 1);
    if (copyPath == NULL)
        return NULL;
    else
        return strcpy(copyPath,
                      Path_getPathname(NodeFT_getPath(oNNode)));
}