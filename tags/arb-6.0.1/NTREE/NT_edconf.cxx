// =============================================================== //
//                                                                 //
//   File      : NT_edconf.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_cb.h"
#include "NT_local.h"

#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <ad_config.h>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <arb_global_defs.h>

static void init_config_awars(AW_root *root) {
    root->awar_string(AWAR_CONFIGURATION, "default_configuration", GLOBAL.gb_main);
}

// -----------------------------
//      class Store_species

class Store_species : virtual Noncopyable {
    // stores an amount of species:
    GBT_TREE *node;
    Store_species *next;
public:
    Store_species(GBT_TREE *aNode) {
        node = aNode;
        next = 0;
    }
    ~Store_species();

    Store_species* add(Store_species *list) {
        nt_assert(next==0);
        next = list;
        return this;
    }

    Store_species* remove() {
        Store_species *follower = next;
        next = 0;
        return follower;
    }

    GBT_TREE *getNode() const { return node; }

    void call(void (*aPizza)(GBT_TREE*)) const;
};

Store_species::~Store_species() {
    delete next;
}

void Store_species::call(void (*aPizza)(GBT_TREE*)) const {
    aPizza(node);
    if (next) next->call(aPizza);
}

static void unmark_species(GBT_TREE *node) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)!=0);
    GB_write_flag(node->gb_node, 0);
}

static void mark_species(GBT_TREE *node, Store_species **extra_marked_species) {
    nt_assert(node);
    nt_assert(node->gb_node);
    nt_assert(GB_read_flag(node->gb_node)==0);
    GB_write_flag(node->gb_node, 1);

    *extra_marked_species = (new Store_species(node))->add(*extra_marked_species);
}



static GBT_TREE *rightmost_leaf(GBT_TREE *node) {
    nt_assert(node);
    while (!node->is_leaf) {
        node = node->rightson;
        nt_assert(node);
    }
    return node;
}

static GBT_TREE *left_neighbour_leaf(GBT_TREE *node) {
    if (node) {
        GBT_TREE *father = node->father;
        while (father) {
            if (father->rightson==node) {
                node = rightmost_leaf(father->leftson);
                nt_assert(node->is_leaf);
                if (!node->gb_node) { // Zombie
                    node = left_neighbour_leaf(node);
                }
                return node;
            }
            node = father;
            father = node->father;
        }
    }
    return 0;
}

static int nt_build_conf_string_rek(GB_HASH *used, GBT_TREE *tree, GBS_strstruct *memfile,
                             Store_species **extra_marked_species, int use_species_aside,
                             int *auto_mark, int marked_at_left, int *marked_at_right)
{
    /*! Builds a configuration string from a tree.
     *
     * @param used                      all species inserted by this function are stored here
     * @param tree                      used for group information
     * @param memfile                   generated configuration string is stored here
     * @param extra_marked_species      all extra marked species are inserted here
     * @param use_species_aside         number of species to mark left and right of marked species
     * @param auto_mark                 number species to extra-mark (if not already marked)
     * @param marked_at_left            number of species which were marked (looking to left)
     * @param marked_at_right           number of species which are marked (when returning from recursion)
     *
     * @return the number of marked species
     *
     * --------------------------------------------------
     * Format of configuration string : [Part]+ \0
     *
     * Part : '\A' ( Group | Species | Sai )
     *
     * Group : ( OpenedGroup | ClosedGroup )
     * OpenedGroup : 'G' GroupDef
     * ClosedGroup : 'F' GroupDef
     * GroupDef : 'groupname' [PART]* EndGroup
     * EndGroup : '\AE'
     *
     * SPECIES : 'L' 'speciesname'
     * SAI : 'S' 'sainame'
     *
     * \0 : ASCII 0 (eos)
     * \A : ASCII 1
     */

    if (!tree) return 0;
    if (tree->is_leaf) {
        if (!tree->gb_node) {
            *marked_at_right = marked_at_left;
            return 0;   // Zombie
        }

        if (!GB_read_flag(tree->gb_node)) { // unmarked species
            if (*auto_mark) {
                (*auto_mark)--;
                mark_species(tree, extra_marked_species);
            }
            else {
                *marked_at_right = 0;
                return 0;
            }
        }
        else { // marked species
            if (marked_at_left<use_species_aside) {
                // on the left side there are not as many marked species as needed!

                nt_assert(marked_at_left>=0);

                GBT_TREE *leaf_at_left = tree;
                int       step_over    = marked_at_left+1; // step over myself
                int       then_mark    = use_species_aside-marked_at_left;

                while (step_over--) { // step over self and over any adjacent, marked species
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                Store_species *marked_back = 0;
                while (leaf_at_left && then_mark--) { // then additionally mark some species
                    if (GB_read_flag(leaf_at_left->gb_node) == 0) { // if they are not marked yet
                        mark_species(leaf_at_left, extra_marked_species);
                        marked_back = (new Store_species(leaf_at_left))->add(marked_back);
                    }
                    leaf_at_left = left_neighbour_leaf(leaf_at_left);
                }

                while (marked_back) {
                    GBS_chrcat(memfile, 1);             // Separated by 1
                    GBS_strcat(memfile, "L");
                    GBS_strcat(memfile, marked_back->getNode()->name);
                    GBS_write_hash(used, marked_back->getNode()->name, 1);      // Mark species

                    Store_species *rest = marked_back->remove();
                    delete marked_back;
                    marked_back = rest;
                }

                marked_at_left = use_species_aside;
            }
            // now use_species_aside species to left are marked!
            *auto_mark = use_species_aside;
        }

        GBS_chrcat(memfile, 1);             // Separated by 1
        GBS_strcat(memfile, "L");
        GBS_strcat(memfile, tree->name);
        GBS_write_hash(used, tree->name, 1);    // Mark species

        *marked_at_right = marked_at_left+1;
        return 1;
    }

    long oldpos = GBS_memoffset(memfile);
    if (tree->gb_node && tree->name) {      // but we are a group
        GBDATA *gb_grouped = GB_entry(tree->gb_node, "grouped");
        GBS_chrcat(memfile, 1);             // Separated by 1
        if (gb_grouped && GB_read_byte(gb_grouped)) {
            GBS_strcat(memfile, "F");
        }
        else {
            GBS_strcat(memfile, "G");
        }

        GBS_strcat(memfile, tree->name);
    }

    int right_of_leftson;
    long nspecies = nt_build_conf_string_rek(used, tree->leftson, memfile, extra_marked_species, use_species_aside, auto_mark, marked_at_left, &right_of_leftson);
    nspecies += nt_build_conf_string_rek(used, tree->rightson, memfile, extra_marked_species, use_species_aside, auto_mark, right_of_leftson, marked_at_right);

    if (tree->gb_node && tree->name) {      // but we are a group
        GBS_chrcat(memfile, 1);         // Separated by 1
        GBS_chrcat(memfile, 'E');        // Group end indicated by 'E'
    }

    if (!nspecies) {
        long newpos = GBS_memoffset(memfile);
        GBS_str_cut_tail(memfile, newpos-oldpos);   // delete group info
    }
    return nspecies;
}

struct SAI_string_builder {
    GBS_strstruct *sai_middle;
    const char    *last_group_name;
};

static long nt_build_sai_string_by_hash(const char *key, long val, void *cd_sai_builder) {
    SAI_string_builder *sai_builder = (SAI_string_builder*)cd_sai_builder;

    const char *sep = strchr(key, 1);
    if (!sep) return val;                           // what's wrong

    GBS_strstruct *sai_middle      = sai_builder->sai_middle;
    const char    *last_group_name = sai_builder->last_group_name;

    if (!last_group_name || strncmp(key, last_group_name, sep-key)) { // new group
        if (last_group_name) {
            GBS_chrcat(sai_middle, 1);              // Separated by 1
            GBS_chrcat(sai_middle, 'E');             // End of old group
        }
        GBS_chrcat(sai_middle, 1);                  // Separated by 1
        GBS_strcat(sai_middle, "FSAI:");
        GBS_strncat(sai_middle, key, sep-key);
        sai_builder->last_group_name = key;
    }
    GBS_chrcat(sai_middle, 1);                      // Separated by 1
    GBS_strcat(sai_middle, "S");
    GBS_strcat(sai_middle, sep+1);
    return val;
}


static void nt_build_sai_string(GBS_strstruct *topfile, GBS_strstruct *middlefile) {
    //! collect all Sais, place some SAI in top area, rest in middle

    GBDATA *gb_sai_data = GBT_get_SAI_data(GLOBAL.gb_main);
    if (gb_sai_data) {
        GB_HASH *hash = GBS_create_hash(GB_number_of_subentries(gb_sai_data), GB_IGNORE_CASE);

        for (GBDATA *gb_sai = GBT_first_SAI_rel_SAI_data(gb_sai_data); gb_sai; gb_sai = GBT_next_SAI(gb_sai)) {
            GBDATA *gb_name = GB_search(gb_sai, "name", GB_FIND);
            if (gb_name) {
                char *name = GB_read_string(gb_name);

                if (strcmp(name,  "HELIX") == 0  || strcmp(name,  "HELIX_NR") == 0 || strcmp(name,  "ECOLI") == 0) {
                    GBS_chrcat(topfile, 1);             // Separated by 1
                    GBS_strcat(topfile, "S");
                    GBS_strcat(topfile, name);
                }
                else {
                    GBDATA *gb_gn = GB_search(gb_sai, "sai_group", GB_FIND);
                    char   *gn;

                    if (gb_gn)  gn = GB_read_string(gb_gn);
                    else        gn = strdup("SAI's");

                    char *cn = new char[strlen(gn) + strlen(name) + 2];
                    sprintf(cn, "%s%c%s", gn, 1, name);
                    GBS_write_hash(hash, cn, 1);
                    delete [] cn;
                    free(gn);
                }
                free(name);
            }
        }

        // open surrounding SAI-group:
        GBS_chrcat(middlefile, 1);
        GBS_strcat(middlefile, "GSAI-Maingroup");

        SAI_string_builder sai_builder = { middlefile, 0 };
        GBS_hash_do_sorted_loop(hash, nt_build_sai_string_by_hash, GBS_HCF_sortedByKey, &sai_builder);
        if (sai_builder.last_group_name) {
            GBS_chrcat(middlefile, 1);              // Separated by 1
            GBS_chrcat(middlefile, 'E');             // End of old group
        }

        // close surrounding SAI-group:
        GBS_chrcat(middlefile, 1);
        GBS_chrcat(middlefile, 'E');

        GBS_free_hash(hash);
    }
}

static void nt_build_conf_marked(GB_HASH *used, GBS_strstruct *file) {
    GBS_chrcat(file, 1);            // Separated by 1
    GBS_strcat(file, "FMore Sequences");
    GBDATA *gb_species;
    for (gb_species = GBT_first_marked_species(GLOBAL.gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)) {
        char *name = GBT_read_string(gb_species, "name");
        if (GBS_read_hash(used, name)) {
            free(name);
            continue;
        }
        GBS_chrcat(file, 1);
        GBS_strcat(file, "L");
        GBS_strcat(file, name);
        free(name);
    }

    GBS_chrcat(file, 1); // Separated by 1
    GBS_chrcat(file, 'E');   // Group end indicated by 'E'
}

enum extractType {
    CONF_EXTRACT,
    CONF_MARK,
    CONF_UNMARK,
    CONF_INVERT,
    CONF_COMBINE // logical AND
};

static void nt_extract_configuration(AW_window *aww, extractType ext_type) {
    GB_transaction  ta(GLOBAL.gb_main);
    AW_root        *aw_root = aww->get_root();
    char           *cn      = aw_root->awar(AWAR_CONFIGURATION)->read_string();

    if (strcmp(cn, NO_CONFIG_SELECTED) == 0) {
        aw_message("Please select a configuration");
    }
    else {
        GBDATA *gb_configuration = GBT_find_configuration(GLOBAL.gb_main, cn);
        if (!gb_configuration) {
            aw_message(GBS_global_string("Configuration '%s' not found in the database", cn));
        }
        else {
            GBDATA *gb_middle_area  = GB_search(gb_configuration, "middle_area", GB_STRING);
            char   *md              = 0;
            size_t  unknown_species = 0;
            bool    refresh         = false;

            if (gb_middle_area) {
                GB_HASH *was_marked = NULL; // only used for CONF_COMBINE

                switch (ext_type) {
                    case CONF_EXTRACT:
                        GBT_mark_all(GLOBAL.gb_main, 0); // unmark all for extract
                        refresh = true;
                        break;
                    case CONF_COMBINE: {
                        // store all marked species in hash and unmark them
                        was_marked = GBS_create_hash(GBT_get_species_count(GLOBAL.gb_main), GB_IGNORE_CASE);
                        for (GBDATA *gbd = GBT_first_marked_species(GLOBAL.gb_main); gbd; gbd = GBT_next_marked_species(gbd)) {
                            int marked = GB_read_flag(gbd);
                            if (marked) {
                                GBS_write_hash(was_marked, GBT_read_name(gbd), 1);
                                GB_write_flag(gbd, 0);
                            }
                        }

                        refresh = refresh || GBS_hash_count_elems(was_marked);
                        break;
                    }
                    default:
                        break;
                }

                md = GB_read_string(gb_middle_area);
                if (md) {
                    char *p;
                    char tokens[2];
                    tokens[0] = 1;
                    tokens[1] = 0;
                    for (p = strtok(md, tokens); p; p = strtok(NULL, tokens)) {
                        if (p[0] == 'L') {
                            const char *species_name = p+1;
                            GBDATA     *gb_species   = GBT_find_species(GLOBAL.gb_main, species_name);

                            if (gb_species) {
                                int oldmark = GB_read_flag(gb_species);
                                int newmark = oldmark;
                                switch (ext_type) {
                                    case CONF_EXTRACT:
                                    case CONF_MARK:     newmark = 1; break;
                                    case CONF_UNMARK:   newmark = 0; break;
                                    case CONF_INVERT:   newmark = !oldmark; break;
                                    case CONF_COMBINE: {
                                        nt_assert(!oldmark); // should have been unmarked above
                                        newmark = GBS_read_hash(was_marked, species_name); // mark if was_marked
                                        break;
                                    }
                                    default: nt_assert(0); break;
                                }
                                if (newmark != oldmark) {
                                    GB_write_flag(gb_species, newmark);
                                    refresh = true;
                                }
                            }
                            else {
                                unknown_species++;
                            }
                        }
                    }
                }

                if (was_marked) GBS_free_hash(was_marked);
            }

            if (unknown_species>0) {
                aw_message(GBS_global_string("configuration '%s' contains %zu unknown species", cn, unknown_species));
            }

            if (refresh) aw_root->awar(AWAR_TREE_REFRESH)->touch();

            free(md);
        }
    }
    free(cn);
}

static void nt_delete_configuration(AW_window *aww) {
    GB_transaction transaction_var(GLOBAL.gb_main);
    char *cn = aww->get_root()->awar(AWAR_CONFIGURATION)->read_string();
    GBDATA *gb_configuration = GBT_find_configuration(GLOBAL.gb_main, cn);
    if (gb_configuration) {
        GB_ERROR error = GB_delete(gb_configuration);
        if (error) aw_message(error);
    }
    free(cn);
}


static GB_ERROR nt_create_configuration(AW_window *, GBT_TREE *tree, const char *conf_name, int use_species_aside) {
    char     *to_free = NULL;
    GB_ERROR  error   = NULL;

    if (!conf_name) {
        char *existing_configs = awt_create_string_on_configurations(GLOBAL.gb_main);
        conf_name              = to_free = aw_string_selection2awar("CREATE CONFIGURATION", "Enter name of configuration:", AWAR_CONFIGURATION, existing_configs, NULL);
        free(existing_configs);
    }

    if (!conf_name || !conf_name[0]) error = "no config name given";
    else {
        if (use_species_aside==-1) {
            static int last_used_species_aside = 3;
            {
                const char *val                    = GBS_global_string("%i", last_used_species_aside);
                char       *use_species            = aw_input("How many extra species to view aside marked:", val);
                if (use_species) use_species_aside = atoi(use_species);
                free(use_species);
            }

            if (use_species_aside<1) error = "illegal number of 'species aside'";
            else last_used_species_aside = use_species_aside; // remember for next time
        }

        if (!error) {
            GB_transaction  ta(GLOBAL.gb_main); // open close transaction
            GB_HASH        *used    = GBS_create_hash(GBT_get_species_count(GLOBAL.gb_main), GB_MIND_CASE);
            GBS_strstruct  *topfile = GBS_stropen(1000);
            GBS_strstruct  *topmid  = GBS_stropen(10000);
            {
                GBS_strstruct *middlefile = GBS_stropen(10000);
                nt_build_sai_string(topfile, topmid);

                if (use_species_aside) {
                    Store_species *extra_marked_species = 0;
                    int            auto_mark            = 0;
                    int            marked_at_right;
                    
                    nt_build_conf_string_rek(used, tree, middlefile, &extra_marked_species, use_species_aside, &auto_mark, use_species_aside, &marked_at_right);
                    if (extra_marked_species) {
                        extra_marked_species->call(unmark_species);
                        delete extra_marked_species;
                    }
                }
                else {
                    int dummy_1=0, dummy_2;
                    nt_build_conf_string_rek(used, tree, middlefile, 0, 0, &dummy_1, 0, &dummy_2);
                }
                nt_build_conf_marked(used, topmid);
                char *mid = GBS_strclose(middlefile);
                GBS_strcat(topmid, mid);
                free(mid);
            }

            {
                char   *middle    = GBS_strclose(topmid);
                char   *top       = GBS_strclose(topfile);
                GBDATA *gb_config = GBT_create_configuration(GLOBAL.gb_main, conf_name);

                if (!gb_config) error = GB_await_error();
                else {
                    error             = GBT_write_string(gb_config, "top_area", top);
                    if (!error) error = GBT_write_string(gb_config, "middle_area", middle);
                }

                free(middle);
                free(top);
            }
            GBS_free_hash(used);
        }
    }
    
    free(to_free);
    return error;
}

static void nt_store_configuration(AW_window*, AWT_canvas *ntw) {
    GB_ERROR err = nt_create_configuration(0, nt_get_tree_root_of_canvas(ntw), 0, 0);
    aw_message_if(err);
}

static void nt_rename_configuration(AW_window *aww) {
    AW_awar  *awar_curr_cfg = aww->get_root()->awar(AWAR_CONFIGURATION);
    char     *old_name      = awar_curr_cfg->read_string();
    GB_ERROR  err           = 0;

    GB_transaction ta(GLOBAL.gb_main);

    char *new_name = aw_input("Rename selection", "Enter the new name of the selection", old_name);
    if (new_name) {
        GBDATA *gb_existing_cfg  = GBT_find_configuration(GLOBAL.gb_main, new_name);
        if (gb_existing_cfg) err = GBS_global_string("There is already a selection named '%s'", new_name);
        else {
            GBDATA *gb_old_cfg = GBT_find_configuration(GLOBAL.gb_main, old_name);
            if (gb_old_cfg) {
                GBDATA *gb_name = GB_entry(gb_old_cfg, "name");
                if (gb_name) {
                    err = GB_write_string(gb_name, new_name);
                    if (!err) awar_curr_cfg->write_string(new_name);
                }
                else err = "Selection has no name";
            }
            else err = "Can't find that selection";
        }
        free(new_name);
    }

    if (err) aw_message(err);
    free(old_name);
}

static AW_window *create_configuration_admin_window(AW_root *root, AWT_canvas *ntw) {
    static AW_window_simple *existing_aws[MAX_NT_WINDOWS] = { MAX_NT_WINDOWS_NULLINIT };

    int ntw_id = NT_get_canvas_id(ntw);
    if (!existing_aws[ntw_id]) {
        init_config_awars(root);

        AW_window_simple *aws = new AW_window_simple;
        aws->init(root, "SPECIES_SELECTIONS", "Species Selections");
        aws->load_xfig("nt_selection.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("configuration.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at("list");
        awt_create_selection_list_on_configurations(GLOBAL.gb_main, aws, AWAR_CONFIGURATION, false);

        aws->at("store");
        aws->callback(makeWindowCallback(nt_store_configuration, ntw));
        aws->create_button(GBS_global_string("STORE_%i", ntw_id), "STORE", "S");

        aws->at("extract");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_EXTRACT));
        aws->create_button("EXTRACT", "EXTRACT", "E");

        aws->at("mark");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_MARK));
        aws->create_button("MARK", "MARK", "M");

        aws->at("unmark");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_UNMARK));
        aws->create_button("UNMARK", "UNMARK", "U");

        aws->at("invert");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_INVERT));
        aws->create_button("INVERT", "INVERT", "I");

        aws->at("combine");
        aws->callback(makeWindowCallback(nt_extract_configuration, CONF_COMBINE));
        aws->create_button("COMBINE", "COMBINE", "C");

        aws->at("delete");
        aws->callback(nt_delete_configuration);
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("rename");
        aws->callback(nt_rename_configuration);
        aws->create_button("RENAME", "RENAME", "R");

        existing_aws[ntw_id] = aws;
    }
    return existing_aws[ntw_id];
}

void NT_popup_configuration_admin(AW_window *aw_main, AW_CL cl_ntw, AW_CL) {
    AW_window *aww = create_configuration_admin_window(aw_main->get_root(), (AWT_canvas*)cl_ntw);
    aww->activate();
}

// -----------------------------------------
//      various ways to start the editor

#define CONFNAME "default_configuration"

static void nt_start_editor_on_configuration(AW_window *aww) {
    aww->hide();

    const char *cn  = aww->get_root()->awar(AWAR_CONFIGURATION)->read_char_pntr();
    const char *com = GBS_global_string("arb_edit4 -c '%s' &", cn);

    aw_message_if(GBK_system(com));
}

AW_window *NT_create_startEditorOnOldConfiguration_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        init_config_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "SELECT_CONFIGURATION", "SELECT A CONFIGURATION");
        aws->at(10, 10);
        aws->auto_space(0, 0);
        awt_create_selection_list_on_configurations(GLOBAL.gb_main, aws, AWAR_CONFIGURATION, false);
        aws->at_newline();

        aws->callback((AW_CB0)nt_start_editor_on_configuration);
        aws->create_button("START", "START");

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->window_fit();
    }
    return aws;
}

void NT_start_editor_on_tree(AW_window *, AW_CL cl_use_species_aside, AW_CL cl_ntw) {
    GB_ERROR error = nt_create_configuration(0, nt_get_tree_root_of_canvas((AWT_canvas*)cl_ntw), CONFNAME, (int)cl_use_species_aside);
    if (!error) error = GBK_system("arb_edit4 -c " CONFNAME " &");
    aw_message_if(error);
}


