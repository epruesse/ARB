//   Coded by Ralf Westram (coder@reallysoft.de) in March 2011   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //

#ifndef AISC_LOCATION_H
#define AISC_LOCATION_H

#ifndef DUPSTR_H
#include <dupstr.h>
#endif

class Location {
    int linenr;
    char *path;
    
    void print_internal(const char *msg, const char *msg_type, const char *launcher_file, int launcher_line) const;

    void fprint_location(const char *file, int line, FILE *fp) const {
        fprintf(fp, "%s:%i: ", file, line);
    }
    void fprint_location(FILE *fp) const {
        fprint_location(path, linenr, fp);
    }

    static Location exit_pc;
    static int global_error_count;
    
public:

    Location() : linenr(-666), path(NULL) {}
    Location(int linenr_, const char *path_)
        : linenr(linenr_), path(strdup(path_)) { }
    Location(const Location& other)
        : linenr(other.linenr), path(strdup(other.path)) { }
    Location& operator=(const Location& other) {
        linenr = other.linenr;
        freedup(path, other.path);
        return *this;
    }
    ~Location() { free(path); }

    bool valid() const { return path != 0; }

    const char *get_path() const { return path; }
    int get_linenr() const { return linenr; }

    void print_error_internal(const char *err, const char *launcher_file, int launcher_line) const {
        print_internal(err, "Error", launcher_file, launcher_line);
        global_error_count++;
    }
    void print_warning_internal(const char *msg, const char *launcher_file, int launcher_line) const {
        print_internal(msg, "Warning", launcher_file, launcher_line);
    }
    void start_message(const char *prefix) const {
        print_internal(NULL, prefix, NULL, 0);
    }

    static const Location& guess_pc();
    static void announce_exit_pc(const Location& exitingHere) { exit_pc = exitingHere; }

    static int get_error_count() { return global_error_count; }

    Location& operator++() {
        aisc_assert(valid());
        linenr++;
        return *this;
    }
    bool operator == (const Location& other) const {
        if (!valid()) return !other.valid();
        return linenr == other.linenr && strcmp(path, other.path) == 0;
    }
    bool operator != (const Location& other) const { return !(*this == other); }
};

#else
#error aisc_location.h included twice
#endif // AISC_LOCATION_H
