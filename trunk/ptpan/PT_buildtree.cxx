#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <unistd.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"

/* nice niffy macro to get a compressed sequence code */
#define GetSeqCodeQuick(pos) \
  (((seqptr[(pos) / MAXCODEFITLONG] \
      >> pg->pg_BitsShiftTable[MAXCODEFITLONG]) \
     / pg->pg_PowerTable[MAXCODEFITLONG - ((pos) % MAXCODEFITLONG) - 1]) \
    % pg->pg_AlphaSize)

#define GetSeqCode(pos) ((pos >= pg->pg_TotalRawSize) ? SEQCODE_N : GetSeqCodeQuick(pos))

/* /// "BuildStdSuffixTree()" */
BOOL BuildStdSuffixTree(struct PTPanGlobal *pg)
{
  STRPTR newtreename;
  struct PTPanPartition *pp;
  ULONG memfree;

  printf("********************************\n"
        "* Building Std Suffix Index... *\n"
        "********************************\n");
  // Delete old tree first (why, can't we just build a new one and
  // then rename it? Needs some extra disk space then though)
  if(unlink(pg->pg_IndexName))
  {
    if(GB_size_of_file(pg->pg_IndexName) >= 0)
    {
      fprintf(stderr, "Cannot remove %s\n", pg->pg_IndexName);
      return(FALSE);
    }
  }

  // allocate memory for a temporary filename
  newtreename = (STRPTR) malloc(strlen(pg->pg_IndexName) + 2);
  strcpy(newtreename, pg->pg_IndexName);
  strcat(newtreename, "~");

  pg->pg_IndexFile = fopen(newtreename, "w"); /* open file for output */
  if(!pg->pg_IndexFile)
  {
    fprintf(stderr, "Cannot open %s for output.\n", newtreename);
    free(newtreename);
    return(FALSE);
  }
  GB_set_mode_of_file(newtreename, 0666);

  //GB_begin_transaction(pg->pg_MainDB);

  /* build index */
  BuildMergedDatabase(pg);

  printf("Freeing alignment cache to save memory...");
  memfree = FlushCache(pg->pg_SpeciesCache);
  printf("%ld KB freed.\n", memfree >> 10);

  /* everything has to fit into one partition */
  pp = (struct PTPanPartition *) calloc(1, sizeof(struct PTPanPartition));
  if(!pp)
  {
    return(FALSE); /* out of memory */
  }

  /* fill in sensible values */
  pp->pp_PTPanGlobal = pg;
  pp->pp_ID = 0;
  pp->pp_Prefix = 0;
  pp->pp_PrefixLen = 0;
  pp->pp_Size = pg->pg_TotalRawSize;
  pp->pp_RawOffset = 0;
  pp->pp_PartitionName = (STRPTR) calloc(strlen(pg->pg_IndexName) + 5, 1);
  strncpy(pp->pp_PartitionName, pg->pg_IndexName, strlen(pg->pg_IndexName) - 3);
  strcat(pp->pp_PartitionName, "sfx");
  AddTail(&pg->pg_Partitions, &pp->pp_Node);
  pg->pg_NumPartitions = 1;
  printf("Using only one partition for %ld leaves.\n", pp->pp_Size);

  WriteIndexHeader(pg);
  fclose(pg->pg_IndexFile);

  BuildMemoryStdSuffixTree(pp);

  /* write out tree */
  printf(">>> Phase 3: Writing tree to secondary storage... <<<\n");
  WriteStdSuffixTreeToDisk(pp);

  printf(">>> Phase 4: Freeing memory and cleaning it up... <<<\n");

  /* return some memory not used anymore */
  free(pp->pp_StdSfxNodes);
  pp->pp_StdSfxNodes = NULL;

  if(GB_rename_file(newtreename, pg->pg_IndexName))
  {
    GB_print_error();
  }

  if(GB_set_mode_of_file(pg->pg_IndexName, 0666))
  {
    GB_print_error();
  }
  free(newtreename);
  return(TRUE);
}
/* \\\ */

/* /// "BuildMemoryStdSuffixTree()" */
BOOL BuildMemoryStdSuffixTree(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG oldheadnum;
  ULONG oldleafnum;
  ULONG nodecnt;
  //struct StdSfxNode *parnode;
  struct StdSfxNode *vnode;
  struct StdSfxNode *oldheadnode;
  struct StdSfxNode *oldleafnode;
  ULONG vnum, parnum;

  BenchTimePassed(pg);

  pp->pp_SfxMemorySize = pp->pp_Size * sizeof(struct StdSfxNode) * 2;
  pp->pp_StdSfxNodes = (struct StdSfxNode *) calloc(pp->pp_Size * 2, sizeof(struct StdSfxNode));
  if(!pp->pp_StdSfxNodes)
  {
    printf("Couldn't allocate %ld KB for suffix nodes.\n",
           pp->pp_SfxMemorySize >> 10);
    return(FALSE);
  }
  /* init pointing offsets */
  pp->pp_NumBigNodes = 0;

  printf("Allocated %ld KB suffix nodes buffer.\n", pp->pp_SfxMemorySize >> 10);

  /* fill in root node */
  vnode = pp->pp_StdSfxNodes;
  vnode->ssn_Parent = 0;
  vnode->ssn_FirstChild = 1;
  vnode++;
  vnode->ssn_Parent = 0;
  vnode->ssn_StartPos = 0;
  vnode->ssn_EdgeLen = pg->pg_TotalRawSize;
  vnode->ssn_FirstChild = 0;
  vnode->ssn_NextSibling = 0;
  vnum = 1;
  oldheadnum = 0;
  oldleafnum = 1;
  pp->pp_NumBigNodes = 2;

  /* main loop to build up the tree */
  /* NOTE: as a special precaution, all longwords have MAXCODEFITLONG code length */

  for(nodecnt = 1; nodecnt < pg->pg_TotalRawSize; nodecnt++)
  {
    ULONG gst, gend; // gamma
    ULONG bst, bend; // beta

    oldleafnode = &pp->pp_StdSfxNodes[oldleafnum];
    oldheadnode = &pp->pp_StdSfxNodes[oldheadnum];
    gst = oldleafnode->ssn_StartPos;
    gend = gst + oldleafnode->ssn_EdgeLen;

    //printf("%ld\n", nodecnt);

    //cerr << "Step " << i << ": oldleaf(" << source.substr(gst, gend - gst) << ")->" << source.substr(i, n - i) << endl;
    //printTree(source, r, 0);

    if(!oldheadnum) // oldhead ist wurzel
    {
      //printf("  oldhead == root\n");
      //vnode = pp->pp_StdSfxNodes;
      vnum = 0;
      gst++;
    }
    else if((vnum = oldheadnode->ssn_Prime)) // oldhead ist nicht Wurzel hat aber einen Querlink
    {
      //printf("  oldhead has nodeprime\n");
    } else { // oldhead ist nicht Wurzel hat aber keinen Querlink
      parnum = oldheadnode->ssn_Parent;
      bst = oldheadnode->ssn_StartPos;
      bend = bst + oldheadnode->ssn_EdgeLen;
      //cerr << "  oldhead(" << source.substr(bst, bend - bst) << ") has no nodeprime and is not root" << endl;
      if(!parnum)
      {
        //printf("    p was root\n");
        bst++;
        vnum = FastFindStdSfxNode(pp, 0, bst, bend);
      } else {
        vnum = FastFindStdSfxNode(pp, (pp->pp_StdSfxNodes[parnum]).ssn_Prime, bst, bend);
        //printf("    p->nodeprime was leaf\n");
        /*cerr << "    p->nodeprime was leaf("
          << source.substr(u->sfxstart, u->sfxend - u->sfxstart) << endl;*/
      }
      /*cerr << "  Call to fastFindNode(., ., " << bst << ", " << bend
        << ", '" << source.substr(bst, bend - bst) << "')" << endl;*/
      oldheadnode->ssn_Prime = vnum;
    }
    /*cerr << "  Call to findNode(., ., " << gst << ", " << gend
      << ", '" << source.substr(gst, gend - gst) << "')" << endl;*/
    //printf("FindNode %ld, %ld-%ld\n", vnum, gst, gend);
    oldheadnum = FindStdSfxNode(pp, vnum, gst, gend);
    /*cerr << "  Call to insertNode(., " << gst << ", " << gend
      << ", '" << source.substr(gst, gend - gst) << "')" << endl;*/
    //printf("InsertNode %ld, %ld-%ld\n", oldheadnum, gst, gend);
    oldleafnum = InsertStdSfxNode(pp, gst, gend, oldheadnum);
    if((nodecnt & 0x3fff) == 0)
    {
      if((nodecnt >> 14) % 50)
      {
        printf(".");
        fflush(stdout);
      } else {
        printf(". %2ld%%\n", nodecnt / (pg->pg_TotalRawSize / 100));
      }
    }
  }
  printf("DONE! (%ld KB unused)\n", (pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset) >> 10);

  printf("Nodes     : %6ld\n", pp->pp_NumBigNodes);
  pg->pg_Bench.ts_MemTree += BenchTimePassed(pg);
  return(TRUE);
}
/* \\\ */

// Suche nach bestimmter Kindkante (konstante Zeit, da alphabetgroesse konstant).
/* /// "FindStdSfxChildNode()" */
inline
ULONG FindStdSfxChildNode(struct PTPanPartition *pp, ULONG nodenum, ULONG pos)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG c = GetSeqCodeQuick(pos);
  struct StdSfxNode *node;

  //printf("Searching %ld at %ld in node %ld: ", c, pos, nodenum);
  nodenum = (pp->pp_StdSfxNodes[nodenum]).ssn_FirstChild;
  while(nodenum)
  {
    node = &pp->pp_StdSfxNodes[nodenum];
    if(GetSeqCodeQuick(node->ssn_StartPos) == c)
    {
      break;
    }
    nodenum = node->ssn_NextSibling;
  }
  //printf("%ld\n", nodenum);
  return(nodenum);
}
/* \\\ */

/* /// "SplitStdSfxNode()" */
// Neue Node einfuegen und initialisieren
ULONG SplitStdSfxNode(struct PTPanPartition *pp, ULONG leafnum)
{
  struct StdSfxNode *leafnode = &pp->pp_StdSfxNodes[leafnum];
  ULONG parnum = leafnode->ssn_Parent;
  struct StdSfxNode *parnode = &pp->pp_StdSfxNodes[parnum];
  ULONG inum = pp->pp_NumBigNodes++;
  struct StdSfxNode *inode = &pp->pp_StdSfxNodes[inum];
  struct StdSfxNode *tmpnode;

  //printf("Split node %ld\n", leafnum);
  if(parnode->ssn_FirstChild == leafnum) // case 1: leaf is first child
  {
    // correct all linkages
    parnode->ssn_FirstChild = inum;
  } else { // case 2 leaf is some other child
    // find previous sibling of leaf
    tmpnode = &pp->pp_StdSfxNodes[parnode->ssn_FirstChild];
    while(tmpnode->ssn_NextSibling != leafnum)
    {
      tmpnode = &pp->pp_StdSfxNodes[tmpnode->ssn_NextSibling];
    }
    // correct all linkages
    tmpnode->ssn_NextSibling = inum;
  }
  inode->ssn_FirstChild = leafnum;
  inode->ssn_NextSibling = leafnode->ssn_NextSibling;
  inode->ssn_Parent = parnum;
  leafnode->ssn_NextSibling = 0;
  leafnode->ssn_Parent = inum;
  return(inum);
}
/* \\\ */

/* /// "FindStdSfxNode()" */
// langsames Finden der naechsten Node (wir wissen nicht, ob diese existiert)
ULONG FindStdSfxNode(struct PTPanPartition *pp, ULONG snum, ULONG &sfxstart, ULONG sfxend)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG *seqptr = pg->pg_MergedRawData;
  //struct StdSfxNode *snode;

  //printf("FindStdSfxNode: %ld, (%ld-%ld)\n", snum, sfxstart, sfxend);
  if(sfxstart == sfxend) // terminal character is always found
  {
    return(snum);
  }
  //snode = &pp->pp_StdSfxNodes[snum];
  do
  {
    ULONG i,ic;
    ULONG fst, fend;
    struct StdSfxNode *leafnode;
    struct StdSfxNode *inode;
    ULONG inum;
    ULONG leafnum;

    leafnum = FindStdSfxChildNode(pp, snum, sfxstart);
    if(!leafnum)
    {
      return(snum);
    }
    leafnode = &pp->pp_StdSfxNodes[leafnum];
    fst = leafnode->ssn_StartPos;
    fend = fst + leafnode->ssn_EdgeLen;

    // Zeichen fuer Zeichen ueberpruefen
    for(i = fst+1, ic = sfxstart+1; i < fend; i++, ic++)
    {
      if(i-fst+sfxstart >= sfxend) // Suchstringende erreicht -> "$" Blatt
        break;
      if(GetSeqCodeQuick(i) != GetSeqCodeQuick(ic)) // Zeichendifferenz?
        break;
    }
    //printf("i: %ld\n", i);
    if(i != fend) // Muss leaf gesplitted werden?
    {
      inum = SplitStdSfxNode(pp, leafnum); // Generate new node
      inode = &pp->pp_StdSfxNodes[inum];
      inode->ssn_StartPos = fst; // neues Kind hat selben Suffixstart
      inode->ssn_EdgeLen = i - fst; // aber Suffix endet zwischen den beiden
      leafnode->ssn_StartPos = i; // leaf hat neuen Suffixstart bei i
      leafnode->ssn_EdgeLen = fend - i; // leaf Suffixend bleibt unveraendert
      /*printf("Inode %ld (%ld-%ld), leafnode %ld (%ld-%ld)\n",
        inum, fst, i, leafnum, i, fend);*/
      sfxstart += i - fst; // Suffixstart fuer insertNode() korrigieren
      return(inum);
    } else { // Knoten vollstaendig gematcht
      snum = leafnum;
      sfxstart += fend - fst;
    }
  } while(TRUE);
}
/* \\\ */

/* /// "FastFindStdSfxNode()" */
ULONG FastFindStdSfxNode(struct PTPanPartition *pp, ULONG snum, ULONG sfxstart, ULONG sfxend)
{
  //struct StdSfxNode *snode;

  /* fast finding of next node (we know that it has to exist) */

  if(sfxstart == sfxend)
  {
    return(snum);
  }
  do
  {
    ULONG fst, fend;
    ULONG i;
    struct StdSfxNode *leafnode;
    struct StdSfxNode *inode;
    ULONG inum;
    ULONG leafnum;

    leafnum = FindStdSfxChildNode(pp, snum, sfxstart);
    if(!leafnum)
    {
      printf("Shit!\n");
      exit(1);
    }
    leafnode = &pp->pp_StdSfxNodes[leafnum];
    i = leafnode->ssn_EdgeLen;

    if(sfxstart + i == sfxend) // do we terminate at a leaf?
    {
      return(leafnum);
    }
    fst = leafnode->ssn_StartPos;
    fend = fst + i;
    if(sfxstart + i > sfxend) // Bleiben wir innerhalb des Blattes haengen?
    {
      inum = SplitStdSfxNode(pp, leafnum); // Generate new node
      inode = &pp->pp_StdSfxNodes[inum];
      inode->ssn_StartPos = fst; // neues Kind hat selben Suffixstart
      inode->ssn_EdgeLen = sfxend - sfxstart; // aber Suffix endet zwischen den beiden
      leafnode->ssn_StartPos = fst + (sfxend - sfxstart); // leaf hat neuen Suffixstart bei i
      leafnode->ssn_EdgeLen = fend - leafnode->ssn_StartPos; // leaf Suffixend bleibt unveraendert
      return(inum);
    }
    sfxstart += i;
    snum = leafnum;
  } while(TRUE);
}
/* \\\ */

/* /// "InsertStdSfxNode()" */
ULONG InsertStdSfxNode(struct PTPanPartition *pp, ULONG sfxstart, ULONG sfxend, ULONG parnum)
{
  struct StdSfxNode *parnode = &pp->pp_StdSfxNodes[parnum];
  ULONG inum = pp->pp_NumBigNodes++;
  struct StdSfxNode *inode = &pp->pp_StdSfxNodes[inum];
  ULONG tmpnum;
  struct StdSfxNode *tmpnode;

  inode->ssn_Parent = parnum;
  inode->ssn_StartPos = sfxstart;
  inode->ssn_EdgeLen = sfxend - sfxstart;
  if((tmpnum = parnode->ssn_FirstChild))
  {
    tmpnode = &pp->pp_StdSfxNodes[tmpnum];
    while((tmpnum = tmpnode->ssn_NextSibling))
    {
      tmpnode = &pp->pp_StdSfxNodes[tmpnum];
    }
    tmpnode->ssn_NextSibling = inum;
  } else {
    parnode->ssn_FirstChild = inum;
  }
  return(inum);
}
/* \\\ */

/* /// "WriteStdSuffixTreeToDisk()" */
BOOL WriteStdSuffixTreeToDisk(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct StdSfxNode *ssn;
  struct StdSfxNodeOnDisk *ssndisk;
  ULONG cnt;
  ULONG buffill;
  ULONG chunksize = 1UL<<20;
  ULONG pval;
  ULONG *seqptr;
  UWORD lcnt;
  STRPTR tarseq;

  pg->pg_Bench.ts_Reloc += BenchTimePassed(pg);
  /* now finally write it to disk */
  pp->pp_PartitionFile = fopen(pp->pp_PartitionName, "w");
  if(!pp->pp_PartitionFile)
  {
    printf("ERROR: Couldn't open partition file %s for writing!\n",
        pp->pp_PartitionName);
    return(FALSE);
  }

  pp->pp_DiskTreeSize = pp->pp_NumBigNodes * sizeof(struct StdSfxNodeOnDisk);
  pp->pp_DiskBufferSize = sizeof(StdSfxNodeOnDisk) * chunksize;
  pp->pp_DiskBuffer = (UBYTE *) calloc(1, pp->pp_DiskBufferSize);
  pp->pp_DiskPos = 0;

  WriteStdSuffixTreeHeader(pp);

  printf("Writing tree (%ld KB)",pp->pp_DiskTreeSize >> 10);
  fflush(NULL);
  cnt = pp->pp_NumBigNodes;
  ssn = pp->pp_StdSfxNodes;
  do
  {
    ssndisk = (struct StdSfxNodeOnDisk *) pp->pp_DiskBuffer;
    buffill = 0;
    do
    {
      ssndisk->ssn_StartPos = ssn->ssn_StartPos;
      ssndisk->ssn_EdgeLen = ssn->ssn_EdgeLen;
      ssndisk->ssn_FirstChild = ssn->ssn_FirstChild;
      ssndisk->ssn_NextSibling = ssn->ssn_NextSibling;
      ssndisk++;
      ssn++;
      cnt--;
    } while((++buffill < chunksize) && cnt);

    printf(".");
    fflush(NULL);
    fwrite(pp->pp_DiskBuffer, sizeof(struct StdSfxNodeOnDisk) * buffill, 1, pp->pp_PartitionFile);
  } while(cnt);
  printf(".\n");
  printf("Writing raw text (%ld KB)",pp->pp_DiskTreeSize >> 10);
  cnt = (pg->pg_TotalRawSize / MAXCODEFITLONG) + 1;
  seqptr = pg->pg_MergedRawData;
  tarseq = (STRPTR) pp->pp_DiskBuffer;
  pp->pp_DiskPos = 0;
  do
  {
    /* get next longword */
    pval = *seqptr++;
    lcnt = MAXCODEFITLONG;
    pval >>= pg->pg_BitsShiftTable[MAXCODEFITLONG];
    /* unpack compressed longword */
    do
    {
      *tarseq++ = (pval / pg->pg_PowerTable[--lcnt]) % pg->pg_AlphaSize;
    } while(lcnt);
    pp->pp_DiskPos += MAXCODEFITLONG;
    if(pp->pp_DiskPos > pp->pp_DiskBufferSize - MAXCODEFITLONG)
    {
      fwrite(pp->pp_DiskBuffer, pp->pp_DiskPos, 1, pp->pp_PartitionFile);
      tarseq = (STRPTR) pp->pp_DiskBuffer;
      pp->pp_DiskPos = 0;
    }
  } while(--cnt);
  pp->pp_DiskIdxSpace = ftell(pp->pp_PartitionFile);
  fclose(pp->pp_PartitionFile);
  free(pp->pp_DiskBuffer);

  pg->pg_Bench.ts_Writing += BenchTimePassed(pg);

  return(TRUE);
}
/* \\\ */

/* /// "BuildPTPanIndex()" */
/* build a whole new fresh and tidy index (main routine) */
BOOL BuildPTPanIndex(struct PTPanGlobal *pg)
{
  STRPTR newtreename;
  struct PTPanPartition *pp;
  ULONG memfree;

  printf("********************************\n"
        "* Building new PT Pan Index... *\n"
        "********************************\n");
  // Delete old tree first (why, can't we just build a new one and
  // then rename it? Needs some extra disk space then though)
  if(unlink(pg->pg_IndexName))
  {
    if(GB_size_of_file(pg->pg_IndexName) >= 0)
    {
      fprintf(stderr, "Cannot remove %s\n", pg->pg_IndexName);
      return(FALSE);
    }
  }

  // allocate memory for a temporary filename
  newtreename = (STRPTR) malloc(strlen(pg->pg_IndexName) + 2);
  strcpy(newtreename, pg->pg_IndexName);
  strcat(newtreename, "~");

  pg->pg_IndexFile = fopen(newtreename, "w"); /* open file for output */
  if(!pg->pg_IndexFile)
  {
    fprintf(stderr, "Cannot open %s for output.\n", newtreename);
    free(newtreename);
    return(FALSE);
  }
  GB_set_mode_of_file(newtreename, 0666);

  //GB_begin_transaction(pg->pg_MainDB);

  /* build index */
  BuildMergedDatabase(pg);

  printf("Freeing alignment cache to save memory...");
  memfree = FlushCache(pg->pg_SpeciesCache);
  printf("%ld KB freed.\n", memfree >> 10);

  PartitionPrefixScan(pg);

  WriteIndexHeader(pg);
  fclose(pg->pg_IndexFile);

  /* build tree for each partition */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  while(pp->pp_Node.ln_Succ)
  {
    CreateTreeForPartition(pp);
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
  }
  //CreatePartitionLookup(pg);

  //GB_commit_transaction(pg->pg_MainDB);

  //if(GB_rename_file(newtreename, pg->pg_IndexName)) *** FIXME ***
  if(GB_rename_file(newtreename, pg->pg_IndexName))
  {
    GB_print_error();
  }

  if(GB_set_mode_of_file(pg->pg_IndexName, 0666))
  {
    GB_print_error();
  }
  free(newtreename);
  return(TRUE);
}
/* \\\ */

/* /// "BuildMergedDatabase()" */
BOOL BuildMergedDatabase(struct PTPanGlobal *pg)
{
  struct PTPanSpecies *ps;
  ULONG *seqptr;
  ULONG seqcode;
  ULONG cnt;
  ULONG pval;
  ULONG len;
  BOOL dbopen = FALSE;
  ULONG verlen;
#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
  ULONG relpos;
#endif  
  ULONG abspos;
  ULONG specabspos;
  ULONG hash;

  BenchTimePassed(pg);

  /* allocate memory for compressed data */
  /* about the +3: 1 for rounding, 1 for MAXCODEFITLONG*SEQCODE_N
     and 1 for terminal */
  pg->pg_MergedRawData = (ULONG *) malloc(((pg->pg_TotalRawSize /
                         MAXCODEFITLONG) + 3) * sizeof(ULONG));
  if(!pg->pg_MergedRawData)
  {
    printf("Sorry, couldn't allocate %ld KB of memory for the compressed DB!\n",
           (((pg->pg_TotalRawSize / MAXCODEFITLONG) + 3) * sizeof(ULONG)) >> 10);
    return(FALSE);
  }

  /* init */
  seqptr = pg->pg_MergedRawData;
  cnt = 0;
  pval = 0;
  len = 4;
  abspos = 0;

  /* note: This has to be modified a bit to support compressed databases of >2GB
     alignment data using pp->pp_RawPartitionOffset */

  printf("Step 1: Building compressed database...\n");
  /* doing a linear scan -- caching is useless */
  DisableCache(pg->pg_SpeciesCache);

#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
  /* traverse species to find good ChkPntIVal */
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    /* as long as the interval is less than the square root of the data */
    ps->ps_ChkPntIVal = 128;
    while(((ps->ps_ChkPntIVal * ps->ps_ChkPntIVal) < ps->ps_RawDataSize) &&
        (ps->ps_ChkPntIVal < 65536))
    {
      ps->ps_ChkPntIVal <<= 1;
    }
    ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
  }
#endif

  /* traverse all species */
  ps = (struct PTPanSpecies *) pg->pg_Species.lh_Head;
  while(ps->ps_Node.ln_Succ)
  {
    /* compress sequence */
    STRPTR srcstr;
    UBYTE code;

#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
    if(!ps->ps_SeqData)
    {
      /* seems like the sequence is not in memory due to low-memory mode */
      if(!dbopen) /* open database */
      {
        GB_begin_transaction(pg->pg_MainDB);
        dbopen = TRUE;
      }
      /* load alignment data */
      ps->ps_CacheNode = CacheLoadData(pg->pg_SpeciesCache, ps->ps_CacheNode, ps);
    }
#endif

    if(abspos != ps->ps_AbsOffset)
    {
      /* species seems to be corrupt! */
      printf("AbsPos %ld != %ld mismatch at %s\n",
        abspos, ps->ps_AbsOffset, ps->ps_Name);
    }


#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
    /* allocate checkpoint buffer */                // TODO: remove Checkpoints for COMPRESSSEQUENCEWITHDOTSANDHYPHENS
    ps->ps_NumCheckPoints = ps->ps_RawDataSize / ps->ps_ChkPntIVal;
    ps->ps_CheckPoints = (ULONG *) calloc(ps->ps_NumCheckPoints, sizeof(ULONG));
    if(!ps->ps_CheckPoints)
    {
      printf("Out of memory for checkpoint interval table!\n");
    }
    /* now actually compress the sequence */
    srcstr = ps->ps_SeqData;
#endif
    verlen = 0;
    hash = 0;
    specabspos = 0;
#ifdef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
    ULONG bitpos = 0;
    ULONG count;
    while((code = GetNextCharacter(pg, ps->ps_SeqDataCompressed, bitpos, count)) != 0xff)
#else
    relpos = 0;
    while((code = *srcstr++))
#endif
    {
      if(pg->pg_SeqCodeValidTable[code])
      {
        /* add sequence code */
        if(verlen++ < ps->ps_RawDataSize)
        {
#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
          /* write checkpoint location */
          if(!(specabspos % ps->ps_ChkPntIVal) && specabspos)
          {
            //printf("%ld, ", relpos);
            ps->ps_CheckPoints[(specabspos / ps->ps_ChkPntIVal)-1] = relpos;
          }
#endif          
          abspos++;
          specabspos++;
          seqcode = pg->pg_CompressTable[code];
          pval *= pg->pg_AlphaSize;
          pval += seqcode;
          /* calculate hash */
          hash *= pg->pg_AlphaSize;
          hash += seqcode;
          hash %= HASHPRIME;
          /* check, if storage capacity was reached? */
          if(++cnt == MAXCODEFITLONG)
          {
            /* write out compressed longword (with eof bit) */
            //printf("[%08lx]", pval | pg->pg_BitsMaskTable[cnt]);
            *seqptr++ = (pval << pg->pg_BitsShiftTable[cnt]) | pg->pg_BitsMaskTable[cnt];
            cnt = 0;
            pval = 0;
            len += 4;
          }
        }
      }
#ifndef COMPRESSSEQUENCEWITHDOTSANDHYPHENS
      relpos++;
#endif      
    }
    if(verlen != ps->ps_RawDataSize)
    {
      printf("Len %ld != %ld mismatch with %s\n",
        verlen, ps->ps_RawDataSize, ps->ps_Name);
      printf("Please check if this alignment is somehow corrupt!\n");
    }
    //printf("\n");
    ps->ps_SeqHash = hash;
    ps = (struct PTPanSpecies *) ps->ps_Node.ln_Succ;
  }

  /* write pending bits (with eof bit) */
  /* after a lot of experimenting, padding with SEQCODE_N is the only sensible thing
     to keep code size down and reduce cases */
  *seqptr++ = ((pval * pg->pg_PowerTable[MAXCODEFITLONG - cnt])
               << pg->pg_BitsShiftTable[MAXCODEFITLONG])
    | pg->pg_BitsMaskTable[MAXCODEFITLONG];
  //printf("[%08lx]\n", seqptr[-1]);

  /* add a final padding longword with SEQCODE_N */
  *seqptr++ = pg->pg_BitsMaskTable[MAXCODEFITLONG];

  /* and a terminating bit */
  *seqptr = pg->pg_BitsMaskTable[0];

  if(dbopen) /* close DB, if open */
  {
    GB_commit_transaction(pg->pg_MainDB);
  }

  /* Enable caching again */
  EnableCache(pg->pg_SpeciesCache);

#if 0 /* debug */
  {
    FILE *fh;
    char tmpbuf[80];
    STRPTR str;

    sprintf(tmpbuf, "%s.raw", pg->pg_IndexName);
    str = DecompressSequence(pg, pg->pg_MergedRawData);
    fh = fopen(tmpbuf, "w");
    fputs(str, fh);
    fclose(fh);
    free(str);
  }
#endif

  /* calculate hash sum over all data */
  //pg->pg_AllHashSum = GetSeqHash(pg, 0, pg->pg_TotalRawSize, 0);
  /* formerly a global hash, now only a random key to check integrity
     with other files. Database up2date state is checked with sequence
     hashes */
  pg->pg_AllHashSum = rand();
  pg->pg_Bench.ts_MergeDB = BenchTimePassed(pg);

  printf("Merged compressed database size: %ld KB\n", len >> 10);
  return(TRUE);
}
/* \\\ */

/* /// "PartitionPrefixScan() */
BOOL PartitionPrefixScan(struct PTPanGlobal *pg)
{
  struct PTPanPartition *pp;
  ULONG cnt;
  ULONG prefix;
  ULONG seqpos;
  ULONG *seqptr;
  ULONG pval;
  ULONG maxpartblock;
  ULONG partsize;
  ULONG leftpresize;
  ULONG rightpresize;
  ULONG prefixstart;
  UWORD partcnt;

  BenchTimePassed(pg);
  printf("Step 2: Partition calculation...\n");
  if(pg->pg_TotalRawSize < pg->pg_MaxPartitionSize)
  {
    /* everything fits into one partition */
    pp = (struct PTPanPartition *) calloc(1, sizeof(struct PTPanPartition));
    if(!pp)
    {
      return(FALSE); /* out of memory */
    }

    /* fill in sensible values */
    pp->pp_PTPanGlobal = pg;
    pp->pp_ID = 0;
    pp->pp_Prefix = 0;
    pp->pp_PrefixLen = 0;
    pp->pp_Size = pg->pg_TotalRawSize;
    pp->pp_RawOffset = 0;
    pp->pp_PartitionName = (STRPTR) calloc(strlen(pg->pg_IndexName) + 5, 1);
    strncpy(pp->pp_PartitionName, pg->pg_IndexName, strlen(pg->pg_IndexName) - 2);
    strcat(pp->pp_PartitionName, "t000");
    AddTail(&pg->pg_Partitions, &pp->pp_Node);
    pg->pg_NumPartitions = 1;
    printf("Using only one partition for %ld leaves.\n", pp->pp_Size);
    return(TRUE);
  }
  if(!pg->pg_MergedRawData) /* safe checking */
  {
    printf("Huh? No merged raw data. Blame your programmer NOW!\n");
    return(FALSE);
  }

  /* make histogram */
  pg->pg_HistoTable = (ULONG *) calloc(pg->pg_PowerTable[MAXPREFIXSIZE], sizeof(ULONG));
  if(!pg->pg_HistoTable)
  {
    printf("Out of memory for histogram!\n");
    return(FALSE);
  }

  /* NOTE: ordering for the index of a prefix in the table is:
     c_{m} + c_{m-1} * 5 + ... + c_{1} * 5^{m-1}
     This means that the last part of the prefix has the lowest
     significance. */

  /* scan through the compressed database */
  printf("Scanning through compact data...\n");
  seqptr = pg->pg_MergedRawData;
  prefix = 0;
  cnt = 0;
  pval = 0;
  for(seqpos = 0; seqpos < pg->pg_TotalRawSize + MAXPREFIXSIZE - 1; seqpos++)
  {
    /* get sequence code from packed data */
    if(!cnt)
    {
      pval = *seqptr++;
      cnt = GetCompressedLongSize(pg, pval);
      pval >>= pg->pg_BitsShiftTable[cnt];
    }

    /* generate new prefix code */
    prefix %= pg->pg_PowerTable[MAXPREFIXSIZE - 1];
    prefix *= pg->pg_AlphaSize;
    prefix += (pval / pg->pg_PowerTable[--cnt]) % pg->pg_AlphaSize;

    /* increase histogram value */
    if(seqpos >= MAXPREFIXSIZE - 1)
    {
      pg->pg_HistoTable[prefix]++;
    }
  }

  /* generate partitions */
  cnt = 0;
  prefixstart = 0;
  maxpartblock = pg->pg_PowerTable[MAXPREFIXSIZE];
  partsize = 0;
  partcnt = 0;
  pg->pg_MaxPrefixLen = 0;
  for(prefix = 0; prefix < pg->pg_PowerTable[MAXPREFIXSIZE];)
  {
    partsize++;
    cnt += pg->pg_HistoTable[prefix];
    if((cnt > pg->pg_MaxPartitionSize) || (partsize >= maxpartblock))
    {
      /* partition is full! */
      if(prefixstart == prefix)
      {
        printf("Warning: Partition overflow! Increase MAXPREFIXSIZE!\n");
        break;
      } else {
        ULONG ppos;
        /* check first, if we had to partition the thing anyway */
        if(partsize < maxpartblock)
        {
          /* find out, how many leaves of the tree can be merged */
          for(leftpresize = 0; leftpresize < MAXPREFIXSIZE; leftpresize++)
          {
            if(((prefixstart / pg->pg_PowerTable[leftpresize]) %
                pg->pg_AlphaSize) != 0)
            {
              break;
            }
          }
          /* check, if we're still in the same block */
          if(prefix / pg->pg_PowerTable[leftpresize] ==
             prefixstart / pg->pg_PowerTable[leftpresize])
          {
            for(rightpresize = 0; rightpresize <= leftpresize; rightpresize++)
            {
              if(prefixstart + pg->pg_PowerTable[rightpresize] > prefix)
              {
                break;
              }
            }
            rightpresize--;
            /* setup maxpartblock for all subpartions */
            maxpartblock = pg->pg_PowerTable[rightpresize];
            prefix = prefixstart + maxpartblock - 1;
          } else {
            /* we crossed the boundary, do last partition */
            prefix = prefixstart + pg->pg_PowerTable[leftpresize] - 1;
            maxpartblock = pg->pg_PowerTable[MAXPREFIXSIZE] - prefix - 1;
          }
        } else {
          /* we had to split the tree before, so this is just
             another leaf */
          if((prefix + 1) % (maxpartblock * pg->pg_AlphaSize) == 0)
          {
            /* we have reached the last block, go back to the
               normal search */
            maxpartblock = pg->pg_PowerTable[MAXPREFIXSIZE] - prefix - 1;
          }
        }
        //printf("New range: %ld - %ld\n", prefixstart, prefix);
        /* recalculate leaf count */
        cnt = 0;
        for(ppos = prefixstart; ppos <= prefix; ppos++)
        {
          cnt += pg->pg_HistoTable[ppos];
        }

        /* don't create empty trees! */
        if(cnt)
        {
          /* get prefix length */
          for(leftpresize = 1; leftpresize < MAXPREFIXSIZE; leftpresize++)
          {
            if((prefix - prefixstart) < pg->pg_PowerTable[leftpresize])
            {
              break;
            }
          }
          pp = (struct PTPanPartition *) calloc(1, sizeof(struct PTPanPartition));
          if(!pp)
          {
            return(FALSE); /* out of memory */
          }

          /* fill in sensible values */
          pp->pp_PTPanGlobal = pg;
          pp->pp_ID = partcnt++;
          pp->pp_Prefix = prefix / pg->pg_PowerTable[leftpresize];
          pp->pp_PrefixLen = MAXPREFIXSIZE - leftpresize;
          pp->pp_Size = cnt;
      /* this is the point where you would change some things to support larger
         raw datasets than 2 GB */
      pp->pp_RawOffset = 0; /* FIXME */
      pp->pp_PartitionName = (STRPTR) calloc(strlen(pg->pg_IndexName) + 5, 1);
      strncpy(pp->pp_PartitionName, pg->pg_IndexName, strlen(pg->pg_IndexName) - 2);
      sprintf(&pp->pp_PartitionName[strlen(pg->pg_IndexName) - 2], "t%03ld",
      pp->pp_ID);
          AddTail(&pg->pg_Partitions, &pp->pp_Node);
          /* check, if we need a bigger prefix table */
          if(pp->pp_PrefixLen > pg->pg_MaxPrefixLen)
          {
            pg->pg_MaxPrefixLen = pp->pp_PrefixLen;
          }
#ifdef DEBUG
    {
        STRPTR tarseq = pg->pg_TempBuffer;
        ULONG ccnt = pp->pp_PrefixLen;
        do
        {
        *tarseq++ = pg->pg_DecompressTable[(pp->pp_Prefix / pg->pg_PowerTable[--ccnt])
                                                        % pg->pg_AlphaSize];
        } while(ccnt);
        *tarseq = 0;
        printf("New partition %ld - %ld, size %ld, prefix = %ld, prefixlen = %ld %s\n",
        prefixstart, prefix, cnt, pp->pp_Prefix, pp->pp_PrefixLen,
        pg->pg_TempBuffer);
    }
#endif
        } else {
#ifdef DEBUG
          printf("Empty partition %ld - %ld\n", prefixstart, prefix);
#endif
        }
        prefix++;
        /* restart */
        prefixstart = prefix;
        cnt = 0;
        partsize = 0;
      }
    } else {
      prefix++;
    }
  }
  free(pg->pg_HistoTable);
  pg->pg_NumPartitions = partcnt;
  printf("Using %d partitions.\n", partcnt);
  pg->pg_Bench.ts_PrefixScan = BenchTimePassed(pg);
  return(TRUE);
}
/* \\\ */

/* /// "CreateTreeForPartition()" */
BOOL CreateTreeForPartition(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;

  printf("*** Partition %ld/%d: %ld nodes (%ld%%) ***\n",
    pp->pp_ID + 1, pg->pg_NumPartitions,
    pp->pp_Size, pp->pp_Size / (pg->pg_TotalRawSize / 100));

  printf(">>> Phase 1: Building up suffix tree in memory... <<<\n");
  BuildMemoryTree(pp);

  /* prepare tree building */
  printf(">>> Phase 2: Calculating tree statistical data... <<<\n");
  CalculateTreeStats(pp);

  /* write out tree */
  printf(">>> Phase 3: Writing tree to secondary storage... <<<\n");
  WriteTreeToDisk(pp);

  printf(">>> Phase 4: Freeing memory and cleaning it up... <<<\n");

  /* return some memory not used anymore */
  free(pp->pp_SfxNodes);
  free(pp->pp_BranchCode);
  free(pp->pp_ShortEdgeCode);
  free(pp->pp_LongEdgeLenCode);
  free(pp->pp_LongDictRaw);
  free(pp->pp_LeafBuffer);
  pp->pp_SfxNodes = NULL;
  pp->pp_LevelStats = NULL;
  pp->pp_BranchCode = NULL;
  pp->pp_ShortEdgeCode = NULL;
  pp->pp_LongEdgeLenCode = NULL;
  pp->pp_LongDictRaw = NULL;
  pp->pp_LeafBuffer = NULL;
  pp->pp_LeafBufferSize = 0;
  return(TRUE);
}
/* \\\ */

/* /// "BuildMemoryTree()" */
BOOL BuildMemoryTree(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG *seqptr ;
  ULONG pval, pvalnext;
  ULONG cnt;
  ULONG nodecnt;
  ULONG pos;
  ULONG len;
  ULONG window;

  BenchTimePassed(pg);
  /* setup node buffer:
     some notes about the node organisation: nodes are kept in a partly
     compressed way in memory already: they only store the number of edges they
     need. Therefore the big node buffer is split into two parts:
     - Nodes with three to five edges starting from the first offset and increasing
     - Nodes with two edges starting from the very end and decreasing
     - Leaf nodes are stored directly as offset inside a node with the MSB indicating
       that this ptr is a leaf.
  */
  /* this is based on empirical data: Big Nodes are approx 8-15%, small nodes 60-90% */
  pp->pp_SfxMemorySize = ((((pp->pp_Size / 100) * SMALLNODESPERCENT) *
                           sizeof(struct SfxNode2Edges)) +
                          (((pp->pp_Size / 100) * BIGNODESPERCENT) *
                           sizeof(struct SfxNodeNEdges)) + (4UL << 20)) & ~3;
  if(pp->pp_SfxMemorySize >= (1UL << 30))
  {
    pp->pp_SfxMemorySize = (1UL << 30)-4;
    printf("Warning! Memory limited to 1 GB! Might run out!\n");
  }
  pp->pp_SfxNodes = (UBYTE *) malloc(pp->pp_SfxMemorySize);
  if(!pp->pp_SfxNodes)
  {
    printf("Couldn't allocate %ld KB for suffix nodes.\n",
           pp->pp_SfxMemorySize >> 10);
    return(FALSE);
  }
  /* init pointing offsets */
  pp->pp_SfxNEdgeOffset = 0;
  pp->pp_Sfx2EdgeOffset = pp->pp_SfxMemorySize;

  printf("Allocated %ld KB suffix nodes buffer.\n", pp->pp_SfxMemorySize >> 10);

  /* fill in root node */
  pp->pp_SfxRoot = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_SfxNEdgeOffset];
  pp->pp_SfxRoot->sn_Parent = 0;
  pp->pp_SfxRoot->sn_StartPos = 0;//pos;
  pp->pp_SfxRoot->sn_EdgeLen = 0;//pg->pg_TotalRawSize - len;
  memset(pp->pp_SfxRoot->sn_Children, 0, pg->pg_AlphaSize * sizeof(ULONG));
  pp->pp_SfxNEdgeOffset += sizeof(struct SfxNodeNEdges);

  /* main loop to build up the tree */
  /* NOTE: as a special precaution, all longwords have MAXCODEFITLONG code length */

  /* allocate quick lookup table. This is used to speed up traversal of the
     MAXQPREFIXLOOKUPSIZE lowest levels */
  pp->pp_QuickPrefixLookup = (ULONG *) calloc(pg->pg_PowerTable[MAXQPREFIXLOOKUPSIZE],
                                              sizeof(ULONG));
  if(!pp->pp_QuickPrefixLookup)
  {
    printf("Out of memory for Quick Prefix Lookup!\n");
    return(FALSE);
  }

  len = pg->pg_TotalRawSize;
  seqptr = pg->pg_MergedRawData;
  /* get first longword */
  pval = *seqptr++;
  pval >>= pg->pg_BitsShiftTable[MAXCODEFITLONG];
  cnt = MAXCODEFITLONG;
  /* get second longword */
  pvalnext = *seqptr++;
  pvalnext >>= pg->pg_BitsShiftTable[MAXCODEFITLONG];
  pos = 0;
  nodecnt = 0;
  pp->pp_QuickPrefixCount = 0;
  len -= MAXCODEFITLONG;
  do
  {
    BOOL takepos;

    window = ((pval % pg->pg_PowerTable[cnt]) *
              pg->pg_PowerTable[MAXCODEFITLONG - cnt]) +
      (pvalnext / pg->pg_PowerTable[cnt]);
    if(pp->pp_PrefixLen)
    {
      /* check, if the prefix matches */
      takepos = (window / pg->pg_PowerTable[MAXCODEFITLONG - pp->pp_PrefixLen] == pp->pp_Prefix);
    } else {
      takepos = TRUE; /* it's all one big partition */
    }

    if(takepos) /* only add this position, if it matches the prefix */
    {
      if(!(InsertTreePos(pp, pos, window)))
      {
        break;
      }
      if((++nodecnt & 0x3fff) == 0)
      {
        if((nodecnt >> 14) % 50)
        {
          printf(".");
          fflush(stdout);
        } else {
          printf(". %2ld%% (%6ld KB free)\n",
                 pos / (pg->pg_TotalRawSize / 100),
                 (pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset) >> 10);
        }
      }
    }

    /* get next byte */
    cnt--;
    if(len)
    {
      if(!cnt)
      {
        /* get next position */
        pval = pvalnext;
        pvalnext = *seqptr++;
        pvalnext >>= pg->pg_BitsShiftTable[MAXCODEFITLONG];
        cnt = MAXCODEFITLONG;
      }
      len--;
    } else {
      if(!cnt)
      {
        pval = pvalnext;
        pvalnext = 0;
        cnt = MAXCODEFITLONG;
      }
    }
  } while(++pos < pg->pg_TotalRawSize);
  pp->pp_NumBigNodes = pp->pp_SfxNEdgeOffset / sizeof(struct SfxNodeNEdges);
  pp->pp_NumSmallNodes = (pp->pp_SfxMemorySize - pp->pp_Sfx2EdgeOffset) /
    sizeof(struct SfxNode2Edges);

  printf("DONE! (%ld KB unused)\n", (pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset) >> 10);

  /* free some memory not required anymore */
  free(pp->pp_QuickPrefixLookup);
  pp->pp_QuickPrefixLookup = NULL;

  printf("Quick Prefix Lookup speedup: %ld%% (%ld)\n",
         (pp->pp_QuickPrefixCount * 100) / pp->pp_Size, pp->pp_QuickPrefixCount);

  if(pp->pp_Size != nodecnt)
  {
    printf("Something very bad has happened! Predicted partition size [%ld] didn't\n"
           "match the actual generated nodes [%ld].\n",
           pp->pp_Size, nodecnt);
    return(FALSE);
  }

  printf("Nodes     : %6ld\n", nodecnt);
  printf("SmallNodes: %6ld (%ld%%)\n",
         pp->pp_NumSmallNodes,
         (pp->pp_NumSmallNodes * 100) / nodecnt);
  printf("BigNodes  : %6ld (%ld%%)\n",
         pp->pp_NumBigNodes,
         (pp->pp_NumBigNodes * 100) / nodecnt);

  pg->pg_Bench.ts_MemTree += BenchTimePassed(pg);
  return(TRUE);
}
/* \\\ */

/* /// "CommonSequenceLength()" */
ULONG CommonSequenceLength(struct PTPanPartition *pp,
                           ULONG spos1, ULONG spos2, ULONG maxlen)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG off1 = spos1 / MAXCODEFITLONG;
  ULONG cnt1 = spos1 % MAXCODEFITLONG;
  ULONG off2 = spos2 / MAXCODEFITLONG;
  ULONG cnt2 = spos2 % MAXCODEFITLONG;
  ULONG len = 0;

  /* a note on the implementation: this routine will not work correctly,
     if the sequences are identical or completely of SEQCODE_N, as it
     doesn't detect the end of sequence bits, but assumes MAXCODEFITLONG
     entries for all longwords. */
  /* compare one code */
  while((((seqptr[off1] >> pg->pg_BitsShiftTable[MAXCODEFITLONG]) /
         pg->pg_PowerTable[MAXCODEFITLONG - cnt1 - 1]) % pg->pg_AlphaSize) ==
       (((seqptr[off2] >> pg->pg_BitsShiftTable[MAXCODEFITLONG]) /
         pg->pg_PowerTable[MAXCODEFITLONG - cnt2 - 1]) % pg->pg_AlphaSize))
  {
    /* loop while code is identical */
    len++;
    if(len >= maxlen)
    {
      break;
    }
    if(++cnt1 >= MAXCODEFITLONG)
    {
      cnt1 = 0;
      off1++;
    }
    if(++cnt2 >= MAXCODEFITLONG)
    {
      cnt2 = 0;
      off2++;
    }
  }
  return(len);
}
/* \\\ */

/* /// "CompareCompressedSequence()" */
LONG CompareCompressedSequence(struct PTPanGlobal *pg, ULONG spos1, ULONG spos2)
{
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG off1 = spos1 / MAXCODEFITLONG;
  ULONG cnt1 = spos1 % MAXCODEFITLONG;
  ULONG off2 = spos2 / MAXCODEFITLONG;
  ULONG cnt2 = spos2 % MAXCODEFITLONG;
  ULONG window1, window2;

  /* a note on the implementation: this routine will not work correctly,
     if the sequences are identical or completely of SEQCODE_N, as it
     doesn't detect the end of sequence bits, but assumes MAXCODEFITLONG
     entries for all longwords. */
  do
  {
    /* get windows */
    /* I know this looks like very obfusciated code, but take your time and you will
       understand: take a few codebits from the one longword, add a few bits from
       the other */
    window1 = (((seqptr[off1] >> pg->pg_BitsShiftTable[MAXCODEFITLONG])
                % pg->pg_PowerTable[MAXCODEFITLONG - cnt1]) * pg->pg_PowerTable[cnt1])
      + ((seqptr[off1 + 1] >> pg->pg_BitsShiftTable[MAXCODEFITLONG]) /
         pg->pg_PowerTable[MAXCODEFITLONG - cnt1]);
    window2 = (((seqptr[off2] >> pg->pg_BitsShiftTable[MAXCODEFITLONG])
                % pg->pg_PowerTable[MAXCODEFITLONG - cnt2]) * pg->pg_PowerTable[cnt2])
      + ((seqptr[off2 + 1] >> pg->pg_BitsShiftTable[MAXCODEFITLONG]) /
         pg->pg_PowerTable[MAXCODEFITLONG - cnt2]);

    /* compare the windows */
    if(window1 == window2)
    {
      /* was the same, look at next position */
      if(++cnt1 >= MAXCODEFITLONG)
      {
        cnt1 = 0;
        off1++;
      }
      if(++cnt2 >= MAXCODEFITLONG)
      {
        cnt2 = 0;
        off2++;
      }
    } else {
      return(((LONG) window1) - ((LONG) window2));
    }
  } while(TRUE);
  return(0); /* never reached */
}
/* \\\ */

/* /// "InsertTreePos()" */
BOOL InsertTreePos(struct PTPanPartition *pp, ULONG pos, ULONG window)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode = pp->pp_SfxRoot;
  struct SfxNode *prevnode;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG relptr;
  ULONG len;
  UBYTE seqcode;
  ULONG childptr;
  UWORD childidx, childcnt;
  UWORD previdx;
  BOOL childisleaf;
  ULONG prefix;
  ULONG treepos;

  prefix = window / pg->pg_PowerTable[MAXCODEFITLONG - MAXQPREFIXLOOKUPSIZE];
  /* see if we can use a quick lookup to skip the root levels of the tree */
  if((childptr = pp->pp_QuickPrefixLookup[prefix]) &&
    (pos + MAXQPREFIXLOOKUPSIZE < pg->pg_TotalRawSize))
  {
    pp->pp_QuickPrefixCount++;
    sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[childptr];
    pos += MAXQPREFIXLOOKUPSIZE;
    treepos = MAXQPREFIXLOOKUPSIZE;
  } else {
    treepos = 0;
  }
  len = pg->pg_TotalRawSize - pos;
  while(len)
  {
    /* get first sequence code */
    seqcode = GetSeqCodeQuick(pos);

    /*printf("[%ld%c] ", pos, //((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes),
      pg->pg_DecompressTable[seqcode]);*/

    /* check, if there's already a child */
    relptr = 0;
    childidx = sfxnode->sn_Parent >> RELOFFSETBITS;

    while(childidx--)
    {
      childptr = sfxnode->sn_Children[childidx];
      if(childptr >> LEAFBIT)
      {
        /* this is a leaf pointer and doesn't contain a seqcode */
        childptr &= ~LEAFMASK;
    childptr += treepos;
        //printf("<%c>", pg->pg_DecompressTable[GetSeqCodeQuick(childptr)]);
        if(GetSeqCodeQuick(childptr) == seqcode)
        {
          /* fill in a dummy relptr -- we'll use childptr later */
          relptr = ~0UL;
          childisleaf = TRUE;
          break;
        }
      } else {
        //printf("[%c]", pg->pg_DecompressTable[childptr >> RELOFFSETBITS]);
        if((childptr >> RELOFFSETBITS) == seqcode)
        {
          /* hey, we actually found the right child */
          relptr = (childptr & RELOFFSETMASK) << 2;
          childisleaf = FALSE;
          break;
        }
      }
    }
    /* did we find a child? */
    if(relptr)
    {
      ULONG matchsize;

      if(childisleaf)
      {
        struct SfxNode *splitnode;
        /* relptr is no pointer to a node, but a startpos instead */
        matchsize = CommonSequenceLength(pp, pos, childptr, pg->pg_TotalRawSize - childptr);

        /* this will always lead to partial matches! */

        /* allocate a new branching node */
        pp->pp_Sfx2EdgeOffset -= sizeof(struct SfxNode2Edges);

        /*printf("Leaf split (pos %ld, len %ld): %d (%c != %c) -> [%ld]\n",
               pos, len, matchsize,
               pg->pg_DecompressTable[GetSeqCodeQuick(childptr + matchsize)],
               pg->pg_DecompressTable[GetSeqCodeQuick(pos + matchsize)],
               pp->pp_Sfx2EdgeOffset);*/

        splitnode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_Sfx2EdgeOffset];
        /* fill in the node with the two leaves */
        splitnode->sn_Parent = ((((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes)) >> 2) |
          (2 << RELOFFSETBITS);
        splitnode->sn_StartPos = childptr;
        splitnode->sn_EdgeLen = matchsize;
        splitnode->sn_Children[0] = LEAFMASK | (childptr - treepos);
        splitnode->sn_Children[1] = LEAFMASK | (pos - treepos);
    /*printf("Child0 = %ld, Child1 = %ld\n",
        splitnode->sn_Children[0] & ~LEAFMASK,
        splitnode->sn_Children[1] & ~LEAFMASK);*/
#if 0 // debug
    if(GetSeqCodeQuick(childptr + matchsize) == GetSeqCodeQuick(pos + matchsize))
    {
    printf("CIS: %ld<->%ld [%ld|%ld] (matchsize %ld)\n",
        GetSeqCodeQuick(childptr + matchsize),
        GetSeqCodeQuick(pos + matchsize),
        childptr,
        pos,
        matchsize);
    }
#endif
        /* fix downlink (child) pointer */
        sfxnode->sn_Children[childidx] = (seqcode << RELOFFSETBITS) | (pp->pp_Sfx2EdgeOffset >> 2);
        break;
      }
      //printf("->[%ld]", relptr);
      /* okay, there is a child, get it */
      prevnode = sfxnode;
      previdx = childidx; /* is needed for correcting the downlink ptr */
      sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[relptr];
      /* compare its edge */
      if(sfxnode->sn_EdgeLen > 1)
      {
        matchsize = CommonSequenceLength(pp, pos, sfxnode->sn_StartPos, sfxnode->sn_EdgeLen);
      } else {
        matchsize = 1;
      }
      if(matchsize < sfxnode->sn_EdgeLen) /* did the whole edge match? */
      {
        struct SfxNode *upnode;
        //printf("Partmatch(%ld, %ld): %d\n", pos, len, matchsize);
        /* we only had a partial match, we need to split the node */

        /* allocate a new node */
        pp->pp_Sfx2EdgeOffset -= sizeof(struct SfxNode2Edges);
        upnode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_Sfx2EdgeOffset];

        /* fix linkage of middle node and set two edges */
        upnode->sn_Parent = ((((ULONG) prevnode) - ((ULONG) pp->pp_SfxNodes)) >> 2) |
          (2 << RELOFFSETBITS);
        upnode->sn_StartPos = sfxnode->sn_StartPos;
        upnode->sn_EdgeLen = matchsize;
        upnode->sn_Children[0] = ((((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes)) >> 2) |
          (GetSeqCodeQuick(upnode->sn_StartPos + matchsize) << RELOFFSETBITS);
        /* enter child leaf node */
        upnode->sn_Children[1] = LEAFMASK | (pos - treepos);

#if 0 // debug
    if(GetSeqCodeQuick(upnode->sn_StartPos + matchsize) == GetSeqCodeQuick(pos + matchsize))
    {
    printf("SN: %ld<->%ld\n",
        GetSeqCodeQuick(upnode->sn_StartPos + matchsize),
        GetSeqCodeQuick(pos + matchsize));
    }
#endif

        /* fix sfxnode linkage and edge */
        sfxnode->sn_Parent = (pp->pp_Sfx2EdgeOffset >> 2) |
          (sfxnode->sn_Parent & ~RELOFFSETMASK);
        sfxnode->sn_StartPos += matchsize;
        sfxnode->sn_EdgeLen -= matchsize;

        /* fix prevnode */
        prevnode->sn_Children[childidx] = (pp->pp_Sfx2EdgeOffset >> 2) |
          (prevnode->sn_Children[childidx] & ~RELOFFSETMASK);

        if(pp->pp_SfxNEdgeOffset >= pp->pp_Sfx2EdgeOffset)
        {
          printf("Node buffer was too small!\n");
          return(FALSE);
        }
        break;
      } else {
        /* the whole edge matched, just follow the path */
        /*printf("Wholematch(%ld, %ld): %d [%d] -> %ld\n",
          pos, len, matchsize, seqcode, (((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes)));*/
        pos += matchsize;
        len -= matchsize;
        treepos += matchsize;
      }
    } else {
      childcnt = (sfxnode->sn_Parent >> RELOFFSETBITS);
      /*printf("New leaf[%d]-(pos %ld, len %ld): (%c)\n",
        childcnt, pos, len, pg->pg_DecompressTable[seqcode]);*/
      if((childcnt == 2) && (sfxnode != pp->pp_SfxRoot))
      {
        struct SfxNode *bignode;
        struct SfxNode *lastnode;

        /* we need to expand this node from 2 to 5 branches first */
        /*printf("2T5 [%ld]->[%ld] [%ld]->[%ld]\n",
               ((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes),
               pp->pp_SfxNEdgeOffset,
               pp->pp_Sfx2EdgeOffset,
               ((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes));*/

        /* allocate a new big node */
        bignode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_SfxNEdgeOffset];

        /* copy node */
        memcpy(bignode, sfxnode, sizeof(struct SfxNode2Edges));

        /* fix prevnode -> bignode child pointer */
        prevnode->sn_Children[previdx] = (pp->pp_SfxNEdgeOffset >> 2) |
          (prevnode->sn_Children[previdx] & ~RELOFFSETMASK);

        /* fix children -> bignode parent pointers */
        childidx = 2; //(bignode->sn_Parent >> RELOFFSETBITS);
        while(childidx--)
        {
          childptr = bignode->sn_Children[childidx];
          if(!(childptr >> LEAFBIT)) /* only check for real nodes */
          {
            relptr = (childptr & RELOFFSETMASK) << 2;
            //printf("Fixup childidx=%d from %ld\n", childidx, relptr);
            /* fix the pointer to new location */
            prevnode = (struct SfxNode *) &pp->pp_SfxNodes[relptr];
            prevnode->sn_Parent = (prevnode->sn_Parent & ~RELOFFSETMASK) |
              (((ULONG) bignode) - ((ULONG) pp->pp_SfxNodes) >> 2);
          }
        }

        /* avoid copy, if both are the same */
        if(pp->pp_Sfx2EdgeOffset !=
           ((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes))
        {
          /* regain memory by copying last two edge node into hole */
          lastnode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_Sfx2EdgeOffset];

          /* copy node */
          memcpy(sfxnode, lastnode, sizeof(struct SfxNode2Edges));

          /* find lastnode->parent->lastnode downward pointer */
          prevnode = (struct SfxNode *) &pp->pp_SfxNodes[(lastnode->sn_Parent & RELOFFSETMASK) << 2];
          childidx = (prevnode->sn_Parent >> RELOFFSETBITS);
          while(childidx--)
          {
            childptr = prevnode->sn_Children[childidx];
            if(!(childptr >> LEAFBIT)) /* only check for real nodes */
            {
              if((childptr & RELOFFSETMASK) == (pp->pp_Sfx2EdgeOffset >> 2))
              {
                /* fix the pointer to new location */
                /*printf("Fixdown childidx=%d from %ld\n",
                  childidx, lastnode->sn_Parent & RELOFFSETMASK);*/
                prevnode->sn_Children[childidx] = (childptr & ~RELOFFSETMASK) |
                  (((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes) >> 2);
                break;
              }
            }
          }

          /* fix children->parent upward pointers */
          childidx = 2;//(lastnode->sn_Parent >> RELOFFSETBITS);
          while(childidx--)
          {
            childptr = lastnode->sn_Children[childidx];
            if(!(childptr >> LEAFBIT)) /* only check for real nodes */
            {
              relptr = (childptr & RELOFFSETMASK) << 2;
              //printf("Fixup childidx=%d from %ld\n", childidx, relptr);
              /* fix the pointer to new location */
              prevnode = (struct SfxNode *) &pp->pp_SfxNodes[relptr];
              prevnode->sn_Parent = (prevnode->sn_Parent & ~RELOFFSETMASK) |
                (((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes) >> 2);
            }
          }
        }
        /* we're done with the fixing, now correct memory usage */
        sfxnode = bignode;
        pp->pp_Sfx2EdgeOffset += sizeof(struct SfxNode2Edges);
        pp->pp_SfxNEdgeOffset += sizeof(struct SfxNodeNEdges);
        if(treepos == MAXQPREFIXLOOKUPSIZE)
        {
          pp->pp_QuickPrefixLookup[prefix] = ((ULONG) sfxnode) - ((ULONG) pp->pp_SfxNodes);
        }
      }

      /* enter new child */
      sfxnode->sn_Children[childcnt] = LEAFMASK | (pos - treepos);
      //printf("New child: %ld (%ld)\n", pos-treepos, treepos);
      /* increase edgecount */
      sfxnode->sn_Parent += (1UL << RELOFFSETBITS);
      break;
    }
  }
  //printf("Done (2E: %ld NE: %ld)\n", pp->pp_Sfx2EdgeOffset, pp->pp_SfxNEdgeOffset);
  return(TRUE);
}
/* \\\ */

/* /// "CalculateTreeStats()" */
BOOL CalculateTreeStats(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG cnt;
  ULONG edgetotal;
  ULONG bitsused;
  ULONG threshold;

  BenchTimePassed(pg);
  /* now we need some statistical data. this includes:
     a) the maximum depth of the tree
     b) the number of nodes and leaves on each level (i.e. population)
        out of b) we will generate the TREE PRUNING DEPTH
     c) the appearance of all branch combinations (branch combo mask) up to the pruning depth
        out of c) we will generate a HUFFMAN CODE for the branch ptr mask
     d) the occurrences of the short edges codes and the number of long edges
        out of d) we will generate a HUFFMAN CODE for the edge codes
        moreover, it replaces the short edges by the indexes to the huffman code (sn_StartPos
        is recycled, gets bit 30 set).
     e) generating a HUFFMAN CODE fo the long edge lengths
     f) generating a lookup DICTIONARY for the long edge codes
        f) will allow storing shorter label pointers and only a fraction of memory for lookup
        long edges will be replaced by the position in the dictionary (sn_StartPos is recycled,
        gets bit 31 set).
  */
  //memset(pp->pp_EdgeBranchTable, 0, ALPHASIZE * sizeof(ULONG));

#if 0 /* debug */
  pp->pp_VerifyArray = (UBYTE *) calloc(pg->pg_TotalRawSize+1, 1);

  /* verify the correctness of the tree */
  printf("Verifying correctness of the tree...\n");
  GetTreeStatsVerifyRec(pp, 0, 0, 0);

  for(cnt = 0; cnt < ((pg->pg_TotalRawSize + 7) >> 3); cnt++)
  {
    if(pp->pp_VerifyArray[cnt] != 0xff)
    {
        printf("Hole in Position %ld to %ld (%02x)\n",
        cnt << 3, (cnt << 3)+7, pp->pp_VerifyArray[cnt]);
    } else {
      /*printf("Good in Position %ld to %ld (%02x)\n",
    cnt << 3, (cnt << 3)+7, pp->pp_VerifyArray[cnt]);*/
    }
  }
  free(pp->pp_VerifyArray);
#endif

  /* calculate maximum depth of the tree */
  pp->pp_MaxTreeDepth = 0;
  GetTreeStatsTreeDepthRec(pp, 0, 0);
  pp->pp_MaxTreeDepth++; /* increase by one due to leaf level */

  printf("Max tree depth: %ld\n", pp->pp_MaxTreeDepth);

  /* generate statistical data, part 2: level population */
  pp->pp_LevelStats = (struct TreeLevelStats *) calloc(pp->pp_MaxTreeDepth,
                                sizeof(struct TreeLevelStats));

  GetTreeStatsLevelRec(pp, 0, 0);

  /* calculate missing total values and pruning position */
  if(pg->pg_PruneLength)
  {
    pp->pp_TreePruneDepth = pg->pg_PruneLength;
    pp->pp_TreePruneLength = pg->pg_PruneLength;
  } else {
    pp->pp_TreePruneDepth = 20; /* FIXME */
    pp->pp_TreePruneLength = 20; /* FIXME */
  }
  cnt = pp->pp_MaxTreeDepth;
  while(cnt--)
  {
    pp->pp_LevelStats[cnt].tls_TotalLeafCount = pp->pp_LevelStats[cnt].tls_LeafCount;
    pp->pp_LevelStats[cnt].tls_TotalNodeCount = pp->pp_LevelStats[cnt].tls_NodeCount;
    if(cnt < pp->pp_MaxTreeDepth-1)
    {
      pp->pp_LevelStats[cnt].tls_TotalLeafCount += pp->pp_LevelStats[cnt+1].tls_TotalLeafCount;
      pp->pp_LevelStats[cnt].tls_TotalNodeCount += pp->pp_LevelStats[cnt+1].tls_TotalNodeCount;

      /* calculate tree pruning depth. currently, will prune at 66% of the leaves
    covered */
      if(!pp->pp_TreePruneDepth)
      {
    if(pp->pp_LevelStats[cnt].tls_TotalLeafCount > pp->pp_Size / 3)
    {
    pp->pp_TreePruneDepth = cnt;
    }
      }
    }
  }

  /* debug output */
#if 0
  for(cnt = 0; cnt < pp->pp_MaxTreeDepth; cnt++)
  {
    printf("Level %3ld: Nodes=%6ld, Leaves=%6ld, TotalNodes=%6ld, TotalLeaves=%6ld\n",
    cnt,
    pp->pp_LevelStats[cnt].tls_NodeCount,
    pp->pp_LevelStats[cnt].tls_LeafCount,
    pp->pp_LevelStats[cnt].tls_TotalNodeCount,
    pp->pp_LevelStats[cnt].tls_TotalLeafCount);
  }
#endif

  printf("Tree pruning at depth %ld, length %ld.\n",
    pp->pp_TreePruneDepth,
    pp->pp_TreePruneLength);

  free(pp->pp_LevelStats);
  pp->pp_LevelStats = NULL;

  /* allocate branch histogram */
  pp->pp_BranchCode = (struct HuffCode *) calloc(1UL << pg->pg_AlphaSize,
                    sizeof(struct HuffCode));
  if(!pp->pp_BranchCode)
  {
    printf("Out of memory for Branch Histogram!\n");
    return(FALSE);
  }
  GetTreeStatsBranchHistoRec(pp, 0, 0, 0);

  /* generate statistical data, part 3: edge lengths and combinations */

  /* allocate short edge code histogram (for edges between of 2-7 base pairs) */
  bitsused = pg->pg_BitsUseTable[SHORTEDGEMAX]+1;
  pp->pp_ShortEdgeCode = (struct HuffCode *) calloc((1UL << bitsused),
                        sizeof(struct HuffCode));
  if(!pp->pp_ShortEdgeCode)
  {
    printf("Out of memory for Short Edge Histogram!\n");
    return(FALSE);
  }

  /* get short edge stats */
  pp->pp_EdgeCount = 0;
  pp->pp_ShortEdgeCode[1].hc_Weight++;
  GetTreeStatsShortEdgesRec(pp, 0, 0, 0);

  /* define threshold for small edge optimization */
  threshold = pp->pp_EdgeCount / 10000;

  /* calculate the number of longedges required */
  edgetotal = pp->pp_EdgeCount;
  for(cnt = 1; cnt < (1UL << bitsused); cnt++)
  {
    if(pp->pp_ShortEdgeCode[cnt].hc_Weight > threshold) /* code will remain in the table */
    {
      edgetotal -= pp->pp_ShortEdgeCode[cnt].hc_Weight;
    }
  }
  /* code 0 will be used for long edges */
  pp->pp_ShortEdgeCode[0].hc_Weight = edgetotal;

  printf("Considering %ld (%ld+%ld) edges for the final tree.\n",
    pp->pp_EdgeCount, pp->pp_EdgeCount - edgetotal, edgetotal);

  /* generate huffman code for short edge, but only for codes that provide at least
     1/10000th of the edges (other stuff goes into the long edges) */
  printf("Generating huffman code for short edges\n");
  BuildHuffmanCode(pp->pp_ShortEdgeCode, (1UL << bitsused), threshold);

#if 0
  /* debug */
  for(cnt = 0; cnt < (1UL << bitsused); cnt++)
  {
    WORD bitcnt;
    if(pp->pp_ShortEdgeCode[cnt].hc_CodeLength)
    {
      printf("%6ld: %7ld -> %2d ", cnt, pp->pp_ShortEdgeCode[cnt].hc_Weight,
        pp->pp_ShortEdgeCode[cnt].hc_CodeLength);
      for(bitcnt = pp->pp_ShortEdgeCode[cnt].hc_CodeLength - 1; bitcnt >= 0; bitcnt--)
      {
    printf("%s", pp->pp_ShortEdgeCode[cnt].hc_Codec & (1UL << bitcnt) ? "1" : "0");
      }
      printf("\n");
    }
  }
#endif

  /* now generate dictionary for the long edges. This is done by generating an array of
     all long edges and then sorting it according to the length. Then, the dictionary
     string is built up. It replaces the starting pos with the pos in the dictionary
     and sets bit 31 to indicate this.
   */
  //GetTreeStatsDebugRec(pp, 0, 0);
  pg->pg_Bench.ts_TreeStats += BenchTimePassed(pg);

  BuildLongEdgeDictionary(pp);

  BenchTimePassed(pg);

  /* generate huffman code for edge bit mask, saves 1-2 bits per node */
  printf("Generating huffman code for branch mask\n");
  BuildHuffmanCode(pp->pp_BranchCode, (1UL << pg->pg_AlphaSize), 0);

  /* debug output */
#if 0
  for(cnt = 0; cnt < (1UL << pg->pg_AlphaSize); cnt++)
  {
    WORD bitcnt;
    if(pp->pp_BranchCode[cnt].hc_Weight)
    {
      printf("%2ld: %6ld -> %2d ", cnt, pp->pp_BranchCode[cnt].hc_Weight,
        pp->pp_BranchCode[cnt].hc_CodeLength);
      for(bitcnt = 0; bitcnt < pg->pg_AlphaSize; bitcnt++)
      {
    printf("%c", (cnt & (1UL << bitcnt)) ? pg->pg_DecompressTable[bitcnt] : ' ');
      }
      printf(" ");
      for(bitcnt = pp->pp_BranchCode[cnt].hc_CodeLength - 1; bitcnt >= 0; bitcnt--)
      {
    printf("%s", pp->pp_BranchCode[cnt].hc_Codec & (1UL << bitcnt) ? "1" : "0");
      }
      printf("\n");
    }
  }
#endif

  pg->pg_Bench.ts_TreeStats += BenchTimePassed(pg);
  //GetTreeStatsDebugRec(pp, 0, 0);

  return(TRUE);
}
/* \\\ */

/* /// "GetTreeStatsDebugRec()" */
void GetTreeStatsDebugRec(struct PTPanPartition *pp, ULONG pos, ULONG level)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  ULONG cnt;
  ULONG *seqptr = pg->pg_MergedRawData;

  level++;
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  printf("Pos: [%08lx], Level %ld, Edge [",
    pos, level);
  if(sfxnode->sn_StartPos & (3UL << 30))
  {
    printf("%08lx", sfxnode->sn_StartPos);
  } else {
    for(cnt = 0; cnt < sfxnode->sn_EdgeLen; cnt++)
    {
      printf("%c", pg->pg_DecompressTable[GetSeqCodeQuick(cnt + sfxnode->sn_StartPos)]);
    }
  }
  printf("] EdgeStart %ld, EdgeLen %d children %d\nChildren: ",
    sfxnode->sn_StartPos, sfxnode->sn_EdgeLen, childidx);
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    printf("[%08lx] ", childptr);
  }
  printf("\n");

  if(level < 3)
  {
    /* traverse children */
    childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
    while(childidx--)
    {
      childptr = sfxnode->sn_Children[childidx];
      if(!(childptr >> LEAFBIT))
      {
    /* this is a normal node pointer, recurse */
    GetTreeStatsDebugRec(pp, (childptr & RELOFFSETMASK) << 2, level);
      }
    }
  }
  printf("End Level %ld\n", level);
}
/* \\\ */

/* /// "GetTreeStatsTreeDepthRec()" */
void GetTreeStatsTreeDepthRec(struct PTPanPartition *pp, ULONG pos, ULONG level)
{
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;

  /* calculate maximum tree depth */
  if(++level > pp->pp_MaxTreeDepth)
  {
    pp->pp_MaxTreeDepth = level;
  }

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(!(childptr >> LEAFBIT))
    {
      /* this is a normal node pointer, recurse */
      GetTreeStatsTreeDepthRec(pp, (childptr & RELOFFSETMASK) << 2, level);
    }
  }
}
/* \\\ */

/* /// "GetTreeStatsLevelRec()" */
void GetTreeStatsLevelRec(struct PTPanPartition *pp, ULONG pos, ULONG level)
{
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  //sfxnode->sn_Parent |= RELOFFSETMASK;
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* update leaf counter */
      pp->pp_LevelStats[level+1].tls_LeafCount++;
    } else {
      /* this is a normal node pointer, recurse */
      GetTreeStatsLevelRec(pp, (childptr & RELOFFSETMASK) << 2, level+1);
      /* update node counter */
      pp->pp_LevelStats[level].tls_NodeCount++;
    }
  }
}
/* \\\ */

/* /// "GetTreeStatsShortEdgesRec()" */
void GetTreeStatsShortEdgesRec(struct PTPanPartition *pp,
                    ULONG pos, ULONG level, ULONG elen)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG edgelen;
  ULONG epos;
  ULONG prefix;
  ULONG *seqptr = pg->pg_MergedRawData;
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;

  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  /* update short edge code histogram */
  epos = sfxnode->sn_StartPos + 1;
  edgelen = sfxnode->sn_EdgeLen;
  /* do we need to truncate the edge? */
  if(elen + edgelen > pp->pp_TreePruneLength)
  {
    edgelen = pp->pp_TreePruneLength - elen;
    sfxnode->sn_EdgeLen = edgelen;
  }
  elen += edgelen;
  if((edgelen > 1) && (edgelen <= SHORTEDGEMAX))
  {
    prefix = 0;
    while(--edgelen)
    {
      prefix *= pg->pg_AlphaSize;
      prefix += GetSeqCodeQuick(epos);
      epos++;
    }
    /* add stop bit */
    prefix |= 1UL << pg->pg_BitsUseTable[sfxnode->sn_EdgeLen - 1];
    pp->pp_ShortEdgeCode[prefix].hc_Weight++;
  }
  else if(edgelen == 1)
  {
    pp->pp_ShortEdgeCode[1].hc_Weight++; /* also count length==1 edges */
  }

  /* increase edgecount */
  pp->pp_EdgeCount++;

  /* check for maximum depth reached */
  if((++level >= pp->pp_TreePruneDepth) || (elen >= pp->pp_TreePruneLength))
  {
    //printf("SE Level %ld, Epos %ld\n", level, elen);
    return;
  }

  /* traverse children */
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(!(childptr >> LEAFBIT))
    {
      /* this is a normal node pointer, recurse */
      GetTreeStatsShortEdgesRec(pp, (childptr & RELOFFSETMASK) << 2, level, elen);
    } else {
      /* implicit singular edge */
      childptr &= ~LEAFMASK;
      epos = childptr + 1;
      edgelen = pp->pp_TreePruneLength - elen;
      if((edgelen > 1) && (edgelen <= SHORTEDGEMAX))
      {
    prefix = 0;
    while(--edgelen)
    {
    prefix *= pg->pg_AlphaSize;
    prefix += GetSeqCodeQuick(epos);
    epos++;
    }
    /* add stop bit */
    prefix |= 1UL << pg->pg_BitsUseTable[pp->pp_TreePruneLength - elen - 1];
    pp->pp_ShortEdgeCode[prefix].hc_Weight++;
      }
      else if(edgelen == 1)
      {
    pp->pp_ShortEdgeCode[1].hc_Weight++; /* also count length==1 edges */
      }
      /* increase edgecount */
      pp->pp_EdgeCount++;
    }
  }
}
/* \\\ */

/* /// "GetTreeStatsLongEdgesRec()" */
void GetTreeStatsLongEdgesRec(struct PTPanPartition *pp,
                ULONG pos, ULONG level, ULONG elen)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  ULONG edgelen;
  ULONG epos;
  ULONG prefix;
  ULONG *seqptr = pg->pg_MergedRawData;
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  UWORD seqcode;

  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  /* update short edge code histogram */
  epos = sfxnode->sn_StartPos + 1;
  edgelen = sfxnode->sn_EdgeLen;
  /* do we need to truncate the edge? */
#if 0 /* debug */
  if(elen + edgelen > pp->pp_TreePruneLength)
  {
    edgelen = pp->pp_TreePruneLength - elen;
    printf("DARF NICHT!");
    sfxnode->sn_EdgeLen = edgelen;
  }
#endif
  elen += edgelen;
  if(edgelen > 1)
  {
    if(edgelen <= SHORTEDGEMAX)
    {
      prefix = 0;
      while(--edgelen)
      {
    prefix *= pg->pg_AlphaSize;
    prefix += GetSeqCodeQuick(epos);
    epos++;
      }

      /* add stop bit */
      prefix |= 1UL << pg->pg_BitsUseTable[sfxnode->sn_EdgeLen - 1];
      /* check, if this edge doesn't have a huffman code */
      if(!pp->pp_ShortEdgeCode[prefix].hc_CodeLength)
      {
    pp->pp_LongEdges[pp->pp_LongEdgeCount++] = sfxnode;
      } else {
    /* replace sn_StartPos by huffman code index */
    sfxnode->sn_StartPos = prefix | (1UL << 30);
      }
    } else {
      /* this is a long edge anyway and has no huffman code */
      pp->pp_LongEdges[pp->pp_LongEdgeCount++] = sfxnode;
    }
  }
  else if(edgelen == 1)
  {
    /* replace sn_StartPos by huffman code index */
    sfxnode->sn_StartPos = 1 | (1UL << 30);
  }

  /* check for maximum depth reached */
  if((++level >= pp->pp_TreePruneDepth) || (elen >= pp->pp_TreePruneLength))
  {
    //printf("LE Level %ld, Epos %ld\n", level, elen);
    return;
  }

  /* traverse children */
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
#if 1 // extra leaves switch (see 4.3.4 of diploma thesis)
    if(childptr >> LEAFBIT)
    {
      /* implicit singular edge */
      struct SfxNode *tinynode;
      /* allocate a new branching node */
      pp->pp_Sfx2EdgeOffset -= sizeof(struct SfxNode2Edges) - sizeof(ULONG); /* only one child! */
      if(pp->pp_SfxNEdgeOffset >= pp->pp_Sfx2EdgeOffset)
      {
    printf("Node buffer was too small!\n");
    return;
      }
      childptr &= ~LEAFMASK;
      /* fill in node data */
      tinynode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_Sfx2EdgeOffset];
      tinynode->sn_Parent = 1UL << RELOFFSETBITS; /* one edge */
      tinynode->sn_StartPos = childptr + elen;
      tinynode->sn_EdgeLen = pp->pp_TreePruneLength - elen;
      seqcode = GetSeqCodeQuick(childptr + elen + tinynode->sn_EdgeLen);
      tinynode->sn_AlphaMask = 1UL << SEQCODE_N; /* we don't need this branch code anymore */
      pp->pp_BranchCode[tinynode->sn_AlphaMask].hc_Weight++;
      tinynode->sn_Children[0] = childptr | LEAFMASK;
      /* fix link */
      seqcode = GetSeqCodeQuick(childptr + elen);
      childptr = pp->pp_Sfx2EdgeOffset >> 2;
      sfxnode->sn_Children[childidx] = childptr | (seqcode << RELOFFSETBITS);
    }
#endif
    if(!(childptr >> LEAFBIT))
    {
      /* this is a normal node pointer, recurse */
      GetTreeStatsLongEdgesRec(pp, (childptr & RELOFFSETMASK) << 2, level, elen);
    }
  }
}
/* \\\ */

/* /// "GetTreeStatsBranchHistoRec()" */
void GetTreeStatsBranchHistoRec(struct PTPanPartition *pp,
                ULONG pos, ULONG level, ULONG elen)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG childptr;
  UWORD childidx;
  ULONG alphamask = 0;
  ULONG tmpmem[ALPHASIZE];
  UWORD seqcode;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];

  level++;
  elen += sfxnode->sn_EdgeLen;

  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer and doesn't contain a seqcode */
      childptr &= ~LEAFMASK;
      seqcode = GetSeqCodeQuick(childptr + elen);
      tmpmem[seqcode] = sfxnode->sn_Children[childidx];
      alphamask |= 1UL << seqcode;
    } else {
      /* this is a normal node pointer, recurse */
      seqcode = childptr >> RELOFFSETBITS;
      tmpmem[seqcode] = sfxnode->sn_Children[childidx];
      alphamask |= 1UL << seqcode;
      /* check for maximum depth reached */
      if((level < pp->pp_TreePruneDepth) && (elen < pp->pp_TreePruneLength))
      {
    GetTreeStatsBranchHistoRec(pp, (childptr & RELOFFSETMASK) << 2, level, elen);
      }
    }
  }
  /* update branch histogramm */
  pp->pp_BranchCode[alphamask].hc_Weight++;
  sfxnode->sn_AlphaMask = alphamask;

  /* sort branches and enter alphamask */
  childidx = 0;
  seqcode = 0;
  do
  {
    if(alphamask & (1UL << seqcode))
    {
      sfxnode->sn_Children[childidx++] = tmpmem[seqcode];
    }
  } while(++seqcode < pg->pg_AlphaSize);
}
/* \\\ */

/* /// "GetTreeStatsVerifyRec()" */
void GetTreeStatsVerifyRec(struct PTPanPartition *pp, ULONG pos, ULONG treepos, ULONG hash)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG childptr;
  UWORD childidx;
  ULONG newhash;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  if(sfxnode->sn_EdgeLen)
  {
    treepos += sfxnode->sn_EdgeLen;
    hash = GetSeqHash(pg, sfxnode->sn_StartPos, sfxnode->sn_EdgeLen, hash);
  }
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer and doesn't contain a seqcode */
      childptr &= ~LEAFMASK;
      childptr += treepos;
      /* set bit to verify position */
      if(childptr < treepos)
      {
    printf("Childptr %ld < treepos %ld (pos %ld)\n", childptr, treepos, pos);
      }
      if(childptr >= pg->pg_TotalRawSize)
      {
    printf("Childptr %ld > total size %ld (pos %ld, treepos %ld, sn_EdgeLen %d)\n",
        childptr, pg->pg_TotalRawSize, pos, treepos, sfxnode->sn_EdgeLen);
      }

      if(pp->pp_VerifyArray)
      {
    if(pp->pp_VerifyArray[(childptr-treepos) >> 3] & (1UL << ((childptr-treepos) & 7)))
    {
    printf("Clash at pos %ld, ptr %ld\n", pos, childptr - treepos);
    } else {
    pp->pp_VerifyArray[(childptr-treepos) >> 3] |= (1UL << ((childptr-treepos) & 7));
    }
      }

      newhash = GetSeqHash(pg, childptr - treepos, treepos, 0);
      if(newhash != hash)
      {
        STRPTR tmpstr = (STRPTR) malloc(treepos+1);
        DecompressSequencePartTo(pg, seqptr,
                                 sfxnode->sn_StartPos + sfxnode->sn_EdgeLen - treepos,
                                 treepos, tmpstr);

        printf("Hash mismatch for %s (%ld != %ld), treepos = %ld\n",
               tmpstr, newhash, hash, treepos);
        free(tmpstr);
      } else {
        //printf("Good");
      }
    } else {
      /* this is a normal node pointer, recurse */
      GetTreeStatsVerifyRec(pp, (childptr & RELOFFSETMASK) << 2, treepos, hash);
    }
  }
}
/* \\\ */

/* /// "GetTreeStatsLeafCountRec()" */
ULONG GetTreeStatsLeafCountRec(struct PTPanPartition *pp, ULONG pos)
{
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;
  ULONG cnt = 0;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  //printf("P=%08lx LC: %ld [%d]\n", pos, childidx, sfxnode->sn_AlphaMask);
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer and doesn't contain a seqcode */
      cnt++;
    } else {
      /* this is a normal node pointer, recurse */
      cnt += GetTreeStatsLeafCountRec(pp, (childptr & RELOFFSETMASK) << 2);
    }
  }
  return(cnt);
}
/* \\\ */

/* /// "GetTreeStatsLeafCollectRec()" */
void GetTreeStatsLeafCollectRec(struct PTPanPartition *pp, ULONG pos)
{
  struct SfxNode *sfxnode;
  ULONG childptr;
  UWORD childidx;

  /* traverse children */
  sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pos];
  childidx = sfxnode->sn_Parent >> RELOFFSETBITS;
  while(childidx--)
  {
    childptr = sfxnode->sn_Children[childidx];
    if(childptr >> LEAFBIT)
    {
      /* this is a leaf pointer and doesn't contain a seqcode */
      *pp->pp_LeafBufferPtr++ = childptr & ~LEAFMASK;
    } else {
      /* this is a normal node pointer, recurse */
      GetTreeStatsLeafCollectRec(pp, (childptr & RELOFFSETMASK) << 2);
    }
  }
}
/* \\\ */

/* /// "LongEdgeLengthCompare()" */
LONG LongEdgeLengthCompare(const struct SfxNode **node1, const struct SfxNode **node2)
{
  return(((LONG) (*node2)->sn_EdgeLen) - ((LONG) (*node1)->sn_EdgeLen));
}
/* \\\ */

/* /// "LongEdgePosCompare()" */
LONG LongEdgePosCompare(const struct SfxNode **node1, const struct SfxNode **node2)
{
  return(((LONG) (*node1)->sn_StartPos) - ((LONG) (*node2)->sn_StartPos));
}
/* \\\ */

/* /// "LongEdgeLabelCompare()" */
LONG LongEdgeLabelCompare(struct SfxNode **node1, struct SfxNode **node2)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG spos1 = (*node1)->sn_StartPos;
  ULONG spos2 = (*node2)->sn_StartPos;
  ULONG len = (*node1)->sn_EdgeLen;
  UBYTE seqcode1, seqcode2;

  if(spos1 == spos2) /* no need to compare */
  {
    if(len < (*node2)->sn_EdgeLen)
    {
      return(-1);
    }
    else if(len > (*node2)->sn_EdgeLen)
    {
      return(1);
    }
    return(0); /* string exactly equal */
  }

  if((*node2)->sn_EdgeLen < len)
  {
    len = (*node2)->sn_EdgeLen;
  }

  /* compare sequences */
  do
  {
    seqcode1 = GetSeqCodeQuick(spos1);
    seqcode2 = GetSeqCodeQuick(spos2);
    if(seqcode1 < seqcode2)
    {
      return(-1);
    }
    else if(seqcode1 > seqcode2)
    {
      return(1);
    }
    spos1++;
    spos2++;
  } while(--len);

  /* sequence prefixes were the same! */
  if((*node1)->sn_EdgeLen >= (*node2)->sn_EdgeLen)
  {
    /* move starting pos "down" */
    if((*node1)->sn_StartPos < (*node2)->sn_StartPos)
    {
      //printf("Moved %ld -> %ld\n", (*node2)->sn_StartPos, (*node1)->sn_StartPos);
      (*node2)->sn_StartPos = (*node1)->sn_StartPos;
    }
    if((*node1)->sn_EdgeLen == (*node2)->sn_EdgeLen)
    {
      return(0);
    } else {
      return(1);
    }
  } else {
    /* shorter sequence is "smaller" */
    return(-1);
  }
}
/* \\\ */

/* /// "GetSeqHash()" */
ULONG GetSeqHash(struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash)
{
  ULONG *seqptr = &pg->pg_MergedRawData[seqpos / MAXCODEFITLONG];
  ULONG modval = MAXCODEFITLONG - (seqpos % MAXCODEFITLONG);
  ULONG pval = *seqptr++ >> pg->pg_BitsShiftTable[MAXCODEFITLONG];

  /* calculate the hash value over the string */
  while(len--)
  {
    hash *= pg->pg_AlphaSize;
    if(--modval)
    {
      hash += (pval / pg->pg_PowerTable[modval]) % pg->pg_AlphaSize;
    } else {
      hash += pval % pg->pg_AlphaSize;
      pval = *seqptr++ >> pg->pg_BitsShiftTable[MAXCODEFITLONG];
      modval = MAXCODEFITLONG;
    }
    hash %= HASHPRIME;
  }
  return(hash);
}
/* \\\ */

/* /// "GetSeqHashBackwards()" */
ULONG GetSeqHashBackwards(struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash)
{
  seqpos += len;
  {
    ULONG *seqptr = &pg->pg_MergedRawData[seqpos / MAXCODEFITLONG];
    ULONG modval = MAXCODEFITLONG - (seqpos % MAXCODEFITLONG);
    ULONG pval = *seqptr >> pg->pg_BitsShiftTable[MAXCODEFITLONG];

    /* calculate the hash value over the string */
    while(len--)
    {
      hash *= pg->pg_AlphaSize;
      if(modval < MAXCODEFITLONG)
      {
    hash += (pval / pg->pg_PowerTable[modval++]) % pg->pg_AlphaSize;
      } else {
    modval = 1;
    pval = *(--seqptr) >> pg->pg_BitsShiftTable[MAXCODEFITLONG];
    hash += pval % pg->pg_AlphaSize;
      }
      hash %= HASHPRIME;
    }
  }
  return(hash);
}
/* \\\ */

/* /// "CheckLongEdgeMatch()" */
BOOL CheckLongEdgeMatch(struct PTPanPartition *pp, ULONG seqpos, ULONG edgelen,
                        ULONG dictpos)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  STRPTR dictptr = &pp->pp_LongDict[dictpos];
  ULONG *seqptr = &pg->pg_MergedRawData[seqpos / MAXCODEFITLONG];
  ULONG modval = MAXCODEFITLONG - (seqpos % MAXCODEFITLONG);
  ULONG pval = *seqptr++ >> pg->pg_BitsShiftTable[MAXCODEFITLONG];

  while(edgelen--)
  {
    if(--modval)
    {
      if(pg->pg_DecompressTable[(pval / pg->pg_PowerTable[modval]) % pg->pg_AlphaSize] !=
         *dictptr++)
      {
        return(FALSE);
      }
    } else {
      if(pg->pg_DecompressTable[pval % pg->pg_AlphaSize] != *dictptr++)
      {
        return(FALSE);
      }
      pval = *seqptr++ >> pg->pg_BitsShiftTable[MAXCODEFITLONG];
      modval = MAXCODEFITLONG;
    }
  }
  return(TRUE);
}
/* \\\ */

/* /// "BuildLongEdgeDictionary()" */
BOOL BuildLongEdgeDictionary(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG *seqptr = pg->pg_MergedRawData;
  ULONG cnt;
  ULONG apos;
  ULONG spos;
  STRPTR dictptr;
  ULONG edgelen;
  ULONG dictsize;
  ULONG hashval;
  struct HashEntry *hash;
  ULONG walkinghash;
  ULONG lastedgelen;
  ULONG subfact;
  BOOL notfound;
  ULONG hashhit, hashmiss, walkmiss, walkhit, stringcnt;
  BOOL hassweep;
  BOOL quicksweep;
  BOOL safeskip;

  ULONG olddictsize;

  BenchTimePassed(pg);
  /* allocate long edge array */
  pp->pp_LongEdges = (struct SfxNode **) calloc(pp->pp_EdgeCount,
                                                sizeof(struct SfxNode *));
  if(!pp->pp_LongEdges)
  {
    printf("Out of memory for Long Edges Array!\n");
    return(FALSE);
  }
  pp->pp_LongEdgeCount = 0;
  GetTreeStatsLongEdgesRec(pp, 0, 0, 0);
  printf("Long Edge Array filled with %ld entries ", pp->pp_LongEdgeCount);
  printf("(%ld KB unused)\n", (pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset) >> 10);

#if 0 /* debug */
  for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
  {
    printf("%ld: %ld\n", cnt, pp->pp_LongEdges[cnt]->sn_EdgeLen);
  }
#endif
  /* now sort array */
  if(pp->pp_LongEdgeCount > 2)
  {
#if 1 /* disable this, if sorting takes too long, but
         this causes the dictionary to explode */
#if 0 // debug
    printf("Before sorting:\n");
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      DecompressSequencePartTo(pg, seqptr,
                pp->pp_LongEdges[cnt]->sn_StartPos,
                pp->pp_LongEdges[cnt]->sn_EdgeLen,
                pg->pg_TempBuffer);
      printf("%6ld: %7ld: %s\n", cnt, pp->pp_LongEdges[cnt]->sn_StartPos,
        pg->pg_TempBuffer);
    }
#endif
    printf("Sorting (Pass 1)...\n");
    qsort(pp->pp_LongEdges, pp->pp_LongEdgeCount, sizeof(struct SfxNode *),
    (int (*)(const void *, const void *)) LongEdgeLabelCompare);
    /* some edges might now have been moved to the front after sorting,
       but due to the sorting, these will be alternating, with edges of the
       same length, so fix these in O(n) */
#if 0 // debug
    printf("Before prefix clustering:\n");
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      DecompressSequencePartTo(pg, seqptr,
            pp->pp_LongEdges[cnt]->sn_StartPos,
            pp->pp_LongEdges[cnt]->sn_EdgeLen,
            pg->pg_TempBuffer);
            printf("%6ld: %7ld: %s\n", cnt, pp->pp_LongEdges[cnt]->sn_StartPos,
        pg->pg_TempBuffer);
    }
#endif
    for(cnt = pp->pp_LongEdgeCount-1; cnt > 0; cnt--)
    {
      ULONG spos1 = pp->pp_LongEdges[cnt-1]->sn_StartPos;
      ULONG spos2 = pp->pp_LongEdges[cnt]->sn_StartPos;
      ULONG len = pp->pp_LongEdges[cnt-1]->sn_EdgeLen;

      if(len <= pp->pp_LongEdges[cnt]->sn_EdgeLen && (spos1 != spos2))
      {
    /* compare sequences */
    do
    {
    if(GetSeqCodeQuick(spos1) != GetSeqCodeQuick(spos2))
    {
        break;
    }
    spos1++;
    spos2++;
    } while(--len);
    if(!len)
    {
    /* was equal */
    pp->pp_LongEdges[cnt-1]->sn_StartPos = pp->pp_LongEdges[cnt]->sn_StartPos;
    }
      }
    }
#if 0 // debug
    printf("After prefix clustering:\n");
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      DecompressSequencePartTo(pg, seqptr,
                pp->pp_LongEdges[cnt]->sn_StartPos,
                pp->pp_LongEdges[cnt]->sn_EdgeLen,
                pg->pg_TempBuffer);
      printf("%6ld: %7ld: %s\n", cnt, pp->pp_LongEdges[cnt]->sn_StartPos,
        pg->pg_TempBuffer);
    }
#endif
    printf("Sorting (Pass 2)...\n");
    qsort(pp->pp_LongEdges, pp->pp_LongEdgeCount, sizeof(struct SfxNode *),
    (int (*)(const void *, const void *)) LongEdgePosCompare);
#if 0 // debug
    printf("After offset sorting\n");
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      DecompressSequencePartTo(pg, seqptr,
                pp->pp_LongEdges[cnt]->sn_StartPos,
                pp->pp_LongEdges[cnt]->sn_EdgeLen,
                pg->pg_TempBuffer);
      printf("%6ld: %7ld: %s\n", cnt, pp->pp_LongEdges[cnt]->sn_StartPos,
        pg->pg_TempBuffer);
    }
#endif

    {
      ULONG ivalstart = 0;
      ULONG ivalend = 0;
      ULONG ivalcnt = 0;
      ULONG ivalsum = 0;
      ULONG ivalextend = 0;
      ULONG oldcnt = pp->pp_LongEdgeCount;

      /* examine edges and create bigger intervals */
      for(cnt = 0; cnt < oldcnt; cnt++)
      {
    if(pp->pp_LongEdges[cnt]->sn_StartPos > ivalend)
    {
    /* create new interval (these thresholds were found out using lots
             of testing) */
    if((ivalextend > 5) &&
        (ivalend - ivalstart > 14))
    {
        //printf("Ival: %ld - %ld\n", ivalstart, ivalend);
        if((pp->pp_SfxNEdgeOffset < pp->pp_Sfx2EdgeOffset - sizeof(struct SfxNodeStub)) &&
        (pp->pp_LongEdgeCount < pp->pp_EdgeCount))
        {
        /* generate a small stub node, that will lead the array building */
        pp->pp_Sfx2EdgeOffset -= sizeof(struct SfxNodeStub); /* only a stub! */
        sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[pp->pp_Sfx2EdgeOffset];
        sfxnode->sn_StartPos = ivalstart;
        sfxnode->sn_EdgeLen = ivalend - ivalstart + 1;
        sfxnode->sn_AlphaMask = 0xCAFE;
        pp->pp_LongEdges[pp->pp_LongEdgeCount++] = sfxnode;
        ivalcnt++;
        ivalsum += sfxnode->sn_EdgeLen;
        } else {
        printf("Out of mem!\n");
        cnt = oldcnt;
        }
        ivalextend = 0;
    }
    ivalstart = pp->pp_LongEdges[cnt]->sn_StartPos;
    ivalend = ivalstart + pp->pp_LongEdges[cnt]->sn_EdgeLen - 1;
    } else {
    /* check, if we have to enlarge this interval...
        (is this an attempt to compensate something? :) )*/
    if((pp->pp_LongEdges[cnt]->sn_StartPos +
        pp->pp_LongEdges[cnt]->sn_EdgeLen - 1 > ivalend))
        //(ivalstart + (pp->pp_LongEdges[cnt]->sn_EdgeLen * 2) > ivalend))
    {
        ivalend = pp->pp_LongEdges[cnt]->sn_StartPos +
        pp->pp_LongEdges[cnt]->sn_EdgeLen - 1;
        if(pp->pp_LongEdges[cnt]->sn_StartPos > ivalstart)
        {
        ivalextend++;
        }
    }
    }
      }
      printf("Additional intervals generated %ld (%ld KB)\n", ivalcnt, ivalsum >> 10);
    }
#endif
    printf("Sorting (Pass 3)...\n");
    qsort(pp->pp_LongEdges, pp->pp_LongEdgeCount, sizeof(struct SfxNode *),
    (int (*)(const void *, const void *)) LongEdgeLengthCompare);
#if 0 // debug
    printf("After length sorting\n");
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      DecompressSequencePartTo(pg, seqptr,
                pp->pp_LongEdges[cnt]->sn_StartPos,
                pp->pp_LongEdges[cnt]->sn_EdgeLen,
                pg->pg_TempBuffer);
      printf("%6ld: %7ld: %s\n", cnt, pp->pp_LongEdges[cnt]->sn_StartPos,
        pg->pg_TempBuffer);
    }
#endif
  }
  pg->pg_Bench.ts_LongDictPre += BenchTimePassed(pg);
  if(!pp->pp_LongEdgeCount)
  {
    /* catch special case of no long edges */
    pp->pp_LongEdgeLenSize = 1;
    pp->pp_LongEdgeLenCode = (struct HuffCode *) calloc(pp->pp_LongEdgeLenSize,
                            sizeof(struct HuffCode));
    //pp->pp_LongEdgeLenCode[0].hc_CodeLength = 1;
    pp->pp_LongDictSize = 1;
    pp->pp_LongDict = (STRPTR) malloc(pp->pp_LongDictSize);
    *pp->pp_LongDict = 0;
    dictsize = 0;
  } else {
    pp->pp_LongEdgeLenSize = pp->pp_LongEdges[0]->sn_EdgeLen + 1;
    /* allocate long edge len histogram */
    pp->pp_LongEdgeLenCode = (struct HuffCode *) calloc(pp->pp_LongEdgeLenSize,
                            sizeof(struct HuffCode));
    if(!pp->pp_LongEdgeLenCode)
    {
      printf("Out of memory for Long Edge Length Histogram!\n");
      return(FALSE);
    }

    /* count lengths */
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
      if(pp->pp_LongEdges[cnt]->sn_AlphaMask != 0xCAFE)
      {
    pp->pp_LongEdgeLenCode[pp->pp_LongEdges[cnt]->sn_EdgeLen].hc_Weight++;
      }
    }

    printf("Longest edge: %ld\n", pp->pp_LongEdgeLenSize - 1);

    /* generate huffman code for edge lengths */
    printf("Generating huffman code for long edges length\n");
    BuildHuffmanCode(pp->pp_LongEdgeLenCode, pp->pp_LongEdgeLenSize, 0);

#if 0
    /* debug */
    for(cnt = 0; cnt < pp->pp_LongEdgeLenSize; cnt++)
    {
      WORD bitcnt;
      if(pp->pp_LongEdgeLenCode[cnt].hc_CodeLength)
      {
        printf("%6ld: %7ld -> %2d ", cnt, pp->pp_LongEdgeLenCode[cnt].hc_Weight,
        pp->pp_LongEdgeLenCode[cnt].hc_CodeLength);
    for(bitcnt = pp->pp_LongEdgeLenCode[cnt].hc_CodeLength - 1; bitcnt >= 0; bitcnt--)
    {
    printf("%s", pp->pp_LongEdgeLenCode[cnt].hc_Codec & (1UL << bitcnt) ? "1" : "0");
    }
    printf("\n");
      }
    }
#endif

    /* now allocate a buffer for building the dictionary */
    pp->pp_LongDictSize = 16UL << 10; /* start with 16KB */
    pp->pp_LongDict = (STRPTR) malloc(pp->pp_LongDictSize);
    if(!pp->pp_LongDict)
    {
      printf("Out of memory for long edges dictionary!\n");
      free(pp->pp_LongEdges);
      return(FALSE);
    }
    pp->pp_LongDict[0] = 0;
    dictsize = 0;

    /* allocate the hash table to speed up search */
    pp->pp_LongDictHashSize = 256UL << 10;
    pp->pp_LongDictHash = AllocHashArray(pp->pp_LongDictHashSize);
    if(!pp->pp_LongDictHash)
    {
      printf("Out of memory for long edges dictionary hash!\n");
      free(pp->pp_LongEdges);
      free(pp->pp_LongDict);
      return(FALSE);
    }
    printf("Allocated a hash for %ld entries...\n", pp->pp_LongDictHashSize);

    /* some statistical data */
    hashhit   = 0; /* string found in hash */
    hashmiss  = 0; /* string found in hash, but was no match */
    walkmiss  = 0; /* wrong fingerprint matches */
    walkhit   = 0; /* string found during walk */
    stringcnt = 1; /* number of strings generated */

    /* create special initial string to avoid dictionary exploding by fragmented truncated edges */
    sfxnode = pp->pp_LongEdges[0];
    spos = sfxnode->sn_StartPos + 1;
    edgelen = sfxnode->sn_EdgeLen - 1;
    dictptr = pp->pp_LongDict;
    dictsize = edgelen;
    while(edgelen--)
    {
      *dictptr++ = pg->pg_DecompressTable[GetSeqCodeQuick(spos)];
      spos++;
    }
    /* don't forget the termination char */
    *dictptr = 0;

    /* insert all edges */
    lastedgelen = pp->pp_LongEdges[0]->sn_EdgeLen;
    hassweep = FALSE;
    olddictsize = 0;
    for(cnt = 0; cnt < pp->pp_LongEdgeCount; cnt++)
    {
#if 0 /* debug */
      if((dictsize > olddictsize + 40) || (cnt == pp->pp_LongEdgeCount-1))
      {
    fprintf(stderr, "%ld %ld\n", cnt, dictsize);
    olddictsize = dictsize;
      }
#endif
      if(((cnt+1) & 0x3ff) == 0)
      {
    if(((cnt+1) >> 10) % 50)
    {
    printf(".");
    fflush(stdout);
    } else {
    printf(". %2ld%% (%ld KB, %ld strings)\n",
        (cnt * 100) / pp->pp_LongEdgeCount, dictsize >> 10, stringcnt);
    }
      }
      sfxnode = pp->pp_LongEdges[cnt];
      /* note that we can skip the first base because it is part of the branch */
      spos = sfxnode->sn_StartPos + 1;
      edgelen = sfxnode->sn_EdgeLen - 1;

      /*if(edgelen != lastedgelen)
    {
    printf("%6ld\n", edgelen);
    }*/
      /* if we have swept over the dictionary, it is safe to skip search */
      safeskip = hassweep;

      /* calculate hash value */
      hashval = GetSeqHashBackwards(pg, spos, edgelen, 0);
      //printf("[%ld] Len %ld, Hashval = %ld, dictsize = %ld ", cnt, edgelen, hashval, dictsize);
      if((hash = GetHashEntry(pp->pp_LongDictHash, hashval)))
      {
    //printf("Hash ");
    /* we will check only atmost 12 characters. the probability, that the
    edge had the same hash value and more than 12 matching characters, is
    unbelievably low */
    if(CheckLongEdgeMatch(pp, spos, edgelen > 12 ? 12 : edgelen, hash->he_Data))
    {
    hashhit++;
    //printf("match\n");
    sfxnode->sn_StartPos = hash->he_Data | (1UL << 31);
    continue;
    }
    hashmiss++;
    //printf("miss\n");
    /* we had a hash miss, consider it not safe anymore to skip the search */
    safeskip = FALSE;
      }

      if(edgelen < lastedgelen)
      {
    if(pp->pp_LongDictHash->ha_Used > (pp->pp_LongDictHash->ha_Size >> 3))
    {
    /* seems as if the hash full by more than 1/8 and needs some clearing */
    //printf("Clearing hash...\n");
          ClearHashArray(pp->pp_LongDictHash);
    }
    hassweep = FALSE;
    safeskip = FALSE;
      }
      /* sorry, have to do a linear search */
      notfound = TRUE;

      /* if he have swept over the dictionary, generating all possible hash values
    and did not hit the hash, it is very impossible that we will find it
    anyway, so we just append it.
      */
      if(!safeskip)
      {
    /* first attempt was to walk through the dictionary from the
    beginning to the end. However, in some bright moment,
    I thought about traversing the dictionary the other
    way round and see if this works better */
    /* hash = \sum{i < m}{dict[pos+i]*5^(m-i)}
    to walk right:
    hash = oldhash * 5 + dict[pos] - dict[pos-m] * (5^(m+1))
    to walk left:
    hash = oldhash - dict[pos] / 5 + dict[pos] * (5^m) */

    /* init walking hash/finger print value */
    apos = dictsize;
    dictptr = &pp->pp_LongDict[apos];
    subfact = 1;
    walkinghash = 0;

    //printf("edgelen = %ld\n", edgelen);
    do
    {
    /* calculate character outshifting multiplicator */
    subfact *= pg->pg_AlphaSize;
    subfact %= HASHPRIME;
    /* calculate finger print */
    walkinghash *= pg->pg_AlphaSize;
    walkinghash += pg->pg_CompressTable[*(--dictptr)];
    walkinghash %= HASHPRIME;
    } while(--apos > dictsize - edgelen);

    /* did the length shrink and does it pay to do a sweep? */
    if((edgelen < lastedgelen) &&
    (cnt + 40 < pp->pp_LongEdgeCount) &&
    (edgelen == (ULONG) pp->pp_LongEdges[cnt + 40]->sn_EdgeLen - 1))
    {
    //printf("Q[%ld<-%ld]", edgelen, pp->pp_LongEdges[cnt + 15]->sn_EdgeLen - 1);
    //printf("Quicksweep\n");
    quicksweep = TRUE;
    hassweep = TRUE;
    } else {
    quicksweep = FALSE;
    }
    //hassweep = quicksweep = FALSE; /* FIXME */
    /* loop until found or end of dictionary is reached */
    do
    {
    //printf("Apos = %ld, WH = %ld\n", apos, walkinghash);
    /* finger print value matches */
    if(quicksweep)
    {
        if(!(GetHashEntry(pp->pp_LongDictHash, walkinghash)))
        {
        InsertHashEntry(pp->pp_LongDictHash, walkinghash, apos);
        }
    }
    if(walkinghash == hashval)
    {
        //printf("Walk ");
        /* verify hit (well, to a high probability) */
        if(CheckLongEdgeMatch(pp, spos, edgelen > 12 ? 12 : edgelen, apos))
        {
        //printf("hit\n");
#if 0 /* debug */
        if(safeskip)
        {
        printf("We would have missed %ld [%ld != %08lx], %ld!\n",
            apos, hashval,
            (ULONG) GetHashEntry(pp->pp_LongDictHash, walkinghash),
            edgelen);
      }
#endif
        /* found it! */
        walkhit++;
        sfxnode->sn_StartPos = apos | (1UL << 31);
        //printf("Walk: %f\n", (double) ((double) apos / (double) dictsize));
        notfound = FALSE;
        /* we have to finish the scan in quicksweep mode */
        if(quicksweep)
        {
        hashval = ~0UL; /* make sure we won't find another hit */
        } else {
        /* insert hash entry */
        InsertHashEntry(pp->pp_LongDictHash, hashval, apos);
        break;
        }
        } else {
        walkmiss++;
        //printf("miss\n");
        }
    }
    if(apos)
    {
        /* calculate new hash value */
        walkinghash += HASHPRIME;
        walkinghash *= pg->pg_AlphaSize;
        walkinghash -= pg->pg_CompressTable[dictptr[edgelen-1]] * subfact;
        walkinghash += pg->pg_CompressTable[*(--dictptr)];
        walkinghash %= HASHPRIME;
        apos--;
    } else {
        break;
    }
    } while(TRUE);
      }
      lastedgelen = edgelen;
      /* check, if we already found it */
      if(notfound)
      {
    stringcnt++;
    apos = dictsize;
    dictsize += edgelen;
    //printf("add");
    //printf("Appending %ld at %ld...\n", edgelen, apos);
    if(dictsize >= pp->pp_LongDictSize)
    {
    STRPTR newptr;
    /* double the size of the buffer */
    pp->pp_LongDictSize <<= 1;
    if((newptr = (STRPTR) realloc(pp->pp_LongDict, pp->pp_LongDictSize)))
    {
        pp->pp_LongDict = newptr;
        //printf("Expanded Dictionary to %ld bytes.\n", pp->pp_LongDictSize);
    } else {
        printf("Out of memory while expanding long edges dictionary!\n");
        free(pp->pp_LongDict);
        free(pp->pp_LongEdges);
        return(FALSE);
    }
    }
    /* insert hash entry */
    InsertHashEntry(pp->pp_LongDictHash, hashval, apos);
    /* fix start pos */
    sfxnode->sn_StartPos = apos | (1UL << 31);
    dictptr = &pp->pp_LongDict[apos];
    while(edgelen--)
    {
    *dictptr++ = pg->pg_DecompressTable[GetSeqCodeQuick(spos)];
    spos++;
    }
    /* don't forget the termination char */
    *dictptr = 0;
      }
    }
    pp->pp_LongDictSize = dictsize;
    /* printf("\nHashhit %ld, Hashmiss %ld, Walkhit %ld, Walkmiss %ld\n",
       hashhit, hashmiss, walkhit, walkmiss);*/
    printf("\nDictionary size: %ld KB (%ld strings)\n", dictsize >> 10, stringcnt);
    //printf(pp->pp_LongDict);
  }

  /* calculate bits usage */
  pp->pp_LongRelPtrBits = 1;
  while((1UL << pp->pp_LongRelPtrBits) < dictsize)
  {
    pp->pp_LongRelPtrBits++;
  }

  /* compress sequence to save memory */
  if(!(pp->pp_LongDictRaw = CompressSequence(pg, pp->pp_LongDict)))
  {
    printf("Out of memory for compressed dictionary string!\n");
    FreeHashArray(pp->pp_LongDictHash);
    free(pp->pp_LongEdges);
    free(pp->pp_LongDict);
    return(FALSE);
  }

  printf("Final compressed dictionary size: %ld bytes.\n",
    ((dictsize / MAXCODEFITLONG) + 1) * sizeof(ULONG));

  /* free some memory */
  FreeHashArray(pp->pp_LongDictHash);
  free(pp->pp_LongEdges);
  free(pp->pp_LongDict);

  pg->pg_Bench.ts_LongDictBuild += BenchTimePassed(pg);

  return(TRUE);
}
/* \\\ */

/* /// "WriteTreeToDisk()" */
BOOL WriteTreeToDisk(struct PTPanPartition *pp)
{
  struct PTPanGlobal *pg = pp->pp_PTPanGlobal;
  struct SfxNode *sfxnode;
  ULONG pos;

  ULONG childptr;
  ULONG packsize;
  ULONG bytessaved;
  BOOL  freedisk = FALSE;

  BenchTimePassed(pg);
  /* after we have prepared all the codecs and compression tables, we have to modify
     the tree to be able to store it on the disk. Bottom up approach.
     a) count the space required for the cut off leaves. They are stored as compressed
        arrays, each array holding at least two leaves. The leaves in the upper levels
        of the tree are not saved there, as a pointer to the leaf array containing only
        one leaf would be a waste of space (instead they are stored directly as a child
        ptr).
     b) traverse the tree in DFS order from lowest level to the root, from right to the
        left, enter relative pointers in the ChildPtr[] array, count the space
    consumption.
  */

  /* calculate the relative pointers and the tree traversal */
  pp->pp_DiskOuterLeaves = 0;
  pp->pp_DiskTreeSize = 0;
  pp->pp_TraverseTreeRoot = ~0UL & RELOFFSETMASK;
  
  ULONG tempDiskTreeSize = FixRelativePointersRec(pp, 0, 0, 0);
  pp->pp_DiskTreeSize += tempDiskTreeSize;                      
  //printf("Total size on disk: %ld KB\n", pp->pp_DiskTreeSize >> 10);

  pg->pg_Bench.ts_Reloc += BenchTimePassed(pg);
  /* now finally write it to disk */
  pp->pp_PartitionFile = fopen(pp->pp_PartitionName, "w");
  if(!pp->pp_PartitionFile)
  {
    printf("ERROR: Couldn't open partition file %s for writing!\n",
    pp->pp_PartitionName);
    return(FALSE);
  }

  WriteTreeHeader(pp);

  /* use unused node buffer for temporary write buffer */
  pp->pp_DiskBuffer = (UBYTE *) &pp->pp_SfxNodes[pp->pp_SfxNEdgeOffset];
  pp->pp_DiskBufferSize = pp->pp_Sfx2EdgeOffset - pp->pp_SfxNEdgeOffset;
  if(pp->pp_DiskBufferSize < (128UL << 10))
  {
    /* disk buffer was much too small! */
    pp->pp_DiskBufferSize = 128UL << 10;
    pp->pp_DiskBuffer = (UBYTE *) calloc(1, pp->pp_DiskBufferSize);
    freedisk = TRUE;
  } else {
    if(pp->pp_DiskBufferSize > (512UL << 10))
    {
      pp->pp_DiskBufferSize = 512UL << 10;
    }
  }
  //printf("Diskbuffer: %ld KB\n", pp->pp_DiskBufferSize >> 10);
  pp->pp_DiskPos = 0;
  bytessaved = 0;

#if 0 /* debug */
  pp->pp_BranchTree = BuildHuffmanTreeFromTable(pp->pp_BranchCode, 1UL << pg->pg_AlphaSize);
  pp->pp_ShortEdgeTree = BuildHuffmanTreeFromTable(pp->pp_ShortEdgeCode, 1UL << (pg->pg_BitsUseTable[SHORTEDGEMAX]+1));
  pp->pp_LongEdgeLenTree = BuildHuffmanTreeFromTable(pp->pp_LongEdgeLenCode, pp->pp_LongEdgeLenSize);
#endif

  printf("Writing (%ld KB)",pp->pp_DiskTreeSize >> 10);
  fflush(NULL);
  pp->pp_DiskNodeCount = 0;
  pp->pp_DiskNodeSpace = 0;
  pp->pp_DiskLeafCount = 0;
  pp->pp_DiskLeafSpace = 0;

  pos = pp->pp_TraverseTreeRoot;
  while((pos & RELOFFSETMASK) != (~0UL & RELOFFSETMASK))
  {
    childptr = (pos & RELOFFSETMASK) << 2;
    //printf("Pos=%08lx [%ld]\n", childptr, bytessaved);
    sfxnode = (struct SfxNode *) &pp->pp_SfxNodes[childptr];

    packsize = 0;
    if(sfxnode->sn_AlphaMask) /* is this a normal node or the end of the tree */
    {
      packsize = WritePackedNode(pp, childptr, &pp->pp_DiskBuffer[pp->pp_DiskPos]);
      pp->pp_DiskNodeCount++;
      pp->pp_DiskNodeSpace += packsize;
#if 0 /* debug */
      {
        struct TreeNode *tn;
        pp->pp_DiskTree = &pp->pp_DiskBuffer[pp->pp_DiskPos];
        tn = ReadPackedNode(pp, 0);
        if(tn)
        {
        if(packsize != tn->tn_Size)
        {
            ULONG bc;
            printf("ARGH! ARGH! ARRRRGH! NodePos %08lx [%ld != %ld]\n",
        pos, packsize, tn->tn_Size);
        for(bc = 0; bc < 32; bc++)
        {
        printf("[%02x] ", pp->pp_DiskTree[bc]);
        }
        printf("\n");
    }
    free(tn);
    }
      }
#endif
    } else {
      packsize = WritePackedLeaf(pp, childptr, &pp->pp_DiskBuffer[pp->pp_DiskPos]);
      pp->pp_DiskLeafCount++;
      pp->pp_DiskLeafSpace += packsize;
#if 0 /* debug */
      {
    struct TreeNode *tn;
    BOOL argh;
    ULONG bcnt;

    pp->pp_DiskTree = &pp->pp_DiskBuffer[pp->pp_DiskPos];
    tn = ReadPackedLeaf(pp, 0);
    if(tn)
    {
    argh = (packsize != tn->tn_Size);
    for(bcnt = 0; bcnt < tn->tn_NumLeaves; bcnt++)
    {
        if(tn->tn_Leaves[bcnt] < 0)
        {
        argh = TRUE;
        }
    }
    if(argh)
    {
        ULONG bc;
        printf("ARGH! ARGH! ARRRRGH! LeafPos %08lx [%ld != %ld]\n",
        pos, packsize, tn->tn_Size);
        for(bc = 0; bc < 32; bc++)
        {
        printf("[%02x] ", pp->pp_DiskTree[bc]);
        }
        printf("\n");
    }
    free(tn);
    }
      }
#endif
    }
    pp->pp_DiskPos += packsize;
    bytessaved += packsize;
    /* check if disk buffer is full enough to write a new chunk */
    if(pp->pp_DiskPos > (pp->pp_DiskBufferSize >> 1))
    {

      printf(".");
      fflush(NULL);
      fwrite(pp->pp_DiskBuffer, pp->pp_DiskPos, 1, pp->pp_PartitionFile);
      pp->pp_DiskPos = 0;
    }
    pos = sfxnode->sn_Parent;
  } // end while

  if(pp->pp_DiskPos)
  {
    printf(".\n");
    fwrite(pp->pp_DiskBuffer, pp->pp_DiskPos, 1, pp->pp_PartitionFile);
  }
  pp->pp_DiskIdxSpace = ftell(pp->pp_PartitionFile);
  printf("%ld inner nodes     (%ld KB, %f b.p.n.)\n"
    "%ld leaf nodes      (%ld KB, %f b.p.n.)\n"
         "%ld leaves in array (%ld KB, %f b.p.l.)\n"
    "Overall %f bytes per base.\n",
    pp->pp_DiskNodeCount, pp->pp_DiskNodeSpace >> 10,
    (float) pp->pp_DiskNodeSpace / (float) pp->pp_DiskNodeCount,
    pp->pp_DiskLeafCount, pp->pp_DiskLeafSpace >> 10,
    (float) pp->pp_DiskLeafSpace / (float) pp->pp_DiskLeafCount,
    pp->pp_DiskOuterLeaves, pp->pp_DiskLeafSpace >> 10,
    (float) pp->pp_DiskLeafSpace  / (float) pp->pp_DiskOuterLeaves,
    (float) pp->pp_DiskIdxSpace / (float) pp->pp_Size);
  fclose(pp->pp_PartitionFile);
  if(freedisk)
  {
    free(pp->pp_DiskBuffer);
  }

  pg->pg_Bench.ts_Writing += BenchTimePassed(pg);

  if(bytessaved != pp->pp_DiskTreeSize)
  {
    printf("ERROR: Calculated tree size did not match written data (%ld != %ld)!\n",
    bytessaved, pp->pp_DiskTreeSize);
    return(FALSE);
  }
  return(TRUE);
}
/* \\\ */

/* /// "CreatePartitionLookup()" */
BOOL CreatePartitionLookup(struct PTPanGlobal *pg)
{
  ULONG cnt;
  ULONG len;
  struct PTPanPartition *pp;
  UWORD range;

  /* allocate memory for table */
  len = pg->pg_PowerTable[pg->pg_MaxPrefixLen];
  pg->pg_PartitionLookup = (struct PTPanPartition **) calloc(len, sizeof(struct PTPanPartition *));
  if(!pg->pg_PartitionLookup)
  {
    return(FALSE); /* out of memory */
  }

  /* create lookup table */
  pp = (struct PTPanPartition *) pg->pg_Partitions.lh_Head;
  for(cnt = 0; cnt < len; cnt++)
  {
    if(!pp->pp_Node.ln_Succ)
    {
      break; /* end of partition list reached */
    }
    do
    {
      range = pg->pg_PowerTable[pg->pg_MaxPrefixLen - pp->pp_PrefixLen];
      if((cnt >= pp->pp_Prefix * range) && (cnt < (pp->pp_Prefix+1) * range))
      {
    //printf("Entry %ld = prefix %ld\n", cnt, pp->pp_Prefix);
    pg->pg_PartitionLookup[cnt] = pp;
    break;
      } else {
    if(cnt >= (pp->pp_Prefix+1) * range)
    {
    /* we're past this partition prefix, get next one */
    pp = (struct PTPanPartition *) pp->pp_Node.ln_Succ;
    if(!pp->pp_Node.ln_Succ)
    {
        break; /* end of partition list reached */
    }
    } else {
    break; /* not yet found */
    }
      }
    } while(TRUE);
  }
  return(TRUE);
}
/* \\\ */
