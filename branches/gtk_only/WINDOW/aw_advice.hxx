//  ==================================================================== //
//                                                                       //
//    File      : aw_advice.hxx                                          //
//    Purpose   : general user advices                                   //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2002              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AW_ADVICE_HXX
#define AW_ADVICE_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif


//! define type of @ref AW_advice by or-ing values
enum AW_Advice_Type {
    AW_ADVICE_SIMPLE     = 0,                       // nothing of the following
    AW_ADVICE_TOGGLE     = 1,                       // visible toggle to switch off advice (otherwise advice appears only once)
    AW_ADVICE_HELP       = 2,                       // advice has corresponding help file
    AW_ADVICE_HELP_POPUP = 4,                       // this helpfile should popup immediately

    // convenience defs:
    AW_ADVICE_TOGGLE_AND_HELP = AW_ADVICE_TOGGLE|AW_ADVICE_HELP,
};

//! has to be called one time (before calling AW_advice)
void init_Advisor(AW_root *awr);

/*! @brief Show a message box with an advice for the user
    @param message the text shown to the user
    @param title window title
    @param type type of advice
    @param corresponding_help name of corresponding ARB-helpfile
    @see init_Advisor
*/
void AW_advice(const char *message,
               AW_Advice_Type type = AW_ADVICE_SIMPLE,
               const char *title = 0,
               const char *corresponding_help = 0);

//! reactivates all advices which were disabled by the user
void AW_reactivate_all_advices(AW_window*);


#else
#error aw_advice.hxx included twice
#endif // AW_ADVICE_HXX

