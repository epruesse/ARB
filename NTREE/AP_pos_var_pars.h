// =============================================================== //
//                                                                 //
//   File      : AP_pos_var_pars.h                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_POS_VAR_PARS_H
#define AP_POS_VAR_PARS_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef NT_LOCAL_PROTO_H
#include "NT_local_proto.h"
#endif

class AW_window;
class AW_root;

#define AWAR_PVP_SAI  "tmp/pos_var_pars/sai"
#define AWAR_PVP_TREE "tmp/pos_var_pars/tree"

class arb_progress;

class AP_pos_var : virtual Noncopyable {
    GBDATA        *gb_main;
    long           treesize;         // max value for slider
    arb_progress  *progress;
    GB_UINT4      *frequencies[256]; // count every occurrence
    GB_UINT4      *transitions;      // minimum transitions
    GB_UINT4      *transversions;    // minimum transversions (dna only)
    unsigned char  char_2_freq[256]; // mapper (~ toupper)
    long           char_2_transition[256]; // a->1 c->2 g->4 ...
    long           char_2_transversion[256]; // y->1 r->2

    long  getsize(GBT_TREE *tree); // size of tree
    long  ali_len;              // max len of alignment
    long  is_dna;
    char *ali_name;
    char *tree_name;

    const char *parsimony(GBT_TREE *tree, GB_UINT4 *bases = 0, GB_UINT4 *ltbases = 0);

public:

    AP_pos_var(GBDATA *gb_main, char *ali_name, long ali_len, int isdna, char *tree_name);
    ~AP_pos_var();

    GB_ERROR retrieve(GBT_TREE *tree);
    GB_ERROR delete_old_sai(const char *sai_name);
    GB_ERROR save_sai(const char *sai_name);
};

#else
#error AP_pos_var_pars.h included twice
#endif // AP_POS_VAR_PARS_H
