#define OPENFILES   16
#define HASHSIZE   1024
#define STACKSIZE   10
enum aisc_commands {
    no_command,
    IF,
    ENDIF,
    ELSE,
    FOR,
    NEXT,
    ENDFOR,
    ELSEIF,
    FUNCTION,
    LABEL,
    MAX_COM };

struct hash_entry {
    char *key;
    char *val;
    struct hash_entry *next;
};
typedef struct hash_struct {
    int size;
    struct hash_entry **entries;
} hash;


typedef struct acm_data {
    struct acm_data *next_item;
    struct acm_data *next_line;
    struct acm_data *first_item;
    struct acm_data *first_line;
    struct acm_data *father;
    const char *key;
    struct acm_data *sub;
    char *val;
} AD;

typedef struct header_struct {
    struct header_struct *next;
    char *key;
} HS;
        
struct for_data_struct {
    char            *forstr;
    AD              *forcursor;
    long            forval;
    long            forend;
    struct for_data_struct *next;
};

        

typedef struct command_lines {
    struct command_lines *next;
    char            *str;
    int             linenr;
    char            *path;
    enum aisc_commands      command;
    struct for_data_struct  *fd;
    struct command_lines *IF;
    struct command_lines *ELSE;
    struct command_lines *ENDIF;
    struct command_lines *FOR;
    struct command_lines *NEXT;
    struct command_lines *ENDFOR;
} CL;

struct stack_struct {
    AD *cursor;
    CL *pc;
    hash    *hs;
    struct stack_struct *next;
};

struct param_struct {
    char *param;
    struct param_struct *next;
};

struct global_struct {
    int                  error_flag;
    char                 b_tab[256];
    char                 s_tab[256];
    char                 s2_tab[256];
    char                 s3_tab[256];
    char                 c_tab[256];
    char                 outtab[256];
    AD                  *cursor;
    AD                  *root;
    CL                  *prg;
    CL                  *pc;
    CL                  *nextpc;
    struct stack_struct *st;
    int                  sp;
    int                  line_cnt;
    char                *line_path;
    int                  lastchar;
    char                *linebuf;
    int                  bufsize;
    int                  s_pos;
    FILE                *out;
    FILE        *outs[OPENFILES];
    char        *fouts[OPENFILES]; // type of file
    char        *fouts_name[OPENFILES]; // file-system-name of file
    
    int   tabstop;
    int   tabs[10];
    hash *fns;
    int tabpos;
};

extern struct global_struct *gl;
extern char error_buf[256];
#define READ_SPACES(var)        while(gl->s3_tab[(unsigned)(*var)]) var++;
#define READ_RSPACES(var)       while(gl->s3_tab[(unsigned)(*(--var))]);

#define EOSTR 0
#define BEG_STR1 '('
#define BEG_STR2 '~'
#define END_STR1 '~'
#define END_STR2 ')'



