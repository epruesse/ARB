//  ==================================================================== //
//                                                                       //
//    File      : awt_advice.cpp                                         //
//    Purpose   :                                                        //
//    Time-stamp: <Thu Mar/11/2004 13:42 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2002              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <stdlib.h>
#include <string.h>

#include "awt_advice.hxx"
#include "awt.hxx"


using namespace std;

#define AWAR_ADVICE_TMP "/tmp/advices/"

#define AWAR_ADVICE_TEXT       AWAR_ADVICE_TMP "text"
#define AWAR_ADVICE_UNDERSTOOD AWAR_ADVICE_TMP "understood"

#define AWAR_ADVICE_DISABLED "/advices/disabled"

// -------------------------
//      internal data :
// -------------------------

static bool        initialized  = false;
static AW_root    *advice_root  = 0;
static AW_default  advice_props = 0;

// --------------------------------------------------------
//      void init_Advisor(AW_root *awr, AW_default def)
// --------------------------------------------------------
void init_Advisor(AW_root *awr, AW_default def)
{
    awt_assert(!initialized);   // can't init twice

    advice_root  = awr;
    advice_props = def;

    advice_root->awar_string(AWAR_ADVICE_TEXT, "<no advice>", advice_props);
    advice_root->awar_int(AWAR_ADVICE_UNDERSTOOD, 0, advice_props);

    initialized = true;
}

// -------------------------
//      event handler :
// -------------------------

#define AWT_ADVICE_LISTEN_DELAY 500
static const char *awt_advice_cb_result = 0;

void awt_message_timer_listen_event(AW_root *awr, AW_CL cl1, AW_CL cl2)
{
    AW_window *aww = ((AW_window *)cl1);
    if (aww->get_show()){
        aww->show();
        awr->add_timed_callback(AWT_ADVICE_LISTEN_DELAY, awt_message_timer_listen_event, cl1, cl2);
    }
}

static void advice_close_cb(AW_window*) {
    awt_advice_cb_result = "close";
}
static void advice_hide_and_close_cb(AW_window*) {
    advice_root->awar(AWAR_ADVICE_UNDERSTOOD)->write_int(1);
    awt_advice_cb_result = "close";
}

// ----------------------------
//      disabled advices :
// ----------------------------

static AW_awar *get_disabled_advices() {
    return advice_root->awar_string(AWAR_ADVICE_DISABLED, "", advice_props);
}

static bool advice_disabled(const char* id, AW_awar *var = 0) {
    if (!var) var       = get_disabled_advices();
    char *disabled_list = var->read_string();
    bool  is_disabled   = strstr(disabled_list, GBS_global_string(";%s;", id)) != 0;
    free(disabled_list);
    return is_disabled;
}
static void disable_advice(const char* id) {
    AW_awar *var = get_disabled_advices();
    if (!advice_disabled(id, var)) {
        char *disabled_list = var->read_string();
        if (strlen(disabled_list)) var->write_string(GBS_global_string("%s%s;", disabled_list, id));
        else var->write_string(GBS_global_string(";%s;", id));
        delete disabled_list;
    }
}
// -------------------------------------------
//      void AWT_reactivate_all_advices()
// -------------------------------------------
void AWT_reactivate_all_advices() {
    get_disabled_advices()->write_string("");
}

// -----------------------------------------------------------------------------------------------------------
//      void AWT_advice(const char *message, int type, const char *title, const char *corresponding_help)
// -----------------------------------------------------------------------------------------------------------
void AWT_advice(const char *message, int type, const char *title, const char *corresponding_help) {
    awt_assert(initialized);
    size_t  message_len = strlen(message); awt_assert(message_len>0);
    long    crc32       = GB_checksum(message, message_len, true, " .,-!"); // checksum is used to test if advice was shown
    char   *advice_id   = GB_strdup(GBS_global_string("%lx", crc32));

    bool show_advice = !advice_disabled(advice_id);

    if (show_advice) {
        AW_awar *understood = advice_root->awar(AWAR_ADVICE_UNDERSTOOD);
        understood->write_int(0);

        if (corresponding_help) type = AWT_Advice_Type(type|AWT_ADVICE_HELP);
        else awt_assert((type & AWT_ADVICE_HELP) == 0);

        AW_window_simple *aws = new AW_window_simple; // do not delete (ARB will crash)

        if (!title) title = "Please read carefully";
        aws->init(advice_root, "Advice", GBS_global_string("ARB: %s", title));
        aws->load_xfig("awt/advice.fig");

        bool has_help     = type & AWT_ADVICE_HELP;
        bool help_pops_up = false;

        if (has_help) {
            aws->callback( AW_POPUP_HELP,(AW_CL)corresponding_help);
            aws->at("help");
            aws->create_button("HELP","HELP","H");

            if (type & AWT_ADVICE_HELP_POPUP) help_pops_up = true;
        }

        aws->at("advice");
        aws->create_text_field(AWAR_ADVICE_TEXT);

        advice_root->awar(AWAR_ADVICE_TEXT)
            ->write_string(has_help && !help_pops_up
                           ? GBS_global_string("%s\n\nPlease refer to 'HELP' for more info.", message)
                           : message);

        if (help_pops_up) AW_POPUP_HELP(aws, (AW_CL)corresponding_help);

        if (type & AWT_ADVICE_TOGGLE) {
            aws->label("Do not advice me again");
            aws->at("understood");
            aws->create_toggle(AWAR_ADVICE_UNDERSTOOD);
        }

        aws->at("ok");
        if (type & AWT_ADVICE_TOGGLE) {
            aws->callback(advice_close_cb);
            aws->create_button("OK","OK","O");
        }
        else {
            aws->callback(advice_hide_and_close_cb);
            aws->create_autosize_button("OK","I understand","O", 2);
        }

        aws->window_fit();

        // ----------------------------------------
        //      show window and wait for answer
        // ----------------------------------------

        //     aws->show_grabbed();
        aws->show();

        const char *dummy    = "";
        awt_advice_cb_result = dummy;

        advice_root->add_timed_callback(AWT_ADVICE_LISTEN_DELAY, awt_message_timer_listen_event, (AW_CL)aws, 0);
        while (awt_advice_cb_result == dummy) {
            advice_root->process_events(); // wait for answer
        }
        aws->hide();

        if (understood->read_int() == 1) {
            disable_advice(advice_id);
            if (type & AWT_ADVICE_TOGGLE) {
                static bool in_advice = false;
                if (!in_advice) {
                    in_advice = true;
                    AWT_advice("You have disabled an advice.\n"
                               "In order to disable it PERMANENTLY, save properties.", AWT_ADVICE_TOGGLE);
                    in_advice = false;
                }
            }
        }
    }
    free(advice_id);
}

