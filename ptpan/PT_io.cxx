#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <PT_server.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <arbdbt.h>
#include <BI_helix.hxx>

#ifdef BENCHMARK
/* /// "BenchTimePassed()" */
ULONG BenchTimePassed(struct PTPanGlobal *pg)
{
  ULONG ms;

  gettimeofday(&pg->pg_Bench.ts_Now, NULL);
  ms = (pg->pg_Bench.ts_Now.tv_sec - pg->pg_Bench.ts_Last.tv_sec) * 1000;
  if(pg->pg_Bench.ts_Now.tv_usec < pg->pg_Bench.ts_Last.tv_usec)
  {
    ms -= 1000 - (pg->pg_Bench.ts_Last.tv_usec - pg->pg_Bench.ts_Now.tv_usec) / 1000;
  } else {
    ms += (pg->pg_Bench.ts_Now.tv_usec - pg->pg_Bench.ts_Last.tv_usec) / 1000;
  }
  pg->pg_Bench.ts_Last = pg->pg_Bench.ts_Now;
  return(ms);
}
/* \\\ */

/* /// "BenchOutput()" */
void BenchOutput(struct  PTPanGlobal *pg)
{
  struct PTPanPartition *pp;
  ULONG diskidxspace = 0;
  ULONG disknodecount = 0;
  ULONG disknodespace = 0;
  ULONG diskleafcount = 0;
  ULONG diskleafspace = 0;
  ULONG diskouterleaves = 0;
  ULONG memused;
  ULONG memusedmax = 0;

  pg->pg_Bench.ts_Last = pg->pg_Bench.ts_Init;
  pg->pg_Bench.ts_TotalBuild = BenchTimePassed(pg);
  printf("pDAT: (id np fsize tsize bufmem used 2e 5e depth pdepth plen edges ledges dictsize node# nodespc leaf# leafcnt outl)\n");
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    diskidxspace += pp->pp_DiskIdxSpace;
    disknodecount += pp->pp_DiskNodeCount;
    disknodespace += pp->pp_DiskNodeSpace;
    diskleafcount += pp->pp_DiskLeafCount;
    diskleafspace += pp->pp_DiskLeafSpace;
    diskouterleaves += pp->pp_DiskOuterLeaves;
    memused = pp->pp_SfxMemorySize - (pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset);
    if(memused > memusedmax)
    {
      memusedmax = memused;
    }
    printf("%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %s PDAT\n",
    pp->pp_ID,
    pp->pp_Size,
    pp->pp_DiskIdxSpace,
    pp->pp_DiskTreeSize,
    pp->pp_SfxMemorySize,
    memused,
    pp->pp_NumSmallNodes,
    pp->pp_NumBigNodes,
    pp->pp_MaxTreeDepth,
    pp->pp_TreePruneDepth,
    pp->pp_TreePruneLength,
    pp->pp_EdgeCount,
    pp->pp_LongEdgeCount,
    pp->pp_LongDictRawSize,
    pp->pp_DiskNodeCount,
    pp->pp_DiskNodeSpace,
    pp->pp_DiskLeafCount,
    pp->pp_DiskLeafSpace,
    pp->pp_DiskOuterLeaves,
    pg->pg_DBName);

    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }

  printf("gDAT: (n s lb np t idxsize memusedmax node# nodespc leaf# leafcnt outl Total CollDB MergeDB PScan MemTree Stats LDPre LDBuild Reloc Disk)\n"
    "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %s GDAT\n",
    pg->pg_TotalRawSize,
    pg->pg_NumSpecies,
    pg->pg_MaxBaseLength,
    pg->pg_NumPartitions,
    pg->pg_MaxPartitionSize,
    diskidxspace,
    memusedmax,
    disknodecount,
    disknodespace,
    diskleafcount,
    diskleafspace,
    diskouterleaves,
    pg->pg_Bench.ts_TotalBuild,
    pg->pg_Bench.ts_CollectDB,
    pg->pg_Bench.ts_MergeDB,
    pg->pg_Bench.ts_PrefixScan,
    pg->pg_Bench.ts_MemTree,
    pg->pg_Bench.ts_TreeStats,
    pg->pg_Bench.ts_LongDictPre,
    pg->pg_Bench.ts_LongDictBuild,
    pg->pg_Bench.ts_Reloc,
    pg->pg_Bench.ts_Writing,
    pg->pg_DBName);
};
/* \\\ */
#endif

/* /// "GetSequenceRelPos()" */
ULONG GetSequenceRelPos(struct PTPanGlobal *pg, STRPTR srcseq, ULONG abspos)
{
  ULONG relpos = 0;
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

/* /// "GetSequenceAbsPos()" */
ULONG GetSequenceAbsPos(struct PTPanGlobal *pg, STRPTR srcseq, ULONG relpos)
{
  ULONG abspos = 0;
  /* given an absolute sequence position, search for the relative one,
     e.g. relpos 8 on "-----UU-C-C" will yield 3 */
  while(*srcseq && relpos--)
  {
    if(pg->pg_SeqCodeValidTable[*srcseq++])
    {
      abspos++;
    }
  }
  return(abspos);
}
/* \\\ */

/* /// "CalcLengthForFilteredSequence()" */
ULONG CalcLengthForFilteredSequence(struct PTPanGlobal *pg, STRPTR srcseq)
{
  ULONG len = 0;
  /* calculate size of compressed sequence */
  while(*srcseq)
  {
    len += pg->pg_SeqCodeValidTable[*srcseq++];
  }
  return(len);
}
/* \\\ */

/* /// "FilterSequenceTo()" */
ULONG FilterSequenceTo(struct PTPanGlobal *pg, STRPTR srcstr, STRPTR filtptr)
{
  ULONG len = 0;
  UBYTE code;

  /* now actually filter the sequence */
  while((code = *srcstr++))
  {
    if(pg->pg_SeqCodeValidTable[code])
    {
      /* add sequence code */
      *filtptr++ = pg->pg_DecompressTable[pg->pg_CompressTable[code]];
      len++;
    }
  }
  *filtptr = 0;
  return(len);
}
/* \\\ */

/* /// "FilterSequence()" */
STRPTR FilterSequence(struct PTPanGlobal *pg, STRPTR srcseq)
{
  ULONG len;
  STRPTR filtseq;

  len = CalcLengthForFilteredSequence(pg, srcseq);
  filtseq = (STRPTR) malloc(len + 1);
  if(!filtseq)
  {
    return(NULL); /* out of memory */
  }
  /* now actually compress the sequence */
  len = FilterSequenceTo(pg, srcseq, filtseq);
  //printf("%ld bytes used.\n", len);

  return(filtseq);
}
/* \\\ */

/* /// "CompressSequenceTo()" */
ULONG CompressSequenceTo(struct PTPanGlobal *pg, STRPTR srcseq, ULONG *seqptr)
{
  ULONG len;
  ULONG seqcode;
  UWORD cnt;
  ULONG pval;
  UBYTE code;

  /* now actually compress the sequence */
  len = 4;
  cnt = 0;
  pval = 0;
  while((code = *srcseq++))
  {
    if(pg->pg_SeqCodeValidTable[code])
    {
      /* add sequence code */
      seqcode = pg->pg_CompressTable[code];
      pval *= pg->pg_AlphaSize;
      pval += seqcode;
      /* check, if storage capacity was reached? */
      if(++cnt == MAXCODEFITLONG)
      {
    /* write out compressed longword (with eof bit) */
    //printf("[%08lx]", pval | pg->pg_BitsMaskTable[cnt]);
    *seqptr++ = (pval << pg->pg_BitsShiftTable[MAXCODEFITLONG]) | pg->pg_BitsMaskTable[MAXCODEFITLONG];
    len += 4;
    cnt = 0;
    pval = 0;
      }
    }
  }

  /* write pending bits (with eof bit) */
  *seqptr = (pval << pg->pg_BitsShiftTable[cnt]) | pg->pg_BitsMaskTable[cnt];
  //printf("[%08lx]\n", *seqptr);
  return(len);
}
/* \\\ */

/* /// "CompressSequence()" */
ULONG * CompressSequence(struct PTPanGlobal *pg, STRPTR srcseq)
{
  ULONG len;
  ULONG *compseq;

  len = CalcLengthForFilteredSequence(pg, srcseq);
  //printf("compressing %s (%ld/%ld)...", srcseq, len, (ULONG) strlen(srcseq));

  /* that's all we need: ceil(len/MAXCODEFITLONG) longwords */
  compseq = (ULONG *) malloc(((len / MAXCODEFITLONG) + 1) * sizeof(ULONG));
  if(!compseq)
  {
    return(NULL); /* out of memory */
  }
  /* now actually compress the sequence */
  len = CompressSequenceTo(pg, srcseq, compseq);
  //printf("%ld bytes used.\n", len);

  return(compseq);
}
/* \\\ */

/* /// "GetLengthOfCompressedSequence() */
ULONG GetLengthOfCompressedSequence(struct PTPanGlobal *pg, ULONG *seqptr)
{
  ULONG len = 0;
  UWORD cnt;
  ULONG mask = pg->pg_BitsMaskTable[MAXCODEFITLONG];
  do
  {
    if(*seqptr++ & mask) /* check, if lowest bit is set */
    {
      len += MAXCODEFITLONG;
    } else {
      /* okay, we seem to be at the end of the compressed sequence,
         and we need to find out the actual size */
      --seqptr;
      cnt = MAXCODEFITLONG;
      while(--cnt)
      {
    if(*seqptr & pg->pg_BitsMaskTable[cnt]) /* seems like we found it */
    {
    len += cnt;
    break;
    }
      }
      break;
    }
  } while(TRUE);
  return(len);
}
/* \\\ */

/* /// "GetCompressedLongSize()" */
UWORD GetCompressedLongSize(struct PTPanGlobal *pg, ULONG pval)
{
  UWORD cnt = MAXCODEFITLONG;
  while(!(pval & pg->pg_BitsMaskTable[cnt])) /* check, if termination bit is set */
  {
    cnt--;
  }
  return(cnt);
}
/* \\\ */

/* /// "DecompressSequenceTo() */
ULONG DecompressSequenceTo(struct PTPanGlobal *pg, ULONG *seqptr, STRPTR tarseq)
{
  ULONG len = 0;
  BOOL lastlong;
  UWORD cnt;
  ULONG pval;
  do
  {
    /* get next longword */
    pval = *seqptr++;
    cnt = GetCompressedLongSize(pg, pval);
    pval >>= pg->pg_BitsShiftTable[cnt];
    lastlong = (cnt < MAXCODEFITLONG); /* last longword reached? */

    /* unpack compressed longword */
    if(cnt)
    {
      do
      {
    *tarseq++ = pg->pg_DecompressTable[(pval / pg->pg_PowerTable[--cnt])
                % pg->pg_AlphaSize];
    len++;
      } while(cnt);
    }
  } while(!lastlong);
  *tarseq = 0; /* null terminate string */

  return(len);
}
/* \\\ */

/* /// "DecompressCompressedLongTo() */
ULONG DecompressCompressedLongTo(struct PTPanGlobal *pg, ULONG pval, STRPTR tarseq)
{
  ULONG len;
  UWORD cnt;

  len = cnt = GetCompressedLongSize(pg, pval);
  pval >>= pg->pg_BitsShiftTable[cnt];
  /* unpack compressed longword */
  do
  {
    *tarseq++ = pg->pg_DecompressTable[(pval / pg->pg_PowerTable[--cnt])
                           % pg->pg_AlphaSize];
  } while(cnt);
  *tarseq = 0; /* null terminate string */
  return(len);
}
/* \\\ */

/* /// "DecompressSequence()" */
STRPTR DecompressSequence(struct PTPanGlobal *pg, ULONG *seqptr)
{
  ULONG len;
  STRPTR tarseq;
  /* first get length */
  len = GetLengthOfCompressedSequence(pg, seqptr);

  /* allocate memory for uncompressed sequence */
  tarseq = (STRPTR) malloc(len + 1);
  if(!tarseq)
  {
    return(NULL); /* out of memory */
  }

  /* decompress sequence */
  DecompressSequenceTo(pg, seqptr, tarseq);
  //printf("Decompressed sequence '%s'\n", tarseq);
  return(tarseq);
}
/* \\\ */

/* /// "DecompressSequencePartTo()" */
LONG DecompressSequencePartTo(struct PTPanGlobal *pg,
                ULONG *seqptr, ULONG seqpos, ULONG length,
                STRPTR tarseq)
{
  ULONG off = seqpos / MAXCODEFITLONG;
  UWORD codeoff = seqpos % MAXCODEFITLONG;
  UWORD cnt;
  ULONG len;
  ULONG pval;
  BOOL lastlong;
  BOOL first;

  if(!length) /* empty sequence requested? */
  {
    *tarseq = 0;
    return(0);
  }

  /* decompress sequence */
  first = TRUE;
  seqptr += off;
  do
  {
    /* get next longword */
    pval = *seqptr++;
    cnt = GetCompressedLongSize(pg, pval);
    pval >>= pg->pg_BitsShiftTable[cnt];
    lastlong = (cnt < MAXCODEFITLONG); /* last longword reached? */

    if(first) /* do we need to start at a certain offset? */
    {
      if(codeoff > cnt) /* past end of sequence? */
      {
        break;
      }
      cnt -= codeoff;
      first = FALSE;
    }
    /* unpack compressed longword */
    do
    {
      *tarseq++ = pg->pg_DecompressTable[(pval / pg->pg_PowerTable[--cnt])
                    % pg->pg_AlphaSize];
      len++;
      length--;
    } while(cnt && length);
  } while(length && !lastlong);
  *tarseq = 0; /* null terminate string */

  return(len);
}
/* \\\ */

/* /// "ComplementSequence()" */
void ComplementSequence(struct PTPanGlobal *pg, STRPTR seqstr)
{
  UBYTE code;
  /* flip A<->T and C<->G */
  while((code = *seqstr))
  {
    *seqstr++ = pg->pg_DecompressTable[pg->pg_ComplementTable[pg->pg_CompressTable[code]]];
  }
}
/* \\\ */

/* /// "ReverseSequence()" */
void ReverseSequence(struct PTPanGlobal *, STRPTR seqstr)
{
  char code;
  STRPTR leftptr = seqstr;
  STRPTR rightptr = &seqstr[strlen(seqstr)];

  /* reverse the sequence string */
  while(leftptr < rightptr)
  {
    code = *leftptr;
    *leftptr++ = *--rightptr;
    *rightptr = code;
  }
}
/* \\\ */

/* /// "OpenDataBase()"
   initially open the database and read the species data.
 */
BOOL OpenDataBase(struct PTPanGlobal *pg)
{
  GB_set_verbose();
  /* open the database */
  if(!(pg->pg_MainDB = GB_open(pg->pg_DBName, "r")))
  {
    printf("Error reading file %s\n", pg->pg_DBName);
    return(FALSE);
  }
  GB_begin_transaction(pg->pg_MainDB);
  /* open the species data */
  if(!(pg->pg_SpeciesData = GB_find(pg->pg_MainDB, "species_data", down_level)))
  {
    printf("Database %s is empty\n", pg->pg_DBName);
    return(FALSE);
  }
  /* add the extended data container */
  pg->pg_ExtendedData = GB_search(pg->pg_MainDB, "extended_data", GB_CREATE_CONTAINER);
  pg->pg_AlignmentName = GBT_get_default_alignment(pg->pg_MainDB);

  printf("Building PT-Server for alignment '%s'...\n", pg->pg_AlignmentName);

  GB_commit_transaction(pg->pg_MainDB);

  return(TRUE);
}
/* \\\ */

/* /// "LoadEcoliSequence()" */
BOOL LoadEcoliSequence(struct PTPanGlobal *pg)
{
  GBDATA *gb_extdata;
  STRPTR defaultref = GBT_get_default_ref(pg->pg_MainDB);

  gb_extdata = GBT_find_SAI_rel_exdata(pg->pg_ExtendedData, defaultref);
  free(defaultref);

  /* free memory if previously allocated */
  if(pg->pg_EcoliSeq)
  {
    free(pg->pg_EcoliSeq);
    pg->pg_EcoliSeq = NULL;
  }
  if(pg->pg_EcoliBaseTable)
  {
    free(pg->pg_EcoliBaseTable);
    pg->pg_EcoliBaseTable = NULL;
  }

  /* prepare ecoli sequence */
  if(gb_extdata)
  {
    GBDATA *gb_data;
    gb_data = GBT_read_sequence(gb_extdata, pg->pg_AlignmentName);
    if(gb_data)
    {
      ULONG abspos = 0;
      STRPTR srcseq;
      ULONG *posptr;

      /* load sequence */
      pg->pg_EcoliSeqSize = GB_read_string_count(gb_data);
      pg->pg_EcoliSeq = GB_read_string(gb_data);

      /* calculate look up table to speed up ecoli position calculation */
      pg->pg_EcoliBaseTable = (ULONG *) calloc(pg->pg_EcoliSeqSize + 1, sizeof(ULONG));
      if(pg->pg_EcoliBaseTable)
      {
    srcseq = pg->pg_EcoliSeq;
    posptr = pg->pg_EcoliBaseTable;
    while(*srcseq)
    {
    *posptr++ = abspos;
    if(pg->pg_SeqCodeValidTable[*srcseq++])
    {
        abspos++;
    }
    }
    *posptr = abspos;
    return(TRUE);
      } else {
    printf("Out of memory for ecoli position table!\n");
      }
    }
  }
  return(FALSE);
}
/* \\\ */

/* /// "FreeAllSpecies()" */
void FreeAllSpecies(struct PTPanGlobal *pg)
{
  struct PTPanSpecies *ps;
  FlushCache(pg->pg_SpeciesCache);
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    FreeCacheNode(pg->pg_SpeciesCache, ps->ps_CacheNode);
    Remove(&ps->ps_Node);
    free(ps->ps_Name);
    free(ps->ps_FullName);
    free(ps->ps_CheckPoints);
    free(ps);
    ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  }
  FreeBinTree(pg->pg_SpeciesBinTree);
  pg->pg_SpeciesBinTree = NULL;
  pg->pg_NumSpecies = 0;
  pg->pg_TotalSeqSize = 0;
  pg->pg_TotalRawSize = 0;
  pg->pg_TotalRawBits = 0;
}
/* \\\ */

/* /// "CacheSpeciesLoad()" */
BOOL CacheSpeciesLoad(struct CacheHandler *, struct PTPanSpecies *ps)
{
  //struct PTPanGlobal *pg = (struct PTPanGlobal *pg) ch->ch_UserData;

  if(!ps->ps_SeqData)
  {
    /* load alignment data */
    ps->ps_SeqData = GB_read_string(ps->ps_SeqDataDB);
    return(TRUE);
  }
  return(FALSE);
}
/* \\\ */

/* /// "CacheSpeciesUnload()" */
BOOL CacheSpeciesUnload(struct CacheHandler *, struct PTPanSpecies *ps)
{
  //struct PTPanGlobal *pg = (struct PTPanGlobal *pg) ch->ch_UserData;

  if(ps->ps_SeqData)
  {
    /* load alignment data */
    free(ps->ps_SeqData);
    ps->ps_SeqData = NULL;
    return(TRUE);
  }
  return(FALSE);
}
/* \\\ */

/* /// "CacheSpeciesSize()" */
ULONG CacheSpeciesSize(struct CacheHandler *, struct PTPanSpecies *ps)
{
  //struct PTPanGlobal *pg = (struct PTPanGlobal *pg) ch->ch_UserData;
  return(ps->ps_SeqDataSize);
}
/* \\\ */

/* /// "LoadSpecies()" */
BOOL LoadSpecies(struct PTPanGlobal *pg)
{
  GBDATA *gb_species;
  struct PTPanSpecies *ps;
  ULONG ignorecount;

  ULONG longestali = 0;

  /* NOTE: This database scan should avoided. We should store all the
     data that's built up here in a secondary file. That way we would
     get rid of the loading and scanning of the sequence data in low
     memory mode */

  GB_begin_transaction(pg->pg_MainDB);

  /* get the ecoli reference sequence */
  LoadEcoliSequence(pg);

  if(pg->pg_TotalRawSize) /* seems like we've already have the list */
  {
    /* only load in alignment data */
    if(pg->pg_LowMemoryMode)
    {
      GB_commit_transaction(pg->pg_MainDB);
      return(TRUE);
    }
    printf("Reloading alignment data...\n");
    ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
    while(ps->ps_Node.ln_Succ)
    {
      ps->ps_CacheNode = CacheLoadData(pg->pg_SpeciesCache, ps->ps_CacheNode, ps);
      ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
    }
    GB_commit_transaction(pg->pg_MainDB);
    return(TRUE);
  }

  FreeAllSpecies(pg);

  /* add the species to the list */
  pg->pg_MaxBaseLength = 0;
  pg->pg_TotalSeqSize = 0;
  pg->pg_TotalRawSize = 0;
  pg->pg_NumSpecies = 0;
  ignorecount = 0;
  for(gb_species = GBT_first_species_rel_species_data(pg->pg_SpeciesData);
      gb_species;
      gb_species = GBT_next_species(gb_species))
  {
    GBDATA *gb_name;
    GBDATA *gb_ali;
    GBDATA *gb_data;
    STRPTR spname;

    /* get name */
    gb_name = GB_find(gb_species, "name", down_level);
    if(!gb_name)
    {
      ignorecount++;
      continue; /* huh? couldn't find the name of the species? */
    }
    spname = GB_read_string(gb_name);

    /* get alignments */
    gb_ali = GB_find(gb_species, pg->pg_AlignmentName, down_level);
    if(!gb_ali)
    {
      ignorecount++;
      free(spname);
      continue; /* too bad, no alignment information found */
    }
    gb_data = GB_find(gb_ali, "data", down_level);
    if(!gb_data)
    {
      ignorecount++;
      fprintf(stderr, "Species '%s' has no data in '%s'\n",
      spname, pg->pg_AlignmentName);
      free(spname);
      continue;
    }

    /* okay, cannot fail now anymore, allocate a PTPanSpecies structure */
    ps = (struct PTPanSpecies *) calloc(1, sizeof(struct PTPanSpecies));

    /* write name and long name into the structure */
    ps->ps_SpeciesDB = gb_species;
    ps->ps_SeqDataDB = gb_data;
    ps->ps_IsGroup = TRUE;
    ps->ps_Name = spname;
    gb_name = GB_find(gb_species, "full_name", down_level);
    if(gb_name)
    {
      ps->ps_FullName = GB_read_string(gb_name);
    } else {
      ps->ps_FullName = strdup(ps->ps_Name);
    }

    /* (temporarily) load in the alignment and compress it */
    ps->ps_SeqDataSize = GB_read_string_count(ps->ps_SeqDataDB);
    ps->ps_CacheNode = CacheLoadData(pg->pg_SpeciesCache, ps->ps_CacheNode, ps);
    //ps->ps_SeqData = GB_read_string(ps->ps_SeqDataDB);
    ps->ps_RawDataSize = CalcLengthForFilteredSequence(pg, ps->ps_SeqData);

    //ps->ps_RawData = CompressSequence(pg, ps->ps_SeqData);
    //ps->ps_RawDataSize = GetLengthOfFilteredSequence(pg, ps->ps_RawData);

    if(strlen(ps->ps_SeqData) != ps->ps_SeqDataSize)
    {
      printf("%s is corrupt, ignoring!\n", ps->ps_Name);
      ignorecount++;
      FreeCacheNode(pg->pg_SpeciesCache, ps->ps_CacheNode);
      free(ps->ps_SeqData);
      free(ps->ps_Name);
      free(ps->ps_FullName);
      free(ps);
      continue; /* too bad, alignment was somehow corrupt */
    }

#if 0 /* not required anymore */
    if(pg->pg_LowMemoryMode) /* free memory in low memory case */
    {
      CacheUnloadData(pg->pg_SpeciesCache, ps->ps_CacheNode);
    }
#endif

    /* enter global absolute offset in index */
    ps->ps_AbsOffset = pg->pg_TotalRawSize;
    ps->ps_Node.ln_Pri = ps->ps_AbsOffset;
    pg->pg_TotalSeqSize += ps->ps_SeqDataSize;
    pg->pg_TotalRawSize += ps->ps_RawDataSize;
    if(ps->ps_RawDataSize > pg->pg_MaxBaseLength)
    {
      pg->pg_MaxBaseLength = ps->ps_RawDataSize;
    }
    if(ps->ps_SeqDataSize > longestali)
    {
      longestali = ps->ps_SeqDataSize;
    }
    /* Init complete, now add it to the list */
    //printf("Added %s ('%s')...\n", ps->ps_Name, ps->ps_FullName);
    AddTail(&pg->pg_Species, &ps->ps_Node);
    pg->pg_NumSpecies++;

    /* visual feedback */
    if((pg->pg_NumSpecies % 10) == 0)
    {
      if(pg->pg_NumSpecies % 500)
      {
    printf(".");
    fflush(stdout);
      } else {
    printf(".%6ld (%6ld KB)\n", pg->pg_NumSpecies, (ps->ps_AbsOffset >> 10));
      }
    }
  }

  /* calculate bits usage */
  pg->pg_TotalRawBits = 8;
  while((1UL << pg->pg_TotalRawBits) < pg->pg_TotalRawSize)
  {
    pg->pg_TotalRawBits++;
  }

  /* build tree to find species quicker by raw position */
  pg->pg_SpeciesBinTree = BuildBinTree(&pg->pg_Species);

  printf("\nLongest sequence was %ld bases (alignment size %ld).\n\n",
    pg->pg_MaxBaseLength, longestali);
  printf("Database contains %ld valid species (%ld ignored).\n"
    "%ld bytes alignment data (%ld bases).\n",
    pg->pg_NumSpecies, ignorecount, pg->pg_TotalSeqSize, pg->pg_TotalRawSize);

  pg->pg_Bench.ts_CollectDB = BenchTimePassed(pg);

  /* done! */
  GB_commit_transaction(pg->pg_MainDB);
  return(TRUE);
}
/* \\\ */

/* /// "LoadIndexHeader()" */
BOOL LoadIndexHeader(struct PTPanGlobal *pg)
{
  FILE *fh;
  GBDATA *gb_species;
  struct PTPanSpecies *ps;
  struct PTPanPartition *pp;
  ULONG numspec;
  ULONG ignorecount;
  ULONG endian = 0;
  UWORD version = 0;
  UWORD cnt;
  char idstr[16];

  FreeAllSpecies(pg);
  FreeAllPartitions(pg);

  /* Does similar things as LoadSpecies() */
  if(!(fh = fopen(pg->pg_IndexName, "r")))
  {
    printf("ERROR: Couldn't open index %s!\n", pg->pg_IndexName);
    return(FALSE);
  }

  /* read id string */
  fread(idstr, 16, 1, fh);
  if(strncmp("TUM PeTerPAN IDX", idstr, 16))
  {
    printf("ERROR: This is no index file!\n");
    fclose(fh);
    return(FALSE);
  }

  /* check endianness */
  fread(&endian, sizeof(endian), 1, fh);
  if(endian != 0x01020304)
  {
    printf("ERROR: Index was created on a different endian machine (%08lx)!\n", endian);
    fclose(fh);
    return(FALSE);
  }

  /* check file structure version */
  fread(&version, sizeof(version), 1, fh);
  if(version != FILESTRUCTVERSION)
  {
    printf("ERROR: Index (V%d.%d) does not match current file structure version (V%d.%d)!\n",
    version >> 8, version & 0xff,
    FILESTRUCTVERSION >> 8, FILESTRUCTVERSION & 0xff);
    fclose(fh);
    return(FALSE);
  }

  /* read the rest of the important data */
  fread(&pg->pg_UseStdSfxTree, sizeof(pg->pg_UseStdSfxTree), 1, fh);
  fread(&pg->pg_AlphaSize    , sizeof(pg->pg_AlphaSize)    , 1, fh);
  fread(&pg->pg_TotalSeqSize , sizeof(pg->pg_TotalSeqSize) , 1, fh);
  fread(&pg->pg_TotalRawSize , sizeof(pg->pg_TotalRawSize) , 1, fh);
  fread(&pg->pg_TotalRawBits , sizeof(pg->pg_TotalRawBits) , 1, fh);
  fread(&pg->pg_AllHashSum   , sizeof(pg->pg_AllHashSum)   , 1, fh);
  fread(&pg->pg_NumSpecies   , sizeof(pg->pg_NumSpecies)   , 1, fh);
  fread(&pg->pg_NumPartitions, sizeof(pg->pg_NumPartitions), 1, fh);
  fread(&pg->pg_MaxPrefixLen , sizeof(pg->pg_MaxPrefixLen) , 1, fh);

  /* fix partition loading routine for standard suffix tree */
  if(pg->pg_UseStdSfxTree)
  {
    pg->pg_PartitionCache->ch_LoadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CacheStdSuffixPartitionLoad;
    pg->pg_PartitionCache->ch_UnloadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CacheStdSuffixPartitionUnload;
  }

  GB_begin_transaction(pg->pg_MainDB);

  /* get the ecoli reference sequence */
  LoadEcoliSequence(pg);

  /* add the species to the list */
  pg->pg_SpeciesMap = (struct PTPanSpecies **) calloc(sizeof(struct PTPanSpecies *),
                        pg->pg_NumSpecies);
  ignorecount = 0;
  numspec = 0;
  gb_species = GBT_first_species_rel_species_data(pg->pg_SpeciesData);
  while(numspec < pg->pg_NumSpecies)
  {
    GBDATA *gb_name;
    GBDATA *gb_ali;
    GBDATA *gb_data;
    STRPTR spname;
    STRPTR filespname;
    STRPTR fullname;
    UWORD len;
    BOOL obsolete;

    obsolete = FALSE;
    fullname = NULL;

    /* get name of species on disk */
    fread(&len, sizeof(len), 1, fh);
    filespname = (STRPTR) calloc(len+1, 1);
    fread(filespname, len, 1, fh);

    /* try to find species by name, if NULL */
    if(!gb_species)
    {
      gb_species = GBT_find_species_rel_species_data(pg->pg_SpeciesData, filespname);
      if(!gb_species)
      {
    printf("Couldn't find indexed species %s in DB!\n", filespname);
    obsolete = TRUE;
      }
    }
    if(gb_species)
    {
      /* get name */
      gb_name = GB_find(gb_species, "name", down_level);
      if(!gb_name)
      {
    spname = NULL;
      } else {
    spname = GB_read_string(gb_name);
      }
      /* check, if name matches */
      if((!spname) || strcmp(spname, filespname))
      {
    /* oh, some mess in the species, try to find it manually */
    gb_species = GBT_find_species_rel_species_data(pg->pg_SpeciesData, filespname);
    if(!gb_species)
    {
    printf("Couldn't find indexed species %s in DB!\n", filespname);
    obsolete = TRUE;
    }
      }
      free(spname);
    } else {
      obsolete = TRUE;
    }
    gb_data = NULL;

    if(!obsolete)
    {
      /* get alignments */
      gb_ali = GB_find(gb_species, pg->pg_AlignmentName, down_level);
      if(gb_ali)
      {
    gb_data = GB_find(gb_ali, "data", down_level);
      }
      gb_name = GB_find(gb_species, "full_name", down_level);
      if(gb_name)
      {
    fullname = GB_read_string(gb_name);
      } else {
    fullname = strdup(filespname);
      }
    }
    /* okay, cannot fail now anymore, allocate a PTPanSpecies structure */
    ps = (struct PTPanSpecies *) calloc(1, sizeof(struct PTPanSpecies));
    pg->pg_SpeciesMap[numspec] = ps;
    ps->ps_Num = numspec + ignorecount;

    /* write name and long name into the structure */
    ps->ps_SpeciesDB = gb_species;
    ps->ps_SeqDataDB = gb_data;
    ps->ps_IsGroup = FALSE;
    ps->ps_Obsolete = obsolete;
    ps->ps_Name = filespname;
    ps->ps_FullName = fullname;

    /* load in the alignment information */
    fread(&ps->ps_SeqDataSize, sizeof(ps->ps_SeqDataSize), 1, fh);
    fread(&ps->ps_RawDataSize, sizeof(ps->ps_RawDataSize), 1, fh);
    fread(&ps->ps_AbsOffset, sizeof(ps->ps_AbsOffset), 1, fh);
    fread(&ps->ps_SeqHash, sizeof(ps->ps_SeqHash), 1, fh);
    fread(&ps->ps_ChkPntIVal, sizeof(ps->ps_ChkPntIVal), 1, fh);
    fread(&ps->ps_NumCheckPoints, sizeof(ps->ps_NumCheckPoints), 1, fh);
    ps->ps_CheckPoints = (ULONG *) calloc(ps->ps_NumCheckPoints, sizeof(ULONG));
    if(!ps->ps_CheckPoints)
    {
      printf("Out of memory allocating checkpoint buffer!\n");
    }
    fread(ps->ps_CheckPoints, sizeof(ULONG), ps->ps_NumCheckPoints, fh);
    ps->ps_Node.ln_Pri = ps->ps_AbsOffset;

    /* Init complete, now add it to the list */
    //printf("Added %s ('%s')...\n", ps->ps_Name, ps->ps_FullName);
    AddTail(&pg->pg_Species, &ps->ps_Node);
    numspec++;

    /* visual feedback */
    /*if((numspec % 10) == 0)
    {
      if(numspec % 500)
      {
    printf(".");
    fflush(stdout);
      } else {
    printf(".%6ld (%6ld KB)\n", numspec, (ps->ps_AbsOffset>>10));
      }
    }*/
    gb_species = GBT_next_species(gb_species);
  }

  if(numspec != pg->pg_NumSpecies)
  {
    printf("ERROR: Number of species has changed!\n");
    fclose(fh);
    GB_commit_transaction(pg->pg_MainDB);
    return(FALSE);
  }

  /* build tree to find species quicker by raw position */
  pg->pg_SpeciesBinTree = BuildBinTree(&pg->pg_Species);

  /* build a species name hash to mark species in groups */
  pg->pg_SpeciesNameHash = GBS_create_hash(SPECIESNAMEHASHSIZE, GB_IGNORE_CASE);    // TODO: JB: check if GB_IGNORE_CASE
                                                                                    //           is right (was 0)
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    GBS_write_hash(pg->pg_SpeciesNameHash, ps->ps_Name, ps->ps_Num + 1);
    ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
  }

  printf("\n\nDatabase contains %ld valid species (%ld ignored).\n"
    "%ld bytes alignment data (%ld bases).\n",
    pg->pg_NumSpecies, ignorecount, pg->pg_TotalSeqSize, pg->pg_TotalRawSize);

  printf("Number of partitions: %d\n", pg->pg_NumPartitions);

  for(cnt = 0; cnt < pg->pg_NumPartitions; cnt++)
  {
    ULONG pcnt;

    pp = (struct PTPanPartition *) calloc(1, sizeof(struct PTPanPartition));
    if(!pp)
    {
      fclose(fh);
      GB_commit_transaction(pg->pg_MainDB);
      return(FALSE); /* out of memory */
    }
    pp->pp_PTPanGlobal = pg;
    fread(&pp->pp_ID, sizeof(pp->pp_ID), 1, fh);
    fread(&pp->pp_Prefix, sizeof(pp->pp_Prefix), 1, fh);
    fread(&pp->pp_PrefixLen, sizeof(pp->pp_PrefixLen), 1, fh);
    fread(&pp->pp_Size, sizeof(pp->pp_Size), 1, fh);
    fread(&pp->pp_RawOffset, sizeof(pp->pp_RawOffset), 1, fh);

    pp->pp_PartitionName = (STRPTR) calloc(strlen(pg->pg_IndexName) + 5, 1);
    if(pg->pg_UseStdSfxTree)
    {
      strncpy(pp->pp_PartitionName, pg->pg_IndexName, strlen(pg->pg_IndexName) - 3);
      sprintf(&pp->pp_PartitionName[strlen(pg->pg_IndexName) - 3], "sfx",
      pp->pp_ID);
    } else {
      strncpy(pp->pp_PartitionName, pg->pg_IndexName, strlen(pg->pg_IndexName) - 2);
      sprintf(&pp->pp_PartitionName[strlen(pg->pg_IndexName) - 2], "t%03ld",
      pp->pp_ID);
    }
    /* generate partition string */
    pcnt = pp->pp_PrefixLen;
    pp->pp_PrefixSeq = (STRPTR) malloc(pcnt + 1);
    while(pcnt)
    {
      pp->pp_PrefixSeq[pp->pp_PrefixLen - pcnt] = pg->pg_DecompressTable[(pp->pp_Prefix /
                    pg->pg_PowerTable[pcnt - 1]) % pg->pg_AlphaSize];
      pcnt--;
    }
    pp->pp_PrefixSeq[pp->pp_PrefixLen] = 0;
    //printf("Part %ld (%ld = '%s')\n", pp->pp_ID, pp->pp_Prefix, pp->pp_PrefixSeq);
    /* partition ready, add it */
    AddTail(&pg->pg_Partitions, &pp->pp_Node);
  }
  fclose(fh);

  /* done! */
  GB_commit_transaction(pg->pg_MainDB);
  return(TRUE);
}
/* \\\ */

/* /// "LoadAllPartitions()" */
BOOL LoadAllPartitions(struct PTPanGlobal *pg)
{
  struct PTPanPartition *pp;

  /* load in each partition */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    if(!(pp->pp_CacheNode = CacheLoadData(pg->pg_PartitionCache, pp->pp_CacheNode, pp)))
    {
      return(FALSE);
    }
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }
  return(TRUE);
}
/* \\\ */

/* /// "FreeAllPartitions()" */
void FreeAllPartitions(struct PTPanGlobal *pg)
{
  struct PTPanPartition *pp;
  FlushCache(pg->pg_PartitionCache);
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    FreeCacheNode(pg->pg_PartitionCache, pp->pp_CacheNode);
    Remove(&pp->pp_Node);
    free(pp->pp_PrefixSeq);
    FreeHuffmanTree(pp->pp_BranchTree);
    FreeHuffmanTree(pp->pp_ShortEdgeTree);
    FreeHuffmanTree(pp->pp_LongEdgeLenTree);
    free(pp);
    pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  }
  pg->pg_NumPartitions = 0;
}
/* \\\ */

