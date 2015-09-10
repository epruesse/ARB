#include <arbdbt.h>

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_list.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"

#if defined(DEBUG)
#define TEST_REFRESH_FLAG
#endif

// -----------------------------------------------------------------
//      Manager static properties (used by manager-constructors)

static ED4_objspec main_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_HORIZONTAL), // static props
    ED4_L_ROOT,                                            // level
    ED4_L_ROOTGROUP,                                       // allowed children level
    ED4_L_NO_LEVEL,                                        // handled object
    ED4_L_NO_LEVEL                                         // restriction level
    );

static ED4_objspec device_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_HORIZONTAL), // static props
    ED4_L_DEVICE,                                          // level
    (ED4_level)(ED4_L_AREA | ED4_L_SPACER | ED4_L_LINE),   // allowed children level
    ED4_L_NO_LEVEL,                                        // handled object
    ED4_L_NO_LEVEL                                         // restriction level
    );

static ED4_objspec area_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL),          // static props
    ED4_L_AREA,                                                   // level
    (ED4_level)(ED4_L_MULTI_SPECIES | ED4_L_TREE | ED4_L_SPACER), // allowed children level
    ED4_L_NO_LEVEL,                                               // handled object
    ED4_L_NO_LEVEL                                                // restriction level
    );

static ED4_objspec multi_species_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_HORIZONTAL),   // static props
    ED4_L_MULTI_SPECIES,                                     // level
    (ED4_level)(ED4_L_SPECIES | ED4_L_GROUP | ED4_L_SPACER), // allowed children level
    ED4_L_NO_LEVEL,                                          // handled object
    ED4_L_NO_LEVEL                                           // restriction level
    );

static ED4_objspec species_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL),  // static props
    ED4_L_SPECIES,                                        // level
    // (ED4_level)(ED4_L_MULTI_SEQUENCE | ED4_L_MULTI_NAME), // allowed children level
    (ED4_level)(ED4_L_MULTI_SEQUENCE | ED4_L_MULTI_NAME | // (used by normal species) 
                ED4_L_SPECIES_NAME | ED4_L_SEQUENCE), // allowed children level (used by consensus)
    ED4_L_NO_LEVEL,                                       // handled object
    ED4_L_NO_LEVEL                                        // restriction level
    );
static ED4_objspec multi_sequence_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_HORIZONTAL), // static props
    ED4_L_MULTI_SEQUENCE,                                  // level
    ED4_L_SEQUENCE,                                        // allowed children level
    ED4_L_NO_LEVEL,                                        // handled object
    ED4_L_NO_LEVEL                                         // restriction level
    );

static ED4_objspec sequence_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL),      // static props
    ED4_L_SEQUENCE,                                           // level
    (ED4_level)(ED4_L_SEQUENCE_INFO | ED4_L_SEQUENCE_STRING | ED4_L_ORF | ED4_L_PURE_TEXT | ED4_L_COL_STAT), // allowed children level
    ED4_L_NO_LEVEL,                                           // handled object
    ED4_L_SPECIES                                             // restriction level
    );

static ED4_objspec multi_name_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_HORIZONTAL), // static props
    ED4_L_MULTI_NAME,                                      // level
    ED4_L_NAME_MANAGER,                                    // allowed children level
    ED4_L_NO_LEVEL,                                        // handled object
    ED4_L_NO_LEVEL                                         // restriction level
    );

static ED4_objspec name_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL), // static props
    ED4_L_NAME_MANAGER,                                  // level
    (ED4_level)(ED4_L_SPECIES_NAME),                     // allowed children level
    ED4_L_NO_LEVEL,                                      // handled object
    ED4_L_SPECIES                                        // restriction level
    );

static ED4_objspec group_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL),             // static props
    ED4_L_GROUP,                                                     // level
    (ED4_level)(ED4_L_MULTI_SPECIES | ED4_L_BRACKET), // allowed children level
    ED4_L_NO_LEVEL,                                                  // handled object
    ED4_L_NO_LEVEL                                                   // restriction level
    );

static ED4_objspec root_group_manager_spec(
    (ED4_properties)(ED4_P_IS_MANAGER | ED4_P_VERTICAL), // static props
    ED4_L_ROOTGROUP,                                     // level
    (ED4_level)(ED4_L_DEVICE),                           // allowed children level
    ED4_L_NO_LEVEL,                                      // handled object
    ED4_L_NO_LEVEL                                       // restriction level
    );

// ----------------------------
//      ED4_manager methods

ED4_returncode ED4_manager::rebuild_consensi(ED4_base *start_species, ED4_update_flag update_flag) {
    int                        i;
    ED4_base                  *temp_parent;
    ED4_multi_species_manager *multi_species_manager = NULL;
    ED4_group_manager         *first_group_manager   = NULL;

    temp_parent = start_species;

    switch (update_flag) {
        case ED4_U_UP:          // rebuild consensus from a certain starting point upwards
            while (temp_parent) {
                if (temp_parent->is_group_manager()) {
                    multi_species_manager = temp_parent->to_group_manager()->get_multi_species_manager();
                    for (i=0; i<multi_species_manager->children->members(); i++) {
                        if (multi_species_manager->children->member(i)->is_consensus_manager()) {
                            rebuild_consensus(multi_species_manager->children->member(i)).expect_no_error();
                        }
                    }
                }
                temp_parent = temp_parent->parent;
            }
            break;
        case ED4_U_UP_DOWN:     // only search first groupmanager and update consensi recursively downwards
            while (temp_parent) {
                if (temp_parent->is_group_manager()) {
                    first_group_manager = temp_parent->to_group_manager();
                }
                temp_parent = temp_parent->parent;
            }
            if (first_group_manager)
                first_group_manager->route_down_hierarchy(rebuild_consensus).expect_no_error();
            break;
    }
    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_in_bases(ED4_base *added_base) {
    if (added_base->is_species_manager()) { // add a sequence
        ED4_species_manager *species_manager = added_base->to_species_manager();
        const ED4_terminal *sequence_terminal = species_manager->get_consensus_relevant_terminal();

        if (sequence_terminal) {
            int   seq_len;
            char *seq          = sequence_terminal->resolve_pointer_to_string_copy(&seq_len);
            ED4_returncode res = update_bases(0, 0, seq, seq_len);
            free(seq);
            return res;
        }
    }

    e4_assert(!added_base->is_root_group_manager());
    if (added_base->is_group_manager()) { // add a group
        return update_bases(0, &added_base->to_group_manager()->table());
    }

    e4_assert(0); // wrong type
    
    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_out_bases(ED4_base *subbed_base) {
    if (subbed_base->is_species_manager()) { // sub a sequence
        ED4_species_manager *species_manager = subbed_base->to_species_manager();
        const ED4_terminal *sequence_terminal = species_manager->get_consensus_relevant_terminal();

        if (sequence_terminal) {
            int   seq_len;
            char *seq          = sequence_terminal->resolve_pointer_to_string_copy(&seq_len);
            ED4_returncode res = update_bases(seq, seq_len, (const char *)0, 0);
            free(seq);
            return res;
        }
    }
    else {
        e4_assert(!subbed_base->is_root_group_manager());
        if (subbed_base->is_group_manager()) { // sub a group
            return update_bases(&subbed_base->to_group_manager()->table(), 0);
        }
        e4_assert(0); // wrong type
    }
    return ED4_R_OK;
}

ED4_returncode ED4_manager::update_bases(const char *old_sequence, int old_len, const ED4_base *new_base, PosRange range) {
    if (!new_base) {
        return update_bases(old_sequence, old_len, 0, 0, range);
    }

    e4_assert(new_base->is_species_manager());

    const ED4_species_manager *new_species_manager   = new_base->to_species_manager();
    const ED4_terminal        *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();

    int   new_len;
    char *new_sequence = new_sequence_terminal->resolve_pointer_to_string_copy(&new_len);

    if (range.is_whole()) {
        const PosRange *restricted = ED4_char_table::changed_range(old_sequence, new_sequence, std::min(old_len, new_len));
        
        e4_assert(restricted);
        range = *restricted;
    }

    ED4_returncode res = update_bases(old_sequence, old_len, new_sequence, new_len, range);
    free(new_sequence);
    return res;
}

ED4_returncode ED4_manager::update_bases_and_rebuild_consensi(const char *old_sequence, int old_len, ED4_base *new_base, ED4_update_flag update_flag, PosRange range) {
    e4_assert(new_base);
    e4_assert(new_base->is_species_manager());

    ED4_species_manager *new_species_manager   = new_base->to_species_manager();
    const ED4_terminal  *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();

    int         new_len;
    const char *new_sequence = new_sequence_terminal->resolve_pointer_to_char_pntr(&new_len);

#if defined(DEBUG) && 0
    printf("old: %s\n", old_sequence);
    printf("new: %s\n", new_sequence);
#endif // DEBUG

    const PosRange *changedRange = 0;
    if (range.is_whole()) {
        changedRange = ED4_char_table::changed_range(old_sequence, new_sequence, std::min(old_len, new_len));
    }
    else {
        changedRange = &range; // @@@ use method similar to changed_range here, which just reduces the existing range
    }

    ED4_returncode result = ED4_R_OK;
    if (changedRange) {
        ED4_returncode result1 = update_bases(old_sequence, old_len, new_sequence, new_len, *changedRange);
        ED4_returncode result2 = rebuild_consensi(new_base, update_flag);

        result = (result1 != ED4_R_OK) ? result1 : result2;

        // Refresh aminoacid sequence terminals in Protein Viewer or protstruct // @@@ this is definitely wrong here (omg)
        if (ED4_ROOT->alignment_type == GB_AT_DNA) {
            PV_SequenceUpdate_CB(GB_CB_CHANGED);
        }
        else if (ED4_ROOT->alignment_type == GB_AT_AA) { 
            GB_ERROR err = ED4_pfold_set_SAI(&ED4_ROOT->protstruct, GLOBAL_gb_main, ED4_ROOT->alignment_name, &ED4_ROOT->protstruct_len);
            if (err) { aw_message(err); result = ED4_R_WARNING; }
        }
    }
    return result;
}

ED4_returncode ED4_manager::update_bases(const ED4_base *old_base, const ED4_base *new_base, PosRange range) {
    e4_assert(old_base);
    e4_assert(new_base);

    if (old_base->is_species_manager()) {
        e4_assert(new_base->is_species_manager());
        const ED4_species_manager *old_species_manager   = old_base->to_species_manager();
        const ED4_species_manager *new_species_manager   = new_base->to_species_manager();
        const ED4_terminal        *old_sequence_terminal = old_species_manager->get_consensus_relevant_terminal();
        const ED4_terminal        *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();

        int   old_len;
        int   new_len;
        char *old_seq = old_sequence_terminal->resolve_pointer_to_string_copy(&old_len);
        char *new_seq = new_sequence_terminal->resolve_pointer_to_string_copy(&new_len);

        ED4_returncode res = update_bases(old_seq, old_len, new_seq, new_len, range);
        free(new_seq);
        free(old_seq);
        return res;
    }

    e4_assert(!old_base->is_root_group_manager());
    e4_assert(!new_base->is_root_group_manager());
    if (old_base->is_group_manager()) {
        e4_assert(new_base->is_group_manager());

        return update_bases(&old_base->to_group_manager()->table(),
                           &new_base->to_group_manager()->table(),
                           range);
    }

    return ED4_R_OK;
}

// WITH_ALL_ABOVE_GROUP_MANAGER_TABLES performs a command with all groupmanager-tables
// starting at walk_up (normally the current) until top (or until one table has an ignore flag)

#define WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, COMMAND)                                   \
    do {                                                                                        \
        while (walk_up) {                                                                       \
            if (walk_up->is_abstract_group_manager()) {                                         \
                ED4_char_table& char_table = walk_up->to_abstract_group_manager()->table();     \
                char_table.COMMAND;                                                             \
                if (char_table.is_ignored()) break; /* @@@ problematic */                       \
            }                                                                                   \
            walk_up = walk_up->parent;                                                          \
        }                                                                                       \
    } while (0)

ED4_returncode ED4_manager::update_bases(const char *old_sequence, int old_len, const char *new_sequence, int new_len, PosRange range) {
    ED4_manager *walk_up = this;

    if (old_sequence) {
        if (new_sequence) {
            if (range.is_whole()) {
                const PosRange *restricted = ED4_char_table::changed_range(old_sequence, new_sequence, std::min(old_len, new_len));
                if (!restricted) return ED4_R_OK;
                
                range = *restricted;
            }

#if defined(DEBUG) && 0
            printf("update_bases(..., %i, %i)\n", start_pos, end_pos);
#endif

            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub_and_add(old_sequence, new_sequence, range));
        }
        else {
            e4_assert(range.is_whole());
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub(old_sequence, old_len));
        }
    }
    else {
        if (new_sequence) {
            e4_assert(range.is_whole());
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, add(new_sequence, new_len));
        }
        else {
            return ED4_R_OK;
        }
    }

    ED4_ROOT->root_group_man->remap()->mark_compile_needed();
    return ED4_R_OK;
}

ED4_returncode ED4_manager::update_bases(const ED4_char_table *old_table, const ED4_char_table *new_table, PosRange range) {
    ED4_manager *walk_up = this;

    if (old_table) {
        if (new_table) {
            if (range.is_whole()) {
                const PosRange *restricted = old_table->changed_range(*new_table);
                if (!restricted) return ED4_R_OK;

                range = *restricted;
            }
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub_and_add(*old_table, *new_table, range));
        }
        else {
            e4_assert(range.is_whole());
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub(*old_table));
        }
    }
    else {
        if (new_table) {
            e4_assert(range.is_whole());
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, add(*new_table));
        }
        else {
            return ED4_R_OK;
        }
    }

    ED4_ROOT->root_group_man->remap()->mark_compile_needed();
    return ED4_R_OK;
}

#undef WITH_ALL_ABOVE_GROUP_MANAGER_TABLES

ED4_returncode ED4_manager::remove_callbacks() {
    // removes callbacks
    int i;

    for (i=0; i < children->members(); i++) {
        if (children->member(i)->is_terminal()) {
            children->member(i)->to_terminal()->remove_callbacks();
        }
        else {
            children->member(i)->to_manager()->remove_callbacks();
        }
    }

    return ED4_R_OK;
}

void ED4_manager::update_consensus(ED4_manager *old_parent, ED4_manager *new_parent, ED4_base *sequence) {
    if (old_parent) old_parent->check_out_bases(sequence);
    if (new_parent) new_parent->check_in_bases(sequence);
}

ED4_terminal* ED4_manager::get_first_terminal(int start_index) const {
    ED4_terminal *terminal = 0;

    if (children) {
        int i;
        for (i=start_index; !terminal && i>=0 && i<children->members(); i++) {
            ED4_base *base = children->member(i);
            if (base->is_terminal()) {
                terminal = base->to_terminal();
            }
            else {
                terminal = base->to_manager()->get_first_terminal();
            }
        }
    }

    return terminal;
}

ED4_terminal* ED4_manager::get_last_terminal(int start_index) const {
    ED4_terminal *terminal = 0;

    if (children) {
        int i;
        if (start_index<0) start_index = children->members()-1;
        for (i=start_index; !terminal && i>=0 && i<children->members(); i--) {
            ED4_base *base = children->member(i);
            if (base->is_terminal()) {
                terminal = base->to_terminal();
            }
            else {
                terminal = base->to_manager()->get_last_terminal();
            }
        }
    }

    return terminal;
}

ED4_base* ED4_manager::get_competent_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) {
    ED4_extension  ext;
    ED4_index      temp_index;
    ED4_base      *child;

    ext.position[X_POS] = x;
    ext.position[Y_POS] = y;
    ED4_base::touch_world_cache();

    temp_index = children->search_member(&ext, spec.static_prop);

    if ((temp_index < 0) || (temp_index >= children->members())) {     // no child at given location
        return (NULL);
    }

    child = children->member(temp_index);

    return (child->spec.static_prop & relevant_prop) ? child : (ED4_base*)NULL;
}

ED4_base* ED4_manager::get_competent_clicked_child(AW_pos x, AW_pos y, ED4_properties relevant_prop) {
    ED4_extension  ext;
    ED4_base      *child;
    ED4_base      *temp_parent = NULL;

    ext.position[X_POS] = x;
    ext.position[Y_POS] = y;
    ED4_base::touch_world_cache();

    children->search_target_species(&ext, spec.static_prop, &temp_parent, ED4_L_MULTI_SPECIES);

    if (!temp_parent) {
        temp_parent = this;
    }

    child = temp_parent;
    if (child->spec.static_prop & relevant_prop) {
        return (child);
    }
    else {
        return (NULL);
    }
}

ED4_returncode  ED4_manager::handle_move(ED4_move_info *mi) {
    // handles a move request with target world
    // coordinates within current object's borders

    if ((mi == NULL) || (mi->object->spec.level <= spec.level)) {
        return (ED4_R_IMPOSSIBLE);
    }

    AW_pos rel_x = mi->end_x; // calculate target position relative to current object
    AW_pos rel_y = mi->end_y;
    calc_rel_coords(&rel_x, &rel_y);

    if ((mi->preferred_parent & spec.level) ||             // desired parent or levels match = > can handle moving
        (mi->object->spec.level & spec.allowed_children)) // object(s) myself, take info from list of selected objects
    {
        ED4_base *object;
        bool      i_am_consensus = false;

        if (mi->object->dynamic_prop & ED4_P_IS_HANDLE) { // object is a handle for an object up in the hierarchy = > search it
            ED4_level mlevel = mi->object->spec.handled_level;

            object = mi->object;

            while ((object != NULL) && !(object->spec.level & mlevel)) object = object->parent;
            if (object == NULL) return (ED4_R_IMPOSSIBLE); // no target level found
        }
        else {
            object = mi->object; // selected object is no handle => take it directly

            if (object->is_consensus_manager()) {
                if (this->is_child_of(object->parent)) return ED4_R_IMPOSSIBLE; // has to pass multi_species_manager
                i_am_consensus = true;
                if (object->parent != this) {
                    object = object->parent->parent;
                    mi->object = object;
                }
            }
        }


        ED4_manager *old_parent = object->parent;

        AW_pos x_off = 0;

        // now do move action => determine insertion offsets and insert object

        if (ED4_ROOT->selected_objects->size()>1) {
            ED4_selected_elem *list_elem = ED4_ROOT->selected_objects->head();
            while (list_elem != NULL) {
                ED4_selection_entry *sel_info   = list_elem->elem();
                ED4_terminal        *sel_object = sel_info->object;

                if (sel_object==mi->object) break;
                if (spec.static_prop & ED4_P_VERTICAL) x_off += sel_info->actual_width;

                list_elem = list_elem->next();
            }
        }

        {
            ED4_extension loc;
            loc.position[Y_POS] = mi->end_y;
            loc.position[X_POS] = mi->end_x;
            ED4_base::touch_world_cache();

            ED4_base *found_member = NULL;
            {
                ED4_manager *dummy_mark = ED4_ROOT->main_manager->search_spec_child_rek(ED4_L_DEVICE)->to_manager();
                dummy_mark->children->search_target_species(&loc, ED4_P_HORIZONTAL, &found_member, ED4_L_NO_LEVEL);
            }

            if (found_member==object) { // if we are dropped on ourself => don't move
                return ED4_R_BREAK;
            }
        }

        {
            ED4_base *parent_man = object->get_parent(ED4_L_MULTI_SPECIES);
            object->parent->children->remove_member(object);
            parent_man->to_multi_species_manager()->invalidate_species_counters();
        }

        object->extension.position[X_POS] = rel_x + x_off;
        object->extension.position[Y_POS] = rel_y;
        ED4_base::touch_world_cache();

        object->parent = this;

        if (old_parent != this) { // check whether consensus has to be calculated new or not
            GB_push_transaction(GLOBAL_gb_main);
            update_consensus(old_parent, this, object);
            rebuild_consensi(old_parent, ED4_U_UP);
            rebuild_consensi(this, ED4_U_UP);
            GB_pop_transaction(GLOBAL_gb_main);
        }

        if ((i_am_consensus && object->parent != old_parent) || !i_am_consensus) {
            object->set_width();

            if (dynamic_prop & ED4_P_IS_FOLDED) { // add spacer and consensusheight
                object->extension.position[Y_POS] = children->member(0)->extension.size[HEIGHT] + children->member(1)->extension.size[HEIGHT];
                object->flag.hidden = 1;
                ED4_base::touch_world_cache();
            }
        }

        children->insert_member(object);
        if (is_multi_species_manager()) {
            to_multi_species_manager()->invalidate_species_counters();
        }
        else {
            get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->invalidate_species_counters();
        }

        return (ED4_R_OK);
    }
    else {
        // levels do not match = > ask competent manager child to handle move request
        ED4_manager *manager = (ED4_manager *) get_competent_child(rel_x, rel_y, ED4_P_IS_MANAGER);
        if (!manager) return ED4_R_IMPOSSIBLE; // no manager child covering target location = > move not possible
        return manager->move_requested_by_parent(mi); // there is a manager child covering target location = > pass on move request
    }
}


ED4_returncode  ED4_manager::move_requested_by_parent(ED4_move_info *mi) {
    // handles a move request with target world coordinates coming from parent
    if ((mi == NULL) || !(in_border(mi->end_x, mi->end_y, mi->mode)))
        return (ED4_R_IMPOSSIBLE);

    return (handle_move(mi));
}


ED4_returncode  ED4_manager::move_requested_by_child(ED4_move_info *mi) {
    // handles a move request coming from a child,
    // target location can be out of borders
    ED4_base *temp_parent = NULL;

    if (mi == NULL)
        return (ED4_R_IMPOSSIBLE);

    if (spec.level < mi->object->spec.restriction_level) return (ED4_R_IMPOSSIBLE); // check if there is a level restriction to the move request

    if (mi->object->dynamic_prop & ED4_P_IS_HANDLE) { // determine first if we could be the moving object
        if ((dynamic_prop & ED4_P_MOVABLE) && (spec.level & mi->object->spec.handled_level)) { // yes, we are meant to be the moving object
            mi->object = this;
        }

        if (parent == NULL) return (ED4_R_WARNING);

        return (parent->move_requested_by_child(mi));
    }

    if (!(in_border(mi->end_x, mi->end_y, mi->mode))) {
        // determine if target location is within
        // the borders of current object, do boundary
        // adjustment of target coordinates if necessary
        // target location is not within borders =>
        // ask parent, i.e. move recursively up
        if (parent == NULL) return (ED4_R_WARNING);
        return (parent->move_requested_by_child(mi));
    }
    else { // target location within current borders = > handle move myself
        temp_parent = get_competent_clicked_child(mi->end_x, mi->end_y, ED4_P_IS_MANAGER);

        if (!temp_parent) {
            return (handle_move(mi));
        }
        else {
            if ((temp_parent->is_group_manager()) || (temp_parent->is_area_manager())) {
                temp_parent = temp_parent->to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES);
            }
            else if (!(temp_parent->is_multi_species_manager())) {
                temp_parent = temp_parent->get_parent(ED4_L_MULTI_SPECIES);
            }
            if (!temp_parent) {
                return ED4_R_IMPOSSIBLE;
            }
            return (temp_parent->handle_move(mi));
        }
    }
}

ED4_returncode  ED4_manager::event_sent_by_parent(AW_event *event, AW_window *aww) {
    // handles an input event coming from parent
    ED4_extension  ext;
    ED4_index      temp_index;
    ED4_returncode returncode;

    if (flag.hidden)
        return ED4_R_BREAK;

    ext.position[X_POS] = event->x;
    ext.position[Y_POS] = event->y;

    calc_rel_coords(&ext.position[X_POS], &ext.position[Y_POS]);

    temp_index = children->search_member(&ext, spec.static_prop); // search child who is competent for the location of given event

    if ((temp_index >= children->members()) || (temp_index < 0)) {
        return (ED4_R_IMPOSSIBLE); // no suitable member found
    }

    returncode = children->member(temp_index)->event_sent_by_parent(event, aww);
    return (returncode);
}

ED4_returncode  ED4_manager::refresh_requested_by_child() {
    // handles a refresh-request from a child
    if (!update_info.refresh) { // determine if there were more refresh requests already => no need to tell parent about it
        update_info.set_refresh(1); // this is the first refresh request
        if (parent) parent->refresh_requested_by_child(); // if we have a parent, tell him about the refresh request
    }

#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif
    return ED4_R_OK;
}

int ED4_manager::refresh_flag_ok() {
    ED4_index i;

    for (i=0; i<children->members(); i++) {
        ED4_base *child = children->member(i);

        if (child->is_manager()) {
            if (!child->to_manager()->refresh_flag_ok()) {
                return 0;
            }
        }

        if (child->update_info.refresh==1 && update_info.refresh==0) {
            printf("Forgotten refresh-flag in '%s' (son of '%s')\n", child->id, id);
            fflush(stdout);
            return 0;
        }
    }

    return 1;
}

inline void ED4_base::resize_requested_by_link(ED4_base *IF_ASSERTION_USED(link)) {
    e4_assert((width_link == link) || (height_link == link)); // wrong link
    if (calc_bounding_box()) request_resize();
}

void ED4_base::request_resize_of_linked() {
    if (linked_objects) {
        ED4_base_list_elem *current_list_elem = linked_objects->head();
        while (current_list_elem) {
            ED4_base *object = current_list_elem->elem();
            object->resize_requested_by_link(this);
            current_list_elem = current_list_elem->next();
        }
    }
}

bool ED4_manager::calc_bounding_box() {
    // calculates the smallest rectangle containing the object.
    // returns true if bounding box has changed.
    AW_pos     sum_width  = 0;
    AW_pos     sum_height = 0;
    AW_pos     max_x      = 0;
    AW_pos     max_y      = 0;
    AW_pos     dummy      = 0;
    ED4_index  i          = 0;
    bool       bb_changed = false;
    ED4_base  *child;

    // initialize with first child
    while ((child = children->member(i++)) != NULL) { // check all children
        if (!child->flag.hidden) {
            sum_width  += child->extension.size[WIDTH];
            sum_height += child->extension.size[HEIGHT];

            dummy = child->extension.position[X_POS] + child->extension.size[WIDTH];
            if (dummy > max_x) {
                max_x = dummy;
            }

            dummy = child->extension.position[Y_POS] + child->extension.size[HEIGHT];
            if (dummy > max_y) {
                max_y = dummy;
            }
            ED4_base::touch_world_cache();
        }
    }


    if (spec.static_prop & ED4_P_HORIZONTAL) {
        if (int(extension.size[WIDTH]) != int(max_x)) { // because compares between floats fail sometimes (AW_pos==float)
            extension.size[WIDTH] = max_x;
            bb_changed = true;
        }

        if (int(extension.size[HEIGHT]) != int(sum_height)) {
            extension.size[HEIGHT] = sum_height;
            bb_changed = true;
        }
    }

    if (spec.static_prop & ED4_P_VERTICAL) {
        if (int(extension.size[WIDTH]) != int(sum_width)) {
            extension.size[WIDTH] = sum_width;
            bb_changed = true;
        }
        if (int(extension.size[HEIGHT]) != int(max_y)) {
            extension.size[HEIGHT] = max_y;
            bb_changed = true;
        }
    }

    if (bb_changed) request_resize_of_linked();
    return bb_changed;
}

ED4_returncode ED4_manager::distribute_children() {
    // distributes all children of current object according to current object's properties and
    // justification value; a recalculation of current object's extension will take place if necessary

    ED4_index  current_index;
    ED4_index  rel_pos        = 0;
    ED4_index  rel_size       = 0;
    ED4_index  other_pos      = 0;
    ED4_index  other_size     = 0;
    AW_pos     max_rel_size   = 0;
    AW_pos     max_other_size = 0;
    ED4_base  *current_child;

    // set extension-indexes rel_pos and rel_size according to properties
    if (spec.static_prop & ED4_P_HORIZONTAL) {
        rel_pos    = X_POS;
        other_pos  = Y_POS;
        rel_size   = WIDTH;
        other_size = HEIGHT;
    }
    if (spec.static_prop & ED4_P_VERTICAL) {
        rel_pos    = Y_POS;
        other_pos  = X_POS;
        rel_size   = HEIGHT;
        other_size = WIDTH;
    }

    current_index = 0;  // get maximal relevant and other size of children, set children's other position increasingly
    while ((current_child = children->member(current_index)) != NULL) {
        max_rel_size = std::max(int(max_rel_size), int(current_child->extension.size[rel_size]));
        if (current_child->extension.position[other_pos] != max_other_size) {
            current_child->extension.position[other_pos] = max_other_size;
            ED4_base::touch_world_cache();
        }
        max_other_size += current_child->extension.size[other_size];
        current_index++;
    }

    // set children's relevant position according to justification value
    // (0.0 means top- or left-justified, 1.0 means bottom- or right-justified)
    current_index = 0;
    while ((current_child = children->member(current_index++)) != NULL) {
        current_child->extension.position[rel_pos] = 0.0;
        ED4_base::touch_world_cache();
    }

    refresh_requested_by_child();
    return (ED4_R_OK);
}

void ED4_manager::resize_requested_children() {
    if (update_info.resize) { // object wants to resize
        update_info.set_resize(0); // first clear the resize flag (remember it could be set again from somewhere below the hierarchy)

        ED4_index i = 0;
        while (1) {
            ED4_base *child = children->member(i++);

            if (!child) break;

            child->update_info.set_resize(1);
            child->resize_requested_children();
        }

        distribute_children();
        if (calc_bounding_box()) request_resize();
    }
}
void ED4_root_group_manager::resize_requested_children() {
    if (update_info.resize) {
        if (update_remap()) ED4_ROOT->request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING);
        ED4_manager::resize_requested_children();
    }
    else {
        e4_assert(!update_remap());
    }
}

static void update_scrolled_rectangles(ED4_window *win) { win->update_scrolled_rectangle(); }
void ED4_main_manager::resize_requested_children() {
    if (update_info.resize) {
        ED4_manager::resize_requested_children();
        ED4_with_all_edit_windows(update_scrolled_rectangles);
    }
}

ED4_returncode ED4_main_manager::Show(int refresh_all, int is_cleared) {
#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    AW_device *device = current_device();

    if (!flag.hidden && (refresh_all || update_info.refresh)) {
#if defined(TRACE_REFRESH)
        fprintf(stderr, "- really paint in ED4_main_manager::Show(refresh_all=%i, is_cleared=%i)\n", refresh_all, is_cleared); fflush(stderr);
#endif
        const AW_screen_area& area_rect = device->get_area_size();

        // if update all -> clear_background

        if (update_info.clear_at_refresh && !is_cleared) {
            device->push_clip_scale();
            if (device->reduceClipBorders(area_rect.t, area_rect.b, area_rect.l, area_rect.r)) {
                clear_background();
            }
            is_cleared = 1;
            device->pop_clip_scale();
        }

        // loop through all rectangles between folding lines:

        int x1, y1, x2, y2;
        ED4_window& win = *current_ed4w();
        x1 = area_rect.l;
        for (const ED4_folding_line *flv = win.get_vertical_folding(); ; flv = flv->get_next()) {
            if (flv) {
                x2 = int(flv->get_pos()); // @@@ use AW_INT ? 
            }
            else {
                x2 = area_rect.r;
                if (x1==x2) break; // do not draw last range, if it's only 1 pixel width
            }

            y1 = area_rect.t;
            for (const ED4_folding_line *flh = win.get_horizontal_folding(); ; flh = flh->get_next()) {
                if (flh) {
                    y2 = int(flh->get_pos()); // @@@ use AW_INT ? 
                }
                else {
                    y2 = area_rect.b;
                    if (y1==y2) break; // do not draw last range, if it's only 1 pixel high
                }

                device->push_clip_scale();
                if (device->reduceClipBorders(y1, y2-1, x1, x2-1)) {
                    ED4_manager::Show(refresh_all, is_cleared);
                }
                device->pop_clip_scale();

                if (!flh) break; // break out after drawing lowest range
                y1 = y2;
            }
            if (!flv) break; // break out after drawing rightmost range

            x1 = x2;
        }

        // to avoid text clipping problems between top and middle area we redraw the top-middle-spacer :
        {
            device->push_clip_scale();
            const AW_screen_area& clip_rect = device->get_cliprect();
            device->set_top_clip_border(clip_rect.t-TERMINALHEIGHT);

            int char_width = ED4_ROOT->font_group.get_max_width();
            device->set_left_clip_border(clip_rect.l-char_width);
            device->set_right_clip_border(clip_rect.r+char_width);

            get_top_middle_spacer_terminal()->Show(true, false);
            get_top_middle_line_terminal()->Show(true, false);
            device->pop_clip_scale();
        }

        // always draw cursor
        ED4_cursor& cursor = current_cursor();
        if (cursor.owner_of_cursor && cursor.allowed_to_draw) {
            if (cursor.is_partly_visible()) {
                cursor.ShowCursor(0, ED4_C_NONE, 0);
            }
        }
    }
#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    return ED4_R_OK;
}


ED4_returncode ED4_root_group_manager::Show(int refresh_all, int is_cleared) {
    if (update_remap()) { // @@@ dont call here ? 
#if defined(TRACE_REFRESH)
        printf("map updated in ED4_root_group_manager::Show (bad?)\n");
#endif
    }
    return ED4_manager::Show(refresh_all, is_cleared);
}

ED4_returncode ED4_manager::Show(int refresh_all, int is_cleared) {
#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    if (!flag.hidden && (refresh_all || update_info.refresh)) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
            is_cleared = 1;
        }

        AW_screen_area rect; // clipped rectangle in world coordinates

        {
            const AW_screen_area &clip_rect = current_device()->get_cliprect();      // clipped rectangle in win coordinates
            
            double x, y;
            x = clip_rect.l;
            y = clip_rect.t;

            current_ed4w()->win_to_world_coords(&x, &y);

            rect.l = int(x);
            rect.t = int(y);

            e4_assert(AW::nearlyEqual(current_device()->get_scale(), 1.0)); // assumed by calculation below
            rect.r = rect.l+(clip_rect.r-clip_rect.l);
            rect.b = rect.t+(clip_rect.b-clip_rect.t);
        }

        // binary search to find first visible child

        int first_visible_child = 0; //@@@FIXME: this variable is never again set

        {
            int l = 0;
            int h = children->members()-1;

            while (l<h) {

                while (children->member(l)->flag.hidden && l<h) l++;
                while (children->member(h)->flag.hidden && l<h) h--;

                int m = (l+h)/2;
                int min_m = m;
                int max_m = m+1;

                while (children->member(m)->flag.hidden) {
                    if (m==h) {
                        m = (l+h)/2-1;
                        while (children->member(m)->flag.hidden) {
                            if (m==l) {
                                // all children between l..h are flag.hidden
                                goto no_visible_child_found;
                            }
                            m--;
                        }
                        min_m = m;
                        break;
                    }
                    m++;
                    max_m = m;
                }

                ED4_base *child = children->member(m);
                e4_assert(!child->flag.hidden);

                AW_pos x, y;
                child->calc_world_coords(&x, &y);

                if (spec.static_prop & ED4_P_HORIZONTAL) { // horizontal manager
                    e4_assert((spec.static_prop&ED4_P_VERTICAL)==0);   // otherwise this binary search will not work correctly
                    if ((x+child->extension.size[WIDTH])<=rect.l) { // left of clipping range
                        l = max_m;
                    }
                    else {
                        h = min_m;
                    }
                }
                else if (spec.static_prop & ED4_P_VERTICAL) { // vertical manager
                    if ((y+child->extension.size[HEIGHT])<=rect.t) { // above clipping range
                        l = max_m;
                    }
                    else {
                        h = min_m;
                    }
                }
                else {
                    e4_assert(0);
                }
            }
        }

    no_visible_child_found :

        ED4_index i = 0;

        while (1) {
            ED4_base *child = children->member(i++);
            if (!child) break;

            if (!child->flag.hidden && (refresh_all || child->update_info.refresh) && i>=first_visible_child) {
                AW_pos x, y;
                child->calc_world_coords(&x, &y);

                AW_device *device = current_device();

                if (!(((y-rect.b)>0.5) ||
                      ((rect.t-(y+child->extension.size[HEIGHT]-1))>0.5) ||
                      ((x-rect.r)>0.5) ||
                      ((rect.l-(x+child->extension.size[WIDTH]-1))>0.5)
                     ))
                {
                    // they overlap -> show it
                    device->push_clip_scale();
                    if (child->adjust_clipping_rectangle()) {
                        child->Show(refresh_all, is_cleared);
                    }
                    device->pop_clip_scale();
                }
            }
        }
    }

#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    return ED4_R_OK;
}

ED4_returncode ED4_manager::clear_refresh() {
    e4_assert(update_info.refresh);

    for (int i=0; i<children->members(); i++) {
        ED4_base *child = children->member(i);

        if (child->update_info.refresh) {
            if (child->is_manager()) {
                child->to_manager()->clear_refresh();
            }
            else {
                child->update_info.set_refresh(0);
                child->update_info.set_clear_at_refresh(0);
            }
        }
    }

    update_info.set_refresh(0);
    update_info.set_clear_at_refresh(0);

    return ED4_R_OK;
}

void ED4_manager::update_requested_by_child() { // @@@ same as set_update -> DRY
    if (!update_info.update_requested) {
        if (parent) parent->update_requested_by_child();
        update_info.update_requested = 1;
    }
}
void ED4_manager::delete_requested_by_child() {
    if (!update_info.delete_requested) {
        if (parent) parent->delete_requested_by_child();
        update_info.delete_requested = 1;
    }
}
void ED4_terminal::delete_requested_children() {
    e4_assert(update_info.delete_requested);
    e4_assert(tflag.deleted);

    ED4_ROOT->announce_deletion(this);

    unlink_from_parent();
    delete this;
}

void ED4_manager::update_requested_children() {
    e4_assert(update_info.update_requested);

    for (int i=0; i<children->members(); i++) {
        ED4_base *child = children->member(i);
        if (child->update_info.update_requested) {
            child->update_requested_children();
        }
    }

    update_info.update_requested = 0;
}

void ED4_multi_species_manager::update_requested_children() {
    e4_assert(update_info.update_requested);
    ED4_manager::update_requested_children();
    update_species_counters();
    update_group_id();

    ED4_base *group_base = get_parent(ED4_L_GROUP);
    if (group_base) {
        e4_assert(group_base->is_group_manager());
        e4_assert(!group_base->is_root_group_manager());

        ED4_group_manager *group_man    = parent->to_group_manager();
        ED4_base          *bracket_base = group_man->get_defined_level(ED4_L_BRACKET);

        if (bracket_base) bracket_base->request_refresh();
    }
}

void ED4_manager::delete_requested_children() {
    e4_assert(update_info.delete_requested);

    for (int i = children->members()-1; i >= 0; --i) {
        ED4_base *child = children->member(i);
        if (child->update_info.delete_requested) {
            child->delete_requested_children();
        }
    }

    update_info.delete_requested = 0;

    if (!children->members()) {
        ED4_ROOT->announce_deletion(this);
        
        unlink_from_parent();
        delete this;
    }
}

void ED4_multi_species_manager::delete_requested_children() {
    e4_assert(update_info.delete_requested);
    invalidate_species_counters();
    ED4_manager::delete_requested_children();
}

void ED4_terminal::Delete() {
    if (!tflag.deleted) {
        tflag.deleted                = 1;
        update_info.delete_requested = 1;
        parent->delete_requested_by_child();
    }
}

void ED4_manager::Delete() {
    for (int i=0; i<children->members(); i++) {
        children->member(i)->Delete();
    }
}

void ED4_manager::request_refresh(int clear) {
    // sets refresh flag of current object and its children
    update_info.set_refresh(1);
    update_info.set_clear_at_refresh(clear);

    if (parent) parent->refresh_requested_by_child();

    ED4_index  current_index = 0;
    ED4_base  *current_child;
    while ((current_child = children->member(current_index++)) != NULL) {
        current_child->request_refresh(0); // do not trigger clear for childs
    }
}


ED4_base* ED4_manager::search_ID(const char *temp_id) {
    if (strcmp(temp_id, id) == 0) return this; // this object is the sought one

    ED4_index  current_index = 0;
    ED4_base  *current_child;
    while ((current_child = children->member(current_index))) { // search whole memberlist recursively for object with the given id
        ED4_base *object = current_child->search_ID(temp_id);
        if (object) return object;
        current_index++;
    }

    return NULL; // no object found
}


ED4_manager::ED4_manager(const ED4_objspec& spec_, const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_base(spec_, temp_id, x, y, width, height, temp_parent)
{
    children = new ED4_members(this);
}

ED4_manager::~ED4_manager() {
    ED4_base *current_child;

    while (children->members() > 0) {
        current_child = children->member(0);
        children->remove_member(current_child);
        current_child->parent = NULL;

        if (current_child->is_terminal())       delete current_child->to_terminal();
        else if (current_child->is_manager())   delete current_child->to_manager();
    }

    delete children;
}

// --------------------------
//      ED4_main_manager

ED4_main_manager::ED4_main_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(main_manager_spec, temp_id, x, y, width, height, temp_parent),
    top_middle_line(NULL), 
    top_middle_spacer(NULL)
{
}

// ----------------------------
//      ED4_device_manager

ED4_device_manager::ED4_device_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(device_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

// --------------------------
//      ED4_area_manager

ED4_area_manager::ED4_area_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(area_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

// -----------------------------------
//      ED4_multi_species_manager

ED4_multi_species_manager::ED4_multi_species_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(multi_species_manager_spec, temp_id, x, y, width, height, temp_parent),
    species(-1),
    selected_species(-1)
{
}

int ED4_multi_species_manager::get_no_of_species() {
    if (!has_valid_counters()) {
        update_species_counters();
        e4_assert(has_valid_counters());
    }
    return species;
}

int ED4_multi_species_manager::get_no_of_selected_species() {
    if (!has_valid_counters()) {
        update_species_counters();
        e4_assert(has_valid_counters());
    }
    return selected_species;
}

void ED4_multi_species_manager::invalidate_species_counters() {
    if (has_valid_counters()) {
        species          = -1;
        selected_species = -1;

        ED4_base *pms = get_parent(ED4_L_MULTI_SPECIES);
        if (pms) pms->to_multi_species_manager()->invalidate_species_counters();
        
        update_requested_by_child();
    }
}

void ED4_multi_species_manager::set_species_counters(int no_of_species, int no_of_selected) {

#if defined(DEBUG)
    int sp, sel;

    count_species(&sp, &sel);
    e4_assert(no_of_species==sp);
    e4_assert(no_of_selected==sel);
#endif

    e4_assert(no_of_species>=no_of_selected);

    if (species!=no_of_species || selected_species!=no_of_selected) {
        int species_diff  = no_of_species-species;
        int selected_diff = no_of_selected-selected_species;

        int quickSet = 1;
        if (species==-1 || selected_species==-1) {
            quickSet = 0;
        }

        species          = no_of_species;
        selected_species = no_of_selected;

        ED4_base *gm = get_parent(ED4_L_GROUP);
        if (gm) gm->to_manager()->search_spec_child_rek(ED4_L_BRACKET)->request_refresh();

        ED4_base *ms = get_parent(ED4_L_MULTI_SPECIES);
        if (ms) {
            ED4_multi_species_manager *parent_multi_species_man = ms->to_multi_species_manager();

            if (!quickSet) parent_multi_species_man->invalidate_species_counters();

            if (parent_multi_species_man->has_valid_counters()) {
                parent_multi_species_man->set_species_counters(parent_multi_species_man->species+species_diff,
                                                               parent_multi_species_man->selected_species+selected_diff);
            }
        }
    }
}

#ifdef DEBUG
void ED4_multi_species_manager::count_species(int *speciesPtr, int *selectedPtr) const {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();

            if (!multi_species_man->has_valid_counters()) {
                int sp1, sel1;

                multi_species_man->count_species(&sp1, &sel1);
                sel += sel1;
                sp += sp1;
            }
            else {
                sel += multi_species_man->get_no_of_selected_species();
                sp += multi_species_man->get_no_of_species();
            }
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();
            if (!species_man->is_consensus_manager()) {
                sp++;
                if (species_man->is_selected()) sel++;
            }
        }
    }

    *speciesPtr = sp;
    *selectedPtr = sel;
}
#endif

void ED4_multi_species_manager::update_species_counters() {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();

            if (!multi_species_man->has_valid_counters()) {
                multi_species_man->update_species_counters();
            }
            sel += multi_species_man->get_no_of_selected_species();
            sp += multi_species_man->get_no_of_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;
                if (species_man->is_selected()) sel++;
            }
        }
    }
    set_species_counters(sp, sel);
}

void ED4_multi_species_manager::select_all(bool only_species) {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();
            multi_species_man->select_all(only_species);
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;
                if (!species_man->is_selected()) {
                    if (!only_species || !species_man->is_SAI_manager()) {
                        ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
                        ED4_ROOT->add_to_selected(species_name);
                    }
                }
                if (species_man->is_selected()) sel++;
            }
        }
    }
    set_species_counters(sp, sel);
}
void ED4_multi_species_manager::deselect_all_species_and_SAI() {
    int m;
    int sp = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();
            multi_species_man->deselect_all_species_and_SAI();
            sp += multi_species_man->get_no_of_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;
                if (species_man->is_selected()) {
                    ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
                    ED4_ROOT->remove_from_selected(species_name);
                }
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }
    set_species_counters(sp, 0);
}
void ED4_multi_species_manager::invert_selection_of_all_species() {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();
            multi_species_man->invert_selection_of_all_species();
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;

                if (!species_man->is_SAI_manager()) {
                    ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                    if (species_man->is_selected()) ED4_ROOT->remove_from_selected(species_name);
                    else                            ED4_ROOT->add_to_selected(species_name);
                }
                if (species_man->is_selected()) sel++;
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }

    e4_assert(get_no_of_selected_species()==sel);
    e4_assert(get_no_of_species()==sp);
}
void ED4_multi_species_manager::marked_species_select(bool select) {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();
            multi_species_man->marked_species_select(select);
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;

                if (species_man->is_species_seq_manager()) {
                    GBDATA *gbd = species_man->get_species_pointer();
                    e4_assert(gbd);
                    int is_marked = GB_read_flag(gbd);

                    if (is_marked) {
                        if (select) { // select marked
                            if (!species_man->is_selected()) {
                                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
                                ED4_ROOT->add_to_selected(species_name);
                            }
                        }
                        else { // de-select marked
                            if (species_man->is_selected()) {
                                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
                                ED4_ROOT->remove_from_selected(species_name);
                            }
                        }
                    }
                }
                if (species_man->is_selected()) sel++;
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }
    set_species_counters(sp, sel);
}
void ED4_multi_species_manager::selected_species_mark(bool mark) {
    int m;
    int sp  = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_group_manager()->get_multi_species_manager();
            multi_species_man->selected_species_mark(mark);
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->is_consensus_manager()) {
                sp++;
                if (species_man->is_selected()) {
                    if (species_man->is_species_seq_manager()) {
                        GBDATA *gbd = species_man->get_species_pointer();
                        e4_assert(gbd);

                        GB_write_flag(gbd, mark ? 1 : 0);
                    }
                    sel++;
                }
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }
    set_species_counters(sp, sel);
}

ED4_species_manager *ED4_multi_species_manager::get_consensus_manager() const {
    ED4_species_manager *consensus_manager = 0;

    for (int i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (member->is_consensus_manager()) {
            consensus_manager = member->to_species_manager();
            break;
        }
    }

    return consensus_manager;
}

// -----------------------------
//      ED4_species_manager

ED4_species_manager::ED4_species_manager(ED4_species_type type_, const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(species_manager_spec, temp_id, x, y, width, height, temp_parent),
    type(type_),
    selected(false)
{
    e4_assert(type != ED4_SP_NONE);
    if (type == ED4_SP_SAI) ED4_ROOT->loadable_SAIs_may_have_changed();
}

#if defined(DEBUG)
// #define DEBUG_SPMAN_CALLBACKS
#endif // DEBUG


ED4_species_manager::~ED4_species_manager() {
    if (type == ED4_SP_SAI) ED4_ROOT->loadable_SAIs_may_have_changed();

#if defined(DEBUG_SPMAN_CALLBACKS)
    if (!callbacks.empty()) {
        printf("this=%p - non-empty callbacks\n", (char*)this);
    }
#endif // DEBUG

    e4_assert(callbacks.empty());
    // if assertion fails, callbacks are still bound to this manager.
    // You need to remove all callbacks at two places:
    // 1. ED4_species_manager::remove_all_callbacks
    // 2. ED4_exit()
}

void ED4_species_manager::add_sequence_changed_cb(ED4_species_manager_cb cb, AW_CL cd) {
#if defined(DEBUG_SPMAN_CALLBACKS)
    printf("this=%p - add_sequence_changed_cb\n", (char*)this);
#endif // DEBUG
    callbacks.push_back(ED4_species_manager_cb_data(cb, cd));
}

void ED4_species_manager::remove_sequence_changed_cb(ED4_species_manager_cb cb, AW_CL cd) {
    e4_assert(this);
#if defined(DEBUG_SPMAN_CALLBACKS)
    printf("this=%p - remove_sequence_changed_cb\n", (char*)this);
#endif // DEBUG
    callbacks.remove(ED4_species_manager_cb_data(cb, cd));
}

void ED4_species_manager::do_callbacks() {
    for (std::list<ED4_species_manager_cb_data>::iterator cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        cb->call(this);
    }
}

void ED4_species_manager::remove_all_callbacks() {
    if (!callbacks.empty()) {
        for (ED4_window *ew = ED4_ROOT->first_window; ew; ew = ew->next) {
            ED4_cursor&  cursor                  = ew->cursor;
            ED4_base    *cursors_species_manager = cursor.owner_of_cursor->get_parent(ED4_L_SPECIES);
            if (cursors_species_manager == this) {
                cursor.prepare_shutdown(); // removes any callbacks
            }
        }
        e4_assert(callbacks.empty());
    }
}

static ARB_ERROR removeAllCallbacks(ED4_base *base) {
    if (base->is_species_manager()) {
        base->to_species_manager()->remove_all_callbacks();
    }
    return NULL;
}

void ED4_root::remove_all_callbacks() {
    root_group_man->route_down_hierarchy(removeAllCallbacks).expect_no_error();
}

// ------------------------
//      group managers

ED4_abstract_group_manager::ED4_abstract_group_manager(const ED4_objspec& spec_, const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_manager(spec_, temp_id, x, y, width, height, temp_parent), 
    my_table(0)
{
}

ED4_group_manager::ED4_group_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_abstract_group_manager(group_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

// -------------------
//      ED4_remap

ED4_remap::ED4_remap() {
    mode               = ED4_RM_NONE;
    show_above_percent = 0;

    sequence_table_len = 1;
    screen_table_len   = 1;

    screen_to_sequence_tab = new int[1];        screen_to_sequence_tab[0] = 0;
    sequence_to_screen_tab = new int[1];        sequence_to_screen_tab[0] = 0;

    sequence_len               = 0;
    MAXSEQUENCECHARACTERLENGTH = screen_len = 0;

    changed       = 0;
    update_needed = 1;

}
ED4_remap::~ED4_remap() {
    delete [] screen_to_sequence_tab;
    delete [] sequence_to_screen_tab;
}
int ED4_remap::screen_to_sequence(int screen_pos) const {
    if (size_t(screen_pos) == screen_len) {
        return screen_to_sequence_tab[screen_len-1];
    }
    e4_assert(screen_pos>=0 && size_t(screen_pos)<screen_len);
    return screen_to_sequence_tab[screen_pos];
}
int ED4_remap::clipped_sequence_to_screen_PLAIN(int sequence_pos) const {
    if (sequence_pos<0) {
        sequence_pos = 0;
    }
    else if (size_t(sequence_pos)>sequence_len) {
        sequence_pos = sequence_len;
    }
    return sequence_to_screen_PLAIN(sequence_pos);
}
int ED4_remap::sequence_to_screen(int sequence_pos) const {
    int scr_pos = sequence_to_screen_PLAIN(sequence_pos);
    if (scr_pos<0) scr_pos = -scr_pos;
    return scr_pos;
}
inline void ED4_remap::set_sequence_to_screen(int pos, int newVal) {
    e4_assert(pos>=0 && size_t(pos)<sequence_table_len);
    if (sequence_to_screen_tab[pos]!=newVal) {
        sequence_to_screen_tab[pos] = newVal;
        changed = 1;
    }
}
void ED4_remap::mark_compile_needed_force() {
    if (!update_needed) {
        update_needed = 1;
        if (ED4_ROOT && ED4_ROOT->root_group_man) {                     // test if root_group_man already exists
            ED4_ROOT->root_group_man->resize_requested_by_child();      // remapping is recompiled while re-displaying the root_group_manager
        }
    }
}
void ED4_remap::mark_compile_needed() {
    if (mode!=ED4_RM_NONE) mark_compile_needed_force();
}

GB_ERROR ED4_remap::compile(ED4_root_group_manager *gm)
{
    e4_assert(update_needed);

    const ED4_char_table&  table = gm->table();
    size_t                 i, j;

    changed = 0; // is changed by set_sequence_to_screen
    update_needed = 0;

    sequence_len = table.table('A').size(); // take size of any table
    if ((sequence_len+1) > sequence_table_len) {
        delete [] sequence_to_screen_tab;
        sequence_to_screen_tab = new int[sequence_table_len = sequence_len+1];
        memset(sequence_to_screen_tab, 0, sequence_table_len*sizeof(int));
        changed = 1;
    }

    int above_percent;
    switch (gm->remap()->get_mode()) {
        default: e4_assert(0);
        case ED4_RM_NONE: {
        dont_map :
            for (i=0; i<sequence_table_len; i++) {
                set_sequence_to_screen(i, i);
            }
            screen_len = sequence_len;
            break;
        }
        case ED4_RM_SHOW_ABOVE: {
            above_percent = show_above_percent;
            goto calc_percent;
        }
        case ED4_RM_MAX_ALIGN:
        case ED4_RM_MAX_EDIT: {
            above_percent = 0;
        calc_percent :
            for (i=0, j=0; i<(sequence_table_len-1); i++) {
                int bases;
                int gaps;

                table.bases_and_gaps_at(i, &bases, &gaps);

                if (bases==0 && gaps==0) {  // special case (should occur only after inserting columns)
                    set_sequence_to_screen(i, -j); // hide
                }
                else {
                    int percent = (int)((bases*100L)/table.added_sequences());

                    e4_assert(percent==((bases*100)/(bases+gaps)));

                    if (bases && percent>=above_percent) {
                        set_sequence_to_screen(i, j++);
                    }
                    else {
                        set_sequence_to_screen(i, -j);
                    }
                }
            }
            for (; i<sequence_table_len; i++) { // fill rest of table
                set_sequence_to_screen(i, j++);
            }
            screen_len = j;
            break;
        }
        case ED4_RM_DYNAMIC_GAPS: {
            for (i=0, j=0; i<(sequence_table_len-1); i++) {
                int bases;

                table.bases_and_gaps_at(i, &bases, 0);
                if (bases) {
                    set_sequence_to_screen(i, j++);
                }
                else {
                    size_t k = i+1;

                    while (k<(sequence_table_len-1)) {
                        int bases2;

                        table.bases_and_gaps_at(k, &bases2, 0);
                        if (bases2) {
                            break;
                        }
                        k++;
                    }

                    int gaps = k-i;
                    int shown_gapsize;

                    if (gaps<100) {
                        shown_gapsize = gaps/10 + 1;
                    }
                    else if (gaps<1000) {
                        shown_gapsize = gaps/100 + 10;
                    }
                    else {
                        shown_gapsize = gaps/1000 + 19;
                    }

                    for (; i<k && shown_gapsize; i++, shown_gapsize--) {
                        set_sequence_to_screen(i, j++);
                    }
                    for (; i<k; i++) {
                        set_sequence_to_screen(i, -j);
                    }
                    i--;
                }
            }
            for (; i<sequence_table_len; i++) {
                set_sequence_to_screen(i, j++); // fill rest of table
            }
            screen_len = j;
            break;
        }
    }

    if (sequence_table_len) {
        if (!screen_len && sequence_len) {
            goto dont_map;
        }
        if ((screen_len+1) > screen_table_len) {
            delete [] screen_to_sequence_tab;
            screen_to_sequence_tab = new int[screen_table_len = screen_len+1];
        }
        memset(screen_to_sequence_tab, 0, sizeof(int)*screen_table_len);
        for (i=0; i<sequence_table_len; i++) {
            int screen_pos = sequence_to_screen_tab[i];
            if (screen_pos>=0) {
                screen_to_sequence_tab[screen_pos] = i;
            }
        }
    }

    if (sequence_len>1) {
        MAXSEQUENCECHARACTERLENGTH = sequence_len;
    }

    return NULL;
}

// --------------------------------
//      ED4_root_group_manager

ED4_root_group_manager::ED4_root_group_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_abstract_group_manager(root_group_manager_spec, temp_id, x, y, width, height, temp_parent),
      my_remap()
{
    AW_root *awr = ED4_ROOT->aw_root;
    my_remap.set_mode((ED4_remap_mode)awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int(),
                      awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT)->read_int());
    my_remap.mark_compile_needed_force();
}

bool ED4_root_group_manager::update_remap() {
    bool remapped = false;

    if (my_remap.compile_needed()) {
        my_remap.compile(this);
        if (my_remap.was_changed()) remapped = true;
    }

    return remapped;
}

// -----------------------------------
//      ED4_multi_species_manager

ED4_multi_sequence_manager::ED4_multi_sequence_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_manager(multi_sequence_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_sequence_manager::ED4_sequence_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_manager(sequence_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_multi_name_manager::ED4_multi_name_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_manager(multi_name_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_name_manager::ED4_name_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_manager(name_manager_spec, temp_id, x, y, width, height, temp_parent)
{
}


