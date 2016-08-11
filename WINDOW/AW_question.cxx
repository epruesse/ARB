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
#include "aw_advice.hxx"
#include "aw_dialog.hxx"
#include <gtk/gtk.h>

using namespace std;

int aw_question(const char *unique_id, const char *question, const char *buttons, bool sameSizeButtons, const char *helpfile) {
    /*! Ask the user a question. Blocks all UI input until the answer is provided.
     *
     * @param  unique_id       Unique ID to identify the question. Must be valid hkey.
     * @param  question        The question.
     * @param  buttons         Comma separated list of button names. A button named starting
     *                         with "^" will begin a new row of buttons. A button named "EXIT"
     *                         will cause abnormal (EXIT_FAILURE) termination of program.
     * @param  sameSizeButtons Make all buttons have the same size.
     * @param  helpfile        Adds a "HELP" button. May be NULL. (currently ignored)
     * @return the index of the selected answer
     */

    // @@@ TODO: add help button
    // @@@ TODO: sameSizeButtons is ignored (check example)

    aw_assert(buttons);

    AW_awar *awar_neverAskAgain = NULL;
    if (unique_id) {
        GB_ERROR error = GB_check_key(unique_id);
        if (error) {
            aw_message(error);
            unique_id = NULL;
        }
        else {
            awar_neverAskAgain = AW_root::SINGLETON->awar_int(GBS_global_string("answers/%s", unique_id), 0, AW_ROOT_DEFAULT);
        }
    }

    int result = awar_neverAskAgain ? awar_neverAskAgain->read_int() : 0;
    if (result>0) { // have auto-answer
        --result;
    }
    else { // no auto-answer
        if (!question) question = "No question?! Please report this as a bug.";

        AW_dialog dialog("Question box", question);
        dialog.create_buttons(buttons);

        // create no-repeat checkbox if we have a unique-id
        if (awar_neverAskAgain) {
            dialog.create_toggle(awar_neverAskAgain, "Never show again");
        }

        dialog.run();

        // if we have an awar and no-repeat got checked,
        // store the result and warn user about what he's done.
        if (awar_neverAskAgain && awar_neverAskAgain->read_int()) {
            int answerCode = dialog.get_result() >= 0 ? dialog.get_result()+1 : 0;
            awar_neverAskAgain->write_int(answerCode);

            if (answerCode>0) {
                const char *appname = AW_root::SINGLETON->program_name;
                char       *advice  = GBS_global_string_copy
                    ("You will not be asked that question again in this session.\n"
                     "%s will always assume the answer you just gave.\n"
                     "\n"
                     "After restarting %s that question will be asked again.\n"
                     "To disable that question permanently for future sessions,\n"
                     "you need to save properties.\n"
                     "\n"
                     "Depending on the type of question, disabling it might be\n"
                     "helpful or obstructive.\n"
                     "Disabled questions can be reactivated from the properties menu.\n",
                     appname, appname);

                AW_advice(advice, AW_ADVICE_TOGGLE, "Disabling questions", "questions.hlp");
                free(advice);
            }
        }

        result = dialog.get_result();
    }

    // @@@ missing terminate behavior?

    return result;
}

bool aw_ask_sure(const char *unique_id, const char *msg) {
    /*!
     * pop up a modal yes/no question
     * @param  unique_id If given, the dialog will get an "do not show again" checkbox
     * @param  msg       The question.
     * @return True if the answer was "Yes"
     */
    return aw_question(unique_id, msg, "Yes,No", true, NULL) == 0;
}

void aw_popup_ok(const char *msg) {
    /*!
     * Pop up a modal message with an Ok button
     * @param msg The message.
     */
    aw_question(NULL, msg, "Ok", true, NULL);
}

__ATTR__NORETURN void aw_popup_exit(const char *msg) {
    /*!
     * Pop up a modal message with an Exit button.
     * Won't return but exit with "EXIT_FAILURE"
     */
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

int AW_repeated_question::get_answer(const char *unique_id, const char *question, const char *buttons, const char *to_all, bool add_abort)
{
    if (!buttons_used) {
        buttons_used = ARB_strdup(buttons);
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
        char   *new_buttons     = ARB_alloc<char>(new_buttons_len);
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

        int user_answer = aw_question(unique_id, question, new_buttons, true, helpfile);

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

