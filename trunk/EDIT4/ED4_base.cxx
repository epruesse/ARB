#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
// #include <assert.h>

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

ED4_group_manager *ED4_base::is_in_folded_group() const
{
    if (!parent) return 0;
    ED4_base *group = get_parent(ED4_L_GROUP);
    if (!group) return 0;
    if (group->dynamic_prop & ED4_P_IS_FOLDED) return group->to_group_manager();
    return group->is_in_folded_group();
}

bool ED4_base::remove_deleted_childs()
{
    e4_assert(0);
    return false;
}

bool ED4_terminal::remove_deleted_childs()
{
    if (flag.deleted) {
        if (get_species_pointer() != 0) {
#if defined(DEBUG)
            printf("ED4_terminal: has non-zero species_pointer in remove_deleted_childs (resetting to zero)\n");
#endif // DEBUG
            set_species_pointer(0);
        }
        ED4_ROOT->announce_deletion(this);
        parent->children->delete_member(this);
        return true;
    }

    return false;
}
bool ED4_sequence_info_terminal::remove_deleted_childs()
{
    if (flag.deleted) {
        if (get_species_pointer() != 0) {
#if defined(DEBUG)
            printf("ED4_sequence_info_terminal: has non-zero species_pointer in remove_deleted_childs (resetting to zero)\n");
#endif // DEBUG
            set_species_pointer(0);
        }
        ED4_ROOT->announce_deletion(this);
        parent->children->delete_member(this);
        return true;
    }

    return false;
}

bool ED4_manager::remove_deleted_childs()
{
    int  i;
    bool deletion_occurred = false;

 restart:

    for (i=0; i<children->members(); i++) {
        ED4_base *child = children->member(i);
        if (child->remove_deleted_childs()) {
            deletion_occurred = true;
            goto restart;       // because order of childs may have changed
        }
    }

    if (!children->members()) {
        if (parent) {
            ED4_ROOT->announce_deletion(this);
            parent->children->delete_member(this);
            return true;
        }
#if defined(DEBUG)
        printf("ED4_manager::remove_deleted_childs: I have no parent\n");
#endif // DEBUG
    }
    else if (deletion_occurred) {
        parent->refresh_requested_by_child();
    }

    return false;
}

void ED4_base::changed_by_database(void)
{
    e4_assert(0);
    // this happens if you add a new species_pointer to a ED4_base-derived type
    // without defining changed_by_database for this type
}

void ED4_manager::changed_by_database(void) {
    remove_deleted_childs();
    set_refresh(1);
    if (parent) {
        parent->refresh_requested_by_child();
    }
    else {
#if defined(DEBUG)
        printf("ED4_manager::changed_by_database: I have no parent!\n");
#endif // DEBUG
    }
    ED4_ROOT->main_manager->Show();
}

void ED4_terminal::changed_by_database(void)
{
    if (GB_read_clock(gb_main) > actual_timestamp) { // only if timer_cb occurred after last change by EDIT4

        // test if alignment length has changed:
        {
            GBDATA *gb_alignment = GBT_get_alignment(gb_main,ED4_ROOT->alignment_name);
            e4_assert(gb_alignment);
            GBDATA *gb_alignment_len = GB_search(gb_alignment,"alignment_len",GB_FIND);
            int alignment_length = GB_read_int(gb_alignment_len);

            if (MAXSEQUENCECHARACTERLENGTH!=alignment_length) {
                ED4_alignment_length_changed(gb_alignment_len, 0, GB_CB_CHANGED);
            }
        }

        GBDATA *gb_seq = get_species_pointer();
        int type = GB_read_type(gb_seq);

        if (type==GB_STRING) {
            char *data = (char*)GB_read_old_value();
            int data_len = GB_read_old_size();

            if (data) {
                char *dup_data = new char[data_len+1];

                memcpy(dup_data, data, data_len);
                dup_data[data_len] = 0;

#if defined(DEBUG) && 0
                char *n = GB_read_string(gb_seq);
                e4_assert(strcmp(n,dup_data)!=0); // not really changed
                delete n;
#endif

                if (dynamic_prop & ED4_P_CONSENSUS_RELEVANT) {
                    ED4_multi_species_manager *multiman = get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                    ED4_species_manager *spman = get_parent(ED4_L_SPECIES)->to_species_manager();

                    multiman->check_bases_and_rebuild_consensi(dup_data, data_len, spman, ED4_U_UP);
                    set_refresh(1);
                    parent->refresh_requested_by_child();
//                     ED4_ROOT->main_manager->Show();
                }

                delete [] dup_data;
            }
        }
    }
}

void ED4_base::deleted_from_database()
{
    my_species_pointer.notify_deleted();
}
void ED4_terminal::deleted_from_database()
{
    ED4_base::deleted_from_database();
}
void ED4_text_terminal::deleted_from_database()
{
#if defined(DEBUG)
    printf("ED4_text_terminal::deleted_from_database (%p)\n", this);
#endif // DEBUG
    ED4_terminal::deleted_from_database();

    // @@@ hide all cursors pointing to this terminal!

    if (parent->is_name_manager()) {
#if defined(DEBUG)
        printf("- Deleting name terminal\n");
#endif // DEBUG
        ED4_name_manager *name_man = parent->to_name_manager();
        flag.deleted               = 1;
        name_man->delete_requested_by_child();
    }
    else if (parent->is_sequence_manager()) {
#if defined(DEBUG)
        printf("- Deleting sequence terminal\n");
#endif // DEBUG
        ED4_sequence_manager *seq_man = parent->to_sequence_manager();
        flag.deleted                  = 1;
        seq_man->delete_requested_by_child();
    }
    else {
        e4_assert(0); // not prepated for that situation
    }
}
void ED4_sequence_terminal::deleted_from_database()
{
#if defined(DEBUG)
    printf("ED4_sequence_terminal::deleted_from_database (%p)\n", this);
#endif // DEBUG

    ED4_terminal::deleted_from_database();

    bool was_consensus_relevant = dynamic_prop & ED4_P_CONSENSUS_RELEVANT;
    
    dynamic_prop = ED4_properties(dynamic_prop&~(ED4_P_CONSENSUS_RELEVANT|ED4_P_ALIGNMENT_DATA));

    // @@@ hide all cursors pointing to this terminal!

    char *data = (char*)GB_read_old_value();
    int data_len = GB_read_old_size();

    ED4_multi_species_manager *multi_species_man = get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

    if (was_consensus_relevant) { 
        multi_species_man->check_bases(data, data_len, 0);
        multi_species_man->rebuild_consensi(get_parent(ED4_L_SPECIES)->to_species_manager(), ED4_U_UP);
    }

    ED4_sequence_manager *seq_man = parent->to_sequence_manager();
    flag.deleted = 1;
    seq_man->delete_requested_by_child();
}
void ED4_manager::deleted_from_database()
{
    if (is_species_manager()) {
        ED4_species_manager *species_man = to_species_manager();
        ED4_multi_species_manager *multi_man = species_man->parent->to_multi_species_manager();

        multi_man->children->delete_member(species_man);
        GB_push_transaction(gb_main);
        multi_man->update_consensus(multi_man, 0, species_man);
        multi_man->rebuild_consensi(species_man, ED4_U_UP);
        GB_pop_transaction(gb_main);

        ED4_device_manager *device_manager = ED4_ROOT->main_manager
            ->get_defined_level(ED4_L_GROUP)->to_group_manager()
            ->get_defined_level(ED4_L_DEVICE)->to_device_manager();

        int i;
        for (i=0; i<device_manager->children->members(); i++) { // when killing species numbers have to be recalculated
            ED4_base *member = device_manager->children->member(i);

            if (member->is_area_manager()) {
                member->to_area_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->generate_id_for_groups();
            }
        }

        if (parent) parent->resize_requested_by_child();
        ED4_ROOT->refresh_all_windows(0);

        parent = 0;
        //	delete this; // crashes when removing callback deleted_from_database()
    }
    else {
        e4_assert(0);
    }
}

void ED4_sequence_changed_cb(GBDATA *gb_seq, int *cl, GB_CB_TYPE gbtype)
{
    ED4_base *base = (ED4_base*)(cl);

    if (base->get_species_pointer()!=gb_seq) {
        e4_assert(0);
        aw_message("Illegal callback (ED4_sequence_changed_cb())");
    }

    if (gbtype&GB_CB_DELETE) {
        e4_assert(gbtype==GB_CB_DELETE);
        base->deleted_from_database();
    }

    if (gbtype&GB_CB_CHANGED) {
        base->changed_by_database();
    }

    if (gbtype&GB_CB_SON_CREATED) {
        // @@@ New son for database-member was created ... what may we do now?
    }
}

int ED4_elements_in_species_container; // # of elements in species container
void ED4_species_container_changed_cb(GBDATA *gb_species_data, int */*cl*/, GB_CB_TYPE gbtype)
{
    if (gbtype==GB_CB_CHANGED) {
        int nsons = GB_rescan_number_of_subentries(gb_species_data);

        if (nsons>ED4_elements_in_species_container) { // new species was created
#if defined(DEBUG) && 1
            printf("# of species in species-container changed from %i to %i\n", ED4_elements_in_species_container, nsons);
            aw_message("Species container changed!", "OK");
#endif
            GBDATA *gb_lastSon = GB_search_last_son(gb_species_data);

            if (gb_lastSon) {
                GBDATA *gb_name = GB_search(gb_lastSon, "name", GB_FIND);

                if (gb_name) {
                    char *name = GB_read_as_string(gb_name);

                    ED4_get_and_jump_to_species(name);
#if defined(DEBUG) && 1
                    printf("new species = '%s'\n", name);
#endif
                    free(name);
                }
            }
        }
        ED4_elements_in_species_container = nsons;
    }
}

ED4_species_pointer::ED4_species_pointer()
{
    species_pointer = 0;
}
ED4_species_pointer::~ED4_species_pointer()
{
    e4_assert(species_pointer==0);	// must be destroyed before
}
void ED4_species_pointer::add_callback(int *clientdata)
{
    GB_push_transaction(gb_main);
    GB_add_callback(species_pointer, (GB_CB_TYPE ) (GB_CB_CHANGED|GB_CB_DELETE), (GB_CB)ED4_sequence_changed_cb, clientdata);
    GB_pop_transaction(gb_main);
}
void ED4_species_pointer::remove_callback(int *clientdata)
{
    GB_push_transaction(gb_main);
    GB_remove_callback(species_pointer, (GB_CB_TYPE ) (GB_CB_CHANGED|GB_CB_DELETE), (GB_CB)ED4_sequence_changed_cb, clientdata);
    GB_pop_transaction(gb_main);
}
void ED4_species_pointer::set_species_pointer(GBDATA *gbd, int *clientdata)
{
    if (species_pointer) remove_callback(clientdata);
    species_pointer = gbd;
    if (species_pointer) add_callback(clientdata);
}

//*****************************************
//* ED4_base Methods		beginning *
//*****************************************

bool ED4_base::is_visible(AW_pos x, AW_pos y, ED4_direction direction)
  // indicates whether x, y are in the visible scrolling area
  // x, y are calculated by calc_world_coords
{
    bool        visible = true;
    ED4_coords& coords  = ED4_ROOT->temp_ed4w->coords;

    switch (direction)
    {
        case ED4_D_HORIZONTAL: 
            if ((long(x)<coords.window_left_clip_point) ||
                (long(x)>coords.window_right_clip_point)) {
                visible = false;
            }
            break;
        case ED4_D_VERTICAL: 
            if ((long(y)<coords.window_upper_clip_point) ||
                (long(y)>coords.window_lower_clip_point)) {
                visible = false;
            }
            break;
        case ED4_D_VERTICAL_ALL: // special case for scrolling (whole object visible)
            if ((long(y) < coords.window_upper_clip_point) ||
                (long(y) + extension.size[HEIGHT] > coords.window_lower_clip_point)) {
                visible = false;
            }
            break;
        case ED4_D_ALL_DIRECTION:
            if ((long(x) < coords.window_left_clip_point)  ||
                (long(x) > coords.window_right_clip_point) ||
                (long(y) < coords.window_upper_clip_point) ||
                (long(y) > coords.window_lower_clip_point)) {
                visible = false;
            }
            break;
        default:
            visible = false;
            e4_assert(0);
            break;
    }

    return visible;
}

bool ED4_base::is_visible(AW_pos x1, AW_pos y1, AW_pos x2, AW_pos y2, ED4_direction direction)
// optimized case of function above (to avoid the need to call it 4 times)
{
    e4_assert(direction == ED4_D_ALL_DIRECTION);
    e4_assert(x1 <= x2);
    e4_assert(y1 <= y2);

    bool        visible = true;
    ED4_coords& coords  = ED4_ROOT->temp_ed4w->coords;
    if ((long(x2)<coords.window_left_clip_point) ||
        (long(x1)>coords.window_right_clip_point) ||
        (long(y2)<coords.window_upper_clip_point) ||
        (long(y1)>coords.window_lower_clip_point)) {
        visible = true;
    }
    return visible;
}


char *ED4_base::resolve_pointer_to_string_copy(int *) const { return NULL; }
const char *ED4_base::resolve_pointer_to_char_pntr(int *) const { return NULL; }

ED4_ERROR *ED4_base::write_sequence(const char */*seq*/, int /*seq_len*/)
{
    e4_assert(0);
    return 0;
}


ED4_returncode ED4_manager::create_group(ED4_group_manager **group_manager, GB_CSTR group_name)			// creates group from user menu of AW_Window
{
    ED4_species_manager		*species_manager	= NULL;
    ED4_species_name_terminal	*species_name_terminal	= NULL;
    ED4_sequence_manager	*sequence_manager	= NULL;
    ED4_sequence_info_terminal	*sequence_info_terminal	= NULL;
    ED4_sequence_terminal	*sequence_terminal	= NULL;
    ED4_spacer_terminal		*group_spacer_terminal1	= NULL;
    ED4_spacer_terminal 	*group_spacer_terminal2	= NULL;
    ED4_multi_species_manager	*multi_species_manager	= NULL;
    ED4_bracket_terminal	*bracket_terminal	= NULL;

    char buffer[35];

    sprintf(buffer, "Group_Manager.%ld", ED4_counter);								//create new group manager
    *group_manager = new ED4_group_manager(buffer, 0, 0, 0, 0, NULL);

    sprintf(buffer, "Bracket_Terminal.%ld", ED4_counter);
    bracket_terminal = new ED4_bracket_terminal(buffer, 0, 0, BRACKETWIDTH, 0, *group_manager);
    (*group_manager)->children->append_member(bracket_terminal);

    sprintf(buffer, "MultiSpecies_Manager.%ld", ED4_counter);							//create new multi_species_manager
    multi_species_manager = new ED4_multi_species_manager(buffer, BRACKETWIDTH, 0, 0, 0,*group_manager);	//Objekt Gruppen name_terminal noch
    (*group_manager)->children->append_member( multi_species_manager );					//auszeichnen

    (*group_manager)->set_properties((ED4_properties) ( ED4_P_MOVABLE));
    multi_species_manager->set_properties((ED4_properties) (ED4_P_IS_HANDLE));
    bracket_terminal->set_properties((ED4_properties) ( ED4_P_IS_HANDLE));
    bracket_terminal->set_links( NULL, multi_species_manager );

    sprintf(buffer, "Group_Spacer_Terminal_Beg.%ld", ED4_counter);							//Spacer at beginning of group
    group_spacer_terminal1 = new ED4_spacer_terminal( buffer , 0, 0, 10, SPACERHEIGHT, multi_species_manager);	//For better Overview
    multi_species_manager->children->append_member( group_spacer_terminal1 );

    sprintf( buffer, "Consensus_Manager.%ld", ED4_counter );							//Create competence terminal
    species_manager = new ED4_species_manager( buffer, 0, SPACERHEIGHT, 0, 0, multi_species_manager );
    species_manager->set_properties( ED4_P_MOVABLE );
    species_manager->flag.is_consensus = 1;
    multi_species_manager->children->append_member( species_manager );


    species_name_terminal = new ED4_species_name_terminal(  group_name, 0, 0, MAXSPECIESWIDTH - BRACKETWIDTH, TERMINALHEIGHT, species_manager );
    species_name_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );	//only some terminals
    species_name_terminal->set_links( NULL, ED4_ROOT->ref_terminals.get_ref_sequence() );
    species_manager->children->append_member( species_name_terminal );							//properties

    sprintf( buffer, "Consensus_Seq_Manager.%ld", ED4_counter);
    sequence_manager = new ED4_sequence_manager( buffer, MAXSPECIESWIDTH, 0, 0, 0, species_manager );
    sequence_manager->set_properties( ED4_P_MOVABLE );
    species_manager->children->append_member( sequence_manager );

    sequence_info_terminal = new ED4_sequence_info_terminal( "DATA",/*NULL,*/ 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, sequence_manager );  	// Info fuer Gruppe
    sequence_info_terminal->set_links( ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence_info() );
    sequence_info_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
    sequence_manager->children->append_member( sequence_info_terminal );

    sequence_terminal = new ED4_consensus_sequence_terminal( "", SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, sequence_manager );
    sequence_terminal->set_properties( ED4_P_CURSOR_ALLOWED );
    sequence_terminal->set_links( ED4_ROOT->ref_terminals.get_ref_sequence() , ED4_ROOT->ref_terminals.get_ref_sequence());
    sequence_manager->children->append_member( sequence_terminal );

    sprintf(buffer, "Group_Spacer_Terminal_End.%ld", ED4_counter);							//Spacer at beginning of group
    group_spacer_terminal2 = new ED4_spacer_terminal( buffer , 0, SPACERHEIGHT + TERMINALHEIGHT, 10, SPACERHEIGHT, multi_species_manager);	//For better Overview
    multi_species_manager->children->append_member( group_spacer_terminal2 );

    ED4_counter ++;

    return ED4_R_OK;
}


ED4_returncode ED4_base::generate_configuration_string(char **generated_string)
{
    ED4_multi_species_manager *multi_species_manager = NULL;
    char sep_name[2];
    sep_name[0] = 1;
    sep_name[1] = 0;

    char *old_string;
    char *dummy        = NULL;
    char *species_name = NULL;
    int	  i;
    long  old_size;
    long  new_size;

    AW_pos old_pos = 0;

    if (!(*generated_string)) {
        *generated_string = new char[2];
        strcpy(*generated_string,sep_name);
    }
    ED4_manager *consensus_manager = NULL;

    if (is_species_name_terminal() &&
        !((ED4_terminal *)this)->flag.deleted) { // wenn multi_name_manager mehrere name_terminals hat, dann muss das echte name_terminal markiert sein

        old_size   = strlen(*generated_string);
        old_string = *generated_string;

        if (parent->flag.is_consensus) {
            dummy = new char[strlen(id)+1];

            for (i=0; id[i] != '(' && id[i] != '\0'; i++)
                dummy[i] = id[i];

            if (id[i] == '(')
                dummy[i-1] = '\0';

            new_size   = old_size + strlen(dummy) + 2;
        }
        else { // we are Species or SAI
            int len;
            species_name = resolve_pointer_to_string_copy(&len);
            new_size     = old_size + len + 3; // 3 because of separator and identifier
        }

        *generated_string = new char[new_size];

        for (i=0; i<old_size; ++i)
            (*generated_string)[i] = old_string[i];

        for (; i < new_size; ++i)
            (*generated_string)[i] = 0;


        if (parent->flag.is_consensus) { // in consensus terminals the brackets have to be eliminated
            strcat (*generated_string, dummy);
            delete [] dummy;
        }
        else if (parent->parent->parent->flag.is_SAI) {	// if Species_manager has flag is_SAI
            strcat(*generated_string, "S");
            strcat(*generated_string, species_name);
        }
        else {
            strcat(*generated_string, "L");
            strcat(*generated_string, species_name);
        }

        strcat(*generated_string, sep_name);
        delete [] old_string;
    }
    else if (is_group_manager()) {
        old_string = *generated_string;
        old_size   = strlen(*generated_string);
        new_size   = old_size + 1; // 3 because of separator and identifier
        *generated_string = new char[new_size+1];

        for (i=0; i<old_size; ++i) {
            (*generated_string)[i] = old_string[i];
        }

        for (; i < new_size; ++i) {
            (*generated_string)[i] = 0;
        }

        delete [] old_string;

        if (dynamic_prop & ED4_P_IS_FOLDED) {
            strcat(*generated_string, "F");
        }
        else {
            strcat(*generated_string, "G");
        }

        multi_species_manager = to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
        // moving Consensus to top of list is essentially

        // for creating the string
        if (!(multi_species_manager->children->member(1)->flag.is_consensus)) {
            for (i=0; i < multi_species_manager->children->members(); i++) {
                if (multi_species_manager->children->member(i)->flag.is_consensus) {
                    consensus_manager = multi_species_manager->children->member(i)->to_manager();
                    break;
                }
            }

            multi_species_manager->children->delete_member( consensus_manager );
            old_pos = consensus_manager->extension.position[Y_POS];
            consensus_manager->extension.position[Y_POS] = SPACERHEIGHT;
            ED4_base::touch_world_cache();
            multi_species_manager->children->append_member( consensus_manager );
        }
    }

    if (is_manager()) {
        ED4_manager *this_manager = this->to_manager();
        if (this_manager->children) {
            for (i=0; i<this_manager->children->members(); i++) {
                this_manager->children->member(i)->generate_configuration_string( generated_string );
            }
        }
    }

    if (multi_species_manager){
        old_string = *generated_string;
        old_size   = strlen(*generated_string);
        new_size   = old_size + 3;							// 3 because of separator and identifier
        *generated_string = new char[new_size];

        for (i=0; i<old_size; ++i)
            (*generated_string)[i] = old_string[i];

        for (; i < new_size; ++i)
            (*generated_string)[i] = 0;

        delete [] old_string;

        strcat(*generated_string, "E");
        strcat(*generated_string, sep_name);

        if (consensus_manager) {
            multi_species_manager->children->delete_member( consensus_manager );		// move Consensus back to old position
            consensus_manager->extension.position[Y_POS] = old_pos;
            ED4_base::touch_world_cache();
            multi_species_manager->children->append_member( consensus_manager );
        }
    }

    free(species_name);

    return ED4_R_OK;
}

ED4_returncode ED4_base::route_down_hierarchy(void **arg1, void **arg2, ED4_returncode (*function) (void **, void **, ED4_base *))
    // executes 'function' for every element in hierarchy
{
    function(arg1, arg2, this);
    return ED4_R_OK;
}

ED4_returncode ED4_manager::route_down_hierarchy(void **arg1, void **arg2, ED4_returncode (*function) (void **, void **, ED4_base *))
{
    int i;

    function(arg1, arg2, this);

    if (children) {
        for (i=0; i < children->members(); i++) {
            children->member(i)->route_down_hierarchy(arg1, arg2, function);
        }
    }

    return ED4_R_OK;
}

ED4_base *ED4_manager::find_first_that(ED4_level level, int (*condition)(ED4_base *to_test, AW_CL arg), AW_CL arg)
{
    if ((spec->level&level) && condition(this, arg)) {
        return this;
    }

    int i;

    if (children) {
        for (i=0; i<children->members(); i++) {
            ED4_base *child = children->member(i);

            if (child->is_manager()) {
                ED4_base *found = child->to_manager()->find_first_that(level, condition, arg);
                if (found) {
                    return found;
                }
            }
            else if ((child->spec->level&level) && condition(child, arg)) {
                return child;
            }
        }
    }

    return 0;
}

ED4_base *ED4_manager::find_first_that(ED4_level level, int (*condition)(ED4_base *to_test))
{
    return find_first_that(level, (int(*)(ED4_base*,AW_CL))condition, (AW_CL)0);
}

int ED4_base::calc_group_depth()
{
    ED4_base *temp_parent;
    int       cntr = 0;

    temp_parent = parent;
    while (temp_parent->parent && !(temp_parent->is_area_manager())) {
        if (temp_parent->is_group_manager()) {
            cntr ++;
        }
        temp_parent = temp_parent->parent;
    }

    return cntr; // don't count our own group
}

ED4_returncode ED4_base::remove_callbacks() //removes callbacks
{
    return ED4_R_IMPOSSIBLE;
}


ED4_base *ED4_base::search_spec_child_rek( ED4_level level ) //recursive search for level
{
    return spec->level&level ? this : (ED4_base*)NULL;
}

ED4_base *ED4_manager::search_spec_child_rek(ED4_level level)
{
    if (spec->level & level) return this;

    if (children) {
        int i;

        for (i=0; i<children->members(); i++) { // first check children
            if (children->member(i)->spec->level & level) {
                return children->member(i);
            }
        }

        for (i=0; i<children->members(); i++) {
            ED4_base *result = children->member(i)->search_spec_child_rek(level);
            if (result) {
                return result;
            }
        }
    }

    return 0;
}

ED4_terminal *ED4_base::get_next_terminal()
{
    ED4_terminal *terminal = 0;

    if (parent) {
        terminal = parent->get_first_terminal(index+1);
        if (!terminal) {
            terminal = parent->get_next_terminal();
        }
    }

    return terminal;
}

ED4_terminal *ED4_base::get_prev_terminal()
{
    ED4_terminal *terminal = 0;

    if (parent) {
        if (index) {
            terminal = parent->get_last_terminal(index-1);
        }
        if (!terminal) {
            terminal = parent->get_prev_terminal();
        }
    }

    return terminal;
}


int ED4_base::is_child_of(ED4_manager *check_parent)				// checks whether new_check_parent is a child of old_check_parent or not
{
    int i;
    //    int flag = 0;

    for (i=0; i<check_parent->children->members(); i++) {
        ED4_base *child = check_parent->children->member(i);

        if (child==this ||
            (child->is_manager() &&
             is_child_of(child->to_manager()))) {
            return 1;
        }
    }

    return 0;
}


ED4_AREA_LEVEL	ED4_base::get_area_level(ED4_multi_species_manager **multi_species_manager)
{

    ED4_base *temp_manager;
    temp_manager = get_parent( ED4_L_AREA );
    if (!temp_manager){
        return ED4_A_ERROR;
    }

    ED4_area_manager	*temp_parent;
    temp_parent = temp_manager->to_area_manager();

    if (temp_parent == ED4_ROOT->top_area_man)
    {
        if (multi_species_manager){
            *multi_species_manager = temp_parent->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
        }
        return ED4_A_TOP_AREA;
    }

    if (temp_parent == ED4_ROOT->middle_area_man)
    {
        if(multi_species_manager){
            *multi_species_manager = temp_parent->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
        }
        return ED4_A_MIDDLE_AREA;
    }

    return ED4_A_ERROR;
}


void ED4_manager::generate_id_for_groups() {
    int i;

    for (i=0; i<children->members(); i++) {
        if (children->member(i)->is_group_manager()) {
            ED4_multi_species_manager *multi_man = children->member(i)->to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

            multi_man->count_all_children_and_set_group_id();
        }
    }
}


int ED4_multi_species_manager::count_all_children_and_set_group_id() // counts all species of a multi_species_manager
{
    int counter = 0,
        i;
    char *name;


    //    ED4_multi_species_manager *multi_species_manager = get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (member->is_species_manager() && !member->flag.is_consensus) {
            counter ++;
        }
        else if (member->is_group_manager()) {
            counter += member->to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->count_all_children_and_set_group_id();
        }
    }


    ED4_base *consensus_name_terminal = get_consensus_terminal();
    name = (char*)GB_calloc(strlen(consensus_name_terminal->id)+10, sizeof(*name));

    for ( i=0; consensus_name_terminal->id[i] != '(' && consensus_name_terminal->id[i] != '\0' ; i++) {
        name[i] = consensus_name_terminal->id[i];
    }
    if (consensus_name_terminal->id[i] != '\0') { // skip space
        i--;
    }
    name[i] = '\0';
    sprintf(name, "%s (%d)",name, counter);

    free(consensus_name_terminal->id);
    consensus_name_terminal->id = name;

    //    update_species_counters();
    return counter;
}

void ED4_sequence_terminal::calc_intervall_displayed_in_rectangle(AW_rectangle *rect, long *left_index, long *right_index) { // rect contains win-coords
    AW_pos x ,y;
    // int    length_of_char = ED4_ROOT->font_info[ED4_G_SEQUENCES].get_width();
    int    length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords(ED4_ROOT->temp_ed4w->aww, &x, &y);

    int rel_left_x =  (int)(rect->l-x);
    int rel_right_x = (int)(rect->r-x);

    *left_index  = (int)((rel_left_x-CHARACTEROFFSET)/length_of_char); // - 1;
    *right_index = (int)((rel_right_x-CHARACTEROFFSET)/length_of_char) + 1;

    if (*right_index > MAXSEQUENCECHARACTERLENGTH) *right_index = MAXSEQUENCECHARACTERLENGTH;
    if (*left_index < 0) *left_index = 0;
}

void ED4_sequence_terminal::calc_update_intervall(long *left_index, long *right_index )
{
    AW_pos x ,y;
    // int    length_of_char = ED4_ROOT->font_info[ED4_G_SEQUENCES].get_width();
    int    length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    calc_world_coords( &x, &y );
    AW_device *dev = ED4_ROOT->temp_device;
    ED4_coords *coords = &ED4_ROOT->temp_ed4w->coords;

    int rel_left_x =  (int)( (dev->clip_rect.l-x) // Abstand vom linken Terminalrand zum Anfang des Clipping rectangles
                         + (coords->window_left_clip_point-x)); // Verschiebung der Sequenz (durch Scrollen) == slider Position

    int rel_right_x = (int)( (dev->clip_rect.r-x) + (coords->window_left_clip_point-x) );

    *left_index  = (int)((rel_left_x-CHARACTEROFFSET)/length_of_char); // - 1;
    *right_index = (int)((rel_right_x-CHARACTEROFFSET)/length_of_char) + 1;

    if (*right_index > MAXSEQUENCECHARACTERLENGTH) *right_index = MAXSEQUENCECHARACTERLENGTH;
    if (*left_index < 0) *left_index = 0;
}

void ED4_manager::create_consensus(ED4_group_manager *upper_group_manager)	//creates consensus
{											//is called by group manager
    ED4_group_manager *group_manager_for_child = upper_group_manager;

    if (is_group_manager()) {
        ED4_group_manager *group_manager = to_group_manager();

        group_manager->table().init(MAXSEQUENCECHARACTERLENGTH);
        group_manager_for_child = group_manager;
    }

    if (loading) {
        status_total_count += status_add_count;
        if (aw_status(status_total_count) == 1)	{								// Kill has been Pressed
            aw_closestatus();
            GB_close(gb_main);
            while (ED4_ROOT->first_window) {
                ED4_ROOT->first_window->delete_window(ED4_ROOT->first_window);
            }
            GB_commit_transaction( gb_main );
            GB_close(gb_main);
            delete ED4_ROOT->main_manager;
            ::exit(0);
        }
    }

    int i;
    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);

        if (member->is_species_manager()){
            ED4_species_manager *species_manager = member->to_species_manager();
            ED4_terminal *sequence_data_terminal = species_manager->get_consensus_relevant_terminal();

            if (sequence_data_terminal) {
                int   db_pointer_len;
                char *db_pointer = sequence_data_terminal->resolve_pointer_to_string_copy(&db_pointer_len);
                group_manager_for_child->table().add(db_pointer, db_pointer_len);
                e4_assert(!group_manager_for_child->table().empty());
                free(db_pointer);
            }
        }else if (member->is_group_manager()){
            ED4_group_manager *sub_group = member->to_group_manager();

            sub_group->create_consensus(sub_group);
            e4_assert(sub_group!=upper_group_manager);
            upper_group_manager->table().add(sub_group->table());
#ifdef DEBUG
            if (!sub_group->table().empty() && !sub_group->table().is_ignored()) {
                e4_assert(!upper_group_manager->table().empty());
            }
#endif
        }
        else if (member->is_manager()) {
            member->to_manager()->create_consensus(group_manager_for_child);
        }
    }
}

ED4_terminal *ED4_base::get_consensus_relevant_terminal()
{
    int i;

    if (is_terminal()) {
        if (dynamic_prop & ED4_P_CONSENSUS_RELEVANT) {
            return this->to_terminal();
        }
        return NULL;
    }

    ED4_manager  *manager           = this->to_manager();
    ED4_terminal *relevant_terminal = 0;
    int           members           = manager->children->members();

    for (i=0; !relevant_terminal && i<members; ++i) {
        ED4_base     *member = manager->children->member(i);
        relevant_terminal    = member->get_consensus_relevant_terminal();
    }

#if defined(DEBUG)
    if (relevant_terminal) {
        for (; i<members; ++i) {
            ED4_base *member = manager->children->member(i);        
            e4_assert(!member->get_consensus_relevant_terminal()); // there shall be only 1 consensus relevant terminal, since much code assumes that
        }
    }
#endif // DEBUG

    return relevant_terminal;
}

int ED4_multi_species_manager::count_visible_children() // is called by a multi_species_manager
{
    int 		counter = 0,
        i;

    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (member->is_species_manager()) {
            counter ++;
        }
        else if (member->is_group_manager()) {
            ED4_group_manager *group_manager = member->to_group_manager();
            if (group_manager->dynamic_prop & ED4_P_IS_FOLDED) {
                counter ++;
            }
            else {
                ED4_multi_species_manager *multi_species_manager = group_manager->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                counter += multi_species_manager->count_visible_children();
            }
        }
    }
    return counter;
}



ED4_base *ED4_base::get_parent(ED4_level lev) const
{
    ED4_base *temp_parent = this->parent;

    while (temp_parent && !(temp_parent->spec->level & lev)) {
        temp_parent = temp_parent->parent;
    }

    return temp_parent;
}

char *ED4_base::get_name_of_species(){
    GB_transaction dummy(gb_main);
    ED4_species_manager *temp_species_manager 	= get_parent( ED4_L_SPECIES )->to_species_manager();
    if (!temp_species_manager) return 0;
    ED4_species_name_terminal *species_name = 0;
    species_name = temp_species_manager->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
    char *name= 0;
    if (species_name){
        if (species_name->get_species_pointer()){
            name = GB_read_as_string(species_name->get_species_pointer());
        }
    }
    return name;
}

ED4_base *ED4_manager::get_defined_level(ED4_level lev) const
{
    int i;

    for (i=0; i<children->members(); i++) { // first make a complete search in myself
        if (children->member(i)->spec->level & lev) {
            return children->member(i);
        }
    }

    for (i=0; i<children->members(); i++) { // then all groups
        ED4_base *member = children->member(i);

        if (member->is_multi_species_manager()) {
            return member->to_multi_species_manager()->get_defined_level(lev);
        }
        else if (member->is_group_manager()) {
            return member->to_group_manager()->children->member(1)->to_multi_species_manager()->get_defined_level(lev);
        }
    }
    return NULL;								//nothing found
}

ED4_returncode ED4_base::set_width()						// sets object length of terminals to Consensus_Name_terminal if existing
{										// else to MAXSPECIESWIDTH
    int 		i;

    if (is_species_manager() && !flag.is_consensus && !parent->flag.is_consensus) {
        ED4_species_manager *species_manager = to_species_manager();
        ED4_multi_name_manager *multi_name_manager = species_manager->get_defined_level(ED4_L_MULTI_NAME)->to_multi_name_manager();	// case I'm a species
        ED4_terminal *consensus_terminal = parent->to_multi_species_manager()->get_consensus_terminal();

        for (i=0; i < multi_name_manager->children->members(); i++){
            ED4_name_manager *name_manager = multi_name_manager->children->member(i)->to_name_manager();
            if (consensus_terminal){
                name_manager->children->member(0)->extension.size[WIDTH] = consensus_terminal->extension.size[WIDTH];
            }
            else{
                name_manager->children->member(0)->extension.size[WIDTH] = MAXSPECIESWIDTH;
            }

            name_manager->resize_requested_by_child();
        }

        for (i=0; i < species_manager->children->members(); i++) { // adjust all managers as far as possible
            ED4_base *smember = species_manager->children->member(i);
            if (consensus_terminal) {
                ED4_base *kmember = consensus_terminal->parent->children->member(i);
                if (kmember) {
                    smember->extension.position[X_POS] = kmember->extension.position[X_POS];
                    ED4_base::touch_world_cache();
                }
            }
            else { // got no consensus
                ED4_species_manager *a_species = parent->get_defined_level(ED4_L_SPECIES)->to_species_manager();
                if (a_species) {
                    smember->extension.position[X_POS] = a_species->children->member(i)->extension.position[X_POS];
                    ED4_base::touch_world_cache();
                }
            }
            species_manager->children->member(i)->resize_requested_by_child();
        }
    }
    else if (is_group_manager()) {
        ED4_group_manager *group_manager = this->to_group_manager();
        ED4_multi_species_manager *multi_species_manager = group_manager->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
        ED4_terminal *mark_consensus_terminal = multi_species_manager->get_consensus_terminal();
        ED4_terminal *consensus_terminal = parent->to_multi_species_manager()->get_consensus_terminal();

        if (consensus_terminal) { // we're a group in another group
            mark_consensus_terminal->extension.size[WIDTH] = consensus_terminal->extension.size[WIDTH] - BRACKETWIDTH;
        }
        else { // we're at the top (no consensus terminal)
            mark_consensus_terminal->extension.size[WIDTH] = MAXSPECIESWIDTH - BRACKETWIDTH;
        }

        mark_consensus_terminal->parent->resize_requested_by_child();

        for (i=0; i < multi_species_manager->children->members(); i++) {
            multi_species_manager->children->member(i)->set_width();
        }

        for (i=0; i < group_manager->children->members(); i++) { // for all groups below from us
            if (group_manager->children->member(i)->is_group_manager()) {
                group_manager->children->member(i)->set_width();
            }
        }
    }

    return ED4_R_OK;
}


short ED4_base::in_border( AW_pos x, AW_pos y, ED4_movemode mode )				// determines if given world coords x and y
{												// are within borders of current object according to move mode
    AW_pos    world_x, world_y;

    calc_world_coords( &world_x, &world_y );						// calculate absolute extension of current object

    switch ( mode )										// which direction ?
    {
        case ED4_M_HORIZONTAL:
            {
                if ( (x >= world_x) && (x < (world_x + extension.size[WIDTH])) )
                    return ( 1 );						// target location is within the borders of parent
                break;
            }
        case ED4_M_VERTICAL:
            {
                if ( (y >= world_y) && (y < (world_y + extension.size[HEIGHT])) )
                    return ( 1 );						// target location is within the borders of parent
                break;
            }
        case ED4_M_FREE:
            {
                return ( in_border( x, y, ED4_M_HORIZONTAL ) && in_border( x, y, ED4_M_VERTICAL ) );
            }
        case ED4_M_NO_MOVE:
            {
                break;
            }
    }

    return ( 0 );
}


void ED4_base::calc_rel_coords(AW_pos *x, AW_pos *y) // calculates coordinates relative to current object from given world coords
{
    AW_pos   world_x, world_y;

    calc_world_coords( &world_x, &world_y );						// calculate world coordinates of current object

    *x -= world_x;										// calculate relative coordinates by substracting world
    *y -= world_y;										// coords of current object
}


ED4_returncode	ED4_base::event_sent_by_parent( AW_event */*event*/, AW_window */*aww*/)
{
    return ( ED4_R_OK );
}

ED4_returncode ED4_manager::hide_children()
{
    int i;

    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (!member->is_spacer_terminal() && !member->flag.is_consensus) { // don't hide spacer and Consensus
            member->flag.hidden = 1;
        }
    }

    update_info.set_resize(1);
    resize_requested_by_parent();

    return ED4_R_OK;
}


ED4_returncode ED4_manager::make_children_visible()
{
    if (!children) {
        return ED4_R_WARNING;
    }
    else {
        for (int i=0; i < children->members(); i++) {
            children->member(i)->flag.hidden = 0; // make child visible
        }

        update_info.set_resize(1);
        resize_requested_by_parent();

        return ED4_R_OK;
    }
}

ED4_returncode ED4_manager::unfold_group(char *bracket_ID_to_unfold)
{
    int i,
        nr_of_visible_species = 0,
        nr_of_children_in_group = 0;
    ED4_base		*bracket_terminal;
    ED4_manager		*temp_parent;
    ED4_AREA_LEVEL		level;


    bracket_terminal = search_ID(bracket_ID_to_unfold);
    if (!bracket_terminal) return ED4_R_WARNING;

    temp_parent = bracket_terminal->parent;
    if (!temp_parent) return ED4_R_WARNING;

    ED4_multi_species_manager *multi_species_manager = NULL;

    level = temp_parent->get_area_level(&multi_species_manager);

    if (level==ED4_A_TOP_AREA || level==ED4_A_BOTTOM_AREA) { // check if there are any unfolding restrictions
        nr_of_visible_species = multi_species_manager->count_visible_children();

        if (nr_of_visible_species >= 10) {
            aw_message("Top area limited to 10 species\n"
                       "Advice: Move group to main area and try again");
            return ED4_R_IMPOSSIBLE;
        }

        nr_of_children_in_group = temp_parent->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->count_visible_children();

        if (nr_of_children_in_group + nr_of_visible_species - 1 > 10) {
            aw_message("Top area limited to 10 species\n"
                       "Advice: Move group to main area and try again");
            return ED4_R_IMPOSSIBLE;
        }
    }

    for (i=0; i < temp_parent->children->members(); i++) {
        ED4_base *member = temp_parent->children->member(i);

        if (member->is_multi_species_manager()) {
            multi_species_manager = member->to_multi_species_manager();
            multi_species_manager->make_children_visible();
            multi_species_manager->dynamic_prop = ED4_properties(multi_species_manager->dynamic_prop & ~ED4_P_IS_FOLDED );

            ED4_spacer_terminal *spacer = multi_species_manager->get_defined_level(ED4_L_SPACER)->to_spacer_terminal();
            spacer->extension.size[HEIGHT] = SPACERHEIGHT;
        }
    }

    bracket_terminal->dynamic_prop =  ED4_properties(bracket_terminal->dynamic_prop & ~ED4_P_IS_FOLDED);
    temp_parent->dynamic_prop =  ED4_properties(temp_parent->dynamic_prop & ~ED4_P_IS_FOLDED);

    ED4_ROOT->main_manager->update_info.set_resize(1);
    ED4_ROOT->main_manager->resize_requested_by_parent();

    return ED4_R_OK;
}

ED4_returncode ED4_manager::fold_group(char *bracket_ID_to_fold)
{
    ED4_base *bracket_terminal;
    ED4_manager *temp_parent;

    bracket_terminal = search_ID(bracket_ID_to_fold);

    if (!bracket_terminal) return ED4_R_WARNING;

    temp_parent = bracket_terminal->parent;
    if (!temp_parent) return ED4_R_WARNING;

    ED4_multi_species_manager *multi_species_manager = temp_parent->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
    ED4_manager *consensus_manager = NULL;

    int consensus_shown = 0;
    if (!(multi_species_manager->children->member(1)->flag.is_consensus)) { // if consensus is not a top => move to top
        ED4_members *multi_children = multi_species_manager->children;
        int i;

        for (i=0; i<multi_children->members(); i++) { // search for consensus
            if (multi_children->member(i)->flag.is_consensus) {
                consensus_manager = multi_children->member(i)->to_manager();
                break;
            }
        }

        if (consensus_manager) {
            multi_children->move_member(i, 1); // move Consensus to top of list
            consensus_manager->extension.position[Y_POS] = SPACERHEIGHT;
            ED4_base::touch_world_cache();
            consensus_shown = 1;
        }
    }
    else  {
        consensus_shown = 1;
    }

    if (consensus_shown && ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_SHOW)->read_int()==0) {
        consensus_shown = 0;
    }

    ED4_spacer_terminal *spacer = multi_species_manager->get_defined_level(ED4_L_SPACER)->to_spacer_terminal();
    if (spacer) {
        spacer->extension.size[HEIGHT] = consensus_shown ? SPACERHEIGHT : SPACERNOCONSENSUSHEIGHT;
    }

    multi_species_manager->hide_children();
    multi_species_manager->set_properties((ED4_properties) (ED4_P_IS_FOLDED) );

    bracket_terminal->set_properties((ED4_properties) (ED4_P_IS_FOLDED) );
    temp_parent->set_properties((ED4_properties) (ED4_P_IS_FOLDED) );

    // fix scrollbars:
    ED4_ROOT->main_manager->update_info.set_resize(1);
    ED4_ROOT->main_manager->resize_requested_by_parent();

    return ED4_R_OK;
}


void ED4_base::check_all()
{
    AW_pos x,y;

    //	printf("\nName des Aufrufers von Check_All : \t%.40s\n", id);
    //	if (spec->level & ED4_L_MULTI_SPECIES)
    //	{
    calc_world_coords( &x, &y);

    printf("Typ des Aufrufers :\t\t\t%s\n", is_manager() ? "Manager" : "Terminal");
    printf("Name des Aufrufers von Check_All : \t%.30s\n", (id) ? id : "Keine ID");
    printf("Linke obere Ecke x, y : \t\t%f, %f\n", extension.position[0], extension.position[1]);
    printf("Breite und Hoehe x, y : \t\t%f, %f\n", extension.size[0], extension.size[1]);
    printf("World Coords     x, y : \t\t%f, %f\n\n", x, y);
    //		printf("Laenge des Inhalts    : \t\t%d\n",strlen(resolve_pointer_to_string()));
    //	}

    //	if ((spec->static_prop & ED4_P_IS_MANAGER) == 1)					//only scan further if calling object is manager and has children
    //	{
    //		for (i=0; i < children->no_of_members; i++)
    //			(children->member_list[i])->check_all();
    //	}
    printf("***********************************************\n\n");

}

int ED4_base::adjust_clipping_rectangle( void )
// return 0 if clipping rectangle disappeared (nothing left to draw)
{
    AW_pos x, y;

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );
    return ED4_ROOT->temp_device->reduceClipBorders(int(y), int(y+extension.size[HEIGHT]-1), int(x), int(x+extension.size[WIDTH]-1));
}


void ED4_base::set_properties( ED4_properties prop )
{
    dynamic_prop = (ED4_properties) (dynamic_prop | prop);
}


ED4_returncode ED4_base::set_links(ED4_base *temp_width_link, ED4_base *temp_height_link )	//sets links in hierarchy :
    //width-link sets links between objects on same level
{												//height-link sets links between objects on different levels
    if (temp_width_link)
    {
        if (width_link)								//if object already has a link
            width_link->linked_objects.delete_elem( (void *) this );		//delete link and

        width_link = temp_width_link;						//set new link to temp_width_link
        temp_width_link->linked_objects.append_elem( (void *) this );
    }

    if (temp_height_link)
    {
        if ( height_link != NULL )
            height_link->linked_objects.delete_elem( (void *) this );

        height_link = temp_height_link;
        temp_height_link->linked_objects.append_elem( (void *) this );
    }

    return ( ED4_R_OK );
}

ED4_returncode ED4_base::link_changed( ED4_base *link )
{
    if ( (width_link == link) || (height_link == link) )
        if ( calc_bounding_box() )
            if ( parent != NULL )
                parent->resize_requested_by_child();

    return ( ED4_R_OK );
}

int ED4_base::actualTimestamp = 1;
void ED4_base::update_world_coords_cache() {
    if (parent) {
        parent->calc_world_coords(&lastXpos, &lastYpos);
    }
    else {
        lastXpos = 0;
        lastYpos = 0;
    }
    lastXpos += extension.position[X_POS];
    lastYpos += extension.position[Y_POS];
    timestamp = actualTimestamp;
}


ED4_returncode ED4_base::clear_background(int color)
{
    AW_pos x, y;

    if (ED4_ROOT->temp_device)
    {
        calc_world_coords( &x, &y );
        ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

        ED4_ROOT->temp_device->push_clip_scale();
        if (adjust_clipping_rectangle()) {
            if (!color) {
                ED4_ROOT->temp_device->clear_part(x, y, extension.size[WIDTH], extension.size[HEIGHT], (AW_bitset)-1);
            }
            else {
                ED4_ROOT->temp_device->box(color, x, y, extension.size[WIDTH], extension.size[HEIGHT], (AW_bitset)-1, 0, 0); // fill range with color for debugging
            }
        }
        ED4_ROOT->temp_device->pop_clip_scale();
    }
    return ( ED4_R_OK );
}

ED4_returncode ED4_base::clear_whole_background( void )						// clear AW_MIDDLE_AREA
{
    if (ED4_ROOT->temp_device)
    {
        ED4_ROOT->temp_device->push_clip_scale();
        ED4_ROOT->temp_device->clear((AW_bitset)-1);
        ED4_ROOT->temp_device->pop_clip_scale();
    }

    return ( ED4_R_OK );
}

void ED4_base::draw_bb(int color)
{
    if (ED4_ROOT->temp_device) {
        ED4_ROOT->temp_device->push_clip_scale();
        if (adjust_clipping_rectangle()) {
            AW_pos x1, y1, x2, y2;
            calc_world_coords( &x1, &y1 );
            ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x1, &y1 );
            x2 = x1+extension.size[WIDTH]-1;
            y2 = y1+extension.size[HEIGHT]-1;

            ED4_ROOT->temp_device->line(color, x1, y1, x1, y2, (AW_bitset)-1, 0, 0);
            ED4_ROOT->temp_device->line(color, x1, y1, x2, y1, (AW_bitset)-1, 0, 0);
            ED4_ROOT->temp_device->line(color, x2, y1, x2, y2, (AW_bitset)-1, 0, 0);
            ED4_ROOT->temp_device->line(color, x1, y2, x2, y2, (AW_bitset)-1, 0, 0);
        }
        ED4_ROOT->temp_device->pop_clip_scale();
    }
}

ED4_base::ED4_base(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
{
    index = 0;
    dynamic_prop = ED4_P_NO_PROP;
    timestamp =  0; // invalid - almost always..

    //	id = new char[strlen(temp_id)+1];


    if (!strcmp(CONSENSUS,temp_id)) {
        id = NULL;
    }
    else {
        id = (char*)malloc(strlen(temp_id)+1);
        strcpy( id, temp_id);
    }


    extension.position[X_POS] = x;
    extension.position[Y_POS] = y;
    ED4_base::touch_world_cache();
    extension.size[WIDTH] = width;
    extension.size[HEIGHT] = height;
    extension.y_folded = 0;
    parent = temp_parent;
    width_link = NULL;
    height_link = NULL;

    memset((char*)&update_info, 0, sizeof(update_info));
    memset((char*)&flag, 0, sizeof(flag));
}


ED4_base::~ED4_base() // before calling this function the first time, parent has to be set NULL
{
    ED4_base *object;
    ED4_base *sequence_terminal = NULL;
    ED4_list_elem *list_elem, *old_elem;
    ED4_window *ed4w;

    list_elem = linked_objects.first();
    while (list_elem) {
        object = (ED4_base *) list_elem->elem();
        if ( object->width_link == this ) {
            object->width_link->linked_objects.delete_elem( (void *) this );		//delete link and
            object->width_link = NULL;
        }

        if ( object->height_link == this ) {
            object->height_link->linked_objects.delete_elem( (void *) this );		//delete link and
            object->height_link = NULL;
        }

        old_elem = list_elem;
        list_elem = list_elem->next();
        linked_objects.delete_elem( old_elem->elem() );
    }

    if ( update_info.linked_to_scrolled_rectangle )
    {
        if (ED4_ROOT->main_manager)
        {
            sequence_terminal = ED4_ROOT->main_manager->search_spec_child_rek( ED4_L_SEQUENCE_STRING );

            if (sequence_terminal)
                sequence_terminal->update_info.linked_to_scrolled_rectangle = 1;

            update_info.linked_to_scrolled_rectangle = 0;
            ED4_ROOT->scroll_links.link_for_hor_slider = sequence_terminal;

            ed4w = ED4_ROOT->temp_ed4w;
            while ( ed4w != NULL )
            {
                if ( ed4w->scrolled_rect.x_link == this )
                    ed4w->scrolled_rect.x_link = sequence_terminal;

                if ( ed4w->scrolled_rect.width_link == this )
                    ed4w->scrolled_rect.width_link = sequence_terminal;

                ed4w = ed4w->next;
            }
        }
    }

    if (width_link)
    {
        width_link->linked_objects.delete_elem( (void *) this );
        width_link = NULL;
    }

    if (height_link)
    {
        height_link->linked_objects.delete_elem( (void *) this );
        height_link = NULL;
    }

    set_species_pointer(0);	// clear pointer to database and remove callbacks
    free(id);
}

//***********************************
//* ED4_base Methods		end *
//***********************************

