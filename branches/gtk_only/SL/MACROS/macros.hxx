// ============================================================= //
//                                                               //
//   File      : macros.hxx                                      //
//   Purpose   : macro interface                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MACROS_HXX
#define MACROS_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

class RecordingMacro;

class MacroRecorder : public UserActionTracker {
    RecordingMacro *recording;
public:
    GB_ERROR start_recording(const char *file, const char *application_id, const char *stop_action_name, bool expand_existing);
    GB_ERROR stop_recording();
    GB_ERROR execute(GBDATA *gb_main, const char *file, AW_RCB1 execution_done_cb, AW_CL client_data);

    void track_action(const char *action_id) OVERRIDE;
    void track_awar_change(AW_awar *awar) OVERRIDE;
};

#else
#error macros.hxx included twice
#endif // MACROS_HXX
