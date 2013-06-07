// =============================================================== //
//                                                                 //
//   File      : SEC_main.cxx                                      //
//   Purpose   : main part of SECEDIT                              //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "SEC_root.hxx"
#include "SEC_graphic.hxx"
#include "SEC_helix.hxx"
#include "SEC_drawn_pos.hxx"
#include "SEC_toggle.hxx"

#include <FileBuffer.h>

#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <arb_file.h>

#ifndef sec_assert // happens in NDEBUG mode
#define sec_assert(cond) arb_assert(cond)
#endif

void SEC_root::invalidate_base_positions() {
    if (root_loop) {
        SEC_base_part *start_part = root_loop->get_fixpoint_strand();
        SEC_base_part *part       = start_part;

        do {
            part->get_region()->invalidate_base_count();
            part = part->next();
        }
        while (part != start_part);
    }
}

// ------------------------------------------------------
//      auto-scrolling (triggered by structure self)

void SEC_root::nail_position(size_t absPos) {
    if (drawnPositions) {
        nailedAbsPos = absPos;
        drawnAbsPos  = *drawnPositions->drawn_at(absPos);
    }
}

void SEC_root::nail_cursor() { // re-position on cursor
    if (cursorAbsPos >= 0) {
        if (drawnPositions && !drawnPositions->empty()) {
            size_t abs;
            drawnAbsPos  = drawnPositions->drawn_after(cursorAbsPos-1, &abs);
            nailedAbsPos = abs;
        }
    }
}

void SEC_root::add_autoscroll(const Vector& scroll) {
    if (autoscroll) *autoscroll += scroll;
    else autoscroll              = new Vector(scroll);
}

bool SEC_root::perform_autoscroll() {
    bool        scrolled = false;
    AWT_canvas *canvas   = db->canvas();

    if (canvas && (nailedAbsPos != -1 || autoscroll)) {
        AW_device *device = canvas->aww->get_device(AW_MIDDLE_AREA);

        if (nailedAbsPos != -1) {
            {
                int     absPos = nailedAbsPos;
                Vector *scroll = autoscroll;

                // avoid endless recursion:
                nailedAbsPos = -1;
                autoscroll   = 0;

#if defined(WARN_TODO)
#warning make the refresh invisible
#endif
                canvas->refresh();

                nailedAbsPos = absPos;
                autoscroll   = scroll;
            }
            const Position *newPos = drawnPositions->drawn_at(nailedAbsPos);
            if (newPos) {
                Vector old2new(drawnAbsPos, *newPos);
#if defined(DEBUG) && 0
                printf("drawnAbsPos=%.2f/%.2f newPos=%.2f/%.2f old2new=%.2f/%.2f\n",
                       drawnAbsPos.xpos(), drawnAbsPos.ypos(),
                       newPos->xpos(), newPos->ypos(),
                       old2new.x(), old2new.y());
#endif // DEBUG
                add_autoscroll(old2new);
            }
            nailedAbsPos = -1;
        }
        if (autoscroll) {
            canvas->init_device(device); // loads correct zoom
            Vector screen_scroll = device->transform(*autoscroll);

#if defined(DEBUG) && 0
            printf("autoscroll=%.2f/%.2f screen_scroll=%.2f/%.2f\n",
                   autoscroll->x(), autoscroll->y(),
                   screen_scroll.x(), screen_scroll.y());
#endif // DEBUG

            delete autoscroll;
            autoscroll = 0;

            device->clear(-1);
            canvas->scroll(screen_scroll);
            scrolled = true;
        }
    }
    return scrolled;
}
// --------------------------------------------------------------------------------

void SEC_root::position_cursor(bool toCenter, bool evenIfVisible) {
    // centers the cursor in display (or scrolls it into view)
    // if 'evenIfVisible' is true, always do it, otherwise only if not (fully) visible

    const LineVector& cursorLine = get_last_drawed_cursor_position();
    sec_assert(cursorLine.valid());

    AWT_canvas *scr    = db->canvas();
    AW_device  *device = scr->aww->get_device(AW_MIDDLE_AREA);

    Rectangle cursor(device->transform(cursorLine));
    Rectangle screen(device->get_area_size(), INCLUSIVE_OUTLINE);

    if (evenIfVisible || !screen.contains(cursor)) {
        if (!toCenter) {
            if (perform_autoscroll()) {
                cursor = Rectangle(device->transform(cursorLine));
                if (!screen.contains(cursor)) {
                    toCenter = true;
                }
            }
            else { // if autoscroll didn't work
                toCenter = true; // center cursor
            }
        }

        if (toCenter) {
            Vector scroll(cursor.centroid(), screen.centroid());

#if defined(DEBUG) && 1
            printf("Auto-scroll: scroll = (%f, %f) [Center cursor]\n", scroll.x(), scroll.y());
#endif
            scr->scroll(-scroll);
            scr->refresh();
        }
    }
}

static void SEC_toggle_cb(AW_window *, AW_CL cl_secroot, AW_CL) {
    SEC_root               *root = (SEC_root*)cl_secroot;
    const SEC_db_interface *db   = root->get_db();

    GB_ERROR error = db->structure()->next();
    if (error) aw_message(error);
    db->canvas()->refresh();
}

static void SEC_center_cb(AW_window *, AW_CL cl_secroot, AW_CL) {
    SEC_root *root = (SEC_root*)cl_secroot;
    root->position_cursor(true, true);
}

static void SEC_fit_window_cb(AW_window * /* aw */, AW_CL cl_secroot, AW_CL) {
    SEC_root               *root = (SEC_root*)cl_secroot;
    const SEC_db_interface *db   = root->get_db();

    db->graphic()->request_update(SEC_UPDATE_ZOOM_RESET);
    db->canvas()->refresh();
}

static void sec_mode_event(AW_window *aws, AW_CL cl_secroot, AW_CL cl_mode)
{
    SEC_root         *sec_root = (SEC_root*)cl_secroot;
    AWT_COMMAND_MODE  mode     = (AWT_COMMAND_MODE)cl_mode;
    const char       *text     = 0;

    sec_root->set_show_constraints(SEC_NO_TYPE);

    switch (mode) {
        case AWT_MODE_ZOOM: {
            text="ZOOM MODE : CLICK or SELECT an area to ZOOM IN (LEFT) or ZOOM OUT (RIGHT) ";
            break;
        }
        case AWT_MODE_MOVE: {
            text="HELIX MODE    LEFT: build helix   RIGHT: remove helix";
            break;
        }
        case AWT_MODE_SETROOT: {
            text="SET ROOT MODE    LEFT: Set logical center of structure   RIGHT: Reset angles on sub-structure";
            break;
        }
        case AWT_MODE_ROT: {
            text="ROTATE MODE    LEFT: Rotate helix/loop RIGHT: w/o substructure";
            break;
        }
        case AWT_MODE_STRETCH: {
            text="STRETCH MODE  : LEFT-CLICK & DRAG: stretch HELICES and LOOPS   RIGHT: Reset";
            sec_root->set_show_constraints(SEC_ANY_TYPE);
            break;
        }
        case AWT_MODE_EDIT: {
            text="CONSTRAINT MODE    LEFT: modify constraint";
            sec_root->set_show_constraints(SEC_ANY_TYPE);
            break;
        }
        case AWT_MODE_LINE: {
            text="SET CURSOR MODE    LEFT: Set Cursor in ARB_EDIT4";
            break;
        }
        case AWT_MODE_PROINFO: {
            text="PROBE INFO MODE    LEFT: Displays PROBE information   RIGHT: Clears the Display";
            break;
        }
        default: {
#if defined(DEBUG)
            sec_assert(0);
#endif // DEBUG
            break;
        }
    }

    sec_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    aws->get_root()->awar(AWAR_FOOTER)->write_string(text);

    AWT_canvas *scr = sec_root->get_db()->canvas();
    scr->set_mode(mode);
    scr->refresh();
}

static void SEC_undo_cb(AW_window *, AW_CL cl_db, AW_CL cl_undo_type) {
    SEC_db_interface *db        = (SEC_db_interface*)cl_db;
    GB_UNDO_TYPE      undo_type = (GB_UNDO_TYPE)cl_undo_type;

    GBDATA   *gb_main = db->gbmain();
    GB_ERROR  error   = GB_undo(gb_main, undo_type);
    if (error) {
        aw_message(error);
    }
    else {
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
        db->canvas()->refresh();
    }
}

// --------------------------------------------------------------------------------

#define ASS       "ARB secondary structure v1" // don't change version here!
#define ASS_START "[" ASS "]"
#define ASS_EOS   "[end of structure]"
#define ASS_EOF   "[end of " ASS "]"

static void export_structure_to_file(AW_window *, AW_CL cl_db)
{
    SEC_db_interface *db       = (SEC_db_interface*)cl_db;
    AW_root          *aw_root  = db->awroot();
    char             *filename = aw_root->awar(AWAR_SECEDIT_SAVEDIR"/file_name")->read_string();
    FILE             *out      = fopen(filename, "wt");
    GB_ERROR          error    = 0;

    if (out) {
        SEC_root *sec_root = db->secroot();

        fputs(ASS_START, out); fputc('\n', out);

        char *strct = sec_root->buildStructureString();
        fputs(strct, out);
        delete [] strct;

        fputs(ASS_EOS, out); fputc('\n', out);

        const XString& xstr     = sec_root->get_xString();
        const char    *x_string = xstr.get_x_string();

        sec_assert(xstr.get_x_string_length() == strlen(x_string));

        char *foldInfo = SEC_xstring_to_foldedHelixList(x_string, xstr.get_x_string_length(), sec_root->get_helixDef(), error);
        if (foldInfo) {
            fprintf(out, "foldedHelices=%s\n", foldInfo);
            free(foldInfo);
        }

        fputs(ASS_EOF, out); fputc('\n', out);
        fclose(out);

        if (error) GB_unlink_or_warn(filename, &error);
    }
    else {
        error = GB_export_errorf("Can't write secondary structure to '%s'", filename);
    }

    free(filename);
    if (error) aw_message(error);
}

inline GB_ERROR expectedError(const char *expected) {
    return GBS_global_string("expected '%s'", expected);
}
inline GB_ERROR expectContent(FileBuffer& file, const char *expected) {
    GB_ERROR error = 0;
    string   line;
    if (!file.getLine(line) || line != expected) {
        error = expectedError(expected);
    }
    return error;
}

static string scanToken(FileBuffer& file, string& rest, GB_ERROR& error) {
    string line;
    string token;

    sec_assert(error == 0);

    if (file.getLine(line)) {
        size_t equal = line.find('=');

        if (equal == string::npos) {
            error = "Expected '='";
        }
        else {
            token = line.substr(0, equal);
            rest  = line.substr(equal+1);
        }
    }
    else {
        error = "Unexpected EOF";
    }
    return token;
}

static GB_ERROR expectToken(FileBuffer& file, const char *token, string& content) {
    GB_ERROR error      = 0;
    string   foundToken = scanToken(file, content, error);
    if (foundToken != token) error = expectedError(token);
    return error;
}

static void import_structure_from_file(AW_window *, AW_CL cl_db) {
    GB_ERROR          error = 0;
    SEC_db_interface *db    = (SEC_db_interface*)cl_db;
    SEC_root         *root  = db->secroot();

    if (!root->has_xString()) {
        error = "Please select a species in EDIT4";
    }
    else {
        char        *filename = db->awroot()->awar(AWAR_SECEDIT_SAVEDIR"/file_name")->read_string();
        FILE        *in       = fopen(filename, "rt"); // closed by FileBuffer

        if (!in) {
            error = GB_export_errorf("Can't open file '%s'", filename);
        }
        else {
            FileBuffer file(filename, in);
            error = expectContent(file, ASS_START);

            string structure;
            while (!error) {
                string line;
                if (!file.getLine(line)) error = expectedError(ASS_EOS);
                else {
                    if (line == ASS_EOS) break;
                    structure += line + "\n";
                }
            }

            char *x_string = 0;
            if (!error) {
                string content;
                string token = scanToken(file, content, error);

                if (!error) {
                    // we already have an existing xstring, use it's length for new xstring
                    size_t xlength = root->get_xString().getLength();

                    if (token == "foldedHelices") { // new version
                        x_string = SEC_foldedHelixList_to_xstring(content.c_str(), xlength, root->get_helixDef(), error); // sets error
                    }
                    else if (token == "no of helices") { // old version
                        int saved_helices = atoi(content.c_str());

                        error             = expectToken(file, "length of xstring", content); // ignore, using curr value
                        if (!error) error = expectToken(file, "xstring_rel_helix", content);
                        if (!error) {
                            int helices_in_db;
                            x_string = old_decode_xstring_rel_helix(content.c_str(), xlength, root->get_helixDef(), &helices_in_db);
                            if (helices_in_db != saved_helices) {
                                error = GBS_global_string("Number of helices does not match (file=%i, db=%i).\n"
                                                          "Saving the structure again from another DB with correct number of helices will work around this restriction.",
                                                          saved_helices, helices_in_db);
                            }
                        }
                    }
                    else {
                        error = "Expected 'foldedHelices' or 'no of helices'";
                    }
                }
                if (!error) error = expectContent(file, ASS_EOF);
            }

            if (error) {
                error = GBS_global_string("%s", file.lineError(error).c_str());
            }
            else {
                db->graphic()->write_data_to_db(structure.c_str(), x_string);
                db->canvas()->refresh();
            }
            free(x_string);
        }
        free(filename);
    }
    if (error) aw_message(error);
}

#undef ASS
#undef ASS_START
#undef ASS_EOS
#undef ASS_EOF

static AW_window *SEC_importExport(AW_root *root, int export_to_file, SEC_db_interface *db)
{
    AW_window_simple *aws = new AW_window_simple;

    if (export_to_file) aws->init(root, "export_secondary_structure", "Export secondary structure to ...");
    else                aws->init(root, "import_secondary_structure", "Import secondary structure from ...");

    aws->load_xfig("sec_imexport.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"sec_imexport.hlp");
    aws->create_button("HELP", "HELP", "H");

    AW_create_fileselection(aws, AWAR_SECEDIT_SAVEDIR);

    aws->at("save");
    if (export_to_file) {
        aws->callback(export_structure_to_file, (AW_CL)db);
        aws->create_button("EXPORT", "EXPORT", "E");
    }
    else {
        aws->callback(import_structure_from_file, (AW_CL)db);
        aws->create_button("IMPORT", "IMPORT", "I");
    }

    return aws;
}

static AW_window *SEC_import(AW_root *root, AW_CL cl_db) { return SEC_importExport(root, 0, (SEC_db_interface*)cl_db); }
static AW_window *SEC_export(AW_root *root, AW_CL cl_db) { return SEC_importExport(root, 1, (SEC_db_interface*)cl_db); }

static void SEC_rename_structure(AW_window *, AW_CL cl_db, AW_CL) {
    SEC_db_interface      *db        = (SEC_db_interface*)cl_db;
    SEC_structure_toggler *structure = db->structure();

    char *new_name = aw_input("Rename structure", "New name", structure->name());
    if (new_name) {
        structure->setName(new_name);
        free(new_name);
        db->canvas()->refresh();
    }
}

static void SEC_new_structure(AW_window *, AW_CL cl_db, AW_CL) {
    SEC_db_interface      *db        = (SEC_db_interface*)cl_db;
    SEC_structure_toggler *structure = db->structure();

    if (!structure) {
        db->init_toggler();
        structure = db->structure();
        sec_assert(structure);
    }

    GB_ERROR error = 0;
    bool     done  = false;

    switch (aw_question(NULL, "Create new structure?", "Default bone,Copy current,Abort")) {
        case 0:                 // default bone
            error = structure->copyTo("Default");
            if (!error) {
                db->secroot()->create_default_bone();
                db->graphic()->save(0, 0, 0, 0);
                done = true;
            }
            break;

        case 1:                 // copy current
            error = structure->copyTo(GBS_global_string("%s(copy)", structure->name()));
            done  = !error;
            break;

        case 2:                 // abort
            break;
    }

    if (done) {
        db->graphic()->request_update(SEC_UPDATE_ZOOM_RESET);
        db->canvas()->refresh();
        SEC_rename_structure(0, cl_db, 0);
    }
}

static void SEC_delete_structure(AW_window *, AW_CL cl_db, AW_CL) {
    SEC_db_interface      *db        = (SEC_db_interface*)cl_db;
    SEC_structure_toggler *structure = db->structure();

    if (structure->getCount()>1) {
        if (aw_ask_sure("delete_sec_structure", GBS_global_string("Are you sure to delete structure '%s'?", structure->name()))) {
            GB_ERROR error = structure->remove();
            if (error) aw_message(error);
            db->canvas()->refresh();
        }
    }
    else {
        aw_message("You cannot delete the last structure");
    }
}

static void SEC_sync_colors(AW_window *aww, AW_CL cl_mode, AW_CL) {
    // overwrites color settings with those from EDIT4

    int mode = (int)cl_mode;

    if (mode & 1) { // search string colors
        AW_copy_GCs(aww->get_root(), "ARB_EDIT4", "ARB_SECEDIT", false,
                    "User1",   "User2",   "Probe",
                    "Primerl", "Primerr", "Primerg",
                    "Sigl",    "Sigr",    "Sigg",
                    "MISMATCHES",
                    NULL);
    }
    if (mode & 2) { // range colors
        AW_copy_GCs(aww->get_root(), "ARB_EDIT4", "ARB_SECEDIT", false,
                    "RANGE_0", "RANGE_1", "RANGE_2",
                    "RANGE_3", "RANGE_4", "RANGE_5",
                    "RANGE_6", "RANGE_7", "RANGE_8",
                    "RANGE_9",
                    NULL);
    }
    if (mode & 4) { // other colors
        AW_copy_GCs(aww->get_root(), "ARB_EDIT4", "ARB_SECEDIT", false,
                    "CURSOR",
                    NULL);
    }
}

static AW_window *SEC_create_bonddef_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "SEC_BONDDEF", "Bond definitions");
    aws->load_xfig("sec_bonddef.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"sec_bonddef.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("label"); int x_label = aws->get_at_xposition();
    aws->at("pairs"); int x_pairs = aws->get_at_xposition();
    aws->at("chars"); int x_chars = aws->get_at_xposition();

    aws->auto_space(0, 0);

#define INSERT_PAIR_FIELDS(label, pairname)                             \
    aws->at_x(x_label);                                                 \
    aws->create_button("", label);                                      \
    aws->at_x(x_pairs);                                                 \
    aws->create_input_field(AWAR_SECEDIT_##pairname##_PAIRS, 30);       \
    aws->at_x(x_chars);                                                 \
    aws->create_input_field(AWAR_SECEDIT_##pairname##_PAIR_CHAR, 1);    \
    aws->at_newline();

    INSERT_PAIR_FIELDS("Strong pairs", STRONG);
    INSERT_PAIR_FIELDS("Normal pairs", NORMAL);
    INSERT_PAIR_FIELDS("Weak pairs", WEAK);
    INSERT_PAIR_FIELDS("No pairs", NO);
    INSERT_PAIR_FIELDS("User pairs", USER);

#undef INSERT_PAIR_FIELDS

    return aws;
}

static AW_window *SEC_create_display_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "SEC_DISPLAY_OPTS", "Display options");
    aws->load_xfig("sec_display.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"sec_display.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    // ----------------------------------------

    aws->at("bases");
    aws->label("Display bases              :");
    aws->create_inverse_toggle(AWAR_SECEDIT_HIDE_BASES);

    aws->at("strand_dist");
    aws->label("Distance between strands   :");
    aws->create_input_field(AWAR_SECEDIT_DIST_BETW_STRANDS);

    aws->at("bonds");
    aws->label("Display bonds");
    aws->create_option_menu(AWAR_SECEDIT_SHOW_BONDS);
    aws->insert_option("None",       "n", SHOW_NO_BONDS);
    aws->insert_option("Helix",      "h", SHOW_HELIX_BONDS);
    aws->insert_option("+Non-helix", "o", SHOW_NHELIX_BONDS);
    aws->update_option_menu();

    aws->at("bonddef");
    aws->callback(AW_POPUP, (AW_CL)SEC_create_bonddef_window, 0);
    aws->create_button("sec_bonddef", "Define", 0);

    aws->at("bondThickness");
    aws->label("Bond thickness             :");
    aws->create_input_field(AWAR_SECEDIT_BOND_THICKNESS);

    // ----------------------------------------

    aws->at("cursor");
    aws->label("Annotate cursor            :");
    aws->create_option_menu(AWAR_SECEDIT_SHOW_CURPOS);
    aws->insert_option("None",     "n", SHOW_NO_CURPOS);
    aws->insert_option("Absolute", "a", SHOW_ABS_CURPOS);
    aws->insert_option("Ecoli",    "e", SHOW_ECOLI_CURPOS);
    aws->insert_option("Base",     "b", SHOW_BASE_CURPOS);
    aws->update_option_menu();

    aws->at("helixNrs");
    aws->label("Annotate helices           :");
    aws->create_toggle(AWAR_SECEDIT_SHOW_HELIX_NRS);

    aws->at("ecoli");
    aws->label("Annotate ecoli positions   :");
    aws->create_toggle(AWAR_SECEDIT_SHOW_ECOLI_POS);

    aws->at("search");
    aws->label("Visualize search results   :");
    aws->create_toggle(AWAR_SECEDIT_DISPLAY_SEARCH);

    aws->at("sai");
    aws->label("Visualize SAI              :");
    aws->create_toggle(AWAR_SECEDIT_DISPLAY_SAI);

    // ----------------------------------------

    aws->at("binding");
    aws->label("Binding helix positions    :");
    aws->create_toggle(AWAR_SECEDIT_DISPPOS_BINDING);

    aws->at("ecoli2");
    aws->label("Ecoli base positions       :");
    aws->create_toggle(AWAR_SECEDIT_DISPPOS_ECOLI);

    // ----------------------------------------

    aws->at("strSkeleton");
    aws->label("Display structure skeleton :");
    aws->create_toggle(AWAR_SECEDIT_SHOW_STR_SKELETON);

    aws->at("skelThickness");
    aws->label("Skeleton thickness         :");
    aws->create_input_field(AWAR_SECEDIT_SKELETON_THICKNESS);

#ifdef DEBUG
    aws->at("show_debug");
    aws->label("Show debug info:");
    aws->create_toggle(AWAR_SECEDIT_SHOW_DEBUG);
#endif

    return aws;
}

#if defined(WARN_TODO)
#warning use popdown callback for SEC_exit and valgrind open/close/open secedit 
#endif

static void SEC_exit(GBDATA *, void *cl_sec_root) {
    SEC_root *sec_root = static_cast<SEC_root*>(cl_sec_root);
    delete sec_root;
}

AW_window *start_SECEDIT_plugin(ED4_plugin_host& host) {
    AW_root *awr     = host.get_application_root();
    GBDATA  *gb_main = host.get_database();

    SEC_graphic *gfx  = new SEC_graphic(awr, gb_main); // never freed
    SEC_root    *root = gfx->sec_root;

    AW_window_menu_modes *awm = new AW_window_menu_modes;
    awm->init(awr, "ARB_SECEDIT", "ARB_SECEDIT: Secondary structure editor", 200, 200);

    AWT_canvas *scr = new AWT_canvas(gb_main, awm, awm->get_window_id(), gfx, AWAR_SPECIES_NAME);
    root->init(gfx, scr, host);

    scr->recalc_size();
    scr->set_mode(AWT_MODE_ZOOM); // Default-Mode

    const SEC_db_interface *db = root->get_db();

    GB_atclose(gb_main, SEC_exit, root);

    awm->create_menu("File", "F", AWM_ALL);

    awm->insert_menu_topic("secedit_new", "New structure", "N", 0, AWM_ALL, SEC_new_structure, (AW_CL)db, 0);
    awm->insert_menu_topic("secedit_rename", "Rename structure", "R", 0, AWM_ALL, SEC_rename_structure, (AW_CL)db, 0);
    awm->insert_menu_topic("secedit_delete", "Delete structure", "D", 0, AWM_ALL, SEC_delete_structure, (AW_CL)db, 0);
    awm->sep______________();
    awm->insert_menu_topic("secedit_import", "Load structure", "L", "secedit_imexport.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_import, (AW_CL)db);
    awm->insert_menu_topic("secedit_export", "Save structure", "S", "secedit_imexport.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_export, (AW_CL)db);
    awm->sep______________();
    awm->insert_menu_topic("secStruct2xfig", "Export Structure to XFIG", "X", "sec_layout.hlp",  AWM_ALL, AWT_popup_sec_export_window, (AW_CL)scr, 0);
    awm->insert_menu_topic("print_secedit",  "Print Structure",          "P", "secedit2prt.hlp", AWM_ALL, AWT_popup_print_window,      (AW_CL)scr, 0);
    awm->sep______________();

    awm->insert_menu_topic("close", "Close", "C", "quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 0, 0);

    awm->create_menu("Properties", "P", AWM_ALL);
    awm->insert_menu_topic("sec_display", "Display options", "D", "sec_display.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_create_display_window, 0);
    awm->sep______________();
    awm->insert_menu_topic("props_secedit", "Change Colors and Fonts", "C", "secedit_props_data.hlp", AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)scr->gc_manager);
    awm->sep______________();
    awm->insert_menu_topic("sync_search_colors", "Sync search colors with EDIT4", "s", "sync_colors.hlp", AWM_ALL, SEC_sync_colors, (AW_CL)1, 0);
    awm->insert_menu_topic("sync_range_colors",  "Sync range colors with EDIT4",  "r", "sync_colors.hlp", AWM_ALL, SEC_sync_colors, (AW_CL)2, 0);
    awm->insert_menu_topic("sync_other_colors",  "Sync other colors with EDIT4",  "o", "sync_colors.hlp", AWM_ALL, SEC_sync_colors, (AW_CL)4, 0);
    awm->insert_menu_topic("sync_all_colors",    "Sync all colors with EDIT4",    "a", "sync_colors.hlp", AWM_ALL, SEC_sync_colors, (AW_CL)(1|2|4), 0);
    awm->sep______________();
    awm->insert_menu_topic("sec_save_props",    "How to save properties",   "p", "savedef.hlp", AWM_ALL, (AW_CB) AW_POPUP_HELP, (AW_CL)"sec_props.hlp", 0);

    awm->create_mode("pzoom.xpm",       "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_ZOOM);
    awm->create_mode("sec_modify.xpm",  "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_MOVE);
    awm->create_mode("setroot.xpm",     "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_SETROOT);
    awm->create_mode("rot.xpm",         "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_ROT);
    awm->create_mode("stretch.xpm",     "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_STRETCH);
    awm->create_mode("info.xpm",        "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_EDIT);
    awm->create_mode("sec_setcurs.xpm", "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_LINE);
    awm->create_mode("probeInfo.xpm",   "sec_mode.hlp", AWM_ALL, sec_mode_event, (AW_CL)root, (AW_CL)AWT_MODE_PROINFO);

    awm->set_info_area_height(250);
    awm->at(5, 2);
    awm->auto_space(0, -2);

    awm->button_length(0);
    awm->help_text("quit.hlp");
    awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("Close", "#quit.xpm"); // use quit button, cause users regard secedit as separate program

    awm->callback(AW_help_entry_pressed);
    awm->help_text("arb_secedit.hlp");
    awm->create_button("HELP", "#help.xpm");

    awm->callback(SEC_undo_cb, (AW_CL)db, (AW_CL)GB_UNDO_UNDO);
    awm->help_text("undo.hlp");
    awm->create_button("Undo", "#undo.xpm");

    awm->callback(SEC_undo_cb, (AW_CL)db, (AW_CL)GB_UNDO_REDO);
    awm->help_text("undo.hlp");
    awm->create_button("Redo", "#redo.xpm");

    awm->callback(SEC_toggle_cb, (AW_CL)root, 0);
    awm->help_text("sec_main.hlp");
    awm->create_button("Toggle", "Toggle");

    awm->callback(SEC_center_cb, (AW_CL)root, 0);
    awm->help_text("sec_main.hlp");
    awm->create_button("Center", "Center");

    awm->callback((AW_CB)SEC_fit_window_cb, (AW_CL)root, 0);
    awm->help_text("sec_main.hlp");
    awm->create_button("fitWindow", "Fit");

    awm->at_newline();

    {
        AW_at_maxsize maxSize; // store size (so AWAR_FOOTER does not affect min. window size)
        maxSize.store(awm->_at);
        awm->button_length(AWAR_FOOTER_MAX_LEN);
        awm->create_button(0, AWAR_FOOTER);
        awm->at_newline();
        maxSize.restore(awm->_at);
    }

    awm->set_info_area_height(awm->get_at_yposition());

    return awm;
}

