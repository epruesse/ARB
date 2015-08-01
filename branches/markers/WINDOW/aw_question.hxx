//  ==================================================================== //
//                                                                       //
//    File      : aw_question.hxx                                        //
//    Purpose   : Functions to ask questions to user                     //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AW_QUESTION_HXX
#define AW_QUESTION_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

// if you ask the same question in a loop, it is recommended to use AW_repeated_question
// to avoid asking the same question again and again.
//
// Usage : 1. Create a new instance of AW_repeated_question outside the loop
//         2. call get_answer() inside the loop

// The second way to get rid of unwanted questions is by using a 'unique_id'
// - if unique_id is set, a toggle "Never ask me again" will be added to the question-popup.
// - if unique_id is NULL, no such toggle will appear (i.e. the popup can NOT be suppressed!)
// The latter is recommended
// - whenever the buttons get generated dynamically
// - when not asking is REALLY dangerous
//
// Whenever you change the meaning of a question, it is mandatory that you change the unique_id!!!

class AW_repeated_question : virtual Noncopyable {
    int   answer;
    bool  dont_ask_again;
    char *buttons_used;
    char *helpfile;

public:
    AW_repeated_question()
        : answer(0),
          dont_ask_again(false),
          buttons_used(0),
          helpfile(0)
    {}
    virtual ~AW_repeated_question() {
        free(buttons_used);
        free(helpfile);
    }

    void add_help(const char *help_file); // when called, a help button is added to the prompter

    int get_answer(const char *unique_id, const char *question, const char *buttons, const char *to_all, bool add_abort);
    // return 0 for first button, 1 for second button, 2 for third button, ...
    // the single buttons are separated by commas (i.e. "YES,NO")
    // if add_abort is true an 'ABORT' button is added behind the last
};

int  aw_question(const char *unique_id, const char *msg, const char *buttons, bool sameSizeButtons = true, const char *helpfile = 0);
bool aw_ask_sure(const char *unique_id, const char *msg);

// the following functions should only be used in very special cases - please use aw_message if possible!
void aw_popup_ok  (const char *msg);
void aw_popup_exit(const char *msg) __ATTR__NORETURN;

void AW_reactivate_all_questions(AW_window*);

#else
#error aw_question.hxx included twice
#endif // AW_QUESTION_HXX

