// =============================================================== //
//                                                                 //
//   File      : AP_pos_var_pars.cxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "AP_pos_var_pars.h"

#include <AP_pro_a_nucs.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_TreeAwars.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <arb_global_defs.h>

#include <cctype>
#include <cmath>

#define ap_assert(cond) arb_assert(cond)


AP_pos_var::AP_pos_var(GBDATA *gb_maini, char *ali_namei, long ali_leni, int isdna, char *tree_namei) {
    memset((char  *)this, 0, sizeof(AP_pos_var));
    gb_main   = gb_maini;
    ali_name  = strdup(ali_namei);
    is_dna    = isdna;
    ali_len   = ali_leni;
    tree_name = strdup(tree_namei);
    progress  = NULL;
}

AP_pos_var::~AP_pos_var() {
    delete progress;
    free(ali_name);
    free(tree_name);
    free(transitions);
    free(transversions);
    for (int i=0; i<256; i++) free(frequencies[i]);
}

long AP_pos_var::getsize(GBT_TREE *tree) {
    if (!tree) return 0;
    if (tree->is_leaf) return 1;
    return getsize(tree->leftson) + getsize(tree->rightson) + 1;
}

const char *AP_pos_var::parsimony(GBT_TREE *tree, GB_UINT4 *bases, GB_UINT4 *tbases) {
    GB_ERROR error = 0;

    if (tree->is_leaf) {
        if (tree->gb_node) {
            GBDATA *gb_data = GBT_read_sequence(tree->gb_node, ali_name);
            if (gb_data) {
                size_t seq_len = ali_len;
                if (GB_read_string_count(gb_data) < seq_len) {
                    seq_len = GB_read_string_count(gb_data);
                }

                unsigned char *sequence = (unsigned char*)GB_read_char_pntr(gb_data);
                for (size_t i = 0; i< seq_len; i++) {
                    long L = char_2_freq[sequence[i]];
                    if (L) {
                        ap_assert(frequencies[L]);
                        frequencies[L][i]++;
                    }
                }

                if (bases) {
                    for (size_t i = 0; i< seq_len; i++) bases[i] = char_2_transition[sequence[i]];
                }
                if (tbases) {
                    for (size_t i = 0; i< seq_len; i++) tbases[i] = char_2_transversion[sequence[i]];
                }
            }
        }
    }
    else {
        GB_UINT4 *ls  = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);
        GB_UINT4 *rs  = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);
        GB_UINT4 *lts = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);
        GB_UINT4 *rts = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);

        if (!error) error = this->parsimony(tree->leftson, ls, lts);
        if (!error) error = this->parsimony(tree->rightson, rs, rts);
        if (!error) {
            for (long i=0; i< ali_len; i++) {
                long l = ls[i];
                long r = rs[i];
                if (l & r) {
                    if (bases) bases[i] = l&r;
                }
                else {
                    transitions[i] ++;
                    if (bases) bases[i] = l|r;
                }
                l = lts[i];
                r = rts[i];
                if (l & r) {
                    if (tbases) tbases[i] = l&r;
                }
                else {
                    transversions[i] ++;
                    if (tbases) tbases[i] = l|r;
                }
            }
        }

        free(lts);
        free(rts);

        free(ls);
        free(rs);
    }
    progress->inc_and_check_user_abort(error);
    return error;
}


// Calculate the positional variability: control procedure
GB_ERROR AP_pos_var::retrieve(GBT_TREE *tree) {
    GB_ERROR error = 0;

    if (is_dna) {
        unsigned char *char_2_bitstring;
        char_2_freq[(unsigned char)'a'] = 'A';
        char_2_freq[(unsigned char)'A'] = 'A';
        char_2_freq[(unsigned char)'c'] = 'C';
        char_2_freq[(unsigned char)'C'] = 'C';
        char_2_freq[(unsigned char)'g'] = 'G';
        char_2_freq[(unsigned char)'G'] = 'G';
        char_2_freq[(unsigned char)'t'] = 'U';
        char_2_freq[(unsigned char)'T'] = 'U';
        char_2_freq[(unsigned char)'u'] = 'U';
        char_2_freq[(unsigned char)'U'] = 'U';
        char_2_bitstring                = (unsigned char *)AP_create_dna_to_ap_bases();
        for (int i=0; i<256; i++) {
            int j;
            if (i=='-') j = '.'; else j = i;
            long base = char_2_transition[i] = char_2_bitstring[j];
            char_2_transversion[i] = 0;
            if (base & (AP_A | AP_G)) char_2_transversion[i] = 1;
            if (base & (AP_C | AP_T)) char_2_transversion[i] |= 2;
        }
        delete [] char_2_bitstring;
    }
    else {
        AWT_translator *translator       = AWT_get_user_translator(gb_main);
        const long     *char_2_bitstring = translator->Pro2Bitset();

        for (int i=0; i<256; i++) {
            char_2_transversion[i] = 0;
            long base = char_2_transition[i] = char_2_bitstring[i];
            if (base) char_2_freq[i] = toupper(i);
        }
    }
    treesize = getsize(tree);
    progress = new arb_progress(treesize);

    for (int i=0; i<256; i++) {
        int j;
        if ((j = char_2_freq[i])) {
            if (!frequencies[j]) {
                frequencies[j] = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);
            }
        }
    }

    transitions = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);
    transversions = (GB_UINT4 *)calloc(sizeof(GB_UINT4), (int)ali_len);

    error = this->parsimony(tree);

    return error;
}

GB_ERROR AP_pos_var::delete_old_sai(const char *sai_name) {
    GBDATA *gb_extended = GBT_find_SAI(gb_main, sai_name);
    if (gb_extended) {
        GBDATA *gb_ali = GB_search(gb_extended, ali_name, GB_FIND);
        if (gb_ali) {
            return GB_delete(gb_ali);
        }
    }
    return NULL; // sai/ali did not exist
}

GB_ERROR AP_pos_var::save_sai(const char *sai_name) {
    GB_ERROR  error       = 0;
    GBDATA   *gb_extended = GBT_find_or_create_SAI(gb_main, sai_name);

    if (!gb_extended) error = GB_await_error();
    else {
        GBDATA *gb_ali     = GB_search(gb_extended, ali_name, GB_DB);
        if (!gb_ali) error = GB_await_error();
        else {
            const char *description =
                GBS_global_string("PVP: Positional Variability by Parsimony: tree '%s' ntaxa %li",
                                  tree_name, treesize/2);

            error = GBT_write_string(gb_ali, "_TYPE", description);
        }

        if (!error) {
            char *data = (char*)calloc(1, (int)ali_len+1);
            int  *sum  = (int*)calloc(sizeof(int), (int)ali_len);

            for (int j=0; j<256 && !error; j++) {                   // get sum of frequencies
                if (frequencies[j]) {
                    for (int i=0; i<ali_len; i++) {
                        sum[i] += frequencies[j][i];
                    }

                    if (j >= 'A' && j <= 'Z') {
                        GBDATA *gb_freq     = GB_search(gb_ali, GBS_global_string("FREQUENCIES/N%c", j), GB_INTS);
                        if (!gb_freq) error = GB_await_error();
                        else    error       = GB_write_ints(gb_freq, frequencies[j], ali_len);
                    }
                }
            }

            if (!error) {
                GBDATA *gb_transi     = GB_search(gb_ali, "FREQUENCIES/TRANSITIONS", GB_INTS);
                if (!gb_transi) error = GB_await_error();
                else    error         = GB_write_ints(gb_transi, transitions, ali_len);
            }
            if (!error) {
                GBDATA *gb_transv     = GB_search(gb_ali, "FREQUENCIES/TRANSVERSIONS", GB_INTS);
                if (!gb_transv) error = GB_await_error();
                else    error         = GB_write_ints(gb_transv, transversions, ali_len);
            }

            if (!error) {
                int    max_categ = 0;
                double logbase   = sqrt(2.0);
                double lnlogbase = log(logbase);
                double b         = .75;
                double max_rate  = 1.0;

                for (int i=0; i<ali_len; i++) {
                    if (sum[i] * 10 <= treesize) {
                        data[i] = '.';
                        continue;
                    }
                    if (transitions[i] == 0) {
                        data[i] = '-';
                        continue;
                    }
                    double rate = transitions[i] / (double)sum[i];
                    if (rate >= b * .95) {
                        rate = b * .95;
                    }
                    rate = -b * log(1-rate/b);
                    if (rate > max_rate) rate = max_rate;
                    rate /= max_rate;       // scaled  1.0 == fast rate
                    // ~0.0 slow rate
                    double dcat = -log(rate)/lnlogbase;
                    int icat = (int)dcat;
                    if (icat > 35) icat = 35;
                    if (icat >= max_categ) max_categ = icat + 1;
                    data[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[icat];
                }

                error = GBT_write_string(gb_ali, "data", data);

                if (!error) {
                    // Generate Categories
                    GBS_strstruct *out = GBS_stropen(1000);
                    for (int i = 0; i<max_categ; i++) {
                        GBS_floatcat(out, pow(1.0/logbase, i));
                        GBS_chrcat(out, ' ');
                    }

                    error = GBT_write_string(gb_ali, "_CATEGORIES", GBS_mempntr(out));
                    GBS_strforget(out);
                }
            }

            free(sum);
            free(data);
        }
    }

    return error;
}



// Calculate the positional variability: window interface
static void AP_calc_pos_var_pars(AW_window *aww) {
    AW_root        *root  = aww->get_root();
    GB_transaction  ta(GLOBAL.gb_main);
    GB_ERROR        error = NULL;

    char *ali_name = GBT_get_default_alignment(GLOBAL.gb_main);
    long  ali_len  = GBT_get_alignment_len(GLOBAL.gb_main, ali_name);

    if (ali_len <= 0) {
        error = "Please select a valid alignment";
        GB_clear_error();
    }
    else {
        arb_progress progress("Calculating positional variability");
        progress.subtitle("Loading Tree");

        // get tree
        GBT_TREE *tree;
        char     *tree_name;
        {
            tree_name = root->awar(AWAR_PVP_TREE)->read_string();
            tree = GBT_read_tree(GLOBAL.gb_main, tree_name, GBT_TREE_NodeFactory());
            if (!tree) {
                error = GB_await_error();
            }
            else {
                GBT_link_tree(tree, GLOBAL.gb_main, true, 0, 0);
            }
        }

        if (!error) {
            progress.subtitle("Counting mutations");

            GB_alignment_type  at       = GBT_get_alignment_type(GLOBAL.gb_main, ali_name);
            int                isdna    = at==GB_AT_DNA || at==GB_AT_RNA;
            char              *sai_name = root->awar(AWAR_PVP_SAI)->read_string();
            AP_pos_var         pv(GLOBAL.gb_main, ali_name, ali_len, isdna, tree_name);

            error             = pv.delete_old_sai(sai_name);
            if (!error) error = pv.retrieve(tree);
            if (!error) error = pv.save_sai(sai_name);

            free(sai_name);
        }

        delete tree;
        free(tree_name);
    }
    free(ali_name);

    if (error) aw_message(error);
}

AW_window *AP_create_pos_var_pars_window(AW_root *root) {
    GB_transaction ta(GLOBAL.gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CSP_BY_PARSIMONY", "Conservation Profile: Parsimony Method");
    aws->load_xfig("cpro/parsimony.fig");

    root->awar_string(AWAR_PVP_SAI, "POS_VAR_BY_PARSIMONY", AW_ROOT_DEFAULT);
    const char *largest_tree = GBT_name_of_largest_tree(GLOBAL.gb_main);

    AW_awar *tree_awar = root->awar_string(AWAR_PVP_TREE, NO_TREE_SELECTED, AW_ROOT_DEFAULT);
    AWT_registerTreeAwarSimple(tree_awar);

    root->awar(AWAR_PVP_TREE)->write_string(largest_tree);

    aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help"); aws->callback(makeHelpCallback("pos_var_pars.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("name");
    aws->create_input_field(AWAR_PVP_SAI);

    aws->at("box");
    awt_create_selection_list_on_sai(GLOBAL.gb_main, aws, AWAR_PVP_SAI, false);

    aws->at("trees");
    awt_create_selection_list_on_trees(GLOBAL.gb_main, aws, AWAR_PVP_TREE, true);

    aws->at("go");
    aws->highlight();
    aws->callback(AP_calc_pos_var_pars);
    aws->create_button("GO", "GO");

    return (AW_window *)aws;
}
