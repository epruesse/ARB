#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>

#if defined(DEBUG)
// #define DUMP
#endif // DEBUG

static char *filename = 0;

static void error(const char *msg) {
    fprintf(stderr, "%s:1: %s\n", filename, msg);
}

int main(int argc, char **argv)
{
    if (argc <= 2) {
        fprintf(stderr,"arb_proto_2_xsub converts GB_prototypes to perl interface\n");
        fprintf(stderr,"Usage: arb_proto_2_xsub <prototypes.h> <xs-header>\n");
        fprintf(stderr,"<xs-header> may contain prototypes. Those will not be overwritten!!!\n");
        return(-1);
    }
    char *data = GB_read_file(filename = argv[1]);
    if (!data){
        GB_print_error();
        exit(EXIT_FAILURE);
    }

    /* read old version (i.e. ARB.xs.default)
       and put all existing functions to exclude hash */

    char *head = GB_read_file(argv[2]);
    printf("%s",head); /* inserting the *.xs.default in the output *.xs file*/

    GB_HASH *exclude_hash = GBS_create_hash(1024,0); /*prepare list for excluded functions from xs header*/
    {
        char *tok;
        /*   initializer            cond  updater                */
        for (tok = strtok(head,"\n");tok;tok = strtok(NULL,"\n")){
            if ( !strncmp(tok,"P2A_",4)
                 ||  !strncmp(tok,"P2AT_",5)){  /* looks like the if-branch is entered for every token*/
                char *fn = GBS_string_eval(tok,"(*=",0);
                GBS_write_hash(exclude_hash,fn,1);
                free(fn);
            }
        }
    }

    /*parsing of proto.h and replacing substrings*/
    data = GBS_string_eval(data,
                           "\nchar=\nschar" // strdupped char
                           ":const ="
                           ":GB_CSTR =char \\*"
                           ":GB_ERROR =char \\*"
                           ":GB_CPNTR =char \\*"
                           ":GB_ULONG =long "
                           ":enum gb_call_back_type =char \\*"
                           ":GB_TYPES =char \\*"
                           ":GB_BOOL =int "
                           ":GB_UNDO_TYPE =char \\*"
                           ,0);


    char *tok;
    void *gb_out    = GBS_stropen(100000);
    void *gbt_out   = GBS_stropen(100000);
    bool  inComment = false;
    char *type      = 0;

    for (tok = strtok(data,";\n");tok;tok = strtok(0,";\n")) {
        if (type) { free(type); type = 0; }

#if defined(DEBUG)
//         fprintf(stderr,"tok='%s'\n",tok);
#endif // DEBUG

        // comment handling :

        if (inComment) {
            char *cmtEnd = strstr(tok, "*/");
            if (!cmtEnd) continue; // continued comment -> search on

            strcpy(tok, cmtEnd+2); // remove comment
            inComment = false;
        }

        arb_assert(!inComment);

        for (char *cmtStart = strstr(tok, "/*"); cmtStart; cmtStart = strstr(tok, "/*")) {
            char *cmtEnd = strstr(cmtStart+2, "*/");
            if (cmtEnd) {
                cmtStart[0] = ' ';
                strcpy(cmtStart+1, cmtEnd+2);
            }
            else {
                inComment = true;
                break;
            }
        }

        if (inComment) continue;

        while (tok[0] == ' ' || tok[0] == '\t') ++tok; // skip whitespace at beginning of line
        if (!tok[0]) continue; // skip empty lines

#if defined(DEBUG)
//         fprintf(stderr,"noc='%s'\n",tok);
#endif // DEBUG

        if (strpbrk(tok,"#{}")) continue;
        // ignore blocks like:
        //
        //#ifdef __cplusplus
        //extern "C" {
        //#endif
        //#ifdef __cplusplus
        //}
        //#endif

        /* exclude some functions  because of type problems */
        if (strstr(tok,"NOT4PERL")) continue;
        if (strstr(tok,"struct")) continue;
        if (strstr(tok,"GBDATA_SET")) continue;
        if (strstr(tok,"UINT4 *")) continue;
        if (strstr(tok,"FLOAT *")) continue;
        if (strstr(tok,"...")) continue;
        if (strstr(tok,"GBQUARK")) continue;
        if (strstr(tok,"GB_HASH")) continue;
        if (strstr(tok,"GBT_TREE")) continue;
        if (strstr(tok,"GB_Link_Follower")) continue;
        if (strstr(tok,"GB_alignment_type")) continue;
        if (strstr(tok,"GB_COMPRESSION_MASK")) continue;
        if (strstr(tok,"float *")) continue;
        if (strstr(tok,"**")) continue;
        if (!GBS_string_cmp(tok,"*(*(*)(*",0)) continue; // no function parameters
        if (strstr(tok,"GB_CB")) continue; // this is a function parameter as well

#if defined(DUMP)
        fprintf(stderr,"Good='%s'\n",tok);
#endif // DUMP

        /* extract function type */
        char *sp = strchr(tok,' ');
        arb_assert(type == 0);
        {
            // is the expected declaration format "type name(..)" or "type name P_((..))" ??
            if (!sp) error(GBS_global_string("Space expected in '%s'",tok));
            while (sp[1] == '*' || sp[1] == ' ') sp++; // function type

            // create a copy of the return type
            int c = sp[1];
            sp[1] = 0;
            type  = strdup(tok);
            sp[1] = c;

            // remove spaces from return type
            char *t = type;
            char *f = type;

            while (*f) {
                if (*f != ' ') *t++ = *f;
                ++f;
            }
            t[0] = 0;
        }

        /* check type */
        GB_BOOL const_char = GB_FALSE;
        GB_BOOL free_flag = GB_FALSE;
        if (!strcmp(type,"char*"))     const_char = GB_TRUE;
        if (!strncmp(type,"schar",5)){
            free_flag     = GB_TRUE;
            char *newtype = strdup(type+1);
            free(type);
            type          = newtype;
        }

        if (!strcmp(type,"float")) {
            free(type);
            type = strdup("double");
        }
        if (!strcmp(type,"GB_alignment_type")) {
            free(type);
            type = strdup("double");
        }

        tok = sp;
        while (tok[0] == ' ' || tok[0] == '*') ++tok;

        char *func_name = 0;
        char *arguments = 0;

        char *P_wrapped = strstr(tok, " P_(");
        if (P_wrapped) {
            sp = strchr(tok, ' ');
            if (!sp) error(GBS_global_string("Space expected in '%s'", tok));

            sp[0]     = 0;      // end function name
            func_name = tok;
            arguments = P_wrapped+5;

            char *last_paren = strrchr(arguments, ')');
            if (!last_paren) error(GBS_global_string("')' expected in '%s'", arguments));

            if (last_paren[-1] == ')') {
                last_paren[-1] = 0; // end arguments
            }
            else {
                error(GBS_global_string("'))' expected in '%s'", P_wrapped));
            }
        }
        else {
            char *open_paren = strchr(tok, '(');
            if (!open_paren) error(GBS_global_string("'(' expected in '%s'", tok));

            open_paren[0] = 0;
            func_name     = tok;
            arguments     = open_paren+1;

            char *last_paren = strrchr(arguments, ')');
            if (!last_paren) error(GBS_global_string("')' expected in '%s'", arguments));

            last_paren[0] = 0;
        }

        arb_assert(func_name);
        arb_assert(arguments);

#if defined(DUMP)
        fprintf(stderr, "type='%s'  func_name='%s'  arguments='%s'\n", type, func_name, arguments);
#endif // DUMP

        /* exclude some funtions */

        if (!strcmp(func_name,"GBT_add_data")) continue;

        // translate prefixes

        void *out = 0;
        if (!strncmp(func_name,"GB_",3)) out = gb_out;
        if (!strncmp(func_name,"GBT_",4)) out = gbt_out;
        if (!strncmp(func_name,"GEN_",4)) out = gbt_out;
        if (!out) continue;

        char *perl_func_name = GBS_string_eval(func_name,"GBT_=P2AT_:GB_=P2A_:GEN_=P2AT_",0);
        if (GBS_read_hash(exclude_hash,perl_func_name)) continue;
        GBS_write_hash(exclude_hash,perl_func_name,1); // don't list functions twice

#if defined(DUMP)
        fprintf(stderr, "-> accepted!\n");
#endif // DUMP

        char *p;
        void *params = GBS_stropen(1000);
        void *args   = GBS_stropen(1000);
        for (p=arguments; arguments; arguments=p ){
            p = strchr(arguments,',');
            if (p) *(p++) = 0;
            if (p && *p == ' ') p++;
            if (strcmp(arguments,"void")){
                GBS_strcat(params," ");
                GBS_strcat(params,arguments);
                GBS_strcat(params,"\n");
                char *arp = strrchr(arguments,' ');
                if (arp) {
                    arp++;
                    if (arp[0] == '*') arp++;
                    GBS_strcat(args,",");
                    GBS_strcat(args,arp);
                }
            }
        }
        char *sargs = GBS_strclose(args);
        if (sargs[0] == ',') sargs++;
        char *sparams = GBS_strclose(params);
        GBS_strnprintf(out,1000,"%s\n",type);
        GBS_strnprintf(out,1000,"%s(%s)\n%s\n",perl_func_name,sargs,sparams);

        if (!strncmp(type,"void",4)){
            GBS_strnprintf(out,100,"    PPCODE:\n");
            GBS_strnprintf(out,1000,"   %s(%s);\n\n",   func_name,sargs);
        }else{
            GBS_strnprintf(out,100,"    CODE:\n");
            if (const_char){
                GBS_strnprintf(out,1000,    "   RETVAL = (char *)%s(%s);\n",        (char *)func_name,sargs);
            }else if(free_flag){
                GBS_strnprintf(out,1000,    "   if (static_pntr) free(static_pntr);\n");
                GBS_strnprintf(out,1000,    "   static_pntr = %s(%s);\n",       func_name,sargs);
                GBS_strnprintf(out,1000,    "   RETVAL = static_pntr;\n");
            }else{
                GBS_strnprintf(out,1000,    "   RETVAL = %s(%s);\n",            func_name,sargs);
            }
            GBS_strnprintf(out,    1000,    "   OUTPUT:\n   RETVAL\n\n");
        }
    }

    if (inComment) error("Comment until end of file");

    printf("%s",GBS_strclose(gb_out));
    printf("MODULE = ARB   PACKAGE = BIO  PREFIX = P2AT_\n\n");
    printf("%s",GBS_strclose(gbt_out));

    return EXIT_SUCCESS;
}
