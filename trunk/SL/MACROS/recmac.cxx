// ============================================================= //
//                                                               //
//   File      : recmac.cxx                                      //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "recmac.hxx"
#include "macros_local.hxx"

#include <arbdbt.h>

#include <arb_file.h>
#include <arb_defs.h>
#include <arb_diff.h>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <FileContent.h>

#include <cctype>
#include <arb_str.h>
#include <aw_file.hxx>

void warn_unrecordable(const char *what) {
    aw_message(GBS_global_string("could not record %s", what));
}

void RecordingMacro::write_dated_comment(const char *what) const {
    write("# ");
    write(what);
    write(" @ ");
    write(GB_date_string());
    write('\n');
}

RecordingMacro::RecordingMacro(const char *filename, const char *application_id_, const char *stop_action_name_, bool expand_existing)
    : stop_action_name(strdup(stop_action_name_)),
      application_id(strdup(application_id_)),
      path(NULL),
      out(NULL),
      error(NULL)
{
    path = (filename[0] == '/')
        ? strdup(filename)
        : GBS_global_string_copy("%s/%s", GB_getenvARBMACROHOME(), filename);

    if (expand_existing && !GB_is_readablefile(path)) {
        error = GBS_global_string("Can only expand existing macros (no such file: %s)", path);
    }

    if (!error) {
        char *content = NULL;
        {
            const char *from    = expand_existing ? path : GB_path_in_ARBLIB("macro.head");
            content             = GB_read_file(from);
            if (!content) error = GB_await_error();
            else {
                if (expand_existing) {
                    // cut off end of macro
                    char *close = strstr(content, "ARB::close");
                    if (close) close[0] = 0;
                }
            }
        }

        if (!error) {
            out = fopen(path, "w");

            if (out) {
                write(content);
                write_dated_comment(expand_existing ? "recording resumed" : "recording started");
                flush();
            }
            else error = GB_IO_error("recording to", filename);
        }

        free(content);
        ma_assert(implicated(error, !out));
    }
}

void RecordingMacro::write_as_perl_string(const char *value) const {
    const char SQUOTE = '\'';
    write(SQUOTE);
    for (int i = 0; value[i]; ++i) {
        char c = value[i];
        if (c == SQUOTE) {
            write('\\');
            write(SQUOTE);
        }
        else {
            write(c);
        }
    }
    write(SQUOTE);
}

void RecordingMacro::write_action(const char *app_id, const char *action_name) {
    bool handled = false;

    // Recording "macro-execution" as GUI-clicks caused multiple macros running asynchronously (see #455)
    // Instead of recording GUI-clicks, macros are called directly:
    static const char *MACRO_ACTION_START = MACRO_WINDOW_ID "/";
    if (ARB_strBeginsWith(action_name, MACRO_ACTION_START)) {
        static int  MACRO_START_LEN = strlen(MACRO_ACTION_START);
        const char *sub_action      = action_name+MACRO_START_LEN;

        int playbackType = 0;
        if      (strcmp(sub_action, MACRO_PLAYBACK_ID)        == 0) playbackType = 1;
        else if (strcmp(sub_action, MACRO_PLAYBACK_MARKED_ID) == 0) playbackType = 2;

        if (playbackType) {
            char       *macroFullname = AW_get_selected_fullname(AW_root::SINGLETON, AWAR_MACRO_BASE);
            const char *macroName     = GBT_relativeMacroname(macroFullname); // points into macroFullname

            write("BIO::macro_execute(");
            write_as_perl_string(macroName); // use relative macro name (allows to share macros between users)
            write(", ");
            write('0'+(playbackType-1));
            write(", 0);\n"); // never run asynchronously (otherwise (rest of) current and called macro will interfere)
            flush();

            free(macroFullname);

            handled = true;
        }
    }

    // otherwise "normal" operation (=trigger GUI element)
    if (!handled) {
        write("BIO::remote_action($gb_main");
        write(','); write_as_perl_string(app_id);
        write(','); write_as_perl_string(action_name);
        write(");\n");
    }
    flush();
}
void RecordingMacro::write_awar_change(const char *app_id, const char *awar_name, const char *content) {
    write("BIO::remote_awar($gb_main");
    write(','); write_as_perl_string(app_id);
    write(','); write_as_perl_string(awar_name);
    write(','); write_as_perl_string(content);
    write(");\n");
    flush();
}


void RecordingMacro::track_action(const char *action_id) {
    ma_assert(out && !error);
    if (!action_id) {
        warn_unrecordable("anonymous GUI element");
    }
    else if (strcmp(action_id, stop_action_name) != 0) { // silently ignore stop-recording button press
        write_action(application_id, action_id);
    }
}

void RecordingMacro::track_awar_change(AW_awar *awar) {
    // see also trackers.cxx@AWAR_CHANGE_TRACKING

    ma_assert(out && !error);

    char *svalue = awar->read_as_string();
    if (!svalue) {
        warn_unrecordable(GBS_global_string("change of '%s'", awar->awar_name));
    }
    else {
        write_awar_change(application_id, awar->awar_name, svalue);
        free(svalue);
    }
}

GB_ERROR RecordingMacro::stop() {
    if (out) {
        write_dated_comment("recording stopped");
        write("ARB::close($gb_main);\n");
        fclose(out);

        post_process();

        long mode = GB_mode_of_file(path);
        error     = GB_set_mode_of_file(path, mode | ((mode >> 2)& 0111));

        out = 0;
    }
    return error;
}

// -------------------------
//      post processing

inline const char *closing_quote(const char *str, char qchar) {
    const char *found = strchr(str, qchar);
    if (found>str) {
        if (found[-1] == '\\') { // escaped -> search behind
            return closing_quote(found+1, qchar);
        }
    }
    return found;
}

inline char *parse_quoted_string(const char *& line) {
    // read '"string"' from start of line.
    // return 'string'.
    // skips spaces.

    while (isspace(line[0])) ++line;
    if (line[0] == '\"' || line[0] == '\'') {
        const char *other_quote = closing_quote(line+1, line[0]);
        if (other_quote) {
            char *str = GB_strpartdup(line+1, other_quote-1);
            line      = other_quote+1;
            while (isspace(line[0])) ++line;
            return str;
        }
    }
    return NULL;
}

inline char *modifies_awar(const char *line, char *& app_id) {
    // return awar_name, if line modifies an awar.
    // return NULL otherwise
    //
    // if 'app_id' is NULL, it'll be set to found application id.
    // otherwise it'll be checked against found id. function returns NULL on mimatch.

    while (isspace(line[0])) ++line;

    const char cmd[]  = "BIO::remote_awar($gb_main,";
    const int cmd_len = ARRAY_ELEMS(cmd)-1;

    if (strncmp(line, cmd, cmd_len) == 0) {
        line     += cmd_len;
        char *id  = parse_quoted_string(line);
        if (app_id) {
            bool app_id_differs = strcmp(app_id, id) != 0;
            free(id);
            if (app_id_differs) return NULL;
        }
        else {
            app_id = id;
        }
        if (line[0] == ',') {
            ++line;
            char *awar = parse_quoted_string(line);
            return awar;
        }
    }
    return NULL;
}

inline bool opens_macro_dialog(const char *line) {
    // return true, if the macro-command in 'line' opens the macro dialog
    return strcmp(line, "BIO::remote_action($gb_main,\'ARB_NT\',\'macros\');") == 0;
}
inline bool is_end_of_macro(const char *line) {
    // return true, if the macro-command in 'line' belongs to code at end (of any macro)
    return strcmp(line, "ARB::close($gb_main);") == 0;
}

inline bool is_comment(const char *line) {
    int i = 0;
    while (isspace(line[i])) ++i;
    return line[i] == '#';
}

void RecordingMacro::post_process() {
    ma_assert(!error);

    FileContent macro(path);
    error = macro.has_error();
    if (!error) {
        StrArray& line = macro.lines();

        // remove duplicate awar-changes
        for (size_t i = 0; i<line.size(); ++i) {
            char *app_id   = NULL;
            char *mod_awar = modifies_awar(line[i], app_id);
            if (mod_awar) {
                for (size_t n = i+1; n<line.size(); ++n) {
                    if (!is_comment(line[n])) {
                        char *mod_next_awar = modifies_awar(line[n], app_id);
                        if (mod_next_awar) {
                            if (strcmp(mod_awar, mod_next_awar) == 0) {
                                // seen two lines (i and n) which modify the same awar
                                // -> remove the 1st line
                                line.remove(i);

                                // make sure that it also works for 3 or more consecutive modifications
                                ma_assert(i>0);
                                i--;
                            }
                            free(mod_next_awar);
                        }
                        break;
                    }
                }
                free(mod_awar);
            }
            else if (opens_macro_dialog(line[i])) {
                bool isLastCommand = true;
                for (size_t n = i+1; n<line.size() && isLastCommand; ++n) {
                    if (!is_comment(line[n]) && !is_end_of_macro(line[n])) {
                        isLastCommand = false;
                    }
                }
                if (isLastCommand) {
                    free(line.replace(i, GBS_global_string_copy("# %s", line[i])));
                }
            }
            free(app_id);
        }
        error = macro.save();
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define TEST_PARSE_QUOTED_STRING(in,res_exp,out_exp) do {       \
        const char *line = (in);                                \
        char *res =parse_quoted_string(line);                   \
        TEST_EXPECT_EQUAL(res, res_exp);                        \
        TEST_EXPECT_EQUAL(line, out_exp);                       \
        free(res);                                              \
    } while(0)

#define TEST_MODIFIES_AWAR(cmd,app_exp,awar_exp,app_in) do {    \
        char *app  = app_in;                                    \
        char *awar = modifies_awar(cmd, app);                   \
        TEST_EXPECTATION(all().of(that(awar).is_equal_to(awar_exp),  \
                             that(app).is_equal_to(app_exp)));  \
        free(awar);                                             \
        free(app);                                              \
    } while(0)

void TEST_parse() {
    const char *null = NULL;
    TEST_PARSE_QUOTED_STRING("", null, "");
    TEST_PARSE_QUOTED_STRING("\"str\"", "str", "");
    TEST_PARSE_QUOTED_STRING("\"part\", rest", "part", ", rest");
    TEST_PARSE_QUOTED_STRING("\"\"", "", "");
    TEST_PARSE_QUOTED_STRING("\"\"rest", "", "rest");
    TEST_PARSE_QUOTED_STRING("\"unmatched", null, "\"unmatched");

    TEST_MODIFIES_AWAR("# BIO::remote_awar($gb_main,\"app\", \"awar_name\", \"value\");", null, null, NULL);
    TEST_MODIFIES_AWAR("BIO::remote_awar($gb_main,\"app\", \"awar_name\", \"value\");", "app", "awar_name", NULL);
    TEST_MODIFIES_AWAR("BIO::remote_awar($gb_main,\"app\", \"awar_name\", \"value\");", "app", "awar_name", strdup("app"));
    TEST_MODIFIES_AWAR("BIO::remote_awar($gb_main,\"app\", \"awar_name\", \"value\");", "diff", null, strdup("diff"));

    TEST_MODIFIES_AWAR("   \t BIO::remote_awar($gb_main,\"app\", \"awar_name\", \"value\");", "app", "awar_name", NULL);
}

#define RUN_TOOL_NEVER_VALGRIND(cmdline)      GBK_system(cmdline)
#define TEST_RUN_TOOL_NEVER_VALGRIND(cmdline) TEST_EXPECT_NO_ERROR(RUN_TOOL_NEVER_VALGRIND(cmdline))

void TEST_post_process() {
    // ../../UNIT_TESTER/run/general
    const char *source   = "general/pp.amc";
    const char *dest     = "general/pp_out.amc";
    const char *expected = "general/pp_exp.amc";

    TEST_RUN_TOOL_NEVER_VALGRIND(GBS_global_string("cp %s %s", source, dest));

    char *fulldest = strdup(GB_path_in_ARBHOME(GB_concat_path("UNIT_TESTER/run", dest)));
    TEST_EXPECT(GB_is_readablefile(fulldest));

    {
        RecordingMacro recording(fulldest, "whatever", "whatever", true);

        TEST_EXPECT_NO_ERROR(recording.has_error());
        TEST_EXPECT_NO_ERROR(recording.stop()); // triggers post_process
    }

    TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(dest, expected, 0);
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(dest));

    free(fulldest);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
