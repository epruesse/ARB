//  ==================================================================== //
//                                                                       //
//    File      : awt_advice.hxx                                         //
//    Purpose   : general user advices                                   //
//    Time-stamp: <Tue Dec/21/2004 16:56 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2002              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_ADVICE_HXX
#define AWT_ADVICE_HXX

class AW_root;

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif


/// define type of @ref AWT_advice by or-ing values
enum AWT_Advice_Type {
    AWT_ADVICE_SIMPLE     = 0,  // nothing of the following
    AWT_ADVICE_TOGGLE     = 1,  // visible toggle to switch off advice (otherwise advice appears only once)
    AWT_ADVICE_HELP       = 2,  // advice has corresponding help file
    AWT_ADVICE_HELP_POPUP = 4, // this helpfile should popup immidiately

} ;

/// has to be called one time (before calling AWT_advice)
void init_Advisor(AW_root *awr, AW_default def);

/** @brief Show a message box with an advice for the user
    @param message the text shown to the user
    @param title window title
    @param type type of advice
    @param corresponding_help name of corresponding ARB-helpfile
    @see init_Advisor
*/
void AWT_advice(const char *message,
                int type = AWT_ADVICE_SIMPLE,
                const char *title = 0,
                const char *corresponding_help = 0);

/// reactivates all advices which were disabled by the user
void AWT_reactivate_all_advices();


#else
#error awt_advice.hxx included twice
#endif // AWT_ADVICE_HXX

