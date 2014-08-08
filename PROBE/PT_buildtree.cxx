// =============================================================== //
//                                                                 //
//   File      : PT_buildtree.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <PT_server_prototypes.h>
#include "probe_tree.h"
#include "pt_prototypes.h"
#include "PT_partition.h"

#include <arb_defs.h>
#include <arb_file.h>
#include <arb_misc.h>
#include <arb_diff.h>

#include <arb_progress.h>

#include <unistd.h>
#include <ctype.h>

// #define PTM_TRACE_MAX_MEM_USAGE
// #define PTM_TRACE_PARTITION_DETECTION

// AISC_MKPT_PROMOTE: class DataLoc;

static POS_TREE1 *build_pos_tree(POS_TREE1 *const root, const ReadableDataLoc& loc) {
    POS_TREE1 *at     = root;
    int        height = 0;

    while (at->is_node()) {    // now we got an inner node
        POS_TREE1 *pt_next = PT_read_son(at, loc[height]);
        if (!pt_next) { // there is no son of that type -> simply add the new son to that path
            POS_TREE1 *new_root = root;
            POS_TREE1 *leaf;
            {
                bool atRoot = (at == root);

                leaf = PT_create_leaf(&at, loc[height], loc);
                ++height;

                if (atRoot) new_root = at;
            }

            while (height<PT_MIN_TREE_HEIGHT && loc[height-1] != PT_QU) {
                at   = PT_change_leaf_to_node(leaf);
                leaf = PT_create_leaf(&at, loc[height], loc);
                ++height;
            }

            pt_assert(height >= PT_MIN_TREE_HEIGHT || loc[height-1] == PT_QU);
            return new_root;
        }
        else {            // go down the tree
            at = pt_next;
            height++;

            if (loc[height-1] == PT_QU) {
                // end of sequence reached -> change node to chain and add
                pt_assert(at->is_chain());
                PT_add_to_chain(at, loc);
                return root;
            }
        }
    }

    // type == leaf or chain
    if (at->is_chain()) {      // old chain reached
        PT_add_to_chain(at, loc);
        return root;
    }

    // change leaf to node and create two sons

    const ReadableDataLoc loc_ref(at);

    while (loc[height] == loc_ref[height]) {  // creates nodes until sequences are different
        pt_assert(!at->is_node());

        if (at->is_chain()) {
            PT_add_to_chain(at, loc);
            return root;
        }
        if (height >= PT_POS_TREE_HEIGHT) {
            if (at->is_leaf()) at = PT_leaf_to_chain(at);
            pt_assert(at->is_chain());
            PT_add_to_chain(at, loc);
            return root;
        }

        pt_assert(at->is_leaf());

        at = PT_change_leaf_to_node(at);                // change tip to node and append two new leafs
        at = PT_create_leaf(&at, loc[height], loc_ref); // dummy leaf just to create a new node; may become a chain

        height++;

        if (loc[height-1] == PT_QU) {
            pt_assert(loc_ref[height-1] == PT_QU); // end of both sequences
            pt_assert(at->is_chain());

            PT_add_to_chain(at, loc);  // and add node
            return root;
        }
        pt_assert(loc_ref[height-1] != PT_QU);
    }

    pt_assert(loc[height] != loc_ref[height]);

    if (height >= PT_POS_TREE_HEIGHT) {
        if (at->is_leaf()) at = PT_leaf_to_chain(at);
        PT_add_to_chain(at, loc);
        return root;
    }
    if (at->is_chain()) {
        // not covered by test - but looks similar to case in top-loop
        PT_add_to_chain(at, loc);
    }
    else {
        at = PT_change_leaf_to_node(at);             // delete leaf
        PT_create_leaf(&at, loc[height], loc); // two new leafs
        PT_create_leaf(&at, loc_ref[height], loc_ref);
    }
    return root;
}


inline void get_abs_align_pos(char *seq, int &pos) {
    // get the absolute alignment position
    int q_exists = 0;
    if (pos > 3) {
        pos-=3;
        while (pos > 0) {
            uint32_t c = *((uint32_t*)(seq+pos));
            if (c == 0x2E2E2E2E) {
                q_exists = 1;
                pos-=4;
                continue;
            }
            if (c == 0x2D2D2D2D) {
                pos-=4;
                continue;
            }
            break;
        }
        pos+=3;
    }
    while (pos) {
        unsigned char c = seq[pos];
        if (c == '.') {
            q_exists = 1;
            pos--;
            continue;
        }
        if (c == '-') {
            pos--;
            continue;
        }
        break;
    }
    pos+=q_exists;
}

static bool all_sons_saved(POS_TREE1 *node);
inline bool has_unsaved_sons(POS_TREE1 *node) {
    POS_TREE1::TYPE type = node->get_type();
    return (type == PT1_NODE) ? !all_sons_saved(node) : (type != PT1_SAVED);
}
static bool all_sons_saved(POS_TREE1 *node) {
    pt_assert(node->is_node());

    for (int i = PT_QU; i < PT_BASES; i++) {
        POS_TREE1 *son = PT_read_son(node, (PT_base)i);
        if (son) {
            if (has_unsaved_sons(son)) return false;
        }
    }
    return true;
}

static long write_subtree(FILE *out, POS_TREE1 *node, long pos, long *node_pos, ARB_ERROR& error) {
    pt_assert_stage(STAGE1);
    node->clear_fathers();
    return PTD_write_leafs_to_disk(out, node, pos, node_pos, error);
}

static long save_lower_subtree(FILE *out, POS_TREE1 *node, long pos, int height, ARB_ERROR& error) {
    if (height >= PT_MIN_TREE_HEIGHT) { // in lower part of tree
        long dummy;
        pos = write_subtree(out, node, pos, &dummy, error);
    }
    else {
        switch (node->get_type()) {
            case PT1_NODE:
                for (int i = PT_QU; i<PT_BASES; ++i) {
                    POS_TREE1 *son = PT_read_son(node, PT_base(i));
                    if (son) pos = save_lower_subtree(out, son, pos, height+1, error);
                }
                break;

            case PT1_CHAIN: {
                long dummy;
                pos = write_subtree(out, node, pos, &dummy, error);
                break;
            }
            case PT1_LEAF: pt_assert(0); break; // leafs shall not occur above PT_MIN_TREE_HEIGHT
            case PT1_SAVED: break; // ok - saved by previous call
            case PT1_UNDEF: pt_assert(0); break;
        }
    }
    return pos;
}

static long save_upper_tree(FILE *out, POS_TREE1 *node, long pos, long& node_pos, ARB_ERROR& error) {
    pos = write_subtree(out, node, pos, &node_pos, error);
    return pos;
}

inline void check_tree_was_saved(POS_TREE1 *node, const char *whatTree, bool completely, ARB_ERROR& error) {
    if (!error) {
        bool saved = completely ? node->is_saved() : !has_unsaved_sons(node);
        if (!saved) {
#if defined(DEBUG)
            fprintf(stderr, "%s was not completely saved:\n", whatTree);
            PT_dump_POS_TREE_recursive(node, " ", stderr);
#endif
            error = GBS_global_string("%s was not saved completely", whatTree);
        }
    }
}

long PTD_save_lower_tree(FILE *out, POS_TREE1 *node, long pos, ARB_ERROR& error) {
    pos = save_lower_subtree(out, node, pos, 0, error);
    check_tree_was_saved(node, "lower tree", false, error);
    return pos;
}

long PTD_save_upper_tree(FILE *out, POS_TREE1*& node, long pos, long& node_pos, ARB_ERROR& error) {
    pt_assert(!has_unsaved_sons(node)); // forgot to call PTD_save_lower_tree?
    pos = save_upper_tree(out, node, pos, node_pos, error);
    check_tree_was_saved(node, "tree", true, error);
    PTD_delete_saved_node(node);
    return pos;
}

#if defined(PTM_TRACE_MAX_MEM_USAGE)
static void dump_memusage() {
    fflush(stderr);
    printf("\n------------------------------ dump_memusage:\n"); fflush(stdout);

    malloc_stats();

    pid_t     pid   = getpid();
    char     *cmd   = GBS_global_string_copy("pmap -d %i | grep -v lib", pid);
    GB_ERROR  error = GBK_system(cmd);
    if (error) {
        printf("Warning: %s\n", error);
    }
    free(cmd);
    printf("------------------------------ dump_memusage [end]\n");
    fflush_all();
}
#endif

class PartitionSpec {
    int    passes;
    size_t memuse;
    int    depth;

    const char *passname() const {
        switch (depth) {
            case 0: return "pass";
            case 1: return "Level-I-passes";
            case 2: return "Level-II-passes";
            case 3: return "Level-III-passes";
            case 4: return "Level-IV-passes";
            default : pt_assert(0); break;
        }
        return NULL; // unreached
    }

public:
    PartitionSpec() : passes(0), memuse(0), depth(0) {}
    PartitionSpec(int passes_, size_t memuse_, int depth_) : passes(passes_), memuse(memuse_), depth(depth_) {}

    bool willUseMoreThan(size_t max_kb_usable) const { return memuse > max_kb_usable; }
    bool isBetterThan(const PartitionSpec& other, size_t max_kb_usable) const {
        if (!passes) return false;
        if (!other.passes) return true;

        bool swaps       = willUseMoreThan(max_kb_usable);
        bool other_swaps = other.willUseMoreThan(max_kb_usable);

        int cmp = int(other_swaps)-int(swaps); // not to swap is better
        if (cmp == 0) {
            if (swaps) { // optimize for memory
                cmp = other.memuse-memuse; // less memuse is better (@@@ true only if probe->pass-calculation is cheap)
                if (cmp == 0) {
                    cmp = other.passes-passes; // less passes are better
                    if (cmp == 0) {
                        cmp = other.depth-depth;
                    }
                }
            }
            else { // optimize for number of passes
                cmp = other.passes-passes; // less passes are better
                if (cmp == 0) {
                    cmp = other.depth-depth; // less depth is better
                    if (cmp == 0) {
                        cmp = other.memuse-memuse; // less memuse is better (@@@ true only if probe->pass-calculation is cheap)
                    }
                }
            }
        }
        return cmp>0;
    }

    void print_info(FILE *out, size_t max_kb_usable) const {
        fprintf(out,
                "Estimated memory usage for %i %s: %s%s\n",
                passes,
                passname(),
                GBS_readable_size(memuse*1024, "b"),
                memuse>max_kb_usable ? " (would swap)" : "");
    }

    Partition partition() const { return Partition(depth, passes); }
};

static Partition decide_passes_to_use(size_t overallBases, size_t max_kb_usable, int forced_passes) {
    // if 'forced_passes' == 0 -> decide number of passes such that estimated memuse is hard up for 'max_kb_usable'
    // if 'forced_passes' > 0 -> ignore available memory (for DEBUGGING memory estimation)

    fflush_all();

    PartitionSpec best;

    for (int depth = 0; depth <= PT_MAX_PARTITION_DEPTH; ++depth) {
        PrefixProbabilities prob(depth);

        int maxPasses = prob.get_prefix_count();
        if (forced_passes) {
            if (maxPasses >= forced_passes) {
                PartitionSpec curr(forced_passes, max_kb_for_passes(prob, forced_passes, overallBases), depth);
                if (curr.isBetterThan(best, max_kb_usable)) {
                    best = curr;
#if defined(PTM_TRACE_PARTITION_DETECTION)
                    best.print_info(stdout, max_kb_usable);
#endif
                }
            }
        }
        else {
            for (int passes = 1; passes <= maxPasses; ++passes) {
                PartitionSpec curr(passes, max_kb_for_passes(prob, passes, overallBases), depth);
                if (curr.isBetterThan(best, max_kb_usable)) {
                    best = curr;
#if defined(PTM_TRACE_PARTITION_DETECTION)
                    best.print_info(stdout, max_kb_usable);
#endif
                }
                if (!curr.willUseMoreThan(max_kb_usable)) break;
            }
        }
    }
    fflush(stdout);

    best.print_info(stdout, max_kb_usable);

    if (best.willUseMoreThan(max_kb_usable)) {
        const int allowed_passes = PrefixIterator(PT_QU, PT_T, PT_MAX_PARTITION_DEPTH).steps();

        fprintf(stderr,
                "Warning: \n"
                "  You try to build a ptserver from a very big database!\n"
                "\n"
                "  The memory installed on your machine would require to build the ptserver\n"
                "  in more than %i passes (the maximum allowed number of passes).\n"
                "\n"
                "  As a result the build of this server may cause your machine to swap huge\n"
                "  amounts of memory and will possibly run for days, weeks or even months.\n"
                "\n", allowed_passes);

        fflush(stderr);
    }

    return best.partition();
}

ARB_ERROR enter_stage_1_build_tree(PT_main * , const char *tname, ULONG ARM_size_kb) { // __ATTR__USERESULT
    // initialize tree and call the build pos tree procedure

    ARB_ERROR error;

    if (unlink(tname)) {
        if (GB_size_of_file(tname) >= 0) {
            error = GBS_global_string("Cannot remove %s", tname);
        }
    }

    if (!error) {
        char *t2name = (char *)calloc(sizeof(char), strlen(tname) + 2);
        sprintf(t2name, "%s%%", tname);
        
        FILE *out = fopen(t2name, "w");
        if (!out) {
            error = GBS_global_string("Cannot open %s", t2name);
        }
        else {
            POS_TREE1 *pt = NULL;
            
            {
                GB_ERROR sm_error = GB_set_mode_of_file(t2name, 0666);
                if (sm_error) {
                    GB_warningf("%s\nOther users might get problems when they try to access this file.", sm_error);
                }
            }

            fputc(0, out);       // disable zero father
            long pos = 1;

            // now temp file exists -> trigger ptserver-selectionlist-update in all
            // ARB applications by writing to log
            GBS_add_ptserver_logentry(GBS_global_string("Calculating probe tree (%s)", tname));

            psg.enter_stage(STAGE1);
            PT_init_cache_sizes(STAGE1);

            pt = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));  // create main node
            pt = PT_change_leaf_to_node(pt);
            psg.stat.cut_offs = 0;                  // statistic information
            GB_begin_transaction(psg.gb_main);

            ULONG available_memory = GB_get_usable_memory() - ARM_size_kb - PTSERVER_BIN_MB*1024;
            printf("Memory available for build: %s\n", GBS_readable_size(available_memory*1024, "b"));

            int forcedPasses = 0; // means "do not force"
            {
                const char *forced = GB_getenv("ARB_PTS_FORCE_PASSES");
                if (forced) {
                    int f = atoi(forced);
                    if (f >= 1) {
                        forcedPasses = f;
                        printf("Warning: Forcing %i passes (by envvar ARB_PTS_FORCE_PASSES='%s')\n", forcedPasses, forced);
                    }
                }
            }

            Partition partition = decide_passes_to_use(psg.char_count, available_memory, forcedPasses);
            {
                size_t max_part = partition.estimate_max_probes_for_any_pass(psg.char_count);
                printf("Overall bases:       %s\n", GBS_readable_size(psg.char_count, "bp"));
                printf("Max. partition size: %s (=%.1f%%)\n", GBS_readable_size(max_part, "bp"), max_part*100.0/psg.char_count);
            }

            int          passes   = partition.number_of_passes();
            arb_progress pass_progress(GBS_global_string("Build index in %i passes", passes), passes);
            int          currPass = 0;
            do {
                pt_assert(!partition.done());

                ++currPass;
                arb_progress data_progress(GBS_global_string("pass %i/%i", currPass, passes), psg.data_count);

                for (int name = 0; name < psg.data_count; name++) {
                    const probe_input_data& pid = psg.data[name];

                    SmartCharPtr  seqPtr = pid.get_dataPtr();
                    const char   *seq    = &*seqPtr;

                    pid.preload_rel2abs();
                    ReadableDataLoc insertLoc(name, 0, 0);
                    for (int rel = pid.get_size() - 1; rel >= 0; rel--) {
                        if (partition.contains(seq+rel)) {
                            insertLoc.set_position(pid.get_abspos(rel), rel);
                            pt = build_pos_tree(pt, insertLoc);
                        }
                    }
                    ++data_progress;
                }

#if defined(PTM_TRACE_MAX_MEM_USAGE)
                dump_memusage();
#endif

                pos = PTD_save_lower_tree(out, pt, pos, error);
                if (error) break;

#ifdef PTM_DEBUG_NODES
                PTD_debug_nodes();
#endif
            }
            while (partition.next());

            long last_obj = 0;
            if (!error) {
                pos = PTD_save_upper_tree(out, pt, pos, last_obj, error);
                pt_assert(!pt);
            }
            if (!error) {
                bool need64bit = false; // does created db need a 64bit ptserver ?

                pt_assert(last_obj);
#ifdef ARB_64
                if (last_obj >= 0xffffffff) need64bit = true;   // last_obj is bigger than int
#else
                if (last_obj <= 0) {                            // overflow ?
                    GBK_terminate("Overflow - out of memory");
                }
#endif

                // write information about database
                long info_pos = pos;

                PTD_put_int(out, PT_SERVER_MAGIC);              // marker to identify PT-Server file
                PTD_put_byte(out, PT_SERVER_VERSION);           // version of PT-Server file
                pos += 4+1;

                // as last element of info block, write it's size (2byte)
                long info_size = pos-info_pos;
                PTD_put_short(out, info_size);
                pos += 2;

                // save DB footer (which is the entry point on load)

                if (need64bit) {                                // last_obj is bigger than int
#ifdef ARB_64
                    PTD_put_longlong(out, last_obj);            // write last_obj as long long (64 bit)
                    PTD_put_int(out, 0xffffffff);               // write 0xffffffff at the end to signalize 64bit ptserver is needed
#else
                    pt_assert(0);
#endif
                }
                else {
                    PTD_put_int(out, last_obj);                 // last_obj fits into an int -> store it as usual (compatible to old unversioned format)
                }
            }
            if (error) {
                GB_abort_transaction(psg.gb_main);
                fclose(out);

                int res = GB_unlink(t2name);
                if (res == -1) fputs(GB_await_error(), stderr);
            }
            else {
                GB_commit_transaction(psg.gb_main);
                fclose(out);

                error = GB_rename_file(t2name, tname);
                if (!error) {
                    GB_ERROR sm_error = GB_set_mode_of_file(tname, 00666);
                    if (sm_error) GB_warning(sm_error);
                }
            }

            if (error) pass_progress.done();
        }
        free(t2name);
    }

#if defined(DEBUG)
    {
        char *related = strdup(tname);
        char *starpos = strstr(related, ".arb.pt");

        pt_assert(starpos);
        strcpy(starpos, ".*");

        fflush_all();

        char     *listRelated = GBS_global_string_copy("ls -al %s", related);
        GB_ERROR  lserror       = GBK_system(listRelated);

        fflush_all();

        if (lserror) fprintf(stderr, "Warning: %s\n", lserror);
        free(listRelated);
        free(related);
    }
#endif

    return error;
}

ARB_ERROR enter_stage_2_load_tree(PT_main *, const char *tname) { // __ATTR__USERESULT
    // load tree from disk
    ARB_ERROR error;

    psg.enter_stage(STAGE2);
    PT_init_cache_sizes(STAGE2);

    {
        long size = GB_size_of_file(tname);
        if (size<0) {
            error = GB_IO_error("stat", tname);
        }
        else {
            printf("- mapping ptindex ('%s', %s) from disk\n", tname, GBS_readable_size(size, "b"));
            FILE *in = fopen(tname, "r");
            if (!in) {
                error = GB_IO_error("read", tname);
            }
            else {
                error = PTD_read_leafs_from_disk(tname, psg.TREE_ROOT2());
                fclose(in);
            }
        }
    }

    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#include "PT_compress.h"
#endif

void TEST_PrefixProbabilities() {
    PrefixProbabilities prob0(0);
    PrefixProbabilities prob1(1);
    PrefixProbabilities prob2(2);

    const double EPS = 0.00001;

    TEST_EXPECT_SIMILAR(prob0.of(0), 1.0000, EPS); // all

    TEST_EXPECT_SIMILAR(prob1.of(0), 0.0014, EPS); // PT_QU
    TEST_EXPECT_SIMILAR(prob1.of(1), 0.0003, EPS); // PT_N
    TEST_EXPECT_SIMILAR(prob1.of(2), 0.2543, EPS);
    TEST_EXPECT_SIMILAR(prob1.of(3), 0.2268, EPS);
    TEST_EXPECT_SIMILAR(prob1.of(4), 0.3074, EPS);
    TEST_EXPECT_SIMILAR(prob1.of(5), 0.2098, EPS);

    TEST_EXPECT_SIMILAR(prob2.of( 0), 0.00140, EPS); // PT_QU
    TEST_EXPECT_SIMILAR(prob2.of( 1), 0.00000, EPS); // PT_N PT_QU
    TEST_EXPECT_SIMILAR(prob2.of( 2), 0.00000, EPS); // PT_N PT_N
    TEST_EXPECT_SIMILAR(prob2.of( 3), 0.00008, EPS); // PT_N PT_A
    TEST_EXPECT_SIMILAR(prob2.of( 7), 0.00036, EPS); // PT_A PT_QU
    TEST_EXPECT_SIMILAR(prob2.of( 9), 0.06467, EPS); // PT_A PT_A
    TEST_EXPECT_SIMILAR(prob2.of(30), 0.04402, EPS); // PT_T PT_T

    TEST_EXPECT_SIMILAR(prob1.left_of(4), 0.4828, EPS);
    TEST_EXPECT_SIMILAR(prob1.left_of(6), 1.0000, EPS); // all prefixes together

    TEST_EXPECT_SIMILAR(prob2.left_of(19), 0.4828, EPS);
    TEST_EXPECT_SIMILAR(prob2.left_of(31), 1.0000, EPS); // all prefixes together

    TEST_EXPECT_EQUAL(prob0.find_index_near_leftsum(1.0), 1);

    TEST_EXPECT_EQUAL(prob1.find_index_near_leftsum(0.5), 4);
    TEST_EXPECT_SIMILAR(prob1.left_of(4), 0.4828, EPS);
    TEST_EXPECT_SIMILAR(prob1.left_of(5), 0.7902, EPS);

    TEST_EXPECT_EQUAL(prob2.find_index_near_leftsum(0.5), 21);
    TEST_EXPECT_SIMILAR(prob2.left_of(21), 0.48332, EPS);
    TEST_EXPECT_SIMILAR(prob2.left_of(22), 0.56149, EPS);
}

static int count_passes(Partition& p) {
    p.reset();
    int count = 0;
    while (!p.done()) {
        p.next();
        ++count;
    }
    p.reset();
    return count;
}

class Compressed {
    size_t        len;
    PT_compressed compressed;

public:
    Compressed(const char *readable)
        : len(strlen(readable)),
          compressed(len)
    {
        compressed.createFrom(readable, len);
    }
    const char *seq() const { return compressed.get_seq(); }
};

void TEST_MarkedPrefixes() {
    MarkedPrefixes mp0(0);
    MarkedPrefixes mp1(1);
    MarkedPrefixes mp2(2);

    mp0.predecide();
    TEST_EXPECT_EQUAL(mp0.isMarked(Compressed(".").seq()), false);
    TEST_EXPECT_EQUAL(mp0.isMarked(Compressed("T").seq()), false);

    mp0.mark(0, 0);
    mp0.predecide();
    TEST_EXPECT_EQUAL(mp0.isMarked(Compressed(".").seq()), true);
    TEST_EXPECT_EQUAL(mp0.isMarked(Compressed("T").seq()), true);

    mp1.mark(3, 5);
    mp1.predecide();
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed(".").seq()), false);
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed("N").seq()), false);
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed("A").seq()), false);
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed("C").seq()), true);
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed("G").seq()), true);
    TEST_EXPECT_EQUAL(mp1.isMarked(Compressed("T").seq()), true);

    mp2.mark(1, 7);
    mp2.predecide();
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed(".").seq()),  false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("N.").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("NN").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("NA").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("NC").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("NG").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("NT").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("A.").seq()), true);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("AN").seq()), false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("AC").seq()), false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("AG").seq()), false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("AT").seq()), false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("GG").seq()), false);
    TEST_EXPECT_EQUAL(mp2.isMarked(Compressed("TA").seq()), false);
}

#if defined(ARB_64)
#define VAL_64_32_BITDEP(val64,val32) (val64)
#else // !defined(ARB_64)
#define VAL_64_32_BITDEP(val64,val32) (val32)
#endif

void TEST_Partition() {
    PrefixProbabilities p0(0);
    PrefixProbabilities p1(1);
    PrefixProbabilities p2(2);
    PrefixProbabilities p3(3);
    PrefixProbabilities p4(4);

    const int BASES_100k = 100000;

    {
        Partition P01(p0, 1);
        TEST_EXPECT_EQUAL(P01.estimate_probes_for_pass(1, BASES_100k), 100000);
        TEST_EXPECT_EQUAL(P01.estimate_max_probes_for_any_pass(BASES_100k), 100000);
    }

    {
        // distributing memory to 6 passes on a level.1 Partitioner doesn't allow much choice:
        Partition P16(p1, 6);
        TEST_EXPECT_EQUAL(P16.number_of_passes(), 6);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(1, BASES_100k), 140);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(2, BASES_100k), 30);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(3, BASES_100k), 25430);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(4, BASES_100k), 22680);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(5, BASES_100k), 30740);
        TEST_EXPECT_EQUAL(P16.estimate_probes_for_pass(6, BASES_100k), 20980);
        TEST_EXPECT_EQUAL(P16.estimate_max_probes_for_any_pass(BASES_100k), 30740);
        TEST_EXPECT_EQUAL(P16.estimate_max_kb_for_any_pass(BASES_100k), VAL_64_32_BITDEP(440583, 205015));
    }

    {
        // 3 passes
        Partition P13(p1, 3);
        TEST_EXPECT_EQUAL(P13.number_of_passes(), 3);
        TEST_EXPECT_EQUAL(count_passes(P13), 3);

        TEST_EXPECT_EQUAL(P13.contains(Compressed(".").seq()), true);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("N").seq()), true);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("A").seq()), true);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("C").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("G").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("T").seq()), false);

        TEST_EXPECT_EQUAL(P13.next(), true);

        TEST_EXPECT_EQUAL(P13.contains(Compressed(".").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("N").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("A").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("C").seq()), true);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("G").seq()), true);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("T").seq()), false);

        TEST_EXPECT_EQUAL(P13.next(), true);

        TEST_EXPECT_EQUAL(P13.contains(Compressed(".").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("N").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("A").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("C").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("G").seq()), false);
        TEST_EXPECT_EQUAL(P13.contains(Compressed("T").seq()), true);

        TEST_EXPECT_EQUAL(P13.next(), false);

        TEST_EXPECT_EQUAL(P13.estimate_probes_for_pass(1, BASES_100k), 25600);
        TEST_EXPECT_EQUAL(P13.estimate_probes_for_pass(2, BASES_100k), 53420);
        TEST_EXPECT_EQUAL(P13.estimate_probes_for_pass(3, BASES_100k), 20980);
        TEST_EXPECT_EQUAL(P13.estimate_max_probes_for_any_pass(BASES_100k), 53420);
        TEST_EXPECT_EQUAL(P13.estimate_max_kb_for_any_pass(BASES_100k), VAL_64_32_BITDEP(440687, 205101));
    }

    {
        // 2 passes
        Partition P12(p1, 2);
        TEST_EXPECT_EQUAL(P12.number_of_passes(), 2);
        TEST_EXPECT_EQUAL(count_passes(P12), 2);

        TEST_EXPECT_EQUAL(P12.contains(Compressed(".").seq()), true);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("N").seq()), true);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("A").seq()), true);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("C").seq()), true);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("G").seq()), false);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("T").seq()), false);

        TEST_EXPECT_EQUAL(P12.next(), true);

        TEST_EXPECT_EQUAL(P12.contains(Compressed(".").seq()), false);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("N").seq()), false);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("A").seq()), false);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("C").seq()), false);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("G").seq()), true);
        TEST_EXPECT_EQUAL(P12.contains(Compressed("T").seq()), true);

        TEST_EXPECT_EQUAL(P12.next(), false);

        TEST_EXPECT_EQUAL(P12.estimate_probes_for_pass(1, BASES_100k), 48280);
        TEST_EXPECT_EQUAL(P12.estimate_probes_for_pass(2, BASES_100k), 51720);
        TEST_EXPECT_EQUAL(P12.estimate_max_probes_for_any_pass(BASES_100k), 51720);
        TEST_EXPECT_EQUAL(P12.estimate_max_kb_for_any_pass(BASES_100k), VAL_64_32_BITDEP(440679, 205095));
    }

    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   1, BASES_100k), 100000);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   2, BASES_100k), 51720);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   3, BASES_100k), 53420);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   4, BASES_100k), 30740);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   5, BASES_100k), 30740);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p1,   6, BASES_100k), 30740);

    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   1, BASES_100k), 100000);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   2, BASES_100k), 51668);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   3, BASES_100k), 36879);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   4, BASES_100k), 27429);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   5, BASES_100k), 26571);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   6, BASES_100k), 21270);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   7, BASES_100k), 18958);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   8, BASES_100k), 16578);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,   9, BASES_100k), 15899);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,  10, BASES_100k), 14789);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,  15, BASES_100k), 11730);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,  20, BASES_100k), 9449);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p2,  30, BASES_100k), 9449);

    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   1, BASES_100k), 100000);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   2, BASES_100k), 50333);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   3, BASES_100k), 33890);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   4, BASES_100k), 25853);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   5, BASES_100k), 20906);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   6, BASES_100k), 17668);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   7, BASES_100k), 15099);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   8, BASES_100k), 13854);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,   9, BASES_100k), 12259);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  10, BASES_100k), 11073);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  15, BASES_100k), 8168);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  20, BASES_100k), 6401);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  30, BASES_100k), 4737);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  40, BASES_100k), 4176);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3,  50, BASES_100k), 2983);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3, 100, BASES_100k), 2905);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p3, 150, BASES_100k), 2905);

    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   1, BASES_100k), 100000);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   2, BASES_100k), 50084);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   3, BASES_100k), 33425);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   4, BASES_100k), 25072);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   5, BASES_100k), 20145);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   6, BASES_100k), 16837);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   7, BASES_100k), 14528);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   8, BASES_100k), 12606);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,   9, BASES_100k), 11319);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  10, BASES_100k), 10158);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  15, BASES_100k), 6887);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  20, BASES_100k), 5315);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  30, BASES_100k), 3547);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  40, BASES_100k), 2805);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4,  50, BASES_100k), 2336);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4, 100, BASES_100k), 1397);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4, 150, BASES_100k), 1243);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4, 200, BASES_100k), 954);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4, 300, BASES_100k), 893);
    TEST_EXPECT_EQUAL(max_probes_for_passes(p4, 600, BASES_100k), 893);
}

static arb_test::match_expectation decides_on_passes(ULONG bp, size_t avail_mem_kb, int expected_passes, int expected_depth, size_t expected_passsize, size_t expected_memuse, bool expect_to_swap) {
    size_t ARM_size_kb = bp/1800;         // just guess .ARM size

    avail_mem_kb -= ARM_size_kb + PTSERVER_BIN_MB*1024;

    Partition part             = decide_passes_to_use(bp, avail_mem_kb, 0);
    int       decided_passes   = part.number_of_passes();
    int       decided_depth    = part.split_depth();
    size_t    decided_passsize = part.estimate_max_probes_for_any_pass(bp);
    size_t    decided_memuse   = part.estimate_max_kb_for_any_pass(bp);
    bool      decided_to_swap  = decided_memuse>avail_mem_kb;

    using namespace arb_test;
    expectation_group expected;
    expected.add(that(decided_passes).is_equal_to(expected_passes));
    expected.add(that(decided_depth).is_equal_to(expected_depth));
    expected.add(that(decided_passsize).is_equal_to(expected_passsize));
    expected.add(that(decided_memuse).is_equal_to(expected_memuse));
    expected.add(that(decided_to_swap).is_equal_to(expect_to_swap));
    return all().ofgroup(expected);
}

#define TEST_DECIDES_PASSES(bp,memkb,expected_passes,expected_depth,expected_passsize,expected_memuse,expect_to_swap)   \
    TEST_EXPECTATION(decides_on_passes(bp, memkb, expected_passes, expected_depth, expected_passsize, expected_memuse, expect_to_swap))

#define TEST_DECIDES_PASSES__BROKEN(bp,memkb,expected_passes,expected_depth,expected_passsize,expected_memuse,expect_to_swap)   \
    TEST_EXPECTATION__BROKEN(decides_on_passes(bp, memkb, expected_passes, expected_depth, expected_passsize, expected_memuse, expect_to_swap))

void TEST_SLOW_decide_passes_to_use() {
    const ULONG MB = 1024;    // kb
    const ULONG GB = 1024*MB; // kb

    const ULONG BP_SILVA_108_REF  = 891481251ul;
    const ULONG BP_SILVA_108_PARC = BP_SILVA_108_REF * (2492653/618442.0); // rough estimation by number of species
    const ULONG BP_SILVA_108_40K  = 56223289ul;
    const ULONG BP_SILVA_108_12K  = 17622233ul;

    const ULONG MINI_PC       =   2 *GB;
    const ULONG SMALL_PC      =   4 *GB;
#if defined(ARB_64)
    const ULONG SMALL_SERVER  =  12 *GB; // "bilbo"
    const ULONG MEDIUM_SERVER =  20 *GB; // "boarisch"
    const ULONG BIG_SERVER    =  64 *GB;
    const ULONG HUGE_SERVER   = 128 *GB;

    const ULONG MEM1 = ULONG(45.7*GB+0.5);
    const ULONG MEM2 = ULONG(18.7*GB+0.5);
    const ULONG MEM3 = ULONG(11.23*GB+0.5);
    const ULONG MEM4 = ULONG(8*GB+0.5);
#endif
    const ULONG MEM5 = ULONG(4*GB+0.5);

    const ULONG LMEM1 = 3072*MB;
    const ULONG LMEM2 = 2560*MB;
    const ULONG LMEM3 = 2048*MB;
    const ULONG LMEM4 = 1536*MB;
    const ULONG LMEM5 = 1024*MB;
    const ULONG LMEM6 =  768*MB;
    const ULONG LMEM7 =  512*MB;

    const int SWAPS = 1;

#if defined(ARB_64)
    // ---------------- database --------- machine -- passes depth ---- probes ----- memuse - swap?

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEM1,           1,    0, 3593147643UL,  21318473,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEM2,           2,    1, 1858375961,    13356142,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEM3,           4,    2,  985573522,     9350115,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEM4,          11,    4,  333053234,     6355149,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM1,           1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM2,           1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM3,           1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM4,           1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM5,           2,    1,  461074103,     3644812,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM1,          4,    3,  230472727,     2586388,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM2,          8,    3,  123505999,     2095427,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM3,        111,    4,   11443798,     1581079,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM1,          1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM2,          1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM3,          1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM4,          1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM5,          1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM6,          2,    1,   29078685,      642419,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM7,        194,    4,     502032,      511256,  SWAPS);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MINI_PC,      194,    4,   32084148,     4973748,  SWAPS);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MINI_PC,      111,    4,   11443798,     1581079,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  MINI_PC,        1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  MINI_PC,        1,    0,   17622233,      542715,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, SMALL_PC,     194,    4,   32084148,     4973748,  SWAPS);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  SMALL_PC,       2,    1,  461074103,     3644812,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  SMALL_PC,       1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  SMALL_PC,       1,    0,   17622233,      542715,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, SMALL_SERVER,   3,    3, 1217700425,    10415541,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  SMALL_SERVER,   1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  SMALL_SERVER,   1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  SMALL_SERVER,   1,    0,   17622233,      542715,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEDIUM_SERVER,  2,    1, 1858375961,    13356142,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEDIUM_SERVER,  1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  MEDIUM_SERVER,  1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  MEDIUM_SERVER,  1,    0,   17622233,      542715,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, BIG_SERVER,     1,    0, 3593147643UL,  21318473,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  BIG_SERVER,     1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  BIG_SERVER,     1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  BIG_SERVER,     1,    0,   17622233,      542715,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, HUGE_SERVER,    1,    0, 3593147643UL,  21318473,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  HUGE_SERVER,    1,    0,  891481251,     5620314,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  HUGE_SERVER,    1,    0,   56223289,      767008,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  HUGE_SERVER,    1,    0,   17622233,      542715,  0);

#else // !defined(ARB_64)  =>  only test for situations with at most 4Gb
    // ---------------- database --------- machine -- passes depth ---- probes ----- memuse - swap?

    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEM5,           2,    1,  461074103,     2831431,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM1,          3,    2,  328766946,     2327527,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM2,          4,    2,  244526639,     2006690,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  LMEM3,          7,    4,  129515581,     1568659,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM1,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM2,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM3,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM4,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM5,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM6,          1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  LMEM7,          2,    1,   29078685,      370454,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MINI_PC,      194,    4,   32084148,     3835929,  SWAPS);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MINI_PC,        7,    4,  129515581,     1568659,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  MINI_PC,        1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  MINI_PC,        1,    0,   17622233,      289125,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, SMALL_PC,     194,    4,   32084148,     3835929,  SWAPS);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  SMALL_PC,       2,    1,  461074103,     2831431,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  SMALL_PC,       1,    0,   56223289,      473837,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  SMALL_PC,       1,    0,   17622233,      289125,  0);

#endif
}

void NOTEST_SLOW_maybe_build_tree() {
    // does only test sth if DB is present.

    char        dbarg[]    = "-D" "extra_pt_src.arb";
    char       *testDB     = dbarg+2;
    const char *resultPT   = "extra_pt_src.arb.pt";
    const char *expectedPT = "extra_pt_src.arb_expected.pt";
    bool        exists     = GB_is_regularfile(testDB);

    if (exists) {
        char pname[] = "fake_pt_server";
        char barg[]  = "-build";
        char *argv[] = {
            pname,
            barg,
            dbarg,
        };

        // build
        int res = ARB_main(ARRAY_ELEMS(argv), argv);
        TEST_EXPECT_EQUAL(res, EXIT_SUCCESS);

// #define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE(resultPT, expectedPT);
#else // !defined(TEST_AUTO_UPDATE)
        TEST_EXPECT_FILES_EQUAL(resultPT, expectedPT);
#endif
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
