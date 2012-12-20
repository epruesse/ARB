// ============================================================= //
//                                                               //
//   File      : PT_debug.cxx                                    //
//   Purpose   : debugging code                                  //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "probe.h"
#include "probe_tree.h"
#include "pt_prototypes.h"

#include <arb_misc.h>
#include <arb_file.h>

#if defined(CALCULATE_STATS_ON_QUERY)

#define DEBUG_MAX_CHAIN_SIZE      10000000
#define DEBUG_MAX_CHAIN_NAME_DIST 40
#define DEBUG_TREE_DEPTH          (PT_POS_TREE_HEIGHT+1)


struct PT_statistic {
    // nodes
    size_t nodes[DEBUG_TREE_DEPTH];
    size_t nodes_mem[DEBUG_TREE_DEPTH];

    size_t splits[DEBUG_TREE_DEPTH][PT_BASES];

    // leafs
    size_t tips[DEBUG_TREE_DEPTH];
    size_t tips_mem[DEBUG_TREE_DEPTH];

    // chains
    size_t chains_at_depth[DEBUG_TREE_DEPTH];
    size_t chains_at_depth_mem[DEBUG_TREE_DEPTH];

    size_t chains_of_size[DEBUG_MAX_CHAIN_SIZE+1];
    size_t chains_of_size_mem[DEBUG_MAX_CHAIN_SIZE+1];

    size_t chaincount;

    size_t chain_name_dist[DEBUG_MAX_CHAIN_NAME_DIST+1];

    PT_statistic() { memset(this, 0, sizeof(*this)); }

    void analyse(POS_TREE2 *pt, int height) {
        pt_assert(height<DEBUG_TREE_DEPTH);
        switch (pt->get_type()) {
            case PT2_NODE: {
                int basecnt = 0;
                for (int i=PT_QU; i<PT_BASES; i++) {
                    POS_TREE2 *pt_help = PT_read_son(pt, (PT_base)i);
                    if (pt_help) {
                        basecnt++;
                        analyse(pt_help, height+1);
                    }
                }

                nodes[height]++;
                nodes_mem[height] += PT_node_size(pt);

                splits[height][basecnt]++;
                break;
            }
            case PT2_LEAF:
                tips[height]++;
                tips_mem[height] += PT_leaf_size(pt);
                break;

            case PT2_CHAIN: {
                size_t              size = 1;
                ChainIteratorStage2 iter(pt);

                int lastName = iter.at().get_name();
                while (++iter) {
                    {
                        int thisName = iter.at().get_name();
                        pt_assert(thisName >= lastName);

                        int nameDist = thisName-lastName;
                        if (nameDist>DEBUG_MAX_CHAIN_NAME_DIST) nameDist = DEBUG_MAX_CHAIN_NAME_DIST;
                        ++chain_name_dist[nameDist];
                        lastName = thisName;
                    }
                    ++size;
                }

                if (size>DEBUG_MAX_CHAIN_SIZE) size = DEBUG_MAX_CHAIN_SIZE;

                size_t mem = iter.memptr()-(char*)pt;

                chains_of_size[size]++;
                chains_of_size_mem[size]    += mem;
                chains_at_depth[height]++;
                chains_at_depth_mem[height] += mem;

                chaincount++;
                break;
            }
        }
    }

    void printCountAndMem(size_t count, size_t mem) {
        printf("%10zu mem:%7s mean:%5.2f b", count, GBS_readable_size(mem, "b"), mem/double(count));
    }

    void LF() { putchar('\n'); }

    void dump(size_t indexsize) {
        size_t sum    = 0;
        size_t mem    = 0;
        size_t allmem = 0;

        for (int i=0; i<DEBUG_TREE_DEPTH; i++) {
            size_t k = nodes[i];
            if (k) {
                sum += k;
                mem += nodes_mem[i];
                printf("nodes at depth  %2i:", i); printCountAndMem(k, nodes_mem[i]);
                for (int j=0; j<PT_BASES; j++) {
                    k = splits[i][j];
                    printf("  [%i]=%-10zu", j, k);
                }
                LF();
            }
        }
        fputs("nodes (all):       ", stdout); printCountAndMem(sum, mem); printf(" (%5.2f%%)\n", mem*100.0/indexsize);
        allmem += mem;

        mem = sum = 0;
        for (int i=0; i<DEBUG_TREE_DEPTH; i++) {
            size_t k = tips[i];
            if (k) {
                sum += k;
                mem += tips_mem[i];
                printf("tips at depth   %2i:", i); printCountAndMem(k, tips_mem[i]); LF();
            }
        }
        fputs("tips (all):        ", stdout); printCountAndMem(sum, mem); printf(" (%5.2f%%)\n", mem*100.0/indexsize);
        allmem += mem;

        mem = sum = 0;
        for (int i=0; i<DEBUG_TREE_DEPTH; i++) {
            size_t k = chains_at_depth[i];
            if (k) {
                sum += k;
                mem += chains_at_depth_mem[i];
                printf("chains at depth %2i:", i); printCountAndMem(k, chains_at_depth_mem[i]); LF();
            }
        }
        fputs("chains (all):      ", stdout); printCountAndMem(sum, mem); printf(" (%5.2f%%)\n", mem*100.0/indexsize);
        allmem += mem;

#if defined(ASSERTION_USED)
        size_t chain_mem1 = mem;
#endif

        mem = sum = 0;
        for (int i=0; i <= DEBUG_MAX_CHAIN_SIZE; i++) {
            size_t k = chains_of_size[i];
            if (k) {
                size_t e  = i*k;
                sum      += e;
                mem      += chains_of_size_mem[i];
                printf("chain of size %5i occur %10zu entries:", i, k); printCountAndMem(e, chains_of_size_mem[i]); LF();
            }
        }
        fputs("ch.entries (all):  ", stdout); printCountAndMem(sum, mem); printf(" (%5.2f%%)\n", mem*100.0/indexsize);
        // Note: chains were already added to allmem above
        pt_assert(chain_mem1 == mem); // both chain-examinations should result in same mem size

        {
            size_t followup_chain_entries = 0;
            for (int i = 0; i <= DEBUG_MAX_CHAIN_NAME_DIST; ++i) {
                followup_chain_entries += chain_name_dist[i];
            }
            for (int i = 0; i <= DEBUG_MAX_CHAIN_NAME_DIST; ++i) {
                size_t k = chain_name_dist[i];
                if (k) {
                    printf("chain-entry-name-dist of %2i occurs %10zu (=%5.2f%%)\n", i, k, k*100.0/followup_chain_entries);
                }
            }
            printf("overall followup-chain-entries: %zu\n", followup_chain_entries);
        }

        size_t known_other_mem =
            1 + // dummy byte at start of file (avoids that address of a node is zero)
            4 + // PT_SERVER_MAGIC
            1 + // PT_SERVER_VERSION
            2 + // info block size
            (psg.big_db ? 8 : 0) + // big pointer to root node (if final int is 0xffffffff)
            4;  // final int

        allmem += known_other_mem;

        printf("overall mem:%7s\n", GBS_readable_size(allmem, "b"));
        printf("indexsize:  %7s\n", GBS_readable_size(indexsize, "b"));

        if (indexsize>allmem) {
            printf("(file-mem):%7s\n", GBS_readable_size(indexsize-allmem, "b"));
        }
        else if (indexsize<allmem) {
            printf("(mem-file):%7s\n", GBS_readable_size(allmem-indexsize, "b"));
        }
        else {
            puts("indexsize exactly matches counted memory");
        }
    }
};
#endif

void PT_dump_tree_statistics(const char *indexfilename) {
#if defined(CALCULATE_STATS_ON_QUERY)
    // show various debug information about the tree
    PT_statistic *stat = new PT_statistic;
    stat->analyse(psg.TREE_ROOT2(), 0);

    size_t filesize = GB_size_of_file(indexfilename);
    stat->dump(filesize);

    delete stat;
#endif
}

// --------------------------------------------------------------------------------

class PT_dump_loc {
    const char *prefix;
    FILE       *out;
public:
    PT_dump_loc(const char *Prefix, FILE *Out) : prefix(Prefix), out(Out) {}

    int operator()(const DataLoc& loc) {
        fprintf(out, "%s %i=%s@%i>%i\n", prefix, loc.get_name(), loc.get_pid().get_shortname(), loc.get_abs_pos(), loc.get_rel_pos());
        return 0;
    }
    int operator()(const AbsLoc& loc) {
        fprintf(out, "%s %i=%s@%i\n", prefix, loc.get_name(), loc.get_pid().get_shortname(), loc.get_abs_pos());
        return 0;
    }
};

template <typename PT> inline void dump_POS_TREE_special(PT *pt, const char *prefix, FILE *out);
template <> inline void dump_POS_TREE_special(POS_TREE1 *pt, const char *prefix, FILE *out) {
    if (pt->is_saved()) {
        fprintf(out, "{x} %s [saved]\n", prefix);
    }
    else {
        pt_assert(pt->get_type() == PT1_UNDEF);
        fprintf(out, "{?} %s [undefined]\n", prefix);
    }
}
template <> inline void dump_POS_TREE_special(POS_TREE2 *, const char *, FILE *) {}

template <typename PT> void PT_dump_POS_TREE_recursive(PT *pt, const char *prefix, FILE *out) {
    if (pt->is_node()) {
        for (int b = PT_QU; b<PT_BASES; b++) {
            PT *son = PT_read_son<PT>(pt, PT_base(b));
            if (son) {
                char *subPrefix = GBS_global_string_copy("%s%c", prefix, base_2_readable(b));
                PT_dump_POS_TREE_recursive(son, subPrefix, out);
                free(subPrefix);
            }
        }
    }
    else if (pt->is_leaf()) {
        char *subPrefix = GBS_global_string_copy("{l} %s", prefix);
        PT_dump_loc dump_leaf(subPrefix, out);
        dump_leaf(DataLoc(pt));
        free(subPrefix);
    }
    else if (pt->is_chain()) {
        char *subPrefix = GBS_global_string_copy("{c} %s", prefix);
        PT_dump_loc locDumper(subPrefix, out);
        PT_forwhole_chain(pt, locDumper);
        free(subPrefix);
    }
    else {
        dump_POS_TREE_special(pt, prefix, out);
    }
}

// --------------------------------------------------------------------------------

void PT_dump_POS_TREE(POS_TREE1 *IF_DEBUG(node), FILE *IF_DEBUG(out)) {
    // Debug function for all stages
#if defined(DEBUG)
    if (!node) fputs("<zero node>\n", out);

    fprintf(out, "node father %p\n", node->get_father());

    switch (node->get_type()) {
        case PT1_LEAF: {
            DataLoc loc(node);
            fprintf(out, "leaf %i:%i,%i\n", loc.get_name(), loc.get_rel_pos(), loc.get_abs_pos());
            break;
        }
        case PT1_NODE:
            for (long i = 0; i < PT_BASES; i++) {
                fprintf(out, "%6li:0x%p\n", i, PT_read_son(node, (PT_base)i));
            }
            break;
        case PT1_CHAIN:
            fputs("chain:\n", out);
            PTD_chain_print chainPrinter;
            PT_forwhole_chain(node, chainPrinter);
            break;
        case PT1_SAVED:
            fputs("saved:\n", out);
            break;
        case PT1_UNDEF:
            fputs("<invalid node type>\n", out);
            break;
    }
    fflush_all();
#endif
}

static void PT_dump_POS_TREE_to_file(const char *dumpfile) {
    FILE *dump = fopen(dumpfile, "wt");
    if (!dump) {
        GBK_terminate(GB_IO_error("writing", dumpfile));
    }
    if (psg.get_stage() == STAGE1) {
        PT_dump_POS_TREE_recursive(psg.TREE_ROOT1(), "", dump);
    }
    else {
        PT_dump_POS_TREE_recursive(psg.TREE_ROOT2(), "", dump);
    }
    fclose(dump);

    fprintf(stderr, "Note: index has been dumped to '%s'\n", dumpfile);
}


int PT_index_dump(const PT_main *main) {
    const char *outfile = main->dumpname;
    PT_dump_POS_TREE_to_file(outfile);
    return 0;
}
