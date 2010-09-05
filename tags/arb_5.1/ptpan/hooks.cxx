/************************************************************************
 Definition for call back hooks
 Written by Chris Hodges <hodges@in.tum.de>.
 Last change: 06.05.03
 ************************************************************************/

#include "hooks.h"

APTR CallHook(struct Hook *hook)
{
    return((*(hook->h_Func))(hook));
}
