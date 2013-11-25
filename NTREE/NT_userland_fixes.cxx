// ================================================================ //
//                                                                  //
//   File      : NT_userland_fixes.cxx                              //
//   Purpose   : Repair situations caused by ARB bugs in userland   //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2013   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_msg.hxx>

#include <arbdb.h>
#include <arb_strarray.h>
#include <arb_file.h>

#include <time.h>
#include <vector>
#include <string>

using namespace std;

typedef void (*fixfun)();

const long DAYS   = 24*60*60L;
const long WEEKS  = 7 * DAYS;
const long MONTHS = 30 * DAYS;
const long YEARS  = 365 * DAYS;

class UserlandCheck {
    vector<fixfun> fixes;
    time_t         now;
public:

    UserlandCheck() { time(&now); }
    void Register(fixfun fix, long IF_DEBUG(addedAt), long IF_DEBUG(expire)) {
        fixes.push_back(fix);
#if defined(DEBUG)
        if (addedAt+expire < now) {
            aw_message(GBS_global_string("Warning: UserlandCheck #%zu has expired\n", fixes.size()));
        }
#endif
    }
    void Run() {
        for (vector<fixfun>::iterator f = fixes.begin(); f != fixes.end(); ++f) {
            (*f)();
        }
    }
};

// ------------------------------------------------------------

static const char *changeDir(const char *file, const char *newDir) {
    char *name;
    GB_split_full_path(file, NULL, &name, NULL, NULL);

    const char *changed = GB_concat_path(newDir, name);
    free(name);
    return changed;
}

static void correct_wrong_placed_files(const char *filekind, const char *wrongDir, string rightDir, const char *ext) {
    // move files with extension 'ext' from 'wrongDir' into 'rightDir'
    StrArray wrong_placed_files;
    GBS_read_dir(wrong_placed_files, wrongDir, ext);

    size_t wrong_found = wrong_placed_files.size();
    if (wrong_found>0) {
        string failed = "";
        for (size_t i = 0; i<wrong_found; ++i) {
            const char *src   = wrong_placed_files[i];
            string      dst   = changeDir(src, rightDir.c_str());
            GB_ERROR    error = GB_is_regularfile(dst.c_str()) ? "target exists" : GB_rename_file(src, dst.c_str());

            if (error) {
                if (!failed.empty()) failed += "\n";
                failed = failed+"* "+wrong_placed_files[i]+" (move failed: "+error+")";
            }
        }
        if (!failed.empty()) {
            string msg =
                string("Due to a bug, ARB stored ")+filekind+"s in '"+wrongDir+"'\n"+
                "The attempt to move these wrong placed macros into the\n"+
                "correct directory '"+rightDir+"'\n"+
                "has failed. Please move the following files manually:\n"+
                failed;

            aw_message(msg.c_str());
        }
    }
}

static void fix_config_and_macros_in_props() {
    // macros and configs were save in wrong directory
    // caused by: http://bugs.arb-home.de/ticket/425
    // active for: 3 weeks
    char *props = strdup(GB_getenvARB_PROP());
    correct_wrong_placed_files("macro",  props, GB_getenvARBMACROHOME(), "*.amc");
    correct_wrong_placed_files("config", props, GB_getenvARBCONFIG(),    "*.arbcfg");
    free(props);
}

void NT_repair_userland_problems() {
    // place to repair problems caused by earlier bugs

    UserlandCheck checks;
    // generate timestamp: date "--date=2013/09/20" "+%s"
    //                     date +%s
    checks.Register(fix_config_and_macros_in_props, 1385141688 /* = 2013/11/22 */, 1 * YEARS);
    checks.Run();
}

