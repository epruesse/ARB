#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "ptpan.h"
//#include "probe_tree.hxx"
#include "pt_prototypes.h"

//  ------------------------------------------------------
//      extern "C" int PT_find_exProb(PT_exProb *pep)
//  ------------------------------------------------------

/* called by AISC */

/* /// "PT_find_exProb()" */
extern "C" int PT_find_exProb(PT_exProb *pep)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  struct PTPanPartition *pp;
  struct TreeNode *tn;
  struct TreeNode *parenttn;
  BOOL done;
  BOOL first;
  ULONG cnt;
  UWORD seqcode;
  STRPTR outptr;
  ULONG len;

  printf("EXTERN: PT_find_exProb\n");

  /* free old result */
  freenull(pep->result);

  /* do we need to start from scratch? */
  if(pep->restart)
  {
    /* reset everything */
    if((tn = (struct TreeNode *) pep->next_probe.data))
    {
      /* free old tree nodes */
      do
      {
        parenttn = tn->tn_Parent;
        free(tn);
      } while((tn = parenttn));
    }
    /* get first partition */
    pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
    if(!(pp->pp_CacheNode = CacheLoadData(pg->pg_PartitionCache, pp->pp_CacheNode, pp)))
    {
      printf("That's it, I'm outta here!\n");
      return(-1); /* something went wrong while loading */
    }
    /* get root node */
    tn = ReadPackedNode(pp, 0);
    pep->restart = 0;
  } else {
    tn = (struct TreeNode *) pep->next_probe.data;
  }
  if(!tn)
  {
    pep->result = strdup(""); /* empty string */
    return(0);
  }
  pp = tn->tn_PTPanPartition;
  outptr = (STRPTR) malloc(pep->numget * (pep->plength + 1) + 1);
  pep->result = outptr;

  /* collect all the strings that are on our way */
  done = FALSE;
  first = TRUE;
  seqcode = 0;
  len = (pep->plength < pp->pp_TreePruneLength) ? pep->plength : pp->pp_TreePruneLength;
  for(cnt = 0; cnt < (ULONG) pep->numget; cnt++)
  {
    //printf("Cnt: %ld\n", cnt);
    /* go down! */
    while(tn->tn_TreeOffset < len)
    {
      while(seqcode < pg->pg_AlphaSize)
      {
        //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
        if(tn->tn_Children[seqcode])
        {
          /* there is a child, go down */
          tn = GoDownNodeChild(tn, seqcode);
          //printf("Down %d %08lx\n", seqcode, tn);
          seqcode = 0;
          break;
        }
        seqcode++;
      }

      while(seqcode == pg->pg_AlphaSize) /* we didn't find any children */
      {
        /* go up again */
        //printf("Up\n");
        parenttn = tn->tn_Parent;
        seqcode = tn->tn_ParentSeq + 1;
        freeset(tn, parenttn);
        if(!tn)
        {
        /* we're done with this partition */
          pep->next_probe.data = NULL;
          if(pp->pp_Node.ln_Succ->ln_Succ)
         {
           /* load next partition */
            pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
            if(!(pp->pp_CacheNode = CacheLoadData(pg->pg_PartitionCache, pp->pp_CacheNode, pp)))
            {
              printf("That's it, I'm outta here!\n");
              return(-1); /* something went wrong while loading */
            }
            /* get root node */
            tn = ReadPackedNode(pp, 0);
         } else {
           done = TRUE;
           break; /* we're done! */
        }
      }
      }
      if(done)
      {
       break;
      }
    }

    if(done)
    {
      break;
    }

    if(!first)
    {
      *outptr++ = ';';
    } else {
      first = FALSE;
    }
    GetTreePath(tn, outptr, pep->plength);
    outptr += pep->plength;

    parenttn = tn->tn_Parent;
    seqcode = tn->tn_ParentSeq + 1;
    freeset(tn, parenttn);
  }
  *outptr = 0;
  pep->next_probe.data = (STRPTR) tn;
  return(0);
}
/* \\\ */
