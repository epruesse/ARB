// =============================================================== //
//                                                                 //
//   File      : adlang1.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_aci.h"
#include "gb_key.h"
#include "gb_localdata.h"

#include <adGene.h>
#include "TreeNode.h"
#include <ad_cb.h>

#include <arb_defs.h>
#include <arb_strbuf.h>
#include <arb_file.h>
#include <aw_awar_defs.hxx>

#include <cctype>
#include <cmath>
#include <algorithm>

// hook for 'export_sequence'

static gb_export_sequence_cb get_export_sequence = 0;

NOT4PERL void GB_set_export_sequence_hook(gb_export_sequence_cb escb) {
    gb_assert(!get_export_sequence || !escb); // avoid unwanted overwrite
    get_export_sequence = escb;
}

// global ACI/SRT debug switch
static int trace = 0;

void GB_set_ACISRT_trace(int enable) { trace = enable; }
int GB_get_ACISRT_trace() { return trace; }


// export stream

#define PASS_2_OUT(args,s)  (args)->output.insert(s)
#define COPY_2_OUT(args,s)  PASS_2_OUT(args, strdup(s))
#define IN_2_OUT(args,i)    PASS_2_OUT(args, args->input.get_smart(i))
#define PARAM_2_OUT(args,i) PASS_2_OUT(args, args->param.get_smart(i))

// ----------------------------
//      Parameter functions

struct gbl_param {
    gbl_param  *next;
    GB_TYPES    type;                               // type of variable
    void       *varaddr;                            // address of variable where value gets stored
    const char *param_name;                         // parameter name (e.g. 'include=')
    const char *help_text;                          // help text for parameter
};

#define GBL_BEGIN_PARAMS gbl_param *params = 0

static void gbl_new_param(gbl_param **pp, GB_TYPES type, void *vaddr, const char *param_name, const char *help_text) {
    gbl_param *gblp = (gbl_param *)GB_calloc(1, sizeof(gbl_param));

    gblp->next = *pp;
    *pp         = gblp;

    gblp->type       = type;
    gblp->varaddr    = vaddr;
    gblp->param_name = param_name;
    gblp->help_text  = help_text;
}

typedef const char   *String;
typedef int           bit;
typedef unsigned int  uint;

static int gbl_param_int(const char *param_name, int def, const char *help_text, gbl_param **pp, int *vaddr) {
    gbl_new_param(pp, GB_INT, vaddr, param_name, help_text);
    return def;
}

static char gbl_param_char(const char *param_name, char def, const char *help_text, gbl_param **pp, char *vaddr) {
    gbl_new_param(pp, GB_BYTE, vaddr, param_name, help_text);
    return def;
}

static uint gbl_param_uint(const char *param_name, uint def, const char *help_text, gbl_param **pp, uint *vaddr) {
    gbl_new_param(pp, GB_INT, vaddr, param_name, help_text);
    return def;
}

static const char *gbl_param_String(const char *param_name, const char *def, const char *help_text, gbl_param **pp, String *vaddr) {
    gbl_new_param(pp, GB_STRING, vaddr, param_name, help_text);
    return def;
}

static int gbl_param_bit(const char *param_name, int def, const char *help_text, gbl_param **pp, bit *vaddr) {
    gbl_new_param(pp, GB_BIT, vaddr, param_name, help_text);
    return def;
}

#define GBL_PARAM_TYPE(type, var, param_name, def, help_text) type  var = gbl_param_##type(param_name, def, help_text, &params, &var)
#define GBL_STRUCT_PARAM_TYPE(type, strct, member, param_name, def, help_text) strct.member = gbl_param_##type(param_name, def, help_text, &params, &strct.member)

// use PARAM_IF for parameters whose existence depends on condition 
#define PARAM_IF(cond,param) ((cond) ? (param) : NULL)

#define GBL_PARAM_INT(var,    param_name, def, help_text) GBL_PARAM_TYPE(int,    var, param_name, def, help_text)
#define GBL_PARAM_CHAR(var,   param_name, def, help_text) GBL_PARAM_TYPE(char,   var, param_name, def, help_text)
#define GBL_PARAM_UINT(var,   param_name, def, help_text) GBL_PARAM_TYPE(uint,   var, param_name, def, help_text)
#define GBL_PARAM_STRING(var, param_name, def, help_text) GBL_PARAM_TYPE(String, var, param_name, def, help_text)
#define GBL_PARAM_BIT(var,    param_name, def, help_text) GBL_PARAM_TYPE(bit,    var, param_name, def, help_text)

#define GBL_STRUCT_PARAM_INT(strct,    member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(int,    strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_CHAR(strct,   member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(char,   strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_UINT(strct,   member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(uint,   strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_STRING(strct, member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(String, strct, member, param_name, def, help_text)
#define GBL_STRUCT_PARAM_BIT(strct,    member, param_name, def, help_text) GBL_STRUCT_PARAM_TYPE(bit,    strct, member, param_name, def, help_text)

#define GBL_TRACE_PARAMS(args)                                          \
    do {                                                                \
        GB_ERROR def_error =                                            \
            trace_params((args)->param, params, (args)->command);       \
        if (def_error) { GBL_END_PARAMS; return def_error; }            \
    } while(0)

#define GBL_END_PARAMS                                                  \
    do {                                                                \
        gbl_param *_gblp;                                               \
        while (params) {                                                \
            _gblp = params;                                             \
            params = params->next;                                      \
            free(_gblp);                                                \
        }                                                               \
    } while (0)

#define GBL_CHECK_FREE_PARAM(nr, cnt)           \
    do {                                        \
        if ((nr)+(cnt) >= GBL_MAX_ARGUMENTS) {  \
            /* gb_assert(0); */                 \
            return "max. parameters exceeded";  \
        }                                       \
    } while (0)

#if defined(WARN_TODO)
#warning remove GBL_MAX_ARGUMENTS - instead allocate dynamic
#endif

static GB_ERROR trace_params(const GBL_streams& param, gbl_param *ppara, const char *com) {
    GB_ERROR error = 0;
    int      i;

    int argc = param.size();
    for (i=0; i<argc; i++) {
        gbl_param  *para;
        const char *argument = param.get(i);

        for (para = ppara; para && !error; para = para->next) {
            if (para->param_name) { // NULL means param is inactive (see PARAM_IF)
                int len = strlen(para->param_name);

                if (strncmp(argument, para->param_name, len) == 0) {
                    const char *value = argument+len; // set to start of value

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
                            STATIC_ASSERT(sizeof(int) == sizeof(uint)); // assumed by GBL_PARAM_UINT
                            *(int *)para->varaddr = atoi(value);
                            break;

                        case GB_BIT:
                            // 'param=' is same as 'param' or 'param=1' (historical reason, don't change)
                            *(int *)para->varaddr = (value[0] ? atoi(value) : 1);
                            break;

                        case GB_BYTE:
                            *(char *)para->varaddr = *value; // this may use the terminal zero-byte (e.g. for p1 in 'p0=0,p1=,p2=2' )
                            if (value[0] && value[1]) { // found at least 2 chars
                                GB_warningf("Only one character expected in value '%s' of param '%s' - rest is ignored", value, para->param_name);
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
        }

        if (!error && !para) { // no parameter found for argument
            int             pcount = 0;
            gbl_param     **params;
            int             k;
            char           *res;
            GBS_strstruct  *str    = GBS_stropen(1000);
            GB_ERROR        err;


            for (para = ppara; para; para = para->next) pcount++;
            params = (gbl_param **)GB_calloc(sizeof(void *), pcount);
            for (k = 0, para = ppara; para; para = para->next) params[k++] = para;


            for (pcount--; pcount>=0; pcount--) {
                para = params[pcount];
                if (para->param_name) {
                    GBS_strcat(str, "  ");
                    GBS_strcat(str, para->param_name);
                    switch (para->type) {
                        case GB_STRING: GBS_strcat(str, "STRING"); break;
                        case GB_INT:    GBS_strcat(str, "INT");    break;
                        case GB_FLOAT:  GBS_strcat(str, "FLOAT");  break;
                        case GB_BYTE:   GBS_strcat(str, "CHAR");   break;
                        case GB_BIT:    GBS_strcat(str, "    ");   break;
                        default:        gb_assert (0); GBS_strcat(str, "????"); break;
                    }
                    GBS_strcat(str, "\t\t;");
                    GBS_strcat(str, para->help_text);
                    GBS_strcat(str, "\n");
                }
            }
            freenull(params);
            res = GBS_strclose(str);
            err = GB_export_errorf("Unknown Parameter '%s' in command '%s'\n  PARAMETERS:\n%s", argument, com, res);
            free(res);
            return err;
        }
    }

    return error;
}

inline char *interpret_subcommand(const GBL_reference& ref, const char *input, const char *command) {
    return GB_command_interpreter(ref.get_main(), input, command, ref.get_ref(), ref.get_tree_name());
}

// --------------------------------
//      parameter/stream checks

inline GB_ERROR check_no_parameter(GBL_command_arguments *args) {
    if (args->param.size() == 0) return NULL;
    return GBS_global_string("syntax: %s (no parameters)", args->command);
}
inline GB_ERROR check_parameters(GBL_command_arguments *args, int expected, const char *parameterList) {
    if (args->param.size() == expected) return NULL;
    return GBS_global_string("syntax: %s(%s)", args->command, parameterList);
}
inline GB_ERROR check_optional_parameters(GBL_command_arguments *args, int fix, const char *fixParam, int opt, const char *optParam, bool opt_trailing = true) {
    int params = args->param.size();
    if (params >= fix && params <= (fix+opt)) return NULL;
    if (!fix) return GBS_global_string("syntax: %s[(%s)]", args->command, optParam);
    if (opt_trailing) return GBS_global_string("syntax: %s(%s[,%s])", args->command, fixParam, optParam);
    return GBS_global_string("syntax: %s([%s,]%s)", args->command, optParam, fixParam);
}

inline GB_ERROR check_valid_index(int number, const char *what, int min, int max) {
    if (number >= min && number <= max) return NULL;
    return GBS_global_string("Illegal %s number '%i' (allowed [%i..%i])", what, number, min, max);
}
inline GB_ERROR check_valid_stream_index(GBL_command_arguments *args, int number) { return check_valid_index(number, "stream", 1, args->input.size()); }
inline GB_ERROR check_valid_param_index (GBL_command_arguments *args, int number) { return check_valid_index(number, "param",  0, args->param.size()-1); }

#define EXPECT_NO_PARAM(args) do {                      \
        GB_ERROR perr = check_no_parameter(args);       \
        if (perr) return perr;                          \
    } while(0)

#define EXPECT_PARAMS(args,count,help) do {                     \
        GB_ERROR perr = check_parameters(args, count, help);    \
        if (perr) return perr;                                  \
    } while(0)

#define EXPECT_OPTIONAL_PARAMS(args,fixcount,fixhelp,optcount,opthelp) do { \
        GB_ERROR perr = check_optional_parameters(args, fixcount, fixhelp, optcount, opthelp); \
        if (perr) return perr;                                          \
    } while(0)

#define EXPECT_LEGAL_STREAM_INDEX(args,number) do {             \
        GB_ERROR serr = check_valid_stream_index(args, number); \
        if (serr) return serr;                                  \
    } while(0)

#define COMMAND_DROPS_INPUT_STREAMS(args) do {                          \
        if (trace && args->input.size()>0) {                            \
            if (args->input.size()>1 || args->input.get(0)[0]) {        \
                printf("Warning: Dropped %i input streams\n",           \
                       args->input.size());                             \
            }                                                           \
        }                                                               \
    } while(0)

// -------------------------
//      String functions

static int gbl_stricmp(const char *s1, const char *s2) {
    // case insensitive strcmp
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
    // case insensitive strcmp
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
    // case insensitive strstr
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

inline int approve_pos(int pos, int len) { return pos<0 ? (-pos<len ? len+pos : 0) : pos; }

static GB_ERROR gbl_mid_streams(const GBL_streams& arg_input, GBL_streams& arg_out, int start, int end) {
    // used as well to copy all streams (e.g. by 'dd')
    for (int i=0; i<arg_input.size(); i++) {
        const char *p   = arg_input.get(i);
        int         len = strlen(p);

        int s = approve_pos(start, len);
        int e = approve_pos(end, len);

        char *res;
        if (s >= len || e<s) {
            res = strdup("");
        }
        else {
            gb_assert(s >= 0);
            res = GB_strpartdup(p+s, p+e);
        }
        arg_out.insert(res);
    }
    return 0;
}

static GB_ERROR gbl_trace(GBL_command_arguments *args) {
    int tmp_trace;

    EXPECT_PARAMS(args, 1, "0|1");

    tmp_trace = atoi(args->param.get(0));
    if (tmp_trace<0 || tmp_trace>1) return GBS_global_string("Illegal value %i to trace", tmp_trace);

    if (tmp_trace != GB_get_ACISRT_trace()) {
        printf("*** %sctivated ACI trace ***\n", tmp_trace ? "A" : "De-a");
        GB_set_ACISRT_trace(tmp_trace);
    }

    return gbl_mid_streams(args->input, args->output, 0, -1); // copy all streams
}

/* ---------------------------------------------------------------------------------------
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
 *    @@@ this decription does not match behavior!
 *    @@@ check description in helpfile as well
 *
 *    Inputstreams will be ignored and the operator is applied
 *    to the arguments
 *
 *    Example : a;b | plus(c,d)
 *    Result  : c+d
 */

typedef char* (*gbl_binary_operator)(const char *arg1, const char *arg2, void *client_data);

static GB_ERROR gbl_apply_binary_operator(GBL_command_arguments *args, gbl_binary_operator op, void *client_data) {
    GB_ERROR error = 0;
    switch (args->param.size()) {
        case 0:
            if (args->input.size() == 0) error = "Expect at least two input streams if called with 0 parameters";
            else if (args->input.size()%2) error = "Expect an even number of input streams if called with 0 parameters";
            else {
                int inputpairs = args->input.size()/2;
                int i;
                for (i = 0; i<inputpairs; ++i) {
                    PASS_2_OUT(args, op(args->input.get(i*2), args->input.get(i*2+1), client_data));
                }
            }
            break;

        case 1:
            if (args->input.size() == 0) error = "Expect at least one input stream if called with 1 parameter";
            else {
                int         i;
                const char *argument = args->param.get(0);
                for (i = 0; i<args->input.size(); ++i) {
                    PASS_2_OUT(args, op(args->input.get(i), argument, client_data));
                }
            }
            break;

        case 2:
            for (int i = 0; i<args->input.size(); ++i) {
                char *result1       = interpret_subcommand(*args, args->input.get(i), args->param.get(0));
                if (!result1) error = GB_await_error();
                else {
                    char *result2       = interpret_subcommand(*args, args->input.get(i), args->param.get(1));
                    if (!result2) error = GB_await_error();
                    else {
                        PASS_2_OUT(args, op(result1, result2, client_data));
                        free(result2);
                    }
                    free(result1);
                }
            }
            break;

        default:
            error = check_optional_parameters(args, 0, NULL, 2, "Expr1[,Expr2]");
            break;
    }

    return error;
}

// --------------------------------
//      escape/unescape strings

static char *unEscapeString(const char *escapedString) {
    // replaces all \x by x
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
static char *escapeString(const char *unescapedString) {
    // replaces all '\' and '"' by '\\' and '\"'
    int         len    = strlen(unescapedString);
    char       *result = (char*)malloc(2*len+1);
    char       *to     = result;
    const char *from   = unescapedString;

    while (1) {
        char c = *from++;
        if (!c) break;

        if (c=='\\' || c == '\"') {
            *to++ = '\\';
            *to++ = c;
        }
        else {
            *to++ = c;
        }
    }
    *to = 0;
    return result;
}

// ---------------------------------
//      the commands themselves:

static GB_ERROR gbl_quote(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); i++) {
        char *quoted  = GBS_global_string_copy("\"%s\"", args->input.get(i));
        PASS_2_OUT(args, quoted);
    }
    return NULL;
}
static GB_ERROR gbl_unquote(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); i++) {
        const char *str = args->input.get(i);
        int         len = strlen(str);

        if (str[0] == '\"' && str[len-1] == '\"') {
            PASS_2_OUT(args, GB_strpartdup(str+1, str+len-2));
        }
        else {
            IN_2_OUT(args, i);
        }
    }
    return NULL;
}

static GB_ERROR gbl_escape(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); i++) {
        char *escaped = escapeString(args->input.get(i));
        PASS_2_OUT(args, escaped);
    }
    return NULL;
}
static GB_ERROR gbl_unescape(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); i++) {
        char *unescaped = unEscapeString(args->input.get(i));
        PASS_2_OUT(args, unescaped);
    }
    return NULL;
}

static GB_ERROR gbl_command(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"ACI command\"");

    GB_ERROR  error   = NULL;
    char     *command = unEscapeString(args->param.get(0));
#if defined(DEBUG)
    printf("executing command '%s'\n", command);
#endif

    for (int i=0; i<args->input.size() && !error; i++) {
        char *result = interpret_subcommand(*args, args->input.get(i), command);
        if (!result) error = GB_await_error();
        else PASS_2_OUT(args, result);
    }
    free(command);
    return error;
}

static GB_ERROR gbl_eval(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"expression evaluating to ACI command\"");

    GB_ERROR  error   = NULL;
    char     *to_eval = unEscapeString(args->param.get(0));
    char     *command = interpret_subcommand(*args, "", to_eval); // evaluate independent

    if (!command) error = GB_await_error();
    else {
        if (GB_get_ACISRT_trace()) {
            printf("evaluating '%s'\n", to_eval);
            printf("executing '%s'\n", command);
        }

        for (int i=0; i<args->input.size() && !error; i++) {
            char *result       = interpret_subcommand(*args, args->input.get(i), command);
            if (!result) error = GB_await_error();
            else  PASS_2_OUT(args, result);
        }
        free(command);
    }
    free(to_eval);
    return error;
}

class DefinedCommands : virtual Noncopyable {
    GB_HASH *cmds;
public:
    DefinedCommands() { cmds = GBS_create_dynaval_hash(100, GB_MIND_CASE, GBS_dynaval_free); }
    ~DefinedCommands() { GBS_free_hash(cmds); }

    void set(const char *name, char* cmd) { GBS_dynaval_free(GBS_write_hash(cmds, name, (long)cmd)); } // takes ownership of 'cmd'!
    const char *get(const char *name) const { return (const char *)GBS_read_hash(cmds, name); }
};

static DefinedCommands defined_commands;

static GB_ERROR gbl_define(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);
    EXPECT_PARAMS(args, 2, "name, \"ACI command\"");

    const char *name = args->param.get(0);
    char       *cmd  = unEscapeString(args->param.get(1));

    defined_commands.set(name, cmd);
    if (GB_get_ACISRT_trace()) printf("defining command '%s'='%s'\n", name, cmd);
    return NULL;
}

static GB_ERROR gbl_do(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "definedCommandName");

    GB_ERROR    error = 0;
    const char *name  = args->param.get(0);
    const char *cmd   = defined_commands.get(name);

    if (!cmd) {
        error = GBS_global_string("Can't do undefined command '%s' - use define(%s, ...) first", name, name);
    }
    else {
        if (GB_get_ACISRT_trace()) {
            printf("executing defined command '%s'='%s' on %i streams\n", name, cmd, args->input.size());
        }

        for (int i=0; i<args->input.size() && !error; i++) {
            char *result       = interpret_subcommand(*args, args->input.get(i), cmd);
            if (!result) error = GB_await_error();
            else  PASS_2_OUT(args, result);
        }
    }
    return error;
}

static GB_ERROR gbl_streams(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);
    PASS_2_OUT(args, GBS_global_string_copy("%i", args->input.size()));
    return 0;
}

static GB_ERROR gbl_origin(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"ACI command\"");

    GB_ERROR error = NULL;
    if (!GEN_is_pseudo_gene_species(args->get_ref())) {
        error = GBS_global_string("'%s' applies to gene-species only", args->command);
    }
    else {
        GBDATA *gb_origin = NULL;
        if (strcmp(args->command, "origin_organism") == 0) {
            gb_origin = GEN_find_origin_organism(args->get_ref(), 0);
        }
        else {
            gb_assert(strcmp(args->command, "origin_gene") == 0);
            gb_origin = GEN_find_origin_gene(args->get_ref(), 0);
        }

        if (!error && !gb_origin) error = GB_await_error();

        if (!error) {
            char          *command = unEscapeString(args->param.get(0));
            GBL_reference  ref(gb_origin, args->get_tree_name()); // refer to gb_origin for subcommands

            for (int i=0; i<args->input.size() && !error; i++) {
                char *result       = interpret_subcommand(ref, args->input.get(i), command);
                if (!result) error = GB_await_error();
                else  PASS_2_OUT(args, result);
            }

            free(command);
        }
    }

    return error;
}

class Tab {
    bool tab[256];
public:
    Tab(bool take, const char *invert) {
        bool init = !take;
        for (int i = 0; i<256; ++i) tab[i] = init;
        for (int i = 0; invert[i]; ++i) tab[safeCharIndex(invert[i])] = take;
    }
    bool operator[](int i) const { return tab[i]; }
};

inline GB_ERROR count_by_tab(GBL_command_arguments *args, const Tab& tab) {
    for (int i=0; i<args->input.size(); ++i) {
        long        sum = 0;            // count frequencies
        const char *p   = args->input.get(i);

        while (*p) sum += tab[safeCharIndex(*(p++))];
        PASS_2_OUT(args, GBS_global_string_copy("%li", sum));
    }
    return NULL;
}
inline GB_ERROR remove_by_tab(GBL_command_arguments *args, const Tab& tab) {
    for (int i=0; i<args->input.size(); ++i) {
        GBS_strstruct *strstruct = GBS_stropen(1000);
        for (const char *p = args->input.get(i); *p; p++) {
            if (!tab[(unsigned int)*p]) {
                GBS_chrcat(strstruct, *p);
            }
        }
        PASS_2_OUT(args, GBS_strclose(strstruct));
    }
    return NULL;
}

static GB_ERROR gbl_count(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"characters to count\"");
    return count_by_tab(args, Tab(true, args->param.get(0)));
}
static GB_ERROR gbl_len(GBL_command_arguments *args) {
    EXPECT_OPTIONAL_PARAMS(args, 0, NULL, 1, "\"characters not to count\"");
    const char *exclude = args->param.size() ? args->param.get(0) : "";
    return count_by_tab(args, Tab(false, exclude));
}
static GB_ERROR gbl_remove(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"characters to remove\"");
    return remove_by_tab(args, Tab(true, args->param.get(0)));
}
static GB_ERROR gbl_keep(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"characters to keep\"");
    return remove_by_tab(args, Tab(false, args->param.get(0)));
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

static GB_ERROR gbl_compare  (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_compare, (void*)1); }
static GB_ERROR gbl_icompare (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_compare, (void*)0); }
static GB_ERROR gbl_equals   (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_equals,  (void*)1); }
static GB_ERROR gbl_iequals  (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_equals,  (void*)0); }
static GB_ERROR gbl_contains (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_contains, (void*)1); }
static GB_ERROR gbl_icontains(GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_contains, (void*)0); }
static GB_ERROR gbl_partof   (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_partof,  (void*)1); }
static GB_ERROR gbl_ipartof  (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, binop_partof,  (void*)0); }

static GB_ERROR gbl_translate(GBL_command_arguments *args) {
    EXPECT_OPTIONAL_PARAMS(args, 2, "old,new", 1, "other");

    char replace_other = 0;
    if (args->param.size() == 3) {
        const char *other = args->param.get(2);
        if (other[0] == 0 || other[1] != 0) {
            return "third parameter of translate has to be one character (i.e. \"-\")";
        }
        replace_other = other[0];
    }

    // build translation table :
    unsigned char tab[256];
    {
        const unsigned char *o = (const unsigned char *)args->param.get(0);
        const unsigned char *n = (const unsigned char *)args->param.get(1);
        char        used[256];

        if (strlen((const char *)o) != strlen((const char *)n)) {
            return "arguments 1 and 2 of translate should be strings with identical length";
        }

        for (int i = 0; i<256; ++i) {
            tab[i]  = replace_other ? replace_other : i; // replace unused or identity translation
            used[i] = 0;
        }

        for (int i = 0; o[i]; ++i) {
            if (used[o[i]]) return GBS_global_string("character '%c' used twice in argument 1 of translate", o[i]);
            used[o[i]] = 1;
            tab[o[i]]  = n[i]; // real translation
        }
    }

    for (int i=0; i<args->input.size(); i++) {
        GBS_strstruct *strstruct = GBS_stropen(1000);
        for (const char *p = args->input.get(i); *p; p++) {
            GBS_chrcat(strstruct, tab[(unsigned char)*p]);
        }
        PASS_2_OUT(args, GBS_strclose(strstruct));
    }
    return 0;
}


static GB_ERROR gbl_echo(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);
    for (int i=0; i<args->param.size(); i++) PARAM_2_OUT(args, i);
    return 0;
}

static GB_ERROR gbl_dd(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);
    return gbl_mid_streams(args->input, args->output, 0, -1); // copy all streams
}

static GB_ERROR gbl_string_convert(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    int mode = -1;
    if (strcmp(args->command, "lower")      == 0) mode = 0;
    else if (strcmp(args->command, "upper") == 0) mode = 1;
    else if (strcmp(args->command, "caps")  == 0) mode = 2;
    else return GB_export_errorf("Unknown command '%s'", args->command);

    for (int i=0; i<args->input.size(); i++) {
        char *p              = strdup(args->input.get(i));
        bool  last_was_alnum = false;

        for (char *pp = p; pp[0]; ++pp) {
            switch (mode) {
                case 0:  pp[0] = tolower(pp[0]); break;
                case 1:  pp[0] = toupper(pp[0]); break;
                case 2: { // caps
                    bool alnum = isalnum(pp[0]);
                    if (alnum) pp[0] = (last_was_alnum ? tolower : toupper)(pp[0]);
                    last_was_alnum = alnum;
                    break;
                }
                default: gb_assert(0); break;
            }
        }

        PASS_2_OUT(args, p);
    }

    return 0;
}

static GB_ERROR gbl_head(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "length_of_head");
    int start = atoi(args->param.get(0));
    if (start <= 0) return gbl_mid_streams(args->input, args->output, 1, 0); // empty all streams
    return gbl_mid_streams(args->input, args->output, 0, start-1);
}
static GB_ERROR gbl_tail(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "length_of_tail");
    int end = atoi(args->param.get(0));
    if (end <= 0) return gbl_mid_streams(args->input, args->output, 1, 0); // empty all streams
    return gbl_mid_streams(args->input, args->output, -end, -1);
}

inline GB_ERROR mid(GBL_command_arguments *args, int start_index) {
    EXPECT_PARAMS(args, 2, "start,end");
    return gbl_mid_streams(args->input, args->output, atoi(args->param.get(0))-start_index, atoi(args->param.get(1))-start_index);
}
static GB_ERROR gbl_mid0(GBL_command_arguments *args) { return mid(args, 0); }
static GB_ERROR gbl_mid (GBL_command_arguments *args) { return mid(args, 1); }

static GB_ERROR tab(GBL_command_arguments *args, bool pretab) {
    EXPECT_PARAMS(args, 1, "tabstop");

    int tab = atoi(args->param.get(0));
    for (int i=0; i<args->input.size(); i++) {
        int len = strlen(args->input.get(i));
        if (len >= tab) IN_2_OUT(args, i);
        else {
            char *p = (char *)malloc(tab+1);
            if (pretab) {
                int spaces = tab-len;
                for (int j = 0; j<spaces; ++j) p[j] = ' ';
                strcpy(p+spaces, args->input.get(i));
            }
            else {
                strcpy(p, args->input.get(i));
                for (int j=len; j<tab; j++) p[j] = ' ';
                p[tab] = 0;
            }
            PASS_2_OUT(args, p);
        }
    }
    return 0;
}
static GB_ERROR gbl_tab   (GBL_command_arguments *args) { return tab(args, false); }
static GB_ERROR gbl_pretab(GBL_command_arguments *args) { return tab(args, true); }

static GB_ERROR gbl_crop(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 1, "\"chars_to_crop\"");

    const char *chars_to_crop = args->param.get(0);
    for (int i=0; i<args->input.size(); i++) {
        const char *s = args->input.get(i);
        while (s[0] && strchr(chars_to_crop, s[0]) != 0) s++; // crop at beg of line

        int   len = strlen(s);
        char *p   = (char*)malloc(len+1);
        strcpy(p, s);

        {
            char *pe = p+len-1;

            while (pe >= p && strchr(chars_to_crop, pe[0]) != 0) { // crop at end of line
                --pe;
            }
            gb_assert(pe >= (p-1));
            pe[1] = 0;
        }
        PASS_2_OUT(args, p);
    }
    return 0;
}



static GB_ERROR gbl_cut(GBL_command_arguments *args) {
    for (int i=0; i<args->param.size(); i++) {
        int stream = atoi(args->param.get(i));
        EXPECT_LEGAL_STREAM_INDEX(args, stream);
        IN_2_OUT(args, bio2info(stream));
    }
    return 0;
}
static GB_ERROR gbl_drop(GBL_command_arguments *args) {
    GB_ERROR  error   = 0;
    bool     *dropped = (bool*)malloc(args->input.size()*sizeof(*dropped));

    for (int i=0; i<args->input.size(); ++i) dropped[i] = false;

    for (int i=0; i<args->param.size() && !error; ++i) {
        int stream = atoi(args->param.get(i));
        error = check_valid_stream_index(args, stream);
        if (!error) dropped[bio2info(stream)] = true;
    }

    if (!error) {
        for (int i=0; i<args->input.size(); ++i) {
            if (!dropped[i]) IN_2_OUT(args, i);
        }
    }
    free(dropped);

    return error;
}

static GB_ERROR gbl_dropempty(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); ++i) {
        if (args->input.get(i)[0]) { // if non-empty
            IN_2_OUT(args, i);
        }
    }
    return 0;
}

static GB_ERROR gbl_dropzero(GBL_command_arguments *args) {
    EXPECT_NO_PARAM(args);

    for (int i=0; i<args->input.size(); ++i) {
        if (atoi(args->input.get(i))) { // if non-zero
            IN_2_OUT(args, i);
        }
    }
    return 0;
}

static GB_ERROR gbl_swap(GBL_command_arguments *args) {
    if (args->input.size()<2) return "need at least two input streams";

    int swap1;
    int swap2;
    if (args->param.size() == 0) {
        swap1 = args->input.size()-1;
        swap2 = args->input.size()-2;
    }
    else if (args->param.size() == 2) {
        swap1 = atoi(args->param.get(0));
        swap2 = atoi(args->param.get(1));

        EXPECT_LEGAL_STREAM_INDEX(args, swap1);
        EXPECT_LEGAL_STREAM_INDEX(args, swap2);

        swap1 = bio2info(swap1);
        swap2 = bio2info(swap2);
    }
    else {
        return "expected 0 or 2 parameters";
    }

    for (int i = 0; i<args->input.size(); ++i) {
        int j = i == swap1 ? swap2 : (i == swap2 ? swap1 : i);
        IN_2_OUT(args, j);
    }

    return 0;
}

static GB_ERROR backfront_stream(GBL_command_arguments *args, int toback) {
    if (args->input.size()<1) return "need at least one input stream";
    if (args->param.size() != 1) return "expecting one parameter";

    int stream_to_move = atoi(args->param.get(0));
    EXPECT_LEGAL_STREAM_INDEX(args, stream_to_move);
    stream_to_move = bio2info(stream_to_move);

    if (!toback) IN_2_OUT(args, stream_to_move);
    for (int i = 0; i<args->input.size(); ++i) {
        if (i != stream_to_move) IN_2_OUT(args, i);
    }
    if (toback) IN_2_OUT(args, stream_to_move);

    return 0;
}
static GB_ERROR gbl_toback (GBL_command_arguments *args) { return backfront_stream(args, 1); }
static GB_ERROR gbl_tofront(GBL_command_arguments *args) { return backfront_stream(args, 0); }

static GB_ERROR gbl_merge(GBL_command_arguments *args) {
    const char *separator;
    switch (args->param.size()) {
        case 0: separator = 0; break;
        case 1: separator = args->param.get(0); break;
        default: return check_optional_parameters(args, 0, NULL, 1, "\"separator\"");
    }

    if (args->input.size()) {
        GBS_strstruct *str = GBS_stropen(1000);
        GBS_strcat(str, args->input.get(0));

        for (int i = 1; i<args->input.size(); ++i) {
            if (separator) GBS_strcat(str, separator);
            GBS_strcat(str, args->input.get(i));
        }

        PASS_2_OUT(args, GBS_strclose(str));
    }
    return 0;
}

static GB_ERROR gbl_split(GBL_command_arguments *args) {
    const char *separator;
    int   split_mode = 0;       // 0 = remove separator, 1 = split before separator, 2 = split behind separator

    switch (args->param.size()) {
        case 0:                 // default behavior: split into lines and remove LF
            separator  = "\n";
            break;
        case 2:
            split_mode = atoi(args->param.get(1));
            if (split_mode<0 || split_mode>2) {
                return GBS_global_string("Illegal split mode '%i' (valid: 0..2)", split_mode);
            }
            // fall-through
        case 1:
            separator = args->param.get(0);
            break;
        default:
            return check_optional_parameters(args, 0, NULL, 2, "\"separator\"[,mode]");
    }

    {
        size_t sepLen = strlen(separator);

        for (int i = 0; i<args->input.size(); ++i) {
            const char *in   = args->input.get(i);
            const char *from = in; // search from here

            while (in) {
                const char *splitAt = strstr(from, separator);
                if (splitAt) {
                    size_t  len;
                    char   *copy;

                    if (split_mode == 2) splitAt += sepLen; // split behind separator

                    len  = splitAt-in;
                    copy = (char*)malloc(len+1);

                    memcpy(copy, in, len);
                    copy[len] = 0;

                    PASS_2_OUT(args, copy);

                    in   = splitAt + (split_mode == 0 ? sepLen : 0);
                    from = in+(split_mode == 1 ? sepLen : 0);
                }
                else {
                    COPY_2_OUT(args, in); // last part
                    in = 0;
                }
            }
        }
    }

    return 0;
}

// ----------------------------------
//      Extended string functions

static GB_ERROR gbl_extract_words(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 2, "\"chars\", minchars");

    float len = atof(args->param.get(1));
    for (int i=0; i<args->input.size(); i++) {
        char *res = GBS_extract_words(args->input.get(i), args->param.get(0), len, 1);
        gb_assert(res);
        PASS_2_OUT(args, res);
    }
    return 0;
}

static GB_ERROR gbl_extract_sequence(GBL_command_arguments *args) {
    EXPECT_PARAMS(args, 2, "\"chars\",minFrequency");

    const char *chars   = args->param.get(0);
    float       minFreq = atof(args->param.get(1));

    if (minFreq <0.0 || minFreq > 1.0) return GBS_global_string("Illegal minFrequency=%f", minFreq);

    for (int i=0; i<args->input.size(); i++) {
        char *res = GBS_extract_words(args->input.get(i), chars, minFreq, 0);
        gb_assert(res);
        PASS_2_OUT(args, res);
    }
    return 0;
}

static GB_ERROR gbl_checksum(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(exclude, "exclude=", "", "Remove given characters before calculating");
    GBL_PARAM_BIT   (upper,   "toupper",  0,  "Convert all characters to uppercase before calculating");
    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    for (int i=0; i<args->input.size(); i++) {
        long id = GBS_checksum(args->input.get(i), upper, exclude);
        PASS_2_OUT(args, GBS_global_string_copy("%lX", id));
    }
    return 0;
}

static GB_ERROR gbl_gcgcheck(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    for (int i=0; i<args->input.size(); i++) {
        long id = GBS_gcgchecksum(args->input.get(i));
        PASS_2_OUT(args, GBS_global_string_copy("%li", id));
    }
    return 0;
}

// ------------
//      SRT

static GB_ERROR gbl_srt(GBL_command_arguments *args) {
    GB_ERROR error = NULL;
    for (int i=0; i<args->input.size() && !error; i++) {
        char *modsource = 0;

        for (int j=0; j<args->param.size() && !error; j++) {
            char *hs = GBS_string_eval(modsource ? modsource : args->input.get(i),
                                       args->param.get(j),
                                       args->get_ref());

            if (hs) freeset(modsource, hs);
            else {
                error = GB_await_error();
                free(modsource);
            }
        }

        if (!error) {
            if (modsource) PASS_2_OUT(args, modsource);
            else           IN_2_OUT(args, i);
        }
    }
    return error;
}

// -----------------------------
//      Calculator Functions


// numeric binary operators

typedef int (*numeric_binary_operator)(int i1, int i2);

static char *apply_numeric_binop(const char *arg1, const char *arg2, void *client_data) {
    numeric_binary_operator nbo = (numeric_binary_operator)client_data;

    int i1     = atoi(arg1);
    int i2     = atoi(arg2);
    int result = nbo(i1, i2);

    return GBS_global_string_copy("%i", result);
}

static int binop_plus    (int i1, int i2) { return i1+i2; }
static int binop_minus   (int i1, int i2) { return i1-i2; }
static int binop_mult    (int i1, int i2) { return i1*i2; }
static int binop_div     (int i1, int i2) { return i2 ? i1/i2 : 0; }
static int binop_rest    (int i1, int i2) { return i2 ? i1%i2 : 0; }
static int binop_per_cent(int i1, int i2) { return i2 ? (i1*100)/i2 : 0; }

static GB_ERROR gbl_plus    (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_plus); }
static GB_ERROR gbl_minus   (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_minus); }
static GB_ERROR gbl_mult    (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_mult); }
static GB_ERROR gbl_div     (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_div); }
static GB_ERROR gbl_rest    (GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_rest); }
static GB_ERROR gbl_per_cent(GBL_command_arguments *args) { return gbl_apply_binary_operator(args, apply_numeric_binop, (void*)binop_per_cent); }


static GB_ERROR gbl_select(GBL_command_arguments *args) {
    GB_ERROR error = NULL;
    for (int i=0; i<args->input.size() && !error; i++) {
        int paraidx = atoi(args->input.get(i));
        error       = check_valid_param_index(args, paraidx);
        if (!error) {
            char *result = interpret_subcommand(*args, "", args->param.get(paraidx));
            if (!result) error = GB_await_error();
            else PASS_2_OUT(args, result);
        }
    }
    return error;
}

static GB_ERROR gbl_readdb(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GBS_strstruct *strstr = GBS_stropen(1024);
    for (int i=0; i<args->param.size(); i++) {
        char *val = GBT_read_as_string(args->get_ref(), args->param.get(i));
        if (val) {
            GBS_strcat(strstr, val);
            free(val);
        }
    }
    PASS_2_OUT(args, GBS_strclose(strstr));
    return 0;
}


enum GBT_ITEM_TYPE {
    GBT_ITEM_UNKNOWN,
    GBT_ITEM_SPECIES,
    GBT_ITEM_GENE
};

static GBT_ITEM_TYPE identify_gb_item(GBDATA *gb_item) {
    /* returns: GBT_ITEM_UNKNOWN    -> unknown database_item
     *          GBT_ITEM_SPECIES    -> /species_data/species
     *          GBT_ITEM_GENE       -> /species_data/species/gene_data/gene */

    GBT_ITEM_TYPE res = GBT_ITEM_UNKNOWN;
    if (gb_item) {
        GBDATA *gb_father = GB_get_father(gb_item);
        if (gb_father) {
            const char *key = GB_KEY(gb_item);

            if (strcmp(key, "species")                    == 0 &&
                strcmp(GB_KEY(gb_father), "species_data") == 0) {
                res = GBT_ITEM_SPECIES;
            }
            else if (strcmp(key, "gene")                   == 0 &&
                strcmp(GB_KEY(gb_father), "gene_data")     == 0 &&
                identify_gb_item(GB_get_father(gb_father)) == GBT_ITEM_SPECIES) {
                res = GBT_ITEM_GENE;
            }
        }
    }
    return res;
}

// --------------------------------------------------------------------------------
// taxonomy caching

#if defined(DEBUG)
// #define DUMP_TAXONOMY_CACHING
#endif


#define GROUP_COUNT_CHARS 6                         // characters in taxonomy-key reserved for group-counter (hex number)
#define BITS_PER_HEXCHAR  4
#define MAX_GROUPS        (1 << (GROUP_COUNT_CHARS*BITS_PER_HEXCHAR)) // resulting number of groups

struct cached_taxonomy {
    char    *tree_name;         // tree for which taxonomy is cached here
    int      groups;            // number of named groups in tree (at time of caching)
    GB_HASH *taxonomy; /* keys: "!species", ">XXXXgroup" and "<root>".
                        * Species and groups contain their first parent (i.e. '>XXXXgroup' or '<root>').
                        * Species not in hash are not members of tree.
                        * The 'XXXX' in groupname is simply a counter to avoid multiple groups with same name.
                        * The group-db-entries are stored in hash as pointers ('>>%p') and
                        * point to their own group entry ('>XXXXgroup')
                        *
                        * Note: the number of 'X's in 'XXXX' above is defined by GROUP_COUNT_CHARS! 
                        */
};

static void free_cached_taxonomy(cached_taxonomy *ct) {
    free(ct->tree_name);
    GBS_free_hash(ct->taxonomy);
    free(ct);
}

static void build_taxonomy_rek(TreeNode *node, GB_HASH *tax_hash, const char *parent_group, int *group_counter) {
    if (node->is_leaf) {
        GBDATA *gb_species = node->gb_node;
        if (gb_species) { // not zombie
            GBS_write_hash(tax_hash, GBS_global_string("!%s", GBT_read_name(gb_species)), (long)strdup(parent_group));
        }
    }
    else {
        if (node->name) {       // named group
            char       *hash_entry;
            const char *hash_binary_entry;
            (*group_counter)++;

            gb_assert((*group_counter)<MAX_GROUPS); // overflow - increase GROUP_COUNT_CHARS

            hash_entry = GBS_global_string_copy(">%0*x%s", GROUP_COUNT_CHARS, *group_counter, node->name);
            GBS_write_hash(tax_hash, hash_entry, (long)strdup(parent_group));

            hash_binary_entry = GBS_global_string(">>%p", node->gb_node);
            GBS_write_hash(tax_hash, hash_binary_entry, (long)strdup(hash_entry));

            build_taxonomy_rek(node->get_leftson(), tax_hash, hash_entry, group_counter);
            build_taxonomy_rek(node->get_rightson(), tax_hash, hash_entry, group_counter);
            free(hash_entry);
        }
        else {
            build_taxonomy_rek(node->get_leftson(), tax_hash, parent_group, group_counter);
            build_taxonomy_rek(node->get_rightson(), tax_hash, parent_group, group_counter);
        }
    }
}

static GB_HASH *cached_taxonomies = 0;

static bool is_cached_taxonomy(const char */*key*/, long val, void *cl_ct) {
    cached_taxonomy *ct1 = (cached_taxonomy *)val;
    cached_taxonomy *ct2 = (cached_taxonomy *)cl_ct;

    return ct1 == ct2;
}

static const char *tree_of_cached_taxonomy(cached_taxonomy *ct) {
    /* search the hash to find the correct cached taxonomy.
     * searching for tree name does not work, because the tree possibly already was deleted
     */
    const char *tree = GBS_hash_next_element_that(cached_taxonomies, NULL, is_cached_taxonomy, ct);
#ifdef DUMP_TAXONOMY_CACHING
    if (tree) printf("tree_of_cached_taxonomy: tree='%s' ct->tree_name='%s'\n", tree, ct->tree_name);
#endif // DUMP_TAXONOMY_CACHING
    return tree;
}

static void flush_taxonomy_cb(GBDATA *gbd, cached_taxonomy *ct) {
    /* this cb is bound all tree db members below "/tree_data/tree_xxx" which
     * may have an effect on the displayed taxonomy
     * it invalidates cached taxonomies for that tree (when changed or deleted)
     */

    GB_ERROR    error = 0;
    const char *found = tree_of_cached_taxonomy(ct);

    if (found) {
#ifdef DUMP_TAXONOMY_CACHING
        fprintf(stderr, "Deleting cached taxonomy ct=%p (tree='%s')\n", ct, found);
#endif // DUMP_TAXONOMY_CACHING
        GBS_write_hash(cached_taxonomies, found, 0); // delete cached taxonomy from hash
        free_cached_taxonomy(ct);
    }
#ifdef DUMP_TAXONOMY_CACHING
    else {
        fprintf(stderr, "No tree found for cached_taxonomies ct=%p (already deleted?)\n", ct);
    }
#endif // DUMP_TAXONOMY_CACHING

    if (!GB_inside_callback(gbd, GB_CB_DELETE)) {
        GB_remove_all_callbacks_to(gbd, GB_CB_CHANGED_OR_DELETED, (GB_CB)flush_taxonomy_cb);
    }

    if (found && !error) {
        GBDATA *gb_main = GB_get_gb_main_during_cb();
        if (gb_main) {
            GBDATA *gb_tree_refresh = GB_search(gb_main, AWAR_TREE_REFRESH, GB_INT);
            if (!gb_tree_refresh) {
                error = GBS_global_string("%s (while trying to force refresh)", GB_await_error());
            }
            else {
                GB_touch(gb_tree_refresh); // Note : force tree update
            }
        }
    }

    if (error) {
        fprintf(stderr, "Error in flush_taxonomy_cb: %s\n", error);
    }
}

static void flush_taxonomy_if_new_group_cb(GBDATA *gb_tree, cached_taxonomy *ct) {
    // detects the creation of new groups and call flush_taxonomy_cb() manually
#ifdef DUMP_TAXONOMY_CACHING
    fputs("flush_taxonomy_if_new_group_cb() has been called\n", stderr);
#endif // DUMP_TAXONOMY_CACHING

    const char *tree_name = tree_of_cached_taxonomy(ct);
    if (tree_name) {
        int     groups = 0;
        GBDATA *gb_group_node;

        for (gb_group_node = GB_entry(gb_tree, "node");
             gb_group_node;
             gb_group_node = GB_nextEntry(gb_group_node))
        {
            if (GB_entry(gb_group_node, "group_name")) {
                groups++; // count named groups only
            }
        }

#ifdef DUMP_TAXONOMY_CACHING
        fprintf(stderr, "cached_groups=%i  counted_groups=%i\n", ct->groups, groups);
#endif // DUMP_TAXONOMY_CACHING
        if (groups != ct->groups) {
#ifdef DUMP_TAXONOMY_CACHING
            fprintf(stderr, "Number of groups changed -> invoking flush_taxonomy_cb() manually\n");
#endif // DUMP_TAXONOMY_CACHING
            flush_taxonomy_cb(gb_tree, ct);
        }
    }
#ifdef DUMP_TAXONOMY_CACHING
    else {
        fprintf(stderr, "cached taxonomy no longer valid.\n");
    }
#endif // DUMP_TAXONOMY_CACHING
}

static cached_taxonomy *get_cached_taxonomy(GBDATA *gb_main, const char *tree_name, GB_ERROR *error) {
    long cached;
    *error = 0;
    if (!cached_taxonomies) {
        cached_taxonomies = GBS_create_hash(20, GB_IGNORE_CASE);
    }
    cached = GBS_read_hash(cached_taxonomies, tree_name);
    if (!cached) {
        TreeNode *tree    = GBT_read_tree(gb_main, tree_name, new SimpleRoot);
        if (!tree) *error = GB_await_error();
        else     *error   = GBT_link_tree(tree, gb_main, false, 0, 0);

        if (!*error) {
            GBDATA *gb_tree = GBT_find_tree(gb_main, tree_name);
            if (!gb_tree) {
                *error = GBS_global_string("Can't find tree '%s'", tree_name);
            }
            else {
                cached_taxonomy *ct            = (cached_taxonomy*)malloc(sizeof(*ct));
                long             nodes         = GBT_count_leafs(tree);
                int              group_counter = 0;

                ct->tree_name = strdup(tree_name);
                ct->taxonomy  = GBS_create_dynaval_hash(int(nodes), GB_IGNORE_CASE, GBS_dynaval_free);
                ct->groups    = 0; // counted below

                build_taxonomy_rek(tree, ct->taxonomy, "<root>", &group_counter);
                cached = (long)ct;
                GBS_write_hash(cached_taxonomies, tree_name, (long)ct);

                GB_remove_all_callbacks_to(gb_tree, GB_CB_SON_CREATED, (GB_CB)flush_taxonomy_if_new_group_cb);
                GB_add_callback(gb_tree, GB_CB_SON_CREATED, makeDatabaseCallback(flush_taxonomy_if_new_group_cb, ct));

                {
                    GBDATA *gb_tree_entry = GB_entry(gb_tree, "tree");
                    GBDATA *gb_group_node;

                    if (gb_tree_entry) {
                        GB_remove_all_callbacks_to(gb_tree_entry, GB_CB_CHANGED_OR_DELETED, (GB_CB)flush_taxonomy_cb);
                        GB_add_callback(gb_tree_entry, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(flush_taxonomy_cb, ct));
                    }

                    // add callbacks for all node/group_name subentries
                    for (gb_group_node = GB_entry(gb_tree, "node");
                         gb_group_node;
                         gb_group_node = GB_nextEntry(gb_group_node))
                    {
                        GBDATA *gb_group_name = GB_entry(gb_group_node, "group_name");
                        if (gb_group_name) { // group with id = 0 has no name
                            GB_remove_all_callbacks_to(gb_group_name, GB_CB_CHANGED_OR_DELETED, (GB_CB)flush_taxonomy_cb);
                            GB_add_callback(gb_group_name, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(flush_taxonomy_cb, ct));
                            ct->groups++;
                        }
                    }
                }
#ifdef DUMP_TAXONOMY_CACHING
                fprintf(stderr, "Created taxonomy hash for '%s' (ct=%p)\n", tree_name, ct);
#endif // DUMP_TAXONOMY_CACHING
            }
        }

        destroy(tree);
    }

    if (!*error) {
        cached_taxonomy *ct = (cached_taxonomy*)cached;
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
            result = strdup(group_key+(GROUP_COUNT_CHARS+1)); // return own group name
        }
        else {
            if (depth>1) {
                char *parent_name = get_taxonomy_string(tax_hash, parent_group_key, depth-1, error);
                if (parent_name) {
                    result = GBS_global_string_copy("%s/%s", parent_name, group_key+(GROUP_COUNT_CHARS+1));
                    free(parent_name);
                }
                else {
                    *error = GBS_global_string("In get_taxonomy_string(%s): %s", group_key, *error);
                    result = 0;
                }
            }
            else {
                result = strdup(group_key+(GROUP_COUNT_CHARS+1)); // return own group name
            }
        }
    }
    else {
        *error = GBS_global_string("Not in tax_hash: '%s'", group_key);
    }
    return result;
}

static const char *get_taxonomy(GBDATA *gb_species_or_group, const char *tree_name, bool is_current_tree, int depth, GB_ERROR *error) {
    GBDATA          *gb_main = GB_get_root(gb_species_or_group);
    cached_taxonomy *tax     = get_cached_taxonomy(gb_main, tree_name, error);
    const char      *result  = 0;

    if (tax) {
        GBDATA *gb_name       = GB_entry(gb_species_or_group, "name");
        GBDATA *gb_group_name = GB_entry(gb_species_or_group, "group_name");

        if (gb_name && !gb_group_name) { // it's a species
            char *name = GB_read_string(gb_name);
            if (name) {
                GB_HASH *tax_hash = tax->taxonomy;
                long     found    = GBS_read_hash(tax_hash, GBS_global_string("!%s", name));

                if (found) {
                    const char *parent_group = (const char *)found;

                    if (strcmp(parent_group, "<root>") == 0) {
                        result = ""; // not member of any group
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
        else if (gb_group_name && !gb_name) { // it's a group
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
            *error = "Container has neither 'name' nor 'group_name' entry - can't detect container type";
        }
    }

    return result;
}

static GB_ERROR gbl_taxonomy(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GB_ERROR error = check_optional_parameters(args, 1, "count", 1, "tree_name", false);
    if (!error) {
        char *tree_name       = 0;
        bool  is_current_tree = false;
        int   depth           = -1;
        char *result          = 0;

        if (args->param.size() == 1) {   // only 'depth'
            if (!args->get_tree_name()) {
                result = strdup("No default tree");
            }
            else {
                tree_name = strdup(args->get_tree_name());
                depth = atoi(args->param.get(0));
                is_current_tree = true;
            }
        }
        else { // 'tree_name', 'depth'
            tree_name = strdup(args->param.get(0));
            depth     = atoi(args->param.get(1));
        }

        if (!result) {
            if (depth<1) {
                error = GBS_global_string("Illegal depth '%i' (allowed 1..n)", depth);
            }
            if (!error) {
                const char *taxonomy_string = get_taxonomy(args->get_ref(), tree_name, is_current_tree, depth, &error);
                if (taxonomy_string) result = strdup(taxonomy_string);
            }
        }

        gb_assert(contradicted(result, error));
        if (result) PASS_2_OUT(args, result);
        free(tree_name);
    }
    return error;
}

static GB_ERROR gbl_sequence(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GB_ERROR error = check_no_parameter(args);
    if (!error) {
        switch (identify_gb_item(args->get_ref())) {
            case GBT_ITEM_UNKNOWN: {
                error = "'sequence' used for unknown item";
                break;
            }
            case GBT_ITEM_SPECIES: {
                char *use = GBT_get_default_alignment(args->get_main());

                if (!use) error = GB_await_error();
                else {
                    GBDATA *gb_seq = GBT_find_sequence(args->get_ref(), use);

                    if (gb_seq) PASS_2_OUT(args, GB_read_string(gb_seq));
                    else        COPY_2_OUT(args, ""); // if current alignment does not exist -> return empty string

                    free(use);
                }
                break;
            }
            case GBT_ITEM_GENE: {
                char *seq = GBT_read_gene_sequence(args->get_ref(), true, 0);

                if (!seq) error = GB_await_error();
                else PASS_2_OUT(args, seq);

                break;
            }
        }
    }
    return error;
}

static GB_ERROR gbl_export_sequence(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GB_ERROR error = check_no_parameter(args);
    if (!error) {
        switch (identify_gb_item(args->get_ref())) {
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
                    const char *seq = get_export_sequence(args->get_ref(), &len, &error);

                    gb_assert(error || seq);

                    if (seq) PASS_2_OUT(args, GB_strduplen(seq, len));
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

static GB_ERROR gbl_ali_name(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GB_ERROR error = check_no_parameter(args);
    if (!error) {
        GBDATA *gb_main = args->get_main();
        char   *use     = GBT_get_default_alignment(gb_main);
        PASS_2_OUT(args, use);
    }
    return error;
}

static GB_ERROR gbl_sequence_type(GBL_command_arguments *args) {
    COMMAND_DROPS_INPUT_STREAMS(args);

    GB_ERROR error = check_no_parameter(args);
    if (!error) {
        GBDATA *gb_main = args->get_main();
        char   *use     = GBT_get_default_alignment(gb_main);
        PASS_2_OUT(args, GBT_get_alignment_type_string(gb_main, use));
        free(use);
    }

    return error;
}

static GB_ERROR gbl_format_sequence(GBL_command_arguments *args) {
    GB_ERROR error = 0;
    int      ic;

    int simple_format = (strcmp(args->command, "format") == 0);         // "format_sequence" == !simple_format

    GBL_BEGIN_PARAMS;
    GBL_PARAM_UINT  (firsttab, "firsttab=", 10,   "Indent first line");
    GBL_PARAM_UINT  (tab,      "tab=",      10,   "Indent not first line");
    GBL_PARAM_UINT  (width,    "width=",    50,   "Sequence width (bases only)");

    // "format_sequence"-only
    GBL_PARAM_BIT (numleft,  PARAM_IF(!simple_format, "numleft"),  0,  "Numbers left of sequence");
    GBL_PARAM_INT (numright, PARAM_IF(!simple_format, "numright="), 0, "Numbers right of sequence (specifies width; -1 -> auto-width)");
    GBL_PARAM_UINT(gap,      PARAM_IF(!simple_format, "gap="),     10, "Insert ' ' every n sequence characters");

    // "format"-only
    GBL_PARAM_STRING(nl,      PARAM_IF(simple_format, "nl="),      " ",  "Break line at characters 'str' if wrapping needed");
    GBL_PARAM_STRING(forcenl, PARAM_IF(simple_format, "forcenl="), "\n", "Always break line at characters 'str'");

    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    if (width == 0)               return "Illegal zero width";
    if (numleft && numright != 0) return "You may only specify 'numleft' OR 'numright',  not both.";

    for (ic = 0; ic<args->input.size(); ++ic) {
        {
            const char *src           = args->input.get(ic);
            size_t      data_size     = strlen(src);
            size_t      needed_size;
            size_t      line_size;
            int         numright_used = numright;

            if (numright_used<0) {
                numright_used = calc_digits(data_size);
            }

            {
                size_t lines;

                if (simple_format) {
                    lines     = data_size/2 + 1; // worst case
                    line_size = tab + width + 1;
                }
                else {
                    size_t gapsPerLine = (width-1)/gap;
                    lines              = data_size/width+1;
                    line_size          = tab + width + gapsPerLine + 1;

                    if (numright_used) {
                        // add space for numright
                        line_size += numright_used+1; // plus space
                    }
                }

                needed_size = lines*line_size + firsttab + 1 + 10;
            }

            char *result = (char*)malloc(needed_size);
            if (!result) {
                error = GBS_global_string("Out of memory (tried to alloc %zu bytes)", needed_size);
            }
            else {
                char   *dst       = result;
                size_t  rest_data = data_size;

                if (simple_format) {
                    /* format string w/o gaps or numleft
                     * does word-wrapping at chars in nl
                     */

                    // build wrap table
                    unsigned char isWrapChar[256];
                    memset(isWrapChar, 0, sizeof(isWrapChar));
                    for (int i = 0; nl[i]; ++i) isWrapChar[(unsigned char)nl[i]] = 1;
                    for (int i = 0; forcenl[i]; ++i) isWrapChar[(unsigned char)forcenl[i]] = 2;

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
                        if (take <= 0) { // no wrap character found -> hard wrap at width
                            take  = move = width;
                        }
                        else { // soft wrap at last found wrap character
                            move = take+1;
                        }

                        for (took = 0; took<take; took++) {
                            char c = src[took];
                            if (isWrapChar[(unsigned char)c] == 2) { // forced newline
                                take = took;
                                move = take+1;
                                break;
                            }
                            dst[took] = c;
                        }

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
                    // "format_sequence" with gaps and numleft
                    char       *format        = 0;
                    const char *src_start     = src;
                    const char *dst_linestart = dst;

                    if (numleft) {
                        /* Warning: Be very careful, when you change format strings here!
                         * currently all format strings result in '%u' or '%-##u' (where # are digits)
                         */
                        if (firsttab>0) {
                            char *firstFormat = GBS_global_string_copy("%%-%iu ", firsttab-1);
                            dst += sprintf(dst, firstFormat, (unsigned)1);
                            free(firstFormat);
                        }
                        else {
                            dst += sprintf(dst, "%u ", (unsigned)1);
                        }
                        format = tab>0 ? GBS_global_string_copy("%%-%iu ", tab-1) : strdup("%u ");
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

                        if (numright_used) {
                            if (rest_data) *dst++ = ' ';
                            else {
                                // fill in missing spaces for proper alignment of numright
                                size_t currSize = dst-dst_linestart;
                                size_t wantSize = line_size-numright_used-1;
                                if (currSize<wantSize) {
                                    size_t spaces  = wantSize-currSize;
                                    memset(dst, ' ', spaces);
                                    dst           += spaces;
                                }
                            }
                            unsigned int num  = (src-src_start);
                            dst              += sprintf(dst, "%*u", numright_used, num);
                        }

                        if (rest_data>0) {
                            *dst++ = '\n';
                            dst_linestart = dst;
                            if (numleft) {
                                unsigned int num  = (src-src_start)+1; // this goes to the '%u' (see comment above)
                                dst              += sprintf(dst, format, num);
                            }
                            else if (tab>0) {
                                memset(dst, ' ', tab);
                                dst += tab;
                            }
                        }
                    }

                    free(format);
                }

                *dst++ = 0;         // close str

#if defined(DEBUG)
                { // check for array overflow
                    size_t used_size = dst-result;
                    gb_assert(used_size <= needed_size);

                    char *new_result = (char*)realloc(result, used_size);
                    if (!new_result) {
                        error = "Out of memory";
                    }
                    else {
                        result = new_result;
                    }
                }
#endif // DEBUG
            }

            if (!error) PASS_2_OUT(args, result);
            else free(result);
        }
    }
    return error;
}

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
                            seq     = GB_read_bits(gb_seq, '-', '+');
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

#define GBL_COMMON_FILTER_PARAMS                                        \
    common_filter_params common_param;                                  \
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

static GB_ERROR apply_filters(GBL_command_arguments *args, common_filter_params *common, filter_fun filter_one, void *param) {
    GB_ERROR error = 0;

    if (args->input.size()==0) error = "No input stream";
    else {
        int methodCount = !!common->sai + !!common->species + !!common->pairwise + !!common->first;

        if (methodCount != 1) error = "Need exactly one of the parameters 'SAI', 'species', 'pairwise' or 'first'";
        else {
            if (common->pairwise) {
                if (args->input.size() % 2) error = "Using 'pairwise' requires an even number of input streams";
                else {
                    int i;
                    for (i = 1; i<args->input.size(); i += 2) {
                        PASS_2_OUT(args, filter_one(args->input.get(i), args->input.get(i-1), 0, param));
                    }
                }
            }
            else {
                int     i      = 0;
                char   *filter = 0;
                size_t  flen   = 0;

                if (common->first) {
                    if (args->input.size()<2) error = "Using 'first' needs at least 2 input streams";
                    else {
                        const char *in = args->input.get(i++);
                        gb_assert(in);

                        flen   = strlen(in);
                        filter = GB_strduplen(in, flen);
                    }
                }
                else {
                    filter = gbl_read_seq_sai_or_species(common->species, common->sai, common->align, &flen);
                    if (!filter) error = GB_await_error();
                }

                gb_assert(filter || error);
                if (filter) {
                    for (; i<args->input.size(); ++i) {
                        PASS_2_OUT(args, filter_one(args->input.get(i), filter, flen, param));
                    }
                }
                free(filter);
            }
        }
    }
    return error;
}

// -------------------------
//      calculate diff

struct diff_params {
    char equalC;
    char diffC;
};
static char *calc_diff(const char *seq, const char *filter, size_t /*flen*/, void *paramP) {
    // filters 'seq' through 'filter'
    // - replace all equal     positions by 'equal_char' (if != 0)
    // - replace all differing positions by 'diff_char'  (if != 0)

    diff_params *param      = (diff_params*)paramP;
    char         equal_char = param->equalC;
    char         diff_char  = param->diffC;

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

    diff_params param;
    GBL_STRUCT_PARAM_CHAR(param, equalC,   "equal=",    '.', "symbol for equal characters");
    GBL_STRUCT_PARAM_CHAR(param, diffC,    "differ=",   0,   "symbol for diff characters (default: use char from input stream)");

    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    return apply_filters(args, &common_param, calc_diff, &param);
}

// -------------------------
//      standard filter

enum filter_function { FP_FILTER, FP_MODIFY };

struct filter_params { // used by gbl_filter and gbl_change_gc
    filter_function function;

    const char *include;
    const char *exclude;

    // FP_MODIFY only:
    int         change_pc;
    const char *change_to;
};

static char *filter_seq(const char *seq, const char *filter, size_t flen, void *paramP) {
    filter_params *param = (filter_params*)paramP;

    size_t slen     = strlen(seq);
    if (!flen) flen = strlen(filter);
    size_t mlen     = slen<flen ? slen : flen;

    GBS_strstruct *out = GBS_stropen(mlen+1); // +1 to avoid invalid, zero-length buffer

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

            switch (param->function) {
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

    filter_params param;
    GBL_STRUCT_PARAM_STRING(param, exclude, "exclude=", 0, "Exclude colums");
    GBL_STRUCT_PARAM_STRING(param, include, "include=", 0, "Include colums");
    param.function = FP_FILTER;

    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    GB_ERROR error  = 0;
    int      inOrEx = !!param.include + !!param.exclude;

    if (inOrEx != 1)    error = "Need exactly one parameter of: 'include', 'exclude'";
    else error                = apply_filters(args, &common_param, filter_seq, &param);

    return error;
}

static GB_ERROR gbl_change_gc(GBL_command_arguments *args) {
    GBL_BEGIN_PARAMS;
    GBL_COMMON_FILTER_PARAMS;

    filter_params param;
    GBL_STRUCT_PARAM_STRING(param, exclude,   "exclude=", 0,    "Exclude colums");
    GBL_STRUCT_PARAM_STRING(param, include,   "include=", 0,    "Include colums");
    GBL_STRUCT_PARAM_INT   (param, change_pc, "change=",  0,    "percentage of changed columns (default: silently change nothing)");
    GBL_STRUCT_PARAM_STRING(param, change_to, "to=",      "GC", "change to one of this");
    param.function = FP_MODIFY;

    GBL_TRACE_PARAMS(args);
    GBL_END_PARAMS;

    GB_ERROR error  = 0;
    int      inOrEx = !!param.include + !!param.exclude;

    if (inOrEx != 1) error = "Need exactly one parameter of: 'include', 'exclude'";
    else {
        error = apply_filters(args, &common_param, filter_seq, &param);
    }

    return error;
}

static GB_ERROR gbl_exec(GBL_command_arguments *args) {
    GB_ERROR error = 0;

    if (args->param.size()==0) {
        error = "exec needs parameters:\nexec(command[,arguments])";
    }
    else {
        // write inputstreams to temp file:
        char *inputname;
        int i;
        {
            char *filename = GB_unique_filename("arb_exec_input", "tmp");
            FILE *out      = GB_fopen_tempfile(filename, "wt", &inputname);

            if (!out) error = GB_await_error();
            else {
                for (i=0; i<args->input.size(); i++) {
                    fprintf(out, "%s\n", args->input.get(i));
                }
                fclose(out);
            }
            free(filename);
        }

        if (!error) {
            // build shell command to execute
            char *sys;
            {
                GBS_strstruct *str = GBS_stropen(1000);

                GBS_strcat(str, args->param.get(0));
                for (i=1; i<args->param.size(); i++) {
                    GBS_strcat(str, " \'");
                    GBS_strcat(str, args->param.get(i));
                    GBS_chrcat(str, '\'');
                }
                GBS_strcat(str, " <");
                GBS_strcat(str, inputname);

                sys = GBS_strclose(str);
            }

            char *result = 0;
            {
                FILE *in = popen(sys, "r");
                if (in) {
                    GBS_strstruct *str = GBS_stropen(4096);

                    while ((i=getc(in)) != EOF) { GBS_chrcat(str, i); }
                    result = GBS_strclose(str);
                    pclose(in);
                }
                else {
                    error = GBS_global_string("Cannot execute shell command '%s'", sys);
                }
            }

            if (!error) {
                gb_assert(result);
                PASS_2_OUT(args, result);
            }

            free(sys);
        }

        gb_assert(GB_is_privatefile(inputname, false));
        GB_unlink_or_warn(inputname, &error);
        free(inputname);
    }

    return error;
}


static GBL_command_table gbl_command_table[] = {
    { "ali_name",        gbl_ali_name },
    { "caps",            gbl_string_convert },
    { "change",          gbl_change_gc },
    { "checksum",        gbl_checksum },
    { "command",         gbl_command },
    { "compare",         gbl_compare },
    { "icompare",        gbl_icompare },
    { "contains",        gbl_contains },
    { "icontains",       gbl_icontains },
    { "count",           gbl_count },
    { "crop",            gbl_crop },
    { "cut",             gbl_cut },
    { "dd",              gbl_dd },
    { "define",          gbl_define },
    { "diff",            gbl_diff },
    { "div",             gbl_div },
    { "do",              gbl_do },
    { "drop",            gbl_drop },
    { "dropempty",       gbl_dropempty },
    { "dropzero",        gbl_dropzero },
    { "echo",            gbl_echo },
    { "equals",          gbl_equals },
    { "iequals",         gbl_iequals },
    { "escape",          gbl_escape },
    { "unescape",        gbl_unescape },
    { "eval",            gbl_eval },
    { "exec",            gbl_exec },
    { "export_sequence", gbl_export_sequence },
    { "extract_sequence", gbl_extract_sequence },
    { "extract_words",   gbl_extract_words },
    { "filter",          gbl_filter },
    { "format",          gbl_format_sequence },
    { "format_sequence", gbl_format_sequence },
    { "gcgchecksum",     gbl_gcgcheck },
    { "head",            gbl_head },
    { "keep",            gbl_keep },
    { "left",            gbl_head },
    { "len",             gbl_len },
    { "lower",           gbl_string_convert },
    { "merge",           gbl_merge },
    { "mid",             gbl_mid },
    { "mid0",            gbl_mid0 },
    { "minus",           gbl_minus },
    { "mult",            gbl_mult },
    { "origin_gene",     gbl_origin },
    { "origin_organism", gbl_origin },
    { "partof",          gbl_partof },
    { "ipartof",         gbl_ipartof },
    { "per_cent",        gbl_per_cent },
    { "plus",            gbl_plus },
    { "pretab",          gbl_pretab },
    { "quote",           gbl_quote },
    { "unquote",         gbl_unquote },
    { "readdb",          gbl_readdb },
    { "remove",          gbl_remove },
    { "rest",            gbl_rest },
    { "right",           gbl_tail },
    { "select",          gbl_select },
    { "sequence",        gbl_sequence },
    { "sequence_type",   gbl_sequence_type },
    { "split",           gbl_split },
    { "srt",             gbl_srt },
    { "streams",         gbl_streams },
    { "swap",            gbl_swap },
    { "tab",             gbl_tab },
    { "tail",            gbl_tail },
    { "taxonomy",        gbl_taxonomy },
    { "toback",          gbl_toback },
    { "tofront",         gbl_tofront },
    { "trace",           gbl_trace },
    { "translate",       gbl_translate },
    { "upper",           gbl_string_convert },
    { 0, 0 }

};

void gbl_install_standard_commands(GBDATA *gb_main) {
    gb_install_command_table(gb_main, gbl_command_table, ARRAY_ELEMS(gbl_command_table));
}
