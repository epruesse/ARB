#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include <fstream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awt_canvas.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>
#include <BI_helix.hxx>

#include "sec_graphic.hxx"
#include "secedit.hxx"

void SEC_create_awars(AW_root *aw_root,AW_default def)
{
    aw_root->awar_int(AWAR_SECEDIT_BASELINEWIDTH,0,def)->set_minmax(0,10);
    aw_root->awar_string(AWAR_FOOTER);

    {
        char *dir = GBS_global_string_copy("%s/lib/secondary_structure", GB_getenvARBHOME());
        aw_create_selection_box_awars(aw_root, AWAR_SECEDIT_IMEXPORT_BASE, dir, ".ass", "noname.ass");
        free(dir);
    }

    aw_root->awar_float(AWAR_SECEDIT_DIST_BETW_STRANDS, 1, def)->set_minmax(0.001, 1000);
    aw_root->awar_float(AWAR_SECEDIT_SKELETON_THICKNESS, 1, def)->set_minmax(1, 100);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_DEBUG, 0, def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_HELIX_NRS, 0, def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_STR_SKELETON, 0, def);
    aw_root->awar_int(AWAR_SECEDIT_HIDE_BASES, 0, def);
    aw_root->awar_int(AWAR_SECEDIT_HIDE_BONDS, 0, def);
    aw_root->awar_int(AWAR_SECEDIT_DISPLAY_SAI, 0, def);

#define DEFINE_PAIR(type, pairs, character)                             \
    aw_root->awar_string(AWAR_SECEDIT_##type##_PAIRS, pairs);           \
    aw_root->awar_string(AWAR_SECEDIT_##type##_PAIR_CHAR, character)

    DEFINE_PAIR(STRONG, "GC AU AT", "-");
    DEFINE_PAIR(NORMAL, "GU GT", ".");
    DEFINE_PAIR(WEAK, "GA", "o");
    DEFINE_PAIR(NO, "AA AC CC CU CT GG UU TT TU", "");
    DEFINE_PAIR(USER, "", "");
#undef DEFINE_PAIR

}

// required dummy for AWT, even if you don't use trees...
#if defined(FREESTANDING)
void AD_map_viewer(GBDATA *dummy, AD_MAP_VIEWER_TYPE)
{
    AWUSE(dummy);
}
#endif


AW_gc_manager SEC_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2)
{
    AW_gc_manager preset_window =
        AW_manage_GC(aww,
                     device,
                     SEC_GC_LOOP,
                     SEC_GC_MAX,
                     AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb,
                     (AW_CL)ntw,
                     cd2,
                     false,
                     "#777777",
                     "LOOP$#ffffaa",
                     "HELIX$#5cb1ff",
                     "NONPAIRING HELIX$#ff95ea",
                     "DEFAULT$#000000",
                     "BONDS$#000000",
                     "ECOLI POSITION$#ffffff",
                     "HELIX NUMBERS$#ffffff",

                     // Color Ranges to paint SAIs
                     "+-RANGE 0$#FFFFFF",    "+-RANGE 1$#E0E0E0",    "-RANGE 2$#C0C0C0",
                     "+-RANGE 3$#A0A0A0",    "+-RANGE 4$#909090",    "-RANGE 5$#808080",
                     "+-RANGE 6$#808080",    "+-RANGE 7$#505050",    "-RANGE 8$#404040",
                     "+-RANGE 9$#303030",    "+-CURSOR$#ff0000",     "-MISMATCHES$#FF9AFF",

                     // colors used to Paint search patterns
                     // (do not change the names of these gcs)
                     "+-User1$#B8E2F8",          "+-User2$#B8E2F8",         "-Probe$#B8E2F8",
                     "+-Primer(l)$#A9FE54",      "+-Primer(r)$#A9FE54",     "-Primer(g)$#A9FE54",
                     "+-Sig(l)$#DBB0FF",         "+-Sig(r)$#DBB0FF",        "-Sig(g)$#DBB0FF",

                     //colors used to paint the skeleton of the structure
                     "+-SKELETON HELIX$#B8E2F8", "+-SKELETON LOOP$#DBB0FF", "-SKELETON NONHELIX$#A9FE54",
                     0 );

    return preset_window;
}

static GB_CSTR double2string(double d) {
    static char buffer[40];

    sprintf(buffer, "%f", d);
    char *point = strchr(buffer, '.');

    if (point) {
        char *end = strchr(point+1, 0);
        while (end>buffer) {
            end--;
            if (*end=='0') {
                *end = 0;
            }
            else if (*end=='.') {
                *end = 0;
                break;
            }
            else  {
                break;
            }
        }
    }

    return buffer;
}

static GB_ERROR change_constraints(GB_CSTR constraint_type, GB_CSTR element_type, double& lower_constraint, double& upper_constraint) {
    char *question = GBS_global_string_copy("%s-constraints for %s", constraint_type, element_type);
    char *default_input;
    {
        char *f1 = strdup(double2string(lower_constraint));
        GB_CSTR f2 = double2string(upper_constraint);

        default_input = GBS_global_string_copy("%s-%s", f1, f2);
        free(f1);
    }
    char *answer = aw_input(question, 0, default_input);
    GB_CSTR error = 0;

    if (answer) {
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

                // set new constraints:
                lower_constraint = low;
                upper_constraint = high;
            }
        }

        if (error) aw_message(error, "Ok");
        free(answer);
    }
    else {
        error = "input aborted";
    }

    free(default_input);
    free(question);

    return error;
}

void SEC_graphic::command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char, AW_event_type type, AW_pos screen_x, AW_pos screen_y, AW_clicked_line *cl, AW_clicked_text *ct) {
    AWUSE(cl);
    AW_pos world_x;
    AW_pos world_y;
    device->rtransform(screen_x, screen_y, world_x, world_y); // x and y positions on the screen

    if (cmd!=AWT_MODE_MOD) {
        sec_root->show_constraints = 0;
    }

    switch (cmd) {
        /* ******************************************************** */
        case AWT_MODE_ZOOM: {
            break;
        }
            /* ******************************************************** */
        case AWT_MODE_STRETCH: {
            if(button==AWT_M_MIDDLE) {
                break;
            }

            SEC_Base *base;
            SEC_helix_strand *strand;
            SEC_helix *helix_info;
            SEC_segment *segment;
            SEC_loop *loop;

            double fixpoint_x, fixpoint_y;
            double loopCentre_x, loopCentre_y;
            double finalLengthConstraint;
            // 	double initialLengthConstraint;
            // 	double initialRadiusConstraint, finalRadiusConstraint;
            double startDist, endDist;

            if(button==AWT_M_LEFT) {
                switch(type){
                    case AW_Mouse_Press: {
                        base = (SEC_Base*)ct->client_data1;
                        if (base) {
                            if(base->getType()==SEC_HELIX_STRAND) {
                                strand     = (SEC_helix_strand*)base;
                                helix_info = strand->get_helix_info();
                                fixpoint_x = strand->get_fixpoint_x();
                                fixpoint_y = strand->get_fixpoint_y();
                                startDist  = sqrt(((fixpoint_x-world_x)*(fixpoint_x-world_x))+((fixpoint_y- world_y)*(fixpoint_y- world_y)));
                                //	initialLengthConstraint = helix_info->get_length();
                            }
                            if(base->getType()==SEC_SEGMENT) {
                                segment      = (SEC_segment*)base;
                                loop         = segment->get_loop();
                                loopCentre_x = loop->get_x_loop();
                                loopCentre_y = loop->get_y_loop();
                                startDist    = sqrt(((loopCentre_x-world_x)*(loopCentre_x-world_x))+((loopCentre_y- world_y)*(loopCentre_y- world_y)));
                                //initialRadiusConstraint = loop->get_min_radius();
                            }
                        }
                        break;
                    }
                    case AW_Mouse_Drag:{
                        base = (SEC_Base*)ct->client_data1;
                        if (base) {
                            if(base->getType()==SEC_HELIX_STRAND) {
                                strand     = (SEC_helix_strand*)base;
                                helix_info = strand->get_helix_info();
                                fixpoint_x = strand->get_fixpoint_x();
                                fixpoint_y = strand->get_fixpoint_y();
                                endDist    = sqrt(((fixpoint_x-world_x)*(fixpoint_x-world_x))+((fixpoint_y- world_y)*(fixpoint_y- world_y)));
                                finalLengthConstraint            = ((endDist/startDist) + endDist);
                                helix_info->get_min_length_ref() = finalLengthConstraint;
                            }
                            if(base->getType()==SEC_SEGMENT) {
                                segment      = (SEC_segment*)base;
                                loop         = segment->get_loop();
                                loopCentre_x = loop->get_x_loop();
                                loopCentre_y = loop->get_y_loop();
                                endDist      = sqrt(((loopCentre_x-world_x)*(loopCentre_x-world_x))+((loopCentre_y- world_y)*(loopCentre_y- world_y)));
                                //	finalRadiusConstraint      = ((endDist/startDist) * initialRadiusConstraint);
                                loop->get_min_radius_ref() = endDist;//finalRadiusConstraint;
                            }
                        }
                        sec_root->update();
                        exports.refresh = 1;
                        exports.save    = 1;
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }

            // --------------------------------------------------------------------------------

        case AWT_MODE_PROINFO: {
            if(button==AWT_M_MIDDLE) {
                break;
            }
            if (button==AWT_M_LEFT) { // should paint the pattern which user selects
                if (type==AW_Mouse_Press) {
                    SEC_Base *base  = (SEC_Base*)ct->client_data1;
                    int clicked_pos = ct->client_data2;
                    if (base)
                        sec_root->paintSearchPatternStrings(device, clicked_pos, world_x+1, world_y);
                }
            }
            if(button==AWT_M_RIGHT) {
                exports.refresh = 1;
                sec_root->update(0);
                //awmm->callback(AW_POPUP, (AW_CL)ED4_create_search_window, (AW_CL)probe);
                break; // should popup probe search pattern window
            }
            break;
        }

            // --------------------------------------------------------------------------------

        case AWT_MODE_MOVE: { // helix<->loop-exchange-modus
            if(button==AWT_M_MIDDLE) {
                break;
            }
            if (button==AWT_M_LEFT) {
                switch(type) {
                    case AW_Mouse_Press: {
                        if (!(ct && ct->exists)) {
                            break;
                        }

                        {
                            //check security level @@@
                            SEC_Base *base = (SEC_Base *)ct->client_data1;
                            if (base) {
#if defined(DEBUG) && 1
                                const char *whatAmI;
                                SEC_region *reg = 0;

                                switch (base->getType()) {
                                    case SEC_SEGMENT: {
                                        whatAmI = "Segment";
                                        SEC_segment *segment = (SEC_segment*)base;
                                        reg = segment->get_region();
                                        break;
                                    }
                                    case SEC_HELIX_STRAND: {
                                        whatAmI = "Strand";
                                        SEC_helix_strand *strand = (SEC_helix_strand*)base;
                                        reg = strand->get_region();
                                        break;
                                    }
                                    case SEC_LOOP: {
                                        whatAmI = "Loop";
                                        break;
                                    }
                                    default: {
                                        whatAmI = "unknown";
                                        break;
                                    }
                                }

                                GB_CSTR msg;
                                if (reg) {
                                    msg = GBS_global_string("Clicked on %s : positions[%i, %i]", (char*)whatAmI, reg->get_sequence_start(), reg->get_sequence_end());
                                }
                                else {
                                    msg = GBS_global_string("Clicked on %s - should not happen!?!", (char*)whatAmI);
                                }
                                aw_message(msg);
#endif
                                if (base->getType() == SEC_SEGMENT) {
                                    int clicked_pos = ct->client_data2;
                                    if (clicked_pos > int(sec_root->helix->size)) {
                                        aw_message("Index of clicked Base is out of range");
                                        break;
                                    }
                                    if (sec_root->helix->entries[clicked_pos].pair_type == HELIX_NONE) {
                                        aw_message("Selected Base is not suitable to split");
                                        break;
                                    }

                                    char *clicked_helix_nr = sec_root->helix->entries[clicked_pos].helix_nr;
                                    char *helix_nr;
                                    int i, min_index=(-1), max_index=(-1);
                                    for (i=0; i<int(sec_root->helix->size); i++) {
                                        helix_nr = sec_root->helix->entries[i].helix_nr;
                                        if ( (helix_nr != NULL) && (clicked_helix_nr != NULL) ) {
                                            if(!strcmp(helix_nr, clicked_helix_nr)) {
                                                if (min_index<0) {
                                                    min_index = i;
                                                }
                                                max_index = i;
                                            }
                                        }
                                    }
                                    int min_index_partner = sec_root->helix->entries[min_index].pair_pos;
                                    int max_index_partner = sec_root->helix->entries[max_index].pair_pos;

                                    //prevent "going backwards through the sequence"
                                    if (min_index > max_index_partner) {
                                        sec_root->split_loop(max_index_partner, min_index_partner+1, min_index, max_index+1);
                                    }
                                    else {
                                        sec_root->split_loop(min_index, max_index+1, max_index_partner, min_index_partner+1);
                                    }

                                    exports.refresh = 1;
                                    exports.save = 1;
                                    exports.resize = 1;
                                }else{
                                    aw_message("You can only split loops");
                                }
                                this->rot_ct = *ct;
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            else if (button==AWT_M_RIGHT) {
                switch(type) {
                    case AW_Mouse_Press: {
                        if( !(ct && ct->exists) ) {
                            break;
                        }

                        /*** check security level @@@ ***/
                        SEC_Base *base = (SEC_Base *)ct->client_data1;
                        if (base) {
                            if (base->getType() == SEC_HELIX_STRAND) {
                                sec_root->unsplit_loop((SEC_helix_strand *) base);

#if defined(DEBUG) && 0
                                ofstream fout("unsplit_test", ios::out);
                                fout << sec_root->write_data();
                                fout.close();
#endif

                                exports.refresh = 1;
                                exports.save = 1;
                                exports.resize = 1;
                            }
                            else {
                                aw_message("You can only unsplit helices");
                            }
                            this->rot_ct = *ct;
                        }
                        break;   // break of case AW_Mouse_Press
                    }
                    default: {
                        break;
                    }
                }
            }
            break; //break for case AWT_MODE_MOVE
        }
            /* ******************************************************** */
        case AWT_MODE_SETROOT: { // set-root-mode
            if(button==AWT_M_MIDDLE) {
                break;
            }
            switch(type) {
                case AW_Mouse_Press: {
                    if( !(ct && ct->exists) ) {
                        break;
                    }

                    /*** check security level @@@ ***/

                    SEC_Base *base = (SEC_Base *)ct->client_data1;
                    if(base) {
                        sec_root->set_root(base);
                        exports.refresh = 1;
                        exports.save = 1;
                        exports.resize = 1;
                        this->rot_ct = *ct;
                        sec_root->update(0);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
            /* ******************************************************** */
        case AWT_MODE_ROT: { // rotate-branches-mode
            if(button==AWT_M_MIDDLE) {
                break;
            }
            if(button==AWT_M_LEFT) {
                sec_root->drag_recursive = 1;
            }

            double end_angle, dif_angle, delta;
            double fixpoint_x, fixpoint_y;
            SEC_helix_strand *strand_pointer;
            SEC_helix *helix_info;
            SEC_Base *base;

            switch(type) {
                case AW_Mouse_Press: {
                    base = (SEC_Base *)ct->client_data1;
                    if (base) {
                        if (base->getType() == SEC_HELIX_STRAND) {
                            strand_pointer = (SEC_helix_strand *) base;
                            sec_root->rotateBranchesMode = 1;

                            //special treating for root_loop's caller
                            // 		    SEC_loop *root_loop = (sec_root->get_root_segment())->get_loop();

                            /* if (strand_pointer == root_loop->get_segment()->get_next_helix()) {
                            //turn around root_loop caller's angle  -- warum??
                            helix_info = strand_pointer->get_helix_info();
                            double tmp_delta = helix_info->get_delta();
                            helix_info->set_delta(tmp_delta + M_PI);
                            }*/

                            fixpoint_x = strand_pointer->get_fixpoint_x();
                            fixpoint_y = strand_pointer->get_fixpoint_y();
                            strand_pointer->start_angle = (2*M_PI) + atan2( (world_y - fixpoint_y), (world_x - fixpoint_x) );
                            helix_info = strand_pointer->get_helix_info();
                            strand_pointer->old_delta = helix_info->get_delta();
                        }
                        else {
                            aw_message("Only helix-strands can be dragged");
                        }
                    }
                    break;
                }
                case AW_Mouse_Drag: {
                    base = (SEC_Base *)ct->client_data1;
                    if(base) {
                        if (base->getType() == SEC_HELIX_STRAND) {
                            strand_pointer = (SEC_helix_strand *) base;

                            fixpoint_x = strand_pointer->get_fixpoint_x();
                            fixpoint_y = strand_pointer->get_fixpoint_y();
                            end_angle = (2*M_PI) + atan2( (world_y - fixpoint_y), (world_x - fixpoint_x) );
                            dif_angle = end_angle - strand_pointer->start_angle;

                            if (sec_root->drag_recursive) {
                                strand_pointer->update(fixpoint_x, fixpoint_y, dif_angle);
                                helix_info = strand_pointer->get_helix_info();
                                strand_pointer->compute_attachment_points(helix_info->get_delta());
                                strand_pointer->start_angle = end_angle;
                            }
                            else {
                                delta = strand_pointer->old_delta;
                                delta = delta + dif_angle + (2*M_PI);  //adding 2 PI converts negative angles into positive ones
                                helix_info = strand_pointer->get_helix_info();
                                helix_info->set_delta(delta);
                                strand_pointer->compute_attachment_points(delta);
                                strand_pointer->update(fixpoint_x, fixpoint_y, 0);
                            }

                            SEC_loop *root_loop = (sec_root->get_root_segment())->get_loop();
                            if (strand_pointer != root_loop->get_segment()->get_next_helix()) {
                                sec_root->update(0);
                            }
                            exports.refresh = 1;
                        }
                    }
                    break;
                }
                case AW_Mouse_Release: {
                    sec_root->drag_recursive = 0;
                    exports.refresh = 1;
                    exports.save = 1;

                    base = (SEC_Base *)ct->client_data1;
                    if(base) {
                        if (base->getType() == SEC_HELIX_STRAND) {
                            strand_pointer = (SEC_helix_strand *) base;

                            //special treating for root_loop's caller
                            // 		    SEC_loop *root_loop = (sec_root->get_root_segment())->get_loop();

                            /*  if (strand_pointer == root_loop->get_segment()->get_next_helix()) {
                                helix_info = strand_pointer->get_helix_info();
                                double tmp_delta = helix_info->get_delta();
                                helix_info->set_delta(tmp_delta + M_PI );
                                }*/
                        }
                    }
                    sec_root->update(0);
                    sec_root->rotateBranchesMode = 0;
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
            /* ******************************************************** */
        case AWT_MODE_MOD: { // show constraints
            exports.refresh = 1;
            if(button==AWT_M_MIDDLE) {
                break;
            }
            if(button==AWT_M_LEFT) {
                if (type==AW_Mouse_Press) {
                    SEC_Base *base = (SEC_Base*)ct->client_data1;

                    if (base) {
                        switch (base->getType()) {
                            case SEC_HELIX_STRAND: {
                                SEC_helix_strand *strand = (SEC_helix_strand*)base;
                                SEC_helix *helix_info = strand->get_helix_info();
                                if (change_constraints("length", "strand", helix_info->get_min_length_ref(), helix_info->get_max_length_ref())==0) {
                                    sec_root->update();
                                }
                                break;
                            }
                            case SEC_SEGMENT: {
                                SEC_segment *segment = (SEC_segment*)base;
                                SEC_loop *loop = segment->get_loop();
                                if (change_constraints("radius", "loop", loop->get_min_radius_ref(), loop->get_max_radius_ref())==0) {
                                    sec_root->update();
                                }
                                break;
                            }
                            default: {
#if defined(DEBUG)
                                sec_assert(0);
#endif // DEBUG
                                break;
                            }
                        }
                    }
                }
            }

            break;  //break for case AWT_MODE_MOD
        }
            /* ******************************************************** */

        case AWT_MODE_LINE: { // set-cursor-mode (in ARB_EDIT4)
            if (button==AWT_M_MIDDLE) {
                break;
            }
            if (button==AWT_M_LEFT) {
                if (type==AW_Mouse_Press) {
                    SEC_Base *base = (SEC_Base*)ct->client_data1;
                    int clicked_pos = ct->client_data2;

                    if (base) {
                        if (clicked_pos>int(sec_root->helix->size)) {
                            aw_message("Index of clicked Base is out of range");
                            break;
                        }
#if defined(DEBUG) && 1
                        printf("Clicked pos = %i\n", clicked_pos);
#endif
                        aw_root->awar_int(AWAR_SET_CURSOR_POSITION)->write_int(clicked_pos+1); // sequence position is starting with 1 !!!
                    }
                }
            }
            break;
        }
            /* ******************************************************** */
        default: {
#if defined(DEBUG)
            sec_assert(0);
#endif // DEBUG
            break;
        }
    }
}

extern GBDATA *gb_main;
SEC_graphic::SEC_graphic(AW_root *aw_rooti, GBDATA *gb_maini):AWT_graphic() {
    gb_struct = 0;
    gb_struct_ref = 0;
    last_saved = 0;
    change_flag = 0;

    exports.dont_fit_x = 0;
    exports.dont_fit_y = 0;
    exports.left_offset = 20;
    exports.right_offset = 20;
    exports.top_offset = 20;
    exports.bottom_offset = 20;
    exports.dont_scroll = 0;

    this->aw_root = aw_rooti;
    this->gb_main = gb_maini;
    rot_ct.exists = AW_FALSE;
    rot_cl.exists = AW_FALSE;
    sec_root = 0;
    sec_root = new SEC_root(NULL, 0, aw_rooti->awar(AWAR_SECEDIT_DIST_BETW_STRANDS)->read_float(),aw_rooti->awar(AWAR_SECEDIT_SKELETON_THICKNESS)->read_float());
    sec_root->set_show_debug(aw_rooti->awar(AWAR_SECEDIT_SHOW_DEBUG)->read_int());
    sec_root->set_show_helixNrs(aw_rooti->awar(AWAR_SECEDIT_SHOW_HELIX_NRS)->read_int());
    sec_root->set_show_strSkeleton(aw_rooti->awar(AWAR_SECEDIT_SHOW_STR_SKELETON)->read_int());
    sec_root->set_hide_bases(aw_rooti->awar(AWAR_SECEDIT_HIDE_BASES)->read_int());
    sec_root->set_hide_bonds(aw_rooti->awar(AWAR_SECEDIT_HIDE_BONDS)->read_int());
    sec_root->set_display_sai(aw_rooti->awar(AWAR_SECEDIT_DISPLAY_SAI)->read_int());
}

SEC_graphic::~SEC_graphic(void) {

}

void SEC_structure_changed_cb(GBDATA *gb_seq, SEC_graphic *sec, GB_CB_TYPE type) { // !!!
    if (type == GB_CB_DELETE) {
        sec->gb_struct = NULL;
        sec->gb_struct_ref = NULL;
        sec->load(gb_main, 0,0,0);
        return;
    }
    // if we are the reason for the save, do nothing
    if (GB_read_clock(gb_seq) <= sec->last_saved) return;

    sec->load(gb_main,0,0,0); 	// reload everything
}

//  ---------------------------------------------------------
//      static void SEC_alignment_length_changed(AW_root *awr)
//  ---------------------------------------------------------
// This is called when ... is set to a new value.
// Reloads all column-specific stuff (helix, ecoli)
static void SEC_alignment_length_changed(AW_root */*awr*/) {
    SEC_graphic *sec_graphic = SEC_GRAPHIC;
    sec_graphic->gb_struct = NULL;
    sec_graphic->gb_struct_ref = NULL;
    sec_graphic->load(sec_graphic->gb_main, 0, 0, 0);
}

/** read awar AWAR_HELIX_NAME to get the name */
GB_ERROR SEC_graphic::load(GBDATA *dummy, const char *,AW_CL link_to_database, AW_CL insert_delete_cbs) {
    AWUSE(dummy);
    AWUSE(link_to_database);
    AWUSE(insert_delete_cbs);
    GB_transaction tscope(gb_main);
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


    change_flag = SEC_UPDATE_RELOADED;
    if (gb_struct) {
        this->last_saved = GB_read_clock(gb_struct);	// mark as loaded
    }


    /************************** Setup new structure *******************************/
    char *name = GBT_read_string2(gb_main,AWAR_HELIX_NAME, GBT_get_default_helix(gb_main) );
    GBDATA *gb_species = GBT_find_SAI(gb_main,name);
    if (!gb_species) return GB_export_error("Cannot find helix templace SAI '%s'",name);

    char *ali_name = GBT_get_default_alignment(gb_main);
    long ali_len = GBT_get_alignment_len(gb_main,ali_name);
    if (ali_len < 10) {
        return GB_export_error("alignment '%s' to short to generate helix",ali_name);
    }
    GBDATA *gb_ali = GB_search(gb_species, ali_name, GB_FIND);
    if (!gb_ali) return GB_export_error("Your helix structure template '%s' has no valid sequence for alignment '%s'",
                                        name,ali_name);				// no sequence for name in the database !!!

    /******************************* Build default bone struct for empty templates *******************************/
    gb_struct = GB_search(gb_ali,NAME_OF_STRUCT_SEQ, GB_FIND);

    int create_default = 0;
    GB_ERROR err = 0;
    {
        char *reason = 0;
        if (gb_struct) {
            gb_struct_ref = GB_search(gb_ali , NAME_OF_REF_SEQ , GB_STRING);
            /********** Load structure **************/
            char *strct = GB_read_string(gb_struct);
            char *ref = GB_read_string(gb_struct_ref);
            err = sec_root->read_data(strct,ref,ali_len);
            free(strct);
            free(ref);

            if (err) {
                reason = (char*)malloc(200);
#if defined(DEBUG)
                int printed = sprintf(reason, "an error (%s) which occurred while loading your secondary structure", err);
                sec_assert(printed<200);
#endif // DEBUG
                create_default = 1;
            }
        }
        else {
            reason = strdup("no secondary structure was found in your database");
            create_default = 1;
        }

        if (create_default) {
#if defined(DEBUG)
            sec_assert(reason);
#endif // DEBUG
            int len = strlen(reason)+100;
            char *msg = (char*)malloc(len);
            /*int printed = */ sprintf(msg, "Due to %s\nyou can choose between..", reason);

            int answer = aw_message(msg, "Create new,Exit");
            if (answer==1) {
                create_default = 0;
                err = GB_export_error("no secondary structure in database");
            }
            else {
                err = 0;
            }
            free(msg);
            free(reason);
        }
        else {
#if defined(DEBUG)
            sec_assert(!reason);
#endif // DEBUG
        }
    }

    if (create_default) {	// build default helix struct

        sec_root->create_default_bone(ali_len);
        gb_struct = GB_create(gb_ali, NAME_OF_STRUCT_SEQ, GB_STRING);
        gb_struct_ref = GB_search(gb_ali, NAME_OF_REF_SEQ, GB_STRING);
        GB_write_string(gb_struct, ""); // really clean up
        GB_write_string(gb_struct_ref, "");
    }

    if (!err) {
        last_saved = GB_read_clock(gb_struct);	// mark as loaded

        /************************* Listen to the database ***************************/
        GB_add_callback(gb_struct,GB_CB_ALL,(GB_CB)SEC_structure_changed_cb, (int *)this);
        GB_add_callback(gb_struct_ref,GB_CB_ALL,(GB_CB)SEC_structure_changed_cb, (int *)this);
    }

    free(name);
    free(ali_name);

    return err;
}

/** Save secondary structure to database */
GB_ERROR SEC_graphic::save(GBDATA *, const char *,AW_CL,AW_CL)
{
    if (!gb_struct) return 0;	// not loaded, so don't save
    if (!sec_root) return 0;
    char *data = sec_root->write_data();
    char *x_string = sec_root->x_string;
    GB_transaction tscope(gb_main);
    GB_ERROR error = GB_write_string(gb_struct,data);
    if (!error) {
        error = GB_write_string(gb_struct_ref,x_string);
    }
    this->last_saved = GB_read_clock(gb_struct);
    if (error) aw_message(error);
    return NULL;
}

GB_ERROR SEC_graphic::write_data_to_db(const char *data, const char *x_string)
{
    if (!gb_struct) return 0;
    if (!sec_root) return 0;
    GB_transaction tscope(gb_main);
    GB_ERROR error = GB_write_string(gb_struct, data);
    if (!error) {
        error = GB_write_string(gb_struct_ref, x_string);
    }
    last_saved = 0; // force reload of data
    return error;
}

int SEC_graphic::check_update(GBDATA *gbdummy) {
    AWUSE(gbdummy);
    GB_transaction dummy(gb_main);
    int res = change_flag;
    change_flag = SEC_UPDATE_OK;
    return res;
}

void SEC_graphic::show(AW_device *device)	{
    if (sec_root) {
        sec_root->clear_last_drawed_cursor_position();
        sec_root->paint(device);
    }else{
        device->line(SEC_GC_DEFAULT, -100,-100,100,100);
        device->line(SEC_GC_DEFAULT, -100,100,100,-100);
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
// implementation of SEC_bond_def
// --------------------------------------------------------------------------------

void SEC_bond_def::clear()
{
    int i, j;
    for (i=0; i<SEC_BOND_BASE_CHARS; i++) {
        for (j=0; j<SEC_BOND_BASE_CHARS; j++) {
            bond[i][j] = ' ';
        }
    }
}

int SEC_bond_def::get_index(char base) const
{
    const char *allowed = SEC_BOND_BASE_CHAR;
    const char *found = strchr(allowed, toupper(base));

    if (!found) return -1;
    int idx = int(found-allowed);
#if defined(DEBUG)
    sec_assert(idx>=0 && idx<SEC_BOND_BASE_CHARS);
#endif // DEBUG
    return idx;
}

int SEC_bond_def::update(AW_root *aw_root)
{
    int ok = 1;

    clear();
#define INSERT(type) insert(aw_root->awar(AWAR_SECEDIT_##type##_PAIRS)->read_string(), aw_root->awar(AWAR_SECEDIT_##type##_PAIR_CHAR)->read_string()[0]);
    ok = ok && INSERT(STRONG);
    ok = ok && INSERT(NORMAL);
    ok = ok && INSERT(WEAK);
    ok = ok && INSERT(NO);
    ok = ok && INSERT(USER);
#undef INSERT

    return ok;
}
int SEC_bond_def::insert(const char *pairs, char pair_char)
{
    char c1 = 0;

    if (pair_char==0) pair_char = ' ';

    if (!strchr(SEC_BOND_PAIR_CHAR, pair_char)) {
        aw_message(GBS_global_string("Illegal pair-character '%c' (allowed: '%s')", pair_char, SEC_BOND_PAIR_CHAR), "OK");
        pair_char = SEC_BOND_PAIR_CHAR[0];
    }

    while (1) {
        char c2 = *pairs++;

        if (!c2) break;

        if (c2!=' ') {
            if (c1==0) {
                c1 = c2;
            }
            else {
                int i1 = get_index(c1);
                int i2 = get_index(c2);

                if (i1==-1 || i2==-1) {
                    char ic = i1==-1 ? c1 : c2;
                    aw_message(GBS_global_string("Illegal base-character '%c' (allowed: '%s')", ic, SEC_BOND_BASE_CHAR), "OK");
                    return 0;
                }
                else {
                    bond[i1][i2] = pair_char;
                    bond[i2][i1] = pair_char;
                }
                c1 = 0;
            }
        }
    }
    return 1;
}

char SEC_bond_def::get_bond(char base1, char base2) const
{
    int i1 = get_index(base1);
    int i2 = get_index(base2);

    if (i1==-1 || i2==-1) {
        return ' ';
    }
    return bond[i1][i2];
}

void SEC_bond_def::paint(AW_device *device, SEC_root *root, char base1, char base2, double x1, double y1, double x2, double y2, double base_dist, double char_size) const
{
    char Bond = get_bond(base1, base2);

    if (Bond==' ') return;

    double dx = x2-x1;
    double dy = y2-y1;
    double dist = sqrt(dx*dx + dy*dy);

    double factor = (char_size*0.5)/dist; // distance from center of char is 50% of char size

    double ox = dx*factor; // distance bond <-> center of base character
    double oy = dy*factor;

    if (fabs(dx)<=fabs(2*ox) || fabs(dy)<=fabs(2*oy)) return;

#if defined(DEBUG) && 0
    printf("x1=%f y1=%f x2=%f y2=%f char_size=%f dx=%f dy=%f dist=%f factor=%f ox=%f oy=%f\n", x1, y1, x2, y2, char_size, dx, dy, dist, factor, ox, oy);
#endif

    double x3 = x1 + ox; // (x3, y3), (x4, y4) = end points of bond
    double y3 = y1 + oy;
    double x4 = x2 - ox;
    double y4 = y2 - oy;

    double xm = (x3+x4)/2; // (xm, ym) = mid point of bond
    double ym = (y3+y4)/2;

    double xs = ((-dy)*base_dist)/(10*dist); 	// (xs, ys) = vector orthogonal to (dx, dy)
    double ys = (dx*base_dist)/(10*dist);	// length = 10% of base_dist

    device->set_line_attributes(SEC_GC_CURSOR, 3, AW_SOLID);

    switch (Bond) {
        case '-': { // line
            device->line(SEC_GC_BONDS, x3, y3, x4, y4, root->helix_filter, 0, 0);
            break;
        }
        case '=': {
            device->line(SEC_GC_BONDS, x3+xs, y3+ys, x4+xs, y4+ys, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, x3-xs, y3-ys, x4-xs, y4-ys, root->helix_filter, 0, 0);
            break;
        }
        case '+': {
            device->line(SEC_GC_BONDS, x3, y3, x4, y4, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, xm+2*xs, ym+2*ys, xm-2*xs, ym-2*ys, root->helix_filter, 0, 0);
            break;
        }
        case '#': {
            device->line(SEC_GC_BONDS, x3+xs, y3+ys, x4+xs, y4+ys, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, x3-xs, y3-ys, x4-xs, y4-ys, root->helix_filter, 0, 0);

            double xs1 = (x3+xm)/2;	// points where horizontal bars cross the line (x3/y3)-(x4/y4)
            double ys1 = (y3+ym)/2;
            double xs2 = (x4+xm)/2;
            double ys2 = (y4+ym)/2;

            device->line(SEC_GC_BONDS, xs1+2*xs, ys1+2*ys, xs1-2*xs, ys1-2*ys, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, xs2+2*xs, ys2+2*ys, xs2-2*xs, ys2-2*ys, root->helix_filter, 0, 0);

            break;
        }
        case '~': {
            double xs1 = (x3+xm)/2 + xs;
            double ys1 = (y3+ym)/2 + ys;
            double xs2 = (x4+xm)/2 - xs;
            double ys2 = (y4+ym)/2 - ys;

            device->line(SEC_GC_BONDS, x3,  y3,  xs1, ys1, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, xs1, ys1, xs2, ys2, root->helix_filter, 0, 0);
            device->line(SEC_GC_BONDS, xs2, ys2, x4,  y4,  root->helix_filter, 0, 0);

            break;
        }
        case 'o': { // circle
            double diameter = dist*0.5*0.3;
            device->circle(SEC_GC_BONDS, false, xm, ym, diameter, diameter, root->helix_filter, 0, 0);
            break;
        }
        case '.': { // point
            double diameter = dist*0.5*0.1;
            device->circle(SEC_GC_BONDS, false, xm, ym, diameter, diameter, root->helix_filter, 0, 0);
            break;
        }
        case ' ': { // no bond
            break;
        }
        default: {
#if defined(DEBUG) && 1
            printf("Illegal bond-char '%c' (=%i) for %c/%c\n", Bond, Bond, base1, base2);
            sec_assert(0);
#endif
            break;
        }
    }
}


void SEC_add_awar_callbacks(AW_root *aw_root,AW_default /*def*/, AWT_canvas *ntw) {
    aw_root->awar(AWAR_SECEDIT_DIST_BETW_STRANDS)->add_callback(SEC_distance_between_strands_changed_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_SKELETON_THICKNESS)->add_callback(SEC_skeleton_thickness_changed_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_SHOW_DEBUG)->add_callback(SEC_show_debug_toggled_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_SHOW_HELIX_NRS)->add_callback(SEC_show_helixNrs_toggled_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_SHOW_STR_SKELETON)->add_callback(SEC_show_strSkeleton_toggled_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_HIDE_BASES)->add_callback(SEC_hide_bases_toggled_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_HIDE_BONDS)->add_callback(SEC_hide_bonds_toggled_cb, (AW_CL)ntw);
    aw_root->awar(AWAR_SECEDIT_DISPLAY_SAI)->add_callback(SEC_display_sai_toggled_cb, (AW_CL)ntw);

    char *ali_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_alignment = GBT_get_alignment(gb_main,ali_name);
    if (!gb_alignment) aw_message("You can't edit without an existing alignment", "EXIT");
    GBDATA *gb_alignment_len = GB_search(gb_alignment,"alignment_len",GB_FIND);
    GB_add_callback(gb_alignment_len, (GB_CB_TYPE)GB_CB_CHANGED, (GB_CB)SEC_alignment_length_changed, 0);

#define CALLBACK_PAIR(type)                                                                             \
    aw_root->awar(AWAR_SECEDIT_##type##_PAIRS)    ->add_callback(SEC_pair_def_changed_cb, (AW_CL)ntw);  \
    aw_root->awar(AWAR_SECEDIT_##type##_PAIR_CHAR)->add_callback(SEC_pair_def_changed_cb, (AW_CL)ntw)

    CALLBACK_PAIR(STRONG);
    CALLBACK_PAIR(NORMAL);
    CALLBACK_PAIR(WEAK);
    CALLBACK_PAIR(NO);
    CALLBACK_PAIR(USER);

#undef CALLBACK_PAIR

    SEC_GRAPHIC->bond.update(SEC_GRAPHIC->aw_root);
}

