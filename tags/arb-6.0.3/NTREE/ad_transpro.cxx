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

#include <awt_sel_boxes.hxx>
#include <Translate.hxx>
#include <AP_codon_table.hxx>
#include <AP_pro_a_nucs.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <arbdbt.h>
#include <cctype>
#include <arb_defs.h>

static GB_ERROR arb_r2a(GBDATA *gb_main, bool use_entries, bool save_entries, int selected_startpos,
                        bool    translate_all, const char *ali_source, const char *ali_dest)
{
    // if use_entries   == true -> use fields 'codon_start' and 'transl_table' for translation
    //                           (selected_startpos and AWAR_PROTEIN_TYPE are only used both fields are missing,
    //                            if only one is missing, now an error occurs)
    // if use_entries   == false -> always use selected_startpos and AWAR_PROTEIN_TYPE
    // if translate_all == true -> a selected_startpos > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)

    nt_assert(selected_startpos >= 0 && selected_startpos < 3);

    GB_ERROR  error   = 0;
    char     *to_free = 0;

    // check/create alignments
    {
        GBDATA *gb_source = GBT_get_alignment(gb_main, ali_source);
        if (!gb_source) {
            error = GBS_global_string("No valid source alignment (%s)", GB_await_error());
        }
        else {
            GBDATA *gb_dest = GBT_get_alignment(gb_main, ali_dest);
            if (!gb_dest) {
                GB_clear_error();
                const char *msg = GBS_global_string("You have not selected a destination alignment\n"
                                                    "Shall I create one ('%s_pro') for you?", ali_source);
                if (!aw_ask_sure("create_protein_ali", msg)) {
                    error = "Cancelled by user";
                }
                else {
                    long slen = GBT_get_alignment_len(gb_main, ali_source);
                    to_free   = GBS_global_string_copy("%s_pro", ali_source);
                    ali_dest  = to_free;
                    gb_dest   = GBT_create_alignment(gb_main, ali_dest, slen/3+1, 0, 1, "ami");

                    if (!gb_dest) error = GB_await_error();
                    else {
                        char *fname = GBS_global_string_copy("%s/data", ali_dest);
                        error       = GBT_add_new_changekey(gb_main, fname, GB_STRING);
                        free(fname);
                    }
                }
            }
        }
    }

    int no_data             = 0;  // count species w/o data
    int spec_no_transl_info = 0;  // counts species w/o or with illegal transl_table and/or codon_start
    int count               = 0;  // count translated species
    int stops               = 0;  // count overall stop codons
    int selected_ttable     = -1;

    if (!error) {
        arb_progress progress("Translating", GBT_count_marked_species(gb_main));

        bool table_used[AWT_CODON_TABLES];
        memset(table_used, 0, sizeof(table_used));
        selected_ttable = *GBT_read_int(gb_main, AWAR_PROTEIN_TYPE); // read selected table

        if (use_entries) {
            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species && !error;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                int arb_table, codon_start;
                error = AWT_getTranslationInfo(gb_species, arb_table, codon_start);

                if (!error) {
                    if (arb_table == -1) arb_table = selected_ttable; // no transl_table entry -> default to selected standard code
                    table_used[arb_table] = true;
                }
            }
        }
        else {
            table_used[selected_ttable] = true; // and mark it used
        }

        for (int table = 0; table<AWT_CODON_TABLES && !error; ++table) {
            if (!table_used[table]) continue;

            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species && !error;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                bool found_transl_info = false;
                int  startpos          = selected_startpos;

                if (use_entries) {  // if entries are used, test if field 'transl_table' matches current table
                    int sp_arb_table, sp_codon_start;

                    error = AWT_getTranslationInfo(gb_species, sp_arb_table, sp_codon_start);

                    nt_assert(!error); // should already have been handled after first call to AWT_getTranslationInfo above

                    if (sp_arb_table == -1) { // no table in DB
                        nt_assert(sp_codon_start == -1);    // either both should be defined or none
                        sp_arb_table   = selected_ttable;   // use selected translation table as default (if 'transl_table' field is missing)
                        sp_codon_start = selected_startpos; // use selected codon startpos (if 'codon_start' field is missing)
                    }
                    else {
                        nt_assert(sp_codon_start != -1); // either both should be defined or none
                        found_transl_info = true;
                    }

                    if (sp_arb_table != table) continue; // species has not current transl_table

                    startpos = sp_codon_start;
                }

                GBDATA *gb_source = GB_entry(gb_species, ali_source);
                if (!gb_source) { ++no_data; }
                else {
                    GBDATA *gb_source_data = GB_entry(gb_source, "data");
                    if (!gb_source_data) { ++no_data; }
                    else {
                        char *data = GB_read_string(gb_source_data);
                        if (!data) {
                            GB_print_error(); // cannot read data (ignore species)
                            ++no_data;
                        }
                        else {
                            if (!found_transl_info) ++spec_no_transl_info; // count species with missing info

                            stops += AWT_pro_a_nucs_convert(table, data, GB_read_string_count(gb_source_data), startpos, translate_all, false, false, 0); // do the translation
                            ++count;

                            GBDATA *gb_dest_data     = GBT_add_data(gb_species, ali_dest, "data", GB_STRING);
                            if (!gb_dest_data) error = GB_await_error();
                            else    error            = GB_write_string(gb_dest_data, data);


                            if (!error && save_entries && !found_transl_info) {
                                error = AWT_saveTranslationInfo(gb_species, selected_ttable, startpos);
                            }
                            
                            free(data);
                        }
                    }
                }
                progress.inc_and_check_user_abort(error);
            }
        }
    }

    if (!error) {
        if (use_entries) { // use 'transl_table' and 'codon_start' fields ?
            if (spec_no_transl_info) {
                int embl_transl_table = AWT_arb_code_nr_2_embl_transl_table(selected_ttable);
                aw_message(GBS_global_string("%i taxa had no valid translation info (fields 'transl_table' and 'codon_start')\n"
                                             "Defaults (%i and %i) have been used%s.",
                                             spec_no_transl_info,
                                             embl_transl_table, selected_startpos+1,
                                             save_entries ? " and written to DB entries" : ""));
            }
            else { // all entries were present
                aw_message("codon_start and transl_table entries were found for all translated taxa");
            }
        }

        if (no_data>0) {
            aw_message(GBS_global_string("%i taxa had no data in '%s'", no_data, ali_source));
        }
        if ((count+no_data) == 0) {
            aw_message("Please mark species to translate");
        }
        else {
            aw_message(GBS_global_string("%i taxa converted\n  %f stops per sequence found",
                                         count, (double)stops/(double)count));
        }
    }

    free(to_free);

    return error;
}

#define AWAR_TRANSPRO_PREFIX "transpro/"
#define AWAR_TRANSPRO_SOURCE AWAR_TRANSPRO_PREFIX "source"
#define AWAR_TRANSPRO_DEST   AWAR_TRANSPRO_PREFIX "dest"
#define AWAR_TRANSPRO_POS    AWAR_TRANSPRO_PREFIX "pos" // [0..N-1]
#define AWAR_TRANSPRO_MODE   AWAR_TRANSPRO_PREFIX "mode"
#define AWAR_TRANSPRO_XSTART AWAR_TRANSPRO_PREFIX "xstart"
#define AWAR_TRANSPRO_WRITE  AWAR_TRANSPRO_PREFIX "write"

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

        error             = arb_r2a(GLOBAL.gb_main, strcmp(mode, "fields") == 0, save2fields, startpos, translate_all, ali_source, ali_dest);
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
    awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_SOURCE, "dna=:rna=");

    aws->at("dest");
    awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_DEST, "pro=:ami=");

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

// Realign a dna alignment with a given protein source

static int synchronizeCodons(const char *proteins, const char *dna, int minCatchUp, int maxCatchUp, int *foundCatchUp,
                             const AWT_allowedCode& initially_allowed_code, AWT_allowedCode& allowed_code_left) {

    for (int catchUp=minCatchUp; catchUp<=maxCatchUp; catchUp++) {
        const char *dna_start = dna+catchUp;
        AWT_allowedCode allowed_code;
        allowed_code = initially_allowed_code;

        for (int p=0; ; p++) {
            char prot = proteins[p];

            if (!prot) { // all proteins were synchronized
                *foundCatchUp = catchUp;
                return 1;
            }

            if (!AWT_is_codon(prot, dna_start, allowed_code, allowed_code_left)) break;

            allowed_code = allowed_code_left; // if synchronized: use left codes as allowed codes!
            dna_start += 3;
        }
    }

    return 0;
}

#define SYNC_LENGTH 4
// every X in amino-alignment, it represents 1 to 3 bases in DNA-Alignment
// SYNC_LENGTH is the # of codons which will be synchronized (ahead!)
// before deciding "X was realigned correctly"

static GB_ERROR arb_transdna(GBDATA *gb_main, char *ali_source, char *ali_dest, long *neededLength) {
    AP_initialize_codon_tables();

    GBDATA *gb_source = GBT_get_alignment(gb_main, ali_source); if (!gb_source) return "Please select a valid source alignment";
    GBDATA *gb_dest   = GBT_get_alignment(gb_main, ali_dest);   if (!gb_dest)   return "Please select a valid destination alignment";

    long     ali_len            = GBT_get_alignment_len(gb_main, ali_dest);
    long     max_wanted_ali_len = 0;
    GB_ERROR error              = 0;

    long no_of_marked_species    = GBT_count_marked_species(gb_main);
    long no_of_realigned_species = 0;

    arb_progress progress("Re-aligner", no_of_marked_species);
    progress.auto_subtitles("Re-aligning species");

    int ignore_fail_pos = 0;

    for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        gb_source              = GB_entry(gb_species, ali_source); if (!gb_source)      continue;
        GBDATA *gb_source_data = GB_entry(gb_source,  "data");     if (!gb_source_data) continue;
        gb_dest                = GB_entry(gb_species, ali_dest);   if (!gb_dest)        continue;
        GBDATA *gb_dest_data   = GB_entry(gb_dest,    "data");     if (!gb_dest_data)   continue;

        char *source = GB_read_string(gb_source_data); if (!source) { GB_print_error(); continue; }
        char *dest   = GB_read_string(gb_dest_data);   if (!dest)   { GB_print_error(); continue; }

        long source_len = GB_read_string_count(gb_source_data);
        long dest_len = GB_read_string_count(gb_dest_data);

        // compress destination DNA (=remove align-characters):
        char *compressed_dest = (char*)malloc(dest_len+1);
        {
            char *f = dest;
            char *t = compressed_dest;

            while (1) {
                char c = *f++;
                if (!c) break;
                if (c!='.' && c!='-') *t++ = c;
            }
            *t = 0;
        }

        int failed = 0;
        const char *fail_reason = 0;

        long  wanted_ali_len = source_len*3L;
        char *buffer         = (char*)malloc(ali_len+1);
        if (ali_len<wanted_ali_len) {
            failed          = 1;
            fail_reason     = GBS_global_string("Alignment '%s' is too short (increase its length to %li)", ali_dest, wanted_ali_len);
            ignore_fail_pos = 1;

            if (wanted_ali_len>max_wanted_ali_len) max_wanted_ali_len = wanted_ali_len;
        }

        AWT_allowedCode allowed_code;               // default: all allowed

        if (!failed) {
            int arb_transl_table, codon_start;
            GB_ERROR local_error = AWT_getTranslationInfo(gb_species, arb_transl_table, codon_start);
            if (local_error) {
                failed          = 1;
                fail_reason     = GBS_global_string("Error while reading 'transl_table' (%s)", local_error);
                ignore_fail_pos = 1;
            }
            else if (arb_transl_table >= 0) {
                // we found a 'transl_table' entry -> restrict used code to the code stored there
                allowed_code.forbidAllBut(arb_transl_table);
            }
        }

        char *d = compressed_dest;
        char *s = source;

        if (!failed) {
            char *p = buffer;
            int x_count = 0;
            const char *x_start = 0;

            for (;;) {
                char c = *s++;
                if (!c) {
                    if (x_count) {
                        int off = -(x_count*3);
                        while (d[0]) {
                            p[off++] = *d++;
                        }
                    }
                    break;
                }

                if (c=='.' || c=='-') {
                    p[0] = p[1] = p[2] = c;
                    p += 3;
                }
                else if (toupper(c)=='X') { // one X represents 1 to 3 DNAs
                    x_start = s-1;
                    x_count = 1;
                    int gap_count = 0;

                    for (;;) {
                        char c2 = toupper(s[0]);

                        if (c2=='X') {
                            x_count++;
                        }
                        else {
                            if (c2!='.' && c2!='-') break;
                            gap_count++;
                        }
                        s++;
                    }

                    int setgap = (x_count+gap_count)*3;
                    memset(p, '.', setgap);
                    p += setgap;
                }
                else {
                    AWT_allowedCode allowed_code_left;

                    if (x_count) { // synchronize
                        char protein[SYNC_LENGTH+1];
                        int count;
                        {
                            int off;

                            protein[0] = toupper(c);
                            for (count=1, off=0; count<SYNC_LENGTH; off++) {
                                char c2 = s[off];

                                if (c2!='.' && c2!='-') {
                                    c2 = toupper(c2);
                                    if (c2=='X') break; // can't sync X
                                    protein[count++] = c2;
                                }
                            }
                        }

                        nt_assert(count>=1);
                        protein[count] = 0;

                        int catchUp;
                        if (count<SYNC_LENGTH) {
                            int sync_possibilities = 0;
                            int *sync_possible_with_catchup = new int[x_count*3+1];
                            int maxCatchup = x_count*3;

                            catchUp = x_count-1;
                            for (;;) {
                                if (!synchronizeCodons(protein, d, catchUp+1, maxCatchup, &catchUp, allowed_code, allowed_code_left)) {
                                    break;
                                }
                                sync_possible_with_catchup[sync_possibilities++] = catchUp;
                            }

                            if (sync_possibilities==0) {
                                delete [] sync_possible_with_catchup;
                                failed = 1;
                                fail_reason = "Can't synchronize after 'X'";
                                break;
                            }
                            if (sync_possibilities>1) {
                                delete [] sync_possible_with_catchup;
                                failed = 1;
                                fail_reason = "Not enough data behind 'X' (please contact ARB-Support)";
                                break;
                            }

                            nt_assert(sync_possibilities==1);
                            catchUp = sync_possible_with_catchup[0];
                            delete [] sync_possible_with_catchup;
                        }
                        else if (!synchronizeCodons(protein, d, x_count, x_count*3, &catchUp, allowed_code, allowed_code_left)) {
                            failed = 1;
                            fail_reason = "Can't synchronize after 'X'";
                            break;
                        }

                        allowed_code = allowed_code_left;

                        // copy 'catchUp' characters (they are the content of the found Xs):
                        {
                            const char *after = s-1;
                            const char *i;
                            int off = int(after-x_start);
                            nt_assert(off>=x_count);
                            off = -(off*3);
                            int x_rest = x_count;

                            for (i=x_start; i<after; i++) {
                                switch (i[0]) {
                                    case 'x':
                                    case 'X':
                                        {
                                            int take_per_X = catchUp/x_rest;
                                            int o;
                                            for (o=0; o<3; o++) {
                                                if (o<take_per_X) {
                                                    p[off++] = *d++;
                                                }
                                                else {
                                                    p[off++] = '.';
                                                }
                                            }
                                            x_rest--;
                                            break;
                                        }
                                    case '.':
                                    case '-':
                                        p[off++] = i[0];
                                        p[off++] = i[0];
                                        p[off++] = i[0];
                                        break;
                                    default:
                                        nt_assert(0);
                                        break;
                                }
                            }
                        }
                        x_count = 0;
                    }
                    else {
                        const char *why_fail;
                        if (!AWT_is_codon(c, d, allowed_code, allowed_code_left, &why_fail)) {
                            failed = 1;
                            fail_reason = GBS_global_string("Not a codon (%s)", why_fail);
                            break;
                        }

                        allowed_code = allowed_code_left;
                    }

                    // copy one codon:
                    p[0] = d[0];
                    p[1] = d[1];
                    p[2] = d[2];

                    p += 3;
                    d += 3;
                }
            }

            if (!failed) {
                int len = p-buffer;
                int rest = ali_len-len;

                memset(p, '.', rest);
                p += rest;
                p[0] = 0;
            }
        }

        if (failed) {
            int source_fail_pos = (s-1)-source+1;
            int dest_fail_pos = 0;
            {
                int fail_d_base_count = d-compressed_dest;
                char *dp = dest;

                for (;;) {
                    char c = *dp++;

                    if (!c) {
                        nt_assert(c);
                        break;
                    }
                    if (c!='.' && c!='-') {
                        if (!fail_d_base_count) {
                            dest_fail_pos = (dp-1)-dest+1;
                            break;
                        }
                        fail_d_base_count--;
                    }
                }
            }

            {
                char *dup_fail_reason = strdup(fail_reason); // otherwise it may be destroyed by GBS_global_string
                aw_message(GBS_global_string("Automatic re-align failed for '%s'", GBT_read_name(gb_species)));

                if (ignore_fail_pos) {
                    aw_message(GBS_global_string("Reason: %s", dup_fail_reason));
                }
                else {
                    aw_message(GBS_global_string("Reason: %s at %s:%i / %s:%i", dup_fail_reason, ali_source, source_fail_pos, ali_dest, dest_fail_pos));
                }

                free(dup_fail_reason);
            }

        }
        else {
            nt_assert(strlen(buffer) == (unsigned)ali_len);

            // re-alignment successful
            error = GB_write_string(gb_dest_data, buffer);

            if (!error) {
                int explicit_table_known = allowed_code.explicit_table();

                if (explicit_table_known >= 0) { // we know the exact code -> write codon_start and transl_table
                    const int codon_start  = 1; // by definition (after realignment)
                    error = AWT_saveTranslationInfo(gb_species, explicit_table_known, codon_start);
                }
                else { // we dont know the exact code -> delete codon_start and transl_table
                    error = AWT_removeTranslationInfo(gb_species);
                }
            }
        }

        free(buffer);
        free(compressed_dest);
        free(dest);
        free(source);

        progress.inc_and_check_user_abort(error);
        no_of_realigned_species++;
    }

    *neededLength = max_wanted_ali_len;

    if (!error) {
        int not_realigned = no_of_marked_species - no_of_realigned_species;
        if (not_realigned>0) {
            aw_message(GBS_global_string("Did not try to realign %i species (source/dest alignment missing?)", not_realigned));
        }
    }

    if (!error) error = GBT_check_data(gb_main,ali_dest);

    return error;
}

#undef SYNC_LENGTH


static void transdna_event(AW_window *aww) {
    AW_root  *aw_root      = aww->get_root();
    char     *ali_source   = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char     *ali_dest     = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    long      neededLength = -1;
    bool      retrying     = false;
    GB_ERROR  error        = 0;

    while (!error && neededLength) {
        error = GB_begin_transaction(GLOBAL.gb_main);
        if (!error) error = arb_transdna(GLOBAL.gb_main, ali_source, ali_dest, &neededLength);
        error = GB_end_transaction(GLOBAL.gb_main, error);

        if (!error && neededLength>0) {
            if (retrying || !aw_ask_sure("increase_ali_length", GBS_global_string("Increase length of '%s' to %li?", ali_dest, neededLength))) {
                error = GBS_global_string("Missing %li columns in alignment '%s'", neededLength, ali_dest);
            }
            else {
                error             = GB_begin_transaction(GLOBAL.gb_main);
                if (!error) error = GBT_set_alignment_len(GLOBAL.gb_main, ali_dest, neededLength); // @@@ has no effect ? ? why ?
                error             = GB_end_transaction(GLOBAL.gb_main, error);

                if (!error) {
                    aw_message(GBS_global_string("Alignment length of '%s' has been set to %li\n"
                                                 "running re-aligner again!",
                                                 ali_dest, neededLength));
                    retrying     = true;
                    neededLength = -1;
                }
            }
        }
        else {
            neededLength = 0;
        }
    }

    if (error) aw_message(error);
    free(ali_dest);
    free(ali_source);
}

AW_window *NT_create_realign_dna_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "REALIGN_DNA", "REALIGN DNA");

    aws->load_xfig("transdna.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("realign_dna.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("source");
    awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_SOURCE, "dna=:rna=");
    aws->at("dest");
    awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, AWAR_TRANSPRO_DEST, "pro=:ami=");

    aws->at("realign");
    aws->callback(transdna_event);
    aws->highlight();
    aws->create_button("REALIGN", "REALIGN", "T");

    return aws;
}


void NT_create_transpro_variables(AW_root *root, AW_default db1)
{
    root->awar_string(AWAR_TRANSPRO_SOURCE, "",         db1);
    root->awar_string(AWAR_TRANSPRO_DEST,   "",         db1);
    root->awar_string(AWAR_TRANSPRO_MODE,   "settings", db1);
    root->awar_int(AWAR_TRANSPRO_POS,    0, db1);
    root->awar_int(AWAR_TRANSPRO_XSTART, 1, db1);
    root->awar_int(AWAR_TRANSPRO_WRITE, 0, db1);
}
