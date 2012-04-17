// ============================================================= //
//                                                               //
//   File      : AW_macro.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_macro.hxx"
#include "aw_msg.hxx"
#include <arbdbt.h>
#include <arb_file.h>

void RecordingMacro::write_dated_comment(const char *what) const {
    write("# ");
    write(what);
    write(" @ ");
    write(GB_date_string());
    write('\n');
}

void RecordingMacro::warn_unrecordable(const char *what) const {
    aw_message(GBS_global_string("could not record %s", what));
}


RecordingMacro::RecordingMacro(const char *filename, const char *application_id_, const char *stop_action_name_)
    : stop_action_name(strdup(stop_action_name_)),
      application_id(strdup(application_id_)),
      path(NULL),
      out(NULL),
      error(NULL)
{
    path = (filename[0] == '/')
        ? strdup(filename)
        : GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), filename);

    char *macro_header       = GB_read_file(GB_path_in_ARBLIB("macro.head"));
    if (!macro_header) error = GB_await_error();
    else {
        out = fopen(path, "w");

        if (out) {
            write(macro_header);
            write_dated_comment("recording started");
        }
        else error = GB_IO_error("recording to", filename);

        free(macro_header);
    }

    aw_assert(implicated(error, !out));
}

void RecordingMacro::record_action(const char *action_id) {
    aw_assert(out && !error);
    if (!action_id) {
        warn_unrecordable("anonymous GUI element");
    }
    else if (strcmp(action_id, stop_action_name) != 0) { // silently ignore stop-recording button press
        write("BIO::remote_action($gb_main");
        write_quoted_param(application_id);
        write(','); GBS_fwrite_string(action_id, out);
        write(");\n");
    }
}

void RecordingMacro::record_awar_change(AW_awar *awar) {
    aw_assert(out && !error);

    char *svalue = awar->read_as_string();
    if (!svalue) {
        warn_unrecordable(GBS_global_string("change of '%s'", awar->awar_name));
    }
    else {
        write("BIO::remote_awar($gb_main");
        write_quoted_param(application_id);
        write(','); GBS_fwrite_string(awar->awar_name, out);
        write(','); GBS_fwrite_string(svalue, out);
        write(");\n");
    
        free(svalue);
    }
}

GB_ERROR RecordingMacro::stop() {
    if (out) {
        write_dated_comment("recording stopped");
        write("ARB::close($gb_main);\n");
        fclose(out);

        long mode = GB_mode_of_file(path);
        error     = GB_set_mode_of_file(path, mode | ((mode >> 2)& 0111));

        out = 0;
    }
    return error;
}

