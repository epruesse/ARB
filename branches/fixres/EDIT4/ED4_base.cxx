#include <arbdbt.h>
#include <ad_cb.h>

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <arb_progress.h>
#include <arb_strbuf.h>

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_list.hxx"

ED4_group_manager *ED4_base::is_in_folded_group() const
{
    if (!parent) return 0;
    ED4_base *group = get_parent(ED4_L_GROUP);
    if (!group) return 0;
    if (group->dynamic_prop & ED4_P_IS_FOLDED) return group->to_group_manager();
    return group->is_in_folded_group();
}

void ED4_base::changed_by_database()
{
    e4_assert(0);
    // this happens if you add a new species_pointer to a ED4_base-derived type
    // without defining changed_by_database for this type
}

void ED4_manager::changed_by_database() { request_refresh(); }

void ED4_terminal::changed_by_database()
{
    if (GB_read_clock(GLOBAL_gb_main) > curr_timestamp) { // only if timer_cb occurred after last change by EDIT4

        // test if alignment length has changed:
        {
            GBDATA *gb_alignment = GBT_get_alignment(GLOBAL_gb_main, ED4_ROOT->alignment_name);
            e4_assert(gb_alignment);
            GBDATA *gb_alignment_len = GB_search(gb_alignment, "alignment_len", GB_FIND);
            int alignment_length = GB_read_int(gb_alignment_len);

            if (MAXSEQUENCECHARACTERLENGTH!=alignment_length) {
                ED4_alignment_length_changed(gb_alignment_len, GB_CB_CHANGED);
            }
        }

        GBDATA *gb_seq = get_species_pointer();
        int type = GB_read_type(gb_seq);

        if (type==GB_STRING) {
            char *data = (char*)GB_read_old_value();
            if (data) {
                int data_len = GB_read_old_size();
                e4_assert(data_len >= 0);
                char *dup_data = new char[data_len+1];

                memcpy(dup_data, data, data_len);
                dup_data[data_len] = 0;

#if defined(DEBUG) && 0
                char *n = GB_read_string(gb_seq);
                e4_assert(strcmp(n, dup_data)!=0); // not really changed
                delete n;
#endif

                ED4_species_manager *spman = get_parent(ED4_L_SPECIES)->to_species_manager();
                spman->do_callbacks();

                if (dynamic_prop & ED4_P_CONSENSUS_RELEVANT) {
                    ED4_multi_species_manager *multiman = get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                    multiman->update_bases_and_rebuild_consensi(dup_data, data_len, spman, ED4_U_UP);
                    request_refresh();
                }

                delete [] dup_data;
            }
            else { // sth else changed (e.g. protection)
                GB_clear_error();
            }
        }
    }
}

void ED4_base::deleted_from_database() {
    my_species_pointer.notify_deleted();
}

void ED4_terminal::deleted_from_database() {
    ED4_base::deleted_from_database();
}

void ED4_text_terminal::deleted_from_database() {
    ED4_terminal::deleted_from_database();
    parent->Delete();
}

void ED4_sequence_terminal::deleted_from_database()
{
#if defined(DEBUG)
    printf("ED4_sequence_terminal::deleted_from_database (%p)\n", this);
#endif // DEBUG

    ED4_terminal::deleted_from_database();

    bool was_consensus_relevant = dynamic_prop & ED4_P_CONSENSUS_RELEVANT;

    clr_property(ED4_properties(ED4_P_CONSENSUS_RELEVANT|ED4_P_ALIGNMENT_DATA));

    if (was_consensus_relevant) { 
        const char *data     = (const char*)GB_read_old_value();
        int         data_len = GB_read_old_size();

        ED4_multi_species_manager *multi_species_man = get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

        multi_species_man->update_bases(data, data_len, 0);
        multi_species_man->rebuild_consensi(get_parent(ED4_L_SPECIES)->to_species_manager(), ED4_U_UP);
    }

    parent->Delete();
}

void ED4_manager::deleted_from_database() {
    if (is_species_manager()) {
        ED4_species_manager *species_man = to_species_manager();
        ED4_multi_species_manager *multi_man = species_man->parent->to_multi_species_manager();

        multi_man->children->remove_member(species_man);
        GB_push_transaction(GLOBAL_gb_main);
        multi_man->update_consensus(multi_man, 0, species_man);
        multi_man->rebuild_consensi(species_man, ED4_U_UP);
        GB_pop_transaction(GLOBAL_gb_main);

        request_resize();
        // parent = 0;
        // delete this; // @@@ crashes when removing callback deleted_from_database()
    }
    else {
        e4_assert(0);
    }
}

static void sequence_changed_cb(GBDATA *gb_seq, ED4_base *base, GB_CB_TYPE gbtype) {
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

ED4_species_pointer::ED4_species_pointer() {
    species_pointer = 0;
}
ED4_species_pointer::~ED4_species_pointer() {
    e4_assert(species_pointer==0);      // must be destroyed before
}

void ED4_species_pointer::addCallback(ED4_base *base) {
    GB_transaction ta(GLOBAL_gb_main);
    GB_add_callback(species_pointer, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sequence_changed_cb, base));
}
void ED4_species_pointer::removeCallback(ED4_base *base) {
    GB_transaction ta(GLOBAL_gb_main);
    GB_remove_callback(species_pointer, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sequence_changed_cb, base));
}

void ED4_species_pointer::Set(GBDATA *gbd, ED4_base *base) {
    if (species_pointer) removeCallback(base);
    species_pointer = gbd;
    if (species_pointer) addCallback(base);
}

// -----------------
//      ED4_base

inline bool ranges_overlap(int p1, int p2, int r1, int r2) {
    // return true if ranges p1..p2 and r1..r2 overlap
    e4_assert(p1 <= p2);
    e4_assert(r1 <= r2);

    return !((r2 <= p1) || (p2 <= r1)); // "exactly adjacent" means "not overlapping"
}

inline bool range_contained_in(int p1, int p2, int r1, int r2) {
    // return true if range p1..p2 is contained in range r1..r2
    e4_assert(p1 <= p2);
    e4_assert(r1 <= r2);

    return p1 >= r1 && p2 <= r2;
}

bool ED4_window::partly_shows(int x1, int y1, int x2, int y2) const {
    // return true if rectangle x1/y1/x2/y2 overlaps with clipped screen
    e4_assert(x1 <= x2);
    e4_assert(y1 <= y2);

    bool visible = (ranges_overlap(x1, x2, coords.window_left_clip_point, coords.window_right_clip_point) &&
                    ranges_overlap(y1, y2, coords.window_upper_clip_point, coords.window_lower_clip_point));

    return visible;
}

bool ED4_window::completely_shows(int x1, int y1, int x2, int y2) const {
    // return true if rectangle x1/y1/x2/y2 is contained in clipped screen
    e4_assert(x1 <= x2);
    e4_assert(y1 <= y2);

    bool visible = (range_contained_in(x1, x2, coords.window_left_clip_point, coords.window_right_clip_point) &&
                    range_contained_in(y1, y2, coords.window_upper_clip_point, coords.window_lower_clip_point));

    return visible;
}

char *ED4_base::resolve_pointer_to_string_copy(int *) const { return NULL; }
const char *ED4_base::resolve_pointer_to_char_pntr(int *) const { return NULL; }

ED4_returncode ED4_manager::create_group(ED4_group_manager **group_manager, GB_CSTR group_name) {
    // creates group from user menu of AW_Window

    char buffer[35];

    sprintf(buffer, "Group_Manager.%ld", ED4_counter);                                                          // create new group manager
    *group_manager = new ED4_group_manager(buffer, 0, 0, 0, 0, NULL);

    sprintf(buffer, "Bracket_Terminal.%ld", ED4_counter);
    ED4_bracket_terminal *bracket_terminal = new ED4_bracket_terminal(buffer, 0, 0, BRACKETWIDTH, 0, *group_manager);
    (*group_manager)->children->append_member(bracket_terminal);

    sprintf(buffer, "MultiSpecies_Manager.%ld", ED4_counter);                                                   // create new multi_species_manager
    ED4_multi_species_manager *multi_species_manager = new ED4_multi_species_manager(buffer, BRACKETWIDTH, 0, 0, 0, *group_manager); // Objekt Gruppen name_terminal noch
    (*group_manager)->children->append_member(multi_species_manager);                                           // auszeichnen

    (*group_manager)->set_property(ED4_P_MOVABLE);
    multi_species_manager->set_property(ED4_P_IS_HANDLE);
    bracket_terminal->set_property(ED4_P_IS_HANDLE);
    bracket_terminal->set_links(NULL, multi_species_manager);

    {
        sprintf(buffer, "Group_Spacer_Terminal_Beg.%ld", ED4_counter);                                                      // Spacer at beginning of group
        ED4_spacer_terminal *group_spacer_terminal1 = new ED4_spacer_terminal(buffer, true, 0, 0, 10, SPACERHEIGHT, multi_species_manager); // For better Overview
        multi_species_manager->children->append_member(group_spacer_terminal1);
    }

    {
        sprintf(buffer, "Consensus_Manager.%ld", ED4_counter);                                                     // Create competence terminal
        ED4_species_manager *species_manager = new ED4_species_manager(ED4_SP_CONSENSUS, buffer, 0, SPACERHEIGHT, 0, 0, multi_species_manager);
        species_manager->set_property(ED4_P_MOVABLE);
        multi_species_manager->children->append_member(species_manager);

        {
            ED4_species_name_terminal *species_name_terminal = new ED4_species_name_terminal(group_name, 0, 0, MAXSPECIESWIDTH - BRACKETWIDTH, TERMINALHEIGHT, species_manager);
            species_name_terminal->set_property((ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE));        // only some terminals
            species_name_terminal->set_links(NULL, ED4_ROOT->ref_terminals.get_ref_sequence());
            species_manager->children->append_member(species_name_terminal);                                                    // properties
        }

        sprintf(buffer, "Consensus_Seq_Manager.%ld", ED4_counter);
        ED4_sequence_manager *sequence_manager = new ED4_sequence_manager(buffer, MAXSPECIESWIDTH, 0, 0, 0, species_manager);
        sequence_manager->set_property(ED4_P_MOVABLE);
        species_manager->children->append_member(sequence_manager);

        {
            ED4_sequence_info_terminal *sequence_info_terminal = new ED4_sequence_info_terminal("DATA", 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, sequence_manager);        // Info fuer Gruppe
            sequence_info_terminal->set_links(ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence_info());
            sequence_info_terminal->set_property((ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE));
            sequence_manager->children->append_member(sequence_info_terminal);
        }

        {
            ED4_sequence_terminal *sequence_terminal = new ED4_consensus_sequence_terminal("", SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, sequence_manager);
            sequence_terminal->set_property(ED4_P_CURSOR_ALLOWED);
            sequence_terminal->set_links(ED4_ROOT->ref_terminals.get_ref_sequence(),   ED4_ROOT->ref_terminals.get_ref_sequence());
            sequence_manager->children->append_member(sequence_terminal);
        }
    }

    {
        sprintf(buffer, "Group_Spacer_Terminal_End.%ld", ED4_counter);                                                      // Spacer at beginning of group
        ED4_spacer_terminal *group_spacer_terminal2 = new ED4_spacer_terminal(buffer, true, 0, SPACERHEIGHT + TERMINALHEIGHT, 10, SPACERHEIGHT, multi_species_manager); // For better Overview
        multi_species_manager->children->append_member(group_spacer_terminal2);
    }

    multi_species_manager->update_requested_by_child();

    ED4_counter ++;

    return ED4_R_OK;
}

void ED4_base::generate_configuration_string(GBS_strstruct& buffer) {
    const char SEPARATOR = '\1';
    if (is_manager()) {
        ED4_members *childs = to_manager()->children;
        if (childs) {
            if (is_group_manager()) {
                buffer.put(SEPARATOR);
                buffer.put(dynamic_prop & ED4_P_IS_FOLDED ? 'F' : 'G');

                for (int writeConsensus = 1; writeConsensus>=0; --writeConsensus) {
                    for (int i=0; i<childs->members(); ++i) {
                        ED4_base *child       = childs->member(i);
                        bool      isConsensus = child->is_consensus_manager();

                        if (bool(writeConsensus) == isConsensus) {
                            child->generate_configuration_string(buffer);
                        }
                    }
                }

                buffer.put(SEPARATOR);
                buffer.put('E');
            }
            else {
                for (int i=0; i<childs->members(); i++) {
                    childs->member(i)->generate_configuration_string(buffer);
                }
            }
        }
    }
    else {
        if (is_species_name_terminal() && !((ED4_terminal*)this)->tflag.deleted) {
            ED4_species_type species_type = get_species_type();
            switch (species_type) {
                case ED4_SP_CONSENSUS: {
                    // write group name (control info already written inside manager-branch above)
                    const char *paren = strchr(id, '(');
                    if (paren) {
                        int namelen = std::max(int(paren-id)-1, 0); // skip starting at " (" behind consensus name

                        if (namelen>0) buffer.ncat(id, namelen);
                        else           buffer.cat("<unnamed>");
                    }
                    else buffer.cat(id);
                    break;
                }
                case ED4_SP_SAI:
                    buffer.put(SEPARATOR);
                    buffer.put('S');
                    buffer.cat(resolve_pointer_to_char_pntr(NULL));
                    break;
                case ED4_SP_SPECIES:
                    buffer.put(SEPARATOR);
                    buffer.put('L');
                    buffer.cat(resolve_pointer_to_char_pntr(NULL));
                    break;
                case ED4_SP_NONE:
                    e4_assert(0);
                    break;
            }
        }
    }
}


ARB_ERROR ED4_base::route_down_hierarchy(const ED4_route_cb& cb) {
    // executes 'cb' for every element in hierarchy
    return cb(this);
}

ARB_ERROR ED4_manager::route_down_hierarchy(const ED4_route_cb& cb) {
    ARB_ERROR error = cb(this);
    if (children && !error) {
        for (int i=0; i <children->members() && !error; i++) {
            error = children->member(i)->route_down_hierarchy(cb);
        }
    }
    return error;
}

ED4_base *ED4_manager::find_first_that(ED4_level level, const ED4_basePredicate& fulfills_predicate) {
    if ((spec.level&level) && fulfills_predicate(this)) {
        return this;
    }

    if (children) {
        for (int i=0; i<children->members(); i++) {
            ED4_base *child = children->member(i);

            if (child->is_manager()) {
                ED4_base *found = child->to_manager()->find_first_that(level, fulfills_predicate);
                if (found) {
                    return found;
                }
            }
            else if ((child->spec.level&level) && fulfills_predicate(child)) {
                return child;
            }
        }
    }

    return 0;
}

int ED4_base::calc_group_depth() {
    int       cntr        = 0;
    ED4_base *temp_parent = parent;
    while (temp_parent->parent && !(temp_parent->is_area_manager())) {
        if (temp_parent->is_group_manager()) cntr++;
        temp_parent = temp_parent->parent;
    }
    return cntr; // don't count our own group
}

ED4_returncode ED4_base::remove_callbacks() // removes callbacks
{
    return ED4_R_IMPOSSIBLE;
}


ED4_base *ED4_base::search_spec_child_rek(ED4_level level)   // recursive search for level
{
    return spec.level&level ? this : (ED4_base*)NULL;
}

ED4_base *ED4_manager::search_spec_child_rek(ED4_level level)
{
    if (spec.level & level) return this;

    if (children) {
        int i;

        for (i=0; i<children->members(); i++) { // first check children
            if (children->member(i)->spec.level & level) {
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


bool ED4_base::has_parent(ED4_manager *Parent)
{
    // return true if 'parent' is a parent of this

    if (is_manager()) {
        if (this == static_cast<ED4_base*>(Parent)) {
            return true;
        }
    }

    if (!parent) return false;
    return parent->has_parent(Parent);
}


ED4_AREA_LEVEL ED4_base::get_area_level(ED4_multi_species_manager **multi_species_manager) const {
    ED4_base       *area_base = get_parent(ED4_L_AREA);
    ED4_AREA_LEVEL  result    = ED4_A_ERROR;

    if (area_base) {
        ED4_area_manager *area_man = area_base->to_area_manager();

        if      (area_man == ED4_ROOT->top_area_man)    result = ED4_A_TOP_AREA;
        else if (area_man == ED4_ROOT->middle_area_man) result = ED4_A_MIDDLE_AREA;

        if (result != ED4_A_ERROR && multi_species_manager) {
            *multi_species_manager = area_man->get_multi_species_manager();
        }
    }
    return result;
}


void ED4_multi_species_manager::update_group_id() {
    ED4_species_name_terminal *consensus_name_terminal = get_consensus_name_terminal();
    if (consensus_name_terminal) { // top managers dont show consensus
        e4_assert(has_valid_counters());
        
        const char *cntid = consensus_name_terminal->id;
        char       *name  = (char*)ARB_calloc(strlen(cntid)+10, sizeof(*name));

        int i;
        for (i=0; cntid[i] && cntid[i] != '(';   i++) {
            name[i] = cntid[i];
        }
        if (i>0 && cntid[i-1] == ' ') --i; // skip terminal space
        sprintf(name+i, " (%d)", species);

        freeset(consensus_name_terminal->id, name);

        consensus_name_terminal->request_refresh();
    }
}

PosRange ED4_abstract_sequence_terminal::pixel2index(PosRange pixel_range) {
    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    int left_index  = int((pixel_range.start()-CHARACTEROFFSET)/length_of_char);
    int right_index = int((pixel_range.end()  -CHARACTEROFFSET)/length_of_char) + 1;

    return PosRange(left_index, std::min(right_index, MAXSEQUENCECHARACTERLENGTH-1));
}

PosRange ED4_abstract_sequence_terminal::calc_interval_displayed_in_rectangle(AW_screen_area *rect) { // rect contains win-coords
    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    int rel_left_x  = int(rect->l-x);
    int rel_right_x = int(rect->r-x);

    return pixel2index(PosRange(rel_left_x, rel_right_x)); // changed behavior: clip at MAXSEQUENCECHARACTERLENGTH-1 (was MAXSEQUENCECHARACTERLENGTH)
}

PosRange ED4_abstract_sequence_terminal::calc_update_interval() {
    AW_pos x, y;
    calc_world_coords(&x, &y);

    const AW_screen_area& clip_rect = current_device()->get_cliprect();

    int scroll_shift = current_ed4w()->coords.window_left_clip_point-x; // Verschiebung der Sequenz (durch Scrollen) == slider Position
    int rel_left_x   = int(clip_rect.l - x + scroll_shift);                   // Abstand vom linken Terminalrand zum Anfang des Clipping rectangles + scroll_shift
    int rel_right_x  = int(clip_rect.r - x + scroll_shift);

    return pixel2index(PosRange(rel_left_x, rel_right_x));
}

void ED4_manager::create_consensus(ED4_abstract_group_manager *upper_group_manager, arb_progress *progress) {
    // creates consensus
    // is called by group manager

    ED4_abstract_group_manager *group_manager_for_child = upper_group_manager;

    if (is_abstract_group_manager()) {
        ED4_abstract_group_manager *group_manager = to_abstract_group_manager();

        group_manager->table().init(MAXSEQUENCECHARACTERLENGTH);
        group_manager_for_child = group_manager;

        if (progress) progress->inc();
    }
    int i;
    for (i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);

        if (member->is_species_manager()) {
            ED4_species_manager *species_manager        = member->to_species_manager();
            const ED4_terminal  *sequence_data_terminal = species_manager->get_consensus_relevant_terminal();

            if (sequence_data_terminal) {
                int   db_pointer_len;
                char *db_pointer = sequence_data_terminal->resolve_pointer_to_string_copy(&db_pointer_len);
                group_manager_for_child->table().add(db_pointer, db_pointer_len);
                e4_assert(!group_manager_for_child->table().empty());
                free(db_pointer);

                if (progress) progress->inc();
            }
        }
        else if (member->is_group_manager()) {
            ED4_group_manager *sub_group = member->to_group_manager();

            sub_group->create_consensus(sub_group, progress);
            e4_assert(sub_group!=upper_group_manager);
            upper_group_manager->table().add(sub_group->table());
#if defined(TEST_CHAR_TABLE_INTEGRITY)
            if (!sub_group->table().empty() && !sub_group->table().is_ignored()) {
                e4_assert(!upper_group_manager->table().empty());
            }
#endif
        }
        else if (member->is_manager()) {
            member->to_manager()->create_consensus(group_manager_for_child, progress);
        }
    }
}

const ED4_terminal *ED4_base::get_consensus_relevant_terminal() const {
    int i;

    if (is_terminal()) {
        if (dynamic_prop & ED4_P_CONSENSUS_RELEVANT) {
            return this->to_terminal();
        }
        return NULL;
    }

    const ED4_manager  *manager           = this->to_manager();
    const ED4_terminal *relevant_terminal = 0;

    int members = manager->children->members();

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
    int counter = 0;

    for (int i=0; i<children->members(); i++) {
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
                ED4_multi_species_manager *multi_species_manager = group_manager->get_multi_species_manager();
                counter += multi_species_manager->count_visible_children();
            }
        }
    }
    return counter;
}



ED4_base *ED4_base::get_parent(ED4_level lev) const
{
    ED4_base *temp_parent = this->parent;

    while (temp_parent && !(temp_parent->spec.level & lev)) {
        temp_parent = temp_parent->parent;
    }

    return temp_parent;
}

void ED4_base::unlink_from_parent() {
    e4_assert(parent);
    parent->children->remove_member(this);
}

char *ED4_base::get_name_of_species() {
    char                *name        = 0;
    ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();
    if (species_man) {
        ED4_species_name_terminal *species_name = species_man->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
        if (species_name) {
            GBDATA *gb_name   = species_name->get_species_pointer();
            if (gb_name) {
                GB_transaction ta(gb_name);
                name = GB_read_as_string(gb_name);
            }
        }
    }
    return name;
}

ED4_base *ED4_manager::get_defined_level(ED4_level lev) const
{
    int i;

    for (i=0; i<children->members(); i++) { // first make a complete search in myself
        if (children->member(i)->spec.level & lev) {
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
    return NULL;                                                                // nothing found
}

ED4_returncode ED4_base::set_width() {
    // sets object length of terminals to Consensus_Name_terminal if existing
    // else to MAXSPECIESWIDTH

    if (is_species_manager()) {
        ED4_species_manager *species_manager = to_species_manager();

        if (!species_manager->is_consensus_manager()) {
            ED4_multi_name_manager    *multi_name_manager = species_manager->get_defined_level(ED4_L_MULTI_NAME)->to_multi_name_manager();  // case I'm a species
            ED4_species_name_terminal *consensus_terminal = parent->to_multi_species_manager()->get_consensus_name_terminal();

            for (int i=0; i < multi_name_manager->children->members(); i++) {
                ED4_name_manager *name_manager = multi_name_manager->children->member(i)->to_name_manager();
                ED4_base         *nameTerm     = name_manager->children->member(0);
                int               width        = consensus_terminal ? consensus_terminal->extension.size[WIDTH] : MAXSPECIESWIDTH;

                nameTerm->extension.size[WIDTH] = width;
                nameTerm->request_resize();
            }

            for (int i=0; i < species_manager->children->members(); i++) { // adjust all managers as far as possible
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
                smember->request_resize();
            }
        }
    }
    else if (is_group_manager()) {
        ED4_group_manager         *group_manager           = to_group_manager();
        ED4_multi_species_manager *multi_species_manager   = group_manager->get_multi_species_manager();
        ED4_species_name_terminal *mark_consensus_terminal = multi_species_manager->get_consensus_name_terminal();
        ED4_species_name_terminal *consensus_terminal      = parent->to_multi_species_manager()->get_consensus_name_terminal();

        if (consensus_terminal) { // we're a group in another group
            mark_consensus_terminal->extension.size[WIDTH] = consensus_terminal->extension.size[WIDTH] - BRACKETWIDTH;
        }
        else { // we're at the top (no consensus terminal)
            mark_consensus_terminal->extension.size[WIDTH] = MAXSPECIESWIDTH - BRACKETWIDTH;
        }

        mark_consensus_terminal->request_resize();

        for (int i=0; i < multi_species_manager->children->members(); i++) {
            multi_species_manager->children->member(i)->set_width();
        }

        for (int i=0; i < group_manager->children->members(); i++) { // for all groups below from us
            if (group_manager->children->member(i)->is_group_manager()) {
                group_manager->children->member(i)->set_width();
            }
        }
    }

    return ED4_R_OK;
}


short ED4_base::in_border(AW_pos x, AW_pos y, ED4_movemode mode)                                // determines if given world coords x and y
{                                                                                               // are within borders of current object according to move mode
    AW_pos    world_x, world_y;

    calc_world_coords(&world_x, &world_y);                                              // calculate absolute extension of current object

    switch (mode)                                                                               // which direction ?
    {
        case ED4_M_HORIZONTAL:
            {
                if ((x >= world_x) && (x < (world_x + extension.size[WIDTH])))
                    return (1);                                                 // target location is within the borders of parent
                break;
            }
        case ED4_M_VERTICAL:
            {
                if ((y >= world_y) && (y < (world_y + extension.size[HEIGHT])))
                    return (1);                                                 // target location is within the borders of parent
                break;
            }
        case ED4_M_FREE:
            {
                return (in_border(x, y, ED4_M_HORIZONTAL) && in_border(x, y, ED4_M_VERTICAL));
            }
        case ED4_M_NO_MOVE:
            {
                break;
            }
    }

    return (0);
}


void ED4_base::calc_rel_coords(AW_pos *x, AW_pos *y) // calculates coordinates relative to current object from given world coords
{
    AW_pos   world_x, world_y;

    calc_world_coords(&world_x, &world_y);          // calculate world coordinates of current object

    *x -= world_x;                                  // calculate relative coordinates by subtracting world
    *y -= world_y;                                  // coords of current object
}


ED4_returncode  ED4_base::event_sent_by_parent(AW_event * /* event */, AW_window * /* aww */)
{
    return (ED4_R_OK);
}

void ED4_manager::hide_children() {
    for (int i=0; i<children->members(); i++) {
        ED4_base *member = children->member(i);
        if (!member->is_spacer_terminal() && !member->is_consensus_manager()) { // don't hide spacer and Consensus
            member->flag.hidden = 1;
        }
    }
    request_resize();
}


void ED4_manager::unhide_children() {
    if (children) {
        for (int i=0; i < children->members(); i++) {
            children->member(i)->flag.hidden = 0; // make child visible
        }
        request_resize();
    }
}

void ED4_bracket_terminal::unfold() {
    if (parent) {
        for (int i=0; i < parent->children->members(); i++) {
            ED4_base *member = parent->children->member(i);

            if (member->is_multi_species_manager()) {
                ED4_multi_species_manager *multi_species_manager = member->to_multi_species_manager();
                multi_species_manager->unhide_children();
                multi_species_manager->clr_property(ED4_P_IS_FOLDED);

                ED4_spacer_terminal *spacer = multi_species_manager->get_defined_level(ED4_L_SPACER)->to_spacer_terminal();
                spacer->extension.size[HEIGHT] = SPACERHEIGHT;
            }
        }

        clr_property(ED4_P_IS_FOLDED);
        parent->clr_property(ED4_P_IS_FOLDED);
    }
}

void ED4_bracket_terminal::fold() {
    if (parent) {
        ED4_multi_species_manager *multi_species_manager = parent->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

        int consensus_shown = 0;
        if (!(multi_species_manager->children->member(1)->is_consensus_manager())) { // if consensus is not a top = > move to top
            ED4_members *multi_children    = multi_species_manager->children;
            ED4_manager *consensus_manager = NULL;

            int i;
            for (i=0; i<multi_children->members(); i++) { // search for consensus
                if (multi_children->member(i)->is_consensus_manager()) {
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
        else {
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
        multi_species_manager->set_property(ED4_P_IS_FOLDED);

        set_property(ED4_P_IS_FOLDED);
        parent->set_property(ED4_P_IS_FOLDED);
    }
}

void ED4_base::check_all()
{
    AW_pos x, y;

    calc_world_coords(&x, &y);

    printf("Typ des Aufrufers :\t\t\t%s\n", is_manager() ? "Manager" : "Terminal");
    printf("Name des Aufrufers von Check_All : \t%.30s\n", (id) ? id : "Keine ID");
    printf("Linke obere Ecke x, y : \t\t%f, %f\n", extension.position[0], extension.position[1]);
    printf("Breite und Hoehe x, y : \t\t%f, %f\n", extension.size[0], extension.size[1]);
    printf("World Coords     x, y : \t\t%f, %f\n\n", x, y);
    printf("***********************************************\n\n");
}

int ED4_base::adjust_clipping_rectangle() {
    // return 0 if clipping rectangle disappeared (nothing left to draw)
    AW::Rectangle base_area = get_win_area(current_ed4w());
    return current_device()->reduceClipBorders(base_area.top(), base_area.bottom(), base_area.left(), base_area.right());
}

void ED4_base::set_links(ED4_base *new_width, ED4_base *new_height) {
    // sets links in hierarchy :
    // width-link sets links between objects on same level
    // height-link sets links between objects on different levels

    if (new_width) {
        if (width_link) width_link->linked_objects->remove_elem(this);
        width_link = new_width;
        if (!new_width->linked_objects) new_width->linked_objects = new ED4_base_list;
        new_width->linked_objects->append_elem(this);
    }

    if (new_height) {
        if (height_link) height_link->linked_objects->remove_elem(this);
        height_link = new_height;
        if (!new_height->linked_objects) new_height->linked_objects = new ED4_base_list;
        new_height->linked_objects->append_elem(this);
    }
}

int ED4_base::currTimestamp = 1;

#if defined(DEBUG)
// #define VISIBLE_AREA_REFRESH
#endif

ED4_returncode ED4_base::clear_background(int color) {
    if (current_device()) { // @@@ should clear be done for all windows?
        AW_pos x, y;
        calc_world_coords(&x, &y);
        current_ed4w()->world_to_win_coords(&x, &y);

        current_device()->push_clip_scale();
        if (adjust_clipping_rectangle()) {
            if (!color) {
#if defined(VISIBLE_AREA_REFRESH)
                // for debugging draw each clear in different color:
                static int gc_area = ED4_G_FIRST_COLOR_GROUP;

                current_device()->box(gc_area, true, x, y, extension.size[WIDTH], extension.size[HEIGHT]);
                gc_area = (gc_area == ED4_G_LAST_COLOR_GROUP) ? ED4_G_FIRST_COLOR_GROUP : gc_area+1;
#else // !defined(VISIBLE_AREA_REFRESH)
                current_device()->clear_part(x, y, extension.size[WIDTH], extension.size[HEIGHT], AW_ALL_DEVICES);
#endif
            }
            else {
                // fill range with color for debugging
                current_device()->box(color, AW::FillStyle::SOLID, x, y, extension.size[WIDTH], extension.size[HEIGHT]);
            }
        }
        current_device()->pop_clip_scale();
    }
    return (ED4_R_OK);
}

void ED4_main_manager::clear_whole_background() {
    // clear AW_MIDDLE_AREA
    for (ED4_window *window = ED4_ROOT->first_window; window; window=window->next) {
        AW_device *device = window->get_device();
        if (device) {
            device->push_clip_scale();
            device->clear(AW_ALL_DEVICES);
            device->pop_clip_scale();
        }
    }
}

void ED4_base::draw_bb(int color) {
    if (current_device()) {
        current_device()->push_clip_scale();
        if (adjust_clipping_rectangle()) {
            AW_pos x1, y1;
            calc_world_coords(&x1, &y1);
            current_ed4w()->world_to_win_coords(&x1, &y1);
            current_device()->box(color, AW::FillStyle::EMPTY, x1, y1, extension.size[WIDTH]-1, extension.size[HEIGHT]-1);
        }
        current_device()->pop_clip_scale();
    }
}

ED4_base::ED4_base(const ED4_objspec& spec_, GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : spec(spec_)
{
    index = 0;
    dynamic_prop = ED4_P_NO_PROP;
    timestamp =  0; // invalid - almost always..

    e4_assert(temp_id);
    if (temp_id) {
        id = (char*)malloc(strlen(temp_id)+1);
        strcpy(id, temp_id);
    }

    linked_objects = NULL;

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


ED4_base::~ED4_base() {
    // before calling this function the first time, parent has to be set NULL
    e4_assert(!parent); // unlink from parent first!

    if (linked_objects) {
        ED4_base_list_elem *list_elem = linked_objects->head();
        while (list_elem) {
            ED4_base *object = list_elem->elem();
            if (object->width_link == this) {
                object->width_link->linked_objects->remove_elem(this);              // delete link and
                object->width_link = NULL;
            }

            if (object->height_link == this) {
                object->height_link->linked_objects->remove_elem(this);             // delete link and
                object->height_link = NULL;
            }

            ED4_base_list_elem *old_elem = list_elem;
            list_elem = list_elem->next();
            linked_objects->remove_elem(old_elem->elem());
        }
        delete linked_objects;
    }

    if (update_info.linked_to_scrolled_rectangle)
    {
        if (ED4_ROOT->main_manager)
        {
            ED4_base *sequence_terminal = ED4_ROOT->main_manager->search_spec_child_rek(ED4_L_SEQUENCE_STRING);

            if (sequence_terminal)
                sequence_terminal->update_info.linked_to_scrolled_rectangle = 1;

            update_info.linked_to_scrolled_rectangle = 0;
            ED4_ROOT->scroll_links.link_for_hor_slider = sequence_terminal;

            ED4_window *ed4w = ED4_ROOT->first_window;
            while (ed4w != NULL) {
                ed4w->scrolled_rect.replace_x_width_link_to(this, sequence_terminal);
                ed4w = ed4w->next;
            }
        }
    }

    if (width_link) {
        width_link->linked_objects->remove_elem(this);
        width_link = NULL;
    }

    if (height_link) {
        height_link->linked_objects->remove_elem(this);
        height_link = NULL;
    }

    set_species_pointer(0);     // clear pointer to database and remove callbacks
    free(id);
}

