// ================================================================ //
//                                                                  //
//   File      : ED4_seq_colors.cxx                                 //
//   Purpose   : Sequence foreground coloring.                      //
//               Viewing differences only.                          //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "ed4_seq_colors.hxx"
#include "ed4_class.hxx"

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_awar_defs.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

#include <cctype>

static int default_NUC_set = 0;     // number of default nucleotide set
static int default_AMI_set = 3;     // number of default amino acid set

#define SEQ_COLOR_SETS      8
#define SEQ_COLOR_SET_ELEMS 28 // has to be a even number!

#define AWAR_SEQ_PATH                  "awt/seq_colors/"
#define AWAR_SEQ_NAME_STRINGS_TEMPLATE AWAR_SEQ_PATH  "strings/elem_%i"
#define AWAR_SEQ_NAME_TEMPLATE         AWAR_SEQ_PATH  "set_%i/elem_%i"
#define AWAR_SEQ_NAME_SELECTOR_NA      AWAR_SEQ_PATH   "na/select"
#define AWAR_SEQ_NAME_SELECTOR_AA      AWAR_SEQ_PATH   "aa/select"

static const char *default_sets[SEQ_COLOR_SETS] = {
    //A B C D E F G H I J K L M N O P Q R S T U V W X Y Z * -
    "=2=0=3=0=0=0=4=0=0=0=0=0=0=6=0=0=0=0=0=5=5=0=0=0=0=0=0=6", // A, C, G, TU and N in 5 colors
    "R2=0Y3=0=0=0R2=0=0=0=0=0=0=0=0=0=0=2=0Y3Y3=0=0=0=3=0=0=6", // AG and CTU in 2 colors
    "=6=5=6=5=7=7=6=5=7=7=3=7=3=9=7=7=7=3=3=6=6=5=3=7=3=7=7=6", // ambiguity
    "=7=0=7=8=2=9=8=9=3=0=2=3=7=8=0=8=2=2=2=2=0=3=9=6=9=0=0=6", // Protein colors

    "=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=6",
    "o9=0|2=0=0=0o5=0=0=0=0=0=0=0=0=0=0=0=0|8|8=0=0=0=0=0=0=6", // ambiguity (symbols)
    "=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=6",
    "=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=0=6",
};

static bool seq_color_awars_created = false;

// --------------------------------------------------------------------------------

static const char *default_characters(int elem) {
    static char result[3] = "xX";

    if (elem<26) { //  first 26 elements (0-25) are characters
        result[0] = 'a'+elem;
        result[1] = 'A'+elem;
    }
    else if (elem == 26) { // stop codon
        result[0] = '*';
        result[1] = 0;
    }
    else if (elem == 27) { // gaps
        result[0] = '-';
        result[1] = '.';
    }

    return result;
}
static const char *default_color(int cset, int elem) {
    // returns default color numbers for seq-color-set
    static char result[3] = "=0";
    const char *pos       = default_sets[cset]+2*elem;

    result[0] = pos[0];
    result[1] = pos[1];

    if (result[0] == '=' && result[1] == '0') result[0] = 0;

    return result;
}

static void color_awar_changed_cb(AW_root *, ED4_seq_colors *sc) {
    sc->reload();
}

static void create_seq_color_awars(AW_root *awr, ED4_seq_colors *sc) {
    e4_assert(!seq_color_awars_created);

    RootCallback update_cb = makeRootCallback(color_awar_changed_cb, sc);
    awr->awar_int(AWAR_SEQ_NAME_SELECTOR_NA, default_NUC_set, AW_ROOT_DEFAULT)->add_callback(update_cb);
    awr->awar_int(AWAR_SEQ_NAME_SELECTOR_AA, default_AMI_set, AW_ROOT_DEFAULT)->add_callback(update_cb);

    for (int elem = 0; elem<SEQ_COLOR_SET_ELEMS; ++elem) {
        const char *awar_name = GBS_global_string(AWAR_SEQ_NAME_STRINGS_TEMPLATE, elem);
        awr->awar_string(awar_name, default_characters(elem))->add_callback(update_cb);

        for (int cset = 0; cset<SEQ_COLOR_SETS; ++cset) {
            awar_name         = GBS_global_string(AWAR_SEQ_NAME_TEMPLATE, cset, elem);
            AW_awar *awar_col = awr->awar_string(awar_name, default_color(cset, elem));

            if (strcmp(awar_col->read_char_pntr(), "=0") == 0) { // translate old->new default
                awar_col->write_string("");
            }

            // add callback AFTER writing to awar above to avoid recursion
            // (the CB calls this function again, and seq_color_awars_created is set
            // to true at the very end...
            awar_col->add_callback(update_cb);
        }
    }

    seq_color_awars_created = true;
}

AW_window *ED4_create_seq_colors_window(AW_root *awr, ED4_seq_colors *sc) {
    char                     buf[256];
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    if (!seq_color_awars_created) create_seq_color_awars(awr, sc);

    aws = new AW_window_simple;
    aws->init(awr, "SEQUENCE_MAPPING", "Sequence color mapping");

    aws->at(10, 10);
    aws->auto_space(0, 3);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("sequence_colors.hlp"));
    aws->create_button("HELP", "HELP");
    
    aws->at_newline();

    for (int seqType=0; seqType<2; seqType++) {
        if (seqType==0) {
            aws->label("Select color-set for Nucleotides (NA):");
            aws->create_toggle_field(AWAR_SEQ_NAME_SELECTOR_NA, 1);
        }
        else {
            aws->label("Select color-set for Amino Acids (AA):");
            aws->create_toggle_field(AWAR_SEQ_NAME_SELECTOR_AA, 1);
        }

        for (int cset = 0; cset < SEQ_COLOR_SETS; cset++) {
            sprintf(buf, "%i", cset+1);
            aws->insert_toggle(buf, " ", cset);
        }
        aws->update_toggle_field();
        aws->at_newline();
    }

    const int BIG_COLUMNS    = 2;
    const int CHAR_COL_WIDTH = 4;
    const int SET_COL_WIDTH  = 2;

    int col_x_off[BIG_COLUMNS][SEQ_COLOR_SETS+1];

    aws->auto_space(3, 2);

    for (int bcol = 0; bcol<BIG_COLUMNS; ++bcol) {
        col_x_off[bcol][0] = aws->get_at_xposition();
        aws->button_length(CHAR_COL_WIDTH);
        aws->create_button(0, "Chars");
        
        aws->button_length(SET_COL_WIDTH);
        for (int cset = 0; cset < SEQ_COLOR_SETS; cset++) {
            sprintf(buf, "  %i", cset+1);
            col_x_off[bcol][cset+1] = aws->get_at_xposition();
            aws->create_button(0, buf);
        }

        if (!bcol) {
            int set_col_pixel_width = col_x_off[0][1]-col_x_off[0][0];
            aws->at_x(aws->get_at_xposition()+set_col_pixel_width);
        }
    }

    aws->at_newline();

    const int ROWS = SEQ_COLOR_SET_ELEMS/2;
    for (int r = 0; r<ROWS; r++) {
        for (int bcol = 0; bcol<BIG_COLUMNS; ++bcol) {
            int elem = bcol*ROWS+r;

            sprintf(buf, AWAR_SEQ_NAME_STRINGS_TEMPLATE, elem);
            aws->at_x(col_x_off[bcol][0]);
            aws->create_input_field(buf, CHAR_COL_WIDTH);

            for (int cset = 0; cset < SEQ_COLOR_SETS; cset++) {
                sprintf(buf, AWAR_SEQ_NAME_TEMPLATE, cset, elem);
                aws->at_x(col_x_off[bcol][cset+1]);
                aws->create_input_field(buf, SET_COL_WIDTH);
            }
        }
        aws->at_newline();
    }

    aws->window_fit();
    
    return aws;
}

void ED4_seq_colors::reload() {
    for (int i=0; i<256; i++) {
        char_2_gc[i]   = char_2_gc_aa[i]   = base_gc;
        char_2_char[i] = char_2_char_aa[i] = i;
    }

    AW_root *aw_root = AW_root::SINGLETON;

    if (!seq_color_awars_created) create_seq_color_awars(aw_root, this);

    const char *selector_awar[2] = { AWAR_SEQ_NAME_SELECTOR_NA, AWAR_SEQ_NAME_SELECTOR_AA };

    for (int selector = 0; selector<2; selector++) {
        long def_set = selector == 0 ? default_NUC_set : default_AMI_set;
        long cset    = aw_root->awar(selector_awar[selector])->read_int();

        if (cset < 0 || cset >= SEQ_COLOR_SETS) {
            cset = def_set;
        }

        for (int elem = 0; elem < SEQ_COLOR_SET_ELEMS; elem++) {
            char awar_name[256];

            sprintf(awar_name, AWAR_SEQ_NAME_STRINGS_TEMPLATE, elem);
            unsigned char *sc = (unsigned char *)aw_root->awar(awar_name)->read_string();

            sprintf(awar_name, AWAR_SEQ_NAME_TEMPLATE, (int)cset, elem);
            char *val = aw_root->awar(awar_name)->read_string();
            if (!val[0]) freedup(val, "=0"); // interpret '' as '  = 0'

            if (strlen(val) != 2 || val[1] >'9' || val[1] < '0') {
                aw_message(GB_export_errorf("Error in Color Lookup Table: '%s' is not of type X#", val));
            }
            else {
                if (selector == 0) { // Nucleotide colors
                    for (int i=0; sc[i]; i++) {
                        char_2_gc[sc[i]] = val[1]-'0' + base_gc;
                        if (val[0] != '=') char_2_char[sc[i]] = val[0];
                    }
                }
                else {
                    for (int i=0; sc[i]; i++) {
                        char_2_gc_aa[sc[i]] = val[1]-'0' + base_gc;
                        if (val[0] != '=') char_2_char_aa[sc[i]] = val[0];
                    }
                }
            }
            free(val);
            free(sc);
        }
    }

    run_cb();
}

ED4_seq_colors::ED4_seq_colors(int baseGC, void (*changed_cb)()) {
    cb      = changed_cb;
    base_gc = baseGC;

    this->reload();
}

// -----------------------
//      ED4_reference

ED4_reference::ED4_reference(GBDATA *_gb_main)
    : gb_main(_gb_main),
      nodiff('#'), // notused; overwritten by user default later
      mindcase(true),
      ref_len(0),
      reference(NULL),
      ref_term(NULL)
{
    reset_gap_table();
}

ED4_reference::~ED4_reference() {
    clear();
}

void ED4_reference::reset_gap_table() {
    for (int i = 0; i<256; ++i) is_gap[i] = false;
}

void ED4_reference::set_gap_handling(bool mindgaptype, const char *gaptypes) {
    reset_gap_table();
    if (!mindgaptype) { // treat all gaps as "equal"
        for (int i = 0; gaptypes[i]; ++i) {
            is_gap[safeCharIndex(gaptypes[i])] = true;
        }
    }
}

void ED4_reference::expand_to_length(int len) {
    if (len>ref_len && is_set()) {
        char *ref2 = (char *)GB_calloc(sizeof(char), len+1);

        if (reference) {
            strcpy(ref2, reference);
            free(reference);
        }
        reference = ref2;
        ref_len   = len;
    }
}

void ED4_reference::update_data() {
    freeset(reference, ref_term->get_sequence_copy(&ref_len));
}

void ED4_reference::data_changed_cb(ED4_species_manager *IF_ASSERTION_USED(calledFrom)) {
    e4_assert(ref_term);
    if (ref_term) {
#if defined(ASSERTION_USED)
        if (calledFrom) e4_assert(ref_term->get_parent(ED4_L_SPECIES)->to_species_manager() == calledFrom);
#endif
        update_data();
    }
}
static void refdata_changed_cb(ED4_species_manager *sman, ED4_reference *ref) {
    ref->data_changed_cb(sman);
    ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING); // refresh all sequences
}
static void refdata_deleted_cb() {
    ED4_viewDifferences_disable();
}

void ED4_reference::clear() {
    // remove change cb
    if (ref_term) {
        ED4_species_manager *sman = ref_term->get_parent(ED4_L_SPECIES)->to_species_manager();
        sman->remove_sequence_changed_cb(makeED4_species_managerCallback(refdata_changed_cb, this));
        sman->remove_delete_callback(makeED4_managerCallback(refdata_deleted_cb));
    }

    freenull(reference);
    ref_len  = 0;
    ref_term = NULL;
}

void ED4_reference::define(const ED4_sequence_terminal *rterm) {
    clear();
    ref_term = rterm;
    update_data();

    // add change cb
    ED4_species_manager *sman = ref_term->get_parent(ED4_L_SPECIES)->to_species_manager();
    sman->add_sequence_changed_cb(makeED4_species_managerCallback(refdata_changed_cb, this));
    sman->add_delete_callback(makeED4_managerCallback(refdata_deleted_cb));
}

bool ED4_reference::reference_is_a_consensus() const {
    return is_set() && ref_term->is_consensus_sequence_terminal();
}

// --------------------------------------------------------------------------------

#define APREFIX_DIFF_SAVE "edit4/diff/"
#define APREFIX_DIFF_TEMP "tmp/" APREFIX_DIFF_SAVE

#define AWAR_DIFF_TYPE        APREFIX_DIFF_TEMP "type"
#define AWAR_DIFF_NAME        APREFIX_DIFF_TEMP "name"
#define AWAR_NODIFF_INDICATOR APREFIX_DIFF_SAVE "indicator"
#define AWAR_DIFF_MINDCASE    APREFIX_DIFF_SAVE "mindcase"
#define AWAR_DIFF_MINDGAPTYPE APREFIX_DIFF_SAVE "mindgaptype"
#define AWAR_DIFF_GAPTYPES    APREFIX_DIFF_SAVE "gaptypes"

enum ViewDiffType {
    VD_DISABLED,
    VD_SELECTED,
    VD_FOLLOW,
};

static ED4_terminal *detect_current_ref_terminal()  {
    ED4_cursor   *cursor  = &current_cursor();
    ED4_terminal *refTerm = cursor->owner_of_cursor;

    if (refTerm) {
        if (!(refTerm->is_consensus_terminal() ||
              refTerm->is_SAI_terminal() ||
              refTerm->is_species_seq_terminal()))
        {
            refTerm = NULL;
        }
    }

    if (!refTerm) {
        aw_message("Please set the cursor to a species, SAI or group consensus.");
    }

    return refTerm;
}

static void set_diff_reference(ED4_terminal *refTerm) {
    ED4_reference *ref = ED4_ROOT->reference;
    if (!refTerm) {
        ref->clear();
    }
    else {
        ED4_sequence_terminal *refSeqTerm = dynamic_cast<ED4_sequence_terminal*>(refTerm);
        if (refSeqTerm) {
            AW_awar                   *awar_refName = AW_root::SINGLETON->awar(AWAR_DIFF_NAME);
            ED4_species_name_terminal *nameTerm     = refSeqTerm->corresponding_species_name_terminal();

            ref->define(refSeqTerm);
            if (refTerm->is_consensus_terminal()) {
                awar_refName->write_string(GBS_global_string("consensus %s", nameTerm->get_displayed_text()));
            }
            else {
                e4_assert(refTerm->is_species_seq_terminal() || refTerm->is_SAI_terminal());
                awar_refName->write_string(nameTerm->get_displayed_text());
            }
        }
        else {
            aw_message("Not supported for this terminal type");
        }
    }

    ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING);
}

static SmartCharPtr last_used_ref_term_name;

static void set_current_as_diffRef(bool enable) {
    ED4_MostRecentWinContext context;

    ED4_terminal *refTerm = enable ? detect_current_ref_terminal() : NULL;
    if (!enable || refTerm) { // do not disable, if current terminal has wrong type
        set_diff_reference(refTerm);
        if (refTerm) last_used_ref_term_name = strdup(refTerm->id);
    }
}

static void change_reference_cb(AW_window *aww) {
    set_current_as_diffRef(true);

    AW_awar *awar_refType = aww->get_root()->awar(AWAR_DIFF_TYPE);
    if (awar_refType->read_int() == VD_DISABLED) {
        awar_refType->write_int(VD_SELECTED);
    }
}

static void diff_type_changed_cb(AW_root *awr) {
    AW_awar      *awar_refType = awr->awar(AWAR_DIFF_TYPE);
    ViewDiffType  type         = ViewDiffType(awar_refType->read_int());

    switch (type) {
        case VD_DISABLED:
            set_current_as_diffRef(false);
            break;

        case VD_FOLLOW:
        case VD_SELECTED: {
            ED4_terminal *last_used_ref_term = NULL;
            if (last_used_ref_term_name.isSet()) {
                ED4_base *found = ED4_ROOT->main_manager->search_ID(&*last_used_ref_term_name);
                if (found && found->is_terminal()) {
                    last_used_ref_term = found->to_terminal();
                }
            }
            if (last_used_ref_term) {
                set_diff_reference(last_used_ref_term);
                if (type == VD_FOLLOW) set_current_as_diffRef(true);
            }
            else {
                set_current_as_diffRef(true);
            }
            if (!ED4_ROOT->reference->is_set()) {
                awar_refType->write_int(VD_DISABLED);
            }
            break;
        }
    }
}

static void update_reference_settings(AW_root *awr) {
    ED4_reference *ref = ED4_ROOT->reference;
    ref->set_nodiff_indicator(awr->awar(AWAR_NODIFF_INDICATOR)->read_char_pntr()[0]);
    ref->set_case_sensitive(awr->awar(AWAR_DIFF_MINDCASE)->read_int());
    ref->set_gap_handling(awr->awar(AWAR_DIFF_MINDGAPTYPE)->read_int(), awr->awar(AWAR_DIFF_GAPTYPES)->read_char_pntr());
}
static void diff_setting_changed_cb(AW_root *awr) {
    update_reference_settings(awr);
    ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING);
}

static void nodiff_indicator_changed_cb(AW_root *awr) {
    AW_awar    *awar_indicator = awr->awar(AWAR_NODIFF_INDICATOR);
    const char *indicator      = awar_indicator->read_char_pntr();

    if (!indicator[0]) {
        awar_indicator->write_string(" ");
    }
    else {
        diff_setting_changed_cb(awr);
    }
}

static bool viewDifferences_awars_initialized = false;

static void create_viewDifferences_awars(AW_root *awr) {
    if (!viewDifferences_awars_initialized) {
        awr->awar_int(AWAR_DIFF_TYPE, VD_DISABLED)->add_callback(diff_type_changed_cb);
        awr->awar_string(AWAR_DIFF_NAME, "<none selected>");
        awr->awar_string(AWAR_NODIFF_INDICATOR, " ")->add_callback(nodiff_indicator_changed_cb)->set_srt(" ?=?:? =?:?*=?");
        awr->awar_int(AWAR_DIFF_MINDCASE, 1)->add_callback(diff_setting_changed_cb);
        awr->awar_int(AWAR_DIFF_MINDGAPTYPE, 1)->add_callback(diff_setting_changed_cb);
        awr->awar_string(AWAR_DIFF_GAPTYPES, "-=.?")->add_callback(diff_setting_changed_cb);

        viewDifferences_awars_initialized = true;
    }
}

void ED4_toggle_viewDifferences(AW_root *awr) {
    static ViewDiffType lastActiveType = VD_SELECTED;

    create_viewDifferences_awars(awr);
    update_reference_settings(awr);

    AW_awar      *awar_difftype = awr->awar(AWAR_DIFF_TYPE);
    ViewDiffType  currType      = ViewDiffType(awar_difftype->read_int());

    if (currType == VD_DISABLED || !ED4_ROOT->reference->is_set()) {
        currType = lastActiveType;
    }
    else {
        lastActiveType = currType;
        currType       = VD_DISABLED;
    }

    awar_difftype->rewrite_int(currType); // rewrite to allow activation after automatic deactivation in ED4_reference (e.g. by killing ref-term)
}
void ED4_viewDifferences_setNewReference() {
    set_current_as_diffRef(true);
}
void ED4_viewDifferences_announceTerminalChange() {
    if (ED4_ROOT->reference->is_set() &&
        ED4_ROOT->aw_root->awar(AWAR_DIFF_TYPE)->read_int() == VD_FOLLOW)
    {
        ED4_viewDifferences_setNewReference();
    }
}
void ED4_viewDifferences_disable() {
    set_current_as_diffRef(false);
}

AW_window *ED4_create_viewDifferences_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        if (!ED4_ROOT->reference->is_set()) ED4_toggle_viewDifferences(awr); // automatically activate if off

        aws = new AW_window_simple;
        aws->init(awr, "VIEW_DIFF", "View sequence differences");
        aws->load_xfig("edit4/viewdiff.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("viewdiff.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("show");
        aws->create_toggle_field(AWAR_DIFF_TYPE);
        aws->insert_toggle("None (=disable)", "N", VD_DISABLED);
        aws->insert_toggle("Selected:",       "S", VD_SELECTED);
        aws->insert_toggle("Follow cursor",   "F", VD_FOLLOW);
        aws->update_toggle_field();

        aws->at("ref");
        aws->button_length(20);
        aws->create_button(NULL, AWAR_DIFF_NAME, NULL, "+");

        aws->at("set");
        aws->button_length(4);
        aws->callback(change_reference_cb);
        aws->create_button("SET", "SET");

        aws->at("nodiff");
        aws->create_input_field(AWAR_NODIFF_INDICATOR);

        aws->at("case");
        aws->create_toggle(AWAR_DIFF_MINDCASE);

        aws->at("gap");
        aws->create_toggle(AWAR_DIFF_MINDGAPTYPE);
        aws->at("gapchars");
        aws->create_input_field(AWAR_DIFF_GAPTYPES, 8);
    }
    return aws;
}
