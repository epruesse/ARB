#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_awars.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_pro_a_nucs.hxx>
#include <awt_codon_table.hxx>

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

extern GBDATA *gb_main;

static int awt_pro_a_nucs_convert(char *data, long size, int pos, bool translate_all)
    // if translate_all == true -> 'pos' > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)
    // returns the translated protein sequence in 'data'
{
    nt_assert(pos >= 0 && pos <= 2);

    for (char *p = data; *p ; p++) {
        char c = *p;
        if ((c>='a') && (c<='z')) c = c+'A'-'a';
        if (c=='U') c = 'T';
        *p = c;
    }

    char buffer[4];
    buffer[3] = 0;

    char *dest  = data;

    if (pos && translate_all) {
        for (char *p = data; p<data+pos; ++p) {
            char c = *p;
            if (c!='.' && c!='-') { // found a nucleotide
                *dest++ = 'X';
                break;
            }
        }
    }

    int  stops = 0;
    long i     = pos;
    
    for (char *p = data+pos; i+2<size; p+=3,i+=3) {
        buffer[0] = p[0];
        buffer[1] = p[1];
        buffer[2] = p[2];
        int spro  = (int)GBS_read_hash(awt_pro_a_nucs->t2i_hash,buffer);
        int C;
        if (!spro) {
            C = 'X';
        }
        else {
            if (spro == '*') stops++;
            C = spro;
            if (spro == 's') C = 'S';
        }
        *(dest++) = (char)C;
    }
    dest[0] = 0;
    return stops;
}

static GB_ERROR findTranslationTable(GBDATA *gb_species, int& arb_transl_table) {
    // looks for a sub-entry 'transl_table' of 'gb_species'
    // if found -> test for validity and translate from EMBL to ARB table number
    // returns: an error in case of problems
    // 'arb_transl_table' is set to -1 if not found, otherwise it contains the arb table number

    arb_transl_table          = -1; // not found yet
    GB_ERROR  error           = 0;
    GBDATA   *gb_transl_table = GB_find(gb_species, "transl_table", 0, down_level);

    if (gb_transl_table) {
        int embl_table   = atoi(GB_read_char_pntr(gb_transl_table));
        arb_transl_table = AWT_embl_transl_table_2_arb_code_nr(embl_table);
        if (arb_transl_table == -1) { // ill. table
            const char *name    = "<unnamed>";
            GBDATA     *gb_name = GB_find(gb_species, "name", 0, down_level);
            if (gb_name) name = GB_read_char_pntr(gb_name);

            error = GBS_global_string("Illegal (or unsupported) value (%i) in 'transl_table' (species=%s)", embl_table, name);
        }
    }

    return error;
}

static GB_ERROR arb_r2a(GBDATA *gbmain, bool use_entries, bool save_entries, int selected_startpos,
                        bool    translate_all, const char *ali_source, const char *ali_dest)
{
    // if use_entries   == true -> use fields 'codon_start' and 'transl_table' for translation
    //                           (selected_startpos and AWAR_PROTEIN_TYPE are only used if one or both fields are missing)
    // if use_entries   == false -> always use selected_startpos and AWAR_PROTEIN_TYPE
    // if translate_all == true -> a selected_startpos > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)

    nt_assert(selected_startpos >= 0 && selected_startpos < 3);

    GBDATA   *gb_source;
    GBDATA   *gb_dest;
    GBDATA   *gb_species;
    GBDATA   *gb_source_data;
    GBDATA   *gb_dest_data;
    GB_ERROR  error = 0;
    char     *data;
    int       count = 0;
    int       stops = 0;

    gb_source = GBT_get_alignment(gbmain,ali_source);
    if (!gb_source) return "Please select a valid source alignment";
    gb_dest = GBT_get_alignment(gbmain,ali_dest);
    if (!gb_dest) {
        const char *msg = GBS_global_string("You have not selected a destination alingment\n"
                                            "May I create one ('%s_pro') for you?",ali_source);
        if (aw_message(msg,"CREATE,CANCEL")){
            return "Cancelled";
        }

        long slen = GBT_get_alignment_len(gbmain,ali_source);
        ali_dest  = GBS_global_string_copy("%s_pro",ali_source);
        gb_dest   = GBT_create_alignment(gbmain,ali_dest,slen/3+1,0,1,"ami");

        {
            char *fname = GBS_global_string_copy("%s/data",ali_dest);
            awt_add_new_changekey(gbmain,fname,GB_STRING);
            free(fname);
        }

        if (!gb_dest){
            aw_closestatus();
            return GB_get_error();
        }
    }

    aw_openstatus("Translating");

    int spec_count           = GBT_count_species(gbmain);
    int spec_i               = 0;
    int spec_no_transl_table = 0;
    int spec_no_codon_start  = 0;

    bool table_used[AWT_CODON_TABLES];
    memset(table_used, 0, sizeof(table_used));
    int  selected_ttable = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE); // read selected table

    if (use_entries) {
        for (gb_species = GBT_first_marked_species(gbmain);
             gb_species && !error;
             gb_species = GBT_next_marked_species(gb_species) )
        {
            int arb_table;
            error = findTranslationTable(gb_species, arb_table);

            if (!error) {
                if (arb_table == -1) arb_table = 0; // no transl_table entry -> default to standard code
                table_used[arb_table] = true;
            }

//             GBDATA *gb_transl_table = GB_find(gb_species, "transl_table", 0, down_level);
//             int     arb_table       = 0; // use 'Standard Code' if no 'transl_table' entry was found

//             if (gb_transl_table) {
//                 int embl_table = atoi(GB_read_char_pntr(gb_transl_table));
//                 arb_table  = AWT_embl_transl_table_2_arb_code_nr(embl_table);

//                 if (arb_table == -1) {
//                     GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
//                     return GB_export_error("Illegal (or unsupported) value for 'transl_table' in '%s'", GB_read_char_pntr(gb_name));
//                 }
//             }

//             table_used[arb_table] = true;
        }
    }
    else {
        table_used[selected_ttable] = true; // and mark it used
    }

    for (int table = 0; table<AWT_CODON_TABLES && !error; ++table) {
        if (!table_used[table]) continue;

        GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, table); // set wanted protein table
        awt_pro_a_nucs_convert_init(gb_main); // (re-)initialize codon tables for current translation table

        for (   gb_species = GBT_first_marked_species(gbmain);
                gb_species && !error;
                gb_species = GBT_next_marked_species(gb_species) )
        {
            bool found_table_entry = false;
            bool found_start_entry = false;
            int  startpos          = selected_startpos;

            if (use_entries) {  // if entries are used, test if field 'transl_table' matches current table
                GBDATA *gb_transl_table = GB_find(gb_species, "transl_table", 0, down_level);
                int     sp_arb_table    = selected_ttable; // use selected translation table as default (if 'transl_table' field is missing)

                if (gb_transl_table) {
                    int sp_embl_table = atoi(GB_read_char_pntr(gb_transl_table));
                    sp_arb_table      = AWT_embl_transl_table_2_arb_code_nr(sp_embl_table);
                    found_table_entry = true;

                    nt_assert(sp_arb_table != -1); // sp_arb_table must be a valid code (or error should occur above)
                }

                if (sp_arb_table != table) continue; // species has not current transl_table

                if (!gb_transl_table) {
                    ++spec_no_transl_table; // count species w/o transl_table entry
                }

                GBDATA *gb_codon_start = GB_find(gb_species, "codon_start", 0, down_level);
                int     sp_codon_start = selected_startpos+1; // default codon startpos (if 'codon_start' field is missing)

                if (gb_codon_start) {
                    sp_codon_start = atoi(GB_read_char_pntr(gb_codon_start));
                    if (sp_codon_start<1 || sp_codon_start>3) {
                        GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
                        error = GB_export_error("'%s' has invalid codon_start entry %i (allowed: 1..3)",
                                                GB_read_char_pntr(gb_name), sp_codon_start);
                        break;
                    }
                    found_start_entry = true;
                }
                else {
                    ++spec_no_codon_start;
                }
                startpos = sp_codon_start-1; // internal value is 0..2
            }

            if (aw_status(double(spec_i++)/double(spec_count))) {
                error = "Aborted";
                break;
            }
            gb_source = GB_find(gb_species,ali_source,0,down_level);
            if (!gb_source) continue;
            gb_source_data = GB_find(gb_source,"data",0,down_level);
            if (!gb_source_data) continue;
            data = GB_read_string(gb_source_data);
            if (!data) {
                GB_print_error(); // cannot read data (ignore species)
                continue;
            }

            stops += awt_pro_a_nucs_convert(data, GB_read_string_count(gb_source_data), startpos, translate_all); // do the translation

            count ++;
            gb_dest_data = GBT_add_data(gb_species,ali_dest,"data", GB_STRING);
            if (!gb_dest_data) return GB_get_error();
            error        = GB_write_string(gb_dest_data,data);
            free(data);

            if (!error && save_entries) {
                if (!found_table_entry || !use_entries) {
                    GBDATA *gb_transl_table = GB_search(gb_species, "transl_table", GB_STRING);
                    if (!gb_transl_table) {
                        error = GB_get_error();
                    }
                    else {
                        GB_write_string(gb_transl_table, GBS_global_string("%i", AWT_arb_code_nr_2_embl_transl_table(selected_ttable)));
                    }
                }
                if (!error && (!found_start_entry || !use_entries)) {
                    GBDATA *gb_start_pos = GB_search(gb_species, "codon_start", GB_STRING);
                    if (!gb_start_pos) {
                        error = GB_get_error();
                    }
                    else {
                        GB_write_string(gb_start_pos, GBS_global_string("%i", startpos+1));
                    }
                }
            }
        }
    }

    GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, selected_ttable); // restore old value

    aw_closestatus();
    if (!error) {
        if (use_entries) { // use 'transl_table' and 'codon_start' fields ?
            if (spec_no_transl_table) {
                aw_message(GBS_global_string("%i taxa had no 'transl_table' field (defaulted to %i)",
                                             spec_no_transl_table, selected_ttable));
            }
            if (spec_no_codon_start) {
                aw_message(GBS_global_string("%i taxa had no 'codon_start' field (defaulted to %i)",
                                             spec_no_codon_start, selected_startpos));
            }
            if ((spec_no_codon_start+spec_no_transl_table) == 0) { // all entries were present
                aw_message("codon_start and transl_table entries found for all taxa");
            }
            else if (save_entries) {
                aw_message("The defaults have been written into 'transl_table' and 'codon_start' fields.");
            }
        }

        aw_message(GBS_global_string("%i taxa converted\n  %f stops per sequence found",
                                     count, (double)stops/(double)count));
    }
    return error;
}

#define AWAR_TRANSPRO_PREFIX "transpro/"
#define AWAR_TRANSPRO_SOURCE AWAR_TRANSPRO_PREFIX "source"
#define AWAR_TRANSPRO_DEST   AWAR_TRANSPRO_PREFIX "dest"
#define AWAR_TRANSPRO_POS    AWAR_TRANSPRO_PREFIX "pos"
#define AWAR_TRANSPRO_MODE   AWAR_TRANSPRO_PREFIX "mode"
#define AWAR_TRANSPRO_XSTART AWAR_TRANSPRO_PREFIX "xstart"
#define AWAR_TRANSPRO_WRITE  AWAR_TRANSPRO_PREFIX "write"

void transpro_event(AW_window *aww){
    AW_root *aw_root = aww->get_root();
    GB_ERROR error;
    GB_begin_transaction(gb_main);

#if defined(DEBUG) && 0
    test_AWT_get_codons();
#endif
    awt_pro_a_nucs_convert_init(gb_main);

    char *ali_source    = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    char *ali_dest      = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char *mode          = aw_root->awar(AWAR_TRANSPRO_MODE)->read_string();
    int   startpos      = aw_root->awar(AWAR_TRANSPRO_POS)->read_int();
    bool  save2fields   = aw_root->awar(AWAR_TRANSPRO_WRITE)->read_int();
    bool  translate_all = aw_root->awar(AWAR_TRANSPRO_XSTART)->read_int();

    error = arb_r2a(gb_main, strcmp(mode, "fields") == 0, save2fields, startpos, translate_all, ali_source, ali_dest);
    if (error) {
        GB_abort_transaction(gb_main);
        aw_message(error);
    }else{
        GBT_check_data(gb_main,0);
        GB_commit_transaction(gb_main);
    }

    free(mode);
    free(ali_dest);
    free(ali_source);
}

void nt_trans_cursorpos_changed(AW_root *awr) {
    int pos = awr->awar(AWAR_CURSOR_POSITION)->read_int()-1;
    pos = pos %3;
    awr->awar(AWAR_TRANSPRO_POS)->write_int(pos);
}

AW_window *NT_create_dna_2_pro_window(AW_root *root) {
    AWUSE(root);
    GB_transaction dummy(gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "TRANSLATE_DNA_TO_PRO", "TRANSLATE DNA TO PRO");

    //     aws->auto_off();

    aws->load_xfig("transpro.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"translate_dna_2_pro.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("source");
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws, AWAR_TRANSPRO_SOURCE,"dna=:rna=");

    aws->at("dest");
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws, AWAR_TRANSPRO_DEST,"pro=:ami=");

    root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, gb_main);
    aws->at("table");
    aws->create_option_menu(AWAR_PROTEIN_TYPE);
    for (int code_nr=0; code_nr<AWT_CODON_TABLES; code_nr++) {
        aws->insert_option(AWT_get_codon_code_name(code_nr), "", code_nr);
    }
    aws->update_option_menu();

    aws->at("mode");
    aws->create_toggle_field(AWAR_TRANSPRO_MODE,0,"");
    aws->insert_toggle( "from fields 'codon_start' and 'transl_table'", "", "fields" );
    aws->insert_default_toggle( "use settings below (same for all species):", "", "settings" );
    aws->update_toggle_field();

    aws->at("pos");
    aws->create_option_menu(AWAR_TRANSPRO_POS,0,"");
    aws->insert_option( "1", "1", 0 );
    aws->insert_option( "2", "2", 1 );
    aws->insert_option( "3", "3", 2 );
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
    aws->create_button("TRANSLATE","TRANSLATE","T");

    aws->window_fit();

    return (AW_window *)aws;
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
// SYNC_LENGTH is the # of codons which will be syncronized (ahead!)
// before deciding "X was realigned correctly"

GB_ERROR arb_transdna(GBDATA *gbmain, char *ali_source, char *ali_dest, long *neededLength)
{
    AWT_initialize_codon_tables();

    GBDATA *gb_source = GBT_get_alignment(gbmain,ali_source);           if (!gb_source) return "Please select a valid source alignment";
    GBDATA *gb_dest = GBT_get_alignment(gbmain,ali_dest);           if (!gb_dest) return "Please select a valid destination alignment";

    long     ali_len            = GBT_get_alignment_len(gbmain,ali_dest);
    long     max_wanted_ali_len = 0;
    GB_ERROR error              = 0;

    aw_openstatus("Re-aligner");
    int no_of_marked_species = GBT_count_marked_species(gbmain);
    int no_of_realigned_species = 0;
    int ignore_fail_pos = 0;

    for (GBDATA *gb_species = GBT_first_marked_species(gbmain);
         !error && gb_species;
         gb_species = GBT_next_marked_species(gb_species) ) {

        {
            char stat[200];

            sprintf(stat, "Re-aligning #%i of %i ...", no_of_realigned_species+1, no_of_marked_species);
            aw_status(stat);
        }

        gb_source = GB_find(gb_species,ali_source,0,down_level);    if (!gb_source) continue;
        GBDATA *gb_source_data = GB_find(gb_source,"data",0,down_level);if (!gb_source_data) continue;
        gb_dest = GB_find(gb_species,ali_dest,0,down_level);        if (!gb_dest) continue;
        GBDATA *gb_dest_data = GB_find(gb_dest,"data",0,down_level);    if (!gb_dest_data) continue;

        char *source = GB_read_string(gb_source_data);          if (!source) { GB_print_error(); continue; }
        char *dest = GB_read_string(gb_dest_data);          if (!dest) { GB_print_error(); continue; }

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

        AWT_allowedCode allowed_code; // default = all allowed

        if (!failed) {
            int arb_transl_table;
            GB_ERROR local_error = findTranslationTable(gb_species, arb_transl_table);
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
                            for (count=1,off=0; count<SYNC_LENGTH; off++) {
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
                                delete sync_possible_with_catchup;
                                failed = 1;
                                fail_reason = "Can't syncronize after 'X'";
                                break;
                            }
                            if (sync_possibilities>1) {
                                delete sync_possible_with_catchup;
                                failed = 1;
                                fail_reason = "Not enough data behind 'X' (please contact ARB-Support)";
                                break;
                            }

                            nt_assert(sync_possibilities==1);
                            catchUp = sync_possible_with_catchup[0];
                            delete sync_possible_with_catchup;
                        }
                        else if (!synchronizeCodons(protein, d, x_count, x_count*3, &catchUp, allowed_code, allowed_code_left)) {
                            failed = 1;
                            fail_reason = "Can't syncronize after 'X'";
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
                char *name = GBT_read_name(gb_species);
                if (!name) name = strdup("(unknown species)");

                char *dup_fail_reason = strdup(fail_reason); // otherwise it may be destroyed by GBS_global_string
                aw_message(GBS_global_string("Automatic re-align failed for '%s'", name));

                if (ignore_fail_pos) {
                    aw_message(GBS_global_string("Reason: %s", dup_fail_reason));
                }
                else {
                    aw_message(GBS_global_string("Reason: %s at %s:%i / %s:%i", dup_fail_reason, ali_source, source_fail_pos, ali_dest, dest_fail_pos));
                }

                free(dup_fail_reason);
                free(name);
            }

        }
        else {
            nt_assert(strlen(buffer) == (unsigned)ali_len);
            
            // re-alignment sucessfull
            error = GB_write_string(gb_dest_data,buffer);
            if (!error) {
                GBDATA *gb_codon_start = GB_find(gb_species, "codon_start", 0, down_level);
                if (gb_codon_start) {
                    // overwrite existing 'codon_start' entries
                    error = GB_write_string(gb_codon_start, "1"); // after re-alignment codon_start is always 1
                }
            }
        }

        free(buffer);
        free(compressed_dest);
        free(dest);
        free(source);

        no_of_realigned_species++;
        GB_status(double(no_of_realigned_species)/double(no_of_marked_species));
    }
    aw_closestatus();

    if (max_wanted_ali_len>0) {
        if (neededLength) *neededLength = max_wanted_ali_len;
    }

    if (error) {
        return error;
    }

    error = GBT_check_data(gbmain,ali_dest);

    return error;
}

#undef SYNC_LENGTH


void transdna_event(AW_window *aww)
{
    AW_root  *aw_root      = aww->get_root();
    GB_ERROR  error;
    char     *ali_source   = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char     *ali_dest     = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    long      neededLength = 0;
    bool      retrying     = false;

    retry :
    GB_begin_transaction(gb_main);

    error = arb_transdna(gb_main,ali_source,ali_dest, &neededLength);
    if (error) {
        GB_abort_transaction(gb_main);
        aw_message(error,"OK");
    }else{
        // GBT_check_data(gb_main,ali_dest); // done by arb_transdna()
        GB_commit_transaction(gb_main);
    }

    if (!retrying && neededLength) {
        if (aw_message(GBS_global_string("Increase length of '%s' to %li?", ali_dest, neededLength), "Yes,No") == 0) {
            GB_transaction dummy(gb_main);
            GBT_set_alignment_len(gb_main, ali_dest, neededLength); // @@@ has no effect ? ? why ?
            aw_message(GBS_global_string("Alignment length of '%s' set to %li\nrunning re-aligner again!", ali_dest, neededLength));
            retrying = true;
            goto retry;
        }
    }

    free(ali_source);
    free(ali_dest);

}

AW_window *NT_create_realign_dna_window(AW_root *root)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "REALIGN_DNA", "REALIGN DNA");

    aws->load_xfig("transdna.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"realign_dna.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("source");
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws, AWAR_TRANSPRO_SOURCE,"dna=:rna=");
    aws->at("dest");
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws, AWAR_TRANSPRO_DEST,"pro=:ami=");

    aws->at("realign");
    aws->callback(transdna_event);
    aws->highlight();
    aws->create_button("REALIGN","REALIGN","T");

    return (AW_window *)aws;
}


void create_transpro_menus(AW_window *awmm)
{
    awmm->insert_menu_topic("dna_2_pro",    "Translate Nucleic to Amino Acid ...","T","translate_dna_2_pro.hlp",        AWM_PRO,    AW_POPUP, (AW_CL)NT_create_dna_2_pro_window, 0 );
    awmm->insert_menu_topic("realign_dna",  "Realign Nucleic Acid according to Aligned Protein ...","r","realign_dna.hlp",  AWM_PRO,    AW_POPUP, (AW_CL)NT_create_realign_dna_window, 0 );
}

void NT_create_transpro_variables(AW_root *root,AW_default db1)
{
    root->awar_string(AWAR_TRANSPRO_SOURCE, "",         db1);
    root->awar_string(AWAR_TRANSPRO_DEST,   "",         db1);
    root->awar_string(AWAR_TRANSPRO_MODE,   "settings", db1);
    root->awar_int(AWAR_TRANSPRO_POS,    0, db1);
    root->awar_int(AWAR_TRANSPRO_XSTART, 1, db1);
    root->awar_int(AWAR_TRANSPRO_WRITE, 0, db1);
}
