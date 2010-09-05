/*
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

#ifndef PT_PROTOTYPES_H
#define PT_PROTOTYPES_H

#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif


/* PTP_new_design.cxx */
extern "C" int pt_init_bond_matrix P_((PT_pdc *THIS));
extern "C" char *get_design_info P_((PT_tprobes *tprobe));
extern "C" char *get_design_hinfo P_((PT_tprobes *tprobe));
ULONG MarkSpeciesGroup P_((struct PTPanGlobal *pg, STRPTR specnames));
struct DesignQuery *AllocDesignQuery P_((struct PTPanGlobal *pg));
void FreeDesignQuery P_((struct DesignQuery *dq));
struct DesignHit *AddDesignHit P_((struct DesignQuery *dq));
void RemDesignHit P_((struct DesignHit *dh));
void CalcProbeQuality P_((struct DesignQuery *dq));
BOOL FindProbeInPartition P_((struct DesignQuery *dq));
extern "C" int PT_start_design P_((PT_pdc *pdc, int dummy_1x));

/* PTP_main.cxx */
struct PTPanGlobal *AllocPTPanGlobal P_((void));
void FreePTPanGlobal P_((struct PTPanGlobal *pg));
extern "C" int server_shutdown P_((PT_main *, aisc_string passwd));
extern "C" int broadcast P_((PT_main *main, int dummy_1x));

/* PTP_io.cxx */
ULONG BenchTimePassed P_((struct PTPanGlobal *pg));
void BenchOutput P_((struct PTPanGlobal *pg));
ULONG CalcLengthForFilteredSequence P_((struct PTPanGlobal *pg, STRPTR srcseq));
ULONG FilterSequenceTo P_((struct PTPanGlobal *pg, STRPTR srcstr, STRPTR filtptr));
STRPTR FilterSequence P_((struct PTPanGlobal *pg, STRPTR srcseq));
ULONG CompressSequenceTo P_((struct PTPanGlobal *pg, STRPTR srcseq, ULONG *seqptr));
ULONG *CompressSequence P_((struct PTPanGlobal *pg, STRPTR srcseq));
ULONG GetLengthOfCompressedSequence P_((struct PTPanGlobal *pg, ULONG *seqptr));
UWORD GetCompressedLongSize P_((struct PTPanGlobal *pg, ULONG pval));
ULONG DecompressSequenceTo P_((struct PTPanGlobal *pg, ULONG *seqptr, STRPTR tarseq));
ULONG DecompressCompressedLongTo P_((struct PTPanGlobal *pg, ULONG pval, STRPTR tarseq));
STRPTR DecompressSequence P_((struct PTPanGlobal *pg, ULONG *seqptr));
LONG DecompressSequencePartTo P_((struct PTPanGlobal *pg, ULONG *seqptr, ULONG seqpos, ULONG length, STRPTR tarseq));
UBYTE GetNextCharacter P_((struct PTPanGlobal *pg, UBYTE *buffer, ULONG &bitpos, ULONG &count));
ULONG WriteManyChars P_((UBYTE *buffer, ULONG bitpos, BYTE c, ULONG i));
ULONG CompressSequenceWithDotsAndHyphens P_((struct PTPanGlobal *pg, struct PTPanSpecies *ps));
void ComplementSequence P_((struct PTPanGlobal *pg, STRPTR seqstr));
void ReverseSequence P_((struct PTPanGlobal *, STRPTR seqstr));
BOOL OpenDataBase P_((struct PTPanGlobal *pg));
BOOL LoadEcoliSequence P_((struct PTPanGlobal *pg));
void FreeAllSpecies P_((struct PTPanGlobal *pg));
BOOL CacheSpeciesLoad P_((struct CacheHandler *, struct PTPanSpecies *ps));
BOOL CacheSpeciesUnload P_((struct CacheHandler *, struct PTPanSpecies *ps));
ULONG CacheSpeciesSize P_((struct CacheHandler *, struct PTPanSpecies *ps));
BOOL LoadSpecies P_((struct PTPanGlobal *pg));
BOOL LoadIndexHeader P_((struct PTPanGlobal *pg));
BOOL LoadAllPartitions P_((struct PTPanGlobal *pg));
void FreeAllPartitions P_((struct PTPanGlobal *pg));

/* PTP_etc.cxx */
void SetARBErrorMsg P_((PT_local *locs, const STRPTR error));
extern "C" STRPTR virt_name P_((PT_probematch *ml));
extern "C" STRPTR virt_fullname P_((PT_probematch *ml));
extern "C" bytestring *PT_unknown_names P_((struct_PT_pdc *pdc));

/* PTP_family.cxx */
extern "C" int ff_find_family P_((PT_local *locs, bytestring *species));
extern "C" int find_family P_((PT_local *locs, bytestring *species));

/* PT_lowlevel.cxx */
ULONG WriteBits P_((UBYTE *adr, ULONG bitpos, ULONG code, UWORD len));
ULONG ReadBits P_((UBYTE *adr, ULONG bitpos, UWORD len));

/* PT_huffman.cxx */
void BuildHuffmanCodeRec P_((struct HuffCode *hcbase, struct HuffCodeInternal *hc, ULONG len, ULONG rootidx, ULONG codelen, ULONG code));
BOOL BuildHuffmanCode P_((struct HuffCode *hcbase, ULONG len, LONG threshold));
void WriteHuffmanTree P_((struct HuffCode *hc, ULONG size, FILE *fh));
struct HuffTree *ReadHuffmanTree P_((FILE *fh));
struct HuffTree *BuildHuffmanTreeFromTable P_((struct HuffCode *hc, ULONG maxid));
void FreeHuffmanTree P_((struct HuffTree *root));
struct HuffTree *FindHuffTreeID P_((struct HuffTree *ht, UBYTE *adr, ULONG bitpos));

/* PT_treepack.cxx */
BOOL WriteIndexHeader P_((struct PTPanGlobal *pg));
BOOL WriteTreeHeader P_((struct PTPanPartition *pp));
BOOL CachePartitionLoad P_((struct CacheHandler *, struct PTPanPartition *pp));
void CachePartitionUnload P_((struct CacheHandler *, struct PTPanPartition *pp));
ULONG CachePartitionSize P_((struct CacheHandler *, struct PTPanPartition *pp));
BOOL WriteStdSuffixTreeHeader P_((struct PTPanPartition *pp));
BOOL CacheStdSuffixPartitionLoad P_((struct CacheHandler *, struct PTPanPartition *pp));
void CacheStdSuffixPartitionUnload P_((struct CacheHandler *, struct PTPanPartition *pp));
ULONG FixRelativePointersRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen));
LONG ULONGCompare P_((const ULONG *node1, const ULONG *node2));
ULONG CalcPackedNodeSize P_((struct PTPanPartition *pp, ULONG pos));
ULONG CalcPackedLeafSize P_((struct PTPanPartition *pp, ULONG pos));
void DebugTreeNode P_((struct TreeNode *tn));
void GetTreePath P_((struct TreeNode *tn, STRPTR strptr, ULONG len));
struct TreeNode *GoDownStdSuffixNodeChild P_((struct TreeNode *oldtn, UWORD seqcode));
struct TreeNode *GoDownNodeChild P_((struct TreeNode *oldtn, UWORD seqcode));
struct TreeNode *GoDownNodeChildNoEdge P_((struct TreeNode *oldtn, UWORD seqcode));
struct TreeNode *ReadPackedNode P_((struct PTPanPartition *pp, ULONG pos));
struct TreeNode *ReadPackedNodeNoEdge P_((struct PTPanPartition *pp, ULONG pos));
struct TreeNode *ReadPackedLeaf P_((struct PTPanPartition *pp, ULONG pos));
ULONG WritePackedNode P_((struct PTPanPartition *pp, ULONG pos, UBYTE *buf));
ULONG WritePackedLeaf P_((struct PTPanPartition *pp, ULONG pos, UBYTE *buf));
struct SearchQuery *CloneSearchQuery P_((struct SearchQuery *oldsq));
struct SearchQuery *AllocSearchQuery P_((struct PTPanGlobal *pg));
void FreeSearchQuery P_((struct SearchQuery *sq));
void SearchTree P_((struct SearchQuery *sq));
void PostFilterQueryHits P_((struct SearchQuery *sq));
BOOL AddQueryHit P_((struct SearchQuery *sq, ULONG hitpos));
void RemQueryHit P_((struct QueryHit *qh));
void MergeQueryHits P_((struct SearchQuery *tarsq, struct SearchQuery *srcsq));
void PrintSearchQueryState P_((const char *s1, const char *s2, struct SearchQuery *sq));
void SearchTreeRec P_((struct SearchQuery *sq));
void CollectTreeRec P_((struct SearchQuery *sq));
BOOL MatchSequence P_((struct SearchQuery *sq));
BOOL MatchSequenceRec P_((struct SearchQuery *sq));
BOOL FindSequenceMatch P_((struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarstr));
BOOL FindSequenceMatchRec P_((struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarptr));

/* PTP_buildtree.cxx */
BOOL BuildStdSuffixTree P_((struct PTPanGlobal *pg));
BOOL BuildMemoryStdSuffixTree P_((struct PTPanPartition *pp));
ULONG SplitStdSfxNode P_((struct PTPanPartition *pp, ULONG leafnum));
ULONG FindStdSfxNode P_((struct PTPanPartition *pp, ULONG snum, ULONG &sfxstart, ULONG sfxend));
ULONG FastFindStdSfxNode P_((struct PTPanPartition *pp, ULONG snum, ULONG sfxstart, ULONG sfxend));
ULONG InsertStdSfxNode P_((struct PTPanPartition *pp, ULONG sfxstart, ULONG sfxend, ULONG parnum));
BOOL WriteStdSuffixTreeToDisk P_((struct PTPanPartition *pp));
BOOL BuildPTPanIndex P_((struct PTPanGlobal *pg));
BOOL BuildMergedDatabase P_((struct PTPanGlobal *pg));
BOOL PartitionPrefixScan P_((struct PTPanGlobal *pg));
BOOL CreateTreeForPartition P_((struct PTPanPartition *pp));
BOOL BuildMemoryTree P_((struct PTPanPartition *pp));
ULONG CommonSequenceLength P_((struct PTPanPartition *pp, ULONG spos1, ULONG spos2, ULONG maxlen));
LONG CompareCompressedSequence P_((struct PTPanGlobal *pg, ULONG spos1, ULONG spos2));
BOOL InsertTreePos P_((struct PTPanPartition *pp, ULONG pos, ULONG window));
BOOL CalculateTreeStats P_((struct PTPanPartition *pp));
void GetTreeStatsDebugRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level));
void GetTreeStatsTreeDepthRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level));
void GetTreeStatsLevelRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level));
void GetTreeStatsShortEdgesRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen));
void GetTreeStatsLongEdgesRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen));
void GetTreeStatsBranchHistoRec P_((struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen));
void GetTreeStatsVerifyRec P_((struct PTPanPartition *pp, ULONG pos, ULONG treepos, ULONG hash));
ULONG GetTreeStatsLeafCountRec P_((struct PTPanPartition *pp, ULONG pos));
void GetTreeStatsLeafCollectRec P_((struct PTPanPartition *pp, ULONG pos));
LONG LongEdgeLengthCompare P_((const struct SfxNode **node1, const struct SfxNode **node2));
LONG LongEdgePosCompare P_((const struct SfxNode **node1, const struct SfxNode **node2));
LONG LongEdgeLabelCompare P_((struct SfxNode **node1, struct SfxNode **node2));
ULONG GetSeqHash P_((struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash));
ULONG GetSeqHashBackwards P_((struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash));
BOOL CheckLongEdgeMatch P_((struct PTPanPartition *pp, ULONG seqpos, ULONG edgelen, ULONG dictpos));
BOOL BuildLongEdgeDictionary P_((struct PTPanPartition *pp));
BOOL WriteTreeToDisk P_((struct PTPanPartition *pp));
BOOL CreatePartitionLookup P_((struct PTPanGlobal *pg));

/* PTP_match.cxx */
void SearchPartition P_((struct PTPanPartition *pp, struct SearchQuery *sq));
void QueryTests P_((struct PTPanGlobal *pg));
void PP_convertBondMatrix P_((PT_pdc *pdc, PTPanGlobal *pg));
void PP_buildPosWeight P_((SearchQuery *sq));
extern "C" int probe_match P_((PT_local *locs, aisc_string probestring));
void SortHitsList P_((struct SearchQuery *sq));
void CreateHitsGUIList P_((struct SearchQuery *sq));
extern "C" STRPTR get_match_info P_((PT_probematch *ml));
STRPTR GetMatchListHeader P_((STRPTR seq));
extern "C" STRPTR get_match_hinfo P_((PT_probematch *));
extern "C" STRPTR c_get_match_hinfo P_((PT_probematch *));
extern "C" bytestring *match_string P_((PT_local *locs));
extern "C" bytestring *MP_match_string P_((PT_local *locs));
extern "C" bytestring *MP_all_species_string P_((PT_local *));
extern "C" int MP_count_all_species P_((PT_local *));

/* PTP_findEx.cxx */
extern "C" int PT_find_exProb P_((PT_exProb *pep));

/* PT_cachehandler.cxx */
struct CacheHandler *AllocCacheHandler P_((void));
void FreeCacheHandler P_((struct CacheHandler *ch));
struct CacheNode *CacheLoadData P_((struct CacheHandler *ch, struct CacheNode *cn, APTR ud));
ULONG CacheMemUsage P_((struct CacheHandler *ch));
void DisableCache P_((struct CacheHandler *ch));
void EnableCache P_((struct CacheHandler *ch));
BOOL CacheDataLoaded P_((struct CacheNode *cn));
void FreeCacheNode P_((struct CacheHandler *ch, struct CacheNode *cn));
BOOL CacheUnloadData P_((struct CacheHandler *ch, struct CacheNode *cn));
ULONG FlushCache P_((struct CacheHandler *ch));

/* PT_hashing.cxx */
struct HashArray *AllocHashArray P_((ULONG size));
void FreeHashArray P_((struct HashArray *ha));
void ClearHashArray P_((struct HashArray *ha));
struct HashEntry *GetHashEntry P_((struct HashArray *ha, ULONG hashkey));
BOOL EnlargeHashArray P_((struct HashArray *ha));
BOOL InsertHashEntry P_((struct HashArray *ha, ULONG hashkey, ULONG data));

/* dlist.cxx */
void NewList P_((struct List *lh));
void AddHead P_((struct List *lh, struct Node *nd));
void AddTail P_((struct List *lh, struct Node *nd));
void Remove P_((struct Node *nd));
LONG NodePriCompare P_((const struct Node **node1, const struct Node **node2));
BOOL SortList P_((struct List *lh));
struct BinTree *BuildBinTreeRec P_((struct Node **nodearr, ULONG left, ULONG right));
struct BinTree *BuildBinTree P_((struct List *list));
void FreeBinTree P_((struct BinTree *root));
struct Node *FindBinTreeLowerKey P_((struct BinTree *root, LLONG key));

#undef P_

#else
#error pt_prototypes.h included twice
#endif /* PT_PROTOTYPES_H */
