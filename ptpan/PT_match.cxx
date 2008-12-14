
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include <struct_man.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <arbdbt.h>
#include <math.h>

/* /// "SearchPartition()" */
void SearchPartition(struct PTPanPartition *pp, struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SearchQuery *tmpsq;

  /* do search on this partition */
  tmpsq = CloneSearchQuery(sq);
  tmpsq->sq_PTPanPartition = pp;
  tmpsq->sq_SourceSeq = pp->pp_PrefixSeq;

  if (PTPanGlobalPtr->pg_verbose >0)
    printf("== SearchPartition: for %s\n", sq->sq_Query);

  if(MatchSequence(tmpsq))
  {
    if(!(pp->pp_CacheNode = CacheLoadData(pg->pg_PartitionCache, pp->pp_CacheNode, pp)))
    {
      return; /* something went wrong while loading */
    }
    SearchTree(tmpsq);
    PostFilterQueryHits(tmpsq);
    MergeQueryHits(sq, tmpsq); /* needs semaphore protection on parallel runs */
  }
  pp->pp_Done = TRUE;
  FreeSearchQuery(tmpsq);
}
/* \\\ */

#ifdef BENCHMARK
/* /// "QueryTests()" */
void QueryTests(struct PTPanGlobal *pg)
{
  PT_local *locs;
  STRPTR ecoli;
  ULONG ecolilen;
  ULONG pos;
  ULONG qlen;
  char buf[32];

  locs = (PT_local *) calloc(1, sizeof(PT_local));
  ecoli = FilterSequence(pg, pg->pg_EcoliSeq);
  ecolilen = strlen(ecoli);
  locs->pm_max = 0; /* exact search */
  locs->pm_complement = 0;
  locs->pm_reversed = 0;
  locs->sort_by = SORT_HITS_WEIGHTED;

  //qlen = 18;
  for(locs->pm_max = 4; locs->pm_max < 5; locs->pm_max++)
  {
//    for(qlen = 20; qlen - locs->pm_max >= 10; qlen--)
    for(qlen = 9 + locs->pm_max; qlen >= 10; qlen--)
//    for(qlen = 31; qlen >= 16; qlen--)
    {
      pg->pg_Bench.ts_Hits = 0;
      pg->pg_Bench.ts_UnsafeHits = 0;
      pg->pg_Bench.ts_UnsafeKilled = 0;
      pg->pg_Bench.ts_DupsKilled = 0;
      pg->pg_Bench.ts_CrossBoundKilled = 0;
      pg->pg_Bench.ts_DotsKilled = 0;
      pg->pg_Bench.ts_OutHits = 0;
      pg->pg_Bench.ts_CandSetTime= 0;
      pg->pg_Bench.ts_OutputTime = 0;

      for(pos = 0; pos < ecolilen - qlen; pos += 2)
      {
       strncpy(buf, &ecoli[pos], qlen);
       buf[qlen] = 0;
       probe_match(locs, strdup(buf));
      }
      printf("qDAT: (queries qlen err hits gentime unsafe unkill dupskill crosskill dotskill outhits outtime)\n");
      printf("%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %s QDAT\n",
             pos, qlen, locs->pm_max,
             pg->pg_Bench.ts_Hits,
             pg->pg_Bench.ts_CandSetTime,
             pg->pg_Bench.ts_UnsafeHits,
             pg->pg_Bench.ts_UnsafeKilled,
             pg->pg_Bench.ts_DupsKilled,
             pg->pg_Bench.ts_CrossBoundKilled,
        pg->pg_Bench.ts_DotsKilled,
        pg->pg_Bench.ts_OutHits,
        pg->pg_Bench.ts_OutputTime,
        pg->pg_DBName);
      fflush(stdout);
    }
  }
  printf("Done\n");
  free(ecoli);
  free(locs);
}
/* \\\ */
#endif


static void convertBondMatrix(PT_pdc *pdc, PTPanGlobal *pg)
{
    for (int query = SEQCODE_A; query <= SEQCODE_T; ++query) {
        for (int species = SEQCODE_A; species <= SEQCODE_T; ++species) {
            int rowIdx     = (pg->pg_ComplementTable[query] - SEQCODE_A)*4;
            int maxIdx     = rowIdx + query - SEQCODE_A;
            int newIdx     = rowIdx + species - SEQCODE_A;
            
            double max_bind = pdc->bond[maxIdx].val;
            double new_bind = pdc->bond[newIdx].val;
            
            pg->pg_MismatchWeights.mw_Replace[query * ALPHASIZE + species] = max_bind - new_bind;
        }
    }
}


static double calc_position_wmis(int pos, int seq_len, double y1, double y2)
{                                                           // TODO: check if (seq_len -1) is necessary
    return (double)(((double)(pos * (seq_len - 1 - pos)) / (double)((seq_len - 1) * (seq_len - 1)))* (double)(y2*4.0) + y1);
}


static void buildPosWeight(SearchQuery *sq)
{
    if (sq->sq_PosWeight) delete[] sq->sq_PosWeight;
    //printf("buildPosWeight: ...new double[%i];\n", sq->sq_QueryLen+1);
    sq->sq_PosWeight = new double[sq->sq_QueryLen+1];       // TODO: check if +1 is necessary

    for (int pos=0; pos < sq->sq_QueryLen; ++pos) {
        if (sq->sq_SortMode == SORT_HITS_WEIGHTED) {
            sq->sq_PosWeight[pos] = calc_position_wmis(pos, sq->sq_QueryLen, 0.3, 1.0);
        }else{
            sq->sq_PosWeight[pos] = 1.0;
        }
    }
    sq->sq_PosWeight[sq->sq_QueryLen] = 0.0;                // TODO: check if last pos is necessary
}


/* /// "probe_match()" */
extern "C" int probe_match(PT_local *locs, aisc_string probestring)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  struct PTPanPartition *pp;
  struct SearchQuery *sq;
  struct SearchQuery *compsq = NULL;
  PT_probematch *ml;

  pg->pg_SearchPrefs = locs;

  convertBondMatrix(locs->pdc, pg);
#if defined(DEBUG)
    PT_pdc *pdc = locs->pdc;
    printf("Current bond values:\n");
    for (int y = 0; y<4; y++) {
        for (int x = 0; x<4; x++) {
            printf("%5.2f", pdc->bond[y*4+x].val);
        }
        printf("\n");
    }
    printf("Current Replace Matrix:\n");
    for (int query = SEQCODE_A; query <= SEQCODE_T; ++query) {
        for (int species = SEQCODE_A; species <= SEQCODE_T; ++species) {
            printf("%5.2f", pg->pg_MismatchWeights.mw_Replace[query * ALPHASIZE + species]);
        }
        printf("\n");
    }
#endif // DEBUG

  /* find out where a given probe matches */
  if (PTPanGlobalPtr->pg_verbose >0) {
    printf("Search request for %s (errs = %d, compl = %d, rev = %d, weight = %d)\n",
           probestring,
           pg->pg_SearchPrefs->pm_max,
           pg->pg_SearchPrefs->pm_complement,
           pg->pg_SearchPrefs->pm_reversed,
           pg->pg_SearchPrefs->sort_by);
  }

  /* free the old sequence */
  if(pg->pg_SearchPrefs->pm_sequence)
  {
    free(pg->pg_SearchPrefs->pm_sequence);
  }
  pg->pg_SearchPrefs->pm_sequence = FilterSequence(pg, probestring);

  /* do we need to check the complement instead of the normal one? */
  if(pg->pg_SearchPrefs->pm_complement)
  {
    ComplementSequence(pg, pg->pg_SearchPrefs->pm_sequence);
  }

  /* do we need to look at the reversed sequence as well? */
  if(pg->pg_SearchPrefs->pm_reversed)
  {
    if(pg->pg_SearchPrefs->pm_csequence)
    {
      free(pg->pg_SearchPrefs->pm_csequence);
    }
    pg->pg_SearchPrefs->pm_csequence = strdup(pg->pg_SearchPrefs->pm_sequence);
    ReverseSequence(pg, pg->pg_SearchPrefs->pm_csequence);
    ComplementSequence(pg, pg->pg_SearchPrefs->pm_csequence);
  }

  //psg.main_probe = strdup(probestring);

  /* clear all old matches */
  while((ml = pg->pg_SearchPrefs->pm))
  {
    destroy_PT_probematch(ml);
  }

#if 1
  /* check, if the probe string is too short */
  if(strlen(pg->pg_SearchPrefs->pm_sequence) +
     (2 * pg->pg_SearchPrefs->pm_max) < MIN_PROBE_LENGTH)
  {
    SetARBErrorMsg(pg->pg_SearchPrefs, (STRPTR) "error: probe too short!!\n");
    free(probestring);
    return(0);
  }
#endif

  /* allocate query that configures and holds all the merged results */
  sq = AllocSearchQuery(pg);

  /* prefs */
  sq->sq_Query = (STRPTR) pg->pg_SearchPrefs->pm_sequence;
  sq->sq_QueryLen = strlen(sq->sq_Query);
  sq->sq_MaxErrors = (float) pg->pg_SearchPrefs->pm_max;
  sq->sq_Reversed = FALSE;
  sq->sq_AllowReplace = TRUE;
  sq->sq_AllowInsert = TRUE;
  sq->sq_AllowDelete = TRUE;
  sq->sq_KillNSeqsAt = strlen(sq->sq_Query) / 3;
  sq->sq_MinorMisThres = pg->pg_SearchPrefs->pdc->split;
  sq->sq_SortMode = pg->pg_SearchPrefs->sort_by;

  /* init */
  sq->sq_PTPanPartition = NULL;
  buildPosWeight(sq);
  if(pg->pg_SearchPrefs->sort_by)
  {
    /* user requested weighted searching */
    sq->sq_MismatchWeights = &sq->sq_PTPanGlobal->pg_MismatchWeights;
  } else {
    /* user wants unified searching */
    sq->sq_MismatchWeights = &sq->sq_PTPanGlobal->pg_NoWeights;
  }

  /* do we need to do a second query on the complement? */
  if(pg->pg_SearchPrefs->pm_reversed)
  {
    compsq = CloneSearchQuery(sq);
    compsq->sq_Query = (STRPTR) pg->pg_SearchPrefs->pm_csequence;
    compsq->sq_Reversed = TRUE;
  }

  /* start time here */
#ifdef BENCHMARK
  if (PTPanGlobalPtr->pg_verbose >0)
    BenchTimePassed(pg);
#endif

  /* search over partitions that are still in cache */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    pp->pp_Done = FALSE;
    if(CacheDataLoaded(pp->pp_CacheNode))
    {
      /* search normal */
      SearchPartition(pp, sq);
      /* and optionally, search complement */
      if(compsq)
      {
    SearchPartition(pp, compsq);
      }
    }
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }

  /* search over all partitions not done yet */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    if(!pp->pp_Done)
    {
      /* search normal */
      SearchPartition(pp, sq);
      /* and optionally, search complement */
      if(compsq)
      {
    SearchPartition(pp, compsq);
      }
    }
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }

#ifdef BENCHMARK
  if (PTPanGlobalPtr->pg_verbose >0)
    pg->pg_Bench.ts_CandSetTime += BenchTimePassed(pg);
#endif

  SortHitsList(sq);
  CreateHitsGUIList(sq);
  FreeSearchQuery(sq);
  if(compsq)
  {
    SortHitsList(compsq);
    CreateHitsGUIList(compsq);
    FreeSearchQuery(compsq);
  }

#ifdef BENCHMARK
  if (PTPanGlobalPtr->pg_verbose >0)
    pg->pg_Bench.ts_OutputTime += BenchTimePassed(pg);
#endif

  free(probestring); /* I actually don't know, if this is required */
  return 0;
}
/* \\\ */

/* /// "SortHitsList()" */
void SortHitsList(struct SearchQuery *sq)
{
  //struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  struct QueryHit *qh;

  /* enter priority and sort */
  qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  if(sq->sq_SortMode == SORT_HITS_NOWEIGHT)
  {
    /* sorting criteria:
       - normal/composite (1 bit)
       - replace only or insert/delete (1 bit)
       - mismatch count (5 bits)
       - error count (8 bits)
       - species (20 bits)
       - absolute position (28 bits)
    */
    //printf("Sort no weight...\n");
    while(qh->qh_Node.ln_Succ)
    {
      arb_assert(((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) <= 0x1f);  // 5 bit
      arb_assert(((LLONG) round(qh->qh_ErrorCount * 10.0)) <= 0xff);                        //  8 bit
      arb_assert(((LLONG) qh->qh_Species->ps_Num) <= 0xfffff);                              // 20 bit
      arb_assert(((LLONG) (qh->qh_AbsPos - qh->qh_Species->ps_AbsOffset)) <= 0xfffffff);    // 28 bit
      qh->qh_Node.ln_Pri = (LLONG)
    ((qh->qh_Flags & QHF_REVERSED) ? (1LL << 62) : 0LL) +
        ((qh->qh_InsertCount | qh->qh_DeleteCount) ? (1LL << 61) : 0LL) +
    (((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) << 56) +
    (((LLONG) round(qh->qh_ErrorCount * 10.0)) << 48) +
    (((LLONG) qh->qh_Species->ps_Num) << 28) +
    ((LLONG) (qh->qh_AbsPos - qh->qh_Species->ps_AbsOffset));
      qh = (struct QueryHit *) qh->qh_Node.ln_Succ;
    }
  } else {
    //printf("Sort with weight...\n");
    while(qh->qh_Node.ln_Succ)
    {
      arb_assert(((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) <= 0x1f);  // 5 bit
      arb_assert(((LLONG) round(qh->qh_ErrorCount * 10.0)) <= 0xff);                        //  8 bit
      arb_assert(((LLONG) qh->qh_Species->ps_Num) <= 0xfffff);                              // 20 bit
      arb_assert(((LLONG) (qh->qh_AbsPos - qh->qh_Species->ps_AbsOffset)) <= 0xfffffff);    // 28 bit
      qh->qh_Node.ln_Pri = (LLONG)
    ((LLONG) (qh->qh_Flags & QHF_REVERSED) ? (1LL << 62) : 0LL) +
        ((LLONG) (qh->qh_InsertCount | qh->qh_DeleteCount) ? (1LL << 61) : 0LL) +
    (((LLONG) round(qh->qh_ErrorCount * 10.0)) << 53) +
    (((LLONG) (qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount)) << 48) +
    ((LLONG) qh->qh_Species->ps_Num << 28) +
    ((LLONG) (qh->qh_AbsPos - qh->qh_Species->ps_AbsOffset));
      //printf("%16llx\n", qh->qh_Node.ln_Pri);
      qh = (struct QueryHit *) qh->qh_Node.ln_Succ;
    }
  }
  SortList(&sq->sq_Hits);
#if 0
  qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  //printf("... and after\n");
  while(qh->qh_Node.ln_Succ)
  {
    //printf("%16llx\n", qh->qh_Node.ln_Pri);
    qh = (struct QueryHit *) qh->qh_Node.ln_Succ;
  }
#endif
}
/* \\\ */

/* /// "GetSpeciesRelPos()" */
ULONG GetSpeciesRelPos(struct PTPanGlobal *pg, struct PTPanSpecies *ps, ULONG abspos)
{
  ULONG relpos = 0;
  STRPTR srcseq = ps->ps_SeqData;

  /* use checkpointing to skip over large regions */
  if(abspos >= ps->ps_ChkPntIVal)
  {
    relpos = ps->ps_CheckPoints[(abspos / ps->ps_ChkPntIVal) - 1];
    srcseq += relpos;
    abspos %= ps->ps_ChkPntIVal;
  }

  /* given an absolute sequence position, search for the relative one,
     e.g. abspos 2 on "-----UU-C-C" will yield 8 */
  while(*srcseq)
  {
    if(pg->pg_SeqCodeValidTable[*srcseq++])
    {
      if(!(abspos--))
      {
    break; /* position found */
      }
    }
    relpos++;
  }
  return(relpos);
}
/* \\\ */


/* /// "CreateHitsGUIList()" */
void CreateHitsGUIList(struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  struct PTPanSpecies *ps;
  struct QueryHit *qh;
  STRPTR srcptr;
  STRPTR tarptr;
  ULONG maxlen;
  float minweight;
  ULONG cnt;
  ULONG numhits;
  ULONG tarlen;

  if (PTPanGlobalPtr->pg_verbose >0) printf(">> CreateHitsGUIList\n");
    
  /* calculate maximum size of string that we have to examine */
  minweight = sq->sq_MismatchWeights->mw_Delete[0];
  for(cnt = 1; cnt < pg->pg_AlphaSize; cnt++)
  {
    if(sq->sq_MismatchWeights->mw_Delete[cnt] < minweight)
    {
      minweight = sq->sq_MismatchWeights->mw_Delete[cnt];
    }
  }
  maxlen = sq->sq_QueryLen + (ULONG) ((sq->sq_MaxErrors + minweight) / minweight);
  sq->sq_SourceSeq = (STRPTR) malloc(maxlen + 1);

  GB_begin_transaction(pg->pg_MainDB);
  numhits = 0;
  pg->pg_SpeciesCache->ch_SwapCount = 0;

  qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  while(qh->qh_Node.ln_Succ)
  {
    LONG relpos;
    BOOL good;
    ULONG nmismatch;
    UBYTE code;
    UBYTE seqcode;

    STRPTR seqout;
    STRPTR seqptr;
    LONG   relposcnt;
    PT_probematch *ml;

    good = TRUE;
    ps = qh->qh_Species;
#ifdef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
    char prefix[10], postfix[10];
    for (int i = 0; i < 9; ++i)
    {
        prefix[i] = '>';
        postfix[i] = '<';
    }
    prefix[9] = postfix[9] = 0x00;

    relpos    = 0;
    nmismatch = 0;
    ULONG abspos = qh->qh_AbsPos - ps->ps_AbsOffset;
    ULONG bitpos = 0;
    ULONG count;
    /* given an absolute sequence position, search for the relative one,
       e.g. abspos 2 on "-----UU-C-C" will yield 8 */
    while (bitpos < ps->ps_SeqDataCompressedSize)           // get relpos and store prefix
    {
        code = GetNextCharacter(pg, ps->ps_SeqDataCompressed, bitpos, count);
/*        
    if (strcmp(ps->ps_Name, "RcnComm4") == 0)
        printf("prefix: %9s  postfix: %9s  abspos: %i  relpos: %i  code: %c  count: %i\n", 
               prefix, postfix, abspos, relpos, code, count);        
*/
        if (pg->pg_SeqCodeValidTable[code])
        {           // it's a validchar
            if (!(abspos--)) break;                         // position found
            if (abspos <= 8) prefix[8-abspos] = code;       // store prefix
            ++relpos;
        } else
        {
            arb_assert((code == '.') || (code == '-'));
            relpos += count;
            if ((code == '.') && (abspos <= 9))             // fill prefix with '.'
            {   
                for (int i = 0; i < (9 - abspos); ++i)
                {
                    prefix[i] = '.';
                }
            }
        }
    }
    arb_assert(bitpos <= ps->ps_SeqDataCompressedSize);
    bitpos -= 3;      // bitpos now points to the first character of found seq
    
    tarlen = sq->sq_QueryLen - qh->qh_DeleteCount + qh->qh_InsertCount;
    for (cnt = 0; cnt < tarlen;)
    {
        if (bitpos >= ps->ps_SeqDataCompressedSize)
        {
            arb_assert(false);
            good = FALSE;
            break;
        }

        code = GetNextCharacter(pg, ps->ps_SeqDataCompressed, bitpos, count);
        if (pg->pg_SeqCodeValidTable[code])                 // valid character
        {
            sq->sq_SourceSeq[cnt++] = code;
            if(code == 'N') nmismatch++;
        } else
        {
            if (code == '.')                                // if we got a dot in sequence
            {                                               // the hit is bogus
                pg->pg_Bench.ts_DotsKilled++;
                good = FALSE;
                break;
            }
        }
    }
    sq->sq_SourceSeq[tarlen] = 0;
    if(nmismatch == tarlen) good = FALSE;

    if(good)
    {
        /* we need to verify the hit? */
        if(qh->qh_Flags & QHF_UNSAFE)
        {
            pg->pg_Bench.ts_UnsafeHits++;

            good = MatchSequence(sq);
            if(!good)
            {
                pg->pg_Bench.ts_UnsafeKilled++;
                //printf("Verify failed on %s != %s\n", sq->sq_SourceSeq, sq->sq_Query);
                qh->qh_Flags &= ~QHF_ISVALID;
            } else {
                /* fill in correct match */
                qh->qh_ErrorCount   = sq->sq_State.sqs_ErrorCount;
                qh->qh_ReplaceCount = sq->sq_State.sqs_ReplaceCount;
                qh->qh_InsertCount  = sq->sq_State.sqs_InsertCount;
                qh->qh_DeleteCount  = sq->sq_State.sqs_DeleteCount;
            }
            //qh->qh_Flags &= ~QHF_UNSAFE;
        }
    } //else printf("'.'-Sequence!\n");

    if(good)
    {
        seqout = (STRPTR) calloc(9 + 1 + sq->sq_QueryLen + 1 + 9 + 1, 0x01);
        strncpy(seqout, prefix, 0x09);                          // copy prefix
        seqout[9] = '-';                                        // 1st delimiter
        good = FindSequenceMatch(sq, qh, &seqout[10]);          // generate mismatch sequence */
        seqout[10 + sq->sq_QueryLen] = '-';                     // 2nd delimiter
        if (!good) free(seqout);
    }

    if (good)
    {
        for (cnt = 0; cnt < 9;)                                 // generate postfix
        {
            code = GetNextCharacter(pg, ps->ps_SeqDataCompressed, bitpos, count);
            if (code == 0xff) break;
            if (pg->pg_SeqCodeValidTable[code])                 // valid character
            {  
                postfix[cnt++] = code;
            } else if (code == '.')                             // '.' found
            {
                for (; cnt < 9; ++cnt)                          // fill postfix with '.'
                {
                    postfix[cnt] = '.';
                }
            }
        }

        strncpy(&seqout[11 + sq->sq_QueryLen], postfix, 0x09);  // copy postfix

        ml = create_PT_probematch();
        ml->name = qh->qh_Species->ps_Num;
        ml->b_pos = relpos;
        ml->rpos = qh->qh_AbsPos - ps->ps_AbsOffset;
        ml->wmismatches = (double) qh->qh_ErrorCount;
        ml->mismatches = qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount;
        ml->N_mismatches = nmismatch;
        ml->sequence = seqout; /* warning! potentional memory leak -- FIX destroy_PT_probematch(ml) */
        ml->reversed = (qh->qh_Flags & QHF_REVERSED) ? 1 : 0;

        aisc_link((struct_dllpublic_ext *) &(pg->pg_SearchPrefs->ppm), (struct_dllheader_ext *) ml);
        numhits++;

        if (PTPanGlobalPtr->pg_verbose >0) printf("SeqOut: '%s'\n", seqout);
    }

    RemQueryHit(qh);
    qh = (struct QueryHit *) sq->sq_Hits.lh_Head;

#else
    /* load alignment */
    ps->ps_CacheNode = CacheLoadData(pg->pg_SpeciesCache, ps->ps_CacheNode, ps);
    /* get relative position in unfiltered alignment */
    //relpos = GetSequenceRelPos(pg, ps->ps_SeqData, qh->qh_AbsPos - ps->ps_AbsOffset);

    /* new faster version with checkpoint lookup */
    relpos = GetSpeciesRelPos(pg, ps, qh->qh_AbsPos - ps->ps_AbsOffset);

    if (PTPanGlobalPtr->pg_verbose >1) {
      int i;

      printf("Pos: %ld (%s) %s [%ld/%ld] (%f error) [%d/%d/%d]\n",
        qh->qh_AbsPos,
        (qh->qh_Flags & QHF_UNSAFE) ? "unsafe" : "safe",
        ps->ps_Name, qh->qh_AbsPos - ps->ps_AbsOffset, ps->ps_RawDataSize,
        qh->qh_ErrorCount,
        qh->qh_ReplaceCount, qh->qh_InsertCount, qh->qh_DeleteCount);
      printf("SrcPos: '");
      for(i=0;i<50;i++)
    printf("%c", ps->ps_SeqData[relpos+i]);
      printf("'\n");
    }

    /* and filter sequence starting from that position */
    nmismatch = 0;
    srcptr = &ps->ps_SeqData[relpos];
    tarptr = sq->sq_SourceSeq;
    tarlen = sq->sq_QueryLen - qh->qh_DeleteCount + qh->qh_InsertCount;
    cnt = tarlen;
    while((code = *srcptr++))
    {
      /* if we go into a . sequence, the hit is bogus */
      if(code == '.')
      {
    pg->pg_Bench.ts_DotsKilled++;
    good = FALSE;
    break;
      }
      if(pg->pg_SeqCodeValidTable[code])
      {
    /* add sequence code */
    seqcode = pg->pg_CompressTable[code];
    *tarptr++ = pg->pg_DecompressTable[seqcode];

    /* check if it was an N */
    if(!seqcode)
    {
    nmismatch++;
    }
    if(!(--cnt))
    {
    break;
    }
      }
    }
    *tarptr++ = 0;
    /* filter all complete N strings */
    if(nmismatch == tarlen)
    {
      good = FALSE;
    }

    /* still okay? */
    if(good)
    {
      /* we need to verify the hit? */
      if(qh->qh_Flags & QHF_UNSAFE)
      {
    pg->pg_Bench.ts_UnsafeHits++;

    good = MatchSequence(sq);
    if(!good)
    {
    pg->pg_Bench.ts_UnsafeKilled++;
    //printf("Verify failed on %s != %s\n", sq->sq_SourceSeq, sq->sq_Query);
    qh->qh_Flags &= ~QHF_ISVALID;
    } else {
    /* fill in correct match */
    qh->qh_ErrorCount = sq->sq_State.sqs_ErrorCount;
    qh->qh_ReplaceCount = sq->sq_State.sqs_ReplaceCount;
    qh->qh_InsertCount = sq->sq_State.sqs_InsertCount;
    qh->qh_DeleteCount = sq->sq_State.sqs_DeleteCount;
    }
    //qh->qh_Flags &= ~QHF_UNSAFE;
      }
    } else {
      //printf("'.'-Sequence!\n");
    }

    /* this is a valid one */
    if(good)
    {
      /* allocate memory for the result string
    nine chars in front of the sequence (pre-seq), one delimiter,
    the sequence string, a delimiter, mine more chars (post-seq),
    and the termination byte */
      seqout = (STRPTR) malloc(9 + 1 + sq->sq_QueryLen + 1 + 9 + 1);

      /* generate pre-seq */
      relposcnt = relpos;
      seqptr = &seqout[9];
      for(cnt = 0; cnt < 9; cnt++)
      {
    /* loop while we're not at the start of the sequence */
    while(--relposcnt >= 0)
    {
    code = ps->ps_SeqData[relposcnt];
    if(code == '.') /* we encountered a ., fill rest with dots */
    {
        while(cnt++ < 9)
        {
        *--seqptr = code;
        }
        break;
    }
    /* if it's a valid char, we add it */
    if(pg->pg_SeqCodeValidTable[code])
    {
        *--seqptr = pg->pg_DecompressTable[pg->pg_CompressTable[code]];
        break;
    }
    }
    if(relposcnt < 0)
    {
    /* we reached the start of the sequence, fill with > chars */
    while(cnt++ < 9)
    {
        *--seqptr = '>';
    }
    }
      }
      seqout[9] = '-'; /* delimiter */
      seqptr = &seqout[7 + sq->sq_QueryLen];

      /* generate mismatch sequence */
      good = FindSequenceMatch(sq, qh, &seqout[10]);
    }

    if(good)
    {
      seqptr = &seqout[10 + sq->sq_QueryLen];
      *seqptr++ = '-'; /* delimiter */

      /* skip over the query itself */
      relposcnt = relpos;
      cnt = tarlen;
      while((code = ps->ps_SeqData[relposcnt++]))
      {
    /* if it's a valid char, decrease count */
    if(pg->pg_SeqCodeValidTable[code])
    {
    if(!--cnt)
    {
        break;
    }
    }
      }

      /* generate post-seq */
      for(cnt = 0; cnt < 9; cnt++)
      {
        /* loop while we're not at the end of the sequence */
       while(relposcnt < (LONG) ps->ps_SeqDataSize)
    {
    code = ps->ps_SeqData[relposcnt++];
    if(code == '.') /* we encountered a ., fill rest with dots */
    {
        while(cnt++ < 9)
        {
        *seqptr++ = code;
        }
        break;
    }
    /* if it's a valid char, we add it */
    if(pg->pg_SeqCodeValidTable[code])
    {
        *seqptr++ = pg->pg_DecompressTable[pg->pg_CompressTable[code]];
        break;
    }
    }
    if(relposcnt >= (LONG) ps->ps_SeqDataSize)
    {
    /* we reached the start of the sequence, fill with > chars */
    while(cnt++ < 9)
    {
        *seqptr++ = '<';
    }
    }
      }
      *seqptr = 0;

      ml = create_PT_probematch();
      ml->name = qh->qh_Species->ps_Num;
      ml->b_pos = relpos;
      ml->rpos = qh->qh_AbsPos - ps->ps_AbsOffset;
      ml->wmismatches = (double) qh->qh_ErrorCount;
      ml->mismatches = qh->qh_ReplaceCount + qh->qh_InsertCount + qh->qh_DeleteCount;
      ml->N_mismatches = nmismatch;
      ml->sequence = seqout; /* warning! potentional memory leak -- FIX destroy_PT_probematch(ml) */
      ml->reversed = (qh->qh_Flags & QHF_REVERSED) ? 1 : 0;

      aisc_link((struct_dllpublic_ext *) &(pg->pg_SearchPrefs->ppm), (struct_dllheader_ext *) ml);
      numhits++;

      if (PTPanGlobalPtr->pg_verbose >0)
    printf("SeqOut: '%s'\n", seqout);
    }

    RemQueryHit(qh);
    qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
#endif  
  } // while(qh->qh_Node.ln_Succ)
  GB_commit_transaction(pg->pg_MainDB);
  free(sq->sq_SourceSeq);

  if (PTPanGlobalPtr->pg_verbose >0) {
    pg->pg_Bench.ts_OutHits += numhits;
    printf("<< CreateHitsGUIList: Number of hits %ld (SwapCount %ld)\n",
    numhits, pg->pg_SpeciesCache->ch_SwapCount);
  }
}
/* \\\ */

/* /// "get_match_info()" */
extern "C" STRPTR get_match_info(PT_probematch *ml)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  struct PTPanSpecies *ps;
  ULONG ecolipos = 0;

  /* calculate ecoli position in O(1) */
  if(pg->pg_EcoliBaseTable)
  {
    if((ULONG) ml->b_pos < pg->pg_EcoliSeqSize)
    {
      ecolipos = pg->pg_EcoliBaseTable[ml->b_pos];
    } else {
      ecolipos = pg->pg_EcoliBaseTable[pg->pg_EcoliSeqSize];
    }
  }
  ps = pg->pg_SpeciesMap[ml->name];
  sprintf(pg->pg_TempBuffer, "%10s %-30.30s %2d  %2d    %1.1f %7d %4ld  %1d  %s",
    ps->ps_Name, ps->ps_FullName,
    ml->mismatches, ml->N_mismatches, ml->wmismatches,
    ml->b_pos, ecolipos,
    ml->reversed, ml->sequence);

  if (PTPanGlobalPtr->pg_verbose >0) printf("== get_match_info: %s\n", pg->pg_TempBuffer);

  return(pg->pg_TempBuffer);
}
/* \\\ */

/* /// "GetMatchListHeader()" */
STRPTR GetMatchListHeader(STRPTR seq)
{
  STRPTR res;

  if(seq)
  {
    res = (STRPTR) GBS_global_string("   name      fullname                       "
                    "mis N_mis wmis    pos ecoli rev         '%s'", seq);
  } else {
    res = (STRPTR) "   name      fullname                       "
      "mis N_mis wmis    pos ecoli rev";
  }

  if (PTPanGlobalPtr->pg_verbose >0) printf("== GetMatchListHeader: %s\n", res);

  return(res);
}
/* \\\ */

/* /// "get_match_hinfo()" */
extern "C" STRPTR get_match_hinfo(PT_probematch *)
{
  return(GetMatchListHeader(NULL));
}
/* \\\ */

/* /// "c_get_match_hinfo()" */
extern "C" STRPTR c_get_match_hinfo(PT_probematch *)
{
  printf("EXTERN: c_get_match_hinfo\n");
  return(GetMatchListHeader(NULL));
}
/* \\\ */

/* /// "match_string()" */
/* Create a big output string:  header\001name\001info\001name\001info....\000 */
extern "C" bytestring * match_string(PT_local *locs)
{
#ifdef DEVEL_JB
    struct PTPanGlobal *pg = PTPanGlobalPtr;
    struct GBS_strstruct *outstr;
    PT_probematch *ml;
    STRPTR srcptr;
    LONG entryCount = 0;
  
    printf("EXTERN: match_string\n");
    free(pg->pg_ResultString.data);             // free old memory
    for(ml = locs->pm; ml; ml = ml->next)       // count number of entries
        ++entryCount;
    
    outstr = GBS_stropen(entryCount * 150);     // 150 bytes per entry seemes to be a good estimation

    if(locs->pm)                                // add header
    {
        srcptr = GetMatchListHeader(locs->pm->reversed ? locs->pm_csequence : locs->pm_sequence);
        GBS_strcat(outstr, srcptr);
        GBS_chrcat(outstr, 1);
    }
        
    for(ml = locs->pm; ml; ml = ml->next)       // add each entry to the list
    {
        srcptr = virt_name(ml);                 // add the name
        GBS_strcat(outstr, srcptr);
        GBS_chrcat(outstr, 1);

        srcptr = get_match_info(ml);            // and the info
        GBS_strcat(outstr, srcptr);
        GBS_chrcat(outstr, 1);
    }

    pg->pg_ResultString.data = GBS_strclose(outstr);  
    pg->pg_ResultString.size = strlen(pg->pg_ResultString.data) + 1;

    if (PTPanGlobalPtr->pg_verbose >0) printf("== match_string: %s\n", pg->pg_ResultString.data);

#ifdef DEBUG
    printf("%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n", 
           entryCount, pg->pg_ResultString.size, pg->pg_ResultString.size >> 20, 
           (double)pg->pg_ResultString.size/(double)entryCount);
#endif
  return(&pg->pg_ResultString);

#else
  
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  PT_probematch *ml;
  BOOL first = TRUE;
  BOOL revstate = TRUE;
  STRPTR outptr;
  STRPTR srcptr;
  LONG buflen = 100000000;                 // TODO: calculate buflen instead of using hard coded value

  printf("EXTERN: match_string\n");
  /* free old memory */
  free(pg->pg_ResultString.data);

  outptr = (STRPTR) malloc(buflen);
  pg->pg_ResultString.data = outptr;

  buflen--; /* space for termination byte */

  LONG entryCount = 0;
  /* add each entry to the list */
  for(ml = locs->pm; ml; ml = ml->next)
  {
    ++entryCount;

    /* do we need to add a header? */
    if(first)// || (ml->reversed != revstate))
    {
      revstate = ml->reversed;
      /* copy the header */
      srcptr = GetMatchListHeader(revstate ? locs->pm_csequence : locs->pm_sequence);
      while((--buflen > 0) && (*outptr++ = *srcptr++));
      if(buflen <= 0)
      {
        printf("ERROR: buffer too small - see function match_string(...) in file PT_match.cxx\n");
        break;
      }
      outptr[-1] = 1;
      if(!first)
      {
    *outptr++ = 1;
    *outptr++ = 1;
      }
      first = FALSE;
    }
    /* add the name */
    srcptr = virt_name(ml);
    while((--buflen > 0) && (*outptr++ = *srcptr++));
    if(buflen <= 0)
    {
      printf("ERROR: buffer too small - see function match_string(...) in file PT_match.cxx\n");
      break;
    }
    outptr[-1] = 1;

    /* and the info */
    srcptr = get_match_info(ml);
    while((--buflen > 0) && (*outptr++ = *srcptr++));
    if(buflen <= 0)
    {
      printf("ERROR: buffer too small - see function match_string(...) in file PT_match.cxx\n");
      break;
    }
    outptr[-1] = 1;
  }
  /* terminate string */
  *outptr++ = 0;
  pg->pg_ResultString.size = (ULONG) outptr - (ULONG) pg->pg_ResultString.data;
  /* free unused memory */
  pg->pg_ResultString.data = (STRPTR) realloc(pg->pg_ResultString.data,
                                  pg->pg_ResultString.size);

  if (PTPanGlobalPtr->pg_verbose >0) printf("== match_string: %s\n", pg->pg_ResultString.data);

#if defined(DEBUG)
  printf("%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n", 
         entryCount, (100000000-buflen), (100000000-buflen) >> 20, (double)(100000000-buflen)/(double)entryCount);
#endif         
  return(&pg->pg_ResultString);
#endif
}
/* \\\ */

/* /// "MP_match_string()" */
/* Create a big output string:  header\001name\001#mismatch\001name\001#mismatch....\000 */
extern "C" bytestring * MP_match_string(PT_local *locs)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  PT_probematch *ml;
  STRPTR outptr;
  STRPTR srcptr;
  LONG buflen = 100000000;                 // TODO: calculate buflen instead of using hard coded value

  printf("EXTERN: MP_match_string\n");
  /* free old memory */
  free(pg->pg_ResultMString.data);

  outptr = (STRPTR) malloc(buflen);
  pg->pg_ResultMString.data = outptr;

  buflen--; /* space for termination byte */

  LONG entryCount = 0;
  /* add each entry to the list */
  for(ml = locs->pm; ml; ml = ml->next)
  {
    ++entryCount;
    /* add the name */
    srcptr = virt_name(ml);
    while((--buflen > 0) && (*outptr++ = *srcptr++));
    if(buflen <= 0)
    {
      printf("ERROR: buffer too small - see function MP_match_string(...) in file PT_match.cxx\n");
      break;
    }
    outptr[-1] = 1;

    /* and and the mismatch and wmismatch count */
    sprintf(pg->pg_TempBuffer, "%2d\001%1.1f", ml->mismatches, ml->wmismatches);
    srcptr = pg->pg_TempBuffer;
    while((--buflen > 0) && (*outptr++ = *srcptr++));
    if(buflen <= 0)
    {
      printf("ERROR: buffer too small - see function MP_match_string(...) in file PT_match.cxx\n");
      break;
    }
    outptr[-1] = 1;
  }
  /* terminate string */
  *outptr++ = 0;

  pg->pg_ResultMString.size = (ULONG) outptr - (ULONG) pg->pg_ResultMString.data;
  /* free unused memory */
  pg->pg_ResultMString.data = (STRPTR) realloc(pg->pg_ResultMString.data,
                    pg->pg_ResultMString.size);

  if (PTPanGlobalPtr->pg_verbose >0) printf("== MP_match_string: %s\n", pg->pg_ResultString.data);

printf("%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n", 
        entryCount, (100000000-buflen), (100000000-buflen) >> 20, (double)(100000000-buflen)/(double)entryCount);
  return(&pg->pg_ResultMString);
}
/* \\\ */

/* /// "MP_all_species_string()" */
/* Create a big output string: 001name\001name\....\000 */
extern "C" bytestring * MP_all_species_string(PT_local *)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  struct PTPanSpecies *ps;
  STRPTR outptr;
  STRPTR srcptr;
//  LONG buflen = 500000; /* enough for about 50000 species */
  LONG buflen = 100000000;                 // TODO: calculate buflen instead of using hard coded value

  printf("EXTERN: MP_all_species_string\n");
  /* free old memory */
  free(pg->pg_SpeciesString.data);

  outptr = (STRPTR) malloc(buflen);
  pg->pg_SpeciesString.data = outptr;

  buflen--; /* space for termination byte */

  LONG entryCount = 0;
  /* add each entry to the list */
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    ++entryCount;
    /* add the name */
    srcptr = ps->ps_Name;
    while((--buflen > 0) && (*outptr++ = *srcptr++));
    if(buflen <= 0)
    {
      printf("ERROR: buffer too small - see function MP_all_species_string(...) in file PT_match.cxx\n");
      break;
    }
    outptr[-1] = 1;
    ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
  }
  /* terminate string */
  *outptr++ = 0;

  pg->pg_SpeciesString.size = (ULONG) outptr - (ULONG) pg->pg_SpeciesString.data;
  /* free unused memory */
  pg->pg_SpeciesString.data = (STRPTR) realloc(pg->pg_SpeciesString.data,
                        pg->pg_SpeciesString.size);
printf("%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n", 
        entryCount, (100000000-buflen), (100000000-buflen) >> 20, (double)(100000000-buflen)/(double)entryCount);
  return(&pg->pg_SpeciesString);
}
/* \\\ */

/* /// "MP_count_all_species()" */
extern "C" int MP_count_all_species(PT_local *)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  printf("EXTERN: MP_count_all_species\n");
  return(pg->pg_NumSpecies);
}
/* \\\ */


