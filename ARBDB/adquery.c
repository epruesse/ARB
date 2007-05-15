#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <ctype.h>

#include "adlocal.h"
/*#include "arbdb.h"*/

static void build_GBDATA_path(GBDATA *gbd, char **buffer) {
    GBCONTAINER *gbc = GB_FATHER(gbd);
    const char  *key;

    if (gbc) {
        build_GBDATA_path((GBDATA*)gbc, buffer);
        key = GB_KEY(gbd);
        {
            char *bp = *buffer;
            *bp++ = '/';
            while (*key) *bp++ = *key++;
            *bp      = 0;

            *buffer = bp;
        }
    }
}

#define BUFFERSIZE 1024

const char *GB_get_GBDATA_path(GBDATA *gbd) {
    static char *orgbuffer = 0;
    char        *buffer;

    if (!orgbuffer) orgbuffer = (char*)malloc(BUFFERSIZE);
    buffer                    = orgbuffer;

    build_GBDATA_path(gbd, &buffer);

    if ((buffer-orgbuffer) >= BUFFERSIZE) {
        GB_CORE; // buffer overflow
    }

    return orgbuffer;
}

/********************************************************************************************
                    QUERIES
********************************************************************************************/

static GB_BOOL gb_find_value_equal(GBDATA *gb, GB_TYPES type, const char *val) {
    GB_BOOL equal = GB_FALSE;

#if defined(DEBUG)
    GB_TYPES realtype = GB_TYPE(gb);
    gb_assert(val);
    if (type == GB_STRING) {
        gb_assert(realtype == GB_STRING || realtype == GB_LINK); /* gb_find_internal called with wrong type */
    }
    else {
        gb_assert(realtype == type); /* gb_find_internal called with wrong type */
    }
#endif /* DEBUG */

    switch (type) {
        case GB_STRING:
        case GB_LINK:
            if (GBS_string_cmp(GB_read_char_pntr(gb), val, 1) == 0) equal = GB_TRUE;
            break;
        case GB_INT: {
            int i                      = GB_read_int(gb);
            if (i == *(int*)val) equal = GB_TRUE;
            break;
        }
        case GB_FLOAT: {
            double d                      = GB_read_float(gb);
            if (d == *(double*)val) equal = GB_TRUE;
            break;
        }
        default: {
            const char *err = GBS_global_string("Value search not supported for data type %i (%i)", GB_TYPE(gb), type);
            GB_internal_error(err);
            break;
        }
    }

    return equal;
}

NOT4PERL GBDATA *GB_find_sub_by_quark(GBDATA *father, int key_quark, GB_TYPES type, const char *val, GBDATA *after){
    /* search an entry with a key 'key_quark' below a container 'father'
       after position 'after'

       if (val != 0) search for entry with value 'val':
       
       GB_STRING/GB_LINK: compares string case-insensitive!
       GB_INT: compares values
       GB_FLOAT: dito (val MUST be a 'double*')
       others: not implemented yet

       Note: to search for non-char*-values use GB_find_int()
             for other types write a new similar function

       if key_quark<0 search everything
    */

    register int end, index;
    register GBCONTAINER *gbf = (GBCONTAINER*)father;
    register struct gb_header_list_struct *header;
    GBDATA *gb;

    end  = gbf->d.nheader;
    header = GB_DATA_LIST_HEADER(gbf->d);
    if (after) index = (int)after->index+1; else index = 0;

    if (key_quark<0) { /* unspecific key quark (i.e. search all) */
        gb_assert(!val);        /* search for val not possible if searching all keys! */
        if (!val) {
            for ( ; index < end; index++) {
                if ((int)header[index].flags.key_quark != 0) {
                    if ( (int)header[index].flags.changed >= gb_deleted) continue;
                    if (!(gb=GB_HEADER_LIST_GBD(header[index]))) {
                        gb_unfold( gbf, 0, index);
                        header = GB_DATA_LIST_HEADER(gbf->d);
                        gb     = GB_HEADER_LIST_GBD(header[index]);
                        if (!gb) {
                            const char *err = GBS_global_string("Database entry #%u is missing (in '%s')", index, GB_get_GBDATA_path(father));
                            GB_internal_error(err);
                            continue;
                        }
                    }
                    return gb;
                }
            }
        }
    }
    else { /* specific key quark */
        for ( ; index < end; index++) {
            if ( (key_quark == (int)header[index].flags.key_quark))
                /* if ( (key_quark<0 && ((int)header[index].flags.key_quark != 0)) || ((int)header[index].flags.key_quark  == key_quark) ) */
            {
                if ( (int)header[index].flags.changed >= gb_deleted) continue;
                if (!(gb=GB_HEADER_LIST_GBD(header[index])))
                {
                    gb_unfold( gbf, 0, index);
                    header = GB_DATA_LIST_HEADER(gbf->d);
                    gb = GB_HEADER_LIST_GBD(header[index]);
                    if (!gb){
                        const char *err = GBS_global_string("Database entry #%u is missing (in '%s')", index, GB_get_GBDATA_path(father));
                        GB_internal_error(err);
                        continue;
                    }
                }
                if (val){
                    if (!gb){
                        GB_internal_error("Cannot unfold data");
                        continue;
                    }
                    else {
                        if (!gb_find_value_equal(gb, type, val)) continue;
                    }
                }
                return gb;
            }
        }
    }
    return 0;
}

static GBDATA *GB_find_sub_sub_by_quark(GBDATA *father, const char *key, int sub_key_quark, GB_TYPES type, const char *val, GBDATA *after){
    register int end, index;
    register struct gb_header_list_struct *header;
    register GBCONTAINER *gbf = (GBCONTAINER*)father;
    GBDATA *gb;
    GBDATA *res;
    struct gb_index_files_struct *ifs=0;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbf);

    end  = gbf->d.nheader;
    header = GB_DATA_LIST_HEADER(gbf->d);

    if (after) index = (int)after->index+1; else index = 0;

    /******* look for any hash index tables *********/
    /******* no wildcards allowed       ********/
    if (!Main->local_mode) {
        if (gbf->flags2.folded_container){
            /* do the query in the server */
            if (GB_ARRAY_FLAGS(gbf).changed){
                if (!gbf->flags2.update_in_server){
                    GB_update_server((GBDATA *)gbf);
                }
            }
        }
        if (gbf->d.size > GB_MAX_LOCAL_SEARCH && val) {
            if (after) res = GBCMC_find(after,  key, type, val, (enum gb_search_types)(down_level|search_next));
            else       res = GBCMC_find(father, key, type, val, down_2_level);
            return res;
        }
    }
    if (val &&
        (ifs=GBCONTAINER_IFS(gbf))!=NULL &&
        (!strchr(val, '*')) &&
        (!strchr(val, '?')))
    {
        for (; ifs; ifs = GB_INDEX_FILES_NEXT(ifs)) {
            if (ifs->key != sub_key_quark) continue;
            /****** We found the index table ******/
            res = gb_index_find(gbf, ifs, sub_key_quark, val, index);
            return res;
        }
    }

    if (after)  gb = after;
    else        gb = NULL;

    for ( ; index < end; index++) {
        GBDATA *gbn = GB_HEADER_LIST_GBD(header[index]);

        if ( (int)header[index].flags.changed >= gb_deleted) continue;
        if (!gbn) {
            if (!Main->local_mode) {
                if (gb) res = GBCMC_find(gb,     key, type, val, (enum gb_search_types)(down_level|search_next));
                else    res = GBCMC_find(father, key, type, val, down_2_level);
                return res;
            }
            GB_internal_error("Empty item in server");
            continue;
        }
        gb = gbn;
        if (GB_TYPE(gb) != GB_DB) continue;
        res = GB_find_sub_by_quark(gb, sub_key_quark, type, val, 0);
        if (res) return res;
    }
    return 0;
}


static GBDATA *gb_find_internal(GBDATA *gbd, const char *key, GB_TYPES type, const char *val, long /*enum gb_search_types*/ gbs)
/* searches from 'gdb' for the first entry 'key'
 * if 'val' != 0 then we search for the first entry 'key' that is equal to 'val' (case-insensitive!!)
 * Test depends on 'type' :
 * GB_STRING -> case insensitive compare (as well works for GB_LINK)
 * GB_INT -> compare integers
 */
{
    GBCONTAINER *gbc;
    GBQUARK      key_quark;
    GBDATA      *after = 0;

    if ( gbd == NULL ) return NULL;

    ad_assert(GB_FATHER(gbd));     /* otherwise your GBDATA has been deleted !? */

    if (gbs & this_level) {
        gbs &= ~this_level;
        gbs |= down_level;
        gbc = GB_FATHER(gbd);
        if (gbs & search_next) {
            after = gbd;
            gbs &= ~search_next;
        }
    }else{
        if (gbs & search_next)
        {
            after = gbd;
            gbc = GB_FATHER(gbd);
            gbs = down_2_level;
        }else{
            if (GB_TYPE(gbd) != GB_DB) return 0;
            gbc = (GBCONTAINER *)gbd;
        }
    }

    key_quark = key ? GB_key_2_quark(gbd,key) : -1;

    switch (gbs) {
        case down_level:    return GB_find_sub_by_quark((GBDATA*)gbc, key_quark, type, val, after);
        case down_2_level:  return GB_find_sub_sub_by_quark((GBDATA*)gbc, key, key_quark, type, val, after);
        default:            GB_internal_error("Unknown seach type %li",gbs); return 0;
    }
}

GBDATA *GB_find(GBDATA *gbd, const char *key, const char *str, long /*enum gb_search_types*/ gbs) {
    return gb_find_internal(gbd, key, GB_STRING, str, gbs);
}
NOT4PERL GBDATA *GB_find_int(GBDATA *gbd, const char *key, long val, long gbs) {
    return gb_find_internal(gbd, key, GB_INT, (const char *)&val, gbs);
}

/* iterate over all subentries of a container.
   mostly thought for perl use */

GBDATA *GB_first(GBDATA *father) {
    return GB_find(father, 0, 0, down_level);
}
GBDATA *GB_next(GBDATA *brother) {
    return GB_find(brother, 0, 0, this_level|search_next);
}

/* get a subentry by its internal number:
   Warning: This subentry must exists, otherwise internal error */

GBDATA *gb_find_by_nr(GBDATA *father, int index){
    register GBCONTAINER *gbf = (GBCONTAINER*)father;
    register struct gb_header_list_struct *header;
    GBDATA *gb;
    if (GB_TYPE(father) != GB_DB) {
        GB_internal_error("type is not GB_DB");
        return 0;
    }
    header = GB_DATA_LIST_HEADER(gbf->d);
    if (index >= gbf->d.nheader || index <0){
        GB_internal_error("Index '%i' out of range [%i:%i[",index,0,gbf->d.nheader);
        return 0;
    }
    if ( (int)header[index].flags.changed >= gb_deleted || !header[index].flags.key_quark){
        GB_internal_error("Entry already deleted");
        return 0;
    }
    if (!(gb=GB_HEADER_LIST_GBD(header[index])))
    {
        gb_unfold( gbf, 0, index);
        header = GB_DATA_LIST_HEADER(gbf->d);
        gb = GB_HEADER_LIST_GBD(header[index]);
        if (!gb) {
            GB_internal_error("Could not unfold data");
            return 0;
        }
    }
    return gb;
}

/********************************************************************************************
                    Another Query Procedure
********************************************************************************************/
char  gb_ctype_table[256];
void gb_init_ctype_table(){
    int i;
    for (i=0;i<256;i++){
        if (islower(i) || isupper(i) || isdigit(i) || i=='_' || i=='@' ){
            gb_ctype_table[i] = 1;
        }else{
            gb_ctype_table[i] = 0;
        }
    }
}

static GB_INLINE char *gb_first_non_key_character(const char *str){
    const char *s = str;
    int c;
    while(1){
        c = *s;
        if (!gb_ctype_table[c]){
            if (c ==0) break;
            return (char *)(s);
        }
        s++;
    }
    return 0;
}

char *GB_first_non_key_char(const char *str){
    return gb_first_non_key_character(str);
}

GBDATA *gb_search(GBDATA * gbd, const char *str, GB_TYPES create, int internflag)
{
    /* finds a hierarchical key,
       if create != 0 than create the keys
       force types if ! intern
    */

    char            *s1, *s2;
    GBDATA          *gbp, *gbsp;
    int     len;
    int     seperator = 0;
    char    buffer[GB_PATH_MAX];

    /*fprintf(stderr, "gb_search(%p, %s, %li, %i)\n", gbd, str, create, internflag);*/

    GB_TEST_TRANSACTION(gbd);
    if (!str) {
        return GB_find(gbd,0,0,down_level);
    }
    if (*str == '/') {
        gbd = GB_get_root(gbd);
        str++;
    }

    if (!gb_first_non_key_character(str)) {
        gbsp = GB_find(gbd,str,0,down_level);
        if (gbsp && create && create != GB_TYPE(gbsp)){ /* force type */
            GB_delete(gbsp);
            GB_internal_error("Inconsistent Type1 '%s', repairing database",str);
            gbsp = GB_find(gbd,str,0,down_level);
        }
        if (!gbsp && create) {
            if (internflag){
                if (create == GB_CREATE_CONTAINER) {
                    gbsp = gb_create_container(gbd,str);
                }else{
                    gbsp = gb_create(gbd,str,create);
                }
            }else{
                if (create == GB_CREATE_CONTAINER) {
                    gbsp = GB_create_container(gbd,str);
                }else{
                    gbsp = gb_create(gbd,str,create);
                }
            }
            if (!gbsp) GB_print_error();
        }
        return gbsp;
    }
    {
        len = strlen(str)+1;
        if (len > GB_PATH_MAX) {
            GB_internal_error("Path Length '%i' exceeded by '%s'",GB_PATH_MAX,str);
            return 0;
        }
        GB_MEMCPY(buffer,str,len);
    }

    gbp = gbd;
    for ( s1 = buffer;s1; s1 = s2) {

        s2 = gb_first_non_key_character(s1);
        if (s2) {
            seperator = *s2;
            *(s2++) = 0;
            if (seperator == '-') {
                if ((*s2)  != '>'){
                    GB_export_error("Invalid key for gb_search '%s'",str);
                    GB_print_error();
                    return 0;
                }
                s2++;
            }
        }

        if (strcmp("..", s1) == 0) {
            gbsp = GB_get_father(gbp);
        } else {
            gbsp = GB_find(gbp, s1, 0, down_level);
            if (gbsp && seperator == '-'){ /* follow link !!! */
                if (GB_TYPE(gbsp) != GB_LINK){
                    if (create){
                        GB_export_error("Cannot create links on the fly in GB_search");
                        GB_print_error();
                    }
                    return 0;
                }
                gbsp = GB_follow_link(gbsp);
                seperator = 0;
                if (!gbsp) return 0; /* cannot resolve link  */
            }
            while (gbsp && create) {
                if (s2){ /* non terminal */
                    if (GB_DB == GB_TYPE(gbsp)) break;
                }else{  /* terminal */
                    if (create == GB_TYPE(gbsp)) break;
                }
                GB_internal_error("Inconsistent Type %u:%u '%s':'%s', repairing database", create, GB_TYPE(gbsp), str, s1);
                GB_print_error();
                GB_delete(gbsp);
                gbsp = GB_find(gbd,s1,0,down_level);
            }
        }
        if (!gbsp) {
            if(!create) return 0; /* read only mode */
            if (seperator == '-'){
                GB_export_error("Cannot create linked objects");
                return 0; /* do not create linked objects */
            }
            if(internflag){
                if ( s2 || (create ==GB_CREATE_CONTAINER) ) {   /* not last key */
                    gbsp = gb_create_container(gbp, s1);
                }else{
                    gbsp = gb_create(gbp, s1,(GB_TYPES)create);
                    if (create == GB_STRING) {
                        GB_write_string(gbsp,"");
                    }
                }
            }else{
                if ( s2 || (create == GB_CREATE_CONTAINER) ) {  /* not last key */
                    gbsp = GB_create_container(gbp, s1);
                }else{
                    gbsp = GB_create(gbp, s1,(GB_TYPES)create);
                    if (create == GB_STRING) {   GB_write_string(gbsp,"");}
                }
            }

            if (!gbsp) return 0;
        }
        gbp = gbsp;
    }
    return gbp;
}


GBDATA *GB_search(GBDATA * gbd, const char *str, long /*enum gb_search_enum*/ create){
    return gb_search(gbd,str,create,0);
}


/********************************************************************************************
                                                 Search for select syb entries
********************************************************************************************/

GBDATA *gb_search_marked(GBCONTAINER *gbc, GBQUARK key_quark, int firstindex)
{
    int userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit;
    int index;
    int end = gbc->d.nheader;
    struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbc->d);

    for (index = firstindex; index<end; index++)
    {
        GBDATA *gb;

        if ( ! (userbit & header[index].flags.flags) ) continue;
        if ( (key_quark>=0) && ((int)header[index].flags.key_quark  != key_quark) ) continue;
        if ((int)header[index].flags.changed >= gb_deleted) continue;
        if ((gb=GB_HEADER_LIST_GBD(header[index]))==NULL) {
            gb_unfold( gbc, 0, index);
            header = GB_DATA_LIST_HEADER(gbc->d);
            gb = GB_HEADER_LIST_GBD(header[index]);
        }
        return gb;
    }
    return 0;
}

GBDATA *GB_search_last_son(GBDATA *gbd){
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    int index;
    int end = gbc->d.nheader;
    GBDATA *gb;
    struct gb_header_list_struct *header = GB_DATA_LIST_HEADER(gbc->d);
    for (index = end-1; index>=0; index--){
        if ((int)header[index].flags.changed >= gb_deleted) continue;
        if ((gb=GB_HEADER_LIST_GBD(header[index]))==NULL)
        {
            gb_unfold( gbc, 0, index);
            header = GB_DATA_LIST_HEADER(gbc->d);
            gb = GB_HEADER_LIST_GBD(header[index]);
        }
        return gb;
    }
    return 0;
}

long GB_number_of_marked_subentries(GBDATA *gbd){
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    int userbit = GBCONTAINER_MAIN(gbc)->users[0]->userbit;
    int index;
    int end = gbc->d.nheader;
    struct gb_header_list_struct *header;
    long count = 0;

    header = GB_DATA_LIST_HEADER(gbc->d);
    for (index = 0; index<end; index++) {
        if ( ! (userbit & header[index].flags.flags) ) continue;
        if ((int)header[index].flags.changed >= gb_deleted) continue;
        count++;
    }
    return count;
}

GBDATA *GB_first_marked(GBDATA *gbd, const char *keystring){
    register GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    GBQUARK key_quark;
    if (keystring) {
        key_quark = GB_key_2_quark(gbd,keystring);
    }else{
        key_quark = -1;
    }
    GB_TEST_TRANSACTION(gbd);
    return gb_search_marked(gbc,key_quark, 0);
}


GBDATA *GB_next_marked(GBDATA *gbd, const char *keystring)
{
    register GBCONTAINER *gbc = GB_FATHER(gbd);
    GBQUARK key_quark;

    if (keystring) {
        key_quark = GB_key_2_quark(gbd,keystring);
    }else{
        key_quark = -1;
    }
    GB_TEST_TRANSACTION(gbd);
    return gb_search_marked(gbc,key_quark, (int)gbd->index+1);
}





/********************************************************************************************
                    Command Interpreter
********************************************************************************************/

void gb_install_command_table(GBDATA *gb_main,struct GBL_command_table *table)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (!Main->command_hash) Main->command_hash = GBS_create_hash(1024,1);

    for (; table->command_identifier; table++) {
        GBS_write_hash(Main->command_hash,table->command_identifier,(long)table->function);
    }
}

/*************** Run commands *************************/

char *gbs_search_second_x(const char *str)
{
    register int c;
    for (;(c=*str);str++) {
        if (c=='\\') {      /* escaped characters */
            str++;
            if (!(c=*str)) return 0;
            continue;
        }
        if (c=='"') return (char *)str;
    }
    return 0;
}

char *gbs_search_second_bracket(const char *source)
{
    register int c;
    int deep = 0;
    if (*source != '(') deep --;    /* first bracket */
    for (;(c=*source);source++){
        if (c=='\\') {      /* escaped characters */
            source++;
            if (!*source) break;
            continue;
        }
        if(c=='(') deep--;
        else if (c==')') deep++;
        if (!deep) return (char *)source;
        if (c=='"') {       /* search the second " */
            source = gbs_search_second_x(source);
            if (!source) return 0;
        }
    }
    if (!c) return 0;
    return (char *)source;
}


char *gbs_search_next_seperator(const char *source,const char *seps){
    /* search the next seperator */
    static char tab[256];
    static int flag = 0;
    register int c;
    register const char *p;
    if (!flag) {
        flag = 1;
        memset(tab,0,256);
    }
    for (p = seps; (c=*p);p++) tab[c] = 1; /* tab[seps[x]] = 1 */
    tab['('] = 1;               /* exclude () pairs */
    tab['"'] = 1;               /* exclude " pairs */
    tab['\\'] = 1;              /* exclude \-escaped chars */

    for (;(c=*source);source++){
        if (tab[c]) {
            if (c=='\\') {
                source++;
                continue;
            }
            if (c=='(') {
                source = gbs_search_second_bracket(source);
                if (!source) break;
                continue;
            }
            if (c=='"') {
                source = gbs_search_second_x(source+1);
                if (!source) break;
                continue;
            }
            for (p = seps; (c=*p);p++) tab[c] = 0;
            return (char *)source;
        }
    }
    for (p = seps; (c=*p);p++) tab[c] = 0;  /* clear tab */
    return 0;
}

static void dumpStreams(const char *name, int count, const GBL *args) {
    printf("%s=%i\n", name, count);
    if (count > 0) {
        int c;
        for (c = 0; c<count; c++) {
            printf("  %02i='%s'\n", c, args[c].str);
        }
    }
}

char *GB_command_interpreter(GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name) {
    /* simple command interpreter returns 0 on error (+ GB_export_error) */
    /* if first character is == ':' run string parser
       if first character is == '/' run regexpr
       else command interpreter

    */
    int           strmalloc = 0;
    int           len;
    char         *buffer;
    GB_ERROR      error;
    int           i;
    int           argcinput;
    int           argcparam;
    int           argcout;
    char         *bracket;
    GB_MAIN_TYPE *Main      = GB_MAIN(gb_main);

    GBL morig[GBL_MAX_ARGUMENTS];
    GBL min[GBL_MAX_ARGUMENTS];
    GBL mout[GBL_MAX_ARGUMENTS];

    GBL *orig  = & morig[0];
    GBL *in    = & min[0];
    GBL *out   = & mout[0];
    int  trace = GB_get_ACISRT_trace();

    if (!str) {
        if (!gbd) {
            GB_internal_error("ARB_command_interpreter no input streams found");
            return GB_STRDUP("????");
        }
        str = GB_read_as_string(gbd);
        strmalloc = 1;
    }


    if (trace) {
        printf("GB_command_interpreter: str='%s'\n"
               "                        command='%s'\n", str, commands);
    }
    
    /*********************** empty command -> do not modify string ***************/
    if (!commands || !commands[0]) {    /* empty command -> return GB_STRDUP(str) */
        if (!strmalloc) return GB_STRDUP(str);
        return (char *)str;
    }

    /*********************** ':' -> string parser ********************/
    if (commands[0] == ':') {
        return GBS_string_eval(str,commands+1,gbd);
    }
    if (commands[0] == '/'){
        char *result = GBS_regreplace(str,commands,gbd);
        if (!result) {
            GB_ERROR err = GB_get_error();

            if (strstr(err, "no '/' found in regexpr") != 0) {
                /* if GBS_regreplace didn't find a third '/' we use GBS_regmatch: */
                char *found;
                GB_clear_error();

                found  = GBS_regmatch(str, commands);
                result = found ? found : GB_STRDUP("");
            }
        }
        return result;
    }

    /*********************** init ********************/

    gb_local->gbl.gb_main = gb_main;
    len = strlen(commands)+1;
    buffer = GB_STRDUP(commands);

    /*********************** remove all spaces and tabs ********************/
    {
        const char *s1;
        char *s2;
        s1 = commands;
        s2 = buffer;
        {
            register int c;
            for (; (c= *s1); s1++){
                if (c=='\\') {
                    *(s2++) = c;
                    if (!(c=*s1)) { break; }
                    *(s2++) = c;
                    continue;
                }

                if (c=='"' ) {      /* search the second " */
                    const char *hp = gbs_search_second_x(s1+1);
                    if (!hp){
                        GB_export_error("unbalanced '\"' in '%s'",commands);
                        return 0;
                    }
                    while (s1 <= hp) *(s2++) = *(s1++);
                    s1--;
                    continue;
                }
                if (c!=' ' && c!='\t') *(s2++) = c;
            }
        }
        *s2 = 0;
    }



    memset( (char *)orig,0,sizeof(GBL)*GBL_MAX_ARGUMENTS);
    memset( (char *)in,0,sizeof(GBL)*GBL_MAX_ARGUMENTS);
    memset( (char *)out,0,sizeof(GBL)*GBL_MAX_ARGUMENTS);

    if (strmalloc) {
        orig[0].str = (char *)str;
    }else{
        orig[0].str = GB_STRDUP(str);
    }
    argcinput = 1;
    argcout = 0;
    error = 0;
    {
        char *s1,*s2;
        s1 = buffer;
        if (*s1 == '|') s1++;

        /*** loop over all commands ***/
        for (s1 = s1; s1 ; s1 = s2) {
            int seperator;
            GBL_COMMAND command;
            s2= gbs_search_next_seperator(s1,"|;,");
            if (s2) {
                seperator = *(s2);
                *(s2++) = 0;
            }else{
                seperator = 0;
            }
            /* collect the parameters */
            memset((char*)in,0,sizeof(GBL)*GBL_MAX_ARGUMENTS);
            if (*s1 == '"') {           /* copy "text" to out */
                char *end = gbs_search_second_x(s1+1);
                if (!end) {
                    error = "Missing second '\"'";
                    break;
                }
                *end = 0;
                out[argcout++].str = GB_STRDUP(s1+1);
            }
            else {
                argcparam = 0;
                bracket = strchr(s1,'(');
                if (bracket){       /* I got the parameter list */
                    int slen;
                    *(bracket++) = 0;
                    slen  = strlen(bracket);
                    if (bracket[slen-1] != ')') {
                        error = "Missing ')'";
                    }else{
                        /* go through the parameters */
                        char *p1,*p2;
                        bracket[slen-1] = 0;
                        for (p1 = bracket; p1 ; p1 = p2) {
                            p2 = gbs_search_next_seperator(p1,";,");
                            if (p2) {
                                *(p2++) = 0;
                            }
                            if (p1[0] == '"') { /* remove "" pairs */
                                int len2;
                                p1++;
                                len2 = strlen(p1)-1;

                                if (p1[len2] != '\"')  {
                                    error = "Missing '\"'";
                                }
                                else {
                                    p1[len2] = 0;
                                }
                            }
                            in[argcparam++].str = GB_STRDUP(p1);
                        }
                    }
                    if (error) break;
                }
                if (!error && ( bracket || *s1) ) {
                    char *p = s1;
                    int c;
                    while ( (c = *p) ) {        /* command to lower case */
                        if (c>='A' && c<='Z') {
                            c += 'a'-'A';
                            *p = c;
                        }
                        p++;
                    }

                    command = (GBL_COMMAND)GBS_read_hash(Main->command_hash,s1);
                    if (!command) {
                        error = GBS_global_string("Unknown command '%s'", s1);
                    }
                    else {
                        GBL_command_arguments args;
                        args.gb_ref            = gbd;
                        args.default_tree_name = default_tree_name;
                        args.command           = s1;
                        args.cinput            = argcinput;
                        args.vinput            = orig;
                        args.cparam            = argcparam;
                        args.vparam            = in;
                        args.coutput           = &argcout;
                        args.voutput           = &out;

                        if (trace) {
                            printf("-----------------------\nExecution of command '%s':\n", args.command);
                            dumpStreams("Arguments", args.cparam, args.vparam);
                            dumpStreams("InputStreams", args.cinput, args.vinput);
                        }

                        error = command(&args); /* execute the command */

                        if (!error && trace) dumpStreams("OutputStreams", *args.coutput, *args.voutput);

                        if (error) {
                            char *inputstreams = 0;
                            char *paramlist    = 0;
                            int   i;

                            for (i = 0; i<args.cparam; ++i) {
                                if (!paramlist) paramlist = strdup(args.vparam[i].str);
                                else {
                                    char *conc = GBS_global_string_copy("%s,%s", paramlist, args.vparam[i].str);
                                    free(paramlist);
                                    paramlist  = conc;
                                }
                            }
                            for (i = 0; i<args.cinput; ++i) {
                                if (!inputstreams) inputstreams = strdup(args.vinput[i].str);
                                else {
                                    char *conc   = GBS_global_string_copy("%s;%s", inputstreams, args.vinput[i].str);
                                    free(inputstreams);
                                    inputstreams = conc;
                                }
                            }

                            if (paramlist) {
                                error = GBS_global_string("while applying '%s(%s)' to '%s': %s", s1, paramlist, inputstreams, error);
                            }
                            else {
                                error = GBS_global_string("while applying '%s' to '%s': %s", s1, inputstreams, error);
                            }

                            free(inputstreams);
                            free(paramlist);
                        }
                    }
                }

                for (i=0;i<argcparam;i++) {     /* free intermediate arguments */
                    if (in[i].str) free(in[i].str);
                }
            }

            if (error) break;

            if (seperator == '|') {         /* swap in and out in pipes */
                GBL *h;
                for (i=0;i<argcinput;i++) {
                    if (orig[i].str)    free(orig[i].str);
                }
                memset((char*)orig,0,sizeof(GBL)*GBL_MAX_ARGUMENTS);
                argcinput = 0;

                h = out;            /* swap orig and out */
                out = orig;
                orig = h;

                argcinput = argcout;
                argcout = 0;
            }

        }
    }
    for (i=0;i<argcinput;i++) {
        if (orig[i].str) free((char *)(orig[i].str));
    }

    {
        char *s1;
        if (!argcout) {
            s1 = GB_STRDUP(""); /* returned '<NULL>' in the past */
        }
        else if (argcout ==1) {
            s1 = out[0].str;
        }
        else{              /* concatenate output strings */
            void *strstruct = GBS_stropen(1000);
            for (i=0;i<argcout;i++) {
                if (out[i].str){
                    GBS_strcat(strstruct,out[i].str);
                    free(out[i].str);
                }
            }
            s1 = GBS_strclose(strstruct);
        }
        free(buffer);

        if (!error) {
            if (trace) printf("GB_command_interpreter: result='%s'\n", s1);
            return s1;
        }
        free(s1);
    }

    GB_export_error("in '%s': %s", commands, error);
    return 0;
}

