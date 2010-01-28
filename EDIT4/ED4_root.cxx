// =============================================================== //
//                                                                 //
//   File      : ED4_root.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_tools.hxx"
#include "ed4_block.hxx"
#include "ed4_dots.hxx"
#include "ed4_nds.hxx"
#include "ed4_secedit.hxx"
#include "ed4_visualizeSAI.hxx"
#include "ed4_naligner.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"
#include "graph_aligner_gui.hxx"

#if defined(ARB_OPENGL)
#include "ed4_RNA3D.hxx"
#endif // ARB_OPENGL

#include <aw_awars.hxx>
#include <aw_preset.hxx>

#include <awt.hxx>
#include <awt_seq_colors.hxx>
#include <awt_map_key.hxx>

#include <fast_aligner.hxx>

#include <arb_version.h>

#include <AW_helix.hxx>
#include <st_window.hxx>
#include <gde.hxx>

#include <ed4_extern.hxx>

#include <cctype>

AW_window *AWTC_create_island_hopping_window(AW_root *root, AW_CL);

ED4_returncode ED4_root::refresh_window_simple(int redraw)
{
    e4_assert(!main_manager->update_info.delete_requested);
    e4_assert(!main_manager->update_info.resize);

    int refresh_all = 0;
    if (redraw) {
        main_manager->update_info.set_clear_at_refresh(1);
        refresh_all = 1;
    }
    main_manager->Show(refresh_all, 0);
    return (ED4_R_OK);
}

ED4_returncode ED4_root::refresh_window(int redraw) // this function should only be used for window specific updates (i.e. cursor placement)
{
    if (main_manager->update_info.delete_requested) {
        main_manager->delete_requested_children();
        redraw = 1;
    }

    while (main_manager->update_info.resize) {
        main_manager->resize_requested_by_parent();
        redraw = 1;
    }

    return refresh_window_simple(redraw);
}

ED4_returncode ED4_root::refresh_all_windows(int redraw)
{
    ED4_window *old_window = get_ed4w();

    last_window_reached = 0;

    if (main_manager->update_info.delete_requested) {
        main_manager->delete_requested_children();
        redraw = 1;
    }

    while (main_manager->update_info.resize) {
        main_manager->resize_requested_by_parent();
        redraw = 1;
    }

    ED4_window *window = first_window;
    GB_transaction dummy(GLOBAL_gb_main);
    while (window) {
        if (!window->next) last_window_reached = 1;
        use_window(window);
        refresh_window_simple(redraw);
        window = window->next;
    }

    use_window(old_window);

    return (ED4_R_OK);
}


ED4_returncode ED4_root::win_to_world_coords(AW_window *aww, AW_pos *x, AW_pos *y)              // calculates transformation from window to
{                                                   // world coordinates in a given window
    ED4_window        *current_window;
    ED4_folding_line  *current_fl;
    AW_pos             temp_x, temp_y;

    current_window = first_window->get_matching_ed4w(aww);

    if (current_window == NULL)
        return (ED4_R_WARNING);

    temp_x = *x;
    temp_y = *y;

    current_fl = current_window->vertical_fl;                           // calculate x-offset due to vertical folding lines
    while ((current_fl != NULL) && (*x >= current_fl->world_pos[X_POS]))
    {
        if ((current_fl->length == INFINITE) ||
                ((*y >= current_fl->window_pos[Y_POS]) && (*y <= (current_fl->window_pos[Y_POS] + current_fl->length))))
            temp_x += current_fl->dimension;

        current_fl = current_fl->next;
    }

    current_fl = current_window->horizontal_fl;                         // calculate y-offset due to horizontal folding lines
    while ((current_fl != NULL) && (*y >= current_fl->world_pos[Y_POS]))
    {
        if ((current_fl->length == INFINITE) ||
                  ((*x >= current_fl->window_pos[X_POS]) && (*x <= (current_fl->window_pos[X_POS] + current_fl->length))))
            temp_y += current_fl->dimension;

        current_fl = current_fl->next;
    }

    *x = temp_x;
    *y = temp_y;

    return (ED4_R_OK);
}


short ED4_root::is_primary_selection(ED4_terminal *object)
{
    ED4_list_elem        *tmp_elem;
    ED4_selection_entry  *tmp_entry;

    tmp_elem = (ED4_list_elem *) selected_objects.last();
    if (!tmp_elem) return 0;

    tmp_entry = (ED4_selection_entry *) tmp_elem->elem();
    return (tmp_entry != NULL) && (tmp_entry->object == object);
}

ED4_returncode ED4_root::deselect_all()
{
    ED4_multi_species_manager *main_multi_man = middle_area_man->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

    main_multi_man->deselect_all_species();
    main_multi_man = top_area_man->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
    main_multi_man->deselect_all_species();

    return ED4_R_OK;
}

ED4_returncode  ED4_root::remove_from_selected(ED4_terminal *object)
{
    if (object == NULL)
        return (ED4_R_IMPOSSIBLE);

    if ((selected_objects.is_elem((void *) object->selection_info))) {
        selected_objects.delete_elem((void *) object->selection_info);

        delete object->selection_info;
        object->selection_info = NULL;
        object->flag.selected = 0;
        object->flag.dragged = 0;

        if (object->is_species_name_terminal()) {
            ED4_species_name_terminal *name_term = object->to_species_name_terminal();

#ifdef DEBUG
            GBDATA *gbd = name_term->get_species_pointer();

            if (gbd) {
                printf("removed term '%s'\n", GB_read_char_pntr(gbd));
            }
            else {
                ED4_species_manager *spec_man = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();

                if (spec_man->flag.is_consensus) {
                    printf("removed consensus '%s'\n", name_term->id);
                }
                else {
                    printf("removed unknown term '%s'\n", name_term->id ? name_term->id : "NULL");
                }
            }
#endif

            name_term->set_refresh(1);
            name_term->parent->refresh_requested_by_child();

            ED4_sequence_terminal *seq_term = object->to_species_name_terminal()->corresponding_sequence_terminal();
            if (seq_term) {
                seq_term->set_refresh(1);
                seq_term->parent->refresh_requested_by_child();
            }

            // ProtView: Refresh corresponding AA_sequence terminals
            if (alignment_type == GB_AT_DNA) {
                PV_CallBackFunction(this->aw_root);
            }

            ED4_multi_species_manager *multi_man = object->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_man->invalidate_species_counters();
        }
        else {
            object->set_refresh();
            object->parent->refresh_requested_by_child();
        }
    }

    return (ED4_R_OK);
}

void ED4_root::announce_deletion(ED4_base *object) {
    // remove any links which might point to the object

    ED4_cursor& cursor = get_ed4w()->cursor;
    if (cursor.owner_of_cursor == object) { // about to delete owner_of_cursor
        cursor.HideCursor();
        e4_assert(cursor.owner_of_cursor != object);
    }
}


ED4_returncode ED4_root::add_to_selected(ED4_terminal *object)
{
    ED4_base            *tmp_object;
    ED4_level            mlevel;
    ED4_terminal        *sel_object;
    ED4_selection_entry *sel_info;


    if ((object == NULL) || !(object->dynamic_prop & ED4_P_SELECTABLE)) {   // check if object exists and may be selected
        return (ED4_R_IMPOSSIBLE);
    }

    if (selected_objects.no_of_entries() > 0) {   // check if object is of the same type as previously selected objects
        sel_info = (ED4_selection_entry *) selected_objects.first()->elem();
        sel_object = sel_info->object;
        if (object->spec->level != sel_object->spec->level) {   // object levels are different
            return (ED4_R_IMPOSSIBLE);
        }
    }


    if (!(selected_objects.is_elem((void *) object->selection_info))) {     // object is really new to our list => calculate current extension and append it
        object->selection_info = new ED4_selection_entry;

        if (object->dynamic_prop & ED4_P_IS_HANDLE)                 // object is a handle for an object up in the hierarchy => search it
        {
            mlevel = object->spec->handled_level;
            tmp_object = object;

            while ((tmp_object != NULL) && !(tmp_object->spec->level & mlevel)) {
                tmp_object = tmp_object->parent;
            }

            if (tmp_object == NULL) {
                return (ED4_R_WARNING);                 // no target level found
            }

            object->selection_info->actual_width = tmp_object->extension.size[WIDTH];
            object->selection_info->actual_height = tmp_object->extension.size[HEIGHT];
        }
        else                                    // selected object is no handle => take it directly
        {
            object->selection_info->actual_width = object->extension.size[WIDTH];
            object->selection_info->actual_height = object->extension.size[HEIGHT];
        }

        object->selection_info->drag_old_x = 0;
        object->selection_info->drag_old_y = 0;
        object->selection_info->drag_off_x = 0;
        object->selection_info->drag_off_y = 0;
        object->selection_info->old_event_y = 0;
        object->selection_info->object = object;
        selected_objects.append_elem_backwards((void *) object->selection_info);
        object->flag.selected = 1;

        if (object->is_species_name_terminal()) {
            ED4_species_name_terminal *name_term = object->to_species_name_terminal();

#ifdef DEBUG
            GBDATA *gbd = name_term->get_species_pointer();

            if (gbd) {
                printf("added term '%s'\n", GB_read_char_pntr(gbd));
            }
            else {
                ED4_species_manager *spec_man = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
                if (spec_man->flag.is_consensus) {
                    printf("added consensus '%s'\n", name_term->id);
                }
                else {
                    printf("added unknown term '%s'\n", name_term->id ? name_term->id : "NULL");
                }
            }
#endif

            name_term->set_refresh();
            name_term->parent->refresh_requested_by_child();

            ED4_sequence_terminal *seq_term = object->to_species_name_terminal()->corresponding_sequence_terminal();
            if (seq_term) {
                seq_term->set_refresh();
                seq_term->parent->refresh_requested_by_child();
            }

            // ProtView: Refresh corresponding AA_sequence terminals
            if (alignment_type == GB_AT_DNA) {
                PV_CallBackFunction(this->aw_root);
            }

            ED4_multi_species_manager *multi_man = object->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_man->invalidate_species_counters();
        }
        else {
            e4_assert(0); // test ob ueberhaupt was anderes als species_name_terminals verwendet wird
            object->set_refresh();
            object->parent->refresh_requested_by_child();
        }

        return (ED4_R_OK);
    }

    return (ED4_R_IMPOSSIBLE);
}

ED4_returncode ED4_root::resize_all()
{
    while (main_manager->update_info.resize) {
        main_manager->resize_requested_by_parent();
    }

    return (ED4_R_OK);
}

static ARB_ERROR change_char_table_length(ED4_base *base, AW_CL new_length) {
    if (base->is_group_manager()) {
        ED4_group_manager *group_man = base->to_group_manager();
        group_man->table().change_table_length(new_length);
    }
    return NULL;
}

void ED4_alignment_length_changed(GBDATA *gb_alignment_len, int * /* cl */, GB_CB_TYPE IF_DEBUG(gbtype)) // callback from database
{
    e4_assert(gbtype==GB_CB_CHANGED);
    int new_length = GB_read_int(gb_alignment_len);

#if defined(DEBUG) && 1
    printf("alignment_length_changed from %i to %i\n", MAXSEQUENCECHARACTERLENGTH, new_length);
#endif

    if (MAXSEQUENCECHARACTERLENGTH!=new_length) { // otherwise we already did this (i.e. we were called by changed_by_database)
        bool was_increased = new_length>MAXSEQUENCECHARACTERLENGTH;

        MAXSEQUENCECHARACTERLENGTH = new_length;

        const char *err = ED4_ROOT->helix->init(GLOBAL_gb_main); // reload helix
        if (err) { aw_message(err); err = 0; }

        err = ED4_ROOT->ecoli_ref->init(GLOBAL_gb_main); // reload ecoli
        if (err) { aw_message(err); err = 0; }

        if (ED4_ROOT->alignment_type == GB_AT_AA) {
            // TODO: is this needed here?
            err = ED4_pfold_set_SAI(&ED4_ROOT->protstruct, GLOBAL_gb_main, ED4_ROOT->alignment_name, &ED4_ROOT->protstruct_len); // reload protstruct
            if (err) { aw_message(err); err = 0; }
        }

        if (was_increased) {
            ED4_ROOT->main_manager->route_down_hierarchy(change_char_table_length, new_length).expect_no_error();
            ED4_ROOT->root_group_man->remap()->mark_compile_needed_force();
        }
    }
}

ED4_returncode ED4_root::init_alignment() {
    GB_transaction dummy(GLOBAL_gb_main);

    alignment_name = GBT_get_default_alignment(GLOBAL_gb_main);
    alignment_type = GBT_get_alignment_type(GLOBAL_gb_main, alignment_name);
    if (alignment_type==GB_AT_UNKNOWN) {
        aw_popup_exit("You have to select a valid alignment before you can start ARB_EDIT4");
    }

    GBDATA *gb_alignment = GBT_get_alignment(GLOBAL_gb_main, alignment_name);
    if (!gb_alignment) aw_popup_exit("You can't edit without an existing alignment");

    GBDATA *gb_alignment_len = GB_search(gb_alignment, "alignment_len", GB_FIND);
    int alignment_length = GB_read_int(gb_alignment_len);
    MAXSEQUENCECHARACTERLENGTH = alignment_length;

    GB_add_callback(gb_alignment_len, (GB_CB_TYPE)GB_CB_CHANGED, (GB_CB)ED4_alignment_length_changed, 0);

    aw_root->awar_string(AWAR_EDITOR_ALIGNMENT, alignment_name);

    return ED4_R_OK;
}

void ED4_root::recalc_font_group() {
    font_group.unregisterAll();
    for (int f=ED4_G_FIRST_FONT; f<=ED4_G_LAST_FONT; f++) {
        font_group.registerFont(get_device(), f);
    }
}

ED4_returncode ED4_root::create_hierarchy(char *area_string_middle, char *area_string_top) // creates internal hierarchy of editor
{
    int index = 0, x = 0, change = 0;
    ED4_index y = 0, help = 0;
    ED4_base *x_link, *y_link, *width_link, *height_link;
    ED4_window *new_window;
    long total_no_of_species, total_no_of_groups, group_count, species_count;

    // count species and related info (e.g. helix) displayed in the top region
    database->calc_no_of_all(area_string_top, &group_count, &species_count);
    total_no_of_groups = group_count;
    total_no_of_species = species_count;

    // count no of species and sais including groups in middle region
    database->calc_no_of_all(area_string_middle, &group_count, &species_count);
    total_no_of_groups += group_count;
    total_no_of_species += species_count;

    aw_status_counter species_progress(total_no_of_species);

    GB_push_transaction(GLOBAL_gb_main);

    ecoli_ref = new BI_ecoli_ref();
    ecoli_ref->init(GLOBAL_gb_main);

    // [former position of ali-init-code]

    main_manager = new ED4_main_manager  ("Main_Manager",    0, 0, 0, 0, NULL);            // build a test hierarchy
    root_group_man = new ED4_root_group_manager ("Root_Group_Manager", 0, 0, 0, 0,   main_manager);
    main_manager->children->append_member (root_group_man);

    ED4_device_manager *device_manager = new ED4_device_manager("Device_Manager", 0, 0, 0, 0, root_group_man);
    root_group_man->children->append_member(device_manager);

    ED4_calc_terminal_extentions();

    {
        int col_stat_term_height = 50; // @@@ Hoehe des ColumnStatistics Terminals ausrechnen

        ref_terminals.init(new ED4_sequence_info_terminal("Reference_Sequence_Info_Terminal", /* NULL, */ 250, 0, MAXINFOWIDTH, TERMINALHEIGHT, NULL),
                           new ED4_sequence_terminal("Reference_Sequence_Terminal", 300, 0, 300, TERMINALHEIGHT, NULL),
                           new ED4_sequence_info_terminal("Reference_ColumnStatistics_Info_Terminal", /* NULL, */ 250, 0, MAXINFOWIDTH, col_stat_term_height, NULL),
                           new ED4_columnStat_terminal("Reference_ColumnStatistics_Terminal", 300, 0, 300, col_stat_term_height, NULL));
    }
    x = 100;

    recalc_font_group();

    {
        ED4_area_manager *middle_area_manager;
        ED4_tree_terminal *tree_terminal;
        ED4_multi_species_manager *top_multi_species_manager;
        ED4_multi_species_manager *mid_multi_species_manager;
        ED4_spacer_terminal *top_spacer_terminal;
        ED4_spacer_terminal *top_mid_spacer_terminal;
        ED4_spacer_terminal *top_multi_spacer_terminal_beg;
        ED4_spacer_terminal *mid_multi_spacer_terminal_beg;
        ED4_line_terminal *top_mid_line_terminal;
        ED4_line_terminal *mid_bot_line_terminal;
        ED4_spacer_terminal *total_bottom_spacer;

        // ********** Top Area beginning **********

        {
            ED4_area_manager *top_area_manager = new ED4_area_manager("Top_Area_Manager", 0, y, 0, 0, device_manager);
            device_manager->children->append_member(top_area_manager);
            top_area_man = top_area_manager;

            top_spacer_terminal = new ED4_spacer_terminal("Top_Spacer",   0, 0, 100, 10, top_area_manager);
            top_area_manager->children->append_member(top_spacer_terminal);

            top_multi_species_manager = new ED4_multi_species_manager("Top_MultiSpecies_Manager", x, 0, 0, 0, top_area_manager);
            top_area_manager->children->append_member(top_multi_species_manager);

            top_multi_spacer_terminal_beg = new ED4_spacer_terminal("Top_Multi_Spacer_Terminal_Beg", 0, 0, 0, 3, top_multi_species_manager);
            top_multi_species_manager->children->append_member(top_multi_spacer_terminal_beg);

            y+=3;


            aw_openstatus("Reading species from database");
            aw_status((double)0);

            reference = new AWT_reference(GLOBAL_gb_main);
            database->scan_string(top_multi_species_manager, ref_terminals.get_ref_sequence_info(), ref_terminals.get_ref_sequence(),
                                  area_string_top, &index, &y, species_progress);
            GB_pop_transaction(GLOBAL_gb_main);

            const int TOP_MID_LINE_HEIGHT   = 3;
            int       TOP_MID_SPACER_HEIGHT = font_group.get_max_height()-TOP_MID_LINE_HEIGHT;

            top_mid_line_terminal = new ED4_line_terminal("Top_Mid_Line_Terminal", 0, y, 0, TOP_MID_LINE_HEIGHT, device_manager);    // width will be set below
            device_manager->children->append_member(top_mid_line_terminal);

            y += TOP_MID_LINE_HEIGHT;


            top_mid_spacer_terminal = new ED4_spacer_terminal("Top_Middle_Spacer", 0, y, 880, TOP_MID_SPACER_HEIGHT,   device_manager);
            device_manager->children->append_member(top_mid_spacer_terminal);

            // needed to avoid text-clipping problems:
            main_manager->set_top_middle_spacer_terminal(top_mid_spacer_terminal);
            main_manager->set_top_middle_line_terminal(top_mid_line_terminal);

            y += TOP_MID_SPACER_HEIGHT; // add top-mid_spacer_terminal height

            top_multi_species_manager->generate_id_for_groups();
        }

        // ********** Top Area end **********


        // ********** Middle Area beginning **********

        {
            middle_area_manager = new ED4_area_manager("Middle_Area_Manager", 0, y, 0, 0, device_manager);
            device_manager->children->append_member(middle_area_manager);
            middle_area_man = middle_area_manager;

            tree_terminal = new ED4_tree_terminal("Tree", 0, 0, 2, change, middle_area_manager);
            middle_area_manager->children->append_member(tree_terminal);

            mid_multi_species_manager = new ED4_multi_species_manager("Middle_MultiSpecies_Manager", x, 0, 0, 0, middle_area_manager);
            middle_area_manager->children->append_member(mid_multi_species_manager);

            mid_multi_spacer_terminal_beg = new ED4_spacer_terminal("Mid_Multi_Spacer_Terminal_Beg", 0, 0, 0, 3, mid_multi_species_manager);
            mid_multi_species_manager->children->append_member(mid_multi_spacer_terminal_beg);

            y+=3;               // dummy height, to create a dummy layout ( to preserve order of objects )

            scroll_links.link_for_ver_slider = middle_area_manager;

            help = y;
            index = 0;
            {
                GB_transaction dummy(GLOBAL_gb_main);
                database->scan_string(mid_multi_species_manager, ref_terminals.get_ref_sequence_info(), ref_terminals.get_ref_sequence(),
                                      area_string_middle, &index, &y, species_progress);
            }

            aw_closestatus();

            {
                ED4_spacer_terminal *mid_bot_spacer_terminal = new ED4_spacer_terminal("Middle_Bot_Spacer_Terminal", 0, y, 880, 10, device_manager);
                device_manager->children->append_member(mid_bot_spacer_terminal);
            }

            tree_terminal->extension.size[HEIGHT] = y - help;

            mid_multi_species_manager->generate_id_for_groups();
            y += 10;                                                // add top-mid_spacer_terminal height

            mid_bot_line_terminal = new ED4_line_terminal("Mid_Bot_Line_Terminal", 0, y, 0, 3, device_manager);    // width will be set below
            device_manager->children->append_member(mid_bot_line_terminal);
            y += 3;

            total_bottom_spacer = new ED4_spacer_terminal("Total_Bottom_Spacer_terminal", 0, y, 0, 10000, device_manager);
            device_manager->children->append_member(total_bottom_spacer);
            y += 10000;
        }

        // ********** Middle Area end **********

        if (scroll_links.link_for_hor_slider) {
            long ext_width = long(scroll_links.link_for_hor_slider->extension.size[WIDTH]);

            top_multi_spacer_terminal_beg->extension.size[WIDTH] = ext_width + MAXSPECIESWIDTH + SEQUENCEINFOSIZE;
            mid_multi_spacer_terminal_beg->extension.size[WIDTH] = ext_width + MAXSPECIESWIDTH + SEQUENCEINFOSIZE;
            total_bottom_spacer->extension.size[WIDTH] = ext_width + MAXSPECIESWIDTH + SEQUENCEINFOSIZE;

            top_mid_line_terminal->extension.size[WIDTH] = ext_width + TREETERMINALSIZE + MAXSPECIESWIDTH + SEQUENCEINFOSIZE;
            mid_bot_line_terminal->extension.size[WIDTH] = ext_width + TREETERMINALSIZE + MAXSPECIESWIDTH + SEQUENCEINFOSIZE;

        }

        tree_terminal->set_links(NULL, mid_multi_species_manager);                          // set links
        top_spacer_terminal->set_links(tree_terminal, top_multi_species_manager);
        top_mid_spacer_terminal->set_links(middle_area_manager, NULL);
        total_bottom_spacer->set_links(mid_bot_line_terminal, 0);
    }

    first_window->update_window_coords();
    resize_all();


    // build consensi
    {
        aw_openstatus("Initializing consensi");
        aw_status((double)0);

        aw_status_counter progress(total_no_of_groups+1); // 1 is root_group_man

        root_group_man->create_consensus(root_group_man, &progress);
        e4_assert(root_group_man->table().ok());

        aw_closestatus();
    }

    root_group_man->remap()->mark_compile_needed_force();
    root_group_man->update_remap();

    // calc size and display:

    resize_all();

    x_link = y_link = NULL;
    if (main_manager)
    {
        x_link = scroll_links.link_for_hor_slider;
        y_link = scroll_links.link_for_ver_slider;
    }

    width_link  = x_link;
    height_link = y_link;

    new_window = first_window;

    while (new_window)
    {
        new_window->set_scrolled_rectangle(0, 0, 0, 0, x_link, y_link, width_link, height_link);
        new_window->aww->show();
        new_window->update_scrolled_rectangle();
        ED4_refresh_window(new_window->aww, 0, 0);
        new_window = new_window->next;
    }

    aw_root->add_timed_callback(2000, ED4_timer, (AW_CL)0, (AW_CL)0);

    ED4_finish_and_show_notFoundMessage();

    return (ED4_R_OK);
}

ED4_returncode ED4_root::get_area_rectangle(AW_rectangle *rect, AW_pos x, AW_pos y) { // returns win-coordinates of area (defined by folding lines) which contains position x/y
    ED4_folding_line *flv, *flh;
    int x1, x2, y1, y2;
    AW_rectangle area_rect;
    get_device()->get_area_size(&area_rect);

    x1 = area_rect.l;
    for (flv=get_ed4w()->vertical_fl; ; flv = flv->next) {
        if (flv) {
            e4_assert(flv->length==INFINITE);
            x2 = int(flv->window_pos[X_POS]);
        }
        else {
            x2 = area_rect.r;
            if (x1==x2) {
                break;
            }
        }

        y1 = area_rect.t;
        for (flh=get_ed4w()->horizontal_fl; ; flh = flh->next) {
            if (flh) {
                e4_assert(flh->length==INFINITE);
                y2 = int(flh->window_pos[Y_POS]);
            }
            else {
                y2 = area_rect.b;
                if (y1==y2) {
                    break;
                }
            }

            if (x1<=x && x<=x2 && y1<=y && y<=y2) {
                rect->t = y1;
                rect->b = y2;
                rect->l = x1;
                rect->r = x2;
                return ED4_R_OK;
            }
            y1 = y2;
            if (!flh) break;
        }
        x1 = x2;
        if (!flv) break;
    }
    return ED4_R_IMPOSSIBLE; // no area contains x/y :-(
}

ED4_returncode ED4_root::world_to_win_coords(AW_window *IF_DEBUG(aww), AW_pos *xPtr, AW_pos *yPtr)  // calculates transformation from world to window
{                                               // coordinates in a given window
    ED4_window      *current_window;
    ED4_folding_line    *current_fl;
    AW_pos      temp_x, temp_y, x, y;

    current_window = get_ed4w();
    e4_assert(current_window==first_window->get_matching_ed4w(aww));

    if (! current_window) {
        e4_assert(0);
        return (ED4_R_WARNING);   // should never occur
    }

    temp_x = x = *xPtr;
    temp_y = y = *yPtr;

    current_fl = current_window->vertical_fl;                       // calculate x-offset due to vertical folding lines
    while (current_fl && (x>=current_fl->world_pos[X_POS])) {
        if ((current_fl->length == INFINITE) ||
            ((y >= current_fl->world_pos[Y_POS]) &&
             (y <= current_fl->world_pos[Y_POS] + current_fl->length)))
        {
            temp_x -= current_fl->dimension;

        }
        current_fl = current_fl->next;
    }

    current_fl = current_window->horizontal_fl;                     // calculate y-offset due to horizontal folding lines
    while (current_fl && (y >= current_fl->world_pos[Y_POS])) {
        if ((current_fl->length == INFINITE) ||
            ((x >= current_fl->world_pos[X_POS]) &&
             (x <= (current_fl->world_pos[X_POS] + current_fl->length))))
        {
            temp_y -= current_fl->dimension;
        }
        current_fl = current_fl->next;
    }

    *xPtr = temp_x;
    *yPtr = temp_y;

    return (ED4_R_OK);
}

void ED4_root::copy_window_struct(ED4_window *source,   ED4_window *destination)
{
    destination->slider_pos_horizontal  = source->slider_pos_horizontal;
    destination->slider_pos_vertical    = source->slider_pos_vertical;
    destination->coords         = source->coords;
}


void ED4_reload_helix_cb(AW_window *aww, AW_CL cd1, AW_CL cd2) {
    const char *err = ED4_ROOT->helix->init(GLOBAL_gb_main);
    if (err) aw_message(err);
    ED4_refresh_window(aww, cd1, cd2);
}


void ED4_reload_ecoli_cb(AW_window *aww, AW_CL cd1, AW_CL cd2)
{
    const char *err = ED4_ROOT->ecoli_ref->init(GLOBAL_gb_main);
    if (err) aw_message(err);
    ED4_refresh_window(aww, cd1, cd2);
}

// --------------------------------------------------------------------------------
//  recursion through all species
// --------------------------------------------------------------------------------

typedef ARB_ERROR (*ED4_Species_Callback)(GBDATA*, AW_CL);

ARB_ERROR do_sth_with_species(ED4_base *base, AW_CL cl_spec_cb, AW_CL cd) {
    ARB_ERROR error = NULL;

    if (base->is_species_manager()) {
        ED4_Species_Callback       cb                    = (ED4_Species_Callback)cl_spec_cb;
        ED4_species_manager       *species_manager       = base->to_species_manager();
        ED4_species_name_terminal *species_name_terminal = species_manager->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();

        if (species_name_terminal->get_species_pointer()) {
            char *species_name = GB_read_as_string(species_name_terminal->get_species_pointer());

            e4_assert(species_name);
            GBDATA *species = GBT_find_species(GLOBAL_gb_main, species_name);

            error = species
                ? cb(species, cd)
                : GB_append_exportedError(GBS_global_string("can't find species '%s'", species_name));

            free(species_name);
        }
    }

    return error;
}


static ARB_ERROR ED4_with_all_loaded_species(ED4_Species_Callback cb, AW_CL cd) {
    return ED4_ROOT->root_group_man->route_down_hierarchy(do_sth_with_species, (AW_CL)cb, cd);
}

// --------------------------------------------------------------------------------
//  faligner
// --------------------------------------------------------------------------------

static int has_species_name(ED4_base *base, AW_CL cl_species_name)
{
    ED4_species_name_terminal *name_term = base->to_species_name_terminal();
    GBDATA *gbd = name_term->get_species_pointer();

    if (gbd) {
        const char *name = GB_read_char_pntr(gbd);
        e4_assert(name);
        int has_name = strcmp(name, (const char*)cl_species_name)==0;
        return has_name;
    }

    return 0;
}

ED4_species_name_terminal *ED4_find_species_name_terminal(const char *species_name)
{
    ED4_base *base = ED4_ROOT->root_group_man->find_first_that(ED4_L_SPECIES_NAME, has_species_name, (AW_CL)species_name);

    return base ? base->to_species_name_terminal() : 0;
}

static char *get_group_consensus(const char *species_name, int start_pos, int end_pos)
{
    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(species_name);
    char *consensus = 0;

    if (name_term) {
        ED4_group_manager *group_man = name_term->get_parent(ED4_L_GROUP)->to_group_manager();
        if (group_man) {
            consensus = group_man->table().build_consensus_string(start_pos, end_pos, 0);
        }
    }

    return consensus;
}

static int get_selected_range(int *firstColumn, int *lastColumn)
{
    ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
    if (listElem) {
        ED4_selection_entry *selectionEntry = (ED4_selection_entry*)listElem->elem();
        ED4_sequence_terminal *seqTerm = selectionEntry->object->get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();

        ED4_get_selected_range(seqTerm, firstColumn, lastColumn);
        return 1;
    }
    return 0;
}

static ED4_list_elem *actual_aligner_elem = 0;
static GBDATA *get_next_selected_species()
{
    if (!actual_aligner_elem) return 0;

    ED4_selection_entry *selectionEntry = (ED4_selection_entry*)actual_aligner_elem->elem();
    ED4_species_manager *specMan = selectionEntry->object->get_parent(ED4_L_SPECIES)->to_species_manager();

    actual_aligner_elem = actual_aligner_elem->next();
    return specMan->get_species_pointer();
}
static GBDATA *get_first_selected_species(int *total_no_of_selected_species)
{
    int selected = ED4_ROOT->selected_objects.no_of_entries();

    if (total_no_of_selected_species) {
        *total_no_of_selected_species = selected;
    }

    if (selected) {
        actual_aligner_elem = ED4_ROOT->selected_objects.first();
    }
    else {
        actual_aligner_elem = 0;
    }

    return get_next_selected_species();
}

struct AWTC_faligner_cd faligner_client_data = {
    1,                                              // default is to do a refresh
    ED4_timer_refresh,                              // with this function
    get_group_consensus,                            // aligner fetches consensus of group of species via this function
    get_selected_range,                             // aligner fetches column range of selection via this function
    get_first_selected_species,                     // aligner fetches first and..
    get_next_selected_species,                      // .. following selected species via this functions
    0                                               // AW_helix (needed for island_hopping)
};
static ARB_ERROR ED4_delete_temp_entries(GBDATA *species, AW_CL cl_alignment_name) {
    return AWTC_delete_temp_entries(species, (GB_CSTR)cl_alignment_name);
}

void ED4_remove_faligner_entries(AW_window *, AW_CL, AW_CL) {
    ARB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);
    if (!error) error = ED4_with_all_loaded_species(ED4_delete_temp_entries, (AW_CL)ED4_ROOT->alignment_name);
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}

void ED4_turnSpecies(AW_window *aw, AW_CL, AW_CL) {
    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);

    if (!error) {
        AW_root *root       = aw->get_root();
        char    *name       = root->awar(AWAR_SPECIES_NAME)->read_string();
        char    *ali        = ED4_ROOT->alignment_name;
        GBDATA  *gb_species = GBT_expect_species(GLOBAL_gb_main, name);
        GBDATA  *gbd        = gb_species ? GBT_read_sequence(gb_species, ali) : NULL;
        char    *data       = gbd ? GB_read_string(gbd) : NULL;

        if (!data) error = GB_await_error();
        else {
            long length = GB_read_string_count(gbd);

            char T_or_U;
            error = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "reverse-complement");
            if (!error) {
                GBT_reverseComplementNucSequence(data, length, T_or_U);
                error = GB_write_string(gbd, data);
            }
        }

        free(data);
        free(name);
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}


#if defined(DEBUG) && 0

void ED4_testSplitNMerge(AW_window *aw, AW_CL, AW_CL)
{
    AW_root *root = aw->get_root();
    char *name;
    GBDATA *species;
    GBDATA *gbd;
    char *data;
    long length;

    GB_ERROR error = GB_begin_transaction(gb_main);
    char *ali = ED4_ROOT->alignment_name;

    if (!error)
    {
        name = root->awar(AWAR_SPECIES_NAME)->read_string();
        species = GBT_find_species(gb_main, name);
        gbd = species ? GBT_read_sequence(species, ali) : NULL;
        data = gbd ? GB_read_string(gbd) : NULL;
        length = gbd ? GB_read_string_count(gbd) : NULL;

        if (data)
        {
            char *newData = AWTC_testConstructSequence(data);

            if (newData)
            {
                error = GB_write_string(gbd, newData);
                delete [] newData;
            }
        }
        else
        {
            error = GB_get_error();
            if (!error) error = GB_export_error("Can't read data of '%s'", name);
        }

        delete name;
        delete data;
    }

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

#endif

static void col_stat_activated(AW_window *)
{
    ED4_ROOT->column_stat_initialized = 1;
    ED4_ROOT->column_stat_activated = 1;
    ED4_ROOT->refresh_all_windows(1);
}

void ED4_activate_col_stat(AW_window *aww, AW_CL, AW_CL) {
    if (!ED4_ROOT->column_stat_initialized) {
        AW_window *aww_st = st_create_main_window(ED4_ROOT->aw_root, ED4_ROOT->st_ml, (AW_CB0)col_stat_activated, (AW_window *)aww);
        aww_st->show();
        return;
    }
    else { // re-activate
        ED4_ROOT->column_stat_activated = 1;
        ED4_ROOT->refresh_all_windows(1);
    }
}
static void disable_col_stat(AW_window *, AW_CL, AW_CL) {
    if (ED4_ROOT->column_stat_initialized && ED4_ROOT->column_stat_activated) {
        ED4_ROOT->column_stat_activated = 0;
        ED4_ROOT->refresh_all_windows(1);
    }
}

static void title_mode_changed(AW_root *aw_root, AW_window *aww)
{
    int title_mode = aw_root->awar(AWAR_EDIT_TITLE_MODE)->read_int();

    if (title_mode==0) {
        aww->set_info_area_height(57);
    }
    else {
        aww->set_info_area_height(170);
    }
}

void ED4_undo_redo(AW_window*, AW_CL undo_type)
{
    GB_ERROR error = GB_undo(GLOBAL_gb_main, (GB_UNDO_TYPE)undo_type);

    if (error) {
        aw_message(error);
    }
    else {
        GB_begin_transaction(GLOBAL_gb_main);
        GB_commit_transaction(GLOBAL_gb_main);
        ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
        if (cursor->owner_of_cursor) {
            ED4_terminal *terminal = cursor->owner_of_cursor->to_terminal();

            terminal->set_refresh();
            terminal->parent->refresh_requested_by_child();
        }
    }
}

void aw_clear_message_cb(AW_window *aww);

void ED4_clear_errors(AW_window*, AW_CL)
{
    aw_clear_message_cb(ED4_ROOT->get_aww());
}

AW_window *ED4_zoom_message_window(AW_root *root, AW_CL)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "ZOOM_ERR_MSG", "Errors and warnings");
    aws->load_xfig("edit4/message.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("hide");
    aws->create_button("HIDE", "HIDE");

    aws->callback(ED4_clear_errors, (AW_CL)0);
    aws->at("clear");
    aws->create_button("CLEAR", "CLEAR");

    aws->at("errortext");
    aws->create_text_field(AWAR_ERROR_MESSAGES);

    return (AW_window *)aws;
}


char *cat(char *toBuf, const char *s1, const char *s2)
{
    char *buf = toBuf;

    while ((*buf++=*s1++)!=0) ;
    buf--;
    while ((*buf++=*s2++)!=0) ;

    return toBuf;
}

static void insert_search_fields(AW_window_menu_modes *awmm,
                                 const char *label_prefix, const char *macro_prefix,
                                 const char *pattern_awar_name, ED4_SearchPositionType type, const char *show_awar_name,
                                 int short_form)
{
    char buf[200];

    if (!short_form) {
        awmm->at(cat(buf, label_prefix, "search"));
        awmm->create_input_field(pattern_awar_name, 30);
    }

    awmm->at(cat(buf, label_prefix, "n"));
    awmm->callback(ED4_search, ED4_encodeSearchDescriptor(+1, type));
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_NEXT"), "#edit/next.bitmap");

    awmm->at(cat(buf, label_prefix, "l"));
    awmm->callback(ED4_search, ED4_encodeSearchDescriptor(-1, type));
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_LAST"), "#edit/last.bitmap");

    awmm->at(cat(buf, label_prefix, "d"));
    awmm->callback(AW_POPUP, (AW_CL)ED4_create_search_window, (AW_CL)type);
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_DETAIL"), "#edit/detail.bitmap");

    awmm->at(cat(buf, label_prefix, "s"));
    awmm->create_toggle(show_awar_name);
}

void ED4_set_protection(AW_window * /* aww */, AW_CL cd1, AW_CL /* cd2 */) {
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    GB_ERROR    error  = 0;

    if (cursor->owner_of_cursor) {
        int                         wanted_protection = int(cd1);
        ED4_sequence_terminal      *seq_term          = cursor->owner_of_cursor->to_sequence_terminal();
        ED4_sequence_info_terminal *seq_info_term     = seq_term->parent->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();
        GBDATA                     *gbd               = seq_info_term->data();

        GB_push_transaction(gbd);

        GB_push_my_security(gbd);
        error = GB_write_security_write(gbd, wanted_protection);
        GB_pop_my_security(gbd);

        error = GB_end_transaction(gbd, error);
    }
    else {
        error = "No species selected";
    }

    if (error) aw_message(error);
}

typedef enum {
    ED4_MS_NONE,
    ED4_MS_ALL,
    ED4_MS_INVERT,
    ED4_MS_INVERT_GROUP,
    ED4_MS_UNMARK_ALL,
    ED4_MS_MARK_SELECTED,
    ED4_MS_UNMARK_SELECTED,
    ED4_MS_SELECT_MARKED,
    ED4_MS_DESELECT_MARKED,
    ED4_MS_TOGGLE_BLOCKTYPE

} MenuSelectType;

static void ED4_menu_select(AW_window *aww, AW_CL type, AW_CL) {
    GB_transaction dummy(GLOBAL_gb_main);
    MenuSelectType select = MenuSelectType(type);
    ED4_multi_species_manager *middle_multi_man = ED4_ROOT->middle_area_man->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

    e4_assert(middle_multi_man);

    switch (select) {
        case ED4_MS_NONE: {
            if (ED4_getBlocktype()!=ED4_BT_NOBLOCK) {
                ED4_ROOT->deselect_all();
                ED4_setBlocktype(ED4_BT_NOBLOCK);
            }
            break;
        }
        case ED4_MS_ALL: {
            middle_multi_man->select_all_species();
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_INVERT: {
            middle_multi_man->invert_selection_of_all_species();
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_SELECT_MARKED: {
            middle_multi_man->select_marked_species(1);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_DESELECT_MARKED: {
            middle_multi_man->select_marked_species(0);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_UNMARK_ALL: {
            GBT_mark_all(GLOBAL_gb_main, 0);
            break;
        }
        case ED4_MS_MARK_SELECTED: {
            middle_multi_man->mark_selected_species(1);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_UNMARK_SELECTED: {
            middle_multi_man->mark_selected_species(0);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_INVERT_GROUP: {
            ED4_cursor *cursor = &ED4_ROOT->first_window->get_matching_ed4w(aww)->cursor;
            int done = 0;

            if (cursor->owner_of_cursor) {
                ED4_multi_species_manager *multi_man = cursor->owner_of_cursor->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();

                multi_man->invert_selection_of_all_species();
                ED4_correctBlocktypeAfterSelection();
                done = 1;
            }

            if (!done) {
                aw_message("Place cursor into group");
            }
            break;
        }
        case ED4_MS_TOGGLE_BLOCKTYPE: {
            ED4_toggle_block_type();
            break;
        }
        default: {
            e4_assert(0);
            break;
        }
    }

    ED4_refresh_window(aww, 1, 0);
}

void ED4_menu_perform_block_operation(AW_window*, AW_CL type, AW_CL) {
    ED4_perform_block_operation(ED4_blockoperation_type(type));
}

static void modes_cb(AW_window*, AW_CL cd1, AW_CL)
{
    ED4_ROOT->species_mode = ED4_species_mode(cd1);
}
void ED4_no_dangerous_modes()
{
    switch (ED4_ROOT->species_mode) {
        case ED4_SM_KILL: {
            ED4_ROOT->species_mode = ED4_SM_MOVE;
            ED4_ROOT->get_aww()->select_mode(0);
            break;
        }
        default: {
            break;
        }
    }
}

void ED4_init_faligner_data(AWTC_faligner_cd *faligner_data) {
    GB_ERROR  error          = GB_push_transaction(GLOBAL_gb_main);
    char     *alignment_name = GBT_get_default_alignment(GLOBAL_gb_main);
    long      alilen         = GBT_get_alignment_len(GLOBAL_gb_main, alignment_name);

    if (alilen<=0) error = GB_await_error();
    else {
        char   *helix_string = 0;
        char   *helix_name   = GBT_get_default_helix(GLOBAL_gb_main);
        GBDATA *gb_helix_con = GBT_find_SAI(GLOBAL_gb_main, helix_name);
        if (gb_helix_con) {
            GBDATA *gb_helix = GBT_read_sequence(gb_helix_con, alignment_name);
            if (gb_helix) helix_string = GB_read_string(gb_helix);
        }
        free(helix_name);
        freeset(faligner_data->helix_string, helix_string);
    }

    free(alignment_name);
    error = GB_end_transaction(GLOBAL_gb_main, error);
    if (error) aw_message(error);
}

static void ED4_create_faligner_window(AW_root *awr, AW_CL cd) {
    ED4_init_faligner_data((AWTC_faligner_cd*)cd);
    AWTC_create_faligner_window(awr, cd);
}

static void ED4_save_defaults(AW_window *aw, AW_CL cl_mode, AW_CL) {
    int mode = (int)cl_mode;

    AW_save_specific_defaults(aw, ED4_propertyName(mode));
}

static AW_window *ED4_create_gc_window(AW_root *aw_root, AW_gc_manager id) {
    static AW_window *gc_win = 0;
    if (!gc_win) {
        gc_win = AW_create_gc_window(aw_root, id);
    }
    return gc_win;
}

ED4_returncode ED4_root::generate_window(AW_device **device,    ED4_window **new_window)
{
    AW_window_menu_modes *awmm;

    {
        ED4_window *ed4w = first_window;

        while (ed4w) { // before creating a window look for a hidden window
            if (ed4w->is_hidden) {
                ed4w->aww->show();
                ed4w->is_hidden = false;
                return ED4_R_BREAK;
            }
            ed4w = ed4w->next;
        }
    }

    if (ED4_window::no_of_windows == MAXWINDOWS)                            // no more than 5 windows allowed
    {
        aw_message(GBS_global_string("Restricted to %i windows", MAXWINDOWS));
        return ED4_R_BREAK;
    }

    awmm = new AW_window_menu_modes;
    {
        int   len = strlen(alignment_name)+35;
        char *buf = GB_give_buffer(len);
        snprintf(buf, len-1, "ARB_EDIT4 *%d* [%s]", ED4_window::no_of_windows+1, alignment_name);
        awmm->init(aw_root, "ARB_EDIT4", buf, 800, 450);
    }

    *device     = awmm->get_device(AW_MIDDLE_AREA); // points to Middle Area device
    *new_window = ED4_window::insert_window(awmm);   // append to window list

    e4_assert(ED4_window::no_of_windows >= 1);
    bool clone = ED4_window::no_of_windows>1;
    if (!clone) {                                   // this is the first window
        AW_init_color_group_defaults("arb_edit4");
    }
    else { // additional edit windows
        copy_window_struct(first_window, *new_window);
    }

    // each window has its own gc-manager
    aw_gc_manager = AW_manage_GC(awmm,              // window
                                 *device,           // device-handle of window
                                 ED4_G_STANDARD,    // GC_Standard configuration
                                 ED4_G_DRAG,
                                 AW_GCM_DATA_AREA,
                                 ED4_expose_cb,     // callback function
                                 1,                 // AW_CL for callback function
                                 0,                 // AW_CL for callback function
                                 true,              // use color groups

                                 "#f8f8f8",
                                 "STANDARD$black",  // Standard Color showing sequences
                                 "#SEQUENCES (0)$#505050", // default color for sequences (color 0)
                                 "+-HELIX (1)$#8E0000",  "+-COLOR 2$#0000dd",    "-COLOR 3$#00AA55",
                                 "+-COLOR 4$#80f",       "+-COLOR 5$#c0a020",    "-COLOR 6$grey",
                                 "+-COLOR 7$#ff0000",    "+-COLOR 8$#44aaff",    "-COLOR 9$#ffaa00",

                                 "+-RANGE 0$#FFFFFF",    "+-RANGE 1$#F0F0F0",    "-RANGE 2$#E0E0E0",
                                 "+-RANGE 3$#D8D8D8",    "+-RANGE 4$#D0D0D0",    "-RANGE 5$#C8C8C8",
                                 "+-RANGE 6$#C0C0C0",    "+-RANGE 7$#B8B8B8",    "-RANGE 8$#B0B0B0",
                                 "-RANGE 9$#A0A0A0",

                                 // colors used to Paint search patterns
                                 // (do not change the names of these gcs)
                                 "+-User1$#B8E2F8",      "+-User2$#B8E2F8",      "-Probe$#B8E2F8", // see also SEC_graphic::init_devices
                                 "+-Primer(l)$#A9FE54",  "+-Primer(r)$#A9FE54",  "-Primer(g)$#A9FE54",
                                 "+-Sig(l)$#DBB0FF",     "+-Sig(r)$#DBB0FF",     "-Sig(g)$#DBB0FF",

                                 "+-MISMATCHES$#FF9AFF", "-CURSOR$#FF0080",
                                 "+-MARKED$#f4f8e0",     "-SELECTED$#FFFF80",

                                 NULL);

    // since the gc-managers of all EDIT4-windows point to the same window properties,
    // changing fonts and colors is always done on first gc-manager
    static AW_gc_manager first_gc_manager = 0;
    if (!first_gc_manager) first_gc_manager = aw_gc_manager;

#define SEP________________________SEP awmm->insert_separator()

    // ------------------------------
    //  File
    // ------------------------------

#if defined(DEBUG)
    AWT_create_debug_menu(awmm);
#endif // DEBUG

    awmm->create_menu("File", "F", AWM_ALL);

    awmm->insert_menu_topic("new_win",        "New Editor Window",     "W", 0, AWM_ALL, ED4_new_editor_window,         0,                                             0);
    awmm->insert_menu_topic("save_config",    "Save Configuration",    "S", 0, AWM_ALL, (AW_CB)ED4_save_configuration, (AW_CL) 0,                                     (int)0);
    awmm->insert_menu_topic("save_config_as", "Save Configuration As", "A", 0, AWM_ALL, AW_POPUP,                      (AW_CL) ED4_save_configuration_as_open_window, (int)0);
    SEP________________________SEP;
    awmm->insert_menu_topic("load_config",   "Load Configuration",   "L", 0, AWM_ALL, AW_POPUP,           (AW_CL)ED4_start_editor_on_old_configuration, 0);
    awmm->insert_menu_topic("reload_config", "Reload Configuration", "R", 0, AWM_ALL, ED4_restart_editor, 0,                                            0);
    SEP________________________SEP;
    GDE_load_menu(awmm, AWM_ALL, "Print");
    SEP________________________SEP;

    if (clone) awmm->insert_menu_topic("close", "CLOSE", "C", 0, AWM_ALL, ED4_quit_editor, 0, 0);
    else       awmm->insert_menu_topic("quit",   "QUIT",  "Q", 0, AWM_ALL, ED4_quit_editor, 0, 0);

    // ------------------------------
    //  Create
    // ------------------------------

    awmm->create_menu("Create", "C", AWM_ALL);
    awmm->insert_menu_topic("create_species",                "Create new species",                "n", 0, AWM_ALL, AW_POPUP, (AW_CL)ED4_create_new_seq_window, (int)0);
    awmm->insert_menu_topic("create_species_from_consensus", "Create new species from consensus", "u", 0, AWM_ALL, AW_POPUP, (AW_CL)ED4_create_new_seq_window, (int)1);
    awmm->insert_menu_topic("copy_species",                  "Copy current species",              "C", 0, AWM_ALL, AW_POPUP, (AW_CL)ED4_create_new_seq_window, (int)2);
    SEP________________________SEP;
    awmm->insert_menu_topic("create_group",           "Create new Group",              "G", 0, AWM_ALL, group_species_cb, 0, 0);
    awmm->insert_menu_topic("create_groups_by_field", "Create new groups using Field", "F", 0, AWM_ALL, group_species_cb, 1, 0);

    // ------------------------------
    //  Edit
    // ------------------------------

    awmm->create_menu("Edit", "E", AWM_ALL);
    awmm->insert_menu_topic("refresh",     "Refresh [Ctrl-L]",           "f",  0, AWM_ALL, ED4_refresh_window,                   1, 0);
    awmm->insert_menu_topic("load_actual", "Load current species [GET]", "G", 0, AWM_ALL, ED4_get_and_jump_to_actual_from_menu, 0, 0);
    awmm->insert_menu_topic("load_marked", "Load marked species",        "m", 0, AWM_ALL, ED4_get_marked_from_menu,             0, 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("refresh_ecoli",       "Reload Ecoli sequence",        "E", "ecoliref.hlp", AWM_ALL, ED4_reload_ecoli_cb,     0, 0);
    awmm->insert_menu_topic("refresh_helix",       "Reload Helix",                 "H", "helix.hlp",    AWM_ALL, ED4_reload_helix_cb,     0, 0);
    awmm->insert_menu_topic("helix_jump_opposite", "Jump helix opposite [Ctrl-J]", "J", 0,              AWM_ALL, ED4_helix_jump_opposite, 0, 0);
    SEP________________________SEP;

    awmm->insert_sub_menu("Set protection of current ", "p");
    {
        char macro[] = "to_0",
            topic[] = ".. to 0",
            hotkey[] = "0";

        for (char i='0'; i<='6'; i++) {
            macro[3] = topic[6] = hotkey[0] = i;
            awmm->insert_menu_topic(macro, topic, hotkey, "security.hlp", AWM_ALL, ED4_set_protection, AW_CL(i-'0'), 0);
        }
    }
    awmm->close_sub_menu();

#if !defined(NDEBUG) && 0
    awmm->insert_menu_topic("turn_sequence", "Turn Sequence",             "T", 0, AWM_ALL, ED4_turnSpecies,     1, 0);
    awmm->insert_menu_topic(0,               "Test (test split & merge)", "T", 0, AWM_ALL, ED4_testSplitNMerge, 1, 0);
#endif
    SEP________________________SEP;
    awmm->insert_menu_topic("fast_aligner",       INTEGRATED_ALIGNERS_TITLE " [Ctrl-A]", "I", "faligner.hlp",     AWM_ALL,            AW_POPUP,                               (AW_CL)ED4_create_faligner_window, (AW_CL)&faligner_client_data);
    awmm->insert_menu_topic("fast_align_set_ref", "Set aligner reference [Ctrl-R]",      "R", "faligner.hlp",     AWM_ALL,            (AW_CB)AWTC_set_reference_species_name, (AW_CL)aw_root,                    0);
    awmm->insert_menu_topic("align_sequence",     "Old aligner from ARB_EDIT",           "O", "ne_align_seq.hlp", AWM_EXP,            AW_POPUP,                               (AW_CL)create_naligner_window,     0);
    awmm->insert_menu_topic("sina",               "SINA (SILVA Incremental Aligner)",    "S", "sina_main.hlp",    sina_mask(aw_root), show_sina_window,                       (AW_CL)&faligner_client_data,      0);
    awmm->insert_menu_topic("del_ali_tmp",        "Remove all aligner Entries",          "v", 0,                  AWM_ALL,            ED4_remove_faligner_entries,            1,                                 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("missing_bases", "Dot potentially missing bases", "D", "missbase.hlp", AWM_EXP, ED4_popup_dot_missing_bases_window, 0, 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("sec_edit", "Edit secondary structure", "c", 0, AWM_ALL, ED4_SECEDIT_start, (AW_CL)GLOBAL_gb_main, 0);

    // ------------------------------
    //  View
    // ------------------------------

    awmm->create_menu("View", "V", AWM_ALL);
    awmm->insert_sub_menu("Search", "S");
    {
        int         s;
        const char *hotkeys  = "12Poiclrg";
        char        hotkey[] = "_";

        e4_assert(strlen(hotkeys) == SEARCH_PATTERNS);

        for (s=0; s<SEARCH_PATTERNS; s++) {
            ED4_SearchPositionType type = ED4_SearchPositionType(s);
            const char *id = ED4_SearchPositionTypeId[type];
            int len = strlen(id);
            char *macro_name = GB_give_buffer(2*(len+8));
            char *menu_entry_name = macro_name+(len+8);

#ifndef NDEBUG
            memset(macro_name, 0, 2*(len+8)); // to avoid memchk-warning
#endif
            sprintf(macro_name, "%s_SEARCH", id);
            char *p = macro_name;
            while (1) {
                char c = *p++;
                if (!c) break;
                p[-1] = toupper(c);
            }
            sprintf(menu_entry_name, "%s Search", id);

            hotkey[0] = hotkeys[s];
            awmm->insert_menu_topic(macro_name, menu_entry_name, hotkey, "e4_search.hlp", AWM_ALL, AW_POPUP, AW_CL(ED4_create_search_window), AW_CL(type));
        }
    }
    awmm->close_sub_menu();
    SEP________________________SEP;
    awmm->insert_sub_menu("Cursor position ", "p");
    awmm->insert_menu_topic("store_curpos",   "Store cursor position",    "S", 0, AWM_ALL, ED4_store_curpos,        0, 0);
    awmm->insert_menu_topic("restore_curpos", "Restore cursor position ", "R", 0, AWM_ALL, ED4_restore_curpos,      0, 0);
    awmm->insert_menu_topic("clear_curpos",   "Clear stored positions",   "C", 0, AWM_ALL, ED4_clear_stored_curpos, 0, 0);
    awmm->close_sub_menu();

    SEP________________________SEP;
    awmm->insert_menu_topic("change_cursor", "Change cursor type",                "t", 0,                   AWM_ALL, ED4_change_cursor,         0, 0);
    awmm->insert_menu_topic("show_all",      "Show all bases ",                   "a", "set_reference.hlp", AWM_ALL, ED4_set_reference_species, 1, 0);
    awmm->insert_menu_topic("show_diff",     "Show only differences to selected", "d", "set_reference.hlp", AWM_ALL, ED4_set_reference_species, 0, 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("enable_col_stat",  "Activate column statistics", "v", "st_ml.hlp", AWM_EXP, ED4_activate_col_stat,          0, 0);
    awmm->insert_menu_topic("disable_col_stat", "Disable column statistics",  "i", "st_ml.hlp", AWM_EXP, disable_col_stat,               0, 0);
    awmm->insert_menu_topic("detail_col_stat",  "Detailed column statistics", "c", "st_ml.hlp", AWM_EXP, ED4_show_detailed_column_stats, 0, 0);
    awmm->insert_menu_topic("dcs_threshold",    "Set threshold for D.c.s.",   "f", "st_ml.hlp", AWM_EXP, ED4_set_col_stat_threshold,     1, 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("visualize_SAI", "Visualize SAIs", "z", "visualizeSAI.hlp", AWM_ALL, AW_POPUP, (AW_CL)ED4_createVisualizeSAI_window, 0);
    // Enable ProteinViewer only for DNA sequence type
    if (alignment_type == GB_AT_DNA) {
        awmm->insert_menu_topic("Protein_Viewer", "Protein Viewer", "w", "proteinViewer.hlp", AWM_ALL, AW_POPUP, (AW_CL)ED4_CreateProteinViewer_window, 0);
    }

    // ------------------------------
    //  Block
    // ------------------------------

    awmm->create_menu("Block", "B", AWM_ALL);

    awmm->insert_menu_topic("select_marked",   "Select marked species",   "e", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_SELECT_MARKED),   0);
    awmm->insert_menu_topic("deselect_marked", "Deselect marked species", "k", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_DESELECT_MARKED), 0);
    awmm->insert_menu_topic("select_all",      "Select all species",      "S", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_ALL),             0);
    awmm->insert_menu_topic("deselect_all",    "Deselect all species",    "D", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_NONE),            0);
    SEP________________________SEP;
    awmm->insert_menu_topic("mark_selected",   "Mark selected species",   "M", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_MARK_SELECTED),   0);
    awmm->insert_menu_topic("unmark_selected", "Unmark selected species", "n", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_UNMARK_SELECTED), 0);
    awmm->insert_menu_topic("unmark_all",      "Unmark all species",      "U", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_UNMARK_ALL),      0);
    SEP________________________SEP;
    awmm->insert_menu_topic("invert_all",   "Invert selected species", "I", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_INVERT),       0);
    awmm->insert_menu_topic("invert_group", "Invert group",            "g", "e4_block.hlp", AWM_ALL, ED4_menu_select, AW_CL(ED4_MS_INVERT_GROUP), 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("lowcase", "Change to lower case ", "w", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_LOWER_CASE), 0);
    awmm->insert_menu_topic("upcase",  "Change to upper case",  "p", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_UPPER_CASE), 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("reverse",            "Reverse selection ",    "v", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_REVERSE),            0);
    awmm->insert_menu_topic("complement",         "Complement selection ", "o", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_COMPLEMENT),         0);
    awmm->insert_menu_topic("reverse_complement", "Reverse complement",    "t", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_REVERSE_COMPLEMENT), 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("unalignBlockLeft",  "Unalign block",       "a", "e4_block.hlp",   AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_UNALIGN),                                0);
    awmm->insert_menu_topic("unalignBlockRight", "Unalign block right", "b", "e4_block.hlp",   AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_UNALIGN_RIGHT),                          0);
    awmm->insert_menu_topic("replace",           "Search & Replace ",   "h", "e4_replace.hlp", AWM_ALL, AW_POPUP,                              (AW_CL)               ED4_create_replace_window, 0);
    SEP________________________SEP;
    awmm->insert_menu_topic("toggle_block_type", "Line block <-> Column block", "C", "e4_block.hlp", AWM_ALL, ED4_menu_select,                  AW_CL(ED4_MS_TOGGLE_BLOCKTYPE), 0);
    awmm->insert_menu_topic("shift_left",        "Shift block left ",           "l", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_SHIFT_LEFT),       0);
    awmm->insert_menu_topic("shift_right",       "Shift block right",           "r", "e4_block.hlp", AWM_ALL, ED4_menu_perform_block_operation, AW_CL(ED4_BO_SHIFT_RIGHT),      0);

    // ------------------------------
    //  Properties
    // ------------------------------

    awmm->create_menu("Properties", "P", AWM_ALL);

    awmm->insert_menu_topic("props_frame",     "Frame Settings ",       "F", 0,                  AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window,                       0);
    awmm->insert_menu_topic("props_options",   "Editor Options ",       "O", "e4_options.hlp",   AWM_ALL, AW_POPUP, (AW_CL)ED4_create_level_1_options_window,      0);
    awmm->insert_menu_topic("props_consensus", "Consensus Definition ", "u", "e4_consensus.hlp", AWM_ALL, AW_POPUP, (AW_CL)ED4_create_consensus_definition_window, 0);
    SEP________________________SEP;

    awmm->insert_menu_topic("props_data",       "Change Colors & Fonts ", "C", 0,         AWM_ALL, AW_POPUP, (AW_CL)ED4_create_gc_window,      (AW_CL)first_gc_manager);
    awmm->insert_menu_topic("props_seq_colors", "Set Sequence Colors ",   "S", "no help", AWM_ALL, AW_POPUP, (AW_CL)create_seq_colors_window, (AW_CL)sequence_colors);

    SEP________________________SEP;

    static AW_cb_struct *expose_cb = 0;
    if (!expose_cb) expose_cb = new AW_cb_struct(awmm, (AW_CB)ED4_expose_cb, 0, 0);

    if (alignment_type == GB_AT_AA) awmm->insert_menu_topic("props_pfold",     "Protein Match Settings ", "P", "pfold_props.hlp", AWM_ALL, AW_POPUP, (AW_CL)ED4_pfold_create_props_window, (AW_CL)expose_cb);
    else                            awmm->insert_menu_topic("props_helix_sym", "Helix Settings ",         "H", "helixsym.hlp",    AWM_ALL, AW_POPUP, (AW_CL)create_helix_props_window,     (AW_CL)expose_cb);

    awmm->insert_menu_topic("props_key_map", "Key Mappings ",              "K", "nekey_map.hlp", AWM_ALL, AW_POPUP, (AW_CL)create_key_map_window, 0);
    awmm->insert_menu_topic("props_nds",     "Select visible info (NDS) ", "D", "ed4_nds.hlp",   AWM_ALL, AW_POPUP, (AW_CL)ED4_create_nds_window, 0);
    SEP________________________SEP;
    awmm->insert_sub_menu("Save properties ...", "a");
    {
        static const char * const tag[] = { "save_alispecific_props", "save_alitype_props", "save_props" };
        static const char * const entry_type[] = { "alignment specific ", "ali-type specific ", "" };

        // check what is the default mode
        int default_mode = -1;
        for (int mode = 0; mode <= 2; ++mode) {
            if (0 == strcmp(ED4_propertyName(mode), db_name)) {
                default_mode = mode;
                break;
            }
        }

        const char *entry = GBS_global_string("Save loaded Properties (~/%s)", db_name);
        awmm->insert_menu_topic("save_loaded_props", entry, "l", "e4_defaults.hlp", AWM_ALL, ED4_save_defaults, (AW_CL)default_mode, 0);
        SEP________________________SEP;

        for (int mode = 2; mode >= 0; --mode) {
            char hotkey[] = "x";
            hotkey[0]     = "Pta"[mode];
            entry         = GBS_global_string("Save %sProperties (~/%s)", entry_type[mode], ED4_propertyName(mode));
            awmm->insert_menu_topic(tag[mode], entry, hotkey, "e4_defaults.hlp", AWM_ALL, ED4_save_defaults, (AW_CL)mode, 0);
        }
    }
    awmm->close_sub_menu();

    awmm->insert_help_topic("ARB_EDIT4 help",     "E", "e4.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"e4.hlp", 0);

    // ----------------------------------------------------------------------------------------------------

#undef SEP________________________SEP

    aw_root->awar_int(AWAR_EDIT_TITLE_MODE)->add_callback((AW_RCB1)title_mode_changed, (AW_CL)awmm);
    awmm->set_bottom_area_height(0);   // No bottom area

    awmm->auto_space(5, -2);
    awmm->shadow_width(3);

    int db_pathx, db_pathy;
    awmm->get_at_position(&db_pathx, &db_pathy);

    awmm->shadow_width(1);
    awmm->load_xfig("edit4/editmenu.fig", false);

    // -------------------------
    //      help /quit /fold
    // -------------------------

    awmm->button_length(0);

    awmm->at("quit");
    awmm->callback(ED4_quit_editor, 0, 0);
    awmm->help_text("quit.hlp");

    if (clone) awmm->create_button("CLOSE", "#close.xpm");
    else       awmm->create_button("QUIT", "#quit.xpm");

    awmm->at("help");
    awmm->callback(AW_help_entry_pressed);
    awmm->help_text("e4.hlp");
    awmm->create_button("HELP", "#help.xpm");

    awmm->at("fold");
    awmm->help_text("e4.hlp");
    awmm->create_toggle(AWAR_EDIT_TITLE_MODE, "#more.bitmap", "#less.bitmap");

    // ------------------
    //      positions
    // ------------------

    awmm->button_length(0);

    awmm->at("posTxt");     awmm->create_button(0, "Position");

    awmm->button_length(6+1);

    awmm->at("ecoliTxt");   awmm->create_button(0, ED4_AWAR_NDS_ECOLI_NAME, 0, "+");

    awmm->button_length(0);

    awmm->at("baseTxt");    awmm->create_button(0, "Base");
    awmm->at("iupacTxt");   awmm->create_button(0, "IUPAC");
    awmm->at("helixnrTxt"); awmm->create_button(0, "Helix#");

    awmm->at("pos");
    awmm->callback((AW_CB)ED4_jump_to_cursor_position, (AW_CL) get_ed4w()->awar_path_for_cursor, AW_CL(1));
    awmm->create_input_field(get_ed4w()->awar_path_for_cursor, 7);

    awmm->at("ecoli");
    awmm->callback((AW_CB)ED4_jump_to_cursor_position, (AW_CL) get_ed4w()->awar_path_for_Ecoli, AW_CL(0));
    awmm->create_input_field(get_ed4w()->awar_path_for_Ecoli, 6);

    awmm->at("base");
    awmm->callback((AW_CB)ED4_jump_to_cursor_position, (AW_CL) get_ed4w()->awar_path_for_basePos, AW_CL(0));
    awmm->create_input_field(get_ed4w()->awar_path_for_basePos, 6);

    awmm->at("iupac");
    awmm->callback((AW_CB)ED4_set_iupac, (AW_CL) get_ed4w()->awar_path_for_IUPAC, AW_CL(0));
    awmm->create_input_field(get_ed4w()->awar_path_for_IUPAC, 4);

    awmm->at("helixnr");
    awmm->callback((AW_CB)ED4_set_helixnr, (AW_CL) get_ed4w()->awar_path_for_helixNr, AW_CL(0));
    awmm->create_input_field(get_ed4w()->awar_path_for_helixNr, 5);

    // ---------------------------
    //      jump/get/undo/redo
    // ---------------------------

    awmm->button_length(4);

    awmm->at("jump");
    awmm->callback(ED4_jump_to_current_species, 0);
    awmm->help_text("e4.hlp");
    awmm->create_button("JUMP", "Jump");

    awmm->at("get");
    awmm->callback(ED4_get_and_jump_to_actual, 0);
    awmm->help_text("e4.hlp");
    awmm->create_button("GET", "Get");

    awmm->button_length(0);

    awmm->at("undo");
    awmm->callback(ED4_undo_redo, GB_UNDO_UNDO);
    awmm->help_text("undo.hlp");
    awmm->create_button("UNDO", "#undo.bitmap");

    awmm->at("redo");
    awmm->callback(ED4_undo_redo, GB_UNDO_REDO);
    awmm->help_text("undo.hlp");
    awmm->create_button("REDO", "#redo.bitmap");

    // -------------------------
    //      aligner / SAIviz
    // -------------------------

    awmm->button_length(7);

    awmm->at("aligner");
    awmm->callback(AW_POPUP, (AW_CL)ED4_create_faligner_window, (AW_CL)&faligner_client_data);
    awmm->help_text("faligner.hlp");
    awmm->create_button("ALIGNER", "Aligner");

    awmm->at("saiviz");
    awmm->callback(AW_POPUP, (AW_CL)ED4_createVisualizeSAI_window, 0);
    awmm->help_text("visualizeSAI.hlp");
    awmm->create_button("SAIVIZ", "SAIviz");

    // ------------------------------------------
    //      align/insert/protection/direction
    // ------------------------------------------

    awmm->button_length(0);

    awmm->at("protect");
    awmm->create_option_menu(AWAR_EDIT_SECURITY_LEVEL);
    awmm->insert_option("0", 0, 0);
    awmm->insert_option("1", 0, 1);
    awmm->insert_option("2", 0, 2);
    awmm->insert_option("3", 0, 3);
    awmm->insert_option("4", 0, 4);
    awmm->insert_option("5", 0, 5);
    awmm->insert_default_option("6", 0, 6);
    awmm->update_option_menu();

    // draw protection icon AFTER protection!!
    awmm->at("pico");
    awmm->create_button("PROTECT", "#protect.xpm");

    // draw align/edit-button AFTER protection!!
    awmm->at("edit");
    awmm->create_toggle(AWAR_EDIT_MODE, "#edit/align.xpm", "#edit/editseq.xpm", 7);

    awmm->at("insert");
    awmm->create_text_toggle(AWAR_INSERT_MODE, "Replace", "Insert", 7);

    awmm->at("direct");
    awmm->create_toggle(AWAR_EDIT_DIRECTION, "#edit/3to5.bitmap", "#edit/5to3.bitmap", 7);

    // ------------------------
    //      secedit / rna3d
    // ------------------------

    int xoffset = 0;

    // Enable RNA3D and SECEDIT programs if the database contains valid alignment of rRNA sequences.
    if (alignment_type == GB_AT_RNA) {
        awmm->button_length(0);

        awmm->at("secedit");
        awmm->callback(ED4_SECEDIT_start, (AW_CL)GLOBAL_gb_main, 0);
        awmm->help_text("arb_secedit.hlp");
        awmm->create_button("SECEDIT", "#edit/secedit.xpm");

#if defined(ARB_OPENGL)
        awmm->at("rna3d");
        awmm->callback(ED4_RNA3D_Start, 0, 0);
        awmm->help_text("rna3d_general.hlp");
        awmm->create_button("RNA3D", "#edit/rna3d.xpm");
#endif // ARB_OPENGL
    }
    else {
        awmm->at("secedit");
        int xsecedit = awmm->get_at_xposition();
        awmm->at("zoom");
        int xzoom    = awmm->get_at_xposition();
        xoffset      = xsecedit-xzoom; // shift message stuff to the left by xoffset
    }

    {
        int x, y;

        awmm->at("zoom");
        if (xoffset) { awmm->get_at_position(&x, &y); awmm->at(x+xoffset, y); }
        awmm->callback(AW_POPUP, (AW_CL)ED4_zoom_message_window, (AW_CL)0);
        awmm->create_button("ZOOM", "#edit/zoom.bitmap");

        awmm->at("clear");
        if (xoffset) { awmm->get_at_position(&x, &y); awmm->at(x+xoffset, y); }
        awmm->callback(ED4_clear_errors, (AW_CL)0);
        awmm->create_button("CLEAR", "#edit/clear.bitmap");

        awmm->at("errortext");
        if (xoffset) { awmm->get_at_position(&x, &y); awmm->at(x+xoffset, y); }
        aw_root->awar_string(AWAR_ERROR_MESSAGES, "This is ARB Edit4 [Build " ARB_VERSION "]");
        awmm->create_text_field(AWAR_ERROR_MESSAGES);
        aw_set_local_message();
    }

    // --------------------
    //      'more' area
    // --------------------

    awmm->at("cons");
    awmm->create_toggle(ED4_AWAR_CONSENSUS_SHOW, "#edit/nocons.bitmap", "#edit/cons.bitmap");

    awmm->at("num");
    awmm->create_toggle(ED4_AWAR_DIGITS_AS_REPEAT, "#edit/norepeat.bitmap", "#edit/repeat.bitmap");

    awmm->at("key");
    awmm->create_toggle("key_mapping/enable", "#edit/nokeymap.bitmap", "#edit/keymap.bitmap");

    // search

    awmm->button_length(0);
#define INSERT_SEARCH_FIELDS(Short, label_prefix, prefix) insert_search_fields(awmm, #label_prefix, #prefix, ED4_AWAR_##prefix##_SEARCH_PATTERN, ED4_##prefix##_PATTERN, ED4_AWAR_##prefix##_SEARCH_SHOW, Short)

    INSERT_SEARCH_FIELDS(0, u1, USER1);
    INSERT_SEARCH_FIELDS(0, u2, USER2);
    INSERT_SEARCH_FIELDS(0, pro, PROBE);
    INSERT_SEARCH_FIELDS(1, pri1, PRIMER1);
    INSERT_SEARCH_FIELDS(1, pri2, PRIMER2);
    INSERT_SEARCH_FIELDS(1, pri3, PRIMER3);
    INSERT_SEARCH_FIELDS(1, sig1, SIG1);
    INSERT_SEARCH_FIELDS(1, sig2, SIG2);
    INSERT_SEARCH_FIELDS(1, sig3, SIG3);

#undef INSERT_SEARCH_FIELDS

    awmm->at("alast");
    awmm->callback(ED4_search, ED4_encodeSearchDescriptor(-1, ED4_ANY_PATTERN));
    awmm->create_button("ALL_SEARCH_LAST", "#edit/last.bitmap");

    awmm->at("anext");
    awmm->callback(ED4_search, ED4_encodeSearchDescriptor(+1, ED4_ANY_PATTERN));
    awmm->create_button("ALL_SEARCH_NEXT", "#edit/next.bitmap");

    title_mode_changed(aw_root, awmm);

    // Buttons at left window border

    awmm->create_mode("edit/arrow.bitmap", "normal.hlp", AWM_ALL, (AW_CB)modes_cb, (AW_CL)0, (AW_CL)0);
    awmm->create_mode("edit/kill.bitmap",  "kill.hlp",   AWM_ALL, (AW_CB)modes_cb, (AW_CL)1, (AW_CL)0);
    awmm->create_mode("edit/mark.bitmap",  "mark.hlp",   AWM_ALL, (AW_CB)modes_cb, (AW_CL)2, (AW_CL)0);

    AWTC_create_faligner_variables(awmm->get_root(), db);

    return (ED4_R_OK);
}

AW_window *ED4_root::create_new_window()                            // only the first time, other cases: generate_window
{
    AW_device  *device     = NULL;
    ED4_window *new_window = NULL;

    generate_window(&device, &new_window);

    ED4_calc_terminal_extentions();

    last_window_reached     = 1;
    DRAW                    = 1;
    move_cursor             = 0;
    max_seq_terminal_length = 0;

    ED4_init_notFoundMessage();

    return (new_window->aww);
}

ED4_index ED4_root::pixel2pos(AW_pos click_x) {
    // 'click_x' is the x-offset into the terminal in pixels
    // returns the x-offset into the terminal in base positions (clipped to max. allowed position)

    int       length_of_char = font_group.get_width(ED4_G_SEQUENCES);
    ED4_index scr_pos        = int((click_x-CHARACTEROFFSET) / length_of_char);
    int       max_scrpos     = root_group_man->remap()->get_max_screen_pos();

    if (scr_pos>max_scrpos) scr_pos = max_scrpos;

    return scr_pos;
}

ED4_root::ED4_root()
{
    memset((char *)this, 0, sizeof(*this));

    aw_root      = new AW_root;
    first_window = NULL;
    main_manager = NULL;
    database     = NULL;
    tmp_ed4w     = NULL;
    tmp_aww      = NULL;
    tmp_device   = NULL;
    temp_gc      = 0;

    scroll_links.link_for_hor_slider = NULL;
    scroll_links.link_for_ver_slider = NULL;

    folding_action = 0;

    species_mode            = ED4_SM_MOVE;
    scroll_picture.scroll   = 0;
    scroll_picture.old_x    = 0;
    scroll_picture.old_y    = 0;
    middle_area_man         = NULL;
    top_area_man            = NULL;
    root_group_man          = NULL;
    ecoli_ref               = NULL;
    protstruct              = NULL;
    protstruct_len          = 0;
    column_stat_activated   = 0;
    column_stat_initialized = 0;
    visualizeSAI            = 0;
    visualizeSAI_allSpecies = 0;

    aw_initstatus();
}


ED4_root::~ED4_root()
{
    delete aw_root;
    delete first_window;
    delete main_manager;
    delete middle_area_man;
    delete top_area_man;
    delete database;
    delete ecoli_ref;
    if (protstruct) free(protstruct);
}
