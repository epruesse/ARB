

#ifndef _ALI_ARBDB_INC_
#define _ALI_ARBDB_INC_

#include "ali_other_stuff.hxx"
#include "ali_sequence.hxx"

/*
 * Class for accessing the database
 */
class ALI_ARBDB {
private:
   char *alignment;

public:
   GBDATA *gb_main;

   ALI_ARBDB(void) {
        alignment = 0;
        gb_main = 0;
    }
   ~ALI_ARBDB(void);

   int open(char *name, char *use_alignment = 0);
   void close(void);

    void begin_transaction(void) {
        GB_begin_transaction(gb_main);
    }
    void commit_transaction(void) {
        GB_commit_transaction(gb_main);
    }

    char *get_sequence_string(const char *name, int and_mark = 0);
    ALI_SEQUENCE *get_sequence(char *name, int and_mark = 0);
    char *get_extended(char *name);
    int put_sequence_string(char *name, char *sequence, char *info = 0);
    int put_sequence(char *name, ALI_SEQUENCE *sequence, char *info = 0);
    int put_extended(char *name, char *sequence, char *info = 0);
};


#endif

