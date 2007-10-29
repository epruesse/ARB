#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_window.hxx>

#include "ed4_class.hxx"



//*****************************************
//* ED4_members Methods		beginning *
//*****************************************


ED4_returncode ED4_members::search_target_species( ED4_extension *location,  ED4_properties prop,  ED4_base **found_member, ED4_level return_level )
{
    // who's extension falls within the given
    // location thereby considering orientation given by
    // prop (only pos[] is relevant) list has to be
    // ordered in either x- or y- direction, ( binary
    // search algorithm later ) returns index of found member,
    // -1 if list is empty or no_of_members if search reached end of list
    ED4_index	current_index = 0,
        old_index,
        rel_pos,								// relativ position, i.e. on screen, to check
        rel_size;
    AW_pos		abs_pos_x = 0,
        abs_pos_y = 0,
        abs_pos;								// relativ size of object, to check
    ED4_base	*current_member;


    current_member = member(0);	// search list
    if (! current_member) { // there's no list
        return ( ED4_R_IMPOSSIBLE );
    }

    // case for the device_manager:
    if (current_member->is_area_manager()) {
        current_member->to_area_manager()
            ->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()
            ->children->search_target_species(location, prop, found_member,return_level ); //there are always the three areas !!!

        if (*found_member) {
            return ED4_R_OK;
        }
    }

    current_member->parent->calc_world_coords( & abs_pos_x, & abs_pos_y );

    if ( prop & ED4_P_HORIZONTAL )								// set extension-indexes rel_pos and rel_size according to properties
    {
        rel_pos = Y_POS;
        rel_size = HEIGHT;
        abs_pos = abs_pos_y;
    }
    else											// i.e. prop & ED4_P_VERTICAL
    {
        rel_pos = X_POS;
        rel_size = WIDTH;
        abs_pos = abs_pos_x;
    }


    while (  current_member &&
             (location->position[rel_pos] >= (current_member->extension.position[rel_pos] + abs_pos)) &&	// just as long as possibility exists, to find the object
             (location->position[rel_pos] >= abs_pos && location->position[rel_pos] <= current_member->parent->extension.size[rel_size] + abs_pos))
    {
        if (current_member->is_group_manager() &&
            !current_member->flag.hidden &&
            !current_member->flag.is_consensus) { // search_clicked_member for multi_species_manager in groups
            current_member->to_group_manager()->children->search_target_species(location, prop, found_member,return_level);
        }
        else if ( !(current_member->flag.hidden)  &&
                  (location->position[rel_pos] <= (current_member->extension.position[rel_pos] +
                                                   abs_pos + current_member->extension.size[rel_size])))			// found a suitable member
        {
            if (return_level & ED4_L_MULTI_SPECIES) { // search for drag target
                if (current_member->is_multi_species_manager()) {
                    *found_member = current_member;					// we have to give back the multi_species_manager for
                    // insertion
                    current_member->to_multi_species_manager()->children->search_target_species(location, prop, found_member,return_level);
                }
                else if ((current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members)) { // if we have found the last spacer
                    *found_member = current_member->get_parent( ED4_L_MULTI_SPECIES );				// in a group we can move the
                    if ((*found_member) && !((*found_member)->parent->is_area_manager())) {
                        *found_member = (*found_member)->parent->get_parent( ED4_L_MULTI_SPECIES );
                    }
                }
                else if (!(current_member->is_terminal()) || (current_member->is_spacer_terminal())) { // object behind the group
                    *found_member = current_member->get_parent( ED4_L_MULTI_SPECIES );
                }

            }
            else									// search for drag target line
            {
                if (current_member->is_multi_species_manager()) {
                    current_member->to_multi_species_manager()->children->search_target_species(location, prop, found_member,return_level);
                }
                else if ((current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members)) { // if we have found the last spacer
                    *found_member = current_member->get_parent( ED4_L_MULTI_SPECIES ); // in a group we can move the
                    if ((*found_member) && !((*found_member)->parent->is_area_manager())) {
                        *found_member = (*found_member)->parent;
                    }
                }
                else if ((current_member->is_species_manager()) ||
                         ((current_member->is_spacer_terminal()) && current_member->parent->is_multi_species_manager())) { // we are a species manager
                    *found_member = current_member;
                }
            }

        }

        current_index++;								// no hit => search on
        current_member = member(current_index);

        if (current_member) { // handle folding groups
            old_index = current_index;
            while (current_member->flag.hidden && current_index!=no_of_members) {
                current_index ++;
                current_member = member(current_index);
            }

            if (current_index != old_index) {
                if (	 current_member &&
                         !((location->position[rel_pos] >= (current_member->extension.position[rel_pos] + abs_pos)) &&
                           (location->position[rel_pos] >= abs_pos && location->position[rel_pos] <= current_member->parent->extension.size[rel_size] + abs_pos)) &&
                         (current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members))
                {
                    if (return_level & ED4_L_MULTI_SPECIES)
                        *found_member = current_member->get_parent( ED4_L_MULTI_SPECIES )->parent->get_parent( ED4_L_MULTI_SPECIES );
                    else
                        *found_member = current_member->get_parent( ED4_L_MULTI_SPECIES )->parent;
                }
            }
        }

        if (current_member) {
            if (current_member->is_area_manager()) {
                current_member->to_area_manager()
                    ->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()
                    ->children->search_target_species( location, prop, found_member,return_level );	//there are always the three areas !!!

                if (*found_member) {
                    return ED4_R_OK;
                }
            }
        }
    }

    return ED4_R_OK;
}

ED4_returncode ED4_members::insert_member( ED4_base *new_member )			// inserts a new member into current owners's member array and
{											// asks to adjust owner's bounding box
    ED4_index	index;
    ED4_properties	prop;

    prop = owner()->spec->static_prop;						//properties of parent object

    if ( (index = search_member( &(new_member->extension), prop )) < 0 ) {	// search list for a suitable position
        index = 0;								// list was empty
    }
    else if ( index != no_of_members ) {					// we want to insert new member just behind found position
        index++;						 		// if index == no_of_members we reached the end of the list
    }										// and index already has the right value

    if (index > 0) { // ensure to insert before group_end_spacer
        if (member(index-1)) {
            if (member(index-1)->is_spacer_terminal() && !owner()->is_device_manager()) { // only in group_manager
                if (index > 1) {
                    index --;
                }
            }
        }
    }

    if ( shift_list( index, 1 ) != ED4_R_OK ) {// insert new_member at index after shifting to the right
        return ( ED4_R_WARNING );
    }

    memberList[index] = new_member; // insert new member in list in just allocated memory
    no_of_members ++;
    new_member->index = index;

    owner()->resize_requested_by_child();					// tell owner about resize

    return ( ED4_R_OK );
}

ED4_returncode ED4_members::append_member(ED4_base *new_member) {
    ED4_index index = no_of_members;

    if (index>=size_of_list) { // ensure free element
        ED4_index new_size_of_list = (size_of_list*3)/2;	// resize to 1.5*size_of_list
        ED4_base **new_member_list = (ED4_base**)GB_calloc(new_size_of_list, sizeof(*new_member_list));
        if (!new_member_list) {
            GB_memerr();
            return ED4_R_IMPOSSIBLE;
        }
        memcpy(new_member_list, memberList, size_of_list*sizeof(*new_member_list));

        free(memberList);
        memberList = new_member_list;
        size_of_list = new_size_of_list;
    }

    if (index>0) { // ensure to insert before group_end_spacer
        if (member(index-1)) {
            if (member(index-1)->is_spacer_terminal() && !owner()->is_device_manager()) {
                if (index>1) {
                    index--;
                }
            }
        }
    }

    if (shift_list(index, 1)!=ED4_R_OK) { // shift member if necessary
        return ED4_R_WARNING;
    }

    memberList[index] = new_member;
    no_of_members++;
    new_member->index = index;

    owner()->resize_requested_by_child();
    return ED4_R_OK;
}

ED4_returncode ED4_members::delete_member(ED4_base *member_to_del)
{
    if (!member_to_del || (no_of_members <= 0)) {
        return ( ED4_R_IMPOSSIBLE );
    }

    ED4_index index = member_to_del->index;

    if ( shift_list( (index + 1), -1 ) != ED4_R_OK ) { //shift member list to left, starting at index+1
        return ( ED4_R_WARNING );
    }

    member_to_del->parent = 0; // avoid referencing wrong parent

    no_of_members--;
    e4_assert(members_ok());

    owner()->resize_requested_by_child(); // tell owner about resize

    return ( ED4_R_OK );
}

ED4_returncode	ED4_members::shift_list( ED4_index start_index, int length )		// shifts member_list of current object by |length| positions starting with start_index,
{											// if length is positive shift is to the right, allocating new memory if necessary
    // if length is negative shift is to the left (up to position 0) without freeing memory
    ED4_index	i = 0;
    ED4_base	**tmp_ptr;
    unsigned int	new_alloc_size = 0;

    if (length>0) { // shift list to the right
        if ( (no_of_members + length) >= size_of_list ) { // member_list is full => allocate more memory
            new_alloc_size = (unsigned int) ((size_of_list + length) * 1.3); // calculate new size of member_list for realloc()

            tmp_ptr = memberList;
            tmp_ptr = (ED4_base **) realloc((char *) memberList, (new_alloc_size * sizeof(ED4_base *))); // try to realloc memory

            if (! tmp_ptr) { // realloc() failed => try malloc() and copy member_list
                aw_message("ED4_members::shift_list: realloc problem !", 0);
                tmp_ptr = (ED4_base **) malloc( (new_alloc_size * sizeof(ED4_base *)) );

                if (! tmp_ptr)									// malloc has failed, too
                    return ( ED4_R_DESASTER );
                else
                    tmp_ptr = (ED4_base **) memcpy( (char *) tmp_ptr,			// malloc was successfull, now copy memory
                                                    (char *) memberList,
                                                    (new_alloc_size * sizeof(ED4_base *)));
            }
            memberList = tmp_ptr;
            size_of_list = new_alloc_size;
            for ( i = no_of_members; i < size_of_list; i++ )				// clear free entries at the end of the list
                memberList[i] = NULL;
        }

        for ( i = (no_of_members + length); i > start_index; i-- )				// start shifting to the right
        {
            memberList[i] = memberList[i - length];
            if ( memberList[i] != NULL )
                (memberList[i])->index = i;
        }
    }
    else if ( length<0 )										// shift list to the left, thereby not freeing any memory !
    {
        if ( (start_index + length) < 0 )
        {
            aw_message("ED4_members::shift_list: shift is too far to the left !", 0);
            return ( ED4_R_WARNING );
        }

        for ( i = (start_index + length); i <= (no_of_members + length); i++ )			//start shifting left
        {
            memberList[i] = memberList[i - length];					// length is negative
            if ( memberList[i] != NULL )
                (memberList[i])->index = i;
        }
    }

    return ( ED4_R_OK );
}


ED4_returncode ED4_members::move_member(ED4_index old_pos, ED4_index new_pos) {
    if (old_pos>=0 && old_pos<no_of_members && new_pos>=0 && new_pos<no_of_members) {
        if (new_pos!=old_pos) {
            ED4_base *moved_member = memberList[old_pos];

            shift_list(old_pos+1, -1);
            shift_list(new_pos, 1);

            memberList[new_pos] = moved_member;
            moved_member->index = new_pos;

            e4_assert(members_ok());
        }
        return ED4_R_OK;
    }

    return ED4_R_IMPOSSIBLE;
}

ED4_index ED4_members::search_member( ED4_extension *location, ED4_properties prop)
    // searches member_list of current object for a member who's extension falls within the given location
    // thereby considering orientation given by prop (only pos[] is relevant)
    // list has to be ordered in either x- or y- direction, ( binary search algorithm later )
    // returns index of found member, -1 if list is empty or no_of_members if search reached end of list
{
    ED4_index	current_index = 0,
        rel_pos,
        rel_size;

    ED4_base *current_member;

    if ( prop & ED4_P_HORIZONTAL ) { // set extension-indexes rel_pos and rel_size according to properties
        rel_pos = Y_POS; rel_size = HEIGHT;
    }
    else { // i.e. prop & ED4_P_VERTICAL
        rel_pos = X_POS; rel_size = WIDTH;
    }

    current_member = memberList[0]; // search list
    if (! current_member) { // there's no list
        return ( -1 );
    }

    current_index = 0;
    while (current_member) { // just as long as possibility exists, to find the object
        if ( location->position[rel_pos] <= (current_member->extension.position[rel_pos] + current_member->extension.size[rel_size])) { // found a suitable member
            return ( current_index );
        }
        current_index++; // no hit => search on
        current_member = memberList[current_index];
    }

    return ( no_of_members );                                                         // reached this position => no hit, return no_of_members
}

#ifdef ASSERTION_USED
int ED4_members::members_ok() const {
    int m;
    int error = 0;

    for (m=0; m<no_of_members; m++) {
        ED4_base *base = memberList[m];

        if (base->index!=m) {
            printf("Member %i has illegal index %li\n", m, base->index);
            error = 1;
        }
    }

    return !error;
}
#endif // ASSERTION_USED

ED4_members::ED4_members(ED4_manager *the_owner)
{
    my_owner = the_owner;
    memberList = (ED4_base **) calloc(1, sizeof(ED4_base*));

    if ( memberList == NULL ) {
        aw_message( "ED4_member::ED4_member: memory problem !", "EXIT" );
    }

    memberList[0] = NULL;
    no_of_members = 0;
    size_of_list = 1;
}


ED4_members::~ED4_members()
{
    if (memberList) {
        free( (char *) memberList );
    }
}

//***********************************
//* ED4_members Methods		end *
//***********************************

