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

#if defined(DEBUG)

#define DEBUG_MAX_CHAIN_SIZE 1000
#define DEBUG_TREE_DEEP      PT_POS_TREE_HEIGHT+50

struct PT_debug {
    int chainsizes[DEBUG_MAX_CHAIN_SIZE][DEBUG_TREE_DEEP];
    int chainsizes2[DEBUG_MAX_CHAIN_SIZE];
    int splits[DEBUG_TREE_DEEP][PT_B_MAX];
    int nodes[DEBUG_TREE_DEEP];
    int tips[DEBUG_TREE_DEEP];
    int chains[DEBUG_TREE_DEEP];
    int chaincount;
    } *ptds;

struct PT_chain_count {
    int operator() (const DataLoc&) {
        psg.height++;
        return 0;
    }
};

static void analyse_tree(POS_TREE *pt, int height) {
    PT_NODE_TYPE type;
    POS_TREE *pt_help;
    int i;
    int basecnt;
    type = PT_read_type(pt);
    switch (type) {
        case PT_NT_NODE:
            ptds->nodes[height]++;
            basecnt = 0;
            for (i=PT_QU; i<PT_B_MAX; i++) {
                if ((pt_help = PT_read_son(psg.ptmain, pt, (PT_BASES)i)))
                {
                    basecnt++;
                    analyse_tree(pt_help, height+1);
                };
            }
            ptds->splits[height][basecnt]++;
            break;
        case PT_NT_LEAF:
            ptds->tips[height]++;
            break;
        default:
            ptds->chains[height]++;
            psg.height = 0;
            PT_forwhole_chain(psg.ptmain, pt, PT_chain_count());
            if (psg.height >= DEBUG_MAX_CHAIN_SIZE) psg.height = DEBUG_MAX_CHAIN_SIZE;
            ptds->chainsizes[psg.height][height]++;
            ptds->chainsizes2[psg.height]++;
            ptds->chaincount++;
            if (ptds->chaincount<20) {
                printf("\n\n\n\n");
                PT_forwhole_chain(psg.ptmain, pt, PTD_chain_print());
            }
            break;
    };
}
#endif

void PT_dump_tree_statistics() {
#if defined(DEBUG)
    // show various debug information about the tree
    ptds = (PT_debug *)calloc(sizeof(PT_debug), 1);
    analyse_tree(psg.pt, 0);
    int i, j, k;
    int sum = 0;            // sum of chains
    for (i=0; i<DEBUG_TREE_DEEP; i++) {
        k = ptds->nodes[i];
        if (k) {
            sum += k;
            printf("nodes at deep %i: %i        sum %i\t\t", i, k, sum);
            for (j=0; j<PT_B_MAX; j++) {
                k =     ptds->splits[i][j];
                printf("%i:%i\t", j, k);
            }
            printf("\n");
        }
    }
    sum = 0;
    for (i=0; i<DEBUG_TREE_DEEP; i++) {
        k = ptds->tips[i];
        sum += k;
        if (k)  printf("tips at deep %i: %i     sum %i\n", i, k, sum);
    }
    sum = 0;
    for (i=0; i<DEBUG_TREE_DEEP; i++) {
        k = ptds->chains[i];
        if (k) {
            sum += k;
            printf("chains at deep %i: %i       sum %i\n", i, k, sum);
        }
    }
    sum = 0;
    int sume = 0;           // sum of chain entries
    for (i=0; i<DEBUG_MAX_CHAIN_SIZE; i++) {
            k =     ptds->chainsizes2[i];
            sum += k;
            sume += i*k;
            if (k) printf("chain of size %i occur %i    sc %i   sce %i\n", i, k, sum, sume);
    }
#endif
}

// --------------------------------------------------------------------------------

#if defined(DEBUG)

inline char PT_BASES_2_char(PT_BASES i) {
    static char buffer[] = "x";

    buffer[0] = i;
    PT_base_2_string(buffer, 1);

    return buffer[0];
}

class PT_dump_leaf {
    const char *prefix;
public:
    PT_dump_leaf(const char *Prefix) : prefix(Prefix) {}

    int operator()(const DataLoc& leaf) {
        struct probe_input_data& data = psg.data[leaf.name];

        PT_BASES b = (PT_BASES)data.data[leaf.rpos];

        printf("%s[%c] %s apos=%i rpos=%i\n", prefix, PT_BASES_2_char(b), data.name, leaf.apos, leaf.rpos);
        return 0;
    }
};
#endif // DEBUG

void PT_dump_POS_TREE_recursive(POS_TREE *pt, const char *prefix) {
#if defined(DEBUG)
    switch (PT_read_type(pt)) {
        case PT_NT_NODE:
            for (int i = PT_N; i<PT_B_MAX; i++) {
                PT_BASES  b   = PT_BASES(i);
                POS_TREE *son = PT_read_son(psg.ptmain, pt, b);
                if (son) {
                    char *subPrefix = GBS_global_string_copy("%s%c", prefix, PT_BASES_2_char(b));
                    PT_dump_POS_TREE_recursive(son, subPrefix);
                    free(subPrefix);
                }
            }
            break;
        case PT_NT_LEAF: {
            PT_dump_leaf dump_leaf(prefix);
            dump_leaf(DataLoc(psg.ptmain, pt));
            break;
        }
        case PT_NT_CHAIN:
            PT_forwhole_chain(psg.ptmain, pt, PT_dump_leaf(prefix));
            break;
        default:
            printf("%s [unhandled]\n", prefix);
            pt_assert(0);
            break;
    }
#endif
}

// --------------------------------------------------------------------------------

void PT_dump_POS_TREE(POS_TREE * IF_DEBUG(node)) {
    // Debug function for all stages
#if defined(DEBUG)
    long             i;
    PTM2        *ptmain = psg.ptmain;
    if (!node) printf("Zero node\n");
    PT_READ_PNTR(&node->data, i);
    printf("node father 0x%lx\n", i);
    switch (PT_read_type(node)) {
        case PT_NT_LEAF:
            printf("leaf %i:%i,%i\n", PT_read_name(ptmain, node),
                   PT_read_rpos(ptmain, node), PT_read_apos(ptmain, node));
            break;
        case PT_NT_NODE:
            for (i = 0; i < PT_B_MAX; i++) {
                printf("%6li:0x%p\n", i, PT_read_son(ptmain, node, (enum PT_bases_enum)i));
            }
            break;
        case PT_NT_CHAIN:
            printf("chain:\n");
            PT_forwhole_chain(ptmain, node, PTD_chain_print());
            break;
        case PT_NT_SAVED:
            printf("saved:\n");
            break;
        default:
            printf("?????\n");
            break;
    }
#endif
}

