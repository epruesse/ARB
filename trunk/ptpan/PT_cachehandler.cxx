#include <stdio.h>
// #include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"

/* /// "AllocCacheHandler()" */
struct CacheHandler * AllocCacheHandler(void)
{
  struct CacheHandler *ch;
  ch = (struct CacheHandler *) calloc(sizeof(struct CacheHandler), 1);
  if(!ch)
  {
    return(NULL); /* out of memory */
  }
  /* freshly initialize a cache handler */
  NewList(&ch->ch_UsedNodes);
  NewList(&ch->ch_FreeNodes);
  return(ch);
}
/* \\\ */

/* /// "FreeCacheHandler()" */
void FreeCacheHandler(struct CacheHandler *ch)
{
  struct CacheNode *cn;

  /* NOTE: Does NOT flush the cache! */
  /* free memory in used node cache */
  cn = (struct CacheNode *) ch->ch_UsedNodes.lh_Head;
  while(cn->cn_Node.ln_Succ)
  {
    Remove(&cn->cn_Node);
    freeset(cn, (struct CacheNode *) ch->ch_UsedNodes.lh_Head);
  }
  /* free memory in free node cache */
  cn = (struct CacheNode *) ch->ch_FreeNodes.lh_Head;
  while(cn->cn_Node.ln_Succ)
  {
    Remove(&cn->cn_Node);
    freeset(cn, (struct CacheNode *) ch->ch_FreeNodes.lh_Head);
  }
  /* free structure itself */
  free(ch);
}
/* \\\ */

/* /// "CacheLoadData()" */
struct CacheNode * CacheLoadData(struct CacheHandler *ch,
 struct CacheNode *cn, APTR ud)
{
  struct CacheNode *firstcn;
  struct CacheNode *secondcn;
  ULONG memused;

  if(ch->ch_CacheDisabled && ch->ch_LastNode)
  {
    /* unload last node, if cache was disabled */
    CacheUnloadData(ch, ch->ch_LastNode);
    ch->ch_LastNode = NULL;
  }

  /* allocate CacheNode, if not allocated already */
  if(!cn)
  {
    cn = (struct CacheNode *) calloc(sizeof(struct CacheNode), 1);
    if(!cn)
    {
      return(NULL);
    }
  } else {
    Remove(&cn->cn_Node); /* unlink first */
  }

  /* only fill in user data, if given, preventing overwrite */
  if(ud)
  {
    cn->cn_UserData = ud;
  }
  cn->cn_Node.ln_Pri++; /* how often was this thing used? */
  cn->cn_LastUseID = ++ch->ch_AccessID; /* set last access */

  if(cn->cn_Loaded) /* check if already loaded */
  {
    AddTail(&ch->ch_UsedNodes, &cn->cn_Node); /* move to the end of the list */
    return(cn); /* no need to load, we're done */
  }

  /* do we need to unload some bits? */
  memused = (*ch->ch_SizeFunc)(ch, cn->cn_UserData);
  while((ch->ch_MemUsage + memused > ch->ch_MaxCapacity) &&
    (ch->ch_UsedNodes.lh_Head->ln_Succ) &&
    (!ch->ch_CacheDisabled))
  {
    /* capacity exhausted, must unload */
    ULONG remmemused;
    firstcn = (struct CacheNode *) ch->ch_UsedNodes.lh_Head;
    secondcn = (struct CacheNode *) firstcn->cn_Node.ln_Succ;
    if(secondcn->cn_Node.ln_Succ) /* there are at least two nodes, check which one to kill */
    {
      if(firstcn->cn_Node.ln_Pri > secondcn->cn_Node.ln_Pri)
      {
    //printf("Second");
    firstcn = secondcn; /* the second one loses */
      } else {
    //printf("First");
      }
    } else {
      //printf("Only");
    }
    Remove(&firstcn->cn_Node); /* unlink anyway */

    /* get size of data */
    remmemused = (*ch->ch_SizeFunc)(ch, firstcn->cn_UserData);
    if((*ch->ch_UnloadFunc)(ch, firstcn->cn_UserData))
    {
      /* data unloaded successfully */
      //printf(" unloaded!\n");
      ch->ch_SwapCount++;
      ch->ch_MemUsage -= remmemused;
      /* move the node to the unused list */
      AddTail(&ch->ch_FreeNodes, &firstcn->cn_Node);
      firstcn->cn_Loaded = FALSE;
    } else {
      /* unload failed, move node to the end of the queue */
      //printf(" unload failed!\n");
      AddTail(&ch->ch_UsedNodes, &firstcn->cn_Node);
      /* avoid infinite loop */
      if(!secondcn)
      {
    //printf("Bailing out!\n");
    break;
      }
    }
  }

  /* now load the data */
  if((*ch->ch_LoadFunc)(ch, cn->cn_UserData))
  {
    /* loading successful */
    ch->ch_MemUsage += memused;
    //printf("Successfully loaded (%ld/%ld)\n", memused, ch->ch_MemUsage);
    cn->cn_Loaded = TRUE;
    AddTail(&ch->ch_UsedNodes, &cn->cn_Node);
    if(ch->ch_CacheDisabled)
    {
      ch->ch_LastNode = cn;
    }
    return(cn); /* no need to load, we're done */
  } else {
    /* loading failed */
    //printf("Loading failed (%ld)\n", memused);
    AddTail(&ch->ch_FreeNodes, &cn->cn_Node);
    return(cn);
  }
}
/* \\\ */

/* /// "CacheMemUsage()" */
ULONG CacheMemUsage(struct CacheHandler *ch)
{
  return(ch->ch_MemUsage);
}
/* \\\ */

/* /// "DisableCache()" */
void DisableCache(struct CacheHandler *ch)
{
  ch->ch_CacheDisabled = TRUE;
}
/* \\\ */

/* /// "DisableCache()" */
void EnableCache(struct CacheHandler *ch)
{
  ch->ch_CacheDisabled = FALSE;
}
/* \\\ */

/* /// "CacheDataLoaded()" */
BOOL CacheDataLoaded(struct CacheNode *cn)
{
  /* if cn == NULL, it's not loaded for sure */
  if(!cn)
  {
    return(FALSE);
  }
  return(cn->cn_Loaded);
}
/* \\\ */

/* /// "FreeCacheNode()" */
void FreeCacheNode(struct CacheHandler *ch, struct CacheNode *cn)
{
  if(!cn)
  {
    return; /* ignore null pointers */
  }
  if(cn->cn_Loaded) /* unload data first */
  {
    CacheUnloadData(ch, cn);
  }
  /* remove from list */
  Remove(&cn->cn_Node);
  free(cn);
}
/* \\\ */

/* /// "CacheUnloadData()" */
BOOL CacheUnloadData(struct CacheHandler *ch, struct CacheNode *cn)
{
  ULONG memused;

  if(!cn)
  {
    return(FALSE); /* null pointer */
  }
  if(!cn->cn_Loaded)
  {
    return(FALSE); /* not loaded */
  }

  /* try to unload the data */
  memused = (*ch->ch_SizeFunc)(ch, cn->cn_UserData);
  if((*ch->ch_UnloadFunc)(ch, cn->cn_UserData))
  {
    //printf("Unloaded %ld\n", memused);
    ch->ch_MemUsage -= memused; /* we actually unloaded the data */
    /* move the node to the unused list */
    Remove(&cn->cn_Node);
    AddTail(&ch->ch_FreeNodes, &cn->cn_Node);
    cn->cn_Loaded = FALSE;
  }
  return(TRUE);
}
/* \\\ */

/* /// "FlushCache()" */
ULONG FlushCache(struct CacheHandler *ch)
{
  struct CacheNode *cn;
  struct CacheNode *nextcn;
  ULONG oldmemused = ch->ch_MemUsage;

  /* traverse nodes and call UnloadFunc for each of them */
  cn = (struct CacheNode *) ch->ch_UsedNodes.lh_Head;
  while(cn->cn_Node.ln_Succ)
  {
    nextcn = (struct CacheNode *) cn->cn_Node.ln_Succ;
    CacheUnloadData(ch, cn);
    cn = nextcn;
  }
  return(oldmemused - ch->ch_MemUsage);
}
/* \\\ */
