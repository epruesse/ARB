/************************************************************************
 Definition for call back hooks
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 06.05.03
 ************************************************************************/

#ifndef HOOKS_H
#define HOOKS_H

#include "dlist.h"

/* standard callback hook */
struct Hook
{
    struct Node h_Node; /* node for linkage */
    APTR (*h_Func)(struct Hook *); /* calling function */
    APTR        h_UserData;
};

/* prototypes */

APTR CallHook(struct Hook *hook);

#endif /* HOOKS_H */

