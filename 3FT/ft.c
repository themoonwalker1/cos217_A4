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

/*--------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------*/

/** Helper Functions **/

/*
   The FT_traversePath and FT_findNode functions modularize the common
   functionality of going as far as possible down an FT towards a path
   and returning either the node of however far was reached or the
   node if the full path was reached, respectively.
*/

/*
   Traverses the FT starting at the root as far as possible towards
   absolute path oPPath. If able to traverse, returns an int SUCCESS
   status and sets *poNFurthest to the furthest node reached (which may
   be only a prefix of oPPath, or even NULL if the root is NULL).

   Otherwise, sets *poNFurthest to NULL and returns with status:
   * CONFLICTING_PATH if the root's path is not a prefix of oPPath
   * MEMORY_ERROR if memory could not be allocated to complete request

   Precondition:
   * oPPath cannot be NULL
   * poNFurthest cannot be NULL
*/
static int FT_traversePath(Path_T oPPath, NodeFT_T *poNFurthest) {
    int iStatus;
    Path_T oPPrefix = NULL;
    NodeFT_T oNCurr;
    NodeFT_T oNChild = NULL;
    size_t ulDepth;
    size_t i;
    size_t ulChildID = 0;

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
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if (iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }

        if (NodeFT_hasFile(oNCurr, oPPrefix, &ulChildID) == TRUE) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = NodeFT_getChild(oNCurr, ulChildID, TRUE,
                                      &oNChild);
            if (iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
            break;
        } else if (NodeFT_hasDir(oNCurr, oPPrefix, &ulChildID) ==
                   TRUE) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = NodeFT_getChild(oNCurr, ulChildID, FALSE,
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

/*
   Traverses the FT to find a node with absolute path pcPath. Returns an
   int SUCCESS status and sets *poNResult to be the node, if found.

   Otherwise, sets *poNResult to NULL and returns with status:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root's path is not a prefix of pcPath
   * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
   * MEMORY_ERROR if memory could not be allocated to complete request

   Precondition:
   * pcPath cannot be NULL
   * poNResult cannot be NULL
*/
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

    /* find the closest ancestor */
    iStatus = FT_traversePath(oPPath, &oNFound);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    /* no ancestor */
    if (oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    /* "closest" ancestor is not the node itself */
    if (Path_comparePath(NodeFT_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}

/*--------------------------------------------------------------------*/

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

    /* cannot add something to a file */
    if (oNCurr != NULL && NodeFT_isFile(oNCurr) == TRUE) {
        Path_free(oPPath);
        return NOT_A_DIRECTORY;
    }

    ulDepth = Path_getDepth(oPPath);
    if (oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeFT_getPath(oNCurr)) + 1;

        /* oNCurr is the node we're trying to insert */
        if (ulIndex == ulDepth + 1
            && !Path_comparePath(oPPath, NodeFT_getPath(oNCurr))) {
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
        iStatus = NodeFT_new(oNCurr, oPPrefix, pvContents, 0, bIsFile,
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
    return (boolean) (iStatus == SUCCESS
                      && NodeFT_isFile(oNFound) == FALSE);
}

int FT_rmDir(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    if (NodeFT_isFile(oNFound) == TRUE)
        return NOT_A_DIRECTORY;

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

    /* cannot add something to a file */
    if (oNCurr != NULL && NodeFT_isFile(oNCurr)) {
        Path_free(oPPath);
        return NOT_A_DIRECTORY;
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
                             ulLength,
                             (boolean)(ulIndex == ulDepth),
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

    return (boolean) (iStatus == SUCCESS
                      && NodeFT_isFile(oNFound) == TRUE);
}

int FT_rmFile(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    if (NodeFT_isFile(oNFound) == FALSE)
        return NOT_A_FILE;

    ulCount -= NodeFT_free(oNFound);
    if (ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    NodeFT_T oNFound = NULL;
    void *pvContents = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS) {
        return NULL;
    }

    pvContents = NodeFT_getContents(oNFound);

    return pvContents;
}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {

    int iStatus;
    NodeFT_T oNFound = NULL;
    void *pvContents = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS) {
        return NULL;
    }

    pvContents = NodeFT_setContents(oNFound, pvNewContents,
                                    ulNewLength);

    return pvContents;
}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    int iStatus;
    NodeFT_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS) {
        return iStatus;
    }

    if (NodeFT_isFile(oNFound)) {
        *pbIsFile = TRUE;
        *pulSize = NodeFT_getFileSize(oNFound);
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

/*--------------------------------------------------------------------*/

/** toString Functions **/

/*
  Performs a pre-order traversal of the tree rooted at oNParent,
  inserting each payload to DynArray_T oDNodes beginning at index
  ulIndex.

  Returns the next unused index in oDNodes after the insertion(s).

  Precondition:
  * oDNodes cannot be NULL
*/

static size_t FT_preOrderTraversal(NodeFT_T oNParent,
                                   DynArray_T oDNodes, size_t ulIndex) {
    size_t c;
    int iStatus;

    assert(oDNodes != NULL);

    if (oNParent == NULL) {
        return ulIndex;
    }

    (void) DynArray_set(oDNodes, ulIndex, oNParent);
    ulIndex++;

    if (NodeFT_isFile(oNParent) == TRUE) {
        return ulIndex;
    }

    /* collect children that are FILES before DIRECTORIES */
    for (c = 0; c < NodeFT_getNumChildren(oNParent, TRUE); c++) {
        NodeFT_T oNChild = NULL;
        iStatus = NodeFT_getChild(oNParent, c, TRUE, &oNChild);
        assert(iStatus == SUCCESS);
        ulIndex = FT_preOrderTraversal(oNChild, oDNodes, ulIndex);
    }
    for (c = 0; c < NodeFT_getNumChildren(oNParent, FALSE); c++) {
        NodeFT_T oNChild = NULL;
        iStatus = NodeFT_getChild(oNParent, c, FALSE, &oNChild);
        assert(iStatus == SUCCESS);
        ulIndex = FT_preOrderTraversal(oNChild, oDNodes, ulIndex);
    }

    return ulIndex;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.

  Precondition:
  * pulAcc cannot be NULL
*/
static void FT_strlenAccumulate(NodeFT_T oNNode, size_t *pulAcc) {
    assert(pulAcc != NULL);

    if (oNNode != NULL)
        *pulAcc += (Path_getStrLength(NodeFT_getPath(oNNode)) + 1);
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.

  Precondition:
  * pulAcc cannot be NULL
*/
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
