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

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AWTI_IMPORT_HXX
#include <awti_import.hxx>
#endif
#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

#define awti_assert(cond) arb_assert(cond)

#define AWAR_FILE_BASE      "tmp/import/pattern"
#define AWAR_FILE           AWAR_FILE_BASE"/file_name"
#define AWAR_FORM           "tmp/import/form"
#define AWAR_ALI            "tmp/import/alignment"
#define AWAR_ALI_TYPE       "tmp/import/alignment_type"
#define AWAR_ALI_PROTECTION "tmp/import/alignment_protection"

#define AWTI_IMPORT_CHECK_BUFFER_SIZE 10000


struct import_match : virtual Noncopyable {
    // one for each "MATCH" section of the import format

    char     *match;
    char     *aci;
    char     *srt;
    char     *mtag;
    char     *append;
    char     *write;
    char     *setvar;
    GB_TYPES  type;
    char     *defined_at;     // where was match defined

    import_match *next;

    import_match *reverse(import_match *to_append) {
        import_match *rest = next;
        next = to_append;
        return rest ? rest->reverse(this) : this;
    }

    import_match();
    ~import_match();
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


struct import_format : virtual Noncopyable {
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

    import_match *match;

    import_format();
    ~import_format();
};

struct ArbImporter : virtual Noncopyable {
    import_format *ifo;  // main input format
    import_format *ifo2; // symlink to input format

    GBDATA *gb_import_main; // import database

    RootCallback after_import_cb;

    StrArray filenames;
    int      current_file_idx;

    FILE *in;
    bool  doExit; // whether import window 'close' does exit // @@@ rename (meaning is: import from inside ARB or not)

    GBDATA *gb_main_4_nameserver; // main DB (needed to select nameserver-settings)

    ArbImporter(const RootCallback& after_import_cb_)
        : ifo(NULL),
          ifo2(NULL),
          gb_import_main(NULL),
          after_import_cb(after_import_cb_),
          current_file_idx(0),
          in(NULL),
          doExit(false),
          gb_main_4_nameserver(NULL)
    {
    }

    ~ArbImporter() {
        if (gb_import_main) GB_close(gb_import_main);
        delete ifo;
        delete ifo2;

        awti_assert(!in);
    }

    GB_ERROR read_format(const char *file);
    void detect_format(AW_root *root);

    int       next_file();
    char     *read_line(int tab, char *sequencestart, char *sequenceend);
    GB_ERROR  read_data(char *ali_name, int security_write);

    void go(AW_window *aww);

    GBDATA *peekImportDB() {
        return gb_import_main;
    }
    GBDATA *takeImportDB() {
        GBDATA *gbm    = gb_import_main;
        gb_import_main = NULL;
        return gbm;
    }
};


#else
#error awti_imp_local.hxx included twice
#endif // AWTI_IMP_LOCAL_HXX
