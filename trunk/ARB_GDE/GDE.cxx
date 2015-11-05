#include "GDE_extglob.h"
#include "GDE_awars.h"

#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <aw_msg.hxx>
#include <aw_awar.hxx>
#include <aw_file.hxx>
#include <aw_root.hxx>
#include <aw_awar_defs.hxx>
#include <aw_select.hxx>

#include <arb_str.h>

#include <cmath>
#include <awt_config_manager.hxx>

// AISC_MKPT_PROMOTE:#ifndef GDE_MENU_H
// AISC_MKPT_PROMOTE:#include "GDE_menu.h"
// AISC_MKPT_PROMOTE:#endif

adfiltercbstruct *agde_filter = 0;

Gmenu menu[GDEMAXMENU];
int   num_menus = 0;

static char GDEBLANK[] = "\0";

#define SLIDERWIDTH 5           // input field width for numbers

#define AWAR_GDE_ALIGNMENT   AWAR_PREFIX_GDE_TEMP "/alignment"
#define AWAR_GDE_FILTER_NAME AWAR_PREFIX_GDE_TEMP "/filter/name"

struct gde_iteminfo {
    GmenuItem *item;
    int        idx;
    gde_iteminfo(GmenuItem *item_, int idx_) : item(item_), idx(idx_) {}
};

static void GDE_showhelp_cb(AW_window *aw, GmenuItem *gmenuitem) {
    if (gmenuitem->help) {
        AW_help_popup(aw, gmenuitem->help);
    }
    else {
        aw_message("Sorry - no help available (please report to devel@arb-home.de)");
    }
}

static char *GDE_makeawarname_in(GmenuItem *gmenuitem, long i, const char *awar_root) {
    char *gmenu_label     = GBS_string_2_key(gmenuitem->parent_menu->label);
    char *gmenuitem_label = GBS_string_2_key(gmenuitem->label);
    char *arg             = GBS_string_2_key(gmenuitem->arg[i].symbol);

    char *name = GBS_global_string_copy("%s/%s/%s/%s", awar_root, gmenu_label, gmenuitem_label, arg);

    free(gmenu_label);
    free(gmenuitem_label);
    free(arg);

    return name;
}

char *GDE_makeawarname   (GmenuItem *gmenuitem, long i) { return GDE_makeawarname_in(gmenuitem, i, AWAR_PREFIX_GDE); }
char *GDE_maketmpawarname(GmenuItem *gmenuitem, long i) { return GDE_makeawarname_in(gmenuitem, i, AWAR_PREFIX_GDE_TEMP); }

static void GDE_slide_awar_int_cb(AW_window *aws, const char *awar_name, int diff) {
    AW_awar *awar = aws->get_root()->awar(awar_name);
    awar->write_int(awar->read_int()+diff);
}

static void GDE_slide_awar_float_cb(AW_window *aws, const char *awar_name, int millidiff) {
    AW_awar *awar    = aws->get_root()->awar(awar_name);
    double   diff    = millidiff/1000.0;
    double   new_val = awar->read_float()+diff;

    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);

    // do it again (otherwise internal awar-range correction sometimes leads to 1+eXX values)
    new_val = awar->read_float();
    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);
}

static void GDE_create_infieldwithpm(AW_window *aws, char *newawar, long width) {
    aws->create_input_field(newawar, (int)width);
    if (aws->get_root()->awar(newawar)->get_type() == AW_INT) {
        aws->button_length(3);
        char *awar = strdup(newawar);
        aws->callback(makeWindowCallback(GDE_slide_awar_int_cb, awar, -1)); aws->create_button(0, "-", "-");
        aws->callback(makeWindowCallback(GDE_slide_awar_int_cb, awar, +1)); aws->create_button(0, "+", "+");
    }
    else if (aws->get_root()->awar(newawar)->get_type() == AW_FLOAT) {
        aws->button_length(3);
        char *awar = strdup(newawar);
        aws->callback(makeWindowCallback(GDE_slide_awar_float_cb, awar, -100)); aws->create_button(0, "-", "-");
        aws->callback(makeWindowCallback(GDE_slide_awar_float_cb, awar, +100)); aws->create_button(0, "+", "+");
    }
}

static char *gde_filter_weights(GBDATA *gb_sai) {
    char   *ali_name = GBT_get_default_alignment(GB_get_root(gb_sai));
    GBDATA *gb_ali   = GB_entry(gb_sai, ali_name);
    char   *result   = 0;

    if (gb_ali) {
        GBDATA *gb_type = GB_entry(gb_ali, "_TYPE");
        if (gb_type) {
            const char *type = GB_read_char_pntr(gb_type);

            if (GBS_string_matches(type, "PV?:*", GB_MIND_CASE)) {
                result = GBS_global_string_copy("%s: %s", GBT_read_name(gb_sai), type);
            }

        }
    }

    free(ali_name);
    return result;

}

static void refresh_weights_sellist_cb(AW_root*, AW_DB_selection *saisel) {
    saisel->refresh();
}

static AW_window *GDE_create_filename_browser_window(AW_root *aw_root, const char *awar_prefix, const char *title) {
    AW_window_simple *aws = new AW_window_simple;

    {
        char *wid = GBS_string_2_key(awar_prefix);
        aws->init(aw_root, wid, title);
        free(wid);
    }
    aws->load_xfig("sel_box.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    AW_create_standard_fileselection(aws, awar_prefix);

    return aws;
}

static void GDE_popup_filename_browser(AW_window *aw, gde_iteminfo *info, const char *title) {
    GmenuItem *gmenuitem = info->item;
    int        idx       = info->idx;
    char      *base_awar = GDE_maketmpawarname(gmenuitem, idx);

    static GB_HASH *popup_hash  = NULL;
    if (!popup_hash) popup_hash = GBS_create_hash(20, GB_MIND_CASE);

    AW_window *aw_browser = (AW_window*)GBS_read_hash(popup_hash, base_awar);
    if (!aw_browser) {
        aw_browser        = GDE_create_filename_browser_window(aw->get_root(), base_awar, title);
        GBS_write_hash(popup_hash, base_awar, (long)aw_browser);
    }
    aw_browser->activate();
    free(base_awar);
}

inline bool shall_store_in_config(const GmenuItemArg& itemarg) {
    return itemarg.type != FILE_SELECTOR;
}
inline bool want_config_manager(GmenuItem *gmenuitem) {
    for (int i=0; i<gmenuitem->numargs; i++) {
        const GmenuItemArg& itemarg = gmenuitem->arg[i];
        if (shall_store_in_config(itemarg)) return true;
    }
    return false;
}
static void setup_gde_config_def(AWT_config_definition& cdef, GmenuItem *gmenuitem) {
    for (int i=0; i<gmenuitem->numargs; i++) {
        const GmenuItemArg& itemarg = gmenuitem->arg[i];
        if (shall_store_in_config(itemarg)) {
            char *awar = GDE_makeawarname(gmenuitem, i);

            gde_assert(awar);
            gde_assert(itemarg.symbol);

            if (awar) {
                cdef.add(awar, itemarg.symbol);
                free(awar);
            }
        }
    }
}

static AW_window *GDE_menuitem_cb(AW_root *aw_root, GmenuItem *gmenuitem) {
#define BUFSIZE 200
    char bf[BUFSIZE+1];
    IF_ASSERTION_USED(int printed =)
        sprintf(bf, "GDE / %s / %s", gmenuitem->parent_menu->label, gmenuitem->label);

    gde_assert(printed<=BUFSIZE);
    char seqtype = gmenuitem->seqtype;

    if (gmenuitem->aws == NULL) {
        AW_window_simple *aws = new AW_window_simple;
        aws->init(aw_root, bf, bf);

        switch (db_access.window_type) {
            case GDE_WINDOWTYPE_DEFAULT: {
                if (seqtype == '-') aws->load_xfig("gdeitem_simple.fig");
                else                aws->load_xfig("gdeitem.fig");
                break;
            }
            case GDE_WINDOWTYPE_EDIT4:
                gde_assert(seqtype != '-');
                aws->load_xfig("gde3item.fig");
                break;
        }

        aws->button_length(10);
        aws->at(10, 10);
        aws->auto_space(0, 10);

        aws->at("help");
        aws->callback(makeWindowCallback(GDE_showhelp_cb, gmenuitem));
        aws->create_button("GDE_HELP", "HELP", "H");

        aws->at("start");
        aws->callback(makeWindowCallback(GDE_startaction_cb, gmenuitem));
        aws->create_button("GO", "GO", "O");

        aws->at("cancel");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        if (want_config_manager(gmenuitem)) {
            aws->at("config");
            AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, aws->window_defaults_name, makeConfigSetupCallback(setup_gde_config_def, gmenuitem));
        }

        if (gmenuitem->numinputs>0) {
            switch (db_access.window_type) {
                case GDE_WINDOWTYPE_DEFAULT: {
                    if (seqtype != '-') { // '-' means "skip sequence export"
                        aws->at("which_alignment");
                        const char *ali_filter = seqtype == 'A' ? "pro=:ami=" : (seqtype == 'N' ? "dna=:rna=" : "*=");
                        awt_create_ALI_selection_list(db_access.gb_main, (AW_window *)aws, AWAR_GDE_ALIGNMENT, ali_filter);

                        aws->at("which_species");
                        aws->create_toggle_field(AWAR_GDE_SPECIES);
                        aws->insert_toggle        ("all",    "a", 0);
                        aws->insert_default_toggle("marked", "m", 1);
                        aws->update_toggle_field();

                        if (seqtype != 'N') {
                            aws->at("stop_codon");
                            aws->label("Cut stop-codon");
                            aws->create_toggle(AWAR_GDE_CUTOFF_STOPCODON);
                        }
                    }
                    break;
                }
                case GDE_WINDOWTYPE_EDIT4:
                    aws->at("topk");      aws->create_toggle("gde/top_area_kons");
                    aws->at("middlek");   aws->create_toggle("gde/middle_area_kons");
                    aws->at("topr");      aws->create_toggle("gde/top_area_remark");
                    aws->at("middler");   aws->create_toggle("gde/middle_area_remark");
                    aws->at("top");       aws->create_toggle("gde/top_area");
                    aws->at("topsai");    aws->create_toggle("gde/top_area_sai");
                    aws->at("toph");      aws->create_toggle("gde/top_area_helix");
                    aws->at("middle");    aws->create_toggle("gde/middle_area");
                    aws->at("middlesai"); aws->create_toggle("gde/middle_area_sai");
                    aws->at("middleh");   aws->create_toggle("gde/middle_area_helix");
                    break;
            }

            if (seqtype != '-') {
                aws->at("compression");
                aws->create_option_menu(AWAR_GDE_COMPRESSION, true);
                aws->insert_option        ("none",             "n", COMPRESS_NONE);
                aws->insert_option        ("vertical gaps",    "v", COMPRESS_VERTICAL_GAPS);
                aws->insert_default_option("columns w/o info", "i", COMPRESS_NONINFO_COLUMNS);
                aws->insert_option        ("all gaps",         "g", COMPRESS_ALL);
                aws->update_option_menu();

                aws->button_length(12);
                aws->at("filtername");
                if (!agde_filter) { // create only one filter - used for all GDE calls
                    agde_filter = awt_create_select_filter(aws->get_root(), db_access.gb_main, AWAR_GDE_FILTER_NAME);
                }
                aws->callback(makeCreateWindowCallback(awt_create_select_filter_win, agde_filter));
                aws->create_button("SELECT_FILTER", AWAR_GDE_FILTER_NAME);
            }

            aws->at("paramsb");
        }
        else {
            aws->at("paramsb");
        }


        int labellength = 1;
        for (int i=0; i<gmenuitem->numargs; i++) {
            if (!(gmenuitem->arg[i].label)) gmenuitem->arg[i].label = GDEBLANK;

            const char *label    = gmenuitem->arg[i].label;
            const char *linefeed = strchr(label, '\n');

            int lablen;
            while (linefeed) {
                lablen = linefeed-label;
                if (lablen>labellength) {
                    labellength = lablen;
                }
                label    = linefeed+1;
                linefeed = strchr(label, '\n');
            }

            lablen = strlen(label);
            if (lablen>labellength) {
                labellength = lablen;
            }
        }
        aws->label_length(labellength);
        aws->auto_space(0, 0);

        for (int i=0; i<gmenuitem->numargs; i++) {
            const GmenuItemArg& itemarg = gmenuitem->arg[i];

            if (itemarg.type==SLIDER) {
                char *newawar = GDE_makeawarname(gmenuitem, i);

                if (int(itemarg.fvalue) == itemarg.fvalue &&
                    int(itemarg.min) == itemarg.min &&
                    int(itemarg.max) == itemarg.max)
                {
                    aw_root->awar_int(newawar, (long)itemarg.fvalue, AW_ROOT_DEFAULT);
                }
                else {
                    aw_root->awar_float(newawar, itemarg.fvalue, AW_ROOT_DEFAULT);
                }
                aw_root->awar(newawar)->set_minmax(itemarg.min, itemarg.max);
                aws->label(itemarg.label);
                aws->sens_mask(itemarg.active_mask);
                GDE_create_infieldwithpm(aws, newawar, SLIDERWIDTH);
                // maybe bound checking //
                free(newawar);
            }
            else if (itemarg.type==CHOOSER) {
                char    *defopt           = itemarg.choice[0].method;
                char    *newawar          = GDE_makeawarname(gmenuitem, i);
                AW_awar *curr_awar        = aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                char    *curr_value       = curr_awar->read_string();
                bool     curr_value_legal = false;

                aws->label(itemarg.label);
                aws->sens_mask(itemarg.active_mask);
                if ((strcasecmp(itemarg.choice[0].label, "no") == 0) ||
                    (strcasecmp(itemarg.choice[0].label, "yes") == 0))
                {
                    aws->create_toggle_field(newawar, 1);
                }
                else {
                    aws->create_toggle_field(newawar);
                }

                for (long j=0; j<itemarg.numchoices; j++) {
                    if (strcmp(itemarg.choice[j].method, curr_value) == 0) curr_value_legal = true;

                    if (!j) {
                        aws->insert_default_toggle(itemarg.choice[j].label, "1", itemarg.choice[j].method);
                    }
                    else {
                        aws->insert_toggle(itemarg.choice[j].label, "1", itemarg.choice[j].method);
                    }
                }
                if (!curr_value_legal) curr_awar->write_string(defopt); // if saved value no longer occurs in choice -> overwrite with default
                free(curr_value);
                aws->update_toggle_field();
                free(newawar);
            }
            else if (itemarg.type==CHOICE_MENU) {
                char    *defopt           = itemarg.choice[itemarg.ivalue].method;
                char    *newawar          = GDE_makeawarname(gmenuitem, i);
                AW_awar *curr_awar        = aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                char    *curr_value       = curr_awar->read_string();
                bool     curr_value_legal = false;

                if (itemarg.label[0]) aws->label(itemarg.label);
                aws->sens_mask(itemarg.active_mask);
                aws->create_option_menu(newawar, true);

                for (long j=0; j<itemarg.numchoices; j++) {
                    if (strcmp(itemarg.choice[j].method, curr_value) == 0) curr_value_legal = true;
                    aws->insert_option(itemarg.choice[j].label, "1", itemarg.choice[j].method);
                }
                if (!curr_value_legal) curr_awar->write_string(defopt); // if saved value no longer occurs in choice -> overwrite with default
                free(curr_value);
                aws->update_option_menu();
                free(newawar);
            }
            else if (itemarg.type==TEXTFIELD) {
                char *defopt  = itemarg.textvalue;
                char *newawar = GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->label(itemarg.label);
                aws->sens_mask(itemarg.active_mask);
                aws->create_input_field(newawar, itemarg.textwidth);  // TEXTFIELDWIDTH
                free(newawar);
            }
            else if (itemarg.type==FILE_SELECTOR) {
                char *base_awar = GDE_maketmpawarname(gmenuitem, i);
                char *name_awar = GBS_global_string_copy("%s/file_name", base_awar);

                AW_create_fileselection_awars(aw_root, base_awar, "", itemarg.textvalue, "");

                aws->label(itemarg.label);
                aws->sens_mask(itemarg.active_mask);
                aws->create_input_field(name_awar, 40);
                aws->callback(makeWindowCallback(GDE_popup_filename_browser, new gde_iteminfo(gmenuitem, i), strdup(itemarg.label)));
                aws->create_button("", "Browse");

                free(name_awar);
                free(base_awar);
            }
            else if (itemarg.type==CHOICE_TREE) {
                char *defopt  = itemarg.textvalue;
                char *newawar = GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->sens_mask(itemarg.active_mask);
                if (itemarg.label[0]) aws->create_button(NULL, itemarg.label);
                awt_create_TREE_selection_list(db_access.gb_main, aws, newawar, true);
                free(newawar);
            }
            else if (itemarg.type==CHOICE_SAI) {
                char *defopt  = itemarg.textvalue;
                char *newawar = GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->sens_mask(itemarg.active_mask);
                if (itemarg.label[0]) aws->create_button(NULL, itemarg.label);
                awt_create_SAI_selection_list(db_access.gb_main, aws, newawar, true);
                free(newawar);
            }
            else if (itemarg.type==CHOICE_WEIGHTS) {
                char *defopt  = itemarg.textvalue;
                char *newawar = GDE_makeawarname(gmenuitem, i);

                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->sens_mask(itemarg.active_mask);
                if (itemarg.label[0]) aws->create_button(NULL, itemarg.label);
                AW_DB_selection *saisel = awt_create_SAI_selection_list(db_access.gb_main, aws, newawar, true, makeSaiSelectionlistFilterCallback(gde_filter_weights));
                free(newawar);
                aw_root->awar(AWAR_GDE_ALIGNMENT)->add_callback(makeRootCallback(refresh_weights_sellist_cb, saisel));
            }

            aws->at_newline();
        }
        aws->at_newline();
        aws->window_fit();

        gmenuitem->aws = aws;
    }
    return gmenuitem->aws;
#undef BUFSIZE
}




void GDE_load_menu(AW_window *awm, AW_active /*mask*/, const char *menulabel) {
    // Load GDE menu items.
    //
    // If 'menulabel' == NULL -> load all menus
    // Else                   -> load specified menu
    //
    // Always loads complete menu(s).

    gde_assert(db_access.gb_main); // forgot to call GDE_init() ?

    char hotkey[]   = "x";
    bool menuloaded = false;

    for (long nmenu = 0; nmenu<num_menus; nmenu++) {
        {
            const char *menuname = menu[nmenu].label;
            if (menulabel) {
                if (strcmp(menulabel, menuname)) {
                    continue;
                }
            }
            else {
                hotkey[0] = menu[nmenu].meta;
                awm->insert_sub_menu(menuname, hotkey, menu[nmenu].active_mask);
            }
        }

        menuloaded = true;

        long num_items = menu[nmenu].numitems;
        for (long nitem=0; nitem<num_items; nitem++) {
            GmenuItem *menuitem = &menu[nmenu].item[nitem];
            gde_assert(!menuitem->help || ARB_strBeginsWith(menuitem->help, "agde_"));
            hotkey[0]           = menuitem->meta;

            if (!menuitem->popup) {
                menuitem->popup = new WindowCallback(AW_window::makeWindowPopper(makeCreateWindowCallback(GDE_menuitem_cb, menuitem)));
            }
            awm->insert_menu_topic(menuitem->label, menuitem->label, hotkey, menuitem->help, menuitem->active_mask, *menuitem->popup);
        }

        if (!menulabel) {
            awm->close_sub_menu();
        }
    }

    if (!menuloaded && menulabel) {
        fprintf(stderr, "GDE-Warning: Could not find requested menu '%s'\n", menulabel);
    }
}

struct gde_database_access db_access = { NULL, NULL, GDE_WINDOWTYPE_DEFAULT, NULL};

GB_ERROR GDE_init(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main, GDE_get_sequences_cb get_sequences, GDE_format_alignment_cb format_ali, gde_window_type window_type) {
    db_access.get_sequences = get_sequences;
    db_access.format_ali    = format_ali;
    db_access.window_type   = window_type;
    db_access.gb_main       = gb_main;


    switch (db_access.window_type) {
        case GDE_WINDOWTYPE_EDIT4:
            aw_root->awar_int("gde/top_area_kons",      1, aw_def);
            aw_root->awar_int("gde/top_area_remark",    1, aw_def);
            aw_root->awar_int("gde/middle_area_kons",   1, aw_def);
            aw_root->awar_int("gde/middle_area_remark", 1, aw_def);
            aw_root->awar_int("gde/top_area",           1, aw_def);
            aw_root->awar_int("gde/top_area_sai",       1, aw_def);
            aw_root->awar_int("gde/top_area_helix",     1, aw_def);
            aw_root->awar_int("gde/middle_area",        1, aw_def);
            aw_root->awar_int("gde/middle_area_sai",    1, aw_def);
            aw_root->awar_int("gde/middle_area_helix",  1, aw_def);
            aw_root->awar_int("gde/bottom_area",        1, aw_def);
            aw_root->awar_int("gde/bottom_area_sai",    1, aw_def);
            aw_root->awar_int("gde/bottom_area_helix",  1, aw_def);
            break;
        case GDE_WINDOWTYPE_DEFAULT:
            break;
    }

    AW_awar *awar_defali = aw_root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", db_access.gb_main);
    aw_root->awar_string(AWAR_GDE_ALIGNMENT, "", aw_def)->map(awar_defali);

    awt_create_filter_awars(aw_root, aw_def, AWAR_GDE_FILTER_NAME, AWAR_GDE_ALIGNMENT);

    aw_root->awar_int(AWAR_GDE_CUTOFF_STOPCODON, 0, aw_def);
    aw_root->awar_int(AWAR_GDE_SPECIES,          1, aw_def);

    aw_root->awar_int(AWAR_GDE_COMPRESSION, COMPRESS_NONINFO_COLUMNS, aw_def);

    return LoadMenus();
}

