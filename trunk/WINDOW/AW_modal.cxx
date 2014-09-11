// ================================================================ //
//                                                                  //
//   File      : AW_modal.cxx                                       //
//   Purpose   : Modal dialogs                                      //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_window.hxx>
#include <aw_global.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include "aw_root.hxx"
#include "aw_question.hxx"
#include "aw_advice.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"
#include "aw_window_Xm.hxx"

#include <arbdbt.h>
#include <arb_strarray.h>

#include <deque>
#include <string>
#include <algorithm>

using namespace std;

int aw_message_cb_result;

void message_cb(AW_window*, int result) {
    if (result == -1) { // exit
        exit(EXIT_FAILURE);
    }
    aw_message_cb_result = result;
}

unsigned aw_message_timer_listen_event(AW_root *, AW_window *aww) {
#if defined(TRACE_STATUS_MORE)
    fprintf(stderr, "in aw_message_timer_listen_event\n"); fflush(stdout);
#endif // TRACE_STATUS_MORE

    if (aww->is_shown()) {
        return AW_MESSAGE_LISTEN_DELAY;
    }
    return 0;
}

// -----------------
//      aw_input

static char *aw_input_cb_result = 0;

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

void input_history_cb(AW_window *aw, int mode) {
    // mode: -1 = '<' +1 = '>'
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

void input_cb(AW_window *aw, int buttonNr) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result = 0;

    if (buttonNr >= 0) { // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_INPUT_AWAR)->read_as_string();
    }
}

void file_selection_cb(AW_window *aw, int ok_cancel_flag) {
    // any previous contents were passed to client (who is responsible to free the resources)
    // so DON'T free aw_input_cb_result here:
    aw_input_cb_result = 0;

    if (ok_cancel_flag >= 0) { // <0 = cancel button -> no result
        // create heap-copy of result -> client will get the owner
        aw_input_cb_result = aw->get_root()->awar(AW_FILE_SELECT_FILE_AWAR)->read_as_string();
    }
}

#define INPUT_SIZE 50           // size of input prompts in aw_input

static AW_window_message *new_input_window(AW_root *root, const char *title, const char *buttons) {
    // helper for aw_input
    //
    // 'buttons': comma separated list of button names
    // - a buttonname starting with - marks the abort button (only one possible)
    // - buttonnames starting with \n force a newline
    // (has to be '-\nNAME' if combined)

    aw_assert(buttons);

    AW_window_message *aw_msg = new AW_window_message;

    aw_msg->init(root, title, false);

    aw_msg->label_length(0);
    aw_msg->auto_space(10, 10);

    aw_msg->at(10, 10);
    aw_msg->button_length(INPUT_SIZE+1);
    aw_msg->create_button(0, AW_INPUT_TITLE_AWAR);

    aw_msg->at_newline();
    aw_msg->create_input_field(AW_INPUT_AWAR, INPUT_SIZE);

    ConstStrArray button_names;
    int           maxlen = 0; // track max. button length (used as min.length for buttons)

    GBT_split_string(button_names, buttons, ',');
    int butCount = button_names.size();

    int abortButton = -1; // index of abort button (-1 means 'none')
    for (int b = 0; b<butCount; b++) {
        if (button_names[b][0] == '-') {
            aw_assert(abortButton<0); // only one abort button possible!
            abortButton        = b;
            button_names.replace(b, button_names[b]+1); // point behind '-'
        }
        int len = strlen(button_names[b]);
        if (len>maxlen) maxlen = len;
    }

    aw_msg->button_length(maxlen+1);

#define MAXBUTTONSPERLINE 5

    aw_msg->at_newline();
    aw_msg->callback(makeWindowCallback(input_history_cb, -1)); aw_msg->create_button("bwd", "<<", 0);
    aw_msg->callback(makeWindowCallback(input_history_cb,  1)); aw_msg->create_button("fwd", ">>", 0);
    int thisLine = 2;

    if (butCount>(MAXBUTTONSPERLINE-thisLine) && butCount <= MAXBUTTONSPERLINE) { // approx. 5 buttons (2+3) fit into one line
        aw_msg->at_newline();
        thisLine = 0;
    }

    for (int b = 0; b<butCount; b++) {
        const char *name    = button_names[b];
        bool        forceLF = name[0] == '\n';

        if (thisLine >= MAXBUTTONSPERLINE || forceLF) {
            aw_msg->at_newline();
            thisLine = 0;
            if (forceLF) name++;
        }

        // use 0 as result for 1st button, 1 for 2nd button, etc.
        // use -1 for abort button
        int resultCode = b == abortButton ? -1 : b;
        aw_msg->callback(makeWindowCallback(input_cb, resultCode));
        aw_msg->create_button(name, name, "");
        thisLine++;
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

    AW_root *root = AW_root::SINGLETON;
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

    if (!aw_msg) aw_msg = new_input_window(root, title, "Ok,-Abort");
    else aw_msg->set_window_title(title);

    aw_msg->window_fit();
    aw_msg->show_modal();
    char dummy[]       = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, makeTimedCallback(aw_message_timer_listen_event, static_cast<AW_window*>(aw_msg)));
    {
        LocallyModify<bool> flag(root->disable_callbacks, true);
        while (aw_input_cb_result == dummy) {
            root->process_events();
        }
    }
    aw_msg->hide();

    if (aw_input_cb_result) input_history_insert(aw_input_cb_result, true);
    return aw_input_cb_result;
}

char *aw_input(const char *prompt, const char *default_input) {
    return aw_input("Enter string", prompt, default_input);
}

// --------------------------
//      aw_file_selection

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    AW_root *root = AW_root::SINGLETON;

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

        AW_create_standard_fileselection(aw_msg, AW_FILE_SELECT_BASE);

        aw_msg->button_length(7);

        aw_msg->at("ok");
        aw_msg->callback(makeWindowCallback(file_selection_cb, 0));
        aw_msg->create_button("OK", "OK", "O");

        aw_msg->at("cancel");
        aw_msg->callback(makeWindowCallback(file_selection_cb, -1));
        aw_msg->create_button("CANCEL", "CANCEL", "C");

        aw_msg->window_fit();
    }

    aw_msg->show_modal();
    char dummy[] = "";
    aw_input_cb_result = dummy;

    root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, makeTimedCallback(aw_message_timer_listen_event, static_cast<AW_window*>(aw_msg)));
    {
        LocallyModify<bool> flag(root->disable_callbacks, true);
        while (aw_input_cb_result == dummy) {
            root->process_events();
        }
    }
    aw_msg->hide();

    return aw_input_cb_result;
}


