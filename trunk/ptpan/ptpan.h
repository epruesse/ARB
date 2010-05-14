#include "globalprefs.h"
#include "types.h"
#include "dlist.h"
#include "hooks.h"

#include <PT_com.h>
#include <arbdb.h>
#include "pt_manualprotos.h"

#define FILESTRUCTVERSION 0x0105 /* version<<8 and revision */

#define BENCHMARK

/* compressed sequence codes */
#define SEQCODE_N      0
#define SEQCODE_A      1
#define SEQCODE_C      2
#define SEQCODE_G      3
#define SEQCODE_T      4
#define SEQCODE_DOT    5
#define SEQCODE_HYPHEN 6
#define SEQCODE_IGNORE 255

/* 5261276th prime, small enough for pg->pg_AlphaSize < 47 */
#define HASHPRIME 90809777

/* mismatch matrix and edit distance weights */
struct MismatchWeights
{
  float       mw_Replace[ALPHASIZE*ALPHASIZE]; /* costs for replacing one char by the other */
  float       mw_Insert[ALPHASIZE]; /* cost for inserting one char */
  float       mw_Delete[ALPHASIZE]; /* cost for deleting one char */
};

/* benchmarking/debug */
struct TimeStats
{
  struct timeval ts_Init;        /* first time stamp after init */
  struct timeval ts_End;         /* total time */
  struct timeval ts_Last;        /* last stamp */
  struct timeval ts_Now;         /* temporary stamp */

  ULONG ts_CollectDB;            /* scanning for species */
  ULONG ts_MergeDB;              /* merging database */
  ULONG ts_PrefixScan;           /* partition prefix scanning */
  ULONG ts_MemTree;              /* memory tree generation */
  ULONG ts_TreeStats;            /* depth, short edge, long edge */
  ULONG ts_LongDictPre;          /* long edge dict presorting */
  ULONG ts_LongDictBuild;        /* long edge dict building */
  ULONG ts_Reloc;                /* relocation */
  ULONG ts_Writing;              /* writing of tree to disk */
  ULONG ts_TotalBuild;           /* total time spent for building */
  ULONG ts_Hits;                 /* total number of candidates */
  ULONG ts_UnsafeHits;           /* number of hits being verified */
  ULONG ts_UnsafeKilled;         /* number of false positives */
  ULONG ts_DupsKilled;           /* duplicates filtered */
  ULONG ts_CrossBoundKilled;     /* hits, that were filtered due to cross boundary */
  ULONG ts_DotsKilled;           /* filtered hits that were in non-coding areas */
  ULONG ts_OutHits;              /* number of output lines generated */
  ULONG ts_CandSetTime;          /* time for generating candidates */
  ULONG ts_OutputTime;           /* time for generating output */
};

/* global persistent data structure */
struct PTPanGlobal
{
  struct Node pg_Node;           /* linkage */

  /* communication stuff */
  PT_main    *pg_AISC;           /* main communication pointer */
  arb_params *pg_ArbParams;
  STRPTR      pg_ServerName;     /* hostname of server */
  Hs_struct *pg_ComSocket; /* the communication socket */

  BOOL        pg_UseStdSfxTree;  /* selects between PTPan and SfxTree */
  FILE       *pg_IndexFile;      /* file handle for main index file */

  STRPTR      pg_DBName;         /* DataBase name */
  STRPTR      pg_IndexName;      /* name of primary (?) index file */

  /* Main data base information */
  GBDATA     *pg_MainDB;         /* ARBDB interface */
  GBDATA     *pg_SaiData;

  STRPTR      pg_AlignmentName;  /* Name of the alignment in DB */

  GBDATA     *pg_SpeciesData;    /* DB pointer to species data */
  ULLONG      pg_TotalSeqSize;   /* total size of sequences */
  ULLONG      pg_TotalSeqCompressedSize;     // total size of compressed Seq. Data (with '.' and '-') in byte
  ULLONG      pg_TotalRawSize;   /* total size of data tuples in species */
  UWORD       pg_TotalRawBits;   /* number of bits required to store a position pointer */
  ULONG       pg_MaxBaseLength;  /* maximum base length over all species */
  ULONG       pg_AllHashSum;     /* hash sum over everything to checksum integrity */

  ULONG       pg_NumSpecies;     /* number of species accounted for */
  struct List pg_Species;        /* list of species data */

  /* stuff for building the index and partitions */
  ULONG      *pg_MergedRawData;  /* all compressed and merged sequence data */
  ULONG       pg_MaxPartitionSize; /* maximum allowed number of tuples in each partition */
  ULONG      *pg_HistoTable;     /* prefix histogramm */
  UWORD       pg_NumPartitions;  /* number of partitions generated */
  struct List pg_Partitions;     /* prefix partitions */
  UWORD       pg_MaxPrefixLen;   /* maximum used prefix length */
  struct PTPanPartition **pg_PartitionLookup; /* quick lookup table */

  /* precalculated Tables for speedup */
  UWORD       pg_AlphaSize;      /* size of alphabet */
  ULONG       pg_PowerTable[MAXCODEFITLONG+1]; /* stores powers of ALPHASIZE^i */
  UWORD       pg_BitsShiftTable[MAXCODEFITLONG+1]; /* how many bits to shift left to get valid code */
  UWORD       pg_BitsUseTable[MAXCODEFITLONG+1]; /* how many bits are required to store ALPHASIZE^i */
  ULONG       pg_BitsMaskTable[MAXCODEFITLONG+1]; /* eof bit mask for i codes */

  UWORD       pg_BitsCountTable[256]; /* count how many bits are set */

  UBYTE       pg_CompressTable[256]; /* table to compress char to { N, A, C, G, T } */
  UBYTE       pg_DecompressTable[ALPHASIZE]; /* inverse mapping of above */
  UBYTE       pg_ComplementTable[ALPHASIZE]; /* table with A<->T, C<->G swapped */
  UBYTE       pg_SeqCodeValidTable[256]; /* 1 for valid char, 0 for invalid (-.) */

  /* various stuff */
  UWORD       pg_PruneLength;    /* Pruning control for depth and length */
  BOOL        pg_LowMemoryMode;  /* whether to load and keep all sequences in memory */
  ULONG       pg_FreeMem;        /* free memory in KB */
  STRPTR      pg_EcoliSeq;       /* Ecoli sequence data */
  ULONG       pg_EcoliSeqSize;   /* length of ecoli sequence */
  ULONG      *pg_EcoliBaseTable; /* quick table lookup for ecoli base position */

  /* species name hashing */
  struct BinTree *pg_SpeciesBinTree; /* tree to find a species by its absolute offset */
  struct PTPanSpecies **pg_SpeciesMap; /* direct mapping to support legacy stuff */

  GB_HASH    *pg_SpeciesNameHash; /* species name to int */

  struct CacheHandler *pg_SpeciesCache; /* alignment caching */
  struct CacheHandler *pg_PartitionCache; /* partition caching */

  /* mismatch matrix */
  struct MismatchWeights pg_MismatchWeights; /* matrix containing the default mismatch values */
  struct MismatchWeights pg_NoWeights; /* matrix containing the default mismatch values */

  /* output stuff */
  PT_local   *pg_SearchPrefs;    /* search prefs from GUI */
  bytestring  pg_ResultString;   /* complete merged output string (header/name/format) */
  bytestring  pg_ResultMString;  /* complete merged output string (name/mismatch/wmis) */
  bytestring  pg_SpeciesString;  /* complete merged species string (species) */

  bytestring  pg_UnknownSpecies; /* string containing the unknown species */

  char        pg_TempBuffer[1024]; /* temporary buffer */

  /* benchmarking */
  struct TimeStats pg_Bench;     /* Benchmarking data structure */

  /* Command line flags */
  UWORD pg_verbose;               /* Verbosity level to stdout for debugging */
};

extern struct PTPanGlobal *PTPanGlobalPtr;

/* information on a species */
struct PTPanSpecies
{
  struct Node ps_Node;           /* linkage */
  ULONG       ps_Num;            /* species number for map */
  GBDATA     *ps_SpeciesDB;      /* database pointer */
  GBDATA     *ps_SeqDataDB;      /* alignment data pointer */
  STRPTR      ps_Name;           /* short name of species */
  STRPTR      ps_FullName;       /* long name of species */

  STRPTR      ps_SeqData;        /* original sequence data */
  ULONG       ps_SeqDataSize;    /* length of original sequence data */
  ULONG      *ps_RawData;        /* compressed sequence data pointer */
  ULONG       ps_RawDataSize;    /* length of alignment data (filtered) */
  ULONG       ps_SeqHash;        /* sequence hash sum */

  BOOL        ps_IsGroup;        /* marked for probe design */
  BOOL        ps_Obsolete;       /* this sequence has been removed from the DB, ignore */
  ULLONG      ps_AbsOffset;      /* absolute offset in the resulting raw image */
  ULONG       ps_SerialTouch;    /* last time touched (for duplicate elimation) */
  struct CacheNode *ps_CacheNode; /* caching information node */
  
  STRPTR ps_test;
  UBYTE      *ps_SeqDataCompressed;     // compressed Seq. Data (with '.' and '-')
  ULONG       ps_SeqDataCompressedSize; // size in bits (includes end flag 111)
};

/* index partition structure (memory) */
struct PTPanPartition
{
  struct Node pp_Node;           /* linkage */
  struct PTPanGlobal *pp_PTPanGlobal; /* up link */
  ULONG       pp_ID;             /* partition id, e.g. used for filename */
  ULONG       pp_Prefix;         /* compressed prefix code */
  ULONG       pp_PrefixLen;      /* length of prefix code */
  STRPTR      pp_PrefixSeq;      /* prefix sequence */
  ULONG       pp_Size;           /* number of nodes for this prefix */
  ULLONG      pp_RawOffset;      /* offset of this partition inside the total raw data */

  BOOL        pp_Done;           /* flag to check, if this partition has been done yet */

  /* variables for alternate suffix tree implementation */
  struct StdSfxNode *pp_StdSfxNodes; /* suffix node array */

  /* intermediate tree in memory */
  struct SfxNode *pp_SfxRoot;    /* root of all evil */
  ULONG       pp_SfxMemorySize;  /* size of the node array */
  UBYTE      *pp_SfxNodes;       /* suffix node array */
  ULONG       pp_Sfx2EdgeOffset; /* offset for two edge nodes */
  ULONG       pp_SfxNEdgeOffset; /* offset for five edge nodes */
  ULONG       pp_NumBigNodes;    /* number of big nodes */
  ULONG       pp_NumSmallNodes;  /* number of small nodes */
  ULONG       pp_MaxTreeDepth;   /* maximum measured tree depth */
  ULONG       pp_TreePruneDepth; /* at which depth should the tree be pruned? */
  ULONG       pp_TreePruneLength; /* length at which tree should be pruned? */
  ULONG       pp_PruneLeafCnt;   /* count of leaves after pruning */

  UBYTE      *pp_VerifyArray;    /* bitarray to check, if all positions are found */

  struct TreeLevelStats *pp_LevelStats; /* stats for each level */

  ULONG      *pp_QuickPrefixLookup; /* hash table lookup */
  ULONG       pp_QuickPrefixCount; /* number of time the qpl was taken */

  struct HuffCode *pp_BranchCode; /* branch combination histogram */

  /* edges huffman code stuff */
  ULONG       pp_EdgeCount;      /* number of edges in the final tree */
  ULONG       pp_LongEdgeCount;  /* number of long edges */

  struct HuffCode *pp_ShortEdgeCode; /* short edge combination histogram */
  struct SfxNode **pp_LongEdges;  /* sorted array of tree pointers to long edges */

  STRPTR      pp_LongDict;       /* Dictionary for long edges */
  ULONG       pp_LongDictSize;   /* allocation size */
  UWORD       pp_LongRelPtrBits; /* bits to use to reference a long edge offset */

  struct HashArray *pp_LongDictHash; /* lookup hash array to speed up search */
  ULONG       pp_LongDictHashSize; /* size of hash */

  ULONG       pp_LongEdgeLenSize; /* size of the huffman array (longest length + 1) */
  struct HuffCode *pp_LongEdgeLenCode; /* long edges length histogram */

  ULONG      *pp_LongDictRaw;    /* compressed long dictionary */
  ULONG       pp_LongDictRawSize; /* compressed long dictionary size on disk */

  /* leaf collection */
  LONG       *pp_LeafBuffer;     /* temporary leaf buffer */
  ULONG       pp_LeafBufferSize; /* size of leaf buffer */
  LONG       *pp_LeafBufferPtr;  /* current pointer position */

  /* file saving */
  STRPTR      pp_PartitionName;  /* file name of partition */
  FILE       *pp_PartitionFile;  /* file handle for partition */
  ULONG       pp_TraverseTreeRoot; /* start of in order traversal (sn_Parent) */
  ULONG       pp_DiskTreeSize;   /* backward position on disk (growing size) */
  ULONG       pp_DiskNodeCount;  /* number of inner nodes written */
  ULONG       pp_DiskNodeSpace;  /* memory used for nodes */
  ULONG       pp_DiskLeafCount;  /* number of inner leaf nodes */
  ULONG       pp_DiskLeafSpace;  /* memory used for leaf nodes */
  ULONG       pp_DiskOuterLeaves; /* number of leaf positions saved in arrays */
  ULONG       pp_DiskIdxSpace;   /* size of file on disk */
  UBYTE      *pp_DiskBuffer;     /* Buffer for writing tree */
  ULONG       pp_DiskBufferSize; /* Size of disk buffer */
  ULONG       pp_DiskPos;        /* Fill state of buffer */
  UBYTE      *pp_DiskTree;       /* start of compressed tree */
  UBYTE      *pp_MapFileBuffer;  /* buffer for mapped file */
  ULONG       pp_MapFileSize;    /* size of virtual memory */
  UBYTE      *pp_StdSfxMapBuffer; /* buffer for mapped source text (StdSfx) */

  /* node decrunching */
  struct HuffTree *pp_BranchTree; /* branch combination tree */
  struct HuffTree *pp_ShortEdgeTree; /* short edge combination tree */
  struct HuffTree *pp_LongEdgeLenTree; /* long edges length tree */

  struct CacheNode *pp_CacheNode; /* caching information node */
};

#define RELOFFSETBITS 28
#define RELOFFSETMASK ((1UL << RELOFFSETBITS) - 1)
#define LEAFBIT       31
#define LEAFMASK      0x80000000

/* suffix tree stub node for long edge extra intervals */
struct SfxNodeStub
{
  ULONG       sn_StartPos;    /* start position of edge (or prefix or dictionary position) */
  UWORD       sn_EdgeLen;     /* length of edge */
  UWORD       sn_Dummy;       /* alpha mask */
};

/* suffix tree inner node for two edges */
struct SfxNode2Edges
{
  ULONG       sn_StartPos;    /* start position */
  UWORD       sn_EdgeLen;     /* length of edge */
  UWORD       sn_AlphaMask;   /* alpha mask */
  ULONG       sn_Parent;      /* pointer to parent offset (upper 4 bits for edge count) */
  ULONG       sn_Children[2]; /* array of children (lower 28 bits for array offset,
                                 upper 4 bits for edge id and terminal bit */
};

/* suffix tree inner node for five edges */
struct SfxNodeNEdges
{
  ULONG       sn_StartPos;    /* start position */
  UWORD       sn_EdgeLen;     /* length of edge */
  UWORD       sn_AlphaMask;   /* alpha mask */
  ULONG       sn_Parent;      /* pointer to parent offset (upper 4 bits for edge count) */
  ULONG       sn_Children[ALPHASIZE]; /* array of children (lower 28 bits for array
                                offset ,upper 4 bits for edge id and terminal bit */
};

/* suffix tree node template */
struct SfxNode
{
  ULONG       sn_StartPos;    /* start position of edge (or prefix or dictionary position) */
  UWORD       sn_EdgeLen;     /* length of edge */
  UWORD       sn_AlphaMask;   /* alpha mask */
  ULONG       sn_Parent;      /* pointer to parent offset (upper 4 bits for edge count) */
  ULONG       sn_Children[1]; /* array of children */
};

/* alternate implementation: standard suffix tree */
struct StdSfxNode
{
  ULONG       ssn_StartPos;   /* start position of edge */
  ULONG       ssn_EdgeLen;    /* length of edge */
  ULONG       ssn_FirstChild; /* first child offset */
  ULONG       ssn_NextSibling; /* next sibling offset */
  ULONG       ssn_Parent;     /* parent node offset */
  ULONG       ssn_Prime;      /* link node offset */
};

/* StdSfxNode truncated */
struct StdSfxNodeOnDisk
{
  ULONG       ssn_StartPos;   /* start position of edge */
  ULONG       ssn_EdgeLen;    /* length of edge */
  ULONG       ssn_FirstChild; /* first child offset */
  ULONG       ssn_NextSibling; /* next sibling offset */
};

/* tree statistics */
struct TreeLevelStats
{
  ULONG       tls_NodeCount;      /* number of nodes on this level */
  ULONG       tls_LeafCount;      /* number of leaves on this level */
  ULONG       tls_TotalNodeCount; /* number of nodes on this level and below */
  ULONG       tls_TotalLeafCount; /* number of leaves on this level and below */
  ULONG       tls_LevelSize;      /* number of bytes used on for this level */
  ULONG       tls_Offset;         /* current position < tls_LevelSize */
};

/* decrunched tree node data structure */
struct TreeNode
{
  struct PTPanPartition *tn_PTPanPartition; /* uplinking */
  ULONG       tn_Pos;         /* absolute tree position */
  ULONG       tn_Size;        /* size of the uncompressed node in bytes */
  UWORD       tn_Level;       /* level of the tree node */

  UWORD       tn_ParentSeq;   /* branch that lead there */
  ULONG       tn_ParentPos;   /* position of parent node */
  struct TreeNode *tn_Parent; /* parent node */
  UWORD       tn_TreeOffset;  /* sum of the parent edges (i.e. relative string position) */
  UWORD       tn_EdgeLen;     /* length of edge */
  STRPTR      tn_Edge;        /* decompressed edge */

  ULONG       tn_Children[ALPHASIZE]; /* absolute position of the children */
  ULONG       tn_NumLeaves;   /* number of leaves in this node */
  ULONG       tn_Leaves[1];   /* variable size of leaf array */
};

/* values for sq_SortMode */
#define SORT_HITS_NOWEIGHT   0 /* use mismatch count */
#define SORT_HITS_WEIGHTED   1 /* use weighted error */

/* automata state for search algorithm */
struct SearchQueryState
{
  struct TreeNode *sqs_TreeNode; /* tree node */
  ULONG       sqs_SourcePos;  /* current position within source string (linear only) */
  ULONG       sqs_QueryPos;   /* current position within query */
  UWORD       sqs_ReplaceCount; /* number of replace operations so far */
  UWORD       sqs_InsertCount; /* number of insert operations so far */
  UWORD       sqs_DeleteCount; /* number of delete operations so far */
  UWORD       sqs_NCount;     /* number of N-sequence codes encountered */
  float       sqs_ErrorCount; /* errors currently found */
};

/* parameter block and variables for a query */
struct SearchQuery
{
  struct Node sq_Node;        /* linkage */
  struct PTPanGlobal *sq_PTPanGlobal; /* up link */
  struct PTPanPartition *sq_PTPanPartition; /* up link */

  /* prefs/settings */
  STRPTR      sq_SourceSeq;   /* for linear comparision ONLY */
  STRPTR      sq_Query;       /* query string */
  ULONG       sq_QueryLen;    /* length of the query string */
  BOOL        sq_Reversed;    /* query is reversed */
  BOOL        sq_AllowReplace; /* allow replace operations */
  BOOL        sq_AllowInsert; /* allow inserting operations */
  BOOL        sq_AllowDelete; /* allow delete operations */
  ULONG       sq_KillNSeqsAt; /* if string has too many Ns, kill it */
  float       sq_MinorMisThres; /* threshold for small letter mismatch output */
  struct MismatchWeights *sq_MismatchWeights; /* pointer to matrix for mismatches */
  float       sq_MaxErrors;   /* maximum allowed errors */
  double*     sq_PosWeight;   // mismatch multiplier (Gaussian like distribution over search string)

  /* results */
  struct HashArray *sq_HitsHash; /* hash array to avoid double entries */
  ULONG       sq_HitsHashSize; /* size of hash array */
  struct List sq_Hits;        /* list of results */
  ULONG       sq_NumHits;     /* number of hits stored in array */
  UWORD       sq_SortMode;    /* describes the ordering for sorting */

  struct SearchQueryState sq_State; /* running variables */
};

#define QHF_UNSAFE   0x0001   /* this hit must be verified */
#define QHF_REVERSED 0x0002   /* this hit was found in the reversed sequence */
#define QHF_ISVALID  0x8000   /* it's a valid hit */

/* data stored for a candidate */
struct QueryHit
{
  struct Node qh_Node;        /* linkage */
  ULLONG      qh_AbsPos;      /* absolute position where hit was found */
  UWORD       qh_Flags;       /* is this still valid */
  UWORD       qh_ReplaceCount; /* Amount of replace operations required */
  UWORD       qh_InsertCount; /* Amount of insert operations required */
  UWORD       qh_DeleteCount; /* Amount of delete operations required */
  float       qh_ErrorCount;  /* errors that were needed to reach that hit */
  struct PTPanSpecies *qh_Species; /* pointer to species this hit is in */
};

/* data tuple for collected hits */
struct HitTuple
{
  ULLONG      ht_AbsPos;      /* absolute position of hit */
  struct PTPanSpecies *ht_Species; /* link to species */
};

/* probe candidate */
struct DesignHit
{
  struct Node dh_Node;        /* linkage */
  STRPTR      dh_ProbeSeq;    /* sequence of the probe */
  ULONG       dh_GroupHits;   /* number of species covered by the hit */
  ULONG       dh_NonGroupHits; /* number of non group hits inside this hit */
  ULONG       dh_Hairpin;     /* number of hairpin bonds */
  double      dh_Temp;        /* temperature */
  UWORD       dh_GCContent;   /* number of G and C nucleotides in the hit */
  ULONG       dh_NumMatches;  /* number of hits in this puddle */
  struct HitTuple *dh_Matches; /* Absolute pos of all hits */
  struct SearchQuery *dh_SearchQuery; /* search query for probe quality calculation */
  ULONG       dh_NonGroupHitsPerc[PERC_SIZE]; /* number of nongroup hits with decreasing temperature */
};

/* data structure for probe design */
struct DesignQuery
{
  struct Node dq_Node;        /* linkage */
  struct PTPanGlobal *dq_PTPanGlobal; /* up link */
  struct PTPanPartition *dq_PTPanPartition; /* up link */
  ULONG       dq_Serial;      /* running number for duplicate detection */
  UWORD       dq_ProbeLength; /* length of the probe */
  ULONG       dq_MarkedSpecies; /* number of marked species */
  ULONG       dq_MinGroupHits; /* minimum species covered by the hit */
  ULONG       dq_MaxNonGroupHits; /* number of non group hits allowed */
  ULONG       dq_MaxHairpin;  /* maximum number of hairpin bonds */
  double      dq_MinTemp;     /* minimum temperature */
  double      dq_MaxTemp;     /* maximum temperature */
  UWORD       dq_MinGC;       /* minimum number of G and C nucleotides in the hit */
  UWORD       dq_MaxGC;       /* maximum number of G and C nucleotides in the hit */
  ULONG       dq_MaxHits;     /* maximum number of matches */
  ULONG       dq_NumHits;     /* number of matches */
  struct List dq_Hits;        /* list of matches */

  struct DesignHit dq_TempHit; /* temporary memory to avoid too many alloc/free's */
  ULONG       dq_TempMemSize; /* memory allocated */

  struct TreeNode *dq_TreeNode; /* current tree node traversed */

  PT_pdc     *dq_PDC;         /* legacy link */

};

/* cache handler information node (private) */
struct CacheHandler
{
  struct Node ch_Node;           /* linkage */
  struct List ch_UsedNodes;      /* list of loaded nodes */
  struct List ch_FreeNodes;      /* list of free nodes */
  APTR        ch_UserData;       /* public: user data pointer */
  ULONG       ch_SwapCount;      /* swapping count */

  /* private stuff */
  BOOL        ch_CacheDisabled;  /* do not cache new stuff */
  ULONG       ch_AccessID;       /* running sequence ID */
  ULONG       ch_MemUsage;       /* memory used for this cache list */
  ULONG       ch_MaxCapacity;    /* how much to hold in this cache maximum */

  BOOL (*ch_LoadFunc)(struct CacheHandler *, APTR); /* callback hook to load data */

  BOOL (*ch_UnloadFunc)(struct CacheHandler *, APTR); /* callback hook to unload data */
  ULONG (*ch_SizeFunc)(struct CacheHandler *, APTR); /* callback hook to determine size of data */

  struct CacheNode *ch_LastNode; /* for unloading (in no caching mode) */
};

/* node containing statistical and management data for caching */
struct CacheNode
{
  struct Node cn_Node;           /* linking node */
  APTR        cn_UserData;       /* link to the user node this is attached to */
  ULONG       cn_LastUseID;      /* access sequence ID of last use */
  BOOL        cn_Loaded;         /* is loaded */
};

/* huffman tree node */
struct HuffTree
{
  struct HuffTree *ht_Child[2]; /* children */
  ULONG       ht_ID;          /* ID to return */
  ULONG       ht_Codec;       /* complete codec */
  UWORD       ht_CodeLength;  /* code length */
};

/* huffman table entry */
struct HuffCode
{
  ULONG       hc_Weight;      /* weight of item */
  ULONG       hc_Codec;       /* actual bits (in lsb) */
  UWORD       hc_CodeLength;  /* lengths in bits */
};

/* temporary internal node type */
struct HuffCodeInternal
{
  ULONG       hc_ID;          /* code id */
  ULONG       hc_Weight;      /* weight of item */
  UWORD       hc_Left;        /* ID of left parent */
  UWORD       hc_Right;       /* ID of right parent */
};

/* dynamic hashing struct */
struct HashArray
{
  struct HashEntry *ha_Array; /* Array of entries */
  ULONG       ha_Size;        /* Size of array */
  ULONG       ha_Used;        /* Number of entries used */
  ULONG       ha_InitialSize; /* Initial size */
};

/* static hashing hash entry */
struct HashEntry
{
  ULONG       he_Key;         /* Hash Key */
  ULONG       he_Data;        /* Auxilliary data */
};
