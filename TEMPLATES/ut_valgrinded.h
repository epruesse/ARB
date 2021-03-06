// ================================================================ //
//                                                                  //
//   File      : ut_valgrinded.h                                    //
//   Purpose   : wrapper to call subprocesses inside valgrind       //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2011   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef UT_VALGRINDED_H
#define UT_VALGRINDED_H


#ifdef UNIT_TESTS

#ifndef ARB_MSG_H
#include <arb_msg.h>
#endif
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif

#define UTVG_CALL_SEEN "flag.valgrind.callseen"

namespace utvg {

    inline const char *flag_name(const char *name) {
        const char *ARBHOME = getenv("ARBHOME");
        const int   BUFSIZE = 200;
        static char buf[BUFSIZE];

        IF_ASSERTION_USED(int printed =)
            snprintf(buf, BUFSIZE, "%s/UNIT_TESTER/valgrind/%s", ARBHOME, name);
        arb_assert(printed<BUFSIZE);

        return buf;
    }
    inline bool flag_exists(const char *name) {
        const char  *path = flag_name(name);
        struct stat  stt;

        return stat(path, &stt) == 0 && S_ISREG(stt.st_mode);
    }
    inline void raise_flag(const char *name) {
        const char *path = flag_name(name);
        FILE       *fp   = fopen(path, "w");
        arb_assert(fp);
        fclose(fp);
    }

    struct valgrind_info {
        bool wanted;
        bool leaks;
        bool reachable;

        valgrind_info() {
            // The following flag files are generated by ../UNIT_TESTER/Makefile.suite
            // which reads the settings from ../UNIT_TESTER/Makefile.setup.local
            wanted    = flag_exists("flag.valgrind");
            leaks     = flag_exists("flag.valgrind.leaks");
            reachable = flag_exists("flag.valgrind.reachable");
        }
    };

    inline const valgrind_info& get_valgrind_info() {
        static valgrind_info vinfo;
        return vinfo;
    }
};

inline void make_valgrinded_call(char *&command) {
    using namespace utvg;
    const valgrind_info& valgrind = get_valgrind_info();
    if (valgrind.wanted) {
// #define VALGRIND_ONLY_SOME
#if defined(VALGRIND_ONLY_SOME)
        bool perform_valgrind = false;

        perform_valgrind = perform_valgrind || strstr(command, "arb_pt_server");
        perform_valgrind = perform_valgrind || strstr(command, "arb_primer");

        if (!perform_valgrind) return;
#endif

        const char *switches           = valgrind.leaks ? (valgrind.reachable ? "-l -r" : "-l") : "";
        char       *valgrinded_command = GBS_global_string_copy("$ARBHOME/UNIT_TESTER/valgrind/arb_valgrind_logged CALL %s -c 15 %s", switches, command);
        freeset(command, valgrinded_command);

        utvg::raise_flag(UTVG_CALL_SEEN);
    }
}

inline bool will_valgrind_calls() { return utvg::get_valgrind_info().wanted; }
inline bool seen_valgrinded_call() { return utvg::flag_exists(UTVG_CALL_SEEN); }

#else // !UNIT_TESTS

#define make_valgrinded_call(command)
inline bool will_valgrind_calls() { return false; }
inline bool seen_valgrinded_call() { return false; }

#endif // UNIT_TESTS


#else
#error ut_valgrinded.h included twice
#endif // UT_VALGRINDED_H
