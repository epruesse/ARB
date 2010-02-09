// ================================================================ //
//                                                                  //
//   File      : AW_modal.cxx                                       //
//   Purpose   : Modal dialogs                                      //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_window.hxx"
#include "aw_global.hxx"

#include <awt.hxx>

#include <arbdbt.h>

#include <deque>
#include <string>
#include <algorithm>

using namespace std;

#define AWAR_QUESTION "tmp/question"

#define AW_MESSAGE_LISTEN_DELAY 500 // look in ms whether a father died

int aw_message_cb_result;

void message_cb(AW_window *aw, AW_CL cd1) {
    AWUSE(aw);
    long result = (long)cd1;
    if (result == -1) { /* exit */
        exit(EXIT_FAILURE);
    }
    aw_message_cb_result = ((int)result);
    return;
}

static void aw_message_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2)
{
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_message_timer_listen_event\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE

    AW_window *aww = ((AW_window *)cl1);
    if (aww->is_shown()) { // if still shown, then auto-activate to avoid that user minimizes prompter
        aww->activate();
        awr->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, cl1, cl2);
    }
}

// --------------------
//      aw_question

int aw_question(const char *question, const char *buttons, bool fixedSizeButtons, const char *helpfile) {
    // return 0 for first button, 1 for second button, 2 for third button, ...
    //
    // the single buttons are separated by commas (e.g. "YES,NO")
    // If the button-name starts with ^ it starts a new row of buttons
    // (if a button named 'EXIT' is pressed the program terminates using exit(EXIT_FAILURE))
    //
    // The remaining arguments only apply if 'buttons' is non-zero:
    //
    // If fixedSizeButtons is true all buttons have the same size
    // otherwise the size for every button is set depending on the text length
    //
    // If 'helpfile' is non-zero a HELP button is added.

    aw_assert(buttons);

    AW_root *root = AW_root::THIS;

    AW_window_message *aw_msg;
    char              *button_list  = strdup(buttons ? buttons : "OK");

    if (button_list[0] == 0) {
        freedup(button_list, "Maybe ok,EXIT");
        GBK_dump_backtrace(stderr, "Empty buttonlist");
        question = GBS_global_string_copy("%s\n"
                                          "(Program error - Unsure what happens when you click ok\n"
                                          " Check console for backtrace and report error)",
                                          question);
    }

    AW_awar *awar_quest     = root->awar_string(AWAR_QUESTION);
    if (!question) question = "<oops - no question?!>";
    awar_quest->write_string(question);

    size_t question_length, question_lines;
    aw_detect_text_size(question, question_length, question_lines);

    // hash key to find matching window
    char *hindex = GBS_global_string_copy("%s$%zu$%zu$%i$%s",
                                          button_list,
                                          question_length, question_lines, int(fixedSizeButtons),
                                          helpfile ? helpfile : "");

    static GB_HASH *hash_windows    = 0;
    if (!hash_windows) hash_windows = GBS_create_hash(256, GB_MIND_CASE);
    aw_msg                          = (AW_window_message *)GBS_read_hash(hash_windows, hindex);

#if defined(DEBUG)
    printf("question_length=%zu question_lines=%zu\n", question_length, question_lines);
    printf("hindex='%s'\n", hindex);
    if (aw_msg) printf("[Reusing existing aw_question-window]\n");
#endif // DEBUG

    if (!aw_msg) {
        aw_msg = new AW_window_message;
        GBS_write_hash(hash_windows, hindex, (long)aw_msg);

        aw_msg->init(root, "QUESTION BOX", false);
        aw_msg->recalc_size_atShow(AW_RESIZE_DEFAULT); // force size recalc (ignores user size)

        aw_msg->label_length(10);

        aw_msg->at(10, 10);
        aw_msg->auto_space(10, 10);

        aw_msg->button_length(question_length+1);
        aw_msg->button_height(question_lines);

        aw_msg->create_button(0, AWAR_QUESTION);

        aw_msg->button_height(0);

        aw_msg->at_newline();

        if (fixedSizeButtons) {
            size_t  max_button_length = helpfile ? 4 : 0;
            char   *pos               = button_list;

            while (1) {
                char *comma       = strchr(pos, ',');
                if (!comma) comma = strchr(pos, 0);

                size_t len                                   = comma-pos;
                if (len>max_button_length) max_button_length = len;

                if (!comma[0]) break;
                pos = comma+1;
            }

            aw_msg->button_length(max_button_length+1);
        }
        else {
            aw_msg->button_length(0);
        }

        // insert the buttons:
        char *ret              = strtok(button_list, ",");
        bool  help_button_done = false;
        int   counter          = 0;

        while (ret) {
            if (ret[0] == '^') {
                if (helpfile && !help_button_done) {
                    aw_msg->callback(AW_POPUP_HELP, (AW_CL)helpfile);
                    aw_msg->create_button("HELP", "HELP", "H");
                    help_button_done = true;
                }
                aw_msg->at_newline();
                ++ret;
            }
            if (strcmp(ret, "EXIT") == 0) {
                aw_msg->callback(message_cb, -1);
            }
            else {
                aw_msg->callback(message_cb, (AW_CL)counter++);
            }

            if (fixedSizeButtons) {
                aw_msg->create_button(0, ret);
            }
            else {
                aw_msg->create_autosize_button(0, ret);
            }
            ret = strtok(NULL, ",");
        }

        if (helpfile && !help_button_done) { // if not done above
            aw_msg->callback(AW_POPUP_HELP, (AW_CL)helpfile);
            aw_msg->create_button("HELP", "HELP", "H");
            help_button_done = true;
        }

        aw_msg->window_fit();
    }
    free(hindex);
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();

    free(button_list);
    aw_message_cb_result = -13;

#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "add aw_message_timer_listen_event with delay = %i\n", AW_MESSAGE_LISTEN_DELAY); fflush(stdout);
#endif // TRACE_STATUS_MORE
    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_message_cb_result == -13) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    switch (aw_message_cb_result) {
        case -1:                /* exit with core */
            fprintf(stderr, "Core dump requested\n");
            ARB_SIGSEGV(1);
            break;
        case -2:                /* exit without core */
            exit(-1);
            break;
    }
    return aw_message_cb_result;
}

bool aw_ask_sure(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    return aw_question(msg, "Yes,No", fixedSizeButtons, helpfile) == 0;
}
void aw_popup_ok(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    aw_question(msg, "Ok", fixedSizeButtons, helpfile);
}
void aw_popup_exit(const char *msg, bool fixedSizeButtons, const char *helpfile) {
    aw_question(msg, "EXIT", fixedSizeButtons, helpfile);
    aw_assert(0); // should not be reached
    exit(EXIT_FAILURE);
}


// -----------------
//      aw_input

char       *aw_input_cb_result        = 0;
static int  aw_string_selected_button = -2;

int aw_string_selection_button() {
    aw_assert(aw_string_selected_button != -2);
    return aw_string_selected_button;
}

#define AW_INPUT_AWAR       "tmp/input/string"
#define AW_INPUT_TITLE_AWAR "tmp/input/title"

#define AW_FILE_SELECT_BASE        "tmp/file_select"
#define AW_FILE_SELECT_DIR_AWAR    AW_FILE_SELECT_BASE "/directory"
#define AW_FILE_SELECT_FILE_AWAR   AW_FILE_SELECT_BASE "/file_name"
#define AW_FILE_SELECT_FILTER_AWAR AW_FILE_SELECT_BASE "/filter"
#define AW_FILE_SELECT_TITLE_AWAR  AW_FILE_SELECT_BASE "/title"

static void create_input_awars(AW_root *aw_root) {
    aw_root->awar_string(AW_INPUT_TITLE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_INPUT_AWAR,       "", AW_ROOT_DEFAULT);
}

static void create_fileSelection_awars(AW_root *aw_root) {
    aw_root->awar_string(AW_FILE_SELECT_TITLE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_DIR_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_FILE_AWAR, "", AW_ROOT_DEFAULT);
    aw_root->awar_string(AW_FILE_SELECT_FILTER_AWAR, "", AW_ROOT_DEFAULT);
}


// -------------------------
//      aw_input history

static deque<string> input_history; // front contains newest entries

#if defined(DEBUG)
// # define TRACE_HISTORY
#endif // DEBUG


#if defined(TRACE_HISTORY)
static void dumpHistory(const char *where) {
    printf("History [%s]:\n", where);
    for (deque<string>::iterator h = input_history.begin(); h != input_history.end(); ++h) {
        printf("'%s'\n", h->c_str());
    }
}
#endif // TRACE_HISTORY

static void input_history_insert(const char *str, bool front) {
    string s(str);

    if (input_history.empty()) {
        input_history.push_front(""); // insert an empty string into history
    }
    else {
        deque<string>::iterator found = find(input_history.begin(), input_history.end(), s);
        if (found != input_history.end()) {
            input_history.erase(found);
        }
    }
    if (front) {
        input_history.push_front(s);
    }
    else {
        input_history.push_back(s);
    }

#if defined(TRACE_HISTORY)
    dumpHistory(GBS_global_string("input_history_insert('%s', front=%i)", str, front));
#endif // TRACE_HISTORY
}

void input_history_cb(AW_window *aw, AW_CL cl_mode) {
    int      mode    = (int)cl_mode;                // -1 = '<' +1 = '>'
    AW_root *aw_root = aw->get_root();
    AW_awar *awar    = aw_root->awar(AW_INPUT_AWAR);
    char    *content = awar->read_string();

    if (content) input_history_insert(content, mode == 1);

    if (!input_history.empty()) {
        if (mode == -1) {
            string s = input_history.front();
            awar->write_string(s.c_str());
            input_history.pop_front();
            input_history.push_back(s);
        }
        else {
            string s = input_history.back();
            awar->write_string(s.c_str());
            input_history.pop_back();
            input_history.push_front(s);
        }
    }

#if defined(TRACE_HISTORY)
    dumpHistory(GBS_global_string("input_history_cb(mode=%i)", mode));
#endif // TRACE_HISTORY

    free(content);
}

void input_cb(AW_window *aw, AW_CL cd1) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result        = 0;
    aw_string_selected_button = int(cd1);

    if (cd1 >= 0) {              // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_INPUT_AWAR)->read_as_string();
    }
}

void file_selection_cb(AW_window *aw, AW_CL cd1) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result        = 0;
    aw_string_selected_button = int(cd1);

    if (cd1 >= 0) {              // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_FILE_SELECT_FILE_AWAR)->read_as_string();
    }
}

#define INPUT_SIZE 50           // size of input prompts in aw_input and aw_string_selection

static AW_window_message *new_input_window(AW_root *root, const char *title, const char *buttons) {
    // helper for aw_input and aw_string_selection
    //
    // 'buttons' comma separated list of button names (buttons starting with \n force a newline)

    AW_window_message *aw_msg = new AW_window_message;

    aw_msg->init(root, title, false);

    aw_msg->label_length(0);
    aw_msg->auto_space(10, 10);

    aw_msg->at(10, 10);
    aw_msg->button_length(INPUT_SIZE+1);
    aw_msg->create_button(0, AW_INPUT_TITLE_AWAR);

    aw_msg->at_newline();
    aw_msg->create_input_field(AW_INPUT_AWAR, INPUT_SIZE);

    int    butCount     = 2;    // ok and cancel
    char **button_names = 0;
    int    maxlen       = 6;    // use as min.length for buttons (for 'CANCEL')

    if (buttons) {
        button_names = GBT_split_string(buttons, ',', &butCount);

        for (int b = 0; b<butCount; b++) {
            int len = strlen(button_names[b]);
            if (len>maxlen) maxlen = len;
        }

    }

    aw_msg->button_length(maxlen+1);

#define MAXBUTTONSPERLINE 5

    aw_msg->at_newline();
    aw_msg->callback(input_history_cb, -1); aw_msg->create_button("bwd", "<<", 0);
    aw_msg->callback(input_history_cb,  1); aw_msg->create_button("fwd", ">>", 0);
    int thisLine = 2;

    // @@@ add a history button (opening a window with elements from history)

    if (butCount>(MAXBUTTONSPERLINE-thisLine) && butCount <= MAXBUTTONSPERLINE) { // approx. 5 buttons (2+3) fit into one line
        aw_msg->at_newline();
        thisLine = 0;
    }

    if (buttons) {
        for (int b = 0; b<butCount; b++) {
            const char *name    = button_names[b];
            bool        forceLF = name[0] == '\n';

            if (thisLine >= MAXBUTTONSPERLINE || forceLF) {
                aw_msg->at_newline();
                thisLine = 0;
                if (forceLF) name++;
            }
            aw_msg->callback(input_cb, b);          // use b == 0 as result for 1st button, 1 for 2nd button, etc.
            aw_msg->create_button(name, name, "");
            thisLine++;
        }
        GBT_free_names(button_names);
        button_names = 0;
    }
    else {
        aw_msg->callback(input_cb,  0); aw_msg->create_button("OK", "OK", "O");
        aw_msg->callback(input_cb, -1); aw_msg->create_button("CANCEL", "CANCEL", "C");
    }

    return aw_msg;
}

char *aw_input(const char *title, const char *prompt, const char *default_input) {
    // prompt user to enter a string
    //
    // title         = title of window
    // prompt        = question
    // default_input = default for answer (NULL -> "")
    //
    // result is NULL, if cancel was pressed
    // otherwise result contains the user input (maybe an empty string)

    static AW_window_message *aw_msg = 0;

    AW_root *root = AW_root::THIS;
    if (!aw_msg) create_input_awars(root); // first call -> create awars

    root->awar(AW_INPUT_TITLE_AWAR)->write_string(prompt);
    aw_assert(strlen(prompt) <= INPUT_SIZE);

    AW_awar *inAwar = root->awar(AW_INPUT_AWAR);
    if (default_input) {
        input_history_insert(default_input, true); // insert default into history
        inAwar->write_string(default_input);
    }
    else {
        inAwar->write_string("");
    }

    aw_assert(GB_get_transaction_level(inAwar->gb_var) <= 0); // otherwise history would not work

    if (!aw_msg) aw_msg = new_input_window(root, title, NULL);
    else aw_msg->set_window_title(title);

    aw_msg->window_fit();
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[]       = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    if (aw_input_cb_result) input_history_insert(aw_input_cb_result, true);
    return aw_input_cb_result;
}

char *aw_input(const char *prompt, const char *default_input) {
    return aw_input("Enter string", prompt, default_input);
}

char *aw_input2awar(const char *title, const char *prompt, const char *awar_name) {
    AW_root *aw_root       = AW_root::THIS;
    AW_awar *awar          = aw_root->awar(awar_name);
    char    *default_value = awar->read_string();
    char    *result        = aw_input(title, prompt, default_value);

    awar->write_string(result);
    free(default_value);

    return result;
}

char *aw_input2awar(const char *prompt, const char *awar_name) {
    return aw_input2awar("Enter string", prompt, awar_name);
}


char *aw_string_selection(const char *title, const char *prompt, const char *default_input, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    // A modal input window. A String may be entered by hand or selected from value_list
    //
    //      title           window title
    //      prompt          prompt at input field
    //      default_input   default value (if NULL => "").
    //      value_list      Existing selections (separated by ';') or NULL if no selection exists
    //      buttons         String containing answer button names separated by ',' (default is "OK,Cancel")
    //                      Use aw_string_selected_button() to detect which has been pressed.
    //      check_fun       function to correct input (or NULL for no check). The function may return NULL to indicate no correction
    //
    // returns the value of the inputfield

    static GB_HASH *str_sels = 0; // for each 'buttons' store window + selection list

    if (!str_sels) str_sels = GBS_create_hash(20, GB_MIND_CASE);

    struct str_sel_data {
        AW_window_message *aw_msg;
        AW_selection_list *sel;
    };

    const char   *bkey = buttons ? buttons : ",default,";
    str_sel_data *sd      = (str_sel_data*)GBS_read_hash(str_sels, bkey);
    if (!sd) {
        sd         = new str_sel_data;
        sd->aw_msg = 0;
        sd->sel    = 0;

        GBS_write_hash(str_sels, bkey, (long)sd);
    }

    AW_window_message *& aw_msg = sd->aw_msg;
    AW_selection_list *& sel    = sd->sel;

    AW_root *root = AW_root::THIS;
    if (!aw_msg) create_input_awars(root); // first call -> create awars

    root->awar(AW_INPUT_TITLE_AWAR)->write_string(prompt);
    aw_assert(strlen(prompt) <= INPUT_SIZE);

    AW_awar *inAwar = root->awar(AW_INPUT_AWAR);
    if (default_input) {
        input_history_insert(default_input, true); // insert default into history
        inAwar->write_string(default_input);
    }
    else {
        inAwar->write_string("");
    }

    aw_assert(GB_get_transaction_level(inAwar->gb_var) <= 0); // otherwise history would not work

    if (!aw_msg) {
        aw_msg = new_input_window(root, title, buttons);

        aw_msg->at_newline();
        sel = aw_msg->create_selection_list(AW_INPUT_AWAR, 0, 0, INPUT_SIZE, 10);
        aw_msg->insert_default_selection(sel, "", "");
        aw_msg->update_selection_list(sel);
    }
    else {
        aw_msg->set_window_title(title);
    }
    aw_msg->window_fit();

    // update the selection box :
    aw_assert(sel);
    aw_msg->clear_selection_list(sel);
    if (value_list) {
        char *values = strdup(value_list);
        char *word;

        for (word = strtok(values, ";"); word; word = strtok(0, ";")) {
            aw_msg->insert_selection(sel, word, word);
        }
        free(values);
    }
    aw_msg->insert_default_selection(sel, "<new>", "");
    aw_msg->update_selection_list(sel);

    // do modal loop :
    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;

    char *last_input = root->awar(AW_INPUT_AWAR)->read_string();
    while (aw_input_cb_result == dummy) {
        root->process_events();

        char *this_input = root->awar(AW_INPUT_AWAR)->read_string();
        if (strcmp(this_input, last_input) != 0) {
            if (check_fun) {
                char *corrected_input = check_fun(this_input);
                if (corrected_input) {
                    if (strcmp(corrected_input, this_input) != 0) {
                        root->awar(AW_INPUT_AWAR)->write_string(corrected_input);
                    }
                    free(corrected_input);
                }
            }
            reassign(last_input, this_input);
        }
        free(this_input);

        if (!aw_msg->is_shown()) { // somebody hided/closed the window
            input_cb(aw_msg, (AW_CL)-1); // CANCEL
            break;
        }
    }

    free(last_input);

    root->disable_callbacks = false;
    aw_msg->hide();

    return aw_input_cb_result;
}

char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name, const char *value_list, const char *buttons, char *(*check_fun)(const char*)) {
    // params see aw_string_selection
    // default_value is taken from and result is written back to awar 'awar_name'

    AW_root *aw_root       = AW_root::THIS;
    AW_awar *awar          = aw_root->awar(awar_name);
    char    *default_value = awar->read_string();
    char    *result        = aw_string_selection(title, prompt, default_value, value_list, buttons, check_fun);

    awar->write_string(result);
    free(default_value);

    return result;
}

// --------------------------
//      aw_file_selection

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    AW_root *root = AW_root::THIS;

    static AW_window_simple *aw_msg = 0;
    if (!aw_msg) create_fileSelection_awars(root);

    {
        char *edir      = GBS_eval_env(dir);
        char *edef_name = GBS_eval_env(def_name);

        root->awar(AW_FILE_SELECT_TITLE_AWAR) ->write_string(title);
        root->awar(AW_FILE_SELECT_DIR_AWAR)   ->write_string(edir);
        root->awar(AW_FILE_SELECT_FILE_AWAR)  ->write_string(edef_name);
        root->awar(AW_FILE_SELECT_FILTER_AWAR)->write_string(suffix);

        free(edef_name);
        free(edir);
    }

    if (!aw_msg) {
        aw_msg = new AW_window_simple;

        aw_msg->init(root, "AW_FILE_SELECTION", "File selection");
        aw_msg->allow_delete_window(false); // disable closing the window

        aw_msg->load_xfig("fileselect.fig");

        aw_msg->at("title");
        aw_msg->create_button(0, AW_FILE_SELECT_TITLE_AWAR);

        awt_create_fileselection(aw_msg, AW_FILE_SELECT_BASE);

        aw_msg->button_length(7);

        aw_msg->at("ok");
        aw_msg->callback     (file_selection_cb, 0);
        aw_msg->create_button("OK", "OK", "O");

        aw_msg->at("cancel");
        aw_msg->callback     (file_selection_cb, -1);
        aw_msg->create_button("CANCEL", "CANCEL", "C");

        aw_msg->window_fit();
    }

    aw_msg->recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    aw_msg->show_grabbed();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, aw_message_timer_listen_event, (AW_CL)aw_msg, 0);
    root->disable_callbacks = true;
    while (aw_input_cb_result == dummy) {
        root->process_events();
    }
    root->disable_callbacks = false;
    aw_msg->hide();

    return aw_input_cb_result;
}


