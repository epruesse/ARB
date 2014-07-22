// ============================================================= //
//                                                               //
//   File      : recmac.hxx                                      //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AW_MACRO_HXX
#define AW_MACRO_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef AW_AWAR_HXX
#include "aw_awar.hxx"
#endif

#define ma_assert(bed) arb_assert(bed)

class RecordingMacro : virtual Noncopyable {
    char *stop_action_name;
    char *application_id;

    char *path;
    FILE *out;

    GB_ERROR error;

    void write(char ch) const { fputc(ch, out); }
    void write(const char *text) const { fputs(text, out); }

    void write_as_perl_string(const char *value) const;
    void write_dated_comment(const char *what) const;

    void flush() const { fflush(out); }

    void post_process();

public:
    RecordingMacro(const char *filename, const char *application_id_, const char *stop_action_name_, bool expand_existing);
    ~RecordingMacro() {
        ma_assert(!out); // forgot to call stop()
        free(path);
        free(application_id);
        free(stop_action_name);
    }

    GB_ERROR has_error() const { return error; }

    void track_action(const char *action_id);
    void track_awar_change(AW_awar *awar);

    void write_action(const char *app_id, const char *action_name);
    void write_awar_change(const char *app_id, const char *awar_name, const char *content);

    GB_ERROR stop();
};

void warn_unrecordable(const char *what);

#else
#error AW_macro.hxx included twice
#endif // AW_MACRO_HXX
