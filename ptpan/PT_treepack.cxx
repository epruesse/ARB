#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <sys/mman.h>

/* /// "WriteIndexHeader()" */
BOOL WriteIndexHeader(struct PTPanGlobal *pg)
{
  struct PTPanSpecies *ps;
  struct PTPanPartition *pp;
  FILE *fh = pg->pg_IndexFile;
  ULONG endian = 0x01020304;
  UWORD version = FILESTRUCTVERSION;

  /* write 16 bytes of ID */
  fputs("TUM PeTerPAN IDX", fh);
  /* write a specific code word to allow detection of correct endianess */
  fwrite(&endian, sizeof(endian), 1, fh);
  /* write file version */
  fwrite(&version, sizeof(version), 1, fh);
  /* write some global data */
  fwrite(&pg->pg_UseStdSfxTree, sizeof(pg->pg_UseStdSfxTree), 1, fh);
  fwrite(&pg->pg_AlphaSize    , sizeof(pg->pg_AlphaSize)    , 1, fh);
  fwrite(&pg->pg_TotalSeqSize , sizeof(pg->pg_TotalSeqSize) , 1, fh);
  fwrite(&pg->pg_TotalSeqCompressedSize, sizeof(pg->pg_TotalSeqCompressedSize) , 1, fh);
  fwrite(&pg->pg_TotalRawSize , sizeof(pg->pg_TotalRawSize) , 1, fh);
  fwrite(&pg->pg_TotalRawBits , sizeof(pg->pg_TotalRawBits) , 1, fh);
  fwrite(&pg->pg_AllHashSum   , sizeof(pg->pg_AllHashSum)   , 1, fh);
  fwrite(&pg->pg_NumSpecies   , sizeof(pg->pg_NumSpecies)   , 1, fh);
  fwrite(&pg->pg_NumPartitions, sizeof(pg->pg_NumPartitions), 1, fh);
  fwrite(&pg->pg_MaxPrefixLen , sizeof(pg->pg_MaxPrefixLen) , 1, fh);

  // write Ecoli Sequence
  fwrite(&pg->pg_EcoliSeqSize  , sizeof(pg->pg_EcoliSeqSize) , 1        , fh);
  if (pg->pg_EcoliSeqSize > 0)
  {                                                                                 // only write EcoliSeq and
    fwrite(pg->pg_EcoliSeq       , 1            , pg->pg_EcoliSeqSize + 1 , fh);    // EcoliBaseTable if we
    fwrite(pg->pg_EcoliBaseTable , sizeof(ULONG), pg->pg_EcoliSeqSize + 1 , fh);    // found them earlier...
  } 

  /* write species info */
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    /* write names */
    UWORD len;
    len = strlen(ps->ps_Name);
    fwrite(&len, sizeof(len), 1, fh);
    fputs(ps->ps_Name, fh);

    len = strlen(ps->ps_FullName);
    fwrite(&len, sizeof(len), 1, fh);
    fputs(ps->ps_FullName, fh);

    /* write some more relevant data */
    fwrite(&ps->ps_SeqDataSize, sizeof(ps->ps_SeqDataSize), 1, fh);
    fwrite(&ps->ps_RawDataSize, sizeof(ps->ps_RawDataSize), 1, fh);
    fwrite(&ps->ps_AbsOffset, sizeof(ps->ps_AbsOffset), 1, fh);
    fwrite(&ps->ps_SeqHash, sizeof(ps->ps_SeqHash), 1, fh);
    fwrite(&ps->ps_SeqDataCompressedSize, sizeof(ps->ps_SeqDataCompressedSize), 1, fh);     // save compressed Seq Data
    fwrite(ps->ps_SeqDataCompressed, 1, ((ps->ps_SeqDataCompressedSize >> 3) + 1), fh);     // .
    free(ps->ps_SeqDataCompressed);                                                         // and free the memory
    ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
  }

  /* write partition info */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    fwrite(&pp->pp_ID, sizeof(pp->pp_ID), 1, fh);
    fwrite(&pp->pp_Prefix, sizeof(pp->pp_Prefix), 1, fh);
    fwrite(&pp->pp_PrefixLen, sizeof(pp->pp_PrefixLen), 1, fh);
    fwrite(&pp->pp_Size, sizeof(pp->pp_Size), 1, fh);
    fwrite(&pp->pp_RawOffset, sizeof(pp->pp_RawOffset), 1, fh);
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }
  return(TRUE);
}
/* \\\ */

/* /// "WriteTreeHeader()" */
BOOL WriteTreeHeader(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  FILE *fh = pp->pp_PartitionFile;

  /* write 16 bytes of ID */
  fputs("TUM PeTerPAN P3I", fh);
  /* checksum to verify */
  fwrite(&pg->pg_AllHashSum     , sizeof(pg->pg_AllHashSum)     , 1, fh);
  /* write partition data */
  fwrite(&pp->pp_ID             , sizeof(pp->pp_ID)             , 1, fh);
  fwrite(&pp->pp_Prefix         , sizeof(pp->pp_Prefix)         , 1, fh);
  fwrite(&pp->pp_PrefixLen      , sizeof(pp->pp_PrefixLen)      , 1, fh);
  fwrite(&pp->pp_Size           , sizeof(pp->pp_Size)           , 1, fh);
  fwrite(&pp->pp_RawOffset      , sizeof(pp->pp_RawOffset)      , 1, fh);
  fwrite(&pp->pp_TreePruneDepth , sizeof(pp->pp_TreePruneDepth) , 1, fh);
  fwrite(&pp->pp_TreePruneLength, sizeof(pp->pp_TreePruneLength), 1, fh);
  fwrite(&pp->pp_LongDictSize   , sizeof(pp->pp_LongDictSize)   , 1, fh);
  fwrite(&pp->pp_LongRelPtrBits , sizeof(pp->pp_LongRelPtrBits) , 1, fh);

  /* branch code */
  WriteHuffmanTree(pp->pp_BranchCode, 1UL << pg->pg_AlphaSize, fh);

  /* short edge code */
  WriteHuffmanTree(pp->pp_ShortEdgeCode, 1UL << (pg->pg_BitsUseTable[SHORTEDGEMAX]+1), fh);

  /* long edge len code */
  WriteHuffmanTree(pp->pp_LongEdgeLenCode, pp->pp_LongEdgeLenSize, fh);

  /* long dictionary */
  pp->pp_LongDictRawSize = ((pp->pp_LongDictSize / MAXCODEFITLONG) + 1) * sizeof(ULONG);
  fwrite(&pp->pp_LongDictRawSize, sizeof(pp->pp_LongDictRawSize), 1, fh);
  fwrite(pp->pp_LongDictRaw, pp->pp_LongDictRawSize, 1, fh);

  /* write tree length */
  fwrite(&pp->pp_DiskTreeSize, sizeof(pp->pp_DiskTreeSize)  , 1, fh);

  return(TRUE);
}
/* \\\ */

/* /// "CachePartitionLoad()" */
BOOL CachePartitionLoad(struct CacheHandler *, struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  FILE *fh;
  char idstr[16];
  ULONG hashsum;
  ULONG len;

  if(!(fh = fopen(pp->pp_PartitionName, "r")))
  {
    printf("Couldn't open partition file %s\n", pp->pp_PartitionName);
    return(FALSE);
  }

  printf("Loading partition file %s... ", pp->pp_PartitionName);

  /* read id string */
  fread(idstr, 16, 1, fh);
  if(strncmp("TUM PeTerPAN P3I", idstr, 16))
  {
    printf("ERROR: This is no partition file!\n");
    fclose(fh);
    return(FALSE);
  }

  fread(&hashsum, sizeof(pg->pg_AllHashSum), 1, fh);
  if(hashsum != pg->pg_AllHashSum)
  {
    printf("ERROR: Partition file does not match index file!\n");
    fclose(fh);
    return(FALSE);
  }

  /* read partition data */
  fread(&pp->pp_ID             , sizeof(pp->pp_ID)             , 1, fh);
  fread(&pp->pp_Prefix         , sizeof(pp->pp_Prefix)         , 1, fh);
  fread(&pp->pp_PrefixLen      , sizeof(pp->pp_PrefixLen)      , 1, fh);
  fread(&pp->pp_Size           , sizeof(pp->pp_Size)           , 1, fh);
  fread(&pp->pp_RawOffset      , sizeof(pp->pp_RawOffset)      , 1, fh);
  fread(&pp->pp_TreePruneDepth , sizeof(pp->pp_TreePruneDepth) , 1, fh);
  fread(&pp->pp_TreePruneLength, sizeof(pp->pp_TreePruneLength), 1, fh);
  fread(&pp->pp_LongDictSize   , sizeof(pp->pp_LongDictSize)   , 1, fh);
  fread(&pp->pp_LongRelPtrBits , sizeof(pp->pp_LongRelPtrBits) , 1, fh);

  /* read huffman tables */
  pp->pp_BranchTree = ReadHuffmanTree(fh);
  pp->pp_ShortEdgeTree = ReadHuffmanTree(fh);
  pp->pp_LongEdgeLenTree = ReadHuffmanTree(fh);

  /* read compressed dictionary */
  fread(&len, sizeof(len), 1, fh);
  pp->pp_LongDictRaw = (ULONG *) malloc(len);
  if(pp->pp_LongDictRaw)
  {
    fread(pp->pp_LongDictRaw, len, 1, fh);

    /* read compressed tree */
    fread(&pp->pp_DiskTreeSize, sizeof(pp->pp_DiskTreeSize), 1, fh);

    /* if we're low on memory, use virtual memory instead */
    if(pg->pg_LowMemoryMode)
    {
      LONG pos;
      pos = ftell(fh);
      /* map file to virtual memory */
      pp->pp_MapFileSize = pos + pp->pp_DiskTreeSize;
      pp->pp_MapFileBuffer = (UBYTE *) mmap(0, pp->pp_MapFileSize,
          PROT_READ, MAP_SHARED, fileno(fh), 0);
      if(pp->pp_MapFileBuffer)
      {
        fclose(fh);
           /* calculate start of buffer inside file */
        pp->pp_DiskTree = &pp->pp_MapFileBuffer[pos];
        printf("VMEM!\n");
        return(TRUE);
      } else {
        printf("Unable to map file to ram!\n");
      }
    } else {
      pp->pp_MapFileBuffer = NULL;
      pp->pp_DiskTree = (UBYTE *) malloc(pp->pp_DiskTreeSize);
      if(pp->pp_DiskTree)
      {
        fread(pp->pp_DiskTree, pp->pp_DiskTreeSize, 1, fh);
        fclose(fh);
        printf("DONE!\n");
        return(TRUE);
      } else {
        printf("Out of memory while loading tree (%ld)!\n", pp->pp_DiskTreeSize);
      }
    }
    free(pp->pp_LongDictRaw);
    pp->pp_LongDictRaw = NULL;
  } else {
    printf("Out of memory while loading long dictionary (%ld)!\n", len);
  }
  fclose(fh);
  FreeHuffmanTree(pp->pp_BranchTree);
  FreeHuffmanTree(pp->pp_ShortEdgeTree);
  FreeHuffmanTree(pp->pp_LongEdgeLenTree);
  pp->pp_BranchTree = NULL;
  pp->pp_ShortEdgeTree = NULL;
  pp->pp_LongEdgeLenTree = NULL;
  return(FALSE);
}
/* \\\ */

/* /// "CachePartitionUnload()" */
void CachePartitionUnload(struct CacheHandler *, struct PTPanPartition *pp)
{
  /* free partition memory */
  printf("Unloading %s\n", pp->pp_PartitionName);
  FreeHuffmanTree(pp->pp_BranchTree);
  FreeHuffmanTree(pp->pp_ShortEdgeTree);
  FreeHuffmanTree(pp->pp_LongEdgeLenTree);
  free(pp->pp_LongDictRaw);
  if(pp->pp_MapFileBuffer)
  {
    munmap(pp->pp_MapFileBuffer, pp->pp_MapFileSize);
    pp->pp_MapFileBuffer = NULL;
    pp->pp_DiskTree = NULL;
  } else {
    free(pp->pp_DiskTree);
  }
  pp->pp_BranchTree = NULL;
  pp->pp_ShortEdgeTree = NULL;
  pp->pp_LongEdgeLenTree = NULL;
  pp->pp_LongDictRaw = NULL;
  pp->pp_DiskTree = NULL;
}
/* \\\ */

/* /// "CachePartitionSize()" */
ULONG CachePartitionSize(struct CacheHandler *, struct PTPanPartition *pp)
{
  //struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct stat ppstat;

  /* determine filesize */
  ppstat.st_size = 0;
  stat(pp->pp_PartitionName, &ppstat);

  /* and return it */
  return((ULONG) ppstat.st_size);
}
/* \\\ */

/* /// "WriteStdSuffixTreeHeader()" */
BOOL WriteStdSuffixTreeHeader(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  FILE *fh = pp->pp_PartitionFile;

  /* write 16 bytes of ID */
  fputs("TUM StdSfxTree I", fh);
  /* checksum to verify */
  fwrite(&pg->pg_AllHashSum     , sizeof(pg->pg_AllHashSum)     , 1, fh);
  /* write partition data */
  fwrite(&pp->pp_ID             , sizeof(pp->pp_ID)             , 1, fh);
  fwrite(&pp->pp_Prefix         , sizeof(pp->pp_Prefix)         , 1, fh);
  fwrite(&pp->pp_PrefixLen      , sizeof(pp->pp_PrefixLen)      , 1, fh);
  fwrite(&pp->pp_Size           , sizeof(pp->pp_Size)           , 1, fh);
  fwrite(&pp->pp_RawOffset      , sizeof(pp->pp_RawOffset)      , 1, fh);

  /* write tree length */
  fwrite(&pp->pp_DiskTreeSize   , sizeof(pp->pp_DiskTreeSize)   , 1, fh);

  return(TRUE);
}
/* \\\ */

/* /// "CacheStdSuffixPartitionLoad()" */
BOOL CacheStdSuffixPartitionLoad(struct CacheHandler *, struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  FILE *fh;
  char idstr[16];
  ULONG hashsum;
  ULONG len;

  if(!(fh = fopen(pp->pp_PartitionName, "r")))
  {
    printf("Couldn't open suffix tree file %s\n", pp->pp_PartitionName);
    return(FALSE);
  }

  printf("Loading suffix tree file %s... ", pp->pp_PartitionName);

  /* read id string */
  fread(idstr, 16, 1, fh);
  if(strncmp("TUM StdSfxTree I", idstr, 16))
  {
    printf("ERROR: This is no partition file!\n");
    fclose(fh);
    return(FALSE);
  }

  fread(&hashsum, sizeof(pg->pg_AllHashSum), 1, fh);
  if(hashsum != pg->pg_AllHashSum)
  {
    printf("ERROR: Partition file does not match index file!\n");
    fclose(fh);
    return(FALSE);
  }

  /* read partition data */
  fread(&pp->pp_ID             , sizeof(pp->pp_ID)             , 1, fh);
  fread(&pp->pp_Prefix         , sizeof(pp->pp_Prefix)         , 1, fh);
  fread(&pp->pp_PrefixLen      , sizeof(pp->pp_PrefixLen)      , 1, fh);
  fread(&pp->pp_Size           , sizeof(pp->pp_Size)           , 1, fh);
  fread(&pp->pp_RawOffset      , sizeof(pp->pp_RawOffset)      , 1, fh);
  fread(&pp->pp_DiskTreeSize   , sizeof(pp->pp_DiskTreeSize)   , 1, fh);
  if(pg->pg_LowMemoryMode)
  {
    LONG pos;
    pos = ftell(fh);
    /* map file to virtual memory */
    pp->pp_MapFileSize = pos + pp->pp_DiskTreeSize;
    pp->pp_MapFileBuffer = (UBYTE *) mmap(0, pp->pp_MapFileSize,
                                     PROT_READ, MAP_SHARED, fileno(fh), 0);
    if(pp->pp_MapFileBuffer)
    {
      fclose(fh);
      /* calculate start of buffer inside file */
      pp->pp_DiskTree = &pp->pp_MapFileBuffer[pos];
      printf("VMEM!\n");
      return(TRUE);
    } else {
      printf("Unable to map file to ram!\n");
    }
  } else {
    pp->pp_MapFileBuffer = NULL;
    pp->pp_DiskTree = (UBYTE *) malloc(pp->pp_DiskTreeSize);
    if(pp->pp_DiskTree)
    {
      fread(pp->pp_DiskTree, pp->pp_DiskTreeSize, 1, fh);
      fclose(fh);
      printf("DONE!\n");
      return(TRUE);
    } else {
      printf("Out of memory while loading tree (%ld)!\n", pp->pp_DiskTreeSize);
    }
  }
  fclose(fh);
  return(FALSE);
}
/* \\\ */

/* /// "CacheStdSuffixPartitionUnload()" */
void CacheStdSuffixPartitionUnload(struct CacheHandler *, struct PTPanPartition *pp)
{
  /* free partition memory */
  printf("Unloading %s\n", pp->pp_PartitionName);
  if(pp->pp_MapFileBuffer)
  {
    munmap(pp->pp_MapFileBuffer, pp->pp_MapFileSize);
    pp->pp_MapFileBuffer = NULL;
    pp->pp_DiskTree = NULL;
  } else {
    free(pp->pp_DiskTree);
  }
  pp->pp_DiskTree = NULL;
}
/* \\\ */

/* /// "FixRelativePointersRec()" */
ULONG FixRelativePointersRec(struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen)
{
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  ULONG nodesize = 0;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;

  level++;
  elen += sfxnode->sn_EdgeLen;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(!(childptr >> LEAFBIT))
    {
      /* this is a normal node pointer, recurse */
      /* enter absolute position */
      childptr &= RELOFFSETMASK;
      childptr <<= 2;

      /* check for maximum depth reached */
      if((level >= pp->pp_TreePruneDepth) || (elen >= pp->pp_TreePruneLength))
      {
        struct SfxNode *childnode = (struct SfxNode *) &pp->pp_SfxNodes[childptr];
//printf("L%08lx ", childptr);
nodesize = CalcPackedLeafSize(pp, childptr);
/* add leaf node to traversal path */
               childnode->sn_Parent &= ~RELOFFSETMASK;
               childnode->sn_Parent |= pp->pp_TraverseTreeRoot;
               pp->pp_TraverseTreeRoot = childptr >> 2;
               childnode->sn_AlphaMask = 0; /* indicate stop-leaf */
      } else {
        nodesize = FixRelativePointersRec(pp, childptr, level, elen);
      }
      pp->pp_DiskTreeSize += nodesize;
      sfxnode->sn_Children[childidx] = (sfxnode->sn_Children[childidx] & ~RELOFFSETMASK) |
              pp->pp_DiskTreeSize;
    }
  }
  /* now convert absolute pointers to relative ones */
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(!(childptr >> LEAFBIT))
    {
      /* fix relative pointer */
      childptr &= RELOFFSETMASK;
      childptr = pp->pp_DiskTreeSize - childptr;
      sfxnode->sn_Children[childidx] = (sfxnode->sn_Children[childidx] & ~RELOFFSETMASK) | childptr;
    }
  }
  nodesize = CalcPackedNodeSize(pp, pos);

  //printf("N%08lx ", pos);
  /* fix travel route, as we lost our children pointers */
  sfxnode->sn_Parent &= ~RELOFFSETMASK;
  sfxnode->sn_Parent |= pp->pp_TraverseTreeRoot;
  pp->pp_TraverseTreeRoot = pos >> 2;
  return(nodesize);
}
/* \\\ */

/* /// "ULONGCompare()" */
/* compare function for offset sorting */
LONG ULONGCompare(const ULONG *node1, const ULONG *node2)
{
  return((LONG) *node1 - (LONG) *node2);
}
/* \\\ */

/* /// "CalcPackedNodeSize()" */
ULONG CalcPackedNodeSize(struct PTPanPartition *pp, ULONG pos)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  ULONG bitscnt = 0;
  ULONG val;
  ULONG cnt;
  ULONG newbase;

#if 0 // debugging
  bitscnt = 8;
#endif

  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];

  /* huffman code for short / long edge */
  if(sfxnode->sn_StartPos & (1UL << 31))
  {
    /* long edge */
    /*printf("LE[%02d+%02d+%02d] ",
          pp->pp_ShortEdgeCode[0].hc_CodeLength,
          pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength,
          pp->pp_LongRelPtrBits);*/
#if 0 // debug
    if(sfxnode->sn_EdgeLen >= pp->pp_LongEdgeLenSize)
    {
      printf("LongEdgeLen: code %d out of range!\n", sfxnode->sn_EdgeLen);
    }
    if(!pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength)
    {
      printf("LongEdgeLen: no code for %d!\n", sfxnode->sn_EdgeLen);
    }
#endif
    bitscnt += pp->pp_ShortEdgeCode[0].hc_CodeLength;
    bitscnt += pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength;
    bitscnt += pp->pp_LongRelPtrBits;
  }
  else if(sfxnode->sn_StartPos & (1UL << 30))
  {
    /* short edge */
    /*printf("SE[      %02d] ",
      pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_CodeLength);*/
#if 0 // debug
    if((sfxnode->sn_StartPos & RELOFFSETMASK) >= (1UL << (pg->pg_BitsUseTable[SHORTEDGEMAX]+1)))
    {
      printf("ShortEdge: code %ld out of range!\n", sfxnode->sn_StartPos & RELOFFSETMASK);
    }
    if(!pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_CodeLength)
    {
      printf("ShortEdge: no code for %ld!\n", sfxnode->sn_StartPos & RELOFFSETMASK);
    }
#endif
    bitscnt += pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_CodeLength;
  } else {
    if(pos == 0)
    {
      /* special root handling */
      bitscnt += pp->pp_ShortEdgeCode[1].hc_CodeLength;
    } else {
      printf("Arrrrgghhh?!?\n");
      exit(1);
    }
  }

  /* branch code */
  //printf("BC[%02d] ", pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_CodeLength);
#if 0 // debug
  if(sfxnode->sn_AlphaMask >= (1UL << pg->pg_AlphaSize))
  {
    printf("Branch: code %d out of range!\n", sfxnode->sn_AlphaMask);
  }
  if(!pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_CodeLength)
  {
    printf("Branch: no code for %d!\n", sfxnode->sn_AlphaMask);
  }
#endif
  bitscnt += pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_CodeLength;

  /* child pointers */
  /* msb = {0}   -> rel is next
     msb = {10}  ->  6 bit rel node ptr
     msb = {1100} -> 11 bit rel node ptr
     msb = {1101} -> 27 bit rel node ptr (large enough for a jump over 128 MB)
     msb = {111} -> nn bit abs leaf ptr (nn = pg->pg_TotalRawBits)
  */
  cnt = sfxnode->sn_Parent >> RELOFFSETBITS;
#if 0 /* debug */
  if(cnt > pg->pg_AlphaSize)
  {
    printf("too many children %ld!\n", cnt);
  }
#endif
  childidx = 0;
  newbase = 0;
  while(childidx < cnt)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer and doesn't contain a seqcode */
      //printf("LF[%02d] ", 2+pg->pg_TotalRawBits);
      bitscnt += 3+pg->pg_TotalRawBits;
    } else {
      /* this is a normal node pointer, recurse */
      val = childptr & RELOFFSETMASK;
      val -= newbase;
      newbase += val;
      //printf("Dist: %6ld\n", val);
      if(val == 0)
      {
        /* next */
        bitscnt++;
      } else {
        if(val < (1UL << 6))
        {
          //printf("NP[8] ");
          bitscnt += 2+6;
        } else {
          if(val < (1UL << 11))
          {
            //printf("NP[15] ");
            bitscnt += 4+11;
          } else {
            //printf("NP[31] ");
            bitscnt += 4+27;
          }
        }
      }
    }
    childidx++;
  }
  //printf("P[%08lx] %ld\n", pos, bitscnt);
  return((bitscnt + 7) >> 3);
}
/* \\\ */

/* /// "CalcPackedLeafSize()" */
ULONG CalcPackedLeafSize(struct PTPanPartition *pp, ULONG pos)
{
  struct SfxNode *sfxnode;
  ULONG leafcnt;
  ULONG cnt;
  LONG oldval;
  LONG val;
  ULONG leafsize = 1;
  ULONG sdleafsize = 1;

#if 0
  static BOOL example = TRUE;
#endif

  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];

  /* how many leaves do we have? */
  leafcnt = GetTreeStatsLeafCountRec(pp, pos);
  pp->pp_DiskOuterLeaves += leafcnt;
  //printf("%ld leaves\n", leafcnt);
  /* enlarge memory, if too small */
  if(pp->pp_LeafBufferSize < leafcnt)
  {
    pp->pp_LeafBuffer = (LONG *) realloc(pp->pp_LeafBuffer, leafcnt * sizeof(LONG));
    pp->pp_LeafBufferSize = leafcnt;
  }
  pp->pp_LeafBufferPtr = pp->pp_LeafBuffer;
  /* collect leafs */
  GetTreeStatsLeafCollectRec(pp, pos);

  if(leafcnt > 1)
  {
    qsort(pp->pp_LeafBuffer, leafcnt, sizeof(ULONG),
          (int (*)(const void *, const void *)) ULONGCompare);
  }

#if 0 /* debug */
  if((leafcnt > 20) && example)
  {
    printf("Original:\n");
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
      printf("%ld %ld\n", cnt, pp->pp_LeafBuffer[cnt]);
    }
  }
#endif

  /* do delta compression */
  oldval = pp->pp_LeafBuffer[0];
  for(cnt = 1; cnt < leafcnt; cnt++)
  {
    pp->pp_LeafBuffer[cnt] -= oldval;
    oldval += pp->pp_LeafBuffer[cnt];
    //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
  }
#if 0 /* debug */
  if((leafcnt > 20) && example)
  {
    printf("Delta 1:\n");
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
      printf("%ld %ld\n", cnt, pp->pp_LeafBuffer[cnt]);
    }
  }
#endif

#ifdef DOUBLEDELTALEAVES
#ifdef DOUBLEDELTAOPTIONAL
  for(cnt = 0; cnt < leafcnt; cnt++)
  {
    val = pp->pp_LeafBuffer[cnt];
    if((val < -63) || (val > 63))
    {
      if((val < -8192) || (val > 8191))
      {
        if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
        {
          if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
          {
            /* five bytes */
            sdleafsize += 5;
          } else {
            /* four bytes */
            sdleafsize += 4;
          }
        } else {
          /* three bytes */
          sdleafsize += 3;
        }
      } else {
        /* two bytes */
        sdleafsize += 2;
      }
    } else {
      /* one byte */
      sdleafsize++;
    }
  }
  *pp->pp_LeafBuffer = -(*pp->pp_LeafBuffer);
  (*pp->pp_LeafBuffer)--;
#endif
  /* do delta compression a second time */
  if(leafcnt > 2)
  {
    oldval = pp->pp_LeafBuffer[1];
    for(cnt = 2; cnt < leafcnt; cnt++)
    {
      pp->pp_LeafBuffer[cnt] -= oldval;
      oldval += pp->pp_LeafBuffer[cnt];
      //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
    }
  }
#if 0 /* debug */
  if((leafcnt > 20) && example)
  {
    printf("Delta 2:\n");
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
      printf("%ld %ld\n", cnt, pp->pp_LeafBuffer[cnt]);
    }
    example = FALSE;
  }
#endif

#endif
  /* compression:
       msb == {1}   -> 1 byte offset (-   63 -     63)
       msb == {01}  -> 2 byte offset (- 8192 -   8191)
       msb == {001} -> 3 byte offset (- 2^20 - 2^20-1)
       msb == {000} -> 4 byte offset (- 2^28 - 2^28-1)
     special opcodes:
       0xff      -> end of array
     This means the upper limit is currently 1 GB for raw sequence data
     (this can be changed though: Just introduce another opcode)
  */

  for(cnt = 0; cnt < leafcnt; cnt++)
  {
    val = pp->pp_LeafBuffer[cnt];
    if((val < -63) || (val > 63))
    {
      if((val < -8192) || (val > 8191))
      {
        if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
        {
          if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
          {
            /* five bytes */
            leafsize += 5;
          } else {
            /* four bytes */
            leafsize += 4;
          }
        } else {
          /* three bytes */
          leafsize += 3;
        }
      } else {
        /* two bytes */
        leafsize += 2;
      }
    } else {
      /* one byte */
      leafsize++;
    }
  }
#ifdef DOUBLEDELTAOPTIONAL
  return((leafsize < sdleafsize) ? leafsize : sdleafsize);
#else
  return(leafsize);
#endif
}
/* \\\ */

/* /// "DebugTreeNode()" */
void DebugTreeNode(struct TreeNode *tn)
{
  struct PTPanPartition *pp = tn->tn_PTPanPartition;
  ULONG cnt;

  printf("*** NODE 0x%08lx\n"
         "Pos: %ld [%ld bytes] (Parent: %ld [0x%08lx])\n"
         "Level: %d (%d bases into tree)\n"
         "Edge: %d (%s)\n"
         "Children: ",
         (ULONG) tn,
         tn->tn_Pos, tn->tn_Size, tn->tn_ParentPos, (ULONG) tn->tn_Parent,
         tn->tn_Level, tn->tn_TreeOffset,
         tn->tn_EdgeLen, tn->tn_Edge);
  for(cnt = 0; cnt < pp->pp_PTPanGlobal->pg_AlphaSize; cnt++)
  {
    printf("[0x%08lx] ", tn->tn_Children[cnt]);
  }
  printf("\nLeaves: %ld\n   ", tn->tn_NumLeaves);
  for(cnt = 0; cnt < tn->tn_NumLeaves; cnt++)
  {
    printf("<%ld> ", tn->tn_Leaves[cnt]);
  }
  printf("\n");
}
/* \\\ */

/* /// "GetTreePath()" */
void GetTreePath(struct TreeNode *tn, STRPTR strptr, ULONG len)
{
  strptr[len] = 0; /* NULL termination of the string */
  if(!len)
  {
    return;
  }
  /* if the edge is too short, fill with question marks */
  while(len > tn->tn_TreeOffset)
  {
    len--;
    strptr[len] = '?';
  }
  while(len)
  {
    while(len + tn->tn_EdgeLen > tn->tn_TreeOffset)
    {
      len--;
      strptr[len] = tn->tn_Edge[len + tn->tn_EdgeLen - tn->tn_TreeOffset];
    }
    tn = tn->tn_Parent;
  }
}
/* \\\ */

/* /// "GoDownStdSuffixNodeChild()" */
struct TreeNode * GoDownStdSuffixNodeChild(struct TreeNode *oldtn, UWORD seqcode)
{
  struct PTPanPartition *pp = oldtn->tn_PTPanPartition;
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;
  struct StdSfxNodeOnDisk *ssn;
  ULONG childoff;

  if(!oldtn->tn_Children[seqcode])
  {
    return(NULL);
  }
  ssn = &((struct StdSfxNodeOnDisk *) pp->pp_DiskTree)[oldtn->tn_Children[seqcode]];
  if(!ssn->ssn_FirstChild)
  {
    /* create a single leaf node */
    tn = (struct TreeNode *) calloc(sizeof(struct TreeNode), 1);
    /* fill in data */
    tn->tn_PTPanPartition = pp;
    tn->tn_NumLeaves = 1;
    tn->tn_Edge = (STRPTR) &pp->pp_StdSfxMapBuffer[ssn->ssn_StartPos];
    tn->tn_EdgeLen = ssn->ssn_EdgeLen;
    tn->tn_Leaves[0] = ssn->ssn_StartPos;
  } else {
    /* create a single leaf node */
    tn = (struct TreeNode *) calloc(sizeof(struct TreeNode), 1);
    tn->tn_PTPanPartition = pp;
    tn->tn_NumLeaves = 0;
    tn->tn_Edge = (STRPTR) &pp->pp_StdSfxMapBuffer[ssn->ssn_StartPos];
    tn->tn_EdgeLen = ssn->ssn_EdgeLen;
    tn->tn_Leaves[0] = ssn->ssn_StartPos;
    /* determine children */
    childoff = ssn->ssn_FirstChild;
    do
    {
      ssn = &((struct StdSfxNodeOnDisk *) pp->pp_DiskTree)[childoff];
      tn->tn_Children[pp->pp_StdSfxMapBuffer[ssn->ssn_StartPos]] = childoff;
      if(!ssn->ssn_NextSibling)
      {
       break;
      }
    }
    while((childoff = ssn->ssn_NextSibling));
  }
  /* fill in downlink information */
  tn->tn_TreeOffset = oldtn->tn_TreeOffset + tn->tn_EdgeLen;
  tn->tn_Level = oldtn->tn_Level + 1;
  tn->tn_ParentSeq = seqcode;
  tn->tn_ParentPos = oldtn->tn_Pos;
  tn->tn_Parent = oldtn;
  return(tn);
}
/* \\\ */

/* /// "GoDownNodeChild()" */
struct TreeNode * GoDownNodeChild(struct TreeNode *oldtn, UWORD seqcode)
{
  struct PTPanPartition *pp = oldtn->tn_PTPanPartition;
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;

  if(!oldtn->tn_Children[seqcode])
  {
    return(NULL);
  }

  if(oldtn->tn_Children[seqcode] & LEAFMASK)
  {
    /* create a single leaf node */
    tn = (struct TreeNode *) calloc(sizeof(struct TreeNode) +
                     sizeof(ULONG) + 1 + 1, 1);
    /* fill in data */
    tn->tn_PTPanPartition = pp;
    tn->tn_NumLeaves = 1;
    tn->tn_Edge = (STRPTR) &tn->tn_Leaves[1];
    tn->tn_EdgeLen = 1;
    tn->tn_Leaves[0] = oldtn->tn_Children[seqcode] & ~LEAFMASK;
  } else {
    if(((ULONG) oldtn->tn_Level + 1 < pp->pp_TreePruneDepth) &&
       ((ULONG) oldtn->tn_TreeOffset + 1 <= pp->pp_TreePruneLength))
    {
      /* just go down normally */
      tn = ReadPackedNode(pp, oldtn->tn_Children[seqcode]);
    } else {
      /* leaf reached */
      tn = ReadPackedLeaf(pp, oldtn->tn_Children[seqcode]);
    }
  }
#if 0 /* debug */
  if(!tn)
  {
    printf("ERROR: Couldn't decrunch %d... going up!\n", seqcode);
    tn = oldtn;
    do
    {
      printf("[%s] ", tn->tn_Edge);
      tn = tn->tn_Parent;
    } while(tn);
    printf("\n");
    return(NULL);
  }
#endif
  /* fill in downlink information */
  tn->tn_TreeOffset = oldtn->tn_TreeOffset + tn->tn_EdgeLen;
  if(tn->tn_TreeOffset > pp->pp_TreePruneLength)
  {
    tn->tn_EdgeLen = pp->pp_TreePruneLength - oldtn->tn_TreeOffset;
  }
  *tn->tn_Edge = pg->pg_DecompressTable[seqcode];
  tn->tn_Level = oldtn->tn_Level + 1;
  tn->tn_ParentSeq = seqcode;
  tn->tn_ParentPos = oldtn->tn_Pos;
  tn->tn_Parent = oldtn;
  return(tn);
}
/* \\\ */

/* /// "GoDownNodeChildNoEdge()" */
struct TreeNode * GoDownNodeChildNoEdge(struct TreeNode *oldtn, UWORD seqcode)
{
  struct PTPanPartition *pp = oldtn->tn_PTPanPartition;
  //struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;

  if(!oldtn->tn_Children[seqcode])
  {
    return(NULL);
  }

  if(oldtn->tn_Children[seqcode] & LEAFMASK)
  {
    /* create a single leaf node */
    tn = (struct TreeNode *) calloc(sizeof(struct TreeNode), 1);
    /* fill in data */
    tn->tn_PTPanPartition = pp;
    tn->tn_NumLeaves = 1;
    tn->tn_EdgeLen = 1;
    tn->tn_Leaves[0] = oldtn->tn_Children[seqcode] & ~LEAFMASK;
  } else {
    if(((ULONG) oldtn->tn_Level + 1 < pp->pp_TreePruneDepth) &&
       ((ULONG) oldtn->tn_TreeOffset + 1 <= pp->pp_TreePruneLength))
    {
      /* just go down normally */
      tn = ReadPackedNodeNoEdge(pp, oldtn->tn_Children[seqcode]);
    } else {
      /* leaf reached */
      tn = ReadPackedLeaf(pp, oldtn->tn_Children[seqcode]);
    }
  }
  /* fill in downlink information */
  tn->tn_TreeOffset = oldtn->tn_TreeOffset + tn->tn_EdgeLen;
  if(tn->tn_TreeOffset > pp->pp_TreePruneLength)
  {
    tn->tn_EdgeLen = pp->pp_TreePruneLength - oldtn->tn_TreeOffset;
  }
  tn->tn_Level = oldtn->tn_Level + 1;
  tn->tn_ParentSeq = seqcode;
  tn->tn_ParentPos = oldtn->tn_Pos;
  tn->tn_Parent = oldtn;
  return(tn);
}
/* \\\ */

/* /// "ReadPackedNode()" */
struct TreeNode * ReadPackedNode(struct PTPanPartition *pp, ULONG pos)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;
  struct HuffTree *ht;
  ULONG bitpos = 0;
  UBYTE *treeptr = &pp->pp_DiskTree[pos];
  ULONG edgelen;
  ULONG edgepos;
  ULONG cnt;
  ULONG mask;
  ULONG pval;
  ULONG alphamask;
  ULONG newbase;
  BOOL shortedge;

#if 0 // debugging
  bitpos = 8;
  if(*treeptr != 0xf2)
  {
    printf("Pos %08lx: Magic %02x. We're in trouble.\n", pos, *treeptr);
    return(NULL);
  } else {
    //printf("Pos %08lx: GOOD\n", pos);
  }
#endif
  //printf("Pos %ld ", pos);
  /* read short edge tree */
  ht = FindHuffTreeID(pp->pp_ShortEdgeTree, treeptr, bitpos);
  bitpos += ht->ht_CodeLength;
  pval = ht->ht_ID;
  if(pval) /* short edge */
  {
    /* id refers to a short edge code */
    /* find out length of edge */
    //printf("SHORT EDGE ID %ld (bitlen %d)\n", pval, ht->ht_CodeLength);
    if(pval > 1)
    {
      edgelen = SHORTEDGEMAX;
      while(!(pval & (1UL << pg->pg_BitsUseTable[edgelen])))
      {
        edgelen--;
      }
      pval -= 1UL << pg->pg_BitsUseTable[edgelen];
      edgelen++;
    } else {
      edgelen = 1;
    }
    shortedge = TRUE;
  } else {
    /* long edge */
    ht = FindHuffTreeID(pp->pp_LongEdgeLenTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    edgelen = ht->ht_ID;
    //printf("LONG EDGE ID %ld (bitlen %d) ", edgelen, ht->ht_CodeLength);
    edgepos = ReadBits(treeptr, bitpos, pp->pp_LongRelPtrBits);
    bitpos += pp->pp_LongRelPtrBits;
    //printf("RELPTR %ld (bitlen %d)\n", edgepos, pp->pp_LongRelPtrBits);
    shortedge = FALSE;
  }

  tn = (struct TreeNode *) calloc(sizeof(struct TreeNode) +
                                  edgelen + 1, 1);
  /* fill in data */
  tn->tn_PTPanPartition = pp;
  tn->tn_Pos = pos;
  tn->tn_NumLeaves = 0;
  tn->tn_EdgeLen = edgelen;
  tn->tn_Edge = (STRPTR) &tn->tn_Leaves;
  *tn->tn_Edge = 'X';
  if(shortedge) /* decompress edge */
  {
    //printf("Short %ld ", pval);
    cnt = edgelen;
    while(--cnt)
    {
      tn->tn_Edge[cnt] = pg->pg_DecompressTable[pval % pg->pg_AlphaSize];
      pval /= pg->pg_AlphaSize;
    }
  } else {
    //printf("Long");
    DecompressSequencePartTo(pg, pp->pp_LongDictRaw, edgepos, edgelen - 1,
                                   &tn->tn_Edge[1]);
  }

  /* decode branch array */
  ht = FindHuffTreeID(pp->pp_BranchTree, treeptr, bitpos);
  bitpos += ht->ht_CodeLength;
  alphamask = ht->ht_ID;

  //printf("AlphaMask %lx (bitlen %d)\n", alphamask, ht->ht_CodeLength);
  for(cnt = 0; cnt < pg->pg_AlphaSize; cnt++)
  {
    if(alphamask & (1UL << cnt)) /* bit set? */
    {
      mask = ReadBits(treeptr, bitpos, 4);
      //printf("Mask %lx\n", mask);
      if(mask >> 3) /* {1} */
        {
          /* first bit is 1 (no "rel is next" pointer) */
          if((mask >> 2) == 0x2) /* {10} */
            {
              /* this is a 6 bit relative node pointer */
              bitpos += 2;
              tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 6);
              bitpos += 6;
            }
          else if((mask >> 1) == 0x7) /* {111} */
         {
          /* absolute leaf pointer */
         bitpos += 3;
         tn->tn_Children[cnt] = LEAFMASK | ReadBits(treeptr, bitpos, pg->pg_TotalRawBits);
         bitpos += pg->pg_TotalRawBits;
         }
         else if(mask == 0xc) /* {1100} */
         {
           /* this is a 11 bit relative node pointer */
           bitpos += 4;
           tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 11);
           bitpos += 11;
         } else { /* {1101} */
           /* this is a 27 bit relative node pointer */
           bitpos += 4;
           tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 27);
           bitpos += 27;
         }
         } else { /* {0} */
           /* rel is next pointer */
           bitpos++;
           tn->tn_Children[cnt] = 0;
         }
           //printf("Child %08lx\n", children[cnt]);
         }
         }
         tn->tn_Size = (bitpos + 7) >> 3;

  /* add size and pos to relative pointers */
  newbase = pos;
  for(cnt = 0; cnt < pg->pg_AlphaSize; cnt++)
  {
    if(alphamask & (1UL << cnt)) /* bit set? */
    {
      if(!(tn->tn_Children[cnt] >> LEAFBIT))
      {
        //printf("N[%08lx] ", children[cnt]);
        newbase += tn->tn_Children[cnt];
        tn->tn_Children[cnt] = newbase;
        tn->tn_Children[cnt] += tn->tn_Size;
#if 0
        /* debug */
        if((tn->tn_Children[cnt] > pp->pp_DiskTreeSize) ||
           (pp->pp_DiskTree[tn->tn_Children[cnt]] != 0xf2))
        {
          ULONG bc;
          printf("ARGH: [%08lx]: Child Pos %08lx for %ld out of range (%ld)!\n",
                 pos, tn->tn_Children[cnt], cnt, pp->pp_DiskTreeSize);
          for(bc = 0; bc < 32; bc++)
          {
            printf("[%02x] ", treeptr[bc]);
          }
          printf("\n Size=%ld, Edge(%s %ld) = %s\n",
          tn->tn_Size, shortedge ? "Short" : "Long", pval, tn->tn_Edge);
          return(NULL);
         }
#endif
      }
    }
  }
  return(tn);
}
/* \\\ */

/* /// "ReadPackedNodeNoEdge()" */
struct TreeNode * ReadPackedNodeNoEdge(struct PTPanPartition *pp, ULONG pos)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;
  struct HuffTree *ht;
  ULONG bitpos = 0;
  UBYTE *treeptr = &pp->pp_DiskTree[pos];
  ULONG edgelen;
  ULONG cnt;
  ULONG mask;
  ULONG pval;
  ULONG alphamask;
  ULONG newbase;

  /* read short edge tree */
  ht = FindHuffTreeID(pp->pp_ShortEdgeTree, treeptr, bitpos);
  bitpos += ht->ht_CodeLength;
  pval = ht->ht_ID;
  /* this is the quick version -- no need to fully decode the edge */
  if(pval) /* short edge */
  {
    /* id refers to a short edge code */
    /* find out length of edge */
    if(pval > 1)
    {
      edgelen = SHORTEDGEMAX;
      while(!(pval & (1UL << pg->pg_BitsUseTable[edgelen])))
      {
        edgelen--;
      }
      edgelen++;
    } else {
      edgelen = 1;
    }
  } else {
    /* long edge */
    ht = FindHuffTreeID(pp->pp_LongEdgeLenTree, treeptr, bitpos);
    bitpos += ht->ht_CodeLength;
    edgelen = ht->ht_ID;
    bitpos += pp->pp_LongRelPtrBits;
  }

  tn = (struct TreeNode *) calloc(sizeof(struct TreeNode), 1);
  /* fill in data */
  tn->tn_PTPanPartition = pp;
  tn->tn_Pos = pos;
  tn->tn_NumLeaves = 0;
  tn->tn_EdgeLen = edgelen;

  /* decode branch array */
  ht = FindHuffTreeID(pp->pp_BranchTree, treeptr, bitpos);
  bitpos += ht->ht_CodeLength;
  alphamask = ht->ht_ID;

  for(cnt = 0; cnt < pg->pg_AlphaSize; cnt++)
  {
    if(alphamask & (1UL << cnt)) /* bit set? */
    {
      mask = ReadBits(treeptr, bitpos, 4);
      if(mask >> 3) /* {1} */
      {
       /* first bit is 1 (no "rel is next" pointer) */
        if((mask >> 2) == 0x2) /* {10} */
        {
/* this is a 6 bit relative node pointer */
bitpos += 2;
tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 6);
bitpos += 6;
}
else if((mask >> 1) == 0x7) /* {111} */
{
/* absolute leaf pointer */
bitpos += 3;
tn->tn_Children[cnt] = LEAFMASK | ReadBits(treeptr, bitpos, pg->pg_TotalRawBits);
bitpos += pg->pg_TotalRawBits;
}
else if(mask == 0xc) /* {1100} */
        {
        /* this is a 11 bit relative node pointer */
        bitpos += 4;
        tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 11);
        bitpos += 11;
        } else { /* {1101} */
        /* this is a 27 bit relative node pointer */
        bitpos += 4;
        tn->tn_Children[cnt] = ReadBits(treeptr, bitpos, 27);
        bitpos += 27;
        }
      } else { /* {0} */
        /* rel is next pointer */
        bitpos++;
        tn->tn_Children[cnt] = 0;
      }
    }
  }
  tn->tn_Size = (bitpos + 7) >> 3;

  /* add size and pos to relative pointers */
  newbase = pos;
  for(cnt = 0; cnt < pg->pg_AlphaSize; cnt++)
  {
    if(alphamask & (1UL << cnt)) /* bit set? */
    {
      if(!(tn->tn_Children[cnt] >> LEAFBIT))
      {
        newbase += tn->tn_Children[cnt];
        tn->tn_Children[cnt] = newbase;
        tn->tn_Children[cnt] += tn->tn_Size;
      }
    }
  }
  return(tn);
}
/* \\\ */

/* /// "ReadPackedLeaf()" */
struct TreeNode * ReadPackedLeaf(struct PTPanPartition *pp, ULONG pos)
{
  //struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct TreeNode *tn;
  UBYTE *treeptr = &pp->pp_DiskTree[pos];
  ULONG leaves = 0;
  UBYTE code;
  ULONG cnt;
  ULONG *lptr;
  LONG val;
  /* compression:
       msb == {1}    -> 1 byte offset (-   63 -     63)
       msb == {01}   -> 2 byte offset (- 8192 -   8191)
       msb == {001}  -> 3 byte offset (- 2^20 - 2^20-1)
       msb == {0001} -> 4 byte offset (- 2^27 - 2^27-1)
       msb == {0000} -> 5 byte offset (- 2^35 - 2^35-1)
  */

  /* get number of leaves first */
  while((code = *treeptr) != 0xff)
  {
    leaves++;
    if(code >> 7) /* {1} */
    {
      /* 1 byte */
      treeptr++;
    }
    else if(code >> 6) /* {01} */
    {
      /* 2 bytes */
      treeptr += 2;
    }
    else if(code >> 5) /* {001} */
    {
      /* 3 bytes */
      treeptr += 3;
    }
    else if(code >> 4) /* {0001} */
    {
      /* 4 bytes */
      treeptr += 4;
    } else {           /* {0000} */
      /* 5 bytes */
      treeptr += 5;
    }
  }
  treeptr++;
  tn = (struct TreeNode *) calloc(sizeof(struct TreeNode) +
                                  sizeof(ULONG) * leaves +
                                   1 + 1, 1);
  /* fill in data */
  tn->tn_PTPanPartition = pp;
  tn->tn_Pos = pos;
  tn->tn_Size = (ULONG) (treeptr - &pp->pp_DiskTree[pos]);
  tn->tn_NumLeaves = leaves;
  tn->tn_Edge = (STRPTR) &tn->tn_Leaves[leaves];
  *tn->tn_Edge = 'X';
  tn->tn_EdgeLen = 1;
  treeptr = &pp->pp_DiskTree[pos];
  lptr = tn->tn_Leaves;
  while((code = *treeptr++) != 0xff)
  {
    if(code >> 7) /* {1} */
    {
      /* 1 byte */
      *lptr++ = (code & 0x7f) - 63;
    }
    else if(code >> 6) /* {01} */
    {
      /* 2 bytes */
      val = (code & 0x3f) << 8;
      val |= *treeptr++;
      *lptr++ = val - 8192;
    }
    else if(code >> 5) /* {001} */
    {
      /* 3 bytes */
      val = (code & 0x1f) << 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      *lptr++ = val - (1L << 20);
    }
    else if(code >> 4) /* {0001} */
    {
      /* 4 bytes */
      val = (code & 0x0f) << 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      *lptr++ = val - (1L << 27);
    } else {           /* {0000} */
      /* 5 bytes */
      val = (code & 0x0f) << 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      val <<= 8;
      val |= *treeptr++;
      *lptr++ = val - (1L << 35);
    }
  }
  /* double delta decode */
#ifdef DOUBLEDELTAOPTIONAL
  if(((LONG) *tn->tn_Leaves) < 0)
  {
    *tn->tn_Leaves = -(*tn->tn_Leaves);
    (*tn->tn_Leaves)++;
#else
  {
#endif

#ifdef DOUBLEDELTALEAVES
    for(cnt = 2; cnt < leaves; cnt++)
    {
      tn->tn_Leaves[cnt] += tn->tn_Leaves[cnt-1];
    }
#endif
#ifdef DOUBLEDELTAOPTIONAL
  }
#else
  }
#endif
  for(cnt = 1; cnt < leaves; cnt++)
  {
    tn->tn_Leaves[cnt] += tn->tn_Leaves[cnt-1];
    arb_assert(tn->tn_Leaves[cnt] < pp->pp_PTPanGlobal->pg_TotalRawSize);
  }
  return(tn);
}
/* \\\ */

/* /// "WritePackedNode()" */
ULONG WritePackedNode(struct PTPanPartition *pp, ULONG pos, UBYTE *buf)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  ULONG val;
  ULONG bpos = 0;
  ULONG cnt;
  ULONG newbase;

  //*buf = 0xF2; bpos = 8; /* debugging */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];

  /* huffman code for short / long edge */
  if(sfxnode->sn_StartPos & (1UL << 31))
  {
    /* long edge */
    /*printf("LE[%02d+%02d+%02d] ",
           pp->pp_ShortEdgeCode[0].hc_CodeLength,
           pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength,
           pp->pp_LongRelPtrBits);*/
    bpos = WriteBits(buf, bpos,
                     pp->pp_ShortEdgeCode[0].hc_Codec,
                     pp->pp_ShortEdgeCode[0].hc_CodeLength);
    bpos = WriteBits(buf, bpos,
                     pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_Codec,
                     pp->pp_LongEdgeLenCode[sfxnode->sn_EdgeLen].hc_CodeLength);
    bpos = WriteBits(buf, bpos,
                     sfxnode->sn_StartPos & RELOFFSETMASK,
                     pp->pp_LongRelPtrBits);
  }
  else if(sfxnode->sn_StartPos & (1UL << 30))
  {
    /* short edge */
    /*printf("SE[%04ld= %02d] ", sfxnode->sn_StartPos & RELOFFSETMASK,
      pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_CodeLength);*/
    bpos = WriteBits(buf, bpos,
                     pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_Codec,
                     pp->pp_ShortEdgeCode[sfxnode->sn_StartPos & RELOFFSETMASK].hc_CodeLength);

  } else {
    if(pos == 0)
    {
      /* special root handling */
      bpos = WriteBits(buf, bpos,
                       pp->pp_ShortEdgeCode[1].hc_Codec,
                       pp->pp_ShortEdgeCode[1].hc_CodeLength);
    } else {
      printf("Internal error: Tree corrupt at %ld (%08lx)\n", pos, sfxnode->sn_StartPos);
    }
  }

  /* branch code */
  //printf("BC[%02d] ", pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_CodeLength);
  bpos = WriteBits(buf, bpos,
                   pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_Codec,
                   pp->pp_BranchCode[sfxnode->sn_AlphaMask].hc_CodeLength);

  /* child pointers */
  /* msb = {0}   -> rel is next
     msb = {10}  ->  6 bit rel node ptr
     msb = {1100} -> 11 bit rel node ptr
     msb = {1101} -> 27 bit rel node ptr (large enough for a jump over 128 MB)
     msb = {111} -> nn bit abs leaf ptr (nn = pg->pg_TotalRawBits)
  */
  cnt = sfxnode->sn_Parent >> RELOFFSETBITS;
  childidx = 0;
  newbase = 0;
  while(childidx < cnt)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer */
      //printf("LF[%08lx]\n", childptr);
      bpos = WriteBits(buf, bpos, 0x7, 3);
      bpos = WriteBits(buf, bpos,
                       childptr & RELOFFSETMASK,
                       pg->pg_TotalRawBits);
    } else {
      /* this is a normal node pointer */
      val = childptr & RELOFFSETMASK;
      val -= newbase;
      newbase += val;
      //printf("Dist: %6ld\n", val);
      if(val == 0)
      {
        /* next */
        //printf("NP[1] ");
        bpos = WriteBits(buf, bpos, 0x0, 1);
      } else {
        if(val < (1UL << 6))
        {
          //printf("NP[8] ");
          bpos = WriteBits(buf, bpos, 0x2, 2);
          bpos = WriteBits(buf, bpos, val, 6);
        } else {
          if(val < (1UL << 11))
          {
            //printf("NP[15] ");
            bpos = WriteBits(buf, bpos, 0xc, 4);
            bpos = WriteBits(buf, bpos, val, 11);
          } else {
            //printf("NP[31] ");
            bpos = WriteBits(buf, bpos, 0xd, 4);
            bpos = WriteBits(buf, bpos, val, 27);
          }
        }
      }
    }
    childidx++;
  }
  //printf("P[%08lx] %ld\n", pos, bpos);
  return((bpos + 7) >> 3);
}
/* \\\ */

/* /// "WritePackedLeaf()" */
ULONG WritePackedLeaf(struct PTPanPartition *pp, ULONG pos, UBYTE *buf)
{
  struct SfxNode *sfxnode;
  ULONG leafcnt;
  ULONG cnt;
  LONG oldval;
  LONG val;
  ULONG leafsize = 1;
  ULONG sdleafsize = 1;
  UBYTE *origbuf = buf;

  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];

  /* how many leaves do we have? */
  leafcnt = GetTreeStatsLeafCountRec(pp, pos);
  //printf("%ld leaves\n", leafcnt);
  /* enlarge memory, if too small */
  if(pp->pp_LeafBufferSize < leafcnt)
  {
    pp->pp_LeafBuffer = (LONG *) realloc(pp->pp_LeafBuffer, leafcnt * sizeof(LONG));
    pp->pp_LeafBufferSize = leafcnt;
  }
  pp->pp_LeafBufferPtr = pp->pp_LeafBuffer;
  /* collect leafs */
  GetTreeStatsLeafCollectRec(pp, pos);
  if(leafcnt > 1)
  {
    qsort(pp->pp_LeafBuffer, leafcnt, sizeof(ULONG),
          (int (*)(const void *, const void *)) ULONGCompare);
  }
  /* do delta compression */
  oldval = pp->pp_LeafBuffer[0];
  arb_assert(pp->pp_LeafBuffer[0] < pp->pp_PTPanGlobal->pg_TotalRawSize);
  for(cnt = 1; cnt < leafcnt; cnt++)
  {
    arb_assert(pp->pp_LeafBuffer[cnt] < pp->pp_PTPanGlobal->pg_TotalRawSize);
    pp->pp_LeafBuffer[cnt] -= oldval;
    oldval += pp->pp_LeafBuffer[cnt];
    //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
  }

#ifdef DOUBLEDELTALEAVES
#ifdef DOUBLEDELTAOPTIONAL
  for(cnt = 0; cnt < leafcnt; cnt++)
  {
    val = pp->pp_LeafBuffer[cnt];
    if((val < -63) || (val > 63))
    {
      if((val < -8192) || (val > 8191))
      {
        if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
        {
          if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
          {
            /* five bytes */
            sdleafsize += 5;
          } else {
            /* four bytes */
            sdleafsize += 4;
          }
        } else {
          /* three bytes */
          sdleafsize += 3;
        }
      } else {
        /* two bytes */
        sdleafsize += 2;
      }
    } else {
      /* one byte */
      sdleafsize++;
    }
  }
  *pp->pp_LeafBuffer = -(*pp->pp_LeafBuffer);
  (*pp->pp_LeafBuffer)--;
#endif
  /* do delta compression a second time */
  if(leafcnt > 2)
  {
    oldval = pp->pp_LeafBuffer[1];
    for(cnt = 2; cnt < leafcnt; cnt++)
    {
      pp->pp_LeafBuffer[cnt] -= oldval;
      oldval += pp->pp_LeafBuffer[cnt];
      //printf("%ld\n", pp->pp_LeafBuffer[cnt]);
    }
  }
#endif

  /* compression:
       msb == {1}    -> 1 byte offset (-   63 -     63)
       msb == {01}   -> 2 byte offset (- 8192 -   8191)
       msb == {001}  -> 3 byte offset (- 2^20 - 2^20-1)
       msb == {0001} -> 4 byte offset (- 2^27 - 2^27-1)
       msb == {0000} -> 5 byte offset (- 2^35 - 2^35-1)
     special opcodes:
       0xff      -> end of array
     This means the upper limit is currently 512 MB for raw sequence data
     (this can be changed though: Just introduce another opcode)
  */

  for(cnt = 0; cnt < leafcnt; cnt++)
  {
    val = pp->pp_LeafBuffer[cnt];
    if((val < -63) || (val > 63))
    {
      if((val < -8192) || (val > 8191))
      {
        if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
        {
          if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
          {
            /* five bytes */
            if((val < -(1L << 35)) || (val > ((1L << 35) - 1)))
            {
              printf("ERROR: %ld: %ld, %ld out of range\n", pos, cnt, val);
            }
            val += 1L << 35;
            *buf++ = (val >> 32) & 0x0f;
            *buf++ = (val >> 24);
            *buf++ = (val >> 16);
            *buf++ = (val >> 8);
            *buf++ = val;
            leafsize += 5;

          } else {
            /* four bytes */
            val += 1L << 27;
            *buf++ = ((val >> 24) & 0x0f) | 0x10;
            *buf++ = (val >> 16);
            *buf++ = (val >> 8);
            *buf++ = val;
            leafsize += 4;
          }
        } else {
          /* three bytes */
          val += 1L << 20;
          *buf++ = ((val >> 16) & 0x1f) | 0x20;
          *buf++ = (val >> 8);
          *buf++ = val;
          leafsize += 3;
        }
      } else {
        /* two bytes */
        val += 8192;
        *buf++ = ((val >> 8) & 0x3f) | 0x40;
        *buf++ = val;
        leafsize += 2;
      }
    } else {
      /* one byte */
      val += 63;
      *buf++ = (val & 0x7f) | 0x80;
      leafsize++;
    }
  }
#ifdef DOUBLEDELTAOPTIONAL
  if(sdleafsize < leafsize)
  {
    /* undo double delta */
    for(cnt = 2; cnt < leafcnt; cnt++)
    {
      pp->pp_LeafBuffer[cnt] += pp->pp_LeafBuffer[cnt-1];
    }
    buf = origbuf;
    leafsize = sdleafsize;
    for(cnt = 0; cnt < leafcnt; cnt++)
    {
      val = pp->pp_LeafBuffer[cnt];
      if((val < -63) || (val > 63))
      {
        if((val < -8192) || (val > 8191))
        {
          if((val < -(1L << 20)) || (val > ((1L << 20) - 1)))
          {
              if((val < -(1L << 27)) || (val > ((1L << 27) - 1)))
              {
                /* five bytes */
                val += 1L << 35;
                *buf++ = (val >> 32) & 0x0f;
                *buf++ = (val >> 24);
                *buf++ = (val >> 16);
                *buf++ = (val >> 8);
                *buf++ = val;
              } else {
                /* four bytes */
                val += 1L << 27;
                *buf++ = ((val >> 24) & 0x0f) | 0x10;
                *buf++ = (val >> 16);
                *buf++ = (val >> 8);
                *buf++ = val;
              }
          } else {
            /* three bytes */
            val += 1L << 20;
            *buf++ = ((val >> 16) & 0x1f) | 0x20;
            *buf++ = (val >> 8);
            *buf++ = val;
          }
        } else {
          /* two bytes */
          val += 8192;
          *buf++ = ((val >> 8) & 0x3f) | 0x40;
          *buf++ = val;
      }
      } else {
        /* one byte */
        val += 63;
        *buf++ = (val & 0x7f) | 0x80;
      }
    }
  }
#endif
  *buf = 0xff;
  return(leafsize);
}
/* \\\ */

/* /// "CloneSearchQuery()" */
struct SearchQuery * CloneSearchQuery(struct SearchQuery *oldsq)
{
  struct SearchQuery *sq;

  sq = (struct SearchQuery *) calloc(sizeof(struct SearchQuery), 1);
  if(!sq)
  {
    return(NULL);
  }
  /* copy the whole information */
  *sq = *oldsq;
  /* fix not cloneable stuff */
  NewList(&sq->sq_Hits);
  sq->sq_HitsHash = NULL;
  sq->sq_PosWeight = new double[sq->sq_QueryLen + 1];
  memcpy(sq->sq_PosWeight, oldsq->sq_PosWeight, (sq->sq_QueryLen + 1) * sizeof(double));
  return(sq);
}
/* \\\ */

/* /// "AllocSearchQuery()" */
struct SearchQuery * AllocSearchQuery(struct PTPanGlobal *pg)
{
  struct SearchQuery *sq;

  sq = (struct SearchQuery *) calloc(sizeof(struct SearchQuery), 1);
  if(!sq)
  {
    return(NULL);
  }

  NewList(&sq->sq_Hits);
  sq->sq_PTPanGlobal = pg;
  sq->sq_MismatchWeights = &pg->pg_NoWeights;
  sq->sq_MaxErrors = 0.0;
  sq->sq_Reversed = FALSE;
  sq->sq_AllowReplace = TRUE;
  sq->sq_AllowInsert = FALSE;
  sq->sq_AllowDelete = FALSE;
  sq->sq_KillNSeqsAt = 0x80000000; /* maximum */
  sq->sq_MinorMisThres = 0.5;
  sq->sq_SortMode = SORT_HITS_NOWEIGHT;
  sq->sq_HitsHashSize = QUERYHITSHASHSIZE;
  sq->sq_HitsHash  = NULL;
  sq->sq_PosWeight = NULL;
  return(sq);
}
/* \\\ */

/* /// "FreeSearchQuery()" */
void FreeSearchQuery(struct SearchQuery *sq)
{
  struct QueryHit *qh;

  /* free hits */
  qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  while(qh->qh_Node.ln_Succ)
  {
    RemQueryHit(qh);
    qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  }
  /* free hash table */
  FreeHashArray(sq->sq_HitsHash);
  
  if (sq->sq_PosWeight) 
  {
    delete[] sq->sq_PosWeight;
    sq->sq_PosWeight = NULL;
  }
  
  /* free structure itself */
  free(sq);
}
/* \\\ */

/* /// "SearchTree()" */
void SearchTree(struct SearchQuery *sq)
{
  if (PTPanGlobalPtr->pg_verbose >0)
    printf(">> SearchTree: Searching for %s\n", sq->sq_Query);

  /* init */
  FreeHashArray(sq->sq_HitsHash);
  sq->sq_HitsHash = AllocHashArray(sq->sq_HitsHashSize);
  sq->sq_QueryLen = strlen(sq->sq_Query);
  sq->sq_NumHits = 0;
  NewList(&sq->sq_Hits);
  sq->sq_State.sqs_TreeNode = ReadPackedNode(sq->sq_PTPanPartition, 0);
  sq->sq_State.sqs_QueryPos = 0;
  sq->sq_State.sqs_ErrorCount = 0.0;
  sq->sq_State.sqs_ReplaceCount = 0;
  sq->sq_State.sqs_InsertCount = 0;
  sq->sq_State.sqs_DeleteCount = 0;
  sq->sq_State.sqs_NCount = 0;
  SearchTreeRec(sq);
  free(sq->sq_State.sqs_TreeNode);

  if (PTPanGlobalPtr->pg_verbose >0)
    printf("<< SearchTree\n");
}
/* \\\ */

/* /// "PostFilterQueryHits()" */
void PostFilterQueryHits(struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  struct PTPanSpecies *ps;
  struct QueryHit *qh;
  struct QueryHit *nextqh;

  if (PTPanGlobalPtr->pg_verbose >1) {
    printf(">> PostFilterQueryHits: Hits %ld\n", sq->sq_NumHits);
  }

  if(!sq->sq_NumHits) /* do we have hits at all? */
  {
    return;
  }
  /* do we need to sort the list? */
  if(sq->sq_NumHits > 1)
  {
    /* enter priority and sort */
    qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
    while(qh->qh_Node.ln_Succ)
    {
      qh->qh_Node.ln_Pri = qh->qh_AbsPos;
      qh = (struct QueryHit *) qh->qh_Node.ln_Succ;
    }
    SortList(&sq->sq_Hits);
  }

  /* get species, delete duplicates, delete alignment crossing hits */
  qh = (struct QueryHit *) sq->sq_Hits.lh_Head;
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while((nextqh = (struct QueryHit *) qh->qh_Node.ln_Succ))
  {
    /* check if current species is still valid */
    if((qh->qh_AbsPos < ps->ps_AbsOffset) ||
       (qh->qh_AbsPos >= (ps->ps_AbsOffset + (ULLONG) ps->ps_RawDataSize)))
    {
      /* go to next node by chance */
      ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
      if(ps->ps_Node.ln_Succ)
      {
        if((qh->qh_AbsPos < ps->ps_AbsOffset) ||
          (qh->qh_AbsPos >= (ps->ps_AbsOffset + (ULLONG) ps->ps_RawDataSize)))
        {
          /* still didn't match, so find it using the hard way */
          ps = (struct PTPanSpecies *) FindBinTreeLowerKey(pg->pg_SpeciesBinTree,
                                                       qh->qh_AbsPos);
        }
      } else {
        ps = (struct PTPanSpecies *) FindBinTreeLowerKey(pg->pg_SpeciesBinTree,
                                                        qh->qh_AbsPos);
      }
    }
    if((qh->qh_AbsPos < ps->ps_AbsOffset) ||
       (qh->qh_AbsPos >= (ps->ps_AbsOffset + (ULLONG) ps->ps_RawDataSize)))
    {
      printf("Mist gebaut (%s(%ld) Pos: %ld, Len %ld HitPos %ld)!\n",
            ps->ps_Name, ps->ps_Num, ps->ps_AbsOffset, ps->ps_RawDataSize, qh->qh_AbsPos);
    }

    if (PTPanGlobalPtr->pg_verbose >1) {
      printf(" Hit %s (%ld) Pos: %ld, Len %ld HitPos %ld\n",
        ps->ps_Name, ps->ps_Num, ps->ps_AbsOffset, ps->ps_RawDataSize, qh->qh_AbsPos);
    }

    /* enter species */
    qh->qh_Species = ps;
    /* filter alignment crossing hits */
    if(qh->qh_AbsPos - ps->ps_AbsOffset > ps->ps_RawDataSize - sq->sq_QueryLen)
    {
      if (PTPanGlobalPtr->pg_verbose >1) {
        printf(" Border crossed [%ld/%ld]\n",
        qh->qh_AbsPos - ps->ps_AbsOffset, ps->ps_RawDataSize);
      }
      pg->pg_Bench.ts_CrossBoundKilled++;
      RemQueryHit(qh);
    }
    qh = nextqh;
  }
}
/* \\\ */

/* /// "AddQueryHit()" */
BOOL AddQueryHit(struct SearchQuery *sq, ULONG hitpos)
{
  arb_assert(hitpos < sq->sq_PTPanGlobal->pg_TotalRawSize);

  struct QueryHit *qh;
  struct HashEntry *hash;

  if (PTPanGlobalPtr->pg_verbose >1) {
    struct TreeNode *tn;
    printf(">> AddQueryHit: ");
    tn = sq->sq_State.sqs_TreeNode;
    do
    {
      printf("%s", tn->tn_Edge);
      tn = tn->tn_Parent;
      if (tn) printf("-");
    } while(tn);
    printf(" (%f/%d/%d/%d) QryPos %ld/%ld\n",
           sq->sq_State.sqs_ErrorCount,
           sq->sq_State.sqs_ReplaceCount,
           sq->sq_State.sqs_InsertCount,
           sq->sq_State.sqs_DeleteCount,
           sq->sq_State.sqs_QueryPos,
           sq->sq_QueryLen);
  }

  /* try eliminating duplicates even at this stage */
  if((hash = GetHashEntry(sq->sq_HitsHash, hitpos)))
  {
    qh = (struct QueryHit *) hash->he_Data;
    if((qh->qh_AbsPos == hitpos + sq->sq_PTPanPartition->pp_RawOffset))
    {
      /* check, if the new hit was better */
      if(qh->qh_ErrorCount > sq->sq_State.sqs_ErrorCount)
      {
        qh->qh_ErrorCount = sq->sq_State.sqs_ErrorCount;
        qh->qh_ReplaceCount = sq->sq_State.sqs_ReplaceCount;
        qh->qh_InsertCount = sq->sq_State.sqs_InsertCount;
        qh->qh_DeleteCount = sq->sq_State.sqs_DeleteCount;
        if(sq->sq_Reversed) /* copy reversed flag */
        {
        qh->qh_Flags |= QHF_REVERSED;
        } else {
        qh->qh_Flags &= ~QHF_REVERSED;
        }
      }
      /* if the hit was safe now, we can clear the unsafe bit */
      if((sq->sq_State.sqs_QueryPos >= sq->sq_QueryLen))
      {
       qh->qh_Flags &= ~QHF_UNSAFE; /* clear unsafe bit */
      }
      return(TRUE);
    }
  }
#if 0 // old stuff
  qh = (struct QueryHit *) sq->sq_Hits.lh_TailPred;
  if(qh->qh_Node.ln_Pred && (qh->qh_AbsPos == hitpos + sq->sq_PTPanPartition->pp_RawOffset))
  {
    //printf("Duplicate!\n");
    /* check, if the new hit was better */
    if(qh->qh_ErrorCount > sq->sq_State.sqs_ErrorCount)
    {
      qh->qh_ErrorCount = sq->sq_State.sqs_ErrorCount;
      qh->qh_ReplaceCount = sq->sq_State.sqs_ReplaceCount;
      qh->qh_InsertCount = sq->sq_State.sqs_InsertCount;
      qh->qh_DeleteCount = sq->sq_State.sqs_DeleteCount;
      if(sq->sq_Reversed) /* copy reversed flag */
      {
        qh->qh_Flags |= QHF_REVERSED;
      } else {
        qh->qh_Flags &= ~QHF_REVERSED;
      }
    }
    /* if the hit was safe now, we can clear the unsafe bit */
    if((sq->sq_State.sqs_QueryPos >= sq->sq_QueryLen))
    {
      qh->qh_Flags &= ~QHF_UNSAFE; /* clear unsafe bit */
    }
    return(TRUE);
  }
#endif
  qh = (struct QueryHit *) malloc(sizeof(struct QueryHit));
  if(!qh)
  {
    return(FALSE); /* out of memory */
  }
  sq->sq_NumHits++;

  /* add hit to array */
  qh->qh_AbsPos = hitpos;
  qh->qh_AbsPos += sq->sq_PTPanPartition->pp_RawOffset; /* allow more than 2 GB */
  qh->qh_ErrorCount = sq->sq_State.sqs_ErrorCount;
  qh->qh_ReplaceCount = sq->sq_State.sqs_ReplaceCount;
  qh->qh_InsertCount = sq->sq_State.sqs_InsertCount;
  qh->qh_DeleteCount = sq->sq_State.sqs_DeleteCount;
  qh->qh_Species = NULL;
  qh->qh_Flags = QHF_ISVALID;
  if(sq->sq_Reversed) /* set reversed flag */
  {
    qh->qh_Flags |= QHF_REVERSED;
  }
  if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
  {
    qh->qh_Flags |= QHF_UNSAFE;
  }
  AddTail(&sq->sq_Hits, &qh->qh_Node);
  InsertHashEntry(sq->sq_HitsHash, hitpos, (ULONG) qh);

  if (PTPanGlobalPtr->pg_verbose >1) {
    struct TreeNode *tn;
    printf("<< AddQueryHit: %ld [%08lx] <%s>: ",
        qh->qh_AbsPos, qh->qh_AbsPos, sq->sq_PTPanPartition->pp_PrefixSeq);
    tn = sq->sq_State.sqs_TreeNode;
    do
    {
      printf("%s", tn->tn_Edge);
      tn = tn->tn_Parent;
      if (tn) printf("-");
    } while(tn);

    printf(" (%f/%d/%d/%d)\n",
        qh->qh_ErrorCount,
        qh->qh_ReplaceCount,
        qh->qh_InsertCount,
        qh->qh_DeleteCount);
  }

  return(TRUE);
}
/* \\\ */

/* /// "RemQueryHit()" */
void RemQueryHit(struct QueryHit *qh)
{
  /* unlink and free node */
  Remove(&qh->qh_Node);
  free(qh);
}
/* \\\ */

/* /// "MergeQueryHits()" */
void MergeQueryHits(struct SearchQuery *tarsq, struct SearchQuery *srcsq)
{
  struct QueryHit *srcqh = (struct QueryHit *) srcsq->sq_Hits.lh_Head;
  struct QueryHit *tarqh = (struct QueryHit *) tarsq->sq_Hits.lh_Head;
  struct QueryHit *lasttarqh = NULL; //(struct QueryHit *) tarqh->qh_Node.ln_Pred;

  /* iterate over source list */
  while(srcqh->qh_Node.ln_Succ)
  {
    while(tarqh->qh_Node.ln_Succ) /* are we at the end of the list? */
    {
      if(tarqh->qh_AbsPos > srcqh->qh_AbsPos) /* find the appropriate position */
      {
        break;
      }
      lasttarqh = tarqh; /* this one is before srcqh */
      tarqh = (struct QueryHit *) tarqh->qh_Node.ln_Succ;
    }
    //printf("Src: %ld\n", srcqh->qh_AbsPos);
    Remove(&srcqh->qh_Node);
    tarsq->sq_NumHits++;
    if(lasttarqh) /* is there a previous node to compare? */
    {
      /* check for duplicate */
      if(lasttarqh->qh_AbsPos == srcqh->qh_AbsPos)
      {
        //printf("Duplicate %ld!\n", lasttarqh->qh_AbsPos);
        tarsq->sq_PTPanGlobal->pg_Bench.ts_DupsKilled++;
        /* okay, this is a double -- which one do we keep? */
        if(lasttarqh->qh_ErrorCount < srcqh->qh_ErrorCount)
        {
        /* the old target was a better hit, so eliminate the old one */
          srcqh->qh_ErrorCount = lasttarqh->qh_ErrorCount;
          srcqh->qh_ReplaceCount = lasttarqh->qh_ReplaceCount;
          srcqh->qh_InsertCount = lasttarqh->qh_InsertCount;
          srcqh->qh_DeleteCount = lasttarqh->qh_DeleteCount;
        }
        /* remove the query hit */
        RemQueryHit(lasttarqh);
        tarsq->sq_NumHits--;
        lasttarqh = (struct QueryHit *) tarqh->qh_Node.ln_Pred;
        if(!lasttarqh->qh_Node.ln_Pred)
        {
        /* we killed the very first entry */
        //printf("Killed First!\n");
        AddHead(&tarsq->sq_Hits, &srcqh->qh_Node);
        } else {
        //printf("Insert\n");
        /* attach between lasttarqh and tarqh */
        lasttarqh->qh_Node.ln_Succ = tarqh->qh_Node.ln_Pred = &srcqh->qh_Node;
        srcqh->qh_Node.ln_Pred = &lasttarqh->qh_Node;
        srcqh->qh_Node.ln_Succ = &tarqh->qh_Node;
        }
      } else {
        /* normal case: insert it between lasttarqh and tarqh */
        lasttarqh->qh_Node.ln_Succ = tarqh->qh_Node.ln_Pred = &srcqh->qh_Node;
        srcqh->qh_Node.ln_Pred = &lasttarqh->qh_Node;
        srcqh->qh_Node.ln_Succ = &tarqh->qh_Node;
      }
    } else {
      /* we were at the start of the list */
      //printf("AddHead %ld\n", srcqh->qh_AbsPos);
      AddHead(&tarsq->sq_Hits, &srcqh->qh_Node);
    }
    tarqh = lasttarqh = srcqh;
    srcqh = (struct QueryHit *) srcsq->sq_Hits.lh_Head;
  }
  tarsq->sq_PTPanGlobal->pg_Bench.ts_Hits += tarsq->sq_NumHits;
  //printf("%ld Hits\n", tarsq->sq_NumHits);
}
/* \\\ */

void PrintSearchQueryState(const char* s1, const char* s2, struct SearchQuery *sq)
{
  static char seq[100];
  int i = 0;
  struct TreeNode *tn;

  tn = sq->sq_State.sqs_TreeNode;
  do {
    i += sprintf(seq+i, "%s", tn->tn_Edge);
    tn = tn->tn_Parent;
    if (tn) i += sprintf(seq+i,"-");
  } while(tn);
  seq[i] = 0;

#if 0
  if (strcmp(seq, "G-A-A-G-U-A-A-A-G-A-C-X")==0) {
    printf("STOP!!!\n");
  }
#endif

  if (PTPanGlobalPtr->pg_verbose >2) {
    printf("%s%s Src/Qry %ld/%ld: ", s1, s2,
           sq->sq_State.sqs_SourcePos, sq->sq_State.sqs_QueryPos);
    printf("%s (%f/%d/%d/%d)\n", seq,
           sq->sq_State.sqs_ErrorCount,
           sq->sq_State.sqs_ReplaceCount,
           sq->sq_State.sqs_InsertCount,
           sq->sq_State.sqs_DeleteCount);
  }

}


/* /// "SearchTreeRec()" */
void SearchTreeRec(struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  struct TreeNode *oldtn = sq->sq_State.sqs_TreeNode;
  struct TreeNode *tn;
  UWORD seqcode;
  UWORD seqcode2;
  UWORD seqcodetree;
  BOOL ignore;
  BOOL seqcompare;
  struct SearchQueryState oldstate;
  ULONG cnt;
  ULONG *leafptr;
  ULONG leaf;
  BOOL leafadded;

  const char* indent;
  if (PTPanGlobalPtr->pg_verbose >1) {
    const char* spaces = "                                            ";
    indent = spaces + strlen(spaces) - sq->sq_State.sqs_QueryPos;
  }

  seqcompare = (sq->sq_State.sqs_QueryPos < sq->sq_QueryLen);
  /* collect leaves on our way down */
  if((cnt = oldtn->tn_NumLeaves))
  {
    /* collect hits */
    leafptr = oldtn->tn_Leaves;
    do
    {
      AddQueryHit(sq, *leafptr++);
    } while(--cnt);
    /* if we got leaves, we're at the end of the tree */
    return;
  }

  if (PTPanGlobalPtr->pg_verbose >1)
    PrintSearchQueryState(indent, "=> SearchTreeRec: ", sq);

#if 1
  /* Maximum tree level reached?
   * This is possible even without <oldtn> being a LEAF node,
   * when we reach the node via a long edge. (JW, 13.3.05)
   */
  if(((ULONG) oldtn->tn_Level >= oldtn->tn_PTPanPartition->pp_TreePruneDepth) ||
     ((ULONG) oldtn->tn_TreeOffset >= oldtn->tn_PTPanPartition->pp_TreePruneLength))
  {
    AddQueryHit(sq, oldtn->tn_Children[0] & ~LEAFMASK);
    return;
  }
#endif


  oldstate = sq->sq_State;
  /* recurse on children */
  for(seqcode = 0; seqcode < pg->pg_AlphaSize; seqcode++)
  {
    /* check, if we have a child */
    if((leaf = oldtn->tn_Children[seqcode]))
    {
      tn = NULL;
      leafadded = FALSE;

      if (PTPanGlobalPtr->pg_verbose >2) {
        printf("%s>> SearchChild [%d] ", indent, seqcode);
        if(leaf & LEAFMASK)
            printf("LEAF %ld\n", leaf & ~LEAFMASK);
        else
            printf("%ld\n", leaf);
      }

      /* *** phase one: check 1:1 replacement; this considers the hamming distance */
      ignore = FALSE;
      if(seqcompare)
      {
    /* check, if first seqcode of edge matches (N (seqcode 0) matches always) */
    seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];
    if(seqcode2 != seqcode)
    {
        if(sq->sq_AllowReplace)
        {
        /* increase error level by replace operation */
        sq->sq_State.sqs_ReplaceCount++;
        sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + seqcode] 
                                       * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
        if(!seqcode)
        {
            if(++sq->sq_State.sqs_NCount > sq->sq_KillNSeqsAt)
            {
            ignore = TRUE;
            }
        }
            } else {
                ignore = TRUE;
            }
    }
/* check, if more errors are tolerable. */
if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
{
/* too many errors, do not recurse */
ignore = TRUE;
}
    sq->sq_State.sqs_QueryPos++;
      }

      /* should we take a deeper look? */
      if(!ignore)
      {
    /* did we have a leaf? */
    if(leaf & LEAFMASK)
    {
    AddQueryHit(sq, leaf & ~LEAFMASK);
    leafadded = TRUE;
    } else {
    /* get the child node */
    tn = GoDownNodeChild(oldtn, seqcode);

    if (PTPanGlobalPtr->pg_verbose >2) {
        // debug
        if(!tn)
        {
         printf("%s  Couldn't go down on %d!\n", indent, seqcode);
         break;
        }

        if(sq->sq_State.sqs_QueryPos == 1)
        {
        printf("%s  Down: %s\n", indent, tn->tn_Edge);
        }
    }

    /* do we reach the end of the query with the first seqcode in the edge? */
    if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
    {
        /* oooh, we have a longer edge, so check its contents before we follow it to the end */
        if(tn->tn_EdgeLen > 1)
        {
        STRPTR tarptr = &tn->tn_Edge[1];
        UBYTE tcode;
        while((tcode = *tarptr++))
            {
        /* check, if codes are the same (or N is found) */
        seqcodetree = pg->pg_CompressTable[tcode];
        seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];
        if(seqcode2 != seqcodetree)
        {
            sq->sq_State.sqs_ReplaceCount++;
            sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[seqcode2 * ALPHASIZE + seqcodetree]
                                           * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
            /* check, if more errors are tolerable. */
            if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
            {
                /* too many errors, do not recurse */
                ignore = TRUE;
                break;
            }
            if(!seqcodetree)
                {
                if(++sq->sq_State.sqs_NCount > sq->sq_KillNSeqsAt)
                    {
                    ignore = TRUE;
                    break;
                    }
                }
        }
        /* check if the end of the query string was reached */
        if(++sq->sq_State.sqs_QueryPos >= sq->sq_QueryLen)
        {
            break;
        }
        }
        }
    }
    /* are we allowed to go any further down the tree? */
        if(!ignore)
        {
            /* recurse */
            sq->sq_State.sqs_TreeNode = tn;
            if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
            {
            SearchTreeRec(sq);
            } else {
            CollectTreeRec(sq);
            }
        } else {
            if (PTPanGlobalPtr->pg_verbose >2)
            printf("%s(%f >= %f)\n", indent, sq->sq_State.sqs_ErrorCount, sq->sq_MaxErrors);
        }
        }
      }


      /* *** phase two: check for adding a character in the query string  */
      /* this will be only done, if the inserting operation is allowed and
         we are not at the very beginning nor the end of the query (because
    then it will found without insert operation anyway) */
      if(oldstate.sqs_QueryPos &&
    (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) &&
    sq->sq_AllowInsert)
    {
    ignore = FALSE;
    sq->sq_State = oldstate;
    /* increase error level by insert operation */
    sq->sq_State.sqs_InsertCount++;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Insert[seqcode];
    if(!seqcode)
    {
    if(++sq->sq_State.sqs_NCount > sq->sq_KillNSeqsAt)
    {
        sq->sq_State.sqs_ErrorCount = sq->sq_MaxErrors + 1.0;
    }
    }
    if(sq->sq_State.sqs_ErrorCount <= sq->sq_MaxErrors)
    {
    /* did we have a leaf? */
    if(leaf & LEAFMASK)
    {
        if(!leafadded) /* don't add the same leaf twice! */
        {
        AddQueryHit(sq, leaf & ~LEAFMASK);
        leafadded = TRUE;
        }
    } else {
        /* get the child node */
        if(!tn)
        {
        tn = GoDownNodeChild(oldtn, seqcode);

        if (PTPanGlobalPtr->pg_verbose >2) {
        // debug
        if(!tn)
        {
            printf("%s  Couldn't go down on %d!\n", indent, seqcode);
            break;
        }
        }
        }
        /* do we reach the end of the query with the first seqcode in the edge? */
        if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
        {
        /* oooh, we have a longer edge, so check its contents before we follow it to the end */
        if(tn->tn_EdgeLen > 1)
        {
    STRPTR tarptr = &tn->tn_Edge[1];
    UBYTE tcode;
    while((tcode = *tarptr++))
    {
    /* check, if codes are the same (or N is found) */
    seqcodetree = pg->pg_CompressTable[tcode];
    seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];
    if(seqcode2 != seqcodetree)
    {
        sq->sq_State.sqs_ReplaceCount++;
        sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[seqcode2 * ALPHASIZE + seqcodetree]
                                       * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
        /* check, if more errors are tolerable. */
        if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
        {
        /* too many errors, do not recurse */
        ignore = TRUE;
        break;
        }
        if(!seqcodetree)
        {
        if(++sq->sq_State.sqs_NCount > sq->sq_KillNSeqsAt)
        {
        ignore = TRUE;
        break;
        }
        }
    }
    /* check if the end of the query string was reached */
    if(++sq->sq_State.sqs_QueryPos >= sq->sq_QueryLen)
    {
        break;
    }
    }
        }
        }
        /* are we allowed to go any further down the tree? */
        if(!ignore)
        {
        /* recurse */
        sq->sq_State.sqs_TreeNode = tn;
        if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
        {
        SearchTreeRec(sq);
        } else {
        CollectTreeRec(sq);
        }
        }
    }
    }
      }

      /* *** phase three: check for deleting a character in the query string  */
      /* this will be only done, if we're not at the end of the string and
    the delete operation is allowed */
      if(oldstate.sqs_QueryPos &&
    (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) &&
    sq->sq_AllowDelete)
      {
    ignore = FALSE;
    sq->sq_State = oldstate;
    /* increase error level by delete operation */
    sq->sq_State.sqs_DeleteCount++;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Delete[pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]]];
    sq->sq_State.sqs_QueryPos++;
    if(sq->sq_State.sqs_ErrorCount <= sq->sq_MaxErrors)
    {
    /* check, if first seqcode of edge matches (N (seqcode 0) matches always) */
    seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];
    if(seqcode2 != seqcode)
    {
        sq->sq_State.sqs_ReplaceCount++;
        sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + seqcode]
                                       * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
        /* check, if more errors are tolerable. */
        if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
        {
        /* too many errors, do not recurse */
        ignore = TRUE;
        }
    }
    sq->sq_State.sqs_QueryPos++;
    } else {
    ignore = TRUE;
    }
    /* should we take a deeper look? */
    if(!ignore)
    {
    /* did we have a leaf? */
    if(leaf & LEAFMASK)
    {
        if(!leafadded) /* don't add the same leaf twice! */
        {
        AddQueryHit(sq, leaf & ~LEAFMASK);
        }
    } else {
        /* get the child node */
        if(!tn)
        {
        tn = GoDownNodeChild(oldtn, seqcode);
#if 0 // debug
        if(!tn)
        {
    printf("%s  Couldn't go down on %d!\n", indent, seqcode);
    break;
      }
#endif
    }
    /* do we reach the end of the query with the first seqcode in the edge? */
    if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
    {
      /* oooh, we have a longer edge, so check its contents before we follow it to the end */
      if(tn->tn_EdgeLen > 1)
      {
        STRPTR tarptr = &tn->tn_Edge[1];
        UBYTE tcode;
        while((tcode = *tarptr++))
        {
        /* check, if codes are the same (or N is found) */
        seqcodetree = pg->pg_CompressTable[tcode];
        seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];
        if(seqcode2 != seqcodetree)
        {
            sq->sq_State.sqs_ReplaceCount++;
            sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[seqcode2 * ALPHASIZE + seqcodetree]
                                           * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
            /* check, if more errors are tolerable. */
            if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
            {
            /* too many errors, do not recurse */
            ignore = TRUE;
            break;
            }
            if(!seqcodetree)
            {
            if(++sq->sq_State.sqs_NCount > sq->sq_KillNSeqsAt)
            {
            ignore = TRUE;
            break;
            }
            }
        }
        /* check if the end of the query string was reached */
        if(++sq->sq_State.sqs_QueryPos >= sq->sq_QueryLen)
        {
            break;
        }
        }
        }
        }
        /* are we allowed to go any further down the tree? */
        if(!ignore)
        {
        /* recurse */
        sq->sq_State.sqs_TreeNode = tn;
        if(sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
        {
        SearchTreeRec(sq);
        } else {
        CollectTreeRec(sq);
        }
        }
    }
    }
      }
      if(tn)
      {
    /* clean up */
    free(tn);
      }
    }
    /* restore possible altered data */
    sq->sq_State = oldstate;
  }
}
/* \\\ */

/* /// "CollectTreeRec()" */
void CollectTreeRec(struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  struct TreeNode *oldtn = sq->sq_State.sqs_TreeNode;
  struct TreeNode *tn;
  UWORD seqcode;
  ULONG cnt;
  ULONG *leafptr;
  ULONG leaf;

  const char* indent;
  if (PTPanGlobalPtr->pg_verbose >2) {
    const char* spaces = "                                            ";
    indent = spaces + strlen(spaces) - sq->sq_State.sqs_QueryPos;
  }

  /* collect leaves on our way down */
  if((cnt = oldtn->tn_NumLeaves))
  {
    /* collect hits */
    leafptr = oldtn->tn_Leaves;
    do
    {
      AddQueryHit(sq, *leafptr++);
    } while(--cnt);
    /* if we got leaves, we're at the end of the tree */
    return;
  }

  if (PTPanGlobalPtr->pg_verbose >2) {
    struct TreeNode *tn;
    printf("%s=> CollectTreeRec: Src/Qry %ld/%ld: ",
    indent,
    sq->sq_State.sqs_SourcePos, sq->sq_State.sqs_QueryPos);
    tn = sq->sq_State.sqs_TreeNode;
    do
    {
      printf("%s", tn->tn_Edge);
      tn = tn->tn_Parent;
      if (tn) printf("-");
    } while(tn);
    printf(" (%f/%d/%d/%d)\n",
    sq->sq_State.sqs_ErrorCount,
    sq->sq_State.sqs_ReplaceCount,
    sq->sq_State.sqs_InsertCount,
    sq->sq_State.sqs_DeleteCount);
  }

  /* recurse on children */
  for(seqcode = 0; seqcode < pg->pg_AlphaSize; seqcode++)
  {
    /* check, if we have a child */
    if((leaf = oldtn->tn_Children[seqcode]))
    {
      if (PTPanGlobalPtr->pg_verbose >2) {
    printf("%s>> CollectChild [%d] ", indent, seqcode);
    if(leaf & LEAFMASK)
    printf("LEAF %ld\n", leaf & ~LEAFMASK);
    else
    printf("%ld\n", leaf);
      }

      /* did we have a leaf? */
      if(leaf & LEAFMASK)
      {
    AddQueryHit(sq, leaf & ~LEAFMASK);
      } else {
    /* get the child node */
    //tn = GoDownNodeChild(oldtn, seqcode);
    tn = GoDownNodeChildNoEdge(oldtn, seqcode);
    /* recurse */
    sq->sq_State.sqs_TreeNode = tn;
    CollectTreeRec(sq);
    /* clean up */
    free(tn);
      }
    }
  }
  sq->sq_State.sqs_TreeNode = oldtn;
}
/* \\\ */

/* /// "MatchSequence()" */
BOOL MatchSequence(struct SearchQuery *sq)
{
  BOOL res;

  if (PTPanGlobalPtr->pg_verbose >0)
    printf(">> MatchSequence: Matching %s in %s\n", sq->sq_Query, sq->sq_SourceSeq);

  /* init */
  sq->sq_QueryLen = strlen(sq->sq_Query);
  sq->sq_State.sqs_SourcePos = 0;
  sq->sq_State.sqs_QueryPos = 0;
  sq->sq_State.sqs_ErrorCount = 0.0;
  sq->sq_State.sqs_ReplaceCount = 0;
  sq->sq_State.sqs_InsertCount = 0;
  sq->sq_State.sqs_DeleteCount = 0;
  res = MatchSequenceRec(sq);

  if (PTPanGlobalPtr->pg_verbose >0) {

    if (res) {
      printf("<< MatchSequence: yes (%f/%d/%d/%d)\n",
        sq->sq_State.sqs_ErrorCount,
        sq->sq_State.sqs_ReplaceCount,
        sq->sq_State.sqs_InsertCount,
        sq->sq_State.sqs_DeleteCount);
    } else {
      printf("<< MatchSequence:no\n");
    }
  }

  return(res);
}
/* \\\ */

/* /// "MatchSequenceRec()" */
BOOL MatchSequenceRec(struct SearchQuery *sq)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  UWORD seqcode;
  UWORD seqcode2;
  BOOL ignore;
  struct SearchQueryState oldstate;

  if(!(sq->sq_SourceSeq[sq->sq_State.sqs_SourcePos]))
  {
    return(TRUE); /* end of source sequence reached */
  }

  oldstate = sq->sq_State;
  seqcode  = pg->pg_CompressTable[sq->sq_SourceSeq[sq->sq_State.sqs_SourcePos]];
  seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];

  /* *** phase one: check 1:1 replacement; this considers the hamming distance */
  ignore = FALSE;
  /* check, if first seqcode of edge matches (N (seqcode 0) matches always) */
  if(seqcode2 != seqcode)
  {
    if(sq->sq_AllowReplace)
    {
      sq->sq_State.sqs_ReplaceCount++;
      sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + seqcode]
                                     * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
    } else {
      ignore = TRUE;
    }
  }
  /* check, if more errors are tolerable. */
  if(sq->sq_State.sqs_ErrorCount > sq->sq_MaxErrors)
  {
    /* too many errors, do not recurse */
    ignore = TRUE;
  }
  /* should we take a deeper look? */
  if(!ignore)
  {
    /* do we reach the end of the query? */
    if(++sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
    {
      /* recurse (actually, iterate) */
      ++sq->sq_State.sqs_SourcePos;
      if(MatchSequenceRec(sq))
      {
    return(TRUE);
      }
    } else {
      return(TRUE);
    }
  }

  /* *** phase two: check for adding a character in the query string  */
  /* this will be only done, if the inserting operation is allowed and
     we are not at the very beginning of the query (because then it will
     found without insert operation anyway) */
  if(oldstate.sqs_QueryPos && (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) && sq->sq_AllowInsert)
  {
    sq->sq_State = oldstate;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Insert[seqcode];
    sq->sq_State.sqs_InsertCount++;
    if(sq->sq_State.sqs_ErrorCount <= sq->sq_MaxErrors)
    {
      /* recurse */
      ++sq->sq_State.sqs_SourcePos;
      if(MatchSequenceRec(sq))
      {
    return(TRUE);
      }
    }
  }

  /* *** phase three: check for deleting a character in the query string */
  /* this will be only done, if we're neither at the beginning of the string
     nor the end of it and the delete operation is allowed */
  if(oldstate.sqs_QueryPos && (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) && sq->sq_AllowDelete)
  {
    sq->sq_State = oldstate;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Delete[seqcode2];
    sq->sq_State.sqs_DeleteCount++;
    if(sq->sq_State.sqs_ErrorCount <= sq->sq_MaxErrors)
    {
      ++sq->sq_State.sqs_QueryPos;
      /* recurse */
      if(MatchSequenceRec(sq))
      {
    return(TRUE);
      }
    }
  }
  /* restore possible altered data */
  sq->sq_State = oldstate;
  return(FALSE);
}
/* \\\ */

/* /// "FindSequenceMatch()" */
BOOL FindSequenceMatch(struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarstr)
{
  BOOL res;

  if (PTPanGlobalPtr->pg_verbose >0) {
    printf(">> FindSequenceMatch: Finding %s in %s [%ld] %s %f [%d/%d/%d] %s\n",
    sq->sq_Query,
    qh->qh_Species->ps_Name,
    qh->qh_AbsPos,
    sq->sq_SourceSeq,
    qh->qh_ErrorCount,
    qh->qh_ReplaceCount,
    qh->qh_InsertCount,
    qh->qh_DeleteCount,
    qh->qh_Flags & QHF_UNSAFE ? "(u)" : "(s)");
  }

  /* init */
  sq->sq_QueryLen = strlen(sq->sq_Query);
  sq->sq_State.sqs_SourcePos = 0;
  sq->sq_State.sqs_QueryPos = 0;
  sq->sq_State.sqs_ErrorCount = 0.0;
  sq->sq_State.sqs_ReplaceCount = 0;
  sq->sq_State.sqs_InsertCount = 0;
  sq->sq_State.sqs_DeleteCount = 0;
  res = FindSequenceMatchRec(sq, qh, tarstr);

  if (PTPanGlobalPtr->pg_verbose >0) {
    printf("<< FindSequenceMatch: ");

    if(res)
      {
    printf("yes %f [%d/%d/%d]\n",
        sq->sq_State.sqs_ErrorCount,
        sq->sq_State.sqs_ReplaceCount,
        sq->sq_State.sqs_InsertCount,
        sq->sq_State.sqs_DeleteCount);
      } else {
    printf("no\n");
      }
  }

  return(res);
}
/* \\\ */

/* /// "FindSequenceMatchRec()" */
BOOL FindSequenceMatchRec(struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarptr)
{
  struct PTPanGlobal *pg = sq->sq_PTPanGlobal;
  UWORD seqcode;
  UWORD seqcode2;
  BOOL ignore;
  struct SearchQueryState oldstate;
  STRPTR oldtarptr = tarptr;
  BOOL check;
  float misweight, maxweight;

  if(!(sq->sq_SourceSeq[sq->sq_State.sqs_SourcePos]))
  {
    return(TRUE); /* end of source sequence reached */
  }

  oldstate = sq->sq_State;
  seqcode = pg->pg_CompressTable[sq->sq_SourceSeq[sq->sq_State.sqs_SourcePos]];
  seqcode2 = pg->pg_CompressTable[sq->sq_Query[sq->sq_State.sqs_QueryPos]];

  /* *** phase one: check 1:1 replacement; this considers the hamming distance */
  ignore = FALSE;
  /* check, if first seqcode of edge matches (N (seqcode 0) matches always) */
  if(seqcode2 != seqcode)
  {
#ifdef ALLOWDOTSINMATCH
    if (sq->sq_SourceSeq[sq->sq_State.sqs_SourcePos] == '.')
    {
        sq->sq_State.sqs_ReplaceCount++;
        misweight = sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + SEQCODE_N]  // '.' in match
                    * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];                          // will be treated
        sq->sq_State.sqs_ErrorCount += misweight;                                           // as 'N'
        if(tarptr)
        {
          *tarptr++ = '.';
          *tarptr = 0;
        }
    } else 
#endif
    if(sq->sq_AllowReplace && (sq->sq_State.sqs_ReplaceCount < qh->qh_ReplaceCount))
    {
      sq->sq_State.sqs_ReplaceCount++;
      misweight = sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + seqcode]
                  * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
      sq->sq_State.sqs_ErrorCount += misweight;
      if(tarptr) /* write mismatching char */
      {
    if(seqcode) /* only for A, C, G, T codes, not for N */
    {
    maxweight = sq->sq_MismatchWeights->mw_Replace[(seqcode2 * ALPHASIZE) + pg->pg_ComplementTable[seqcode2]]
                * sq->sq_PosWeight[sq->sq_State.sqs_QueryPos];
    /* is it a great mismatch? */
    if(misweight > sq->sq_MinorMisThres)
    {
        /* output upper case letter */
        *tarptr++ = pg->pg_DecompressTable[seqcode];
    } else {
        /* output lower case letter */
        *tarptr++ = pg->pg_DecompressTable[seqcode]|0x20;
    }
    } else {
    *tarptr++ = pg->pg_DecompressTable[seqcode];
    }
    *tarptr = 0;
      }
    } else {
      ignore = TRUE;
    }
  } else {
    if(tarptr) /* write a '=' for a matching char */
    {
      *tarptr++ = '=';
      *tarptr = 0;
    }
  }
  /* check, if more errors are tolerable. */
  if(sq->sq_State.sqs_ErrorCount > qh->qh_ErrorCount)
  {
    /* too many errors, do not recurse */
    ignore = TRUE;
  }
  /* should we take a deeper look? */
  if(!ignore)
  {
    /* do we reach the end of the query? */
    if(++sq->sq_State.sqs_QueryPos < sq->sq_QueryLen)
    {
      /* recurse (actually, iterate) */
      ++sq->sq_State.sqs_SourcePos;
      check = FindSequenceMatchRec(sq, qh, tarptr);
    } else {
      check = TRUE;
    }
    if(check)
    {
      /* check error count */
      if((sq->sq_State.sqs_ErrorCount == qh->qh_ErrorCount) &&
    (sq->sq_State.sqs_ReplaceCount == qh->qh_ReplaceCount) &&
    (sq->sq_State.sqs_InsertCount == qh->qh_InsertCount) &&
    (sq->sq_State.sqs_DeleteCount == qh->qh_DeleteCount))
      {
    if (PTPanGlobalPtr->pg_verbose >1)
    printf("-- FindSequenceMatchRec: Found!\n");

    return(TRUE);
      }
    }
  }

  if (PTPanGlobalPtr->pg_verbose >1) {
    int i;
    for(i=0;i<50;i++) if (*(tarptr-i)=='-') {
      printf("  After replacements: %s %f [%d/%d/%d]\n",
        tarptr-i,
        sq->sq_State.sqs_ErrorCount,
        sq->sq_State.sqs_ReplaceCount,
        sq->sq_State.sqs_InsertCount,
        sq->sq_State.sqs_DeleteCount);
      break;
    }
    if (i==50)
      printf("-- FindSequenceMatchRec: ERROR: Start not found ?!\n");
  }

  /* *** phase two: check for adding a character in the query string  */
  /* this will be only done, if the inserting operation is allowed and
     we are not at the very beginning of the query (because then it will
     found without insert operation anyway) */
  if(oldstate.sqs_QueryPos && (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) &&
     sq->sq_AllowInsert &&
     (sq->sq_State.sqs_InsertCount < qh->qh_InsertCount))
  {
    tarptr = oldtarptr;
    sq->sq_State = oldstate;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Insert[seqcode];
    sq->sq_State.sqs_InsertCount++;
    if(sq->sq_State.sqs_ErrorCount <= qh->qh_ErrorCount)
    {
      /* recurse */
      ++sq->sq_State.sqs_SourcePos;
      if(tarptr) /* write mismatching char */
      {
        tarptr[-1] = '*';
        *tarptr = 0;
      }
      check = FindSequenceMatchRec(sq, qh, tarptr);
    } else {
      check = TRUE;
    }
    if(check)
    {
      /* check error count */
      if((sq->sq_State.sqs_ErrorCount == qh->qh_ErrorCount) &&
    (sq->sq_State.sqs_ReplaceCount == qh->qh_ReplaceCount) &&
    (sq->sq_State.sqs_InsertCount == qh->qh_InsertCount) &&
    (sq->sq_State.sqs_DeleteCount == qh->qh_DeleteCount))
      {
    if (PTPanGlobalPtr->pg_verbose >1)
    printf("-- FindSequenceMatchRec: Found!\n");

    return(TRUE);
      }
    }
  }

  if (PTPanGlobalPtr->pg_verbose >1) {
    int i;
    for(i=0;i<50;i++) if (*(tarptr-i)=='-') {
      printf("  After insertions:   %s %f [%d/%d/%d]\n",
        tarptr-i,
        sq->sq_State.sqs_ErrorCount,
        sq->sq_State.sqs_ReplaceCount,
        sq->sq_State.sqs_InsertCount,
        sq->sq_State.sqs_DeleteCount);
      break;
    }
    if (i==50)
      printf("-- FindSequenceMatchRec: ERROR: Start not found ?!\n");
  }

  /* *** phase three: check for deleting a character in the query string */
  /* this will be only done, if we're neither at the beginning of the string
     nor the end of it and the delete operation is allowed */
  if(oldstate.sqs_QueryPos && (oldstate.sqs_QueryPos + 1 < sq->sq_QueryLen) &&
     sq->sq_AllowDelete &&
     (sq->sq_State.sqs_DeleteCount < qh->qh_DeleteCount))
  {
    tarptr = oldtarptr;
    sq->sq_State = oldstate;
    sq->sq_State.sqs_ErrorCount += sq->sq_MismatchWeights->mw_Delete[seqcode2];
    sq->sq_State.sqs_DeleteCount++;
    if(sq->sq_State.sqs_ErrorCount <= qh->qh_ErrorCount)
    {
      ++sq->sq_State.sqs_QueryPos;
      /* recurse */
      if(tarptr) /* write char omission */
      {
        *tarptr++ = '_';
        *tarptr = 0;
      }
      check = FindSequenceMatchRec(sq, qh, tarptr);
    } else {
      check = TRUE;
    }
    if(check)
    {
      /* check error count */
      if((sq->sq_State.sqs_ErrorCount == qh->qh_ErrorCount) &&
         (sq->sq_State.sqs_ReplaceCount == qh->qh_ReplaceCount) &&
         (sq->sq_State.sqs_InsertCount == qh->qh_InsertCount) &&
         (sq->sq_State.sqs_DeleteCount == qh->qh_DeleteCount))
      {
        if (PTPanGlobalPtr->pg_verbose >1)
        printf("-- FindSequenceMatchRec: Found!\n");

        return(TRUE);
      }
    }
  }
  /* restore possible altered data */
  sq->sq_State = oldstate;

  if (PTPanGlobalPtr->pg_verbose >1) {
    int i;
    for(i=0;i<50;i++) if (*(tarptr-i)=='-') {
      printf("  After deletions:    %s %f [%d/%d/%d]\n",
        tarptr-i,
        sq->sq_State.sqs_ErrorCount,
        sq->sq_State.sqs_ReplaceCount,
        sq->sq_State.sqs_InsertCount,
        sq->sq_State.sqs_DeleteCount);
      break;
    }
    if (i==50)
      printf("-- FindSequenceMatchRec: ERROR: Start not found ?!\n");
  }

  return(FALSE);
}
/* \\\ */
