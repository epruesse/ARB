#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* #include <malloc.h> */
#include <ctype.h>

#include "adlocal.h"
#include "arbdbt.h"
#include "adGene.h"

/********************************************************************************************
					Parameter Functions
********************************************************************************************/

struct gbl_param {
    struct gbl_param *next;
    GB_TYPES type;
    char **addr;
    const char *str;
    const char *help_text;
};


#define GBL_BEGIN_PARAMS struct gbl_param *params = 0

static int gbl_param_int(const char *str,int def, const char *help_text, struct gbl_param **pp, char **vaddr)
{
    struct gbl_param *_gblp = (struct gbl_param *)GB_calloc(1,sizeof(struct gbl_param));
    _gblp->next = *pp; *pp = _gblp; _gblp->help_text = help_text;
    _gblp->addr = vaddr; _gblp->type = GB_INT; _gblp->str = str;
    _gblp->type = GB_INT;
    return def;
}

static const char *gbl_param_string(const char *str,const char *def, const char *help_text, struct gbl_param **pp, char **vaddr) {
    struct gbl_param *_gblp = (struct gbl_param *)GB_calloc(1,sizeof(struct gbl_param));
    _gblp->next = *pp; *pp = _gblp; _gblp->help_text = help_text;
    _gblp->addr = vaddr; _gblp->type = GB_INT; _gblp->str = str;
    _gblp->type = GB_STRING;
    return def;
}

/*
static float gbl_param_float(char *str,float def, char *help_text, struct gbl_param **pp, char **vaddr) {
	struct gbl_param *_gblp = (struct gbl_param *)GB_calloc(1,sizeof(struct gbl_param));
	_gblp->next = *pp; *pp = _gblp; _gblp->help_text = help_text;
	_gblp->addr = vaddr; _gblp->type = GB_INT; _gblp->str = str;
	_gblp->type = GB_FLOAT;
	return def;
	}
	*/

static int gbl_param_bit(const char *str,int def, const char *help_text, struct gbl_param **pp, char **vaddr) {
    struct gbl_param *_gblp = (struct gbl_param *)GB_calloc(1,sizeof(struct gbl_param));
    _gblp->next = *pp; *pp = _gblp; _gblp->help_text = help_text;
    _gblp->addr = vaddr; _gblp->type = GB_INT; _gblp->str = str;
    _gblp->type = GB_BIT;
    return def;
}

#define GBL_PARAM_INT(var,_str,_def,_help_text) int var = \
		gbl_param_int(_str,_def,_help_text, &params, (char **)&var)
#define GBL_PARAM_STRING(var,_str,_def,_help_text) const char *var = \
		gbl_param_string(_str,_def,_help_text, &params, (char **)&var)
#define GBL_PARAM_FLOAT(var,_str,_def,_help_text) float var = \
		gbl_param_float(_str,_def,_help_text, &params, (char **)&var)
#define GBL_PARAM_BIT(var,_str,_def,_help_text) int var = \
		gbl_param_bit(_str,_def,_help_text, &params, (char **)&var)



#define GBL_TRACE_PARAMS(argc,argv) { \
	GB_ERROR error = trace_params(argc,argv,params,com); \
	if (error) { GBL_END_PARAMS; return error; }; };

#define GBL_END_PARAMS { struct gbl_param *_gblp; while (params) { \
		_gblp = params; params = params->next; \
		free((char *)_gblp); } };

#define GBL_CHECK_FREE_PARAM(nr,cnt) if (nr+cnt >= GBL_MAX_ARGUMENTS) {\
	return "Max Parameters exceeded";}

GB_ERROR trace_params(int argc, GBL *argv, struct gbl_param *ppara, char *com) {
    int i;
    int len;
    struct gbl_param *para;
    for (i=0;i<argc;i++) {
        for (para = ppara; para; para = para->next) {
            len = strlen(para->str);
            if (!strncmp(argv[i].str,para->str,len)) {
                switch(para->type) {
                    case GB_STRING:
                        *para->addr = argv[i].str+len;
                        break;
                    case GB_INT:
                        *((int *)para->addr) = atoi(argv[i].str+len);
                        break;
                    case GB_FLOAT:
                        *((float *)para->addr) = atof(argv[i].str+len);
                        break;
                    case GB_BIT:
                        *((float *)para->addr) += 1;
                        break;
                    default:
                        GB_internal_error("ACI: Unknown Parameter Type");
                        break;
                }
                break;
            }
        }
        if (!para) {		/* error */
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
                GBS_strcat(str,"	");
                GBS_strcat(str,para->str);
                switch(para->type) {
                    case GB_STRING:
                        GBS_strcat(str,"STRING");
                        break;
                    case GB_INT:
                        GBS_strcat(str,"INT");
                        break;
                    case GB_FLOAT:
                        GBS_strcat(str,"REAL");
                        break;
                    default:
                        GBS_strcat(str,"   ");
                        break;
                }
                GBS_strcat(str,"\t\t;");
                GBS_strcat(str,para->help_text);
                GBS_strcat(str,"\n");
            }
            GB_DELETE(params);
            res = GBS_strclose(str);
            err = GB_export_error("Unknown Parameter '%s' in command '%s'\n\tPARAMETERS:\n%s",argv[i].str,com,res);
            free(res);
            return err;
        }
    }
    return 0;
}


/********************************************************************************************
					String Functions
********************************************************************************************/

static char *unEscapeString(const char *escapedString) {
    /* replaces all \x by x */
    char *result = GBS_strdup(escapedString);
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


GB_ERROR gbl_command(GBDATA *gb_species, char *com,
                     int     argcinput, GBL *argvinput,
                     int     argcparam,GBL *argvparam,
                     int    *argcout, GBL **argvout)
{
    GBDATA *gb_main = (GBDATA*)GB_MAIN(gb_species)->data;
    int     i;
    char   *command;
    GBUSE(com);

    if (argcparam!=1) return "command syntax: command(\"escaped command\")";

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);

    command = unEscapeString(argvparam[0].str);
#if defined(DEBUG)
    printf("executing command '%s'\n", command);
#endif /* DEBUG */

    for (i=0;i<argcinput;i++) {	/* go through all orig streams	*/
        char *result = GB_command_interpreter(gb_main, argvinput[0].str, command, gb_species);
        if (!result) return GB_get_error();

        (*argvout)[(*argcout)++].str = result;
        /* export result string */
    }
    free(command);
    return 0;
}

GB_ERROR gbl_eval(GBDATA *gb_species, char *com,
                  int     argcinput, GBL *argvinput,
                  int     argcparam,GBL *argvparam,
                  int    *argcout, GBL **argvout)
{
    GBDATA *gb_main = (GBDATA*)GB_MAIN(gb_species)->data;
    int     i;
    char   *command;
    char   *to_eval;
    GBUSE(com);

    if (argcparam!=1) return "eval syntax: eval(\"escaped command evaluating to command\")";

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);

    to_eval = unEscapeString(argvparam[0].str);
    command = GB_command_interpreter(gb_main, "", to_eval, gb_species); // evaluate independent
#if defined(DEBUG)
    printf("evaluating '%s'\n", to_eval);
    printf("executing '%s'\n", command);
#endif /* DEBUG */

    for (i=0;i<argcinput;i++) {	/* go through all orig streams	*/
        char *result = GB_command_interpreter(gb_main, argvinput[0].str, command, gb_species);
        if (!result) return GB_get_error();

        (*argvout)[(*argcout)++].str = result;
        /* export result string */
    }
    free(to_eval);
    free(command);
    return 0;
}

GB_ERROR gbl_origin(GBDATA *gb_species, char *com,
                 int        argcinput, GBL *argvinput,
                 int        argcparam,GBL *argvparam,
                 int       *argcout, GBL **argvout)
{
    int     i;
    GBDATA *gb_origin = 0;
    GBDATA *gb_main   = (GBDATA*)GB_MAIN(gb_species)->data;
    char   *command;

    if (argcparam!=1) return GB_export_error("syntax: %s(\"escaped command\")", com);
    if (!GEN_is_pseudo_gene_species(gb_species)) return GB_export_error("'%s' applies to gene-species only", com);

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);

    if (strcmp(com, "origin_organism") == 0) {
        gb_origin = GEN_find_origin_organism(gb_species);
    }
    else {
        ad_assert(strcmp(com, "origin_gene") == 0);
        gb_origin = GEN_find_origin_gene(gb_species);
    }

    command = unEscapeString(argvparam[0].str);

    for (i=0;i<argcinput;i++) {	/* go through all orig streams	*/
        char *result = GB_command_interpreter(gb_main, argvinput[0].str, command, gb_origin);
        if (!result) return GB_get_error();

        (*argvout)[(*argcout)++].str = result; /* export result string */
    }

    free(command);
    return 0;
}

GB_ERROR gbl_count(GBDATA *gb_species, char *com,
                   int     argcinput, GBL *argvinput,
                   int     argcparam,GBL *argvparam,
                   int    *argcout, GBL **argvout)	{
    int                    i;
    char                   tab[256];				/* if tab[char] count 'char' */
    char                   result[100];
    GBUSE(gb_species);GBUSE(com);

    if (argcparam!=1) return "count syntax: count(\"characters to count\")";

    for (i=0;i<256;i++) {
        if (strchr(argvparam[0].str,i)) tab[i] = 1;
        else tab[i] = 0;
    }
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all orig streams	*/
        char *p;
        long sum = 0;			/* count frequencies	*/
        p = argvinput[i].str;
        while (*p){
            sum += tab[(unsigned int)*(p++)];
        }
        sprintf(result,"%li ",sum);
        (*argvout)[(*argcout)++].str = GB_STRDUP(result);	/* export result string */
    }
    return 0;
}

GB_ERROR gbl_len(GBDATA *gb_species, char *com,
                 int argcinput, GBL *argvinput,
                 int argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)
{
    int i;
    char tab[256];				/* if tab[char] count 'char' */
    char result[100];
    const char *option;

    GBUSE(gb_species);GBUSE(com);

    if (argcparam == 0) option = "";
    else option = argvparam[0].str;
    if (argcparam>=2) return "len syntax: len[(\"characters not to count\")]";

    for (i=0;i<256;i++) {
        if (strchr(option,i)) tab[i] = 0;
        else tab[i] = 1;
    }
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all orig streams	*/
        char *p;
        long sum = 0;			/* count frequencies	*/
        p = argvinput[i].str;
        while (*p){
            sum += tab[(unsigned int)*(p++)];
        }
        sprintf(result,"%li ",sum);
        (*argvout)[(*argcout)++].str = GB_STRDUP(result);	/* export result string */
    }
    return 0;
}

GB_ERROR gbl_remove(GBDATA *gb_species, char *com,
                    int argcinput, GBL *argvinput,
                    int argcparam,GBL *argvparam,
                    int *argcout, GBL **argvout)
{
    int i;
    char tab[256];				/* if tab[char] count 'char' */
    GBUSE(gb_species);GBUSE(com);


    if (argcparam!=1) return "remove syntax: remove(\"characters to remove\")";

    for (i=0;i<256;i++) {
        if (strchr(argvparam[0].str,i)) tab[i] = 1;
        else tab[i] = 0;
    }

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all orig streams	*/
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = argvinput[i].str;*p;p++){
            if (!tab[(unsigned int)*p]) {
                GBS_chrcat(strstruct,*p);
            }
        }
        (*argvout)[(*argcout)++].str = GBS_strclose(strstruct);
        /* export result string */
    }
    return 0;
}

GB_ERROR gbl_keep(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int  i;
    char tab[256];				/* if tab[char] != 0 then keep'char' */
    GBUSE(gb_species);GBUSE(com);


    if (argcparam!=1) return "keep syntax: keep(\"characters not to remove\")";

    memset(tab, 0, 256); // keep none
    {
        unsigned char *keep = (unsigned char*)argvparam[0].str;
        for (i = 0; keep[i]; ++i) {
            tab[keep[i]] = 1; // keep all members of argument
        }
    }

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all orig streams	*/
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = argvinput[i].str;*p;p++){
            if (tab[(unsigned int)*p]) {
                GBS_chrcat(strstruct,*p);
            }
        }
        (*argvout)[(*argcout)++].str = GBS_strclose(strstruct);
        /* export result string */
    }
    return 0;
}

GB_ERROR gbl_translate(GBDATA *gb_species, char *com,
                       int argcinput, GBL *argvinput,
                       int argcparam,GBL *argvparam,
                       int *argcout, GBL **argvout)
{
    unsigned char tab[256];
    int  i;
    char replace_other = 0;
    GBUSE(gb_species);GBUSE(com);

    if (argcparam<2 || argcparam>3) return "translate syntax: translate(#old, #new [,#other])";

    if (argcparam == 3) {
        const char *other = argvparam[2].str;
        if (other[0] == 0 || other[1] != 0) {
            return "third parameter of translate has to be one character (i.e. \"-\")";
        }
        replace_other = other[0];
    }

    /* build translation table : */
    {
        const unsigned char *o = (const unsigned char *)argvparam[0].str;
        const unsigned char *n = (const unsigned char *)argvparam[1].str;
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

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all orig streams	*/
        void *strstruct;
        char *p;
        strstruct = GBS_stropen(1000);
        for  (p = argvinput[i].str;*p;p++){
            GBS_chrcat(strstruct, tab[(unsigned char)*p]);
        }
        (*argvout)[(*argcout)++].str = GBS_strclose(strstruct);
        /* export result string */
    }
    return 0;
}


GB_ERROR gbl_echo(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int i;
    argcinput = argcinput;
    argvinput = argvinput;
    GBUSE(gb_species);GBUSE(com);
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcparam;i++) {		/* go through all in streams	*/
        char *p;
        p = argvparam[i].str;
        (*argvout)[(*argcout)++].str = GB_STRDUP(p);	/* export result string */
    }
    return 0;
}

GB_ERROR _gbl_mid(	int argcinput, GBL *argvinput,
                    int *argcout, GBL **argvout,
                    int start,int mstart,int end,int relend)	{
    int i;
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *p;
        int c;

        int len;
        int nstart = start;
        int nend = end;

        p = argvinput[i].str;
        len = strlen(p);
        if (nstart<0) nstart = len - mstart;
        if (nend<0) nend = len - relend;	/* check rel len */

        if (nstart>len ) nstart = len;	/* check boundaries */
        if (nstart<0) nstart = 0;
        if (nend>len) nend = len;
        if (nend<nstart) nend = nstart;

        c = p[nend];
        p[nend] =0;
        (*argvout)[(*argcout)++].str = GB_STRDUP(p+nstart);	/* export result string */
        p[nend] = c;
    }
    return 0;
}

GB_ERROR gbl_dd(GBDATA *gb_species, char *com,
                int argcinput, GBL *argvinput,
                int argcparam,GBL *argvparam,
                int *argcout, GBL **argvout)	{
    if (argcparam!=0) return "dd syntax: dd (no parameters)";
    argvparam = argvparam;
    GBUSE(gb_species);GBUSE(com);
    return _gbl_mid(argcinput, argvinput,argcout, argvout, 0, 0, -1, 0);
}

GB_ERROR gbl_string_convert(GBDATA *gb_species, char *com, int argcinput, GBL *argvinput, int argcparam,GBL *argvparam, int *argcout, GBL **argvout)	{
    int mode  = -1;
    int i;
    if (argcparam!=0) return "dd syntax: dd (no parameters)";
    argvparam = argvparam;
    GBUSE(gb_species);

    if (strcmp(com, "lower")      == 0) mode = 0;
    else if (strcmp(com, "upper") == 0) mode = 1;
    else if (strcmp(com, "caps")  == 0) mode = 2;
    else return GB_export_error("Unknown command '%s'", com);

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {	/* go through all in streams	*/
        char *p              = GB_STRDUP(argvinput[i].str);
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

        (*argvout)[(*argcout)++].str = p;	/* export result string */
    }

    return 0;
}

GB_ERROR gbl_head(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int start;
    GBUSE(gb_species);GBUSE(com);
    if (argcparam!=1) return "head syntax: head(#start)";
    start = atoi(argvparam[0].str);
    return _gbl_mid(argcinput, argvinput,argcout, argvout, 0,0, start, -start);
}

GB_ERROR gbl_tail(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int end;
    GBUSE(gb_species);GBUSE(com);
    if (argcparam!=1) return "tail syntax: tail(#length_of_tail)";
    end = atoi(argvparam[0].str);
    return _gbl_mid(argcinput, argvinput,argcout, argvout, -1, end, -1, 0);
}

GB_ERROR gbl_mid0(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int start;
    int end;
    if (argcparam!=2) return "mid0 syntax: mid0(#start;#end)";
    GBUSE(gb_species);GBUSE(com);
    start = atoi(argvparam[0].str);
    end = atoi(argvparam[1].str);
    return _gbl_mid(argcinput, argvinput, argcout, argvout, start, -start, end, -end);
}

GB_ERROR gbl_mid(GBDATA *gb_species, char *com,
                 int argcinput, GBL *argvinput,
                 int argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int start;
    int end;
    if (argcparam!=2) return "mid syntax: mid(#start;#end)";
    GBUSE(gb_species);GBUSE(com);
    start = atoi(argvparam[0].str)-1;
    end = atoi(argvparam[1].str)-1;
    return _gbl_mid(argcinput, argvinput, argcout, argvout, start, -start, end, -end);
}

GB_ERROR gbl_tab(GBDATA *gb_species, char *com,
                 int argcinput, GBL *argvinput,
                 int argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int i,j;
    int tab;
    if (argcparam!=1) return "tab syntax: tab(#tab)";
    GBUSE(gb_species);GBUSE(com);
    tab = atoi(argvparam[0].str);
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *p;
        int   len = strlen(argvinput[i].str);

        if (len >= tab) {
            (*argvout)[(*argcout)++].str = GB_STRDUP(argvinput[i].str);
        }else{
            p = (char *)GB_calloc(sizeof(char),tab+1);
            strcpy(p,argvinput[i].str);
            for (j=len; j<tab; j++) p[j] = ' ';
            (*argvout)[(*argcout)++].str = p;
        }
    }
    return 0;
}

GB_ERROR gbl_pretab(GBDATA *gb_species, char *com,
                 int     argcinput, GBL *argvinput,
                 int     argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int i,j;
    int tab;
    if (argcparam!=1) return "pretab syntax: pretab(#tab)";
    GBUSE(gb_species);GBUSE(com);
    tab = atoi(argvparam[0].str);
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *p;
        int   len = strlen(argvinput[i].str);

        if (len >= tab) {
            (*argvout)[(*argcout)++].str = GB_STRDUP(argvinput[i].str);
        }else{
            int spaces = tab-len;

            p = (char *)GB_calloc(sizeof(char),tab+1);
            for (j = 0; j<spaces; ++j) p[j] = ' ';
            strcpy(p+j, argvinput[i].str);
            (*argvout)[(*argcout)++].str = p;
        }
    }
    return 0;
}

GB_ERROR gbl_crop(GBDATA *gb_species, char *com,
                 int     argcinput, GBL *argvinput,
                 int     argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int         i;
    const char *chars_to_crop;

    GBUSE(gb_species);GBUSE(com);
    if (argcparam != 1) return "crop syntax: pretab(chars_to_crop)";

    chars_to_crop = argvparam[0].str;
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        const char *s = argvinput[i].str;
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

        (*argvout)[(*argcout)++].str = p;
    }
    return 0;
}

GB_ERROR gbl_cut(GBDATA *gb_species, char *com,
                 int argcinput, GBL *argvinput,
                 int argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int i;
    GBUSE(gb_species);GBUSE(com);

    GBL_CHECK_FREE_PARAM(*argcout,argcparam);
    for (i=0; i<argcparam;i++) {
        int j = atoi(argvparam[i].str)-1;
        if (j<0) continue;
        if (j>= argcinput) continue;
        (*argvout)[(*argcout)++].str = GB_STRDUP(argvinput[j].str);/* export result string */
    }
    return 0;
}

/********************************************************************************************
					Extended String Functions
********************************************************************************************/

GB_ERROR gbl_extract_words(GBDATA *gb_species, char *com,
                           int argcinput, GBL *argvinput,
                           int argcparam,GBL *argvparam,
                           int *argcout, GBL **argvout)	{
    int   i;
    float len;
    if (argcparam != 2) return "extract_words needs two parameters:\nextract_words(\"Characters\",min_characters)";
    GBUSE(gb_species);GBUSE(com);
    len = atof(argvparam[1].str);

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *res;
        res = GBS_extract_words(argvinput[i].str, argvparam[0].str, len, 1);
        if (!res) return GB_get_error();
        (*argvout)[(*argcout)++].str = res;
    }
    return 0;
}
GB_ERROR gbl_extract_sequence(GBDATA *gb_species, char *com,
                              int argcinput, GBL *argvinput,
                              int argcparam,GBL *argvparam,
                              int *argcout, GBL **argvout)	{
    int      i;
    float    len;
    GB_ERROR syntax_err = "extract_sequence needs two parameters:\nextract_words(\"Characters\",min_rel_characters [0.0-1.0])";

    if (argcparam != 2) return syntax_err;
    GBUSE(gb_species);GBUSE(com);
    len = atof(argvparam[1].str);
    if (len <0.0 || len > 1.0) return syntax_err;

    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *res;
        res = GBS_extract_words(argvinput[i].str, argvparam[0].str, len, 0);
        if (!res) return GB_get_error();
        (*argvout)[(*argcout)++].str = res;
    }
    return 0;
}

GB_ERROR gbl_check(GBDATA *gb_species, char *com,
                   int argcinput, GBL *argvinput,
                   int argcparam,GBL *argvparam,
                   int *argcout, GBL **argvout)	{
    int i;
    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(exclude,"exclude=","","Remove characters 'str' before calculating");
    GBL_PARAM_BIT(upper,"toupper",0,"Convert all characters to uppercase before calculating");
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBUSE(gb_species);GBUSE(com);
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char buf[100];
        long id;
        id = GBS_checksum(argvinput[i].str,upper,exclude);
        sprintf(buf,"%lX",id);
        (*argvout)[(*argcout)++].str = GB_STRDUP(buf);
    }
    return 0;
}

GB_ERROR gbl_gcgcheck(GBDATA *gb_species, char *com,
                      int argcinput, GBL *argvinput,
                      int argcparam,GBL *argvparam,
                      int *argcout, GBL **argvout)	{
    int i;
    GBL_BEGIN_PARAMS;
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBUSE(gb_species);GBUSE(com);
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char buf[100];
        long id;
        id = GBS_gcgchecksum(argvinput[i].str);
        sprintf(buf,"%li",id);
        (*argvout)[(*argcout)++].str = GB_STRDUP(buf);
    }
    return 0;
}

/********************************************************************************************
					SRT
********************************************************************************************/

GB_ERROR gbl_srt(GBDATA *gb_species, char *com,
                 int argcinput, GBL *argvinput,
                 int argcparam,GBL *argvparam,
                 int *argcout, GBL **argvout)	{
    int i;
    int j;
    com = com;
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *source = argvinput[i].str;
        for (j=0;j<argcparam;j++) {
            char *hs = GBS_string_eval(source,argvparam[j].str,gb_species);
            if (!hs) return GB_get_error();
            if (source != argvinput[i].str) free(source);
            source = hs;
        }
        if (source != argvinput[i].str){
            (*argvout)[(*argcout)++].str = source;
        }
        else{
            (*argvout)[(*argcout)++].str = GB_STRDUP(source);
        }
    }
    return 0;
}

/********************************************************************************************
					Calculator Functions
********************************************************************************************/

GB_ERROR gbl_calculator(GBDATA *gb_species, char *com,
                        int argcinput, GBL *argvinput,
                        int argcparam,GBL *argvparam,
                        int *argcout, GBL **argvout)	{
    int val1;
    int val2;
    int val =0;
    char result[100];
    GBUSE(gb_species);GBUSE(com);

    if (argcparam>=2) return "calculator syntax: plus|minus|.. (arg1, arg2)";
    if (!argcinput) return "calculating function: missing input";

    val1 = atoi(argvinput[0].str);

    if (argcparam==1){
        val2 = atoi(argvparam[0].str);
    }else{
        if (argcinput<=1) return "calculating function: missing second input";
        val2 = atoi(argvinput[1].str);
    }
    if (!strcmp(com,"plus")) val = val1+val2;
    else if (!strcmp(com,"minus")) val = val1-val2;
    else if (!strcmp(com,"mult")) val = val1*val2;
    else if (!strcmp(com,"div")) { if (val2) val = val1/val2;else val = 0; }
    else if (!strcmp(com,"per_cent")) { if (val2) val = val1*100/val2; else val =0; }


    sprintf(result,"%i",val);
    GBL_CHECK_FREE_PARAM(*argcout,1);
    (*argvout)[(*argcout)++].str = GB_STRDUP(result);	/* export result string */
    return 0;
}
/********************************************************************************************
					Database Functions
********************************************************************************************/
GB_ERROR gbl_readdb(GBDATA *gb_species, char *com,
                    int argcinput, GBL *argvinput,
                    int argcparam,GBL *argvparam,
                    int *argcout, GBL **argvout)
{
    int i;
    void *strstr = GBS_stropen(1024);

    GBUSE(com);
    GBUSE(argcparam);
    GBUSE(argvparam);
    GBUSE(argcinput);
    GBUSE(argvinput);

    for (i=0;i<argcparam;i++){
        GBDATA *gb = GB_search(gb_species,argvparam[i].str,GB_FIND);
        char *val;
        if(!gb) continue;
        val = GB_read_as_string(gb);
        GBS_strcat(strstr,val);
        free(val);
    }
    (*argvout)[(*argcout)++].str = GBS_strclose(strstr);	/* export result string */
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


GB_ERROR gbl_sequence(GBDATA *gb_item, char *com, /* gb_item may be a species or a gene */
                      int     argcinput, GBL *argvinput,
                      int     argcparam,GBL *argvparam,
                      int    *argcout, GBL **argvout)
{
    char     *use;
    GBDATA   *gb_seq;
    GB_ERROR  error = 0;

    if (argcparam!=0) return "\"sequence\" syntax: \"sequence\" (no parameters)";
    argvparam = argvparam;
    com       = com;
    GBUSE(argvinput);
    if (argcinput==0) return "No input stream";
    GBL_CHECK_FREE_PARAM(*argcout,1);

    switch (identify_gb_item(gb_item)) {
        case GBT_ITEM_UNKNOWN: {
            error = "'sequence' used for unknown item";
            break;
        }
        case GBT_ITEM_SPECIES: {
            use    = GBT_get_default_alignment(GB_get_root(gb_item));
            gb_seq = GBT_read_sequence(gb_item,use);

            if (gb_seq) (*argvout)[(*argcout)++].str = GB_read_string(gb_seq); /* export result string */
            else        (*argvout)[(*argcout)++].str = GB_STRDUP(""); /* if current alignment does not exist -> return empty string */

            free(use);

            break;
        }
        case GBT_ITEM_GENE: {
            char *seq = GBT_read_gene_sequence(gb_item, GB_TRUE);
            error     = GB_get_error();
            gb_assert(!seq || !error);
            if (!error) (*argvout)[(*argcout)++].str = seq;

            break;
        }
    }

    return error;
}

GB_ERROR gbl_sequence_type(GBDATA *gb_species, char *com,
                           int argcinput, GBL *argvinput,
                           int argcparam,GBL *argvparam,
                           int *argcout, GBL **argvout)	{
    char *use;
    if (argcparam!=0) return "\"sequence_type\" syntax: \"sequence\" (no parameters)";
    argvparam = argvparam;
    com = com;GBUSE(argvinput);
    GBL_CHECK_FREE_PARAM(*argcout,1);
    if (argcinput==0) return "No input stream";
    use = GBT_get_default_alignment(GB_get_root(gb_species));
    (*argvout)[(*argcout)++].str = GBT_get_alignment_type_string(GB_get_root(gb_species),use);
    free(use);
    return 0;
}

GB_ERROR gbl_format_sequence(GBDATA *gb_species, char *com,
                             int argcinput, GBL *argvinput,
                             int argcparam,GBL *argvparam,
                             int *argcout, GBL **argvout)	{
    int i;
    void *strstruct;
    char	*p;
    char	buffer[256];
    char	spr[256];

    GBL_BEGIN_PARAMS;
    GBL_PARAM_INT(firsttab,"firsttab=",10,"Indent first line");
    GBL_PARAM_INT(tab,"tab=",10,"Indent not first line");
    GBL_PARAM_BIT(numleft,"numleft",0,"Numbers left of sequence");
    GBL_PARAM_INT(gap,"gap=",10,"Insert ' ' every n sequence characters");
    GBL_PARAM_INT(width,"width=",50,"Sequence width (bases only)");
    GBL_PARAM_STRING(nl,"nl="," ","Break line at characters 'str'");
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBUSE(gb_species);
    if (argcinput==0) return "No input stream";
    GBL_CHECK_FREE_PARAM(*argcout,1);

    strstruct = GBS_stropen(5000);

    if (!strcmp(com,"format")) {
        for (i=0,p= argvinput[0].str; *p; i++) {
            int len = strlen(p);
            if (i==0) {
                int j;
                for (j=0;j<firsttab;j++) buffer[j] = ' ';
                buffer[j] = 0;
                GBS_strcat(strstruct,buffer);
            }
            if (len>=width) {		/* cut too long lines */
                int j;
                for (j=width-1;j>=0;j--){
                    if (strchr(nl,p[j])) 	break;
                }
                if (j<0) {
                    GBS_strncat(strstruct,p,width);
                    p+=width;
                }else{
                    GBS_strncat(strstruct,p,j);
                    p += j+1;
                }
                buffer[0] = '\n';
                for (j=1;j<=tab;j++) buffer[j] = ' ';
                buffer[j] = 0;
                GBS_strcat(strstruct,buffer);
            }else{
                GBS_strcat(strstruct,p);
                p+=len;
            }
        }
    }else{
        for (i=0,p= argvinput[0].str; *p; i++) {
            if (i==0) {
                if (numleft) {
                    sprintf(spr,"%%-%ii ",firsttab-1);
                    sprintf(buffer,spr,p-argvinput[0].str);
                }else{
                    int j;
                    for (j=0;j<firsttab;j++) buffer[j] = ' ';
                    buffer[j] = 0;
                }
                GBS_strcat(strstruct,buffer);
            }else if (i%width ==0) {
                if (numleft) {
                    sprintf(spr,"\n%%-%ii ",tab-1);
                    sprintf(buffer,spr,p-argvinput[0].str);
                }else{
                    int j;
                    buffer[0] = '\n';
                    for (j=1;j<=tab;j++) buffer[j] = ' ';
                    buffer[j] = 0;
                }
                GBS_strcat(strstruct,buffer);
            }else if (gap && (i%gap==0)){
                GBS_strcat(strstruct," ");
            };
            GBS_chrcat(strstruct,*(p++));

        }
    }
    (*argvout)[(*argcout)++].str = GBS_strclose(strstruct);
    return 0;
}

/********************************************************************************************
					Filter Functions
********************************************************************************************/

GBDATA *gbl_search_sai_species(const char *species, const char *sai) {
    GBDATA *gbd;
    if (sai) {
        GBDATA *gb_cont = GB_find(gb_local->gbl.gb_main,"extended_data",0,down_level);
        if (!gb_cont) return 0;
        gbd = GBT_find_SAI_rel_exdata(gb_cont,sai);
    }else{
        GBDATA *gb_cont = GB_find(gb_local->gbl.gb_main,"species_data",0,down_level);
        if (!gb_cont) return 0;
        gbd = GBT_find_species_rel_species_data(gb_cont,species);
    }
    return gbd;
}


GB_ERROR gbl_filter(GBDATA *gb_spec, char *com,
                    int argcinput, GBL *argvinput,
                    int argcparam,GBL *argvparam,
                    int *argcout, GBL **argvout)	{
    int i;
    GBDATA	*gb_species;
    GBDATA *gb_seq;
    GBDATA *gb_ali;
    char *filter = 0;
    long flen = 0;

    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(sai,"SAI=",0,"Use default Sequence of SAI as a filter");
    GBL_PARAM_STRING(species,"species=",0,"Use default Sequence of species as a filter");
    GBL_PARAM_STRING(exclude,"exclude=",0,"Exclude colums");
    GBL_PARAM_STRING(include,"include=",0,"Use only those columns whose filter occur in include");
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    GBUSE(gb_spec);
    if (argcinput==0) return "No input stream";
    if (!exclude && ! include) return "Missing parameter: 'exclude=' or 'include='";
    if (!sai && !species) return "Missing parameter: 'SAI=' or 'species='";
    gb_species = gbl_search_sai_species(species,sai);
    if (!gb_species) return "SAI/Species not found";

    {	char *use = GBT_get_default_alignment(gb_local->gbl.gb_main);
    gb_ali = GB_find(gb_species,use,0,down_level);
    free(use);
    if (!gb_ali) {
        return GB_export_error( "Your filter has no alignment '%s'",use);
    }
    }

    for (	gb_seq = GB_find(gb_ali,0,0,down_level);
            gb_seq;
            gb_seq = GB_find(gb_seq,0,0,search_next|this_level)){
        long type = GB_read_type(gb_seq);
        if (type == GB_BITS) {
            flen = GB_read_bits_count(gb_seq);
            filter = GB_read_bits(gb_seq,'-','+');
            break;
        }
        if (type == GB_STRING) {
            flen = GB_read_string_count(gb_seq);
            filter = GB_read_string(gb_seq);
            break;
        }
    }
    if (!filter) return "I cannot find any string in your filter SAI/species";

    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *dest = GB_STRDUP(argvinput[i].str);
        char *source = dest;
        char *org = source;
        int j;
        int c = 0;
        if (exclude) {
            for (j = 0; j <flen && (c= *(source++));j++){
                if (strchr(exclude, filter[j])) continue;
                *(dest++) = c;
            }
        }else{
            for (j = 0; j <flen && (c= *(source++));j++){
                if (!strchr(include, filter[j])) continue;
                *(dest++) = c;
            }
        }
        if (c) while ( (c= *(source++)) ) { *(dest++) = c; };
        *(dest) = 0;

        (*argvout)[(*argcout)++].str = org;
    }
    free(filter);
    return 0;
}

GB_ERROR gbl_diff(GBDATA *gb_spec, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int i;
    GBDATA	*gb_species;
    GBDATA *gb_seq;
    char *filter = 0;
    long flen;
    char *use;
    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(sai,"SAI=",0,"Use default Sequence of SAI as a filter");
    GBL_PARAM_STRING(species,"species=",0,"Use default Sequence of species as a filter");
    GBL_PARAM_STRING(equal,"equal=",".","symbol for equal characters");
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBL_CHECK_FREE_PARAM(*argcout,argcinput);
    GBUSE(gb_spec);
    if (argcinput==0) return "No input stream";
    if (!sai && !species) return "Missing parameter: 'SAI=' or 'species='";
    gb_species = gbl_search_sai_species(species,sai);
    if (!gb_species) return GB_export_error("SAI/Species '%s/%s' not found",sai,species);

    use = GBT_get_default_alignment(gb_local->gbl.gb_main);
    gb_seq = GBT_read_sequence(gb_species,use);
    if (!gb_seq) {
        GB_ERROR error = GB_export_error("SAI/Species '%s/%s' has no sequence '%s'",sai,species,use);
        free(use);
        return error;
    }
    free(use);

    filter = GB_read_string(gb_seq);
    flen = strlen(filter);

    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *dest = GB_STRDUP(argvinput[i].str);
        char *source = dest;
        char *org = source;
        int j;
        int c =0;
        for (j = 0; j <flen && (c= *(source++));j++){
            if ( c== filter[j] )	*(dest++) = equal[0];
            else			*(dest++) = c;
        }
        if (c) while ( (c= *(source++)) ) { *(dest++) = c; };
        *(dest) = 0;
        (*argvout)[(*argcout)++].str = org;
    }
    free(filter);
    return 0;
}

GB_ERROR gbl_change_gc(GBDATA *gb_species, char *com,
                       int argcinput, GBL *argvinput,
                       int argcparam,GBL *argvparam,
                       int *argcout, GBL **argvout)	{
    int i;
    GBDATA *gb_seq = 0;
    char *filter = 0;
    long flen;
    char *use;
    int ctl;
    GBL_BEGIN_PARAMS;
    GBL_PARAM_STRING(sai,		"SAI=",		0,	"Name of filter SAI");
    GBL_PARAM_STRING(species,	"species=",	0,	"Name of filter species");
    GBL_PARAM_STRING(exclude,	"exclude=",	0,	"Exclude colums");
    GBL_PARAM_STRING(include,	"include=",	0,	"Use only those columns whose filter occur in include");
    GBL_PARAM_INT(change,     	"change=", 	0,	"percent of coloums changed");
    GBL_PARAM_STRING(change_to,	"to=",		"GC",   "to one of this");
    GBL_TRACE_PARAMS(argcparam,argvparam);
    GBL_END_PARAMS;
    GBL_CHECK_FREE_PARAM(*argcout,0);
    if (argcinput==0) return "No input stream";
    if (sai || species){
        if (!exclude && ! include) return "Missing parameter: 'exclude=' or 'include='";
        gb_species = gbl_search_sai_species(species,sai);
        if (!gb_species) return GB_export_error("SAI/Species '%s/%s' not found",sai,species);
        use = GBT_get_default_alignment(gb_local->gbl.gb_main);
        gb_seq = GBT_read_sequence(gb_species,use);
        if (!gb_seq) {
            GB_ERROR error = GB_export_error("SAI/Species '%s/%s' has no sequence '%s'",sai,species,use);
            free(use);
            return error;
        }
        free(use);
    }
    if (gb_seq) {
        filter = GB_read_string(gb_seq);
    }else{
        filter = strdup("");
    }
    flen = strlen(filter);
    ctl = strlen(change_to);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        char *dest = GB_STRDUP(argvinput[i].str);
        char *source = dest;
        char *org = source;
        int j;
        int c = 1;
        if (exclude) {
            for (j = 0; j <flen && (c= *(source++));j++){
                if (!strchr(exclude, filter[j])){
                    if ( isalpha(c) && (rand() * (100.0 / (double)0xffff)) < change){
                        c = change_to[ (rand()>>6)% ctl];
                    }
                }
                *(dest++) = c;
            }
        }else{
            for (j = 0; j <flen && (c= *(source++));j++){
                if (strchr(include, filter[j])){
                    if ( isalpha(c) && (rand() * (100.0 / (double)0xffff)) < change){
                        c = change_to[ (rand()>>6)% ctl];
                    }
                }
                *(dest++) = c;
            }
        }

        if (c) while ( (c= *(source++)) ) {
            if ( isalpha(c) && (rand() * (100.0 / (double)0xffff)) < change){
                c = change_to[ (rand()>>6)% ctl];
            }
            *(dest++) = c;
        };
        *(dest) = 0;
        (*argvout)[(*argcout)++].str = org;
    }
    free(filter);
    return 0;
}
/********************************************************************************************
					Exec Functions
********************************************************************************************/

GB_ERROR gbl_exec(GBDATA *gb_species, char *com,
                  int argcinput, GBL *argvinput,
                  int argcparam,GBL *argvparam,
                  int *argcout, GBL **argvout)	{
    int i;
    char fname[1024];
    void *str;
    FILE *out;
    FILE *in;
    char *sys;

    sprintf(fname,"/tmp/arb_tmp_%s_%i",GB_getenv("USER"),getpid());
    if (argcparam==0) return "exec needs parameters:\nexec(command,...)";
    GBUSE(gb_species);GBUSE(com);
    out = fopen(fname,"w");
    if (!out) return GB_export_error("Cannot open temporary file '%s'",fname);
    for (i=0;i<argcinput;i++) {		/* go through all in streams	*/
        fprintf(out,"%s\n",argvinput[i].str);
    }
    fclose(out);
    str = GBS_stropen(1000);
    for (i=0;i<argcparam;i++) {		/* go through all in params	*/
        if (i) GBS_chrcat(str,'\'');
        GBS_strcat(str, argvparam[i].str);
        if (i) GBS_chrcat(str,'\'');
        GBS_chrcat(str, ' ');
    }
    GBS_strcat(str, "<");
    GBS_strcat(str, fname);
    sys = GBS_strclose(str);
    if (! (in = popen(sys,"r"))) {
        free(com);
        unlink(fname);
        return GB_export_error("Cannot execute '%s'",com);
    }
    free(sys);
    str = GBS_stropen(1000);
    while ( (i=getc(in)) != EOF ) GBS_chrcat(str,i);
    pclose(in);
    (*argvout)[(*argcout)++].str = GBS_strclose(str);
    return 0;
}


struct GBL_command_table gbl_command_table[] = {
    {"caps", gbl_string_convert } ,
    {"change", gbl_change_gc },
    {"checksum", gbl_check } ,
    {"command", gbl_command } ,
    {"eval", gbl_eval } ,
    {"count", gbl_count } ,
    {"cut", gbl_cut } ,
    {"crop", gbl_crop } ,
    {"dd", gbl_dd } ,
    {"diff", gbl_diff },
    {"div", gbl_calculator } ,
    {"echo", gbl_echo } ,
    {"exec", gbl_exec } ,
    {"extract_sequence", gbl_extract_sequence } ,
    {"extract_words", gbl_extract_words } ,
    {"filter", gbl_filter },
    {"format", gbl_format_sequence } ,
    {"format_sequence", gbl_format_sequence } ,
    {"gcgchecksum", gbl_gcgcheck } ,
    {"head", gbl_head } ,
    {"keep", gbl_keep } ,
    {"left", gbl_head } ,
    {"len", gbl_len } ,
    {"lower", gbl_string_convert } ,
    {"mid", gbl_mid } ,
    {"mid0", gbl_mid0 } ,
    {"minus", gbl_calculator } ,
    {"mult", gbl_calculator } ,
    {"per_cent", gbl_calculator } ,
    {"plus", gbl_calculator } ,
    {"pretab", gbl_pretab } ,
    {"readdb", gbl_readdb } ,
    {"remove", gbl_remove } ,
    {"right", gbl_tail } ,
    {"sequence", gbl_sequence } ,
    {"sequence_type", gbl_sequence_type } ,
    {"srt", gbl_srt },
    {"tab", gbl_tab } ,
    {"tail", gbl_tail } ,
    {"translate", gbl_translate } ,
    {"upper", gbl_string_convert } ,
    {"origin_organism", gbl_origin } ,
    {"origin_gene", gbl_origin } ,
    {0,0}

};

void gbl_install_standard_commands(GBDATA *gb_main)
{
    GB_install_command_table(gb_main,gbl_command_table);
}
