#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "ptpan.h"
#include "pt_prototypes.h"
#include <arbdbt.h>
#include <servercntrl.h>
#include <server.h>
#include <client.h>
#include <struct_man.h>
#define MAX_TRY 3
#define TIME_OUT 1000*60*60*24

/*****************************************************************************
        NEW STUFF
*******************************************************************************/

/* /// "AllocPTPanGlobal()" */
struct PTPanGlobal * AllocPTPanGlobal(void)
{
  struct PTPanGlobal *pg;
  ULONG cnt;
  ULONG pval;
  ULONG numbits;
  struct MismatchWeights *mw;

  /**** init server main struct ****/
  printf("Init internal structs...\n");

  pg = (struct PTPanGlobal *) calloc(1, sizeof(struct PTPanGlobal));
  if(!pg)
  {
    printf("Error allocating PTPanGlobal!\n");
    return(FALSE);
  }
  /* init some stuff and precalc tables */
  NewList(&pg->pg_Species);
  NewList(&pg->pg_Partitions);
  pg->pg_AlphaSize = ALPHASIZE;

  /* calculate table of powers of alphabet size */
  pval = 1;
  for(cnt = 0, pval = 1; cnt <= MAXCODEFITLONG; cnt++)
  {
    pg->pg_PowerTable[cnt] = pval;
    pval *= pg->pg_AlphaSize;
  }

  /* calculate table of bits used for code of size n */
  pval = pg->pg_AlphaSize;
  numbits = 0;
  pg->pg_BitsUseTable[0] = 0;
  pg->pg_BitsMaskTable[0] = (1UL << (31-numbits));
  for(cnt = 1; cnt <= MAXCODEFITLONG; cnt++)
  {
    while(pval > (1UL << numbits))
    {
      numbits++;
    }
    pg->pg_BitsUseTable[cnt] = numbits; /* bits required for codesize */
    pg->pg_BitsShiftTable[cnt] = 32 - numbits; /* how many bits to shift left */
    pg->pg_BitsMaskTable[cnt] = (1UL << (31-numbits));
    pval *= pg->pg_AlphaSize;
  }

  /* bits count table */
  for(cnt = 0; cnt < 256; cnt++)
  {
    numbits = 0;
    pval = cnt;
    while(pval)
    {
      numbits += (pval & 1);
      pval >>= 1;
    }
    pg->pg_BitsCountTable[cnt] = numbits;
  }

  /* sequence compression tables */
  /* due to calloc, this table is already set to SEQCODE_N for all other codes */
  pg->pg_CompressTable[(UBYTE) 'a'] = SEQCODE_A;
  pg->pg_CompressTable[(UBYTE) 'A'] = SEQCODE_A;
  pg->pg_CompressTable[(UBYTE) 'c'] = SEQCODE_C;
  pg->pg_CompressTable[(UBYTE) 'C'] = SEQCODE_C;
  pg->pg_CompressTable[(UBYTE) 'g'] = SEQCODE_G;
  pg->pg_CompressTable[(UBYTE) 'G'] = SEQCODE_G;
  pg->pg_CompressTable[(UBYTE) 't'] = SEQCODE_T;
  pg->pg_CompressTable[(UBYTE) 'T'] = SEQCODE_T;
  pg->pg_CompressTable[(UBYTE) 'u'] = SEQCODE_T;
  pg->pg_CompressTable[(UBYTE) 'U'] = SEQCODE_T;
  pg->pg_CompressTable[(UBYTE) '-'] = SEQCODE_IGNORE; /* these chars don't go */
  pg->pg_CompressTable[(UBYTE) '.'] = SEQCODE_IGNORE; /* into the tree */
  pg->pg_CompressTable[0] = SEQCODE_IGNORE; /* terminal char, to optimize certain routines */

  /* inverse table for decompressing */
  pg->pg_DecompressTable[SEQCODE_N] = 'N';
  pg->pg_DecompressTable[SEQCODE_A] = 'A';
  pg->pg_DecompressTable[SEQCODE_C] = 'C';
  pg->pg_DecompressTable[SEQCODE_G] = 'G';
  pg->pg_DecompressTable[SEQCODE_T] = 'U';

  /* table for creating the complement sequence */
  pg->pg_ComplementTable[SEQCODE_N] = SEQCODE_N;
  pg->pg_ComplementTable[SEQCODE_A] = SEQCODE_T;
  pg->pg_ComplementTable[SEQCODE_C] = SEQCODE_G;
  pg->pg_ComplementTable[SEQCODE_G] = SEQCODE_C;
  pg->pg_ComplementTable[SEQCODE_T] = SEQCODE_A;

  /* counting table to avoid branches */
  for(cnt = 0; cnt < 256; cnt++)
  {
    pg->pg_SeqCodeValidTable[cnt] = (pg->pg_CompressTable[cnt] == SEQCODE_IGNORE) ? 0 : 1;
  }

  pg->pg_FreeMem = GB_get_physical_memory();
#ifdef DEBUG
  printf("physical_memory=%lu k (%lu Mb)\n", pg->pg_FreeMem, pg->pg_FreeMem >> 10);
#endif // DEBUG
  pg->pg_FreeMem <<= 10;

  /* initialize species cache handler */
  pg->pg_SpeciesCache = AllocCacheHandler();
  if(!pg->pg_SpeciesCache)
  {
    printf("Couldn't allocate species cache handler!\n");
    free(pg);
    return(FALSE);
  }
  pg->pg_SpeciesCache->ch_UserData = pg;
  /* reserve about 10% of the memory for the species cache */
  pg->pg_SpeciesCache->ch_MaxCapacity = (pg->pg_FreeMem / 10) * 1;
  pg->pg_SpeciesCache->ch_LoadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CacheSpeciesLoad;
  pg->pg_SpeciesCache->ch_UnloadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CacheSpeciesUnload;
  pg->pg_SpeciesCache->ch_SizeFunc = (ULONG (*)(struct CacheHandler *, APTR)) CacheSpeciesSize;

  /* initialize partitions cache handler */
  pg->pg_PartitionCache = AllocCacheHandler();
  if(!pg->pg_PartitionCache)
  {
    printf("Couldn't allocate partitions cache handler!\n");
    FreeCacheHandler(pg->pg_SpeciesCache);
    free(pg);
    return(FALSE);
  }
  pg->pg_PartitionCache->ch_UserData = pg;
  /* reserve about 90% of the memory for the partition cache */
  pg->pg_PartitionCache->ch_MaxCapacity = (pg->pg_FreeMem / 10) * 9;
  pg->pg_PartitionCache->ch_LoadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CachePartitionLoad;
  pg->pg_PartitionCache->ch_UnloadFunc = (BOOL (*)(struct CacheHandler *, APTR)) CachePartitionUnload;
  pg->pg_PartitionCache->ch_SizeFunc = (ULONG (*)(struct CacheHandler *, APTR)) CachePartitionSize;

  /* default mismatch weights */
  mw = &pg->pg_MismatchWeights;
  /* format: replace code1 (query) by code2 (database) adds x to the error value */
  /*     N   A   C   G   T (-> query)
      N 0.0 0.1 0.1 0.1 0.1
      A  *  0.0 1.0 1.0 1.0
      C  *  1.0 0.0 1.0 1.0
      G  *  1.0 1.0 0.0 1.0
      T  *  1.0 1.0 1.0 0.0
    ins 2.0 2.0 2.0 2.0 2.0
    del  *  2.0 2.0 2.0 2.0
  */
  /* fill diagonal first (no mismatch) */
  for(cnt = 0; cnt < ALPHASIZE; cnt++)
  {
    mw->mw_Replace[cnt * ALPHASIZE + cnt] = 0.0;
  }

  /* N is a joker, but setting it to slightly higher values might be sensible */
  mw->mw_Replace[SEQCODE_A * ALPHASIZE + SEQCODE_N] = 0.1;
  mw->mw_Replace[SEQCODE_C * ALPHASIZE + SEQCODE_N] = 0.1;
  mw->mw_Replace[SEQCODE_G * ALPHASIZE + SEQCODE_N] = 0.1;
  mw->mw_Replace[SEQCODE_T * ALPHASIZE + SEQCODE_N] = 0.1;

  /* replacing N by A, C, G, T will not occur (search string may not contain N) */
  mw->mw_Replace[SEQCODE_N * ALPHASIZE + SEQCODE_A] = 99999.0;
  mw->mw_Replace[SEQCODE_N * ALPHASIZE + SEQCODE_C] = 99999.0;
  mw->mw_Replace[SEQCODE_N * ALPHASIZE + SEQCODE_G] = 99999.0;
  mw->mw_Replace[SEQCODE_N * ALPHASIZE + SEQCODE_T] = 99999.0;

  /* other parts of the matrix (should be symmetrical, but doesn't need to) */
  mw->mw_Replace[SEQCODE_A * ALPHASIZE + SEQCODE_C] = 1.1;
  mw->mw_Replace[SEQCODE_C * ALPHASIZE + SEQCODE_A] = 1.0;

  mw->mw_Replace[SEQCODE_A * ALPHASIZE + SEQCODE_G] = 0.2;
  mw->mw_Replace[SEQCODE_G * ALPHASIZE + SEQCODE_A] = 1.5;

  mw->mw_Replace[SEQCODE_A * ALPHASIZE + SEQCODE_T] = 1.1;
  mw->mw_Replace[SEQCODE_T * ALPHASIZE + SEQCODE_A] = 1.1;

  mw->mw_Replace[SEQCODE_C * ALPHASIZE + SEQCODE_G] = 1.1;
  mw->mw_Replace[SEQCODE_G * ALPHASIZE + SEQCODE_C] = 1.5;

  mw->mw_Replace[SEQCODE_C * ALPHASIZE + SEQCODE_T] = 0.6;
  mw->mw_Replace[SEQCODE_T * ALPHASIZE + SEQCODE_C] = 1.1;

  mw->mw_Replace[SEQCODE_G * ALPHASIZE + SEQCODE_T] = 1.5;
  mw->mw_Replace[SEQCODE_T * ALPHASIZE + SEQCODE_G] = 0.6;

  /* insert operations (to query string) */
  mw->mw_Insert[SEQCODE_N] = 2.0;
  mw->mw_Insert[SEQCODE_A] = 2.0;
  mw->mw_Insert[SEQCODE_C] = 2.0;
  mw->mw_Insert[SEQCODE_G] = 2.0;
  mw->mw_Insert[SEQCODE_T] = 2.0;

  /* delete operations (from query string) */
  mw->mw_Delete[SEQCODE_N] = 99999.0; /* should never happen */
  mw->mw_Delete[SEQCODE_A] = 2.0;
  mw->mw_Delete[SEQCODE_C] = 2.0;
  mw->mw_Delete[SEQCODE_G] = 2.0;
  mw->mw_Delete[SEQCODE_T] = 2.0;

  /* init matrix for non-weighted stuff */
  mw = &pg->pg_NoWeights;
  /* fill standard 1.0 values */
  for(cnt = 0; cnt < (ALPHASIZE * ALPHASIZE); cnt++)
  {
    mw->mw_Replace[cnt] = 1.0;
  }
  /* fill diagonal first (no mismatch) and insert / delete */
  for(cnt = 0; cnt < ALPHASIZE; cnt++)
  {
    mw->mw_Replace[cnt * ALPHASIZE] = 0.1; // N (joker) replacement
    mw->mw_Replace[cnt * ALPHASIZE + cnt] = 0.0; // diagonal
    mw->mw_Insert[cnt] = 2.0;
    mw->mw_Delete[cnt] = 2.0;
  }

  /* calculate maximum partition size (estimate 24 bytes per node) */
  {
    ULONG partmem = pg->pg_FreeMem;
    /* tree building implementation is limited to max. 1 GB per partition */
    if(partmem > (1UL<<30))
    {
      partmem = 1UL<<30;
    }

    pg->pg_MaxPartitionSize = partmem / (((sizeof(struct SfxNode2Edges) * SMALLNODESPERCENT) +
          (sizeof(struct SfxNodeNEdges) * BIGNODESPERCENT)) / 100);
  }
  /* enable low memory mode */
  pg->pg_LowMemoryMode = TRUE;

  gettimeofday(&pg->pg_Bench.ts_Init, NULL);
  pg->pg_Bench.ts_Last = pg->pg_Bench.ts_Init;

  /* init command line flags */
  pg->pg_verbose = 0;

  return(pg);
}
/* \\\ */

/* /// "FreePTPanGlobal()" */
void FreePTPanGlobal(struct PTPanGlobal *pg)
{
  FlushCache(pg->pg_SpeciesCache);
  FreeCacheHandler(pg->pg_SpeciesCache);
  FlushCache(pg->pg_PartitionCache);
  FreeCacheHandler(pg->pg_PartitionCache);
  free(pg);
}
/* \\\ */

/*****************************************************************************
        END OF NEW STUFF
*******************************************************************************/

// *** FIXME *** see if we can get rid of this global structures

struct PTPanGlobal *PTPanGlobalPtr = NULL;

/*****************************************************************************
        Communication
*******************************************************************************/

PT_main *aisc_main; /* muss so heissen */

extern "C" int server_shutdown(PT_main *, aisc_string passwd)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;

  printf("EXTERN: server_shutdown\n");
  /** passwdcheck **/
  if(strcmp(passwd, "47@#34543df43%&3667gh"))
  {
    return 1;
  }
  printf("\nI got the shutdown message.\n");
  /** shoot clients **/
  aisc_broadcast(pg->pg_ComSocket, 0,
        "SERVER UPDATE BY ADMINISTRATOR!\n"
        "You'll get the latest version. Your on-screen\n"
        "information will be lost, sorry!");
  /** shutdown **/
  aisc_server_shutdown_and_exit(pg->pg_ComSocket, 0);
  return(0);
}

extern "C" int broadcast(PT_main *main, int)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;

  printf("EXTERN: broadcast\n");
  aisc_broadcast(pg->pg_ComSocket, main->m_type, main->m_text);
  return(0);
}

extern int aisc_core_on_error;

int main(int argc, char *argv[])
{
  struct PTPanGlobal *pg;
  STRPTR commandflag;

  printf("\nTUM PeTer PAN SERVER (Chris Hodges) V0.12 18-Aug-04 (C) 2003-2004\n"
    "Complete rewrite of the original code by Oliver Strunk from 1993\n\n");

  /* allocate the PTPanGlobal structure */
  if(!(pg = AllocPTPanGlobal()))
  {
    exit(1);
  }

  /* argh! global variable! would be nice, if we could get rid of this --
     it is only used by the AISC functions */
  PTPanGlobalPtr = pg;

  /* aisc init */
  GB_install_pid(0); /* not arb_clean able */
  aisc_core_on_error = 0;
  pg->pg_AISC = create_PT_main();

  GB_init_gb(); // nedded for PT_new_design

  /* set global variable -- sigh */
  aisc_main = pg->pg_AISC;

  /* first get the parameters */
  pg->pg_ArbParams = arb_trace_argv(&argc, argv);

  /* try to open com with any other pb server */
  /* check command line syntax */
  if((argc > 2) ||
     ((argc < 2) && !pg->pg_ArbParams->db_server) ||
     (argc >= 2 && strcmp(argv[1], "--help") == 0))
  {
    printf("Syntax: %s [-look/-build/-kill/-QUERY] -Dfile.arb -TSocketid\n", argv[0]);
    exit(-1);
  }
  /* add default command flag */
  if(argc == 2)
  {
    commandflag = argv[1];
  } else {
    commandflag = (STRPTR) "-boot";
  }

  /* get server host name */
  if(!(pg->pg_ServerName = pg->pg_ArbParams->tcp))
  {
    if(!(pg->pg_ServerName = (STRPTR) GBS_read_arb_tcp("ARB_PT_SERVER0")))
    {
      GB_print_error(); /* no host name found */
      exit(-1);
    }
  }

  /* generate tree filename */
  pg->pg_DBName    = pg->pg_ArbParams->db_server;
  pg->pg_IndexName = GBS_global_string_copy("%s.pan", pg->pg_DBName);

  /* check for other active servers */
  {
    aisc_com *ptlink;
    T_PT_MAIN ptmain;
    ptlink = (aisc_com *) aisc_open(pg->pg_DBName, &ptmain, AISC_MAGIC_NUMBER);
    if(ptlink)
    {
      if(!strcasecmp(commandflag, "-look"))
      {
        exit(0); /* already another serther */
      }
      printf("There is another active server. I'll try to terminate it violently...\n");
      aisc_nput(ptlink, PT_MAIN, ptmain, MAIN_SHUTDOWN, "47@#34543df43%&3667gh", NULL);
      aisc_close(ptlink);
    }
  }
  if(!strcmp(commandflag, "-kill"))
  {
    exit(0);
  }

  if(!strncasecmp(commandflag, "-build", 6)) /* build command */
  {
    ULONG val = atoi(&commandflag[6]);
    if(val) /* extra option */
    {
      if(val > 100000) /* read out threshold */
      {
        pg->pg_MaxPartitionSize = atoi(&commandflag[6]);
        printf("Forcing MaxPartitionSize = %ld.\n", pg->pg_MaxPartitionSize);
      } else {
        pg->pg_PruneLength = atoi(&commandflag[6]);
        printf("Forcing PruneLength = %d.\n", pg->pg_PruneLength);
      }
    }
    LoadSpecies(pg);
    if(!strncmp(commandflag, "-bUiLd", 6))
    {
      pg->pg_UseStdSfxTree = TRUE;
      if(BuildStdSuffixTree(pg))
      {
        printf("Suffix Tree index for database '%s' has been created.\n", pg->pg_DBName);
        BenchOutput(pg);
        exit(0);
      } else {
        printf("Unable to create Suffix Tree index for database '%s'!\n", pg->pg_DBName);
        exit(1);
      }
    } else {
      if(BuildPTPanIndex(pg))
      {
        printf("PT_PAN index for database '%s' has been created.\n", pg->pg_DBName);
        BenchOutput(pg);
        exit(0);
      } else {
        printf("Unable to create PT_PAN index for database '%s'!\n", pg->pg_DBName);
        exit(1);
      }
    }
  }

  if(!strcasecmp(commandflag, "-QUERY"))
  {
    //enter_stage_3_load_tree(aisc_main, tname); /* now stage 3 */
    exit(0);
  }

  /* Check if index is up2date */
  {
    struct stat dbstat, idxstat;
    BOOL forcebuild = FALSE;
    if(stat(pg->pg_DBName, &dbstat))
    {
      printf("PT_PAN: error while stat source %s\n", pg->pg_DBName);
      aisc_server_shutdown_and_exit(pg->pg_ComSocket, -1);
    }

    if(stat(pg->pg_IndexName, &idxstat))
    {
      forcebuild = TRUE; /* there is no index at all! */
    } else {
      if((dbstat.st_mtime > idxstat.st_mtime) || (idxstat.st_size == 0))
      {
        /* so the index file was older or of zero size */
        printf("PT_PAN: Database %s has been modified\n"
            "more recently than index %s.\n"
            "Forcing rebuilding of index...\n",
            pg->pg_DBName, pg->pg_IndexName);
        forcebuild = TRUE;
      }
      if(!LoadIndexHeader(pg))
      {
        forcebuild = TRUE; /* an error occured while loading the index header */
      }
    }
    if(forcebuild)
    {
      LoadSpecies(pg);
      if(BuildPTPanIndex(pg))
      {
        printf("PT_PAN index for database '%s' has been created.\n", pg->pg_DBName);
      } else {
        printf("Unable to create PT_PAN index for database '%s'!\n", pg->pg_DBName);
        exit(1);
      }
      if(!LoadIndexHeader(pg))
      {
        printf("Fatal error, couldn't load index even after creation attempt!\n");
        exit(1);
      }
    }
  }

  /*if(!LoadAllPartitions(pg))
  {
    printf("ERROR: Failed to load partitions into memory!\n");
    exit(1);
    }*/

  /* so much for the the init, now let's do some real work */

  if(!strcasecmp(commandflag, "-v")) pg->pg_verbose = 1;
  if(!strcasecmp(commandflag, "-vv")) pg->pg_verbose = 2;
  if(!strcasecmp(commandflag, "-vvv")) pg->pg_verbose = 3;

#if 0
  {
    PT_exProb pep;
    pep.result = NULL;
    pep.restart = 1;
    pep.plength = 21;
    pep.numget = 100;
    PT_find_exProb(&pep);
    printf("%s\n", pep.result);
  }
#endif

  /* open the socket connection */
  printf("Opening connection...\n");
  //sleep(1);
  {
    UWORD i;
    for(i = 0; i < MAX_TRY; i++)
    {
      if((pg->pg_ComSocket = open_aisc_server(pg->pg_ServerName, TIME_OUT, 0)))
      {
        break;
      } else {
        sleep(10);
      }
    }
    if(!pg->pg_ComSocket)
    {
      printf("PT_PAN: Gave up on opening the communication socket!\n");
      exit(0);
    }
  }
  /****** all ok: main loop ********/

  printf("ok, server is running.\n");

  aisc_accept_calls(pg->pg_ComSocket);
  aisc_server_shutdown_and_exit(pg->pg_ComSocket, 0);

  return(0);
}


