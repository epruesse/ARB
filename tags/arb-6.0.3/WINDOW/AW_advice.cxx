//  ==================================================================== //
//                                                                       //
//    File      : aw_advice.cpp                                          //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2002              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "aw_advice.hxx"
#include "aw_window.hxx"
#include "aw_awar.hxx"
#include "aw_msg.hxx"
#include "aw_root.hxx"

#include <arbdb.h>

using namespace std;

#define AWAR_ADVICE_TMP "/tmp/advices/"

#define AWAR_ADVICE_TEXT       AWAR_ADVICE_TMP "text"
#define AWAR_ADVICE_UNDERSTOOD AWAR_ADVICE_TMP "understood"

#define AWAR_ADVICE_DISABLED "/advices/disabled"
#define AWAR_ADVICE_SHOWN    "/tmp/advices/shown"

// -------------------------
//      internal data :

static bool     initialized = false;
static AW_root *advice_root = 0;

// --------------------------------------------------------

void init_Advisor(AW_root *awr) {
    if (!initialized) {
        advice_root  = awr;

        advice_root->awar_string(AWAR_ADVICE_TEXT, "<no advice>");
        advice_root->awar_int(AWAR_ADVICE_UNDERSTOOD, 0);

        initialized = true;
    }
}

// ----------------------------
//      disabled advices :

inline AW_awar *get_disabled_advices() { return advice_root->awar_string(AWAR_ADVICE_DISABLED, ""); }
inline AW_awar *get_shown_advices()    { return advice_root->awar_string(AWAR_ADVICE_SHOWN, ""); }

inline int advice_id_offset(const char* id, const char *idlist) {
    const char *found = strstr(idlist, GBS_global_string(";%s;", id));
    return found ? found-idlist : -1;
}

inline bool advice_id_is_set(const char* id, const char *idlist) { return advice_id_offset(id, idlist) >= 0; }

inline void set_advice_id(const char* id, AW_awar *var) {
    const char *idlist = var->read_char_pntr();
    if (!advice_id_is_set(id, idlist)) {
        if (idlist[0]) var->write_string(GBS_global_string("%s%s;", idlist, id));
        else var->write_string(GBS_global_string(";%s;", id));
    }
}
inline void remove_advice_id(const char* id, AW_awar *var) {
    const char *idlist = var->read_char_pntr();
    if (advice_id_is_set(id, idlist)) {
        int offset = advice_id_offset(id, idlist);
        if (offset >= 0) {
            char *newList = 0;
            if (offset == 0) {
                newList = strdup(idlist+strlen(id)+1);
            }
            else {
                newList     = strdup(idlist);
                char *idPos = newList+offset;
                strcpy(idPos, idPos+strlen(id)+1);
            }
            var->write_string(newList);
            free(newList);
        }
    }
}
inline void toggle_advice_id(const char* id, AW_awar *var) {
    if (advice_id_is_set(id, var->read_char_pntr())) {
        remove_advice_id(id, var);
    }
    else {
        set_advice_id(id, var);
    }
}

inline bool advice_disabled(const char* id) { return advice_id_is_set(id, get_disabled_advices()->read_char_pntr()); }
inline void disable_advice(const char* id)  { set_advice_id(id, get_disabled_advices()); }

inline bool advice_currently_shown(const char *id) { return advice_id_is_set(id, get_shown_advices()->read_char_pntr()); }
inline void toggle_advice_shown(const char *id)    { toggle_advice_id(id, get_shown_advices()); }

// -------------------------------------------

static void advice_close_cb(AW_window *aww, const char *id, AW_Advice_Type type) {
    int understood = advice_root->awar(AWAR_ADVICE_UNDERSTOOD)->read_int();

    // switch to 'not understood'. Has to be checked by user for each advice.
    advice_root->awar(AWAR_ADVICE_UNDERSTOOD)->write_int(0);
    aww->hide();
    
    toggle_advice_shown(id);

    if (understood) {
        disable_advice(id);
        if (type & AW_ADVICE_TOGGLE) {
            static bool in_advice = false;
            if (!in_advice) {
                in_advice = true;
                AW_advice("You have disabled an advice.\n"
                          "In order to disable it PERMANENTLY, save properties.", AW_ADVICE_TOGGLE);
                in_advice = false;
            }
        }
    }
}
static void advice_hide_and_close_cb(AW_window *aww, const char *id, AW_Advice_Type type) {
    advice_root->awar(AWAR_ADVICE_UNDERSTOOD)->write_int(1);
    advice_close_cb(aww, id, type);
}

// -------------------------------------------

void AW_reactivate_all_advices(AW_window*) {
    AW_awar *awar_disabled = get_disabled_advices();

    char       *disabled = awar_disabled->read_string();
    char       *nosemi   = GBS_string_eval(disabled, ";=", NULL);
    int         entries  = strlen(disabled)-strlen(nosemi);
    const char *msg      = "No advices were disabled yet.";

    if (entries>0) {
        aw_assert(entries>1);
        entries--;
        msg = GBS_global_string("Reactivated %i advices (for this session)\n"
                                "To reactivate them for future sessions, save properties.",
                                entries);
    }
    aw_message(msg);

    free(nosemi);
    free(disabled);

    awar_disabled->write_string("");
}

void AW_advice(const char *message, AW_Advice_Type type, const char *title, const char *corresponding_help) {
    aw_assert(initialized);
    size_t  message_len = strlen(message); aw_assert(message_len>0);
    long    crc32       = GB_checksum(message, message_len, true, " .,-!"); // checksum is used to test if advice was shown
    char   *advice_id   = GBS_global_string_copy("%lx", crc32); // do not delete (bound to cbs)

    bool show_advice = !advice_disabled(advice_id) && !advice_currently_shown(advice_id);

    if (show_advice) {
        AW_awar *understood = advice_root->awar(AWAR_ADVICE_UNDERSTOOD);
        understood->write_int(0);

        if (corresponding_help) type = AW_Advice_Type(type|AW_ADVICE_HELP);
#if defined(ASSERTION_USED)
        else aw_assert((type & AW_ADVICE_HELP) == 0);
#endif // ASSERTION_USED

        AW_window_simple *aws = new AW_window_simple; // do not delete (ARB will crash) -- maybe reuse window for all advices?

        if (!title) title = "Please read carefully";
        aws->init(advice_root, "advice", GBS_global_string("ARB: %s", title));
        aws->load_xfig("window/advice.fig");

        bool has_help     = type & AW_ADVICE_HELP;
        bool help_pops_up = false;

        if (has_help) {
            aws->callback(makeHelpCallback(corresponding_help));
            aws->at("help");
            aws->create_button(0, "HELP", "H");

            if (type & AW_ADVICE_HELP_POPUP) help_pops_up = true;
        }

        aws->at("advice");
        aws->create_text_field(AWAR_ADVICE_TEXT);

        advice_root->awar(AWAR_ADVICE_TEXT)
            ->write_string(has_help && !help_pops_up
                           ? GBS_global_string("%s\n\nPlease refer to 'HELP' for more info.", message)
                           : message);

        if (help_pops_up) AW_help_popup(aws, corresponding_help);

        if (type & AW_ADVICE_TOGGLE) {
            aws->label("Do not advice me again");
            aws->at("understood");
            aws->create_toggle(AWAR_ADVICE_UNDERSTOOD);
        }

        aws->at("ok");
        if (type & AW_ADVICE_TOGGLE) {
            aws->callback(makeWindowCallback(advice_close_cb, advice_id, type));
            aws->create_button(0, "OK", "O");
        }
        else {
            aws->callback(makeWindowCallback(advice_hide_and_close_cb, advice_id, type));
            aws->create_autosize_button(0, "I understand", "O", 2);
        }

        aws->window_fit();
        aws->allow_delete_window(false); // disable closing the window via 'X'-button
        aws->show();
        toggle_advice_shown(advice_id);
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_advice_id_awar_handling() {
    GB_shell  shell;
    AW_root  root("min_ascii.arb");
    init_Advisor(&root);

    const char *one = "one";
    const char *two = "second";

    TEST_REJECT(advice_disabled(one));
    TEST_REJECT(advice_disabled(two));

    disable_advice(one);
    TEST_EXPECT(advice_disabled(one));
    TEST_REJECT(advice_disabled(two));

    disable_advice(two);
    TEST_EXPECT(advice_disabled(one));
    TEST_EXPECT(advice_disabled(two));


    TEST_REJECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));

    toggle_advice_shown(two);
    TEST_REJECT(advice_currently_shown(one));
    TEST_EXPECT(advice_currently_shown(two));

    toggle_advice_shown(one);
    TEST_EXPECT(advice_currently_shown(one));
    TEST_EXPECT(advice_currently_shown(two));
    
    toggle_advice_shown(two);
    TEST_EXPECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));

    toggle_advice_shown(one);
    TEST_REJECT(advice_currently_shown(one));
    TEST_REJECT(advice_currently_shown(two));
}

void TEST_another_AW_root() {
    GB_shell  shell;
    AW_root("min_ascii.arb");
}

#endif // UNIT_TESTS
