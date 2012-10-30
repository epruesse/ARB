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
            bool atRoot = (at == root);
            PT_create_leaf(&at, loc[height], loc);
            return atRoot ? at : root; // inside tree return old root, otherwise new root has been created
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

static bool all_sons_saved(POS_TREE *node) {
    pt_assert(PT_read_type(node) == PT_NT_NODE);

    for (int i = PT_QU; i < PT_B_MAX; i++) {
        POS_TREE *son = PT_read_son_stage_1(node, (PT_BASES)i);
        if (son) {
            if (!is_saved(son)) return false;
        }
    }
    return true;
}

long PTD_write_subtree(FILE *out, POS_TREE *node, long pos, long *node_pos, ARB_ERROR& error) {
    pt_assert_stage(STAGE1);
    PTD_clear_fathers(node);
    return PTD_write_leafs_to_disk(out, node, pos, node_pos, error);
}

static long save_partial_subtree(FILE *out, POS_TREE *node, const char *partstring, int partsize, long pos, long *node_pos, ARB_ERROR& error) {
    pt_assert_stage(STAGE1);
    if (partsize) {
        pt_assert(PT_read_type(node) == PT_NT_NODE);
        POS_TREE *son = PT_read_son_stage_1(node, (PT_BASES)partstring[0]);
        if (son) {
            long dummy;
            pos = save_partial_subtree(out, son, partstring+1, partsize-1, pos, &dummy, error);
        }
        *node_pos = 0;
    }
    else {
        pos = PTD_write_subtree(out, node, pos, node_pos, error);
    }
    return pos;
}

long PTD_save_partial_tree(FILE *out, POS_TREE *node, const char *partstring, int partsize, long pos, long *node_pos, ARB_ERROR& error) {
    // 'node_pos' is set to the root-node of the last written subtree (only if partsize == 0)
    pos = save_partial_subtree(out, node, partstring, partsize, pos, node_pos, error);
    if (!error && !is_saved(node)) {
#if defined(DEBUG)
        fputs("tree was not completely saved:\n", stderr);
        PT_dump_POS_TREE_recursive(node, " ", stderr);
#endif
        error = "(sub)tree was not saved completely";
    }
    return pos;
}

#if defined(PTM_TRACE_MAX_MEM_USAGE)

static void dump_memusage() {
    malloc_stats();
}
#endif

ARB_ERROR enter_stage_1_build_tree(PT_main * , char *tname) { // __ATTR__USERESULT
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

            putc(0, out);       // disable zero father
            long pos = 1;

            // now temp file exists -> trigger ptserver-selectionlist-update in all
            // ARB applications by writing to log
            GBS_add_ptserver_logentry(GBS_global_string("Calculating probe tree (%s)", tname));

            psg.ptdata = PT_init(STAGE1);

            pt = PT_create_leaf(NULL, PT_N, DataLoc(0, 0, 0));  // create main node
            pt = PT_change_leaf_to_node(pt);
            psg.stat.cut_offs = 0;                  // statistic information
            GB_begin_transaction(psg.gb_main);

            int  partsize = 0;

            {
                ULONG overallBases = psg.char_count;

                printf("Overall bases: %li\n", overallBases);

                while (1) {
                    // @@@ update STAGE1_INDEX_BYTES_PER_BASE
#ifdef ARB_64
# define STAGE1_INDEX_BYTES_PER_BASE 55
#else
# define STAGE1_INDEX_BYTES_PER_BASE 35
#endif

                    ULONG estimated_kb = (STAGE1_INDEX_BYTES_PER_BASE*overallBases)/1024;
                    int   passes       = PrefixIterator(PT_QU, PT_T, partsize).steps();

                    printf("Estimated memory usage for %i passes: %s\n", passes, GBS_readable_size(estimated_kb*1024, "b"));

                    if (estimated_kb <= physical_memory) break;

                    overallBases /= 4; // ignore PT_N- and PT_QU-branches (they are very small compared with other branches)
                    partsize ++;
                }
#if defined(DEBUG) && 1
                // @@@ deactivate when fixed
                // when active, uncomment ../UNIT_TESTER/TestEnvironment.cxx@TEST_AUTO_UPDATE

                partsize = 1;  // again works 
                // partsize = 2; // @@@ fails assertion in save_partial_subtree

                printf("OVERRIDE: Forcing partsize=%i\n", partsize);
#endif
            }

            PrefixIterator partitionPrefix(PT_QU, PT_T, partsize);
            int            passes = partitionPrefix.steps();

            arb_progress pass_progress(GBS_global_string("Tree Build: %s in %i passes",
                                                         GBS_readable_size(psg.char_count, "bp"),
                                                         passes),
                                       passes);

            int  currPass = 0;
            long last_obj = 0;
            while (!partitionPrefix.done()) {
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

                        if (partitionPrefix.matches_at(probe+j)) {
                            pt = build_pos_tree(pt, DataLoc(i, abs_align_pos, j));
                        }
                    }
                    free(align_abs);

                    ++data_progress;
                }

#if defined(PTM_TRACE_MAX_MEM_USAGE)
                dump_memusage();
#endif
                
                pos = PTD_save_partial_tree(out, pt, partitionPrefix.prefix(), partitionPrefix.length(), pos, &last_obj, error);
                if (error) break;

#ifdef PTM_DEBUG_NODES
                PTD_debug_nodes();
#endif
                ++partitionPrefix;
            }

            if (!error) {
                if (partsize) {
#if defined(PTM_TRACE_MAX_MEM_USAGE)
                    dump_memusage();
#endif
                    
                    pos = PTD_save_partial_tree(out, pt, NULL, 0, pos, &last_obj, error);
#ifdef PTM_DEBUG_NODES
                    PTD_debug_nodes();
#endif
                }
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
                fputc(PT_SERVER_VERSION, out);                  // version of PT-Server file
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
