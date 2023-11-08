/*--------------------------------------------------------------------*/
/* checkerFT.h                                                        */
/* Author: Praneeth Bhandaru                                          */
/*--------------------------------------------------------------------*/

#ifndef CHECKER_INCLUDED
#define CHECKER_INCLUDED

#include "nodeFT.h"

boolean CheckerFT_Node_isValid(NodeFT_T oNNode);

boolean CheckerFT_isValid(boolean bIsInitialized,
                          NodeFT_T oNRoot,
                          size_t ulCount);

#endif
