// ============================================================== //
//                                                                //
//   File      : ED4_flags.cxx                                    //
//   Purpose   : species flag implementation                      //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2016   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "ed4_flags.hxx"
#include "ed4_class.hxx"

#include <item_sel_list.h>
#include <awt_config_manager.hxx>

#include <aw_awar.hxx>
#include <aw_root.hxx>

#include <arbdbt.h>

#include <arb_strbuf.h>

using namespace std;

#define MAX_SPECIES_FLAGS 5

#define AWAR_FLAGS_PREFIX         "arb_edit/flags" // in main database
#define AWAR_FLAGS_ENABLED        AWAR_FLAGS_PREFIX "/display"
#define AWAR_FLAG_PREFIX_TEMPLATE AWAR_FLAGS_PREFIX "/flag%i"
#define AWAR_FLAG_ENABLE_TEMPLATE AWAR_FLAG_PREFIX_TEMPLATE "/enable"
#define AWAR_FLAG_HEADER_TEMPLATE AWAR_FLAG_PREFIX_TEMPLATE "/header"
#define AWAR_FLAG_FIELD_TEMPLATE  AWAR_FLAG_PREFIX_TEMPLATE "/field"

#define MAX_AWARNAME_LENGTH (8+1+5+1+5+1+6)

inline const char *awarname(const char *awarname_template, int idx) {
    e4_assert(idx>=0 && idx<MAX_SPECIES_FLAGS);
    static char buffer[MAX_AWARNAME_LENGTH+1];

#if defined(ASSERTION_USED)
    int printed =
#endif
        sprintf(buffer, awarname_template, idx);
    e4_assert(printed<=MAX_AWARNAME_LENGTH);

    return buffer;
}

static void header_changed_cb(AW_root *) {
    SpeciesFlags::forget();
    ED4_request_relayout();
}

static void init_flag_awars() {
    static bool initialized = false;

    if (!initialized) {
        AW_root *awr = AW_root::SINGLETON;
        GBDATA  *db  = GLOBAL_gb_main;

        awr->awar_int(AWAR_FLAGS_ENABLED, 0, db)->add_callback(header_changed_cb);

        for (int i = 0; i<MAX_SPECIES_FLAGS; ++i) {
            awr->awar_int   (awarname(AWAR_FLAG_ENABLE_TEMPLATE, i), 0, db)->add_callback(header_changed_cb);
            awr->awar_string(awarname(AWAR_FLAG_HEADER_TEMPLATE, i), 0, db)->add_callback(header_changed_cb);
            awr->awar_string(awarname(AWAR_FLAG_FIELD_TEMPLATE,  i), 0, db)->add_callback(header_changed_cb);
        }
    }
}

// --------------------------------------------------------------------------------

const char *SpeciesFlag::prepare_itemfield() const {
    /*! setup DB field for flag (create changekey)
     * @return fieldname if changekey exists or has been created
     * otherwise error is exported
     */

    const char *awar_name     = awarname(AWAR_FLAG_FIELD_TEMPLATE, awar_index);
    const char *reg_awar_name = get_itemfield_type_awarname(awar_name);

    const char *key   = NULL;
    GB_ERROR    error = NULL;

    AW_root       *awr      = AW_root::SINGLETON;
    GBDATA        *gb_main  = GLOBAL_gb_main;
    ItemSelector&  selector = SPECIES_get_selector();

    if (reg_awar_name) { // field selection was used (normal case) -> create field as specified by user
        key   = prepare_and_get_selected_itemfield(awr, awar_name, gb_main, selector, FIF_STANDARD);
        error = GB_incur_error_if(!key);
    }
    else { // field was stored in properties -> check changekey
        key           = awr->awar(awar_name)->read_char_pntr();
        GB_TYPES type = GBT_get_type_of_changekey(gb_main, key, selector.change_key_path);

        if (type == GB_NONE) { // changekey does not exist -> default to type GB_INT
            error          = GBT_add_new_changekey_to_keypath(gb_main, key, GB_INT, selector.change_key_path);
            if (error) key = NULL;
        }
    }

    e4_assert(correlated(!key, error));
    if (error) GB_export_errorf("Failed to prepare flag-field (Reason: %s)", error);

    return key;
}

// --------------------------------------------------------------------------------

SpeciesFlags *SpeciesFlags::SINGLETON = NULL;

void SpeciesFlags::create_instance() {
    e4_assert(!SINGLETON);
    SINGLETON = new SpeciesFlags;

    init_flag_awars();

    {
        // create shown flags from AWAR state:
        AW_root *awr = AW_root::SINGLETON;

        if (awr->awar(AWAR_FLAGS_ENABLED)->read_int()) {
            for (int f = 0; f<MAX_SPECIES_FLAGS; ++f) {
                if (awr->awar(awarname(AWAR_FLAG_ENABLE_TEMPLATE, f))->read_int()) {
                    const char *abbr  = awr->awar(awarname(AWAR_FLAG_HEADER_TEMPLATE, f))->read_char_pntr();
                    const char *field = awr->awar(awarname(AWAR_FLAG_FIELD_TEMPLATE,  f))->read_char_pntr();

                    if (!abbr[0]) abbr = "?";

                    SINGLETON->push_back(SpeciesFlag(abbr, field, f));
                }
            }
        }
    }
}

void SpeciesFlags::build_header_text() const {
    GBS_strstruct buf(30);

    bool first = true;
    for (SpeciesFlagCiter i = begin(); i != end(); ++i) {
        if (!first) buf.put(' ');
        const SpeciesFlag& flag = *i;
        buf.cat(flag.get_shortname().c_str());
        first = false;
    }

    header = buf.release();
}

void SpeciesFlags::calculate_header_dimensions(AW_device *device, int gc) {
    int space_width = device->get_string_size(gc, " ", 1);
    int xpos        = CHARACTEROFFSET; // x-offset of first character of header-text

    const SpeciesFlag *prev_flag = NULL;

    min_flag_distance = INT_MAX;

    for (SpeciesFlagIter i = begin(); i != end(); ++i) {
        SpeciesFlag&  flag       = *i;
        const string& shortname  = flag.get_shortname();
        int           text_width = device->get_string_size(gc, shortname.c_str(), shortname.length());

        flag.set_dimension(xpos, text_width);
        xpos += text_width+space_width;

        if (prev_flag) {
            min_flag_distance = std::min(min_flag_distance, int(floor(flag.center_xpos()-prev_flag->center_xpos())));
        }
        else { // first
            min_flag_distance = std::min(min_flag_distance, int(floor(2*flag.center_xpos())));
        }
        prev_flag = &flag;
    }

    pixel_width = xpos - space_width + 1 + 1;       // +1 (pos->width) +1 (avoid character clipping)

    e4_assert(prev_flag);
    min_flag_distance = std::min(min_flag_distance, int(floor(2*((pixel_width-1)-prev_flag->center_xpos()))));
}

inline const char *settingName(const char *name, int idx) {
    e4_assert(idx>=0 && idx<MAX_SPECIES_FLAGS);
    const int   BUFSIZE = 20;
    static char buffer[BUFSIZE];
#if defined(ASSERTION_USED)
    int printed =
#endif
        sprintf(buffer, "%s%i", name, idx);
    e4_assert(printed<BUFSIZE);
    return buffer;
}

static void setup_species_flags_config(AWT_config_definition& cfg) {
    cfg.add(AWAR_FLAGS_ENABLED, "display");
    for (int i = 0; i<MAX_SPECIES_FLAGS; ++i) {
        cfg.add(awarname(AWAR_FLAG_ENABLE_TEMPLATE, i), settingName("enable", i));
        cfg.add(awarname(AWAR_FLAG_HEADER_TEMPLATE, i), settingName("header", i));
        cfg.add(awarname(AWAR_FLAG_FIELD_TEMPLATE,  i), settingName("field",  i));
    }
}

AW_window *ED4_configure_species_flags(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "SPECIES_FLAGS", "Species flags");

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("ed4_flags.hlp"));
    aws->create_button("HELP", "HELP");

    int cfg_x, cfg_y;
    aws->get_at_position(&cfg_x, &cfg_y);

    aws->at_newline();

    aws->label("Display flags");
    aws->create_toggle(AWAR_FLAGS_ENABLED);

    aws->at_newline();

    // header will be added at the current position (after looping over flags)
    const int COLUMNS      = 3;
    const int HEADERHEIGHT = 20;

    int header_y = aws->get_at_yposition();
    int header_x[COLUMNS];

    const char *headertext[COLUMNS] = {
        "on?",
        "abbreviation",
        "field to display",
    };

    aws->at_y(header_y+HEADERHEIGHT);

    for (int i = 0; i<MAX_SPECIES_FLAGS; ++i) {
        aws->at_newline();

        if (!i) header_x[0] = aws->get_at_xposition();
        aws->create_toggle(awarname(AWAR_FLAG_ENABLE_TEMPLATE, i));

        if (!i) header_x[1] = aws->get_at_xposition();
        aws->create_input_field(awarname(AWAR_FLAG_HEADER_TEMPLATE, i), 10);

        if (!i) header_x[2] = aws->get_at_xposition();
        FieldSelDef field_sel(awarname(AWAR_FLAG_FIELD_TEMPLATE, i), gb_main, SPECIES_get_selector(), FIELD_FILTER_BYTE_WRITEABLE, "flag field", SF_ALLOW_NEW);
        create_itemfield_selection_button(aws, field_sel, NULL);
    }

    for (int h = 0; h<COLUMNS; ++h)  {
        aws->at(header_x[h], header_y);
        aws->create_autosize_button(0, headertext[h]);
    }

    aws->at(cfg_x, cfg_y);
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "species_flags", makeConfigSetupCallback(setup_species_flags_config));

    return aws;
}
