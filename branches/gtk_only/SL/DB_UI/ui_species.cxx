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
#include <probe_gui.hxx>
#include <arb_defs.h>
#include <awtc_next_neighbours.hxx>
#include <db_scanner.hxx>
#include <db_query.h>
#include <AW_rename.hxx>
#include <aw_awar_defs.hxx>
#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_question.hxx>
#include <algorithm>
#include <arb_progress.h>
#include <item_sel_list.h>
#include <map>
#include <info_window.h>
#include <awt_config_manager.hxx>

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
// more defined in ../../AWTC/awtc_next_neighbours.hxx@AWAR_NN_BASE
#define AWAR_NN_COMPLEMENT  AWAR_NN_BASE "complement"
#define AWAR_NN_RANGE_START AWAR_NN_BASE "range_start"
#define AWAR_NN_RANGE_END   AWAR_NN_BASE "range_end"
#define AWAR_NN_MIN_SCORE   AWAR_NN_BASE "min_scored"
#define AWAR_NN_MAX_HITS    AWAR_NN_BASE "max_hits"

// next neighbours of selected only:
#define AWAR_NN_BASE_SELECTED        AWAR_NN_BASE "selected/"
#define AWAR_NN_SELECTED_HIT_COUNT   "tmp/" AWAR_NN_BASE_SELECTED "hit_count"
#define AWAR_NN_SELECTED_AUTO_SEARCH "tmp/" AWAR_NN_BASE_SELECTED "auto_search"
#define AWAR_NN_SELECTED_AUTO_MARK   "tmp/" AWAR_NN_BASE_SELECTED "auto_mark"

// next neighbours of listed only:
#define AWAR_NN_BASE_LISTED           AWAR_NN_BASE "listed/"
#define AWAR_NN_LISTED_DEST_FIELD     AWAR_NN_BASE_LISTED "dest_field"
#define AWAR_NN_LISTED_SCORED_ENTRIES AWAR_NN_BASE_LISTED "scored_entries"

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

static void move_species_to_extended(AW_window *aww, GBDATA *gb_main) {
    char     *source = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GB_ERROR  error  = GB_begin_transaction(gb_main);

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


static void species_create_cb(AW_window *aww, GBDATA *gb_main) {
    char *dest = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();
    if (dest[0]) {
        GB_ERROR error = GB_begin_transaction(gb_main);
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
    }
    else {
        aw_message("Please enter a name for the new species");
    }
    free(dest);
}

static AW_window *create_species_create_window(AW_root *root, GBDATA *gb_main) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "CREATE_SPECIES", "SPECIES CREATE");
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(species_create_cb, gb_main));
    aws->create_button("GO", "Go", "G");

    return aws;
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

static void species_copy_cb(AW_window *aww, GBDATA *gb_main) {
    AW_root *aw_root    = aww->get_root();
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

static void species_rename_cb(AW_window *aww, GBDATA *gb_main) {
    AW_root *aw_root    = aww->get_root();
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
                bool recreateID = ARB_in_expert_mode(aw_root) && // never re-create ID in novice mode
                    aw_ask_sure("recreate_name_field",
                                "Regenerate species identifier ('name')?\n"
                                "(only do this if you know what you're doing)");
                if (recreateID) {
                    arb_progress progress("Regenerating species ID", 1);
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

static void species_delete_cb(AW_window *aww, GBDATA *gb_main) {
    AW_root  *aw_root    = aww->get_root();
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

    __ATTR__USERESULT GB_ERROR save_changes() {
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
    
    int left_index  = sel_left->get_sellist()->get_index_of_selected();
    int right_index = sel_right->get_sellist()->get_index_of_selected();

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

        warning = sorter.save_changes();
    }
    GB_commit_transaction(gb_main);

    if (warning) {
        aw_message(warning);
    }
    else {
        sel_left->get_sellist()->select_element_at(left_index);
        sel_right->get_sellist()->select_element_at(right_index);
    }
}

static void reorder_right_behind_left(AW_window *aws, Itemfield_Selection *selleft, Itemfield_Selection *selright) { reorder_keys(aws, RIGHT_BEHIND_LEFT, selleft, selright); }
static void reverse_key_order        (AW_window *aws, Itemfield_Selection *selleft, Itemfield_Selection *selright) { reorder_keys(aws, REVERSE_ORDER,     selleft, selright); }

static void sort_keys(AW_window *aws, Itemfield_Selection *selleft, Itemfield_Selection *selright) {
    ReorderMode mode = ReorderMode(aws->get_root()->awar(AWAR_FIELD_REORDER_ORDER)->read_int());
    reorder_keys(aws, mode, selleft, selright);
}

static void reorder_up_down(AW_window *aws, Itemfield_Selection *sel_right, int dir) {
    GBDATA *gb_main = sel_right->get_gb_main();

    GB_begin_transaction(gb_main);
    ItemSelector& selector   = sel_right->get_selector();
    int           list_index = sel_right->get_sellist()->get_index_of_selected();

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
    if (list_index >= 0) sel_right->get_sellist()->select_element_at(list_index);
    if (warning) aw_message(warning);
}

AW_window *DBUI::create_fields_reorder_window(AW_root *root, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (!awsa[selector.type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector.type]  = aws;

        init_itemType_specific_window(root, aws, selector, "REORDER_FIELDS", "Reorder %s fields");
        aws->load_xfig("ad_kreo.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help");
        const char *HELPFILE = "spaf_reorder.hlp";
        aws->callback(makeHelpCallback(HELPFILE));
        aws->create_button("HELP", "Help", "H");

        Itemfield_Selection *sel1 = create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_REORDER_SOURCE, true, FIELD_FILTER_NDS, "source", 0, selector, 20, 10, SF_STANDARD, NULL);
        Itemfield_Selection *sel2 = create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_REORDER_DEST,   true, FIELD_FILTER_NDS, "dest",   0, selector, 20, 10, SF_STANDARD, NULL);

        aws->button_length(8);

        aws->at("sort");
        aws->callback(makeWindowCallback(sort_keys, sel1, sel2));
        aws->help_text(HELPFILE);
        aws->create_button("SORT", "Sort by");

        aws->at("sorttype");
        aws->create_option_menu(AWAR_FIELD_REORDER_ORDER, true);
        aws->insert_option("name",      "a", ORDER_ALPHA);
        aws->insert_option("type",      "t", ORDER_TYPE);
        aws->insert_option("frequency", "f", ORDER_FREQ);
        aws->update_option_menu();

        aws->at("leftright");
        aws->callback(makeWindowCallback(reorder_right_behind_left, sel1, sel2));
        aws->help_text(HELPFILE);
        aws->create_autosize_button("MOVE_RIGHT_BEHIND_LEFT", "Move right\nbehind left");

        aws->at("reverse");
        aws->callback(makeWindowCallback(reverse_key_order, sel1, sel2));
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
            aws->callback(makeWindowCallback(reorder_up_down, sel2, reorder[i].dir));
            aws->help_text(HELPFILE);
            aws->create_button(reorder[i].macro, reorder[i].tag);
        }
    }

    return awsa[selector.type];
}

static void hide_field_cb(AW_window *aws, Itemfield_Selection *item_sel, int hide) {
    GBDATA   *gb_main = item_sel->get_gb_main();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        char          *source    = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        ItemSelector&  selector  = item_sel->get_selector();
        GBDATA        *gb_source = GBT_get_changekey(gb_main, source, selector.change_key_path);

        if (!gb_source) error = "Please select the field you want to (un)hide";
        else error            = GBT_write_int(gb_source, CHANGEKEY_HIDDEN, hide);

        free(source);
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);
    if (!error) item_sel->get_sellist()->move_selection(1);
}

static void field_delete_cb(AW_window *aws, Itemfield_Selection *item_sel) {
    GBDATA   *gb_main = item_sel->get_gb_main();
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        char              *source     = aws->get_root()->awar(AWAR_FIELD_DELETE)->read_string();
        ItemSelector&      selector   = item_sel->get_selector();
        AW_selection_list *sellist    = item_sel->get_sellist();
        int                curr_index = sellist->get_index_of_selected();
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
                        sellist->select_element_at(curr_index);
                    }
                }
            }
        }

        free(source);
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
}


AW_window *DBUI::create_field_delete_window(AW_root *root, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (!awsa[selector.type]) {
        AW_window_simple *aws = new AW_window_simple;
        awsa[selector.type]  = aws;

        init_itemType_specific_window(root, aws, selector, "DELETE_FIELD", "Delete %s field");
        aws->load_xfig("ad_delof.fig");
        aws->button_length(6);

        aws->at("close"); aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help"); aws->callback(makeHelpCallback("spaf_delete.hlp"));
        aws->create_button("HELP", "Help", "H");

        Itemfield_Selection *item_sel = create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_DELETE, true, -1, "source", 0, selector, 20, 10, SF_HIDDEN, NULL);

        aws->button_length(13);
        aws->at("hide");
        aws->callback(makeWindowCallback(hide_field_cb, item_sel, 1));
        aws->create_button("HIDE_FIELD", "Hide field", "H");

        aws->at("unhide");
        aws->callback(makeWindowCallback(hide_field_cb, item_sel, 0));
        aws->create_button("UNHIDE_FIELD", "Unhide field", "U");

        aws->at("delf");
        aws->callback(makeWindowCallback(field_delete_cb, item_sel));
        aws->create_button("DELETE_FIELD", "Delete field\n(data deleted)", "C");
    }
    
    return awsa[selector.type];
}

static void field_create_cb(AW_window *aws, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

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

AW_window *DBUI::create_field_create_window(AW_root *root, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (awsa[selector.type]) return awsa[selector.type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector.type]  = aws;

    init_itemType_specific_window(root, aws, selector, "CREATE_FIELD", "Create new %s field");
    aws->load_xfig("ad_fcrea.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

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
    aws->callback(makeWindowCallback(field_create_cb, bound_selector));
    aws->create_button("CREATE", "Create", "C");

    return aws;
}

#if defined(WARN_TODO)
#warning GBT_convert_changekey currently only works for species fields, make it work with genes/exp/... as well (use selector)
#endif

static void field_convert_commit_cb(AW_window *aws, BoundItemSel *bound_selector) {
    AW_root *root    = aws->get_root();
    GBDATA  *gb_main = bound_selector->gb_main;

    GB_push_transaction(gb_main);
    GB_ERROR error = GBT_convert_changekey(gb_main,
                                           root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_char_pntr(),
                                           (GB_TYPES)root->awar(AWAR_FIELD_CONVERT_TYPE)->read_int());

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static void field_convert_update_typesel_cb(AW_window *aws, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

    AW_root *root    = aws->get_root();
    GBDATA  *gb_main = bound_selector->gb_main;

    GB_push_transaction(gb_main);
    int type = GBT_get_type_of_changekey(gb_main,
                                         root->awar(AWAR_FIELD_CONVERT_SOURCE)->read_char_pntr(),
                                         selector.change_key_path);
    GB_pop_transaction(gb_main);

    root->awar(AWAR_FIELD_CONVERT_TYPE)->write_int(type);
}

static AW_window *create_field_convert_window(AW_root *root, BoundItemSel *bound_selector) {
    ItemSelector& selector = bound_selector->selector;

    static AW_window_simple *awsa[QUERY_ITEM_TYPES];
    if (awsa[selector.type]) return awsa[selector.type];

    AW_window_simple *aws = new AW_window_simple;
    awsa[selector.type]  = aws;

    init_itemType_specific_window(root, aws, selector, "CONVERT_FIELD", "Convert %s field");
    aws->load_xfig("ad_conv.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("spaf_convert.hlp"));
    aws->create_button("HELP", "Help", "H");

    aws->callback(makeWindowCallback(field_convert_update_typesel_cb, bound_selector));
    create_selection_list_on_itemfields(bound_selector->gb_main, aws, AWAR_FIELD_CONVERT_SOURCE, true, -1, "source", 0, selector, 40, 20, SF_HIDDEN, NULL);

    aws->at("typesel");
    aws->create_toggle_field(AWAR_FIELD_CONVERT_TYPE, NULL, "F");
    aws->insert_toggle("Ascii Text",        "S", (int)GB_STRING);
    aws->insert_toggle("Link",              "L", (int)GB_LINK);
    aws->insert_toggle("Rounded Numerical", "N", (int)GB_INT);
    aws->insert_toggle("Numerical",         "R", (int)GB_FLOAT);
    aws->insert_toggle("MASK = 01 Text",    "0", (int)GB_BITS);
    aws->update_toggle_field();

    aws->at("convert");
    aws->callback(makeWindowCallback(field_convert_commit_cb, bound_selector));
    aws->create_button("CONVERT", "Convert", "T");

    return aws;
}

void DBUI::insert_field_admin_menuitems(AW_window *aws, GBDATA *gb_main) {
    static BoundItemSel *bis = new BoundItemSel(gb_main, SPECIES_get_selector());
    ui_assert(bis->gb_main == gb_main);
    
    aws->insert_menu_topic(aws->local_id("spec_reorder_fields"), "Reorder fields ...",     "R", "spaf_reorder.hlp", AWM_ALL, makeCreateWindowCallback(create_fields_reorder_window, bis));
    aws->insert_menu_topic(aws->local_id("spec_delete_field"),   "Delete/Hide fields ...", "D", "spaf_delete.hlp",  AWM_EXP, makeCreateWindowCallback(create_field_delete_window,   bis));
    aws->insert_menu_topic(aws->local_id("spec_create_field"),   "Create fields ...",      "C", "spaf_create.hlp",  AWM_ALL, makeCreateWindowCallback(create_field_create_window,   bis));
    aws->insert_menu_topic(aws->local_id("spec_convert_field"),  "Convert fields ...",     "t", "spaf_convert.hlp", AWM_EXP, makeCreateWindowCallback(create_field_convert_window,  bis));
    aws->sep______________();
    aws->insert_menu_topic("spec_unhide_fields",  "Show all hidden fields", "S", "scandb.hlp", AWM_ALL, makeWindowCallback(species_field_selection_list_unhide_all_cb, gb_main, FIELD_FILTER_NDS));
    aws->insert_menu_topic("spec_refresh_fields", "Refresh fields",         "f", "scandb.hlp", AWM_ALL, makeWindowCallback(species_field_selection_list_update_cb,     gb_main, FIELD_FILTER_NDS));
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

class NN_GlobalData {
    DbQuery           *query;
    AW_selection_list *resultList;     // result list from create_next_neighbours_selected_window()

public:
    NN_GlobalData() : query(0), resultList(0) {}

    void set_query(DbQuery *new_query) {
        if (new_query != query) {
            ui_assert(!query); // need redesign b4 changing query works
            query = new_query;
        }
    }
    void set_result_list(AW_selection_list *new_resultList) {
        if (new_resultList != resultList) {
            ui_assert(!resultList); // need redesign b4 changing query works
            resultList = new_resultList;
        }
    }

    DbQuery *get_query() const { ui_assert(query); return query; }

    AW_selection_list *get_result_list() const { ui_assert(resultList); return resultList; }
    GBDATA *get_gb_main() const { return query_get_gb_main(get_query()); }
};
static NN_GlobalData NN_GLOBAL;

static PosRange get_nn_range_from_awars(AW_root *aw_root) {
    int start = get_and_fix_range_from_awar(aw_root->awar(AWAR_NN_RANGE_START));
    int end   = get_and_fix_range_from_awar(aw_root->awar(AWAR_NN_RANGE_END));

    return PosRange(start, end);
}

inline char *read_sequence_region(GBDATA *gb_data, const PosRange& range) {
    return range.dup_corresponding_part(GB_read_char_pntr(gb_data), GB_read_count(gb_data));
}

static void awtc_nn_search_all_listed(AW_window *aww) {
    DbQuery *query   = NN_GLOBAL.get_query();
    GBDATA  *gb_main = query_get_gb_main(query);

    ui_assert(get_queried_itemtype(query).type == QUERY_ITEM_SPECIES);

    GB_begin_transaction(gb_main);

    AW_root *aw_root    = aww->get_root();
    char    *dest_field = aw_root->awar(AWAR_NN_LISTED_DEST_FIELD)->read_string();

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

    long max = count_queried_items(query, QUERY_ALL_ITEMS);
    if (!max) {
        error = "No species listed in query";
    }
    else {
        arb_progress progress("Searching next neighbours", max);
        progress.auto_subtitles("Species");

        int     pts            = aw_root->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
        char   *ali_name       = aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
        int     oligo_len      = aw_root->awar(AWAR_NN_OLIGO_LEN)->read_int();
        int     mismatches     = aw_root->awar(AWAR_NN_MISMATCHES)->read_int();
        bool    fast_mode      = aw_root->awar(AWAR_NN_FAST_MODE)->read_int();
        bool    rel_matches    = aw_root->awar(AWAR_NN_REL_MATCHES)->read_int();
        int     wanted_entries = aw_root->awar(AWAR_NN_MAX_HITS)->read_int();
        bool    scored_entries = aw_root->awar(AWAR_NN_LISTED_SCORED_ENTRIES)->read_int();
        double  min_score      = aw_root->awar(AWAR_NN_MIN_SCORE)->read_float();

        FF_complement        compl_mode  = static_cast<FF_complement>(aw_root->awar(AWAR_NN_COMPLEMENT)->read_int());
        RelativeScoreScaling rel_scaling = static_cast<RelativeScoreScaling>(aw_root->awar(AWAR_NN_REL_SCALING)->read_int());

        PosRange org_range = get_nn_range_from_awars(aw_root);

        for (GBDATA *gb_species = GBT_first_species(gb_main);
             !error && gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            if (!IS_QUERIED(gb_species, query)) continue;
            GBDATA *gb_data = GBT_find_sequence(gb_species, ali_name);
            if (gb_data) {
                PosRange         range    = org_range; // modified by read_sequence_region
                char            *sequence = read_sequence_region(gb_data, range);
                PT_FamilyFinder  ff(gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches, rel_scaling);

                ff.restrict_2_region(range);

                error = ff.searchFamily(sequence, compl_mode, wanted_entries, min_score); 
                if (!error) {
                    const FamilyList *fm = ff.getFamilyList();

                    GBS_strstruct *value = NULL;
                    while (fm) {
                        const char *thisValue = 0;
                        if (rel_matches) {
                            ui_assert((fm->rel_matches*100) >= min_score); // filtered by ptserver
                            thisValue = scored_entries
                                ? GBS_global_string("%.1f%%:%s", fm->rel_matches*100, fm->name)
                                : fm->name;
                        }
                        else {
                            ui_assert(fm->matches >= min_score); // filtered by ptserver
                            thisValue = scored_entries
                                ? GBS_global_string("%li:%s", fm->matches, fm->name)
                                : fm->name;
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

                        error = GB_write_autoconv_string(gb_dest, GBS_mempntr(value));
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
        free(ali_name);
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);
    free(dest_field);
}

static void awtc_mark_hits(AW_window *) {
    AW_selection_list *resultList = NN_GLOBAL.get_result_list();
    GB_HASH           *list_hash  = resultList->to_hash(false);
    GBDATA            *gb_main    = NN_GLOBAL.get_gb_main();

    GB_transaction ta(gb_main);
    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        int hit = GBS_read_hash(list_hash, GBT_get_name(gb_species));
        GB_write_flag(gb_species, hit);
    }
}

static void awtc_nn_search(AW_window *) {
    AW_root  *aw_root  = AW_root::SINGLETON;
    GBDATA   *gb_main  = NN_GLOBAL.get_gb_main();
    GB_ERROR  error    = 0;
    PosRange  range    = get_nn_range_from_awars(aw_root);
    char     *sequence = 0;
    {
        GB_transaction  ta(gb_main);

        char   *sel_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
        GBDATA *gb_species  = GBT_find_species(gb_main, sel_species);

        if (!gb_species) {
            error = "Select a species first";
        }
        else {
            char   *ali_name = aw_root->awar(AWAR_DEFAULT_ALIGNMENT)->read_string();
            GBDATA *gb_data  = GBT_find_sequence(gb_species, ali_name);

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

    int    pts         = aw_root->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    int    oligo_len   = aw_root->awar(AWAR_NN_OLIGO_LEN)->read_int();
    int    mismatches  = aw_root->awar(AWAR_NN_MISMATCHES)->read_int();
    bool   fast_mode   = aw_root->awar(AWAR_NN_FAST_MODE)->read_int();
    bool   rel_matches = aw_root->awar(AWAR_NN_REL_MATCHES)->read_int();
    double min_score   = aw_root->awar(AWAR_NN_MIN_SCORE)->read_float();

    RelativeScoreScaling rel_scaling = static_cast<RelativeScoreScaling>(aw_root->awar(AWAR_NN_REL_SCALING)->read_int());

    PT_FamilyFinder ff(gb_main, pts, oligo_len, mismatches, fast_mode, rel_matches, rel_scaling);

    ff.restrict_2_region(range);

    int max_hits = 0; // max wanted hits

    if (!error) {
        FF_complement compl_mode = static_cast<FF_complement>(aw_root->awar(AWAR_NN_COMPLEMENT)->read_int());
        max_hits                 = aw_root->awar(AWAR_NN_MAX_HITS)->read_int();

        error = ff.searchFamily(sequence, compl_mode, max_hits, min_score);
    }

    // update result list
    {
        AW_selection_list* sel = NN_GLOBAL.get_result_list();
        sel->clear();

        int hits = 0;
        if (error) {
            aw_message(error);
            sel->insert_default("<Error>", "");
        }
        else {
            int count     = 1;
            int shownHits = max_hits>0 ? max_hits : ff.getRealHits();
            int numWidth  = log(shownHits)/log(10)+1;

            for (const FamilyList *fm = ff.getFamilyList(); fm; fm = fm->next) {
                const char *dis;
                if (rel_matches) {
                    dis = GBS_global_string("#%0*i %-12s Rel.hits: %5.1f%%", numWidth, count, fm->name, fm->rel_matches*100);
                }
                else {
                    dis = GBS_global_string("#%0*i %-12s Hits: %4li", numWidth, count, fm->name, fm->matches);
                }

                sel->insert(dis, fm->name);
                count++;
            }

            sel->insert_default(ff.hits_were_truncated() ? "<List truncated>" : "<No more hits>", "");
            hits = ff.getRealHits();
        }
        aw_root->awar(AWAR_NN_SELECTED_HIT_COUNT)->write_int(hits);
        if (aw_root->awar(AWAR_NN_SELECTED_AUTO_MARK)->read_int()) {
            awtc_mark_hits(NULL);
            aw_root->awar(AWAR_TREE_REFRESH)->touch();
        }
        sel->update();
    }

    free(sequence);
}

static void awtc_move_hits(AW_window *aww) {
    AW_root *aw_root         = aww->get_root();
    char    *current_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();

    if (!current_species) current_species = strdup("<unknown>");

    char *hit_description = GBS_global_string_copy("<neighbour of %s: %%s>", current_species);

    copy_selection_list_2_query_box(NN_GLOBAL.get_query(), NN_GLOBAL.get_result_list(), hit_description);

    free(hit_description);
    free(current_species);
}

static bool autosearch_triggered = false;
static unsigned nn_perform_delayed_autosearch_cb(AW_root *awr) {
    awtc_nn_search(NULL);
    autosearch_triggered = false;
    return 0;
}
static void nn_trigger_delayed_autosearch_cb(AW_root *awr) {
    // automatic search is triggered delayed to make sure
    // dependencies between involved awars have propagated.
    if (!autosearch_triggered) { // ignore multiple triggers (happens when multiple awars change, e.g. when loading config)
        autosearch_triggered = true;
        awr->add_timed_callback(200, makeTimedCallback(nn_perform_delayed_autosearch_cb));
    }
}

static void nn_auto_search_changed_cb(AW_root *awr) {
    int auto_search = awr->awar(AWAR_NN_SELECTED_AUTO_SEARCH)->read_int();

    AW_awar *awar_sel_species = awr->awar(AWAR_SPECIES_NAME);
    if (auto_search) {
        awar_sel_species->add_callback(nn_trigger_delayed_autosearch_cb);
        nn_trigger_delayed_autosearch_cb(awr);
    }
    else {
        awar_sel_species->remove_callback(nn_trigger_delayed_autosearch_cb);
    }
}

static AW_window *nn_of_sel_win = NULL;
static void nn_searchRel_awar_changed_cb(AW_root *awr) {
    int auto_search = awr->awar(AWAR_NN_SELECTED_AUTO_SEARCH)->read_int();
    if (auto_search &&
        nn_of_sel_win && nn_of_sel_win->is_shown()) // do not trigger if window is not shown
    {
        nn_trigger_delayed_autosearch_cb(awr);
    }
}

static void create_next_neighbours_vars(AW_root *aw_root) {
    static bool created = false;

    if (!created) {
        RootCallback searchRel_awar_changed_cb = makeRootCallback(nn_searchRel_awar_changed_cb);

        aw_root->awar_int(AWAR_PROBE_ADMIN_PT_SERVER)->add_callback(searchRel_awar_changed_cb);
        aw_root->awar_int(AWAR_NN_COMPLEMENT,  FF_FORWARD)->add_callback(searchRel_awar_changed_cb);

        aw_root->awar_string(AWAR_NN_RANGE_START, "")->add_callback(searchRel_awar_changed_cb);
        aw_root->awar_string(AWAR_NN_RANGE_END,   "")->add_callback(searchRel_awar_changed_cb);
        aw_root->awar_int   (AWAR_NN_MAX_HITS,    10)->set_minmax(1, 1000)->add_callback(searchRel_awar_changed_cb);;
        aw_root->awar_float (AWAR_NN_MIN_SCORE,   80)->set_minmax(0, 200)->add_callback(searchRel_awar_changed_cb);;
        
        aw_root->awar_int(AWAR_NN_SELECTED_HIT_COUNT,   0);
        aw_root->awar_int(AWAR_NN_SELECTED_AUTO_SEARCH, 0)->add_callback(nn_auto_search_changed_cb);
        aw_root->awar_int(AWAR_NN_SELECTED_AUTO_MARK,   0);

        aw_root->awar_string(AWAR_NN_LISTED_DEST_FIELD,     "tmp");
        aw_root->awar_int   (AWAR_NN_LISTED_SCORED_ENTRIES, 1);

        AWTC_create_common_next_neighbour_vars(aw_root, searchRel_awar_changed_cb);

        created = true;
    }
}

static AWT_config_mapping_def next_neighbour_config_mapping[] = {
    // same as ../FAST_ALIGNER/fast_aligner.cxx@RELATIVES_CONFIG
    { AWAR_NN_OLIGO_LEN,   "oligolen" },
    { AWAR_NN_MISMATCHES,  "mismatches" },
    { AWAR_NN_FAST_MODE,   "fastmode" },
    { AWAR_NN_REL_MATCHES, "relmatches" },
    { AWAR_NN_REL_SCALING, "relscaling" },

    { AWAR_NN_COMPLEMENT,  "complement" },
    { AWAR_NN_RANGE_START, "rangestart" },
    { AWAR_NN_RANGE_END,   "rangeend" },
    { AWAR_NN_MAX_HITS,    "maxhits" },
    { AWAR_NN_MIN_SCORE,   "minscore" },

    { 0, 0}
};

static void setup_next_neighbour_config(AWT_config_definition& cdef, bool for_listed) {
    // fields common for 'listed' and 'selected'
    cdef.add(next_neighbour_config_mapping);

    if (for_listed) {
        cdef.add(AWAR_NN_LISTED_SCORED_ENTRIES, "addscore");
    }
    else {
        cdef.add(AWAR_NN_SELECTED_AUTO_SEARCH, "autosearch");
        cdef.add(AWAR_NN_SELECTED_AUTO_MARK,   "automark");
    }
}

static void create_common_next_neighbour_fields(AW_window *aws, bool for_listed) {
    aws->at("pt_server");
    awt_create_PTSERVER_selection_button(aws, AWAR_PROBE_ADMIN_PT_SERVER);

    const int SCALER_LENGTH = 200;

    aws->auto_space(5, 5);
    AWTC_create_common_next_neighbour_fields(aws, SCALER_LENGTH);

    
    aws->at("range");
    aws->create_input_field(AWAR_NN_RANGE_START, 6);
    aws->create_input_field(AWAR_NN_RANGE_END,   6);

    aws->at("compl");
    aws->create_option_menu(AWAR_NN_COMPLEMENT, true);
    aws->insert_default_option("forward",            "", FF_FORWARD);
    aws->insert_option        ("reverse",            "", FF_REVERSE);
    aws->insert_option        ("complement",         "", FF_COMPLEMENT);
    aws->insert_option        ("reverse-complement", "", FF_REVERSE_COMPLEMENT);
    aws->insert_option        ("fwd + rev-compl",    "", FF_FORWARD|FF_REVERSE_COMPLEMENT);
    aws->insert_option        ("rev + compl",        "", FF_REVERSE|FF_COMPLEMENT);
    aws->insert_option        ("any",                "", FF_FORWARD|FF_REVERSE|FF_COMPLEMENT|FF_REVERSE_COMPLEMENT);
    aws->update_option_menu();

    aws->at("results");
    aws->create_input_field_with_scaler(AWAR_NN_MAX_HITS, 5, SCALER_LENGTH, AW_SCALER_EXP_LOWER);

    aws->at("min_score");
    aws->create_input_field_with_scaler(AWAR_NN_MIN_SCORE, 5, SCALER_LENGTH, AW_SCALER_LINEAR);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "next_neighbours", makeConfigSetupCallback(setup_next_neighbour_config, for_listed));
}

static AW_window *create_next_neighbours_listed_window(AW_root *aw_root, DbQuery *query) {
    static AW_window_simple *aws = 0;
    NN_GLOBAL.set_query(query);
    if (!aws) {
        create_next_neighbours_vars(aw_root);

        aws = new AW_window_simple;
        aws->init(aw_root, "SEARCH_NEXT_RELATIVES_OF_LISTED", "Search Next Neighbours of Listed");
        aws->load_xfig("ad_spec_nnm.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("next_neighbours_listed.hlp"));
        aws->create_button("HELP", "Help", "H");

        create_common_next_neighbour_fields(aws, true);

        aws->at("add_score");
        aws->create_toggle(AWAR_NN_LISTED_SCORED_ENTRIES);
        
        aws->at("field");
        create_selection_list_on_itemfields(query_get_gb_main(query), aws, AWAR_NN_LISTED_DEST_FIELD, true, (1<<GB_INT) | (1<<GB_STRING), "field", 0, SPECIES_get_selector(), 20, 10, SF_STANDARD, NULL);

        aws->at("go");
        aws->callback(awtc_nn_search_all_listed);
        aws->create_autosize_button("WRITE_FIELDS", "Write to field");
    }
    return aws;
}

static AW_window *create_next_neighbours_selected_window(AW_root *aw_root, DbQuery *query) {
    static AW_window_simple *aws = 0;
    NN_GLOBAL.set_query(query);
    if (!aws) {
        create_next_neighbours_vars(aw_root);

        aws = new AW_window_simple;
        aws->init(aw_root, "SEARCH_NEXT_RELATIVE_OF_SELECTED", "Search Next Neighbours of Selected");
        aws->load_xfig("ad_spec_nn.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("next_neighbours.hlp"));
        aws->create_button("HELP", "Help", "H");

        create_common_next_neighbour_fields(aws, false);

        aws->button_length(10);
        aws->at("hit_count");
        aws->create_button(0, AWAR_NN_SELECTED_HIT_COUNT, 0, "+");

        aws->at("hits");
        AW_selection_list *resultList = aws->create_selection_list(AWAR_SPECIES_NAME, false);
        NN_GLOBAL.set_result_list(resultList);
        resultList->insert_default("No hits found", "");
        resultList->update();

        aws->at("go");
        aws->callback(awtc_nn_search);
        aws->create_button("SEARCH", "Search");

        aws->at("auto_go");
        aws->label("Auto search on change");
        aws->create_toggle(AWAR_NN_SELECTED_AUTO_SEARCH);
        
        aws->at("mark");
        aws->callback(awtc_mark_hits);
        aws->create_autosize_button("MARK_HITS", "Mark hits");

        aws->at("auto_mark");
        aws->label("Auto");
        aws->create_toggle(AWAR_NN_SELECTED_AUTO_MARK);

        aws->at("move");
        aws->callback(awtc_move_hits);
        aws->create_autosize_button("MOVE_TO_HITLIST", "Move to hitlist");

        nn_of_sel_win = aws; // store current window (to disable auto search when this window was popped down)
    }
    return aws;
}

// ---------------------------------------------
//      species/organism specific callbacks

static AW_window *popup_new_speciesOrganismWindow(AW_root *aw_root, GBDATA *gb_main, bool organismWindow, int detach_id);

static void popup_detached_speciesOrganismWindow(AW_window *aw_parent, const InfoWindow *infoWin) {
    const InfoWindow *reusable = InfoWindowRegistry::infowin.find_reusable_of_same_type_as(*infoWin);
    if (reusable) reusable->reuse();
    else { // create a new window if none is reusable
        popup_new_speciesOrganismWindow(aw_parent->get_root(),
                                        infoWin->get_gbmain(),
                                        infoWin->mapsOrganism(),
                                        InfoWindowRegistry::infowin.allocate_detach_id(*infoWin));
    }
}

static AW_window *popup_new_speciesOrganismWindow(AW_root *aw_root, GBDATA *gb_main, bool organismWindow, int detach_id) { // INFO_WINDOW_CREATOR
    // if detach_id is MAIN_WINDOW -> create main window (not detached)

    AW_window_simple_menu *aws      = new AW_window_simple_menu;
    const ItemSelector&    itemType = organismWindow ? ORGANISM_get_selector() : SPECIES_get_selector();

    init_info_window(aw_root, aws, itemType, detach_id);

    aws->load_xfig("ad_spec.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("search");
    aws->callback(makeCreateWindowCallback(DBUI::create_species_query_window, gb_main));
    aws->create_autosize_button("SEARCH", "Search...", "S");

    aws->at("help");
    aws->callback(makeHelpCallback(detach_id ? "sp_info_locked.hlp" : "sp_info.hlp")); // uses_hlp_res("sp_info_locked.hlp", "sp_info.hlp"); see ../../SOURCE_TOOLS/check_resources.pl@uses_hlp_res
    aws->create_button("HELP", "Help", "H");

    DbScanner         *scanner = create_db_scanner(gb_main, aws, "box", 0, "field", "enable", DB_VIEWER, 0, "mark", FIELD_FILTER_NDS, itemType);
    const InfoWindow&  infoWin = InfoWindowRegistry::infowin.registerInfoWindow(aws, scanner, detach_id);

    if (infoWin.is_maininfo()) {
        if (organismWindow) aws->create_menu("ORGANISM",    "O", AWM_ALL);
        else                aws->create_menu("SPECIES",     "S", AWM_ALL);

        aws->insert_menu_topic("species_delete",                "Delete", "D", "spa_delete.hlp", AWM_ALL, makeWindowCallback      (species_delete_cb,            gb_main));
        aws->insert_menu_topic("species_rename",                "Rename", "R", "spa_rename.hlp", AWM_ALL, makeWindowCallback      (species_rename_cb,            gb_main));
        aws->insert_menu_topic("species_copy",                  "Copy",   "y", "spa_copy.hlp",   AWM_ALL, makeWindowCallback      (species_copy_cb,              gb_main));
        aws->insert_menu_topic(aws->local_id("species_create"), "Create", "C", "spa_create.hlp", AWM_ALL, makeCreateWindowCallback(create_species_create_window, gb_main));
        aws->sep______________();
        aws->insert_menu_topic("species_convert_2_sai", "Convert to SAI", "S", "sp_sp_2_ext.hlp", AWM_ALL, makeWindowCallback      (move_species_to_extended,     gb_main));
    }

    aws->create_menu("FIELDS", "F", AWM_ALL);
    insert_field_admin_menuitems(aws, gb_main);

    aws->at("detach");
    infoWin.add_detachOrGet_button(popup_detached_speciesOrganismWindow);

    aws->show();
    infoWin.attach_selected_item();
    return aws;
}

static void popup_speciesOrganismWindow(AW_root *aw_root, GBDATA *gb_main, bool organismWindow) {
    int windowIdx = (int)organismWindow;

    static AW_window *AWS[2] = { 0, 0 };
    if (!AWS[windowIdx]) {
        AWS[windowIdx] = popup_new_speciesOrganismWindow(aw_root, gb_main, organismWindow, InfoWindow::MAIN_WINDOW);
    }
    else {
        AWS[windowIdx]->activate();
    }
}

void DBUI::popup_species_info_window (AW_root *aw_root, GBDATA *gb_main) { popup_speciesOrganismWindow(aw_root, gb_main, false); }
void DBUI::popup_organism_info_window(AW_root *aw_root, GBDATA *gb_main) { popup_speciesOrganismWindow(aw_root, gb_main, true);  }

static DbQuery *GLOBAL_species_query = NULL; // @@@ fix design

void DBUI::unquery_all() {
    QUERY::unquery_all(0, GLOBAL_species_query);
}

void DBUI::query_update_list() {
    DbQuery_update_list(GLOBAL_species_query);
}

AW_window *DBUI::create_species_query_window(AW_root *aw_root, GBDATA *gb_main) {
    static AW_window_simple_menu *aws = 0;
    if (!aws) {
        aws = new AW_window_simple_menu;
        aws->init(aw_root, "SPECIES_QUERY", "SEARCH and QUERY");
        aws->create_menu("More functions", "f");
        aws->load_xfig("ad_query.fig");


        query_spec awtqs(SPECIES_get_selector());

        awtqs.gb_main             = gb_main;
        awtqs.species_name        = AWAR_SPECIES_NAME;
        awtqs.tree_name           = AWAR_TREE;
        awtqs.select_bit          = GB_USERFLAG_QUERY;
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
        awtqs.popup_info_window   = popup_species_info_window;

        DbQuery *query           = create_query_box(aws, &awtqs, "spec");
        GLOBAL_species_query = query;

        aws->create_menu("More search",     "s");
        aws->insert_menu_topic("spec_search_equal_fields_within_db", "Search For Equal Fields and Mark Duplicates",                "E", "search_duplicates.hlp",      AWM_ALL, makeWindowCallback      (search_duplicated_field_content,        query, false));
        aws->insert_menu_topic("spec_search_equal_words_within_db",  "Search For Equal Words Between Fields and Mark Duplicates",  "W", "search_duplicates.hlp",      AWM_ALL, makeWindowCallback      (search_duplicated_field_content,        query, true));
        aws->insert_menu_topic("spec_search_next_relativ_of_sel",    "Search Next Relatives of SELECTED Species in PT_Server ...", "R", "next_neighbours.hlp",        AWM_ALL, makeCreateWindowCallback(create_next_neighbours_selected_window, query));
        aws->insert_menu_topic("spec_search_next_relativ_of_listed", "Search Next Relatives of LISTED Species in PT_Server ...",   "L", "next_neighbours_listed.hlp", AWM_ALL, makeCreateWindowCallback(create_next_neighbours_listed_window,   query));

        aws->button_length(7);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("sp_search.hlp"));
        aws->create_button("HELP", "Help", "H");
    }
    return aws;
}


