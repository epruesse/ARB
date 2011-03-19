// =============================================================== //
//                                                                 //
//   File      : ad_spec.cxx                                       //
//   Purpose   : Create and modify species and SAI.                //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "map_viewer.hxx"
#include "ad_spec.hxx"
#include "nt_internal.h"
#include "ntree.hxx"

#include <awtc_next_neighbours.hxx>
#include <AW_rename.hxx>
#include <db_scanner.hxx>
#include <awt_item_sel_list.hxx>
#include <awtlocal.hxx>
#include <awt_www.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_detach.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <arb_str.h>
#include <arb_defs.h>
#include <probe_design.hxx>

#include <cctype>


#define nt_assert(bed) arb_assert(bed)

extern GBDATA *GLOBAL_gb_main;

// next neighbours of listed and selected:
#define AWAR_NN_COMPLEMENT  AWAR_NN_BASE "complement"

// next neighbours of selected only:
#define AWAR_NN_MAX_HITS  AWAR_NN_BASE "max_hits"
#define AWAR_NN_HIT_COUNT "tmp/" AWAR_NN_BASE "hit_count"

// next neighbours of listed only:
#define AWAR_NN_DEST_FIELD     AWAR_NN_BASE "dest_field"
#define AWAR_NN_WANTED_ENTRIES AWAR_NN_BASE "wanted_entries"
#define AWAR_NN_SCORED_ENTRIES AWAR_NN_BASE "scored_entries"
#define AWAR_NN_MIN_SCORE      AWAR_NN_BASE "min_score"
#define AWAR_NN_RANGE_START    AWAR_NN_BASE "range_start"
#define AWAR_NN_RANGE_END      AWAR_NN_BASE "range_end"

void NT_create_species_var(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SPECIES_DEST,         "",        aw_def);
    aw_root->awar_string(AWAR_SPECIES_INFO,         "",        aw_def);
    aw_root->awar_string(AWAR_SPECIES_KEY,          "",        aw_def);
    aw_root->awar_string(AWAR_FIELD_REORDER_SOURCE, "",        aw_def);
    aw_root->awar_string(AWAR_FIELD_REORDER_DEST,   "",        aw_def);
    aw_root->awar_string(AWAR_FIELD_CREATE_NAME,    "",        aw_def);
    aw_root->awar_int   (AWAR_FIELD_CREATE_TYPE,    GB_STRING, aw_def);
    aw_root->awar_string(AWAR_FIELD_DELETE,         "",        aw_def);
    aw_root->awar_string(AWAR_FIELD_CONVERT_SOURCE, "",        aw_def);
    aw_root->awar_int   (AWAR_FIELD_CONVERT_TYPE,   GB_STRING, aw_def);
    aw_root->awar_string(AWAR_FIELD_CONVERT_NAME,   "",        aw_def);
}

static void move_species_to_extended(AW_window *aww) {
    char     *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_ERROR  error  = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GBDATA *gb_sai_data     = GBT_get_SAI_data(GLOBAL_gb_main);
        if (!gb_sai_data) error = GB_await_error();
        else {
            GBDATA *gb_species = GBT_find_species(GLOBAL_gb_main, source);
            GBDATA *gb_dest    = GBT_find_SAI_rel_SAI_data(gb_sai_data, source);

            if (gb_dest) error = GBS_global_string("SAI '%s' already exists", source);
            else if (gb_species) {
                gb_dest             = GB_create_container(gb_sai_data, "extended");
                if (!gb_dest) error = GB_await_error();
                else {
                    error = GB_copy(gb_dest, gb_species);
                    if (!error) {
                        error = GB_delete(gb_species);
                        if (!error) aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string("");
                    }
                }
            }
            else error = "Please select a species";
        }
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(source);
}

static const char * const SAI_COUNTED_CHARS = "COUNTED_CHARS";

void NT_count_different_chars(AW_window *, AW_CL cl_gb_main, AW_CL) {
    // @@@ extract algorithm and call extracted from testcode
    ARB_ERROR  error;
    GBDATA    *gb_main = (GBDATA*)cl_gb_main;

    error = GB_begin_transaction(gb_main);                      // open transaction

    char *alignment_name = GBT_get_default_alignment(gb_main);  // info about sequences
    int   alignment_len  = GBT_get_alignment_len(gb_main, alignment_name);
    int   is_amino       = GBT_is_alignment_protein(gb_main, alignment_name);

    if (!error) {
        const int MAXLETTER   = 256;
        const int FIRSTLETTER = 0;

        typedef bool letterOccurs[MAXLETTER];

        letterOccurs *occurs = (letterOccurs*)malloc(sizeof(*occurs)*alignment_len);
        for (int i = 0; i<MAXLETTER; ++i) {
            for (int p = 0; p<alignment_len; ++p) {
                occurs[p][i] = false;
            }
        }

        // loop over all marked species
        {
            int          all_marked = GBT_count_marked_species(gb_main);
            arb_progress progress("Counting different characters", all_marked);

            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species && !error;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                GBDATA *gb_ali = GB_entry(gb_species, alignment_name); // search the sequence database entry ( ali_xxx/data )
                if (gb_ali) {
                    GBDATA *gb_data = GB_entry(gb_ali, "data");
                    if (gb_data) {
                        const char * const seq = GB_read_char_pntr(gb_data);
                        if (seq) {
                            for (int i=0; i< alignment_len; ++i) {
                                unsigned char c = seq[i];
                                if (!c) break;

                                occurs[i][c-FIRSTLETTER] = true;
                            }
                        }
                    }
                }
                progress.inc_and_check_user_abort(error);
            }
        }

        if (!error) {

            char filter[256];
            if (is_amino) for (int c = 0; c<256; ++c) filter[c] = isupper(c) && (strchr("BJOUZ", c) == 0);
            else          for (int c = 0; c<256; ++c) filter[c] = (strchr("ACGTU", c) != 0);

            char result[alignment_len+1];
            for (int i=0; i<alignment_len; i++) {
                int sum = 0;
                for (int c = 'A'; c < 'Z'; ++c) {
                    if (filter[c]) {
                        sum += (occurs[i][c] || occurs[i][tolower(c)]);
                    }
                }
                result[i] = sum<10 ? '0'+sum : 'A'-10+sum;
            }
            result[alignment_len] = 0;

            {
                GBDATA *gb_sai     = GBT_find_or_create_SAI(gb_main, SAI_COUNTED_CHARS);
                if (!gb_sai) error = GB_await_error();
                else {
                    GBDATA *gb_data     = GBT_add_data(gb_sai, alignment_name, "data", GB_STRING);
                    if (!gb_data) error = GB_await_error();
                    else    error       = GB_write_string(gb_data, result);
                }
            }
        }

        free(occurs);
    }

    free(alignment_name);

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

void NT_create_sai_from_pfold(AW_window *aww, AW_CL ntw, AW_CL) {
    /*! \brief Creates an SAI from protein secondary structure of a selected species.
     *
     *  \param[in] aww AW_window
     *  \param[in] ntw AWT_canvas
     *
     *  The function takes the currently selected species and searches for the field
     *  "sec_struct". A new SAI is created using the data in this field. A simple input
     *  window allows the user to change the default name ([species name]_pfold) for
     *  the new SAI.
     *
     *  \note The import filter "dssp_all.ift" allows for importing the amino acid sequence
     *        as well as the protein secondary structure from a dssp file and the structure
     *        is stored in the field "sec_struct". That way, secondary structure can be
     *        aligned along with the sequence manually and can later be extracted to create
     *        an SAI.
     *
     *  \attention The import filter "dssp_2nd_struct.ift" extracts only the protein
     *             secondary structure which is stored as alignment data. SAIs can simply
     *             be created from these species via move_species_to_extended().
     */

    GB_ERROR  error      = 0;
    GB_begin_transaction(GLOBAL_gb_main);
    char     *sai_name   = 0;
    char     *sec_struct = 0;
    bool      canceled   = false;

    // get the selected species
    char *species_name = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = 0;
    if (!strcmp(species_name, "") || !(gb_species = GBT_find_species(GLOBAL_gb_main, species_name))) {
        error = "Please select a species first.";
    }
    else {
        // search for the field "sec_struct"
        GBDATA *gb_species_sec_struct = GB_entry(gb_species, "sec_struct");
        if (!gb_species_sec_struct) {
            error = "Field \"sec_struct\" not found or empty. Please select another species.";
        }
        else if (!(sec_struct = GB_read_string(gb_species_sec_struct))) {
            error = "Couldn't read field \"sec_struct\". Is it empty?";
        }
        else {
            // generate default name and name input field for the new SAI
            {
                char *sai_default_name = GBS_global_string_copy("%s%s", species_name, strstr(species_name, "_pfold") ? "" : "_pfold");
                sai_name         = aw_input("Name of SAI to create:", sai_default_name);
                free(sai_default_name);
            }

            if (!sai_name) {
                canceled = true;
            }
            else if (strspn(sai_name, " ") == strlen(sai_name)) {
                error = "Name of SAI is empty. Please enter a valid name.";
            }
            else {
                GBDATA *gb_sai_data = GBT_get_SAI_data(GLOBAL_gb_main);
                GBDATA *gb_sai      = GBT_find_SAI_rel_SAI_data(gb_sai_data, sai_name);
                char   *ali_name    = GBT_get_default_alignment(GLOBAL_gb_main);

                if (gb_sai) {
                    error = "SAI with the same name already exists. Please enter another name.";
                }
                else {
                    // create SAI container and copy fields from the species to the SAI
                    gb_sai                   = GB_create_container(gb_sai_data, "extended");
                    GBDATA *gb_species_field = GB_child(gb_species);

                    while (gb_species_field && !error) {
                        char   *key          = GB_read_key(gb_species_field);
                        GBDATA *gb_sai_field = GB_search(gb_sai, GB_read_key(gb_species_field), GB_read_type(gb_species_field));

                        if (strcmp(key, "name") == 0) { // write the new name
                            error = GB_write_string(gb_sai_field, sai_name);
                        }
                        else if (strcmp(key, "sec_struct") == 0) { // write contents from the field "sec_struct" to the alignment data
                            GBDATA *gb_sai_ali     = GB_search(gb_sai, ali_name, GB_CREATE_CONTAINER);
                            if (!gb_sai_ali) error = GB_await_error();
                            else    error          = GBT_write_string(gb_sai_ali, "data", sec_struct);
                        }
                        else if (strcmp(key, "acc") != 0 && strcmp(key, ali_name) != 0) { // don't copy "acc" and the old alignment data
                            error = GB_copy(gb_sai_field, gb_species_field);
                        }
                        gb_species_field = GB_nextChild(gb_species_field);
                        free(key);
                    }

                    // generate accession number and delete field "sec_struct" from the SAI
                    if (!error) {
                        // TODO: is it necessary that a new acc is generated here?
                        GBDATA *gb_sai_acc = GB_search(gb_sai, "acc", GB_FIND);
                        if (gb_sai_acc) {
                            GB_delete(gb_sai_acc);
                            GBT_gen_accession_number(gb_sai, ali_name);
                        }
                        GBDATA *gb_sai_sec_struct = GB_search(gb_sai, "sec_struct", GB_FIND);
                        if (gb_sai_sec_struct) GB_delete(gb_sai_sec_struct);
                        aww->get_root()->awar(AWAR_SAI_NAME)->write_string(sai_name);
                    }
                }
            }
        }
    }

    if (canceled) error = "Aborted by user";

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);

    if (!error) {
        AW_window *sai_info = NT_create_extendeds_window(aww->get_root());
        // TODO: why doesn't info box show anything on first startup? proper refresh needed?
        sai_info->activate();
        ((AWT_canvas *)ntw)->refresh(); // refresh doesn't work, I guess...
    }

    free(species_name);
    free(sai_name);
    free(sec_struct);
}

static void species_create_cb(AW_window * aww) {
    char     *dest  = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GB_ERROR  error = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        GBDATA *gb_species_data     = GB_search(GLOBAL_gb_main, "species_data", GB_CREATE_CONTAINER);
        if (!gb_species_data) error = GB_await_error();
        else {
            GBDATA *gb_dest = GBT_find_species_rel_species_data(gb_species_data, dest);

            if (gb_dest) error = GBS_global_string("Species '%s' already exists", dest);
            else {
                gb_dest             = GBT_find_or_create_species_rel_species_data(gb_species_data, dest);
                if (!gb_dest) error = GB_await_error();
                else aww->get_root()->awar(AWAR_SPECIES_NAME)->write_string(dest);
            }
        }
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(dest);
}

static AW_window *create_species_create_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "CREATE_SPECIES", "SPECIES CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST, 15);

    aws->at("ok");
    aws->callback(species_create_cb);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static GBDATA *expect_species_selected(AW_root *aw_root, char **give_name = 0) {
    GB_transaction  ta(GLOBAL_gb_main);
    char           *name       = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species = GBT_find_species(GLOBAL_gb_main, name);

    if (!gb_species) {
        if (name && name[0]) aw_message(GBS_global_string("Species '%s' does not exist.", name));
        else aw_message("Please select a species first");
    }

    if (give_name) *give_name = name;
    else free(name);

    return gb_species;
}

static void ad_species_copy_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    char    *name;
    GBDATA  *gb_species = expect_species_selected(aw_root, &name);

    if (gb_species) {
        GB_transaction      ta(GLOBAL_gb_main);
        GBDATA             *gb_species_data = GB_get_father(gb_species);
        UniqueNameDetector  und(gb_species_data);
        GB_ERROR            error           = 0;
        char               *copy_name       = AWTC_makeUniqueShortName(GBS_global_string("c%s", name), und);

        if (!copy_name) error = GB_await_error();
        else {
            GBDATA *gb_new_species = GB_create_container(gb_species_data, "species");

            if (!gb_new_species) error = GB_await_error();
            else {
                error = GB_copy(gb_new_species, gb_species);
                if (!error) {
                    error = GBT_write_string(gb_new_species, "name", copy_name);
                    if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string(copy_name); // set focus
                }
            }

            free(copy_name);
        }
        if (error) {
            error = ta.close(error);
            aw_message(error);
        }
    }
}

static void ad_species_rename_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    GBDATA  *gb_species = expect_species_selected(aw_root);
    if (gb_species) {
        GB_transaction  ta(GLOBAL_gb_main);
        GBDATA         *gb_full_name  = GB_search(gb_species, "full_name", GB_STRING);
        const char     *full_name     = gb_full_name ? GB_read_char_pntr(gb_full_name) : "";
        char           *new_full_name = aw_input("Enter new 'full_name' of species:", full_name);

        if (new_full_name) {
            GB_ERROR error = 0;

            if (strcmp(full_name, new_full_name) != 0) {
                error = GB_write_string(gb_full_name, new_full_name);
            }
            if (!error) {
                if (aw_ask_sure("Do you want to re-create the 'name' field?")) {
                    arb_progress progress("Recreating species name", 1);
                    error = AWTC_recreate_name(gb_species);
                    if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string(GBT_read_name(gb_species)); // set focus
                }
            }

            if (error) {
                error = ta.close(error);
                aw_message(error);
            }
        }
    }
}

static void ad_species_delete_cb(AW_window *aww, AW_CL, AW_CL) {
    AW_root  *aw_root    = aww->get_root();
    char     *name;
    GBDATA   *gb_species = expect_species_selected(aw_root, &name);
    GB_ERROR  error      = 0;

    if (!gb_species) {
        error = "Please select a species first";
    }
    else if (aw_ask_sure(GBS_global_string("Are you sure to delete the species '%s'?", name))) {
        GB_transaction ta(GLOBAL_gb_main);
        error = GB_delete(gb_species);
        error = ta.close(error);
        if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string("");
    }

    if (error) aw_message(error);
    free(name);
}

static AW_CL    ad_global_scannerid      = 0;
static AW_root *ad_global_scannerroot    = 0;
static AW_root *ad_global_default_awroot = 0;

void MapViewer_set_default_root(AW_root *aw_root) {
    ad_global_default_awroot = aw_root;
}

static void AD_map_species(AW_root *aw_root, AW_CL scannerid, AW_CL mapOrganism)
{
    GB_push_transaction(GLOBAL_gb_main);
    char *source = aw_root->awar((bool)mapOrganism ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(GLOBAL_gb_main, source);
    if (gb_species) {
        map_db_scanner(scannerid, gb_species, CHANGE_KEY_PATH);
    }
    GB_pop_transaction(GLOBAL_gb_main);
    free(source);
}

void launch_MapViewer_cb(GBDATA *gbd, AD_MAP_VIEWER_TYPE type) {
    GB_ERROR error = GB_push_transaction(GLOBAL_gb_main);

    if (!error) {
        const char *species_name    = "";
        GBDATA     *gb_species_data = GB_search(GLOBAL_gb_main, "species_data", GB_CREATE_CONTAINER);

        if (gbd && GB_get_father(gbd) == gb_species_data) {
            species_name = GBT_read_name(gbd);
        }

        if (ad_global_scannerroot) { // if we have an active info-scanner, then update its awar
            ad_global_scannerroot->awar(AWAR_SPECIES_NAME)->write_string(species_name);
        }
        else if (ad_global_default_awroot) { // no active scanner -> write global awar
            ad_global_default_awroot->awar(AWAR_SPECIES_NAME)->write_string(species_name);
        }
    }

    if (!error && gbd && type == ADMVT_WWW) {
        GBDATA *gb_name       = GB_entry(gbd, "name");
        if (!gb_name) gb_name = GB_entry(gbd, "group_name"); // bad hack, should work

        const char *name = gb_name ? GB_read_char_pntr(gb_name) : "noname";
        error = awt_openURL_by_gbd(GLOBAL_NT.awr, GLOBAL_gb_main, gbd, name);
    }

    error = GB_end_transaction(GLOBAL_gb_main, error);
    if (error) aw_message(error);
}

static int count_key_data_elements(GBDATA *gb_key_data) {
    int nitems  = 0;
    for (GBDATA *gb_cnt = GB_child(gb_key_data); gb_cnt; gb_cnt = GB_nextChild(gb_cnt)) {
        ++nitems;
    }

    return nitems;
}

static void reorder_left_behind_right(AW_window *aws, AW_CL cl_selleft, AW_CL cl_selright) {
    GB_begin_transaction(GLOBAL_gb_main);
    char     *source  = aws->get_root()->awar(AWAR_FIELD_REORDER_SOURCE)->read_string();
    char     *dest    = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_string();
    GB_ERROR  warning = 0;

    AWT_itemfield_selection *sel_left  = (AWT_itemfield_selection*)cl_selleft;
    AWT_itemfield_selection *sel_right = (AWT_itemfield_selection*)cl_selright;

    const ad_item_selector *selector = sel_left->get_selector();
    nt_assert(selector == sel_right->get_selector());

    GBDATA *gb_source = GBT_get_changekey(GLOBAL_gb_main, source, selector->change_key_path);
    GBDATA *gb_dest   = GBT_get_changekey(GLOBAL_gb_main, dest, selector->change_key_path);

    int left_index  = aws->get_index_of_selected_element(sel_left->get_sellist());
    int right_index = aws->get_index_of_selected_element(sel_right->get_sellist());

    if (!gb_source) {
        aw_message("Please select an item you want to move (left box)");
    }
    else if (!gb_dest) {
        aw_message("Please select a destination where to place your item (right box)");
    }
    else if (gb_dest != gb_source) {
        nt_assert(left_index != right_index);

        GBDATA  *gb_key_data = GB_search(GLOBAL_gb_main, selector->change_key_path, GB_CREATE_CONTAINER);
        int      nitems      = count_key_data_elements(gb_key_data);
        GBDATA **new_order   = new GBDATA *[nitems];

        nitems = 0;

        for (GBDATA *gb_key = GB_child(gb_key_data); gb_key; gb_key = GB_nextChild(gb_key)) {
            if (gb_key == gb_source) continue;
            new_order[nitems++] = gb_key;
            if (gb_key == gb_dest) {
                new_order[nitems++] = gb_source;
            }
        }
        warning = GB_resort_data_base(GLOBAL_gb_main, new_order, nitems);
        delete [] new_order;

        if (left_index>right_index) { left_index++; right_index++; } // in one case increment indices
    }

    free(source);
    free(dest);

    GB_commit_transaction(GLOBAL_gb_main);

    if (warning) {
        aw_message(warning);
    }
    else {
        aws->select_element_at_index(sel_left->get_sellist(), left_index);
        aws->select_element_at_index(sel_right->get_sellist(), right_index);
    }
}

static void reorder_up_down(AW_window *aws, AW_CL cl_selector, AW_CL cl_dir) {
    GB_begin_transaction(GLOBAL_gb_main);
    int                     dir      = (int)cl_dir;
    const ad_item_selector *selector = (const ad_item_selector*)cl_selector;
    
    char     *field_name = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_string(); // use the list on the right side
    GBDATA   *gb_field   = GBT_get_changekey(GLOBAL_gb_main, field_name, selector->change_key_path);
    GB_ERROR  warning    = 0;

    if (!gb_field) {
        warning = "Please select an item to move (right box)";
    }
    else {
        GBDATA  *gb_key_data = GB_search(GLOBAL_gb_main, selector->change_key_path, GB_CREATE_CONTAINER);
        int      nitems      = count_key_data_elements(gb_key_data);
        GBDATA **new_order   = new GBDATA *[nitems];

        nitems         = 0;
        int curr_index = -1;

        for (GBDATA *gb_key = GB_child(gb_key_data); gb_key; gb_key = GB_nextChild(gb_key)) {
            if (gb_key == gb_field) curr_index = nitems;
            new_order[nitems++] = gb_key;
        }

        nt_assert(curr_index != -1);
        int new_index = curr_index+dir;

        if (new_index<0 || new_index > nitems) {
            warning = GBS_global_string("Illegal target index '%i'", new_index);
        }
        else {
            if (new_index<curr_index) {
                for (int i = curr_index; i>new_index; --i) new_order[i] = new_order[i-1];
                new_order[new_index] = gb_field;
            }
            else if (new_index>curr_index) {
                for (int i = curr_index; i<new_index; ++i) new_order[i] = new_order[i+1];
                new_order[new_index] = gb_field;
            }

            warning = GB_resort_data_base(GLOBAL_gb_main, new_order, nitems);
        }

        delete [] new_order;
    }

    GB_commit_transaction(GLOBAL_gb_main);
    if (warning) aw_message(warning);
}

AW_window *NT_create_ad_list_reorder(AW_root *root, AW_CL cl_item_selector)
{
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;
    
    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (!awsa[selector->type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector->type]  = aws;

        aws->init(root, "REORDER_FIELDS", "REORDER FIELDS");
        aws->load_xfig("ad_kreo.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        AWT_itemfield_selection *sel1 = awt_create_selection_list_on_itemfields(GLOBAL_gb_main, aws, AWAR_FIELD_REORDER_SOURCE, AWT_NDS_FILTER, "source", 0, selector, 20, 10);
        AWT_itemfield_selection *sel2 = awt_create_selection_list_on_itemfields(GLOBAL_gb_main, aws, AWAR_FIELD_REORDER_DEST,   AWT_NDS_FILTER, "dest",   0, selector, 20, 10);

        aws->button_length(0);

        aws->at("doit");
        aws->callback(reorder_left_behind_right, (AW_CL)sel1, (AW_CL)sel2);
        aws->help_text("spaf_reorder.hlp");
        aws->create_button("MOVE_LEFT_BEHIND_RIGHT", "MOVE LEFT\nBEHIND RIGHT", "L");

        aws->at("doit2");
        aws->callback(reorder_up_down, (AW_CL)selector, -1);
        aws->help_text("spaf_reorder.hlp");
        aws->create_button("MOVE_UP_RIGHT", "MOVE RIGHT\nUP", "U");

        aws->at("doit3");
        aws->callback(reorder_up_down, (AW_CL)selector, 1);
        aws->help_text("spaf_reorder.hlp");
        aws->create_button("MOVE_DOWN_RIGHT", "MOVE RIGHT\nDOWN", "U");
    }
    
    return awsa[selector->type];
}

static void ad_hide_field(AW_window *aws, AW_CL cl_sel, AW_CL cl_hide) {
    AWT_itemfield_selection *item_sel = (AWT_itemfield_selection*)cl_sel;
    GB_ERROR                 error    = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        char                   *source    = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        const ad_item_selector *selector  = item_sel->get_selector();
        GBDATA                 *gb_source = GBT_get_changekey(GLOBAL_gb_main, source, selector->change_key_path);

        if (!gb_source) error = "Please select the field you want to (un)hide";
        else error            = GBT_write_int(gb_source, CHANGEKEY_HIDDEN, int(cl_hide));

        free(source);
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    if (!error) aws->move_selection(item_sel->get_sellist(), 1);
}

static void ad_field_delete(AW_window *aws, AW_CL cl_sel) {
    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        char                    *source     = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        AWT_itemfield_selection *item_sel   = (AWT_itemfield_selection*)cl_sel;
        const ad_item_selector  *selector   = item_sel->get_selector();
        AW_selection_list       *sellist    = item_sel->get_sellist();
        int                      curr_index = aws->get_index_of_selected_element(sellist);
        GBDATA                  *gb_source  = GBT_get_changekey(GLOBAL_gb_main, source, selector->change_key_path);

        if (!gb_source) error = "Please select the field you want to delete";
        else error            = GB_delete(gb_source);

        for (GBDATA *gb_item_container = selector->get_first_item_container(GLOBAL_gb_main, aws->get_root(), AWT_QUERY_ALL_SPECIES);
             !error && gb_item_container;
             gb_item_container = selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA * gb_item = selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item = selector->get_next_item(gb_item))
            {
                GBDATA *gbd = GB_search(gb_item, source, GB_FIND);

                if (gbd) {
                    error = GB_delete(gbd);
                    if (!error) {
                        // item has disappeared, this selects the next one:
                        aws->select_element_at_index(sellist, curr_index);
                    }
                }
            }
        }

        free(source);
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}


AW_window *NT_create_ad_field_delete(AW_root *root, AW_CL cl_item_selector) {
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (!awsa[selector->type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector->type]  = aws;

        aws->init(root, "DELETE_FIELD", "DELETE FIELD");
        aws->load_xfig("ad_delof.fig");
        aws->button_length(6);

        aws->at("close"); aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(AW_POPUP_HELP, (AW_CL)"spaf_delete.hlp");
        aws->create_button("HELP", "HELP", "H");

        AWT_itemfield_selection *item_sel =
            awt_create_selection_list_on_itemfields(GLOBAL_gb_main,
                                                    aws, AWAR_FIELD_DELETE,
                                                    -1,
                                                    "source", 0, selector, 20, 10,
                                                    AWT_SF_HIDDEN);

        aws->button_length(13);
        aws->at("hide");
        aws->callback(ad_hide_field, (AW_CL)item_sel, (AW_CL)1);
        aws->help_text("rm_field_only.hlp");
        aws->create_button("HIDE_FIELD", "Hide field", "H");

        aws->at("unhide");
        aws->callback(ad_hide_field, (AW_CL)item_sel, (AW_CL)0);
        aws->help_text("rm_field_only.hlp");
        aws->create_button("UNHIDE_FIELD", "Unhide field", "U");

        aws->at("delf");
        aws->callback(ad_field_delete, (AW_CL)item_sel);
        aws->help_text("rm_field_cmpt.hlp");
        aws->create_button("DELETE_FIELD", "DELETE FIELD\n(DATA DELETED)", "C");
    }
    
    return awsa[selector->type];
}

static void ad_field_create_cb(AW_window *aws, AW_CL cl_item_selector)
{
    GB_push_transaction(GLOBAL_gb_main);
    char *name = aws->get_root()->awar(AWAR_FIELD_CREATE_NAME)->read_string();
    GB_ERROR error = GB_check_key(name);
    GB_ERROR error2 = GB_check_hkey(name);
    if (error && !error2) {
        aw_message("Warning: Your key contain a '/' character,\n"
                   "    that means it is a hierarchical key");
        error = 0;
    }

    int type = (int)aws->get_root()->awar(AWAR_FIELD_CREATE_TYPE)->read_int();

    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;
    if (!error) error = GBT_add_new_changekey_to_keypath(GLOBAL_gb_main, name, type, selector->change_key_path);
    aws->hide_or_notify(error);
    free(name);
    GB_pop_transaction(GLOBAL_gb_main);
}

AW_window *NT_create_ad_field_create(AW_root *root, AW_CL cl_item_selector)
{
    const ad_item_selector *selector = (const ad_item_selector*)cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (awsa[selector->type]) return (AW_window *)awsa[selector->type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector->type]  = aws;

    aws->init(root, "CREATE_FIELD", "CREATE A NEW FIELD");
    aws->load_xfig("ad_fcrea.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("input");
    aws->label("FIELD NAME");
    aws->create_input_field(AWAR_FIELD_CREATE_NAME, 15);

    aws->at("type");
    aws->create_toggle_field(AWAR_FIELD_CREATE_TYPE, "FIELD TYPE", "F");
    aws->insert_toggle("Ascii Text",        "S", (int)GB_STRING);
    aws->insert_toggle("Link",              "L", (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical", "N", (int)GB_INT);
    aws->insert_toggle("Numerical",         "R", (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text",    "0", (int)GB_BITS);
    aws->update_toggle_field();

    aws->at("ok");
    aws->callback(ad_field_create_cb, cl_item_selector);
    aws->create_button("CREATE", "CREATE", "C");

    return (AW_window *)aws;
}

#if defined(DEVEL_RALF)
#warning GBT_convert_changekey currently only works for species fields, make it work with genes/exp/... as well (use selector)
#endif // DEVEL_RALF

static void ad_field_convert_commit_cb(AW_window *aws, AW_CL /*cl_item_selector*/) {
    // const ad_item_selector *selector = (const ad_item_selector*) cl_item_selector;
    AW_root *root = aws->get_root();
    GB_ERROR error = NULL;

    GB_push_transaction(GLOBAL_gb_main);
    error = GBT_convert_changekey(GLOBAL_gb_main,
                                  root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_string(),
                                  (GB_TYPES)root->awar(AWAR_FIELD_CONVERT_TYPE)->read_int());

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}

static void ad_field_convert_update_typesel_cb(AW_window *aws, AW_CL cl_item_selector) {
    const ad_item_selector *selector = (const ad_item_selector*) cl_item_selector;
    AW_root *root = aws->get_root();

    GB_push_transaction(GLOBAL_gb_main);
    int type = GBT_get_type_of_changekey(
        GLOBAL_gb_main,
        root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_string(),
        selector->change_key_path);
    GB_pop_transaction(GLOBAL_gb_main);

    root->awar(AWAR_FIELD_CONVERT_TYPE)->write_int(type);
}

AW_window *NT_create_ad_field_convert(AW_root *root, AW_CL cl_item_selector) {
    const ad_item_selector *selector = (const ad_item_selector*) cl_item_selector;

    static AW_window_simple *awsa[AWT_QUERY_ITEM_TYPES];
    if (awsa[selector->type]) return (AW_window *)awsa[selector->type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector->type]  = aws;

    aws->init(root, "CONVERT_FIELD", "CONVERT FIELDS");
    aws->load_xfig("ad_conv.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"spaf_convert.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->callback(ad_field_convert_update_typesel_cb, cl_item_selector);
    awt_create_selection_list_on_itemfields(GLOBAL_gb_main,
                                            aws,
                                            AWAR_FIELD_CONVERT_SOURCE, // AWAR containing selection
                                            -1,     // type filter
                                            "source", // selector xfig position
                                            0,      // rescan button xfig position
                                            selector,
                                            40, 20, // selector w,h
                                            AWT_SF_HIDDEN);

    aws->at("typesel");
    aws->create_toggle_field(AWAR_FIELD_CONVERT_TYPE, NULL, "F");
    aws->insert_toggle("Ascii Text",        "S", (int)GB_STRING);
    aws->insert_toggle("Link",              "L", (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical", "N", (int)GB_INT);
    aws->insert_toggle("Numerical",         "R", (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text",    "0", (int)GB_BITS);
    aws->update_toggle_field();

    aws->at("convert");
    aws->callback(ad_field_convert_commit_cb, cl_item_selector);
    aws->create_button("CONVERT", "CONVERT", "T");

    return (AW_window*)aws;
}

void NT_spec_create_field_items(AW_window *aws) {
    aws->insert_menu_topic("spec_reorder_fields", "Reorder fields ...",     "R", "spaf_reorder.hlp", AWM_ALL, AW_POPUP, (AW_CL)NT_create_ad_list_reorder, (AW_CL)&AWT_species_selector);
    aws->insert_menu_topic("spec_delete_field",   "Delete/Hide fields ...", "D", "spaf_delete.hlp",  AWM_EXP, AW_POPUP, (AW_CL)NT_create_ad_field_delete, (AW_CL)&AWT_species_selector);
    aws->insert_menu_topic("spec_create_field",   "Create fields ...",      "C", "spaf_create.hlp",  AWM_ALL, AW_POPUP, (AW_CL)NT_create_ad_field_create, (AW_CL)&AWT_species_selector);
    aws->insert_menu_topic("spec_convert_field",  "Convert fields ...",     "t", "spaf_convert.hlp", AWM_EXP, AW_POPUP, (AW_CL)NT_create_ad_field_convert, (AW_CL)&AWT_species_selector);
    aws->insert_separator();
    aws->insert_menu_topic("spec_unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AWM_ALL, (AW_CB)awt_selection_list_unhide_all_cb, (AW_CL)GLOBAL_gb_main, AWT_NDS_FILTER);
    aws->insert_separator();
    aws->insert_menu_topic("spec_scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AWM_ALL, (AW_CB)awt_selection_list_scan_unknown_cb,  (AW_CL)GLOBAL_gb_main, AWT_NDS_FILTER);
    aws->insert_menu_topic("spec_del_unused_fields",   "Forget unused fields",  "e", "scandb.hlp", AWM_ALL, (AW_CB)awt_selection_list_delete_unused_cb, (AW_CL)GLOBAL_gb_main, AWT_NDS_FILTER);
    aws->insert_menu_topic("spec_refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AWM_ALL, (AW_CB)awt_selection_list_update_cb,        (AW_CL)GLOBAL_gb_main, AWT_NDS_FILTER);
}

inline int get_and_fix_range_from_awar(AW_awar *awar) {
    const char *input = awar->read_char_pntr();
    int         bpos = atoi(input);
    int         ipos;

    if (bpos>0) {
        awar->write_string(GBS_global_string("%i", bpos));
        ipos = bio2info(bpos);
    }
    else {
        ipos = -1;
        awar->write_string("");
    }
    return ipos;
}

static TargetRange get_nn_range_from_awars(AW_root *aw_root) {
    int start = get_and_fix_range_from_awar(aw_root->awar(AWAR_NN_RANGE_START));
    int end   = get_and_fix_range_from_awar(aw_root->awar(AWAR_NN_RANGE_END));

    return TargetRange(start, end);
}

static char *read_sequence_region(GBDATA *gb_data, TargetRange& range) {
    const char *cdata  = GB_read_char_pntr(gb_data);
    int         seqlen = GB_read_count(gb_data);
    range.fit(seqlen);
    int   len  = range.length();
    char *part = (char*)malloc(len+1);

    memcpy(part, cdata+range.first_pos(), len);
    part[len] = 0;

    return part;
}

static void awtc_nn_search_all_listed(AW_window *aww, AW_CL _cbs) {
    DbQuery *cbs = (DbQuery *)_cbs;

    nt_assert(cbs->selector->type == AWT_QUERY_ITEM_SPECIES);

    GB_begin_transaction(GLOBAL_gb_main);

    AW_root *aw_root    = aww->get_root();
    char    *dest_field = aw_root->awar(AWAR_NN_DEST_FIELD)->read_string();

    GB_ERROR error     = 0;
    GB_TYPES dest_type = GBT_get_type_of_changekey(GLOBAL_gb_main, dest_field, CHANGE_KEY_PATH);
    if (!dest_type) {
        error = GB_export_error("Please select a valid field");
    }

    if (strcmp(dest_field, "name")==0) {
        int answer = aw_question("CAUTION! This will destroy all name-fields of the listed species.\n",
                                 "Continue and destroy all name-fields,Abort");

        if (answer==1) {
            error = GB_export_error("Aborted by user");
        }
    }

    long         max = awt_count_queried_items(cbs, AWT_QUERY_ALL_SPECIES);
    arb_progress progress("Searching next neighbours", max);
    progress.auto_subtitles("Species");

    int            pts            = aw_root->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    char          *ali_name       = aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
    int            oligo_len      = aw_root->awar(AWAR_NN_OLIGO_LEN)->read_int();
    int            mismatches     = aw_root->awar(AWAR_NN_MISMATCHES)->read_int();
    bool           fast_mode      = aw_root->awar(AWAR_NN_FAST_MODE)->read_int();
    FF_complement  compl_mode     = static_cast<FF_complement>(aw_root->awar(AWAR_NN_COMPLEMENT)->read_int());
    bool           rel_matches    = aw_root->awar(AWAR_NN_REL_MATCHES)->read_int();
    int            wanted_entries = aw_root->awar(AWAR_NN_WANTED_ENTRIES)->read_int();
    bool           scored_entries = aw_root->awar(AWAR_NN_SCORED_ENTRIES)->read_int();
    int            min_score      = aw_root->awar(AWAR_NN_MIN_SCORE)->read_int();

    TargetRange org_range = get_nn_range_from_awars(aw_root);

    for (GBDATA *gb_species = GBT_first_species(GLOBAL_gb_main);
         !error && gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        if (!IS_QUERIED(gb_species, cbs)) continue;

        GBDATA *gb_data = GBT_read_sequence(gb_species, ali_name);
        if (gb_data) {
            TargetRange      range    = org_range; // modified by read_sequence_region
            char            *sequence = read_sequence_region(gb_data, range);
            PT_FamilyFinder  ff(GLOBAL_gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches);

            ff.restrict_2_region(range);

            error = ff.searchFamily(sequence, compl_mode, wanted_entries);
            if (!error) {
                const FamilyList *fm = ff.getFamilyList();

                GBS_strstruct *value = NULL;
                while (fm) {
                    const char *thisValue = 0;
                    if (rel_matches) {
                        if ((fm->rel_matches*100) > min_score) {
                            thisValue = scored_entries
                                ? GBS_global_string("%.1f%%:%s", fm->rel_matches*100, fm->name)
                                : fm->name;
                        }
                    }
                    else {
                        if (fm->matches > min_score) {
                            thisValue = scored_entries
                                ? GBS_global_string("%li:%s", fm->matches, fm->name)
                                : fm->name;
                        }
                    }

                    if (thisValue) {
                        if (value == NULL) { // first entry
                            value = GBS_stropen(1000);
                        }
                        else {
                            GBS_chrcat(value, ';');
                        }
                        GBS_strcat(value, thisValue);
                    }

                    fm = fm->next;
                }

                if (value) {
                    GBDATA *gb_dest = GB_search(gb_species, dest_field, dest_type);

                    error = GB_write_as_string(gb_dest, GBS_mempntr(value));
                    GBS_strforget(value);
                }
                else {
                    GBDATA *gb_dest = GB_search(gb_species, dest_field, GB_FIND);
                    if (gb_dest) error = GB_delete(gb_dest);
                }
            }
            free(sequence);
        }
        progress.inc_and_check_user_abort(error);
    }
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(dest_field);
    free(ali_name);
}

static void awtc_nn_search(AW_window *aww, AW_CL id) {
    AW_root     *aw_root  = aww->get_root();
    GB_ERROR     error    = 0;
    TargetRange  range    = get_nn_range_from_awars(aw_root);
    char        *sequence = 0;
    {
        GB_transaction ta(GLOBAL_gb_main);

        char   *sel_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_species  = GBT_find_species(GLOBAL_gb_main, sel_species);

        if (!gb_species) {
            error = "Select a species first";
        }
        else {
            char   *ali_name = aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
            GBDATA *gb_data  = GBT_read_sequence(gb_species, ali_name);

            if (gb_data) {
                sequence = read_sequence_region(gb_data, range);
            }
            else {
                error = GBS_global_string("Species '%s' has no sequence '%s'", sel_species, ali_name);
            }
            free(ali_name);
        }
        free(sel_species);
    }

    int  pts         = aw_root->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    int  oligo_len   = aw_root->awar(AWAR_NN_OLIGO_LEN)->read_int();
    int  mismatches  = aw_root->awar(AWAR_NN_MISMATCHES)->read_int();
    bool fast_mode   = aw_root->awar(AWAR_NN_FAST_MODE)->read_int();
    bool rel_matches = aw_root->awar(AWAR_NN_REL_MATCHES)->read_int();


    PT_FamilyFinder ff(GLOBAL_gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches);

    ff.restrict_2_region(range);

    int max_hits = 0; // max wanted hits

    if (!error) {
        FF_complement compl_mode = static_cast<FF_complement>(aw_root->awar(AWAR_NN_COMPLEMENT)->read_int());
        max_hits                 = aw_root->awar(AWAR_NN_MAX_HITS)->read_int();

        error = ff.searchFamily(sequence, compl_mode, max_hits);
    }

    // update result list
    {
        AW_selection_list* sel = (AW_selection_list *)id;
        aww->clear_selection_list(sel);

        int hits = 0;
        if (error) {
            aw_message(error);
            aww->insert_default_selection(sel, "<Error>", "");
        }
        else {
            int count    = 1;
            int numWidth = log(max_hits)/log(10)+1;

            for (const FamilyList *fm = ff.getFamilyList(); fm; fm = fm->next) {
                const char *dis;
                if (rel_matches) {
                    dis = GBS_global_string("#%0*i %-12s Rel.hits: %5.1f%%", numWidth, count, fm->name, fm->rel_matches*100);
                }
                else {
                    dis = GBS_global_string("#%0*i %-12s Hits: %4li", numWidth, count, fm->name, fm->matches);
                }

                aww->insert_selection(sel, dis, fm->name);
                count++;
            }

            aww->insert_default_selection(sel, ff.hits_were_truncated() ? "<List truncated>" : "<No more hits>", "");
            hits = ff.getRealHits();
        }
        aw_root->awar(AWAR_NN_HIT_COUNT)->write_int(hits);
        aww->update_selection_list(sel);
    }

    free(sequence);
}

static void awtc_move_hits(AW_window *aww, AW_CL id, AW_CL cbs) {
    AW_root *aw_root         = aww->get_root();
    char    *current_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();

    if (!current_species) current_species = strdup("<unknown>");

    char *hit_description = GBS_global_string_copy("<neighbour of %s: %%s>", current_species);

    awt_copy_selection_list_2_queried_species((DbQuery *)cbs, (AW_selection_list *)id, hit_description);

    free(hit_description);
    free(current_species);
}

static void create_next_neighbours_vars(AW_root *aw_root) {
    static bool created = false;

    if (!created) {
        aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER);
        aw_root->awar_int(AWAR_NN_COMPLEMENT,  FF_FORWARD);

        aw_root->awar_int(AWAR_NN_MAX_HITS,  50);
        aw_root->awar_int(AWAR_NN_HIT_COUNT, 0);

        aw_root->awar_string(AWAR_NN_DEST_FIELD, "tmp");
        aw_root->awar_int(AWAR_NN_WANTED_ENTRIES, 5);
        aw_root->awar_int(AWAR_NN_SCORED_ENTRIES, 1);
        aw_root->awar_int(AWAR_NN_MIN_SCORE,      80);

        aw_root->awar_string(AWAR_NN_RANGE_START, "");
        aw_root->awar_string(AWAR_NN_RANGE_END,   "");

        AWTC_create_common_next_neighbour_vars(aw_root);

        created = true;
    }
}

static void create_common_next_neighbour_fields(AW_window *aws) {
    aws->at("pt_server");
    awt_create_selection_list_on_pt_servers(aws, AWAR_PROBE_ADMIN_PT_SERVER, true);

    AWTC_create_common_next_neighbour_fields(aws);

    aws->auto_space(5, 5);
    
    aws->at("range");
    aws->create_input_field(AWAR_NN_RANGE_START, 6);
    aws->create_input_field(AWAR_NN_RANGE_END,   6);
    
    aws->at("compl");
    aws->create_option_menu(AWAR_NN_COMPLEMENT, 0, 0);
    aws->insert_default_option("forward",            "", FF_FORWARD);
    aws->insert_option        ("reverse",            "", FF_REVERSE);
    aws->insert_option        ("complement",         "", FF_COMPLEMENT);
    aws->insert_option        ("reverse-complement", "", FF_REVERSE_COMPLEMENT);
    aws->insert_option        ("fwd + rev-compl",    "", FF_FORWARD|FF_REVERSE_COMPLEMENT);
    aws->insert_option        ("rev + compl",        "", FF_REVERSE|FF_COMPLEMENT);
    aws->insert_option        ("any",                "", FF_FORWARD|FF_REVERSE|FF_COMPLEMENT|FF_REVERSE_COMPLEMENT);
    aws->update_option_menu();
}

static AW_window *ad_spec_next_neighbours_listed_create(AW_root *aw_root, AW_CL cbs) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        create_next_neighbours_vars(aw_root);

        aws = new AW_window_simple;
        aws->init(aw_root, "SEARCH_NEXT_RELATIVES_OF_LISTED", "Search Next Neighbours of Listed");
        aws->load_xfig("ad_spec_nnm.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours_listed.hlp");
        aws->create_button("HELP", "HELP", "H");

        create_common_next_neighbour_fields(aws);

        aws->at("entries");
        aws->create_input_field(AWAR_NN_WANTED_ENTRIES, 3);

        aws->at("add_score");
        aws->create_toggle(AWAR_NN_SCORED_ENTRIES);

        aws->at("min_score");
        aws->create_input_field(AWAR_NN_MIN_SCORE, 5);

        aws->at("field");
        awt_create_selection_list_on_itemfields(GLOBAL_gb_main, aws, AWAR_NN_DEST_FIELD,
                                                (1<<GB_INT) | (1<<GB_STRING), "field", 0,
                                                &AWT_species_selector, 20, 10);

        aws->at("go");
        aws->callback(awtc_nn_search_all_listed, cbs);
        aws->create_button("WRITE_FIELDS", "Write to field");
    }
    return aws;
}

static AW_window *ad_spec_next_neighbours_create(AW_root *aw_root, AW_CL cbs) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        create_next_neighbours_vars(aw_root);

        aws = new AW_window_simple;
        aws->init(aw_root, "SEARCH_NEXT_RELATIVE_OF_SELECTED", "Search Next Neighbours");
        aws->load_xfig("ad_spec_nn.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"next_neighbours.hlp");
        aws->create_button("HELP", "HELP", "H");

        create_common_next_neighbour_fields(aws);

        aws->at("results");
        aws->create_input_field(AWAR_NN_MAX_HITS, 3);

        aws->at("hit_count");
        aws->create_button(0, AWAR_NN_HIT_COUNT, 0, "+");

        aws->at("hits");
        AW_selection_list *id = aws->create_selection_list(AWAR_SPECIES_NAME);
        aws->insert_default_selection(id, "No hits found", "");
        aws->update_selection_list(id);

        aws->at("go");
        aws->callback(awtc_nn_search, (AW_CL)id);
        aws->create_button("SEARCH", "SEARCH");

        aws->at("move");
        aws->callback(awtc_move_hits, (AW_CL)id, cbs);
        aws->create_button("MOVE_TO_HITLIST", "MOVE TO HITLIST");

    }
    return aws;
}

// -----------------------------------------------------------------------------------------------------------------
//      void NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_Awar_Callback_Info)
// -----------------------------------------------------------------------------------------------------------------

void NT_detach_information_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information) {
    AW_window **aww_pointer = (AW_window**)cl_pointer_to_aww;

    AW_detach_information *di           = (AW_detach_information*)cl_AW_detach_information;
    Awar_Callback_Info    *cb_info      = di->get_cb_info();
    AW_root               *awr          = cb_info->get_root();
    char                  *curr_species = awr->awar(cb_info->get_org_awar_name())->read_string();

    if (*aww_pointer == aww) {  // first click on detach-button
        // create unique awar :
        static int detach_counter = 0;
        char       new_awar[100];
        sprintf(new_awar, "tmp/DETACHED_INFO_%i", detach_counter++);
        awr->awar_string(new_awar, "", AW_ROOT_DEFAULT);

        cb_info->remap(new_awar); // remap the callback from old awar to new unique awar
        aww->update_label(di->get_detach_button(), "GET");

        *aww_pointer = 0;       // caller window will be recreated on next open after clearing this pointer
        // [Note : the aww_pointer points to the static window pointer]
    }

    awr->awar(cb_info->get_awar_name())->write_string(curr_species);
    aww->set_window_title(GBS_global_string("%s INFORMATION", curr_species));
    free(curr_species);
}

static AW_window *create_speciesOrganismWindow(AW_root *aw_root, GBDATA *gb_main, bool organismWindow) {
    int windowIdx = (int)organismWindow;

    static AW_window_simple_menu *AWS[2] = { 0, 0 };
    if (!AWS[windowIdx]) {
        AW_window_simple_menu *& aws = AWS[windowIdx];

        aws = new AW_window_simple_menu;
        if (organismWindow) aws->init(aw_root, "ORGANISM_INFORMATION", "ORGANISM INFORMATION");
        else                aws->init(aw_root, "SPECIES_INFORMATION", "SPECIES INFORMATION");
        aws->load_xfig("ad_spec.fig");

        aws->button_length(8);

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("search");
        aws->callback(AW_POPUP, (AW_CL)NTX_create_query_window, 0);
        aws->create_button("SEARCH", "SEARCH", "S");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"sp_info.hlp");
        aws->create_button("HELP", "HELP", "H");

        AW_CL scannerid = create_db_scanner(gb_main, aws, "box", 0, "field", "enable", DB_VIEWER, 0, "mark", AWT_NDS_FILTER,
                                            organismWindow ? &AWT_organism_selector : &AWT_species_selector);
        ad_global_scannerid = scannerid;
        ad_global_scannerroot = aws->get_root();

        if (organismWindow) aws->create_menu("ORGANISM",    "O", AWM_ALL);
        else                aws->create_menu("SPECIES",     "S", AWM_ALL);

        aws->insert_menu_topic("species_delete",        "Delete",         "D", "spa_delete.hlp",  AWM_ALL, ad_species_delete_cb,            0,                                   0);
        aws->insert_menu_topic("species_rename",        "Rename",         "R", "spa_rename.hlp",  AWM_ALL, ad_species_rename_cb,            0,                                   0);
        aws->insert_menu_topic("species_copy",          "Copy",           "y", "spa_copy.hlp",    AWM_ALL, ad_species_copy_cb,              0,                                   0);
        aws->insert_menu_topic("species_create",        "Create",         "C", "spa_create.hlp",  AWM_ALL, AW_POPUP,                        (AW_CL)create_species_create_window, 0);
        aws->insert_menu_topic("species_convert_2_sai", "Convert to SAI", "S", "sp_sp_2_ext.hlp", AWM_ALL, (AW_CB)move_species_to_extended, 0,                                   0);
        aws->insert_separator();

        aws->create_menu("FIELDS", "F", AWM_ALL);
        NT_spec_create_field_items(aws);

        {
            const char         *awar_name = (bool)organismWindow ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME;
            AW_root            *awr       = aws->get_root();
            Awar_Callback_Info *cb_info   = new Awar_Callback_Info(awr, awar_name, AD_map_species, (AW_CL)scannerid, (AW_CL)organismWindow); // do not delete!
            cb_info->add_callback();

            AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!

            aws->at("detach");
            aws->callback(NT_detach_information_window, (AW_CL)&aws, (AW_CL)detach_info);
            aws->create_button("DETACH", "DETACH", "D");

            detach_info->set_detach_button(aws->get_last_widget());
        }

        aws->show();
        AD_map_species(aws->get_root(), scannerid, (AW_CL)organismWindow);
    }
    return AWS[windowIdx]; // already created (and not detached)
}

AW_window *NT_create_species_window(AW_root *aw_root, AW_CL cl_gb_main) {
    return create_speciesOrganismWindow(aw_root, (GBDATA*)cl_gb_main, false);
}
AW_window *NTX_create_organism_window(AW_root *aw_root, AW_CL cl_gb_main) {
    return create_speciesOrganismWindow(aw_root, (GBDATA*)cl_gb_main, true);
}

AW_CL ad_query_global_cbs = 0;

void NT_unquery_all() {
    awt_unquery_all(0, (DbQuery *)ad_query_global_cbs);
}

void NT_query_update_list() {
    awt_query_update_list(NULL, (DbQuery *)ad_query_global_cbs);
}

AW_window *NTX_create_query_window(AW_root *aw_root)
{
    static AW_window_simple_menu *aws = 0;
    if (aws) {
        return (AW_window *)aws;
    }
    aws = new AW_window_simple_menu;
    aws->init(aw_root, "SPECIES_QUERY", "SEARCH and QUERY");
    aws->create_menu("More functions", "f");
    aws->load_xfig("ad_query.fig");


    awt_query_struct awtqs;

    awtqs.gb_main             = GLOBAL_gb_main;
    awtqs.species_name        = AWAR_SPECIES_NAME;
    awtqs.tree_name           = AWAR_TREE;
    awtqs.select_bit          = 1;
    awtqs.use_menu            = 1;
    awtqs.ere_pos_fig         = "ere2";
    awtqs.by_pos_fig          = "by2";
    awtqs.qbox_pos_fig        = "qbox";
    awtqs.rescan_pos_fig      = 0;
    awtqs.key_pos_fig         = 0;
    awtqs.query_pos_fig       = "content";
    awtqs.result_pos_fig      = "result";
    awtqs.count_pos_fig       = "count";
    awtqs.do_query_pos_fig    = "doquery";
    awtqs.config_pos_fig      = "doconfig";
    awtqs.do_mark_pos_fig     = "domark";
    awtqs.do_unmark_pos_fig   = "dounmark";
    awtqs.do_delete_pos_fig   = "dodelete";
    awtqs.do_set_pos_fig      = "doset";
    awtqs.do_refresh_pos_fig  = "dorefresh";
    awtqs.open_parser_pos_fig = "openparser";
    awtqs.create_view_window  = NT_create_species_window;
    awtqs.selector            = &AWT_species_selector;

    AW_CL cbs           = (AW_CL)awt_create_query_box(aws, &awtqs, "spec");
    ad_query_global_cbs = cbs;

    aws->create_menu("More search",     "s");
    aws->insert_menu_topic("spec_search_equal_fields_within_db", "Search For Equal Fields and Mark Duplicates",                "E", "search_duplicates.hlp", AWM_ALL, (AW_CB)awt_search_equal_entries, cbs,                                          0);
    aws->insert_menu_topic("spec_search_equal_words_within_db",  "Search For Equal Words Between Fields and Mark Duplicates",  "W", "search_duplicates.hlp", AWM_ALL, (AW_CB)awt_search_equal_entries, cbs,                                          1);
    aws->insert_menu_topic("spec_search_next_relativ_of_sel",    "Search Next Relatives of SELECTED Species in PT_Server ...", "R", 0,                       AWM_ALL, (AW_CB)AW_POPUP,                 (AW_CL)ad_spec_next_neighbours_create,        cbs);
    aws->insert_menu_topic("spec_search_next_relativ_of_listed", "Search Next Relatives of LISTED Species in PT_Server ...",   "L", 0,                       AWM_ALL, (AW_CB)AW_POPUP,                 (AW_CL)ad_spec_next_neighbours_listed_create, cbs);

    aws->button_length(7);

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"sp_search.hlp");
    aws->create_button("HELP", "HELP", "H");

    return (AW_window *)aws;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <arb_unit_test.h>

static uint32_t counted_chars_checksum(GBDATA *gb_main)  {
    GB_transaction ta(gb_main);

    GBDATA *gb_sai;
    GBDATA *gb_ali;
    GBDATA *gb_counted_chars;

    char *ali_name = GBT_get_default_alignment(gb_main);

    TEST_ASSERT_RESULT__NOERROREXPORTED(gb_sai = GBT_expect_SAI(gb_main, SAI_COUNTED_CHARS));
    TEST_ASSERT_RESULT__NOERROREXPORTED(gb_ali = GB_entry(gb_sai, ali_name));
    TEST_ASSERT_RESULT__NOERROREXPORTED(gb_counted_chars = GB_entry(gb_ali, "data"));

    const char *data = GB_read_char_pntr(gb_counted_chars);

    free(ali_name);

    return GBS_checksum(data, 0, NULL);
}

void TEST_count_chars() {
    // calculate SAI for test DBs

    arb_suppress_progress silence;
    GB_shell              shell;

    for (int prot = 0; prot<2; ++prot) {
        GBDATA *gb_main;
        TEST_ASSERT_RESULT__NOERROREXPORTED(gb_main = GB_open(prot ? "TEST_prot.arb" : "TEST_nuc.arb", "rw"));

        GBT_mark_all(gb_main, 1);
        NT_count_different_chars(NULL, (AW_CL)gb_main, 0);

        uint32_t expected = prot ? 0x4fa63fa0 : 0xefb05e4e;
        TEST_ASSERT_EQUAL(counted_chars_checksum(gb_main), expected);

        GB_close(gb_main);
    }
}
void TEST_SLOW_count_chars() {
    // calculate a real big test alignment
    //
    // the difference to TEST_count_chars() is just in size of alignment.
    // NT_count_different_chars() crashes for big alignments when running in gdb
    arb_suppress_progress silence;
    GB_shell              shell;
    {
        arb_unit_test::test_alignment_data data_source[] = {
            { 1, "s1", "ACGT" },
            { 1, "s2", "ACGTN" },
            { 1, "s3", "NANNAN" },
            { 1, "s4", "GATTACA" },
        };

        const int alilen = 50000;
        const int count  = ARRAY_ELEMS(data_source);

        char *longSeq[count];
        for (int c = 0; c<count; ++c) {
            char *dest = longSeq[c] = (char*)malloc(alilen+1);

            const char *source = data_source[c].data;
            int         len    = strlen(source);

            for (int p = 0; p<alilen; ++p) {
                dest[p] = source[p%len];
            }
            dest[alilen] = 0;
            
            data_source[c].data = dest;
        }

        ARB_ERROR  error;
        GBDATA    *gb_main = TEST_CREATE_DB(error, "ali_test", data_source, false);

        TEST_ASSERT_NO_ERROR(error.deliver());

        NT_count_different_chars(NULL, (AW_CL)gb_main, 0);

        // TEST_ASSERT_EQUAL(counted_chars_checksum(gb_main), 0x1d34a14f);
        // TEST_ASSERT_EQUAL(counted_chars_checksum(gb_main), 0x609d788b);
        TEST_ASSERT_EQUAL(counted_chars_checksum(gb_main), 0xccdfa527);

        for (int c = 0; c<count; ++c) {
            free(longSeq[c]);
        }

        GB_close(gb_main);
    }
}

#endif // UNIT_TESTS

