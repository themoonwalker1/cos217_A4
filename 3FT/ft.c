/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Praneeth Bhandaru                                    */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "checkerFT.h"
#include "nodeFT.h"
#include "ft.h"

/*
  A Directory Tree is a representation of a hierarchy of directories,
  represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static NodeFT_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

static int FT_traversePath(Path_T oPPath, NodeFT_T *poNFurthest) {
    int iStatus;
    Path_T oPPrefix = NULL;
    NodeFT_T oNCurr;
    NodeFT_T oNChild = NULL;
    size_t ulDepth;
    size_t i;
    size_t ulChildID;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if (oNRoot == NULL) {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if (iStatus != SUCCESS) {
        *poNFurthest = NULL;
        return iStatus;
    }

    if (Path_comparePath(NodeFT_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    for (i = 2; i <= ulDepth; i++) {
        boolean *pbIsFile = calloc(1, sizeof(boolean));

        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if (iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }
        iStatus = NodeFT_hasChild(oNCurr, oPPrefix, pbIsFile,
                                  &ulChildID);
        if (iStatus == TRUE) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = NodeFT_getChild(oNCurr, ulChildID, *pbIsFile,
                                      &oNChild);
            if (iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
        } else {
            /* oNCurr doesn't have child with path oPPrefix:
               this is as far as we can go */
            break;
        }
    }

    Path_free(oPPrefix);
    *poNFurthest = oNCurr;
    return SUCCESS;
}

static int FT_findNode(const char *pcPath, NodeFT_T *poNResult) {
    Path_T oPPath = NULL;
    NodeFT_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if (!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }
    iStatus = FT_traversePath(oPPath, &oNFound);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if (oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    if (Path_comparePath(NodeFT_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}

int FT_insertDir(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    NodeFT_T oNFirstNew = NULL;
    NodeFT_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;
    void *pvContents = NULL;
    boolean bIsFile = FALSE;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    /* validate pcPath and generate a Path_T for it */
    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS)
        return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    iStatus = FT_traversePath(oPPath, &oNCurr);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
       pcPath isn't underneath root. */
    if (oNCurr == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if (oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeFT_getPath(oNCurr)) + 1;

        /* oNCurr is the node we're trying to insert */
        if (ulIndex == ulDepth + 1 && !Path_comparePath(oPPath,
                                                        NodeFT_getPath(
                                                                oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while (ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        NodeFT_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeFT_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* insert the new node for this level */
        iStatus = NodeFT_new(oNCurr, oPPrefix, pvContents, bIsFile,
                             &oNNewNode);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL)
                (void) NodeFT_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if (oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if (oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

boolean FT_containsDir(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    return (boolean) (iStatus == SUCCESS) &&
           (NodeFT_isFile(oNFound) == FALSE);
}

int FT_rmDir(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    ulCount -= NodeFT_free(oNFound);
    if (ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength) {
    int iStatus;
    Path_T oPPath = NULL;
    NodeFT_T oNFirstNew = NULL;
    NodeFT_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    /* validate pcPath and generate a Path_T for it */
    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS)
        return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    iStatus = FT_traversePath(oPPath, &oNCurr);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, and File cannot be root */
    if (oNCurr == NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    ulIndex = Path_getDepth(NodeFT_getPath(oNCurr)) + 1;

    /* oNCurr is the node we're trying to insert */
    if (ulIndex == ulDepth + 1 && !Path_comparePath(oPPath,
                                                    NodeFT_getPath(
                                                            oNCurr))) {
        Path_free(oPPath);
        return ALREADY_IN_TREE;
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while (ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        NodeFT_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeFT_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* insert the new node for this level */
        iStatus = NodeFT_new(oNCurr, oPPrefix, pvContents,
                             ulIndex == ulDepth,
                             &oNNewNode);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL)
                (void) NodeFT_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if (oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if (oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

boolean FT_containsFile(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);

    return (boolean) (iStatus == SUCCESS) &&
           (NodeFT_isFile(oNFound) == TRUE);
}

int FT_rmFile(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    ulCount -= NodeFT_free(oNFound);
    if (ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;
    void **ppvContents = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    iStatus = NodeFT_getContents(oNFound, ppvContents);

    return *ppvContents;
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {

    int iStatus;
    NodeFT_T oNFound = NULL;
    void **ppvContents = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    iStatus = NodeFT_setContents(oNFound, pvNewContents, ppvContents);

    return *ppvContents;
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    int iStatus;
    NodeFT_T oNFound;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS) {
        return iStatus;
    }

    if (NodeFT_isFile(oNFound)) {
        *pbIsFile = TRUE;
        /* *pulSize = NodeFT_getFileSize(oNFound) */
    } else {
        *pbIsFile = FALSE;
    }
    return SUCCESS;
}

int FT_init(void) {
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if (bIsInitialized)
        return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

int FT_destroy(void) {
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    if (oNRoot) {
        ulCount -= NodeFT_free(oNRoot);
        oNRoot = NULL;
    }

    bIsInitialized = FALSE;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

static size_t FT_preOrderTraversal(NodeFT_T n, DynArray_T d, size_t i) {
    size_t c;

    assert(d != NULL);

    if (n != NULL) {
        (void) DynArray_set(d, i, n);
        i++;
        if (NodeFT_isFile(n) == FALSE) {
            for (c = 0; c < NodeFT_getNumChildren(n, TRUE); c++) {
                int iStatus;
                NodeFT_T oNChild = NULL;
                iStatus = NodeFT_getChild(n, c, TRUE, &oNChild);
                assert(iStatus == SUCCESS);
                i = FT_preOrderTraversal(oNChild, d, i);
            }
            for (c = 0; c < NodeFT_getNumChildren(n, FALSE); c++) {
                int iStatus;
                NodeFT_T oNChild = NULL;
                iStatus = NodeFT_getChild(n, c, FALSE, &oNChild);
                assert(iStatus == SUCCESS);
                i = FT_preOrderTraversal(oNChild, d, i);
            }
        }
    }
    return i;
}

static void FT_strlenAccumulate(NodeFT_T oNNode, size_t *pulAcc) {
    assert(pulAcc != NULL);

    if (oNNode != NULL)
        *pulAcc += (Path_getStrLength(NodeFT_getPath(oNNode)) + 1);
}

static void FT_strcatAccumulate(NodeFT_T oNNode, char *pcAcc) {
    assert(pcAcc != NULL);

    if (oNNode != NULL) {
        strcat(pcAcc, Path_getPathname(NodeFT_getPath(oNNode)));
        strcat(pcAcc, "\n");
    }
}

char *FT_toString(void) {
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;

    if (!bIsInitialized)
        return NULL;

    nodes = DynArray_new(ulCount);
    (void) FT_preOrderTraversal(oNRoot, nodes, 0);

    DynArray_map(nodes, (void (*)(void *, void *)) FT_strlenAccumulate,
                 (void *) &totalStrlen);

    result = malloc(totalStrlen);
    if (result == NULL) {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    DynArray_map(nodes, (void (*)(void *, void *)) FT_strcatAccumulate,
                 (void *) result);

    DynArray_free(nodes);

    return result;
}
