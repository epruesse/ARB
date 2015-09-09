// =============================================================== //
//                                                                 //
//   File      : ali_arbdb.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_ARBDB_HXX
#define ALI_ARBDB_HXX

#include "ali_other_stuff.hxx"

class ALI_SEQUENCE;

// Class for accessing the database
class ALI_ARBDB : virtual Noncopyable {
private:
    char *alignment;

public:
    GBDATA *gb_main;

    ALI_ARBDB() {
        alignment = 0;
        gb_main = 0;
    }
    ~ALI_ARBDB();

    int open(char *name, char *use_alignment = 0);
    void close();

    void begin_transaction() {
        GB_begin_transaction(gb_main);
    }
    void commit_transaction() {
        GB_commit_transaction(gb_main);
    }

    char *get_sequence_string(char *name, int and_mark = 0);
    ALI_SEQUENCE *get_sequence(char *name, int and_mark = 0);
    char *get_SAI(char *name);
    int put_sequence_string(char *name, char *sequence);
    int put_sequence(char *name, ALI_SEQUENCE *sequence);
    int put_SAI(const char *name, char *sequence);
};

#else
#error ali_arbdb.hxx included twice
#endif // ALI_ARBDB_HXX
