// =============================================================== //
//                                                                 //
//   File      : SEC_graphic.cxx                                   //
//   Purpose   : GUI for structure window                          //
//   Time-stamp: <Mon Sep/17/2007 11:30 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include <cstdio>
#include <cstdlib>
#include <vector>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <ed4_extern.hxx>

#include "SEC_graphic.hxx"
#include "SEC_root.hxx"
#include "SEC_iter.hxx"
#include "SEC_toggle.hxx"

// SEC_graphic *SEC_GRAPHIC = 0;

using namespace std;

AW_gc_manager SEC_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL)
{
    AW_gc_manager preset_window =
        AW_manage_GC(aww,
                     device,
                     SEC_GC_LOOP,
                     SEC_GC_MAX,
                     AW_GCM_DATA_AREA,
                     (AW_CB)AWT_expose_cb, (AW_CL)ntw, 0,
                     false,
                     "#A1A1A1",
                     "LOOP$#247900",
                     "HELIX$#085DAB",
                     "NONPAIRING HELIX$#D52B69",
                     "DEFAULT$#000000",
                     "BONDS$#000000",
                     "ECOLI POSITION$#FFE223",
                     "HELIX NUMBERS$#D4D4D4",

                     // Color Ranges to paint SAIs
                     "+-RANGE 0$#FFFFFF",    "+-RANGE 1$#E0E0E0",    "-RANGE 2$#C0C0C0",
                     "+-RANGE 3$#A0A0A0",    "+-RANGE 4$#909090",    "-RANGE 5$#808080",
                     "+-RANGE 6$#808080",    "+-RANGE 7$#505050",    "-RANGE 8$#404040",
                     "+-RANGE 9$#303030",    "+-CURSOR$#BF1515",     "-MISMATCHES$#FF9AFF",

                     // colors used to Paint search patterns
                     // (do not change the names of these gcs)
                     "+-User1$#B8E2F8",          "+-User2$#B8E2F8",         "-Probe$#B8E2F8",
                     "+-Primer(l)$#A9FE54",      "+-Primer(r)$#A9FE54",     "-Primer(g)$#A9FE54",
                     "+-Sig(l)$#DBB0FF",         "+-Sig(r)$#DBB0FF",        "-Sig(g)$#DBB0FF",

                     // colors used to paint the skeleton of the structure
                     "+-SKELETON HELIX${HELIX}", "+-SKELETON LOOP${LOOP}", "-SKELETON NONHELIX${NONPAIRING HELIX}",
                     0 );

    return preset_window;
}

static GB_ERROR change_constraints(SEC_base *elem) {
    GB_ERROR  error    = 0;

    GB_CSTR constraint_type;
    GB_CSTR element_type;

    switch (elem->getType()) {
        case SEC_HELIX:
            constraint_type = "length";
            element_type    = "helix";
            break;
        case SEC_LOOP:
            constraint_type = "radius";
            element_type    = "loop";
            break;
        default:
            sec_assert(0);
            break;
    }

    char *question = GBS_global_string_copy("%s-constraints for %s", constraint_type, element_type);
    char *answer   = aw_input(question, 0, GBS_global_string("%.2f-%.2f", elem->minSize(), elem->maxSize()));

    while (answer) {
        char *end;
        double low = strtod(answer, &end);

        if (end[0]!='-') {
            error = "Wrong format! Wanted format is 'lower-upper'";
        }
        else {
            double high = strtod(end+1, 0);

            if (low<0 || high<0 || (low && high && low>high)) {
                error = "Illegal values";
            }
            else {
#if defined(DEBUG)
                sec_assert(!low || !high || low<=high);
#endif // DEBUG
                elem->setConstraints(low, high);
                break;
            }
        }

        sec_assert(error);

        aw_message(error, "Ok");
        char *retry = aw_input(question, 0, answer);
        free(answer);
        
        answer = retry;
    }

    free(answer);
    free(question);

    return error;
}


GB_ERROR SEC_graphic::handleKey(AW_event_type event, AW_key_mod key_modifier, AW_key_code key_code, char key_char) {
    GB_ERROR error = 0;

    if (event == AW_Keyboard_Press) {
        bool wrapped   = false; // avoid deadlock
        int  curpos    = sec_root->get_cursor();
        int  maxIndex  = sec_root->max_index();
        bool setCurpos = false;
        bool handled   = false;

        if (key_modifier == AW_KEYMODE_NONE) {
            switch (key_code) {
                case AW_KEY_LEFT: {
                    while (1) {
                        curpos--;
                        if (curpos<0) {
                            curpos  = maxIndex;
                            if (wrapped) break;
                            wrapped = true;
                        }
                        if (sec_root->shallDisplayPosition(curpos)) {
                            setCurpos = true;
                            break;
                        }
                    }
                    break;
                }
                case AW_KEY_RIGHT: {
                    while (1) {
                        curpos++;
                        if (curpos > maxIndex) {
                            curpos  = 0;
                            if (wrapped) break;
                            wrapped = true;
                        }
                        if (sec_root->shallDisplayPosition(curpos)) {
                            setCurpos = true;
                            break;
                        }
                    }
                    break;
                }
                case AW_KEY_ASCII: {
                    const char *toggle_awar = 0;
                    int         val_max     = 1;

                    switch (key_char) {
                        case 'b': toggle_awar = AWAR_SECEDIT_SHOW_BONDS; val_max = 2; break;
                        case 'B': toggle_awar = AWAR_SECEDIT_HIDE_BASES; break;
                        case 'k': toggle_awar = AWAR_SECEDIT_SHOW_STR_SKELETON; break;

                        case 'c': toggle_awar = AWAR_SECEDIT_SHOW_CURPOS; val_max = 3; break;
                        case 'h': toggle_awar = AWAR_SECEDIT_SHOW_HELIX_NRS; break;
                        case 'e': toggle_awar = AWAR_SECEDIT_SHOW_ECOLI_POS; break;

                        case 's': toggle_awar = AWAR_SECEDIT_DISPLAY_SAI; break;
                        case 'r': toggle_awar = AWAR_SECEDIT_DISPLAY_SEARCH; break;
                            
                        case 'E': toggle_awar = AWAR_SECEDIT_DISPPOS_ECOLI; break;
                        case 'H': toggle_awar = AWAR_SECEDIT_DISPPOS_BINDING; break;

#if defined(DEBUG)
                        case 'd': toggle_awar = AWAR_SECEDIT_SHOW_DEBUG; break;
#endif // DEBUG

                        case 't':
                            error   = sec_root->get_db()->structure()->next();
                            handled = true;
                            break;
                    }

                    if (toggle_awar) {
                        AW_awar *awar = aw_root->awar(toggle_awar);
                        int      val  = awar->read_int()+1;

                        if (val>val_max) val = 0;
                        awar->write_int(val);
                        
                        handled = true;
                    }

                    break;
                }
                default:
                    break;
            }
        }

        if (setCurpos) {
            aw_root->awar_int(AWAR_SET_CURSOR_POSITION)->write_int(curpos);
            handled = true;
        }
        
        if (!handled) { // pass unhandled key events to EDIT4
            AW_event faked_event;

            memset((char*)&faked_event, 0, sizeof(faked_event));

            faked_event.type        = event;
            faked_event.keymodifier = key_modifier;
            faked_event.keycode     = key_code;
            faked_event.character   = key_char;

            ED4_remote_event(&faked_event);
        }
    }

    return error;
}

GB_ERROR SEC_graphic::handleMouse(AW_device *device, AW_event_type event, int button, AWT_COMMAND_MODE cmd, const Position& world, SEC_base *elem, int abspos) {
    GB_ERROR error = 0;

    // -----------------------------------------
    //      handle element dependent actions
    // -----------------------------------------

    if (elem) {
        static Position start;      // click position on mouse down

        Position fixpoint = elem->get_fixpoint(); // of strand or loop

        SEC_loop  *loop  = 0;
        SEC_helix *helix = 0;

        if (elem->getType() == SEC_HELIX) helix = static_cast<SEC_helix*>(elem);
        else {
            sec_assert(elem->getType() == SEC_LOOP);
            loop = static_cast<SEC_loop*>(elem);
        }

        if (event == AW_Mouse_Press) start = world; // store start position

        switch (cmd) {
            case AWT_MODE_STRETCH: { // change constraints with mouse
                static double start_size; // helix/loop size at start click

                switch (event) {
                    case AW_Mouse_Press:
                        if (button == AWT_M_LEFT) {
                            start_size = elem->drawnSize();
                            sec_root->set_show_constraints(elem->getType());
                            exports.refresh = 1;
                        }
                        else { // right button -> reset constraints
                            elem->setConstraints(0, 0);
                            elem->sizeChanged();
                            exports.refresh = 1;
                            exports.save    = 1;
                        }
                        break;
                            
                    case AW_Mouse_Drag:
                        if (button == AWT_M_LEFT) {
                            double dfix1 = Distance(fixpoint, start);
                            double dfix2 = Distance(fixpoint, world);

                            if (dfix1>0 && dfix2>0) {
                                double factor   = dfix2/dfix1;
                                double new_size = start_size*factor;

                                elem->setDrawnSize(new_size);
                                elem->sizeChanged();

                                exports.refresh            = 1;
                            }
                        }
                        break;

                    case AW_Mouse_Release:
                        sec_root->set_show_constraints(SEC_ANY_TYPE);
                        exports.save = 1;
                        break;

                    default: sec_assert(0); break;
                }
                break;
            }
            case AWT_MODE_MOD:  // edit constraints
                if (button==AWT_M_LEFT && event==AW_Mouse_Press) {
                    error = change_constraints(elem);
                    if (!error) {
                        elem->sizeChanged();
                        exports.save = 1;
                    }
                }
                break;

            case AWT_MODE_ROT: { // rotate branches/loops
                if (event == AW_Mouse_Release) {
                    exports.save = 1;
                }
                else {
                    static Angle start; // angle between fixpoint (of loop or helix) and first-click
                    static bool  rotateSubStructure; // whether to rotate the substructure below
                    static vector<Angle> old; // old angles

                    if (loop && loop->is_root_loop()) fixpoint = loop->get_center();

                    Angle fix2world(fixpoint, world);

                    if (event == AW_Mouse_Press) {
                        start = fix2world;
                        old.clear();
                        rotateSubStructure = (button == AWT_M_LEFT);
                            
                        if (loop) {
                            old.push_back(loop->get_abs_angle());
                            if (!rotateSubStructure) {
                                for (SEC_strand_iterator strand(loop); strand; ++strand) {
                                    if (strand->isRootsideFixpoint()) {
                                        old.push_back(strand->get_helix()->get_abs_angle());
                                    }
                                }
                            }
                        }
                        else {
                            old.push_back(helix->get_abs_angle());
                            old.push_back(helix->outsideLoop()->get_abs_angle());
                        }
                    }
                    else {
                        sec_assert(event == AW_Mouse_Drag);
                        Angle diff = fix2world-start;

                        if (loop) {
                            loop->set_abs_angle(old[0]+diff);
                            if (!rotateSubStructure) {
                                int idx = 1;
                                for (SEC_strand_iterator strand(loop); strand; ++strand) {
                                    if (strand->isRootsideFixpoint()) {
                                        strand->get_helix()->set_abs_angle(old[idx++]);
                                    }
                                }
                            }
                        }
                        else {
                            helix->set_abs_angle(old[0]+diff);
                            if (!rotateSubStructure) helix->outsideLoop()->set_abs_angle(old[1]);
                        }

                        exports.refresh = 1;
                        elem->orientationChanged();
                    }
                }
                break ;
            }

            case AWT_MODE_SETROOT:  // set-root-mode / reset angles
                if (event == AW_Mouse_Press) {
                    if (button == AWT_M_LEFT) { // set root
                        if (loop) {
                            sec_root->set_root(loop);
                            exports.save = 1;
                        }
                        else error = "Please click on a loop to change the root";
                    }
                    else { // reset angles
                        sec_assert(button == AWT_M_RIGHT);
                        elem->reset_angles();
                        elem->orientationChanged();
                        exports.save = 1;
                    }
                }
                break ;

            case AWT_MODE_MOVE: { // fold/unfold helix
                if (event == AW_Mouse_Press) {
                    if (button == AWT_M_LEFT) { // fold helix
                        if (loop) {
                            const char *helix_nr = sec_root->helixNrAt(abspos);
                            if (helix_nr) {
                                const size_t *p = sec_root->getHelixPositions(helix_nr);
                                error           = sec_root->split_loop(p[0], p[1]+1, p[2], p[3]+1);

                                if (!error) {
                                    sec_root->nail_position(abspos);
                                    exports.save = 1;
                                }
                            }
                            else {
                                error = GBS_global_string("No helix to fold at position %i", abspos);
                            }
                        }
                        else {
                            error = "Click on a loop region to fold a helix";
                        }
                    }
                    else { // unfold helix
                        sec_assert(button == AWT_M_RIGHT);
                        if (helix) {
                            error = sec_root->unsplit_loop(helix->strandToRoot());
                            if (!error) {
                                sec_root->nail_position(abspos);
                                exports.save = 1;
                            }
                        }
                        else {
                            error = "Right click on a helix to remove it";
                        }
                    }
                }
                break ;
            }
            case AWT_MODE_LINE:
            case AWT_MODE_PROINFO:
                elem = 0; // handle element-independent
                break;
            default: sec_assert(0); break;
        }
    }

    // --------------------------------------
    //      action independent of element
    // --------------------------------------
    
    if (!elem) {
        switch (cmd) {
            case AWT_MODE_LINE: // set cursor in ARB_EDIT4
                if (event == AW_Mouse_Press) {
                    if (abspos >= 0 && size_t(abspos) < sec_root->max_index()) {
                        // sequence position in AWAR_SET_CURSOR_POSITION is starting with 0!
                        aw_root->awar_int(AWAR_SET_CURSOR_POSITION)->write_int(abspos);
                    }
                }
                break;

            case AWT_MODE_PROINFO: // display search pattern
                if (event == AW_Mouse_Press) {
                    if (button == AWT_M_LEFT) {
                        if (abspos >= 0 && size_t(abspos) < sec_root->max_index()) {
                            sec_root->paintSearchPatternStrings(device, abspos, world.xpos()+1, world.ypos());
                        }
                        // dont refresh here!
                    }
                    else {
                        sec_assert(button == AWT_M_RIGHT);
                        exports.refresh = 1; // simply refresh to remove drawn patterns
                    }
                }
                break;
                
            default:
                break;
        }
        return 0; // no error
    }

    if (exports.save == 1) exports.refresh = 1;
    return error;
}

void SEC_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd,
                          int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char,
                          AW_event_type event, AW_pos screen_x, AW_pos screen_y,
                          AW_clicked_line *cl, AW_clicked_text *ct)
{
    if (cmd != AWT_MODE_MOD && cmd != AWT_MODE_STRETCH) sec_root->set_show_constraints(SEC_NO_TYPE);

    GB_ERROR error = 0;
    if (event== AW_Keyboard_Press || event == AW_Keyboard_Release) {
        error = handleKey(event, key_modifier, key_code, key_char);
    }
    else {
        if (button != AWT_M_MIDDLE && cmd != AWT_MODE_ZOOM) { // dont handle scroll + zoom
            AW_CL    cd1, cd2;
            Position world = device->rtransform(Position(screen_x, screen_y)); // current click position

            if (AW_getBestClick(world, cl, ct, &cd1, &cd2)) {
                SEC_base *elem   = reinterpret_cast<SEC_base*>(cd1);
                int       abspos = cd2;

#if defined(DEBUG) && 0
                if (cl->exists) device->line(SEC_GC_CURSOR, cl->x0, cl->y0, cl->x1, cl->y1, -1, 0, -1);
                if (ct->exists) device->box(SEC_GC_CURSOR, false, ct->textArea, -1, 0, -1);
#endif // DEBUG

                error = handleMouse(device, event, button, cmd, world, elem, abspos);
            }
        }
    }

    if (error) aw_message(error);
}


SEC_graphic::SEC_graphic(AW_root *aw_rooti, GBDATA *gb_maini)
    : update_requested(SEC_UPDATE_RELOAD)
    , load_error(0)
    , disp_device(0)
    , gb_main(gb_maini)
    , aw_root(aw_rooti)
    , sec_root(new SEC_root)
    , gb_struct(0)
    , gb_struct_ref(0)
    , last_saved(0)
{
    // update_requested = SEC_UPDATE_RELOAD; // // need to load structure!

    // gb_struct     = 0;
    // gb_struct_ref = 0;
    // last_saved    = 0;

    exports.dont_fit_x    = 0;
    exports.dont_fit_y    = 0;
    exports.left_offset   = 20;
    exports.right_offset  = 20;
    exports.top_offset    = 20;
    exports.bottom_offset = 20;
    exports.dont_scroll   = 0;

    // aw_root = aw_rooti;
    // gb_main = gb_maini;
    
    rot_ct.exists = AW_FALSE;
    rot_cl.exists = AW_FALSE;
    
    // sec_root      = new SEC_root;
}

SEC_graphic::~SEC_graphic(void) {
    delete sec_root;
    delete load_error;
}

void SEC_structure_changed_cb(GBDATA *gb_seq, SEC_graphic *gfx, GB_CB_TYPE type) { 
    if (type == GB_CB_DELETE) {
        gfx->gb_struct     = NULL;
        gfx->gb_struct_ref = NULL;

        gfx->request_update(SEC_UPDATE_RELOAD);
    }
    else if (GB_read_clock(gb_seq) > gfx->last_saved) { // not changed by secedit self
        gfx->request_update(SEC_UPDATE_RELOAD);
    }
}

/** read awar AWAR_HELIX_NAME to get the name */
GB_ERROR SEC_graphic::load(GBDATA *, const char *, AW_CL, AW_CL) {
    sec_assert(sec_root->get_db()->canDisplay()); // need a sequence loaded (to fix bugs in versions < 3)
    sec_root->nail_cursor();
    
    GB_transaction ta(gb_main);
    // first check timestamp, do not load structure that we have saved !!!!
    if (gb_struct) {
        if (GB_read_clock(gb_struct) <= last_saved) return NULL;
    }

    /************************** Reset structure ***********************************/
    if (gb_struct) {
        GB_remove_callback( gb_struct,  GB_CB_ALL, (GB_CB)SEC_structure_changed_cb, (int *)this);
        gb_struct = NULL;
        GB_remove_callback( gb_struct_ref,  GB_CB_ALL, (GB_CB)SEC_structure_changed_cb, (int *)this);
        gb_struct_ref = NULL;
    }

    request_update(SEC_UPDATE_RECOUNT);

    if (gb_struct) {
        this->last_saved = GB_read_clock(gb_struct); // mark as loaded
    }


    GB_ERROR err = 0;

    /************************** Setup new structure *******************************/

    long    ali_len = -1;
    GBDATA *gb_ali  = 0;
    {
        char *name = GBT_read_string2(gb_main, AWAR_HELIX_NAME, GBT_get_default_helix(gb_main));

        GBDATA *gb_species = GBT_find_SAI(gb_main, name);
        if (!gb_species) {
            err = GB_export_error("Cannot find helix templace SAI '%s'",name);
        }
        else {
            char *ali_name = GBT_get_default_alignment(gb_main);

            ali_len = GBT_get_alignment_len(gb_main,ali_name);
            if (ali_len < 10) {
                err = GB_export_error("alignment '%s' to short to generate helix",ali_name);
            }
            else {
                gb_ali = GB_search(gb_species, ali_name, GB_FIND);
                if (!gb_ali) {
                    err = GB_export_error("Your helix structure template '%s' has no valid sequence for alignment '%s'", name,ali_name); // no sequence for name in the database !!!
                }
            }
            free(ali_name);
        }
        
        free(name);
    }

    // -----------------------
    //      read structure
    // -----------------------

    if (!err) {
        gb_struct = GB_search(gb_ali,NAME_OF_STRUCT_SEQ, GB_FIND);

        if (gb_struct) {
            gb_struct_ref = GB_search(gb_ali , NAME_OF_REF_SEQ , GB_STRING);

            char *strct = GB_read_string(gb_struct);
            char *ref = GB_read_string(gb_struct_ref);
            err = sec_root->read_data(strct,ref);
            if (err) {
                err = GBS_global_string("Defect structure in DB\n(read-error: '%s')", err);
            }
#if defined(CHECK_INTEGRITY)
            else {
                sec_root->check_integrity(CHECK_STRUCTURE);
            }
#endif // CHECK_INTEGRITY

            free(strct);
            free(ref);

            // on first load init structure toggler:
            if (!err) {
                err = sec_root->get_db()->init_toggler();
            }
        }
        else {
            err = "no secondary structure was found in your database";
        }

        if (err) {
            request_update(SEC_UPDATE_ZOOM_RESET);
        }
        else {
            // in the past one additional NAME_OF_STRUCT_SEQ-entry was added everytime the default bone was created
            // Fix: delete additional entries

            GBDATA *gb_add = gb_struct;
            do {
                gb_add = GB_find(gb_add, NAME_OF_STRUCT_SEQ, 0, this_level|search_next);
                if (gb_add) {
                    err = GB_delete(gb_add);
                    printf("* Deleting duplicated entry '%s' (%p)\n", NAME_OF_STRUCT_SEQ, gb_add);
                }
            }
            while (gb_add && !err);
        }
    }

    if (!err) {
        last_saved = GB_read_clock(gb_struct); // mark as loaded
        request_update(SEC_UPDATE_RECOUNT);
        if (load_error) { // previous load error?
            free(load_error);
            load_error = 0;
            request_update(SEC_UPDATE_ZOOM_RESET);
        }
    }
    else {
        load_error = strdup(err);
        request_update(SEC_UPDATE_ZOOM_RESET);
    }

    /************************* Listen to the database ***************************/
    GB_add_callback(gb_struct,GB_CB_ALL,(GB_CB)SEC_structure_changed_cb, (int *)this);
    GB_add_callback(gb_struct_ref,GB_CB_ALL,(GB_CB)SEC_structure_changed_cb, (int *)this);

    return err;
}

/** Save secondary structure to database */
GB_ERROR SEC_graphic::save(GBDATA *, const char *,AW_CL,AW_CL)
{
    if (!gb_struct) return 0;   // not loaded, so don't save
    if (!sec_root) return 0;
    
    char           *data  = sec_root->buildStructureString();
    GB_transaction  ta(gb_main);
    GB_ERROR        error = GB_write_string(gb_struct,data);
    if (!error) {
        const XString&  xstr     = sec_root->get_xString();
        const char     *x_string = xstr.get_x_string();

        error = GB_write_string(gb_struct_ref,x_string);

        if (!error && xstr.alignment_too_short()) {
            aw_message("Your helix needs one gap at end. Please format your alignment!");
        }
    }
    this->last_saved = GB_read_clock(gb_struct);
    if (error) {
        aw_message(error);
        ta.abort();
    }
    return NULL;
}

GB_ERROR SEC_graphic::read_data_from_db(char **data, char **x_string) const {
    GB_ERROR error = 0;
    
    sec_assert(gb_struct && gb_struct_ref);
    *data = GB_read_string(gb_struct);
    if (!*data) error = GB_get_error();
    else {
        *x_string = GB_read_string(gb_struct_ref);
        if (!*x_string) error = GB_get_error();
    }
    return error;
}

GB_ERROR SEC_graphic::write_data_to_db(const char *data, const char *x_string) const {
    if (!gb_struct) return 0;
    if (!sec_root) return 0;
    
    GB_transaction ta(gb_main);
    GB_ERROR       error = GB_write_string(gb_struct, data);
    if (!error) {
        error = GB_write_string(gb_struct_ref, x_string);
    }
    last_saved = 0;             // force reload of data
    if (error) ta.abort();
    return error;
}

int SEC_graphic::check_update(GBDATA *) {
    GB_transaction ta(gb_main);
    
    const SEC_db_interface *db = sec_root->get_db();

    if (db && db->canDisplay()) {
        if (update_requested & SEC_UPDATE_RELOAD) {
            GB_ERROR error   = load(0, 0, 0, 0); // sets change_flag
            if (error) {
                aw_message(error);
                ta.abort();
            }

            update_requested = static_cast<SEC_update_request>((update_requested^SEC_UPDATE_RELOAD)|SEC_UPDATE_RECOUNT); // clear reload flag
            exports.refresh  = 1;
        }

        if (update_requested & SEC_UPDATE_SHOWN_POSITIONS) {
            sec_root->update_shown_positions();
            update_requested = static_cast<SEC_update_request>((update_requested^SEC_UPDATE_SHOWN_POSITIONS)|SEC_UPDATE_RECOUNT); // clear reload flag
            exports.refresh  = 1;
        }
    
        if (update_requested & SEC_UPDATE_RECOUNT) {
            sec_root->invalidate_base_positions();
            sec_root->relayout();

            update_requested = static_cast<SEC_update_request>(update_requested^SEC_UPDATE_RECOUNT); // clear recount flag
            exports.refresh  = 1;
        }
    
        sec_root->perform_autoscroll();
    }

    int res = 0;
    if (update_requested & SEC_UPDATE_ZOOM_RESET) {
        res              = 1; // signal zoom reset
        update_requested = static_cast<SEC_update_request>(update_requested^SEC_UPDATE_ZOOM_RESET); // clear zoom reset flag
    }
    return res;
}

void SEC_graphic::update(GBDATA *) {
}

void SEC_graphic::show(AW_device *device) {
    const char *text = 0;

    sec_assert(sec_root);
    sec_root->clear_last_drawed_cursor_position();

    if (sec_root->canDisplay()) {
        if (sec_root->get_root_loop())  {
            GB_ERROR paint_error = sec_root->paint(device);
            if (paint_error) text = GBS_global_string("Error: %s", paint_error);
        }
        else {
            if (load_error)     text = GBS_global_string("Load error: %s", load_error);
            else                text = "No structure loaded (yet)";
        }
    }
    else {
        const SEC_db_interface *db = sec_root->get_db();

        if (!db)                text = "Not connected to database";
        else if (!db->helix())  text = "No helix info";
        else                    text = "No species selected";
    }

    if (text) { // no structure
        device->text(SEC_GC_ECOLI, text, 0, 0, 0, 1, 0, 0, 0);
        sec_root->set_last_drawed_cursor_position(LineVector(Origin, ZeroVector));
    }
}

void SEC_graphic::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct)
{
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);
}


// --------------------------------------------------------------------------------

// #error remove below

// #define STRUCTURE_VERSIONS 3 // // how many different structure versions shall be stored in DB ?

// static GBDATA *getStructuresContainer(GBDATA *gb_main, const char *aliname) {
//     return GB_search(gb_main, GBS_global_string("secedit/structs/%s", aliname), GB_CREATE_CONTAINER);
// }

// static int getCurrentStructureNumber(GBDATA *gb_structures) {
    
// }

// static GB_ERROR loadStructure(const SEC_graphic *gfx, GBDATA *gb_structures, int struct_nr) {
//     GB_ERROR  error   = 0;
//     GBDATA   *gb_idx = GB_find_int(gb_structures, "idx", struct_nr, down_2_level);
//     if (gb_idx) {
//         GBDATA *gb_structCont = GB_get_father(gb_idx);
//         GBDATA *gb_dat        = GB_search(gb_structCont, "data", GB_FIND);

//         if (!gb_dat) error = GB_get_error();
//         else {
//             GBDATA *gb_ref     = GB_search(gb_structCont, "ref", GB_FIND);
//             if (!gb_ref) error = GB_get_error();
//             else {
//                 char *struct_dat = GB_read_string(gb_dat);
//                 if (!struct_dat) error = GB_get_error();
//                 else {
//                     char *struct_ref = GB_read_string(gb_ref);
//                     if (!struct_ref) error = GB_get_error();
//                     else error = gfx->write_data_to_db(struct_dat, struct_ref);
//                     free(struct_ref);
//                 }
//                 free(struct_dat);
//             }
//         }
//     }
//     else {
//         aw_message(GBS_global_string("Created structure #%i as copy of current structure", struct_nr));
//     }
//     return error;
// }

// static GB_ERROR saveStructure(const SEC_graphic *gfx, GBDATA *gb_structures, int struct_nr) {
//     GB_ERROR  error   = 0;
//     GBDATA   *gb_idx = GB_find_int(gb_structures, "idx", struct_nr, down_2_level);
//     GBDATA   *gb_structCont = 0;

//     if (gb_idx) gb_structCont = GB_get_father(gb_idx); 
//     else { // // create default
//         gb_structCont = GB_create_container(gb_structures, "struct");
//         gb_idx        = GB_create(gb_structCont, "idx", GB_INT);
//         error         = GB_write_int(gb_idx, struct_nr);
//     }

//     if (!error) {
//         char *struct_dat = 0;
//         char *struct_ref = 0;
//         error = gfx->read_data_from_db(&struct_dat, &struct_ref);

//         if (!error) {
//             GBDATA *gb_dat = GB_search(gb_structCont, "data", GB_STRING);
//             if (!gb_dat) error = GB_get_error();
//             else {
//                 error = GB_write_string(gb_dat, struct_dat);
//                 if (!error) {
//                     GBDATA *gb_ref = GB_search(gb_structCont, "ref", GB_STRING);
//                     if (!gb_ref) error = GB_get_error();
//                     else error = GB_write_string(gb_ref, struct_ref);
//                 }
//             }
//         }

//         free(struct_ref);
//         free(struct_dat);
//     }

//     return error;
// }

// GB_ERROR SEC_graphic::toggle_structure(const char *aliname) {
    // // toggles between multiple secondary structures

//     GB_ERROR       error = 0;
//     GB_transaction ta(gb_main);

//     GBDATA *gb_structures = getStructuresContainer(gb_main, aliname);
//     if (!gb_structures) {
//         error = GB_get_error();
//     }
//     else {
//         GBDATA *gb_current = GB_search(gb_structures, "current", GB_INT);
//         int     current    = GB_read_int(gb_current);

//         error = saveStructure(this, gb_structures, current);
//         if (!error) {
//             int last = current;
//             if (++current >= STRUCTURE_VERSIONS) {
//                 current = 0;
//             }
//             error = loadStructure(this, gb_structures, current);
//             if (error) {
//                 GB_ERROR err = loadStructure(this, gb_structures, last);
//                 if (err) {
//                     aw_message(GBS_global_string("Failed to reload previous structure (Reason: %s)", err));
//                 }
//             }
//             else {
//                 GB_write_int(gb_current, current);
//             }

//             exports.refresh = 1;
//         }
//     }
//     return error;
// }

// char *SEC_graphic::get_structure_name(const char *aliname) const {
//    //  GB_transaction ta(gb_main);
//    //  int            current = 0; 

//    //  GBDATA *gb_structures = getStructuresContainer(gb_main, aliname);
//    //  if (gb_structures) {
//        //  GBDATA *gb_current = GB_search(gb_structures, "current", GB_INT);
//        //  if (gb_current) {
//            //  current = GB_read_int(gb_current);
//        //  }
//    //  }
//    //  return current;
// }

// GB_ERROR SEC_db_interface::toggle_structure() const {
//     return gfx->toggle_structure(aliname);
// }

// char *SEC_db_interface::get_structure_name() const {
//     return gfx->get_structure_name(aliname);
// }
