//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.cxx                                     //
//    Purpose   : General input masks                                    //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "awt_input_mask_internal.hxx"

#include <arbdbt.h>
#include <ad_cb.h>
#include <arb_file.h>
#include <awt_www.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_question.hxx>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <climits>
#include <set>

using namespace std;

static const char *awt_itemtype_names[AWT_IT_TYPES+1] =
{
 "Unknown",
 "Species", "Organism", "Gene", "Experiment",
 "<overflow>"
};

#define SEC_XBORDER    3        // space left/right of NEW_SECTION line
#define SEC_YBORDER    4        // space above/below
#define SEC_LINE_WIDTH 1        // line width

#define MIN_TEXTFIELD_SIZE 1
#define MAX_TEXTFIELD_SIZE 1000

#define AWT_SCOPE_LOCAL  0
#define AWT_SCOPE_GLOBAL 1

awt_input_mask_id_list awt_input_mask_global::global_ids; // stores global ids

// ---------------------
//      global awars

#define AWAR_INPUT_MASK_BASE   "tmp/inputMask"
#define AWAR_INPUT_MASK_NAME   AWAR_INPUT_MASK_BASE"/name"
#define AWAR_INPUT_MASK_ITEM   AWAR_INPUT_MASK_BASE"/item"
#define AWAR_INPUT_MASK_SCOPE  AWAR_INPUT_MASK_BASE"/scope"
#define AWAR_INPUT_MASK_HIDDEN AWAR_INPUT_MASK_BASE"/hidden"

#define AWAR_INPUT_MASKS_EDIT_ENABLED AWAR_INPUT_MASK_BASE"/edit_enabled"

static bool global_awars_created = false;

static void create_global_awars(AW_root *awr) {
    awt_assert(!global_awars_created);
    awr->awar_string(AWAR_INPUT_MASK_NAME, "new");
    awr->awar_string(AWAR_INPUT_MASK_ITEM, awt_itemtype_names[AWT_IT_SPECIES]);
    awr->awar_int(AWAR_INPUT_MASK_SCOPE, 0);
    awr->awar_int(AWAR_INPUT_MASK_HIDDEN, 0);
    awr->awar_int(AWAR_INPUT_MASKS_EDIT_ENABLED, 0);
    global_awars_created = true;
}

// ------------------------------------------
//      Callbacks from database and awars

static bool in_item_changed_callback  = false;
static bool in_field_changed_callback = false;
static bool in_awar_changed_callback  = false;

static void item_changed_cb(GBDATA*, awt_linked_to_item *item_link, GB_CB_TYPE type) {
    if (!in_item_changed_callback) { // avoid deadlock
        LocallyModify<bool> flag(in_item_changed_callback, true);
        if (type&GB_CB_DELETE) { // handled child was deleted
            item_link->relink();
        }
        else if ((type&GB_CB_CHANGED_OR_SON_CREATED) == GB_CB_CHANGED_OR_SON_CREATED) {
            // child was created (not only changed)
            item_link->relink();
        }
        else if (type&GB_CB_CHANGED) { // only changed
            item_link->general_item_change();
        }
    }
}

static void field_changed_cb(GBDATA*, awt_input_handler *handler, GB_CB_TYPE type) {
    if (!in_field_changed_callback) { // avoid deadlock
        LocallyModify<bool> flag(in_field_changed_callback, true);
        if (type&GB_CB_DELETE) { // field was deleted from db -> relink this item
            handler->relink();
        }
        else if (type&GB_CB_CHANGED) {
            handler->db_changed();  // database entry was changed
        }
    }
}

static void awar_changed_cb(AW_root*, awt_mask_awar_item *item) {
    if (!in_awar_changed_callback) { // avoid deadlock
        LocallyModify<bool> flag(in_awar_changed_callback, true);
        awt_assert(item);
        if (item) item->awar_changed();
    }
}

string awt_input_mask_global::generate_id(const string& mask_name_)
{
    string result;
    result.reserve(mask_name_.length());
    for (string::const_iterator p = mask_name_.begin(); p != mask_name_.end(); ++p) {
        if (isalnum(*p)) result.append(1, *p);
        else result.append(1, '_');
    }
    return result;
}

bool awt_input_mask_global::edit_allowed() const {
    return !test_edit_enabled ||
        get_root()->awar(AWAR_INPUT_MASKS_EDIT_ENABLED)->read_int() == 1;
}

void awt_input_mask_global::no_item_selected() const {
    aw_message(GBS_global_string("This had no effect, because no %s is selected",
                                 awt_itemtype_names[get_itemtype()]));
}

GB_ERROR awt_input_mask_id_list::add(const string& name, awt_mask_item *item) {
    awt_mask_item *existing = lookup(name);
    if (existing) return GBS_global_string("ID '%s' already exists", name.c_str());

    id[name] = item;
    return 0;
}
GB_ERROR awt_input_mask_id_list::remove(const string& name) {
    if (!lookup(name)) return GBS_global_string("ID '%s' does not exist", name.c_str());
    id.erase(name);
    return 0;
}

awt_mask_item::awt_mask_item(awt_input_mask_global& global_)
    : global(global_)
{
}

awt_mask_item::~awt_mask_item() {
    awt_assert(!has_name()); // you forgot to call remove_name()
}

GB_ERROR awt_mask_item::set_name(const string& name_, bool is_global)
{
    GB_ERROR error = 0;
    if (has_name()) {
        error = GBS_global_string("Element already has name (%s)", get_name().c_str());
    }
    else {
        name = new string(name_);
        if (is_global) {
            if (!mask_global().has_global_id(*name)) { // do not add if variable already defined elsewhere
                error = mask_global().add_global_id(*name, this);
            }
        }
        else {
            error = mask_global().add_local_id(*name, this);
        }
    }
    return error;
}

GB_ERROR awt_mask_item::remove_name()
{
    GB_ERROR error = 0;
    if (has_name()) {
        error = mask_global().remove_id(*name);
        name.SetNull();
    }
    return error;
}

awt_mask_awar_item::awt_mask_awar_item(awt_input_mask_global& global_, const string& awar_base, const string& default_value, bool saved_with_properties)
    : awt_mask_item(global_)
{
    const char *root_name;

    if (saved_with_properties) root_name = "/input_masks";
    else root_name = "/tmp/input_masks"; // awars starting with /tmp are not saved

    awarName = GBS_global_string("%s/%s", root_name, awar_base.c_str());
#if defined(DEBUG)
    printf("awarName='%s'\n", awarName.c_str());
#endif // DEBUG
    mask_global().get_root()->awar_string(awarName.c_str(), default_value.c_str()); // create the awar
    add_awarItem_callbacks();
}

void awt_mask_awar_item::add_awarItem_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->add_callback(makeRootCallback(awar_changed_cb, this));
}
void awt_mask_awar_item::remove_awarItem_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->remove_callback(makeRootCallback(awar_changed_cb, this));
}

awt_variable::awt_variable(awt_input_mask_global& global_, const string& id, bool is_global_, const string& default_value, GB_ERROR& error)
    : awt_mask_awar_item(global_, generate_baseName(global_, id, is_global_), default_value, true)
    , is_global(is_global_)
{
    error = set_name(id, is_global);
}

awt_variable::~awt_variable()
{
}

string awt_script::get_value() const
{
    string                        result;
    AW_root                      *root     = mask_global().get_root();
    const awt_item_type_selector *selector = mask_global().get_selector();
    GBDATA                       *gb_main  = mask_global().get_gb_main();
    GBDATA                       *gbd      = selector->current(root, gb_main);

    if (gbd) {
        char           *species_name    = root->awar(selector->get_self_awar())->read_string();
        GB_transaction  tscope(gb_main);

        char *val = GB_command_interpreter(gb_main, species_name, script.c_str(), gbd, 0);
        if (!val) {
            aw_message(GB_await_error());
            result = "<error>";
        }
        else {
            result = val;
            free(val);
        }
        free(species_name);
    }
    else {
        result = "<undefined>";
    }


    return result;
}

GB_ERROR awt_script::set_value(const string& /* new_value */)
{
    return GBS_global_string("You cannot assign a value to script '%s'", has_name() ? get_name().c_str() : "<unnamed>");
}

GB_ERROR awt_linked_to_item::add_db_callbacks()
{
    GB_ERROR error = 0;
    if (gb_item) error = GB_add_callback(gb_item, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(item_changed_cb, this));
    return error;
}

void awt_linked_to_item::remove_db_callbacks() {
    if (!GB_inside_callback(gb_item, GB_CB_DELETE)) {
        GB_remove_callback(gb_item, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(item_changed_cb, this));
    }
}

awt_script_viewport::awt_script_viewport(awt_input_mask_global& global_, const awt_script *script_, const string& label_, long field_width_)
    : awt_viewport(global_, generate_baseName(global_), "", false, label_)
    , script(script_)
    , field_width(field_width_)
{
}

awt_script_viewport::~awt_script_viewport()
{
    unlink();
}

GB_ERROR awt_script_viewport::link_to(GBDATA *gb_new_item)
{
    GB_ERROR       error = 0;
    GB_transaction ta(mask_global().get_gb_main());

    remove_awarItem_callbacks();    // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
    }

    if (gb_new_item) {
        set_item(gb_new_item);
        db_changed();
        error = add_db_callbacks();
    }

    add_awarItem_callbacks();       // rebind awar callbacks

    return error;
}

void awt_script_viewport::build_widget(AW_window *aws)
{
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

void awt_script_viewport::db_changed() {
    awt_assert(script);
    string   current_value = script->get_value();
    GB_ERROR error         = awt_mask_awar_item::set_value(current_value);

    if (error) aw_message(error);
}

void awt_script_viewport::awar_changed() {
    aw_message("It makes no sense to change the result of a script");
}

GB_ERROR awt_input_handler::add_db_callbacks() {
    GB_ERROR error = awt_linked_to_item::add_db_callbacks();
    if (item() && gbd) error = GB_add_callback(gbd, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(field_changed_cb, this));
    return error;
}
void awt_input_handler::remove_db_callbacks() {
    awt_linked_to_item::remove_db_callbacks();
    if (item() && gbd && !GB_inside_callback(gbd, GB_CB_DELETE)) {
        GB_remove_callback(gbd, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(field_changed_cb, this));
    }
}

awt_input_handler::awt_input_handler(awt_input_mask_global& global_, const string& child_path_, GB_TYPES type_, const string& label_)
    : awt_viewport(global_, generate_baseName(global_, child_path_), "", false, label_)
    , gbd(0)
    , child_path(child_path_)
    , db_type(type_)
    , in_destructor(false)
{
}

awt_input_handler::~awt_input_handler() {
    in_destructor = true;
    unlink();
}

GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item) {
    GB_ERROR       error = 0;
    GB_transaction ta(mask_global().get_gb_main());

    remove_awarItem_callbacks(); // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks();  // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
        gbd = 0;
    }
    else {
        awt_assert(!gbd);
    }

    if (!gb_new_item && !in_destructor) { // crashes if we are in ~awt_input_handler
        db_changed();
    }

    if (!error && gb_new_item) {
        set_item(gb_new_item);
        gbd = GB_search(item(), child_path.c_str(), GB_FIND);

        db_changed();           // synchronize AWAR with DB-entry

        error = add_db_callbacks();
    }

    add_awarItem_callbacks(); // rebind awar callbacks

    return error;
}

void awt_string_handler::awar_changed() {
    GBDATA   *gbdata    = data();
    GBDATA   *gb_main   = mask_global().get_gb_main();
    bool      relink_me = false;
    GB_ERROR  error     = 0;

    GB_push_transaction(gb_main);

    if (!mask_global().edit_allowed()) error = "Editing is disabled. Check the 'Enable edit' switch!";

    if (!error && !gbdata) {
        const char *child   = get_child_path().c_str();
        const char *keypath = mask_global().get_selector()->getKeyPath();

        if (item()) {
            gbdata = GB_search(item(), child, GB_FIND);

            if (!gbdata) {
                GB_TYPES found_typ = GBT_get_type_of_changekey(gb_main, child, keypath);
                if (found_typ != GB_NONE) set_type(found_typ); // fix type if different

                gbdata = GB_search(item(), child, type()); // here new items are created
                if (found_typ == GB_NONE) GBT_add_new_changekey_to_keypath(gb_main, child, type(), keypath);
                relink_me = true; //  @@@ only if child was created!!
            }
        }
        else {
            mask_global().no_item_selected();
            aw_message(GBS_global_string("This had no effect, because no %s is selected",
                                         awt_itemtype_names[mask_global().get_itemtype()]));
        }
    }

    if (!error && gbdata) {
        char     *content   = awar()->read_string();
        GB_TYPES  found_typ = GB_read_type(gbdata);
        if (found_typ != type()) set_type(found_typ); // fix type if different

        error = GB_write_autoconv_string(gbdata, awar2db(content).c_str());

        free(content);
    }

    if (error) {
        aw_message(error);
        GB_abort_transaction(gb_main);
        db_changed(); // write back old value
    }
    else {
        GB_pop_transaction(gb_main);
    }

    if (relink_me) relink();
}

void awt_string_handler::db_changed() {
    GBDATA *gbdata = data();
    if (gbdata) { // gbdata may be zero, if field does not exist
        GB_transaction  ta(mask_global().get_gb_main());
        char           *content = GB_read_as_string(gbdata);
        awar()->write_string(db2awar(content).c_str());
        free(content);
    }
    else {
        awar()->write_string(default_value.c_str());
    }
}

// ----------------
//      Widgets

void awt_input_field::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

void awt_text_viewport::build_widget(AW_window *aws)
{
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}

void awt_check_box::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}
void awt_radio_button::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle_field(awar_name().c_str(), vertical ? 0 : 1);

    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    int                            pos = 0;

    for (; b != buttons.end() && v != values.end(); ++b, ++v, ++pos) {
        void (AW_window::*ins_togg)(const char*, const char*, const char*);

        if (pos == default_position) ins_togg = &AW_window::insert_default_toggle;
        else ins_togg                         = &AW_window::insert_toggle;

        (aws->*ins_togg)(b->c_str(), mask_global().hotkey(*b), b->c_str());
    }

    awt_assert(b == buttons.end() && v == values.end());

    aws->update_toggle_field();
}

// -----------------------------------------
//      Special AWAR <-> DB translations

string awt_check_box::awar2db(const string& awar_content) const {
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
string awt_check_box::db2awar(const string& db_content) const {
    if (db_content == "yes" || db_content == "1") return "yes";
    if (db_content == "no" || db_content == "0") return "no";
    return atoi(db_content.c_str()) ? "yes" : "no";
}

string awt_radio_button::awar2db(const string& awar_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*b == awar_content) {
            return *v;
        }
    }

    return string("Unknown awar_content '")+awar_content+"'";
}

string awt_radio_button::db2awar(const string& db_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*v == db_content) {
            return *b;
        }
    }
    return buttons[default_position];
}

string awt_numeric_input_field::awar2db(const string& awar_content) const {
    long val = strtol(awar_content.c_str(), 0, 10);

    // correct numeric value :
    if (val<min) val = min;
    if (val>max) val = max;

    return GBS_global_string("%li", val);
}

// -----------------------------------------
//      Routines to parse user-mask file

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
        awt_assert(res == buffer);
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

inline size_t next_non_white(const string& line, size_t start) {
    if (start == string::npos) return string::npos;
    return line.find_first_not_of(" \t", start);
}

static bool was_last_parameter = false;

inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error) {
    size_t para_sep = next_non_white(line, start);

    if (para_sep == string::npos) {
        error = "',' or ')' expected after parameter";
    }
    else {
        switch (line[para_sep]) {
            case ')':
                was_last_parameter = true;
                break;

            case ',':
                break;

            default:
                error = "',' or ')' expected after parameter";
                break;
        }
        if (!error) para_sep++;
    }

    return para_sep;
}

static void check_last_parameter(GB_ERROR& error, const string& command) {
    if (!error && !was_last_parameter) {
        error = GBS_global_string("Superfluous arguments to '%s'", command.c_str());
    }
}

static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command) {
    size_t start = next_non_white(line, scan_pos);
    scan_pos     = start;

    if (start == string::npos) {
        error = "')' expected";
    }
    else if (line[start] != ')') {
        was_last_parameter = false;
        check_last_parameter(error, command);
    }
    else { // ok
        scan_pos++;
    }
}

inline GB_ERROR decode_escapes(string& s) {
    string::iterator f = s.begin();
    string::iterator t = s.begin();

    for (; f != s.end(); ++f, ++t) {
        if (*f == '\\') {
            ++f;
            if (f == s.end()) return GBS_global_string("Trailing \\ in '%s'", s.c_str());
        }
        *t = *f;
    }

    s.erase(t, f);

    return 0;
}

static string check_data_path(const string& path, GB_ERROR& error) {
    if (!error) error = GB_check_hkey(path.c_str());
    return path;
}

static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, bool allow_escaped = false) {
    string result;
    size_t open_quote = next_non_white(line, scan_pos);
    scan_pos          = open_quote;

    if (open_quote == string::npos || line[open_quote] != '\"') {
        error = "string parameter expected";
    }
    else {
        size_t close_quote = string::npos;

        if (allow_escaped) {
            size_t start = open_quote+1;

            while (1) {
                close_quote = line.find_first_of("\\\"", start);

                if (close_quote == string::npos) break; // error
                if (line[close_quote] == '\"') break; // found closing quote

                if (line[close_quote] == '\\') { // escape next char
                    close_quote++;
                }
                start = close_quote+1;
            }
        }
        else {
            close_quote = line.find('\"', open_quote+1);
        }

        if (close_quote == string::npos) {
            error = "string parameter missing closing '\"'";
        }
        else {
            result = line.substr(open_quote+1, close_quote-open_quote-1);
            if (allow_escaped) {
                awt_assert(!error);
                error = decode_escapes(result);
            }
            if (!error) scan_pos = eat_para_separator(line, close_quote+1, error);
        }
    }

    return result;
}

static string list_keywords(const char **allowed_keywords) {
    string result;
    for (int i = 0; allowed_keywords[i]; ++i) {
        if (i) {
            if (allowed_keywords[i+1]) result += ", ";
            else result                       += " or ";
        }
        result += allowed_keywords[i];
    }
    return result;
}

inline int isKeyword(const char *current, const char *keyword) {
    int pos = 0;
    for (; keyword[pos]; ++pos) {
        if (!current[pos] || std::tolower(current[pos]) != std::tolower(keyword[pos])) {
            return 0;
        }
    }
    return pos;
}

static int scan_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const char **allowed_keywords) {
    // return index of keyword (or -1)
    // allowed_keywords has to be 0-terminated
    int    result = -1;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    awt_assert(!error);

    if (start == string::npos) {
EXPECTED :
        string keywords = list_keywords(allowed_keywords);
        error           = GBS_global_string("%s expected", keywords.c_str());
    }
    else {
        const char *current = line.c_str()+start;

        int i = 0;
        for (; allowed_keywords[i]; ++i) {
            int found_keyword_size = isKeyword(current, allowed_keywords[i]);
            if (found_keyword_size) {
                result    = i;
                scan_pos += found_keyword_size;
                break;
            }
        }
        if (!allowed_keywords[i]) goto EXPECTED;
        awt_assert(!error);
        scan_pos = eat_para_separator(line, scan_pos, error);
    }
    return result;
}

static void scan_string_or_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error,
                                             string& string_para, int& keyword_index, // result parameters
                                             const char **allowed_keywords) {
    // if keyword_index != -1 -> keyword found
    //                  == -1 -> string_para contains string-parameter

    awt_assert(!error);

    string_para = scan_string_parameter(line, scan_pos, error);
    if (!error) {
        keyword_index = -1;
    }
    else {                      // no string
        error         = 0;      // ignore error - test for keywords
        keyword_index = scan_keyword_parameter(line, scan_pos, error, allowed_keywords);
        if (keyword_index == -1) { // no keyword
            error = GBS_global_string("string parameter or %s", error);
        }
    }
}

static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    bool   neg    = false;

    awt_assert(!error);

    while (start != string::npos) {
        char c = line[start];
        if (c != '+' && c != '-') break;

        start             = next_non_white(line, start+1);
        if (c == '-') neg = !neg;
        continue;
    }

    if (start == string::npos || !isdigit(line[start])) {
        scan_pos = start;
        error    = "digits (or+-) expected";
    }
    else {
        size_t end = line.find_first_not_of("0123456789", start);
        scan_pos   = eat_para_separator(line, end, error);
        if (!error) {
            awt_assert(end != string::npos);
            result = atoi(line.substr(start, end-start+1).c_str());
        }
    }

    return neg ? -result : result;
}
static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long min, long max) {
    // with range-check
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        if (result<min || result>max) {
            scan_pos = old_scan_pos;
            error    = GBS_global_string("value %li is outside allowed range (%li-%li)", result, min, max);
        }
    }
    return result;
}

static long scan_optional_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long if_empty) {
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (error) {
        error    = 0;           // ignore and test for empty parameter
        scan_pos = old_scan_pos;
        scan_pos = eat_para_separator(line, scan_pos, error);

        if (error) {
            error    = "expected number or empty parameter";
            scan_pos = old_scan_pos;
        }
        else {
            result = if_empty;
        }
    }
    return result;
}

static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags) {
    // return 0..n-1 ( = position in 'allowed_flags')
    // or error is set
    awt_assert(!error);
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    if (start == string::npos) {
        error    = GBS_global_string("One of '%s' expected", allowed_flags.c_str());
    }
    else {
        char   found       = line[start];
        char   upper_found = std::toupper(found);
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
static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    awt_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);

    if (!error && (result != 0) && (result != 1)) {
        scan_pos = old_scan_pos;
        error    = "'0' or '1' expected";
    }
    return result != 0;
}

static string scan_identifier(const string& line, size_t& scan_pos, GB_ERROR& error) {
    string id;
    size_t start = next_non_white(line, scan_pos);
    if (start == string::npos) {
        error = "identifier expected";
    }
    else {
        size_t end = start;
        while (end<line.length()) {
            char c                 = line[end];
            if (!(isalnum(c) || c == '_')) break;
            ++end;
        }
        id         = line.substr(start, end-start);
        scan_pos   = eat_para_separator(line, end, error);
    }
    return id;
}

inline const char *inputMaskDir(bool local) {
    if (local) {
        static char *local_mask_dir;
        if (!local_mask_dir) local_mask_dir = strdup(GB_path_in_arbprop("inputMasks"));
        return local_mask_dir;
    }

    static char *global_mask_dir;
    if (!global_mask_dir) global_mask_dir = strdup(GB_path_in_ARBLIB("inputMasks"));
    return global_mask_dir;
}

inline string inputMaskFullname(const string& mask_name, bool local) {
    const char *dir = inputMaskDir(local);
    return string(dir)+'/'+mask_name;
}

#define ARB_INPUT_MASK_ID "ARB-Input-Mask"

static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename, bool local) {
    awt_input_mask_descriptor *res = 0;
    FILE     *in    = fopen(filename.c_str(), "rt");
    GB_ERROR  error = 0;

    if (in) {
        string   line;
        size_t   lineNo = 0;
        error = readLine(in, line, lineNo);

        if (!error && line == ARB_INPUT_MASK_ID) {
            bool   done   = false;
            int    hidden = 0; // 0 = 'not hidden'
            string title;
            string itemtype;

            while (!feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue; // ignore comments
                if (line == "@MASK_BEGIN") { done = true; break; }

                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos) {
                    continue; // ignore all other lines
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtype = value;
                    else if (parameter == "TITLE") title  = value;
                    else if (parameter == "HIDE") hidden  = atoi(value.c_str());
                }
            }

            if (!error && done) {
                if (itemtype == "") {
                    error = "No itemtype defined";
                }
                else {
                    if (title == "") title = mask_name;
                    res = new awt_input_mask_descriptor(title.c_str(), mask_name.c_str(), itemtype.c_str(), local, hidden);
                }
            }
        }
        fclose(in);
    }

    if (error) {
        aw_message(GBS_global_string("%s (while scanning user-mask '%s')", error, filename.c_str()));
    }

#if defined(DEBUG)
    printf("Skipping '%s' (not a input mask)\n", filename.c_str());
#endif // DEBUG
    return res;
}

static void AWT_edit_input_mask(AW_window *, const string *mask_name, bool local) {
    string fullmask = inputMaskFullname(*mask_name, local);
    AW_edit(fullmask.c_str()); // @@@ add callback and automatically reload input mask
}

//  ---------------------------------
//      input mask container :

typedef SmartPtr<awt_input_mask>        awt_input_mask_ptr;
typedef map<string, awt_input_mask_ptr> InputMaskList; // contains all active masks
static InputMaskList                    input_mask_list;

static void awt_input_mask_awar_changed_cb(AW_root*, awt_input_mask *mask) {
    mask->relink();
}
static void link_mask_to_database(awt_input_mask_ptr mask) {
    awt_input_mask_global&        global = mask->mask_global();
    const awt_item_type_selector *sel    = global.get_selector();
    AW_root                      *root   = global.get_root();

    sel->add_awar_callbacks(root, makeRootCallback(awt_input_mask_awar_changed_cb, &*mask));
    awt_input_mask_awar_changed_cb(root, &*mask);
}
static void unlink_mask_from_database(awt_input_mask_ptr mask) {
    awt_input_mask_global&        global = mask->mask_global();
    const awt_item_type_selector *sel    = global.get_selector();
    AW_root                      *root   = global.get_root();

    sel->remove_awar_callbacks(root, makeRootCallback(awt_input_mask_awar_changed_cb, &*mask));
}

inline bool isInternalMaskName(const string& s) {
    return s[0] == '0' || s[0] == '1';
}

static void awt_open_input_mask(AW_window *aww, const string *internal_mask_name, const string *mask_to_open, bool reload, bool hide_current) {
    InputMaskList::iterator mask_iter = input_mask_list.find(*internal_mask_name);

    awt_assert(internal_mask_name && isInternalMaskName(*internal_mask_name));
    awt_assert(mask_to_open && isInternalMaskName(*mask_to_open));

    if (mask_iter != input_mask_list.end()) {
        awt_input_mask_ptr     mask   = mask_iter->second;
        awt_input_mask_global& global = mask->mask_global();

        printf("aww=%p root=%p ; global=%p root=%p\n", aww, aww->get_root(), &global, global.get_root());
        awt_assert(aww->get_root() == global.get_root());

        if (reload) mask->set_reload_on_reinit(true);
        if (hide_current) mask->hide();
        // @@@ hier sollte nicht der Selector der alten Maske verwendet werden, sondern anhand des Typs ein
        // Selector ausgewaehlt werden. Dazu muessen jedoch alle Selectoren registriert werden.
        GB_ERROR error = AWT_initialize_input_mask(global.get_root(), global.get_gb_main(), global.get_selector(), mask_to_open->c_str(), global.is_local_mask());
        // CAUTION: AWT_initialize_input_mask invalidates mask and mask_iter
        if (error && hide_current) {
            mask_iter = input_mask_list.find(*internal_mask_name);
            awt_assert(mask_iter != input_mask_list.end());
            mask_iter->second->show();
        }
    }
#if defined(DEBUG)
    else {
        string mask_name = internal_mask_name->substr(1);
        printf("'%s' (no such mask in input_mask_list)\n", mask_name.c_str());
        awt_assert(0);
    }
#endif // DEBUG
}

static void AWT_reload_input_mask(AW_window *aww, const string *internal_mask_name) {
    awt_open_input_mask(aww, internal_mask_name, internal_mask_name, true, true);
}
static void AWT_open_input_mask(AW_window *aww, const string *internal_mask_name, const string *mask_to_open) {
    awt_open_input_mask(aww, internal_mask_name, mask_to_open, false, false);
}
static void AWT_change_input_mask(AW_window *aww, const string *internal_mask_name, const string *mask_to_open) {
    awt_open_input_mask(aww, internal_mask_name, mask_to_open, false, true);
}

//  ------------------------------
//      class awt_mask_action

class awt_mask_action {
    // something that is performed i.e. when user pressed a mask button
    // used as callback parameter
private:
    virtual GB_ERROR action() = 0;
protected:
    awt_input_mask_ptr mask;
public:
    awt_mask_action(awt_input_mask_ptr mask_) : mask(mask_) {}
    virtual ~awt_mask_action() {}

    void perform_action() {
        GB_ERROR error = action();
        if (error) aw_message(error);
    }
};


//  ------------------------------------------------------
//      class awt_assignment : public awt_mask_action

class awt_assignment : public awt_mask_action {
private:
    string id_dest;
    string id_source;

    virtual GB_ERROR action() OVERRIDE {
        GB_ERROR             error       = 0;
        const awt_mask_item *item_source = mask->mask_global().get_identified_item(id_source, error);
        awt_mask_item       *item_dest   = mask->mask_global().get_identified_item(id_dest, error);

        if (!error) error = item_dest->set_value(item_source->get_value());

        return error;
    }
public:
    awt_assignment(awt_input_mask_ptr mask_, const string& id_dest_, const string& id_source_)
        : awt_mask_action(mask_)
        , id_dest(id_dest_)
        , id_source(id_source_)
    {}
    virtual ~awt_assignment() OVERRIDE {}
};

static void AWT_input_mask_perform_action(AW_window*, awt_mask_action *action) {
    action->perform_action();
}

static void AWT_input_mask_browse_url(AW_window *aww, const string *url_srt, const awt_input_mask *mask) {
    const awt_input_mask_global&  global   = mask->mask_global();
    const awt_item_type_selector *selector = global.get_selector();

    AW_root *root    = aww->get_root();
    GBDATA  *gb_main = global.get_gb_main();
    GBDATA  *gbd     = selector->current(root, gb_main);

    if (!gbd) {
        aw_message(GBS_global_string("You have to select a %s first", awt_itemtype_names[selector->get_item_type()]));
    }
    else {
        char     *name  = root->awar(selector->get_self_awar())->read_string();
        GB_ERROR  error = awt_open_ACISRT_URL_by_gbd(root, gb_main, gbd, name, url_srt->c_str());
        if (error) aw_message(error);
        free(name);
    }
}


// ---------------------------
//      User Mask Commands

enum MaskCommand {
    CMD_TEXTFIELD,
    CMD_NUMFIELD,
    CMD_CHECKBOX,
    CMD_RADIO,
    CMD_OPENMASK,
    CMD_CHANGEMASK,
    CMD_TEXT,
    CMD_SELF,
    CMD_WWW,
    CMD_NEW_LINE,
    CMD_NEW_SECTION,
    CMD_ID,
    CMD_GLOBAL,
    CMD_LOCAL,
    CMD_SHOW,
    CMD_ASSIGN,
    CMD_SCRIPT,

    MASK_COMMANDS,
    CMD_UNKNOWN = MASK_COMMANDS
};

struct MaskCommandDefinition {
    const char  *cmd_name;
    MaskCommand  cmd;
};

static struct MaskCommandDefinition mask_command[MASK_COMMANDS+1] =
{
 { "TEXTFIELD", CMD_TEXTFIELD },
 { "NUMFIELD", CMD_NUMFIELD },
 { "CHECKBOX", CMD_CHECKBOX },
 { "RADIO", CMD_RADIO },
 { "OPENMASK", CMD_OPENMASK },
 { "CHANGEMASK", CMD_CHANGEMASK },
 { "TEXT", CMD_TEXT },
 { "SELF", CMD_SELF },
 { "NEW_LINE", CMD_NEW_LINE },
 { "NEW_SECTION", CMD_NEW_SECTION },
 { "WWW", CMD_WWW },
 { "ID", CMD_ID },
 { "GLOBAL", CMD_GLOBAL },
 { "LOCAL", CMD_LOCAL },
 { "SHOW", CMD_SHOW },
 { "ASSIGN", CMD_ASSIGN },
 { "SCRIPT", CMD_SCRIPT },

 { 0, CMD_UNKNOWN }
};

inline MaskCommand findCommand(const string& cmd_name) {
    int mc = 0;

    for (; mask_command[mc].cmd != CMD_UNKNOWN; ++mc) {
        if (mask_command[mc].cmd_name == cmd_name) {
            return mask_command[mc].cmd;
        }
    }
    return CMD_UNKNOWN;
}

static void parse_CMD_RADIO(string& line, size_t& scan_pos, GB_ERROR& error, const string& command,
                            awt_mask_item_ptr& handler1, awt_mask_item_ptr& handler2, awt_input_mask_global& global)
{
    string         label, data_path;
    int            default_position = -1, orientation = -1;
    vector<string> buttons;
    vector<string> values;
    bool           allow_edit       = false;
    int            width            = -1;
    int            edit_position    = -1;

    label                        = scan_string_parameter(line, scan_pos, error);
    if (!error) data_path        = check_data_path(scan_string_parameter(line, scan_pos, error), error);
    if (!error) default_position = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        orientation = scan_flag_parameter(line, scan_pos, error, "HVXY");
        orientation = orientation&1;
    }
    while (!error && !was_last_parameter) {
        string but = scan_string_parameter(line, scan_pos, error);
        string val = "";
        if (!error) {
            int keyword_index;
            const char *allowed_keywords[] = { "ALLOW_EDIT", 0 };
            scan_string_or_keyword_parameter(line, scan_pos, error, val, keyword_index, allowed_keywords);

            if (!error) {
                if (keyword_index != -1) { // found keyword
                    if (allow_edit) error = "ALLOW_EDIT is allowed only once for each RADIO";

                    if (!error) width = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                    if (!error) val   = scan_string_parameter(line, scan_pos, error);

                    if (!error) {
                        allow_edit    = true;
                        edit_position = buttons.size()+1; // positions are 1..n
                        buttons.push_back(but);
                        values.push_back(val);
                    }
                }
                else { // found string
                    buttons.push_back(but);
                    values.push_back(val);
                }
            }
        }
    }
    check_last_parameter(error, command);

    if (!error && (buttons.size()<2)) error = "You have to define at least 2 buttons.";

    if (!error && allow_edit && edit_position != default_position) {
        error = GBS_global_string("Invalid default %i (must be index of ALLOW_EDIT: %i )", default_position, edit_position);
    }
    if (!error && (default_position<1 || size_t(default_position)>buttons.size())) {
        error = GBS_global_string("Invalid default %i (valid: 1..%zu)", default_position, buttons.size());
    }

    if (!error) {
        if (allow_edit) {
            // create radio-button + textfield
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
            handler2 = new awt_input_field(global, data_path, "", width, "", GB_STRING);
        }
        else {
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
        }
    }
}

static string find_internal_name(const string& mask_name, bool search_in_local) {
    const char *internal_local  = 0;
    const char *internal_global = 0;

    for (int id = 0; ; ++id) {
        const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
        if (!descriptor) break;

        const char *internal_name = descriptor->get_internal_maskname();

        if (strcmp(internal_name+1, mask_name.c_str()) == 0) {
            if (descriptor->is_local_mask()) {
                awt_assert(internal_local == 0);
                internal_local = internal_name;
            }
            else {
                awt_assert(internal_global == 0);
                internal_global = internal_name;
            }
        }
    }

    return (search_in_local && internal_local) ? internal_local : (internal_global ? internal_global : "");
}

// ----------------------------------
//      class awt_marked_checkbox

class awt_marked_checkbox : public awt_viewport, public awt_linked_to_item {
private:

    string generate_baseName(awt_input_mask_global& global_) {
        return GBS_global_string("%s/marked", global_.get_maskid().c_str());
    }

public:
    awt_marked_checkbox(awt_input_mask_global& global_, const std::string& label_)
        : awt_viewport(global_, generate_baseName(global_), "0", false, label_)
        , awt_linked_to_item()
    {}
    virtual ~awt_marked_checkbox() OVERRIDE {}

    virtual GB_ERROR link_to(GBDATA *gb_new_item) OVERRIDE; // link to a new item
    virtual GB_ERROR relink() OVERRIDE { return link_to(mask_global().get_selected_item()); }
    virtual void awar_changed() OVERRIDE;
    virtual void db_changed() OVERRIDE;
    virtual void general_item_change() OVERRIDE { db_changed(); } // called if item was changed (somehow)
    virtual void build_widget(AW_window *aws) OVERRIDE; // builds the widget at the current position
};

GB_ERROR awt_marked_checkbox::link_to(GBDATA *gb_new_item) { // link to a new item
    GB_ERROR       error = 0;
    GB_transaction ta(mask_global().get_gb_main());

    remove_awarItem_callbacks();    // unbind awar callbacks temporarily

    if (item()) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        set_item(0);
    }

    if (gb_new_item) {
        set_item(gb_new_item);
        db_changed();
        error = add_db_callbacks();
    }

    add_awarItem_callbacks();       // rebind awar callbacks
    return error;
}

void awt_marked_checkbox::awar_changed() { // called when awar changes
    if (item()) {
        string         value  = get_value();
        bool           marked = value == "yes";
        GB_transaction ta(mask_global().get_gb_main());
        GB_write_flag(item(), marked);
    }
    else {
        mask_global().no_item_selected();
    }
}

void awt_marked_checkbox::db_changed() {
    if (item()) {
        GB_transaction ta(mask_global().get_gb_main());
        set_value(GB_read_flag(item()) ? "yes" : "no");
    }
}

void awt_marked_checkbox::build_widget(AW_window *aws) { // builds the widget at the current position
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}

static GB_ERROR writeDefaultMaskfile(const string& fullname, const string& maskname, const string& itemtypename, bool hidden) {
    FILE *out = fopen(fullname.c_str(), "wt");
    if (!out) return GBS_global_string("Can't open '%s'", fullname.c_str());

    fprintf(out,
            "%s\n"
            "# New mask '%s'\n\n"
            "# What kind of item to edit:\n"
            "@ITEMTYPE=%s\n\n"
            "# Window title:\n"
            "@TITLE=%s\n\n", ARB_INPUT_MASK_ID, maskname.c_str(), itemtypename.c_str(), maskname.c_str());

    fprintf(out,
            "# Should mask appear in 'User mask' menu\n"
            "@HIDE=%i\n\n", int(hidden));

    fputs("# Spacing in window:\n"
          "@X_SPACING=5\n"
          "@Y_SPACING=3\n\n"
          "# Show edit/reload button?\n"
          "@EDIT=1\n"
          "# Show 'edit enable' toggle?\n"
          "@EDIT_ENABLE=1\n"
          "# Show 'marked' toggle?\n"
          "@SHOW_MARKED=1\n"
          "\n# ---------------------------\n"
          "# The definition of the mask:\n\n"
          "@MASK_BEGIN\n\n"
          "    TEXT(\"You are editing\") SELF()\n"
          "    NEW_SECTION()\n"
          "    TEXTFIELD(\"Full name\", \"full_name\", 30)\n\n"
          "@MASK_END\n\n", out);

    fclose(out);
    return 0;
}

class ID_checker {
    bool        reloading;
    set<string> seen;
    set<string> dup;
    string      curr_id;

    bool is_known(const string& id) { return seen.find(id) != seen.end(); }

    string makeUnique(string id) {
        if (is_known(id)) {
            dup.insert(id);
            for (int i = 0; ; ++i) {
                string undup = GBS_global_string("%s%i", id.c_str(), i);
                if (!is_known(undup)) {
                    id = undup;
                    break;
                }
            }
        }
        seen.insert(id);
        return id;
    }

public:
    ID_checker(bool reloading_)
        : reloading(reloading_)
    {}

    const char *fromKey(const char *id) {
        curr_id = makeUnique(id);
        return reloading ? NULL : curr_id.c_str();
    }
    const char *fromText(const string& anystr) {
        SmartCharPtr key = GBS_string_2_key(anystr.c_str());
        return fromKey(&*key);
    }

    bool seenDups() const { return !dup.empty(); }
    const char *get_dup_error(const string& maskName) const {
        string dupList;
        for (set<string>::iterator d = dup.begin(); d != dup.end(); ++d) {
            dupList = dupList+" '"+*d+"'";
        }
        return GBS_global_string("Warning: duplicated IDs seen in '%s':\n"
                                 "%s\n"
                                 "(they need to be unique; change button texts etc. to change them)",
                                 maskName.c_str(), dupList.c_str());
    }
};

static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel,
                                                const string& mask_name, bool local, GB_ERROR& error, bool reloading) {
    awt_input_mask_ptr mask;

    error = 0;

    FILE *in = 0;
    {
        string  fullfile = inputMaskFullname(mask_name, local);
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
        bool          edit_enable = true;
        bool          show_marked = true;

        string line;
        size_t lineNo  = 0;
        size_t err_pos = 0;     // 0 = unknown; string::npos = at end of line; else position+1;

        error = readLine(in, line, lineNo);

        if (!error && line != ARB_INPUT_MASK_ID) {
            error = "'" ARB_INPUT_MASK_ID "' expected";
        }

        while (!error && !mask_began && !feof(in)) {
            error = readLine(in, line, lineNo);
            if (error) break;

            if (line[0] == '#') continue; // ignore comments

            if (line == "@MASK_BEGIN") mask_began = true;
            else {
                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos) {
                    continue;
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE")            itemtypename = value;
                    else if (parameter == "TITLE")          title        = value;
                    else if (parameter == "X_SPACING")      x_spacing    = atoi(value.c_str());
                    else if (parameter == "Y_SPACING")      y_spacing    = atoi(value.c_str());
                    else if (parameter == "EDIT")           edit_reload  = atoi(value.c_str()) != 0;
                    else if (parameter == "EDIT_ENABLE")    edit_enable  = atoi(value.c_str()) != 0;
                    else if (parameter == "SHOW_MARKED")    show_marked  = atoi(value.c_str()) != 0;
                    else if (parameter == "HIDE") ; // ignore parameter here
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
            awt_item_type itemtype = AWT_getItemType(itemtypename);

            if (itemtype == AWT_IT_UNKNOWN)         error = GBS_global_string("Unknown @ITEMTYPE '%s'", itemtypename.c_str());
            if (itemtype != sel->get_item_type())   error = GBS_global_string("Mask is designed for @ITEMTYPE '%s' (current @ITEMTYPE '%s')",
                                                                              itemtypename.c_str(), awt_itemtype_names[sel->get_item_type()]);

            // create mask
            if (!error) mask = new awt_input_mask(root, gb_main, mask_name, itemtype, local, sel, edit_enable);
        }

        // create window
        if (!error) {
            awt_assert(!mask.isNull());
            AW_window_simple*& aws = mask->get_window();
            aws                    = new AW_window_simple;

            ID_checker ID(reloading);

            {
                char *window_id = GBS_global_string_copy("INPUT_MASK_%s", mask->mask_global().get_maskid().c_str()); // create a unique id for each mask
                aws->init(root, window_id, title.c_str());
                free(window_id);
            }

            aws->load_xfig(0, true);
            aws->recalc_size_atShow(AW_RESIZE_DEFAULT); // ignore user size!

            aws->auto_space(x_spacing, y_spacing);
            aws->at_newline();

            aws->callback(AW_POPDOWN);                          aws->create_button(ID.fromKey("CLOSE"), "CLOSE", "C");
            aws->callback(makeHelpCallback("input_mask.hlp"));  aws->create_button(ID.fromKey("HELP"),  "HELP",  "H");

            if (edit_reload) {
                aws->callback(makeWindowCallback(AWT_edit_input_mask, &mask->mask_global().get_maskname(), mask->mask_global().is_local_mask()));
                aws->create_button(0, "!EDIT", "E");

                aws->callback(makeWindowCallback(AWT_reload_input_mask, &mask->mask_global().get_internal_maskname()));
                aws->create_button(0, "RELOAD", "R");
            }

            if (edit_reload && edit_enable && show_marked) aws->at_newline();

            if (edit_enable) {
                aws->label("Enable edit?");
                aws->create_toggle(AWAR_INPUT_MASKS_EDIT_ENABLED);
            }

            if (show_marked) {
                awt_mask_item_ptr handler = new awt_marked_checkbox(mask->mask_global(), "Marked");
                mask->add_handler(handler);
                if (handler->is_viewport()) handler->to_viewport()->build_widget(aws);

            }

            aws->at_newline();

            vector<int>         horizontal_lines; // y-positions of horizontal lines
            map<string, size_t> referenced_ids; // all ids that where referenced by the script (size_t contains lineNo of last reference)
            map<string, size_t> declared_ids; // all ids that where declared by the script (size_t contains lineNo of declaration)

            awt_mask_item_ptr lasthandler;

            // read mask itself :
            while (!error && !mask_ended && !feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line.empty()) continue;
                if (line[0] == '#') continue;

                if (line == "@MASK_END") {
                    mask_ended = true;
                }
                else {
                PARSE_REST_OF_LINE :
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
                                size_t            scan_pos = paren_open+1;
                                awt_mask_item_ptr handler;
                                awt_mask_item_ptr radio_edit_handler;
                                MaskCommand       cmd      = findCommand(command);

                                //  --------------------------------------
                                //      code for different commands :

                                if (cmd == CMD_TEXTFIELD) {
                                    string label, data_path;
                                    long   width          = -1;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = check_data_path(scan_string_parameter(line, scan_pos, error), error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_input_field(mask->mask_global(), data_path, label, width, "", GB_STRING);
                                }
                                else if (cmd == CMD_NUMFIELD) {
                                    string label, data_path;
                                    long   width = -1;

                                    long min = LONG_MIN;
                                    long max = LONG_MAX;

                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = check_data_path(scan_string_parameter(line, scan_pos, error), error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    if (!was_last_parameter) {
                                        if (!error) min = scan_optional_parameter(line, scan_pos, error, LONG_MIN);
                                        if (!error) max = scan_optional_parameter(line, scan_pos, error, LONG_MAX);
                                    }
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_numeric_input_field(mask->mask_global(), data_path, label, width, 0, min, max);
                                }
                                else if (cmd == CMD_CHECKBOX) {
                                    string label, data_path;
                                    bool   checked        = false;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = check_data_path(scan_string_parameter(line, scan_pos, error), error);
                                    if (!error) checked   = scan_bool_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_check_box(mask->mask_global(), data_path, label, checked);
                                }
                                else if (cmd == CMD_RADIO) {
                                    parse_CMD_RADIO(line, scan_pos, error, command, handler, radio_edit_handler, mask->mask_global());
                                }
                                else if (cmd == CMD_SCRIPT) {
                                    string id, script;
                                    id                 = scan_identifier(line, scan_pos, error);
                                    if (!error) script = scan_string_parameter(line, scan_pos, error, true);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        handler          = new awt_script(mask->mask_global(), script);
                                        error            = handler->set_name(id, false);
                                        declared_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_GLOBAL || cmd == CMD_LOCAL) {
                                    string id, def_value;

                                    id                 = scan_identifier(line, scan_pos, error);
                                    bool global_exists = mask->mask_global().has_global_id(id);
                                    bool local_exists  = mask->mask_global().has_local_id(id);

                                    if ((cmd == CMD_GLOBAL && local_exists) || (cmd == CMD_LOCAL && global_exists)) {
                                        error = GBS_global_string("ID '%s' already declared as %s ID (rename your local id)",
                                                                  id.c_str(), cmd == CMD_LOCAL ? "global" : "local");
                                    }
                                    else if (cmd == CMD_LOCAL && local_exists) {
                                        error = GBS_global_string("ID '%s' declared twice", id.c_str());
                                    }

                                    if (!error) def_value = scan_string_parameter(line, scan_pos, error);
                                    if (!error) {
                                        if (cmd == CMD_GLOBAL) {
                                            if (!mask->mask_global().has_global_id(id)) { // do not create globals more than once
                                                // and never free them -> so we don't need pointer
                                                new awt_variable(mask->mask_global(), id, true, def_value, error);
                                            }
                                            awt_assert(handler.isNull());
                                        }
                                        else {
                                            handler = new awt_variable(mask->mask_global(), id, false, def_value, error);
                                        }
                                        declared_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_ID) {
                                    string id = scan_identifier(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        if (lasthandler.isNull()) {
                                            error = "ID() may only be used BEHIND another element";
                                        }
                                        else {
                                            error            = lasthandler->set_name(id, false);
                                            declared_ids[id] = lineNo;
                                        }
                                    }
                                }
                                else if (cmd == CMD_SHOW) {
                                    string         label, id;
                                    long           width = -1;
                                    awt_mask_item *item  = 0;

                                    label             = scan_string_parameter(line, scan_pos, error);
                                    if (!error) id    = scan_identifier(line, scan_pos, error);
                                    if (!error) item  = (awt_mask_item*)mask->mask_global().get_identified_item(id, error);
                                    if (!error) width = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        awt_mask_awar_item *awar_item = dynamic_cast<awt_mask_awar_item*>(item);

                                        if (awar_item) { // item has an awar -> create a viewport using that awar
                                            handler = new awt_text_viewport(awar_item, label, width);
                                        }
                                        else { // item has no awar -> test if it's a script
                                            awt_script *script  = dynamic_cast<awt_script*>(item);
                                            if (script) handler = new awt_script_viewport(mask->mask_global(), script, label, width);
                                            else        error   = "SHOW cannot be used on this ID-type";
                                        }

                                        referenced_ids[id] = lineNo;
                                    }
                                }
                                else if (cmd == CMD_OPENMASK || cmd == CMD_CHANGEMASK) {
                                    string label, mask_to_start;
                                    label                     = scan_string_parameter(line, scan_pos, error);
                                    if (!error) mask_to_start = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        string mask_to_start_internal = find_internal_name(mask_to_start, local);

                                        if (mask_to_start_internal.length() == 0) {
                                            error = "Can't detect which mask to load";
                                        }
                                        else {
                                            const char *key = ID.fromText(label);

                                            string *const internal_mask_name = new string(mask->mask_global().get_internal_maskname());
                                            string *const mask_to_open       = new string(mask_to_start_internal);

                                            awt_assert(internal_mask_name);
                                            awt_assert(mask_to_open);

                                            aws->callback(makeWindowCallback(cmd == CMD_OPENMASK ? AWT_open_input_mask : AWT_change_input_mask, internal_mask_name, mask_to_open));

                                            aws->button_length(label.length()+2);
                                            aws->create_button(key, label.c_str());
                                        }
                                    }
                                }
                                else if (cmd == CMD_WWW) {
                                    string button_text, url_srt;
                                    button_text         = scan_string_parameter(line, scan_pos, error);
                                    if (!error) url_srt = scan_string_parameter(line, scan_pos, error, true);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        const char *key = ID.fromText(button_text);
                                        aws->callback(makeWindowCallback(AWT_input_mask_browse_url, new string(url_srt), &*mask));
                                        aws->button_length(button_text.length()+2);
                                        aws->create_button(key, button_text.c_str());
                                    }
                                }
                                else if (cmd == CMD_ASSIGN) {
                                    string id_dest, id_source, button_text;

                                    id_dest                 = scan_identifier(line, scan_pos, error);
                                    if (!error) id_source   = scan_identifier(line, scan_pos, error);
                                    if (!error) button_text = scan_string_parameter(line, scan_pos, error);

                                    if (!error) {
                                        referenced_ids[id_source] = lineNo;
                                        referenced_ids[id_dest]   = lineNo;

                                        const char *key = ID.fromText(button_text);
                                        aws->callback(makeWindowCallback(AWT_input_mask_perform_action, static_cast<awt_mask_action*>(new awt_assignment(mask, id_dest, id_source))));
                                        aws->button_length(button_text.length()+2);
                                        aws->create_button(key, button_text.c_str());
                                    }
                                }
                                else if (cmd == CMD_TEXT) {
                                    string text;
                                    text = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        aws->button_length(text.length()+2);
                                        aws->create_button(NULL, text.c_str());
                                    }
                                }
                                else if (cmd == CMD_SELF) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        const awt_item_type_selector *selector = mask->mask_global().get_selector();
                                        aws->button_length(selector->get_self_awar_content_length());
                                        aws->create_button(NULL, selector->get_self_awar());
                                    }
                                }
                                else if (cmd == CMD_NEW_LINE) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_NEW_SECTION) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        horizontal_lines.push_back(height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_UNKNOWN) {
                                    error = GBS_global_string("Unknown command '%s'", command.c_str());
                                }
                                else {
                                    error = GBS_global_string("No handler for '%s'", command.c_str());
                                    awt_assert(0);
                                }

                                //  --------------------------
                                //      insert handler(s)

                                if (!handler.isNull() && !error) {
                                    if (!radio_edit_handler.isNull()) { // special radio handler
                                        const awt_radio_button *radio = dynamic_cast<const awt_radio_button*>(&*handler);
                                        awt_assert(radio);

                                        int x_start, y_start;

                                        aws->get_at_position(&x_start, &y_start);

                                        mask->add_handler(handler);
                                        handler->to_viewport()->build_widget(aws);

                                        int x_end, y_end, dummy;
                                        aws->get_at_position(&x_end, &dummy);
                                        aws->at_newline();
                                        aws->get_at_position(&dummy, &y_end);

                                        int height    = y_end-y_start;
                                        int each_line = height/radio->no_of_toggles();
                                        int y_offset  = each_line*(radio->default_toggle());

                                        aws->at(x_end+x_spacing, y_start+y_offset);

                                        mask->add_handler(radio_edit_handler);
                                        radio_edit_handler->to_viewport()->build_widget(aws);

                                        radio_edit_handler.SetNull();
                                    }
                                    else {
                                        mask->add_handler(handler);
                                        if (handler->is_viewport()) handler->to_viewport()->build_widget(aws);
                                    }
                                    lasthandler = handler; // store handler (used by CMD_ID)
                                }

                                // parse rest of line or abort
                                if (!error) {
                                    line = line.substr(scan_pos);
                                    goto PARSE_REST_OF_LINE;
                                }
                                err_pos = scan_pos;
                                if (err_pos != string::npos) err_pos++; // because 0 means unknown
                            }
                        }
                    }
                    else { // reached the end of the current line
                        aws->at_newline();
                    }
                }
            }

            // check declarations and references
            if (!error) {
                for (map<string, size_t>::const_iterator r = referenced_ids.begin(); r != referenced_ids.end(); ++r) {
                    if (declared_ids.find(r->first) == declared_ids.end()) {
                        error = GBS_global_string("ID '%s' used in line #%zu was not declared", r->first.c_str(), r->second);
                        aw_message(error);
                    }
                }

                for (map<string, size_t>::const_iterator d = declared_ids.begin(); d != declared_ids.end(); ++d) {
                    if (referenced_ids.find(d->first) == referenced_ids.end()) {
                        const char *warning = GBS_global_string("ID '%s' declared in line #%zu is never used in %s",
                                                                d->first.c_str(), d->second, mask_name.c_str());
                        aw_message(warning);
                    }
                }

                if (error) error = "Inconsistent IDs";
            }

            if (!error) {
                if (!horizontal_lines.empty()) { // draw all horizontal lines
                    int width, height;
                    aws->get_window_size(width, height);
                    for (vector<int>::const_iterator yi = horizontal_lines.begin(); yi != horizontal_lines.end(); ++yi) {
                        int y = (*yi)+SEC_YBORDER;
                        aws->draw_line(SEC_XBORDER, y, width-SEC_XBORDER, y, SEC_LINE_WIDTH, true);
                    }
                }

                // fit the window
                aws->window_fit();
            }

            if (!error) link_mask_to_database(mask);
            if (ID.seenDups()) aw_message(ID.get_dup_error(mask_name));
        }

        if (error) {
            if (lineNo == 0) {
                ; // do not change error
            }
            else if (err_pos == 0) { // don't knows exact error position
                error = GBS_global_string("%s in line #%zu", error, lineNo);
            }
            else if (err_pos == string::npos) {
                error = GBS_global_string("%s at end of line #%zu", error, lineNo);
            }
            else {
                int    wanted         = 35;
                size_t end            = line.length();
                string context;
                context.reserve(wanted);
                bool   last_was_space = false;

                for (size_t ex = err_pos-1; ex<end && wanted>0; ++ex) {
                    char ch            = line[ex];
                    bool this_is_space = ch == ' ';

                    if (!this_is_space || !last_was_space) {
                        context.append(1, ch);
                        --wanted;
                    }
                    last_was_space = this_is_space;
                }

                error = GBS_global_string("%s in line #%zu at '%s...'", error, lineNo, context.c_str());
            }
        }

        fclose(in);
    }

    if (error) mask.SetNull();

    return mask;
}

GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* internal_mask_name, bool local) {
    const char              *mask_name  = internal_mask_name+1;
    InputMaskList::iterator  mask_iter  = input_mask_list.find(internal_mask_name);
    GB_ERROR                 error      = 0;
    awt_input_mask_ptr       old_mask;
    bool                     unlink_old = false;

    if (mask_iter != input_mask_list.end() && mask_iter->second->reload_on_reinit()) { // reload wanted ?
        // erase mask (so it loads again from scratch)
        old_mask = mask_iter->second;
        input_mask_list.erase(mask_iter);
        mask_iter = input_mask_list.end();

        old_mask->hide();
        unlink_old = true;
    }

    if (mask_iter == input_mask_list.end()) { // mask not loaded yet
        awt_input_mask_ptr newMask = awt_create_input_mask(root, gb_main, sel, mask_name, local, error, unlink_old);
        if (error) {
            error = GBS_global_string("Error reading %s (%s)", mask_name, error);
            if (!old_mask.isNull()) { // are we doing a reload or changemask ?
                input_mask_list[internal_mask_name] = old_mask; // error loading modified mask -> put old one back to mask-list
                unlink_old                          = false;
            }
        }
        else {                                      // new mask has been generated
            input_mask_list[internal_mask_name] = newMask;
        }
        mask_iter = input_mask_list.find(internal_mask_name);
    }

    if (!error) {
        awt_assert(mask_iter != input_mask_list.end());
        mask_iter->second->get_window()->activate();
    }

    if (unlink_old) {
        old_mask->unlink(); // unlink old mask from database ()
        unlink_mask_from_database(old_mask);
    }

    if (error) aw_message(error);
    return error;
}

// start of implementation of class awt_input_mask:

awt_input_mask::~awt_input_mask() {
    unlink();
    for (awt_mask_item_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
        (*h)->remove_name();
    }
}

void awt_input_mask::link_to(GBDATA *gb_item) {
    // this functions links/unlinks all registered item handlers to/from the database
    for (awt_mask_item_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
        if ((*h)->is_linked_item()) (*h)->to_linked_item()->link_to(gb_item);
    }
}

// -end- of implementation of class awt_input_mask.


awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_, bool local, bool hidden_) {
    title = strdup(title_);
    internal_maskname    = (char*)malloc(strlen(maskname_)+2);
    internal_maskname[0] = local ? '0' : '1';
    strcpy(internal_maskname+1, maskname_);
    itemtypename         = strdup(itemtypename_);
    local_mask           = local;
    hidden               = hidden_;
}
awt_input_mask_descriptor::~awt_input_mask_descriptor() {
    free(itemtypename);
    free(internal_maskname);
    free(title);
}

awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other) {
    title             = strdup(other.title);
    internal_maskname = strdup(other.internal_maskname);
    itemtypename      = strdup(other.itemtypename);
    local_mask        = other.local_mask;
    hidden            = other.hidden;
}
awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other) {
    if (this != &other) {
        free(itemtypename);
        free(internal_maskname);
        free(title);

        title             = strdup(other.title);
        internal_maskname = strdup(other.internal_maskname);
        itemtypename      = strdup(other.itemtypename);
        local_mask        = other.local_mask;
        hidden            = other.hidden;
    }

    return *this;
}

static bool scanned_existing_input_masks = false;
static vector<awt_input_mask_descriptor> existing_masks;

static void add_new_input_mask(const string& maskname, const string& fullname, bool local) {
    awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname, local);
    if (descriptor) {
        existing_masks.push_back(*descriptor);
        delete descriptor;
    }
}
static void scan_existing_input_masks() {
    awt_assert(!scanned_existing_input_masks);

    for (int scope = 0; scope <= 1; ++scope) {
        const char *dirname = inputMaskDir(scope == AWT_SCOPE_LOCAL);

        if (!GB_is_directory(dirname)) {
            if (scope == AWT_SCOPE_LOCAL) {         // in local scope
                GB_ERROR warning = GB_create_directory(dirname); // try to create directory
                if (warning) GB_warning(warning);
            }
        }

        DIR *dirp = opendir(dirname);
        if (!dirp) {
            fprintf(stderr, "Warning: No such directory '%s'\n", dirname);
        }
        else {
            struct dirent *dp;
            for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
                struct stat st;
                string      maskname = dp->d_name;
                string      fullname = inputMaskFullname(maskname, scope == AWT_SCOPE_LOCAL);

                if (stat(fullname.c_str(), &st)) continue;
                if (!S_ISREG(st.st_mode)) continue;
                // now we have a regular file

                size_t ext_pos = maskname.find(".mask");

                if (ext_pos != string::npos && maskname.substr(ext_pos) == ".mask") {
                    awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname, scope == AWT_SCOPE_LOCAL);
                    if (descriptor) { // we found a input mask file
                        existing_masks.push_back(*descriptor);
                        delete descriptor;
                    }
                }
            }
            closedir(dirp);
        }
    }
    scanned_existing_input_masks = true;
}

const awt_input_mask_descriptor *AWT_look_input_mask(int id) {
    if (!scanned_existing_input_masks) scan_existing_input_masks();

    if (id<0 || size_t(id) >= existing_masks.size()) return 0;

    const awt_input_mask_descriptor *descriptor = &existing_masks[id];
    return descriptor;
}

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

// -----------------------------
//      Registered Itemtypes

class AWT_registered_itemtype {
    // stores information about so-far-used item types
    RefPtr<AW_window_menu_modes> awm;               // the main window responsible for opening windows
    AWT_OpenMaskWindowCallback   open_window_cb;    // callback to open the window

public:
    AWT_registered_itemtype() :
        awm(0),
        open_window_cb(0)
    {}
    AWT_registered_itemtype(AW_window_menu_modes *awm_, AWT_OpenMaskWindowCallback open_window_cb_) :
        awm(awm_),
        open_window_cb(open_window_cb_)
    {}

    AW_window_menu_modes *getWindow() const { return awm; }
    AWT_OpenMaskWindowCallback getOpenCb() const { return open_window_cb; }
};

typedef map<awt_item_type, AWT_registered_itemtype> TypeRegistry;
typedef TypeRegistry::const_iterator                TypeRegistryIter;

static TypeRegistry registeredTypes;

static GB_ERROR openMaskWindowByType(int mask_id, awt_item_type type) {
    TypeRegistryIter registered = registeredTypes.find(type);
    GB_ERROR         error      = 0;

    if (registered == registeredTypes.end()) error = GBS_global_string("Type '%s' not registered (yet)", awt_itemtype_names[type]);
    else registered->second.getOpenCb()(registered->second.getWindow(), mask_id, NULL);

    return error;
}

static void registerType(awt_item_type type, AW_window_menu_modes *awm, AWT_OpenMaskWindowCallback open_window_cb) {
    TypeRegistryIter alreadyRegistered = registeredTypes.find(type);
    if (alreadyRegistered == registeredTypes.end()) {
        registeredTypes[type] = AWT_registered_itemtype(awm, open_window_cb);
    }
#if defined(DEBUG)
    else {
        awt_assert(alreadyRegistered->second.getOpenCb() == open_window_cb);
    }
#endif // DEBUG
}

// ----------------------------------------------
//      Create a new input mask (interactive)

static void create_new_mask_cb(AW_window *aww) {
    AW_root *awr = aww->get_root();

    string maskname = awr->awar(AWAR_INPUT_MASK_NAME)->read_char_pntr();
    {
        size_t ext = maskname.find(".mask");

        if (ext == string::npos) maskname = maskname+".mask";
        else maskname                     = maskname.substr(0, ext)+".mask";

        awr->awar(AWAR_INPUT_MASK_NAME)->write_string(maskname.c_str());
    }


    string itemname     = awr->awar(AWAR_INPUT_MASK_ITEM)->read_char_pntr();
    int    scope        = awr->awar(AWAR_INPUT_MASK_SCOPE)->read_int();
    int    hidden       = awr->awar(AWAR_INPUT_MASK_HIDDEN)->read_int();
    bool   local        = scope == AWT_SCOPE_LOCAL;
    string maskfullname = inputMaskFullname(maskname, local);
    bool   openMask     = false;

    const char  *error = 0;
    struct stat  st;

    if (stat(maskfullname.c_str(), &st) == 0) { // file exists
        int answer = aw_question("overwrite_mask", "File does already exist", "Overwrite mask,Cancel");
        switch (answer) {
            case 0:
                openMask   = true;
                break;
            case 1: break;
            default: awt_assert(0); break;
        }
    }
    else {                      // file does not exist
        error = GB_create_directory(inputMaskDir(local));
        if (!error) {
            error = writeDefaultMaskfile(maskfullname, maskname, itemname, hidden);
        }
        if (!error) {
            add_new_input_mask(maskname, maskfullname, local);
            openMask = true;
        }
    }

    if (!error && openMask) {
        int           mask_id;
        awt_item_type item_type = AWT_IT_UNKNOWN;

        for (mask_id = 0; ; ++mask_id) {
            const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(mask_id);
            if (!descriptor) {
                error = GBS_global_string("Can't find descriptor for mask '%s'", maskname.c_str());
                break;
            }

            if (strcmp(descriptor->get_maskname(), maskname.c_str()) == 0 &&
                descriptor->is_local_mask() == local)
            {
                item_type = AWT_getItemType(descriptor->get_itemtypename());
                break;          // found wanted mask id
            }
        }

        if (!error) {
            error = openMaskWindowByType(mask_id, item_type);
        }
    }

    if (error) aw_message(error);
}

static void create_new_input_mask(AW_window *aww, awt_item_type item_type) { // create new user mask (interactively)
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aww->get_root(), "CREATE_USER_MASK", "Create new input mask:");

        aws->auto_space(10, 10);

        aws->button_length(10);
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", 0);
        aws->callback(makeHelpCallback("input_mask_new.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at_newline();

        aws->label("Name of new input mask");
        aws->create_input_field(AWAR_INPUT_MASK_NAME, 20);

        aws->at_newline();

        aws->label("Item type");
        aws->create_option_menu(AWAR_INPUT_MASK_ITEM, true);
        for (int i = AWT_IT_UNKNOWN+1; i<AWT_IT_TYPES; ++i) {
            aws->insert_option(awt_itemtype_names[i], "", awt_itemtype_names[i]);
        }
        aws->update_option_menu();

        aws->at_newline();

        aws->label("Scope");
        aws->create_toggle_field(AWAR_INPUT_MASK_SCOPE, 1);
        aws->insert_toggle("Local", "L", AWT_SCOPE_LOCAL);
        aws->insert_toggle("Global", "G", AWT_SCOPE_GLOBAL);
        aws->update_toggle_field();

        aws->at_newline();

        aws->label("Visibility");
        aws->create_toggle_field(AWAR_INPUT_MASK_HIDDEN, 1);
        aws->insert_toggle("Normal", "N", 0);
        aws->insert_toggle("Hidden", "H", 1);
        aws->update_toggle_field();

        aws->at_newline();

        aws->callback(create_new_mask_cb);
        aws->create_button("CREATE", "CREATE", "C");

        aws->window_fit();
    }

    aws->activate();
    aww->get_root()->awar(AWAR_INPUT_MASK_ITEM)->write_string(awt_itemtype_names[item_type]);
}

// -----------------------------------------------------
//      Create User-Mask-Submenu for any application

static bool hadMnemonic(char *availableMnemonics, char c) {
    // return true if 'c' occurs in 'availableMnemonics' (case ignored)
    // (in that case 'c' is removed from 'availableMnemonics').
    // returns false otherwise.
    char  lc   = tolower(c);
    char *cand = strchr(availableMnemonics, lc);
    if (cand) {
        char *last = strchr(cand+1, 0)-1;
        if (last>cand) {
            cand[0] = last[0];
            last[0] = 0;
        }
        else {
            awt_assert(last == cand);
            cand[0] = 0;
        }
        return true;
    }
    return false;
}

static char *selectMnemonic(const char *orgTitle, char *availableMnemonics, char& mnemonic) {
    // select (and remove) one from 'availableMnemonics' occurring in orgTitle
    // return selected in 'mnemonic'
    // return orgTitle (eventually modified if no matching mnemonic available)

    bool prevWasChar = false;
    for (int startOfWord = 1; startOfWord>=0; --startOfWord) {
        for (int i = 0; orgTitle[i]; ++i) {
            char c = orgTitle[i];
            if (isalnum(c)) {
                if (!prevWasChar || !startOfWord) {
                    if (hadMnemonic(availableMnemonics, c)) {
                        mnemonic = c;
                        return strdup(orgTitle);
                    }
                }
                prevWasChar = true;
            }
            else prevWasChar = false;
        }
    }

    for (int i = 0; i<2; ++i) {
        const char *takeAny = i ? availableMnemonics : "1234567890";
        for (int t = 0; takeAny[t]; ++t) {
            char c = takeAny[t];
            if (hadMnemonic(availableMnemonics, c)) {
                mnemonic = c;
                return GBS_global_string_copy("%s [%c]", orgTitle, c);
            }
        }
    }

    mnemonic = 0; // failed
    return strdup(orgTitle);
}

void AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, AWT_OpenMaskWindowCallback open_mask_window_cb, GBDATA *gb_main) {
    // add a user mask submenu at current position
    AW_root *awr = awm->get_root();

    if (!global_awars_created) create_global_awars(awr);

    awm->insert_sub_menu("User Masks", "k");

    char *availableMnemonics = strdup("abcdefghijklmopqrstuvwxyz0123456789"); // 'n' excluded!

    for (int scope = 0; scope <= 1; ++scope) {
        bool entries_made = false;

        for (int id = 0; ; ++id) {
            const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);

            if (!descriptor) break;
            if (descriptor->is_local_mask() != (scope == AWT_SCOPE_LOCAL)) continue; // wrong scope type

            awt_item_type item_type = AWT_getItemType(descriptor->get_itemtypename());

            if (item_type == wanted_item_type) {
                if (!descriptor->is_hidden()) { // do not show masks with hidden-flag
                    entries_made        = true;
                    char *macroname2key = GBS_string_2_key(descriptor->get_internal_maskname());
#if defined(DEBUG) && 0
                    printf("added user-mask '%s' with id=%i\n", descriptor->get_maskname(), id);
#endif // DEBUG
                    char  mnemonic[2] = "x";
                    char *mod_title   = selectMnemonic(descriptor->get_title(), availableMnemonics, mnemonic[0]);

                    awm->insert_menu_topic(macroname2key, mod_title, mnemonic, "input_mask.hlp", AWM_ALL, makeWindowCallback(open_mask_window_cb, id, gb_main));
                    free(mod_title);
                    free(macroname2key);
                }
                registerType(item_type, awm, open_mask_window_cb);
            }
            else if (item_type == AWT_IT_UNKNOWN) {
                aw_message(GBS_global_string("Unknown @ITEMTYPE '%s' in '%s'", descriptor->get_itemtypename(), descriptor->get_internal_maskname()));
            }
        }
        if (entries_made) awm->sep______________();
    }

    {
        const char *itemname            = awt_itemtype_names[wanted_item_type];
        char       *new_item_mask_id    = GBS_global_string_copy("new_%s_mask", itemname);
        char       *new_item_mask_label = GBS_global_string_copy("New %s mask..", itemname);

        awm->insert_menu_topic(new_item_mask_id, new_item_mask_label, "N", "input_mask_new.hlp", AWM_ALL, makeWindowCallback(create_new_input_mask, wanted_item_type));

        free(new_item_mask_label);
        free(new_item_mask_id);
    }
    free(availableMnemonics);
    awm->close_sub_menu();
}

void AWT_destroy_input_masks() {
    // unlink from DB manually - there are too many smartptrs to
    // get rid of all of them before DB gets destroyed on exit
    for (InputMaskList::iterator i = input_mask_list.begin();
         i != input_mask_list.end();
         ++i)
    {
        i->second->unlink(); 
    }
    input_mask_list.clear();
}


void awt_item_type_selector::add_awar_callbacks(AW_root *root, const RootCallback& cb) const {
    root->awar(get_self_awar())->add_callback(cb);
}

void awt_item_type_selector::remove_awar_callbacks(AW_root *root, const RootCallback& cb) const {
    root->awar(get_self_awar())->remove_callback(cb);
}
