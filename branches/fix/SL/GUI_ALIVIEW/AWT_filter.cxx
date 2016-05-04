#include "awt_filter.hxx"
#include "awt_sel_boxes.hxx"
#include "ga_local.h"

#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_select.hxx>
#include <AP_filter.hxx>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <ad_cb.h>
#include <arb_str.h>

//! recalc filter
static void awt_create_select_filter_window_aw_cb(UNFIXED, adfiltercbstruct *cbs) {
    // update the variables
    AW_root *aw_root = cbs->awr;
    GB_push_transaction(cbs->gb_main);
    char *target = aw_root->awar(cbs->def_subname)->read_string();
    char *to_free_target = target;
    char *use = aw_root->awar(cbs->def_alignment)->read_string();
    char *name = strchr(target, 1);
    GBDATA *gbd = 0;
    if (name) {
        *(name++) = 0;
        target++;
        GBDATA *gb_species;
        if (target[-1] == '@') {
            gb_species = GBT_find_species(cbs->gb_main, name);
        }
        else {
            gb_species = GBT_find_SAI(cbs->gb_main, name);
        }
        if (gb_species) {
            GBDATA *gb_ali = GB_search(gb_species, use, GB_FIND);
            if (gb_ali) {
                gbd = GB_search(gb_ali, target, GB_FIND);
            }
            else {
                GB_clear_error();
            }
        }
    }
    if (!gbd) {     // nothing selected
        aw_root->awar(cbs->def_name)->write_string("none");
        aw_root->awar(cbs->def_source)->write_string("No Filter Sequence ->All Columns Selected");
        aw_root->awar(cbs->def_filter)->write_string("");
        aw_root->awar(cbs->def_len)   ->write_int(-1);  // export filter
    }
    else {
        GBDATA *gb_name = GB_get_father(gbd);   // ali_xxxx
        gb_name = GB_brother(gb_name, "name");
        char *name2 = GB_read_string(gb_name);
        aw_root->awar(cbs->def_name)->write_string(name2);
        free(name2);
        char *_2filter = aw_root->awar(cbs->def_2filter)->read_string();
        long _2filter_len = strlen(_2filter);

        char *s, *str;
        long len = GBT_get_alignment_len(cbs->gb_main, use);
        GBS_strstruct *strstruct = GBS_stropen(5000);
        long i; for (i=0; i<len; i++) { // build position line
            if (i%10 == 0) {
                GBS_chrcat(strstruct, '#');
            }
            else if (i%5==0) {
                GBS_chrcat(strstruct, '|');
            }
            else {
                GBS_chrcat(strstruct, '.');
            }
        }
        GBS_chrcat(strstruct, '\n');
        char *data = GBS_mempntr(strstruct);

        for (i=0; i<len-10; i++) {  // place markers
            if (i%10 == 0) {
                char buffer[256];
                sprintf(buffer, "%li", i+1);
                strncpy(data+i+1, buffer, strlen(buffer));
            }
        }

        if (GB_read_type(gbd) == GB_STRING) {   // read the filter
            str = GB_read_string(gbd);
        }
        else {
            str = GB_read_bits(gbd, '-', '+');
        }
        GBS_strcat(strstruct, str);
        GBS_chrcat(strstruct, '\n');
        char *canc = aw_root->awar(cbs->def_cancel)->read_string();
        long min = aw_root->awar(cbs->def_min)->read_int()-1;
        long max = aw_root->awar(cbs->def_max)->read_int()-1;
        long flen = 0;
        for (i=0, s=str; *s; ++s, ++i) {    // transform the filter
            if (strchr(canc, *s) || (i<min) || (max>0 && i > max))
            {
                *s = '0';
            }
            else {
                if (i > _2filter_len || _2filter[i] != '0') {
                    *s = '1';
                    flen++;
                }
                else {
                    *s = '0';
                }
            }
        }
        GBS_strcat(strstruct, str);
        GBS_chrcat(strstruct, '\n');
        data = GBS_strclose(strstruct);
        aw_root->awar(cbs->def_len)   ->write_int(flen);    // export filter
        aw_root->awar(cbs->def_filter)->write_string(str);  // export filter
        aw_root->awar(cbs->def_source)->write_string(data); // set display
        free(_2filter);
        free(str);
        free(canc);
        free(data);
    }
    free(to_free_target);
    free(use);
    GB_pop_transaction(cbs->gb_main);
}

static void awt_add_sequences_to_list(adfiltercbstruct *cbs, const char *use, GBDATA *gb_extended, const char *pre, char tpre) {
    GBDATA *gb_ali = GB_entry(gb_extended, use);

    if (gb_ali) {
        int         count   = 0;
        GBDATA     *gb_type = GB_entry(gb_ali, "_TYPE");
        const char *TYPE    = gb_type ? GB_read_char_pntr(gb_type) : "";
        const char *name    = GBT_read_name(gb_extended);
        GBDATA     *gb_data;

        for (gb_data = GB_child(gb_ali); gb_data; gb_data = GB_nextChild(gb_data)) {
            if (GB_read_key_pntr(gb_data)[0] != '_') {
                long type = GB_read_type(gb_data);

                if (type == GB_BITS || type == GB_STRING) {
                    char *str;

                    if (count) str = GBS_global_string_copy("%s%-20s SEQ_%i %s", pre, name, count + 1, TYPE);
                    else str       = GBS_global_string_copy("%s%-20s       %s", pre, name, TYPE);

                    const char *target = GBS_global_string("%c%s%c%s", tpre, GB_read_key_pntr(gb_data), 1, name);

                    cbs->filterlist->insert(str, target);
                    free(str);
                    count++;
                }
            }
        }
    }
}

static void awt_create_select_filter_window_gb_cb(UNFIXED, adfiltercbstruct *cbs) {
    // update list widget and variables
    GB_push_transaction(cbs->gb_main);

    if (cbs->filterlist) {
        char *use = cbs->awr->awar(cbs->def_alignment)->read_string();

        cbs->filterlist->clear();
        cbs->filterlist->insert_default("none", "");

        const char *name = GBT_readOrCreate_char_pntr(cbs->gb_main, AWAR_SPECIES_NAME, "");
        if (name[0]) {
            GBDATA *gb_species = GBT_find_species(cbs->gb_main, name);
            if (gb_species) {
                awt_add_sequences_to_list(cbs, use, gb_species, "SEL. SPECIES:", '@');
            }
        }

        for (GBDATA *gb_extended = GBT_first_SAI(cbs->gb_main);
             gb_extended;
             gb_extended = GBT_next_SAI(gb_extended))
        {
            awt_add_sequences_to_list(cbs, use, gb_extended, "", ' ');
        }

        cbs->filterlist->update();
        free(use);
    }
    awt_create_select_filter_window_aw_cb(0, cbs);
    GB_pop_transaction(cbs->gb_main);
}

void awt_create_filter_awars(AW_root *aw_root, AW_default aw_def, const char *awar_filtername, const char *awar_mapto_alignment) {
    /*! creates awars needed for filter definition (see awt_create_select_filter())
     *
     * created awars: "SOMETHING/name" (as specified in param awar_filtername; type STRING)
     *                "SOMETHING/filter" (type STRING)
     *                "SOMETHING/alignment" (type STRING)
     *
     * The names of the created awars are available via adfiltercbstruct (after awt_create_select_filter() was called).
     *
     * @param aw_root              application root
     * @param aw_def               database for created awars
     * @param awar_filtername      name of filtername awar (has to be "SOMETHING/name";
     *                             awarname should start with "tmp/", saving in properties/db does not work correctly!).
     * @param awar_mapto_alignment if given, "SOMETHING/alignment" is mapped to this awar
     */

    ga_assert(ARB_strBeginsWith(awar_filtername, "tmp/")); // see above

    char *awar_filter    = GBS_string_eval(awar_filtername, "/name=/filter",    0);;
    char *awar_alignment = GBS_string_eval(awar_filtername, "/name=/alignment", 0);;

    aw_root->awar_string(awar_filtername, "none", aw_def);
    aw_root->awar_string(awar_filter,     "",     aw_def);

    AW_awar *awar_ali = aw_root->awar_string(awar_alignment, "", aw_def);
    if (awar_mapto_alignment) awar_ali->map(awar_mapto_alignment);

    free(awar_alignment);
    free(awar_filter);
}

adfiltercbstruct *awt_create_select_filter(AW_root *aw_root, GBDATA *gb_main, const char *def_name) {
    /*! Create filter definition (allowing customization and programmatical use of a filter)
     *
     * @param aw_root  application root
     * @param gb_main  DB root node
     * @param def_name filtername awarname (name has to be "SOMETHING/name"; create using awt_create_filter_awars())
     * @return the filter definition
     * @see awt_create_select_filter_win(), awt_get_filter()
     */

    adfiltercbstruct *acbs   = new adfiltercbstruct;
    acbs->gb_main            = gb_main;
    AW_default        aw_def = AW_ROOT_DEFAULT;

    GB_push_transaction(acbs->gb_main);

#if defined(DEBUG)
    {
        int len = strlen(def_name);

        ga_assert(len >= 5);
        ga_assert(strcmp(def_name+len-5, "/name") == 0); // filter awar has to be "SOMETHING/name"
    }
#endif                          // DEBUG

    acbs->def_name      = GBS_string_eval(def_name, "/name=/name", 0);
    acbs->def_filter    = GBS_string_eval(def_name, "/name=/filter", 0);
    acbs->def_alignment = GBS_string_eval(def_name, "/name=/alignment", 0);

    acbs->def_min = GBS_string_eval(def_name, "*/name=tmp/*1/min:tmp/tmp=tmp", 0);
    acbs->def_max = GBS_string_eval(def_name, "*/name=tmp/*1/max:tmp/tmp=tmp", 0);
    aw_root->awar_int(acbs->def_min)->add_callback(makeRootCallback(awt_create_select_filter_window_aw_cb, acbs));
    aw_root->awar_int(acbs->def_max)->add_callback(makeRootCallback(awt_create_select_filter_window_aw_cb, acbs));

    acbs->def_len = GBS_string_eval(def_name, "*/name=tmp/*1/len:tmp/tmp=tmp", 0);
    aw_root->awar_int(acbs->def_len);

    acbs->def_dest = GBS_string_eval(def_name, "*/name=tmp/*1/dest:tmp/tmp=tmp", 0);
    aw_root->awar_string(acbs->def_dest, "", aw_def);

    acbs->def_cancel = GBS_string_eval(def_name, "*/name=*1/cancel", 0);
    aw_root->awar_string(acbs->def_cancel, ".0-=", aw_def);

    acbs->def_simplify = GBS_string_eval(def_name, "*/name=*1/simplify", 0);
    aw_root->awar_int(acbs->def_simplify, 0, aw_def);

    acbs->def_subname = GBS_string_eval(def_name, "*/name=tmp/*1/subname:tmp/tmp=tmp", 0);
    aw_root->awar_string(acbs->def_subname);

    acbs->def_source = GBS_string_eval(def_name, "*/name=tmp/*/source:tmp/tmp=tmp", 0);
    aw_root->awar_string(acbs->def_source);

    acbs->def_2name      = GBS_string_eval(def_name, "*/name=tmp/*/2filter/name:tmp/tmp=tmp", 0);
    acbs->def_2filter    = GBS_string_eval(def_name, "*/name=tmp/*/2filter/filter:tmp/tmp=tmp", 0);
    acbs->def_2alignment = GBS_string_eval(def_name, "*/name=tmp/*/2filter/alignment:tmp/tmp=tmp", 0);

    aw_root->awar_string(acbs->def_2name)->write_string("- none -");
    aw_root->awar_string(acbs->def_2filter);
    aw_root->awar_string(acbs->def_2alignment);

    acbs->filterlist = 0;
    acbs->aw_filt    = 0;
    acbs->awr        = aw_root;
    {
        char *fname = aw_root->awar(acbs->def_name)->read_string();
        const char *fsname = GBS_global_string(" data%c%s", 1, fname);
        free(fname);
        aw_root->awar(acbs->def_subname)->write_string(fsname);     // cause an callback
    }

    aw_root->awar(acbs->def_subname)->touch();      // cause an callback

    GBDATA *gb_sai_data = GBT_get_SAI_data(acbs->gb_main);
    GBDATA *gb_sel      = GB_search(acbs->gb_main, AWAR_SPECIES_NAME, GB_STRING);

    GB_add_callback(gb_sai_data, GB_CB_CHANGED, makeDatabaseCallback(awt_create_select_filter_window_gb_cb, acbs));
    GB_add_callback(gb_sel,      GB_CB_CHANGED, makeDatabaseCallback(awt_create_select_filter_window_gb_cb, acbs));

    aw_root->awar(acbs->def_alignment)->add_callback(makeRootCallback(awt_create_select_filter_window_gb_cb, acbs));
    aw_root->awar(acbs->def_2filter)  ->add_callback(makeRootCallback(awt_create_select_filter_window_aw_cb, acbs));
    aw_root->awar(acbs->def_subname)  ->add_callback(makeRootCallback(awt_create_select_filter_window_aw_cb, acbs));

    awt_create_select_filter_window_gb_cb(0, acbs);

    GB_pop_transaction(acbs->gb_main);
    return acbs;
}


void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main, AW_root *awr, const char *awar_name) {
    GB_transaction transaction_var(gb_main);
    if (GBT_find_SAI(gb_main, "POS_VAR_BY_PARSIMONY")) {
        awr->awar(awar_name)->write_string("POS_VAR_BY_PARSIMONY");
        return;
    }
    if (GBT_find_SAI(gb_main, "ECOLI")) {
        awr->awar(awar_name)->write_string("ECOLI");
        return;
    }
}

static AW_window *awt_create_2_filter_window(AW_root *aw_root, adfiltercbstruct *acbs) {
    GB_push_transaction(acbs->gb_main);
    aw_root->awar(acbs->def_2alignment)->map(acbs->def_alignment);
    adfiltercbstruct *s2filter = awt_create_select_filter(aw_root, acbs->gb_main, acbs->def_2name);
    GB_pop_transaction(acbs->gb_main);
    return awt_create_select_filter_win(aw_root, s2filter);
}

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix) {
    char       *combined_name = aw_root->awar(GBS_global_string("%s/filter/name", prefix))->read_string(); // "gde/filter/name"
    const char *awar_prefix   = AWAR_GDE_FILTER;
    const char *awar_repeated = "/2filter";
    const char *awar_postfix  = "/name";
    int         prefix_len    = strlen(awar_prefix);
    int         repeated_len  = strlen(awar_repeated);
    int         postfix_len   = strlen(awar_postfix);
    int         count;

    for (count = 1; ; ++count) {
        char *awar_name = new char[prefix_len + count*repeated_len + postfix_len + 1];
        strcpy(awar_name, awar_prefix);
        int c;
        for (c=0; c<count; ++c) strcat(awar_name, awar_repeated);
        strcat(awar_name, awar_postfix);

        AW_awar *awar_found = aw_root->awar_no_error(awar_name);
        delete [] awar_name;

        if (!awar_found) break; // no more filters defined
        char *content = awar_found->read_string();

        if (strstr(content, "none")==0) { // don't add filters named 'none'
            freeset(combined_name, GBS_global_string_copy("%s/%s", combined_name, content));
        }
    }

    return combined_name;
}

AW_window *awt_create_select_filter_win(AW_root *aw_root, adfiltercbstruct *acbs) {
    //! Create a filter selection window
    if (!acbs->aw_filt) {
        GB_push_transaction(acbs->gb_main);

        AW_window_simple *aws = new AW_window_simple;
        {
            int   checksum  = GBS_checksum(acbs->def_name, true, NULL);
            char *window_id = GBS_global_string_copy("FILTER_SELECT_%i", checksum); // make window id awar specific

            aws->init(aw_root, window_id, "Select Filter");
            free(window_id);
        }
        aws->load_xfig("awt/filter.fig");
        aws->button_length(10);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(makeHelpCallback("sel_fil.hlp"));
        aws->create_button("HELP", "HELP", "H");

        acbs->aw_filt = aws; // store the filter selection window in 'acbs'

        aws->at("filter");
        acbs->filterlist = aws->create_selection_list(acbs->def_subname, 20, 3, true);

        aws->at("2filter");
        aws->callback(makeCreateWindowCallback(awt_create_2_filter_window, acbs));
        aws->create_button(acbs->def_2name, acbs->def_2name);

        aws->at("zero");
        aws->callback(makeWindowCallback(awt_create_select_filter_window_aw_cb, acbs)); // @@@ used as INPUTFIELD_CB (see #559)
        aws->create_input_field(acbs->def_cancel, 10);

        aws->at("sequence");
        aws->create_text_field(acbs->def_source, 1, 1);

        aws->at("min");
        aws->create_input_field(acbs->def_min, 4);

        aws->at("max");
        aws->create_input_field(acbs->def_max, 4);

        aws->at("simplify");
        aws->create_option_menu(acbs->def_simplify, true);
        aws->insert_option("ORIGINAL DATA", "O", 0);
        aws->sens_mask(AWM_EXP);
        aws->insert_option("TRANSVERSIONS ONLY", "T", 1);
        aws->insert_option("SIMPLIFIED AA", "A", 2);
        aws->sens_mask(AWM_ALL);
        aws->update_option_menu();

        awt_create_select_filter_window_gb_cb(0, acbs);

        aws->button_length(7);
        aws->at("len");    aws->create_button(0, acbs->def_len);

        GB_pop_transaction(acbs->gb_main);
    }

    return acbs->aw_filt;
}

AP_filter *awt_get_filter(adfiltercbstruct *acbs) {
    /*! create a filter from settings made in filter-definition window.
     *  always returns a filter, use awt_invalid_filter() to check for validity
     */
    AP_filter *filter = NULL;

    if (acbs) {
        GB_push_transaction(acbs->gb_main);

        char *filter_string = acbs->awr->awar(acbs->def_filter)->read_string();
        long  len           = 0;

        {
            char *use = acbs->awr->awar(acbs->def_alignment)->read_string();

            len = GBT_get_alignment_len(acbs->gb_main, use);
            free(use);
        }

        if (len == -1) { // no alignment -> uses dummy filter
            GB_clear_error();
        }
        else { // have alignment
            filter  = new AP_filter(filter_string, "0", len);
            int sim = acbs->awr->awar(acbs->def_simplify)->read_int();
            filter->enable_simplify((AWT_FILTER_SIMPLIFY)sim);
            free(filter_string);
        }

        GB_pop_transaction(acbs->gb_main);
    }

    if (!filter) filter = new AP_filter(0); // empty dummy filter
    return filter;
}

GB_ERROR awt_invalid_filter(AP_filter *filter) {
    return filter->is_invalid();
}

void awt_destroy_filter(AP_filter *filter) {
    delete filter;
}
