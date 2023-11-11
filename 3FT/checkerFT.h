/*--------------------------------------------------------------------*/
/* checkerFT.h                                                        */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#ifndef CHECKER_INCLUDED
#define CHECKER_INCLUDED

#include "nodeFT.h"

/*
   Returns TRUE if oNNode represents a directory/file entry
   in a valid state, or FALSE otherwise. Prints explanation
   to stderr in the latter case.
*/
boolean CheckerFT_Node_isValid(NodeFT_T oNNode);

/*
   Returns TRUE if the hierarchy is in a valid state or FALSE
   otherwise.  Prints explanation to stderr in the latter case.
   The data structure's validity is based on a boolean
   bIsInitialized indicating whether the DT is in an initialized
   state, a NodeFT_T oNRoot representing the root of the hierarchy, and
   a size_t ulCount representing the total number of directories/files
   in the hierarchy.
*/
boolean CheckerFT_isValid(boolean bIsInitialized,
                          NodeFT_T oNRoot,
                          size_t ulCount);

#endif
