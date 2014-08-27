// =============================================================== //
//                                                                 //
//   File      : ad_transpro.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <TranslateRealign.h>
#include <awt_sel_boxes.hxx>
#include <AP_codon_table.hxx>
#include <AP_pro_a_nucs.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_defs.h>


#define AWAR_TRANSPRO_PREFIX "transpro/"
#define AWAR_TRANSPRO_SOURCE AWAR_TRANSPRO_PREFIX "source"
#define AWAR_TRANSPRO_DEST   AWAR_TRANSPRO_PREFIX "dest"

// translator only:
#define AWAR_TRANSPRO_POS    AWAR_TRANSPRO_PREFIX "pos" // [0..N-1]
#define AWAR_TRANSPRO_MODE   AWAR_TRANSPRO_PREFIX "mode"
#define AWAR_TRANSPRO_XSTART AWAR_TRANSPRO_PREFIX "xstart"
#define AWAR_TRANSPRO_WRITE  AWAR_TRANSPRO_PREFIX "write"

// realigner only:
#define AWAR_REALIGN_INCALI  AWAR_TRANSPRO_PREFIX "incali"
#define AWAR_REALIGN_UNMARK  AWAR_TRANSPRO_PREFIX "unmark"
#define AWAR_REALIGN_CUTOFF  "tmp/" AWAR_TRANSPRO_PREFIX "cutoff" // dangerous -> do not save

static void transpro_event(AW_window *aww) {
    GB_ERROR error = GB_begin_transaction(GLOBAL.gb_main);
    if (!error) {
#if defined(DEBUG) && 0
        test_AWT_get_codons();
#endif
        AW_root *aw_root       = aww->get_root();
        char    *ali_source    = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
        char    *ali_dest      = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
        char    *mode          = aw_root->awar(AWAR_TRANSPRO_MODE)->read_string();
        int      startpos      = aw_root->awar(AWAR_TRANSPRO_POS)->read_int();
        bool     save2fields   = aw_root->awar(AWAR_TRANSPRO_WRITE)->read_int();
        bool     translate_all = aw_root->awar(AWAR_TRANSPRO_XSTART)->read_int();

        error             = ALI_translate_marked(GLOBAL.gb_main, strcmp(mode, "fields") == 0, save2fields, startpos, translate_all, ali_source, ali_dest);
        if (!error) error = GBT_check_data(GLOBAL.gb_main, 0);

        free(mode);
        free(ali_dest);
        free(ali_source);
    }
    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
}

static void nt_trans_cursorpos_changed(AW_root *awr) {
    int pos = bio2info(awr->awar(AWAR_CURSOR_POSITION)->read_int());
    pos = pos % 3;
    awr->awar(AWAR_TRANSPRO_POS)->write_int(pos);
}

AW_window *NT_create_dna_2_pro_window(AW_root *root) {
    GB_transaction ta(GLOBAL.gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "TRANSLATE_DNA_TO_PRO", "TRANSLATE DNA TO PRO");

    aws->load_xfig("transpro.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("translate_dna_2_pro.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("source");
    awt_create_ALI_selection_list(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_SOURCE, "dna=:rna=");

    aws->at("dest");
    awt_create_ALI_selection_list(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_DEST, "pro=:ami=");

    root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, GLOBAL.gb_main);
    aws->at("table");
    aws->create_option_menu(AWAR_PROTEIN_TYPE, true);
    for (int code_nr=0; code_nr<AWT_CODON_TABLES; code_nr++) {
        aws->insert_option(AWT_get_codon_code_name(code_nr), "", code_nr);
    }
    aws->update_option_menu();

    aws->at("mode");
    aws->create_toggle_field(AWAR_TRANSPRO_MODE, 0, "");
    aws->insert_toggle("from fields 'codon_start' and 'transl_table'", "", "fields");
    aws->insert_default_toggle("use settings below (same for all species):", "", "settings");
    aws->update_toggle_field();

    aws->at("pos");
    aws->create_option_menu(AWAR_TRANSPRO_POS, true);
    for (int p = 1; p <= 3; ++p) {
        char label[2] = { char(p+'0'), 0 };
        aws->insert_option(label, label, bio2info(p));
    }
    aws->update_option_menu();
    aws->get_root()->awar_int(AWAR_CURSOR_POSITION)->add_callback(nt_trans_cursorpos_changed);

    aws->at("write");
    aws->label("Save settings (to 'codon_start'+'transl_table')");
    aws->create_toggle(AWAR_TRANSPRO_WRITE);

    aws->at("start");
    aws->label("Translate all data");
    aws->create_toggle(AWAR_TRANSPRO_XSTART);

    aws->at("translate");
    aws->callback(transpro_event);
    aws->highlight();
    aws->create_button("TRANSLATE", "TRANSLATE", "T");

    aws->window_fit();

    return aws;
}

static void realign_event(AW_window *aww) {
    AW_root  *aw_root          = aww->get_root();
    char     *ali_source       = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char     *ali_dest         = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    bool      unmark_succeeded = aw_root->awar(AWAR_REALIGN_UNMARK)->read_int();
    bool      cutoff_dna       = aw_root->awar(AWAR_REALIGN_CUTOFF)->read_int();
    size_t    neededLength     = 0;
    GBDATA   *gb_main          = GLOBAL.gb_main;
    GB_ERROR  error            = ALI_realign_marked(gb_main, ali_source, ali_dest, neededLength, unmark_succeeded, cutoff_dna);

    if (!error && neededLength) {
        bool auto_inc_alisize = aw_root->awar(AWAR_REALIGN_INCALI)->read_int();
        if (auto_inc_alisize) {
            {
                GB_transaction ta(gb_main);
                error = ta.close(GBT_set_alignment_len(gb_main, ali_dest, neededLength));
            }
            if (!error) {
                aw_message(GBS_global_string("Alignment length of '%s' has been set to %li\n"
                                             "running re-aligner again!",
                                             ali_dest, neededLength));

                error = ALI_realign_marked(gb_main, ali_source, ali_dest, neededLength, unmark_succeeded, cutoff_dna);
                if (neededLength) {
                    error = GBS_global_string("internal error: neededLength=%zu (after autoinc)", neededLength);
                }
            }
        }
        else {
            GB_transaction ta(gb_main);
            long           destLen = GBT_get_alignment_len(gb_main, ali_dest);
            nt_assert(destLen>0 && size_t(destLen)<neededLength);
            error = GBS_global_string("Missing %li columns in alignment '%s' (got=%li, need=%zu)\n"
                                      "(check toggle to permit auto-increment)",
                                      size_t(neededLength-destLen), ali_dest, destLen, neededLength);
        }
    }

    if (error) aw_message(error);
    free(ali_dest);
    free(ali_source);
}

AW_window *NT_create_realign_dna_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "REALIGN_DNA", "Realign DNA");

    aws->load_xfig("realign_dna.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("realign_dna.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("source"); awt_create_ALI_selection_list(GLOBAL.gb_main, aws, AWAR_TRANSPRO_SOURCE, "dna=:rna=");
    aws->at("dest");   awt_create_ALI_selection_list(GLOBAL.gb_main, aws, AWAR_TRANSPRO_DEST,   "pro=:ami=");

    aws->at("autolen"); aws->create_toggle(AWAR_REALIGN_INCALI);
    aws->at("unmark");  aws->create_toggle(AWAR_REALIGN_UNMARK);
    aws->at("cutoff");  aws->create_toggle(AWAR_REALIGN_CUTOFF);

    aws->at("realign");
    aws->callback(realign_event);
    aws->create_autosize_button("REALIGN", "Realign marked species", "R");

    return aws;
}


void NT_create_transpro_variables(AW_root *root, AW_default props) {
    root->awar_string(AWAR_TRANSPRO_SOURCE, "",         props);
    root->awar_string(AWAR_TRANSPRO_DEST,   "",         props);
    root->awar_string(AWAR_TRANSPRO_MODE,   "settings", props);

    root->awar_int(AWAR_TRANSPRO_POS,    0, props);
    root->awar_int(AWAR_TRANSPRO_XSTART, 1, props);
    root->awar_int(AWAR_TRANSPRO_WRITE,  0, props);
    root->awar_int(AWAR_REALIGN_INCALI,  0, props);
    root->awar_int(AWAR_REALIGN_UNMARK,  0, props);
    root->awar_int(AWAR_REALIGN_CUTOFF,  0, props);
}

