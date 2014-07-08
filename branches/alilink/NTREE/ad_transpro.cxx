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

// -------------------------------------------------------------
//      Realign a dna alignment with a given protein source

enum SyncResult {
    SYNC_UNKNOWN,      // not tested
    SYNC_FOUND,        // found a way to sync
    SYNC_FAILURE,      // could not sync (no translation succeeded)
    SYNC_DATA_MISSING, // could not sync (not enough dna data)
};

static SyncResult synchronizeCodons(const char *proteins, const char *dna, int dna_len, int minCatchUp, int maxCatchUp, int *foundCatchUp, const AWT_allowedCode& initially_allowed_code, AWT_allowedCode& allowed_code_left) {
    /*! called after X's where encountered.
     * attempts to find a dna-part which translates into the given protein-sequence
     * @param proteins wanted translation
     * @param dna dna sequence
     * @param dna_len length of dna
     * @param minCatchUp minimum number of nucleotides to skip (at start of dna)
     * @param maxCatchUp maximum number of nucleotides to skip (at start of dna)
     * @param foundCatchUp result parameter: number of nucleotides skipped
     * @param initially_allowed_code translation codes that might be considered
     * @param allowed_code_left leftover translation codes (after a sync has been found)
     */
    for (int catchUp=minCatchUp; catchUp<=maxCatchUp; ++catchUp) {
        const char      *dna_start    = dna+catchUp;
        AWT_allowedCode  allowed_code = initially_allowed_code;

        for (int p=0; ; p++) {
            char prot = proteins[p];

            if (!prot) { // all proteins were synchronized
                *foundCatchUp = catchUp;
                return SYNC_FOUND;
            }

            if ((dna_start-dna+3)>dna_len) { // not enough DNA
                // higher catchUp's cannot succeed => return
                return SYNC_DATA_MISSING;
            }
            if (!AWT_is_codon(prot, dna_start, allowed_code, allowed_code_left)) break;

            allowed_code  = allowed_code_left; // if synchronized: use left codes as allowed codes!
            dna_start    += 3;
        }
    }

    return SYNC_FAILURE;
}

#define SYNC_LENGTH 4
// every X in amino-alignment, it represents 1 to 3 bases in DNA-Alignment
// SYNC_LENGTH is the # of codons which will be synchronized (ahead!)
// before deciding "X was realigned correctly"

inline bool isGap(char c) { return c == '-' || c == '.'; }

class RealignAttempt {
    AWT_allowedCode allowed_code;

    const char *compressed_dna;
    size_t      compressed_len; // length of compressed_dna

    const char *aligned_protein;

    char   *target_dna;
    size_t  target_len; // wanted target length

    GB_ERROR fail_reason;
    size_t   protein_fail_position;
    size_t   dna_fail_position; // in compressed seq

public:
    RealignAttempt(const AWT_allowedCode& allowed_code_, const char *compressed_dna_, size_t compressed_len_, const char *aligned_protein_, char *target_dna_, size_t target_len_)
        : allowed_code(allowed_code_),
          compressed_dna(compressed_dna_),
          compressed_len(compressed_len_),
          aligned_protein(aligned_protein_),
          target_dna(target_dna_),
          target_len(target_len_),
          fail_reason(NULL),
          protein_fail_position(size_t(-1)),
          dna_fail_position(size_t(-1))
    {}

    const AWT_allowedCode& get_remaining_code() const { return allowed_code; }
    const char *failure() const { return fail_reason; }

    size_t get_protein_fail_position() const { nt_assert(failure()); return protein_fail_position; }
    size_t get_dna_fail_position() const { nt_assert(failure()); return dna_fail_position; }

    GB_ERROR perform() {
        int         x_count = 0;
        const char *x_start = 0;

        const char *compressed_dna_start  = compressed_dna;
        const char *target_dna_start      = target_dna;
        const char *aligned_protein_start = aligned_protein;

        for (;;) {
            char p = *aligned_protein++;
            if (!p) {
                if (x_count) {
                    int off = -(x_count*3);
                    while (compressed_dna[0]) {
                        target_dna[off++] = *compressed_dna++;
                    }
                }
                break;
            }

            if (isGap(p)) {
                target_dna[0] = target_dna[1] = target_dna[2] = p;
                target_dna += 3;
            }
            else if (toupper(p)=='X') { // one X represents 1 to 3 DNAs (normally 1 or 2, but 'NNN' translates to 'X')
                x_start = aligned_protein-1;
                x_count = 1;
                int gap_count = 0;

                for (;;) {
                    char p2 = toupper(aligned_protein[0]);

                    if (p2=='X') {
                        x_count++;
                    }
                    else {
                        if (!isGap(p2)) break;
                        gap_count++;
                    }
                    aligned_protein++;
                }

                int setgap = (x_count+gap_count)*3;
                memset(target_dna, '.', setgap);
                target_dna += setgap;
            }
            else {
                AWT_allowedCode allowed_code_left;

                if (x_count) { // synchronize
                    char protein[SYNC_LENGTH+1];
                    int count;
                    {
                        int off;

                        protein[0] = toupper(p);
                        for (count=1, off=0; count<SYNC_LENGTH; off++) {
                            char p2 = aligned_protein[off];

                            if (!isGap(p2)) {
                                p2 = toupper(p2);
                                if (p2=='X') break; // can't sync X
                                protein[count++] = p2;
                            }
                        }
                    }

                    nt_assert(count>=1);
                    protein[count] = 0;

                    nt_assert(!fail_reason);

                    int        catchUp;
                    SyncResult sync_result = SYNC_UNKNOWN;
                    if (count<SYNC_LENGTH) {
                        int  sync_possibilities         = 0;
                        int *sync_possible_with_catchup = new int[x_count*3+1];
                        int  maxCatchup                 = x_count*3;

                        catchUp = x_count-1;
                        for (;;) {
#define DEST_COMPRESSED_RESTLEN (compressed_len-(compressed_dna-compressed_dna_start))
                            sync_result = synchronizeCodons(protein, compressed_dna, DEST_COMPRESSED_RESTLEN, catchUp+1, maxCatchup, &catchUp, allowed_code, allowed_code_left);
                            if (sync_result != SYNC_FOUND) {
                                break;
                            }
                            sync_possible_with_catchup[sync_possibilities++] = catchUp;
                        }

                        nt_assert(!fail_reason);
                        if (sync_possibilities==0) {
                            fail_reason = sync_result == SYNC_DATA_MISSING ? "not enough dna data" : "no translation found";
                        }
                        else if (sync_possibilities>1) {
                            fail_reason = "multiple realign possibilities found";
                        }
                        else {
                            nt_assert(sync_possibilities==1);
                            catchUp = sync_possible_with_catchup[0];
                        }
                        delete [] sync_possible_with_catchup;
                    }
                    else {
                        sync_result = synchronizeCodons(protein, compressed_dna, DEST_COMPRESSED_RESTLEN, x_count, x_count*3, &catchUp, allowed_code, allowed_code_left);
                        if (sync_result != SYNC_FOUND) {
                            fail_reason = sync_result == SYNC_DATA_MISSING ? "not enough dna data" : "no translation found";
                        }
                    }

                    if (fail_reason) {
                        fail_reason = GBS_global_string("Can't synchronize after 'X' (%s)", fail_reason);
                        break;
                    }

                    allowed_code = allowed_code_left;

                    // copy 'catchUp' characters (they are the content of the found Xs):
                    {
                        const char *after = aligned_protein-1;
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
                                            target_dna[off++] = *compressed_dna++;
                                        }
                                        else {
                                            target_dna[off++] = '.';
                                        }
                                    }
                                    x_rest--;
                                    break;
                                }
                                case '.':
                                case '-':
                                    target_dna[off++] = i[0];
                                    target_dna[off++] = i[0];
                                    target_dna[off++] = i[0];
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
                    if (!AWT_is_codon(p, compressed_dna, allowed_code, allowed_code_left, &why_fail)) {
                        fail_reason = GBS_global_string("Not a codon (%s)", why_fail);
                        break;
                    }

                    allowed_code = allowed_code_left;
                }

                // copy one codon:
                target_dna[0] = compressed_dna[0];
                target_dna[1] = compressed_dna[1];
                target_dna[2] = compressed_dna[2];

                target_dna     += 3;
                compressed_dna += 3;
            }
        }

        if (!failure()) {
            int inserted = target_dna-target_dna_start;
            int rest     = target_len-inserted;

            memset(target_dna, '.', rest);
            target_dna += rest;
            target_dna[0] = 0;
        }

        if (failure()) {
            protein_fail_position = (aligned_protein-1)-aligned_protein_start+1;
            dna_fail_position     = compressed_dna-compressed_dna_start;
        }
        else {
            nt_assert(strlen(target_dna_start) == (unsigned)target_len);
        }

        return failure();
    }
};

class Realigner {
    const char *ali_source;
    const char *ali_dest;

    size_t ali_len;        // of ali_dest
    size_t needed_ali_len; // >ali_len if ali_dest is too short; 0 otherwise

    const char *fail_reason;

    char *unalign(const char *data, size_t len, size_t& compressed_len) {
        // removes gaps; return compressed length
        char *compressed = (char*)malloc(len+1);
        compressed_len        = 0;
        for (size_t p = 0; p<len && data[p]; ++p) {
            if (!isGap(data[p])) {
                compressed[compressed_len++] = data[p];
            }
        }
        compressed[compressed_len] = 0;
        return compressed;
    }

public:
    Realigner(const char *ali_source_, const char *ali_dest_, size_t ali_len_)
        : ali_source(ali_source_),
          ali_dest(ali_dest_),
          ali_len(ali_len_),
          needed_ali_len(0)
    {
        clear_failure();
    }

    size_t get_needed_dest_alilen() const { return needed_ali_len; }

    void set_failure(const char *reason) { fail_reason = reason; }
    void clear_failure() { fail_reason = NULL; }

    const char *failure() const { return fail_reason; }

    char *realign_seq(AWT_allowedCode& allowed_code, const char *const source, size_t source_len, const char *const dest, size_t dest_len) {
        nt_assert(!failure());

        size_t  wanted_ali_len  = source_len*3;
        char   *buffer          = NULL;
        bool    ignore_fail_pos = false;

        if (ali_len<wanted_ali_len) {
            fail_reason     = GBS_global_string("Alignment '%s' is too short (increase its length to %li)", ali_dest, wanted_ali_len);
            ignore_fail_pos = true;

            if (wanted_ali_len>needed_ali_len) needed_ali_len = wanted_ali_len;
        }
        else {
            // compress destination DNA (=remove align-characters):
            size_t  compressed_len;
            char   *compressed_dest = unalign(dest, dest_len, compressed_len);

            buffer = (char*)malloc(ali_len+1);

            RealignAttempt attempt(allowed_code, compressed_dest, compressed_len, source, buffer, ali_len);
            fail_reason = attempt.perform();
            if (!failure()) {
                allowed_code = attempt.get_remaining_code();
            }
            else {
                if (!ignore_fail_pos) {
                    int source_fail_pos = attempt.get_protein_fail_position();
                    int dest_fail_pos = 0;
                    {
                        int fail_d_base_count = attempt.get_dna_fail_position();

                        const char *dp = dest;

                        for (;;) {
                            char c = *dp++;

                            if (!c) {
                                nt_assert(c);
                                break;
                            }
                            if (!isGap(c)) {
                                if (!fail_d_base_count) {
                                    dest_fail_pos = (dp-1)-dest+1;
                                    break;
                                }
                                fail_d_base_count--;
                            }
                        }
                    }

                    fail_reason = GBS_global_string("%s at %s:%i / %s:%i",
                                                    fail_reason,
                                                    ali_source, source_fail_pos,
                                                    ali_dest, dest_fail_pos);
                }
                freenull(buffer);
            }

            free(compressed_dest);
        }

        nt_assert(contradicted(buffer, fail_reason));
        return buffer;
    }
};

struct Data : virtual Noncopyable {
    GBDATA *gb_data;
    char   *data;
    size_t  len;
    char   *error;

    Data(GBDATA *gb_species, const char *aliName)
        : gb_data(NULL),
          data(NULL),
          error(NULL)
    {
        GBDATA *gb_ali = GB_entry(gb_species, aliName);
        if (gb_ali) {
            gb_data = GB_entry(gb_ali, "data");
            if (gb_data) {
                data          = GB_read_string(gb_data);
                if (data) len = GB_read_string_count(gb_data);
                else error    = strdup(GB_await_error());
                return;
            }
        }
        error = GBS_global_string_copy("No data in alignment '%s'", aliName);
    }
    ~Data() {
        free(data);
        free(error);
    }
};


static GB_ERROR realign(GBDATA *gb_main, const char *ali_source, const char *ali_dest, size_t& neededLength) {
    /*! realigns DNA alignment of marked sequences according to their protein alignment
     * @param ali_source protein source alignment
     * @param ali_dest modified DNA alignment
     * @param neededLength result: minimum alignment length needed in ali_dest (if too short) or 0 if long enough
     */
    AP_initialize_codon_tables();

    {
        GBDATA *gb_source = GBT_get_alignment(gb_main, ali_source); if (!gb_source) return "Please select a valid source alignment";
        GBDATA *gb_dest   = GBT_get_alignment(gb_main, ali_dest);   if (!gb_dest)   return "Please select a valid destination alignment";
    }

    if (GBT_get_alignment_type(gb_main, ali_source) != GB_AT_AA)  return "Invalid source alignment type";
    if (GBT_get_alignment_type(gb_main, ali_dest)   != GB_AT_DNA) return "Invalid destination alignment type";

    long ali_len = GBT_get_alignment_len(gb_main, ali_dest);
    nt_assert(ali_len>0);

    GB_ERROR error = 0;

    long no_of_marked_species    = GBT_count_marked_species(gb_main);
    long no_of_realigned_species = 0;

    arb_progress progress("Re-aligner", no_of_marked_species);
    progress.auto_subtitles("Re-aligning species");

    Realigner realigner(ali_source, ali_dest, ali_len);

    for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        realigner.clear_failure();

        Data source(gb_species, ali_source);
        Data dest(gb_species, ali_dest);

        if      (source.error) realigner.set_failure(source.error);
        else if (dest.error)   realigner.set_failure(dest.error);

        if (!realigner.failure()) {
            AWT_allowedCode allowed_code; // default: all translation tables allowed
            {
                int arb_transl_table, codon_start;
                GB_ERROR local_error = AWT_getTranslationInfo(gb_species, arb_transl_table, codon_start);
                if (local_error) {
                    realigner.set_failure(GBS_global_string("Error while reading 'transl_table' (%s)", local_error));
                }
                else if (arb_transl_table >= 0) {
                    // we found a 'transl_table' entry -> restrict used code to the code stored there
                    allowed_code.forbidAllBut(arb_transl_table);
                }
            }

            if (!realigner.failure()) {
                char *buffer = realigner.realign_seq(allowed_code, source.data, source.len, dest.data, dest.len);
                if (buffer) { // re-alignment successful
                    error = GB_write_string(dest.gb_data, buffer);

                    if (!error) {
                        int explicit_table_known = allowed_code.explicit_table();

                        if (explicit_table_known >= 0) { // we know the exact code -> write codon_start and transl_table
                            const int codon_start  = 1; // by definition (after realignment)
                            error = AWT_saveTranslationInfo(gb_species, explicit_table_known, codon_start);
                        }
                        else { // we dont know the exact code -> delete codon_start and transl_table
                            UNCOVERED();
                            error = AWT_removeTranslationInfo(gb_species);
                        }
                    }
                    free(buffer);
                }
            }
        }

        if (realigner.failure()) {
            nt_assert(!error);
            GB_warning(GBS_global_string("Automatic re-align failed for '%s'\nReason: %s",
                                         GBT_read_name(gb_species), realigner.failure()));
        }

        progress.inc_and_check_user_abort(error);
        no_of_realigned_species++;
    }

    neededLength = realigner.get_needed_dest_alilen();

    if (!error) {
        int not_realigned = no_of_marked_species - no_of_realigned_species;
        if (not_realigned>0) {
            // case should no longer happen!
            UNCOVERED();
            GB_warning(GBS_global_string("Did not try to realign %i species (unknown reason - please report)", not_realigned));
            progress.inc_by(not_realigned);
        }
    }
    else {
        progress.done();
    }

    if (!error) error = GBT_check_data(gb_main,ali_dest);

    return error;
}

#undef SYNC_LENGTH


static void transdna_event(AW_window *aww) {
    AW_root  *aw_root      = aww->get_root();
    char     *ali_source   = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char     *ali_dest     = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    size_t    neededLength = 0;
    bool      retrying     = false;
    GB_ERROR  error        = 0;

    do {
        GBDATA *gb_main = GLOBAL.gb_main;

        error = GB_begin_transaction(gb_main);
        if (!error) error = realign(gb_main, ali_source, ali_dest, neededLength);
        error = GB_end_transaction(gb_main, error);

        if (!error && neededLength) { // alignment is too short
            if (retrying || !aw_ask_sure("increase_ali_length", GBS_global_string("Increase length of '%s' to %zu?", ali_dest, neededLength))) {
                long destLen = GBT_get_alignment_len(gb_main, ali_dest);
                nt_assert(destLen>0 && size_t(destLen)<neededLength);
                error = GBS_global_string("Missing %li columns in alignment '%s'", size_t(neededLength-destLen), ali_dest);
            }
            else {
                error             = GB_begin_transaction(gb_main);
                if (!error) error = GBT_set_alignment_len(gb_main, ali_dest, neededLength); // @@@ has no effect ? ? why ?
                error             = GB_end_transaction(gb_main, error);

                if (!error) {
                    aw_message(GBS_global_string("Alignment length of '%s' has been set to %li\n"
                                                 "running re-aligner again!",
                                                 ali_dest, neededLength));
                    retrying     = true;
                    neededLength = 0;
                }
            }
        }
    } while (!error && neededLength);

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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <arb_handlers.h>

static std::string msgs;

static void msg_to_string(const char *msg) {
    msgs += msg;
    msgs += '\n';
}

static arb_handlers test_handlers = {
    msg_to_string,
    msg_to_string,
    msg_to_string,
    active_arb_handlers->status,
};

#define DNASEQ(name) GB_read_char_pntr(GBT_read_sequence(GBT_find_species(gb_main, name), "ali_dna"))

void TEST_realign() {
    arb_handlers *old_handlers = active_arb_handlers;
    ARB_install_handlers(test_handlers);

    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_realign.arb", "rw");

    {
        GB_ERROR error;
        size_t   neededLength = 0;

        {
            GB_transaction ta(gb_main);

            msgs  = "";
            error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
            TEST_EXPECT_NO_ERROR(error);
            TEST_EXPECT_EQUAL(msgs,
                              "Automatic re-align failed for 'StrCoel9'\nReason: Not a codon ('TGG' does never translate to 'T' (1)) at ali_pro:17 / ali_dna:76\n"
                              "Automatic re-align failed for 'MucRace3'\nReason: Not a codon ('CTC' does not translate to 'T' (for any of the leftover trans-tables: 0)) at ali_pro:11 / ali_dna:28\n"
                              "Automatic re-align failed for 'AbdGlauc'\nReason: Not a codon ('GTT' does never translate to 'N' (1)) at ali_pro:14 / ali_dna:53\n"
                              "Automatic re-align failed for 'CddAlbic'\nReason: Not a codon ('AAC' does never translate to 'K' (1)) at ali_pro:10 / ali_dna:15\n");

            TEST_EXPECT_EQUAL(DNASEQ("BctFra12"), "ATGGCTAAAGAGAAA---TTTGAACGTACCAAA---CCGCACGTAAACATTGGTACA---ATCGGTCACGTTGACCACGGTAAAACCACTTTGACTGCTGCTATCACTACTGTGTTG.........");
            TEST_EXPECT_EQUAL(DNASEQ("CytLyti6"), "A..TGGCAAAGGAAACTTTTGATCGTTCCAAACCGCACTTAA---ATATAG---GTACTATTGGACACGTAGATCACGGTAAAACTACTTTAACTGCTGCTATTACAACAGTAT......TG....");
            TEST_EXPECT_EQUAL(DNASEQ("TaxOcell"), "AT.GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT.........G..");
            TEST_EXPECT_EQUAL(DNASEQ("StrRamo3"), "ATGTCCAAGACGGCATACGTGCGCACCAAACCGCATCTGAACATCGGCACGATGGGTCATGTCGACCACGGCAAGACCACGTTGACCGCCGCCATCACCAAGGTC.........CTC.........");
            TEST_EXPECT_EQUAL(DNASEQ("StrCoel9"), "------------------------------------ATGTCCAAGACGGCGTACGTCCGCCCACCTGAGGCACGATGGCCCGACCACGGCAAGACCACCCTGACCGCCGCCATCACCAAGGTCCTC"); // @@@ fails (see above)
            TEST_EXPECT_EQUAL(DNASEQ("MucRacem"), "......ATGGGTAAAGAG---------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGTATA......AA.");
            TEST_EXPECT_EQUAL(DNASEQ("MucRace2"), "ATGGGTAAGGAG---------------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGTATA......AA.");
            TEST_EXPECT_EQUAL(DNASEQ("MucRace3"), "-----------ATGGGTAAAGAGAAGACTCACGTTAACGTTGTCGTTATTGGTCACGTCGATTCCGGTAAGTCCACCACCACTGGTCACTTGATTTACAAGTGTGGTGGTATAAA-----------"); // @@@ fails
            TEST_EXPECT_EQUAL(DNASEQ("AbdGlauc"), "----------------------ATGGGTAAAGAAAAGACTCACGTTAACGTCGTTGTCATTGGTCACGTCGATTCTGGTAAATCCACCACCACTGGTCATTTGATCTACAAGTGCGGTGGTATAAA"); // @@@ fails
            TEST_EXPECT_EQUAL(DNASEQ("CddAlbic"), "ATGGGTAAAGAAAAAACTCACGTTAACGTTGTTGTTATTGGTCACGTCGATTCCGGTAAATCTACTACCACCGGTCACTTAATTTACAAGTGTGGTGGTATAAA----------------------"); // @@@ fails
        }
        // -----------------------------
        //      provoke some errors

        GBDATA *gb_TaxOcell;
        {
            GB_transaction ta(gb_main);

            gb_TaxOcell = GBT_find_species(gb_main, "TaxOcell");
            TEST_REJECT_NULL(gb_TaxOcell);

            // unmark all but gb_TaxOcell
            for (GBDATA *gbd = GBT_first_marked_species(gb_main); gbd; gbd = GBT_next_marked_species(gbd)) {
                if (gbd != gb_TaxOcell) GB_write_flag(gbd, 0);
            }
        }

        // wrong alignment type
        {
            GB_transaction ta(gb_main);
            msgs  = "";
            TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
            error = realign(gb_main, "ali_dna", "ali_pro", neededLength);
            TEST_EXPECT_ERROR_CONTAINS(error, "Invalid source alignment type");
            TEST_EXPECT_EQUAL(msgs, "");
            ta.close("aborted");
        }

        // document some existing behavior
        {
            GB_transaction ta(gb_main);

            GBDATA *gb_TaxOcell_amino = GBT_read_sequence(gb_TaxOcell, "ali_pro");
            GBDATA *gb_TaxOcell_dna   = GBT_read_sequence(gb_TaxOcell, "ali_dna");
            TEST_REJECT_NULL(gb_TaxOcell_amino);
            TEST_REJECT_NULL(gb_TaxOcell_dna);

            struct realign_check {
                const char *seq;
                const char *result;
            };

            realign_check seq[] = {
                //"XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-.." // original aa sequence
                // { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "sdfjlksdjf" }, // templ
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "AT.GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT.........G.." }, // original (@@@ why is the final 'G' so far to the right end?)
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHG---..", "AT.GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGT---------......" }, // missing some AA at right end -> @@@ DNA gets truncated (should be appended)
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYH-----..", "AT.GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCAC---------------......" }, // same
                // { "---SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "---------AGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT.........G.." }, // missing some AA at left end (@@@ should work similar to AA missing at right end)
                { 0, 0 }
            };

            int arb_transl_table, codon_start;
            TEST_EXPECT_NO_ERROR(AWT_getTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));

            TEST_EXPECT_EQUAL(arb_transl_table, 14);
            TEST_EXPECT_EQUAL(codon_start, 1);

            for (int s = 0; seq[s].seq; ++s) {
                TEST_ANNOTATE(GBS_global_string("s=%i", s));
                TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, seq[s].seq));
                msgs  = "";
                TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
                error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
                TEST_EXPECT_NO_ERROR(error);
                TEST_EXPECT_EQUAL(msgs, "");
                TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_TaxOcell_dna), seq[s].result);

                TEST_EXPECT_NO_ERROR(AWT_saveTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));
            }

            ta.close("aborted");
        }

        { // workaround until #493 is fixed
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL__BROKEN(GBT_count_marked_species(gb_main), 1, 0);
            GB_write_flag(gb_TaxOcell, 1);
        }

        // write some aa sequences provoking failures
        {
            GB_transaction ta(gb_main);

            GBDATA *gb_TaxOcell_amino = GBT_read_sequence(gb_TaxOcell, "ali_pro");
            TEST_REJECT_NULL(gb_TaxOcell_amino);

            struct realign_fail {
                const char *seq;
                const char *failure;
            };

#define ERRPREFIX     "Automatic re-align failed for 'TaxOcell'\nReason: "
#define ERRPREFIX_LEN 49

            realign_fail seq[] = {
                //"XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-.." // original aa sequence
                // { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "sdfjlksdjf" }, // templ
                { "XG*SNFXXXXXAXNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Can't synchronize after 'X' (multiple realign possibilities found) at ali_pro:12 / ali_dna:19\n" }, // @@@ should be fixed with #419
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Alignment 'ali_dna' is too short (increase its length to 252)\n" },
                { "XG*SNFWPVQAARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Can't synchronize after 'X' (no translation found) at ali_pro:25 / ali_dna:61\n" },
                { "XG*SNX-A-X-ARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Can't synchronize after 'X' (no translation found) at ali_pro:8 / ali_dna:16\n" },
                { "XG*SXFXPXQAXRNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Not a codon ('TAA' does not translate to '*' (no trans-table left)) at ali_pro:3 / ali_dna:7\n" },
                { "---SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Not a codon ('ATG' does never translate to 'S' (1)) at ali_pro:4 / ali_dna:1\n" }, // missing some AA at left end (@@@ see commented test in realign_check above)
                { "XG*SNFWPVQAARNHRHD-----GPRQNDSDRCYHHGAX-..", "Not a codon ('CGG' does never translate to 'G' (1)) at ali_pro:24 / ali_dna:61\n" }, // ok to fail (some AA missing in the middle)
                { "XG*SNFWPVQAARNHRHDRSRGPRQNDSDRCYHHGAXHHGA.", "Can't synchronize after 'X' (not enough dna data) at ali_pro:38 / ali_dna:124\n" }, // too many AA
                { 0, 0 }
            };

            int arb_transl_table, codon_start;
            TEST_EXPECT_NO_ERROR(AWT_getTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));

            TEST_EXPECT_EQUAL(arb_transl_table, 14);
            TEST_EXPECT_EQUAL(codon_start, 1);

            for (int s = 0; seq[s].seq; ++s) {
                TEST_ANNOTATE(GBS_global_string("s=%i", s));
                TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, seq[s].seq));
                msgs  = "";
                TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
                error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
                TEST_EXPECT_NO_ERROR(error);
                TEST_EXPECT_CONTAINS(msgs, ERRPREFIX);
                TEST_EXPECT_EQUAL(msgs.c_str()+ERRPREFIX_LEN, seq[s].failure);

                TEST_EXPECT_NO_ERROR(AWT_saveTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));
            }

            ta.close("aborted");

#undef ERRPREFIX
#undef ERRPREFIX_LEN
        }

        { // workaround until #493 is fixed
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL__BROKEN(GBT_count_marked_species(gb_main), 1, 0);
            GB_write_flag(gb_TaxOcell, 1);
        }


        // invalid translation info
        {
            GB_transaction ta(gb_main);

            GBDATA *gb_trans_table = GB_entry(gb_TaxOcell, "transl_table");
            TEST_EXPECT_NO_ERROR(GB_write_string(gb_trans_table, "666")); // evil translation table

            msgs  = "";
            TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
            error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
            TEST_EXPECT_NO_ERROR(error);
            TEST_EXPECT_EQUAL(msgs, "Automatic re-align failed for 'TaxOcell'\nReason: Error while reading 'transl_table' (Illegal (or unsupported) value (666) in 'transl_table' (item='TaxOcell'))\n");
            ta.close("aborted");
        }

        { // workaround until #493 is fixed
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL__BROKEN(GBT_count_marked_species(gb_main), 1, 0);
            GB_write_flag(gb_TaxOcell, 1);
        }

        // source/dest alignment missing
        for (int i = 0; i<2; ++i) {
            TEST_ANNOTATE(GBS_global_string("i=%i", i));

            {
                GB_transaction ta(gb_main);

                GBDATA *gb_ali = GB_get_father(GBT_read_sequence(gb_TaxOcell, i ? "ali_pro" : "ali_dna"));
                GB_push_my_security(gb_main);
                TEST_EXPECT_NO_ERROR(GB_delete(gb_ali));
                GB_pop_my_security(gb_main);

                msgs  = "";
                TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
                error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
                TEST_EXPECT_NO_ERROR(error);
                if (i) {
                    TEST_EXPECT_EQUAL(msgs, "Automatic re-align failed for 'TaxOcell'\nReason: No data in alignment 'ali_pro'\n");
                }
                else {
                    TEST_EXPECT_EQUAL(msgs, "Automatic re-align failed for 'TaxOcell'\nReason: No data in alignment 'ali_dna'\n");
                }
                ta.close("aborted");
            }

            { // workaround until #493 is fixed
                GB_transaction ta(gb_main);
                TEST_EXPECT_EQUAL__BROKEN(GBT_count_marked_species(gb_main), 1, 0);
                GB_write_flag(gb_TaxOcell, 1);
            }
        }
    }

    GB_close(gb_main);
    ARB_install_handlers(*old_handlers);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
