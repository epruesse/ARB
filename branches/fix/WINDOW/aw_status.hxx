// ================================================================= //
//                                                                   //
//   File      : aw_status.hxx                                       //
//   Purpose   : Provide AW_status and related functions             //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_STATUS_HXX
#define AW_STATUS_HXX

bool AW_status(const char *text); // return 1 if exit button is pressed + set statustext
bool AW_status(double gauge);     // return 1 if exit button is pressed + set progress bar
bool AW_status();                 // return 1 if exit button is pressed

bool aw_status_title(const char *text); // return 1 if exit button is pressed + set status title

// ---------------------
//      progress bar

void aw_openstatus(const char *title); // show status
void aw_closestatus();                 // hide status

// special for EDIT4 (obsolete - nobody listens to EDIT4-errors)
void aw_clear_message_cb(AW_window *aww);

#else
#error aw_status.hxx included twice
#endif // AW_STATUS_HXX
