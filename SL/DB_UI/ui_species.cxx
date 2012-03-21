// =============================================================== //
//                                                                 //
//   File      : ui_species.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "dbui.h"

#include <awt_sel_boxes.hxx>
#include <arb_strbuf.h>
#include <cmath>
#include <probe_design.hxx>
#include <arb_defs.h>
#include <awtc_next_neighbours.hxx>
#include <db_scanner.hxx>
#include <db_query.h>
#include <AW_rename.hxx>
#include <aw_awar_defs.hxx>
#include <aw_detach.hxx>
#include <aw_msg.hxx>
#include <aw_question.hxx>
#include <algorithm>
#include <arb_progress.h>
#include <item_sel_list.h>

using namespace DBUI;
using namespace QUERY;

#define ui_assert(cond) arb_assert(cond)

#define AWAR_SPECIES_DEST "tmp/adspec/dest"
#define AWAR_SPECIES_INFO "tmp/adspec/info"
#define AWAR_SPECIES_KEY  "tmp/adspec/key"
#define AWAR_FIELD_REORDER_SOURCE "tmp/ad_reorder/source"
#define AWAR_FIELD_REORDER_DEST   "tmp/ad_reorder/dest"
#define AWAR_FIELD_REORDER_ORDER  "tmp/ad_reorder/order"
#define AWAR_FIELD_CREATE_NAME "tmp/adfield/name"
#define AWAR_FIELD_CREATE_TYPE "tmp/adfield/type"
#define AWAR_FIELD_DELETE      "tmp/adfield/source"
#define AWAR_FIELD_CONVERT_SOURCE "tmp/adconvert/source"
#define AWAR_FIELD_CONVERT_NAME   "tmp/adconvert/name"
#define AWAR_FIELD_CONVERT_TYPE   "tmp/adconvert/type"

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


enum ReorderMode {
    // real orders
    ORDER_ALPHA,
    ORDER_TYPE,
    ORDER_FREQ,
    
    // special modes
    RIGHT_BEHIND_LEFT,
    REVERSE_ORDER,
};

void DBUI::create_dbui_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_string(AWAR_SPECIES_DEST,         "",          aw_def);
    aw_root->awar_string(AWAR_SPECIES_INFO,         "",          aw_def);
    aw_root->awar_string(AWAR_SPECIES_KEY,          "",          aw_def);
    aw_root->awar_string(AWAR_FIELD_REORDER_SOURCE, "",          aw_def);
    aw_root->awar_string(AWAR_FIELD_REORDER_DEST,   "",          aw_def);
    aw_root->awar_int   (AWAR_FIELD_REORDER_ORDER,  ORDER_ALPHA, aw_def);
    aw_root->awar_string(AWAR_FIELD_CREATE_NAME,    "",          aw_def);
    aw_root->awar_int   (AWAR_FIELD_CREATE_TYPE,    GB_STRING,   aw_def);
    aw_root->awar_string(AWAR_FIELD_DELETE,         "",          aw_def);
    aw_root->awar_string(AWAR_FIELD_CONVERT_SOURCE, "",          aw_def);
    aw_root->awar_int   (AWAR_FIELD_CONVERT_TYPE,   GB_STRING,   aw_def);
    aw_root->awar_string(AWAR_FIELD_CONVERT_NAME,   "",          aw_def);
}

static void move_species_to_extended(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    char     *source  = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_sai_data     = GBT_get_SAI_data(gb_main);
        if (!gb_sai_data) error = GB_await_error();
        else {
            GBDATA *gb_species = GBT_find_species(gb_main, source);
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
    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(source);
}


static void species_create_cb(AW_window * aww, AW_CL cl_gb_main) {
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    char     *dest    = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_species_data     = GBT_get_species_data(gb_main);
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
    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(dest);
}

static AW_window *create_species_create_window(AW_root *root, AW_CL cl_gb_main) {
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
    aws->callback(species_create_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static GBDATA *expect_species_selected(AW_root *aw_root, GBDATA *gb_main, char **give_name = 0) {
    GB_transaction  ta(gb_main);
    char           *name       = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species = GBT_find_species(gb_main, name);

    if (!gb_species) {
        if (name && name[0]) aw_message(GBS_global_string("Species '%s' does not exist.", name));
        else aw_message("Please select a species first");
    }

    if (give_name) *give_name = name;
    else free(name);

    return gb_species;
}

static void species_copy_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    GBDATA  *gb_main    = (GBDATA*)cl_gb_main;
    char    *name;
    GBDATA  *gb_species = expect_species_selected(aw_root, gb_main, &name);

    if (gb_species) {
        GB_transaction      ta(gb_main);
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

static void species_rename_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    AW_root *aw_root    = aww->get_root();
    GBDATA  *gb_main    = (GBDATA*)cl_gb_main;
    GBDATA  *gb_species = expect_species_selected(aw_root, gb_main);
    if (gb_species) {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_full_name  = GB_search(gb_species, "full_name", GB_STRING);
        const char     *full_name     = gb_full_name ? GB_read_char_pntr(gb_full_name) : "";
        char           *new_full_name = aw_input("Enter new 'full_name' of species:", full_name);

        if (new_full_name) {
            GB_ERROR error = 0;

            if (strcmp(full_name, new_full_name) != 0) {
                error = GB_write_string(gb_full_name, new_full_name);
            }
            if (!error) {
                if (aw_ask_sure("recreate_name_field", "Do you want to re-create the 'name' field?")) {
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

static void species_delete_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    AW_root  *aw_root    = aww->get_root();
    GBDATA   *gb_main    = (GBDATA*)cl_gb_main;
    char     *name;
    GBDATA   *gb_species = expect_species_selected(aw_root, gb_main, &name);
    GB_ERROR  error      = 0;

    if (!gb_species) {
        error = "Please select a species first";
    }
    else if (aw_ask_sure("info_delete_species", GBS_global_string("Are you sure to delete the species '%s'?", name))) {
        GB_transaction ta(gb_main);
        error = GB_delete(gb_species);
        error = ta.close(error);
        if (!error) aw_root->awar(AWAR_SPECIES_NAME)->write_string("");
    }

    if (error) aw_message(error);
    free(name);
}

static void map_species(AW_root *aw_root, AW_CL scannerid, AW_CL mapOrganism) {
    DbScanner *scanner = (DbScanner*)scannerid;
    GBDATA    *gb_main = get_db_scanner_main(scanner);
    GB_push_transaction(gb_main);

    const char *name       = aw_root->awar((bool)mapOrganism ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME)->read_char_pntr();
    GBDATA     *gb_species = GBT_find_species(gb_main, name);
    if (gb_species) map_db_scanner(scanner, gb_species, CHANGE_KEY_PATH);
    GB_pop_transaction(gb_main);
}

static long count_field_occurrance(BoundItemSel *bsel, const char *field_name) {
    QUERY_RANGE   RANGE = QUERY_ALL_ITEMS;
    long          count = 0;
    ItemSelector& sel   = bsel->selector;

    for (GBDATA *gb_container = sel.get_first_item_container(bsel->gb_main, NULL, RANGE);
         gb_container;
         gb_container = sel.get_next_item_container(gb_container, RANGE))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_container, RANGE);
             gb_item;
             gb_item = sel.get_next_item(gb_item, RANGE))
        {
            GBDATA *gb_field = GB_entry(gb_item, field_name);
            if (gb_field) ++count;
        }
    }
    return count;
}

class KeySorter : virtual Noncopyable {
    int      key_count;
    GBDATA **key;

    int field_count; // = key_count - container_count

    bool order_changed;

    // helper variables for sorting
    static GB_HASH      *order_hash;
    static BoundItemSel *bitem_selector;
    static arb_progress *sort_progress;

    bool legal_pos(int p) { return p >= 0 && p<key_count; }
    bool legal_field_pos(int p) const { return p >= 0 && p<field_count; }
    bool legal_field_pos(int p1, int p2) const { return legal_field_pos(p1) && legal_field_pos(p2); }

    void swap(int p1, int p2) {
        ui_assert(legal_pos(p1));
        ui_assert(legal_pos(p2));

        GBDATA *k = key[p1];
        key[p1]   = key[p2];
        key[p2]   = k;

        order_changed = true;
    }

    const char *field_name(int p) const {
        GBDATA *gb_key_name = GB_entry(key[p], "key_name");
        if (gb_key_name) return GB_read_char_pntr(gb_key_name);
        return NULL;
    }
    GB_TYPES field_type(int p) const {
        GBDATA *gb_key_type = GB_entry(key[p], "key_type");
        if (gb_key_type) return GB_TYPES(GB_read_int(gb_key_type));
        return GB_NONE;
    }
    int field_freq(int p) const {
        const char *name            = field_name(p);
        if (!order_hash) order_hash = GBS_create_hash(key_count, GB_IGNORE_CASE);

        long freq = GBS_read_hash(order_hash, name);
        if (!freq) {
            freq = 1+count_field_occurrance(bitem_selector, name);
            GBS_write_hash(order_hash, name, freq);
            if (sort_progress) sort_progress->inc();
        }
        return freq;
    }

    int compare(int p1, int p2, ReorderMode mode) {
        switch (mode) {
            case RIGHT_BEHIND_LEFT:
            case REVERSE_ORDER: 
                ui_assert(0); // illegal ReorderMode
                break;

            case ORDER_TYPE:  return field_type(p2)-field_type(p1);
            case ORDER_ALPHA: return strcasecmp(field_name(p1), field_name(p2));
            case ORDER_FREQ:  return field_freq(p2)-field_freq(p1);
        }
        return p2-p1; // keep order
    }

public:
    KeySorter(GBDATA *gb_key_data) {
        key_count   = 0;
        field_count = 0;
        key         = NULL;

        for (GBDATA *gb_key = GB_child(gb_key_data); gb_key; gb_key = GB_nextChild(gb_key)) {
            key_count++;
        }

        if (key_count) {
            key = (GBDATA**)malloc(key_count*sizeof(*key));
            
            int container_count = 0;
            for (GBDATA *gb_key = GB_child(gb_key_data); gb_key; gb_key = GB_nextChild(gb_key)) {
                GBDATA   *gb_type = GB_entry(gb_key, CHANGEKEY_TYPE);
                GB_TYPES  type    = GB_TYPES(GB_read_int(gb_type));

                if (type == GB_DB) { // move containers behind fields
                    key[key_count-1-container_count++] = gb_key;
                }
                else {
                    key[field_count++] = gb_key;
                }
            }
            ui_assert((container_count+field_count) == key_count);
            reverse_order(field_count, key_count-1); // of containers
        }
        order_changed = false;
    }
    ~KeySorter() {
        ui_assert(!order_changed); // order changed but not written
        free(key);
    }

    int get_field_count() const { return field_count; }

    void bubble_sort(int p1, int p2, ReorderMode mode, BoundItemSel *selector) {
        if (p1>p2) std::swap(p1, p2);
        if (legal_field_pos(p1, p2)) {
            if (mode == ORDER_FREQ) {
                sort_progress = new arb_progress("Calculating field frequencies", p2-p1+1);
            }
            bitem_selector = selector;
            while (p1<p2) {
                bool changed = false;

                int i = p2;
                while (i>p1) {
                    if (compare(i-1, i, mode)>0) {
                        swap(i-1, i);
                        changed = true;
                    }
                    --i;
                }
                if (!changed) break;
                ++p1;
            }
            if (order_hash) {
                GBS_free_hash(order_hash);
                order_hash = NULL;
            }
            if (sort_progress) {
                delete sort_progress;
                sort_progress = NULL;
            }
        }
    }
    void reverse_order(int p1, int p2) {
        if (p1>p2) std::swap(p1, p2);
        if (legal_field_pos(p1, p2)) while (p1<p2) swap(p1++, p2--);
    }

    int index_of(GBDATA *gb_key) {
        int i;
        for (i = 0; i<key_count; i++) {
            if (gb_key == key[i]) break;
        }
        if (i == key_count) {
            ui_assert(0);
            i = -1;
        }
        return i;
    }

    void move_to(int to_move, int wanted_position) {
        if (legal_field_pos(to_move, wanted_position)) {
            while (to_move<wanted_position) {
                swap(to_move, to_move+1);
                to_move++;
            }
            while (to_move>wanted_position) {
                swap(to_move, to_move-1);
                to_move--;
            }
        }
    }

    GB_ERROR save_changes() {
        GB_ERROR warning = NULL;
        if (order_changed) {
            if (key_count) {
                GBDATA *gb_main = GB_get_root(key[0]);
                warning         = GB_resort_data_base(gb_main, key, key_count);
            }
            order_changed = false;
        }
        return warning;
    }
};

GB_HASH      *KeySorter::order_hash     = NULL;
BoundItemSel *KeySorter::bitem_selector = NULL;
arb_progress *KeySorter::sort_progress  = NULL;

static void reorder_keys(AW_window *aws, ReorderMode mode, Itemfield_Selection *sel_left, Itemfield_Selection *sel_right) {
    ItemSelector& selector = sel_left->get_selector();
    ui_assert(&selector == &sel_right->get_selector());
    
    int left_index  = aws->get_index_of_selected_element(sel_left->get_sellist());
    int right_index = aws->get_index_of_selected_element(sel_right->get_sellist());

    GB_ERROR warning = 0;

    GBDATA  *gb_main = sel_left->get_gb_main();
    AW_root *awr     = aws->get_root();
    
    GB_begin_transaction(gb_main);

    GBDATA *gb_left_field  = GBT_get_changekey(gb_main, awr->awar(AWAR_FIELD_REORDER_SOURCE)->read_char_pntr(), selector.change_key_path);
    GBDATA *gb_right_field = GBT_get_changekey(gb_main, awr->awar(AWAR_FIELD_REORDER_DEST)->read_char_pntr(), selector.change_key_path);

    if (!gb_left_field || !gb_right_field || gb_left_field == gb_right_field) {
        warning = "Please select different fields in both list";
    }
    else {
        GBDATA    *gb_key_data = GB_search(gb_main, selector.change_key_path, GB_CREATE_CONTAINER);
        KeySorter  sorter(gb_key_data);

        int left_key_idx  = sorter.index_of(gb_left_field);
        int right_key_idx = sorter.index_of(gb_right_field);

        switch (mode) {
            case RIGHT_BEHIND_LEFT:
                sorter.move_to(right_key_idx, left_key_idx+(left_key_idx<right_key_idx));
                if (right_index>left_index) { left_index++; right_index++; } // make it simple to move several consecutive keys
                break;
            case REVERSE_ORDER:
                sorter.reverse_order(left_key_idx, right_key_idx);
                std::swap(left_index, right_index);
                break;
            default: {
                BoundItemSel bis(gb_main, selector);
                sorter.bubble_sort(left_key_idx, right_key_idx, mode, &bis);
                break;
            }
        }

        sorter.save_changes();
    }
    GB_commit_transaction(gb_main);

    if (warning) {
        aw_message(warning);
    }
    else {
        aws->select_element_at_index(sel_left->get_sellist(), left_index);
        aws->select_element_at_index(sel_right->get_sellist(), right_index);
    }
}

static void reorder_right_behind_left(AW_window *aws, AW_CL cl_selleft, AW_CL cl_selright) { reorder_keys(aws, RIGHT_BEHIND_LEFT, (Itemfield_Selection*)cl_selleft, (Itemfield_Selection*)cl_selright); }
static void reverse_key_order        (AW_window *aws, AW_CL cl_selleft, AW_CL cl_selright) { reorder_keys(aws, REVERSE_ORDER,     (Itemfield_Selection*)cl_selleft, (Itemfield_Selection*)cl_selright); }

static void sort_keys(AW_window *aws, AW_CL cl_selleft, AW_CL cl_selright) {
    ReorderMode mode = ReorderMode(aws->get_root()->awar(AWAR_FIELD_REORDER_ORDER)->read_int());
    reorder_keys(aws, mode, (Itemfield_Selection*)cl_selleft, (Itemfield_Selection*)cl_selright);
}

static void reorder_up_down(AW_window *aws, AW_CL cl_selright, AW_CL cl_dir) {
    int                  dir       = (int)cl_dir;
    Itemfield_Selection *sel_right = (Itemfield_Selection*)cl_selright;
    GBDATA              *gb_main   = sel_right->get_gb_main();

    GB_begin_transaction(gb_main);
    ItemSelector& selector   = sel_right->get_selector();
    int           list_index = aws->get_index_of_selected_element(sel_right->get_sellist());

    const char *field_name = aws->get_root()->awar(AWAR_FIELD_REORDER_DEST)->read_char_pntr();
    GBDATA     *gb_field   = GBT_get_changekey(gb_main, field_name, selector.change_key_path);
    GB_ERROR    warning    = 0;

    if (!gb_field) {
        warning = "Please select the item to move in the right box";
    }
    else {
        GBDATA    *gb_key_data = GB_search(gb_main, selector.change_key_path, GB_CREATE_CONTAINER);
        KeySorter  sorter(gb_key_data);

        int curr_index = sorter.index_of(gb_field);
        int dest_index = -1;
        if (abs(dir) == 1) {
            dest_index = curr_index+dir;
            list_index = -1;
        }
        else {
            dest_index = dir<0 ? 0 : sorter.get_field_count()-1;
        }

        sorter.move_to(curr_index, dest_index);
        warning = sorter.save_changes();

    }

    GB_commit_transaction(gb_main);
    if (list_index >= 0) aws->select_element_at_index(sel_right->get_sellist(), list_index);
    if (warning) aw_message(warning);
}

AW_window *DBUI::create_fields_reorder_window(AW_root *root, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (!awsa[selector.type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector.type]  = aws;

        aws->init(root, "REORDER_FIELDS", "REORDER FIELDS");
        aws->load_xfig("ad_kreo.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        const char *HELPFILE = "spaf_reorder.hlp";
        aws->callback(AW_POPUP_HELP, (AW_CL)HELPFILE);
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        Itemfield_Selection *sel1 = create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_REORDER_SOURCE, FIELD_FILTER_NDS, "source", 0, selector, 20, 10);
        Itemfield_Selection *sel2 = create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_REORDER_DEST,   FIELD_FILTER_NDS, "dest",   0, selector, 20, 10);

        aws->button_length(8);

        aws->at("sort");
        aws->callback(sort_keys, (AW_CL)sel1, (AW_CL)sel2);
        aws->help_text(HELPFILE);
        aws->create_button("SORT", "Sort by");

        aws->at("sorttype");
        aws->create_option_menu(AWAR_FIELD_REORDER_ORDER);
        aws->insert_option("name",      "a", ORDER_ALPHA);
        aws->insert_option("type",      "t", ORDER_TYPE);
        aws->insert_option("frequency", "f", ORDER_FREQ);
        aws->update_option_menu();

        aws->at("leftright");
        aws->callback(reorder_right_behind_left, (AW_CL)sel1, (AW_CL)sel2);
        aws->help_text(HELPFILE);
        aws->create_autosize_button("MOVE_RIGHT_BEHIND_LEFT", "Move right\nbehind left");

        aws->at("reverse");
        aws->callback(reverse_key_order, (AW_CL)sel1, (AW_CL)sel2);
        aws->help_text(HELPFILE);
        aws->create_autosize_button("REVERSE", "Reverse");
        
        aws->button_length(6);
        struct {
            const char *tag;
            const char *macro;
            int         dir;
        } reorder[4] = {
            { "Top",    "MOVE_TOP_RIGHT",  -2 },
            { "Up",     "MOVE_UP_RIGHT",   -1 },
            { "Down",   "MOVE_DOWN_RIGHT", +1 },
            { "Bottom", "MOVE_BOT_RIGHT",  +2 },
        };

        for (int i = 0; i<4; ++i) {
            aws->at(reorder[i].tag);
            aws->callback(reorder_up_down, (AW_CL)sel2, reorder[i].dir);
            aws->help_text(HELPFILE);
            aws->create_button(reorder[i].macro, reorder[i].tag);
        }
    }

    return awsa[selector.type];
}

static void hide_field_cb(AW_window *aws, AW_CL cl_sel, AW_CL cl_hide) {
    Itemfield_Selection *item_sel = (Itemfield_Selection*)cl_sel;

    GBDATA   *gb_main = item_sel->get_gb_main();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        char          *source    = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        ItemSelector&  selector  = item_sel->get_selector();
        GBDATA        *gb_source = GBT_get_changekey(gb_main, source, selector.change_key_path);

        if (!gb_source) error = "Please select the field you want to (un)hide";
        else error            = GBT_write_int(gb_source, CHANGEKEY_HIDDEN, int(cl_hide));

        free(source);
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);
    if (!error) aws->move_selection(item_sel->get_sellist(), 1);
}

static void field_delete_cb(AW_window *aws, AW_CL cl_sel) {
    Itemfield_Selection *item_sel = (Itemfield_Selection*)cl_sel;

    GBDATA   *gb_main = item_sel->get_gb_main();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        char              *source     = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        ItemSelector&      selector   = item_sel->get_selector();
        AW_selection_list *sellist    = item_sel->get_sellist();
        int                curr_index = aws->get_index_of_selected_element(sellist);
        GBDATA            *gb_source  = GBT_get_changekey(gb_main, source, selector.change_key_path);

        if (!gb_source) error = "Please select the field you want to delete";
        else error            = GB_delete(gb_source);

        for (GBDATA *gb_item_container = selector.get_first_item_container(gb_main, aws->get_root(), QUERY_ALL_ITEMS);
             !error && gb_item_container;
             gb_item_container = selector.get_next_item_container(gb_item_container, QUERY_ALL_ITEMS))
        {
            for (GBDATA * gb_item = selector.get_first_item(gb_item_container, QUERY_ALL_ITEMS);
                 !error && gb_item;
                 gb_item = selector.get_next_item(gb_item, QUERY_ALL_ITEMS))
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

    GB_end_transaction_show_error(gb_main, error, aw_message);
}


AW_window *DBUI::create_field_delete_window(AW_root *root, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (!awsa[selector.type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector.type]  = aws;

        aws->init(root, "DELETE_FIELD", "DELETE FIELD");
        aws->load_xfig("ad_delof.fig");
        aws->button_length(6);

        aws->at("close"); aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(AW_POPUP_HELP, (AW_CL)"spaf_delete.hlp");
        aws->create_button("HELP", "HELP", "H");

        Itemfield_Selection *item_sel =
            create_selection_list_on_itemfields(bound_selector->gb_main,
                                                    aws, AWAR_FIELD_DELETE,
                                                    -1,
                                                    "source", 0, selector, 20, 10,
                                                    SF_HIDDEN);

        aws->button_length(13);
        aws->at("hide");
        aws->callback(hide_field_cb, (AW_CL)item_sel, (AW_CL)1);
        aws->help_text("rm_field_only.hlp");
        aws->create_button("HIDE_FIELD", "Hide field", "H");

        aws->at("unhide");
        aws->callback(hide_field_cb, (AW_CL)item_sel, (AW_CL)0);
        aws->help_text("rm_field_only.hlp");
        aws->create_button("UNHIDE_FIELD", "Unhide field", "U");

        aws->at("delf");
        aws->callback(field_delete_cb, (AW_CL)item_sel);
        aws->help_text("rm_field_cmpt.hlp");
        aws->create_button("DELETE_FIELD", "DELETE FIELD\n(DATA DELETED)", "C");
    }
    
    return awsa[selector.type];
}

static void field_create_cb(AW_window *aws, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    GB_push_transaction(bound_selector->gb_main);
    char     *name   = aws->get_root()->awar(AWAR_FIELD_CREATE_NAME)->read_string();
    GB_ERROR  error  = GB_check_key(name);
    GB_ERROR  error2 = GB_check_hkey(name);
    if (error && !error2) {
        aw_message("Warning: Your key contain a '/' character,\n"
                   "    that means it is a hierarchical key");
        error = 0;
    }

    int type = (int)aws->get_root()->awar(AWAR_FIELD_CREATE_TYPE)->read_int();

    if (!error) error = GBT_add_new_changekey_to_keypath(bound_selector->gb_main, name, type, selector.change_key_path);
    aws->hide_or_notify(error);
    free(name);
    GB_pop_transaction(bound_selector->gb_main);
}

AW_window *DBUI::create_field_create_window(AW_root *root, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (awsa[selector.type]) return (AW_window *)awsa[selector.type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector.type]  = aws;

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
    aws->callback(field_create_cb, cl_bound_item_selector);
    aws->create_button("CREATE", "CREATE", "C");

    return (AW_window *)aws;
}

#if defined(WARN_TODO)
#warning GBT_convert_changekey currently only works for species fields, make it work with genes/exp/... as well (use selector)
#endif

static void field_convert_commit_cb(AW_window *aws, AW_CL cl_bound_item_selector) {
    BoundItemSel *bound_selector = (BoundItemSel*)cl_bound_item_selector;

    AW_root  *root    = aws->get_root();
    GBDATA   *gb_main = bound_selector->gb_main;
    GB_ERROR  error   = NULL;

    GB_push_transaction(gb_main);
    error = GBT_convert_changekey(gb_main,
                                  root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_char_pntr(),
                                  (GB_TYPES)root->awar(AWAR_FIELD_CONVERT_TYPE)->read_int());

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static void field_convert_update_typesel_cb(AW_window *aws, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    AW_root *root    = aws->get_root();
    GBDATA  *gb_main = bound_selector->gb_main;

    GB_push_transaction(gb_main);
    int type = GBT_get_type_of_changekey(gb_main,
                                         root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_char_pntr(),
                                         selector.change_key_path);
    GB_pop_transaction(gb_main);

    root->awar(AWAR_FIELD_CONVERT_TYPE)->write_int(type);
}

static AW_window *create_field_convert_window(AW_root *root, AW_CL cl_bound_item_selector) {
    BoundItemSel  *bound_selector = (BoundItemSel*)cl_bound_item_selector;
    ItemSelector&  selector       = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (awsa[selector.type]) return (AW_window *)awsa[selector.type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector.type]  = aws;

    aws->init(root, "CONVERT_FIELD", "CONVERT FIELDS");
    aws->load_xfig("ad_conv.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"spaf_convert.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->callback(field_convert_update_typesel_cb, cl_bound_item_selector);
    create_selection_list_on_itemfields(bound_selector->gb_main,
                                            aws,
                                            AWAR_FIELD_CONVERT_SOURCE, // AWAR containing selection
                                            -1,                        // type filter
                                            "source",                  // selector xfig position
                                            0,                         // rescan button xfig position
                                            selector,
                                            40, 20,                    // selector w,h
                                            SF_HIDDEN);

    aws->at("typesel");
    aws->create_toggle_field(AWAR_FIELD_CONVERT_TYPE, NULL, "F");
    aws->insert_toggle("Ascii Text",        "S", (int)GB_STRING);
    aws->insert_toggle("Link",              "L", (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical", "N", (int)GB_INT);
    aws->insert_toggle("Numerical",         "R", (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text",    "0", (int)GB_BITS);
    aws->update_toggle_field();

    aws->at("convert");
    aws->callback(field_convert_commit_cb, cl_bound_item_selector);
    aws->create_button("CONVERT", "CONVERT", "T");

    return (AW_window*)aws;
}

void DBUI::insert_field_admin_menuitems(AW_window *aws, GBDATA *gb_main) {
    static BoundItemSel *bis = new BoundItemSel(gb_main, SPECIES_get_selector());
    ui_assert(bis->gb_main == gb_main);
    
    aws->insert_menu_topic("spec_reorder_fields", "Reorder fields ...",     "R", "spaf_reorder.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_fields_reorder_window,  (AW_CL)bis);
    aws->insert_menu_topic("spec_delete_field",   "Delete/Hide fields ...", "D", "spaf_delete.hlp",  AWM_EXP, AW_POPUP, (AW_CL)create_field_delete_window,  (AW_CL)bis);
    aws->insert_menu_topic("spec_create_field",   "Create fields ...",      "C", "spaf_create.hlp",  AWM_ALL, AW_POPUP, (AW_CL)create_field_create_window,  (AW_CL)bis);
    aws->insert_menu_topic("spec_convert_field",  "Convert fields ...",     "t", "spaf_convert.hlp", AWM_EXP, AW_POPUP, (AW_CL)create_field_convert_window, (AW_CL)bis);
    aws->sep______________();
    aws->insert_menu_topic("spec_unhide_fields", "Show all hidden fields", "S", "scandb.hlp", AWM_ALL, (AW_CB)species_field_selection_list_unhide_all_cb, (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->sep______________();
    aws->insert_menu_topic("spec_scan_unknown_fields", "Scan unknown fields",   "u", "scandb.hlp", AWM_ALL, (AW_CB)species_field_selection_list_scan_unknown_cb,  (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->insert_menu_topic("spec_del_unused_fields",   "Forget unused fields",  "e", "scandb.hlp", AWM_ALL, (AW_CB)species_field_selection_list_delete_unused_cb, (AW_CL)gb_main, FIELD_FILTER_NDS);
    aws->insert_menu_topic("spec_refresh_fields",      "Refresh fields (both)", "f", "scandb.hlp", AWM_ALL, (AW_CB)species_field_selection_list_update_cb,        (AW_CL)gb_main, FIELD_FILTER_NDS);
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

inline char *read_sequence_region(GBDATA *gb_data, const TargetRange& range) {
    return range.dup_corresponding_part(GB_read_char_pntr(gb_data), GB_read_count(gb_data));
}

static void awtc_nn_search_all_listed(AW_window *aww, AW_CL cl_query) {
    DbQuery *query   = (DbQuery *)cl_query;
    GBDATA  *gb_main = query_get_gb_main(query);

    ui_assert(get_queried_itemtype(query).type == QUERY_ITEM_SPECIES);

    GB_begin_transaction(gb_main);

    AW_root *aw_root    = aww->get_root();
    char    *dest_field = aw_root->awar(AWAR_NN_DEST_FIELD)->read_string();

    GB_ERROR error     = 0;
    GB_TYPES dest_type = GBT_get_type_of_changekey(gb_main, dest_field, CHANGE_KEY_PATH);
    if (!dest_type) {
        error = GB_export_error("Please select a valid field");
    }

    if (strcmp(dest_field, "name")==0) {
        int answer = aw_question(NULL, "CAUTION! This will destroy all name-fields of the listed species.\n",
                                 "Continue and destroy all name-fields,Abort");

        if (answer==1) {
            error = GB_export_error("Aborted by user");
        }
    }

    long         max = count_queried_items(query, QUERY_ALL_ITEMS);
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

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         !error && gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        if (!IS_QUERIED(gb_species, query)) continue;

        GBDATA *gb_data = GBT_read_sequence(gb_species, ali_name);
        if (gb_data) {
            TargetRange      range    = org_range; // modified by read_sequence_region
            char            *sequence = read_sequence_region(gb_data, range);
            PT_FamilyFinder  ff(gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches);

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
    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(dest_field);
    free(ali_name);
}

static void awtc_nn_search(AW_window *aww, AW_CL id, AW_CL cl_gb_main) {
    AW_root     *aw_root  = aww->get_root();
    GBDATA      *gb_main  = (GBDATA*)cl_gb_main;
    GB_ERROR     error    = 0;
    TargetRange  range    = get_nn_range_from_awars(aw_root);
    char        *sequence = 0;
    {
        GB_transaction  ta(gb_main);

        char   *sel_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_species  = GBT_find_species(gb_main, sel_species);

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


    PT_FamilyFinder ff(gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches);

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

static void awtc_move_hits(AW_window *aww, AW_CL id, AW_CL cl_query) {
    AW_root *aw_root         = aww->get_root();
    char    *current_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();

    if (!current_species) current_species = strdup("<unknown>");

    char *hit_description = GBS_global_string_copy("<neighbour of %s: %%s>", current_species);

    copy_selection_list_2_query_box((DbQuery *)cl_query, (AW_selection_list *)id, hit_description);

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

static AW_window *create_next_neighbours_listed_window(AW_root *aw_root, AW_CL cl_query) {
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
        create_selection_list_on_itemfields(query_get_gb_main((DbQuery*)cl_query),
                                                aws, AWAR_NN_DEST_FIELD,
                                                (1<<GB_INT) | (1<<GB_STRING), "field", 0,
                                                SPECIES_get_selector(), 20, 10);

        aws->at("go");
        aws->callback(awtc_nn_search_all_listed, cl_query);
        aws->create_button("WRITE_FIELDS", "Write to field");
    }
    return aws;
}

static AW_window *create_next_neighbours_selected_window(AW_root *aw_root, AW_CL cl_query) {
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
        aws->callback(awtc_nn_search, (AW_CL)id, (AW_CL)query_get_gb_main((DbQuery*)cl_query));
        aws->create_button("SEARCH", "SEARCH");

        aws->at("move");
        aws->callback(awtc_move_hits, (AW_CL)id, cl_query);
        aws->create_button("MOVE_TO_HITLIST", "MOVE TO HITLIST");

    }
    return aws;
}


void DBUI::detach_info_window(AW_window *aww, AW_CL cl_pointer_to_aww, AW_CL cl_AW_detach_information) {
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
        aws->callback(AW_POPUP, (AW_CL)DBUI::create_species_query_window, (AW_CL)gb_main);
        aws->create_button("SEARCH", "SEARCH", "S");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"sp_info.hlp");
        aws->create_button("HELP", "HELP", "H");

        DbScanner *scannerid = create_db_scanner(gb_main, aws, "box", 0, "field", "enable", DB_VIEWER, 0, "mark", FIELD_FILTER_NDS,
                                                       organismWindow ? ORGANISM_get_selector() : ORGANISM_get_selector());

        if (organismWindow) aws->create_menu("ORGANISM",    "O", AWM_ALL);
        else                aws->create_menu("SPECIES",     "S", AWM_ALL);

        aws->insert_menu_topic("species_delete",        "Delete",         "D", "spa_delete.hlp",  AWM_ALL, species_delete_cb,     (AW_CL)gb_main,                      0);
        aws->insert_menu_topic("species_rename",        "Rename",         "R", "spa_rename.hlp",  AWM_ALL, species_rename_cb,     (AW_CL)gb_main,                      0);
        aws->insert_menu_topic("species_copy",          "Copy",           "y", "spa_copy.hlp",    AWM_ALL, species_copy_cb,       (AW_CL)gb_main,                      0);
        aws->insert_menu_topic("species_create",        "Create",         "C", "spa_create.hlp",  AWM_ALL, AW_POPUP,                 (AW_CL)create_species_create_window, (AW_CL)gb_main);
        aws->insert_menu_topic("species_convert_2_sai", "Convert to SAI", "S", "sp_sp_2_ext.hlp", AWM_ALL, move_species_to_extended, (AW_CL)gb_main,                      0);
        aws->sep______________();

        aws->create_menu("FIELDS", "F", AWM_ALL);
        insert_field_admin_menuitems(aws, gb_main);

        {
            const char         *awar_name = (bool)organismWindow ? AWAR_ORGANISM_NAME : AWAR_SPECIES_NAME;
            AW_root            *awr       = aws->get_root();
            Awar_Callback_Info *cb_info   = new Awar_Callback_Info(awr, awar_name, map_species, (AW_CL)scannerid, (AW_CL)organismWindow); // do not delete!
            cb_info->add_callback();

            AW_detach_information *detach_info = new AW_detach_information(cb_info); // do not delete!

            aws->at("detach");
            aws->callback(detach_info_window, (AW_CL)&aws, (AW_CL)detach_info);
            aws->create_button("DETACH", "DETACH", "D");

            detach_info->set_detach_button(aws->get_last_widget());
        }

        aws->show();
        map_species(aws->get_root(), (AW_CL)scannerid, (AW_CL)organismWindow);
    }
    return AWS[windowIdx]; // already created (and not detached)
}

AW_window *DBUI::create_species_info_window(AW_root *aw_root, AW_CL cl_gb_main) {
    return create_speciesOrganismWindow(aw_root, (GBDATA*)cl_gb_main, false);
}
AW_window *DBUI::create_organism_info_window(AW_root *aw_root, AW_CL cl_gb_main) {
    return create_speciesOrganismWindow(aw_root, (GBDATA*)cl_gb_main, true);
}

static DbQuery *GLOBAL_species_query = NULL; // @@@ fix design

void DBUI::unquery_all() {
    QUERY::unquery_all(0, GLOBAL_species_query);
}

void DBUI::query_update_list() {
    DbQuery_update_list(GLOBAL_species_query);
}

AW_window *DBUI::create_species_query_window(AW_root *aw_root, AW_CL cl_gb_main) {
    static AW_window_simple_menu *aws = 0;
    if (!aws) {
        aws = new AW_window_simple_menu;
        aws->init(aw_root, "SPECIES_QUERY", "SEARCH and QUERY");
        aws->create_menu("More functions", "f");
        aws->load_xfig("ad_query.fig");


        query_spec awtqs(SPECIES_get_selector());

        awtqs.gb_main             = (GBDATA*)cl_gb_main;
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
        awtqs.create_view_window  = create_species_info_window;

        DbQuery *query           = create_query_box(aws, &awtqs, "spec");
        GLOBAL_species_query = query;

        aws->create_menu("More search",     "s");
        aws->insert_menu_topic("spec_search_equal_fields_within_db", "Search For Equal Fields and Mark Duplicates",                "E", "search_duplicates.hlp", AWM_ALL, (AW_CB)search_duplicated_field_content, (AW_CL)query,                                  0);
        aws->insert_menu_topic("spec_search_equal_words_within_db",  "Search For Equal Words Between Fields and Mark Duplicates",  "W", "search_duplicates.hlp", AWM_ALL, (AW_CB)search_duplicated_field_content, (AW_CL)query,                                  1);
        aws->insert_menu_topic("spec_search_next_relativ_of_sel",    "Search Next Relatives of SELECTED Species in PT_Server ...", "R", 0,                       AWM_ALL, (AW_CB)AW_POPUP,                 (AW_CL)create_next_neighbours_selected_window, (AW_CL)query);
        aws->insert_menu_topic("spec_search_next_relativ_of_listed", "Search Next Relatives of LISTED Species in PT_Server ...",   "L", 0,                       AWM_ALL, (AW_CB)AW_POPUP,                 (AW_CL)create_next_neighbours_listed_window,   (AW_CL)query);

        aws->button_length(7);

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP, (AW_CL)"sp_search.hlp");
        aws->create_button("HELP", "HELP", "H");
    }
    return aws;
}

