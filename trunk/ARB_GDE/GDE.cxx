#include "GDE_extglob.h"
#include "GDE_awars.h"

#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>
#include <aw_window.hxx>

/* AISC_MKPT_PROMOTE:#ifndef GDE_MENU_H */
/* AISC_MKPT_PROMOTE:#include "GDE_menu.h" */
/* AISC_MKPT_PROMOTE:#endif */

adfiltercbstruct *agde_filtercd = 0;

Gmenu         menu[GDEMAXMENU];
int           num_menus = 0, repeat_cnt = 0;
int           DisplayType;
GmenuItem    *current_item;
NA_Alignment *DataSet   = NULL;

char GDEBLANK[] = "\0";

#define SLIDERWIDTH 5           // input field width for numbers

static void GDE_showhelp_cb(AW_window *aw, GmenuItem *gmenuitem, AW_CL /* cd */) {
    const char *help_file = gmenuitem->help;
    if (help_file) {
        char *agde_help_file = GBS_string_eval(help_file, "*.help=agde_*1.hlp", 0);
        AW_POPUP_HELP(aw, (AW_CL)agde_help_file);
        free(agde_help_file);
    }
    else {
        aw_message("Sorry - no help available (please report to devel@arb-home.de)");
    }
}

char *GDE_makeawarname(GmenuItem *gmenuitem, long i) {
    char *gmenu_label     = GBS_string_2_key(gmenuitem->parent_menu->label);
    char *gmenuitem_label = GBS_string_2_key(gmenuitem->label);
    char *arg             = GBS_string_2_key(gmenuitem->arg[i].symbol);

    char *name = GBS_global_string_copy("gde/%s/%s/%s", gmenu_label, gmenuitem_label, arg);

    free(gmenu_label);
    free(gmenuitem_label);
    free(arg);

    return name;
}

static void GDE_slide_awar_int_cb(AW_window *aws, AW_CL cl_awar_name, AW_CL cd_diff)
{
    int      diff = (int)cd_diff;
    AW_awar *awar = aws->get_root()->awar((const char *)cl_awar_name);

    awar->write_int(awar->read_int()+diff);
}
static void GDE_slide_awar_float_cb(AW_window *aws, AW_CL cl_awar_name, AW_CL cd_diff)
{
    double   diff    = *(double*)cd_diff;
    AW_awar *awar    = aws->get_root()->awar((const char *)cl_awar_name);
    double   new_val = awar->read_float()+diff;

    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);

    // do it again (otherwise internal awar-range correction sometimes leads to 1+eXX values)
    new_val = awar->read_float();
    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);
}

static void GDE_create_infieldwithpm(AW_window *aws, char *newawar, long width)
{
    char *awar=strdup(newawar);
    aws->create_input_field(newawar, (int)width);
    if (aws->get_root()->awar(newawar)->get_type() == AW_INT) {
        aws->button_length(3);
        aws->callback(GDE_slide_awar_int_cb, (AW_CL)awar, -1);
        aws->create_button(0, "-", "-");
        aws->callback(GDE_slide_awar_int_cb, (AW_CL)awar, + 1);
        aws->create_button(0, "+", "+");
    }
    else if (aws->get_root()->awar(newawar)->get_type() == AW_FLOAT) {
        aws->button_length(3);
        aws->callback(GDE_slide_awar_float_cb, (AW_CL)awar, (AW_CL)new double(-0.1));
        aws->create_button(0, "-", "-");
        aws->callback(GDE_slide_awar_float_cb, (AW_CL)awar, (AW_CL)new double(+0.1));
        aws->create_button(0, "+", "+");
    }
}

static char *gde_filter_weights(GBDATA *gb_sai, AW_CL) {
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



static AW_window *GDE_menuitem_cb(AW_root *aw_root, GmenuItem *gmenuitem) {
#define BUFSIZE 200
    char bf[BUFSIZE+1];
#if defined(ASSERTION_USED)
    int printed =
#endif // ASSERTION_USED
        sprintf(bf, "GDE / %s / %s", gmenuitem->parent_menu->label, gmenuitem->label);

    gde_assert(printed<=BUFSIZE);
    char seqtype = gmenuitem->seqtype;

    if (gmenuitem->aws == NULL) {
        AW_window_simple *aws = new AW_window_simple;
        aws->init(aw_root, bf, bf);

        switch (gde_cgss.wt) {
            case CGSS_WT_DEFAULT: {
                if (seqtype == '-') aws->load_xfig("gdeitem_simple.fig");
                else                aws->load_xfig("gdeitem.fig");
                break;
            }
            case CGSS_WT_EDIT:
                gde_assert(seqtype != '-');
                aws->load_xfig("gde2item.fig");
                break;
            case CGSS_WT_EDIT4:
                gde_assert(seqtype != '-');
                aws->load_xfig("gde3item.fig");
                break;
            default:
                gde_assert(0);
        }

        aws->set_window_size(1000, 2000);
        aws->button_length(10);
        aws->at(10, 10);
        aws->auto_space(0, 10);

        aws->at("help");
        aws->callback((AW_CB2)GDE_showhelp_cb, (AW_CL)gmenuitem, 0);
        aws->create_button("GDE_HELP", "HELP...", "H");

        aws->at("start");
        aws->callback((AW_CB2)GDE_startaction_cb, (AW_CL)gmenuitem, 0);
        aws->create_button("GO", "GO", "O");

        aws->at("cancel");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");


        if (gmenuitem->numinputs>0) {
            switch (gde_cgss.wt) {
                case CGSS_WT_DEFAULT: {
                    if (seqtype != '-') { // '-' means "skip sequence export"
                        aws->at("which_alignment");
                        const char *ali_filter = seqtype == 'A' ? "pro=:ami=" : (seqtype == 'N' ? "dna=:rna=" : "*=");
                        awt_create_selection_list_on_alignments(GLOBAL_gb_main, (AW_window *)aws, AWAR_GDE_ALIGNMENT, ali_filter);

                        aws->at("which_species");
                        aws->create_toggle_field(AWAR_GDE_SPECIES);
                        aws->insert_toggle("all", "a", 0);
                        aws->insert_default_toggle("marked",   "m", 1);
                        aws->update_toggle_field();

                        if (seqtype != 'N') {
                            aws->at("stop_codon");
                            aws->label("Cut stop-codon");
                            aws->create_toggle(AWAR_GDE_CUTOFF_STOPCODON);
                        }
                    }
                    break;
                }
                case CGSS_WT_EDIT:
                    aws->at("bottom"); aws->create_toggle("gde/bottom_area");
                    aws->at("bottomsai"); aws->create_toggle("gde/bottom_area_sai");
                    aws->at("bottomh"); aws->create_toggle("gde/bottom_area_helix");
                    goto both_edits;
                case CGSS_WT_EDIT4:
                    aws->at("topk"); aws->create_toggle("gde/top_area_kons");
                    aws->at("middlek"); aws->create_toggle("gde/middle_area_kons");
                    aws->at("topr"); aws->create_toggle("gde/top_area_remark");
                    aws->at("middler"); aws->create_toggle("gde/middle_area_remark");
                    goto both_edits;
                both_edits :
                    aws->at("top"); aws->create_toggle("gde/top_area");
                    aws->at("topsai"); aws->create_toggle("gde/top_area_sai");
                    aws->at("toph"); aws->create_toggle("gde/top_area_helix");
                    aws->at("middle"); aws->create_toggle("gde/middle_area");
                    aws->at("middlesai"); aws->create_toggle("gde/middle_area_sai");
                    aws->at("middleh"); aws->create_toggle("gde/middle_area_helix");
                    break;
            }

            if (seqtype != '-') {
                aws->at("compression");
                aws->create_option_menu(AWAR_GDE_COMPRESSION, "", "");
                aws->insert_option("none", "n", COMPRESS_NONE);
                aws->insert_option("vertical gaps", "v", COMPRESS_VERTICAL_GAPS);
                aws->insert_default_option("columns w/o info", "i", COMPRESS_NONINFO_COLUMNS);
                aws->insert_option("all gaps", "g", COMPRESS_ALL);
                aws->update_option_menu();

                aws->button_length(12);
                aws->at("filtername");
                if (!agde_filtercd) { // create only one filter - used for all GDE calls
                    agde_filtercd = awt_create_select_filter(aws->get_root(), GLOBAL_gb_main, AWAR_GDE_FILTER_NAME);
                }
                aws->callback((AW_CB2)AW_POPUP, (AW_CL)awt_create_select_filter_win, (AW_CL)agde_filtercd);
                aws->create_button("SELECT_FILTER", AWAR_GDE_FILTER_NAME);
            }

            aws->at("paramsb");
        }
        else {
            aws->at("paramsb");
        }


        int labellength = 1;
        long i;
        for (i=0; i<gmenuitem->numargs; i++) {
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

        for (i=0; i<gmenuitem->numargs; i++) {
            GmenuItemArg itemarg=gmenuitem->arg[i];

            if (itemarg.type==SLIDER) {
                char *newawar=GDE_makeawarname(gmenuitem, i);
                if (int(gmenuitem->arg[i].fvalue) == gmenuitem->arg[i].fvalue &&
                     int(gmenuitem->arg[i].min) == gmenuitem->arg[i].min &&
                     int(gmenuitem->arg[i].max) == gmenuitem->arg[i].max) {
                    aw_root->awar_int(newawar, (long)gmenuitem->arg[i].fvalue, AW_ROOT_DEFAULT);
                }
                else {
                    aw_root->awar_float(newawar, gmenuitem->arg[i].fvalue, AW_ROOT_DEFAULT);
                }
                aw_root->awar(newawar)->set_minmax(gmenuitem->arg[i].min, gmenuitem->arg[i].max);
                aws->label(gmenuitem->arg[i].label);
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

                aws->label(gmenuitem->arg[i].label);
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

                aws->label(gmenuitem->arg[i].label);
                aws->create_option_menu(newawar, NULL, "");

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
                aws->label(gmenuitem->arg[i].label);
                aws->create_input_field(newawar, itemarg.textwidth /* TEXTFIELDWIDTH */);
                free(newawar);
            }
            else if (itemarg.type==CHOICE_TREE) {
                char *defopt=itemarg.textvalue;
                char *newawar=GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->label(gmenuitem->arg[i].label);
                awt_create_selection_list_on_trees(GLOBAL_gb_main, aws, newawar);
                free(newawar);
            }
            else if (itemarg.type==CHOICE_SAI) {
                char *defopt=itemarg.textvalue;
                char *newawar=GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->label(gmenuitem->arg[i].label);
                awt_create_selection_list_on_extendeds(GLOBAL_gb_main, aws, newawar);
                free(newawar);
            }
            else if (itemarg.type==CHOICE_WEIGHTS) {
                char *defopt=itemarg.textvalue;
                char *newawar=GDE_makeawarname(gmenuitem, i);
                aw_root->awar_string(newawar, defopt, AW_ROOT_DEFAULT);
                aws->label(gmenuitem->arg[i].label);
                void *id = awt_create_selection_list_on_extendeds(GLOBAL_gb_main, aws, newawar, gde_filter_weights);
                free(newawar);
                aw_root->awar(AWAR_GDE_ALIGNMENT)->add_callback((AW_RCB1)awt_create_selection_list_on_extendeds_update, (AW_CL)id);
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




void GDE_load_menu(AW_window *awm, AW_active mask, const char *menulabel, const char *menuitemlabel) {
    // Load GDE menu items.
    //
    // If 'menulabel' == NULL -> load all menus
    // Else                   -> load specified menu
    //
    // If 'menuitemlabel' == NULL -> load complete menu(s)
    // Else                       -> load only specific menu topic

    char       buffer[1024];
    char      *help;
    long       nitem, num_items;
    GmenuItem *menuitem;
    char       hotkey[]   = "x";
    bool       menuloaded = false;
    bool       itemloaded = false;

    for (long nmenu = 0; nmenu<num_menus; nmenu++) {
        {
            const char *menuname = menu[nmenu].label;
            if (menulabel) {
                if (strcmp(menulabel, menuname)) {
                    continue;
                }
            }
            else {
                hotkey[0]     = menu[nmenu].meta;
                awm->insert_sub_menu(menuname, hotkey);
            }
        }

        menuloaded = true;

        num_items = menu[nmenu].numitems;
        for (nitem=0; nitem<num_items; nitem++) {
            menuitem=&menu[nmenu].item[nitem];
            if (!menuitemlabel || strcmp(menuitem->label, menuitemlabel) == 0) {
                itemloaded = true;
                if (menuitem->help) {
                    sprintf(buffer, "GDEHELP/%s", menuitem->help);
                    help = strdup(buffer);
                }
                else {
                    help = 0;
                }
                hotkey[0]     = menuitem->meta;
                awm->insert_menu_topic(0, menuitem->label, hotkey,
                                       help, mask,
                                       AW_POPUP, (AW_CL)GDE_menuitem_cb, (AW_CL)menuitem);
            }
        }
        if (!menulabel) {
            awm->close_sub_menu();
        }
    }

    if (!menuloaded && menulabel) {
        fprintf(stderr, "GDE-Warning: Could not find requested menu '%s'\n", menulabel);
    }
    if (!itemloaded && menuitemlabel) {
        if (menulabel) {
            fprintf(stderr, "GDE-Warning: Could not find requested topic '%s' in menu '%s'\n", menuitemlabel, menulabel);
        }
        else {
            fprintf(stderr, "GDE-Warning: Could not find requested topic '%s'\n", menuitemlabel);
        }
    }
}

struct choose_get_sequence_struct gde_cgss = { 0, CGSS_WT_DEFAULT, 0 };

void create_gde_var(AW_root  *aw_root, AW_default aw_def,
                    char *(*get_sequences)(void *THIS, GBDATA **&the_species,
                                           uchar **&the_names,
                                           uchar **&the_sequences,
                                           long &numberspecies, long &maxalignlen),
                    gde_cgss_window_type wt,
                    void *THIS)
{
    gde_cgss.get_sequences = get_sequences;
    gde_cgss.wt = wt;
    gde_cgss.THIS = THIS;

    aw_root->awar_string(AWAR_GDE_ALIGNMENT, "", aw_def);

    switch (gde_cgss.wt)
    {
        case CGSS_WT_EDIT4:
            aw_root->awar_int("gde/top_area_kons", 1, aw_def);
            aw_root->awar_int("gde/top_area_remark", 1, aw_def);
            aw_root->awar_int("gde/middle_area_kons", 1, aw_def);
            aw_root->awar_int("gde/middle_area_remark", 1, aw_def);
        case CGSS_WT_EDIT:
            aw_root->awar_int("gde/top_area", 1, aw_def);
            aw_root->awar_int("gde/top_area_sai", 1, aw_def);
            aw_root->awar_int("gde/top_area_helix", 1, aw_def);
            aw_root->awar_int("gde/middle_area", 1, aw_def);
            aw_root->awar_int("gde/middle_area_sai", 1, aw_def);
            aw_root->awar_int("gde/middle_area_helix", 1, aw_def);
            aw_root->awar_int("gde/bottom_area", 1, aw_def);
            aw_root->awar_int("gde/bottom_area_sai", 1, aw_def);
            aw_root->awar_int("gde/bottom_area_helix", 1, aw_def);
        default:
            break;
    }

    aw_root->awar_string("presets/use",             "", GLOBAL_gb_main);

    aw_root->awar_string(AWAR_GDE_FILTER_NAME,      "", aw_def);
    aw_root->awar_string(AWAR_GDE_FILTER_FILTER,    "", aw_def);
    aw_root->awar_string(AWAR_GDE_FILTER_ALIGNMENT, "", aw_def);

    aw_root->awar_int(AWAR_GDE_CUTOFF_STOPCODON, 0, aw_def);
    aw_root->awar_int(AWAR_GDE_SPECIES,          1, aw_def);

    aw_root->awar_int(AWAR_GDE_COMPRESSION, COMPRESS_NONINFO_COLUMNS, aw_def);

    aw_root->awar(AWAR_GDE_ALIGNMENT)->map("presets/use");
    aw_root->awar(AWAR_GDE_FILTER_ALIGNMENT)->map("presets/use");

    DataSet = (NA_Alignment *) Calloc(1, sizeof(NA_Alignment));
    DataSet->rel_offset = 0;
    ParseMenu();
}

