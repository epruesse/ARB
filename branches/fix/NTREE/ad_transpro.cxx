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

template<typename T>
class BufferPtr {
    T *const  bstart;
    T        *curr;
public:
    explicit BufferPtr(T *b) : bstart(b), curr(b) {}

    const T* start() const { return bstart; }
    size_t offset() const { return curr-bstart; }

    T get() { return *curr++; }

    void put(T c) { *curr++ = c; }
    void put(T c1, T c2, T c3) { put(c1); put(c2); put(c3); }
    void put(T c, size_t count) {
        memset(curr, c, count*sizeof(T));
        inc(count);
    }
    void copy(BufferPtr<const T>& source, size_t count) {
        memcpy(curr, source, count*sizeof(T));
        inc(count);
        source.inc(count);
    }

    T operator[](int i) const {
        nt_assert(i>=0 || size_t(-i)<=offset());
        return curr[i];
    }

    operator const T*() const { return curr; }
    operator T*() { return curr; }

    void inc(int o) { curr += o; nt_assert(curr>=bstart); }

    BufferPtr<T>& operator++() { curr++; return *this; }
    BufferPtr<T>& operator--() { inc(-1); return *this; }
};

template<typename T>
class SizedBufferPtr : public BufferPtr<T> {
    size_t len;
public:
    SizedBufferPtr(T *b, size_t len_) : BufferPtr<T>(b), len(len_) {}
    ~SizedBufferPtr() { nt_assert(valid()); }
    bool valid() const { return this->offset()<=len; }
    size_t restLength() const { nt_assert(valid()); return len-this->offset(); }
    size_t length() const { return len; }
};

typedef SizedBufferPtr<const char> SizedReadBuffer;
typedef SizedBufferPtr<char>       SizedWriteBuffer;

// ----------------------------------------

#define AUTODETECT_STARTPOS 3
inline bool legal_ORF_pos(int p) { return p>=0 && p<=2; }

static GB_ERROR arb_r2a(GBDATA *gb_main, bool use_entries, bool save_entries, int selected_startpos,
                        bool    translate_all, const char *ali_source, const char *ali_dest)
{
    // if use_entries   == true -> use fields 'codon_start' and 'transl_table' for translation
    //                             (selected_startpos and AWAR_PROTEIN_TYPE are only used if both fields are missing,
    //                             if only one is missing, now an error occurs)
    // if use_entries   == false -> always use selected_startpos and AWAR_PROTEIN_TYPE
    // if translate_all == true -> a selected_startpos > 1 produces a leading 'X' in protein data
    //                             (otherwise nucleotides in front of the starting pos are simply ignored)
    // if selected_startpos == AUTODETECT_STARTPOS -> the start pos is chosen to minimise number of stop codons

    nt_assert(legal_ORF_pos(selected_startpos) || selected_startpos == AUTODETECT_STARTPOS);

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
                        nt_assert(legal_ORF_pos(sp_codon_start));
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
                        size_t  data_size = GB_read_string_count(gb_source_data);
                        if (!data) {
                            GB_print_error(); // cannot read data (ignore species)
                            ++no_data;
                        }
                        else {
                            if (!found_transl_info) ++spec_no_transl_info; // count species with missing info

                            if (startpos == AUTODETECT_STARTPOS)
                            {
                                int   cn;
                                int   stop_codons;
                                int   least_stop_codons = -1;
                                char* trial_data[3]     = {data, strdup(data), strdup(data)};

                                for (cn = 0 ; cn < 3 ; cn++)
                                {
                                    stop_codons = AWT_pro_a_nucs_convert(table, trial_data[cn], data_size, cn, translate_all, false, false, 0); // do the translation

                                    if ((stop_codons < least_stop_codons) ||
                                        (least_stop_codons == -1))
                                    {
                                        least_stop_codons = stop_codons;
                                        startpos          = cn;
                                    }
                                }

                                for (cn = 0 ; cn < 3 ; cn++)
                                {
                                    if (cn != startpos)
                                    {
                                        free(trial_data[cn]);
                                    }
                                }

                                data   = trial_data[startpos];
                                stops += least_stop_codons;

                            }
                            else
                            {
                                stops += AWT_pro_a_nucs_convert(table, data, data_size, startpos, translate_all, false, false, 0); // do the translation
                            }

                            nt_assert(legal_ORF_pos(startpos));
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

// translator only:
#define AWAR_TRANSPRO_POS    AWAR_TRANSPRO_PREFIX "pos" // [0..3]: 0-2 = reading frame; 3 = autodetect
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

        error             = arb_r2a(GLOBAL.gb_main, strcmp(mode, "fields") == 0, save2fields, startpos, translate_all, ali_source, ali_dest);
        if (!error) error = GBT_check_data(GLOBAL.gb_main, 0);

        free(mode);
        free(ali_dest);
        free(ali_source);
    }
    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
}

static void nt_trans_cursorpos_changed(AW_root *awr) {
    AW_awar *awar_startpos = awr->awar(AWAR_TRANSPRO_POS);

    if (awar_startpos->read_int() != AUTODETECT_STARTPOS) {
        int pos = bio2info(awr->awar(AWAR_CURSOR_POSITION)->read_int());
        pos     = pos % 3;
        awar_startpos->write_int(pos);
    }
}

AW_window *NT_create_dna_2_pro_window(AW_root *root) {
    GB_transaction ta(GLOBAL.gb_main);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "TRANSLATE_DNA_TO_PRO", "TRANSLATE DNA TO PRO");

    aws->load_xfig("transpro.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
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
    aws->insert_option("choose best", "choose best", AUTODETECT_STARTPOS);

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

#if defined(ASSERTION_USED)
            int maxTake  = std::min(3, leftDNA-minLeave);
            nt_assert(minTake<=maxTake);
#endif

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
        return score + 6 - dist[0] - dist[xcount-1]; // prefer border positions with less nucs
    }

    bool translates_to_Xs(const char *dna, TransTables allowed, TransTables& remaining) const {
        /*! checks whether distribution of 'dna' translates to X's
         * @param dna compressed dna
         * @param allowed allowed translation tables
         * @param remaining remaining translation tables
         * @return true if 'dna' translates to X's
         */
        bool translates = true;
        int  off        = 0;
        for (int p = 0; translates && p<xcount; off += dist[p++]) {
            if (dist[p] == 3) {
                TransTables this_remaining;
                translates = AWT_is_codon('X', dna+off, allowed, this_remaining);
                if (translates) {
                    nt_assert(this_remaining.is_subset_of(allowed));
                    allowed = this_remaining;
                }
            }
        }
        if (translates) remaining = allowed;
        return translates;
    }
};

inline bool isGap(char c) { return c == '-' || c == '.'; }

using std::string;

class FailedAt {
    string             reason;
    RefPtr<const char> at_prot; // points into aligned protein seq
    RefPtr<const char> at_dna;  // points into compressed seq

    int cmp(const FailedAt& other) const {
        ptrdiff_t d = at_prot - other.at_prot;
        if (!d)   d = at_dna - other.at_dna;
        return d<0 ? -1 : d>0 ? 1 : 0;
    }

public:
    FailedAt()
        : at_prot(NULL),
          at_dna(NULL)
    {}
    FailedAt(GB_ERROR reason_, const char *at_prot_, const char *at_dna_)
        : reason(reason_),
          at_prot(at_prot_),
          at_dna(at_dna_)
    {
        nt_assert(reason_);
    }

    GB_ERROR why() const { return reason.empty() ? NULL : reason.c_str(); }
    const char *protein_at() const { return at_prot; }
    const char *dna_at() const { return at_dna; }

    operator bool() const { return !reason.empty(); }

    void add_prefix(const char *prefix) {
        nt_assert(!reason.empty());
        reason = string(prefix)+reason;
    }

    bool operator>(const FailedAt& other) const { return cmp(other)>0; }
};

class RealignAttempt : virtual Noncopyable {
    TransTables           allowed;
    SizedReadBuffer       compressed_dna;
    BufferPtr<const char> aligned_protein;
    SizedWriteBuffer      target_dna;
    FailedAt              fail;
    bool                  cutoff_dna;

    void perform();

    bool sync_behind_X_and_distribute(const int x_count, char *const x_start, const char *const x_start_prot);

public:
    RealignAttempt(const TransTables& allowed_, const char *compressed_dna_, size_t compressed_len_, const char *aligned_protein_, char *target_dna_, size_t target_len_, bool cutoff_dna_)
        : allowed(allowed_),
          compressed_dna(compressed_dna_, compressed_len_),
          aligned_protein(aligned_protein_),
          target_dna(target_dna_, target_len_),
          cutoff_dna(cutoff_dna_)
    {
        nt_assert(aligned_protein[0]);
        perform();
    }

    const TransTables& get_remaining_tables() const { return allowed; }
    const FailedAt& failed() const { return fail; }
};

static GB_ERROR distribute_xdata(SizedReadBuffer& dna, size_t xcount, char *xtarget_, bool gap_before, bool gap_after, const TransTables& allowed, TransTables& remaining) {
    /*! distributes 'dna' to marked X-positions
     * @param xtarget destination buffer (target positions are marked with '!')
     * @param xcount number of X's encountered
     * @param gap_before true if resulting realignment has a gap or the start of alignment before the X-positions
     * @param gap_after analog to 'gap_before'
     * @param allowed allowed translation tables
     * @param remaining remaining allowed translation tables (with those tables disabled for which no distribution possible)
     * @return error if dna distribution wasn't possible
     */

    BufferPtr<char> xtarget(xtarget_);
    Distributor     dist(xcount, dna.length());
    GB_ERROR        error = dist.get_error();
    if (!error) {
        Distributor best(dist);
        TransTables best_remaining = allowed;

        while (dist.next()) {
            if (dist.get_score() > best.get_score()) {
                if (!dist.mayFailTranslation() || best.mayFailTranslation()) {
                    best           = dist;
                    best_remaining = allowed;
                    nt_assert(best_remaining.is_subset_of(allowed));
                }
            }
        }

        if (best.mayFailTranslation()) {
            TransTables curr_remaining;
            if (best.translates_to_Xs(dna, allowed, curr_remaining)) {
                best_remaining = curr_remaining;
                nt_assert(best_remaining.is_subset_of(allowed));
            }
            else {
                nt_assert(!error);
                error = "no translating X-distribution found";
                dist.reset();
                do {
                    if (dist.translates_to_Xs(dna, allowed, curr_remaining)) {
                        best           = dist;
                        best_remaining = curr_remaining;
                        error          = NULL;
                        nt_assert(best_remaining.is_subset_of(allowed));
                        break;
                    }
                } while (dist.next());

                while (dist.next()) {
                    if (dist.get_score() > best.get_score()) {
                        if (dist.translates_to_Xs(dna, allowed, curr_remaining)) {
                            best           = dist;
                            best_remaining = curr_remaining;
                            nt_assert(best_remaining.is_subset_of(allowed));
                        }
                    }
                }
            }
        }

        if (!error) { // now really distribute nucs
            for (int x = 0; x<best.size(); ++x) {
                while (xtarget[0] != '!') {
                    nt_assert(xtarget[1] && xtarget[2]); // buffer overflow
                    xtarget.inc(3);
                }

                switch (best[x]) {
                    case 2: {
                        enum { UNDECIDED, SPREAD, LEFT, RIGHT } mode = UNDECIDED;

                        bool is_1st_X  = xtarget.offset() == 0;
                        bool gaps_left = is_1st_X ? gap_before : isGap(xtarget[-1]);

                        if (gaps_left) mode = LEFT;
                        else { // definitely has no gap left!
                            bool is_last_X  = x == best.size()-1;
                            int  next_nucs  = is_last_X ? 0 : best[x+1];
                            bool gaps_right = isGap(xtarget[3]) || next_nucs == 1 || (is_last_X && gap_after);

                            if (gaps_right) mode = RIGHT;
                            else {
                                bool nogaps_right = next_nucs == 3 || (is_last_X && !gap_after);
                                if (nogaps_right) { // we know, we have NO adjacent gaps
                                    mode = is_last_X ? LEFT : (is_1st_X ? RIGHT : SPREAD);
                                }
                                else {
                                    nt_assert(!is_last_X);
                                    mode = RIGHT; // forward problem to next X
                                }
                            }
                        }

                        char d1 = dna.get();
                        char d2 = dna.get();

                        switch (mode) {
                            case UNDECIDED: nt_assert(0); // fall-through in NDEBUG
                            case SPREAD: xtarget.put(d1,  '-', d2);  break;
                            case LEFT:   xtarget.put(d1,  d2,  '-'); break;
                            case RIGHT:  xtarget.put('-', d1,  d2);  break;
                        }

                        break;
                    }
                    case 1: xtarget.put('-', dna.get(), '-'); break;
                    case 3: xtarget.copy(dna, 3); break;
                    default: nt_assert(0); break;
                }
                nt_assert(dna.valid());
            }

            nt_assert(!error);
            remaining = best_remaining;
            nt_assert(remaining.is_subset_of(allowed));
        }
    }

    return error;
}

bool RealignAttempt::sync_behind_X_and_distribute(const int x_count, char *const x_start, const char *const x_start_prot) {
    /*! brute-force search for sync behind 'X' and distribute dna onto X positions
     * @param x_count number of X encountered
     * @param x_start dna read position
     * @param x_start_prot protein read position
     * @return true if sync and distribution succeed
     */

    bool complete = false;

    nt_assert(!failed());
    nt_assert(aligned_protein.offset()>0);
    const char p = aligned_protein[-1];

    size_t compressed_rest_len = compressed_dna.restLength();
    nt_assert(strlen(compressed_dna) == compressed_rest_len);

    size_t min_dna = x_count;
    size_t max_dna = std::min(size_t(x_count)*3, compressed_rest_len);

    if (min_dna>max_dna) {
        fail = FailedAt("not enough nucs for X's at sequence end", x_start_prot, compressed_dna);
    }
    else if (p) {
        FailedAt foremost;
        size_t   target_rest_len = target_dna.restLength();

        for (size_t x_dna = min_dna; x_dna<=max_dna; ++x_dna) { // prefer low amounts of used dna
            const char *dna_rest     = compressed_dna + x_dna;
            size_t      dna_rest_len = compressed_rest_len     - x_dna;

            nt_assert(strlen(dna_rest) == dna_rest_len);
            nt_assert(compressed_rest_len>=x_dna);

            RealignAttempt attemptRest(allowed, dna_rest, dna_rest_len, aligned_protein-1, target_dna, target_rest_len, cutoff_dna);
            FailedAt       restFailed = attemptRest.failed();

            if (!restFailed) {
                SizedReadBuffer distrib_dna(compressed_dna, x_dna);

                bool has_gap_before = x_start == target_dna.start() ? true : isGap(x_start[-1]);
                bool has_gap_after  = isGap(dna_rest[0]);

                TransTables remaining;
                GB_ERROR    disterr = distribute_xdata(distrib_dna, x_count, x_start, has_gap_before, has_gap_after, attemptRest.get_remaining_tables(), remaining);
                if (disterr) {
                    restFailed = FailedAt(disterr, x_start_prot, dna_rest); // prot=start of Xs; dna=start of sync (behind Xs)
                }
                else {
                    nt_assert(remaining.is_subset_of(allowed));
                    nt_assert(remaining.is_subset_of(attemptRest.get_remaining_tables()));
                    allowed = remaining;
                }
            }

            if (restFailed) {
                if (restFailed > foremost) foremost = restFailed; // track "best" failure (highest fail position)
            }
            else { // success
                foremost = FailedAt();
                complete = true;
                break; // use first success and return
            }
        }

        if (foremost) {
            nt_assert(!complete);
            fail = foremost;
            if (!strstr(fail.why(), "Sync behind 'X'")) { // do not spam repetitive sync-failures
                fail.add_prefix("Sync behind 'X' failed foremost with: ");
            }
        }
        else {
            nt_assert(complete);
        }
    }
    else {
        GB_ERROR fail_reason = "internal error: no distribution attempted";
        nt_assert(min_dna>0);
        size_t x_dna;
        for (x_dna = max_dna; x_dna>=min_dna; --x_dna) {     // prefer high amounts of dna
            SizedReadBuffer append_dna(compressed_dna, x_dna);
            TransTables     remaining;
            fail_reason = distribute_xdata(append_dna, x_count, x_start, false, true, allowed, remaining);
            if (!fail_reason) { // found distribution -> done
                nt_assert(remaining.is_subset_of(allowed));
                allowed = remaining;
                break;
            }
        }

        if (fail_reason) {
            fail = FailedAt(fail_reason, x_start_prot+1, compressed_dna); // report error at start of X's
        }
        else {
            fail = FailedAt(); // clear
            compressed_dna.inc(x_dna);
        }
    }

    nt_assert(implicated(complete, allowed.any()));

    return complete;
}

void RealignAttempt::perform() {
    bool complete = false; // set to true, if recursive attempt succeeds

    while (char p = toupper(aligned_protein.get())) {
        if (p=='X') { // one X represents 1 to 3 DNAs (normally 1 or 2, but 'NNN' translates to 'X')
            char       *x_start      = target_dna;
            const char *x_start_prot = aligned_protein-1;
            int         x_count      = 0;

            for (;;) {
                if      (p=='X')   { x_count++; target_dna.put('!', 3); } // fill X space with marker
                else if (isGap(p)) target_dna.put(p, 3);
                else break;

                p = toupper(aligned_protein.get());
            }

            nt_assert(x_count);
            nt_assert(!complete);
            complete = sync_behind_X_and_distribute(x_count, x_start, x_start_prot);
            if (!complete && !failed()) {
                if (p) { // not all proteins were processed
                    fail = FailedAt("internal error", aligned_protein-1, compressed_dna);
                    nt_assert(0);
                }
            }
            break; // done
        }

        if (isGap(p)) target_dna.put(p, 3);
        else {
            TransTables remaining;
            size_t      compressed_rest_len = compressed_dna.restLength();

            if (compressed_rest_len<3) {
                fail = FailedAt(GBS_global_string("not enough nucs left for codon of '%c'", p), aligned_protein-1, compressed_dna);
            }
            else {
                nt_assert(strlen(compressed_dna) == compressed_rest_len);
                nt_assert(compressed_rest_len >= 3);
                const char *why_fail;
                if (!AWT_is_codon(p, compressed_dna, allowed, remaining, &why_fail)) {
                    fail = FailedAt(why_fail, aligned_protein-1, compressed_dna);
                }
            }

            if (failed()) break;

            nt_assert(remaining.is_subset_of(allowed));
            allowed = remaining;
            target_dna.copy(compressed_dna, 3);
        }
    }

    nt_assert(compressed_dna.valid());

    if (!failed() && !complete) {
        while (target_dna.offset()>0 && isGap(target_dna[-1])) --target_dna; // remove terminal gaps

        if (!cutoff_dna) { // append leftover dna-data (data w/o corresponding aa)
            size_t compressed_rest_len = compressed_dna.restLength();
            size_t target_rest_len = target_dna.restLength();
            if (compressed_rest_len<=target_rest_len) {
                target_dna.copy(compressed_dna, compressed_rest_len);
            }
            else {
                fail = FailedAt(GBS_global_string("too much trailing DNA (%zu nucs, but only %zu columns left)",
                                                  compressed_rest_len, target_rest_len),
                                aligned_protein-1, compressed_dna);
            }
        }

        if (!failed()) target_dna.put('.', target_dna.restLength()); // fill rest of sequence with dots
        *target_dna = 0;
    }

#if defined(ASSERTION_USED)
    if (!failed()) {
        nt_assert(strlen(target_dna.start()) == target_dna.length());
    }
#endif
}

inline char *unalign(const char *data, size_t len, size_t& compressed_len) {
    // removes gaps from sequence
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

class Realigner {
    const char *ali_source;
    const char *ali_dest;

    size_t ali_len;        // of ali_dest
    size_t needed_ali_len; // >ali_len if ali_dest is too short; 0 otherwise

    const char *fail_reason;

    GB_ERROR annotate_fail_position(const FailedAt& failed, const char *source, const char *dest, const char *compressed_dest) {
        int source_fail_pos = failed.protein_at() - source;
        int dest_fail_pos   = 0;
        {
            int fail_d_base_count = failed.dna_at() - compressed_dest;

            const char *dp = dest;

            for (;;) {
                char c = *dp++;

                if (!c) { // failure at end of sequence
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
        return GBS_global_string("%s at %s:%i / %s:%i",
                                 failed.why(),
                                 ali_source, info2bio(source_fail_pos),
                                 ali_dest, info2bio(dest_fail_pos));
    }


    static void calc_needed_dna(const char *prot, size_t len, size_t& minDNA, size_t& maxDNA) {
        minDNA = maxDNA = 0;
        for (size_t o = 0; o<len; ++o) {
            char p = toupper(prot[o]);
            if (p == 'X') {
                minDNA += 1;
                maxDNA += 3;
            }
            else if (!isGap(p)) {
                minDNA += 3;
                maxDNA += 3;
            }
        }
    }
    static size_t countLeadingGaps(const char *buffer) {
        size_t gaps = 0;
        for (int o = 0; isGap(buffer[o]); ++o) ++gaps;
        return gaps;
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

    char *realign_seq(TransTables& allowed, const char *const source, size_t source_len, const char *const dest, size_t dest_len, bool cutoff_dna) {
        nt_assert(!failure());

        size_t  wanted_ali_len = source_len*3;
        char   *buffer         = NULL;

        if (ali_len<wanted_ali_len) {
            fail_reason = GBS_global_string("Alignment '%s' is too short (increase its length to %zu)", ali_dest, wanted_ali_len);
            if (wanted_ali_len>needed_ali_len) needed_ali_len = wanted_ali_len;
        }
        else {
            // compress destination DNA (=remove align-characters):
            size_t  compressed_len;
            char   *compressed_dest = unalign(dest, dest_len, compressed_len);

            buffer = (char*)malloc(ali_len+1);

            RealignAttempt attempt(allowed, compressed_dest, compressed_len, source, buffer, ali_len, cutoff_dna);
            FailedAt       failed = attempt.failed();

            if (failed) {
                // test for superfluous DNA at sequence start
                size_t min_dna, max_dna;
                calc_needed_dna(source, source_len, min_dna, max_dna);

                if (min_dna<compressed_len) { // we have more DNA than we need
                    size_t extra_dna = compressed_len-min_dna;
                    for (size_t skip = 1; skip<=extra_dna; ++skip) {
                        RealignAttempt attemptSkipped(allowed, compressed_dest+skip, compressed_len-skip, source, buffer, ali_len, cutoff_dna);
                        if (!attemptSkipped.failed()) {
                            failed = FailedAt(); // clear
                            if (!cutoff_dna) {
                                size_t start_gaps = countLeadingGaps(buffer);
                                if (start_gaps<skip) {
                                    failed = FailedAt(GBS_global_string("Not enough gaps to place %zu extra nucs at start of sequence",
                                                                        skip), source, compressed_dest);
                                }
                                else { // success
                                    memcpy(buffer+(start_gaps-skip), compressed_dest, skip); // copy-in skipped dna
                                }
                            }
                            if (!failed) {
                                nt_assert(attempt.get_remaining_tables().is_subset_of(allowed));
                                allowed = attemptSkipped.get_remaining_tables();
                            }
                            break; // no need to skip more dna, when we already have too few leading gaps
                        }
                    }
                }
            }
            else {
                nt_assert(attempt.get_remaining_tables().is_subset_of(allowed));
                allowed = attempt.get_remaining_tables();
            }

            if (failed) {
                fail_reason = annotate_fail_position(failed, source, dest, compressed_dest);
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
          len(0),
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


static GB_ERROR realign_marked(GBDATA *gb_main, const char *ali_source, const char *ali_dest, size_t& neededLength, bool unmark_succeeded, bool cutoff_dna) {
    /*! realigns DNA alignment of marked sequences according to their protein alignment
     * @param ali_source protein source alignment
     * @param ali_dest modified DNA alignment
     * @param neededLength result: minimum alignment length needed in ali_dest (if too short) or 0 if long enough
     * @param unmark_succeeded unmark all species that were successfully realigned
     */
    AP_initialize_codon_tables();

    nt_assert(GB_get_transaction_level(gb_main) == 0);
    GB_transaction ta(gb_main); // do not abort (otherwise sth goes wrong with species marks)

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
    long no_of_realigned_species = 0; // count successfully realigned species

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
            TransTables allowed; // default: all translation tables allowed
#if defined(ASSERTION_USED)
            bool has_valid_translation_info = false;
#endif
            {
                int arb_transl_table, codon_start;
                GB_ERROR local_error = AWT_getTranslationInfo(gb_species, arb_transl_table, codon_start);
                if (local_error) {
                    realigner.set_failure(GBS_global_string("Error while reading 'transl_table' (%s)", local_error));
                }
                else if (arb_transl_table >= 0) {
                    // we found a 'transl_table' entry -> restrict used code to the code stored there
                    allowed.forbidAllBut(arb_transl_table);
#if defined(ASSERTION_USED)
                    has_valid_translation_info = true;
#endif
                }
            }

            if (!realigner.failure()) {
                char *buffer = realigner.realign_seq(allowed, source.data, source.len, dest.data, dest.len, cutoff_dna);
                if (buffer) { // re-alignment successful
                    error = GB_write_string(dest.gb_data, buffer);

                    if (!error) {
                        int explicit_table_known = allowed.explicit_table();

                        if (explicit_table_known >= 0) { // we know the exact code -> write codon_start and transl_table
                            const int codon_start  = 0; // by definition (after realignment)
                            error = AWT_saveTranslationInfo(gb_species, explicit_table_known, codon_start);
                        }
#if defined(ASSERTION_USED)
                        else { // we dont know the exact code -> can only happen if species has no translation info
                            nt_assert(allowed.any()); // bug in realigner
                            nt_assert(!has_valid_translation_info);
                        }
#endif
                    }
                    free(buffer);
                    if (!error && unmark_succeeded) GB_write_flag(gb_species, 0);
                }
            }
        }

        if (realigner.failure()) {
            nt_assert(!error);
            GB_warningf("Automatic re-align failed for '%s'\nReason: %s", GBT_read_name(gb_species), realigner.failure());
        }
        else if (!error) {
            no_of_realigned_species++;
        }

        progress.inc_and_check_user_abort(error);
    }

    neededLength = realigner.get_needed_dest_alilen();

    if (no_of_marked_species == 0) {
        GB_warning("Please mark some species to realign them");
    }
    else if (no_of_realigned_species != no_of_marked_species) {
        long failed = no_of_marked_species-no_of_realigned_species;
        nt_assert(failed>0);
        if (no_of_realigned_species) {
            GB_warningf("%li marked species failed to realign (%li succeeded)", failed, no_of_realigned_species);
        }
        else {
            GB_warning("All marked species failed to realign");
        }
    }

    if (error) progress.done();
    else error = GBT_check_data(gb_main,ali_dest);

    return error;
}

static void realign_event(AW_window *aww) {
    AW_root  *aw_root          = aww->get_root();
    char     *ali_source       = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
    char     *ali_dest         = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
    bool      unmark_succeeded = aw_root->awar(AWAR_REALIGN_UNMARK)->read_int();
    bool      cutoff_dna       = aw_root->awar(AWAR_REALIGN_CUTOFF)->read_int();
    size_t    neededLength     = 0;
    GBDATA   *gb_main          = GLOBAL.gb_main;
    GB_ERROR  error            = realign_marked(gb_main, ali_source, ali_dest, neededLength, unmark_succeeded, cutoff_dna);

    if (!error && neededLength) {
        bool auto_inc_alisize = aw_root->awar(AWAR_REALIGN_INCALI)->read_int();
        if (auto_inc_alisize) {
            {
                GB_transaction ta(gb_main);
                error = ta.close(GBT_set_alignment_len(gb_main, ali_dest, neededLength));
            }
            if (!error) {
                aw_message(GBS_global_string("Alignment length of '%s' has been set to %zu\n"
                                             "running re-aligner again!",
                                             ali_dest, neededLength));

                error = realign_marked(gb_main, ali_source, ali_dest, neededLength, unmark_succeeded, cutoff_dna);
                if (neededLength) {
                    error = GBS_global_string("internal error: neededLength=%zu (after autoinc)", neededLength);
                }
            }
        }
        else {
            GB_transaction ta(gb_main);
            long           destLen = GBT_get_alignment_len(gb_main, ali_dest);
            nt_assert(destLen>0 && size_t(destLen)<neededLength);
            error = GBS_global_string("Missing %zu columns in alignment '%s' (got=%li, need=%zu)\n"
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
    aws->init(root, "REALIGN_DNA", "REALIGN DNA");

    aws->load_xfig("transdna.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("realign_dna.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("source");
#if defined(DEVEL_RALF)
    awt_create_ALI_selection_button(GLOBAL.gb_main, aws, AWAR_TRANSPRO_SOURCE, "dna=:rna="); // @@@ nonsense here - just testing awt_create_ALI_selection_button somewhere
#else // !defined(DEVEL_RALF)
    awt_create_ALI_selection_list(GLOBAL.gb_main, aws, AWAR_TRANSPRO_SOURCE, "dna=:rna=");
#endif
    aws->at("dest");
    awt_create_ALI_selection_list(GLOBAL.gb_main, aws, AWAR_TRANSPRO_DEST, "pro=:ami=");

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

static const char *translation_info(GBDATA *gb_species) {
    int      arb_transl_table;
    int      codon_start;
    GB_ERROR error = AWT_getTranslationInfo(gb_species, arb_transl_table, codon_start);

    static SmartCharPtr result;

    if (error) result = GBS_global_string_copy("Error: %s", error);
    else       result = GBS_global_string_copy("t=%i,cs=%i", arb_transl_table, codon_start);

    return &*result;
}

static arb_handlers test_handlers = {
    msg_to_string,
    msg_to_string,
    msg_to_string,
    active_arb_handlers->status,
};

#define DNASEQ(name) GB_read_char_pntr(GBT_find_sequence(GBT_find_species(gb_main, name), "ali_dna"))
#define PROSEQ(name) GB_read_char_pntr(GBT_find_sequence(GBT_find_species(gb_main, name), "ali_pro"))

#define TRANSLATION_INFO(name) translation_info(GBT_find_species(gb_main, name))

void TEST_realign() {
    arb_handlers *old_handlers = active_arb_handlers;
    ARB_install_handlers(test_handlers);

    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_realign.arb", "rw");

    arb_suppress_progress here;
    enum TransResult { SAME, CHANGED };

    {
        GB_ERROR error;
        size_t   neededLength = 0;

        {
            struct transinfo_check {
                const char  *species_name;
                const char  *old_info;
                TransResult  changed;
                const char  *new_info;
            };

            transinfo_check info[] = {
                { "BctFra12", "t=0,cs=1",  SAME,    NULL },        // fails -> unchanged
                { "CytLyti6", "t=9,cs=1",  CHANGED, "t=9,cs=0" },
                { "TaxOcell", "t=14,cs=1", CHANGED, "t=14,cs=0" },
                { "StrRamo3", "t=0,cs=1",  SAME,    NULL },        // fails -> unchanged
                { "StrCoel9", "t=0,cs=0",  SAME,    NULL },        // already correct
                { "MucRacem", "t=0,cs=1",  CHANGED, "t=0,cs=0" },
                { "MucRace2", "t=0,cs=1",  CHANGED, "t=0,cs=0" },
                { "MucRace3", "t=0,cs=0",  SAME,    NULL },        // fails -> unchanged
                { "AbdGlauc", "t=0,cs=0",  SAME,    NULL },        // already correct
                { "CddAlbic", "t=0,cs=0",  SAME,    NULL },        // already correct

                { NULL, NULL, SAME, NULL }
            };

            {
                GB_transaction ta(gb_main);

                for (int i = 0; info[i].species_name; ++i) {
                    const transinfo_check& I = info[i];
                    TEST_ANNOTATE(I.species_name);
                    TEST_EXPECT_EQUAL(TRANSLATION_INFO(I.species_name), I.old_info);
                }
            }
            TEST_ANNOTATE(NULL);

            msgs  = "";
            error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, false);
            TEST_EXPECT_NO_ERROR(error);
            TEST_EXPECT_EQUAL(msgs,
                              "Automatic re-align failed for 'BctFra12'\nReason: not enough nucs for X's at sequence end at ali_pro:40 / ali_dna:109\n" // new correct report (got no nucs for 1 X)
                              "Automatic re-align failed for 'StrRamo3'\nReason: not enough nucs for X's at sequence end at ali_pro:36 / ali_dna:106\n" // new correct report (got 3 nucs for 4 Xs)
                              "Automatic re-align failed for 'MucRace3'\nReason: Sync behind 'X' failed foremost with: Not all IUPAC-combinations of 'NCC' translate to 'T' (for trans-table 0) at ali_pro:28 / ali_dna:78\n" // correct report
                              "3 marked species failed to realign (7 succeeded)\n"
                );

            {
                GB_transaction ta(gb_main);

                TEST_EXPECT_EQUAL(DNASEQ("BctFra12"),    "ATGGCTAAAGAGAAATTTGAACGTACCAAACCGCACGTAAACATTGGTACAATCGGTCACGTTGACCACGGTAAAACCACTTTGACTGCTGCTATCACTACTGTGTTG------------------"); // failed = > seq unchanged
                TEST_EXPECT_EQUAL(DNASEQ("CytLyti6"),    "-A-TGGCAAAGGAAACTTTTGATCGTTCCAAACCGCACTTAA---ATATAG---GTACTATTGGACACGTAGATCACGGTAAAACTACTTTAACTGCTGCTATTACAASAGTAT-T-----G....");
                TEST_EXPECT_EQUAL(DNASEQ("TaxOcell"),    "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G..........");
                TEST_EXPECT_EQUAL(DNASEQ("StrRamo3"),    "ATGTCCAAGACGGCATACGTGCGCACCAAACCGCATCTGAACATCGGCACGATGGGTCATGTCGACCACGGCAAGACCACGTTGACCGCCGCCATCACCAAGGTCCTC------------------"); // failed = > seq unchanged
                TEST_EXPECT_EQUAL(DNASEQ("StrCoel9"),    "ATGTCCAAGACGGCGTACGTCCGC-C--C--A-CC-TG--A----GGCACGATG-G-CC--C-GACCACGGCAAGACCACCCTGACCGCCGCCATCACCAAGGTC-C--T--------C.......");
                TEST_EXPECT_EQUAL(DNASEQ("MucRacem"),    "......ATGGGTAAAGAG---------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGTATA-AA......");
                TEST_EXPECT_EQUAL(DNASEQ("MucRace2"),    "ATGGGTAAGGAG---------AAGACTCACGTTAACGTCGTCGTCATTGGTCACGTCGATTCCGGTAAATCTACTACTACTGGTCACTTGATTTACAAGTGTGGTGGT-ATNNNAT-AAA......");
                TEST_EXPECT_EQUAL(DNASEQ("MucRace3"),    "-----------ATGGGTAAAGAGAAGACTCACGTTRAYGTTGTCGTTATTGGTCACGTCRATTCCGGTAAGTCCACCNCCRCTGGTCACTTGATTTACAAGTGTGGTGGTATAA-A----------"); // failed = > seq unchanged
                TEST_EXPECT_EQUAL(DNASEQ("AbdGlauc"),    "ATGGGTAAA-G--A--A--A--A--G-AC--T-CACGTTAACGTCGTTGTCATTGGTCACGTCGATTCTGGTAAATCCACCACCACTGGTCATTTGATCTACAAGTGCGGTGGTATA-AA......");
                TEST_EXPECT_EQUAL(DNASEQ("CddAlbic"),    "ATG-GG-TAAA-GAA------------AAAACTCACGTTAACGTTGTTGTTATTGGTCACGTCGATTCCGGTAAATCTACTACCACCGGTCACTTAATTTACAAGTGTGGTGGTATA-AA......");
                // ------------------------------------- "123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123"

                for (int i = 0; info[i].species_name; ++i) {
                    const transinfo_check& I = info[i];
                    TEST_ANNOTATE(I.species_name);
                    switch (I.changed) {
                        case SAME:
                            TEST_EXPECT_EQUAL(TRANSLATION_INFO(I.species_name), I.old_info);
                            TEST_EXPECT_NULL(I.new_info);
                            break;
                        case CHANGED:
                            TEST_EXPECT_EQUAL(TRANSLATION_INFO(I.species_name), I.new_info);
                            TEST_EXPECT_DIFFERENT(I.new_info, I.old_info);
                            break;
                    }
                }
                TEST_ANNOTATE(NULL);
            }
        }

        // test translation of sucessful realignments (see previous section)
        {
            GB_transaction ta(gb_main);

            struct translate_check {
                const char *species_name;
                const char *original_prot;
                TransResult retranslation;
                const char *changed_prot; // if changed by translation (NULL for SAME)
            };

            translate_check trans[] = {
                { "CytLyti6", "XWQRKLLIVPNRT*-I*-VLLDT*ITVKLL*SSLLZZYX-X.",
                  CHANGED,    "XWQRKLLIVPNRT*-I*-VLLDT*ITVKLL*SSLLQZYX-X." }, // ok: one of the Zs near end translates to Q
                { "TaxOcell", "XG*SNFWPVQAARNHRHD--RSRGPRQBDSDRCYHHGAX-..",
                  CHANGED,    "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX..." }, // ok - only changes gaptype at EOS
                { "MucRacem", "..MGKE---KTHVNVVVIGHVDSGKSTTTGHLIYKCGGIX..", SAME, NULL },
                { "MucRace2", "MGKE---KTHVNVVVIGHVDSGKSTTTGHLIYKCGGXXXK--",
                  CHANGED,    "MGKE---KTHVNVVVIGHVDSGKSTTTGHLIYKCGGXXXK.." }, // ok - only changes gaptype at EOS
                { "AbdGlauc", "MGKXXXXXXXXHVNVVVIGHVDSGKSTTTGHLIYKCGGIX..", SAME, NULL },
                { "StrCoel9", "MSKTAYVRXXXXXX-GTMXXXDHGKTTLTAAITKVXX--X..", SAME, NULL },
                { "CddAlbic", "MXXXE----KTHVNVVVIGHVDSGKSTTTGHLIYKCGGIX..", SAME, NULL },

                { NULL, NULL, SAME, NULL }
            };

            // check original protein sequences
            for (int t = 0; trans[t].species_name; ++t) {
                const translate_check& T = trans[t];
                TEST_ANNOTATE(T.species_name);
                TEST_EXPECT_EQUAL(PROSEQ(T.species_name), T.original_prot);
            }
            TEST_ANNOTATE(NULL);

            msgs  = "";
            error = arb_r2a(gb_main, true, false, 0, true, "ali_dna", "ali_pro");
            TEST_EXPECT_NO_ERROR(error);
            TEST_EXPECT_EQUAL(msgs, "codon_start and transl_table entries were found for all translated taxa\n10 taxa converted\n  1.100000 stops per sequence found\n");

            // check re-translated protein sequences
            for (int t = 0; trans[t].species_name; ++t) {
                const translate_check& T = trans[t];
                TEST_ANNOTATE(T.species_name);
                switch (T.retranslation) {
                    case SAME:
                        TEST_EXPECT_NULL(T.changed_prot);
                        TEST_EXPECT_EQUAL(PROSEQ(T.species_name), T.original_prot);
                        break;
                    case CHANGED:
                        TEST_REJECT_NULL(T.changed_prot);
                        TEST_EXPECT_DIFFERENT(T.original_prot, T.changed_prot);
                        TEST_EXPECT_EQUAL(PROSEQ(T.species_name), T.changed_prot);
                        break;
                }
            }
            TEST_ANNOTATE(NULL);

            ta.close("dont commit");
        }

        // -----------------------------
        //      provoke some errors

        GBDATA *gb_TaxOcell;
        // unmark all but gb_TaxOcell
        {
            GB_transaction ta(gb_main);

            gb_TaxOcell = GBT_find_species(gb_main, "TaxOcell");
            TEST_REJECT_NULL(gb_TaxOcell);

            GBT_mark_all(gb_main, 0);
            GB_write_flag(gb_TaxOcell, 1);
        }

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        // wrong alignment type
        {
            msgs  = "";
            error = realign_marked(gb_main, "ali_dna", "ali_pro", neededLength, false, false);
            TEST_EXPECT_ERROR_CONTAINS(error, "Invalid source alignment type");
            TEST_EXPECT_EQUAL(msgs, "");
        }

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        GBDATA *gb_TaxOcell_amino;
        GBDATA *gb_TaxOcell_dna;
        {
            GB_transaction ta(gb_main);
            gb_TaxOcell_amino = GBT_find_sequence(gb_TaxOcell, "ali_pro");
            gb_TaxOcell_dna   = GBT_find_sequence(gb_TaxOcell, "ali_dna");
        }
        TEST_REJECT_NULL(gb_TaxOcell_amino);
        TEST_REJECT_NULL(gb_TaxOcell_dna);

        // -----------------------------------------
        //      document some existing behavior
        {
            struct realign_check {
                const char  *seq;
                const char  *result;
                bool         cutoff;
                TransResult  retranslation;
                const char  *changed_prot; // if changed by translation (NULL for SAME)
            };

            realign_check seq[] = {
                //"XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-.." // original aa sequence
                // { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "sdfjlksdjf" }, // templ
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G..........", false, CHANGED, // original
                  "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX..." }, // ok - only changes gaptype at EOS

                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHG.....", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCTG...........", false, CHANGED, // missing some AA at right end (extra DNA gets no longer truncated!)
                  "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX..." }, // ok - adds translation of extra DNA (DNA should never be modified by realigner!)
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHG.....", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGT...............", true,  SAME, NULL }, // missing some AA at right end -> cutoff DNA

                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYH-----..", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCTG...........", false, CHANGED,
                  "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX..." }, // ok - adds translation of extra DNA
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCY---H....", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTAT---------CACCACGGTGCTG..", false, CHANGED, // rightmost possible position of 'H' (see failing test below)
                  "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCY---HHGAX" }, // ok - adds translation of extra DNA

                { "---SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "-ATGGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G..........", false, CHANGED, // missing some AA at left end (extra DNA gets detected now)
                  "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX..." }, // ok - adds translation of extra DNA (start of alignment)
                { "...SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX...", ".........AGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G..........", true,  SAME, NULL }, // missing some AA at left end -> cutoff DNA


                { "XG*SNFXXXXXXAXXXNHRHDXXXXXXPRQNDSDRCYHHGAX", "AT-GGCTAAAGAAACTTT-TG-AC-CG-GT-CCAA-GCC-GC-ACGT-AAACATCGGCACGAT-CG-GT-CA-CG-TGGA-CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G.", false, SAME, NULL },
                { "XG*SNFWPVQAARNHRHD-XXXXXX-PRQNDSDRCYHHGAX-", "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT---CG-GT-CA-CG-TG-GA----CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G....", false, CHANGED,
                  "XG*SNFWPVQAARNHRHD-XXXXXX-PRQNDSDRCYHHGAX." }, // ok - only changes gaptype at EOS
                { "XG*SNXLXRXQA-ARNHRHD-RXXVX-PRQNDSDRCYHHGAX", "AT-GGCTAAAGAAACTT-TTGAC-CGGTC-CAAGCC---GCACGTAAACATCGGCACGAT---CGG-TCAC-GTG-GA---CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G.", false, SAME, NULL },
                { "XG*SXXFXDXVQAXT*TSARXRSXVX-PRQNDSDRCYHHGAX", "AT-GGCTAAAGA-A-AC-TTT-T-GACCG-GTCCAAGCCGC-ACGTAAACATCGGCACGA-T-CGGTCA-C-GTG-GA---CCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G.", false, SAME, NULL },
                // -------------------------------------------- "123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123123"

                { 0, 0, false, SAME, NULL }
            };

            int   arb_transl_table, codon_start;
            char *org_dna;
            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_NO_ERROR(AWT_getTranslationInfo(gb_TaxOcell, arb_transl_table, codon_start));
                TEST_EXPECT_EQUAL(translation_info(gb_TaxOcell), "t=14,cs=0");
                org_dna = GB_read_string(gb_TaxOcell_dna);
            }

            for (int s = 0; seq[s].seq; ++s) {
                TEST_ANNOTATE(GBS_global_string("s=%i", s));
                realign_check& S = seq[s];

                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, S.seq));
                }
                msgs  = "";
                error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, S.cutoff);
                TEST_EXPECT_NO_ERROR(error);
                TEST_EXPECT_EQUAL(msgs, "");
                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_TaxOcell_dna), S.result);

                    // test retranslation:
                    msgs  = "";
                    error = arb_r2a(gb_main, true, false, 0, true, "ali_dna", "ali_pro");
                    TEST_EXPECT_NO_ERROR(error);
                    if (s == 10) {
                        TEST_EXPECT_EQUAL(msgs, "codon_start and transl_table entries were found for all translated taxa\n1 taxa converted\n  2.000000 stops per sequence found\n");
                    }
                    else if (s == 6) {
                        TEST_EXPECT_EQUAL(msgs, "codon_start and transl_table entries were found for all translated taxa\n1 taxa converted\n  0.000000 stops per sequence found\n");
                    }
                    else {
                        TEST_EXPECT_EQUAL(msgs, "codon_start and transl_table entries were found for all translated taxa\n1 taxa converted\n  1.000000 stops per sequence found\n");
                    }

                    switch (S.retranslation) {
                        case SAME:
                            TEST_EXPECT_NULL(S.changed_prot);
                            TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_TaxOcell_amino), S.seq);
                            break;
                        case CHANGED:
                            TEST_REJECT_NULL(S.changed_prot);
                            TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_TaxOcell_amino), S.changed_prot);
                            break;
                    }

                    TEST_EXPECT_EQUAL(translation_info(gb_TaxOcell), "t=14,cs=0");
                    TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_dna, org_dna)); // restore changed DB entry
                }
            }
            TEST_ANNOTATE(NULL);

            free(org_dna);
        }

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        // ----------------------------------------------------
        //      write some aa sequences provoking failures
        {
            struct realign_fail {
                const char *seq;
                const char *failure;
            };

#define ERRPREFIX     "Automatic re-align failed for 'TaxOcell'\nReason: "
#define ERRPREFIX_LEN 49

#define FAILONE "All marked species failed to realign\n"

            // dna of TaxOcell:
            // "AT-GGCTAAAGAAACTTTTGACCGGTCCAAGCCGCACGTAAACATCGGCACGAT------CGGTCACGTGGACCACGGCAAAACGACTCTGACCGCTGCTATCACCACGGTGCT-G----......"

            realign_fail seq[] = {
                //"XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-.." // original aa sequence
                // { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "sdfjlksdjf" }, // templ

                // wanted realign failures:
                { "XG*SNFXXXXXAXNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'GGA' translates to 'G', not to 'P' at ali_pro:25 / ali_dna:70\n" FAILONE },    // ok to fail: 5 Xs impossible
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Alignment 'ali_dna' is too short (increase its length to 252)\n" FAILONE }, // ok to fail: wrong alignment length
                { "XG*SNFWPVQAARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'GGA' translates to 'G', not to 'P' at ali_pro:25 / ali_dna:70\n" FAILONE },    // ok to fail
                { "XG*SNX-A-X-ARNHRHD--XXX-PRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'TGA' never translates to 'A' at ali_pro:8 / ali_dna:19\n" FAILONE },           // ok to fail
                { "XG*SXFXPXQAXRNHRHD--RSRGPRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'ACG' translates to 'T', not to 'R' at ali_pro:13 / ali_dna:36\n" FAILONE },    // ok to fail
                { "XG*SNFWPVQAARNHRHD-----GPRQNDSDRCYHHGAX-..", "Sync behind 'X' failed foremost with: 'CGG' translates to 'R', not to 'G' at ali_pro:24 / ali_dna:61\n" FAILONE },    // ok to fail: some AA missing in the middle
                { "XG*SNFWPVQAARNHRHDRSRGPRQNDSDRCYHHGAXHHGA.", "Sync behind 'X' failed foremost with: not enough nucs left for codon of 'H' at ali_pro:38 / ali_dna:117\n" FAILONE }, // ok to fail: too many AA
                { "XG*SNFWPVQAARNHRHD--RSRGPRQNDSDRCY----H...", "Sync behind 'X' failed foremost with: too much trailing DNA (10 nucs, but only 9 columns left) at ali_pro:43 / ali_dna:106\n" FAILONE }, // ok to fail: not enough space to place extra nucs behind 'H'
                { "--SNFWPVQAARNHRHD--RSRGPRQNDSDRCYHHGAX--..", "Not enough gaps to place 8 extra nucs at start of sequence at ali_pro:1 / ali_dna:1\n" FAILONE }, // also see related, succeeding test above (which has same AA seq; just one more leading gap)

                // failing realignments that should work:

                { 0, 0 }
            };

            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_EQUAL(translation_info(gb_TaxOcell), "t=14,cs=0");
            }

            for (int s = 0; seq[s].seq; ++s) {
                TEST_ANNOTATE(GBS_global_string("s=%i", s));
                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, seq[s].seq));
                }
                msgs  = "";
                error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, false);
                TEST_EXPECT_NO_ERROR(error);
                TEST_EXPECT_CONTAINS(msgs, ERRPREFIX);
                TEST_EXPECT_EQUAL(msgs.c_str()+ERRPREFIX_LEN, seq[s].failure);

                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_EQUAL(translation_info(gb_TaxOcell), "t=14,cs=0"); // should not change if error
                }
            }
            TEST_ANNOTATE(NULL);
        }

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        // ----------------------------------------------
        //      some examples for given DNA/AA pairs

        {
            struct explicit_realign {
                const char *acids;
                const char *dna;
                int         table;
                const char *info;
                const char *msgs;
            };

            // YTR (=X(2,9,16), =L(else))
            //     CTA (=T(2),        =L(else))
            //     CTG (=T(2), =S(9), =L(else))
            //     TTA (=*(16),       =L(else))
            //     TTG (=L(always))
            //
            // AAR (=X(6,11,14), =K(else))
            //     AAA (=N(6,11,14), =K(else))
            //     AAG (=K(always))
            //
            // ATH (=X(1,2,4,10,14), =I(else))
            //     ATA (=M(1,2,4,10,14), =I(else))
            //     ATC (=I(always))
            //     ATT (=I(always))

            const char*const NO_TI = "t=-1,cs=-1";

            explicit_realign example[] = {
                { "LK", "TTGAAG", -1, NO_TI,        NULL }, // fine (for any table)

                { "G",  "RGG",    -1, "t=10,cs=0",  NULL }, // correctly detects TI(10)

                { "LK",  "YTRAAR",    2,  "t=2,cs=0",  "Not all IUPAC-combinations of 'YTR' translate to 'L' (for trans-table 2) at ali_pro:1 / ali_dna:1\n" }, // expected failure (CTA->T for table=2)
                { "LX",  "YTRAAR",    -1, NO_TI,       NULL }, // fine (AAR->X for table=6,11,14)
                { "LXX", "YTRAARATH", -1, "t=14,cs=0", NULL }, // correctly detects TI(14)
                { "LXI", "YTRAARATH", -1, NO_TI,       NULL }, // fine (for table=6,11)

                { "LX", "YTRAAR", 2,  "t=2,cs=0",   "Not all IUPAC-combinations of 'YTR' translate to 'L' (for trans-table 2) at ali_pro:1 / ali_dna:1\n" }, // expected failure (AAR->K for table=2)
                { "LK", "YTRAAR", -1, NO_TI,        NULL }, // fine           (AAR->K for table!=6,11,14)
                { "LK", "YTRAAR", 6,  "t=6,cs=0",   "Not all IUPAC-combinations of 'AAR' translate to 'K' (for trans-table 6) at ali_pro:2 / ali_dna:4\n" }, // expected failure (AAA->N for table=6)
                { "XK", "YTRAAR", -1, NO_TI,        NULL }, // fine           (YTR->X for table=2,9,16)

                { "XX",   "-YTRAAR",      0,  "t=0,cs=0", NULL },                                                                                             // does not fail because it realigns such that it translates back to 'XXX'
                { "XXL",  "YTRAARTTG",    0,  "t=0,cs=0", "Not enough gaps to place 2 extra nucs at start of sequence at ali_pro:1 / ali_dna:1\n" },          // expected failure (none can translate to X with table= 0, so it tries )
                { "-XXL", "-YTRA-AR-TTG", 0,  "t=0,cs=0", NULL },                                                                                             // does not fail because it realigns such that it translates back to 'XXXL'
                { "IXXL", "ATTYTRAARTTG", 0,  "t=0,cs=0", "Sync behind 'X' failed foremost with: 'RTT' never translates to 'L' (for trans-table 0) at ali_pro:4 / ali_dna:9\n" }, // expected failure (none of the 2 middle codons can translate to X with table= 0)
                { "XX",   "-YTRAAR",      -1, NO_TI,      NULL },                                                                                             // does not fail because it realigns such that it translates back to 'XXX'
                { "IXXL", "ATTYTRAARTTG", -1, NO_TI,      "Sync behind 'X' failed foremost with: 'RTT' never translates to 'L' at ali_pro:4 / ali_dna:9\n" }, // expected failure (not both 2 middle codons can translate to X with same table)

                { "LX", "YTRATH", -1, NO_TI,        NULL }, // fine                (ATH->X for table=1,2,4,10,14)
                { "LX", "YTRATH", 2,  "t=2,cs=0",   "Not all IUPAC-combinations of 'YTR' translate to 'L' (for trans-table 2) at ali_pro:1 / ali_dna:1\n" }, // expected failure (YTR->X for table=2)
                { "XX", "YTRATH", 2,  "t=2,cs=0",   NULL }, // fine                (both->X for table=2)
                { "XX", "YTRATH", -1, "t=2,cs=0",   NULL }, // correctly detects TI(2)

                { "XX", "AARATH", 14, "t=14,cs=0",  NULL }, // fine (both->X for table=14)
                { "XX", "AARATH", -1, "t=14,cs=0",  NULL }, // correctly detects TI(14)
                { "KI", "AARATH", -1, NO_TI,        NULL }, // fine (for table!=1,2,4,6,10,11,14)
                { "KI", "AARATH", 4,  "t=4,cs=0",   "Not all IUPAC-combinations of 'ATH' translate to 'I' (for trans-table 4) at ali_pro:2 / ali_dna:4\n" }, // expected failure (ATH->X for table=4)
                { "KX", "AARATH", 14, "t=14,cs=0",  "Not all IUPAC-combinations of 'AAR' translate to 'K' (for trans-table 14) at ali_pro:1 / ali_dna:1\n" }, // expected failure (AAR->X for table=14)
                { "KX", "AARATH", -1, NO_TI,        NULL }, // fine for table=1,2,4,10
                { "KX", "AARATH", 4,  "t=4,cs=0",   NULL }, // test table=4
                { "XI", "AARATH", 14, "t=14,cs=0",  "Sync behind 'X' failed foremost with: Not all IUPAC-combinations of 'ATH' translate to 'I' (for trans-table 14) at ali_pro:2 / ali_dna:4\n" }, // expected failure (ATH->X for table=14)
                { "KI", "AARATH", 14, "t=14,cs=0",  "Not all IUPAC-combinations of 'AAR' translate to 'K' (for trans-table 14) at ali_pro:1 / ali_dna:1\n" }, // expected failure (AAR->X for table=14)

                { NULL, NULL, 0, NULL, NULL }
            };

            for (int e = 0; example[e].acids; ++e) {
                const explicit_realign& E = example[e];
                TEST_ANNOTATE(GBS_global_string("%s <- %s (#%i)", E.acids, E.dna, E.table));

                {
                    GB_transaction ta(gb_main);
                    TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_dna, E.dna));
                    TEST_EXPECT_NO_ERROR(GB_write_string(gb_TaxOcell_amino, E.acids));
                    if (E.table == -1) {
                        TEST_EXPECT_NO_ERROR(AWT_removeTranslationInfo(gb_TaxOcell));
                    }
                    else {
                        TEST_EXPECT_NO_ERROR(AWT_saveTranslationInfo(gb_TaxOcell, E.table, 0));
                    }
                }

                msgs  = "";
                error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, false);
                TEST_EXPECT_NULL(error);
                if (E.msgs) {
                    TEST_EXPECT_CONTAINS(msgs, ERRPREFIX);
                    string wanted_msgs = string(E.msgs)+FAILONE;
                    TEST_EXPECT_EQUAL(msgs.c_str()+ERRPREFIX_LEN, wanted_msgs);
                }
                else {
                    TEST_EXPECT_EQUAL(msgs, "");
                }

                GB_transaction ta(gb_main);
                if (!error) {
                    const char *dnaseq      = GB_read_char_pntr(gb_TaxOcell_dna);
                    size_t      expextedLen = strlen(E.dna);
                    size_t      seqlen      = strlen(dnaseq);
                    char       *firstPart   = GB_strndup(dnaseq, expextedLen);
                    size_t      dna_behind;
                    char       *nothing     = unalign(dnaseq+expextedLen, seqlen-expextedLen, dna_behind);

                    TEST_EXPECT_EQUAL(firstPart, E.dna);
                    TEST_EXPECT_EQUAL(dna_behind, 0);
                    TEST_EXPECT_EQUAL(nothing, "");

                    free(nothing);
                    free(firstPart);
                }
                TEST_EXPECT_EQUAL(translation_info(gb_TaxOcell), E.info);
            }
        }

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        // ----------------------------------
        //      invalid translation info
        {
            GB_transaction ta(gb_main);

            TEST_EXPECT_NO_ERROR(AWT_saveTranslationInfo(gb_TaxOcell, 14, 0));
            GBDATA *gb_trans_table = GB_entry(gb_TaxOcell, "transl_table");
            TEST_EXPECT_NO_ERROR(GB_write_string(gb_trans_table, "666")); // evil translation table
        }

        msgs  = "";
        error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, false);
        TEST_EXPECT_NO_ERROR(error);
        TEST_EXPECT_EQUAL(msgs, ERRPREFIX "Error while reading 'transl_table' (Illegal (or unsupported) value (666) in 'transl_table' (item='TaxOcell'))\n" FAILONE);
        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);

        // ---------------------------------------
        //      source/dest alignment missing
        for (int i = 0; i<2; ++i) {
            TEST_ANNOTATE(GBS_global_string("i=%i", i));

            {
                GB_transaction ta(gb_main);

                GBDATA *gb_ali = GB_get_father(GBT_find_sequence(gb_TaxOcell, i ? "ali_pro" : "ali_dna"));
                GB_push_my_security(gb_main);
                TEST_EXPECT_NO_ERROR(GB_delete(gb_ali));
                GB_pop_my_security(gb_main);
            }

            msgs  = "";
            error = realign_marked(gb_main, "ali_pro", "ali_dna", neededLength, false, false);
            TEST_EXPECT_NO_ERROR(error);
            if (i) {
                TEST_EXPECT_EQUAL(msgs, ERRPREFIX "No data in alignment 'ali_pro'\n" FAILONE);
            }
            else {
                TEST_EXPECT_EQUAL(msgs, ERRPREFIX "No data in alignment 'ali_dna'\n" FAILONE);
            }
        }
        TEST_ANNOTATE(NULL);

        TEST_EXPECT_EQUAL(GBT_count_marked_species(gb_main), 1);
    }

#undef ERRPREFIX
#undef ERRPREFIX_LEN

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

    Distributor check(6, 8);
    TEST_EXPECTATION(stateOf(check, "111113", true));
    TEST_EXPECTATION(stateOf(check, "111122", true));
    TEST_EXPECTATION(stateOf(check, "111131", true));
    TEST_EXPECTATION(stateOf(check, "111212", true));
    TEST_EXPECTATION(stateOf(check, "111221", true));
    TEST_EXPECTATION(stateOf(check, "111311", true));
    TEST_EXPECTATION(stateOf(check, "112112", true));
    TEST_EXPECTATION(stateOf(check, "112121", true));
    TEST_EXPECTATION(stateOf(check, "112211", true));
    TEST_EXPECTATION(stateOf(check, "113111", true));
    TEST_EXPECTATION(stateOf(check, "121112", true));
    TEST_EXPECTATION(stateOf(check, "121121", true));
    TEST_EXPECTATION(stateOf(check, "121211", true));
    TEST_EXPECTATION(stateOf(check, "122111", true));
    TEST_EXPECTATION(stateOf(check, "131111", true));
    TEST_EXPECTATION(stateOf(check, "211112", true));
    TEST_EXPECTATION(stateOf(check, "211121", true));
    TEST_EXPECTATION(stateOf(check, "211211", true));
    TEST_EXPECTATION(stateOf(check, "212111", true));
    TEST_EXPECTATION(stateOf(check, "221111", true));
    TEST_EXPECTATION(stateOf(check, "311111", false));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
