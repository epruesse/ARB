/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef PT_PROTOTYPES_H
#define PT_PROTOTYPES_H


/* PTP_new_design.cxx */
extern "C" int pt_init_bond_matrix(PT_pdc *THIS);
extern "C" char *get_design_info(PT_tprobes *tprobe);
extern "C" char *get_design_hinfo(PT_tprobes *tprobe);
ULONG MarkSpeciesGroup(struct PTPanGlobal *pg, STRPTR specnames);
struct DesignQuery *AllocDesignQuery(struct PTPanGlobal *pg);
void FreeDesignQuery(struct DesignQuery *dq);
struct DesignHit *AddDesignHit(struct DesignQuery *dq);
void RemDesignHit(struct DesignHit *dh);
void CalcProbeQuality(struct DesignQuery *dq);
BOOL FindProbeInPartition(struct DesignQuery *dq);
extern "C" int PT_start_design(PT_pdc *pdc, int dummy_1x);

/* PTP_main.cxx */
struct PTPanGlobal *AllocPTPanGlobal(void);
void FreePTPanGlobal(struct PTPanGlobal *pg);
int server_shutdown(PT_main *, aisc_string passwd);
extern "C" int broadcast(PT_main *main, int dummy_1x);

/* PTP_io.cxx */
ULONG BenchTimePassed(struct PTPanGlobal *pg);
void BenchOutput(struct PTPanGlobal *pg);
ULONG CalcLengthForFilteredSequence(struct PTPanGlobal *pg, STRPTR srcseq);
ULONG FilterSequenceTo(struct PTPanGlobal *pg, STRPTR srcstr, STRPTR filtptr);
STRPTR FilterSequence(struct PTPanGlobal *pg, STRPTR srcseq);
ULONG CompressSequenceTo(struct PTPanGlobal *pg, STRPTR srcseq, ULONG *seqptr);
ULONG *CompressSequence(struct PTPanGlobal *pg, STRPTR srcseq);
ULONG GetLengthOfCompressedSequence(struct PTPanGlobal *pg, ULONG *seqptr);
UWORD GetCompressedLongSize(struct PTPanGlobal *pg, ULONG pval);
ULONG DecompressSequenceTo(struct PTPanGlobal *pg, ULONG *seqptr, STRPTR tarseq);
ULONG DecompressCompressedLongTo(struct PTPanGlobal *pg, ULONG pval, STRPTR tarseq);
STRPTR DecompressSequence(struct PTPanGlobal *pg, ULONG *seqptr);
LONG DecompressSequencePartTo(struct PTPanGlobal *pg, ULONG *seqptr, ULONG seqpos, ULONG length, STRPTR tarseq);
UBYTE GetNextCharacter(struct PTPanGlobal *pg, UBYTE *buffer, ULONG &bitpos, ULONG &count);
ULONG WriteManyChars(UBYTE *buffer, ULONG bitpos, BYTE c, ULONG i);
ULONG CompressSequenceWithDotsAndHyphens(struct PTPanGlobal *pg, struct PTPanSpecies *ps);
void ComplementSequence(struct PTPanGlobal *pg, STRPTR seqstr);
void ReverseSequence(struct PTPanGlobal *, STRPTR seqstr);
BOOL OpenDataBase(struct PTPanGlobal *pg);
BOOL LoadEcoliSequence(struct PTPanGlobal *pg);
void FreeAllSpecies(struct PTPanGlobal *pg);
BOOL CacheSpeciesLoad(struct CacheHandler *, struct PTPanSpecies *ps);
BOOL CacheSpeciesUnload(struct CacheHandler *, struct PTPanSpecies *ps);
ULONG CacheSpeciesSize(struct CacheHandler *, struct PTPanSpecies *ps);
BOOL LoadSpecies(struct PTPanGlobal *pg);
BOOL LoadIndexHeader(struct PTPanGlobal *pg);
BOOL LoadAllPartitions(struct PTPanGlobal *pg);
void FreeAllPartitions(struct PTPanGlobal *pg);

/* PTP_etc.cxx */
void SetARBErrorMsg(PT_local *locs, const STRPTR error);
extern "C" STRPTR virt_name(PT_probematch *ml);
extern "C" STRPTR virt_fullname(PT_probematch *ml);
extern "C" bytestring *PT_unknown_names(PT_pdc *pdc);

/* PTP_family.cxx */
extern "C" int ff_find_family(PT_local *locs, bytestring *species);
extern "C" int find_family(PT_local *locs, bytestring *species);

/* PT_lowlevel.cxx */
ULONG WriteBits(UBYTE *adr, ULONG bitpos, ULONG code, UWORD len);
ULONG ReadBits(UBYTE *adr, ULONG bitpos, UWORD len);

/* PT_huffman.cxx */
void BuildHuffmanCodeRec(struct HuffCode *hcbase, struct HuffCodeInternal *hc, ULONG len, ULONG rootidx, ULONG codelen, ULONG code);
BOOL BuildHuffmanCode(struct HuffCode *hcbase, ULONG len, LONG threshold);
void WriteHuffmanTree(struct HuffCode *hc, ULONG size, FILE *fh);
struct HuffTree *ReadHuffmanTree(FILE *fh);
struct HuffTree *BuildHuffmanTreeFromTable(struct HuffCode *hc, ULONG maxid);
void FreeHuffmanTree(struct HuffTree *root);
struct HuffTree *FindHuffTreeID(struct HuffTree *ht, UBYTE *adr, ULONG bitpos);

/* PT_treepack.cxx */
BOOL WriteIndexHeader(struct PTPanGlobal *pg);
BOOL WriteTreeHeader(struct PTPanPartition *pp);
BOOL CachePartitionLoad(struct CacheHandler *, struct PTPanPartition *pp);
void CachePartitionUnload(struct CacheHandler *, struct PTPanPartition *pp);
ULONG CachePartitionSize(struct CacheHandler *, struct PTPanPartition *pp);
BOOL WriteStdSuffixTreeHeader(struct PTPanPartition *pp);
BOOL CacheStdSuffixPartitionLoad(struct CacheHandler *, struct PTPanPartition *pp);
void CacheStdSuffixPartitionUnload(struct CacheHandler *, struct PTPanPartition *pp);
ULONG FixRelativePointersRec(struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen);
LONG ULONGCompare(const ULONG *node1, const ULONG *node2);
ULONG CalcPackedNodeSize(struct PTPanPartition *pp, ULONG pos);
ULONG CalcPackedLeafSize(struct PTPanPartition *pp, ULONG pos);
void DebugTreeNode(struct TreeNode *tn);
void GetTreePath(struct TreeNode *tn, STRPTR strptr, ULONG len);
struct TreeNode *GoDownStdSuffixNodeChild(struct TreeNode *oldtn, UWORD seqcode);
struct TreeNode *GoDownNodeChild(struct TreeNode *oldtn, UWORD seqcode);
struct TreeNode *GoDownNodeChildNoEdge(struct TreeNode *oldtn, UWORD seqcode);
struct TreeNode *ReadPackedNode(struct PTPanPartition *pp, ULONG pos);
struct TreeNode *ReadPackedNodeNoEdge(struct PTPanPartition *pp, ULONG pos);
struct TreeNode *ReadPackedLeaf(struct PTPanPartition *pp, ULONG pos);
ULONG WritePackedNode(struct PTPanPartition *pp, ULONG pos, UBYTE *buf);
ULONG WritePackedLeaf(struct PTPanPartition *pp, ULONG pos, UBYTE *buf);
struct SearchQuery *CloneSearchQuery(struct SearchQuery *oldsq);
struct SearchQuery *AllocSearchQuery(struct PTPanGlobal *pg);
void FreeSearchQuery(struct SearchQuery *sq);
void SearchTree(struct SearchQuery *sq);
void PostFilterQueryHits(struct SearchQuery *sq);
BOOL AddQueryHit(struct SearchQuery *sq, ULONG hitpos);
void RemQueryHit(struct QueryHit *qh);
void MergeQueryHits(struct SearchQuery *tarsq, struct SearchQuery *srcsq);
void PrintSearchQueryState(const char *s1, const char *s2, struct SearchQuery *sq);
void SearchTreeRec(struct SearchQuery *sq);
void CollectTreeRec(struct SearchQuery *sq);
BOOL MatchSequence(struct SearchQuery *sq);
BOOL MatchSequenceRec(struct SearchQuery *sq);
BOOL FindSequenceMatch(struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarstr);
BOOL FindSequenceMatchRec(struct SearchQuery *sq, struct QueryHit *qh, STRPTR tarptr);

/* PTP_buildtree.cxx */
BOOL BuildStdSuffixTree(struct PTPanGlobal *pg);
BOOL BuildMemoryStdSuffixTree(struct PTPanPartition *pp);
ULONG SplitStdSfxNode(struct PTPanPartition *pp, ULONG leafnum);
ULONG FindStdSfxNode(struct PTPanPartition *pp, ULONG snum, ULONG &sfxstart, ULONG sfxend);
ULONG FastFindStdSfxNode(struct PTPanPartition *pp, ULONG snum, ULONG sfxstart, ULONG sfxend);
ULONG InsertStdSfxNode(struct PTPanPartition *pp, ULONG sfxstart, ULONG sfxend, ULONG parnum);
BOOL WriteStdSuffixTreeToDisk(struct PTPanPartition *pp);
BOOL BuildPTPanIndex(struct PTPanGlobal *pg);
BOOL BuildMergedDatabase(struct PTPanGlobal *pg);
BOOL PartitionPrefixScan(struct PTPanGlobal *pg);
BOOL CreateTreeForPartition(struct PTPanPartition *pp);
BOOL BuildMemoryTree(struct PTPanPartition *pp);
ULONG CommonSequenceLength(struct PTPanPartition *pp, ULONG spos1, ULONG spos2, ULONG maxlen);
LONG CompareCompressedSequence(struct PTPanGlobal *pg, ULONG spos1, ULONG spos2);
BOOL InsertTreePos(struct PTPanPartition *pp, ULONG pos, ULONG window);
BOOL CalculateTreeStats(struct PTPanPartition *pp);
void GetTreeStatsDebugRec(struct PTPanPartition *pp, ULONG pos, ULONG level);
void GetTreeStatsTreeDepthRec(struct PTPanPartition *pp, ULONG pos, ULONG level);
void GetTreeStatsLevelRec(struct PTPanPartition *pp, ULONG pos, ULONG level);
void GetTreeStatsShortEdgesRec(struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen);
void GetTreeStatsLongEdgesRec(struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen);
void GetTreeStatsBranchHistoRec(struct PTPanPartition *pp, ULONG pos, ULONG level, ULONG elen);
void GetTreeStatsVerifyRec(struct PTPanPartition *pp, ULONG pos, ULONG treepos, ULONG hash);
ULONG GetTreeStatsLeafCountRec(struct PTPanPartition *pp, ULONG pos);
void GetTreeStatsLeafCollectRec(struct PTPanPartition *pp, ULONG pos);
LONG LongEdgeLengthCompare(const struct SfxNode **node1, const struct SfxNode **node2);
LONG LongEdgePosCompare(const struct SfxNode **node1, const struct SfxNode **node2);
LONG LongEdgeLabelCompare(struct SfxNode **node1, struct SfxNode **node2);
ULONG GetSeqHash(struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash);
ULONG GetSeqHashBackwards(struct PTPanGlobal *pg, ULONG seqpos, ULONG len, ULONG hash);
BOOL CheckLongEdgeMatch(struct PTPanPartition *pp, ULONG seqpos, ULONG edgelen, ULONG dictpos);
BOOL BuildLongEdgeDictionary(struct PTPanPartition *pp);
BOOL WriteTreeToDisk(struct PTPanPartition *pp);
BOOL CreatePartitionLookup(struct PTPanGlobal *pg);

/* PTP_match.cxx */
void SearchPartition(struct PTPanPartition *pp, struct SearchQuery *sq);
void QueryTests(struct PTPanGlobal *pg);
void PP_convertBondMatrix(PT_pdc *pdc, PTPanGlobal *pg);
void PP_buildPosWeight(SearchQuery *sq);
extern "C" char *get_match_overlay(PT_probematch *ml);
extern "C" int probe_match(PT_local *locs, aisc_string probestring);
void SortHitsList(struct SearchQuery *sq);
void CreateHitsGUIList(struct SearchQuery *sq);
extern "C" STRPTR get_match_info(PT_probematch *ml);
STRPTR GetMatchListHeader(STRPTR seq);
extern "C" STRPTR get_match_hinfo(PT_probematch *);
extern "C" STRPTR c_get_match_hinfo(PT_probematch *);
extern "C" bytestring *match_string(PT_local *locs);
extern "C" bytestring *MP_match_string(PT_local *locs);
extern "C" bytestring *MP_all_species_string(PT_local *);
extern "C" int MP_count_all_species(PT_local *);

/* PTP_findEx.cxx */
int PT_find_exProb(PT_exProb *pep, int dummy_1x);

/* PT_cachehandler.cxx */
struct CacheHandler *AllocCacheHandler(void);
void FreeCacheHandler(struct CacheHandler *ch);
struct CacheNode *CacheLoadData(struct CacheHandler *ch, struct CacheNode *cn, APTR ud);
ULONG CacheMemUsage(struct CacheHandler *ch);
void DisableCache(struct CacheHandler *ch);
void EnableCache(struct CacheHandler *ch);
BOOL CacheDataLoaded(struct CacheNode *cn);
void FreeCacheNode(struct CacheHandler *ch, struct CacheNode *cn);
BOOL CacheUnloadData(struct CacheHandler *ch, struct CacheNode *cn);
ULONG FlushCache(struct CacheHandler *ch);

/* PT_hashing.cxx */
struct HashArray *AllocHashArray(ULONG size);
void FreeHashArray(struct HashArray *ha);
void ClearHashArray(struct HashArray *ha);
struct HashEntry *GetHashEntry(struct HashArray *ha, ULONG hashkey);
BOOL EnlargeHashArray(struct HashArray *ha);
BOOL InsertHashEntry(struct HashArray *ha, ULONG hashkey, ULONG data);

/* dlist.cxx */
void NewList(struct List *lh);
void AddHead(struct List *lh, struct Node *nd);
void AddTail(struct List *lh, struct Node *nd);
void Remove(struct Node *nd);
LONG NodePriCompare(const struct Node **node1, const struct Node **node2);
BOOL SortList(struct List *lh);
struct BinTree *BuildBinTreeRec(struct Node **nodearr, ULONG left, ULONG right);
struct BinTree *BuildBinTree(struct List *list);
void FreeBinTree(struct BinTree *root);
struct Node *FindBinTreeLowerKey(struct BinTree *root, LLONG key);

#else
#error pt_prototypes.h included twice
#endif /* PT_PROTOTYPES_H */
