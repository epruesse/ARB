//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.cxx                                     //
//    Purpose   : General input masks                                    //
//    Time-stamp: <Fri Aug/17/2001 09:53 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <arbdb.h>
#include <awt.hxx>

#ifndef __MAP__
#include <map>
#endif
#ifndef __VECTOR__
#include <vector>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "awt_input_mask.hxx"

using namespace std;

//  ********************************************************************************
//  ********************************************************************************
//      Callbacks from database and awars

static bool in_item_changed_callback = false;
static bool in_field_changed_callback = false;
static bool in_awar_changed_callback = false;

//  -------------------------------------------------------------------------------------------------
//      static void item_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type)
//  -------------------------------------------------------------------------------------------------
static void item_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type) {
    if (!in_item_changed_callback) { // avoid deadlock
        in_item_changed_callback   = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;

        if (type&GB_CB_DELETE) { // handled child was deleted
            handler->relink();
        }
        else if ((type&(GB_CB_CHANGED|GB_CB_SON_CREATED)) == (GB_CB_CHANGED|GB_CB_SON_CREATED)) {
            // child was created (not only changed)
            handler->relink();
        }

        in_item_changed_callback = false;
    }
}

//  --------------------------------------------------------------------------------------------------
//      static void field_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type)
//  --------------------------------------------------------------------------------------------------
static void field_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type) {
    if (!in_field_changed_callback) { // avoid deadlock
        in_field_changed_callback  = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;

        if (type&GB_CB_DELETE) { // field was deleted from db -> relink this item
            handler->relink();
        }
        else if (type&GB_CB_CHANGED) {
            handler->db_changed();  // database entry was changed
        }
        in_field_changed_callback = false;
    }
}
//  ------------------------------------------------------------------------------
//      static void awar_changed_cb(AW_root *awr, AW_CL cl_awt_input_handler)
//  ------------------------------------------------------------------------------
static void awar_changed_cb(AW_root *awr, AW_CL cl_awt_input_handler) {
    if (!in_awar_changed_callback) { // avoid deadlock
        in_awar_changed_callback   = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;
        awt_assert(handler);
        if (handler) handler->awar_changed();
        in_awar_changed_callback   = false;
    }
}


//  ----------------------------------------------------
//      GB_ERROR awt_input_handler::add_callbacks()
//  ----------------------------------------------------
GB_ERROR awt_input_handler::add_db_callbacks() {
    GB_ERROR error = 0;
    if (gb_item) {
        error = GB_add_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
        if (gbd) error = GB_add_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    }
    return error;
}
//  ---------------------------------------------------
//      void awt_input_handler::remove_callbacks()
//  ---------------------------------------------------
GB_ERROR awt_input_handler::remove_db_callbacks() {
    GB_ERROR error = 0;
    if (gb_item) {
        error = GB_remove_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
        if (gbd) error = GB_remove_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    }
    return error;
}

//  ---------------------------------------------------------
//      GB_ERROR awt_input_handler::add_awar_callbacks()
//  ---------------------------------------------------------
GB_ERROR awt_input_handler::add_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->add_callback(awar_changed_cb, AW_CL(this));
    return 0;
}
//  ------------------------------------------------------------
//      GB_ERROR awt_input_handler::remove_awar_callbacks()
//  ------------------------------------------------------------
GB_ERROR awt_input_handler::remove_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->remove_callback((AW_RCB)awar_changed_cb, AW_CL(this), AW_CL(0));
    return 0;
}

int awt_input_handler::awar_counter = 0;
//  ---------------------------------------------------------------------------------------------------------------------------------------------
//      awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
//  ---------------------------------------------------------------------------------------------------------------------------------------------
awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
    : global(global_)
    , gb_item(0)
    , gbd(0)
    , label(label_)
    , child_path(child_path_)
    , id(awar_counter++)
    , db_type(type_)
    , in_destructor(false)
{
}

//  -----------------------------------------------------------------
//      GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item)
//  -----------------------------------------------------------------
GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item) {
    GB_ERROR       error = 0;
    GB_transaction dummy(mask_global()->get_gb_main());

    if (gb_item) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        error   = remove_awar_callbacks();
        gb_item = 0;
        gbd     = 0;
    }
    else {
        assert(!gbd);
    }

    if (!gb_new_item && !in_destructor) { // crashes if we are in ~awt_input_handler
        db_changed();
    }

    if (!error && gb_new_item) {
        gb_item = gb_new_item;
        gbd = GB_search(gb_item, child_path.c_str(), GB_FIND);

        db_changed();           // synchronize AWAR with DB-entry

        error             = add_db_callbacks();
        if (!error) error = add_awar_callbacks();
    }

    return error;
}

//  ------------------------------------------------
//      void awt_string_handler::awar_changed()
//  ------------------------------------------------
void awt_string_handler::awar_changed() {
    GBDATA         *gbd       = data();
    GBDATA         *gb_main   = mask_global()->get_gb_main();
    GB_transaction  dummy(gb_main);
    bool            relink_me = false;

    if (!gbd) {
        const char *child   = get_child_path().c_str();
        const char *keypath = mask_global()->get_selector()->getKeyPath();

        if (item()) {
            gbd = GB_search(item(), child, GB_FIND);

            if (!gbd) {
                GB_TYPES found_typ = awt_get_type_of_changekey(gb_main, child, keypath);
                if (found_typ != GB_NONE) set_type(found_typ); // fix type if different

                gbd = GB_search(item(), child, type()); // here new items are created
                if (found_typ == GB_NONE) awt_add_new_changekey_to_keypath(gb_main, child, type(), keypath);
                relink_me = true; //  @@@ only if child was created!!
            }
        }
        else {
            awar()->write_string(GBS_global_string("<no %s selected>", awt_itemtype_names[mask_global()->get_itemtype()]));
        }
    }

    if (gbd) {
        char     *content   = awar()->read_string();
        GB_TYPES  found_typ = GB_read_type(gbd);
        if (found_typ != type()) set_type(found_typ); // fix type if different

        GB_write_as_string(gbd, awar2db(content).c_str());
        free(content);
    }
    if (relink_me) relink();
}
//  ----------------------------------------------
//      void awt_string_handler::db_changed()
//  ----------------------------------------------
void awt_string_handler::db_changed() {
    GBDATA *gbd = data();
    if (gbd) { // gbd may be zero, if field does not exist
        GB_transaction  dummy(mask_global()->get_gb_main());
        char           *content = GB_read_as_string(gbd);
        awar()->write_string(db2awar(content).c_str());
        free(content);
    }
    else {
        awar()->write_string(default_value.c_str());
    }
}

//  -----------------------------------------------------------
//      void awt_input_field::build_widget(AW_window *aws)
//  -----------------------------------------------------------
void awt_input_field::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}
//  ---------------------------------------------------------
//      void awt_check_box::build_widget(AW_window *aws)
//  ---------------------------------------------------------
void awt_check_box::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}
//  ------------------------------------------------------------
//      void awt_radio_button::build_widget(AW_window *aws)
//  ------------------------------------------------------------
void awt_radio_button::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle_field(awar_name().c_str(), vertical ? 0 : 1);

    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    int                            pos = 0;

    for (; b != buttons.end() && v != values.end(); ++b, ++v, ++pos) {
        /*if (pos == default_position)    aws->insert_default_toggle(b->c_str(), "", v->c_str());
          else*/                            aws->insert_toggle(b->c_str(), "", b->c_str());
    }

    assert(b == buttons.end() && v == values.end());

    aws->update_toggle_field();
}
//  ------------------------------------------------------------------
//      string awt_check_box::awar2db(const string& awar_content)
//  ------------------------------------------------------------------
string awt_check_box::awar2db(const string& awar_content) {
    GB_TYPES typ = type();

    if (awar_content == "yes") {
        if (typ == GB_STRING) return "yes";
        return "1";
    }
    else {
        if (typ == GB_STRING) return "no";
        return "0";
    }
}
//  ----------------------------------------------------------------
//      string awt_check_box::db2awar(const string& db_content)
//  ----------------------------------------------------------------
string awt_check_box::db2awar(const string& db_content) {
    if (db_content == "yes" || db_content == "1") return "yes";
    if (db_content == "no" || db_content == "0") return "no";
    return atoi(db_content.c_str()) ? "yes" : "no";
}

//  ---------------------------------------------------------------------
//      string awt_radio_button::awar2db(const string& awar_content)
//  ---------------------------------------------------------------------
string awt_radio_button::awar2db(const string& awar_content) {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*b == awar_content) {
            return *v;
        }
    }

    return string("Unknown awar_content '")+awar_content+"'";
}
//  -------------------------------------------------------------------
//      string awt_radio_button::db2awar(const string& db_content)
//  -------------------------------------------------------------------
string awt_radio_button::db2awar(const string& db_content) {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*v == db_content) {
            return *b;
        }
    }
    return buttons[default_position];
}


// ********************************************************************************
// ********************************************************************************
// the input-mask-management :

typedef SmartPtr<awt_input_mask>        awt_input_mask_ptr;
typedef map<string, awt_input_mask_ptr> InputMaskList; // contains all active masks

static InputMaskList input_mask_list;

//  -------------------------------------------------------------------------
//      static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo)
//  -------------------------------------------------------------------------
static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo) {
	const int  BUFSIZE = 8000;
	char       buffer[BUFSIZE];
	char      *res     = fgets(&buffer[0], BUFSIZE-1, in);
    GB_ERROR   error   = 0;

    if (int err = ferror(in)) {
        error = strerror(err);
    }
    else if (res == 0) {
        error = "Unexpected end of file (@MASK_BEGIN or @MASK_END missing?)";
    }
    else {
        assert(res == buffer);
        res += strlen(buffer);
        if (res>buffer) {
            size_t last = res-buffer;
            if (buffer[last-1] == '\n') {
                buffer[last-1] = 0;
            }
        }
        line = buffer;
        lineNo++; // increment lineNo

        size_t last = line.find_last_not_of(" \t");
        if (last != string::npos && line[last] == '\\') {
            string next;
            error = readLine(in, next, lineNo);
            line  = line.substr(0, last)+' '+next;
        }
    }

    if (error) line = "";

    return error;
}

const char *awt_itemtype_names[AWT_IT_TYPES+1] =
{
    "Unknown",
        "Species", "Organism", "Gene", "Experiment",
        "<overflow>"
        };

//  -------------------------------------------------------------------
//      awt_item_type AWT_getItemType(const string& itemtype_name)
//  -------------------------------------------------------------------
awt_item_type AWT_getItemType(const string& itemtype_name) {
    awt_item_type type = AWT_IT_UNKNOWN;

    for (int i = AWT_IT_UNKNOWN+1; i<AWT_IT_TYPES; ++i) {
        if (itemtype_name == awt_itemtype_names[i]) {
            type = awt_item_type(i);
            break;
        }
    }

    return type;
}

//  -----------------------------------------------------------------------
//      inline size_t next_non_white(const string& line, size_t start)
//  -----------------------------------------------------------------------
inline size_t next_non_white(const string& line, size_t start) {
    if (start == string::npos) return string::npos;
    return line.find_first_not_of(" \t", start);
}

static bool was_last_parameter = false;

//  --------------------------------------------------------------------------------------------
//      inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error)
//  --------------------------------------------------------------------------------------------
inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error) {
    size_t para_sep = next_non_white(line, start);

    if (para_sep == string::npos) error = "',' or ')' expected after parameter";
    else {
        switch (line[para_sep]) {
            case ')' :
                was_last_parameter = true;
                break;
            case ',' :
                break;
            default :
                error              = "',' or ')' expected after parameter";
                break;
        }
        if (!error) para_sep++;
    }

    return para_sep;
}

//  ---------------------------------------------------------------------------------
//      static void check_last_parameter(GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------
static void check_last_parameter(GB_ERROR& error, const string& command) {
    if (!error && !was_last_parameter) {
        error = GBS_global_string("Superfluous arguments to '%s'", command.c_str());
    }
}

//  ---------------------------------------------------------------------------------------------------------------------
//      static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------------------------------------------
static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command) {
    size_t start = next_non_white(line, scan_pos);
    if (start == string::npos) {
        error = "')' expected";
    }
    else if (line[start] != ')') {
        was_last_parameter = false;
        check_last_parameter(error, command);
    }
    else { // ok
        scan_pos = start+1;
    }
}

//  ---------------------------------------------------------------------------------------------------
//      static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------
static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    string result;
    size_t open_quote = next_non_white(line, scan_pos);

    if (open_quote == string::npos || line[open_quote] != '\"') {
        error = "string parameter expected";
    }
    else {
        size_t close_quote = line.find('\"', open_quote+1);
        if (close_quote == string::npos) {
            error = "string parameter missing closing '\"'";
        }
        else {
            result   = line.substr(open_quote+1, close_quote-open_quote-1);
            scan_pos = eat_para_separator(line, close_quote+1, error);
        }
    }

    return result;
}

//  ---------------------------------------------------------------------------------------------
//      static int scan_int_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------
static int scan_int_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);

    if (start == string::npos || !isdigit(line[start])) {
        error = "digits expected";
    }
    else {
        size_t end = line.find_first_not_of("0123456789", start);
        scan_pos = eat_para_separator(line, end, error);
        if (!error) {
            assert(end != string::npos);
            result = atoi(line.substr(start, end-start+1).c_str());
        }
    }

    return result;
}

//  ---------------------------------------------------------------------------------------------------------------------------
//      static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags)
//  ---------------------------------------------------------------------------------------------------------------------------
// return 0..n-1 ( = position in 'allowed_flags')
// or error is set
static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags) {
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);

    if (start == string::npos) {
        error = GBS_global_string("One of '%s' expected", allowed_flags.c_str());
    }
    else {
        char   found       = line[start];
        char   upper_found = toupper(found);
        size_t pos         = allowed_flags.find(upper_found);

        if (pos != string::npos) {
            result   = pos;
            scan_pos = eat_para_separator(line, start+1, error);
        }
        else {
            error = GBS_global_string("One of '%s' expected (found '%c')", allowed_flags.c_str(), found);
        }
    }
    return result;
}
//  -----------------------------------------------------------------------------------------------
//      static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  -----------------------------------------------------------------------------------------------
static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    int result = scan_int_parameter(line, scan_pos, error);
    if (!error && (result != 0) && (result != 1)) {
        error = "'0' or '1' expected";
    }
    return result != 0;
}

//  --------------------------------------
//      void awt_input_mask::relink()
//  --------------------------------------
void awt_input_mask::relink() {
    const awt_item_type_selector  *sel     = global.get_selector();
    GBDATA                        *gb_item = sel->current(global.get_root());

    for (awt_input_handler_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
        (*h)->link_to(gb_item);
    }
}

//  ---------------------------------------------------------------------------------
//      static void awt_input_mask_awar_changed_cb(AW_root *root, AW_CL cl_mask)
//  ---------------------------------------------------------------------------------
static void awt_input_mask_awar_changed_cb(AW_root *root, AW_CL cl_mask) {
    awt_input_mask *mask = (awt_input_mask*)(cl_mask);
    mask->relink();
}

//  -------------------------------------------------------------------
//      static void link_mask_to_database(awt_input_mask_ptr mask)
//  -------------------------------------------------------------------
static void link_mask_to_database(awt_input_mask_ptr mask) {
    awt_input_mask_global         *global = mask->mask_global();
    const awt_item_type_selector  *sel    = global->get_selector();
    AW_root                       *root   = global->get_root();

    sel->add_awar_callbacks(root, awt_input_mask_awar_changed_cb, (AW_CL)(&*mask));
    awt_input_mask_awar_changed_cb(root, (AW_CL)(&*mask));
}

//  ------------------------------------
//      inline char *inputMaskDir()
//  ------------------------------------
inline char *inputMaskDir() {
    return AWT_unfold_path("lib/inputMasks", "ARBHOME");
}

//  -----------------------------------------------------------------
//      inline string inputMaskFullname(const string& mask_name)
//  -----------------------------------------------------------------
inline string inputMaskFullname(const string& mask_name) {
    char   *dir      = inputMaskDir();
    string  fullname = string(dir)+'/'+mask_name;
    free(dir);
    return fullname;
}

#define ARB_INPUT_MASK_ID "ARB-Input-Mask"

//  -----------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename)
//  -----------------------------------------------------------------------------------------------------------------
static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename) {
    FILE     *in     = fopen(filename.c_str(), "rt");
    size_t    lineNo = 0;
    GB_ERROR  error  = 0;

    if (in) {
        string   line;
        error = readLine(in, line, lineNo);

        if (!error && line == ARB_INPUT_MASK_ID) {
            bool   done = false;
            string title;
            string itemtype;

            while (!feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue; // ignore comments
                if (line == "@MASK_BEGIN") { done = true; break; }

                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos)  {
                    continue; // ignore all other lines
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtype = value;
                    else if (parameter == "TITLE") title  = value;
                }
            }

            if (!error && done) {
                if (itemtype == "") {
                    error = "No itemtype defined";
                }
                else {
                    if (title == "") title = mask_name;
                    return new awt_input_mask_descriptor(title.c_str(), mask_name.c_str(), itemtype.c_str());
                }
            }
        }
    }

    if (error) {
        aw_message(GBS_global_string("%s (while scanning user-mask '%s')", error, filename.c_str()));
    }

#if defined(DEBUG)
    printf("Skipping '%s' (not a input mask)\n", filename.c_str());
#endif // DEBUG
    return 0;
}

//  ---------------------------------------------------------------------
//      void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name)
//  ---------------------------------------------------------------------
void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name) {
    const string *mask_name = (const string *)cl_mask_name;
    string        fullmask  = inputMaskFullname(*mask_name);
    awt_edit(aww->get_root(), fullmask.c_str());
}

//  -----------------------------------------------------------------------
//      void AWT_reload_input_mask(AW_window *aww, AW_CL cl_mask_name)
//  -----------------------------------------------------------------------
void AWT_reload_input_mask(AW_window *aww, AW_CL cl_mask_name) {
    const string            *mask_name = (const string *)cl_mask_name;
    InputMaskList::iterator  mask_iter = input_mask_list.find(*mask_name);

    if (mask_iter != input_mask_list.end()) {
        awt_input_mask_ptr     mask   = mask_iter->second;
        awt_input_mask_global *global = mask->mask_global();

        mask->set_reload_on_reinit(true);
        mask->hide();
        AWT_initialize_input_mask(global->get_root(), global->get_gb_main(), global->get_selector(), mask_name->c_str());
    }
}
//  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const string& mask_name, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const string& mask_name, GB_ERROR& error) {
    size_t             lineNo = 0;
    awt_input_mask_ptr mask;

    error = 0;

    FILE *in = 0;
    {
        string  fullfile = inputMaskFullname(mask_name);
        in               = fopen(fullfile.c_str(), "rt");
        if (!in) error   = GBS_global_string("Can't open '%s'", fullfile.c_str());
    }

    if (!error) {
        bool   mask_began = false;
        bool   mask_ended = false;

        // data to be scanned :
        string        title;
        string        itemtypename;
        int           x_spacing   = 5;
        int           y_spacing   = 3;
        bool          edit_reload = false;
        awt_item_type itemtype;

        string line;
        error = readLine(in, line, lineNo);

        if (!error && line != ARB_INPUT_MASK_ID) {
            error = "'" ARB_INPUT_MASK_ID "' expected";
        }

        while (!error && !mask_began && !feof(in)) {
            error = readLine(in, line, lineNo);
            if (error) break;

            if (line[0] == '#') continue; // ignore comments

            if (line == "@MASK_BEGIN") mask_began = true;
            else  {
                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos) {
                    continue;
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtypename    = value;
                    else if (parameter == "TITLE") title         = value;
                    else if (parameter == "X_SPACING") x_spacing = atoi(value.c_str());
                    else if (parameter == "Y_SPACING") y_spacing = atoi(value.c_str());
                    else if (parameter == "EDIT") edit_reload    = atoi(value.c_str()) != 0;
                    else {
                        error = GBS_global_string("Unknown parameter '%s'", parameter.c_str());
                    }
                }
            }
        }

        if (!error && !mask_began) error = "@MASK_BEGIN expected";

        // check data :
        if (!error) {
            if (title == "") title = string("Untitled (")+mask_name+")";
            itemtype                              = AWT_getItemType(itemtypename);
            if (itemtype == AWT_IT_UNKNOWN) error = GBS_global_string("Unknown @ITEMTYPE '%s'", itemtypename.c_str());
        }

        // create mask
        if (!error) {
            static int awar_root_counter = 0;
            string     awar_root         = GBS_global_string("/tmp/input_mask_%i", awar_root_counter++);
            mask                         = new awt_input_mask(root, gb_main, mask_name, awar_root, itemtype, sel);
        }

        // create window
        if (!error) {
            assert(!mask.Null());
            AW_window_simple*& aws = mask->get_window();

            aws = new AW_window_simple;
#if defined(DEBUG)
            printf("new AW_window_simple: %p\n", aws);
#endif // DEBUG
            aws->init(root, "INPUT_MASK", title.c_str(), 200, 100);

//             aws->auto_space(5, 3);
            aws->auto_space(x_spacing, y_spacing);

            aws->at_newline();
            aws->callback((AW_CB0)AW_POPDOWN);
            aws->create_button("CLOSE", "CLOSE", "C");

            aws->callback( AW_POPUP_HELP,(AW_CL)"input_mask.hlp");
            aws->create_button("HELP","HELP","H");

            if (edit_reload) {
                aws->callback( AWT_edit_input_mask, (AW_CL)&mask->get_maskname());
                aws->create_button("EDIT","EDIT","E");

                aws->callback( AWT_reload_input_mask, (AW_CL)&mask->get_maskname());
                aws->create_button("RELOAD","RELOAD","R");
            }

            aws->at_newline();

            // read mask itself :
            while (!error && !mask_ended && !feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue;

                if (line == "@MASK_END") {
                    mask_ended = true;
                }
                else  {
                PARSE_REST_OF_LINE:
                    was_last_parameter = false;
                    size_t start       = next_non_white(line, 0);
                    if (start != string::npos) { // line contains sth
                        size_t after_command = line.find_first_of(string(" \t("), start);
                        if (after_command == string::npos) {
                            string command = line.substr(start);
                            error          = GBS_global_string("arguments missing after '%s'", command.c_str());
                        }
                        else {
                            string command    = line.substr(start, after_command-start);
                            size_t paren_open = line.find('(', after_command);
                            if (paren_open == string::npos) {
                                error = GBS_global_string("'(' expected after '%s'", command.c_str());
                            }
                            else {
                                size_t                scan_pos = paren_open+1;
                                awt_input_handler_ptr handler;

                                if (command == "TEXTFIELD") {
                                    string label, data_path;
                                    int    width;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) width     = scan_int_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_input_field(mask->mask_global(), data_path, label, width);
                                }
                                else if (command == "CHECKBOX") {
                                    string label, data_path;
                                    bool   checked        = false;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) checked   = scan_bool_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_check_box(mask->mask_global(), data_path, label, checked);
                                }
                                else if (command == "RADIO") {
                                    string         label, data_path;
                                    int            default_position, orientation;
                                    vector<string> buttons;
                                    vector<string> values;

                                    label                        = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path        = scan_string_parameter(line, scan_pos, error);
                                    if (!error) default_position = scan_int_parameter(line, scan_pos, error);
                                    if (!error) {
                                        orientation = scan_flag_parameter(line, scan_pos, error, "HVXY");
                                        orientation = orientation&1;
                                    }
                                    while (!error && !was_last_parameter) {
                                        string but = scan_string_parameter(line, scan_pos, error);
                                        string val = scan_string_parameter(line, scan_pos, error);
                                        buttons.push_back(but);
                                        values.push_back(val);
                                    }
                                    check_last_parameter(error, command);

                                    if (!error && (buttons.size()<2)) error = "You have to define at least 2 buttons.";
                                    if (!error && (default_position<1 || size_t(default_position)>buttons.size())) {
                                        error = GBS_global_string("Invalid default %i (valid: 1..%i)", default_position, buttons.size());
                                    }

                                    if (!error) {
                                        handler = new awt_radio_button(mask->mask_global(), data_path, label, default_position-1, orientation, buttons, values);
                                    }
                                }
                                else if (command == "TEXT") {
                                    string text;
                                    text = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) aws->create_button(0, text.c_str());
                                }
                                else if (command == "SELF") {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) aws->create_button(0, mask->mask_global()->get_selector()->get_self_awar());
                                }
                                else {
                                    error = GBS_global_string("Unknown command '%s'", command.c_str());
                                }

                                if (!handler.Null() && !error) {
                                    mask->add_handler(handler);
                                    handler->build_widget(aws);
                                }

                                line = line.substr(scan_pos);
                                if (!error) goto PARSE_REST_OF_LINE;
                            }
                        }
                    }
                    else {
                        // printf("got newline\n");
                        aws->at_newline();
                    }
                }
            }

            aws->window_fit();
            // link to database
            link_mask_to_database(mask);
        }

        if (error) {
            error = GBS_global_string("Wrong format (%s) in line #%u", error, lineNo);
        }

        fclose(in);
    }

    return mask;
}

//  ---------------------------------------------------------------------------------------------------------------------------------
//      void AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name)
//  ---------------------------------------------------------------------------------------------------------------------------------
void AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name) {
    InputMaskList::iterator mask_iter = input_mask_list.find(mask_name);
    GB_ERROR                error     = 0;

    // erase mask (so it loads again from scratch)
    if (mask_iter != input_mask_list.end() &&
        mask_iter->second->reload_on_reinit()) // reload wanted ?
    {
        awt_input_mask_ptr old_mask = mask_iter->second;
        input_mask_list.erase(mask_iter);
        mask_iter                   = input_mask_list.end();

        old_mask->hide();
        static list<awt_input_mask_ptr> mask_collector;
        mask_collector.push_back(old_mask);
    }

    if (mask_iter == input_mask_list.end()) { // mask not loaded yet
        awt_input_mask_ptr newMask = awt_create_input_mask(root, gb_main, sel, mask_name, error);
        if (error) {
            error = GBS_global_string("Error creating input-mask from %s (%s)",
                                      mask_name, error);
        }
        else {
            input_mask_list[mask_name] = newMask;
            mask_iter                  = input_mask_list.find(mask_name);
        }
    }

    if (!error) {
        AW_window_simple *aws  = mask_iter->second->get_window();
        if (aws->get_show()   == AW_FALSE) aws->show();
    }

    if (error) aw_message(error);
}

//  -----------------------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_)
//  -----------------------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_) {
    title        = strdup(title_);
    maskname     = strdup(maskname_);
    itemtypename = strdup(itemtypename_);
}
//  ----------------------------------------------------------------
//      awt_input_mask_descriptor::~awt_input_mask_descriptor()
//  ----------------------------------------------------------------
awt_input_mask_descriptor::~awt_input_mask_descriptor() {
    free(itemtypename);
    free(maskname);
    free(title);
}
//  -----------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other)
//  -----------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other) {
    title        = strdup(other.title);
    maskname     = strdup(other.maskname);
    itemtypename = strdup(other.itemtypename);
}
//  ------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other)
//  ------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other) {
    if (this != &other) {
        free(itemtypename);
        free(maskname);
        free(title);

        title        = strdup(other.title);
        maskname     = strdup(other.maskname);
        itemtypename = strdup(other.itemtypename);
    }

    return *this;
}

static int scanned_existing_input_masks = false;
static vector<awt_input_mask_descriptor> existing_masks;

//  ------------------------------------------------
//      static void scan_existing_input_masks()
//  ------------------------------------------------
static void scan_existing_input_masks() {
    assert(!scanned_existing_input_masks);

    char *dirname = inputMaskDir();
    DIR  *dirp    = opendir(dirname);

    if (!dirp) {
        fprintf(stderr, "The path '%s' is invalid.\n", dirname);
    }
    else {
        struct dirent *dp;
        for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
            struct stat st;
            string      maskname = dp->d_name;
            string      fullname = inputMaskFullname(maskname);

            if (stat(fullname.c_str(), &st)) continue;
            if (!S_ISREG(st.st_mode)) continue;
            // now we have a regular file

            if (maskname.find(".mask") != string::npos) {
                awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname);
                if (descriptor) { // we found a input mask file
                    existing_masks.push_back(*descriptor);
                    delete descriptor;
                }
            }
        }
    }

    free(dirname);
    scanned_existing_input_masks = true;
}

//  ---------------------------------------------------------------------
//      const awt_input_mask_descriptor *AWT_look_input_mask(int id)
//  ---------------------------------------------------------------------
const awt_input_mask_descriptor *AWT_look_input_mask(int id) {
// #if defined(DEBUG)
//     printf("AWT_look_input_mask id=%i\n", id);
// #endif // DEBUG
    if (!scanned_existing_input_masks) scan_existing_input_masks();

    if (id<0 || size_t(id) >= existing_masks.size()) return 0;

    const awt_input_mask_descriptor *descriptor = &existing_masks[id];
// #if defined(DEBUG)
//     printf("returns descriptor=%p\n", descriptor);
// #endif // DEBUG
    return descriptor;
}

