// ============================================================ //
//                                                              //
//   File      : awti_imp_local.hxx                             //
//   Purpose   : local definitions for import                   //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef AWTI_IMP_LOCAL_HXX
#define AWTI_IMP_LOCAL_HXX

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AWTI_IMPORT_HXX
#include <awti_import.hxx>
#endif

#define awti_assert(cond) arb_assert(cond)

#define AWAR_FILE_BASE      "tmp/import/pattern"
#define AWAR_FILE           AWAR_FILE_BASE"/file_name"
#define AWAR_FORM           "tmp/import/form"
#define AWAR_ALI            "tmp/import/alignment"
#define AWAR_ALI_TYPE       "tmp/import/alignment_type"
#define AWAR_ALI_PROTECTION "tmp/import/alignment_protection"

#define GB_MAIN awtcig.gb_main
#define AWTC_IMPORT_CHECK_BUFFER_SIZE 10000


struct input_format_per_line {
    char *match;
    char *aci;
    char *srt;
    char *mtag;
    char *append;
    char *write;
    char *setvar;
    GB_TYPES type;

    char *defined_at; // where was match defined

    input_format_per_line *next;

    input_format_per_line *reverse(input_format_per_line *to_append) {
        input_format_per_line *rest = next;
        next = to_append;
        return rest ? rest->reverse(this) : this;
    }

    input_format_per_line();
    ~input_format_per_line();
};

#define IFS_VARIABLES 26                            // 'a'-'z'

class SetVariables {
    typedef SmartPtr<std::string> StringPtr;
    StringPtr value[IFS_VARIABLES];

public:
    SetVariables() {}

    void set(char c, const char *s) {
        awti_assert(c >= 'a' && c <= 'z');
        value[c-'a'] = new std::string(s);
    }
    const std::string *get(char c) const {
        awti_assert(c >= 'a' && c <= 'z');
        StringPtr v = value[c-'a'];
        return v.isNull() ? NULL : &*v;
    }
};


struct input_format_struct {
    char   *autodetect;
    char   *system;
    char   *new_format;
    size_t  new_format_lineno;
    size_t  tab;

    char    *begin;

    char   *sequencestart;
    int     read_this_sequence_line_too;
    char   *sequenceend;
    char   *sequencesrt;
    char   *sequenceaci;
    char   *filetag;
    char   *autotag;
    size_t  sequencecolumn;
    int     autocreateacc;
    int     noautonames;

    char *end;

    SetVariables global_variables;                 // values of global variables
    SetVariables variable_errors;                  // user-defined errors (used when var not set)

    char *b1;
    char *b2;

    input_format_per_line *pl;

    input_format_struct();
    ~input_format_struct();
};

struct awtcig_struct {
    struct input_format_struct  *ifo; // main input format
    struct input_format_struct  *ifo2; // symlink to input format
    GBDATA                      *gb_main; // import database
    AW_CL                        cd1, cd2;
    AWTC_RCB(func);
    char                       **filenames;
    char                       **current_file;
    FILE                        *in;
    bool                         doExit; // whether import window 'close' does exit
    GBDATA                      *gb_other_main; // main DB
};


#else
#error awti_imp_local.hxx included twice
#endif // AWTI_IMP_LOCAL_HXX
