/*--------------------------------------------------------------------*/
/* nodeFT.h                                                               */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#ifndef NODE_INCLUDED
#define NODE_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A NodeFT_T is a node in a File Tree */
typedef struct NodeFT *NodeFT_T;

/*
   Creates a new node with path oPPath and parent oNParent. If bIsFile
   is TRUE, then the file characteristics - pvContents and ulLength -
   are stored in the node.

   Returns status SUCCESS and sets *poNResult to new node if successful,
   OR
   Sets *poNResult to NULL and returns status:
   * MEMORY_ERROR if memory could not be allocated to complete request
   * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
   * NO_SUCH_PATH if oPPath is of depth 0
                  or oNParent's path is not oPPath's direct parent
                  or oNParent is NULL but oPPath is not of depth 1
   * ALREADY_IN_TREE if oNParent already has a child with this path

   Precondition:
   * oNParent cannot be NULL
   * oPPath cannot be NULL
*/
int NodeFT_new(NodeFT_T oNParent, Path_T oPPath, void *pvContents,
               size_t ulLength, boolean bIsFile,
               NodeFT_T *poNResult);

/*
   Destroys and frees all memory allocated for oNNode and the nodes
   within the subtree with root oNNode.

   Returns the number of nodes "deleted".

   Precondition:
   * oNNode cannot be NULL
*/
size_t NodeFT_free(NodeFT_T oNNode);

/*
   Checks whether oNParent has a child that is a FILE with path oPPath.

   Returns:
   * TRUE if the child exists and stores the child's identifier in
          pulChildID (as used in Node_getChild).
   * FALSE if it does not exist and stores the child's _would be_
           identifier in pulChildID if it is inserted.

   Precondition:
   * oNParent cannot be NULL
   * oPPath cannot be NULL
   * pulChildId cannot be NULL
*/
boolean NodeFT_hasFile(NodeFT_T oNParent, Path_T oPPath,
                       size_t *pulChildId);

/*
   Checks whether oNParent has a child that is a DIRECTORY with path
   oPPath.

   Returns:
   * TRUE if the child exists and stores the child's identifier in
          pulChildID (as used in Node_getChild).
   * FALSE if it does not exist and stores the child's _would be_
           identifier in pulChildID if it is inserted.

   Precondition:
   * oNParent cannot be NULL
   * oPPath cannot be NULL
   * pulChildId cannot be NULL
*/
boolean NodeFT_hasDir(NodeFT_T oNParent, Path_T oPPath,
                      size_t *pulChildId);


/*
   Returns the path object representing oNNode's absolute path.

   Precondition:
   * oNNode cannot be NULL
*/
Path_T NodeFT_getPath(NodeFT_T oNNode);

/*
   Retrieves the count of either files or directories under a oNParent
   based on the value of bIsFile.

   Returns:
   * Number of children that are FILES under oNParent has when bIsFile
     is TRUE
   * Number of children that are DIRECTORIES under oNParent has when
     bIsFile is TRUE

   Precondition:
   * oNParent cannot be NULL
   * oNParent is a directory node
*/
size_t NodeFT_getNumChildren(NodeFT_T oNParent, boolean bIsFile);

/*
   Retrieves the child under oNParent with identifier ulChildId based on
   the value of bIsFile. If bIsFile is TRUE, retrieves a child that is
   a FILE. If bIsFile is FALSE, retrieves a child that is a DIRECTORY.

   Returns:
   * SUCCESS status and sets *poNResult to be the retrieved child node
   * NO_SUCH_PATH and sets *poNResult to NULL if ulChildID is not a
                  valid identifier for a child

   Precondition:
   * oNParent cannot be NULL
   * oNParent is a directory node
*/
int NodeFT_getChild(NodeFT_T oNParent, size_t ulChildId,
                    boolean bIsFile, NodeFT_T *poNResult);

/*
   Retrieves the parent node of oNNode.

   Returns:
   * The parent node of oNNode if it exists
   * NULL if oNNode is the root and has no parent

   Precondition:
   * oNNode cannot be NULL
 */
NodeFT_T NodeFT_getParent(NodeFT_T oNNode);

/*
   Returns the file contents of FILE node oNNode.

   Precondition:
   * oNNode cannot be NULL
   * oNNode is a file node
*/
void *NodeFT_getContents(NodeFT_T oNNode);

/*
   Sets the contents of a file node oNNode with new data pvContents and
   updates the length of the contents with the value ulNewLength.

   Returns the previous contents of oNNode

   Precondition:
   * oNNode cannot be NULL
   * oNNode is a file node
*/
void *NodeFT_setContents(NodeFT_T oNNode, void *pvContents,
                       size_t ulNewLength);

/*
   Returns the file size of FILE node oNNode

   Precondition:
   * oNNode cannot be NULL
   * oNNode is a file node
*/
size_t NodeFT_getFileSize(NodeFT_T oNNode);

/*
   Returns:
   * TRUE if oNNode is a FILE node
   * FALSE if oNNode is a DIRECTORY node

   Precondition:
   * oNNode cannot be NULL
*/
boolean NodeFT_isFile(NodeFT_T oNNode);

/*
   Compares the two nodes oNFirst and oNSecond.

   Returns:
   * <0 if oNFirst is "less than" oNSecond
   * 0 if oNFirst is "equal to" oNSecond
   * >0 if oNFirst is "greater than" oNSecond

   Precondition:
   * oNFirst cannot be NULL
   * pcSecond cannot be NULL
*/
int NodeFT_compare(NodeFT_T oNFirst, NodeFT_T oNSecond);

/*
   Returns:
   * A string representation for oNNode
   * NULL if there is an allocation error.

   Allocates memory for the returned string, which is then OWNED BY
   THE CALLER!
*/
char *NodeFT_toString(NodeFT_T oNNode);

#endif