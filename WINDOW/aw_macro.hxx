// ============================================================= //
//                                                               //
//   File      : aw_macro.hxx                                    //
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

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

class RecordingMacro : virtual Noncopyable {
    char *stop_action_name;
    char *application_id;

    char *path;
    FILE *out;

    GB_ERROR error;

    void write(char ch) const { fputc(ch, out); }
    void write(const char *text) const { fputs(text, out); }
    void write_quoted_param(const char *value) const { write(",\""); write(value); write('\"'); }

    void write_dated_comment(const char *what) const;
    void warn_unrecordable(const char *what) const;

    void flush() const { fflush(out); }

public:
    RecordingMacro(const char *filename, const char *application_id_, const char *stop_action_name_);
    ~RecordingMacro() {
        aw_assert(!out); // forgot to call stop()
        free(path);
        free(application_id);
        free(stop_action_name);
    }

    GB_ERROR has_error() const { return error; }

    void record_action(const char *action_id);
    void record_awar_change(AW_awar *awar);

    GB_ERROR stop();
};

#else
#error AW_macro.hxx included twice
#endif // AW_MACRO_HXX
