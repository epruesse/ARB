#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

#include <PT_server.h>
#include <PT_server_prototypes.h>
#include <struct_man.h>
#include "probe.h"
#include <arbdbt.h>
#include "pt_prototypes.h"

extern "C" int pt_init_bond_matrix(PT_pdc *THIS)
{
    THIS->bond[0].val = 0.0;
    THIS->bond[1].val = 0.0;
    THIS->bond[2].val = 0.5;
    THIS->bond[3].val = 1.1;
    THIS->bond[4].val = 0.0;
    THIS->bond[5].val = 0.0;
    THIS->bond[6].val = 1.5;
    THIS->bond[7].val = 0.0;
    THIS->bond[8].val = 0.5;
    THIS->bond[9].val = 1.5;
    THIS->bond[10].val = 0.4;
    THIS->bond[11].val = 0.9;
    THIS->bond[12].val = 1.1;
    THIS->bond[13].val = 0.0;
    THIS->bond[14].val = 0.9;
    THIS->bond[15].val = 0.0;
    return 0;
}

extern "C" char *get_design_info(PT_tprobes  *tprobe)
{
    struct PTPanGlobal *pg = PTPanGlobalPtr;
    STRPTR buffer  = (STRPTR) GB_give_buffer(2000);
    UWORD posgroup = 0;
    char possep    = '=';
    LONG alignpos  = tprobe->apos;
    LONG cnt;
    ULONG sum;

    PT_pdc *pdc   = (PT_pdc *) tprobe->mh.parent->parent;
    STRPTR outptr = buffer;
    STRPTR srcptr;

    printf("EXTERN: get_design_info\n");

    /* find variable that is closest to the hit */
    for (posgroup=0; posgroup < 26; ++posgroup)
    {
        LONG dist;
        /* see, if this group has been defined yet */
        if (! (pdc->pos_groups[posgroup]))
        {
            pdc->pos_groups[posgroup] = tprobe->apos;
            break;
        }
        dist = tprobe->apos - pdc->pos_groups[posgroup];
        if ((dist >= 0) && (dist < pdc->probelen))
        {
            alignpos = dist;
            possep = '+';
            break;
        }
        if ((dist < 0) && (dist > -pdc->probelen))
        {
            alignpos = -dist;
            possep = '-';
            break;
        }
    }

    /* generate output */
    sprintf(buffer, "%s %2ld %c%c%4ld %4ld %4ld %4.1f  %5.1f  |",
            tprobe->sequence,
            (ULONG) tprobe->seq_len,
            'A'+posgroup, possep, alignpos,
            (ULONG) 0, // ecoli
            (ULONG) tprobe->groupsize,
            0.0, // gc content
            tprobe->temp);

    outptr += strlen(buffer);
    sum = 0;
    for (cnt = 0; cnt < PERC_SIZE; ++cnt)
    {
        sum += tprobe->perc[cnt];
        sprintf(outptr, "%2ld;", sum);
        outptr += strlen(outptr);
    }

    *outptr++ = ' ';
    srcptr = &tprobe->sequence[tprobe->seq_len];
    for (cnt = 0; cnt < tprobe->seq_len; cnt++)
    {
        *outptr++ = pg->pg_DecompressTable[pg->pg_ComplementTable[pg->pg_CompressTable[*--srcptr]]];
    }
    *outptr = 0;
    return buffer;
}

extern "C" char *get_design_hinfo(PT_tprobes  *tprobe)
{
    STRPTR buffer = (STRPTR) GB_give_buffer(2000);
    STRPTR outptr = buffer;
    LONG cnt;
    STRPTR srcptr;
    PT_pdc *pdc;

    printf("EXTERN: get_design_hinfo\n");

    if (! tprobe)
    {
        return ((STRPTR) "Sorry, there are no probes for your selection!!!");
    }
    pdc = (PT_pdc *) tprobe->mh.parent->parent;
    sprintf(buffer,
    "Probe design Parameters:\n"
    "Length of probe    %4i\n"
    "Temperature        [%4.1f -%4.1f ]\n"
    "GC-Content         [%4.1f%%-%4.1f%%]\n"
    "E.Coli Position    [%4i -%4i]\n"
    "Max Non Group Hits  %4i\n"
    "Min Group Hits      %4.0f%%\n",
    pdc->probelen,
    pdc->mintemp,pdc->maxtemp,
    pdc->min_gc*100.0, pdc->max_gc*100.0,
    pdc->minpos, pdc->maxpos,
    pdc->mishit, pdc->mintarget*100.0);
    outptr += strlen(buffer);
    srcptr = (STRPTR) "Target";
    for (cnt = 0; cnt < pdc->probelen + 1; cnt++)
    {
        if (*srcptr)
        {
            *outptr++ = *srcptr++;
        } else {
            *outptr++ = ' ';
        }
    }
    srcptr = (STRPTR) "le apos   ecol grps  G+C 4GC+2AT |";
    while (*srcptr)
    {
        *outptr++ = *srcptr++;
    }

    srcptr = (STRPTR) "Increase err by n*0.2 -> probe matches n non group species";
    for (cnt = 0; cnt < PERC_SIZE*3; cnt++)
    {
        if (*srcptr)
        {
            *outptr++ = *srcptr++;
        } else {
            *outptr++ = ' ';
        }
    }
    srcptr = (STRPTR) "Probe sequence";
    while (*srcptr)
    {
        *outptr++ = *srcptr++;
    }
    *outptr = 0;
    return buffer;
}

/* /// "MarkSpeciesGroup()" */
ULONG MarkSpeciesGroup(struct PTPanGlobal *pg, STRPTR specnames)
{
    struct PTPanSpecies *ps;
    STRPTR namestart = specnames;
    UBYTE namechr;
    ULONG specnum;
    ULONG markcount = 0;

    /* clear all species marks */
    ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
    while (ps->ps_Node.ln_Succ)
    {
        ps->ps_IsGroup = FALSE;
        ps->ps_SerialTouch = 0;
        ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
    }

    if (! *specnames)
    {
        return(0); /* string was empty! */
    }
    printf("Specnames %s\n", specnames);
    do
    {
        namechr = *specnames;
        /* if we encounter a hash or a nullbyte, we're at the end of a species string */
        if ((namechr == '#') || (!namechr))
        {
            if (namestart < specnames) /* don't try to find an empty string */
            {
                /* temporarily terminate the string */
                *specnames = 0;
                specnum = GBS_read_hash(pg->pg_SpeciesNameHash, namestart);
                if (specnum)
                {
                    pg->pg_SpeciesMap[specnum-1]->ps_IsGroup = TRUE;
                    markcount++;
                } else {
                    printf("Couldn't find %s to mark\n", namestart);
                }
                /* restore character */
                *specnames++ = namechr;
                namestart = specnames;
            } else {
                namestart = ++specnames;
            }
        } else {
            specnames++;
        }
    } while (namechr);
    printf("Markcount %ld\n", markcount);
    return markcount;
}
/* \\\ */

/* /// "AllocDesignQuery()" */
struct DesignQuery * AllocDesignQuery(struct PTPanGlobal *pg)
{
    struct DesignQuery *dq;

    dq = (struct DesignQuery *) calloc(sizeof(struct DesignQuery), 1);
    if (! dq)
    {
        return NULL;
    }
    dq->dq_Serial = 1;
    NewList(&dq->dq_Hits);
    dq->dq_PTPanGlobal = pg;
    return dq;
}
/* \\\ */

/* /// "FreeDesignQuery()" */
void FreeDesignQuery(struct DesignQuery *dq)
{
    struct DesignHit *dh;
    
    free(dq->dq_TempHit.dh_Matches);                        // free temp memory
    dh = (struct DesignHit *) dq->dq_Hits.lh_Head;          // free hits
    while (dh->dh_Node.ln_Succ)
    {
        RemDesignHit(dh);
        dh = (struct DesignHit *) dq->dq_Hits.lh_Head;
    }
    free(dq);                                               // free structure itself
}
/* \\\ */

/* /// "AddDesignHit()" */
struct DesignHit * AddDesignHit(struct DesignQuery *dq)
{
    struct PTPanGlobal *pg = dq->dq_PTPanGlobal;
    struct PTPanPartition *pp = dq->dq_PTPanPartition;
    struct PTPanSpecies *ps;
    struct DesignHit *dh = &dq->dq_TempHit;
    
    ULONG cnt;
    BOOL take = TRUE;
    BOOL done;
    UBYTE seqcode;
    struct HitTuple *ht;
    ULONG *leafptr;
    struct TreeNode *tn = dq->dq_TreeNode;
    struct TreeNode *parenttn;

    dh->dh_GroupHits    = 0;                    // init some more stuff
    dh->dh_NonGroupHits = 0;
    dh->dh_Hairpin      = 0;
    dh->dh_NumMatches   = 0;

    /* now iterate through the lower parts of the tree, collecting all leaf nodes */
    seqcode = SEQCODE_N;
    do
    {
        while (seqcode < pg->pg_AlphaSize)
        {
            //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
            if (tn->tn_Children[seqcode])
            {
                /* there is a child, go down */
                tn = GoDownNodeChildNoEdge(tn, seqcode);
                seqcode = SEQCODE_N;
                //printf("Down %d %08lx\n", seqcode, tn);
            }
            seqcode++;
        }

        while (seqcode == pg->pg_AlphaSize)         // we didn't find any children
        {
            /* when going up, collect any leafs */
            if (tn->tn_NumLeaves)
            {
                cnt = tn->tn_NumLeaves;
                /* check, if enough memory is left */
                if (dh->dh_NumMatches + cnt >= dq->dq_TempMemSize)
                {
                    dq->dq_TempMemSize = (dh->dh_NumMatches + cnt) << 1;
                    dh->dh_Matches = (struct HitTuple *) realloc(dh->dh_Matches,
                                     dq->dq_TempMemSize * sizeof(struct HitTuple));
                }
                ht = &dh->dh_Matches[dh->dh_NumMatches];
                dh->dh_NumMatches += cnt;
                /* enter leaves */
                leafptr = tn->tn_Leaves;
                do
                {
                    //printf("%ld: %ld\n", cnt, *leafptr);
                    ht->ht_AbsPos  = pp->pp_RawOffset + (ULLONG) *leafptr++;
                    ht->ht_Species = NULL;
                    ht++;
                } while (--cnt);
                /* if we got more matches than the sum of marked and nongroup hits,
                it's very probable that we have exceeded non group hits, so check */
                if (dh->dh_NumMatches > dq->dq_MarkedSpecies + dq->dq_MaxNonGroupHits)
                {
                    dh->dh_NonGroupHits = 0;
                    ht  = dh->dh_Matches;
                    cnt = dh->dh_NumMatches;
                    /* increase serial number so we can find unique matches */
                    dq->dq_Serial++;
                    do
                    {
                        if (! (ps = ht->ht_Species))
                        {
                            ps = ht->ht_Species = (struct PTPanSpecies *) FindBinTreeLowerKey(pg->pg_SpeciesBinTree,
                                                                                              ht->ht_AbsPos);
                        }
                        /* check, if hit is really within sequence */
                        if (ht->ht_AbsPos + dq->dq_ProbeLength < ps->ps_AbsOffset + ps->ps_RawDataSize)
                        {
                            /* check, if this species has been visited right now */
                            if (ps->ps_SerialTouch != dq->dq_Serial)
                            {
                                ps->ps_SerialTouch = dq->dq_Serial;
                                if (! ps->ps_IsGroup)
                                {
                                    /* check if we are over the limit */
                                    if (++dh->dh_NonGroupHits > dq->dq_MaxNonGroupHits)
                                    {
/*                                        printf("NonGroupHits exceeded! Species: %s abspos: %lu, relpos: %lu\n",
                                               ps->ps_Name, ht->ht_AbsPos, 
                                               ht->ht_AbsPos - ps->ps_AbsOffset);*/
                                        take = FALSE;
                                        done = TRUE;
                                        break;
                                    }
                                }
                            } else {
                                //printf("Duplicate hit in %s (%ld)\n", ht->ht_Species->ps_Name, ht->ht_AbsPos);
                            }
                        } else {
                            /*printf("Hit crosses boundary %ld > %ld \n",
                                ht->ht_AbsPos + dq->dq_ProbeLength,
                                ps->ps_AbsOffset + ps->ps_RawDataSize);*/
                        }
                        ht++;
                    } while (--cnt);
                }
            }
            if (tn == dq->dq_TreeNode)
            {
                /* we're back at the top level where we started -- stop collecting */
                done = TRUE;
                break;
            }
            /* go up again */
            //printf("Up\n");
            parenttn = tn->tn_Parent;
            seqcode = tn->tn_ParentSeq + 1;
            freeset(tn, parenttn);
        }
    } while (! done);

    /* if the number of hits is smaller than the minimum number
    of group hits, we don't need to verify, as the equation won't hold. */
    if (dh->dh_NumMatches < dq->dq_MinGroupHits)
    {
//        printf("NumMatches %ld < MinGroupHits %ld\n",
//            dh->dh_NumMatches, dq->dq_MinGroupHits);
        take = FALSE;
    }

    /* verify non group hits and group hits limitations */
    if (take)
    {
        dh->dh_NonGroupHits = 0;
        dh->dh_GroupHits    = 0;
        ht  = dh->dh_Matches;
        cnt = dh->dh_NumMatches;
        /* increase serial number so we can find unique matches */
        dq->dq_Serial++;
        do
        {
            if (! (ps = ht->ht_Species))
            {
                ps = ht->ht_Species = (struct PTPanSpecies *) FindBinTreeLowerKey(pg->pg_SpeciesBinTree,
                                                                                  ht->ht_AbsPos);
            }
            //printf("%ld: %s %ld\n", cnt, ht->ht_Species->ps_Name, ht->ht_AbsPos);
            /* check, if hit is really within sequence */
            if (ht->ht_AbsPos + dq->dq_ProbeLength < ps->ps_AbsOffset + ps->ps_RawDataSize)
            {
                /* check, if this species has been visited right now */
                if (ps->ps_SerialTouch != dq->dq_Serial)
                {
                    ht->ht_Species->ps_SerialTouch = dq->dq_Serial;
                    if (ht->ht_Species->ps_IsGroup)
                    {
                        dh->dh_GroupHits++;
                    } else {
                        /* check if we are over the limit */
                        if (++dh->dh_NonGroupHits > dq->dq_MaxNonGroupHits)
                        {
/*                            printf("NonGroupHits exceeded (2)! Species: %s abspos: %lu, relpos: %lu\n",
                                   ps->ps_Name, ht->ht_AbsPos, ht->ht_AbsPos - ps->ps_AbsOffset);*/
                            take = FALSE;
                            break;
                        }
                    }
                } else {
                    //printf("Duplicate hit (2) in %s (%ld)\n", ht->ht_Species->ps_Name, ht->ht_AbsPos);
                }
            } else {
                /*printf("Hit crosses boundary %ld > %ld \n",
                    ht->ht_AbsPos + dq->dq_ProbeLength,
                    ps->ps_AbsOffset + ps->ps_RawDataSize);*/
            }
            ht++;
        } while (--cnt);
        if (dh->dh_GroupHits < dq->dq_MinGroupHits)
        {
            /*printf("grouphits %ld < MinGroupHits %ld\n",
                dh->dh_GroupHits, dq->dq_MinGroupHits);*/
            take = FALSE;
        }
    }
    if (take)
    {
        struct DesignHit *olddh = dh;
        dh = (struct DesignHit *) calloc(1, sizeof(struct DesignHit));
        if (! dh)
        {
            return NULL;    // out of memory
        }

        //dh->dh_ProbeLength = dq->dq_ProbeLength;
        dh->dh_ProbeSeq = (STRPTR) malloc(dq->dq_ProbeLength + 1);
        GetTreePath(tn, dh->dh_ProbeSeq, dq->dq_ProbeLength);

        dh->dh_Temp      = 0.0;                                 // calculate temperature
        dh->dh_GCContent = 0;                                   // and GC content
        for (cnt = 0; cnt < dq->dq_ProbeLength; cnt++)
        {
            switch(pg->pg_CompressTable[dh->dh_ProbeSeq[cnt]])
            {
                case SEQCODE_C:
                case SEQCODE_G:
                                    dh->dh_Temp += 4.0;
                                    dh->dh_GCContent++;
                                    break;
                case SEQCODE_A:
                case SEQCODE_T:
                                    dh->dh_Temp += 2.0;
                                    break;

                default:
                                    take = FALSE;               // ignore this probe -- it contains N sequences!
                                    cnt = dq->dq_ProbeLength;   // abort
                                    break;
            }
        }
        if ((dh->dh_Temp < dq->dq_MinTemp) || (dh->dh_Temp > dq->dq_MaxTemp))
        {
            take = FALSE;           // temperature was out of given range
        }
        if((dh->dh_GCContent < dq->dq_MinGC) || (dh->dh_GCContent > dq->dq_MaxGC))
        {
            take = FALSE;           // gc content was out of given range
        }

        if(!(dh->dh_Matches = (struct HitTuple *) malloc(olddh->dh_NumMatches * sizeof(struct HitTuple))))
        {
            take = FALSE;           // out of memory
        }
        if (! take)                 // aport if not to be taken
        {
            free(dh->dh_ProbeSeq);
            free(dh);
            printf("Huh?\n");
            return(NULL);
        }

        dh->dh_GroupHits    = olddh->dh_GroupHits;          // copy information into hit
        dh->dh_NonGroupHits = olddh->dh_NonGroupHits;
        dh->dh_NumMatches   = olddh->dh_NumMatches;
        memcpy(dh->dh_Matches, olddh->dh_Matches,
        dh->dh_NumMatches * sizeof(struct HitTuple));
        printf("GOOD PROBE %s! (GroupHits=%ld, NonGroupHits=%ld)\n",
        dh->dh_ProbeSeq, dh->dh_GroupHits, dh->dh_NonGroupHits);
#if 0 /* debug */
        ht  = dh->dh_Matches;
        cnt = dh->dh_NumMatches;
        do
        {
            printf("%ld: %s %ld\n", cnt, ht->ht_Species->ps_Name, ht->ht_AbsPos);
            ht++;
        } while (--cnt);
#endif
        dq->dq_NumHits++;
        AddTail(&dq->dq_Hits, &dh->dh_Node);
        return dh;
    } else {
        return NULL;
    }
}
/* \\\ */

/* /// "RemDesignHit()" */
void RemDesignHit(struct DesignHit *dh)
{
    Remove(&dh->dh_Node);               // unlink and free node
    free(dh->dh_ProbeSeq);
    free(dh->dh_Matches);
    free(dh);
}
/* \\\ */

/* /// "CalcProbeQuality()" */
void CalcProbeQuality(struct DesignQuery *dq)
{
    struct SearchQuery *sq;
    struct DesignHit *dh;
    struct PTPanGlobal *pg = dq->dq_PTPanGlobal;
    struct PTPanPartition *pp;
    struct QueryHit *qh;

    printf("Calc Probe Quality\n");
    dh = (struct DesignHit *) dq->dq_Hits.lh_Head;
    while (dh->dh_Node.ln_Succ)
    {
        /* allocate query that configures and holds all the merged results */
        if (! (sq = AllocSearchQuery(pg)))
        {
            return;
        }
        dh->dh_SearchQuery = sq;
        /* prefs */
        sq->sq_Query        = dh->dh_ProbeSeq;
        sq->sq_QueryLen     = strlen(sq->sq_Query);
        sq->sq_MaxErrors    = (float) PERC_SIZE * PROBE_MISM_DEC;
        sq->sq_Reversed     = FALSE;
        sq->sq_AllowReplace = TRUE;
        sq->sq_AllowInsert  = FALSE;
        sq->sq_AllowDelete  = FALSE;
        sq->sq_KillNSeqsAt  = dq->dq_ProbeLength / 3;
        sq->sq_SortMode     = SORT_HITS_WEIGHTED;

        /* init */
        // TODO: add PP_convertBondMatrix ?
        PP_buildPosWeight(sq);
        sq->sq_MismatchWeights = &sq->sq_PTPanGlobal->pg_MismatchWeights;

        dh = (struct DesignHit *) dh->dh_Node.ln_Succ;
    }
    
    printf("Cached parts\n");
    /* search over partitions that are still in cache */
    pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
    while (pp->pp_Node.ln_Succ)
    {
        pp->pp_Done = FALSE;
        if (CacheDataLoaded(pp->pp_CacheNode))
        {
            dh = (struct DesignHit *) dq->dq_Hits.lh_Head;
            while (dh->dh_Node.ln_Succ)
            {
                /* search normal */
                SearchPartition(pp, dh->dh_SearchQuery);
                dh = (struct DesignHit *) dh->dh_Node.ln_Succ;
            }
        }
        pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
    }

    printf("Uncached parts\n");
    /* search over all partitions not done yet */
    pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
    while (pp->pp_Node.ln_Succ)
    {
        if (! pp->pp_Done)
        {
            dh = (struct DesignHit *) dq->dq_Hits.lh_Head;
            while (dh->dh_Node.ln_Succ)
            {
                /* search normal */
                SearchPartition(pp, dh->dh_SearchQuery);
                dh = (struct DesignHit *) dh->dh_Node.ln_Succ;
            }
        }
        pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
    }

    printf("Examine...\n");
    /* now examine the results */
    dh = (struct DesignHit *) dq->dq_Hits.lh_Head;
    while (dh->dh_Node.ln_Succ)
    {
        ULONG hitpos;
        ULONG sum;
        /* check all hits */
        qh = (struct QueryHit *) dh->dh_SearchQuery->sq_Hits.lh_Head;
        while (qh->qh_Node.ln_Succ)
        {
            /* is this a non group hit? */
            /*printf("Hit %ld (%f), Group %s\n",
            qh->qh_AbsPos, qh->qh_ErrorCount,
            qh->qh_Species->ps_IsGroup ? "yes" : "no");*/
            if ( !qh->qh_Species->ps_IsGroup)
            {
                hitpos = (ULONG) (qh->qh_ErrorCount * (1.0 / PROBE_MISM_DEC));
                if (hitpos < PERC_SIZE)
                {
                    dh->dh_NonGroupHitsPerc[hitpos]++;
                }
            }
            qh = (struct QueryHit *) qh->qh_Node.ln_Succ;
        }
        printf("Probe %s ", dh->dh_ProbeSeq);
        sum = 0;
        for (hitpos = 0; hitpos < PERC_SIZE; hitpos++)
        {
            sum += dh->dh_NonGroupHitsPerc[hitpos];
            printf("%ld ", sum);
        }
        printf("\n");

        {
            UWORD cnt;
            PT_tprobes *tprobe;
            /* fill legacy structure */
            tprobe            = create_PT_tprobes();
            tprobe->sequence  = strdup(dh->dh_ProbeSeq);
            tprobe->seq_len   = dq->dq_ProbeLength;
            tprobe->temp      = dh->dh_Temp;
            tprobe->groupsize = dh->dh_GroupHits;
            tprobe->mishit    = dh->dh_NonGroupHits;
            for (cnt = 0; cnt < PERC_SIZE; cnt++)
            {
                tprobe->perc[cnt] = dh->dh_NonGroupHitsPerc[cnt];
            }

            aisc_link((struct_dllpublic_ext *) &(dq->dq_PDC->ptprobes),
            (struct_dllheader_ext *) tprobe);
        }

        dh = (struct DesignHit *) dh->dh_Node.ln_Succ;
    }
}
/* \\\ */

/* /// "FindProbeInPartition()" */
BOOL FindProbeInPartition(struct DesignQuery *dq)
{
    struct PTPanGlobal *pg = dq->dq_PTPanGlobal;
    struct PTPanPartition *pp = dq->dq_PTPanPartition;
    struct TreeNode *tn;
    struct TreeNode *parenttn;
    BOOL done;
    ULONG cnt;
    UWORD seqcode;
    ULONG len;
    double currtemp = 0.0;
    UWORD currgc = 0;
    STRPTR edgeptr;
    BOOL abortpath;
    ULONG pathleft;
    BOOL nohits = TRUE;

    //char buf[100];

    if (! (pp->pp_CacheNode = CacheLoadData(pg->pg_PartitionCache, pp->pp_CacheNode, pp)))
    {
        printf("That's it, I'm outta here!\n");
        return FALSE;                       // something went wrong while loading
    }
    tn = ReadPackedNode(pp, 0);             // get root node

    /* collect all the strings that are on our way */
    done    = FALSE;
    seqcode = SEQCODE_A;
    len     = (dq->dq_ProbeLength < pp->pp_TreePruneLength) ? dq->dq_ProbeLength : pp->pp_TreePruneLength;
    do
    {
        //printf("Cnt: %ld\n", cnt);
        /* go down! */
        while (tn->tn_TreeOffset < len)
        {
            while (seqcode < pg->pg_AlphaSize)
            {
                //printf("Seqcode %d %ld\n", seqcode, tn->tn_Children[seqcode]);
                if (tn->tn_Children[seqcode])
                {
                    /* there is a child, go down */
                    tn = GoDownNodeChild(tn, seqcode);

                    abortpath = FALSE;
                    /* do some early checks */
                    edgeptr = tn->tn_Edge;
                    if (tn->tn_TreeOffset < dq->dq_ProbeLength)
                    {
                        pathleft = dq->dq_ProbeLength - tn->tn_TreeOffset;
                        cnt = 0;
                    } else {
                        cnt = tn->tn_TreeOffset - tn->tn_EdgeLen;
                        pathleft = 0;
                    }
                    while (*edgeptr && (cnt++ < dq->dq_ProbeLength))
                    {
                        switch(pg->pg_CompressTable[*edgeptr++])
                        {
                            case SEQCODE_C:
                            case SEQCODE_G:
                                                currtemp += 4.0;
                                                currgc++;
                                                break;
                            case SEQCODE_A:
                            case SEQCODE_T:
                                                currtemp += 2.0;
                                                break;
                            default:
                                                //printf("N seq\n");
                                                abortpath = TRUE;
                                                break;
                        }
                    }

                    if ((currtemp > dq->dq_MaxTemp) ||              // check temperature out of range
                        (currtemp + (4.0 * pathleft) < dq->dq_MinTemp))
                    {
                        /*printf("temp %f <= [%f] <= %f out of range!\n",
                        dq->dq_MinTemp, currtemp, dq->dq_MaxTemp);*/
                        abortpath = TRUE;
                    }
                    if ((currgc > dq->dq_MaxGC) ||                  // check gc content
                        (currgc + pathleft < dq->dq_MinGC))
                    {
                        /*printf("gc content %ld <= [%ld] <= %ld out of range!\n",
                        dq->dq_MinGC, currgc, dq->dq_MaxGC);*/
                        abortpath = TRUE;
                    }
                    if (abortpath)                                  // abort path processing here
                    {
                        //GetTreePath(tn, buf, 24);
                        //printf("Path aborted: %s\n", buf);
                        seqcode = pg->pg_AlphaSize;
                        break;
                    }
                    //printf("Down %d %08lx\n", seqcode, tn);
                    seqcode = SEQCODE_A;
                    break;
                }
                seqcode++;
            }

            while(seqcode == pg->pg_AlphaSize)                      // we didn't find any children
            {
                /* go up again */
                //printf("Up\n");
                /* undo temperature and GC */
                edgeptr = tn->tn_Edge;
                cnt     = tn->tn_TreeOffset - tn->tn_EdgeLen;
                while (*edgeptr && (cnt++ < dq->dq_ProbeLength))
                {
                    switch (pg->pg_CompressTable[*edgeptr++])
                    {
                        case SEQCODE_C:
                        case SEQCODE_G:
                                            currtemp -= 4.0;
                                            currgc--;
                                            break;
                        case SEQCODE_A:
                        case SEQCODE_T:
                                            currtemp -= 2.0;
                                            break;
                    }
                }
                parenttn = tn->tn_Parent;
                seqcode  = tn->tn_ParentSeq + 1;
                freeset(tn, parenttn);
                if (! tn)
                {
                    /* we're done with this partition */
                    done = TRUE;
                    break; /* we're done! */
                }
            }
            if (done)
            {
                break;
            }
        }

        if (done)
        {
            break;
        }

        nohits = FALSE;

        /* now examine subtree for valid hits */
        dq->dq_TreeNode = tn;
        if (AddDesignHit(dq))
        {
            //GetTreePath(tn, buf, dq->dq_ProbeLength);
            //printf("Foo: %s\n", buf);
        }

        /* undo temperature and GC */
        edgeptr = tn->tn_Edge;
        cnt     = tn->tn_TreeOffset - tn->tn_EdgeLen;
        while (*edgeptr && (cnt++ < dq->dq_ProbeLength))
        {
            switch (pg->pg_CompressTable[*edgeptr++])
            {
                case SEQCODE_C:
                case SEQCODE_G:
                                    currtemp -= 4.0;
                                    currgc--;
                                    break;
                case SEQCODE_A:
                case SEQCODE_T:
                                    currtemp -= 2.0;
                                    break;
            }
        }
        parenttn = tn->tn_Parent;
        seqcode  = tn->tn_ParentSeq + 1;
        freeset(tn, parenttn);
    } while (TRUE);

    if (nohits)
    {
        printf("Warning: Temperature and GC content limitations already outrule any potential hits!\n");
    }
    return TRUE;
}
/* \\\ */

/* /// "PT_start_design()" */
extern "C" int PT_start_design(PT_pdc *pdc, int)
{
    /* I really don't want to know what's behind this: */
    PT_local *locs = (PT_local *) pdc->mh.parent->parent;
    struct PTPanGlobal *pg = PTPanGlobalPtr;
    struct PTPanPartition *pp;
    struct DesignQuery *dq;

    printf("EXTERN: PT_start_design\n");

    /* allocate and fill design query data structure */
    dq = AllocDesignQuery(pg);
    if (! dq)
    {
        printf("Couldn't allocate design query!\n");
        return 0;
    }
    dq->dq_PDC = pdc;
    /* mark species that should be in the group */
    dq->dq_MarkedSpecies = MarkSpeciesGroup(pg, pdc->names.data);
    //locs->group_count = dq->dq_MarkedSpecies;

    dq->dq_ProbeLength = pdc->probelen;
    dq->dq_MinGroupHits = (ULONG) (pdc->mintarget * (double) dq->dq_MarkedSpecies + 0.5);
    dq->dq_MaxNonGroupHits = pdc->mishit;
    dq->dq_MaxHairpin = (ULONG) pdc->maxbonds;
    dq->dq_MinTemp = pdc->mintemp;
    dq->dq_MaxTemp = pdc->maxtemp;
    dq->dq_MinGC   = (ULONG) (pdc->min_gc * (double) pdc->probelen);
    dq->dq_MaxGC   = (ULONG) (pdc->max_gc * (double) pdc->probelen + 0.5);
    dq->dq_MaxHits = pdc->clipresult;

    /* get first partition */
    pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
    while (pp->pp_Node.ln_Succ)
    {
        dq->dq_PTPanPartition = pp;
        FindProbeInPartition(dq);
        pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
    }
    printf("%ld probes generated...\n", dq->dq_NumHits);
    CalcProbeQuality(dq);
    return 0;
}
/* \\\ */





