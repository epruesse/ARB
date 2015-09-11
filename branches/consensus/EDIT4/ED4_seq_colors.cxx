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
    "=0=5=0=5=7=7=0=5=7=7=3=7=3=9=7=7=7=3=3=0=0=5=3=7=3=7=0=6", // ambiguity
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


ED4_reference::ED4_reference(GBDATA *_gb_main) {
    reference = 0;
    ref_len = 0;
    gb_main = _gb_main;
    init_species_name = 0;
}

void ED4_reference::clear() {
    free(reference);
    reference = 0;
    ref_len = 0;
    delete init_species_name;
    init_species_name = 0;
}

void ED4_reference::expand_to_length(int len) {
    if (len > ref_len) {
        char *ref2 = (char *)GB_calloc(sizeof(char), len+1);

        if (reference) {
            strcpy(ref2, reference);
            free(reference);
        }
        reference = ref2;
        ref_len   = len;
    }
}

void ED4_reference::define(const char *species_name, const char *alignment_name) {
    e4_assert(species_name);
    e4_assert(alignment_name);

    GB_transaction ta(gb_main);
    GBDATA *gb_species = GBT_find_species(gb_main, species_name);

    clear();
    if (gb_species) {
        GBDATA *gb_data = GBT_find_sequence(gb_species, alignment_name);
        if (gb_data) {
            reference = GB_read_as_string(gb_data);
            if (reference) {
                ref_len = strlen(reference);
                init_species_name = strdup(species_name);
            }
        }
    }
}

void ED4_reference::define(const char *name, const char *sequence_data, int len) {
    e4_assert(name);
    e4_assert(sequence_data);
    e4_assert(len>0);

    clear();

    reference = (char*)GB_calloc(sizeof(char), len+1);
    memcpy(reference, sequence_data, len);
    reference[len] = 0;
    ref_len = len;
    init_species_name = strdup(name);
}

ED4_reference::~ED4_reference() {
    free(reference);
}

// --------------------------------------------------------------------------------

#define APREFIX_DIFF_SAVE "edit4/diff/"
#define APREFIX_DIFF_TEMP "tmp/" APREFIX_DIFF_SAVE

#define AWAR_DIFF_TYPE APREFIX_DIFF_TEMP "type"
#define AWAR_DIFF_NAME APREFIX_DIFF_TEMP "name"

enum ViewDiffType {
    VD_DISABLED,
    VD_SELECTED,
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
        AW_awar                   *awar_refName = AW_root::SINGLETON->awar(AWAR_DIFF_NAME);
        ED4_species_name_terminal *nameTerm     = dynamic_cast<ED4_abstract_sequence_terminal*>(refTerm)->corresponding_species_name_terminal();

        if (refTerm->is_consensus_terminal()) {
            ED4_char_table *table     = &refTerm->get_parent(ED4_L_GROUP)->to_group_manager()->table();
            char           *consensus = table->build_consensus_string();

            ref->define("CONSENSUS", consensus, table->size()); // @@@ need different names for diff. consensi // @@@ clashes with SAI:CONSENSUS

            awar_refName->write_string(GBS_global_string("consensus %s", nameTerm->get_displayed_text()));
            free(consensus);
        }
        else if (refTerm->is_SAI_terminal()) {
            char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SAI_NAME); // @@@ read from refTerm or its manager
            int   datalen;
            char *data = refTerm->resolve_pointer_to_string_copy(&datalen);

            ref->define(name, data, datalen);
            awar_refName->write_string(nameTerm->get_displayed_text());

            free(data);
            free(name);

        }
        else {
            e4_assert(refTerm->is_species_seq_terminal());
            char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SPECIES_NAME); // @@@ read from refTerm or its manager

            ref->define(name, ED4_ROOT->alignment_name);
            awar_refName->write_string(nameTerm->get_displayed_text());

            free(name);
        }
    }

    ED4_ROOT->request_refresh_for_sequence_terminals();
}

static ED4_terminal *last_used_ref_term = NULL;

static void set_current_as_diffRef(bool enable) {
    ED4_MostRecentWinContext context;
    ED4_terminal *refTerm = enable ? detect_current_ref_terminal() : NULL;
    set_diff_reference(refTerm);

    if (refTerm) last_used_ref_term = refTerm;
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

        case VD_SELECTED:
            if (last_used_ref_term) {
                set_diff_reference(last_used_ref_term);
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

static void create_viewDifferences_awars(AW_root *awr) {
    awr->awar_int(AWAR_DIFF_TYPE, VD_DISABLED)->add_callback(diff_type_changed_cb);
    awr->awar_string(AWAR_DIFF_NAME, "<none selected>");
}
AW_window *ED4_create_viewDifferences_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        create_viewDifferences_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "VIEW_DIFF", "View sequence differences");
        aws->load_xfig("edit4/viewdiff.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("sequence_colors.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("show");
        aws->create_toggle_field(AWAR_DIFF_TYPE);
        aws->insert_toggle("None (=disable)", "N", VD_DISABLED);
        aws->insert_toggle("Selected",        "S", VD_SELECTED);
        aws->update_toggle_field();

        aws->at("ref");
        aws->button_length(20);
        aws->create_button(NULL, AWAR_DIFF_NAME, NULL, "+");

        aws->at("set");
        aws->button_length(4);
        aws->callback(change_reference_cb);
        aws->create_button("SET", "SET");
    }
    return aws;
}
