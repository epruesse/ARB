#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <math.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_ProteinViewer.hxx"

//#define TEST_REFRESH_FLAG

//*******************************************
//* Manager static properties 	  beginning *
//* are used by manager-constructors	    *
//*******************************************

// Each manager should either be ED4_P_HORIZONTAL or ED4_P_VERTICAL - never both !!!

ED4_object_specification main_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_HORIZONTAL ), // static props
	ED4_L_ROOT,                                               // level
	ED4_L_GROUP,                                              // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means left-aligned, 1.0 means right-aligned)
};

ED4_object_specification device_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_HORIZONTAL ), // static props
	ED4_L_DEVICE,                                             // level
	(ED4_level) ( ED4_L_AREA | ED4_L_SPACER ),  	          // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means left-aligned)
};

ED4_object_specification area_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),   // static props
	ED4_L_AREA,                                               // level
	(ED4_level) ( ED4_L_MULTI_SPECIES | ED4_L_TREE |
                  ED4_L_SPACER),	  // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means top-aligned)
};

ED4_object_specification multi_species_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_HORIZONTAL ), // static props
	ED4_L_MULTI_SPECIES,                                      // level
	(ED4_level) (ED4_L_SPECIES | ED4_L_GROUP | ED4_L_SPACER), // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means left-aligned)
};

ED4_object_specification species_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),    // static props
	ED4_L_SPECIES,                                             // level
	(ED4_level) ( ED4_L_MULTI_SEQUENCE | ED4_L_MULTI_NAME ),  // allowed children level
	ED4_L_NO_LEVEL,                                            // handled object
	ED4_L_NO_LEVEL,                                            // restriction level
	0                                                          // justification value (0 means top-aligned)
};
ED4_object_specification multi_sequence_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_HORIZONTAL ), // static props
	ED4_L_MULTI_SEQUENCE,                                     // level
	ED4_L_SEQUENCE,                                           // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means left-aligned)
};

ED4_object_specification sequence_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),	  // static props
	ED4_L_SEQUENCE,                                           // level
	(ED4_level) ( ED4_L_SEQUENCE_INFO |
                  ED4_L_SEQUENCE_STRING | ED4_L_AA_SEQUENCE_STRING),			  // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_SPECIES,                                            // restriction level
	0                                                         // justification value (0 means top-aligned)
};

ED4_object_specification multi_name_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_HORIZONTAL ), // static props
	ED4_L_MULTI_NAME,					  // level
	ED4_L_NAME_MANAGER,                                       // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_NO_LEVEL,                                           // restriction level
	0                                                         // justification value (0 means left-aligned)
};

ED4_object_specification name_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),	  // static props
	ED4_L_NAME_MANAGER,                                       // level
	(ED4_level) ( ED4_L_SPECIES_NAME),			  // allowed children level
	ED4_L_NO_LEVEL,                                           // handled object
	ED4_L_SPECIES,                                            // restriction level
	0                                                         // justification value (0 means top-aligned)
};

ED4_object_specification group_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),    // static props
	ED4_L_GROUP,                                               // level
	(ED4_level) ( ED4_L_MULTI_SPECIES |
                  ED4_L_BRACKET |
                  ED4_L_DEVICE ),				   // allowed children level
	ED4_L_NO_LEVEL,                                            // handled object
	ED4_L_NO_LEVEL,                                            // restriction level
	0                                                          // justification value (0 means top-aligned)
};

ED4_object_specification root_group_manager_spec =
{
	(ED4_properties) ( ED4_P_IS_MANAGER | ED4_P_VERTICAL ),    // static props
	ED4_L_GROUP,                                               // level
	(ED4_level) ( ED4_L_DEVICE ),				   // allowed children level
	ED4_L_NO_LEVEL,                                            // handled object
	ED4_L_NO_LEVEL,                                            // restriction level
	0                                                          // justification value (0 means top-aligned)
};

//*******************************************
//* Manager static properties 		end *
//*******************************************


//*****************************************
//* ED4_manager Methods		beginning *
//*****************************************

ED4_returncode ED4_manager::rebuild_consensi( ED4_base *start_species, ED4_update_flag update_flag)
{
    int	i;
    ED4_base *temp_parent;
    ED4_multi_species_manager	*multi_species_manager 	= NULL;
    ED4_group_manager		*first_group_manager 	= NULL;

    temp_parent = start_species;

    switch (update_flag) {
        case ED4_U_UP:		// rebuild consensus from a certain starting point upwards
            while (temp_parent && !(temp_parent->is_area_manager())) {
                if (temp_parent->is_group_manager()) {
                    ED4_group_manager *group_manager = temp_parent->to_group_manager();
                    multi_species_manager = group_manager->get_defined_level( ED4_L_MULTI_SPECIES )->to_multi_species_manager();
                    for (i=0; i<multi_species_manager->children->members(); i++) {
                        if (multi_species_manager->children->member(i)->flag.is_consensus) {
                            rebuild_consensus( NULL, NULL, multi_species_manager->children->member(i));
                        }
                    }
                }
                temp_parent = temp_parent->parent;
            }
            break;
        case ED4_U_UP_DOWN:	// only search first groupmanager and update consensi recursively downwards
            while (temp_parent && !(temp_parent->is_area_manager())) {
                if (temp_parent->is_group_manager()){
                    first_group_manager = temp_parent->to_group_manager();
                }
                temp_parent = temp_parent->parent;
            }
            if (first_group_manager)
                first_group_manager->route_down_hierarchy( NULL, NULL, rebuild_consensus );
            break;
    }
    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_in_bases(ED4_base *added_base, int start_pos, int end_pos)
{
    if (added_base->is_species_manager()) { // add a sequence
        ED4_species_manager *species_manager = added_base->to_species_manager();
        ED4_terminal *sequence_terminal = species_manager->get_consensus_relevant_terminal();

        if (sequence_terminal) {
            int   seq_len;
            char *seq          = sequence_terminal->resolve_pointer_to_string_copy(&seq_len);
            ED4_returncode res = check_bases(0, 0, seq, seq_len, start_pos, end_pos);
            free(seq);
            return res;
        }
    }

    if (added_base->is_group_manager()) { // add a group
        return check_bases(0, &added_base->to_group_manager()->table(), start_pos, end_pos);
    }

    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_out_bases(ED4_base *subbed_base, int start_pos, int end_pos)
{
    if (subbed_base->is_species_manager()) { // sub a sequence
        ED4_species_manager *species_manager = subbed_base->to_species_manager();
        ED4_terminal *sequence_terminal = species_manager->get_consensus_relevant_terminal();

        if (sequence_terminal) {
            int   seq_len;
            char *seq          = sequence_terminal->resolve_pointer_to_string_copy(&seq_len);
            ED4_returncode res = check_bases(seq, seq_len, 0, 0, start_pos, end_pos);
            free(seq);
            return res;
        }
    }

    if (subbed_base->is_group_manager()) { // sub a group
        return check_bases(&subbed_base->to_group_manager()->table(), 0, start_pos, end_pos);
    }

    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_bases(const char *old_sequence, int old_len, const ED4_base *new_base, int start_pos, int end_pos)
{
    if (!new_base) {
        return check_bases(old_sequence, old_len, 0, 0, start_pos, end_pos);
    }

    e4_assert(new_base->is_species_manager());

    ED4_species_manager *new_species_manager = new_base->to_species_manager();
    ED4_terminal *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();

    int   new_len;
    char *new_sequence = new_sequence_terminal->resolve_pointer_to_string_copy(&new_len);

    if (end_pos==-1) {
        ED4_char_table::changed_range(old_sequence, new_sequence, min(old_len, new_len), &start_pos, &end_pos);
    }

    ED4_returncode res = check_bases(old_sequence, old_len, new_sequence, new_len, start_pos, end_pos);
    free(new_sequence);
    return res;
}

ED4_returncode ED4_manager::check_bases_and_rebuild_consensi(const char *old_sequence, int old_len, ED4_base *new_base, ED4_update_flag update_flag,
                                                             int start_pos, int end_pos)
{
    e4_assert(new_base);
    e4_assert(new_base->is_species_manager());

    ED4_species_manager *new_species_manager = new_base->to_species_manager();
    ED4_terminal *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();

    int   new_len;
    char *new_sequence = new_sequence_terminal->resolve_pointer_to_string_copy(&new_len);

#if defined(DEBUG)
    printf("old: %s\n", old_sequence);
    printf("new: %s\n", new_sequence);
#endif // DEBUG

    if (end_pos==-1) {
        ED4_char_table::changed_range(old_sequence, new_sequence, min(old_len, new_len), &start_pos, &end_pos);
    }

    ED4_returncode result1 = check_bases(old_sequence, old_len, new_sequence, new_len, start_pos, end_pos);
    ED4_returncode result2 = rebuild_consensi(new_base, update_flag);
    ED4_returncode result  = (result1 != ED4_R_OK) ? result1 : result2;

    free(new_sequence);

    // Refresh aminoacid  sequence terminals in Protein Viewer
    if(ED4_ROOT->alignment_type == GB_AT_DNA) {
        PV_AA_SequenceUpdate_CB(GB_CB_CHANGED);
    }

    return result;
}

ED4_returncode ED4_manager::check_bases(const ED4_base *old_base, const ED4_base *new_base, int start_pos, int end_pos)
{
    e4_assert(old_base);
    e4_assert(new_base);

    if (old_base->is_species_manager()) {
        e4_assert(new_base->is_species_manager());
        ED4_species_manager *old_species_manager   = old_base->to_species_manager();
        ED4_species_manager *new_species_manager   = new_base->to_species_manager();
        ED4_terminal        *old_sequence_terminal = old_species_manager->get_consensus_relevant_terminal();
        ED4_terminal        *new_sequence_terminal = new_species_manager->get_consensus_relevant_terminal();
        int                  old_len;
        int                  new_len;
        char                *old_seq               = old_sequence_terminal->resolve_pointer_to_string_copy(&old_len);
        char                *new_seq               = new_sequence_terminal->resolve_pointer_to_string_copy(&new_len);

        ED4_returncode res = check_bases(old_seq, old_len, new_seq, new_len, start_pos, end_pos);
        free(new_seq);
        free(old_seq);
        return res;
    }

    if (old_base->is_group_manager()) {
        e4_assert(new_base->is_group_manager());

        return check_bases(&old_base->to_group_manager()->table(),
                           &new_base->to_group_manager()->table(),
                           start_pos, end_pos);
    }

    return ED4_R_OK;
}

// WITH_ALL_ABOVE_GROUP_MANAGER_TABLES performs a command with all groupmanager-tables
// starting at walk_up (normally the current) until top (or until one table has an ignore flag)

#define WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, COMMAND)                   \
do {                                                                            \
    while (walk_up) {                                                           \
        if (walk_up->is_group_manager()) {                                      \
            ED4_char_table& char_table = walk_up->to_group_manager()->table();  \
            char_table.COMMAND;                                                 \
            if (char_table.is_ignored()) break;                                 \
        }                                                                       \
        walk_up = walk_up->parent;                                              \
    }                                                                           \
} while(0)

ED4_returncode ED4_manager::check_bases( const char *old_sequence, int old_len, const char *new_sequence, int new_len, int start_pos, int end_pos)
{
    ED4_manager *walk_up = this;

#ifdef DEBUG
    if (end_pos==-1) {
        printf("check_bases does not know update range - maybe an error?\n");
    }
#endif

    if (old_sequence) {
        if (new_sequence) {
            if (end_pos==-1) {
                if (!ED4_char_table::changed_range(old_sequence, new_sequence, min(old_len, new_len), &start_pos, &end_pos)) {
                    return ED4_R_OK;
                }
            }

#if defined(DEBUG) && 0
            printf("check_bases(..., %i, %i)\n", start_pos, end_pos);
#endif

            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub_and_add(old_sequence, new_sequence, start_pos, end_pos));
        } else {
            e4_assert(start_pos==0 && end_pos==-1);
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub(old_sequence, old_len));
        }
    } else {
        if (new_sequence) {
            e4_assert(start_pos==0 && end_pos==-1);
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, add(new_sequence, new_len));
        } else {
            return ED4_R_OK;
        }
    }

    ED4_ROOT->root_group_man->remap()->mark_compile_needed();
    return ED4_R_OK;
}

ED4_returncode ED4_manager::check_bases(const ED4_char_table *old_table, const ED4_char_table *new_table, int start_pos, int end_pos)
{
    ED4_manager *walk_up = this;

    if (old_table) {
        if (new_table) {
            if (end_pos==-1) {
                if (!old_table->changed_range(*new_table, &start_pos, &end_pos)) {
                    return ED4_R_OK;
                }
            }
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub_and_add(*old_table, *new_table, start_pos, end_pos));
        } else {
            e4_assert(start_pos==0 && end_pos==-1);
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, sub(*old_table));
        }
    } else {
        if (new_table) {
            e4_assert(start_pos==0 && end_pos==-1);
            WITH_ALL_ABOVE_GROUP_MANAGER_TABLES(walk_up, add(*new_table));
        } else {
            return ED4_R_OK;
        }
    }

    ED4_ROOT->root_group_man->remap()->mark_compile_needed();
    return ED4_R_OK;
}

#undef WITH_ALL_ABOVE_GROUP_MANAGER_TABLES

ED4_returncode ED4_manager::remove_callbacks()						//removes callbacks
{
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

ED4_returncode ED4_manager::update_consensus(ED4_manager *old_parent, ED4_manager *new_parent, ED4_base *sequence, int start_pos, int end_pos)
{
    if (old_parent) old_parent->check_out_bases(sequence, start_pos, end_pos);
    if (new_parent) new_parent->check_in_bases(sequence, start_pos, end_pos);

    return ED4_R_OK;
}

ED4_terminal* ED4_manager::get_first_terminal(int start_index) const
{
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

ED4_terminal* ED4_manager::get_last_terminal(int start_index) const
{
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

ED4_base* ED4_manager::get_competent_child( AW_pos x, AW_pos y, ED4_properties relevant_prop )
{
    ED4_extension  	ext;
    ED4_index      	temp_index;
    ED4_base      	*child;

    ext.position[X_POS] = x;
    ext.position[Y_POS] = y;
    ED4_base::touch_world_cache();

    temp_index = children->search_member( &ext, spec->static_prop );

    if ( (temp_index < 0) || (temp_index >= children->members())) {	// no child at given location
        return ( NULL );
    }

    child = children->member(temp_index);

    return (child->spec->static_prop & relevant_prop) ? child : (ED4_base*)NULL;
}

ED4_base* ED4_manager::get_competent_clicked_child( AW_pos x, AW_pos y, ED4_properties relevant_prop)
{
    ED4_extension  	ext;
    ED4_base      	*child;
	ED4_base	*temp_parent = NULL;

    ext.position[X_POS] = x;
	ext.position[Y_POS] = y;
	ED4_base::touch_world_cache();

    children->search_target_species( &ext, spec->static_prop, &temp_parent, ED4_L_MULTI_SPECIES );

	if (!temp_parent) {
	    temp_parent = this;
	}

	child = temp_parent;
    if ( child->spec->static_prop & relevant_prop ) {
	    return (child);
	}
    else {
	    return (NULL);
	}
}

// #if 0
// ED4_returncode ED4_manager::show_scrolled(ED4_properties scroll_prop, int /*only_text*/)
// {
//     ED4_base      	*current_child;
//     ED4_index      	current_index;
//     AW_pos	       	x, y;
//     ED4_AREA_LEVEL	ar_lv;

//     if (!flag.hidden) {
// 	calc_world_coords( &x, &y );

// 	if ((long) y >= ED4_ROOT->temp_ed4w->coords.window_lower_clip_point)			// we are beyond the visible area
// 	    return ED4_R_BREAK;								// =>don't draw our terminals

// 	ar_lv = get_area_level(0);							// we are over the visible area => don't draw
// 	if ((long) y + extension.size[HEIGHT] < ED4_ROOT->temp_ed4w->coords.window_upper_clip_point &&
// 	    ar_lv == ED4_A_MIDDLE_AREA)
// 	    return ED4_R_BREAK;

// 	if (((update_info.refresh_horizontal_scrolling) && (scroll_prop | ED4_P_SCROLL_HORIZONTAL)) ||
// 	    ((update_info.refresh_vertical_scrolling)   && (scroll_prop | ED4_P_SCROLL_VERTICAL)) ) {
// 	    current_index = 0;
// 	    while ( (current_child = children->member_list[current_index++]) != NULL ) {
// 		if (current_child->is_terminal() && !(current_child->is_bracket_terminal())) { // don't show selected object only if all visible
// 		    if (((ED4_terminal *) current_child)->selection_info && (ar_lv == ED4_A_MIDDLE_AREA)) {
// 			if (current_child->is_visible( 0, y, ED4_D_VERTICAL )) {
// 			    current_child->show_scrolled(scroll_prop, 1 );
// 			}
// 		    }
// 		    else {
// 			current_child->show_scrolled(scroll_prop);
// 		    }
// 		}
// 		else {
// 		    current_child->show_scrolled(scroll_prop);
// 		}
// 	    }
// 	}
//     }
//     return ( ED4_R_OK );
// }
// #endif


ED4_returncode	ED4_manager::handle_move( ED4_move_info *mi )					// handles a move request with target world
{												// coordinates within current object's borders
    AW_pos	          	rel_x, rel_y, x_off, y_off;
    ED4_base             	*object;
    ED4_manager 		*old_parent;
    ED4_manager          	*manager;
    ED4_level            	 mlevel;
    ED4_terminal         	*sel_object;
    ED4_selection_entry  	*sel_info;
    ED4_list_elem        	*list_elem;
    bool			i_am_consensus = 0;
    ED4_AREA_LEVEL		level;
    int 			nr_of_visible_species = 0,
        nr_of_children_in_group = 0;

    ED4_base		*found_member = NULL;
    ED4_extension		loc;

    if ((mi == NULL) || (mi->object->spec->level <= spec->level)) {
        return ( ED4_R_IMPOSSIBLE );
    }

    rel_x = mi->end_x;									// calculate target position relative to current object
    rel_y = mi->end_y;

    calc_rel_coords( &rel_x, &rel_y );

    if ((mi->preferred_parent & spec->level) ||					// desired parent or levels match => can handle moving
        (mi->object->spec->level & spec->allowed_children ))			// object(s) myself, take info from list of selected objects
    {

        if ( mi->object->dynamic_prop & ED4_P_IS_HANDLE )				// object is a handle for an object up in the hierarchy => search it
        {
            mlevel = mi->object->spec->handled_level;
            object = mi->object;

            while ( (object != NULL) && !(object->spec->level & mlevel) )
                object = object->parent;

            if ( object == NULL )
                return ( ED4_R_IMPOSSIBLE );  					// no target level found

        }
        else
        {
            object = mi->object;							// selected object is no handle => take it directly

            if (object->flag.is_consensus)
            {
                if (this->is_child_of(object->parent))			// has to pass multi_species_manager
                    return ED4_R_IMPOSSIBLE;

                i_am_consensus = 1;

                if (object->parent != this)
                {
                    object = object->parent->parent;
                    mi->object = object;
                }
            }
        }


        old_parent = object->parent;
        ED4_multi_species_manager *multi_species_manager	= NULL;

        level = get_area_level(&multi_species_manager);

        if (old_parent->get_area_level() != level) { // when moving between two different areas we have to
            if (level == ED4_A_TOP_AREA || level == ED4_A_BOTTOM_AREA) { // check restrictions
                nr_of_visible_species = multi_species_manager->count_visible_children();

                if (nr_of_visible_species >= 10) {
                    return ED4_R_IMPOSSIBLE;
                }

                if (object->is_group_manager()) {
                    ED4_group_manager *group_manager = object->to_group_manager();

                    if (object->dynamic_prop & ED4_P_IS_FOLDED) {
                        nr_of_children_in_group = 1;
                    }
                    else {
                        nr_of_children_in_group = group_manager->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->count_visible_children();
                    }

                    if (nr_of_children_in_group + nr_of_visible_species > 10) {
                        return ED4_R_IMPOSSIBLE;
                    }
                }
            }
        }

        x_off = 0;
        y_off = 0;

        // now do move action => determine insertion offsets and insert object

        if (ED4_ROOT->selected_objects.no_of_entries()>1) {
            list_elem = ED4_ROOT->selected_objects.first();
            while ( list_elem != NULL ) {
                sel_info = (ED4_selection_entry *) list_elem->elem();
                sel_object = sel_info->object;

                if ((sel_object==mi->object)) break;
                if (spec->static_prop & ED4_P_HORIZONTAL) y_off += sel_info->actual_height;
                if (spec->static_prop & ED4_P_VERTICAL) x_off += sel_info->actual_width;

                list_elem = list_elem->next();
            }
        }

        loc.position[Y_POS] = mi->end_y;
        loc.position[X_POS] = mi->end_x;
        ED4_base::touch_world_cache();

        {
            ED4_manager *dummy_mark = ED4_ROOT->main_manager->search_spec_child_rek(ED4_L_DEVICE)->to_manager();
            dummy_mark->children->search_target_species( &loc, ED4_P_HORIZONTAL, &found_member, ED4_L_NO_LEVEL);
        }

        if (found_member==object) { // if we are dropped on ourself => don't move
            return ED4_R_BREAK;
        }

		ED4_base *parent_man = object->get_parent(ED4_L_MULTI_SPECIES);
        object->parent->children->delete_member( object );
        parent_man->to_multi_species_manager()->invalidate_species_counters();

        object->extension.position[X_POS] = rel_x + x_off;
        object->extension.position[Y_POS] = rel_y; // was: rel_y + y_off;
        ED4_base::touch_world_cache();

        object->parent = this;

        if (old_parent != this) { // check whether consensus has to be calculated new or not
            GB_push_transaction( gb_main );
            update_consensus( old_parent, this, object );
            rebuild_consensi( old_parent, ED4_U_UP );
            rebuild_consensi( this, ED4_U_UP );
            GB_pop_transaction( gb_main );
        }

        if ((i_am_consensus && object->parent != old_parent) || !i_am_consensus) {
            object->set_width();

            if (dynamic_prop & ED4_P_IS_FOLDED) { // add spacer and consensusheight
                object->extension.position[Y_POS] = children->member(0)->extension.size[HEIGHT] + children->member(1)->extension.size[HEIGHT];
                object->flag.hidden = 1;
                ED4_base::touch_world_cache();
            }
        }

        children->insert_member( object );
        if (is_multi_species_manager()) {
            to_multi_species_manager()->invalidate_species_counters();
        }
        else {
            get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->invalidate_species_counters();
        }

        return ( ED4_R_OK );
    }
    else											// levels do not match => ask competent manager child to handle move request
    {
        if ( (manager = (ED4_manager *) get_competent_child( rel_x, rel_y, ED4_P_IS_MANAGER )) == NULL )	// no manager child covering target
            return ( ED4_R_IMPOSSIBLE );										// location => move not possible

        return ( manager->move_requested_by_parent( mi ) );					// there is a manager child covering target location => pass on move request
    }
}


ED4_returncode	ED4_manager::move_requested_by_parent( ED4_move_info *mi )		// handles a move request with target world coordinates coming from parent
{
    if ( (mi == NULL) || !(in_border( mi->end_x, mi->end_y, mi->mode )) )
		return ( ED4_R_IMPOSSIBLE );

	return ( handle_move( mi ) );
}

// #if 0
// ED4_returncode ED4_manager::set_scroll_refresh( AW_pos world_x,
// 						AW_pos world_y,
// 						AW_pos width,
// 						AW_pos height,
// 						ED4_properties scroll_prop )		// sets refresh flag of current object and
// {											// his children if object is (even partly) within given rectangle
//     ED4_index      current_index;
//     ED4_base      *current_child;
//     AW_pos         x, y;

//     calc_world_coords( &x, &y );

//     if (((x + extension.size[WIDTH]) <= world_x) || ((width != INFINITE) && (x >= (world_x + width))) ) // check if x is out of rectangle
// 	return ( ED4_R_OK );


//     if (((y + extension.size[HEIGHT]) <= world_y) || ((height != INFINITE) && (y >= (world_y + height)))) // check if y is out of rectangle
// 	return ( ED4_R_OK );


//     if (scroll_prop & ED4_P_SCROLL_HORIZONTAL) // at this this point we have to be within the rectangle => ask our children
// 	update_info.refresh_horizontal_scrolling = 1;

//     if (scroll_prop & ED4_P_SCROLL_VERTICAL)
// 	update_info.refresh_vertical_scrolling = 1;

//     current_index = 0;
//     while ((current_child = children->member_list[current_index++]) != NULL )
// 	current_child->set_scroll_refresh( world_x, world_y, width, height, scroll_prop );

//     return ( ED4_R_OK );
// }
// #endif


ED4_returncode 	ED4_manager::move_requested_by_child( ED4_move_info *mi)				// handles a move request coming from a child,
{													// target location can be out of borders
    ED4_base	*temp_parent = NULL;

    if ( mi == NULL )
        return ( ED4_R_IMPOSSIBLE );

    if ( spec->level < mi->object->spec->restriction_level )					// check if there is a level restriction to the move request
        return ( ED4_R_IMPOSSIBLE );

    if ( mi->object->dynamic_prop & ED4_P_IS_HANDLE )						// determine first if we could be the moving object
    {
        if ( (dynamic_prop & ED4_P_MOVABLE) && (spec->level & mi->object->spec->handled_level) )	// yes, we are meant to be the moving object
            mi->object = this;

        if ( parent == NULL )
            return ( ED4_R_WARNING );

        return ( parent->move_requested_by_child( mi ) );
    }



    if ( !(in_border( mi->end_x, mi->end_y, mi->mode )) )				    	// determine if target location is within
        // the borders of current object, do boundary
        // adjustment of target coordinates if necessary
        // target location is not within borders =>
    {												// ask parent, i.e. move recursively up
        if ( parent == NULL )
            return ( ED4_R_WARNING );

        return ( parent->move_requested_by_child( mi ) );
    }
    else												// target location within current borders => handle move myself
    {
        temp_parent = get_competent_clicked_child(mi->end_x, mi->end_y, ED4_P_IS_MANAGER);

        if (!temp_parent)
        {
            return ( handle_move( mi ) );
        }
        else
        {
            if ((temp_parent->is_group_manager()) || (temp_parent->is_area_manager())) {
                temp_parent = temp_parent->to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES);
            }
            else if (!(temp_parent->is_multi_species_manager())) {
                temp_parent = temp_parent->get_parent( ED4_L_MULTI_SPECIES );
            }
            if (!temp_parent) {
                return ED4_R_IMPOSSIBLE;
            }
            return ( temp_parent->handle_move(mi));
        }
    }
}

ED4_returncode	ED4_manager::event_sent_by_parent( AW_event *event, AW_window *aww)			// handles an input event coming from parent
{
	ED4_extension  ext;
	ED4_index      temp_index;
	ED4_returncode returncode;

	if (flag.hidden)
		return ED4_R_BREAK;

	ext.position[X_POS] = event->x;
	ext.position[Y_POS] = event->y;

	calc_rel_coords( &ext.position[X_POS], &ext.position[Y_POS] );

	temp_index = children->search_member( &ext, spec->static_prop );		// search child who is competent for the location of given event

	if ( (temp_index >= children->members()) || (temp_index < 0) ) {
	    return ( ED4_R_IMPOSSIBLE ); // no suitable member found
	}

	returncode = children->member(temp_index)->event_sent_by_parent( event, aww );
	return ( returncode );
}

ED4_returncode	ED4_manager::calc_size_requested_by_parent( void )
{
    if (calc_bounding_box()) {
        if (parent) {
            parent->resize_requested_by_child();
        }
    }

    return ( ED4_R_OK );
}

ED4_returncode  ED4_manager::refresh_requested_by_child(void) // handles a refresh-request from a child
{
    if (!update_info.refresh) { // determine if there were more refresh requests already => no need to tell parent about it
        update_info.set_refresh(1); // this is the first refresh request
        if (parent) parent->refresh_requested_by_child(); // if we have a parent, tell him about the refresh request
    }

#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif
    return ED4_R_OK;
}

int ED4_manager::refresh_flag_ok()
{
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
            return 0;
        }
    }

    return 1;
}

short ED4_manager::calc_bounding_box( void )					//calculates the smallest rectangle containing the object and
{										//gives information if something has changed
    AW_pos		sum_width = 0,
        sum_height= 0,
        max_x	  = 0,
        max_y	  = 0,
        dummy	  = 0;
    ED4_index       i	  = 0;
    short           bb_changed= 0;
    ED4_list_elem  *current_list_elem;
    ED4_base       *child, *object;

    //initialize with first child
    while ((child = children->member(i++)) != NULL ) { // check all children
        if (!child->flag.hidden) {
            sum_width  += child->extension.size[WIDTH];
            sum_height += child->extension.size[HEIGHT];

            dummy = child->extension.position[X_POS] + child->extension.size[WIDTH];
            if ( dummy > max_x ) {
                max_x = dummy;
            }

            dummy = child->extension.position[Y_POS] + child->extension.size[HEIGHT];
            if ( dummy > max_y ) {
                max_y = dummy;
            }
            ED4_base::touch_world_cache();
        }
    }


    if ( spec->static_prop & ED4_P_HORIZONTAL ) {
        if ( int(extension.size[WIDTH]) != int(max_x) ) { // because compares between floats fail sometimes (AW_pos==float)
            extension.size[WIDTH] = max_x;
            bb_changed = 1;
        }

        if ( int(extension.size[HEIGHT]) != int(sum_height) ) {
            extension.size[HEIGHT] = sum_height;
            bb_changed = 1;
        }
    }

    if ( spec->static_prop & ED4_P_VERTICAL ) {
        if ( int(extension.size[WIDTH]) != int(sum_width) ) {
            extension.size[WIDTH] = sum_width;
            bb_changed = 1;
        }
        if ( int(extension.size[HEIGHT]) != int(max_y) ) {
            extension.size[HEIGHT] = max_y;
            bb_changed = 1;
        }
    }

    if ( bb_changed ) { // tell linked objects about our change of bounding box

        current_list_elem = linked_objects.first();
        while ( current_list_elem ) {
            object = ( ED4_base *) current_list_elem->elem();
            if (object) {
                object->link_changed( this );
            }

            current_list_elem = current_list_elem->next();
        }
    }
    return ( bb_changed );
}

ED4_returncode ED4_manager::distribute_children( void ) // distributes all children of current object according to current object's properties and
{							// justification value; a recalculation of current object's extension will take place if necessary
    ED4_index	current_index,
        rel_pos = 0,
        rel_size = 0,
        other_pos = 0,
        other_size = 0;
    AW_pos          max_rel_size = 0,
        max_other_size = 0;
    ED4_base	*current_child;

    //    printf("distribute_children this=%p\n", this);

    if ( spec->static_prop & ED4_P_HORIZONTAL )		// set extension-indexes rel_pos and rel_size according to properties
    {
        rel_pos 	= X_POS;
        other_pos 	= Y_POS;
        rel_size 	= WIDTH;
        other_size 	= HEIGHT;
    }

    if ( spec->static_prop & ED4_P_VERTICAL )
    {
        rel_pos = Y_POS;
        other_pos = X_POS;
        rel_size = HEIGHT;
        other_size = WIDTH;
    }

    current_index = 0;	// get maximal relevant and other size of children, set children's other position increasingly
    while ( (current_child = children->member(current_index)) != NULL ) {
        max_rel_size = max( int(max_rel_size), int(current_child->extension.size[rel_size]));
        if ( current_child->extension.position[other_pos] != max_other_size ) {
            current_child->extension.position[other_pos] = max_other_size;
            ED4_base::touch_world_cache();
        }
        max_other_size += current_child->extension.size[other_size];
        current_index++;
    }

    current_index = 0;						// set children's relevant position according to justification value
    // (0.0 means top- or left-justified, 1.0 means bottom- or right-justified)
    while ( (current_child = children->member(current_index++)) != NULL ) {
        current_child->extension.position[rel_pos] = spec->justification * (max_rel_size - current_child->extension.size[rel_size]);
        ED4_base::touch_world_cache();
    }

    refresh_requested_by_child();
    return ( ED4_R_OK );
}

ED4_returncode ED4_manager::resize_requested_by_parent( void )
{
    if (update_info.resize ) { // object wants to resize
        update_info.set_resize(0); // first clear the resize flag (remember it could be set again from somewhere below the hierarchy)

        ED4_index i = 0;
        while (1) {
            ED4_base *child = children->member(i++);

            if (!child) break;

            child->update_info.set_resize(1);
            child->resize_requested_by_parent();
        }

        distribute_children();
        if (calc_bounding_box()) {			// recalculate current object's bounding_box
            resize_requested_by_child();
        }
    }

    return ED4_R_OK;
}
ED4_returncode ED4_root_group_manager::resize_requested_by_parent( void )
{
    ED4_returncode result = ED4_R_OK;

    if (update_info.resize) {
        update_remap();
        result = ED4_manager::resize_requested_by_parent();
    }

    return result;
}

ED4_returncode ED4_main_manager::resize_requested_by_parent() {
    ED4_returncode result = ED4_R_OK;

    if (update_info.resize) {
        result = ED4_manager::resize_requested_by_parent();
        ED4_ROOT->temp_ed4w->update_scrolled_rectangle();
    }
    return ED4_R_OK;
}

ED4_returncode ED4_manager::Resize()
{
    if (!flag.hidden) {
        while (update_info.resize) {
            resize_requested_by_parent();
            if (parent) break;
        }
    }

    return ED4_R_OK;
}
ED4_returncode ED4_terminal::Resize()
{
    return ED4_R_OK;
}

ED4_returncode ED4_main_manager::Show(int refresh_all, int is_cleared)
{
#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif
#if defined(DEBUG) && 0
    printf("Show main_manager\n");
#endif

    AW_device *device = ED4_ROOT->temp_device;

    if (flag.hidden) {
        if (last_window_reached && update_info.refresh) {
            clear_refresh();
        }
    }
    else if (refresh_all || update_info.refresh) {
        AW_rectangle area_rect;
        device->get_area_size(&area_rect);

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

        int x1,y1,x2,y2;
        ED4_folding_line *flv,*flh;
        ED4_window& win = *ED4_ROOT->temp_ed4w;
        int old_last_window_reached = last_window_reached;

        last_window_reached = 0;
        x1 = area_rect.l;
        for (flv = win.vertical_fl; ; flv = flv->next) {
            int lastColumn = 0;

            if (flv) {
                e4_assert(flv->length==INFINITE);
                x2 = int(flv->window_pos[X_POS]);
                if (!flv->next && x2==area_rect.r) {
                    lastColumn = 1;
                }
            }
            else {
                x2 = area_rect.r;
                lastColumn = 1;
                if (x1==x2) {
                    break; // do not draw last range, if it's only 1 pixel width
                }
            }

            y1 = area_rect.t;
            for (flh = win.horizontal_fl; ; flh = flh->next) {
                int lastRow = 0;

                if (flh) {
                    e4_assert(flh->length==INFINITE);
                    y2 = int(flh->window_pos[Y_POS]);
                    if (!flh->next && y2==area_rect.b) {
                        lastRow = 1;
                    }
                }
                else {
                    y2 = area_rect.b;
                    lastRow = 1;
                    if (y1==y2) {
                        break; // do not draw last range, if it's only 1 pixel high
                    }
                }

                if (lastRow && lastColumn) {
                    last_window_reached = old_last_window_reached;
                }
                
                device->push_clip_scale();
                if (device->reduceClipBorders(y1, y2-1, x1, x2-1)) {
                    ED4_manager::Show(refresh_all, is_cleared);
                }
                device->pop_clip_scale();
                
                if (last_window_reached) {
                    update_info.set_refresh(0);
                    update_info.set_clear_at_refresh(0);
                }

                if (!flh) break; // break out after drawing lowest range
                y1 = y2;
            }
            if (!flv) break; // break out after drawing rightmost range

            x1 = x2;
        }
    }

    // to avoid text clipping problems between top and middle area we redraw the top-middle-spacer :
    {
        device->push_clip_scale();
        const AW_rectangle& clip_rect = device->clip_rect;
        device->set_top_clip_border(clip_rect.t-TERMINALHEIGHT);
        
        int char_width = ED4_ROOT->font_group.get_max_width();
        device->set_left_clip_border(clip_rect.l-char_width);
        device->set_right_clip_border(clip_rect.r+char_width);

        get_top_middle_spacer_terminal()->Show(true, false);
        get_top_middle_line_terminal()->Show(true, false);
        device->pop_clip_scale();
    }

#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    return ED4_R_OK;
}


ED4_returncode ED4_root_group_manager::Show(int refresh_all, int is_cleared)
{
    if (update_remap())  {
#if defined(DEBUG)
        printf("map updated\n");
#endif // DEBUG
    }
    return ED4_manager::Show(refresh_all, is_cleared);
}

ED4_returncode ED4_manager::Show(int refresh_all, int is_cleared)
{
#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    if (flag.hidden) {
        if (last_window_reached && update_info.refresh) {
            clear_refresh();
        }
    }
    if (!flag.hidden && (refresh_all || update_info.refresh)) {
        ED4_index i = 0;
        const AW_rectangle &clip_rect = ED4_ROOT->temp_device->clip_rect;	// clipped rectangle in win coordinates
        AW_rectangle rect; // clipped rectangle in world coordinates

        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
            is_cleared = 1;
        }

        {
            double x, y;
            x = clip_rect.l;
            y = clip_rect.t;

            ED4_ROOT->win_to_world_coords(ED4_ROOT->temp_aww, &x, &y);

            rect.l = int(x);
            rect.t = int(y);
            rect.r = rect.l+(clip_rect.r-clip_rect.l);
            rect.b = rect.t+(clip_rect.b-clip_rect.t);
        }

        //	draw_bb(ED4_G_STANDARD); // debugging only!

        // binary search to find first visible child

        int first_visible_child = 0;

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
                                // all childs between l..h are flag.hidden
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

                AW_pos x,y;
                child->calc_world_coords(&x, &y);

                if (spec->static_prop & ED4_P_HORIZONTAL) { // horizontal manager
                    e4_assert((spec->static_prop&ED4_P_VERTICAL)==0);	// otherwise this binary search will not work correctly
                    if ((x+child->extension.size[WIDTH])<=rect.l) { // left of clipping range
                        l = max_m;
                    }
                    else {
                        h = min_m;
                    }
                }
                else if (spec->static_prop & ED4_P_VERTICAL) { // vertical manager
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

    no_visible_child_found:

        i = 0;

        while (1) {
            ED4_base *child = children->member(i++);
            if (!child) break;

            int flags_cleared = 0;

            if (!child->flag.hidden && (refresh_all || child->update_info.refresh) && i>=first_visible_child) {
                AW_pos x,y;
                child->calc_world_coords(&x, &y);

                AW_device *device = ED4_ROOT->temp_device;

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
                    flags_cleared = 1;
                    device->pop_clip_scale();
                }
            }

            if (last_window_reached) {
                if (!flags_cleared && child->is_manager() && child->update_info.refresh) {	// if we didn't show a manager we must clear its childs refresh flags
                    child->to_manager()->clear_refresh();
                }
                else {
                    child->update_info.set_refresh(0);
                    child->update_info.set_clear_at_refresh(0);
                }
            }
        }
    }

    //    if (last_window_reached) update_info.refresh = 0;

#ifdef TEST_REFRESH_FLAG
    e4_assert(refresh_flag_ok());
#endif

    return ED4_R_OK;
}

ED4_returncode ED4_manager::clear_refresh()
{
    e4_assert(update_info.refresh);

    int i;

    for (i=0; i<children->members(); i++) {
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

    return ED4_R_OK;
}

ED4_returncode ED4_manager::resize_requested_by_child(void) // handles a resize-request from a child
{
    if (!update_info.resize) { 	// this is the first resize request
        if (parent) { 		// if we have a parent, tell him about our resize
            parent->resize_requested_by_child();
        }
        update_info.set_resize(1);
    }
    return ED4_R_OK;
}

ED4_returncode ED4_base::delete_requested_by_child()
{
    if (!update_info.delete_requested) {
        if (parent) {
            parent->delete_requested_by_child();
        }
        update_info.delete_requested = 1;
    }
    return ED4_R_OK;
}

ED4_returncode ED4_terminal::delete_requested_by_parent()
{
    flag.deleted = 1;
    return ED4_R_OK;
}
ED4_returncode ED4_manager::delete_requested_by_parent()
{
    for (ED4_index i=0; children->member(i); i++) {
        children->member(i)->delete_requested_by_parent();
    }
    return ED4_R_OK;
}

ED4_returncode ED4_terminal::delete_requested_childs()
{
    e4_assert(update_info.delete_requested);
    e4_assert(flag.deleted);

    delete this;
    return ED4_R_WARNING;	// == remove all links to me
}
ED4_returncode ED4_manager::delete_requested_childs()
{
    e4_assert(update_info.delete_requested);

    int i;
    ED4_base *child;

    for (i=0; (child=children->member(i))!=NULL; i++) {
        if (child->update_info.delete_requested) {
            ED4_returncode removed = child->delete_requested_childs();
            if (removed==ED4_R_WARNING) {
                children->delete_member(child);
                if (i) i--;
            }
        }
    }

    update_info.delete_requested = 0;
    if (children->members()) return ED4_R_OK;

    delete this;
    return ED4_R_WARNING;
}


ED4_returncode ED4_manager::set_refresh(int clear) // sets refresh flag of current object and his children
{
    ED4_index	current_index;
    ED4_base	*current_child;

    update_info.set_refresh(1);
    update_info.set_clear_at_refresh(clear);

    current_index = 0;
    while ( (current_child = children->member(current_index++)) != NULL ) {
        current_child->set_refresh(0);
    }

    return ( ED4_R_OK );
}


ED4_base* ED4_manager::search_ID(const char *temp_id )
{
    ED4_base	*current_child, *object;
    ED4_index	current_index;

    if ( strcmp( temp_id, id ) == 0 ) { // this object is the sought one
        return this;
    }
    else {
        current_index = 0;
        while (( current_child = children->member(current_index))) { // search whole memberlist recursively for object with the given id
            if ( (object = current_child->search_ID( temp_id )) != NULL ) {
                return ( object );
            }
            current_index++;
        }
    }

    return NULL; // no object found
}


ED4_manager::ED4_manager( const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_base( temp_id, x, y, width, height, temp_parent )
{
    children = new ED4_members(this);
    is_group = temp_is_group;
}

ED4_manager::~ED4_manager()
{
    ED4_base  	*current_child;

    while (children->members() > 0) {
        current_child = children->member(0);
        children->delete_member( current_child );
        current_child->parent = NULL;

        if (current_child->is_terminal()) 	delete current_child->to_terminal();
        else if (current_child->is_manager()) 	delete current_child->to_manager();
    }

    delete children;
}


//***********************************
//* ED4_manager Methods		end *
//***********************************



// --------------------------------------------------------------------------------
//	ED4_main_manager
// --------------------------------------------------------------------------------

ED4_main_manager::ED4_main_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
	spec = &(main_manager_spec);
}

ED4_main_manager::~ED4_main_manager()
{
}

// --------------------------------------------------------------------------------
//	ED4_device_manager
// --------------------------------------------------------------------------------

ED4_device_manager::ED4_device_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(device_manager_spec);
}

ED4_device_manager::~ED4_device_manager()
{
}

// --------------------------------------------------------------------------------
//	ED4_area_manager
// --------------------------------------------------------------------------------

ED4_area_manager::ED4_area_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(area_manager_spec);
}


ED4_area_manager::~ED4_area_manager()
{
}

// --------------------------------------------------------------------------------
//	ED4_multi_species_manager
// --------------------------------------------------------------------------------

ED4_multi_species_manager::ED4_multi_species_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group ),
    species(-1),
    selected_species(-1)
{
    spec = &(multi_species_manager_spec);
}

ED4_multi_species_manager::~ED4_multi_species_manager()
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
        species = -1;
        selected_species = -1;

        set_refresh();
        parent->refresh_requested_by_child();

        if (parent->is_group_manager()) {
            ED4_group_manager *group_man = parent->to_group_manager();
            ED4_base *base = group_man->get_defined_level(ED4_L_BRACKET);

            if (base && base->is_bracket_terminal()) {
                ED4_bracket_terminal *bracket_term = base->to_bracket_terminal();

                bracket_term->set_refresh();
                bracket_term->parent->refresh_requested_by_child();
            }
        }

        ED4_base *pms = get_parent(ED4_L_MULTI_SPECIES);
        if (pms) {
            pms->to_multi_species_manager()->invalidate_species_counters();
        }
    }
}

void ED4_multi_species_manager::set_species_counters(int no_of_species, int no_of_selected) {

#if defined(DEBUG)
    int sp,sel;

    count_species(&sp, &sel);
    e4_assert(no_of_species==sp);
    e4_assert(no_of_selected==sel);
#endif

    e4_assert(no_of_species>=no_of_selected);

    if (species!=no_of_species || selected_species!=no_of_selected) {
        int species_diff = no_of_species-species;
        int selected_diff = no_of_selected-selected_species;
        int quickSet = 1;

        if (species==-1 || selected_species==-1) {
            quickSet = 0;
        }

        species = no_of_species;
        selected_species = no_of_selected;

        ED4_base *gm = get_parent(ED4_L_GROUP);
        if (gm) {
            ED4_bracket_terminal *bracket = gm->to_manager()->search_spec_child_rek(ED4_L_BRACKET)->to_bracket_terminal();
            bracket->set_refresh();
            bracket->parent->refresh_requested_by_child();
        }

        ED4_base *ms = get_parent(ED4_L_MULTI_SPECIES);
        if (ms) {
            ED4_multi_species_manager *parent_multi_species_man = ms->to_multi_species_manager();

            if (!quickSet) {
                parent_multi_species_man->invalidate_species_counters();
            }

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
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

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

            if (!species_man->flag.is_consensus) {
                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                sp++;
                if (species_name->flag.selected) {
                    sel++;
                }
            }
        }
    }

    *speciesPtr = sp;
    *selectedPtr = sel;
}
#endif

void ED4_multi_species_manager::update_species_counters() {
    int m;
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

            if (!multi_species_man->has_valid_counters()) {
                multi_species_man->update_species_counters();
            }
            sel += multi_species_man->get_no_of_selected_species();
            sp += multi_species_man->get_no_of_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                sp++;
                if (species_name->flag.selected) {
                    sel++;
                }
            }
        }
    }
    set_species_counters(sp, sel);
}

void ED4_multi_species_manager::select_all_species() {
    int m;
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_species_man->select_all_species();
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                sp++;

                if (!species_man->flag.is_SAI) {
                    ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                    sel++;
                    if (!species_name->flag.selected) {
                        ED4_ROOT->add_to_selected(species_name);
                    }
                }
            }
        }
    }
    set_species_counters(sp, sel);
}
void ED4_multi_species_manager::deselect_all_species() {
    int m;
    int sp = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_species_man->deselect_all_species();
            sp += multi_species_man->get_no_of_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                sp++;
                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                if (species_name->flag.selected) {
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
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_species_man->invert_selection_of_all_species();
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                sp++;
                if (!species_man->flag.is_SAI) {
                    ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                    if (species_name->flag.selected) {
                        ED4_ROOT->remove_from_selected(species_name);
                    }
                    else {
                        ED4_ROOT->add_to_selected(species_name);
                        sel++;
                    }
                }
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }

    e4_assert(get_no_of_selected_species()==sel);
    e4_assert(get_no_of_species()==sp);
}
void ED4_multi_species_manager::select_marked_species(int select) {
    int m;
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_species_man->select_marked_species(select);
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                sp++;

                GBDATA *gbd = species_man->get_species_pointer();
                e4_assert(gbd);
                int is_marked = GB_read_flag(gbd);

                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
                if (is_marked) {
                    if (select) { // select marked
                        if (!species_man->flag.is_SAI) {
                            if (!species_name->flag.selected) {
                                ED4_ROOT->add_to_selected(species_name);
                            }
                            sel++;
                        }
                    }
                    else { // de-select marked
                        if (species_name->flag.selected) {
                            ED4_ROOT->remove_from_selected(species_name);
                        }
                    }
                }
                else {
                    if (species_name->flag.selected) {
                        sel++;
                    }
                }
            }
        }
        else {
            e4_assert(!member->is_manager());
        }
    }
    set_species_counters(sp, sel);
}
void ED4_multi_species_manager::mark_selected_species(int mark) {
    int m;
    int sp = 0;
    int sel = 0;

    for (m=0; m<children->members(); m++) {
        ED4_base *member = children->member(m);

        if (member->is_group_manager()) {
            ED4_multi_species_manager *multi_species_man = member->to_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_species_man->mark_selected_species(mark);
            sp += multi_species_man->get_no_of_species();
            sel += multi_species_man->get_no_of_selected_species();
        }
        else if (member->is_species_manager()) {
            ED4_species_manager *species_man = member->to_species_manager();

            if (!species_man->flag.is_consensus) {
                ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

                sp++;
                if (species_name->flag.selected) {
                    GBDATA *gbd = species_man->get_species_pointer();
                    e4_assert(gbd);

#if defined(ASSERTION_USED)
                    GB_ERROR error =
#endif // ASSERTION_USED
                        GB_write_flag(gbd, mark ? 1 : 0);
                    e4_assert(!error);

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
    int i;

    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (member->flag.is_consensus) {
            consensus_manager = member->to_species_manager();
            break;
        }
    }

    return consensus_manager;
}

ED4_terminal *ED4_multi_species_manager::get_consensus_terminal() { // returns the consensus-name-terminal of a multi_species manager

    ED4_species_manager *consensus_man = get_consensus_manager();
    return consensus_man ? consensus_man->children->member(0)->to_terminal() : 0;
}

// --------------------------------------------------------------------------------
//	ED4_species_manager
// --------------------------------------------------------------------------------

ED4_species_manager::ED4_species_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(species_manager_spec);
}

#if defined(DEBUG)
// #define DEBUG_SPMAN_CALLBACKS
#endif // DEBUG


ED4_species_manager::~ED4_species_manager()
{
#if defined(DEBUG_SPMAN_CALLBACKS)
    if (!callbacks.empty()) {
        printf("this=%p - non-empty callbacks\n", (char*)this);
    }
#endif // DEBUG

    e4_assert(callbacks.empty());
    // if assertion fails, callbacks are still bound to this manager.
    // You need to remove all callbacks at two places:
    // 1. ED4_species_manager::remove_all_callbacks
    // 2. ED4_quit_editor() 
}

void ED4_species_manager::add_sequence_changed_cb(ED4_species_manager_cb cb, AW_CL cd) {
#if defined(DEBUG_SPMAN_CALLBACKS)
    printf("this=%p - add_sequence_changed_cb\n", (char*)this);
#endif // DEBUG
    callbacks.insert(ED4_species_manager_cb_data(cb, cd));
}

void ED4_species_manager::remove_sequence_changed_cb(ED4_species_manager_cb cb, AW_CL cd) {
    e4_assert(this);
#if defined(DEBUG_SPMAN_CALLBACKS)
    printf("this=%p - remove_sequence_changed_cb\n", (char*)this);
#endif // DEBUG
    callbacks.erase(ED4_species_manager_cb_data(cb, cd));
}

void ED4_species_manager::do_callbacks() {
    for (std::set<ED4_species_manager_cb_data>::iterator cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        cb->call(this);
    }
}

void ED4_species_manager::remove_all_callbacks() {
    if (!callbacks.empty()) {
        for (ED4_window *ew = ED4_ROOT->first_window; ew; ew = ew->next) {
            ED4_cursor&  cursor                  = ew->cursor;
            ED4_base    *cursors_species_manager = cursor.owner_of_cursor->get_parent(ED4_L_SPECIES);
            if (cursors_species_manager == this) {
                cursor.invalidate_base_position(); // removes the callback
            }
        }
        e4_assert(callbacks.empty());
    }
}


// --------------------------------------------------------------------------------
// 	ED4_group_manager::
// --------------------------------------------------------------------------------

ED4_group_manager::ED4_group_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group ) :
    ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group ),
    my_table(0)
{
    spec = &(group_manager_spec);
}

ED4_group_manager::~ED4_group_manager()
{
}

// --------------------------------------------------------------------------------
// 		ED4_remap::
// --------------------------------------------------------------------------------

ED4_remap::ED4_remap()
{
    mode = ED4_RM_NONE;
    show_above_percent = 0;

    sequence_table_len = 1;
    screen_table_len = 1;

    screen_to_sequence_tab = new int[1];	screen_to_sequence_tab[0] = 0;
    sequence_to_screen_tab = new int[1];	sequence_to_screen_tab[0] = 0;

    sequence_len = 0;
    MAXSEQUENCECHARACTERLENGTH = screen_len = 0;

    changed = 0;
    update_needed = 1;

}
ED4_remap::~ED4_remap()
{
    delete [] screen_to_sequence_tab;
    delete [] sequence_to_screen_tab;
}
int ED4_remap::screen_to_sequence(int screen_pos) const
{
    if (size_t(screen_pos)>=screen_len) {
        return screen_to_sequence_tab[screen_len-1];
    }
    e4_assert(screen_pos>=0 && size_t(screen_pos)<screen_len);
    return screen_to_sequence_tab[screen_pos];
}
int ED4_remap::sequence_to_screen(int sequence_pos) const
{
    e4_assert(sequence_pos>=0 && size_t(sequence_pos)<=sequence_len);
    return sequence_to_screen_tab[sequence_pos];
}
int ED4_remap::clipped_sequence_to_screen(int sequence_pos) const
{
    if (sequence_pos<0) {
        sequence_pos = 0;
    }
    else if (size_t(sequence_pos)>sequence_len) {
        sequence_pos = sequence_len;
    }

    return sequence_to_screen_tab[sequence_pos];
}
int ED4_remap::sequence_to_screen_clipped(int sequence_pos) const
{
    int scr_pos = sequence_to_screen(sequence_pos);
    if (scr_pos<0) scr_pos = -scr_pos;
    return scr_pos;
}
inline void ED4_remap::set_sequence_to_screen(int pos, int newVal)
{
    e4_assert(pos>=0 && size_t(pos)<sequence_table_len);
    if (sequence_to_screen_tab[pos]!=newVal) {
        sequence_to_screen_tab[pos] = newVal;
        changed = 1;
    }
}
void ED4_remap::mark_compile_needed()
{
    if (mode!=ED4_RM_NONE && !update_needed) {
        update_needed = 1;
        if (ED4_ROOT && ED4_ROOT->root_group_man) {  			// test if root_group_man already exists
            ED4_ROOT->root_group_man->resize_requested_by_child(); 	// remapping is recompiled while re-displaying the root_group_manager
        }
    }
}
void ED4_remap::mark_compile_needed_force()
{
    if (!update_needed) {
        update_needed = 1;
        if (ED4_ROOT && ED4_ROOT->root_group_man) {  			// test if root_group_man already exists
            ED4_ROOT->root_group_man->resize_requested_by_child(); 	// remapping is recompiled while re-displaying the root_group_manager
        }
    }
}
GB_ERROR ED4_remap::compile(ED4_root_group_manager *gm)
{
    e4_assert(update_needed);

    int                    ew;
    ED4_window            *next_win;
    const ED4_char_table&  table = gm->table();
    size_t                 i,j;

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
        dont_map:
            for (i=0; i<sequence_table_len; i++) {
                set_sequence_to_screen(i,i);
            }
            screen_len = sequence_len;
            break;
        }
        // 	case ED4_RM_SHOW_ABOVE_30: {
        // 	    above_percent = 30;
        // 	    goto calc_percent;
        // 	}
        case ED4_RM_SHOW_ABOVE: {
            above_percent = show_above_percent;
            goto calc_percent;
        }
        case ED4_RM_MAX_ALIGN:
        case ED4_RM_MAX_EDIT: {
            above_percent = 0;
        calc_percent:
            for (i=0,j=0; i<(sequence_table_len-1); i++) {
                int bases;
                int gaps;

                table.bases_and_gaps_at(i, &bases, &gaps);

                if (bases==0 && gaps==0)  { // special case (should occur only after inserting columns)
                    set_sequence_to_screen(i, -j); // hide
                }
                else {
                    int percent = (int)((bases*100L)/table.added_sequences());

                    e4_assert(percent==((bases*100)/(bases+gaps)));

                    if (bases && percent>=above_percent) {
                        set_sequence_to_screen(i,j++);
                    }else{
                        set_sequence_to_screen(i, -j);
                    }
                }
            }
            for (;i<sequence_table_len;i++) { // fill rest of table
                set_sequence_to_screen(i,j++);
            }
            screen_len = j;
            break;
        }
        case ED4_RM_DYNAMIC_GAPS: {
            for (i=0,j=0; i<(sequence_table_len-1); i++) {
                int bases;

                table.bases_and_gaps_at(i, &bases, 0);
                if (bases) {
                    set_sequence_to_screen(i,j++);
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

                    for (; i<k && shown_gapsize; i++,shown_gapsize--) {
                        set_sequence_to_screen(i,j++);
                    }
                    for (; i<k; i++) {
                        set_sequence_to_screen(i,-j);
                    }
                    i--;
                }
            }
            for (;i<sequence_table_len;i++) {
                set_sequence_to_screen(i,j++); // fill rest of table
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

// --------------------------------------------------------------------------------
// 		ED4_root_group_manager::
// --------------------------------------------------------------------------------

ED4_root_group_manager::ED4_root_group_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_group_manager(temp_id,x,y,width,height,temp_parent),
      my_remap()
{
    spec = &(root_group_manager_spec);

    AW_root *awr = ED4_ROOT->aw_root;
    my_remap.set_mode((ED4_remap_mode)awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int(),
                      awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT)->read_int());
    my_remap.mark_compile_needed_force();
}

ED4_root_group_manager::~ED4_root_group_manager()
{
}

int ED4_root_group_manager::update_remap()
{
    int remapped = 0;

    if (my_remap.compile_needed()) {
        my_remap.compile(this);
        if (my_remap.was_changed()) {
            ED4_ROOT->main_manager->set_refresh();
            ED4_ROOT->main_manager->refresh_requested_by_child();
            remapped = 1;
        }
    }

    return remapped;
}

// --------------------------------------------------------------------------------
// 		ED4_multi_species_manager::
// --------------------------------------------------------------------------------

ED4_multi_sequence_manager::ED4_multi_sequence_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group )
    : ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(multi_sequence_manager_spec);
}

ED4_multi_sequence_manager::~ED4_multi_sequence_manager()
{
}

ED4_sequence_manager::ED4_sequence_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group )
    : ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
	spec = &(sequence_manager_spec);
}

ED4_sequence_manager::~ED4_sequence_manager()
{
}


ED4_multi_name_manager::ED4_multi_name_manager(const char *temp_id,AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group)
    : ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(multi_name_manager_spec);
}

ED4_multi_name_manager::~ED4_multi_name_manager()
{
}

ED4_name_manager::ED4_name_manager(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent, bool temp_is_group )
    : ED4_manager( temp_id, x, y, width, height, temp_parent, temp_is_group )
{
    spec = &(name_manager_spec);
}

ED4_name_manager::~ED4_name_manager()
{
}

//*********************************************************
//* Manager constructors and destructors	      end *
//*********************************************************

