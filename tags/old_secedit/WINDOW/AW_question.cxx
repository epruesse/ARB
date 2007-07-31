//  ==================================================================== //
//                                                                       //
//    File      : aw_question.cpp                                        //
//    Purpose   :                                                        //
//    Time-stamp: <Thu Mar/11/2004 13:39 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <stdio.h>
#include <string.h>
#include <arbdb.h>
#include <aw_question.hxx>

using namespace std;


// start of implementation of class AW_repeated_question:

void AW_repeated_question::add_help(const char *help_file) {
    if (helpfile) {
        free(helpfile);
        helpfile = 0;
    }
    if (help_file) {
        helpfile = strdup(help_file);
    }
}

int AW_repeated_question::get_answer(const char *question, const char *buttons, const char *to_all, bool add_abort)
{
    if (!buttons_used) {
        buttons_used = strdup(buttons);
    }
    else {
        if (strcmp(buttons_used, buttons) != 0) {
            GB_CORE; // Do not use the same instance of AW_repeated_question with different buttons
        }
    }

    if (answer == -1 || dont_ask_again == false) {

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
                const char *komma = strchr(r, ',');
                if (!komma) komma = strchr(r, 0);
                int         len   = komma-r;

                if (!dont_ask_again) {
                    if (w>new_buttons) *w++ = '^'; // not in front of first button
                    memcpy(w, r, len); w += len;
                    *w++ = ',';
                }
                memcpy(w, r, len); w       += len;
                memcpy(w, all, all_len); w += all_len;
                *w++ = ',';

                button_count++;

                if (!komma[0]) break;
                r = komma+1;
            }
            if (add_abort) {
                const char *abort      = "^ABORT";
                strcpy(w, abort); w += strlen(abort);
            }
            else {
                --w; // delete komma at end
            }
            w[0] = 0;

            aw_assert(size_t(w-new_buttons) < new_buttons_len); // oops buffer overflow

            free(all);
        }

        int user_answer = aw_message(question, new_buttons, true, helpfile);

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

// -end- of implementation of class AW_repeated_question.

