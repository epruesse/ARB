//  ==================================================================== // 
//                                                                       // 
//    File      : GEN_tools.cxx                                          // 
//    Purpose   : Some helper functions                                  // 
//    Time-stamp: <Thu Jul/01/2004 09:08 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2004             // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <arbdb.h>

#include "GEN_local.hxx"
#include "GEN_tools.hxx"

using namespace std;

// provide fast access to strings like
// "pos_begin", "pos_begin2", "pos_begin3", ...
class NumberedEntries {
    int    size;
    char **entries;

    void fill_entries(int from_idx, int to_idx) {
        while (from_idx <= to_idx) {
            entries[from_idx] = GBS_global_string_copy("%s%i", entries[0], from_idx+1); // entries[1] gets number '2'
            ++from_idx;
        }
    }

    void increase_size(int new_size) {
        char **new_entries = new char*[new_size];

        for (int i = 0; i<size; ++i) new_entries[i] = entries[i];

        delete [] entries;

        entries      = new_entries;
        int old_size = size;
        size         = new_size;

        fill_entries(old_size, new_size-1);
    }

    NumberedEntries(const NumberedEntries& other); // copying not allowed
    NumberedEntries& operator = (const NumberedEntries& other); // assignment not allowed
public:
    NumberedEntries(const char *prefix)
        : size(5)
        , entries(new char*[size])
    {
        entries[0] = strdup(prefix);
        fill_entries(1, size-1);
    }
    ~NumberedEntries() {
        for (int i = 0; i<size; ++i) free(entries[i]);
        delete [] entries;
    }

    const char *get_entry(int nr) {
        gen_assert(nr >= 1);
        if (nr > size) {
            int new_size = size+5;
            if (nr>new_size) new_size = nr+5;
            increase_size(new_size);
        }
        return entries[nr-1];
    }
};


const char *GEN_pos_begin_entry_name(int nr) {
    static NumberedEntries entries("pos_begin");
    return entries.get_entry(nr);
}
const char *GEN_pos_end_entry_name(int nr) {
    static NumberedEntries entries("pos_end");
    return entries.get_entry(nr);
}





