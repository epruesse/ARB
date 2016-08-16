// =============================================================== //
//                                                                 //
//   File      : ED4_container.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdb.h>
#include "ed4_class.hxx"
#include <aw_msg.hxx>
#include <aw_question.hxx>

ED4_returncode ED4_container::search_target_species(ED4_extension *location, ED4_properties prop, ED4_base **found_member, ED4_level return_level)
{
    // who's extension falls within the given
    // location thereby considering orientation given by
    // prop (only pos[] is relevant) list has to be
    // ordered in either x- or y- direction, ( binary
    // search algorithm later ) returns index of found member,
    // -1 if list is empty or no_of_members if search reached end of list


    if (!members()) { // there's no list
        return ED4_R_IMPOSSIBLE;
    }

    ED4_base *current_member = member(0);
    // case for the device_manager:
    if (current_member->is_area_manager()) {
        current_member->to_area_manager()
            ->get_multi_species_manager()
            ->search_target_species(location, prop, found_member, return_level); // there are always the three areas !!!

        if (*found_member) {
            return ED4_R_OK;
        }
    }

    ED4_index rel_pos; // relative position, i.e. on screen, to check
    ED4_index rel_size;
    AW_pos    abs_pos; // relative size of object, to check

    {
        AW_pos abs_pos_x = 0;
        AW_pos abs_pos_y = 0;
        current_member->parent->calc_world_coords(&abs_pos_x, &abs_pos_y);

        // set extension-indexes rel_pos, rel_size and abs_pos according to properties:
        if (prop & ED4_P_HORIZONTAL) {
            rel_pos = Y_POS;
            rel_size = HEIGHT;
            abs_pos = abs_pos_y;
        }
        else { // i.e. prop & ED4_P_VERTICAL
            rel_pos = X_POS;
            rel_size = WIDTH;
            abs_pos = abs_pos_x;
        }
    }

    ED4_index current_index = 0;

    while (current_member                                                                           &&
           (location->position[rel_pos] >= (current_member->extension.position[rel_pos] + abs_pos)) && // just as long as possibility exists, to find the object
           (location->position[rel_pos] >= abs_pos && location->position[rel_pos] <= current_member->parent->extension.size[rel_size] + abs_pos))
    {
        e4_assert(!current_member->is_root_group_manager());
        if (current_member->is_group_manager() &&
            !current_member->flag.hidden &&
            !current_member->is_consensus_manager()) { // search_clicked_member for multi_species_manager in groups
            current_member->to_group_manager()->search_target_species(location, prop, found_member, return_level);
        }
        else if (!(current_member->flag.hidden)   &&
                  (location->position[rel_pos] <= (current_member->extension.position[rel_pos] +
                                                   abs_pos + current_member->extension.size[rel_size]))) // found a suitable member
        {
            if (return_level & ED4_L_MULTI_SPECIES) { // search for drag target
                if (current_member->is_multi_species_manager()) {
                    *found_member = current_member; // we have to return the multi_species_manager for insertion
                    current_member->to_multi_species_manager()->search_target_species(location, prop, found_member, return_level);
                }
                else if ((current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members)) { // if we have found the last spacer in a group
                    // -> we can move the object behind the group
                    *found_member = current_member->get_parent(ED4_L_MULTI_SPECIES);
                    if ((*found_member) && !((*found_member)->parent->is_area_manager())) {
                        *found_member = (*found_member)->parent->get_parent(ED4_L_MULTI_SPECIES);
                    }
                }
                else if (!(current_member->is_terminal()) || (current_member->is_spacer_terminal())) {
                    *found_member = current_member->get_parent(ED4_L_MULTI_SPECIES);
                }

            }
            else                                                                        // search for drag target line
            {
                if (current_member->is_multi_species_manager()) {
                    current_member->to_multi_species_manager()->search_target_species(location, prop, found_member, return_level);
                }
                else if ((current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members)) { // if we have found the last spacer
                    *found_member = current_member->get_parent(ED4_L_MULTI_SPECIES);   // in a group we can move the
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

        current_index++; // no hit => continue search
        current_member = existing_index(current_index) ? member(current_index) : NULL;

        if (current_member) { // handle folding groups
            ED4_index old_index = current_index;
            while (current_member && current_member->flag.hidden && current_index!=no_of_members) {
                current_index++;
                current_member = existing_index(current_index) ? member(current_index) : NULL;
            }

            if (current_index != old_index) {
                if (current_member &&
                    !((location->position[rel_pos] >= (current_member->extension.position[rel_pos] + abs_pos)) &&
                      (location->position[rel_pos] >= abs_pos && location->position[rel_pos] <= current_member->parent->extension.size[rel_size] + abs_pos)) &&
                    (current_member->is_spacer_terminal()) && (current_index + 1 == no_of_members))
                {
                    if (return_level & ED4_L_MULTI_SPECIES)
                        *found_member = current_member->get_parent(ED4_L_MULTI_SPECIES)->parent->get_parent(ED4_L_MULTI_SPECIES);
                    else
                        *found_member = current_member->get_parent(ED4_L_MULTI_SPECIES)->parent;
                }
            }
        }

        if (current_member) {
            if (current_member->is_area_manager()) {
                current_member->to_area_manager()
                    ->get_multi_species_manager()
                    ->search_target_species(location, prop, found_member, return_level);      // there are always the three areas !!!

                if (*found_member) {
                    return ED4_R_OK;
                }
            }
        }
    }

    return ED4_R_OK;
}

void ED4_container::correct_insert_position(ED4_index& index) {
    // ensure to insert before group_end_spacer

    if (index>1                               && // avoid underflow
        existing_index(index-1)               &&
        member(index-1)->is_spacer_terminal() &&
        !owner()->is_device_manager())           // only in group_manager
    {
        index--;
    }
}

void ED4_container::resize(ED4_index needed_size) {
    if (needed_size>size_of_list) {
        e4_assert(needed_size>0);
#if defined(DEVEL_RALF)
        ED4_index new_size = needed_size; // @@@ deactivate when running stable for some time!
#else // !DEVEL_RALF
        ED4_index new_size = (needed_size*3)/2+2;
#endif

        ARB_recalloc(memberList, size_of_list, new_size);
        size_of_list = new_size;
    }
}

void ED4_container::insert_member(ED4_base *new_member) {
    // inserts a new member into current owners's member array and
    // asks to adjust owner's bounding box

    ED4_properties prop = owner()->spec.static_prop; // properties of parent object

    ED4_index index;
    if ((index = search_member(&(new_member->extension), prop)) < 0) { // search list for a suitable position
        index = 0;                                                     // list was empty
    }
    else if (index != no_of_members) {                                 // we want to insert new member just behind found position
        index++;                                                       // if index == no_of_members we reached the end of the list
    }                                                                  // and index already has the right value

    correct_insert_position(index); // insert before end-spacer
    shift_list(index, 1);           // shift members if necessary

    e4_assert(index<size_of_list);
    memberList[index] = new_member;
    no_of_members ++;
    new_member->index = index;

    owner()->request_resize(); // tell owner about resize
}

void ED4_container::append_member(ED4_base *new_member) {
    ED4_index index = no_of_members;

    e4_assert(owner()->spec.allowed_to_contain(new_member->spec.level));

    correct_insert_position(index); // insert before end-spacer
    shift_list(index, 1);           // shift members if necessary

    e4_assert(index<size_of_list);
    memberList[index] = new_member;
    no_of_members++;
    new_member->index = index;

    owner()->spec.announce_added(new_member->spec.level);
    owner()->request_resize();
}

ED4_returncode ED4_container::remove_member(ED4_base *member_to_del) {
    if (!member_to_del || (no_of_members <= 0)) {
        return ED4_R_IMPOSSIBLE;
    }

    ED4_index index = member_to_del->index;
    e4_assert(member_to_del->parent == this);

    shift_list(index+1, -1); // shift member list to left, starting at index+1

    member_to_del->parent = 0; // avoid referencing wrong parent
    no_of_members--;
    e4_assert(members_ok());

    owner()->request_resize();

    return ED4_R_OK;
}

void ED4_container::shift_list(ED4_index start_index, int length) {
    // shifts member_list of current object by |length| positions starting with start_index,
    // if length is positive => shift to the right; allocates new memory if necessary
    // if length is negative => shift to the left (down to position 0); does not resize allocated memory

    e4_assert(length != 0);

    if (length>0) { // shift list to the right
        long needed_size = no_of_members + length;

        e4_assert(start_index<needed_size);
        resize(needed_size);

        for (ED4_index n = needed_size-1; n > start_index; n--) { // start shifting to the right
            ED4_index o = n-length;

            e4_assert(valid_index(n));
            e4_assert(valid_index(o));

            memberList[n] = memberList[o];
            if (memberList[n]) {
                memberList[n]->index = n;
            }
            memberList[o] = NULL;
        }
    }
    else if (length<0) { // shift list to the left, thereby not freeing any memory !
        e4_assert((start_index + length) >= 0); // invalid shift!

        for (ED4_index n = start_index + length; n < (no_of_members + length); n++) { // start shifting left
            ED4_index o = n-length; // Note: length is negative!

            e4_assert(valid_index(n));
            e4_assert(valid_index(o));

            memberList[n] = memberList[o];
            if (memberList[n]) {
                memberList[n]->index = n;
            }
            memberList[o] = NULL;
        }
    }
}


ED4_returncode ED4_container::move_member(ED4_index old_pos, ED4_index new_pos) {
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

ED4_index ED4_container::search_member(ED4_extension *location, ED4_properties prop)
// searches member_list of current object for a member who's extension falls within the given location
// thereby considering orientation given by prop (only pos[] is relevant)
// list has to be ordered in either x- or y- direction, ( binary search algorithm later )
// returns index of found member, -1 if list is empty or no_of_members if search reached end of list
{
    ED4_index   current_index = 0,
        rel_pos,
        rel_size;

    ED4_base *current_member;

    if (prop & ED4_P_HORIZONTAL) {   // set extension-indexes rel_pos and rel_size according to properties
        rel_pos = Y_POS; rel_size = HEIGHT;
    }
    else { // i.e. prop & ED4_P_VERTICAL
        rel_pos = X_POS; rel_size = WIDTH;
    }

    current_member = memberList[0]; // search list
    if (! current_member) { // there's no list
        return (-1);
    }

    current_index = 0;
    while (current_member) { // just as long as possibility exists, to find the object
        if (location->position[rel_pos] <= (current_member->extension.position[rel_pos] + current_member->extension.size[rel_size])) {  // found a suitable member
            return (current_index);
        }
        current_index++; // no hit => search on
        current_member = memberList[current_index];
    }

    return (no_of_members);                                                           // reached this position => no hit, return no_of_members
}

#ifdef ASSERTION_USED
int ED4_container::members_ok() const {
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

ED4_container::ED4_container(ED4_manager *the_owner) {
    my_owner      = the_owner;
    ARB_calloc(memberList, 1);
    // memberList[0] = NULL;
    no_of_members = 0;
    size_of_list  = 1;
}


ED4_container::~ED4_container() {
    while (members()) {
        ED4_index  last  = members()-1;
        ED4_base  *child = member(last);

        remove_member(child);
        child->parent = NULL;
        delete child;
    }
    free(memberList);
}

