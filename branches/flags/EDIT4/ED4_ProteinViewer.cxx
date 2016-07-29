// =======================================================================================
//
//    File       : ED4_ProteinViewer.cxx
//    Purpose    : Protein Viewer: Dynamical translation and display of
//                 aminoacid sequences in the dna alignment.
//    Author     : Yadhu Kumar (yadhu@arb-home.de)
//    web site   : http://www.arb-home.de/
//
//    Copyright Department of Microbiology (Technical University Munich)
//
// =======================================================================================

#include "ed4_ProteinViewer.hxx"
#include "ed4_seq_colors.hxx"
#include "ed4_class.hxx"

#include <AP_pro_a_nucs.hxx>
#include <AP_codon_table.hxx>
#include <Translate.hxx>
#include <aw_question.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>

#include <iostream>

using namespace std;

// Definitions used
#define FORWARD_STRAND         1
#define COMPLEMENTARY_STRAND   2
#define DB_FIELD_STRAND        3

#define FORWARD_STRANDS        3
#define COMPLEMENTARY_STRANDS  3
#define DB_FIELD_STRANDS       1

enum {
    PV_MARKED = 0,
    PV_SELECTED,
    PV_CURSOR,
    PV_ALL
};

// Global Variables
extern GBDATA *GLOBAL_gb_main;

static bool gTerminalsCreated       = false;
static int  PV_AA_Terminals4Species = 0;
static int  gMissingTransTable      = 0;
static int  gMissingCodonStart      = 0;
static bool gbWritingData           = false;
static int  giNewAlignments         = 0;

static AW_repeated_question *ASKtoOverWriteData = 0;

static bool PV_LookForNewTerminals(AW_root *root) {
    bool bTerminalsFound = true;

    int all = root->awar(AWAR_PV_DISPLAY_ALL)->read_int();
    int all_f1 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->read_int();
    int all_f2 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->read_int();
    int all_f3 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->read_int();
    int all_c1  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->read_int();
    int all_c2  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->read_int();
    int all_c3  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->read_int();
    int all_db     = root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();

    int sel = root->awar(AWAR_PV_SELECTED)->read_int();
    int sel_db = root->awar(AWAR_PV_SELECTED_DB)->read_int();
    int sel_f1 = root->awar(AWAR_PV_SELECTED_FS_1)->read_int();
    int sel_f2 = root->awar(AWAR_PV_SELECTED_FS_2)->read_int();
    int sel_f3 = root->awar(AWAR_PV_SELECTED_FS_3)->read_int();
    int sel_c1 = root->awar(AWAR_PV_SELECTED_CS_1)->read_int();
    int sel_c2 = root->awar(AWAR_PV_SELECTED_CS_2)->read_int();
    int sel_c3 = root->awar(AWAR_PV_SELECTED_CS_3)->read_int();

    int mrk = root->awar(AWAR_PV_MARKED)->read_int();
    int mrk_db = root->awar(AWAR_PV_MARKED_DB)->read_int();
    int mrk_f1 = root->awar(AWAR_PV_MARKED_FS_1)->read_int();
    int mrk_f2 = root->awar(AWAR_PV_MARKED_FS_2)->read_int();
    int mrk_f3 = root->awar(AWAR_PV_MARKED_FS_3)->read_int();
    int mrk_c1 = root->awar(AWAR_PV_MARKED_CS_1)->read_int();
    int mrk_c2 = root->awar(AWAR_PV_MARKED_CS_2)->read_int();
    int mrk_c3 = root->awar(AWAR_PV_MARKED_CS_3)->read_int();

    int cur = root->awar(AWAR_PV_CURSOR)->read_int();
    int cur_db = root->awar(AWAR_PV_CURSOR_DB)->read_int();
    int cur_f1 = root->awar(AWAR_PV_CURSOR_FS_1)->read_int();
    int cur_f2 = root->awar(AWAR_PV_CURSOR_FS_2)->read_int();
    int cur_f3 = root->awar(AWAR_PV_CURSOR_FS_3)->read_int();
    int cur_c1 = root->awar(AWAR_PV_CURSOR_CS_1)->read_int();
    int cur_c2 = root->awar(AWAR_PV_CURSOR_CS_2)->read_int();
    int cur_c3 = root->awar(AWAR_PV_CURSOR_CS_3)->read_int();

    // Test whether any of the options are selected or not?
    if ((all && (all_f1 || all_f2 || all_f3 || all_c1 || all_c2 || all_c3 || all_db)) ||
        (sel && (sel_db || sel_f1 || sel_f2 || sel_f3 || sel_c1 || sel_c2 || sel_c3)) ||
        (mrk && (mrk_db || mrk_f1 || mrk_f2 || mrk_f3 || mrk_c1 || mrk_c2 || mrk_c3)) ||
        (cur && (cur_db || cur_f1 || cur_f2 || cur_f3 || cur_c1 || cur_c2 || cur_c3)))
        {
            // if so, then test whether the terminals are created already or not?
            if (gTerminalsCreated) {
                // if yes, then set the flag to true
                bTerminalsFound = true;
            }
            else {
                // if not, then new terminals has to be created
                bTerminalsFound = false;
            }
        }
    return bTerminalsFound;
}

static void PV_HideTerminal(ED4_orf_terminal *orfTerm) {
    ED4_sequence_manager *seqManager = orfTerm->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->hide_children();
}

static void PV_UnHideTerminal(ED4_orf_terminal *orfTerm) {
    ED4_sequence_manager *seqManager = orfTerm->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->unhide_children();
}

static void PV_HideAllTerminals() {
    ED4_terminal *terminal = 0;
    for (terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;
         terminal = terminal->get_next_terminal())
    {
        if (terminal->is_orf_terminal()) { 
            ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
            PV_HideTerminal(orfTerm);
        }
    }
}


static void PV_ManageTerminalDisplay(AW_root *root, ED4_orf_terminal *orfTerm, long int DispMode) {
    int all, af_1, af_2, af_3, ac_1, ac_2, ac_3, adb;
    all =  root->awar(AWAR_PV_DISPLAY_ALL)->read_int();
    adb = root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();
    af_1 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->read_int();
    af_2 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->read_int();
    af_3 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->read_int();
    ac_1 = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->read_int();
    ac_2 = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->read_int();
    ac_3 = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->read_int();

    int mrk, mf_1, mf_2, mf_3, mc_1, mc_2, mc_3, mdb;
    mrk = root->awar(AWAR_PV_MARKED)->read_int();
    mdb = root->awar(AWAR_PV_MARKED_DB)->read_int();
    mf_1 = root->awar(AWAR_PV_MARKED_FS_1)->read_int();
    mf_2 = root->awar(AWAR_PV_MARKED_FS_2)->read_int();
    mf_3 = root->awar(AWAR_PV_MARKED_FS_3)->read_int();
    mc_1 = root->awar(AWAR_PV_MARKED_CS_1)->read_int();
    mc_2 = root->awar(AWAR_PV_MARKED_CS_2)->read_int();
    mc_3 = root->awar(AWAR_PV_MARKED_CS_3)->read_int();

    int sel, sf_1, sf_2, sf_3, sc_1, sc_2, sc_3, sdb;
    sel = root->awar(AWAR_PV_SELECTED)->read_int();
    sdb = root->awar(AWAR_PV_SELECTED_DB)->read_int();
    sf_1 = root->awar(AWAR_PV_SELECTED_FS_1)->read_int();
    sf_2 = root->awar(AWAR_PV_SELECTED_FS_2)->read_int();
    sf_3 = root->awar(AWAR_PV_SELECTED_FS_3)->read_int();
    sc_1 = root->awar(AWAR_PV_SELECTED_CS_1)->read_int();
    sc_2 = root->awar(AWAR_PV_SELECTED_CS_2)->read_int();
    sc_3 = root->awar(AWAR_PV_SELECTED_CS_3)->read_int();

    int cur, cf_1, cf_2, cf_3, cc_1, cc_2, cc_3, cdb;
    cur = root->awar(AWAR_PV_CURSOR)->read_int();
    cdb = root->awar(AWAR_PV_CURSOR_DB)->read_int();
    cf_1 = root->awar(AWAR_PV_CURSOR_FS_1)->read_int();
    cf_2 = root->awar(AWAR_PV_CURSOR_FS_2)->read_int();
    cf_3 = root->awar(AWAR_PV_CURSOR_FS_3)->read_int();
    cc_1 = root->awar(AWAR_PV_CURSOR_CS_1)->read_int();
    cc_2 = root->awar(AWAR_PV_CURSOR_CS_2)->read_int();
    cc_3 = root->awar(AWAR_PV_CURSOR_CS_3)->read_int();

    // Get the AA sequence flag - says which strand we are in
    int aaStrandType = int(orfTerm->GET_aaStrandType());
    // Check the display options and make visible or hide the AA seq terminal
    switch (aaStrandType)
        {
        case FORWARD_STRAND:
            {
                int aaStartPos = int(orfTerm->GET_aaStartPos());
                switch (aaStartPos) {
                case 1:
                    switch (DispMode) {
                    case PV_ALL:      (all && af_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mf_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sf_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cf_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                case 2:
                    switch (DispMode) {
                    case PV_ALL:      (all && af_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mf_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sf_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cf_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                case 3:
                    switch (DispMode) {
                    case PV_ALL:      (all && af_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mf_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sf_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cf_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                }
            }
            break;
        case COMPLEMENTARY_STRAND:
            {
                int aaStartPos = int(orfTerm->GET_aaStartPos());
                switch (aaStartPos) {
                case 1:
                    switch (DispMode) {
                    case PV_ALL:      (all && ac_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mc_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sc_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cc_1) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                case 2:
                    switch (DispMode) {
                    case PV_ALL:      (all && ac_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mc_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sc_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cc_2) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                case 3:
                    switch (DispMode) {
                    case PV_ALL:      (all && ac_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_MARKED:   (mrk && mc_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_SELECTED: (sel && sc_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    case PV_CURSOR:   (cur && cc_3) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
                    }
                    break;
                }
            }
            break;
        case DB_FIELD_STRAND:
            switch (DispMode) {
            case PV_ALL:      (all && adb) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
            case PV_MARKED:   (mrk && mdb) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
            case PV_SELECTED: (sel && sdb) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
            case PV_CURSOR:   (cur && cdb) ? PV_UnHideTerminal(orfTerm) : PV_HideTerminal(orfTerm); break;
            }
            break;
        }
}

static void PV_ManageTerminals(AW_root *root) {

    // First Hide all orf Terminals
    PV_HideAllTerminals();

    int dispAll = root->awar(AWAR_PV_DISPLAY_ALL)->read_int();
    if (dispAll) {
        for (ED4_terminal *terminal = ED4_ROOT->root_group_man->get_first_terminal();
             terminal;
             terminal = terminal->get_next_terminal())
        {
            if (terminal->is_orf_terminal()) {
                ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
                PV_ManageTerminalDisplay(root, orfTerm, PV_ALL);
            }
        }
    }

    int dispMarked = root->awar(AWAR_PV_MARKED)->read_int();
    if (dispMarked) {
        GB_transaction ta(GLOBAL_gb_main);
        int marked = GBT_count_marked_species(GLOBAL_gb_main);
        if (marked) {
            for (GBDATA *gbSpecies = GBT_first_marked_species(GLOBAL_gb_main);
                 gbSpecies;
                 gbSpecies = GBT_next_marked_species(gbSpecies))
            {
                ED4_species_name_terminal *spNameTerm = ED4_find_species_name_terminal(GBT_read_name(gbSpecies));
                if (spNameTerm) {
                    ED4_terminal *terminal = spNameTerm->corresponding_sequence_terminal();
                    for (int i=0; i<PV_AA_Terminals4Species; i++) {
                        // get the corresponding orf_terminal skipping sequence_info terminal
                        // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                        terminal = terminal->get_next_terminal()->get_next_terminal();
                        // Make sure it is ORF terminal
                        if (terminal->is_orf_terminal()) {
                            ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
                            PV_ManageTerminalDisplay(root, orfTerm, PV_MARKED);
                        }
                    }
                }
            }
        }
    }

    int dispSelected = root->awar(AWAR_PV_SELECTED)->read_int();
    if (dispSelected) {
        for (ED4_terminal *terminal = ED4_ROOT->root_group_man->get_first_terminal();
             terminal;
             terminal = terminal->get_next_terminal())
        {
            if (terminal->is_sequence_terminal()) {
                ED4_species_manager *speciesManager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
                if (speciesManager->is_species_seq_terminal()) {
                    // we are in the sequence terminal section of a species
                    // walk through all the corresponding ORF terminals for the species and
                    // hide or unhide the terminals based on the display options set by the user
                    if (speciesManager->is_selected()) {
                        for (int i=0; i<PV_AA_Terminals4Species; i++) {
                            // get the corresponding orf_terminal skipping sequence_info terminal
                            // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                            terminal = terminal->get_next_terminal()->get_next_terminal();
                            // Make sure it is ORF terminal
                            if (terminal->is_orf_terminal()) {
                                ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
                                PV_ManageTerminalDisplay(root, orfTerm, PV_SELECTED);
                            }
                        }
                    }
                }
            }
        }
    }

    int dispAtCursor = root->awar(AWAR_PV_CURSOR)->read_int();
    if (dispAtCursor) {
        // only display terminals for species at cursor
        if (ED4_ROOT->get_most_recently_used_window()) {
            ED4_MostRecentWinContext context;
            ED4_cursor&              cursor = current_cursor();

            if (cursor.owner_of_cursor) {
                // Get The Cursor Terminal And The Corresponding Aa_Sequence Terminals And Set The Display Options
                ED4_terminal *cursorTerminal = cursor.owner_of_cursor;
                if (cursorTerminal->is_species_seq_terminal()) {
                    for (int i=0; i<PV_AA_Terminals4Species; i++) {
                        // get the corresponding orf_terminal skipping sequence_info terminal
                        // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                        cursorTerminal = cursorTerminal->get_next_terminal()->get_next_terminal();
                        // Make sure it is ORF terminal
                        if (cursorTerminal->is_orf_terminal()) {
                            ED4_orf_terminal *orfTerm = cursorTerminal->to_orf_terminal();
                            PV_ManageTerminalDisplay(root, orfTerm, PV_CURSOR);
                        }
                    }
                }
            }
        }
    }
}

void PV_RefreshWindow(AW_root *root) {
    // Manage the terminals showing only those selected by the user
    if (gTerminalsCreated) {
        PV_ManageTerminals(root);
    }
    // Refresh all windows
    // ED4_ROOT->refresh_all_windows(0);
}

static GB_ERROR PV_ComplementarySequence(char *sequence) {
    char     T_or_U;
    GB_ERROR error = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "complement");
    if (!error) {
        int   seqLen           = strlen(sequence);
        char *complementarySeq = GBT_complementNucSequence((const char*) sequence, seqLen, T_or_U);

        strcpy(sequence, complementarySeq);
        free(complementarySeq);
    }
    return error;
}

static void PV_WriteTranslatedSequenceToDB(ED4_orf_terminal *aaSeqTerm, const char *spName) {
    GB_ERROR  error      = GB_begin_transaction(GLOBAL_gb_main);
    GBDATA   *gb_species = GBT_find_species(GLOBAL_gb_main, spName);
    if (!gb_species) error = GBS_global_string("Species '%s' does not exist", spName);
    else {
        char *defaultAlignment = GBT_get_default_alignment(GLOBAL_gb_main);
        if (!defaultAlignment) error = GB_await_error();
        else {
            GBDATA *gb_SeqData = GBT_find_sequence(gb_species, defaultAlignment);
            if (!gb_SeqData) {
                error = GB_have_error()
                    ? GB_await_error()
                    : GBS_global_string("Species '%s' has no data in alignment '%s'", spName, defaultAlignment);
            }
            else {
                char *str_SeqData       = GB_read_string(gb_SeqData);
                if (!str_SeqData) error = GB_await_error();
                else {
                    int aaStrandType = int(aaSeqTerm->GET_aaStrandType());
                    if (aaStrandType == COMPLEMENTARY_STRAND) {
                        GB_ERROR compl_err =  PV_ComplementarySequence(str_SeqData);
                        if (compl_err) error = GBS_global_string("Failed to calc complementary strand (Reason: %s)", compl_err);
                    }

                    if (!error) {
                        char newAlignmentName[100];
                        int  aaStartPos = int(aaSeqTerm->GET_aaStartPos());

                        switch (aaStrandType) {
                            case FORWARD_STRAND:       sprintf(newAlignmentName, "ali_pro_ProtView_forward_start_pos_%ld",        (long)aaStartPos); break;
                            case COMPLEMENTARY_STRAND: sprintf(newAlignmentName, "ali_pro_ProtView_complementary_start_pos_%ld",  (long)aaStartPos); break;
                            case DB_FIELD_STRAND:      sprintf(newAlignmentName, "ali_pro_ProtView_database_field_start_pos_%ld", (long)aaStartPos); break;
                        }

                        int len = GB_read_string_count(gb_SeqData);
                        AWT_pro_a_nucs_convert(AWT_default_protein_type(), str_SeqData, len, aaStartPos-1, false, true, false, 0);

                        // Create alignment data to store the translated sequence
                        GBDATA *gb_presets          = GBT_get_presets(GLOBAL_gb_main);
                        GBDATA *gb_alignment_exists = GB_find_string(gb_presets, "alignment_name", newAlignmentName, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
                        GBDATA *gb_new_alignment    = 0;

                        if (gb_alignment_exists) {
                            int     skip_sp     = 0;
                            char   *question    = 0;
                            GBDATA *gb_seq_data = GBT_find_sequence(gb_species, newAlignmentName);
                            if (gb_seq_data) {
                                e4_assert(ASKtoOverWriteData);
                                question = GBS_global_string_copy("\"%s\" contain data in the alignment \"%s\"!", spName, newAlignmentName);
                                skip_sp  = ASKtoOverWriteData->get_answer("overwrite_translated", question, "Overwrite, Skip Species", "all", false);
                            }
                            if (skip_sp) {
                                error = GBS_global_string_copy("%s You chose to skip this Species!", question);
                            }
                            else {
                                gb_new_alignment             = GBT_get_alignment(GLOBAL_gb_main, newAlignmentName);
                                if (!gb_new_alignment) error = GB_await_error();
                            }
                            free(question);
                        }
                        else {
                            long aliLen      = GBT_get_alignment_len(GLOBAL_gb_main, defaultAlignment);
                            gb_new_alignment = GBT_create_alignment(GLOBAL_gb_main, newAlignmentName, aliLen/3+1, 0, 1, "ami");

                            if (!gb_new_alignment) error = GB_await_error();
                            else giNewAlignments++;
                        }

                        if (!error) {
                            GBDATA *gb_ProSeqData     = GBT_add_data(gb_species, newAlignmentName, "data", GB_STRING);
                            if (!gb_ProSeqData) error = GB_await_error();
                            else    error             = GB_write_string(gb_ProSeqData, str_SeqData);
                        }

                        if (!error) error = GBT_check_data(GLOBAL_gb_main, 0);
                    }

                    free(str_SeqData);
                }
            }
            free(defaultAlignment);
        }
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}

static void PV_SaveData(AW_window */*aww*/) {
    // IDEA:
    // 1. walk thru the orf terminals
    // 2. check the visibility status
    // 3. select only the visible terminals
    // 4. get the corresponding species name
    // 5. translate the sequence
    // 6. write to the database
    gbWritingData = true;
    if (gTerminalsCreated) {
        ASKtoOverWriteData = new AW_repeated_question;

        for (ED4_terminal *terminal = ED4_ROOT->root_group_man->get_first_terminal();
             terminal;
             terminal = terminal->get_next_terminal())
        {
            if (terminal->is_species_seq_terminal()) {
                const char *speciesName = terminal->to_sequence_terminal()->species_name;
                
                for (int i=0; i<PV_AA_Terminals4Species; i++) {
                    // get the corresponding orf_terminal skipping sequence_info terminal
                    terminal = terminal->get_next_terminal()->get_next_terminal();
                    // Make sure it is ORF terminal
                    if (terminal->is_orf_terminal()) {
                        ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
                        ED4_base *base = (ED4_base*)orfTerm;
                        if (!base->flag.hidden) {
                            PV_WriteTranslatedSequenceToDB(orfTerm, speciesName);
                        }
                    }
                }
            }
        }
        if (giNewAlignments>0) {
            char *msg = GBS_global_string_copy("Protein data saved to NEW alignments.\n%d new alignments were created and named ali_prot_ProtView_XXXX", giNewAlignments);
            aw_message(msg);
            free(msg);

            giNewAlignments = 0;
        }
    }
    gbWritingData = false;
}

static void TranslateGeneToAminoAcidSequence(AW_root * /* root */, ED4_orf_terminal *seqTerm, const char *speciesName, int startPos4Translation, int translationMode) {
    // This function translates gene sequence to aminoacid sequence and stores translation into the respective AA_Sequence_terminal
    GB_ERROR  error        = NULL;
    GBDATA   *gb_species   = GBT_find_species(GLOBAL_gb_main, speciesName);
    if (!gb_species) error = GBS_global_string("Species '%s' does not exist", speciesName);
    else {
        char *defaultAlignment = GBT_get_default_alignment(GLOBAL_gb_main);
        if (!defaultAlignment) error = GB_await_error();
        else {
            GBDATA *gb_SeqData = GBT_find_sequence(gb_species, defaultAlignment);
            if (!gb_SeqData) {
                error = GB_have_error()
                    ? GB_await_error()
                    : GBS_global_string("Species '%s' has no data in alignment '%s'", speciesName, defaultAlignment);
            }
            else {
                e4_assert(startPos4Translation>=0 && startPos4Translation<=2);

                char *str_SeqData = GB_read_string(gb_SeqData);
                if (!str_SeqData) error = GB_await_error();
                else {
                    int len              = GB_read_string_count(gb_SeqData);
                    int translationTable = AWT_default_protein_type(GLOBAL_gb_main);

                    switch (translationMode) {
                        case FORWARD_STRAND:
                            break;
                            
                        case COMPLEMENTARY_STRAND: {
                            // convert sequence to the complementary sequence
                            GB_ERROR compl_err =  PV_ComplementarySequence(str_SeqData);
                            if (compl_err) error = GBS_global_string("Failed to calc complementary strand for '%s' (Reason: %s)", speciesName, compl_err);
                            break;
                        }
                        case DB_FIELD_STRAND: {
                            // for use database field options - fetch codon start and translation table from the respective species data
                            GBDATA *gb_translTable = GB_entry(gb_species, "transl_table");
                            if (gb_translTable) {
                                int sp_embl_table = atoi(GB_read_char_pntr(gb_translTable));
                                translationTable  = AWT_embl_transl_table_2_arb_code_nr(sp_embl_table);
                            }
                            else {   // use selected translation table as default (if 'transl_table' field is missing)
                                gMissingTransTable++;
                            }
                            GBDATA *gb_codonStart = GB_entry(gb_species, "codon_start");
                            if (gb_codonStart) {
                                startPos4Translation = atoi(GB_read_char_pntr(gb_codonStart))-1;
                                if (startPos4Translation<0 || startPos4Translation>2) {
                                    error = GB_export_errorf("'%s' has invalid codon_start entry %i (allowed: 1..3)", speciesName, startPos4Translation+1);
                                }
                            }
                            else {
                                gMissingCodonStart++;
                                startPos4Translation = 0;
                            }
                            break;
                        }
                    }

                    if (!error) {
                        AWT_pro_a_nucs_convert(translationTable, str_SeqData, len, startPos4Translation, false, true, false, 0); // translate

                        char *s = new char[len+1];
                        int iDisplayMode = ED4_ROOT->aw_root->awar(AWAR_PROTVIEW_DISPLAY_OPTIONS)->read_int();
                        int i, j;

                        if (iDisplayMode == PV_AA_NAME) {
                            char *gc = new char[len+1];

                            for (i=0, j=0; i<len && j<len;) {
                                // start from the start pos of aa sequence
                                for (; i<startPos4Translation; ++i) {
                                    s[i]  = ' ';
                                    gc[i] = ' ';
                                }

                                char        base   = str_SeqData[j++];
                                const char *AAname = 0;
                                
                                if (base>='A' && base<='Z') AAname = AP_get_protein_name(base);
                                else if (base=='*')         AAname = "End";
                                else {
                                    for (int m = 0; m<3 && i<len; m++,i++) {
                                        s[i] = base;
                                        gc[i] = base;
                                    }
                                }
                                if (AAname) {
                                    unsigned nlen = strlen(AAname);
                                    for (unsigned int n = 0; n<nlen && i<len; n++,i++) {
                                        s[i]  = AAname[n];
                                        gc[i] = base;
                                    }
                                }
                            }

                            gc[i] = '\0';
                            seqTerm->SET_aaColor(gc);
                            delete [] gc;
                        }
                        else {
                            int k = startPos4Translation+1;
                            for (i=0, j=0; i<len; i++) {
                                if ((k==i) && (j<len)) {
                                    s[i]  = str_SeqData[j++];
                                    k    += 3;
                                }
                                else s[i] = ' ';
                            }
                            seqTerm->SET_aaColor(NULL);
                        }
                        s[i] = '\0';
                        
                        seqTerm->SET_aaSequence(s);
                        delete [] s;
                    }
                    free(str_SeqData);
                }
            }
        }
        free(defaultAlignment);
    }

    if (error) aw_message(GBS_global_string("Error: %s", error));
}

static void PV_PrintMissingDBentryInformation() {
    if (gMissingCodonStart>0) {
        aw_message(GBS_global_string("WARNING:  'codon start' entry not found in %d of %d species! Using 1st base as codon start...",
                                     gMissingCodonStart,
                                     (int)GBT_count_marked_species(GLOBAL_gb_main)));
        gMissingCodonStart = 0;
    }
    if (gMissingTransTable>0) {
        aw_message(GBS_global_string("WARNING:  'translation table' entry not found in %d of %d species! Using selected translation table  as a default table...",
                                     gMissingTransTable,
                                     (int)GBT_count_marked_species(GLOBAL_gb_main)));
        gMissingTransTable = 0;
    }
}

static void PV_DisplayAminoAcidNames(AW_root *root) {
    GB_transaction ta(GLOBAL_gb_main);

    for (ED4_terminal *terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;
         terminal = terminal->get_next_terminal())
    {
        if (terminal->is_species_seq_terminal()) {
            const char *speciesName = terminal->to_sequence_terminal()->species_name;

            // we are in the sequence terminal section of a species
            // walk through all the corresponding ORF terminals for the species and
            // hide or unhide the terminals based on the display options set by the user
            for (int i=0; i<PV_AA_Terminals4Species; i++) {
                // get the corresponding orf_terminal skipping sequence_info terminal
                terminal = terminal->get_next_terminal()->get_next_terminal();
                // Make sure it is ORF terminal
                if (terminal->is_orf_terminal()) {
                    ED4_orf_terminal *orfTerm = terminal->to_orf_terminal();
                    // we are in ORF terminal
                    int   aaStartPos = int(orfTerm->GET_aaStartPos());
                    int aaStrandType = int(orfTerm->GET_aaStrandType());
                    // retranslate the genesequence and store it to the orf_terminal
                    TranslateGeneToAminoAcidSequence(root, orfTerm, speciesName, aaStartPos-1, aaStrandType);
                }
            }
        }
    }
    // Print missing DB entries
    PV_PrintMissingDBentryInformation();
    PV_RefreshWindow(root);
}

static void PV_RefreshDisplay(AW_root *root) {
    PV_DisplayAminoAcidNames(root);
}

static void PV_RefreshProtViewDisplay(AW_window *aww) {
    if (gTerminalsCreated) {
        PV_RefreshDisplay(aww->get_root());
    }
}

void PV_SequenceUpdate_CB(GB_CB_TYPE gbtype) {
    if (gbtype==GB_CB_CHANGED &&
        gTerminalsCreated &&
        (ED4_ROOT->alignment_type == GB_AT_DNA) &&
        !gbWritingData)
    {
        GB_transaction ta(GLOBAL_gb_main);

        for (ED4_window *win = ED4_ROOT->first_window; win; win = win->next) {
            ED4_cursor& cursor = win->cursor;
            if (cursor.in_species_seq_terminal()) {
                ED4_terminal *cursorTerminal = cursor.owner_of_cursor;
                char         *speciesName    = cursorTerminal->to_sequence_terminal()->species_name;
                for (int i=0; i<PV_AA_Terminals4Species; i++) {
                    // get the corresponding orf_terminal skipping sequence_info terminal
                    // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                    cursorTerminal = cursorTerminal->get_next_terminal()->get_next_terminal();
                    // Make sure it is ORF terminal
                    if (cursorTerminal->is_orf_terminal()) {
                        ED4_orf_terminal *orfTerm = cursorTerminal->to_orf_terminal();
                        // Get the AA sequence flag - says which strand we are in
                        int   aaStartPos = int(orfTerm->GET_aaStartPos());
                        int aaStrandType = int(orfTerm->GET_aaStrandType());
                        // retranslate the genesequence and store it to the orf_terminal
                        TranslateGeneToAminoAcidSequence(ED4_ROOT->aw_root, orfTerm, speciesName, aaStartPos-1, aaStrandType);
                    }
                }
                // Print missing DB entries
                PV_PrintMissingDBentryInformation();
                PV_RefreshWindow(ED4_ROOT->aw_root);
            }
        }
    }
}

static void PV_AddNewAAseqTerminals(ED4_sequence_terminal *seqTerminal, ED4_species_manager *speciesManager) {
    int  translationMode = 0;
    char namebuffer[NAME_BUFFERSIZE];

    for (int i = 0; i<PV_AA_Terminals4Species; i++) {
        int count = 1;
        int startPos = 0;

        sprintf(namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count++);
        ED4_multi_sequence_manager *multiSeqManager = speciesManager->search_spec_child_rek(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
        ED4_sequence_manager *new_SeqManager = new ED4_sequence_manager(namebuffer, 0, 0, 0, 0, multiSeqManager);
        new_SeqManager->set_property(ED4_P_MOVABLE);
        multiSeqManager->children->append_member(new_SeqManager);

        if (i<FORWARD_STRANDS)                              sprintf(namebuffer, "F%d ProteinInfo_Term%ld.%d", i+1, ED4_counter, count++);
        else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS) sprintf(namebuffer, "C%dProteinInfo_Term%ld.%d", (i-FORWARD_STRANDS)+1, ED4_counter, count++);
        else                                                sprintf(namebuffer, "DBProteinInfo_Term%ld.%d", ED4_counter, count++);

        {
            ED4_sequence_info_terminal *new_SeqInfoTerminal = new ED4_sequence_info_terminal(namebuffer, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, new_SeqManager);
            new_SeqInfoTerminal->set_property((ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE));

            ED4_sequence_info_terminal *seqInfoTerminal = speciesManager->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();
            new_SeqInfoTerminal->set_both_links(seqInfoTerminal);
            new_SeqManager->children->append_member(new_SeqInfoTerminal);
        }

        {
            sprintf(namebuffer, "AA_Sequence_Term%ld.%d", ED4_counter, count++);
            ED4_orf_terminal *AA_SeqTerminal = new ED4_orf_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, new_SeqManager);
            AA_SeqTerminal->set_both_links(seqTerminal);

            char *speciesName = seqTerminal->species_name;
            if (i<FORWARD_STRANDS) {
                startPos = i;
                translationMode = FORWARD_STRAND;
            }
            else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS) {
                startPos = i-FORWARD_STRANDS;
                translationMode = COMPLEMENTARY_STRAND;
            }
            else {
                startPos = 0;
                translationMode = DB_FIELD_STRAND;
            }
            TranslateGeneToAminoAcidSequence(ED4_ROOT->aw_root, AA_SeqTerminal, speciesName, startPos, translationMode);
            AA_SeqTerminal->SET_aaSeqFlags(startPos+1, translationMode);
            new_SeqManager->children->append_member(AA_SeqTerminal);
        }

        ED4_counter++;

        new_SeqManager->request_resize();
    }
}

void PV_AddCorrespondingOrfTerminals(ED4_species_name_terminal *spNameTerm) {
    if (gTerminalsCreated && spNameTerm) {
        ED4_sequence_terminal *seqTerminal    = spNameTerm->corresponding_sequence_terminal();
        ED4_species_manager *speciesManager = spNameTerm->get_parent(ED4_L_SPECIES)->to_species_manager();
        PV_AddNewAAseqTerminals(seqTerminal, speciesManager);
        PV_RefreshWindow(ED4_ROOT->aw_root);
    }
}

void PV_AddOrfTerminalsToLoadedSpecies() {
    if (gTerminalsCreated) {
        GB_transaction ta(GLOBAL_gb_main);
        int marked = GBT_count_marked_species(GLOBAL_gb_main);
        if (marked) {
            GBDATA *gbSpecies;
            for (gbSpecies = GBT_first_marked_species(GLOBAL_gb_main);
                 gbSpecies;
                 gbSpecies = GBT_next_marked_species(gbSpecies))
            {
                const char *spName = GBT_read_name(gbSpecies);
                cout<<marked--<<". "<<spName<<endl;
                ED4_species_name_terminal *spNameTerm = ED4_find_species_name_terminal(spName);
                if (spNameTerm) {
                    ED4_terminal *terminal = spNameTerm->corresponding_sequence_terminal();
                    // $$$ If next terminal is species_name terminal => corresponding AA seq terminal doesn't exist ==> create one $$$
                    terminal = terminal->get_next_terminal();
                    if (terminal->is_species_name_terminal() || terminal->is_spacer_terminal())
                    {
                        ED4_sequence_terminal *seqTerminal    = spNameTerm->corresponding_sequence_terminal();
                        ED4_species_manager *speciesManager = spNameTerm->get_parent(ED4_L_SPECIES)->to_species_manager();
                        PV_AddNewAAseqTerminals(seqTerminal, speciesManager);
                    }
                }
            }
            PV_RefreshWindow(ED4_ROOT->aw_root);
        }
    }
}

static void PV_CreateAllTerminals(AW_root *root) {
    // 1. Get the species terminal pointer
    // 2. Append the second terminal
    // 3. resize the multi-species manager
    // 4. refresh all the terminals including new appended terminals

    // Look for already created terminals, if found do nothing, otherwise, create
    // new terminals and set gTerminalsCreated to true
    bool bTerminalsFound = PV_LookForNewTerminals(root);
    // if terminals are already created then do nothing exit the function
    if (bTerminalsFound) return;

    GB_transaction ta(GLOBAL_gb_main);

    // Number of ORF terminals to be created = 3 forward strands + 3 complementary strands + 1 DB field strand
    // totally 7 strands has to be created
    int aaTerminalsToBeCreated = FORWARD_STRANDS + COMPLEMENTARY_STRANDS + DB_FIELD_STRANDS;
    PV_AA_Terminals4Species = aaTerminalsToBeCreated;

    ED4_terminal *terminal = 0;
    for (terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;
         terminal = terminal->get_next_terminal())
    {
        if (terminal->is_sequence_terminal()) {
            ED4_sequence_terminal *seqTerminal = terminal->to_sequence_terminal();
            if (seqTerminal->species_name)
            {
                ED4_species_manager *speciesManager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
                if (speciesManager->is_species_seq_manager()) {
                    PV_AddNewAAseqTerminals(seqTerminal, speciesManager);
                }
            }
        }
    }
    ED4_ROOT->main_manager->request_resize(); // @@@ instead needs to be called whenever adding or deleting PV-terminals

    gTerminalsCreated = true;

    // Print missing DB entries
    PV_PrintMissingDBentryInformation();
}

void PV_CallBackFunction(AW_root *root) {
    // Create New Terminals If Aminoacid Sequence Terminals Are Not Created
    if (!gTerminalsCreated) {
        // AWAR_PROTEIN_TYPE is not initialized (if called from ED4_main before creating proteinViewer window)
        // so, initialize it here
        root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, GLOBAL_gb_main);
        PV_CreateAllTerminals(root);
    }

    PV_RefreshWindow(root);
}

// --------------------------------------------------------------------------------
//        Binding callback function to the AWARS

static void PV_AddCallBacks(AW_root *awr) {

    awr->awar(AWAR_PV_DISPLAY_ALL)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PROTVIEW_DISPLAY_OPTIONS)->add_callback(PV_RefreshDisplay);
    awr->awar(AWAR_SPECIES_NAME)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PV_SELECTED)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_DB)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_FS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_FS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_FS_3)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_CS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_CS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_SELECTED_CS_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PV_MARKED)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_DB)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_FS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_FS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_FS_3)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_CS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_CS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_MARKED_CS_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PV_CURSOR)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_DB)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_FS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_FS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_FS_3)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_CS_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_CS_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PV_CURSOR_CS_3)->add_callback(PV_CallBackFunction);
}

// --------------------------------------------------------------------------------
//        Creating AWARS to be used by the PROTVIEW module

void PV_CreateAwars(AW_root *root, AW_default aw_def) {

    root->awar_int(AWAR_PV_DISPLAY_ALL, 0, aw_def);

    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_1, 0, aw_def);
    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_2, 0, aw_def);
    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_3, 0, aw_def);

    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1, 0, aw_def);
    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2, 0, aw_def);
    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3, 0, aw_def);

    root->awar_int(AWAR_PROTVIEW_DEFINED_FIELDS, 0, aw_def);
    root->awar_int(AWAR_PROTVIEW_DISPLAY_OPTIONS, 0, aw_def);

    root->awar_int(AWAR_PV_SELECTED, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_DB, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_FS_1, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_FS_2, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_FS_3, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_CS_1, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_CS_2, 0, aw_def);
    root->awar_int(AWAR_PV_SELECTED_CS_3, 0, aw_def);

    root->awar_int(AWAR_PV_MARKED, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_DB, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_FS_1, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_FS_2, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_FS_3, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_CS_1, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_CS_2, 0, aw_def);
    root->awar_int(AWAR_PV_MARKED_CS_3, 0, aw_def);

    root->awar_int(AWAR_PV_CURSOR, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_DB, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_FS_1, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_FS_2, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_FS_3, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_CS_1, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_CS_2, 0, aw_def);
    root->awar_int(AWAR_PV_CURSOR_CS_3, 0, aw_def);
}

AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root) {
    GB_transaction ta(GLOBAL_gb_main);

    static AW_window_simple *aws = 0;
    if (aws) return aws;

    aws = new AW_window_simple;

    aws->init(aw_root, "PROTEIN_VIEWER", "Protein Viewer");
    aws->load_xfig("proteinViewer.fig");

    aws->at("refresh");
    aws->callback(PV_RefreshProtViewDisplay);
    aws->button_length(0);
    aws->create_button("REFRESH", "#refresh_text.xpm");

    aws->callback(makeHelpCallback("proteinViewer.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help_text.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#close_text.xpm");

    {
        aw_root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, GLOBAL_gb_main);
        aws->at("table");
        aws->create_option_menu(AWAR_PROTEIN_TYPE, true);
        for (int code_nr=0; code_nr<AWT_CODON_TABLES; code_nr++) {
            aws->insert_option(AWT_get_codon_code_name(code_nr), "", code_nr);
        }
        aws->update_option_menu();

        aws->at("all");
        aws->create_toggle(AWAR_PV_DISPLAY_ALL);

        aws->at("f1");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_1);

        aws->at("f2");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_2);

        aws->at("f3");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_3);

        aws->at("c1");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1);

        aws->at("c2");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2);

        aws->at("c3");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3);

        aws->at("defined");
        aws->create_toggle(AWAR_PROTVIEW_DEFINED_FIELDS);

        aws->at("dispOption");
        aws->create_toggle_field(AWAR_PROTVIEW_DISPLAY_OPTIONS, 0);
        aws->insert_toggle("Single Letter Code", "S", 0);
        aws->insert_toggle("Triple Letter Code", "T", 1);
        aws->insert_toggle("Colored Box", "B", 2);
        aws->update_toggle_field();

        aws->at("colMaps");
        aws->callback(makeCreateWindowCallback(ED4_create_seq_colors_window, ED4_ROOT->sequence_colors));
        aws->button_length(0);
        aws->create_button("COLORMAPS", "#colorMaps.xpm");

        aws->at("colors");
        aws->callback(makeWindowCallback(ED4_popup_gc_window, ED4_ROOT->gc_manager));
        aws->button_length(0);
        aws->create_button("COLORS", "#colors.xpm");

        {
            aws->at("sel"); aws->create_toggle(AWAR_PV_SELECTED);
            aws->at("sf1"); aws->create_toggle(AWAR_PV_SELECTED_FS_1);
            aws->at("sf2"); aws->create_toggle(AWAR_PV_SELECTED_FS_2);
            aws->at("sf3"); aws->create_toggle(AWAR_PV_SELECTED_FS_3);
            aws->at("sc1"); aws->create_toggle(AWAR_PV_SELECTED_CS_1);
            aws->at("sc2"); aws->create_toggle(AWAR_PV_SELECTED_CS_2);
            aws->at("sc3"); aws->create_toggle(AWAR_PV_SELECTED_CS_3);
            aws->at("sdb"); aws->create_toggle(AWAR_PV_SELECTED_DB);

            aws->at("mrk"); aws->create_toggle(AWAR_PV_MARKED);
            aws->at("mf1"); aws->create_toggle(AWAR_PV_MARKED_FS_1);
            aws->at("mf2"); aws->create_toggle(AWAR_PV_MARKED_FS_2);
            aws->at("mf3"); aws->create_toggle(AWAR_PV_MARKED_FS_3);
            aws->at("mc1"); aws->create_toggle(AWAR_PV_MARKED_CS_1);
            aws->at("mc2"); aws->create_toggle(AWAR_PV_MARKED_CS_2);
            aws->at("mc3"); aws->create_toggle(AWAR_PV_MARKED_CS_3);
            aws->at("mdb"); aws->create_toggle(AWAR_PV_MARKED_DB);

            aws->at("cur"); aws->create_toggle(AWAR_PV_CURSOR);
            aws->at("cf1"); aws->create_toggle(AWAR_PV_CURSOR_FS_1);
            aws->at("cf2"); aws->create_toggle(AWAR_PV_CURSOR_FS_2);
            aws->at("cf3"); aws->create_toggle(AWAR_PV_CURSOR_FS_3);
            aws->at("cc1"); aws->create_toggle(AWAR_PV_CURSOR_CS_1);
            aws->at("cc2"); aws->create_toggle(AWAR_PV_CURSOR_CS_2);
            aws->at("cc3"); aws->create_toggle(AWAR_PV_CURSOR_CS_3);
            aws->at("cdb"); aws->create_toggle(AWAR_PV_CURSOR_DB);
        }

        aws->at("save");
        aws->callback(PV_SaveData);
        aws->button_length(5);
        aws->create_button("SAVE", "#save.xpm");
    }

    // binding callback function to the AWARS
    PV_AddCallBacks(aw_root);

    // Create All Terminals
    PV_CreateAllTerminals(aw_root);

    aws->show();

    return (AW_window *)aws;
}

