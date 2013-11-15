//  ==================================================================== //
//                                                                       //
//    File      : AW_question.cxx                                        //
//    Purpose   :                                                        //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//  ==================================================================== //

#include <arbdb.h>
#include <aw_msg.hxx>
#include <aw_question.hxx>
#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_global.hxx"
#include "aw_window.hxx"
#include "aw_window_Xm.hxx"
#include "aw_advice.hxx"

using namespace std;

#define AWAR_QUESTION "tmp/question"

int aw_question(const char *uniqueID, const char *question, const char *buttons, bool fixedSizeButtons, const char *helpfile) {
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

    AW_root *root = AW_root::SINGLETON;

    char *awar_name_neverAskAgain = NULL;
    int   have_auto_answer        = 0;

    if (uniqueID) {
        GB_ERROR error = GB_check_key(uniqueID);
        if (error) {
            aw_message(error);
            uniqueID = NULL;
        }
        else {
            awar_name_neverAskAgain     = GBS_global_string_copy("answers/%s", uniqueID);
            AW_awar *awar_neverAskAgain = root->awar_int(awar_name_neverAskAgain, 0, AW_ROOT_DEFAULT);
            have_auto_answer            = awar_neverAskAgain->read_int();
        }
    }

    if (have_auto_answer>0) {
        aw_message_cb_result = have_auto_answer-1;
    }
    else {
        char *button_list = strdup(buttons ? buttons : "OK");
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
        char *hindex = GBS_global_string_copy("%s$%s$%zu$%zu$%i$%s",
                                              button_list, uniqueID ? uniqueID : "<NOID>",
                                              question_length, question_lines, int(fixedSizeButtons),
                                              helpfile ? helpfile : "");

        static GB_HASH    *hash_windows = 0;
        if (!hash_windows) hash_windows = GBS_create_hash(256, GB_MIND_CASE);
        AW_window_message *aw_msg       = (AW_window_message *)GBS_read_hash(hash_windows, hindex);

#if defined(DEBUG)
        printf("hindex='%s'\n", hindex);
#endif // DEBUG

        if (!aw_msg) {
            aw_msg = new AW_window_message;
            GBS_write_hash(hash_windows, hindex, (long)aw_msg);

            aw_msg->init(root, "QUESTION BOX", false);
            aw_msg->recalc_size_atShow(AW_RESIZE_DEFAULT); // force size recalc (ignores user size)

            aw_msg->label_length(10);

            aw_msg->at(10, 10);
            aw_msg->auto_space(10, 10);

            aw_msg->button_length(question_length+3);
            aw_msg->button_height(question_lines+1);

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

                aw_msg->button_length(max_button_length+2);
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
                        aw_msg->callback(makeHelpCallback(helpfile));
                        aw_msg->create_button("HELP", "HELP", "H");
                        help_button_done = true;
                    }
                    aw_msg->at_newline();
                    ++ret;
                }
                if (strcmp(ret, "EXIT") == 0) {
                    aw_msg->callback(makeWindowCallback(message_cb, -1));
                }
                else {
                    aw_msg->callback(makeWindowCallback(message_cb, counter++));
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
                aw_msg->callback(makeHelpCallback(helpfile));
                aw_msg->create_button("HELP", "HELP", "H");
                help_button_done = true;
            }

            if (uniqueID) {
                aw_msg->at_newline();
                const char *label = counter>1 ? "Never ask again" : "Never notify me again";
                aw_msg->label_length(strlen(label));
                aw_msg->label(label);
                aw_msg->create_toggle(awar_name_neverAskAgain);
            }

            aw_msg->window_fit();
        }
        else {
#if defined(DEBUG)
            printf("[Reusing existing aw_question-window]\n");
#endif
        }
        free(hindex);
        aw_msg->show_modal();

        free(button_list);
        aw_message_cb_result = -13;

#if defined(TRACE_STATUS_MORE)
        fprintf(stderr, "add aw_message_timer_listen_event with delay = %i\n", AW_MESSAGE_LISTEN_DELAY); fflush(stdout);
#endif // TRACE_STATUS_MORE
        root->add_timed_callback_never_disabled(AW_MESSAGE_LISTEN_DELAY, makeTimedCallback(aw_message_timer_listen_event, static_cast<AW_window*>(aw_msg)));

        {
            LocallyModify<bool> flag(root->disable_callbacks, true);
            while (aw_message_cb_result == -13) {
                root->process_events();
            }
        }
        aw_msg->hide();

        if (awar_name_neverAskAgain) {
            AW_awar *awar_neverAskAgain = root->awar(awar_name_neverAskAgain);

            if (awar_neverAskAgain->read_int()) { // user checked "Never ask again"
                int givenAnswer = aw_message_cb_result >= 0 ? aw_message_cb_result+1 : 0;
                awar_neverAskAgain->write_int(givenAnswer); // store given answer for "never asking again"

                if (givenAnswer && strchr(buttons, ',') != 0) {
                    const char *appname = root->program_name;
                    char       *advice  = GBS_global_string_copy("You will not be asked that question again in this session.\n"
                                                                 "%s will always assume the answer you just gave.\n"
                                                                 "\n"
                                                                 "When you restart %s that question will be asked again.\n"
                                                                 "To disable that question permanently for future sessions,\n"
                                                                 "you need to save properties.\n"
                                                                 "\n"
                                                                 "Depending on the type of question doing that might be\n"
                                                                 "helpful or obstructive.\n"
                                                                 "Disabled questions can be reactivated from the properties menu.\n",
                                                                 appname, appname);
                    AW_advice(advice, AW_ADVICE_TOGGLE, "Disabling questions", NULL);
                    free(advice);
                }
            }
        }
    }

    free(awar_name_neverAskAgain);

    switch (aw_message_cb_result) {
        case -1:                // exit with core
            fprintf(stderr, "Core dump requested\n");
            ARB_SIGSEGV(1);
            break;
        case -2:                // exit without core
            exit(-1);
            break;
    }
    return aw_message_cb_result;
}

bool aw_ask_sure(const char *uniqueID, const char *msg) {
    return aw_question(uniqueID, msg, "Yes,No", true, NULL) == 0;
}
void aw_popup_ok(const char *msg) {
    aw_question(NULL, msg, "Ok", true, NULL);
}
void aw_popup_exit(const char *msg) {
    aw_question(NULL, msg, "EXIT", true, NULL);
    aw_assert(0); // should not be reached
    exit(EXIT_FAILURE);
}

void AW_reactivate_all_questions(AW_window*) {
    GB_transaction  ta(AW_ROOT_DEFAULT);
    GBDATA         *gb_neverAskedAgain = GB_search(AW_ROOT_DEFAULT, "answers", GB_FIND);
    const char     *msg                = "No questions were disabled yet.";

    if (gb_neverAskedAgain) {
        int reactivated = 0;
        for (GBDATA *gb_q = GB_child(gb_neverAskedAgain); gb_q; gb_q = GB_nextChild(gb_q)) {
            if (GB_read_int(gb_q)) {
                GB_write_int(gb_q, 0);
                reactivated++;
            }
        }
        if (reactivated) {
            msg = GBS_global_string("Reactivated %i questions (for this session)\n"
                                    "To reactivate them for future sessions, save properties.",
                                    reactivated);
        }
    }
    aw_message(msg);
}


void AW_repeated_question::add_help(const char *help_file) {
    freedup(helpfile, help_file);
}

int AW_repeated_question::get_answer(const char *uniqueID, const char *question, const char *buttons, const char *to_all, bool add_abort)
{
    if (!buttons_used) {
        buttons_used = strdup(buttons);
    }
    else {
        // do not use the same instance of AW_repeated_question with different buttons!
        assert_or_exit(strcmp(buttons_used, buttons) == 0);
    }

    if (answer == -1 || !dont_ask_again) {

        char   *all             = GBS_global_string_copy(" (%s)", to_all);
        int     all_len         = strlen(all);
        size_t  but_len         = strlen(buttons);
        size_t  new_buttons_len = but_len*3+1+(add_abort ? 6 : 0)+all_len*3;
        char   *new_buttons     = (char*)malloc(new_buttons_len);
        int     button_count    = 0; // number of buttons in 'buttons'

        { // transform "YES,NO"  ->   "YES,YES (to_all),^NO,NO (to_all)" or "YES (to_all),NO (to_all)"
            char       *w       = new_buttons;
            const char *r       = buttons;

            while (1) {
                const char *comma = strchr(r, ',');
                if (!comma) comma = strchr(r, 0);
                int         len   = comma-r;

                if (!dont_ask_again) {
                    if (w>new_buttons) *w++ = '^'; // not in front of first button
                    memcpy(w, r, len); w += len;
                    *w++ = ',';
                }
                memcpy(w, r, len); w       += len;
                memcpy(w, all, all_len); w += all_len;
                *w++ = ',';

                button_count++;

                if (!comma[0]) break;
                r = comma+1;
            }
            if (add_abort) {
                const char *abort      = "^ABORT";
                strcpy(w, abort); w += strlen(abort);
            }
            else {
                --w; // delete comma at end
            }
            w[0] = 0;

            aw_assert(size_t(w-new_buttons) < new_buttons_len); // oops buffer overflow

            free(all);
        }

        int user_answer = aw_question(uniqueID, question, new_buttons, true, helpfile);

        if (dont_ask_again) {   // ask question as normal when called first (dont_ask_again later)
            answer = user_answer;
        }
        else {
            answer         = user_answer/2;
            dont_ask_again = (user_answer%2) || (user_answer == (button_count*2));
        }

        free(new_buttons);

        aw_assert(answer<(button_count+(add_abort ? 1 : 0)));
    }

    aw_assert(answer != -1);

    return answer;
}

