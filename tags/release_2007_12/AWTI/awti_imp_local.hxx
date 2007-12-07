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
    char *tag;
    char *append;
    char *write;
    char *setvar;

    int start_line; //  linenumber in filter where this MATCH starts

    struct input_format_per_line *next;

    struct input_format_per_line *reverse(struct input_format_per_line *append) {
        struct input_format_per_line *rest = next;
        next = append;
        return rest ? rest->reverse(this) : this;
    }
};


struct input_format_struct {
    char    *autodetect;
    char    *system;
    char    *new_format;
    size_t  tab;

    char    *begin;

    struct input_format_per_line    *pl;

    char    *sequencestart;
    int     read_this_sequence_line_too;
    char    *sequenceend;
    char    *sequencesrt;
    char    *sequenceaci;
    char    *filetag;
    size_t  sequencecolumn;
    int     autocreateacc;
    int     noautonames;

    char    *end;

    char    *b1;
    char    *b2;
    input_format_struct(void);
    ~input_format_struct(void);
};

extern struct awtcig_struct {
    struct input_format_struct  *ifo; // main input format
    struct input_format_struct  *ifo2; // symlink to input format
    GBDATA                      *gb_main; // import database
    AW_CL                        cd1,cd2;
    AWTC_RCB(func);
    char                       **filenames;
    char                       **current_file;
    FILE                        *in;
    bool                         doExit; // whether import window 'close' does exit
    GBDATA                      *gb_other_main; // main DB 
} awtcig;

