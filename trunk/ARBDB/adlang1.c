#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* #include <malloc.h> */
#include <ctype.h>

#include "adlocal.h"
#include "arbdbt.h"
#include "adGene.h"

#define AWAR_TREE_REFRESH "tmp/focus/tree_refresh" // touch this awar to refresh the tree display

/* hook for 'export_sequence' */

static gb_export_sequence_cb get_export_sequence = 0;

NOT4PERL void GB_set_export_sequence_hook(gb_export_sequence_cb escb) {
    ad_assert(!get_export_sequence || !escb); // avoid unwanted overwrite
    get_export_sequence = escb;
}

/* global ACI/SRT debug switch */
static int trace = 0;

void GB_set_ACISRT_trace(int enable) { trace = enable; }
int GB_get_ACISRT_trace() { return trace; }


/* export stream */

#define STREAM_OUT(args, s)      do { (*((args)->voutput))[(*((args)->coutput))++].str = (s); } while(0)
#define STREAM_DUPOUT(args, s)   STREAM_OUT(args, GB_STRDUP(s));
#define STREAM_INOUT(args, i)    STREAM_DUPOUT(args, args->vinput[i].str);

/********************************************************************************************
                                        Parameter Functions
********************************************************************************************/

struct gbl_param {
    struct gbl_param *next;
    GB_TYPES          type;     // type of variable
    void             *varaddr;  // address of variable where value gets stored
    const char       *param_name; // parameter name (e.g. 'include=')
    const char       *help_text; // help text for parameter
};

#define GBL_BEGIN_PARAMS struct gbl_param *params = 0

static void gbl_new_param(struct gbl_param **pp, GB_TYPES type, void *vaddr, const char *param_name, const char *help_text) {
    struct gbl_param *gblp = (struct gbl_param *)GB_calloc(1,sizeof(struct gbl_param));
    
    gblp->next = *pp;
    *pp         = gblp;

    gblp->type       = type;
    gblp->varaddr    = vaddr;
    gblp->param_name = param_name;
    gblp->help_text  = help_text;
}

typedef const char *String;
typedef int         bit;

static int gbl_param_int(const char *param_name,int def, const char *help_text, struct gbl_param **pp, int *vaddr) {
    gbl_new_param(pp, GB_INT, vaddr, param_name, help_text);
    return def;
}

static char gbl_param_char(const char *param_name,char def, const char *help_text, struct gbl_param **pp, char *vaddr) {
    gbl_new_param(pp, GB_BYTE, vaddr, param_name, help_text);
    return def;
}

static size_t gbl_param_size_t(const char *param_name, size_t def, const char *help_text, struct gbl_param **pp, size_t *vaddr) {
    gbl_new_param(pp, GB_INT, vaddr, param_name, help_text);
    return def;
}

static const char *gbl_param_String(const char *param_name, const char *def, const char *help_text, struct gbl_param **pp, String *vaddr) {
    gbl_new_param(pp, GB_STRING, vaddr, param_name, help_text);
    return def;
}

static int gbl_param_bit(const char *param_name, int def, const char *help_text, struct gbl_param **pp, bit *vaddr) {
    gbl_new_param(pp, GB_BIT, vaddr, param_name, help_text);
    return def;
}

#define GBL_PARAM_TYPE(type, var, param_name, def, help_text) type  var = gbl_param_##type(param_name, def, help_text, &params, &var)
#define GBL_STRUCT_PARAM_TYPE(type, strct, member, param_name, def, help_text) strct.member = gbl_param_##type(param_name, def, help_text, &params, &strct.member)

#define GBL_PARAM_INT(   var, param_name, def, help_text) GBL_PARAM_TYPE(int,    var, param_name, def, help_text)
#define GBL_PARAM_CHAR(  var, param_name, def, help_text) GBL_PARAM_TYPE(char,   var, param_name, def, help_text)
#define GBL_PARAM_SIZET( var, param_name, def, help_text) GBL_PARAM_TYPE(size_t, var, param_name, def, help_text)
#define GBL_PARAM_STRING(var, param_name, def, help_text) GBL_PARAM_TYPE(String, var, param_name, def, help_text)
#define GBL_PARAM_BIT(   var, param_name, def, help_text) GBL_PARAM_TYPE(bit,    var, param_name, def, help_text)

#define GBL_STRUCT_PARAM_INT(   strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(int,    strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_CHAR(  strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(char,   strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_SIZET( strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(size_t, strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_STRING(strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(String, strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_BIT(   strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(bit,    strct, member, param_name, def, help_text)

#define GBL_TRACE_PARAMS(argc,argv) { \
        GB_ERROR def_error = trace_params(argc,argv,params,args->command); \
        if (def_error) { GBL_END_PARAMS; return def_error; }; };

#define GBL_END_PARAMS { struct gbl_param *_gblp; while (params) { \
                _gblp = params; params = params->next; \
                free((char *)_gblp); } };

#define GBL_CHECK_FREE_PARAM(nr,cnt) if (nr+cnt >= GBL_MAX_ARGUMENTS) {\
        return "Max Parameters exceeded";}

static GB_ERROR trace_params(int argc, const GBL *argv, struct gbl_param *ppara, const char *com) {
    GB_ERROR error = 0;
    int      i;

    for (i=0;i<argc;i++) {
        struct gbl_param *para;
        const char       *argument = argv[i].str;

        for (para = ppara; para && !error; para = para->next) {
            int len = strlen(para->param_name);

            if (strncmp(argument, para->param_name, len) == 0) { 
                const char *value = argument+len; // set to start of value;

                if (para->type == GB_BIT) {
                    // GB_BIT is special cause param_name does NOT contain trailing '='

                    if (!value[0]) { // only 'name' -> handle like 'name=1'
                        ;
                    }
                    else if (value[0] == '=') {
                        value++;
                    }
                }

                switch (para->type) {
                    case GB_STRING:
                        *(const char **)para->varaddr  = value;
                        break;
                        
                    case GB_INT:
                        gb_assert(sizeof(int) == sizeof(size_t)); // assumed by GBL_PARAM_SIZET
                        *(int *)para->varaddr = atoi(value);
                        break;

                    case GB_BIT:
                        // 'param=' is same as 'param' or 'param=1' (historical reason, dont change)
                        *(int *)para->varaddr = (value[0] ? atoi(value) : 1);
                        break;

                    case GB_BYTE:
                        *(char *)para->varaddr = *value; // this may use the terminal zero-byte (e.g. for p1 in 'p0=0,p1=,p2=2' )
                        if (value[0] && value[1]) { // found at least 2 chars
                            GB_warning("Only one character expected in value '%s' of param '%s' - rest is ignored", value, para->param_name);
                        }
                        break;

                    default:
                        gb_assert(0);
                        error = GBS_global_string("Parameter '%s': Unknown type %i (internal error)", para->param_name, para->type);
                        break;
                }
                break; // accept parameter
            }
        }

        if (!error && !para) { // no parameter found for argument
            int pcount = 0;
            struct gbl_param **params;
            int k;
            char *res;
            void *str = GBS_stropen(1000);
            GB_ERROR err;


            for (para = ppara; para; para = para->next) pcount++;
            params = (struct gbl_param **)GB_calloc(sizeof(void *),pcount);
            for (k = 0, para = ppara; para; para = para->next) params[k++] = para;


            for (pcount--; pcount>=0; pcount--) {
                para = params[pcount];
                GBS_strcat(str,"\t");
                GBS_strcat(str,para->param_name);
                switch(para->type) {
                    case GB_STRING: GBS_strcat(str, "STRING"); break;
                    case GB_INT:    GBS_strcat(str, "INT");    break;
                    case GB_FLOAT:  GBS_strcat(str, "FLOAT");  break;
                    case GB_BYTE:   GBS_strcat(str, "CHAR");   break;
                    case GB_BIT:    GBS_strcat(str, "    ");   break;
                    default:        gb_assert (0); GBS_strcat(str, "????"); break;
                }
                GBS_strcat(str,"\t\t;");
                GBS_strcat(str,para->help_text);
                GBS_strcat(str,"\n");
            }
            freeset(params, NULL);
            res = GBS_strclose(str);
            err = GB_export_error("Unknown Parameter '%s' in command '%s'\n\tPARAMETERS:\n%s",argv[i].str,com,res);
            free(res);
            return err;
        }
    }

    return error;
}


/********************************************************************************************
          String Functions
********************************************************************************************/

static char *unEscapeString(const char *escapedString) {
    /* replaces all \x by x */
    char *result = nulldup(escapedString);
    char *to     = result;
    char *from   = result;

    while (1) {
        char c = *from++;
        if (!c) break;

        if (c=='\\') {
            *to++ = *from++;
        }
        else {
            *to++ = c;
        }
    }
    *to = 0;
    return result;
}

static int gbl_stricmp(const char *s1, const char *s2) {
    /* case insensitive strcmp */
    int i;
    for (i = 0; ; ++i) {
        char c1 = tolower(s1[i]);
        char c2 = tolower(s2[i]);

        if (c1 == c2) {
            if (!c1) break; // equal strings
        }
        else {
            if (c1<c2) return -1;
            return 1;
        }
    }
    return 0;
}
static int gbl_strincmp(const char *s1, const char *s2, int size2) {
    /* case insensitive strcmp */
    int i;
    for (i = 0; i<size2; ++i) {
        char c1 = tolower(s1[i]);
        char c2 = tolower(s2[i]);

        if (c1 == c2) {
            if (!c1) break; // equal strings
        }
        else {
            if (c1<c2) return -1;
            return 1;
        }
    }
    return 0;
}
static const char *gbl_stristr(const char *haystack, const char *needle) {
    /* case insensitive strstr */
    const char *hp          = haystack;
    char        c1          = toupper(needle[0]);
    char        c2          = tolower(c1);
    int         needle_size = strlen(needle);

    if (c1 == c2) {
        hp = strchr(hp, c1);
        while (hp) {
            if (gbl_strincmp(hp, needle, needle_size) == 0) return hp;
            hp = strchr(hp+1, c1);
        }
    }
    else {
        while (hp) {
            const char *h1 = strchr(hp, c1);
            const char *h2 = strchr(hp, c2);

            if (h1 && h2) {
                if (h1<h2) {
                    if (gbl_strincmp(h1, needle, needle_size) == 0) return h1;
                    hp = h1+1;
                }
                else {
                    gb_assert(h1>h2);
                    if (gbl_strincmp(h2, needle, needle_size) == 0) return h2;
                    hp = h2+1;
                }
            }
            else {
                if (h1) { hp = h1; }
                else if (h2) { hp = h2; c1 = c2; }
                else { hp = 0; }

                while (hp) {
                    if (gbl_strincmp(hp, needle, needle_size) == 0) return hp;
                    hp = strchr(hp+1, c1);
                }
            }
        }
    }
    return 0;
}

/* ***************************** gbl_mid_streams *****************************
 * used as well to copy all streams (e.g. by 'dd')
 */

static GB_ERROR gbl_mid_streams(int arg_cinput, const GBL *arg_vinput,
                                int *arg_cout, GBL **arg_vout,
                                int start,int mstart,int end,int relend)
{
    int i;
    GBL_CHECK_FREE_PARAM(*arg_cout,arg_cinput);
    for (i=0;i<arg_cinput;i++) {         /* go through all in streams    */
        char *p;
        int c;

        int len;
        int nstart = start;
        int nend = end;

        p = arg_vinput[i].str;
        len = strlen(p);
        if (nstart<0) nstart = len - mstart;
        if (nend<0) nend = len - relend;        /* check rel len */

        if (nstart>len ) nstart = len;  /* check boundaries */
        if (nstart<0) nstart = 0;
        if (nend>len) nend = len;
        if (nend<nstart) nend = nstart;

        c = p[nend];
        p[nend] =0;
        (*arg_vout)[(*arg_cout)++].str = GB_STRDUP(p+nstart);     /* export result string */
        p[nend] = c;
    }
    return 0;
}

/* ***************************** trace ***************************** */

static GB_ERROR gbl_trace(GBL_command_arguments *args) {
    int tmp_trace;

    if (args->cparam!=1) return "syntax: trace(0|1)";

    tmp_trace = atoi(args->vparam[0].str);
    if (tmp_trace<0 || tmp_trace>1) return GBS_global_string("Illegal value %i to trace", tmp_trace);

    if (tmp_trace != GB_get_ACISRT_trace()) {
        printf("*** %sctivated ACI trace ***\n", tmp_trace ? "A" : "De-a");
        GB_set_ACISRT_trace(tmp_trace);
    }

    return gbl_mid_streams(args->cinput, args->vinput,args->coutput, args->voutput, 0, 0, -1, 0); /* copy all streams */
}

/********************************************************************************************
          Binary operators:
********************************************************************************************/
/*
 * Binary operators work on pairs of values.
 * Three different operational modes are implemented for all binary operators:
 *
 * 1. inputstreams|operator
 *
 *    The number of inputstreams has to be even and the operator will be
 *    applied to pairs of them.
 *
 *    Example : a;b;c;d;e;f | plus
 *    Result  : a+b;c+d;e+f
 *
 * 2. inputstreams|operator(x)
 *
 *    The number of inputstreams has to be at least 1.
 *    The operator is applied to each inputstream.
 *
 *    Example : a;b;c | plus(d)
 *    Result  : a+d;b+d;c+d
 *
 * 3. operator(x, y)
 *
 *    Inputstreams will be ignored and the operator is applied
 *    to the arguments
 *
 *    Example : a;b | plus(c,d)
 *    Result  : c+d
 */

typedef char* (*gbl_binary_operator)(const char *arg1, const char *arg2, void *client_data);

static GB_ERROR gbl_apply_binary_operator(GBL_command_arguments *args, gbl_binary_operator op, void *client_data)
{
    GB_ERROR  error  = 0;

    switch (args->cparam) {
        case 0:
            if (args->cinput == 0) error = "Expect at least two input streams if called with 0 parameters";
            else if (args->cinput%2) error = "Expect an even number of input streams if called with 0 parameters";
            else {
                int inputpairs = args->cinput/2;
                int i;
                for (i = 0; i<inputpairs; ++i) {
                    STREAM_OUT(args, op(args->vinput[i*2].str, args->vinput[i*2+1].str, client_data));
                }
            }
            break;

        case 1:
            if (args->cinput == 0) error = "Expect at least one input stream if called with 1 parameter";
            else {
                int         i;
                const char *argument = args->vparam[0].str;
                for (i = 0; i<args->cinput; ++i) {
                    STREAM_OUT(args, op(args->vinput[i].str, argument, client_data));
                }
            }
            break;

        case 2:
            {
                GBDATA *gb_main = (GBDATA*)GB_MAIN(args->gb_ref)->data;
                int     i;

                for (i = 0; i<args->cinput; ++i) {
                    char *result1       = GB_command_interpreter(gb_main, args->vinput[i].str, args->vparam[0].str, args->gb_ref, args->default_tree_name);
                    if (!result1) error = GB_get_error();
                    else {
                        char *result2       = GB_command_interpreter(gb_main, args->vinput[i].str, args->vparam[1].str, args->gb_ref, args->default_tree_name);
                        if (!result2) error = GB_get_error();
                        else {
                            STREAM_OUT(args, op(result1, result2, client_data));
                            free(result2);
                        }
                        free(result1);
                    }
                }

                break;
            }
        default :
            error = "0 to 2 arguments expected";
            break;
    }

    return error;
}

/********************************************************************************************
          the commands themselves:
********************************************************************************************/

static GB_ERROR gbl_command(GBL_command_arguments *args)
{
    GBDATA *gb_main = (GBDATA*)GB_MAIN(args->gb_ref)->data;
    int     i;
    char   *command;

    if (args->cparam!=1) return "syntax: command(\"escaped command\")";

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    command = unEscapeString(args->vparam[0].str);
#if defined(DEBUG)
    printf("executing command '%s'\n", command);
#endif /* DEBUG */

    for (i=0;i<args->cinput;i++) { /* go through all orig streams       */
        char *result = GB_command_interpreter(gb_main, args->vinput[i].str, command, args->gb_ref, args->default_tree_name);
        if (!result) return GB_get_error();

        STREAM_OUT(args, result);
    }
    free(command);
    return 0;
}

static GB_ERROR gbl_eval(GBL_command_arguments *args)
{
    GBDATA *gb_main = (GBDATA*)GB_MAIN(args->gb_ref)->data;
    int     i;
    char   *command;
    char   *to_eval;

    if (args->cparam!=1) return "eval syntax: eval(\"escaped command evaluating to command\")";

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    to_eval = unEscapeString(args->vparam[0].str);
    command = GB_command_interpreter(gb_main, "", to_eval, args->gb_ref, args->default_tree_name); // evaluate independent

    if (GB_get_ACISRT_trace()) {
        printf("evaluating '%s'\n", to_eval);
        printf("executing '%s'\n", command);
    }

    for (i=0;i<args->cinput;i++) { /* go through all orig streams  */
        char *result = GB_command_interpreter(gb_main, args->vinput[i].str, command, args->gb_ref, args->default_tree_name);
        if (!result) return GB_get_error();

        STREAM_OUT(args, result);
    }
    free(to_eval);
    free(command);
    return 0;
}

static GB_HASH *definedCommands = 0;

static GB_ERROR gbl_define(GBL_command_arguments *args) {
    GB_ERROR error = 0;

    if (args->cparam!=2) {
        error = "syntax: define(name, \"escaped command\")";
    }
    else {
        const char *name = args->vparam[0].str;
        char       *cmd  = unEscapeString(args->vparam[1].str);
        char       *oldcmd;

        if (!definedCommands) definedCommands = GBS_create_hash(100, GB_MIND_CASE);

        oldcmd = (char*)GBS_read_hash(definedCommands, name);
        if (oldcmd) free(oldcmd);
        GBS_write_hash(definedCommands, name, (long)cmd);

        if (GB_get_ACISRT_trace()) {
            printf("defining command '%s'='%s'\n", name, cmd);
        }
    }

    return error;
}

static GB_ERROR gbl_do(GBL_command_arguments *args) {
    GB_ERROR error = 0;

    if (args->cparam!=1) {
        error = "syntax: do(name)";
    }
    else {
        const char *name = args->vparam[0].str;
        const char *cmd  = 0;
        
        if (definedCommands) cmd = (const char*)GBS_read_hash(definedCommands, name);
        if (!cmd) {
            error = GBS_global_string("Can't do undefined command '%s' - use define(%s, ...) first", name, name);
        }
        else {
            GBDATA *gb_main = (GBDATA*)GB_MAIN(args->gb_ref)->data;
            int     i;

            GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

            if (GB_get_ACISRT_trace()) {
                printf("executing defined command '%s'='%s' on %i streams\n", name, cmd, args->cinput);
            }
            
            for (i=0;i<args->cinput;i++) { /* go through all orig streams  */
                char *result = GB_command_interpreter(gb_main, args->vinput[i].str, cmd, args->gb_ref, args->default_tree_name);
                if (!result) return GB_get_error();

                STREAM_OUT(args, result);
            }
        }
    }
    return error;
}

static GB_ERROR gbl_streams(GBL_command_arguments *args) {
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    STREAM_OUT(args, GBS_global_string_copy("%i", args->cinput));
    return 0;
}

static GB_ERROR gbl_origin(GBL_command_arguments *args)
{
    GBDATA *gb_origin = 0;
    GBDATA *gb_main   = (GBDATA*)GB_MAIN(args->gb_ref)->data;

    if (args->cparam!=1) return GB_export_error("syntax: %s(\"escaped command\")", args->command);
    if (!GEN_is_pseudo_gene_species(args->gb_ref)) return GB_export_error("'%s' applies to gene-species only", args->command);

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    if (strcmp(args->command, "origin_organism") == 0) {
        gb_origin = GEN_find_origin_organism(args->gb_ref, 0);
    }
    else {
        ad_assert(strcmp(args->command, "origin_gene") == 0);
        gb_origin = GEN_find_origin_gene(args->gb_ref, 0);
    }

    {
        char *command = unEscapeString(args->vparam[0].str);
        int   i;

        for (i=0;i<args->cinput;i++) { /* go through all orig streams  */
            char *result = GB_command_interpreter(gb_main, args->vinput[i].str, command, gb_origin, args->default_tree_name);
            if (!result) return GB_get_error();

            STREAM_OUT(args, result);
        }

        free(command);
    }
    return 0;
}

static GB_ERROR gbl_count(GBL_command_arguments *args)
{
    int  i;
    char tab[256];              /* if tab[char] count 'char' */

    if (args->cparam!=1) return "count syntax: count(\"characters to count\")";

    for (i=0;i<256;i++) {
        if (strchr(args->vparam[0].str,i)) tab[i] = 1;
        else tab[i] = 0;
    }
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all orig streams  */
        long           sum = 0;                     /* count frequencies    */
        unsigned char *p   = (unsigned char *)args->vinput[i].str;
        while (*p){
            sum += tab[*(p++)];
        }
        STREAM_OUT(args, GBS_global_string_copy("%li", sum));
    }
    return 0;
}

static GB_ERROR gbl_len(GBL_command_arguments *args)
{
    int i;
    char tab[256];                              /* if tab[char] count 'char' */
    const char *option;

    if (args->cparam == 0) option = "";
    else option = args->vparam[0].str;
    if (args->cparam>=2) return "len syntax: len[(\"characters not to count\")]";

    for (i=0;i<256;i++) {
        if (strchr(option,i)) tab[i] = 0;
        else tab[i] = 1;
    }
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all orig streams  */
        char *p;
        long sum = 0;                   /* count frequencies    */
        p = args->vinput[i].str;
        while (*p){
            sum += tab[(unsigned int)*(p++)];
        }
        STREAM_OUT(args, GBS_global_string_copy("%li", sum));
    }
    return 0;
}

static GB_ERROR gbl_remove(GBL_command_arguments *args)
{
    int i;
    char tab[256];                              /* if tab[char] count 'char' */


    if (args->cparam!=1) return "remove syntax: remove(\"characters to remove\")";

    for (i=0;i<256;i++) {
        if (strchr(args->vparam[0].str,i)) tab[i] = 1;
        else tab[i] = 0;
    }

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all orig streams  */
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = args->vinput[i].str;*p;p++){
            if (!tab[(unsigned int)*p]) {
                GBS_chrcat(strstruct,*p);
            }
        }
        STREAM_OUT(args, GBS_strclose(strstruct));
    }
    return 0;
}

static GB_ERROR gbl_keep(GBL_command_arguments *args)
{
    int  i;
    char tab[256];                              /* if tab[char] != 0 then keep'char' */


    if (args->cparam!=1) return "keep syntax: keep(\"characters not to remove\")";

    memset(tab, 0, 256); // keep none
    {
        unsigned char *keep = (unsigned char*)args->vparam[0].str;
        for (i = 0; keep[i]; ++i) {
            tab[keep[i]] = 1; // keep all members of argument
        }
    }

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all orig streams  */
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = args->vinput[i].str;*p;p++){
            if (tab[(unsigned int)*p]) {
                GBS_chrcat(strstruct,*p);
            }
        }
        STREAM_OUT(args, GBS_strclose(strstruct));
    }
    return 0;
}

static char *binop_compare(const char *arg1, const char *arg2, void *client_data) {
    long case_sensitive = (long)client_data;
    int result;

    if (case_sensitive) result = strcmp(arg1, arg2);
    else result                = gbl_stricmp(arg1, arg2);

    return GBS_global_string_copy("%i", result<0 ? -1 : (result>0 ? 1 : 0));
}
static char *binop_equals(const char *arg1, const char *arg2, void *client_data) {
    long case_sensitive = (long)client_data;
    int result;

    if (case_sensitive) result = strcmp(arg1, arg2);
    else result                = gbl_stricmp(arg1, arg2);

    return GBS_global_string_copy("%i", result == 0 ? 1 : 0);
}
static char *binop_contains(const char *arg1, const char *arg2, void *client_data) {
    long         case_sensitive = (long)client_data;
    const char *found          = 0;

    if (case_sensitive) found = strstr(arg1, arg2);
    else found                = gbl_stristr(arg1, arg2);

    return GBS_global_string_copy("%ti", found == 0 ? 0 : (found-arg1)+1);
}
static char *binop_partof(const char *arg1, const char *arg2, void *client_data) {
    return binop_contains(arg2, arg1, client_data);
}

static GB_ERROR gbl_compare  (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_compare,  (void*)1); }
static GB_ERROR gbl_icompare (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_compare,  (void*)0); }
static GB_ERROR gbl_equals   (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_equals,   (void*)1); }
static GB_ERROR gbl_iequals  (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_equals,   (void*)0); }
static GB_ERROR gbl_contains (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_contains, (void*)1); }
static GB_ERROR gbl_icontains(GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_contains, (void*)0); }
static GB_ERROR gbl_partof   (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_partof,   (void*)1); }
static GB_ERROR gbl_ipartof  (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, binop_partof,   (void*)0); }

static GB_ERROR gbl_translate(GBL_command_arguments *args)
{
    unsigned char tab[256];
    int  i;
    char replace_other = 0;

    if (args->cparam<2 || args->cparam>3) return "translate syntax: translate(#old, #new [,#other])";

    if (args->cparam == 3) {
        const char *other = args->vparam[2].str;
        if (other[0] == 0 || other[1] != 0) {
            return "third parameter of translate has to be one character (i.e. \"-\")";
        }
        replace_other = other[0];
    }

    /* build translation table : */
    {
        const unsigned char *o = (const unsigned char *)args->vparam[0].str;
        const unsigned char *n = (const unsigned char *)args->vparam[1].str;
        char        used[256];

        if (strlen((const char *)o) != strlen((const char *)n)) {
            return "arguments 1 and 2 of translate should be strings with identical length";
        }

        for (i = 0; i<256; ++i) {
            tab[i]  = replace_other ? replace_other : i; // replace unused or identity translation
            used[i] = 0;
        }

        for (i = 0; o[i]; ++i) {
            if (used[o[i]]) return GBS_global_string("character '%c' used twice in argument 1 of translate", o[i]);
            used[o[i]] = 1;
            tab[o[i]]  = n[i]; // real translation
        }
    }

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all orig streams  */
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = args->vinput[i].str;*p;p++){
            GBS_chrcat(strstruct, tab[(unsigned char)*p]);
        }
        STREAM_OUT(args, GBS_strclose(strstruct));
    }
    return 0;
}


static GB_ERROR gbl_echo(GBL_command_arguments *args)
{
    int i;
    args->cinput = args->cinput;
    args->vinput = args->vinput;
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cparam;i++) {         /* go through all in streams    */
        char *p;
        p = args->vparam[i].str;
        STREAM_DUPOUT(args, p);
    }
    return 0;
}

static GB_ERROR gbl_dd(GBL_command_arguments *args)
{
    if (args->cparam!=0) return "syntax: dd (no parameters)";
    return gbl_mid_streams(args->cinput, args->vinput,args->coutput, args->voutput, 0, 0, -1, 0); /* copy all streams */
}

static GB_ERROR gbl_string_convert(GBL_command_arguments *args)
{
    int mode  = -1;
    int i;

    if (strcmp(args->command, "lower")      == 0) mode = 0;
    else if (strcmp(args->command, "upper") == 0) mode = 1;
    else if (strcmp(args->command, "caps")  == 0) mode = 2;
    else return GB_export_error("Unknown command '%s'", args->command);

    if (args->cparam!=0) return GBS_global_string("syntax: %s (no parameters)", args->command);
    
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) { /* go through all in streams    */
        char *p              = GB_STRDUP(args->vinput[i].str);
        char *pp;
        int   last_was_alnum = 0;

        for (pp = p; pp[0]; ++pp) {
            switch (mode) {
                case 0:  pp[0] = tolower(pp[0]); break;
                case 1:  pp[0] = toupper(pp[0]); break;
                case 2: /* caps */
                    if (isalnum(pp[0])) {
                        if (last_was_alnum) pp[0] = tolower(pp[0]);
                        else pp[0]                = toupper(pp[0]);
                        last_was_alnum            = 1;
                    }
                    else {
                        last_was_alnum = 0;
                    }
                    break;
                default : ad_assert(0); break;
            }
        }

        STREAM_OUT(args, p);
    }

    return 0;
}

static GB_ERROR gbl_head(GBL_command_arguments *args)
{
    int start;
    if (args->cparam!=1) return "head syntax: head(#start)";
    start = atoi(args->vparam[0].str);
    return gbl_mid_streams(args->cinput, args->vinput,args->coutput, args->voutput, 0,0, start, -start);
}

static GB_ERROR gbl_tail(GBL_command_arguments *args)
{
    int end;
    if (args->cparam!=1) return "tail syntax: tail(#length_of_tail)";
    end = atoi(args->vparam[0].str);
    return gbl_mid_streams(args->cinput, args->vinput,args->coutput, args->voutput, -1, end, -1, 0);
}

static GB_ERROR gbl_mid0(GBL_command_arguments *args)
{
    int start;
    int end;
    if (args->cparam!=2) return "mid0 syntax: mid0(#start;#end)";
    start = atoi(args->vparam[0].str);
    end = atoi(args->vparam[1].str);
    return gbl_mid_streams(args->cinput, args->vinput, args->coutput, args->voutput, start, -start, end, -end);
}

static GB_ERROR gbl_mid(GBL_command_arguments *args)
{
    int start;
    int end;
    if (args->cparam!=2) return "mid syntax: mid(#start;#end)";
    start = atoi(args->vparam[0].str)-1;
    end = atoi(args->vparam[1].str)-1;
    return gbl_mid_streams(args->cinput, args->vinput, args->coutput, args->voutput, start, -start, end, -end);
}

static GB_ERROR gbl_tab(GBL_command_arguments *args)
{
    int i,j;
    int tab;
    if (args->cparam!=1) return "tab syntax: tab(#tab)";
    tab = atoi(args->vparam[0].str);
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char *p;
        int   len = strlen(args->vinput[i].str);

        if (len >= tab) {
            STREAM_INOUT(args, i);
        }
        else {
            p = (char *)GB_calloc(sizeof(char),tab+1);
            strcpy(p, args->vinput[i].str);
            for (j=len; j<tab; j++) p[j] = ' ';

            STREAM_OUT(args, p);
        }
    }
    return 0;
}

static GB_ERROR gbl_pretab(GBL_command_arguments *args)
{
    int i,j;
    int tab;
    if (args->cparam!=1) return "pretab syntax: pretab(#tab)";
    tab = atoi(args->vparam[0].str);
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char *p;
        int   len = strlen(args->vinput[i].str);

        if (len >= tab) {
            STREAM_INOUT(args, i);
        }
        else {
            int spaces = tab-len;

            p = (char *)GB_calloc(sizeof(char),tab+1);
            for (j = 0; j<spaces; ++j) p[j] = ' ';
            strcpy(p+j, args->vinput[i].str);
            
            STREAM_OUT(args, p);
        }
    }
    return 0;
}

static GB_ERROR gbl_crop(GBL_command_arguments *args)
{
    int         i;
    const char *chars_to_crop;

    if (args->cparam != 1) return "crop syntax: pretab(chars_to_crop)";

    chars_to_crop = args->vparam[0].str;
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        const char *s = args->vinput[i].str;
        char       *p;
        int         len;

        while (s[0] && strchr(chars_to_crop, s[0]) != 0) s++; /* crop at beg of line */

        len = strlen(s);
        p   = (char*)GB_calloc(sizeof(char), len+1);
        memcpy(p, s, len);

        {
            char *pe = p+len-1;

            while (pe >= p && strchr(chars_to_crop, pe[0]) != 0) { /* crop at end of line */
                --pe;
            }
            ad_assert(pe >= (p-1));
            pe[1] = 0;
        }

        STREAM_OUT(args, p);
    }
    return 0;
}

static GB_ERROR gbl_cut(GBL_command_arguments *args)
{
    int i;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cparam);
    for (i=0; i<args->cparam;i++) {
        int j = atoi(args->vparam[i].str);
        if (j<1 || j>args->cinput) {
            return GBS_global_string("Illegal stream number '%i'", j);
        }
        --j;                    // user numbers streams from 1 to N

        STREAM_INOUT(args, j);
    }
    return 0;
}
static GB_ERROR gbl_drop(GBL_command_arguments *args)
{
    int       i;
    int      *dropped;
    GB_ERROR  error    = 0;

    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput-args->cparam);

    dropped = malloc(args->cinput*sizeof(dropped));
    for (i=0; i<args->cinput;++i) {
        dropped[i] = 0;
    }

    for (i=0; i<args->cparam;++i) {
        int j = atoi(args->vparam[i].str);
        if (j<1 || j>args->cinput) {
            error = GBS_global_string("Illegal stream number '%i'", j);
            break;
        }
        --j; // user numbers streams from 1 to N
        dropped[j] = 1;
    }

    if (!error) {
        for (i=0; i<args->cinput;++i) {
            if (dropped[i] == 0) {
                STREAM_INOUT(args, i);
            }
        }
    }
    free(dropped);

    return error;
}

static GB_ERROR gbl_dropempty(GBL_command_arguments *args)
{
    int i;
    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput);
    if (args->cparam != 0) return "expect no parameters";

    for (i=0; i<args->cinput;++i) {
        if (args->vinput[i].str[0]) { /* if non-empty */
            STREAM_INOUT(args, i);
        }
    }
    return 0;
}

static GB_ERROR gbl_dropzero(GBL_command_arguments *args)
{
    int i;
    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput);
    if (args->cparam != 0) return "expect no parameters";

    for (i=0; i<args->cinput;++i) {
        if (atoi(args->vinput[i].str)) { /* if non-zero */
            STREAM_INOUT(args, i);
        }
    }
    return 0;
}

static GB_ERROR gbl_swap(GBL_command_arguments *args)
{
    int swap1;
    int swap2;
    int firstout = *args->coutput;
    int i;

    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput);
    if (args->cinput<2) return "need at least two input streams";

    if (args->cparam == 0) {
        swap1 = args->cinput-1;
        swap2 = args->cinput-2;
    }
    else if (args->cparam == 2) {
        int illegal = 0;

        swap1 = atoi(args->vparam[0].str);
        swap2 = atoi(args->vparam[1].str);

        if (swap1<1 || swap1>args->cinput) illegal      = swap1;
        else if (swap2<1 || swap2>args->cinput) illegal = swap2;

        if (illegal) return GBS_global_string("illegal stream number '%i'", illegal);

        swap1--;
        swap2--;
    }
    else {
        return "expected 0 or 2 parameters";
    }

    for (i = 0; i<args->cinput; ++i) {
        STREAM_INOUT(args, i);
    }

    if (swap1 != swap2) {
        char *temp                           = (*args->voutput)[firstout+swap1].str;
        (*args->voutput)[firstout+swap1].str = (*args->voutput)[firstout+swap2].str;
        (*args->voutput)[firstout+swap2].str = temp;
    }

    return 0;
}

static GB_ERROR backfront_stream(GBL_command_arguments *args, int toback)
{
    int i;
    int stream_to_move;
    int firstout = *args->coutput;

    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput);
    if (args->cinput<1) return "need at least one input stream";
    if (args->cparam != 1) return "expecting one parameter";

    stream_to_move = atoi(args->vparam[0].str);
    if (stream_to_move<1 || stream_to_move>args->cinput) {
        return GBS_global_string("Illegal stream number '%i'", stream_to_move);
    }
    --stream_to_move;

    for (i = 0; i<args->cinput; ++i) {
        STREAM_INOUT(args, i);
    }

    if (toback) {
        if (stream_to_move<(args->cinput-1)) { /* not last */
            GBL  new_last = (*args->voutput)[firstout+stream_to_move];
            /* char *new_last = (*args->voutput)[firstout+stream_to_move].str; */
            for (i = stream_to_move+1; i<args->cinput; ++i) {
                (*args->voutput)[firstout+i-1] = (*args->voutput)[firstout+i];
                /* (*args->voutput)[firstout+i-1].str = (*args->voutput)[firstout+i].str; */
            }
            (*args->voutput)[firstout+args->cinput-1] = new_last;
        }
    }
    else { /* to front */
        if (stream_to_move != 0) { /* not first */
            GBL new_first = (*args->voutput)[firstout+stream_to_move];
            for (i = stream_to_move-1;  i >= 0; --i) {
                (*args->voutput)[firstout+i+1] = (*args->voutput)[firstout+i];
            }
            (*args->voutput)[0] = new_first;
        }
    }
    return 0;
}
static GB_ERROR gbl_toback (GBL_command_arguments *args) { return backfront_stream(args, 1); }
static GB_ERROR gbl_tofront(GBL_command_arguments *args) { return backfront_stream(args, 0); }

static GB_ERROR gbl_merge(GBL_command_arguments *args)
{
    const char *separator;

    switch (args->cparam) {
        case 0: separator = 0; break;
        case 1: separator = args->vparam[0].str; break;
        default : return "expect 0 or 1 parameter";
    }

    GBL_CHECK_FREE_PARAM(*args->coutput, 1);

    if (args->cinput) {
        int   i;
        void *str;

        str = GBS_stropen(1000);
        GBS_strcat(str, args->vinput[0].str);

        for (i = 1; i<args->cinput; ++i) {
            if (separator) GBS_strcat(str, separator);
            GBS_strcat(str, args->vinput[i].str);
        }

        STREAM_OUT(args, GBS_strclose(str));
    }

    return 0;
}

static GB_ERROR gbl_split(GBL_command_arguments *args) {
    const char *separator;
    int   split_mode = 0;       /* 0 = remove separator, 1 = split before separator, 2 = split behind separator */

    switch (args->cparam) {
        case 0:                 /* default behavior: split into lines and remove LF */
            separator  = "\n";
            break;
        case 2:
            split_mode = atoi(args->vparam[1].str);
            if (split_mode<0 || split_mode>2) {
                return GBS_global_string("Illegal split mode '%i' (valid: 0..2)", split_mode);
            }
            /* fall-through */
        case 1:
            separator          = args->vparam[0].str;
            break;
        default : return "expect 0, 1 or 2 parameters";
    }

    {
        size_t sepLen = strlen(separator);
        int    i;

        for (i = 0; i<args->cinput; ++i) {
            char *in   = args->vinput[i].str;
            char *from = in; /* search from here */

            while (in) {
                char *splitAt;
                GBL_CHECK_FREE_PARAM(*args->coutput, 1);

                splitAt = strstr(from, separator);
                if (splitAt) {
                    size_t  len;
                    char   *copy;

                    if (split_mode == 2) splitAt += sepLen; /* split behind separator */

                    len  = splitAt-in;
                    copy = (char*)malloc(len+1);

                    memcpy(copy, in, len);
                    copy[len] = 0;

                    STREAM_OUT(args, copy);

                    in   = splitAt + (split_mode == 0 ? sepLen : 0);
                    from = in+(split_mode == 1 ? sepLen : 0);
                }
                else {          // last part
                    STREAM_DUPOUT(args, in);
                    in = 0;
                }
            }
        }
    }

    return 0;
}

/********************************************************************************************
                                        Extended String Functions
********************************************************************************************/

static GB_ERROR gbl_extract_words(GBL_command_arguments *args)
{
    int   i;
    float len;
    if (args->cparam != 2) return "extract_words needs two parameters:\nextract_words(\"Characters\",min_characters)";
    len = atof(args->vparam[1].str);

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char *res;
        res = GBS_extract_words(args->vinput[i].str, args->vparam[0].str, len, 1);
        if (!res) return GB_get_error();

        STREAM_OUT(args, res);
    }
    return 0;
}

static GB_ERROR gbl_extract_sequence(GBL_command_arguments *args)
{
    int      i;
    float    len;
    GB_ERROR syntax_err = "extract_sequence needs two parameters:\nextract_sequence(\"Characters\",min_rel_characters [0.0-1.0])";

    if (args->cparam != 2) return syntax_err;
    len = atof(args->vparam[1].str);
    if (len <0.0 || len > 1.0) return syntax_err;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char *res;
        res = GBS_extract_words(args->vinput[i].str, args->vparam[0].str, len, 0);
        if (!res) return GB_get_error();

        STREAM_OUT(args, res);
    }
    return 0;
}

static GB_ERROR gbl_check(GBL_command_arguments *args)
{
    int i;

    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(exclude, "exclude=", "", "Remove characters 'str' before calculating"            );
    GBL_PARAM_BIT   (upper,   "toupper",  0,  "Convert all characters to uppercase before calculating");
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char buf[100];
        long id;
        id = GBS_checksum(args->vinput[i].str,upper,exclude);
        sprintf(buf,"%lX",id);
        STREAM_DUPOUT(args, buf);
    }
    return 0;
}

static GB_ERROR gbl_gcgcheck(GBL_command_arguments *args)
{
    int i;

    GBL_BEGIN_PARAMS;
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char buf[100];
        long id;
        id = GBS_gcgchecksum(args->vinput[i].str);
        sprintf(buf,"%li",id);
        STREAM_DUPOUT(args, buf);
    }
    return 0;
}

/********************************************************************************************
                                        SRT
********************************************************************************************/

static GB_ERROR gbl_srt(GBL_command_arguments *args)
{
    int i;
    int j;
    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);
    for (i=0;i<args->cinput;i++) {         /* go through all in streams    */
        char *source = args->vinput[i].str;
        for (j=0;j<args->cparam;j++) {
            char *hs = GBS_string_eval(source,args->vparam[j].str,args->gb_ref);
            if (!hs) return GB_get_error();
            if (source != args->vinput[i].str) free(source);
            source = hs;
        }
        if (source != args->vinput[i].str){
            STREAM_OUT(args, source);
        }
        else{
            STREAM_DUPOUT(args, source);
        }
    }
    return 0;
}

/********************************************************************************************
                                        Calculator Functions
********************************************************************************************/

/* numeric binary operators */

typedef int (*numeric_binary_operator)(int i1, int i2);
static char *apply_numeric_binop(const char *arg1, const char *arg2, void *client_data) {
    int                     i1     = atoi(arg1);
    int                     i2     = atoi(arg2);
    numeric_binary_operator nbo    = (numeric_binary_operator)client_data;
    int                     result = nbo(i1, i2);

    return GBS_global_string_copy("%i", result);
}

static int binop_plus(int i1, int i2) { return i1+i2; }
static int binop_minus(int i1, int i2) { return i1-i2; }
static int binop_mult(int i1, int i2) { return i1*i2; }
static int binop_div(int i1, int i2) { return i2 ? i1/i2 : 0; }
static int binop_rest(int i1, int i2) { return i2 ? i1%i2 : 0; }
static int binop_per_cent(int i1, int i2) { return i2 ? (i1*100)/i2 : 0; }

static GB_ERROR gbl_plus    (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_plus    ); }
static GB_ERROR gbl_minus   (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_minus   ); }
static GB_ERROR gbl_mult    (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_mult    ); }
static GB_ERROR gbl_div     (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_div     ); }
static GB_ERROR gbl_rest    (GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_rest    ); }
static GB_ERROR gbl_per_cent(GBL_command_arguments *args){ return gbl_apply_binary_operator(args, apply_numeric_binop, binop_per_cent); }


/********************************************************************************************
                                        Logical Functions
********************************************************************************************/

static GB_ERROR gbl_select(GBL_command_arguments *args) {
    int       i;
    GB_ERROR  error   = 0;
    GBDATA   *gb_main = (GBDATA*)GB_MAIN(args->gb_ref)->data;

    GBL_CHECK_FREE_PARAM(*args->coutput, args->cinput);

    for (i=0;i<args->cinput && !error;i++) { /* go through all in streams */
        int value = atoi(args->vinput[i].str);
        if (value<0 || value >= args->cparam) {
            error = GBS_global_string("Input stream value (%i) is out of bounds (0 to %i)", value, args->cparam-1);
        }
        else {
            char *result = GB_command_interpreter(gb_main, "", args->vparam[value].str, args->gb_ref, args->default_tree_name);
            if (!result) {
                error = GB_get_error();
            }
            else {
                STREAM_OUT(args, result);
            }
        }
    }
    return error;
}

/********************************************************************************************
                                        Database Functions
********************************************************************************************/
static GB_ERROR gbl_readdb(GBL_command_arguments *args)
{
    int i;
    void *strstr = GBS_stropen(1024);


    for (i=0;i<args->cparam;i++){
        GBDATA *gb = GB_search(args->gb_ref,args->vparam[i].str,GB_FIND);
        char *val;
        if(!gb) continue;
        val = GB_read_as_string(gb);
        GBS_strcat(strstr,val);
        free(val);
    }
    STREAM_OUT(args, GBS_strclose(strstr));
    return 0;
}


/********************************************************************************************
                                        Sequence Functions
********************************************************************************************/

typedef enum enum_gbt_item_type {
    GBT_ITEM_UNKNOWN,
    GBT_ITEM_SPECIES,
    GBT_ITEM_GENE
} gbt_item_type;

static gbt_item_type identify_gb_item(GBDATA *gb_item) {
    /* returns: GBT_ITEM_UNKNOWN    -> unknown database_item
     *          GBT_ITEM_SPECIES    -> /species_data/species
     *          GBT_ITEM_GENE       -> /species_data/species/gene_data/gene */

    GBDATA     *gb_father;
    const char *key;

    if (!gb_item) return GBT_ITEM_UNKNOWN;
    gb_father = GB_get_father(gb_item);
    if (!gb_father) return GBT_ITEM_UNKNOWN;

    key = GB_KEY(gb_item);

    if (strcmp(key, "species")                    == 0 &&
        strcmp(GB_KEY(gb_father), "species_data") == 0) {
        return GBT_ITEM_SPECIES;
    }

    if (strcmp(key, "gene")                        == 0 &&
        strcmp(GB_KEY(gb_father), "gene_data")     == 0 &&
        identify_gb_item(GB_get_father(gb_father)) == GBT_ITEM_SPECIES) {
        return GBT_ITEM_GENE;
    }

    return GBT_ITEM_UNKNOWN;
}

/* -------------------------------------------------------------------------------- */
/* taxonomy caching */

struct cached_taxonomy {
    char    *tree_name;         /* tree for which taxonomy is cached here */
    int      groups;            /* number of named groups in tree (at time of caching) */
    GB_HASH *taxonomy; /* keys: "!species", ">XXXXgroup" and "<root>".
                          Species and groups contain their first parent (i.e. '>XXXXgroup' or '<root>').
                          Species not in hash are not members of tree.
                          The 'XXXX' in groupname is simply a counter to avoid multiple groups with same name.
                          The group-db-entries are stored in hash as pointers ('>>%p') and
                          point to their own group entry ('>XXXXgroup')
                       */
};

static void free_cached_taxonomy(struct cached_taxonomy *ct) {
    free(ct->tree_name);
    GBS_free_hash(ct->taxonomy);
    free(ct);
}

static void build_taxonomy_rek(GBT_TREE *node, GB_HASH *tax_hash, const char *parent_group, int *group_counter) {
    if (node->is_leaf) {
        GBDATA *gb_species = node->gb_node;
        if (gb_species) { /* not zombie */
            GBS_write_hash(tax_hash, GBS_global_string("!%s", GBT_read_name(gb_species)), (long)strdup(parent_group));
        }
    }
    else {
        if (node->name) {       /* named group */
            char *hash_entry;
            const char *hash_binary_entry;
            (*group_counter)++;

            hash_entry = GBS_global_string_copy(">%04x%s", *group_counter, node->name);
            /* printf("hash_entry='%s' gb_node=%p\n", hash_entry, node->gb_node); */
            GBS_write_hash(tax_hash, hash_entry, (long)strdup(parent_group));

            hash_binary_entry = GBS_global_string(">>%p", node->gb_node);
            /* printf("hash_entry='%s' gb_node=%p\n", hash_binary_entry, node->gb_node); */
            GBS_write_hash(tax_hash, hash_binary_entry, (long)strdup(hash_entry));

            build_taxonomy_rek(node->leftson, tax_hash, hash_entry, group_counter);
            build_taxonomy_rek(node->rightson, tax_hash, hash_entry, group_counter);
            free(hash_entry);
        }
        else {
            build_taxonomy_rek(node->leftson, tax_hash, parent_group, group_counter);
            build_taxonomy_rek(node->rightson, tax_hash, parent_group, group_counter);
        }
    }
}

static GB_HASH *cached_taxonomies = 0;

static const char *tree_of_cached_taxonomy(struct cached_taxonomy *ct) {
    long        val;
    const char *key        = 0;
    const char *found_tree = 0;

    /* search the hash to find the correct cached taxonomy.
       searching for tree name does not work, because the tree may already be deleted
    */

    for (GBS_hash_first_element(cached_taxonomies, &key, &val);
         val;
         GBS_hash_next_element(cached_taxonomies, &key, &val))
    {
        struct cached_taxonomy *curr_ct = (struct cached_taxonomy *)val;
        if (ct == curr_ct) {
            found_tree = key;
            break;
        }
    }

    return found_tree;
}

static void flush_taxonomy_cb(GBDATA *gbd, int *cd_ct, GB_CB_TYPE cbt) {
    /* this cb is bound all tree db members below "/tree_data/tree_xxx" which
     * may have an effect on the displayed taxonomy
     * it invalidates cached taxonomies for that tree (when changed or deleted)
     */

    struct cached_taxonomy *ct    = (struct cached_taxonomy *)cd_ct;
    const char             *found = 0;
    GB_ERROR                error = 0;

    GBUSE(cbt);

    found = tree_of_cached_taxonomy(ct);

    if (found) {
#if defined(DEBUG)
        fprintf(stderr, "Deleting cached taxonomy ct=%p (tree='%s')\n", ct, found);
#endif /* DEBUG */
        GBS_write_hash(cached_taxonomies, found, 0); /* delete cached taxonomy from hash */
        free_cached_taxonomy(ct);
    }
#if defined(DEBUG)
    else {
        fprintf(stderr, "No tree found for cached_taxonomies ct=%p (already deleted?)\n", ct);
    }
#endif /* DEBUG */

    /*ignore error result*/ GB_remove_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), flush_taxonomy_cb, 0);

    if (found && !error) {
        GB_MAIN_TYPE *Main            = gb_get_main_during_cb();
        GBDATA       *gb_main         = (GBDATA*)Main->data;
        GBDATA       *gb_tree_refresh = GB_search(gb_main, AWAR_TREE_REFRESH, GB_INT);
        if (!gb_tree_refresh) {
            error = GBS_global_string("%s (while trying to force refresh)", GB_get_error());
        }
        else {
            /* #if defined(DEBUG) */
            /* printf("Skipped tree refresh by touching AWAR_TREE_REFRESH\n"); */
            /* #endif */ /* DEBUG */
            GB_touch(gb_tree_refresh); /* Note : force tree update */
        }
    }

    if (error) {
        fprintf(stderr, "Error in flush_taxonomy_cb: %s\n", error);
    }
}

static void flush_taxonomy_if_new_group_cb(GBDATA *gb_tree, int *cd_ct, GB_CB_TYPE cbt) {
    /* detects the creation of new groups and call flush_taxonomy_cb() manually */
    struct cached_taxonomy *ct        = (struct cached_taxonomy *)cd_ct;
    const char             *tree_name;

#if defined(DEBUG)
    fprintf(stderr, "flush_taxonomy_if_new_group_cb() has been called (cbt=%i)\n", cbt);
#endif /* DEBUG */

    tree_name = tree_of_cached_taxonomy(ct);
    if (tree_name) {
        int     groups = 0;
        GBDATA *gb_group_node;

        for (gb_group_node = GB_entry(gb_tree, "node");
             gb_group_node;
             gb_group_node = GB_nextEntry(gb_group_node))
        {
            if (GB_entry(gb_group_node, "group_name")) {
                groups++; /* count named groups only */
            }
        }

#if defined(DEBUG)
        fprintf(stderr, "cached_groups=%i  counted_groups=%i\n", ct->groups, groups);
#endif /* DEBUG */
        if (groups != ct->groups) {
#if defined(DEBUG)
            fprintf(stderr, "Number of groups changed -> invoking flush_taxonomy_cb() manually\n");
#endif /* DEBUG */
            flush_taxonomy_cb(gb_tree, cd_ct, cbt);
        }
    }
#if defined(DEBUG)
    else {
        fprintf(stderr, "cached taxonomy no longer valid.\n");
    }
#endif /* DEBUG */
}


static struct cached_taxonomy *get_cached_taxonomy(GBDATA *gb_main, const char *tree_name, GB_ERROR *error) {
    long cached;
    *error = 0;
    if (!cached_taxonomies) {
        cached_taxonomies = GBS_create_hash(20, GB_IGNORE_CASE);
    }
    cached = GBS_read_hash(cached_taxonomies, tree_name);
    if (!cached) {
        GBT_TREE *tree = GBT_read_tree(gb_main, tree_name, sizeof(*tree));
        if (!tree) {
            *error = GB_get_error();
        }
        else {
            *error = GBT_link_tree(tree, gb_main, GB_FALSE, 0, 0);
        }

        if (!*error) {
            GBDATA *gb_tree = GBT_get_tree(gb_main, tree_name);
            if (!gb_tree) {
                *error = GB_export_error("Can't find 'tree_data'");
            }
            else {
                struct cached_taxonomy *ct            = malloc(sizeof(*ct));
                long                    nodes         = GBT_count_nodes(tree);
                int                     group_counter = 0;

                ct->tree_name = strdup(tree_name);
                ct->taxonomy  = GBS_create_hash((int)(nodes*2), GB_IGNORE_CASE);
                ct->groups    = 0; // counted below

                build_taxonomy_rek(tree, ct->taxonomy, "<root>", &group_counter);
                cached = (long)ct;
                GBS_write_hash(cached_taxonomies, tree_name, (long)ct);

                GB_remove_callback(gb_tree, GB_CB_SON_CREATED, flush_taxonomy_if_new_group_cb, 0);
                GB_add_callback(gb_tree, GB_CB_SON_CREATED, flush_taxonomy_if_new_group_cb, (int*)ct);

                {
                    GBDATA *gb_tree_entry = GB_entry(gb_tree, "tree");
                    GBDATA *gb_group_node;

                    if (gb_tree_entry) {
                        GB_remove_callback(gb_tree_entry, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), flush_taxonomy_cb, 0); // remove all
                        GB_add_callback(gb_tree_entry, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), flush_taxonomy_cb, (int*)ct);
                    }

                    /* add callbacks for all node/group_name subentries */
                    for (gb_group_node = GB_entry(gb_tree, "node");
                         gb_group_node;
                         gb_group_node = GB_nextEntry(gb_group_node))
                    {
                        GBDATA *gb_group_name = GB_entry(gb_group_node, "group_name");
                        if (gb_group_name) { /* group with id = 0 has no name */
                            GB_remove_callback(gb_group_name, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), flush_taxonomy_cb, 0); // remove all
                            GB_add_callback(gb_group_name, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), flush_taxonomy_cb, (int*)ct);
                            ct->groups++;
                        }
                    }
                }
#if defined(DEBUG)
                fprintf(stderr, "Created taxonomy hash for '%s' (ct=%p)\n", tree_name, ct);
#endif /* DEBUG */
            }
        }

        if (tree) GBT_delete_tree(tree);
    }

    if (!*error) {
        struct cached_taxonomy *ct = (struct cached_taxonomy*)cached;
        gb_assert(ct);
        return ct;
    }

    return 0;
}

static char *get_taxonomy_string(GB_HASH *tax_hash, const char *group_key, int depth, GB_ERROR *error) {
    long  found;
    char *result = 0;

    gb_assert(depth>0);
    gb_assert(!(group_key[0] == '>' && group_key[1] == '>')); // internal group-pointers not allowed here!

    found = GBS_read_hash(tax_hash, group_key);
    if (found) {
        const char *parent_group_key            = (const char *)found;
        if (strcmp(parent_group_key, "<root>") == 0) { // root reached
            result = strdup(group_key+5); // return own group name
        }
        else {
            if (depth>1) {
                char *parent_name = get_taxonomy_string(tax_hash, parent_group_key, depth-1, error);
                if (parent_name) {
                    result = GBS_global_string_copy("%s/%s", parent_name, group_key+5);
                    free(parent_name);
                }
                else {
                    *error = GBS_global_string("In get_taxonomy_string(%s): %s", group_key, *error);
                    result = 0;
                }
            }
            else {
                result = strdup(group_key+5); // return own group name
            }
        }
    }
    else {
        *error = GBS_global_string("Not in tax_hash: '%s'", group_key);
    }
    return result;
}

static const char *get_taxonomy(GBDATA *gb_species_or_group, const char *tree_name, int is_current_tree, int depth, GB_ERROR *error) {
    GBDATA                 *gb_main = GB_get_root(gb_species_or_group);
    struct cached_taxonomy *tax     = get_cached_taxonomy(gb_main, tree_name, error);
    const char             *result  = 0;

    if (tax) {
        GBDATA *gb_name       = GB_entry(gb_species_or_group, "name");
        GBDATA *gb_group_name = GB_entry(gb_species_or_group, "group_name");

        if (gb_name && !gb_group_name) { /* it's a species */
            char *name = GB_read_string(gb_name);
            if (name) {
                GB_HASH *tax_hash = tax->taxonomy;
                long     found    = GBS_read_hash(tax_hash, GBS_global_string("!%s", name));

                if (found) {
                    const char *parent_group = (const char *)found;

                    if (strcmp(parent_group, "<root>") == 0) {
                        result = ""; /* not member of any group */
                    }
                    else {
                        static char *parent = 0;
                        
                        freeset(parent, get_taxonomy_string(tax_hash, parent_group, depth, error));
                        result = parent;
                    }
                }
                else {
                    result = GBS_global_string("Species '%s' not in '%s'", name, tree_name);
                }
                free(name);
            }
            else {
                *error = GBS_global_string("Species without 'name' entry!");
            }
        }
        else if (gb_group_name && !gb_name) { /* it's a group */
            char *group_name = GB_read_string(gb_group_name);
            if (group_name) {
                if (is_current_tree) {
                    GB_HASH *tax_hash = tax->taxonomy;
                    long     found    = GBS_read_hash(tax_hash, GBS_global_string(">>%p", gb_species_or_group));

                    if (found) {
                        static char *full_group = 0;
                        const char  *group_id   = (const char *)found;

                        freeset(full_group, get_taxonomy_string(tax_hash, group_id, depth, error));
                        result = full_group;
                    }
                    else {
                        result = GBS_global_string("Group '%s' not in '%s'", group_name, tree_name);
                    }
                }
                else {
                    *error = "It's not possible to specify the tree name in taxonomy() for groups";
                }
                free(group_name);
            }
            else {
                *error = "Group without 'group_name' entry";
            }
        }
        else if (gb_group_name) {
            *error = "Container has 'name' and 'group_name' entry - can't detect container type";
        }
        else {
            *error = "Container has neighter 'name' nor 'group_name' entry - can't detect container type";
        }
    }

    return result;
}

static GB_ERROR gbl_taxonomy(GBL_command_arguments *args)
{
    char     *tree_name = 0;
    GB_ERROR  error     = 0;


    if (args->cparam<1 || args->cparam>2) {
        error = "\"taxonomy\" syntax: \"taxonomy\"([tree_name,] count)";
    }
    else {
        int   is_current_tree = 0;
        int   count           = -1;
        char *result          = 0;

        if (args->cparam == 1) {   /* only 'count' */
            if (!args->default_tree_name) {
                result = strdup("No default tree");
            }
            else {
                tree_name = strdup(args->default_tree_name);
                count = atoi(args->vparam[0].str);
                is_current_tree = 1;
            }
        }
        else { /* 'tree_name', 'count' */
            tree_name = strdup(args->vparam[0].str);
            count     = atoi(args->vparam[1].str);
        }

        if (!result) {
            if (count<1) {
                error = GBS_global_string("Illegal count '%i' (allowed 1..n)", count);
            }
            if (!error) {
                const char *taxonomy_string = get_taxonomy(args->gb_ref, tree_name, is_current_tree, count, &error);
                if (taxonomy_string) result = strdup(taxonomy_string);
            }
        }

        ad_assert(result || error);
        if (result) STREAM_OUT(args, result);
    }
    if (tree_name) free(tree_name);
    return error;
}

static GB_ERROR gbl_sequence(GBL_command_arguments *args)
{
    char     *use;
    GBDATA   *gb_seq;
    GB_ERROR  error = 0;

    if (args->cparam!=0) return "\"sequence\" syntax: \"sequence\" (no parameters)";
    args->vparam = args->vparam;
    if (args->cinput==0) return "No input stream";
    GBL_CHECK_FREE_PARAM(*args->coutput,1);

    switch (identify_gb_item(args->gb_ref)) {
        case GBT_ITEM_UNKNOWN: {
            error = "'sequence' used for unknown item";
            break;
        }
        case GBT_ITEM_SPECIES: {
            use    = GBT_get_default_alignment(GB_get_root(args->gb_ref));
            gb_seq = GBT_read_sequence(args->gb_ref,use);

            if (gb_seq) STREAM_OUT(args, GB_read_string(gb_seq));
            else        STREAM_DUPOUT(args, ""); /* if current alignment does not exist -> return empty string */

            free(use);

            break;
        }
        case GBT_ITEM_GENE: {
            char *seq = GBT_read_gene_sequence(args->gb_ref, GB_TRUE, 0);
            if (!seq) error = GB_get_error();
            if (!error) STREAM_OUT(args, seq);

            break;
        }
    }

    return error;
}

static GB_ERROR gbl_export_sequence(GBL_command_arguments *args) {
    GB_ERROR error = 0;

    if (args->cparam!=0) {
        error = "\"sequence\" syntax: \"export_sequence\" (no parameters)";
    }
    else if (args->cinput==0) {
        error = "No input stream";
    }
    else {
        GBL_CHECK_FREE_PARAM(*args->coutput,1);

        switch (identify_gb_item(args->gb_ref)) {
            case GBT_ITEM_UNKNOWN: {
                error = "'export_sequence' used for unknown item";
                break;
            }
            case GBT_ITEM_SPECIES: {
                if (get_export_sequence == 0) {
                    error = "No export-sequence-hook defined (can't use 'export_sequence' here)";
                }
                else {
                    size_t      len;
                    const char *seq = get_export_sequence(args->gb_ref, &len, &error);

                    ad_assert(error || seq);
                    
                    if (seq) STREAM_OUT(args, GB_strduplen(seq, len));
                }
                break;
            }
            case GBT_ITEM_GENE: {
                error = "'export_sequence' cannot be used for gene";
                break;
            }
        }
    }
    return error;
}

static GB_ERROR gbl_sequence_type(GBL_command_arguments *args)
{
    GB_ERROR error = 0;

    if (args->cparam) error = "\"sequence_type\" syntax: \"sequence\" (no parameters)";
    else {
        GBL_CHECK_FREE_PARAM(*args->coutput,1);
        if (!args->cinput) error = "No input stream";
        else {
            char *use = GBT_get_default_alignment(GB_get_root(args->gb_ref));
            STREAM_OUT(args, GBT_get_alignment_type_string(GB_get_root(args->gb_ref), use));
            free(use);
        }
    }

    return error;
}

static GB_ERROR gbl_format_sequence(GBL_command_arguments *args)
{
    GB_ERROR error = 0;
    int      ic;

    GBL_BEGIN_PARAMS;
    GBL_PARAM_SIZET (firsttab, "firsttab=", 10,   "Indent first line");
    GBL_PARAM_SIZET (tab,      "tab=",      10,   "Indent not first line");
    GBL_PARAM_BIT   (numleft,  "numleft",   0,    "Numbers left of sequence");
    GBL_PARAM_SIZET (gap,      "gap=",      10,   "Insert ' ' every n sequence characters");
    GBL_PARAM_SIZET (width,    "width=",    50,   "Sequence width (bases only)");
    GBL_PARAM_STRING(nl,       "nl=",       " ",  "Break line at characters 'str' if wrapping needed");
    GBL_PARAM_STRING(forcenl,  "forcenl=",  "\n", "Always break line at characters 'str'");
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    for (ic = 0; ic<args->cinput; ++ic) {
        GBL_CHECK_FREE_PARAM(*args->coutput,1);

        {
            const char *src           = args->vinput[ic].str;
            size_t      data_size     = strlen(src);
            size_t      needed_size;
            char       *result        = 0;
            int         simple_format = (strcmp(args->command,"format") == 0);

            {
                size_t lines;
                size_t line_size;

                if (simple_format) {
                    lines     = data_size/2 + 1; // worst case
                    line_size = tab + width + 1;
                }
                else {
                    size_t gapsPerLine = (width-1)/gap;
                    lines              = data_size/width+1;
                    line_size          = tab + width + gapsPerLine + 1;
                }

                needed_size = lines*line_size + firsttab + 1 + 10;
            }

            result = malloc(needed_size);
            if (!result) {
                error = GBS_global_string("Out of memory (tried to alloc %zu bytes)", needed_size);
            }
            else {
                char   *dst       = result;
                size_t  rest_data = data_size;
                int     i;

                if (simple_format) {
                    /* format string w/o gaps or numleft
                     * does word-wrapping at chars in nl
                     */
                
                    /* build wrap table */
                    unsigned char isWrapChar[256];
                    memset(isWrapChar, 0, sizeof(isWrapChar));
                    for (i = 0; nl[i]; ++i) isWrapChar[(unsigned char)nl[i]] = 1;
                    for (i = 0; forcenl[i]; ++i) isWrapChar[(unsigned char)forcenl[i]] = 2;

                    if (firsttab>0) {
                        memset(dst, ' ', firsttab);
                        dst += firsttab;
                    }

                    while (rest_data>width) {
                        int take;
                        int move;
                        int took;

                        for (take = width; take > 0; --take) {
                            if (isWrapChar[(unsigned char)src[take]]) break;
                        }
                        if (take <= 0) { /* no wrap character found -> hard wrap at width */
                            take  = move = width;
                        }
                        else { /* soft wrap at last found wrap character */
                            move = take+1;
                        }

                        for (took = 0; took<take; took++) {
                            char c = src[took];
                            if (isWrapChar[(unsigned char)c] == 2) { /* forced newline */
                                take = took;
                                move = take+1;
                                break;
                            }
                            dst[took] = c;
                        }

                        /* memcpy(dst, src, take); */
                        dst       += take;
                        src       += move;
                        rest_data -= move;

                        if (rest_data>0) {
                            *dst++ = '\n';
                            if (tab>0) {
                                memset(dst, ' ', tab);
                                dst += tab;
                            }
                        }
                    }

                    if (rest_data>0) {
                        size_t j, k;
                        for (j = 0, k = 0; j<rest_data; ++j) {
                            char c = src[j];

                            if (isWrapChar[(unsigned char)c] == 2) {
                                dst[k++] = '\n';
                                if (tab>0) {
                                    memset(dst+k, ' ', tab);
                                    k += tab;
                                }
                            }
                            else {
                                dst[k++] = c;
                            }
                        }
                        src       += j;
                        dst       += k;
                        rest_data  = 0;
                    }
                }
                else {
                    /* "format_sequence" with gaps and numleft */
                    char       *format    = 0;
                    const char *src_start = src;

                    if (numleft) {
                        if (firsttab>0) {
                            char *firstFormat = GBS_global_string_copy("%%-%zuu ", firsttab-1);
                            dst += sprintf(dst, firstFormat, (size_t)1);
                            free(firstFormat);
                        }
                        else {
                            dst += sprintf(dst, "%zu ", (size_t)1);
                        }
                        format = tab>0 ? GBS_global_string_copy("%%-%zuu ", tab-1) : strdup("%u ");
                    }
                    else if (firsttab>0) {
                        memset(dst, ' ', firsttab);
                        dst += firsttab;
                    }

                    while (rest_data>0) {
                        size_t take = rest_data>width ? width : rest_data;

                        rest_data -= take;

                        while (take>gap) {
                            memcpy(dst, src, gap);
                            dst  += gap;
                            src  += gap;
                            *dst++ = ' ';
                            take -= gap;
                        }

                        memcpy(dst, src, take);
                        dst += take;
                        src += take;

                        if (rest_data>0) {
                            *dst++ = '\n';
                            if (numleft) {
                                dst += sprintf(dst, format, (src-src_start)+1);
                            }
                            else if (tab>0) {
                                memset(dst, ' ', tab);
                                dst += tab;
                            }
                        }
                    }

                    free(format);
                }

                *dst++ = 0;         /* close str */

#if defined(DEBUG)
                { /* check for array overflow */
                    size_t  used_size   = dst-result;
                    char   *new_result;

                    gb_assert(used_size <= needed_size);

                    new_result = realloc(result, used_size);
                    if (!new_result) {
                        error = "Out of memory";
                    }
                    else {
                        result = new_result;
                    }
                }
#endif /* DEBUG */
            }

            if (!error) STREAM_OUT(args, result);
            else free(result);
        }
    }
    return error;
}

/********************************************************************************************
                                        Filter Functions
********************************************************************************************/

static char *gbl_read_seq_sai_or_species(const char *species, const char *sai, const char *ali, size_t *seqLen) {
    /* Reads the alignment 'ali'  of 'species' or 'sai'.
     * If 'ali' is NULL, use default alignment.
     * Returns NULL in case of error (which is exported then)
     */

    char     *seq   = NULL;
    GB_ERROR  error = 0;

    int sources = !!species + !!sai;
    if (sources != 1) {
        error = "Either parameters 'species' or 'SAI' must be specified";
    }
    else {
        GBDATA     *gb_main = gb_local->gbl.gb_main;
        GBDATA     *gb_item = 0;
        const char *what    = 0;
        const char *name    = 0;
        
        if (species) {
            gb_item = GBT_find_species(gb_main, species);
            what    = "species";
            name    = species;
        }
        else {
            gb_item = GBT_find_SAI(gb_main, sai);
            what    = "SAI";
            name    = sai;
        }

        if (!gb_item) error = GBS_global_string("Can't find %s '%s'", what, name);
        else {
            char *freeMe = 0;

            if (!ali) {
                ali = freeMe = GBT_get_default_alignment(gb_main);
                if (!ali) error = "can't detect default alignment";
            }

            if (ali) {
                GBDATA *gb_ali = GB_entry(gb_item, ali);

                if (gb_ali) {
                    GBDATA *gb_seq;

                    for (gb_seq = GB_child(gb_ali); gb_seq; gb_seq = GB_nextChild(gb_seq)) {
                        long type = GB_read_type(gb_seq);
                        if (type == GB_BITS) {
                            seq     = GB_read_bits(gb_seq,'-','+');
                            if (seqLen) *seqLen = GB_read_bits_count(gb_seq);
                            break;
                        }
                        if (type == GB_STRING) {
                            seq     = GB_read_string(gb_seq);
                            if (seqLen) *seqLen = GB_read_string_count(gb_seq);
                            break;
                        }
                    }
                }

                if (!seq) error = GBS_global_string("%s '%s' has no (usable) data in alignment '%s'", what, name, ali);            
            }
            free(freeMe);
        }
    }

    if (error) {
        gb_assert(!seq);
        GB_export_error(error);
    }

    return seq;
}

struct common_filter_params {
    const char *align;
    const char *sai;
    const char *species;
    int         first;
    int         pairwise;
};

#define GBL_COMMON_FILTER_PARAMS                                               \
    struct common_filter_params common_param;                                        \
    GBL_STRUCT_PARAM_STRING(common_param, align,    "align=",    0,   "alignment to use (defaults to default alignment)"); \
    GBL_STRUCT_PARAM_STRING(common_param, sai,      "SAI=",      0,   "Use default sequence of given SAI as a filter"); \
    GBL_STRUCT_PARAM_STRING(common_param, species,  "species=",  0,   "Use default sequence of given species as a filter"); \
    GBL_STRUCT_PARAM_BIT   (common_param, first,    "first=",    0,   "Use 1st stream as filter for other streams"); \
    GBL_STRUCT_PARAM_BIT   (common_param, pairwise, "pairwise=", 0,   "Use 1st stream as filter for 2nd, 3rd for 4th, ...")

typedef char* (*filter_fun)(const char *seq, const char *filter, size_t flen, void *param);
/* Note:
 * filter_fun has to return a heap copy of the filter-result.
 * if 'flen' != 0, it contains the length of 'filter'
 * 'param' may be any client data
 */

static GB_ERROR apply_filters(GBL_command_arguments *args, struct common_filter_params *common, filter_fun filter_one, void *param) {
    GB_ERROR error = 0;
    
    if (args->cinput==0) error = "No input stream";
    else {
        int methodCount = !!common->sai + !!common->species + !!common->pairwise + !!common->first;

        if (methodCount != 1) error = "Need exactly one of the parameters 'SAI', 'species', 'pairwise' or 'first'";
        else {
            if (common->pairwise) {
                if (args->cinput % 2) error = "Using 'pairwise' requires an even number of input streams";
                else {
                    int i;
                    for (i = 1; i<args->cinput; i += 2) {
                        STREAM_OUT(args, filter_one(args->vinput[i].str, args->vinput[i-1].str, 0, param));
                    }
                }
            }
            else {
                int     i      = 0;
                char   *filter = 0;
                size_t  flen   = 0;

                if (common->first) {
                    if (args->cinput<2) error = "Using 'first' needs at least 2 input streams";
                    else {
                        const char *in = args->vinput[i++].str;
                        gb_assert(in);
                        
                        flen   = strlen(in);
                        filter = GB_strduplen(in, flen);
                    }
                }
                else {
                    filter = gbl_read_seq_sai_or_species(common->species, common->sai, common->align, &flen);
                    if (!filter) error = GB_get_error();
                }

                gb_assert(filter || error);
                if (filter) {
                    for (; i<args->cinput; ++i) {
                        STREAM_OUT(args, filter_one(args->vinput[i].str, filter, flen, param));
                    }
                }
                free(filter);
            }
        }
    }
    return error;
}

/* ------------------------- */
/*      calculate diff       */

struct diff_params {
    char equalC;
    char diffC;
};
static char *calc_diff(const char *seq, const char *filter, size_t flen, void *paramP) {
    // filters 'seq' through 'filter'
    // - replace all equal     positions by 'equal_char' (if != 0)
    // - replace all differing positions by 'diff_char'  (if != 0)

    GBUSE(flen);

    struct diff_params *param = (struct diff_params*)paramP;
    char equal_char = param->equalC;
    char diff_char  = param->diffC;

    char *result = strdup(seq);
    int   p;

    for (p = 0; result[p] && filter[p]; ++p) {
        if (result[p] == filter[p]) {
            if (equal_char) result[p] = equal_char;
        }
        else {
            if (diff_char) result[p] = diff_char;
        }
    }

    // if 'seq' is longer than 'filter' and diff_char is given
    // -> fill rest of 'result' with 'diff_char'
    if (diff_char) {
        for (; result[p]; ++p) {
            result[p] = diff_char;
        }
    }

    return result;
}
static GB_ERROR gbl_diff(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_COMMON_FILTER_PARAMS;

    struct diff_params param;
    GBL_STRUCT_PARAM_CHAR(param, equalC,   "equal=",    '.', "symbol for equal characters");
    GBL_STRUCT_PARAM_CHAR(param, diffC,    "differ=",   0,   "symbol for diff characters (default: use char from input stream)");
    
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    return apply_filters(args, &common_param, calc_diff, &param);
}

/* ------------------------- */
/*      standard filter      */

struct filter_params { // used by gbl_filter and gbl_change_gc
    enum { FP_FILTER, FP_MODIFY } function;

    const char *include;
    const char *exclude;

    // FP_MODIFY only: 
    int         change_pc;
    const char *change_to;
};

static char *filter_seq(const char *seq, const char *filter, size_t flen, void *paramP) {
    struct filter_params *param = (struct filter_params*)paramP;

    size_t slen     = strlen(seq);
    if (!flen) flen = strlen(filter);
    size_t mlen     = slen<flen ? slen : flen;

    struct GBS_strstruct *out = GBS_stropen(mlen);

    const char *charset;
    int         include;

    if (param->include) {
        charset = param->include;
        include = 1;
    }
    else {
        gb_assert(param->exclude);
        charset = param->exclude;
        include = 0;
    }

    size_t pos  = 0;
    size_t rest = slen;
    size_t ctl  = 0;
    if (param->function == FP_MODIFY) ctl  = strlen(param->change_to);

    int inset = 1; // 1 -> check chars in charset, 0 -> check chars NOT in charset
    while (rest) {
        size_t count;
        if (pos >= flen) {      // behind filter
            // trigger last loop
            count = rest;
            inset = 0; // if 'include' -> 'applies' will get false, otherwise true
                       // (meaning is: behind filter nothing can match 'include' or 'exclude')
        }
        else {
            count = (inset ? strspn : strcspn)(filter+pos, charset); // count how many chars are 'inset'
        }
        if (count) {
            int applies = !!include == !!inset; // true -> 'filter' matches 'include' or doesn't match 'exclude'
            if (count>rest) count = rest;

            switch(param->function) {
                case FP_FILTER:
                    if (applies) GBS_strncat(out, seq+pos, count);
                    break;

                case FP_MODIFY:
                    if (applies) { // then modify
                        size_t i;
                        for (i = 0; i<count; i++) {
                            char c = seq[pos+i];
                            if (isalpha(c) && GB_random(100)<param->change_pc) c = param->change_to[GB_random(ctl)];
                            GBS_chrcat(out, c);
                        }
                    }
                    else { // otherwise simply copy
                        GBS_strncat(out, seq+pos, count);
                    }
                    break;
            }

            pos  += count;
            rest -= count;
        }
        inset = 1-inset; // toggle
    }
    return GBS_strclose(out);
}

static GB_ERROR gbl_filter(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_COMMON_FILTER_PARAMS;

    struct filter_params param;
    GBL_STRUCT_PARAM_STRING(param, exclude, "exclude=", 0, "Exclude colums");
    GBL_STRUCT_PARAM_STRING(param, include, "include=", 0, "Include colums");
    param.function = FP_FILTER;
    
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    GB_ERROR error  = 0;
    int      inOrEx = !!param.include + !!param.exclude;

    if (inOrEx != 1)    error = "Need exactly one parameter of: 'include', 'exclude'";
    else error                = apply_filters(args, &common_param, filter_seq, &param);

    return error;
}

static GB_ERROR gbl_change_gc(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_COMMON_FILTER_PARAMS;

    struct filter_params param;
    GBL_STRUCT_PARAM_STRING(param, exclude,   "exclude=", 0,    "Exclude colums");
    GBL_STRUCT_PARAM_STRING(param, include,   "include=", 0,    "Include colums");
    GBL_STRUCT_PARAM_INT   (param, change_pc, "change=",  0,    "percentage of changed columns (default: silently change nothing)");
    GBL_STRUCT_PARAM_STRING(param, change_to, "to=",      "GC", "change to one of this");
    param.function = FP_MODIFY;
    
    GBL_TRACE_PARAMS(args->cparam,args->vparam);
    GBL_END_PARAMS;

    GBL_CHECK_FREE_PARAM(*args->coutput,args->cinput);

    GB_ERROR error  = 0;
    int      inOrEx = !!param.include + !!param.exclude;

    if (inOrEx != 1) error = "Need exactly one parameter of: 'include', 'exclude'";
    else {
        error = apply_filters(args, &common_param, filter_seq, &param);
    }

    return error;
}

/********************************************************************************************
                                        Exec Functions
********************************************************************************************/

static GB_ERROR gbl_exec(GBL_command_arguments *args)
{
    GB_ERROR error = 0;

    if (args->cparam==0) {
        error = "exec needs parameters:\nexec(command,...)";
    }
    else {
        // write inputstreams to temp file:
        char *inputname;
        int i;
        {
            char *filename = GB_unique_filename("arb_exec_input", "tmp");
            FILE *out      = GB_fopen_tempfile(filename, "wt", &inputname);

            if (!out) error = GB_get_error();
            else {
                for (i=0; i<args->cinput; i++) {               // go through all in streams
                    fprintf(out,"%s\n",args->vinput[i].str);
                }
                fclose(out);
            }
            free(filename);
        }

        if (!error) {
            // build shell command to execute
            char *sys;
            {
                struct GBS_strstruct *str = GBS_stropen(1000);
                
                GBS_strcat(str, args->vparam[0].str);
                for (i=1; i<args->cparam; i++) {   // go through all in params
                    GBS_strcat(str," \'");
                    GBS_strcat(str, args->vparam[i].str);
                    GBS_chrcat(str,'\'');
                }
                GBS_strcat(str, " <");
                GBS_strcat(str, inputname);
                
                sys = GBS_strclose(str);
            }

            char *result = 0;
            {
                FILE *in = popen(sys,"r");
                if (in) {
                    void *str = GBS_stropen(4096);

                    while ( (i=getc(in)) != EOF ) { GBS_chrcat(str,i); }
                    result = GBS_strclose(str);
                    pclose(in);
                }
                else {
                    error = GBS_global_string("Cannot execute shell command '%s'", sys);
                }
            }

            if (!error) {
                gb_assert(result);
                STREAM_OUT(args, result);
            }

            free(sys);
        }

        gb_assert(GB_is_privatefile(inputname, GB_FALSE));
        if (GB_unlink(inputname)<0 && !error) error = GB_get_error();
        free(inputname);
    }

    return error;
}


static struct GBL_command_table gbl_command_table[] = {
    {"caps",             gbl_string_convert },
    {"change",           gbl_change_gc },
    {"checksum",         gbl_check },
    {"command",          gbl_command },
    {"compare",          gbl_compare },
    {"icompare",         gbl_icompare },
    {"contains",         gbl_contains },
    {"icontains",        gbl_icontains },
    {"count",            gbl_count },
    {"crop",             gbl_crop },
    {"cut",              gbl_cut },
    {"dd",               gbl_dd },
    {"define",           gbl_define },
    {"diff",             gbl_diff },
    {"div",              gbl_div },
    {"do",               gbl_do },
    {"drop",             gbl_drop },
    {"dropempty",        gbl_dropempty },
    {"dropzero",         gbl_dropzero },
    {"echo",             gbl_echo },
    {"equals",           gbl_equals },
    {"iequals",          gbl_iequals },
    {"eval",             gbl_eval },
    {"exec",             gbl_exec },
    {"export_sequence",  gbl_export_sequence },
    {"extract_sequence", gbl_extract_sequence },
    {"extract_words",    gbl_extract_words },
    {"filter",           gbl_filter },
    {"format",           gbl_format_sequence },
    {"format_sequence",  gbl_format_sequence },
    {"gcgchecksum",      gbl_gcgcheck },
    {"head",             gbl_head },
    {"keep",             gbl_keep },
    {"left",             gbl_head },
    {"len",              gbl_len },
    {"lower",            gbl_string_convert },
    {"merge",            gbl_merge },
    {"mid",              gbl_mid },
    {"mid0",             gbl_mid0 },
    {"minus",            gbl_minus },
    {"mult",             gbl_mult },
    {"origin_gene",      gbl_origin },
    {"origin_organism",  gbl_origin },
    {"partof",           gbl_partof },
    {"ipartof",          gbl_ipartof },
    {"per_cent",         gbl_per_cent },
    {"plus",             gbl_plus },
    {"pretab",           gbl_pretab },
    {"readdb",           gbl_readdb },
    {"remove",           gbl_remove },
    {"rest",             gbl_rest },
    {"right",            gbl_tail },
    {"select",           gbl_select },
    {"sequence",         gbl_sequence },
    {"sequence_type",    gbl_sequence_type },
    {"split",            gbl_split },
    {"srt",              gbl_srt },
    {"streams",          gbl_streams },
    {"swap",             gbl_swap },
    {"tab",              gbl_tab },
    {"tail",             gbl_tail },
    {"taxonomy",         gbl_taxonomy },
    {"toback",           gbl_toback },
    {"tofront",          gbl_tofront },
    {"trace",            gbl_trace },
    {"translate",        gbl_translate },
    {"upper",            gbl_string_convert },
    {0,0}

};

void gbl_install_standard_commands(GBDATA *gb_main)
{
    gb_install_command_table(gb_main,gbl_command_table);
}
