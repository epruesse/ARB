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
#define DEBUG_MAX_CHAIN_NAME_DIST 99999
#define DEBUG_TREE_DEPTH          (PT_POS_TREE_HEIGHT+1)

struct PT_statistic {
    static bool show_for_index(int i, int imax) {
        // return true; // uncomment to show all
        if (i == imax) return true; // always show last index
        if (i<20) return true;
        if (i<100) return (i%10)           == 9;
        if (i<1000) return (i%100)         == 99;
        if (i<10000) return (i%1000)       == 999;
        if (i<100000) return (i%10000)     == 9999;
        if (i<1000000) return (i%100000)   == 99999;
        if (i<10000000) return (i%1000000) == 999999;
        pt_assert(0);
        return true;
    }

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

    struct chain_head_mem {
        size_t flag;
        size_t apos;
        size_t size;
    } chm;

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

                {
                    chm.flag++;
                    const char *ptr  = ((char*)pt)+1;
                    const char *next = ptr;

                    next = next + (pt->flags&1 ? 4 : 2); chm.apos += (next-ptr); ptr = next;
                    PT_read_compact_nat(next);           chm.size += (next-ptr); ptr = next;
                }

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
        printf("%10zu  mem:%7s  mean:%5.2f b", count, GBS_readable_size(mem, "b"), mem/double(count));
    }
    void printCountPercentAndMem(size_t count, size_t all, size_t mem, size_t allmem) {
        printf("%10zu (=%5.2f%%)  mem:%7s (=%5.2f%%)  mean:%5.2f b",
               count, count*100.0/all,
               GBS_readable_size(mem, "b"), mem*100.0/allmem,
               mem/double(count));
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

        size_t chain_sum = 0;

        mem = sum = 0;
        for (int i=0; i <= DEBUG_MAX_CHAIN_SIZE; i++) {
            size_t k = chains_of_size[i];
            if (k) {
                size_t e = i*k;

                chain_sum += k;
                sum       += e;
                mem       += chains_of_size_mem[i];
            }
        }
        {
            int first_skipped = 0;

            size_t k_sum = 0;
            size_t e_sum = 0;
            size_t m_sum = 0;

            for (int i=0; i <= DEBUG_MAX_CHAIN_SIZE; i++) {
                bool show = show_for_index(i, DEBUG_MAX_CHAIN_SIZE);
                if (!show && !first_skipped) {
                    first_skipped = i;
                    pt_assert(first_skipped); // 0 has to be shown always
                }

                size_t k = chains_of_size[i];
                if (k) {
                    size_t e = i*k;

                    k_sum += k;
                    e_sum += e;
                    m_sum += chains_of_size_mem[i];
                }

                if (show) {
                    if (k_sum) {
                        if (first_skipped) printf("chain of size %7i-%7i", first_skipped, i);
                        else printf("chain of size %15i", i);

                        printf(" occur %10zu (%5.2f%%)  entries:", k_sum, k_sum*100.0/chain_sum);
                        printCountPercentAndMem(e_sum, sum, m_sum, mem);
                        LF();
                    }

                    first_skipped = 0;

                    k_sum = 0;
                    e_sum = 0;
                    m_sum = 0;
                }
            }
        }
        fputs("ch.entries (all):  ", stdout); printCountAndMem(sum, mem); printf(" (%5.2f%%)\n", mem*100.0/indexsize);
        // Note: chains were already added to allmem above
        pt_assert(chain_mem1 == mem); // both chain-examinations should result in same mem size

        {
            size_t followup_chain_entries = 0;
            for (int i = 0; i <= DEBUG_MAX_CHAIN_NAME_DIST; ++i) { // LOOP_VECTORIZED
                followup_chain_entries += chain_name_dist[i];
            }

            double pcsum         = 0.0;
            int    first_skipped = 0;
            size_t k_sum         = 0;

            for (int i = 0; i <= DEBUG_MAX_CHAIN_NAME_DIST; ++i) {
                bool show = show_for_index(i, DEBUG_MAX_CHAIN_NAME_DIST);
                if (!show && !first_skipped) {
                    first_skipped = i;
                    pt_assert(first_skipped); // 0 has to be shown always
                }

                k_sum += chain_name_dist[i];
                if (show) {
                    if (k_sum) {
                        if (first_skipped) printf("chain-entry-name-dist of %5i-%5i", first_skipped, i);
                        else               printf("chain-entry-name-dist of %11i", i);

                        double pc  = k_sum*100.0/followup_chain_entries;
                        pcsum     += pc;

                        printf(" occurs %10zu (=%5.2f%%; sum=%5.2f%%)\n", k_sum, pc, pcsum);
                    }

                    first_skipped = 0;
                    k_sum         = 0;
                }
            }
            printf("overall followup-chain-entries: %zu\n", followup_chain_entries);
        }

        {
            size_t allhead = chm.flag+chm.apos+chm.size;
            printf("Chain header summary:\n"
                   "  flag  %s (=%5.2f%%) mean:%5.2f b\n", GBS_readable_size(chm.flag,  "b"), chm.flag *100.0/mem, 1.0);
            printf("  apos  %s (=%5.2f%%) mean:%5.2f b\n", GBS_readable_size(chm.apos,  "b"), chm.apos *100.0/mem, double(chm.apos) /chm.flag);
            printf("  size  %s (=%5.2f%%) mean:%5.2f b\n", GBS_readable_size(chm.size,  "b"), chm.size *100.0/mem, double(chm.size) /chm.flag);
            printf("  all   %s (=%5.2f%%) mean:%5.2f b\n", GBS_readable_size(allhead,   "b"), allhead  *100.0/mem, double(allhead)  /chm.flag);
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
        fflush_all();
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
