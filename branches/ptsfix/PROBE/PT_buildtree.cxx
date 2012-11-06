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
#include <arb_defs.h>
#include <arb_file.h>
#include <arb_misc.h>

#include <arb_progress.h>

#include <unistd.h>
#include <malloc.h>
#include "PT_partition.h"

#define PTM_TRACE_MAX_MEM_USAGE // @@@ comment out later

// AISC_MKPT_PROMOTE: class DataLoc;

static POS_TREE *build_pos_tree(POS_TREE *const root, const DataLoc& loc) {
    POS_TREE *at = root;
    int       height = 0;

    while (PT_read_type(at) == PT_NT_NODE) {    // now we got an inner node
        POS_TREE *pt_next = PT_read_son_stage_1(at, loc[height]);
        if (!pt_next) { // there is no son of that type -> simply add the new son to that path
            POS_TREE *new_root = root;
            POS_TREE *leaf;
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

            if (loc.is_shorther_than(height)) {
                // end of sequence reached -> change node to chain and add
                pt_assert(loc[height-1] == PT_QU);
                pt_assert(PT_read_type(at) == PT_NT_CHAIN);

                PT_add_to_chain(at, loc);
                return root;
            }
        }
    }

    // type == leaf or chain
    if (PT_read_type(at) == PT_NT_CHAIN) {      // old chain reached
        PT_add_to_chain(at, loc);
        return root;
    }

    // change leaf to node and create two sons

    const DataLoc loc_ref(at);

    while (loc[height] == loc_ref[height]) {  // creates nodes until sequences are different
        pt_assert(PT_read_type(at) != PT_NT_NODE);

        if (PT_read_type(at) == PT_NT_CHAIN) {
            PT_add_to_chain(at, loc);
            return root;
        }
        if (height >= PT_POS_TREE_HEIGHT) {
            if (PT_read_type(at) == PT_NT_LEAF) at = PT_leaf_to_chain(at);
            pt_assert(PT_read_type(at) == PT_NT_CHAIN);
            PT_add_to_chain(at, loc);
            return root;
        }

        pt_assert(PT_read_type(at) == PT_NT_LEAF);

        bool loc_done = loc.is_shorther_than(height+1);
        bool ref_done = loc_ref.is_shorther_than(height+1);

        pt_assert(correlated(loc_done, ref_done));
        pt_assert(implicated(loc_done, loc[height] == PT_QU));
        pt_assert(implicated(ref_done, loc_ref[height] == PT_QU));

        if (ref_done && loc_done) { // end of both sequences
            pt_assert(loc[height] == PT_QU);
            pt_assert(loc_ref[height] == PT_QU);

            at = PT_leaf_to_chain(at); // change leaf to chain
            PT_add_to_chain(at, loc);  // and add node
            return root;
        }

        at = PT_change_leaf_to_node(at);                // change tip to node and append two new leafs
        at = PT_create_leaf(&at, loc[height], loc_ref); // dummy leaf just to create a new node; may become a chain

        height++;
    }

    pt_assert(loc[height] != loc_ref[height]);

    if (height >= PT_POS_TREE_HEIGHT) {
        if (PT_read_type(at) == PT_NT_LEAF) at = PT_leaf_to_chain(at);
        PT_add_to_chain(at, loc);
        return root;
    }
    if (PT_read_type(at) == PT_NT_CHAIN) {
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

static bool all_sons_saved(POS_TREE *node);
inline bool is_saved(POS_TREE *node) {
    PT_NODE_TYPE type = PT_read_type(node);
    return (type == PT_NT_NODE) ? all_sons_saved(node) : (type == PT_NT_SAVED);
}
inline bool is_completely_saved(POS_TREE *node) { return PT_read_type(node) == PT_NT_SAVED; }
static bool all_sons_saved(POS_TREE *node) {
    pt_assert(PT_read_type(node) == PT_NT_NODE);

    for (int i = PT_QU; i < PT_BASES; i++) {
        POS_TREE *son = PT_read_son_stage_1(node, (PT_base)i);
        if (son) {
            if (!is_saved(son)) return false;
        }
    }
    return true;
}

static long write_subtree(FILE *out, POS_TREE *node, long pos, long *node_pos, ARB_ERROR& error) {
    pt_assert_stage(STAGE1);
    PTD_clear_fathers(node);
    return PTD_write_leafs_to_disk(out, node, pos, node_pos, error);
}

static long save_lower_subtree(FILE *out, POS_TREE *node, long pos, int height, ARB_ERROR& error) {
    if (height >= PT_MIN_TREE_HEIGHT) { // in lower part of tree
        long dummy;
        pos = write_subtree(out, node, pos, &dummy, error);
    }
    else {
        switch (PT_read_type(node)) {
            case PT_NT_NODE:
                for (int i = PT_QU; i<PT_BASES; ++i) {
                    POS_TREE *son = PT_read_son_stage_1(node, PT_base(i));
                    if (son) pos = save_lower_subtree(out, son, pos, height+1, error);
                }
                break;

            case PT_NT_CHAIN: {
                long dummy;
                pos = write_subtree(out, node, pos, &dummy, error);
                break;
            }
            case PT_NT_LEAF: pt_assert(0); break; // leafs shall not occur above PT_MIN_TREE_HEIGHT
            case PT_NT_SAVED: break; // ok - saved by previous call
            default: pt_assert(0); break;
        }
    }
    return pos;
}

static long save_upper_tree(FILE *out, POS_TREE *node, long pos, long& node_pos, ARB_ERROR& error) {
    pos = write_subtree(out, node, pos, &node_pos, error);
    return pos;
}

inline void check_tree_was_saved(POS_TREE *node, const char *whatTree, bool completely, ARB_ERROR& error) {
    if (!error) {
        bool saved = completely ? is_completely_saved(node) : is_saved(node);
        if (!saved) {
#if defined(DEBUG)
            fprintf(stderr, "%s was not completely saved:\n", whatTree);
            PT_dump_POS_TREE_recursive(node, " ", stderr);
#endif
            error = GBS_global_string("%s was not saved completely", whatTree);
        }
    }
}

long PTD_save_lower_tree(FILE *out, POS_TREE *node, long pos, ARB_ERROR& error) {
    pos = save_lower_subtree(out, node, pos, 0, error);
    check_tree_was_saved(node, "lower tree", false, error);
    return pos;
}

long PTD_save_upper_tree(FILE *out, POS_TREE *node, long pos, long& node_pos, ARB_ERROR& error) {
    pt_assert(is_saved(node)); // forgot to call PTD_save_lower_tree?
    pos = save_upper_tree(out, node, pos, node_pos, error);
    check_tree_was_saved(node, "tree", true, error);
    return pos;
}

#if defined(PTM_TRACE_MAX_MEM_USAGE)
static void dump_memusage() {
    fflush(stdout);
    printf("\n------------------------------ dump_memusage:\n");

    malloc_stats();
    pid_t     pid   = getpid();
    char     *cmd   = GBS_global_string_copy("pmap -d %i | grep -v lib", pid);
    GB_ERROR  error = GBK_system(cmd);
    if (error) {
        printf("Warning: %s\n", error);
    }
    free(cmd);
    printf("------------------------------ dump_memusage [end]\n");
    fflush(stdout);
}
#endif

static Partitioner decide_passes_to_use(ULONG overallBases, ULONG max_kb_usable) {
    int  partsize = 0;

    {
        printf("Overall bases: %li\n", overallBases);

        while (1) {
            ULONG estimated_kb = estimate_memusage_kb(overallBases);
            int   passes       = PrefixIterator(PT_QU, PT_T, partsize).steps();

            printf("Estimated memory usage for %i passes: %s\n", passes, GBS_readable_size(estimated_kb*1024, "b"));

            if (estimated_kb <= max_kb_usable) break;

            overallBases /= 4; // ignore PT_N- and PT_QU-branches (they are very small compared with other branches)
            partsize ++;
        }
#if defined(DEBUG) && 0
        // @@@ deactivate when fixed
        // when active, uncomment ../UNIT_TESTER/TestEnvironment.cxx@TEST_AUTO_UPDATE

        // partsize = 0; // works
        // partsize = 1; // works
        // partsize = 2; // works
        // partsize = 3; // works
        // partsize = 4; // works
        // partsize = 5; // to test warning below

        printf("OVERRIDE: Forcing partsize=%i\n", partsize);
#endif
    }

    if (partsize>PT_MAX_PARTITION_DEPTH) {
        int req_passes     = PrefixIterator(PT_QU, PT_T, partsize).steps();
        int allowed_passes = PrefixIterator(PT_QU, PT_T, PT_MAX_PARTITION_DEPTH).steps();

        fprintf(stderr,
                "Warning: \n"
                "  You try to build a ptserver from a very big database!\n"
                "\n"
                "  The memory installed on your machine would require to build the ptserver\n"
                "  in %i passes, but the maximum allowed number of passes is %i.\n"
                "\n"
                "  As a result the build of this server may cause your machine to swap huge\n"
                "  amounts of memory and will possibly run for days, weeks or even months.\n"
                ,
                req_passes,
                allowed_passes);

        partsize = PT_MAX_PARTITION_DEPTH;
    }


    pt_assert(partsize <= PT_MAX_PARTITION_DEPTH);

    Partitioner partition(partsize);
    partition.select_passes(partition.max_allowed_passes());
    return partition;
}

ARB_ERROR enter_stage_1_build_tree(PT_main * , const char *tname) { // __ATTR__USERESULT
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
            POS_TREE *pt = NULL;
            
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

            psg.ptdata = PT_init(STAGE1);

            pt = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));  // create main node
            pt = PT_change_leaf_to_node(pt);
            psg.stat.cut_offs = 0;                  // statistic information
            GB_begin_transaction(psg.gb_main);

            ULONG physical_memory = GB_get_physical_memory();
            printf("Available memory: %s\n", GBS_readable_size(physical_memory*1024, "b"));

            Partitioner partition = decide_passes_to_use(psg.char_count, physical_memory);

            // @@@ comment out later:
#define FORCE_PASSES 13
#if defined(FORCE_PASSES)
            partition.force_passes(FORCE_PASSES);
            printf("Warning: Forcing %i passes (for DEBUG reasons)\n", FORCE_PASSES);
#endif

            int passes = partition.selected_passes();
            pt_assert(passes != 1); // @@@ testing

            arb_progress pass_progress(GBS_global_string("Tree Build: %s in %i passes",
                                                         GBS_readable_size(psg.char_count, "bp"),
                                                         passes),
                                       passes);

            int  currPass = 0;
            do {
                pt_assert(!partition.done());

                ++currPass;
                arb_progress data_progress(GBS_global_string("pass %i/%i", currPass, passes), psg.data_count);

                for (int i = 0; i < psg.data_count; i++) {
                    int                      psize;
                    char                    *align_abs = probe_read_alignment(i, &psize);
                    const probe_input_data&  pid       = psg.data[i];
                    const char              *probe     = pid.get_data();

                    int abs_align_pos = psize-1;
                    for (int j = pid.get_size() - 1; j >= 0; j--, abs_align_pos--) {
                        get_abs_align_pos(align_abs, abs_align_pos); // may result in neg. abs_align_pos (seems to happen if sequences are short < 214bp )
                        if (abs_align_pos < 0) break; // -> in this case abort

                        if (partition.contains(probe+j)) {
                            pt = build_pos_tree(pt, DataLoc(i, abs_align_pos, j));
                        }
                    }
                    free(align_abs);

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

                    psg.pt = pt;
                }
            }

            if (error) pass_progress.done();
        }
        free(t2name);
    }

    // @@@ disable later:
    {
        char *related = strdup(tname);
        char *starpos = strstr(related, ".arb.pt");

        pt_assert(starpos);
        strcpy(starpos, ".*");

        char     *listRelated = GBS_global_string_copy("ls -al %s", related);
        GB_ERROR  lserror       = GBK_system(listRelated);

        if (lserror) fprintf(stderr, "Warning: %s\n", lserror);
        free(listRelated);
        free(related);
    }

    return error;
}

ARB_ERROR enter_stage_3_load_tree(PT_main *, const char *tname) { // __ATTR__USERESULT
    // load tree from disk
    ARB_ERROR error;

    psg.ptdata = PT_init(STAGE3);

    {
        long size = GB_size_of_file(tname);
        if (size<0) {
            error = GB_IO_error("stat", tname);
        }
        else {
            printf("- reading Tree %s of size %li from disk\n", tname, size);
            FILE *in = fopen(tname, "r");
            if (!in) {
                error = GB_IO_error("read", tname);
            }
            else {
                error = PTD_read_leafs_from_disk(tname, &psg.pt);
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
#endif

static arb_test::match_expectation decides_on_passes(ULONG bp, size_t avail_mem_kb, int expected_passes, size_t expected_memuse, bool expect_to_swap) {
    Partitioner partition       = decide_passes_to_use(bp, avail_mem_kb);
    int         decided_passes  = partition.selected_passes();
    size_t      decided_memuse  = partition.max_kb_for_passes(decided_passes, bp);
    bool        decided_to_swap = decided_memuse>avail_mem_kb;

    using namespace arb_test;
    return all().of(that(decided_passes).is_equal_to(expected_passes),
                    that(decided_memuse).is_equal_to(expected_memuse),
                    that(decided_to_swap).is_equal_to(expect_to_swap));
}

#define TEST_DECIDES_PASSES(bp,memkb,expected_passes,expected_memuse,expect_to_swap)            \
    TEST_EXPECT(decides_on_passes(bp, memkb, expected_passes, expected_memuse, expect_to_swap))

void TEST_decide_passes_to_use() {
    const ULONG GB = 1024*1024; // kb

    const ULONG BP_SILVA_108_REF  = 891481251ul;
    const ULONG BP_SILVA_108_PARC = BP_SILVA_108_REF * (2492653/618442.0); // rough estimation by number of species
    const ULONG BP_SILVA_108_40K  = 56223289ul;
    const ULONG BP_SILVA_108_12K  = 17622233ul;

    const ULONG MINI_PC       =   2 *GB;
    const ULONG SMALL_PC      =   4 *GB;
    const ULONG SMALL_SERVER  =  12 *GB; // "bilbo"
    const ULONG MEDIUM_SERVER =  20 *GB; // "boarisch"
    const ULONG BIG_SERVER    =  64 *GB;
    const ULONG HUGE_SERVER   = 128 *GB;

    // ---------------- database --------- machine -- passes -- memuse - swap?

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MINI_PC,      781,  3859826,  1); // will swap (max. number of passes reached)
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MINI_PC,      156,   957645,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  MINI_PC,        6,   724753,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  MINI_PC,        1,   946506,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, SMALL_PC,     156,  3859826,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  SMALL_PC,      31,  2758020,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  SMALL_PC,       1,  3019805,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  SMALL_PC,       1,   946506,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, SMALL_SERVER,  31, 11116300,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  SMALL_SERVER,   6, 11491750,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  SMALL_SERVER,   1,  3019805,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  SMALL_SERVER,   1,   946506,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, MEDIUM_SERVER, 31, 11116300,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  MEDIUM_SERVER,  6, 11491750,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  MEDIUM_SERVER,  1,  3019805,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  MEDIUM_SERVER,  1,   946506,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, BIG_SERVER,     6, 46317918,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  BIG_SERVER,     1, 47882293,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  BIG_SERVER,     1,  3019805,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  BIG_SERVER,     1,   946506,  0);

    TEST_DECIDES_PASSES(BP_SILVA_108_PARC, HUGE_SERVER,    6, 46317918,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_REF,  HUGE_SERVER,    1, 47882293,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_40K,  HUGE_SERVER,    1,  3019805,  0);
    TEST_DECIDES_PASSES(BP_SILVA_108_12K,  HUGE_SERVER,    1,   946506,  0);
}

void NOTEST_SLOW_maybe_build_tree() {
    // does only test sth if DB is present.

    const char *dbarg      = "-D" "extra_pt_src.arb";
    const char *testDB     = dbarg+2;
    const char *resultPT   = "extra_pt_src.arb.pt";
    const char *expectedPT = "extra_pt_src.arb_expected.pt";
    bool        exists     = GB_is_regularfile(testDB);

    if (exists) {
        const char *argv[] = {
            "fake_pt_server",
            "-build",
            dbarg,
        };

#if 1
        // build
        int res = ARB_main(ARRAY_ELEMS(argv), argv);
        TEST_ASSERT_EQUAL(res, EXIT_SUCCESS);
#endif

// #define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE(resultPT, expectedPT);
#else // !defined(TEST_AUTO_UPDATE)
        TEST_ASSERT_FILES_EQUAL(resultPT, expectedPT);
#endif
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
