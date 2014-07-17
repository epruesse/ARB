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
                GB_warning(GBS_global_string("%i taxa had no valid translation info (fields 'transl_table' and 'codon_start')\n"
                                             "Defaults (%i and %i) have been used%s.",
                                             spec_no_transl_info,
                                             embl_transl_table, selected_startpos+1,
                                             save_entries ? " and written to DB entries" : ""));
            }
            else { // all entries were present
                GB_warning("codon_start and transl_table entries were found for all translated taxa");
            }
        }

        if (no_data>0) {
            GB_warning(GBS_global_string("%i taxa had no data in '%s'", no_data, ali_source));
        }
        if ((count+no_data) == 0) {
            GB_warning("Please mark species to translate");
        }
        else {
            GB_warning(GBS_global_string("%i taxa converted\n  %f stops per sequence found",
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

class Distributor {
    int xcount;
    int *dist;
    int *left;

    GB_ERROR error;

    void fillFrom(int off) {
        nt_assert(!error);
        nt_assert(off<xcount);

        do {
            int leftX    = xcount-off;
            int leftDNA  = left[off];
            int minLeave = leftX-1;
            int maxLeave = minLeave*3;
            int minTake  = std::max(1, leftDNA-maxLeave);
            int maxTake  = std::min(3, leftDNA-minLeave);

            nt_assert(minTake<=maxTake);

            dist[off]   = minTake;
            left[off+1] = left[off]-dist[off];

            off++;
        } while (off<xcount);

        nt_assert(left[xcount] == 0); // expect correct amount of dna has been used
    }
    bool incAt(int off) {
        nt_assert(!error);
        nt_assert(off<xcount);

        if (dist[off] == 3) {
            return false;
        }

        int leftX    = xcount-off;
        int leftDNA  = left[off];
        int minLeave = leftX-1;
        int maxTake  = std::min(3, leftDNA-minLeave);

        if (dist[off] == maxTake) {
            return false;
        }

        dist[off]++;
        left[off+1]--;
        fillFrom(off+1);
        return true;
    }

public:
    Distributor(int xcount_, int dnacount)
        : xcount(xcount_),
          dist(new int[xcount]),
          left(new int[xcount+1]),
          error(NULL)
    {
        if (dnacount<xcount) {
            error = "not enough nucleotides";
        }
        else if (dnacount>3*xcount) {
            error = "too much nucleotides";
        }
        else {
            left[0] = dnacount;
            fillFrom(0);
        }
    }
    Distributor(const Distributor& other)
        : xcount(other.xcount),
          dist(new int[xcount]),
          left(new int[xcount+1]),
          error(other.error)
    {
        memcpy(dist, other.dist, sizeof(*dist)*xcount);
        memcpy(left, other.left, sizeof(*left)*(xcount+1));
    }
    DECLARE_ASSIGNMENT_OPERATOR(Distributor);
    ~Distributor() {
        delete [] dist;
        delete [] left;
    }

    void reset() { *this = Distributor(xcount, left[0]); }

    int operator[](int off) const {
        nt_assert(!error);
        nt_assert(off>=0 && off<xcount);
        return dist[off];
    }

    int size() const { return xcount; }

    GB_ERROR get_error() const { return error; }

    bool next() {
        for (int incPos = xcount-2; incPos>=0; --incPos) {
            if (incAt(incPos)) return true;
        }
        return false;
    }

    bool mayFailTranslation() const {
        for (int i = 0; i<xcount; ++i) {
            if (dist[i] == 3) return true;
        }
        return false;
    }
    int get_score() const {
        // rates balanced distributions high
        int score = 1;
        for (int i = 0; i<xcount; ++i) {
            score *= dist[i];
        }
        return score;
    }

    bool translates_to_Xs(const char *dna, const AWT_allowedCode& with_code) const {
        bool translates = true;
        int  off        = 0;
        for (int p = 0; translates && p<xcount; off += dist[p++]) {
            if (dist[p] == 3) {
                AWT_allowedCode  left_code;
                const char      *why_fail;

                translates = AWT_is_codon('X', dna+off, with_code, left_code, &why_fail);

                // if (translates) with_code = left_code; // @@@ code-exclusion ignored here (maybe not even happens..)
            }
        }
        return translates;
    }
};

#define SYNC_LENGTH 4
// every X in amino-alignment, it represents 1 to 3 bases in DNA-Alignment
// SYNC_LENGTH is the # of codons which will be synchronized (ahead!)
// before deciding "X was realigned correctly"

inline bool isGap(char c) { return c == '-' || c == '.'; }

const size_t NOPOS = size_t(-1);

class RealignAttempt {
    AWT_allowedCode allowed_code;

    const char *compressed_dna;
    size_t      compressed_len; // length of compressed_dna

    const char *aligned_protein;

    char   *target_dna;
    size_t  target_len; // wanted target length

    GB_ERROR    fail_reason;
    const char *protein_fail_at; // in aligned protein seq
    const char *dna_fail_at;     // in compressed seq

    GB_ERROR distribute_xdata(size_t dna_dist_size, size_t xcount, char *xtarget, const AWT_allowedCode& with_code) {
        /*! distributes 'dna_dist_size' nucs starting at 'compressed_dna'
         * @param xtarget destination buffer (target positions are marked with '!')
         * @param xcount number of X's encountered
         */

        Distributor dist(xcount, dna_dist_size);
        GB_ERROR    error = dist.get_error();
        if (!error) {
            Distributor best(dist);

            while (dist.next()) {
                if (dist.get_score() > best.get_score()) {
                    if (!dist.mayFailTranslation() || best.mayFailTranslation()) {
                        best = dist;
                    }
                }
            }

            if (best.mayFailTranslation()) {
                if (!best.translates_to_Xs(compressed_dna, with_code)) {
                    nt_assert(!error);
                    error = "no translating X-distribution found";
                    dist.reset();
                    do {
                        if (dist.translates_to_Xs(compressed_dna, with_code)) {
                            best  = dist;
                            error = NULL;
                            break;
                        }
                    } while (dist.next());

                    while (dist.next()) {
                        if (dist.get_score() > best.get_score()) {
                            if (dist.translates_to_Xs(compressed_dna, with_code)) {
                                best = dist;
                            }
                        }
                    }
                }
            }

            if (!error) {    // now really distribute nucs
                int   off           = 0;  // offset into compressed_dna
                char *xtarget_start = xtarget;

                for (int x = 0; x<best.size(); ++x) {
                    while (xtarget[0] != '!') {
                        nt_assert(xtarget[1] && xtarget[2]); // buffer overflow
                        xtarget += 3;
                    }

                    switch (dist[x]) {
                        case 1:
                            xtarget[0] = '-';
                            xtarget[1] = compressed_dna[off];
                            xtarget[2] = '-';
                            break;

                        case 2: {
                            enum { UNDECIDED, SPREAD, LEFT, RIGHT } mode = UNDECIDED;

                            bool gap_right   = isGap(xtarget[3]);
                            int  follow_dist = x<best.size()-1 ? dist[x+1] : 0;

                            bool has_gaps_left  = xtarget>xtarget_start && isGap(xtarget[-1]);
                            bool has_gaps_right = gap_right || follow_dist == 1;

                            if (has_gaps_left && has_gaps_right) mode = SPREAD;
                            else if (has_gaps_left)              mode = LEFT;
                            else if (has_gaps_right)             mode = RIGHT;
                            else {
                                bool has_nogaps_left  = xtarget>xtarget_start && !isGap(xtarget[-1]);
                                bool has_nogaps_right = !gap_right && follow_dist == 3;

                                if (has_nogaps_left && has_nogaps_right) mode = SPREAD;
                                else if (has_nogaps_left)                mode = RIGHT;
                                else                                     mode = LEFT;
                            }

                            switch (mode) {
                                case UNDECIDED: nt_assert(0); // fall-through in NDEBUG
                                case SPREAD:
                                    xtarget[0] = compressed_dna[off];
                                    xtarget[1] = '-';
                                    xtarget[2] = compressed_dna[off+1];
                                    break;
                                case LEFT:
                                    xtarget[0] = compressed_dna[off];
                                    xtarget[1] = compressed_dna[off+1];
                                    xtarget[2] = '-';
                                    break;
                                case RIGHT:
                                    xtarget[0] = '-';
                                    xtarget[1] = compressed_dna[off];
                                    xtarget[2] = compressed_dna[off+1];
                                    break;
                            }

                            break;
                        }
                        case 3:
                            xtarget[0] = compressed_dna[off];
                            xtarget[1] = compressed_dna[off+1];
                            xtarget[2] = compressed_dna[off+2];
                            break;

                        default:
                            nt_assert(0);
                            break;
                    }

                    xtarget += 3;
                    off     += dist[x];
                }
            }
        }

        return error;
    }

public:
    RealignAttempt(const AWT_allowedCode& allowed_code_, const char *compressed_dna_, size_t compressed_len_, const char *aligned_protein_, char *target_dna_, size_t target_len_)
        : allowed_code(allowed_code_),
          compressed_dna(compressed_dna_),
          compressed_len(compressed_len_),
          aligned_protein(aligned_protein_),
          target_dna(target_dna_),
          target_len(target_len_),
          fail_reason(NULL),
          protein_fail_at(NULL),
          dna_fail_at(NULL)
    {}

    const AWT_allowedCode& get_remaining_code() const { return allowed_code; }
    const char *failure() const { return fail_reason; }

    const char *get_protein_fail_at() const { nt_assert(failure()); return protein_fail_at; }
    const char *get_dna_fail_at() const { nt_assert(failure()); return dna_fail_at; }

    GB_ERROR perform() {
        int         x_count      = 0;
        char       *x_start      = NULL; // points into target_dna
        const char *x_start_prot = NULL; // points into aligned_protein

        const char *compressed_dna_start = compressed_dna;
        const char *target_dna_start     = target_dna;

        bool complete = false; // set to true, if recursive attempt succeeds

        for (;;) {
            char p = *aligned_protein++;

            if (isGap(p)) {
                target_dna[0] = target_dna[1] = target_dna[2] = p;
                target_dna += 3;
            }
            else if (toupper(p)=='X') { // one X represents 1 to 3 DNAs (normally 1 or 2, but 'NNN' translates to 'X')
                x_start      = target_dna;
                x_start_prot = aligned_protein-1;

                x_count       = 1;
                int gap_count = 0;

                // @@@ perform by loop below!
                target_dna[0] = target_dna[1] = target_dna[2] = '!'; // fill X space with marker
                target_dna += 3;

                for (;;) {
                    char p2 = toupper(aligned_protein[0]);

                    if (p2=='X') {
                        x_count++;
                        target_dna[0] = target_dna[1] = target_dna[2] = '!'; // fill X space with marker
                    }
                    else if (isGap(p2)) {
                        gap_count++;
                        target_dna[0] = target_dna[1] = target_dna[2] = p2; // insert gap
                    }
                    else {
                        break;
                    }
                    aligned_protein++;
                    target_dna += 3;
                }
            }
            else {
                if (x_count) {
                    size_t written_dna     = target_dna-target_dna_start;
                    size_t target_rest_len = target_len-written_dna;

                    size_t compressed_rest_len = compressed_len - (compressed_dna-compressed_dna_start);
                    nt_assert(compressed_rest_len<=compressed_len);
                    nt_assert(strlen(compressed_dna) == compressed_rest_len);

                    size_t min_dna = x_count;
                    size_t max_dna = std::min(x_count*3, int(compressed_rest_len));

                    if (min_dna>max_dna) {
                        fail_reason     = "not enough nucs for X's at sequence end";
                        // @@@ set correct fail position!
                        protein_fail_at = x_start_prot;
                        dna_fail_at     = compressed_dna;
                    }
                    else if (p) {
                        char       *foremost_rest_failure    = NULL;
                        const char *foremost_protein_fail_at = NULL;
                        const char *foremost_dna_fail_at     = NULL;

                        for (size_t x_dna = min_dna; x_dna<=max_dna; ++x_dna) { // prefer low amounts of used dna
                            const char *dna_rest     = compressed_dna      + x_dna;
                            size_t      dna_rest_len = compressed_rest_len - x_dna;

                            nt_assert(strlen(dna_rest) == dna_rest_len);
                            nt_assert(compressed_rest_len>=x_dna);

                            GB_ERROR    rest_failed          = NULL;
                            const char *rest_protein_fail_at = NULL;
                            const char *rest_dna_fail_at     = NULL;

                            RealignAttempt attemptRest(allowed_code, dna_rest, dna_rest_len, aligned_protein-1, target_dna, target_rest_len);
                            rest_failed = attemptRest.perform();

                            if (rest_failed) {
                                rest_protein_fail_at = attemptRest.get_protein_fail_at();
                                rest_dna_fail_at     = attemptRest.get_dna_fail_at();
                            }
                            else { // use first success
                                rest_failed = distribute_xdata(x_dna, x_count, x_start, attemptRest.get_remaining_code());
                                if (rest_failed) {
                                    // calculate dna/protein failure position (use start position of succeeded attempt)
                                    // UNCOVERED(); // @@@ covered, but not used in resulting error message
                                    rest_protein_fail_at = x_start_prot; // @@@ =start of Xs
                                    rest_dna_fail_at     = dna_rest;     // @@@ =start of sync (behind Xs)
                                }
                                else {
                                    allowed_code = attemptRest.get_remaining_code();
                                }
                            }

                            if (rest_failed) {
                                nt_assert(rest_protein_fail_at && rest_dna_fail_at); // failure w/o position

                                // track failure with highest fail position:
                                bool isFarmost = !foremost_rest_failure || foremost_protein_fail_at<rest_protein_fail_at;
                                if (isFarmost) {
                                    freedup(foremost_rest_failure, rest_failed);
                                    foremost_protein_fail_at = rest_protein_fail_at;
                                    foremost_dna_fail_at     = rest_dna_fail_at;
                                }
                            }
                            else { // success
                                freenull(foremost_rest_failure);
                                complete = true;
                                break; // use first success
                            }
                        }

                        if (foremost_rest_failure) {
                            nt_assert(!complete);
                            if (strstr(foremost_rest_failure, "Sync behind 'X'")) { // do not spam repetitive sync-failures
                                fail_reason = GBS_static_string(foremost_rest_failure);
                            }
                            else {
                                fail_reason = GBS_global_string("Sync behind 'X' failed foremost with: %s", foremost_rest_failure);
                            }
                            freenull(foremost_rest_failure);

                            // set failure positions
                            protein_fail_at = foremost_protein_fail_at;
                            dna_fail_at     = foremost_dna_fail_at;
                        }
                        else {
                            nt_assert(complete);
                        }
                    }
                    else {
                        fail_reason = "internal error: no distribution attempted";
                        nt_assert(min_dna>0);
                        for (size_t x_dna = max_dna; x_dna>=min_dna && fail_reason; --x_dna) {     // prefer high amounts of dna
                            fail_reason = distribute_xdata(x_dna, x_count, x_start, allowed_code); // @@@ pass/modify usable codes
                            // @@@ set correct fail position!
                        }
                        // @@@ clear fail position if !fail_reason

                        if (fail_reason) {
                            // @@@ set correct fail position! (do not set aligned_protein or compressed_dna)
                            aligned_protein = x_start_prot+1; // report error at start of X's
                            // compressed_dna should be correct
                        }
                    }

                    if (fail_reason) break;

                    x_count = 0;
                    x_start = NULL;

                    if (complete) break;
                }
                else if (p) {
                    AWT_allowedCode  allowed_code_left;
                    const char      *why_fail;

                    size_t compressed_rest_len = compressed_len - (compressed_dna-compressed_dna_start);
                    if (compressed_rest_len>compressed_len || compressed_rest_len<3) {
                        fail_reason = GBS_global_string("not enough nucs left for codon of '%c'", p);
                    }
                    else {
                        nt_assert(strlen(compressed_dna) == compressed_rest_len);
                        nt_assert(compressed_rest_len >= 3);
                        if (!AWT_is_codon(p, compressed_dna, allowed_code, allowed_code_left, &why_fail)) {
                            fail_reason = why_fail;
                        }
                    }

                    if (fail_reason) break;

                    allowed_code = allowed_code_left;

                    // copy one codon:
                    target_dna[0] = compressed_dna[0];
                    target_dna[1] = compressed_dna[1];
                    target_dna[2] = compressed_dna[2];

                    target_dna     += 3;
                    compressed_dna += 3;
                }

                if (!p) {
                    break; // done
                }
            }
        }

        nt_assert(!x_count || failure());

        if (!failure() && !complete) {
            // append leftover dna-data (data w/o corresponding aa):
            int inserted = target_dna-target_dna_start;
            int rest     = target_len-inserted;

            memset(target_dna, '.', rest);
            target_dna += rest;
            target_dna[0] = 0;
        }

        if (failure()) {
            if (!protein_fail_at) { // if no position set above, use current positions
                protein_fail_at = aligned_protein-1;
                dna_fail_at     = compressed_dna;
            }
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
                    int source_fail_pos = attempt.get_protein_fail_at() - source;
                    int dest_fail_pos = 0;
                    {
                        int fail_d_base_count = attempt.get_dna_fail_at() - compressed_dest;

                        const char *dp = dest;

                        for (;;) {
                            char c = *dp++;

                            if (!c) { // failure at end of sequence
                                // nt_assert(c);
                                // dest_fail_pos = (dp-1)-dest+1 +1; // fake a position after last
                                dest_fail_pos++; // report position behind last non-gap
                                break;
                            }
                            if (!isGap(c)) {
                                dest_fail_pos = (dp-1)-dest;
                                if (!fail_d_base_count) break;
                                fail_d_base_count--;
                            }
                        }
                    }

                    fail_reason = GBS_global_string("%s at %s:%i / %s:%i",
                                                    fail_reason,
                                                    ali_source, info2bio(source_fail_pos),
                                                    ali_dest, info2bio(dest_fail_pos));
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
                            const int codon_start  = 0; // by definition (after realignment)
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

    arb_suppress_progress here;

    {
        GB_ERROR error;
        size_t   neededLength = 0;

        {
            GB_transaction ta(gb_main);

            msgs  = "";
            error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
            TEST_EXPECT_NO_ERROR(error);
            TEST_EXPECT_EQUAL(msgs,
                              "Automatic re-align failed for 'BctFra12'\nReason: not enough nucs for X's at sequence end at ali_pro:40 / ali_dna:109\n" // new correct report (got no nucs for 1 X)
                              "Automatic re-align failed for 'StrRamo3'\nReason: not enough nucs for X's at sequence end at ali_pro:36 / ali_dna:106\n" // new correct report (got 3 nucs for 4 Xs)
                );

            TEST_EXPECT_EQUAL(DNASEQ("BctFra12"),    "ATGGCTAAAGAGAAATTTGAACGTACCAAACCGCACGTAAACATTGGTACAATCGGTCACGTTGACCACGGTAAAACCACTTTGACTGCTGCTATCACTACTGTGTTG------------------"); // now fails as expected => seq unchanged
            TEST_EXPECT_EQUAL(DNASEQ("CytLyti6"),    "-A-TGGCAAAGGAAACTTTTGATCGTTCCAAACCGCACTTAA---ATATAG---GTACTATTGGACACGTAGATCACGGTAAAACTACTTTAACTGCTGCTATTACAACAGTAT-T-----G-..."); // now correctly realigns the 2 trailing X's
            TEST_EXPECT_EQUAL(DNASEQ("TaxOcell"),    "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G----......"); // ok (manually checked)
            TEST_EXPECT_EQUAL(DNASEQ("StrRamo3"),    "ATGTCCAAGACGGCATACGTGCGCACCAAACCGCATCTGAACATCGGCACGATGGGTCATGTCGACCACGGCAAGACCACGTTGACCGCCGCCATCACCAAGGTCCTC------------------"); // now fails as expected => seq unchanged
            TEST_EXPECT_EQUAL(DNASEQ("StrCoel9"),    "ATGTCCAAGACGGCGTACGTCCGCCCA-C--C--T--G--A----GGCACGATG-GC-C--C-GACCACGGCAAGACCACCCTGACCGCCGCCATCACCAAGGTC-C--T--------C-......"); // performed // @@@ does not translate to 6 + 3 Xs (distribution error?)
            TEST_EXPECT_EQUAL(DNASEQ("MucRacem"),    "......ATGGGTAAAGAG---------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGTATA-AA......"); // ok (manually checked)
            TEST_EXPECT_EQUAL(DNASEQ("MucRace2"),    "ATGGGTAAGGAG---------------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGTATA-AA......"); // ok (manually checked)
            TEST_EXPECT_EQUAL(DNASEQ("MucRace3"),    "ATGGGTAAA-GA-G-------------AAGACTCACGTTAACGTTGTCGTTATTGGTCACGTCGATTCCGGTAAGTCCACCACCACTGGTCACTTGATTTACAAGTGTGGTGGTATA-AA......"); // fixed by rewrite (manually checked)
            TEST_EXPECT_EQUAL(DNASEQ("AbdGlauc"),    "ATGGGTAAA-GA-A--A--A--G--A--C--T-CACGTTAACGTCGTTGTCATTGGTCACGTCGATTCTGGTAAATCCACCACCACTGGTCATTTGATCTACAAGTGCGGTGGTATA-AA......"); // fixed by rewrite (manually checked)
            TEST_EXPECT_EQUAL(DNASEQ("CddAlbic"),    "ATGGGT-AA-A-GAA------------AAAACTCACGTTAACGTTGTTGTTATTGGTCACGTCGATTCCGGTAAATCTACTACCACCGGTCACTTAATTTACAAGTGTGGTGGTATA-AA......"); // performed // @@@ but does not translate 3 Xs near start (distribution error?)
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
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G----......" }, // original
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHG---..", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGT---------......" }, // missing some AA at right end -> @@@ DNA gets truncated (should be appended)
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYH-----..", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCAC---------------......" }, // @@@ same

                // { "---SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "---------AGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT.........G.." }, // missing some AA at left end (@@@ should work similar to AA missing at right end. fails: see below)
                { "XG*SNFXXXXXXAXXXNHRHDXXXXXXPRQNDSDRCYHHGAX", "AT-GGCTAAAGAAACTTTTGACCGGTC-C--A--A-GCCGCA-CG-T-AAACATCGGCACGATCGGTCACGT-G--G--A-CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G-" }, // @@@ wrong realignment
                { "XG*SNFWPVQAARNHRHD-XXXXXX-PRQNDSDRCYHHGAX-", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT---CGGTCACGT-G--G--A----CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G----" }, // @@@ wrong realignment
                { "XG*SNXLXRXQA-ARNHRHD-RXXVX-PRQNDSDRCYHHGAX", "AT-GGCTAAAGAAACTT-TTGAC-CGGTC-CAAGCC---GCACGTAAACATCGGCACGAT---CGGTCA-C-GTG-GA---CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G-" }, // @@@ 'CGGTCAC--' -> 'RSX' (want: 'RXX')
                { "XG*SXXFXDXVQAXT*TSARXRSXVX-PRQNDSDRCYHHGAX", "AT-GGCTAAAGA-AA-C-TTT-T-GACCG-GTCCAAGCCGC-ACGTAAACATCGGCACGA-T-CGGTCA-C-GTG-GA---CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G-" }, // @@@ check

                { 0, 0 }
            };

            int arb_transl_table, codon_start;
            TEST_EXPECT_NO_ERROR(AWT_getTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));

            TEST_EXPECT_EQUAL(arb_transl_table, 14);
            TEST_EXPECT_EQUAL(codon_start, 0);

            char *org_dna = GB_read_string(gb_TaxOcell_dna);

            for (int s = 0; seq[s].seq; ++s) {
                TEST_ANNOTATE(GBS_global_string("s=%i", s));
                TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, seq[s].seq));
                msgs  = "";
                TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
                error = realign(gb_main, "ali_pro", "ali_dna", neededLength);
                TEST_EXPECT_NO_ERROR(error);
                TEST_EXPECT_EQUAL(msgs, "");
                TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_TaxOcell_dna), seq[s].result);

                // restore (possibly) changed DB entries
                TEST_EXPECT_NO_ERROR(AWT_saveTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));
                TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_dna, org_dna));
            }

            free(org_dna);
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

            // dna of TaxOcell:
            // "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G----......"

            realign_fail seq[] = {
                //"XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-.." // original aa sequence
                // { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "sdfjlksdjf" }, // templ

                // wanted realign failures:
                { "XG*SNFXXXXXAXNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'TCA' does never translate to 'P' at ali_pro:25 / ali_dna:64\n" },      // ok to fail: 5 Xs impossible
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Alignment 'ali_dna' is too short (increase its length to 252)\n" }, // ok to fail: wrong alignment length
                { "XG*SNFWPVQAARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'TCA' does never translate to 'P' at ali_pro:25 / ali_dna:64\n" },      // ok to fail
                { "XG*SNX-A-X-ARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'TTT' translates to 'F', not to 'A' at ali_pro:8 / ali_dna:17\n" },     // ok to fail
                { "XG*SXFXPXQAXRNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'CAC' translates to 'H', not to 'R' at ali_pro:13 / ali_dna:35\n" },    // ok to fail
                { "XG*SNFWPVQAARNHRHD-----GPRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'CGG' translates to 'R', not to 'G' at ali_pro:24 / ali_dna:61\n" },    // ok to fail: some AA missing in the middle
                { "XG*SNFWPVQAARNHRHDRSRGPRQNDSDRCYHHGAXHHGA.", "Sync behind 'X' failed foremost with: not enough nucs left for codon of 'H' at ali_pro:38 / ali_dna:117\n" }, // ok to fail: too many AA

                // failing realignments that should work:
                { "---SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "'ATG' translates to 'M', not to 'S' at ali_pro:4 / ali_dna:1\n" },                      // @@@ should succeed; missing some AA at left end (@@@ see commented test in realign_check above)

                { 0, 0 }
            };

            int arb_transl_table, codon_start;
            TEST_EXPECT_NO_ERROR(AWT_getTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));

            TEST_EXPECT_EQUAL(arb_transl_table, 14);
            TEST_EXPECT_EQUAL(codon_start, 0);

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

static const char *permOf(const Distributor& dist) {
    const int   MAXDIST = 10;
    static char buffer[MAXDIST+1];

    nt_assert(dist.size() <= MAXDIST);
    for (int p = 0; p<dist.size(); ++p) {
        buffer[p] = '0'+dist[p];
    }
    buffer[dist.size()] = 0;

    return buffer;
}

static arb_test::match_expectation stateOf(Distributor& dist, const char *expected_perm, bool hasNext) {
    using namespace arb_test;

    expectation_group expected;
    expected.add(that(permOf(dist)).is_equal_to(expected_perm));
    expected.add(that(dist.next()).is_equal_to(hasNext));
    return all().ofgroup(expected);
}

void TEST_distributor() {
    TEST_EXPECT_EQUAL(Distributor(3, 2).get_error(), "not enough nucleotides");
    TEST_EXPECT_EQUAL(Distributor(3, 10).get_error(), "too much nucleotides");

    Distributor minDist(3, 3);
    TEST_EXPECTATION(stateOf(minDist, "111", false));
    // TEST_EXPECT_EQUAL(permOf(minDist), "111"); TEST_EXPECT_EQUAL(minDist.next(), false);

    Distributor maxDist(3, 9);
    TEST_EXPECTATION(stateOf(maxDist, "333", false));

    Distributor meanDist(3, 6);
    TEST_EXPECTATION(stateOf(meanDist, "123", true));
    TEST_EXPECTATION(stateOf(meanDist, "132", true));
    TEST_EXPECTATION(stateOf(meanDist, "213", true));
    TEST_EXPECTATION(stateOf(meanDist, "222", true));
    TEST_EXPECTATION(stateOf(meanDist, "231", true));
    TEST_EXPECTATION(stateOf(meanDist, "312", true));
    TEST_EXPECTATION(stateOf(meanDist, "321", false));

    Distributor belowMax(4, 11);
    TEST_EXPECTATION(stateOf(belowMax, "2333", true));
    TEST_EXPECTATION(stateOf(belowMax, "3233", true));
    TEST_EXPECTATION(stateOf(belowMax, "3323", true));
    TEST_EXPECTATION(stateOf(belowMax, "3332", false));

    Distributor aboveMin(4, 6);
    TEST_EXPECTATION(stateOf(aboveMin, "1113", true));
    TEST_EXPECTATION(stateOf(aboveMin, "1122", true));
    TEST_EXPECTATION(stateOf(aboveMin, "1131", true));
    TEST_EXPECTATION(stateOf(aboveMin, "1212", true));
    TEST_EXPECTATION(stateOf(aboveMin, "1221", true));
    TEST_EXPECTATION(stateOf(aboveMin, "1311", true));
    TEST_EXPECTATION(stateOf(aboveMin, "2112", true));
    TEST_EXPECTATION(stateOf(aboveMin, "2121", true));
    TEST_EXPECTATION(stateOf(aboveMin, "2211", true));
    TEST_EXPECTATION(stateOf(aboveMin, "3111", false));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
