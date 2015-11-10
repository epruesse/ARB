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
#include "ed4_list.hxx"
#include "ed4_plugins.hxx"
#include "ed4_visualizeSAI.hxx"
#include "ed4_naligner.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"
#include "graph_aligner_gui.hxx"
#include "ed4_colStat.hxx"
#include "ed4_seq_colors.hxx"

#include <ed4_extern.hxx>
#include <fast_aligner.hxx>
#include <AW_helix.hxx>
#include <gde.hxx>
#include <awt.hxx>
#include <awt_map_key.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_advice.hxx>
#include "../WINDOW/aw_status.hxx" // @@@ hack - obsolete when EDIT4 status works like elsewhere
#include <arb_version.h>
#include <arb_file.h>
#include <arbdbt.h>
#include <ad_cb.h>
#include <macros.hxx>
#include <st_window.hxx>
#include <rootAsWin.h>

#include <cctype>
#include <map>

AW_window *AWTC_create_island_hopping_window(AW_root *root, AW_CL);

ED4_WinContext ED4_WinContext::current_context;

void ED4_folding_line::warn_illegal_dimension() {
#if defined(DEBUG)
    if (dimension<0.0) {
        // e4_assert(0); // crashes gdb when called from scrollbar callback - so instead only dump backtrace
        const char *msg = GBS_global_string("illegal dimension %f\n", dimension);
        GBK_dump_backtrace(stderr, msg);
    }
#endif
}

void ED4_WinContext::warn_missing_context() const {
    e4_assert(0);
    GBK_dump_backtrace(stderr, "Missing context");
    aw_message("Missing context - please send information from console to devel@arb-home.de");
}

static ARB_ERROR request_terminal_refresh(ED4_base *base, ED4_level lev) {
    if (lev == ED4_L_NO_LEVEL || (base->spec.level&lev) != 0) {
        if (base->is_terminal()) base->request_refresh();
    }
    return NULL;
}

void ED4_root::request_refresh_for_all_terminals() {
    main_manager->route_down_hierarchy(makeED4_route_cb(request_terminal_refresh, ED4_L_NO_LEVEL)).expect_no_error();
}

void ED4_root::request_refresh_for_specific_terminals(ED4_level lev) {
    main_manager->route_down_hierarchy(makeED4_route_cb(request_terminal_refresh, lev)).expect_no_error();
}


static ARB_ERROR request_sequence_refresh(ED4_base *base, bool consensi) {
    ARB_ERROR error;
    if (base->spec.level & ED4_L_SPECIES) {
        if (base->is_consensus_manager() == consensi) {
            error = base->to_manager()->route_down_hierarchy(makeED4_route_cb(request_terminal_refresh, ED4_L_SEQUENCE_STRING));
        }
    }
    return error;
}

// if you want to refresh consensi AND sequences you may use request_refresh_for_specific_terminals(ED4_L_SEQUENCE_STRING)
void ED4_root::request_refresh_for_consensus_terminals() {
    main_manager->route_down_hierarchy(makeED4_route_cb(request_sequence_refresh, true)).expect_no_error();
}
void ED4_root::request_refresh_for_sequence_terminals() {
    if (main_manager) {
        main_manager->route_down_hierarchy(makeED4_route_cb(request_sequence_refresh, false)).expect_no_error();
    }
}

void ED4_root::refresh_window_simple(bool redraw) {
    // if 'redraw' -> update everything (ignoring refresh flag)
    int refresh_all = 0;
    if (redraw) {
#if defined(TRACE_REFRESH)
        fprintf(stderr, "- clear display (refresh_window_simple(redraw=true) called)\n"); fflush(stderr);
#endif
        main_manager->update_info.set_clear_at_refresh(1);
        refresh_all = 1;
    }

    main_manager->Show(refresh_all, 0);
    if (redraw) main_manager->update_info.set_clear_at_refresh(0);
}

void ED4_root::handle_update_requests(bool& redraw) {
    if (main_manager->update_info.delete_requested) {
#if defined(TRACE_REFRESH)
        fprintf(stderr, "- handling requested deletes\n"); fflush(stderr);
#endif
        main_manager->delete_requested_children();
        redraw = true;
    }

    if (main_manager->update_info.update_requested) {
#if defined(TRACE_REFRESH)
        fprintf(stderr, "- handling requested updates\n"); fflush(stderr);
#endif
        main_manager->update_requested_children();
        redraw = true; // @@@ needed ? 
    }

    while (main_manager->update_info.resize) {
#if defined(TRACE_REFRESH)
        fprintf(stderr, "- handling requested resizes\n"); fflush(stderr);
#endif
        main_manager->resize_requested_children();
        redraw = true;
    }

    // make sure all update request have been handled:
    e4_assert(!main_manager->update_info.delete_requested);
    e4_assert(!main_manager->update_info.update_requested);
    e4_assert(!main_manager->update_info.resize);
}

void ED4_root::special_window_refresh(bool handle_updates) {
    // this function should only be used for window specific updates (e.g. cursor placement/deletion)
    e4_assert(ED4_WinContext::have_context());

    bool redraw = true;
    if (handle_updates) handle_update_requests(redraw);
    refresh_window_simple(redraw);
    // do NOT clear_refresh_requests here!! this is no full refresh!
}

ED4_returncode ED4_root::refresh_all_windows(bool redraw) {
    // if 'redraw' -> update everything (ignoring refresh flag)
    GB_transaction ta(GLOBAL_gb_main);

    handle_update_requests(redraw);
    
    ED4_window *window = first_window;
    while (window) {
        ED4_LocalWinContext uses(window);
        refresh_window_simple(redraw);
        window = window->next;
    }

    if (main_manager->update_info.refresh) main_manager->clear_refresh();

    if (loadable_SAIs == LSAI_OUTDATED) {
        GB_touch(GBT_get_SAI_data(GLOBAL_gb_main)); // touch SAI data to force update of SAI selection list
        loadable_SAIs = LSAI_UPTODATE;
    }

    return (ED4_R_OK);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation correct_win2world_calculation(ED4_foldable& foldable, int xwin_org, int ywin_org, int xwrld_expd, int ywrld_expd) {
    using namespace arb_test;
    match_expectation precondition(all().of(that(xwrld_expd).is_more_or_equal(xwin_org),
                                            that(ywrld_expd).is_more_or_equal(ywin_org)));

    AW_pos xwrld_calc = xwin_org;
    AW_pos ywrld_calc = ywin_org;
    foldable.win_to_world_coords(&xwrld_calc, &ywrld_calc);

    match_expectation win_2_world_conversion(all().of(that(xwrld_calc).is_equal_to(xwrld_expd),
                                                      that(ywrld_calc).is_equal_to(ywrld_expd)));

    AW_pos xwin_back = xwrld_calc;
    AW_pos ywin_back = ywrld_calc;
    foldable.world_to_win_coords(&xwin_back, &ywin_back);

    match_expectation world_back2_win_conversion(all().of(that(xwin_back).is_equal_to(xwin_org),
                                                          that(ywin_back).is_equal_to(ywin_org)));

    return all().of(precondition, win_2_world_conversion, world_back2_win_conversion);
}

static arb_test::match_expectation correct_world2win_calculation(ED4_foldable& foldable, int xwrld_org, int ywrld_org, int xwin_expd, int ywin_expd) {
    using namespace arb_test;
    match_expectation precondition(all().of(that(xwrld_org).is_more_or_equal(xwin_expd),
                                            that(ywrld_org).is_more_or_equal(ywin_expd)));


    AW_pos xwin_calc = xwrld_org;
    AW_pos ywin_calc = ywrld_org;
    foldable.world_to_win_coords(&xwin_calc, &ywin_calc);

    match_expectation world_2_win_conversion(all().of(that(xwin_calc).is_equal_to(xwin_expd),
                                                      that(ywin_calc).is_equal_to(ywin_expd)));

    return all().of(precondition, world_2_win_conversion);
}

#define TEST_EXPECT_WIN_UNFOLDED(xwi,ywi)              TEST_EXPECTATION(correct_win2world_calculation(foldable, xwi, ywi, xwi, ywi))
#define TEST_EXPECT_WIN_WORLD_FOLDING(xwi,ywi,fx,fy)   TEST_EXPECTATION(correct_win2world_calculation(foldable, xwi, ywi, (xwi)+(fx), (ywi)+(fy)))
#define TEST_EXPECT_WORLD_WIN_FOLDING(xwo,ywo,xwi,ywi) TEST_EXPECTATION(correct_world2win_calculation(foldable, xwo, ywo, xwi, ywi))

#define TEST_EXPECT_WIN_WORLD_FOLDING__BROKEN(xwi,ywi,fx,fy)        TEST_EXPECTATION__BROKEN(correct_win2world_calculation(foldable, xwi, ywi, (xwi)+(fx), (ywi)+(fy)))
#define TEST_EXPECT_WIN_WORLD_FOLDING__BROKENIF(when,xwi,ywi,fx,fy) TEST_EXPECTATION__BROKENIF(when, correct_win2world_calculation(foldable, xwi, ywi, (xwi)+(fx), (ywi)+(fy)))

void TEST_win_2_world() {
    ED4_foldable foldable;

    const int X1 = 200;
    const int X2 = 300;
    const int Y1 = 100;
    const int Y2 = 200;

    ED4_folding_line *hor100 = foldable.insert_folding_line(Y1, 0, ED4_P_HORIZONTAL);
    ED4_folding_line *ver200 = foldable.insert_folding_line(X1, 0, ED4_P_VERTICAL);
    ED4_folding_line *hor200 = foldable.insert_folding_line(Y2, 0, ED4_P_HORIZONTAL);
    ED4_folding_line *ver300 = foldable.insert_folding_line(X2, 0, ED4_P_VERTICAL);

    // nothing folded yet

    const int x01 = X1/2;
    const int x12 = (X1+X2)/2;
    const int x23 = X2+x01;

    const int y01 = Y1/2;
    const int y12 = (Y1+Y2)/2;
    const int y23 = Y2+y01;

    TEST_EXPECT_WIN_UNFOLDED(x01, y01);
    TEST_EXPECT_WIN_UNFOLDED(x12, y01);
    TEST_EXPECT_WIN_UNFOLDED(x23, y01);

    TEST_EXPECT_WIN_UNFOLDED(x01, y12);
    TEST_EXPECT_WIN_UNFOLDED(x12, y12);
    TEST_EXPECT_WIN_UNFOLDED(x23, y12);

    TEST_EXPECT_WIN_UNFOLDED(x01, y23);
    TEST_EXPECT_WIN_UNFOLDED(x12, y23);
    TEST_EXPECT_WIN_UNFOLDED(x23, y23);

    for (int FACTOR = 1; FACTOR <= 100; FACTOR += 7) {
        TEST_ANNOTATE(GBS_global_string("FACTOR=%i", FACTOR));
        int H1 = FACTOR* 10;
        int H2 = FACTOR* 40;
        int V1 = FACTOR* 20;
        int V2 = FACTOR* 80;

        hor100->set_dimension(H1);
        hor200->set_dimension(H2);
        ver200->set_dimension(V1);
        ver300->set_dimension(V2);

        TEST_EXPECT_WIN_UNFOLDED     (x01, y01); // always in unfolded range
        TEST_EXPECT_WIN_WORLD_FOLDING(x12, y01, V1,    0);
        TEST_EXPECT_WIN_WORLD_FOLDING(x23, y01, V1+V2, 0);

        TEST_EXPECT_WIN_WORLD_FOLDING(x01, y12, 0,     H1);
        TEST_EXPECT_WIN_WORLD_FOLDING(x12, y12, V1,    H1);
        TEST_EXPECT_WIN_WORLD_FOLDING(x23, y12, V1+V2, H1);

        TEST_EXPECT_WIN_WORLD_FOLDING(x01, y23, 0,     H1+H2);
        TEST_EXPECT_WIN_WORLD_FOLDING(x12, y23, V1,    H1+H2);
        TEST_EXPECT_WIN_WORLD_FOLDING(x23, y23, V1+V2, H1+H2);

        // test "folded" world positions.
        // they result in win positions lower than folding line!
        TEST_EXPECT_WORLD_WIN_FOLDING(X1-1,    Y1-1,    X1-1,    Y1-1);    // left of/above folded range
        TEST_EXPECT_WORLD_WIN_FOLDING(X1,      Y1,      X1-V1,   Y1-H1);   // left/upper end of folded range
        TEST_EXPECT_WORLD_WIN_FOLDING(X1+5,    Y1+5,    X1+5-V1, Y1+5-H1); // in folded range
        TEST_EXPECT_WORLD_WIN_FOLDING(X1+V1,   Y1+H1,   X1,      Y1);      // right/lower end of folded range
        TEST_EXPECT_WORLD_WIN_FOLDING(X1+V1+1, Y1+H1+1, X1+1,    Y1+1);    // right of/below folded range
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

ED4_returncode ED4_root::deselect_all()
{
    ED4_multi_species_manager *main_multi_man = middle_area_man->get_multi_species_manager();
    main_multi_man->deselect_all_species_and_SAI();

    main_multi_man = top_area_man->get_multi_species_manager();
    main_multi_man->deselect_all_species_and_SAI();

    return ED4_R_OK;
}

void ED4_root::remove_from_selected(ED4_species_name_terminal *name_term) { // @@@ change param to ED4_species_manager ?
    if (name_term) {
        if ((selected_objects->has_elem(name_term->selection_info))) {
            selected_objects->remove_elem(name_term->selection_info);

            delete name_term->selection_info;
            name_term->selection_info    = NULL;
            name_term->containing_species_manager()->set_selected(false);
            name_term->dragged = false; // @@@ caller shall do this

#ifdef DEBUG
            GBDATA *gbd = name_term->get_species_pointer();

            if (gbd) {
                printf("removed term '%s'\n", GB_read_char_pntr(gbd));
            }
            else {
                ED4_species_manager *spec_man = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();

                if (spec_man->is_consensus_manager()) {
                    printf("removed consensus '%s'\n", name_term->id);
                }
                else {
                    printf("removed unknown term '%s'\n", name_term->id ? name_term->id : "NULL");
                }
            }
#endif

            name_term->request_refresh();
            ED4_sequence_terminal *seq_term = name_term->corresponding_sequence_terminal();
            if (seq_term) seq_term->request_refresh();

            // ProtView: Refresh corresponding orf terminals
            if (alignment_type == GB_AT_DNA) {
                PV_CallBackFunction(this->aw_root);
            }

            ED4_multi_species_manager *multi_man = name_term->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
            multi_man->invalidate_species_counters();
        }
    }
}

void ED4_root::announce_useraction_in(AW_window *aww) {
    for (ED4_window *win = first_window; win; win = win->next) {
        if (win->aww == aww) {
            most_recently_used_window = win;
        }
    }
}

ED4_returncode ED4_root::add_to_selected(ED4_species_name_terminal *name_term) { // @@@ change param to ED4_species_manager ?
    if (!name_term || !(name_term->dynamic_prop & ED4_P_SELECTABLE)) {   // check if object exists and may be selected
        return (ED4_R_IMPOSSIBLE);
    }

    if (!(selected_objects->has_elem(name_term->selection_info))) {     // object is really new to our list => calculate current extension and append it
        ED4_selection_entry *sel_info = new ED4_selection_entry;
        name_term->selection_info     = sel_info;

        if (name_term->dynamic_prop & ED4_P_IS_HANDLE) { // object is a handle for an object up in the hierarchy => search it
            ED4_level  mlevel     = name_term->spec.handled_level;
            ED4_base  *tmp_object = name_term;

            while ((tmp_object != NULL) && !(tmp_object->spec.level & mlevel)) {
                tmp_object = tmp_object->parent;
            }

            if (!tmp_object) return (ED4_R_WARNING); // no target level found

            sel_info->actual_width  = tmp_object->extension.size[WIDTH];
            sel_info->actual_height = tmp_object->extension.size[HEIGHT];
        }
        else // selected object is no handle => take it directly
        {
            sel_info->actual_width  = name_term->extension.size[WIDTH];
            sel_info->actual_height = name_term->extension.size[HEIGHT];
        }

        sel_info->drag_old_x  = 0;
        sel_info->drag_old_y  = 0;
        sel_info->drag_off_x  = 0;
        sel_info->drag_off_y  = 0;
        sel_info->old_event_y = 0;

        sel_info->object = name_term;

        selected_objects->prepend_elem(sel_info);

        name_term->containing_species_manager()->set_selected(true);

#ifdef DEBUG
        GBDATA *gbd = name_term->get_species_pointer();

        if (gbd) {
            printf("added term '%s'\n", GB_read_char_pntr(gbd));
        }
        else {
            ED4_species_manager *spec_man = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
            if (spec_man->is_consensus_manager()) {
                printf("added consensus '%s'\n", name_term->id);
            }
            else {
                printf("added unknown term '%s'\n", name_term->id ? name_term->id : "NULL");
            }
        }
#endif

        name_term->request_refresh();
        ED4_sequence_terminal *seq_term = name_term->corresponding_sequence_terminal();
        if (seq_term) seq_term->request_refresh();

        // ProtView: Refresh corresponding orf terminals
        if (alignment_type == GB_AT_DNA) {
            PV_CallBackFunction(this->aw_root);
        }

        ED4_multi_species_manager *multi_man = name_term->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
        multi_man->invalidate_species_counters();

        return (ED4_R_OK);
    }

    return (ED4_R_IMPOSSIBLE);
}

void ED4_root::resize_all() {
    while (main_manager->update_info.resize) {
        main_manager->resize_requested_children();
    }
}

static ARB_ERROR change_char_table_length(ED4_base *base, int new_length) {
    if (base->is_abstract_group_manager()) {
        ED4_abstract_group_manager *group_man = base->to_abstract_group_manager();
        group_man->table().change_table_length(new_length);
    }
    return NULL;
}

void ED4_alignment_length_changed(GBDATA *gb_alignment_len, GB_CB_TYPE IF_ASSERTION_USED(gbtype)) // callback from database
{
    e4_assert(gbtype==GB_CB_CHANGED);
    int new_length = GB_read_int(gb_alignment_len);

#if defined(DEBUG) && 1
    printf("alignment_length_changed from %i to %i\n", MAXSEQUENCECHARACTERLENGTH, new_length);
#endif

    if (MAXSEQUENCECHARACTERLENGTH!=new_length) { // otherwise we already did this (i.e. we were called by changed_by_database)
        bool was_increased = new_length>MAXSEQUENCECHARACTERLENGTH;

        MAXSEQUENCECHARACTERLENGTH = new_length;

        {
            const char *err = ED4_ROOT->helix->init(GLOBAL_gb_main); // reload helix
            aw_message_if(err);
        }
        {
            const char *err = ED4_ROOT->ecoli_ref->init(GLOBAL_gb_main); // reload ecoli
            aw_message_if(err);
        }

        if (ED4_ROOT->alignment_type == GB_AT_AA) {
            // TODO: is this needed here?
            const char *err = ED4_pfold_set_SAI(&ED4_ROOT->protstruct, GLOBAL_gb_main, ED4_ROOT->alignment_name, &ED4_ROOT->protstruct_len); // reload protstruct
            aw_message_if(err);
        }

        if (was_increased) {
            ED4_ROOT->main_manager->route_down_hierarchy(makeED4_route_cb(change_char_table_length, new_length)).expect_no_error();
            ED4_ROOT->root_group_man->remap()->mark_compile_needed_force();
        }
    }
}

ARB_ERROR ED4_root::init_alignment() {
    GB_transaction ta(GLOBAL_gb_main);

    alignment_name = GBT_get_default_alignment(GLOBAL_gb_main);
    alignment_type = GBT_get_alignment_type(GLOBAL_gb_main, alignment_name);
    if (alignment_type==GB_AT_UNKNOWN) {
        return GBS_global_string("You have to select a valid alignment before you can start ARB_EDIT4\n(%s)", GB_await_error());
    }

    GBDATA *gb_alignment = GBT_get_alignment(GLOBAL_gb_main, alignment_name);
    if (!gb_alignment) {
        return GBS_global_string("You can't edit without an existing alignment\n(%s)", GB_await_error());
    }

    GBDATA *gb_alignment_len = GB_search(gb_alignment, "alignment_len", GB_FIND);
    int alignment_length = GB_read_int(gb_alignment_len);
    MAXSEQUENCECHARACTERLENGTH = alignment_length;

    GB_add_callback(gb_alignment_len, GB_CB_CHANGED, makeDatabaseCallback(ED4_alignment_length_changed));

    aw_root->awar_string(AWAR_EDITOR_ALIGNMENT, alignment_name);

    return NULL;
}

void ED4_root::recalc_font_group() {
    font_group.unregisterAll();
    for (int f=ED4_G_FIRST_FONT; f<=ED4_G_LAST_FONT; f++) {
        ED4_MostRecentWinContext context;
        font_group.registerFont(current_device(), f);
    }
}

static ARB_ERROR force_group_update(ED4_base *base) {
    if (base->is_multi_species_manager()) {
        base->to_multi_species_manager()->update_requested_by_child();
    }
    return NULL;
}

ED4_returncode ED4_root::create_hierarchy(const char *area_string_middle, const char *area_string_top) {
    // creates internal hierarchy of editor

    long total_no_of_groups  = 0;
    long total_no_of_species = 0;
    {
        // count species and related info (e.g. helix) displayed in the top region (a == 0)
        // count no of species and sais including groups in middle region (a == 1)

        const char *area_string[2] = { area_string_top, area_string_middle };
        for (int a = 0; a<2; ++a) {
            long group_count;
            long species_count;
            database->calc_no_of_all(area_string[a], &group_count, &species_count);

            total_no_of_groups  += group_count;
            total_no_of_species += species_count;
        }
    }

    SmartPtr<arb_progress> startup_progress = new arb_progress("EDIT4 startup");

    GB_push_transaction(GLOBAL_gb_main);

    ecoli_ref = new BI_ecoli_ref;
    ecoli_ref->init(GLOBAL_gb_main);

    // [former position of ali-init-code]

    main_manager   = new ED4_main_manager("Main_Manager", 0, 0, 0, 0, NULL); // build a test hierarchy
    root_group_man = new ED4_root_group_manager("Root_Group_Manager", 0, 0, 0, 0, main_manager);
    
    main_manager->children->append_member(root_group_man);

    ED4_device_manager *device_manager = new ED4_device_manager("Device_Manager", 0, 0, 0, 0, root_group_man);
    root_group_man->children->append_member(device_manager);

    ED4_calc_terminal_extentions();

    {
        int col_stat_term_height = 50; // @@@ Hoehe des ColumnStatistics Terminals ausrechnen

        ref_terminals.init(new ED4_sequence_info_terminal("Reference_Sequence_Info_Terminal", /* NULL, */ 250, 0, MAXINFOWIDTH, TERMINALHEIGHT, NULL),
                           new ED4_sequence_terminal("Reference_Sequence_Terminal", 300, 0, 300, TERMINALHEIGHT, NULL, false),
                           new ED4_sequence_info_terminal("Reference_ColumnStatistics_Info_Terminal", /* NULL, */ 250, 0, MAXINFOWIDTH, col_stat_term_height, NULL),
                           new ED4_columnStat_terminal("Reference_ColumnStatistics_Terminal", 300, 0, 300, col_stat_term_height, NULL));
    }


    recalc_font_group();

    ED4_index y = 0;

    {
        arb_progress species_progress("Loading species", total_no_of_species);
        
        const int XPOS_MULTIMAN = 100;

        // ********** Top Area beginning **********

        ED4_multi_species_manager *top_multi_species_manager;
        ED4_spacer_terminal       *top_spacer_terminal;
        ED4_spacer_terminal       *top_mid_spacer_terminal;
        ED4_spacer_terminal       *top_multi_spacer_terminal_beg;
        ED4_line_terminal         *top_mid_line_terminal;
        {
            ED4_area_manager *top_area_manager = new ED4_area_manager("Top_Area_Manager", 0, y, 0, 0, device_manager);
            device_manager->children->append_member(top_area_manager);
            top_area_man = top_area_manager;

            top_spacer_terminal = new ED4_spacer_terminal("Top_Spacer", true, 0, 0, 100, 10, top_area_manager);
            top_area_manager->children->append_member(top_spacer_terminal);

            top_multi_species_manager = new ED4_multi_species_manager("Top_MultiSpecies_Manager", XPOS_MULTIMAN, 0, 0, 0, top_area_manager);
            top_area_manager->children->append_member(top_multi_species_manager);

            top_multi_spacer_terminal_beg = new ED4_spacer_terminal("Top_Multi_Spacer_Terminal_Beg", true, 0, 0, 0, 3, top_multi_species_manager);
            top_multi_species_manager->children->append_member(top_multi_spacer_terminal_beg);

            y += 3;

            reference = new ED4_reference(GLOBAL_gb_main);

            int index = 0;
            database->scan_string(top_multi_species_manager, ref_terminals.get_ref_sequence_info(), ref_terminals.get_ref_sequence(),
                                  area_string_top, &index, &y, species_progress);
            GB_pop_transaction(GLOBAL_gb_main);

            const int TOP_MID_LINE_HEIGHT   = 3;
            int       TOP_MID_SPACER_HEIGHT = font_group.get_max_height()-TOP_MID_LINE_HEIGHT;

            top_mid_line_terminal = new ED4_line_terminal("Top_Mid_Line_Terminal", 0, y, 0, TOP_MID_LINE_HEIGHT, device_manager);    // width will be set below
            device_manager->children->append_member(top_mid_line_terminal);

            y += TOP_MID_LINE_HEIGHT;


            top_mid_spacer_terminal = new ED4_spacer_terminal("Top_Middle_Spacer", true, 0, y, 880, TOP_MID_SPACER_HEIGHT,   device_manager);
            device_manager->children->append_member(top_mid_spacer_terminal);

            // needed to avoid text-clipping problems:
            main_manager->set_top_middle_spacer_terminal(top_mid_spacer_terminal);
            main_manager->set_top_middle_line_terminal(top_mid_line_terminal);

            y += TOP_MID_SPACER_HEIGHT; // add top-mid_spacer_terminal height
        }

        // ********** Middle Area beginning **********

        ED4_area_manager          *middle_area_manager;
        ED4_tree_terminal         *tree_terminal;
        ED4_multi_species_manager *mid_multi_species_manager;
        ED4_spacer_terminal       *mid_multi_spacer_terminal_beg;
        ED4_line_terminal         *mid_bot_line_terminal;
        ED4_spacer_terminal       *total_bottom_spacer;
        {
            middle_area_manager = new ED4_area_manager("Middle_Area_Manager", 0, y, 0, 0, device_manager);
            device_manager->children->append_member(middle_area_manager);
            middle_area_man = middle_area_manager;

            tree_terminal = new ED4_tree_terminal("Tree", 0, 0, 2, 0, middle_area_manager);
            middle_area_manager->children->append_member(tree_terminal);

            mid_multi_species_manager = new ED4_multi_species_manager("Middle_MultiSpecies_Manager", XPOS_MULTIMAN, 0, 0, 0, middle_area_manager);
            middle_area_manager->children->append_member(mid_multi_species_manager);

            mid_multi_spacer_terminal_beg = new ED4_spacer_terminal("Mid_Multi_Spacer_Terminal_Beg", true, 0, 0, 0, 3, mid_multi_species_manager);
            mid_multi_species_manager->children->append_member(mid_multi_spacer_terminal_beg);

            y+=3;               // dummy height, to create a dummy layout ( to preserve order of objects )

            scroll_links.link_for_ver_slider = middle_area_manager;

            ED4_index help = y;
            {
                GB_transaction ta(GLOBAL_gb_main);
                int index  = 0;
                database->scan_string(mid_multi_species_manager, ref_terminals.get_ref_sequence_info(), ref_terminals.get_ref_sequence(),
                                      area_string_middle, &index, &y, species_progress);
            }

            {
                ED4_spacer_terminal *mid_bot_spacer_terminal = new ED4_spacer_terminal("Middle_Bot_Spacer_Terminal", true, 0, y, 880, 10, device_manager);
                device_manager->children->append_member(mid_bot_spacer_terminal);
            }

            tree_terminal->extension.size[HEIGHT] = y - help;

            y += 10; // add top-mid_spacer_terminal height

            mid_bot_line_terminal = new ED4_line_terminal("Mid_Bot_Line_Terminal", 0, y, 0, 3, device_manager);    // width will be set below
            device_manager->children->append_member(mid_bot_line_terminal);
            y += 3;

            total_bottom_spacer = new ED4_spacer_terminal("Total_Bottom_Spacer_terminal", true, 0, y, 0, 10000, device_manager);
            device_manager->children->append_member(total_bottom_spacer);
            y += 10000;
        }

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

        species_progress.done();
    }

    first_window->update_window_coords();
    resize_all();

    main_manager->route_down_hierarchy(makeED4_route_cb(force_group_update)).expect_no_error();

    if (!scroll_links.link_for_hor_slider) { // happens when no species AND no SAI has data
        startup_progress->done();
        startup_progress.SetNull(); // make progress disappear (otherwise prompter below is often behind progress window)
        GB_pop_transaction(GLOBAL_gb_main);
        
        aw_popup_ok(GBS_global_string("No species/SAI contains data in '%s'\nARB_EDIT4 will terminate", alignment_name));
        ED4_exit();
    }

    // build consensi
    {
        arb_progress consensi_progress("Initializing consensi", total_no_of_species+total_no_of_groups+1); // 1 is root_group_man

        root_group_man->create_consensus(root_group_man, &consensi_progress);
        e4_assert(root_group_man->table().ok());
        consensi_progress.done(); // if there is a "top"-group, progress increment is one to low
    }

    root_group_man->remap()->mark_compile_needed_force();
    root_group_man->update_remap();

    // calc size and display:

    resize_all();

    e4_assert(main_manager);

    {
        ED4_base *x_link      = scroll_links.link_for_hor_slider;
        ED4_base *y_link      = scroll_links.link_for_ver_slider;
        ED4_base *width_link  = x_link;
        ED4_base *height_link = y_link;

        ED4_window *win = first_window;
        while (win) {
            win->set_scrolled_rectangle(x_link, y_link, width_link, height_link);
            win->aww->show();
            win->update_scrolled_rectangle();
            win = win->next;
        }
    }

    ED4_trigger_instant_refresh();
    ED4_finish_and_show_notFoundMessage();

    return ED4_R_OK;
}

ED4_returncode ED4_root::get_area_rectangle(AW_screen_area *rect, AW_pos x, AW_pos y) {
    // returns win-coordinates of area (defined by folding lines) which contains position x/y
    int                    x1, x2, y1, y2;
    const AW_screen_area&  area_rect = current_device()->get_area_size();

    x1 = area_rect.l;
    for (const ED4_folding_line *flv=current_ed4w()->get_vertical_folding(); ; flv = flv->get_next()) {
        if (flv) {
            x2 = int(flv->get_pos()); // @@@ use AW_INT ? 
        }
        else {
            x2 = area_rect.r;
            if (x1==x2) {
                break;
            }
        }

        y1 = area_rect.t;
        for (const ED4_folding_line *flh=current_ed4w()->get_horizontal_folding(); ; flh = flh->get_next()) {
            if (flh) {
                y2 = int(flh->get_pos()); // @@@ use AW_INT ? 
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

void ED4_root::copy_window_struct(ED4_window *source,   ED4_window *destination)
{
    destination->slider_pos_horizontal  = source->slider_pos_horizontal;
    destination->slider_pos_vertical    = source->slider_pos_vertical;
    destination->coords         = source->coords;
}


static void reload_helix_cb() { 
    const char *err = ED4_ROOT->helix->init(GLOBAL_gb_main);
    if (err) aw_message(err);
    ED4_request_full_refresh();
}


static void reload_ecoli_cb() {
    const char *err = ED4_ROOT->ecoli_ref->init(GLOBAL_gb_main);
    if (err) aw_message(err);
    ED4_request_full_refresh();
}

// ---------------------------------------
//      recursion through all species

typedef ARB_ERROR (*ED4_Species_Callback)(GBDATA*, AW_CL);

static ARB_ERROR do_sth_with_species(ED4_base *base, ED4_Species_Callback cb, AW_CL cd) {
    ARB_ERROR error = NULL;

    if (base->is_species_manager()) {
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
    return ED4_ROOT->root_group_man->route_down_hierarchy(makeED4_route_cb(do_sth_with_species, cb, cd));
}

static bool is_named(ED4_base *base, AW_CL cl_species_name) {
    ED4_species_name_terminal *name_term = base->to_species_name_terminal();
    GBDATA                    *gbd       = name_term->get_species_pointer();
    if (gbd) {
        const char *wanted_name = (const char*)cl_species_name;
        const char *name        = GB_read_char_pntr(gbd);
        e4_assert(name);

        return strcmp(name, wanted_name) == 0;
    }
    return false;
}

static bool is_species_named(ED4_base *base, AW_CL cl_species_name) {
    return base->inside_species_seq_manager() && is_named(base, cl_species_name);
}

static bool is_SAI_named(ED4_base *base, AW_CL cl_species_name) {
    return base->inside_SAI_manager() && is_named(base, cl_species_name);
}

ED4_species_name_terminal *ED4_find_species_or_SAI_name_terminal(const char *species_name) {
    ED4_base *base = ED4_ROOT->root_group_man->find_first_that(ED4_L_SPECIES_NAME, is_species_named, (AW_CL)species_name);
    return base ? base->to_species_name_terminal() : NULL;
}
ED4_species_name_terminal *ED4_find_species_name_terminal(const char *species_name) {
    ED4_base *base = ED4_ROOT->root_group_man->find_first_that(ED4_L_SPECIES_NAME, is_species_named, (AW_CL)species_name);
    return base ? base->to_species_name_terminal() : NULL;
}
ED4_species_name_terminal *ED4_find_SAI_name_terminal(const char *sai_name) {
    ED4_base *base = ED4_ROOT->root_group_man->find_first_that(ED4_L_SPECIES_NAME, is_SAI_named, (AW_CL)sai_name);
    return base ? base->to_species_name_terminal() : NULL;
}

static char *get_group_consensus(const char *species_name, PosRange range) {
    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(species_name);
    char *consensus = 0;

    if (name_term) {
        ED4_abstract_group_manager *group_man = name_term->get_parent(ED4_level(ED4_L_GROUP|ED4_L_ROOTGROUP))->to_abstract_group_manager();
        if (group_man) {
            consensus = group_man->build_consensus_string(range);
        }
    }

    return consensus;
}

static bool get_selected_range(PosRange& range) {
    ED4_selected_elem *listElem = ED4_ROOT->selected_objects->head();
    if (listElem) {
        ED4_sequence_terminal *seqTerm = listElem->elem()->object->corresponding_sequence_terminal();
        return ED4_get_selected_range(seqTerm, range);
    }
    return false;
}

static ED4_selected_elem *curr_aligner_elem = 0;

static GBDATA *get_next_selected_species() {
    if (!curr_aligner_elem) return 0;

    ED4_species_manager *specMan = curr_aligner_elem->elem()->object->containing_species_manager();
    curr_aligner_elem = curr_aligner_elem->next();
    return specMan->get_species_pointer();
}

static GBDATA *get_first_selected_species(int *total_no_of_selected_species)
{
    int selected = ED4_ROOT->selected_objects->size();

    if (total_no_of_selected_species) {
        *total_no_of_selected_species = selected;
    }

    if (selected) {
        curr_aligner_elem = ED4_ROOT->selected_objects->head();
    }
    else {
        curr_aligner_elem = 0;
    }

    return get_next_selected_species();
}

static ARB_ERROR ED4_delete_temp_entries(GBDATA *species, AW_CL cl_alignment_name) {
    return FastAligner_delete_temp_entries(species, (GB_CSTR)cl_alignment_name);
}

static void ED4_remove_faligner_entries() {
    ARB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);
    if (!error) error = ED4_with_all_loaded_species(ED4_delete_temp_entries, (AW_CL)ED4_ROOT->alignment_name);
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
        gbd = species ? GBT_find_sequence(species, ali) : NULL;
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

static void toggle_helix_for_SAI(AW_window *aww) {
    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();

    if (cursor->in_SAI_terminal()) {
        ED4_sequence_terminal      *sai_term      = cursor->owner_of_cursor->to_sequence_terminal();
        ED4_sequence_info_terminal *sai_info_term = sai_term->parent->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();

        GBDATA         *gb_sai_data = sai_info_term->data();
        GB_transaction  ta(gb_sai_data);

        GBDATA *gb_sai      = GB_get_grandfather(gb_sai_data);
        GBDATA *gb_disp_sec = GB_searchOrCreate_int(gb_sai, "showsec", 0);

        bool show_sec = 1-bool(GB_read_int(gb_disp_sec));
        GB_ERROR error = GB_write_int(gb_disp_sec, show_sec);
        if (!error) {
            sai_term->set_secstruct_display(show_sec);
            sai_term->request_refresh();
        }
        if (error) aw_message(error);
    }
    else {
        aw_message("Please select an SAI");
    }
}

static void title_mode_changed(AW_root *aw_root, AW_window *aww) {
    int title_mode = aw_root->awar(AWAR_EDIT_TITLE_MODE)->read_int();

    if (title_mode==0) {
        aww->set_info_area_height(57);
    }
    else {
        aww->set_info_area_height(170);
    }
}

static void ED4_undo_redo(AW_window *aww, GB_UNDO_TYPE undo_type) {
    ED4_LocalWinContext uses(aww);
    GB_ERROR error = GB_undo(GLOBAL_gb_main, undo_type);

    if (error) {
        aw_message(error);
    }
    else {
        GB_begin_transaction(GLOBAL_gb_main);
        GB_commit_transaction(GLOBAL_gb_main);
        ED4_cursor *cursor = &current_cursor();
        if (cursor->owner_of_cursor) cursor->owner_of_cursor->request_refresh();
    }
}

static AW_window *ED4_zoom_message_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "ZOOM_ERR_MSG", "Errors and warnings");
    aws->load_xfig("edit4/message.fig");

    aws->callback(AW_POPDOWN);
    aws->at("hide");
    aws->create_button("HIDE", "HIDE");

    aws->callback(aw_clear_message_cb);
    aws->at("clear");
    aws->create_button("CLEAR", "CLEAR");

    aws->at("errortext");
    aws->create_text_field(AWAR_ERROR_MESSAGES);

    return (AW_window *)aws;
}


static char *cat(char *toBuf, const char *s1, const char *s2)
{
    char *buf = toBuf;

    while ((*buf++=*s1++)!=0) ;
    buf--;
    while ((*buf++=*s2++)!=0) ;

    return toBuf;
}

static void insert_search_fields(AW_window_menu_modes *awmm,
                                 const char *label_prefix, const char *macro_prefix,
                                 const char *pattern_awar_name, const char *show_awar_name,
                                 int short_form, ED4_SearchPositionType type, ED4_window *ed4w)
{
    char buf[200];

    if (!short_form) {
        awmm->at(cat(buf, label_prefix, "search"));
        awmm->create_input_field(pattern_awar_name, 30);
    }

    awmm->at(cat(buf, label_prefix, "n"));
    awmm->help_text("e4_search.hlp");
    awmm->callback(makeWindowCallback(ED4_search_cb, ED4_encodeSearchDescriptor(+1, type), ed4w));
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_NEXT"), "#edit/next.xpm");

    awmm->at(cat(buf, label_prefix, "l"));
    awmm->help_text("e4_search.hlp");
    awmm->callback(makeWindowCallback(ED4_search_cb, ED4_encodeSearchDescriptor(-1, type), ed4w));
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_LAST"), "#edit/last.xpm");

    awmm->at(cat(buf, label_prefix, "d"));
    awmm->help_text("e4_search.hlp");
    awmm->callback(makeWindowCallback(ED4_popup_search_window, type));
    awmm->create_button(cat(buf, macro_prefix, "_SEARCH_DETAIL"), "#edit/detail.xpm");

    awmm->at(cat(buf, label_prefix, "s"));
    awmm->create_toggle(show_awar_name);
}

static void ED4_set_protection(AW_window *aww, int wanted_protection) {
    ED4_LocalWinContext uses(aww);
    ED4_cursor *cursor = &current_cursor();
    GB_ERROR    error  = 0;

    if (cursor->owner_of_cursor) {
        ED4_sequence_terminal      *seq_term      = cursor->owner_of_cursor->to_sequence_terminal();
        ED4_sequence_info_terminal *seq_info_term = seq_term->parent->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();
        GBDATA                     *gbd           = seq_info_term->data();

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

enum MenuSelectType {
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
};

static void ED4_menu_select(AW_window *aww, MenuSelectType select) {
    GB_transaction             ta(GLOBAL_gb_main);
    ED4_multi_species_manager *middle_multi_man = ED4_ROOT->middle_area_man->get_multi_species_manager();

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
            middle_multi_man->select_all(true); // only species
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_INVERT: {
            middle_multi_man->invert_selection_of_all_species();
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_SELECT_MARKED: {
            middle_multi_man->marked_species_select(true);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_DESELECT_MARKED: {
            middle_multi_man->marked_species_select(false);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_UNMARK_ALL: {
            GBT_mark_all(GLOBAL_gb_main, 0);
            break;
        }
        case ED4_MS_MARK_SELECTED: {
            middle_multi_man->selected_species_mark(true);
            ED4_correctBlocktypeAfterSelection();
            break;
        }
        case ED4_MS_UNMARK_SELECTED: {
            middle_multi_man->selected_species_mark(false);
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

    ED4_request_full_refresh();
}

static void ED4_menu_perform_block_operation(AW_window *, ED4_blockoperation_type type) {
    ED4_perform_block_operation(type);
}

static void modes_cb(AW_window*, ED4_species_mode smode) {
    ED4_ROOT->species_mode = smode;
    for (ED4_window *win = ED4_ROOT->first_window; win; win = win->next) {
        win->aww->select_mode(smode);
    }
}

void ED4_no_dangerous_modes() {
    if (ED4_ROOT->species_mode == ED4_SM_KILL) {
        modes_cb(NULL, ED4_SM_MOVE);
    }
}

static char *get_helix_string(GBDATA *gb_main, const char *alignment_name) {
    GB_transaction ta(gb_main);

    char   *helix_string = 0;
    char   *helix_name   = GBT_get_default_helix(gb_main);
    GBDATA *gb_helix_con = GBT_find_SAI(gb_main, helix_name);
    if (gb_helix_con) {
        GBDATA *gb_helix = GBT_find_sequence(gb_helix_con, alignment_name);
        if (gb_helix) helix_string = GB_read_string(gb_helix);
    }
    free(helix_name);

    return helix_string;
}

const AlignDataAccess *ED4_get_aligner_data_access() {
    static SmartPtr<AlignDataAccess> aliDataAccess;

    if (aliDataAccess.isNull()) {
        aliDataAccess = new AlignDataAccess(GLOBAL_gb_main,
                                            ED4_ROOT->alignment_name,
                                            true, ED4_trigger_instant_refresh, // perform refresh
                                            get_group_consensus,               // aligner fetches consensus of group of species via this function
                                            get_selected_range,                // aligner fetches column range of selection via this function
                                            get_first_selected_species,        // aligner fetches first and..
                                            get_next_selected_species,         // .. following selected species via these functions
                                            get_helix_string);                 // used by island_hopping (if configured to use sec-structure)
    }

    return aliDataAccess.isSet() ? &*aliDataAccess : NULL;
}

static AW_window *ED4_create_faligner_window(AW_root *awr, const AlignDataAccess *data_access) {
#if defined(DEBUG)
    static const AlignDataAccess *last_data_access = NULL;

    e4_assert(!last_data_access || (last_data_access == data_access)); // there shall be only one AlignDataAccess
    last_data_access = data_access;
#endif

    static AW_window *aws = NULL;
    if (!aws) aws = FastAligner_create_window(awr, data_access);
    return aws;
}

static void ED4_save_properties(AW_window *aw, int mode) {
    AW_save_specific_properties(aw, ED4_propertyName(mode));
}

void ED4_popup_gc_window(AW_window *awp, AW_gc_manager gcman) {
    typedef std::map<AW_gc_manager, AW_window*> gcwin;
    static gcwin win;

    gcwin::iterator found = win.find(gcman);

    AW_window *aww = NULL;
    if (found == win.end()) {
        aww        = AW_create_gc_window(awp->get_root(), gcman);
        win[gcman] = aww;
    }
    else {
        aww = win[gcman];
    }
    aww->activate();
}

static void refresh_on_gc_change_cb() {
    ED4_expose_recalculations();
    ED4_request_full_instant_refresh();
}

ED4_returncode ED4_root::generate_window(AW_device **device, ED4_window **new_window) {
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

    AW_window_menu_modes *awmm = new AW_window_menu_modes;
    {
        int   winNum   = ED4_window::no_of_windows+1;
        char *winName  = winNum>1 ? GBS_global_string_copy("ARB_EDIT4_%i", winNum) : strdup("ARB_EDIT4");
        char *winTitle = GBS_global_string_copy("ARB_EDIT4 *%d* [%s]", winNum, alignment_name);

        awmm->init(aw_root, winName, winTitle, 800, 450);

        free(winTitle);
        free(winName);
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

    ED4_LocalWinContext uses(*new_window);

    // each window has its own gc-manager
    gc_manager = AW_manage_GC(awmm,
                              "ARB_EDIT4",                   // but all gc-managers use the same colors
                              *device,
                              ED4_G_STANDARD,                // GC_Standard configuration
                              ED4_G_DRAG,
                              AW_GCM_DATA_AREA,
                              makeWindowCallback(refresh_on_gc_change_cb), // callback triggering refresh on gc-change
                              true,                          // use color groups

                              "#f8f8f8",
                              "STANDARD$black",              // Standard Color showing sequences
                              "#SEQUENCES (0)$#505050",      // default color for sequences (color 0)
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
    if (!first_gc_manager) first_gc_manager = gc_manager;

    // --------------
    //      File

#if defined(DEBUG)
    AWT_create_debug_menu(awmm);
#endif // DEBUG

    awmm->create_menu("File", "F", AWM_ALL);

    awmm->insert_menu_topic("new_win", "New Editor Window", "W", 0, AWM_ALL, ED4_new_editor_window);
    awmm->sep______________();
    awmm->insert_menu_topic("save_config",                    "Save configuration",        "S", "species_configs_saveload.hlp", AWM_ALL, makeWindowCallback(ED4_saveConfiguration, false));
    awmm->insert_menu_topic(awmm->local_id("save_config_as"), "Save configuration as ...", "a", "species_configs_saveload.hlp", AWM_ALL, ED4_create_saveConfigurationAs_window);
    awmm->insert_menu_topic(awmm->local_id("load_config"),    "Load configuration ...",    "L", "species_configs_saveload.hlp", AWM_ALL, ED4_create_loadConfiguration_window);
    awmm->insert_menu_topic("reload_config",                  "Reload configuration",      "R", "species_configs_saveload.hlp", AWM_ALL, ED4_reloadConfiguration);
    insert_macro_menu_entry(awmm, true);
    awmm->sep______________();
    GDE_load_menu(awmm, AWM_ALL, "Print");
    awmm->sep______________();

    if (clone) awmm->insert_menu_topic("close", "CLOSE", "C", 0, AWM_ALL, ED4_quit_editor);
    else       awmm->insert_menu_topic("quit",   "QUIT",  "Q", 0, AWM_ALL, ED4_quit_editor);

    // ----------------
    //      Create

    awmm->create_menu("Create", "C", AWM_ALL);
    awmm->insert_menu_topic(awmm->local_id("create_species"),                "Create new species",                "n", 0, AWM_ALL, makeCreateWindowCallback(ED4_create_new_seq_window, CREATE_NEW_SPECIES));
    awmm->insert_menu_topic(awmm->local_id("create_species_from_consensus"), "Create new species from consensus", "u", 0, AWM_ALL, makeCreateWindowCallback(ED4_create_new_seq_window, CREATE_FROM_CONSENSUS));
    awmm->insert_menu_topic(awmm->local_id("copy_species"),                  "Copy current species",              "C", 0, AWM_ALL, makeCreateWindowCallback(ED4_create_new_seq_window, COPY_SPECIES));
    awmm->sep______________();
    awmm->insert_menu_topic("create_group",           "Create new Group",              "G", 0, AWM_ALL, makeWindowCallback(group_species_cb, false));
    awmm->insert_menu_topic("create_groups_by_field", "Create new groups using Field", "F", 0, AWM_ALL, makeWindowCallback(group_species_cb, true));

    // --------------
    //      Edit

    awmm->create_menu("Edit", "E", AWM_ALL);
    awmm->insert_menu_topic("refresh",                  "Refresh [Ctrl-L]",           "f", 0,                    AWM_ALL, makeWindowCallback(ED4_request_full_refresh));
    awmm->insert_menu_topic("load_current",             "Load current species [GET]", "G", "e4_get_species.hlp", AWM_ALL, makeWindowCallback(ED4_get_and_jump_to_current));
    awmm->insert_menu_topic("load_marked",              "Load marked species",        "m", "e4_get_species.hlp", AWM_ALL, makeWindowCallback(ED4_get_marked_from_menu));
    awmm->insert_menu_topic(awmm->local_id("load_SAI"), "Load SAI ...",               "S", "e4_get_species.hlp", AWM_ALL, ED4_create_loadSAI_window);
    awmm->sep______________();
    awmm->insert_menu_topic("refresh_ecoli",       "Reload Ecoli sequence",        "E", "ecoliref.hlp", AWM_ALL, makeWindowCallback(reload_ecoli_cb));
    awmm->insert_menu_topic("refresh_helix",       "Reload Helix",                 "H", "helix.hlp",    AWM_ALL, makeWindowCallback(reload_helix_cb));
    awmm->insert_menu_topic("helix_jump_opposite", "Jump helix opposite [Ctrl-J]", "J", 0,              AWM_ALL, ED4_helix_jump_opposite);
    awmm->sep______________();

    awmm->insert_sub_menu("Set protection of current ", "p");
    {
        char macro[] = "to_0",
            topic[] = ".. to 0",
            hotkey[] = "0";

        for (char i='0'; i<='6'; i++) {
            macro[3] = topic[6] = hotkey[0] = i;
            awmm->insert_menu_topic(macro, topic, hotkey, "security.hlp", AWM_ALL, makeWindowCallback(ED4_set_protection, i-'0'));
        }
    }
    awmm->close_sub_menu();

#if !defined(NDEBUG) && 0
    awmm->insert_menu_topic(0,               "Test (test split & merge)", "T", 0, AWM_ALL, ED4_testSplitNMerge, 1, 0);
#endif
    
    awmm->sep______________();
    awmm->insert_menu_topic(awmm->local_id("fast_aligner"),   INTEGRATED_ALIGNERS_TITLE,            "I", "faligner.hlp",     AWM_ALL,            makeCreateWindowCallback(ED4_create_faligner_window, ED4_get_aligner_data_access()));
    awmm->insert_menu_topic("fast_align_set_ref",             "Set aligner reference [Ctrl-R]",     "R", "faligner.hlp",     AWM_ALL,            RootAsWindowCallback::simple(FastAligner_set_reference_species));
    awmm->insert_menu_topic(awmm->local_id("align_sequence"), "Old aligner from ARB_EDIT [broken]", "O", "ne_align_seq.hlp", AWM_DISABLED,       create_naligner_window);
    awmm->insert_menu_topic("sina",                           "SINA (SILVA Incremental Aligner)",   "S", "sina_main.hlp",    sina_mask(aw_root), makeWindowCallback(show_sina_window, ED4_get_aligner_data_access()));
    awmm->insert_menu_topic("del_ali_tmp",                    "Remove all aligner Entries",         "v", 0,                  AWM_ALL,            makeWindowCallback(ED4_remove_faligner_entries));
    awmm->sep______________();
    awmm->insert_menu_topic("missing_bases", "Dot potentially missing bases", "D", "missbase.hlp", AWM_EXP, ED4_popup_dot_missing_bases_window);

    if (alignment_type == GB_AT_RNA) { // if the database contains valid alignment of rRNA sequences
        awmm->sep______________();
        awmm->insert_menu_topic("sec_edit", "Edit secondary structure", "c", "arb_secedit.hlp", AWM_ALL, makeWindowCallback(ED4_start_plugin, GLOBAL_gb_main, "SECEDIT"));
#if defined(ARB_OPENGL)
        awmm->insert_menu_topic("rna3d", "View 3D molecule", "3", "rna3d_general.hlp", AWM_ALL, makeWindowCallback(ED4_start_plugin, GLOBAL_gb_main, "RNA3D"));
#endif // ARB_OPENGL
#if defined(DEBUG)
        awmm->insert_menu_topic("noplugin", "DEBUG: call unknown plugin", "u", 0, AWM_ALL, makeWindowCallback(ED4_start_plugin, GLOBAL_gb_main, "testplugin"));
#endif // DEBUG
    }

    // --------------
    //      View

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
            awmm->insert_menu_topic(awmm->local_id(macro_name), menu_entry_name, hotkey, "e4_search.hlp", AWM_ALL, makeWindowCallback(ED4_popup_search_window, type));
        }
    }
    awmm->close_sub_menu();
    awmm->sep______________();
    awmm->insert_sub_menu("Cursor position ", "p");
    awmm->insert_menu_topic("store_curpos",   "Store cursor position",    "S", 0, AWM_ALL, ED4_store_curpos);
    awmm->insert_menu_topic("restore_curpos", "Restore cursor position ", "R", 0, AWM_ALL, ED4_restore_curpos);
    awmm->insert_menu_topic("clear_curpos",   "Clear stored positions",   "C", 0, AWM_ALL, makeWindowCallback(ED4_clear_stored_curpos));
    awmm->close_sub_menu();

    awmm->sep______________();
    awmm->insert_menu_topic("change_cursor",             "Change cursor type",   "t", 0,              AWM_ALL, ED4_change_cursor);
    awmm->insert_menu_topic(awmm->local_id("view_diff"), "View differences ...", "V", "viewdiff.hlp", AWM_ALL, ED4_create_viewDifferences_window);
    awmm->sep______________();
    awmm->insert_menu_topic("enable_col_stat",  "Activate column statistics", "v", "st_ml.hlp", AWM_EXP, ED4_activate_col_stat);
    awmm->insert_menu_topic("disable_col_stat", "Disable column statistics",  "i", "st_ml.hlp", AWM_EXP, ED4_disable_col_stat);
    awmm->insert_menu_topic("detail_col_stat",  "Toggle detailed Col.-Stat.", "C", "st_ml.hlp", AWM_EXP, ED4_toggle_detailed_column_stats);
    awmm->insert_menu_topic("dcs_threshold",    "Set threshold for D.c.s.",   "f", "st_ml.hlp", AWM_EXP, makeWindowCallback(ED4_set_col_stat_threshold));
    awmm->sep______________();
    awmm->insert_menu_topic(awmm->local_id("visualize_SAI"), "Visualize SAIs",                "z", "visualizeSAI.hlp",   AWM_ALL, ED4_createVisualizeSAI_window);
    awmm->insert_menu_topic("toggle_saisec",                 "Toggle secondary info for SAI", "o", "toggle_secinfo.hlp", AWM_ALL, toggle_helix_for_SAI);

    // Enable ProteinViewer only for DNA sequence type
    if (alignment_type == GB_AT_DNA) {
        awmm->insert_menu_topic(awmm->local_id("Protein_Viewer"), "Protein Viewer", "w", "proteinViewer.hlp", AWM_ALL, ED4_CreateProteinViewer_window);
    }

    // ---------------
    //      Block

    awmm->create_menu("Block", "B", AWM_ALL);

    awmm->insert_menu_topic("select_marked",   "Select marked species",   "e", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_SELECT_MARKED));
    awmm->insert_menu_topic("deselect_marked", "Deselect marked species", "k", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_DESELECT_MARKED));
    awmm->insert_menu_topic("select_all",      "Select all species",      "S", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_ALL));
    awmm->insert_menu_topic("deselect_all",    "Deselect all",            "D", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_NONE));
    awmm->sep______________();
    awmm->insert_menu_topic("mark_selected",   "Mark selected species",   "M", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_MARK_SELECTED));
    awmm->insert_menu_topic("unmark_selected", "Unmark selected species", "n", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_UNMARK_SELECTED));
    awmm->insert_menu_topic("unmark_all",      "Unmark all species",      "U", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_UNMARK_ALL));
    awmm->sep______________();
    awmm->insert_menu_topic("invert_all",   "Invert selected species", "I", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_INVERT));
    awmm->insert_menu_topic("invert_group", "Invert group",            "g", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_INVERT_GROUP));
    awmm->sep______________();
    awmm->insert_menu_topic("lowcase", "Change to lower case ", "w", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_LOWER_CASE));
    awmm->insert_menu_topic("upcase",  "Change to upper case",  "p", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_UPPER_CASE));
    awmm->sep______________();
    awmm->insert_menu_topic("reverse",            "Reverse selection ",    "v", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_REVERSE));
    awmm->insert_menu_topic("complement",         "Complement selection ", "o", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_COMPLEMENT));
    awmm->insert_menu_topic("reverse_complement", "Reverse complement",    "t", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_REVERSE_COMPLEMENT));
    awmm->sep______________();
    awmm->insert_menu_topic("unalignBlockLeft",   "Unalign block left",   "a", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_UNALIGN_LEFT));
    awmm->insert_menu_topic("unalignBlockCenter", "Unalign block center", "c", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_UNALIGN_CENTER));
    awmm->insert_menu_topic("unalignBlockRight",  "Unalign block right",  "b", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_UNALIGN_RIGHT));
    awmm->sep______________();
    awmm->insert_menu_topic(awmm->local_id("replace"), "Search & Replace ", "h", "e4_replace.hlp", AWM_ALL, ED4_create_replace_window);
    awmm->insert_menu_topic(awmm->local_id("setsai"),  "Modify SAI ",       "y", "e4_modsai.hlp",  AWM_ALL, ED4_create_modsai_window);
    awmm->sep______________();
    awmm->insert_menu_topic("toggle_block_type", "Exchange: Line<->Column", "x", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_select, ED4_MS_TOGGLE_BLOCKTYPE));
    awmm->insert_menu_topic("shift_left",        "Shift block left ",       "l", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_SHIFT_LEFT));
    awmm->insert_menu_topic("shift_right",       "Shift block right",       "r", "e4_block.hlp", AWM_ALL, makeWindowCallback(ED4_menu_perform_block_operation, ED4_BO_SHIFT_RIGHT));

    // --------------------
    //      Properties

    awmm->create_menu("Properties", "P", AWM_ALL);

#ifdef ARB_MOTIF
    awmm->insert_menu_topic(awmm->local_id("props_frame"), "Frame settings ...", "F", 0, AWM_ALL, AW_preset_window);
#endif

    awmm->insert_menu_topic(awmm->local_id("props_options"),   "Editor Options ",       "O", "e4_options.hlp",   AWM_ALL, ED4_create_editor_options_window);
    awmm->insert_menu_topic(awmm->local_id("props_consensus"), "Consensus Definition ", "u", "e4_consensus.hlp", AWM_ALL, ED4_create_consensus_definition_window);
    awmm->sep______________();

    awmm->insert_menu_topic("props_data",                       "Change Colors & Fonts ", "C", 0,                     AWM_ALL, makeWindowCallback      (ED4_popup_gc_window,          first_gc_manager));
    awmm->insert_menu_topic(awmm->local_id("props_seq_colors"), "Sequence color mapping", "S", "sequence_colors.hlp", AWM_ALL, makeCreateWindowCallback(ED4_create_seq_colors_window, sequence_colors));

    awmm->sep______________();

    {
        static WindowCallback reqRelCb = makeWindowCallback(ED4_request_relayout);
        if (alignment_type == GB_AT_AA) awmm->insert_menu_topic(awmm->local_id("props_pfold"),     "Protein Match Settings ", "P", "pfold_props.hlp", AWM_ALL, makeCreateWindowCallback(ED4_pfold_create_props_window, &reqRelCb));
        else                            awmm->insert_menu_topic(awmm->local_id("props_helix_sym"), "Helix Settings ",         "H", "helixsym.hlp",    AWM_ALL, makeCreateWindowCallback(create_helix_props_window,     &reqRelCb));
    }

    awmm->insert_menu_topic(awmm->local_id("props_key_map"), "Key Mappings ",              "K", "nekey_map.hlp", AWM_ALL, create_key_map_window);
    awmm->insert_menu_topic(awmm->local_id("props_nds"),     "Select visible info (NDS) ", "D", "ed4_nds.hlp",   AWM_ALL, ED4_create_nds_window);
    awmm->sep______________();
    AW_insert_common_property_menu_entries(awmm);
    awmm->sep______________();
    awmm->insert_sub_menu("Save properties ...", "a");
    {
        static const char * const tag[] = { "save_alispecific_props", "save_alitype_props", "save_props" };
        static const char * const entry_type[] = { "alignment specific ", "ali-type specific ", "" };

        // check what is the default mode
        int default_mode = -1;
        for (int mode = 0; mode <= 2; ++mode) {
            if (strcmp(GB_path_in_arbprop(ED4_propertyName(mode)), db_name) == 0) {
                default_mode = mode;
                break;
            }
        }
        if (default_mode == -1) default_mode = 2; // no properties yet -> use 'edit4.arb'

        const char *entry = GBS_global_string("Save loaded Properties (%s)", ED4_propertyName(default_mode));
        awmm->insert_menu_topic("save_loaded_props", entry, "l", "e4_defaults.hlp", AWM_ALL, makeWindowCallback(ED4_save_properties, default_mode));
        awmm->sep______________();

        for (int mode = 2; mode >= 0; --mode) {
            char hotkey[] = "x";
            hotkey[0]     = "Pta"[mode];
            entry         = GBS_global_string("Save %sProperties (%s)", entry_type[mode], ED4_propertyName(mode));
            awmm->insert_menu_topic(tag[mode], entry, hotkey, "e4_defaults.hlp", AWM_ALL, makeWindowCallback(ED4_save_properties, mode));
        }
    }
    awmm->close_sub_menu();

    awmm->insert_help_topic("ARB_EDIT4 help",     "E", "e4.hlp", AWM_ALL, makeHelpCallback("e4.hlp"));

    // ----------------------------------------------------------------------------------------------------

    aw_root->awar_int(AWAR_EDIT_TITLE_MODE)->add_callback(makeRootCallback(title_mode_changed, static_cast<AW_window*>(awmm)));
    awmm->set_bottom_area_height(0);   // No bottom area

    awmm->auto_space(5, -2);
    awmm->shadow_width(3);

    int db_pathx, db_pathy;
    awmm->get_at_position(&db_pathx, &db_pathy);

    awmm->shadow_width(1);
    awmm->load_xfig("edit4/editmenu.fig", false);

    // --------------------------
    //      help /quit /fold

    awmm->button_length(0);

    awmm->at("quit");
    awmm->callback(ED4_quit_editor);
    awmm->help_text("quit.hlp");

    if (clone) {
        awmm->create_button("CLOSE", "#close.xpm");
#if defined(ARB_GTK)
        awmm->set_close_action("CLOSE");
#endif
    }
    else {
        awmm->create_button("QUIT", "#quit.xpm");
#if defined(ARB_GTK)
        awmm->set_close_action("QUIT");
#endif
    }

    awmm->at("help");
    awmm->callback(AW_help_entry_pressed);
    awmm->help_text("e4.hlp");
    awmm->create_button("HELP", "#help.xpm");

    awmm->at("fold");
    awmm->help_text("e4.hlp");
    awmm->create_toggle(AWAR_EDIT_TITLE_MODE, "#more.xpm", "#less.xpm");

    // -------------------
    //      positions

    awmm->button_length(0);

    awmm->at("posTxt");     awmm->create_button(0, "Position");

    awmm->button_length(6+1);

    awmm->at("ecoliTxt");   awmm->create_button(0, ED4_AWAR_NDS_ECOLI_NAME, 0, "+");

    awmm->button_length(0);

    awmm->at("baseTxt");    awmm->create_button(0, "Base");
    awmm->at("iupacTxt");   awmm->create_button(0, "IUPAC");
    awmm->at("helixnrTxt"); awmm->create_button(0, "Helix#");

    awmm->at("pos");
    awmm->callback(makeWindowCallback(ED4_jump_to_cursor_position, current_ed4w()->awar_path_for_cursor, ED4_POS_SEQUENCE));
    awmm->create_input_field(current_ed4w()->awar_path_for_cursor, 7);

    awmm->at("ecoli");
    awmm->callback(makeWindowCallback(ED4_jump_to_cursor_position, current_ed4w()->awar_path_for_Ecoli, ED4_POS_ECOLI));
    awmm->create_input_field(current_ed4w()->awar_path_for_Ecoli, 6);

    awmm->at("base");
    awmm->callback(makeWindowCallback(ED4_jump_to_cursor_position, current_ed4w()->awar_path_for_basePos, ED4_POS_BASE));
    awmm->create_input_field(current_ed4w()->awar_path_for_basePos, 6);

    awmm->at("iupac");
    awmm->callback(makeWindowCallback(ED4_set_iupac, current_ed4w()->awar_path_for_IUPAC));
    awmm->create_input_field(current_ed4w()->awar_path_for_IUPAC, 4);

    awmm->at("helixnr");
    awmm->callback(makeWindowCallback(ED4_set_helixnr, current_ed4w()->awar_path_for_helixNr));
    awmm->create_input_field(current_ed4w()->awar_path_for_helixNr, 5);

    // ----------------------------
    //      jump/get/undo/redo

    awmm->button_length(4);

    awmm->at("jump");
    awmm->callback(ED4_jump_to_current_species);
    awmm->help_text("e4_get_species.hlp");
    awmm->create_button("JUMP", "Jump");

    awmm->at("get");
    awmm->callback(makeWindowCallback(ED4_get_and_jump_to_current));
    awmm->help_text("e4_get_species.hlp");
    awmm->create_button("GET", "Get");

    awmm->button_length(0);

    awmm->at("undo");
    awmm->callback(makeWindowCallback(ED4_undo_redo, GB_UNDO_UNDO));
    awmm->help_text("undo.hlp");
    awmm->create_button("UNDO", "#undo.xpm");

    awmm->at("redo");
    awmm->callback(makeWindowCallback(ED4_undo_redo, GB_UNDO_REDO));
    awmm->help_text("undo.hlp");
    awmm->create_button("REDO", "#redo.xpm");

    // --------------------------
    //      aligner / SAIviz

    awmm->button_length(7);

    awmm->at("aligner");
    awmm->callback(makeCreateWindowCallback(ED4_create_faligner_window, ED4_get_aligner_data_access()));
    awmm->help_text("faligner.hlp");
    awmm->create_button("ALIGNER", "Aligner");

    awmm->at("saiviz");
    awmm->callback(ED4_createVisualizeSAI_window);
    awmm->help_text("visualizeSAI.hlp");
    awmm->create_button("SAIVIZ", "SAIviz");

    // -------------------------------------------
    //      align/insert/protection/direction

    awmm->button_length(0);

    awmm->at("protect");
    awmm->create_option_menu(AWAR_EDIT_SECURITY_LEVEL, true);
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
    awmm->create_button(NULL, "#protect.xpm");

    // draw align/edit-button AFTER protection!!
    awmm->at("edit");
    awmm->create_toggle(AWAR_EDIT_MODE, "#edit/align.xpm", "#edit/editseq.xpm", 7);

    awmm->at("insert");
    awmm->create_text_toggle(AWAR_INSERT_MODE, "Replace", "Insert", 7);

    awmm->at("direct");
    awmm->create_toggle(AWAR_EDIT_RIGHTWARD, "#edit/3to5.xpm", "#edit/5to3.xpm", 7);

    // -------------------------
    //      secedit / rna3d

    int xoffset = 0;

    if (alignment_type == GB_AT_RNA) { // if the database contains valid alignment of rRNA sequences
        // add buttons for RNA3D and SECEDIT plugins

        awmm->button_length(0);

        awmm->at("secedit");
        awmm->callback(makeWindowCallback(ED4_start_plugin, GLOBAL_gb_main, "SECEDIT"));
        awmm->help_text("arb_secedit.hlp");
        awmm->create_button("SECEDIT", "#edit/secedit.xpm");

#if defined(ARB_OPENGL)
        awmm->at("rna3d");
        awmm->callback(makeWindowCallback(ED4_start_plugin, GLOBAL_gb_main, "RNA3D"));
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
        awmm->callback(ED4_zoom_message_window);
        awmm->create_button("ZOOM", "#edit/zoom.xpm");

        awmm->at("clear");
        if (xoffset) { awmm->get_at_position(&x, &y); awmm->at(x+xoffset, y); }
        awmm->callback(aw_clear_message_cb);
        awmm->create_button("CLEAR", "#edit/clear.xpm");

        awmm->at("errortext");
        if (xoffset) { awmm->get_at_position(&x, &y); awmm->at(x+xoffset, y); }
        aw_root->awar_string(AWAR_ERROR_MESSAGES, "This is ARB Edit4 [Build " ARB_VERSION "]");
        awmm->create_text_field(AWAR_ERROR_MESSAGES);
        aw_set_local_message();
    }

    // ---------------------
    //      'more' area

    awmm->at("cons");
    awmm->create_toggle(ED4_AWAR_CONSENSUS_SHOW, "#edit/nocons.xpm", "#edit/cons.xpm");

    awmm->at("num");
    awmm->create_toggle(ED4_AWAR_DIGITS_AS_REPEAT, "#edit/norepeat.xpm", "#edit/repeat.xpm");

    awmm->at("key");
    awmm->create_toggle("key_mapping/enable", "#edit/nokeymap.xpm", "#edit/keymap.xpm");

    // search

    awmm->button_length(0);
#define INSERT_SEARCH_FIELDS(Short, label_prefix, prefix)       \
    insert_search_fields(awmm,                                  \
                         #label_prefix,                         \
                         #prefix,                               \
                         ED4_AWAR_##prefix##_SEARCH_PATTERN,    \
                         ED4_AWAR_##prefix##_SEARCH_SHOW,       \
                         Short,                                 \
                         ED4_##prefix##_PATTERN,                \
                         current_ed4w()                         \
        )

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
    awmm->callback(makeWindowCallback(ED4_search_cb, ED4_encodeSearchDescriptor(-1, ED4_ANY_PATTERN), current_ed4w()));
    awmm->create_button("ALL_SEARCH_LAST", "#edit/last.xpm");

    awmm->at("anext");
    awmm->callback(makeWindowCallback(ED4_search_cb, ED4_encodeSearchDescriptor(+1, ED4_ANY_PATTERN), current_ed4w()));
    awmm->create_button("ALL_SEARCH_NEXT", "#edit/next.xpm");

    title_mode_changed(aw_root, awmm);

    // Buttons at left window border

    awmm->create_mode("edit/arrow.xpm", "e4_mode.hlp", AWM_ALL, makeWindowCallback(modes_cb, ED4_SM_MOVE));
    awmm->create_mode("edit/kill.xpm",  "e4_mode.hlp", AWM_ALL, makeWindowCallback(modes_cb, ED4_SM_KILL));
    awmm->create_mode("edit/mark.xpm",  "e4_mode.hlp", AWM_ALL, makeWindowCallback(modes_cb, ED4_SM_MARK));

    FastAligner_create_variables(awmm->get_root(), props_db);

#if defined(DEBUG)
    AWT_check_action_ids(awmm->get_root(), "");
#endif

    announce_useraction_in(awmm);

    return (ED4_R_OK);
}

AW_window *ED4_root::create_new_window() {
    // only the first window, other windows are generated by generate_window
    AW_device  *device     = NULL;
    ED4_window *new_window = NULL;

    generate_window(&device, &new_window);

    ED4_LocalWinContext uses(new_window);
    
    ED4_calc_terminal_extentions();

    DRAW                    = 1;
    move_cursor             = 0;
    max_seq_terminal_length = 0;

    ED4_init_notFoundMessage();

    return new_window->aww;
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

static char *detectProperties() {
    char *propname = NULL;

    // check if edit4_?na.arb / edit4_ali_???.arb exist in $ARB_PROP
    for (int mode = 0; !propname && mode <= 1; ++mode) { 
        const char *fullprop = GB_path_in_arbprop(ED4_propertyName(mode));
        if (GB_is_regularfile(fullprop)) {
            freedup(propname, fullprop);
        }
    }

    // if not, use 'mode 2', i.e. "edit4.arb"
    // (no full path, we want to load default from arb_defaults)
    if (!propname) propname = strdup(ED4_propertyName(2));

    return propname;
}

ED4_root::ED4_root(int* argc, char*** argv)
    : most_recently_used_window(0),
      db_name(detectProperties()),
      aw_root(AWT_create_root(db_name, "ARB_EDIT4", need_macro_ability(), argc, argv)),
      props_db(AW_ROOT_DEFAULT),
      first_window(0),
      main_manager(0),
      middle_area_man(0),
      top_area_man(0),
      root_group_man(0),
      database(0),
      selected_objects(new ED4_selected_list), 
      folding_action(0),
      species_mode(ED4_SM_MOVE),
      ecoli_ref(0),
      alignment_name(0),
      alignment_type(GB_AT_UNKNOWN),
      reference(0),
      sequence_colors(0),
      gc_manager(0),
      st_ml(0),
      helix(0),
      helix_spacing(0),
      helix_add_spacing(0),
      terminal_add_spacing(0),
      protstruct(0),
      protstruct_len(0),
      edk(0),
      edit_string(0),
      column_stat_activated(false),
      column_stat_initialized(false),
      visualizeSAI(false),
      visualizeSAI_allSpecies(false),
      loadable_SAIs(LSAI_UNUSED),
      temp_gc(0)
{}


ED4_root::~ED4_root() {
    delete aw_root;
    delete first_window;
    delete reference; // needs to be deleted before main_manager (to ensure reference callbacks are removed)
    delete main_manager; // also deletes middle_area_man + top_area_man
    delete database;
    delete ecoli_ref;
    delete selected_objects;
    delete helix;
    delete sequence_colors;
    delete edk;
    STAT_destroy_ST_ML(st_ml);

    free(protstruct);
    free(db_name);
    free(alignment_name);
}
