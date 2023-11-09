/*--------------------------------------------------------------------*/
/* nodeFT.c                                                               */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#ifndef FT_INCLUDED
#define FT_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"

typedef struct NodeFT *NodeFT_T;

int NodeFT_new(NodeFT_T oNParent, Path_T oPPath, void* pvContents,
                   size_t ulLength, boolean bIsFile,
                   NodeFT_T *poNResult);

size_t NodeFT_free(NodeFT_T oNNode);

boolean NodeFT_hasFile(NodeFT_T oNParent, Path_T oPPath, size_t *pulChildId);

boolean NodeFT_hasDir(NodeFT_T oNParent, Path_T oPPath, size_t *pulChildId);

Path_T NodeFT_getPath(NodeFT_T oNNode);

size_t NodeFT_getNumChildren(NodeFT_T oNParent, boolean bIsFile);

int NodeFT_getChild(NodeFT_T oNParent, size_t ulChildId,
                    boolean bIsFile, NodeFT_T *poNResult);

NodeFT_T NodeFT_getParent(NodeFT_T oNNode);

int NodeFT_getContents(NodeFT_T oNNode, void **ppvContents);

int NodeFT_setContents(NodeFT_T oNNode, void *pvContents, size_t ulNewLength, void **ppvPrevContents);

size_t NodeFT_getFileSize(NodeFT_T oNNode);

boolean NodeFT_isFile(NodeFT_T oNNode);

int NodeFT_compare(NodeFT_T oNFirst, NodeFT_T oNSecond);

char *NodeFT_toString(NodeFT_T oNNode);

#endif