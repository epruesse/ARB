#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
/* #include <malloc.h> */

#include "adlocal.h"
#include "arbdbt.h"

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef NO_REGEXPR
int regerrno;
#   define INIT   register char *sp = instring;
#   define GETC() (*sp++)
#   define PEEKC()     (*sp)
#   define UNGETC(c)   (--sp)
#   define RETURN(c)   return c;
#   define ERROR(c)    gbs_regerror(c)
#   include "adregexp.h"
#else
#   ifdef SGI
#   define _MODERN_C_
#   endif
#   include <regexpr.h>
#endif


/********************************************************************************************
                    directory handling
********************************************************************************************/

/* look for quick saves (basename = yyy/xxx no arb ending !!!!) */
GB_ERROR gb_scan_directory(char *basename, struct gb_scandir *sd){
    char *path = GB_STRDUP(basename);
    const char *fulldir = ".";
    char *file = strrchr(path,'/');
    DIR *dirp;
    int curindex;
    char *suffix;
    struct dirent *dp;
    struct stat st;
    const char *oldstyle = ".arb.quick";
    char    buffer[GB_PATH_MAX];
    int oldstylelen = strlen(oldstyle);
    int filelen;

    if (file) {
        *(file++) = 0;
        fulldir = path;
    } else {
        file = path;
    }

    memset((char*)sd,0,sizeof(*sd));
    sd->type = GB_SCAN_NO_QUICK;
    sd->highest_quick_index = -1;
    sd->newest_quick_index = -1;
    sd->date_of_quick_file = 0;

    dirp = opendir(fulldir);
    if (!dirp){
        GB_ERROR error = GB_export_error("Directory %s of file %s.arb not readable",fulldir,file);
        free(path);
        return error;
    }
    filelen = strlen(file);
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)){
        if (strncmp(dp->d_name,file,filelen)) continue;
        suffix = dp->d_name + filelen;
        if (suffix[0] != '.') continue;
        if (!strncmp(suffix,oldstyle,oldstylelen)){
            if (sd->type == GB_SCAN_NEW_QUICK){
                printf("Warning: Found new and old changes files, using new\n");
                continue;
            }
            sd->type = GB_SCAN_OLD_QUICK;
            curindex = atoi(suffix+oldstylelen);
            goto check_time_and_date;
        }
        if (strlen(suffix) == 4 &&
            suffix[0] == '.' &&
            suffix[1] == 'a' &&
            isdigit(suffix[2]) &&
            isdigit(suffix[3])){
            if (sd->type == GB_SCAN_OLD_QUICK){
                printf("Warning: Found new and old changes files, using new\n");
            }
            sd->type = GB_SCAN_NEW_QUICK;
            curindex = atoi(suffix+2);
            goto check_time_and_date;
        }
        continue;
    check_time_and_date:
        if (curindex > sd->highest_quick_index) sd->highest_quick_index = curindex;
        sprintf(buffer,"%s/%s",fulldir,dp->d_name);
        stat(buffer,&st);
        if ((unsigned long)st.st_mtime > sd->date_of_quick_file){
            sd->date_of_quick_file = st.st_mtime;
            sd->newest_quick_index = curindex;
        }
        continue;
    }
    closedir(dirp);
    free(path);
    return 0;
}


char *GB_find_all_files(const char *dir,const char *mask, GB_BOOL filename_only) {
    /* Returns a string containing the filenames of all files matching mask.
       The single filenames are seperated by '*'.
       if 'filename_only' is true -> string contains only filenames w/o path
       returns 0 if no files found (or directory not found)
    */

    DIR           *dirp;
    struct dirent *dp;
    struct stat    st;
    char          *result = 0;
    char           buffer[GB_PATH_MAX];

    dirp = opendir(dir);
    if (dirp) {
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
            if (!GBS_string_scmp(dp->d_name,mask,0)) {
                sprintf(buffer,"%s/%s",dir,dp->d_name);
                if (stat(buffer,&st) == 0  && S_ISREG(st.st_mode)) { // regular file ?
                    if (filename_only) strcpy(buffer, dp->d_name);
                    if (result) {
                        char *neu = GB_strdup(GBS_global_string("%s*%s", result, buffer));
                        free(result);
                        result    = neu;
                    }
                    else {
                        result = GB_strdup(buffer);
                    }
                }
            }
        }
        closedir(dirp);
    }

    return result;
}

char *GB_find_latest_file(const char *dir,const char *mask){
    DIR           *dirp;
    struct dirent *dp;
    char           buffer[GB_PATH_MAX];
    struct stat    st;
    GB_ULONG       newest = 0;
    char          *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)){
            if (!GBS_string_scmp(dp->d_name,mask,0)){
                sprintf(buffer,"%s/%s",dir,dp->d_name);
                if (stat(buffer,&st) == 0){
                    if ((GB_ULONG)st.st_mtime > newest){
                        newest = st.st_mtime;
                        if (result) free(result);
                        result = GB_STRDUP(&dp->d_name[0]);
                    }
                }
            }
        }
        closedir(dirp);
    }
    return result;
}

#define GBS_SET ((char)1)
#define GBS_SEP ((char)2)
#define GBS_MWILD ((char)3)
#define GBS_WILD ((char)4)

gb_warning_func_type gb_warning_func;
gb_information_func_type gb_information_func;
gb_status_func_type gb_status_func;
gb_status_func2_type gb_status_func2;

/********************************************************************************************
                    error handling
********************************************************************************************/

void GB_raise_critical_error(const char *msg) {
    fprintf(stderr, "------------------------------------------------------------\n");
    fprintf(stderr, "A critical error occurred in ARB\nError-Message: %s\n", msg);
#if defined(DEBUG)
    fprintf(stderr, "Run the debugger to find the location where the error was raised.\n");
#endif /* DEBUG */
    fprintf(stderr, "------------------------------------------------------------\n");
    gb_assert(0);
    exit(-1);
}


static char *GB_error_buffer = 0;

GB_ERROR GB_export_error(const char *templat, ...) {
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    char     buffer[GBS_GLOBAL_STRING_SIZE];
    char    *p  = buffer;
    va_list  parg;
    memset(buffer,0,1000);
    sprintf (buffer,"ARB ERROR: ");
    p          += strlen(p);
    va_start(parg,templat);

    vsprintf(p,templat,parg);

    if (GB_error_buffer) free(GB_error_buffer);
    GB_error_buffer = GB_STRDUP(buffer);
    return GB_error_buffer;
}

GB_ERROR GB_export_IO_error(const char *action, const char *filename) {
    /* like GB_export_error() - creates error message from current 'errno'
       action may be NULL (otherwise it should contain sth like "writing" or "deleting")
       filename may be NULL (otherwise it should contain the filename, the IO-Error occurred for)
    */

    const char *error_message;
    if (errno) {
        error_message = strerror(errno);
    }
    else {
        ad_assert(0);           // unhandled error (which is NOT an IO-Error)
        error_message =
            "Some unhandled error occurred, but it was not an IO-Error. "
            "Please send detailed information about how the error occurred to devel@arb-home.de "
            "or ignore this error (if possible).";
    }

    {
        char buffer[GBS_GLOBAL_STRING_SIZE];

        if (action) {
            if (filename) sprintf(buffer, "ARB ERROR: When %s '%s': %s", action, filename, error_message);
            else sprintf(buffer, "ARB ERROR: When %s <unknown file>: %s", action, error_message);
        }
        else {
            if (filename) sprintf(buffer, "ARB ERROR: Concerning '%s': %s", filename, error_message);
            else sprintf(buffer, "ARB ERROR: %s", error_message);
        }

        if (GB_error_buffer) free(GB_error_buffer);
        GB_error_buffer = GB_STRDUP(buffer);
    }
    return GB_error_buffer;
}

GB_ERROR GB_print_error()
{
    if (GB_error_buffer){
        fflush(stdout);
        fprintf(stderr,"%s\n",GB_error_buffer);
    }
    return GB_error_buffer;
}

GB_ERROR GB_get_error()
{
    return GB_error_buffer;
}

void GB_clear_error() {         /* clears the error buffer */
    free(GB_error_buffer);
    GB_error_buffer = 0;
}

/* -------------------------------------------------------------------------------- */

#define GLOBAL_STRING_BUFFERS 4

static GB_CSTR gbs_vglobal_string(const char *templat, va_list parg, int allow_reuse)
{
    static char buffer[GLOBAL_STRING_BUFFERS][GBS_GLOBAL_STRING_SIZE+2]; // two buffers - used alternately
    static int  idx    = 0;
    int         my_idx = (idx+1)%GLOBAL_STRING_BUFFERS; // use next buffer
    int         psize;

    ad_assert(my_idx >= 0 && my_idx<GLOBAL_STRING_BUFFERS);

#ifdef LINUX
    psize = vsnprintf(buffer[my_idx],GBS_GLOBAL_STRING_SIZE,templat,parg);
#else
    psize = vsprintf(buffer[my_idx],templat,parg);
#endif

    if (psize == -1 || psize >= GBS_GLOBAL_STRING_SIZE) {
        ad_assert(0);              // buffer overflow (increase GBS_GLOBAL_STRING_SIZE or use your own buffer)
        GB_CORE;
    }

    if (!allow_reuse) idx = my_idx;

    return buffer[my_idx];
}

GB_CSTR GBS_global_string(const char *templat, ...) {
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;
    GB_CSTR result;

    va_start(parg,templat);
    result = gbs_vglobal_string(templat, parg, 0);
    va_end(parg);

    return result;
}

char *GBS_global_string_copy(const char *templat, ...) {
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;
    GB_CSTR result;

    va_start(parg,templat);
    result = gbs_vglobal_string(templat, parg, 1);
    va_end(parg);

    return GB_STRDUP(result);
}




char *GBS_string_2_key_with_exclusions(const char *str, const char *additional)
     /* converts any string to a valid key (all chars in 'additional' are additionally allowed) */
{
    char buf[GB_KEY_LEN_MAX+1];
    int i;
    int c;
    for (i=0;i<GB_KEY_LEN_MAX;) {
        c = *(str++);
        if (!c) break;

        if (c==' ' || c == '_' ) {
            buf[i++]  = '_';
        }
        else if (isalnum(c) || strchr(additional, c) != 0) {
            buf[i++]  = c;
        }
    }
    for (;i<GB_KEY_LEN_MIN;i++) buf[i] = '_';
    buf[i] = 0;
    return GB_STRDUP(buf);
}

char *GBS_string_2_key(const char *str) /* converts any string to a valid key */
{
    return GBS_string_2_key_with_exclusions(str, "");
}

void gbs_uppercase(char *str)
{
    register char c;
    while( (c=*str) )   {
        if ( (c<='z') && (c>='a')) *str = c - 'a' + 'A';
        str++;
    }
}

void gbs_memcopy(char *dest, const char *source, long len)
{
    register long    i;
    register const char  *s;
    register char  *d;

    i = len;
    s = source;
    d = dest;
    if (s < d) {
        s += i;
        d += i;
        while (i--) {
            *(--d) = *(--s);
        }
    } else {
        while (i--) {
            *(d++) = *(s++);
        }
    }
}

char *gbs_malloc_copy(const char *source, long len)
{
    char *dest;
    dest = (char *)malloc((size_t)len);
    GB_MEMCPY(dest,source,(int)len);
    return dest;
}

GB_ERROR GB_check_key(const char *key)
     /* test whether all characters are letters, numbers or _ */
{
    int  i;
    long len;

    if (!key || key[0] == 0) return GB_export_error("Empty key is not allowed");
    len = strlen(key);
    if (len>GB_KEY_LEN_MAX) return GB_export_error("Invalid key '%s': too long",key);
    if (len < GB_KEY_LEN_MIN) return GB_export_error("Invalid key '%s': too short",key);

    for (i = 0; key[i]; ++i) {
        char c = key[i];
        if ( (c>='a') && (c<='z')) continue;
        if ( (c>='A') && (c<='Z')) continue;
        if ( (c>='0') && (c<='9')) continue;
        if ( (c=='_') ) continue;
        return GB_export_error("Invalid character '%c' in '%s'; allowed: a-z A-Z 0-9 '_' ", c, key);
    }

    return 0;
}
GB_ERROR GB_check_link_name(const char *key)
     /* test whether all characters are letters, numbers or _ */
{
    int  i;
    long len;

    if (!key || key[0] == 0) return GB_export_error("Empty key is not allowed");
    len = strlen(key);
    if (len>GB_KEY_LEN_MAX) return GB_export_error("Invalid key '%s': too long",key);
    if (len < 1) return GB_export_error("Invalid key '%s': too short",key); // here it differs from GB_check_key

    for (i = 0; key[i]; ++i) {
        char c = key[i];
        if ( (c>='a') && (c<='z')) continue;
        if ( (c>='A') && (c<='Z')) continue;
        if ( (c>='0') && (c<='9')) continue;
        if ( (c=='_') ) continue;
        return GB_export_error("Invalid character '%c' in '%s'; allowed: a-z A-Z 0-9 '_' ", c, key);
    }

    return 0;
}
GB_ERROR GB_check_hkey(const char *key)
     /* test whether all characters are letters, numbers or _ */
     /* additionally allow '/' and '->' for hierarchical keys */
{
    GB_ERROR err = 0;

    if (!key || key[0] == 0) {
        err = GB_export_error("Empty key is not allowed");
    }
    else if (!strpbrk(key, "/-")) {
        err = GB_check_key(key);
    }
    else {
        char *key_copy = strdup(key);
        char *start    = key_copy;

        if (start[0] == '/') ++start;

        while (start && !err) {
            char *key_end = strpbrk(start, "/-");

            if (key_end) {
                char c   = *key_end;
                *key_end = 0;
                err      = GB_check_key(start);
                *key_end = c;

                if (c == '-') {
                    if (key_end[1] != '>') {
                        err = GB_export_error("'>' expected after '-' in '%s'", key);
                    }
                    start = key_end+2;
                }
                else  {
                    ad_assert(c == '/');
                    start = key_end+1;
                }
            }
            else {
                err   = GB_check_key(start);
                start = 0;
            }
        }

        free(key_copy);
    }

    return err;
}

/*  int  i; */
/*  long len; */

/*  if (!key || key[0] == 0) return GB_export_error("Empty key is not allowed"); */
/*  len = strlen(key); */
/*  if (len>GB_KEY_LEN_MAX) return GB_export_error("Invalid key '%s': too long",key); */
/*  if (len < GB_KEY_LEN_MIN) return GB_export_error("Invalid key '%s': too short",key); */

/*  for (i = 0; key[i]; ++i) { */
/*         char c = key[i]; */
/*      if ( (c>='a') && (c<='z')) continue; */
/*      if ( (c>='A') && (c<='Z')) continue; */
/*      if ( (c>='0') && (c<='9')) continue; */
/*      if ( (c=='_') ) continue; */

         /* hierarchical keys :  */
/*      if ( (c=='/') ) continue; */
/*      if ( (c=='-') && (key[i+1] == '>') ) { ++i; continue; } */

/*      return GB_export_error("Invalid character '%c' in '%s'; allowed: a-z A-Z 0-9 '_' '->' '/'", c, key); */
/*  } */

/*  return 0; */


char *GBS_strdup(const char *s){
    if (!s) return NULL;
    return GB_STRDUP(s);
}

#define UPPERCASE(c) if ( (c>='a') && (c<='z')) c+= 'A'-'a'

/* search a substring in another string
    match_mode == 0     -> exact match
    match_mode == 1     -> a==A
    match_mode == 2     -> a==a && a==?
    match_mode == else  -> a==A && a==?
*/

GB_CPNTR GBS_find_string(const char *str, const char *key, long match_mode)
{
    register const char  *p1, *p2;
    register char b;
    switch (match_mode) {

        case 0: /* exact match */
            for (p1 = str, p2 = key; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1) {
                        p1++;
                        p2++;
                    } else {
                        p2 = key;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2)   return (char *)str;
            break;

        case 1: /* a==A */
            for (p1 = str, p2 = key; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b)) {
                        p1++;
                        p2++;
                    } else {
                        p2 = key;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
        case 2: /* a==a && a==? */
            for (p1 = str, p2 = key; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1 || (b=='?')) {
                        p1++;
                        p2++;
                    } else {
                        p2 = key;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;

        default: /* a==A && a==? */
            for (p1 = str, p2 = key; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b) || (b=='?') ) {
                        p1++;
                        p2++;
                    } else {
                        p2 = key;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
    }
    return 0;
}
/* @@@ OLI */
/** same as GBS_string_cmp except /regexpr/ matches regexpr */
long GBS_string_scmp(const char *str,const char *search,long upper_case){
    if (search[0] == '/') {
        if (search[strlen(search)-1] == '/'){
            if (GBS_regsearch(str,search)){
                return 0;
            }
            return 1;
        }
    }
    return GBS_string_cmp(str,search,upper_case);
}


long GBS_string_cmp(const char *str,const char *search,long upper_case)
     /* *   Wildcard in search string */
     /* ?   any Charakter   */
     /* if uppercase    change all letters to uppercase */
     /* returns 0 if strings are equal -1 or +1 if left strings is
        less/greater than right string */
{
    register const char *p1,*p2;
    register char a,b,*d;
    register long i;
    char fsbuf[256];

    p1 = str;
    p2 = search;
    while (1) {
        a = *p1;b=*p2;
        if (b == '*')   {
            if (!p2[1]) break; /* -> return 0; */
            i = 0;
            d = fsbuf;
            for (p2++;(b=*p2)&&(b!='*');){
                *(d++) = b;
                p2++;
                i++;
                if (i > 250) break;
            }
            if (*p2 != '*' ) {
                p1 += strlen(p1)-i;     /* check the end of the string */
                if (p1 < str) return -1;    /* neither less or greater */
                p2 -= i;
            }else{
                *d=0;
                p1 = GBS_find_string(p1,fsbuf,upper_case+2);
                if (!p1) return -1;
                p1 += i;
            }
            continue;
        }else{
            if (!a) return b;
            if (!b) return a;
            if (b != '?')   {
                if (upper_case) {
                    if (toupper(a) !=toupper(b) ) return 1;
                }else{
                    if (a!=b) return 1;
                }
            }
        }
        p1++;
        p2++;
    }
    return 0;
}

char *gbs_add_path(char *path,char *name)
{
    long i,len,found;
    char *erg;
    if (!name) return name;
    if (!path) {
        return 0;
    }
    if (*name == '/') return name;
    found =0;
    len = strlen(path);
    for (i=0;i<len;i++) {
        if (path[i] == '/') found = i+1;
    }
    len = found + strlen(name);
    erg = (char *)GB_calloc(sizeof(char),(size_t)(len +1));
    for (i=0;i<found;i++) {
        erg[i] = path[i];
    }
    for (i=found;i<len;i++){
        erg[i] = name[i-found];
    }
    return erg;
}

/********************************************************************************************
                    String Replace
********************************************************************************************/

char *gbs_compress_command(const char *com) /* replaces all '=' by GBS_SET
                                               ':' by GBS_SEP
                                               '?' by GBS_WILD if followed by a number or '?'
                                               '*' by GBS_MWILD  or '('
                                               \ is the escape charakter
                                            */
{
    register char *result,*s,*d;
    int ch;

    s = d = result = GB_STRDUP(com);
    while ( (ch = *(s++)) ){
        switch (ch) {
            case '=':   *(d++) = GBS_SET;break;
            case ':':   *(d++) = GBS_SEP;break;
            case '?':
                ch = *s;
                /*if ( (ch>='0' && ch <='9') || ch=='?'){   *(d++) = GBS_WILD;break;}*/
                *(d++) = GBS_WILD; break;
            case '*':
                ch = *s;
                /* if ( (ch>='0' && ch <='9') || ch=='('){  *(d++) = GBS_MWILD;break;}*/
                *(d++) = GBS_MWILD; break;
            case '\\':
                ch = *(s++); if (!ch) { s--; break; };
                switch (ch) {
                    case 'n':   *(d++) = '\n';break;
                    case 't':   *(d++) = '\t';break;
                    case '0':   *(d++) = '\0';break;
                    default:    *(d++) = ch;break;
                }
                break;
            default:
                *(d++) = ch;
        }
    }
    *d = 0;
    return result;
}

char *GBS_remove_escape(char *com)  /* \ is the escape charakter
                                     */
{
    register char *result,*s,*d;
    int ch;

    s = d = result = GB_STRDUP(com);
    while ( (ch = *(s++)) ){
        switch (ch) {
            case '\\':
                ch = *(s++); if (!ch) { s--; break; };
                switch (ch) {
                    case 'n':   *(d++) = '\n';break;
                    case 't':   *(d++) = '\t';break;
                    case '0':   *(d++) = '\0';break;
                    default:    *(d++) = ch;break;
                }
                break;
            default:
                *(d++) = ch;
        }
    }
    *d = 0;
    return result;
}

/********************************************************************************************
                    escape/unescape characters in strings
********************************************************************************************/

char *GBS_escape_string(const char *str, const char *chars_to_escape, char escape_char) {
    // uses a special escape-method, which eliminates all 'chars_to_escape' completely
    // from str (this makes further processing of the string more easy)
    //
    // escape_char is the character used for escaping. For performance reasons it
    // should be a character rarely used in 'str'.
    //
    // 'chars_to_escape' may not contain 'A'-'Z' (these are used for escaping)
    // and it may not be longer than 26
    //
    // RETURN VALUE : heap copy of escaped string

    int   len    = strlen(str);
    char *buffer = (char*)malloc(2*len+1);
    int   j      = 0;
    int   i;

    ad_assert(strlen(chars_to_escape)              <= 26);
    ad_assert(strchr(chars_to_escape, escape_char) == 0); // escape_char may not be included in chars_to_escape

    for (i = 0; str[i]; ++i) {
        if (str[i] == escape_char) {
            buffer[j++] = escape_char;
            buffer[j++] = escape_char;
        }
        else {
            const char *found = strchr(chars_to_escape, str[i]);
            if (found) {
                buffer[j++] = escape_char;
                buffer[j++] = (found-chars_to_escape+'A');

                ad_assert(found[0]<'A' || found[0]>'Z'); // illegal character in chars_to_escape
            }
            else {

                buffer[j++] = str[i];
            }
        }
    }
    buffer[j] = 0;

    return buffer;
}

char *GBS_unescape_string(const char *str, const char *escaped_chars, char escape_char) {
    // undoes GB_escape_string
    int   len    = strlen(str);
    char *buffer = (char*)malloc(len+1);
    int   j      = 0;
    int   i;

#if defined(DEBUG)
    int escaped_chars_len = strlen(escaped_chars);
#endif // DEBUG

    ad_assert(strlen(escaped_chars)              <= 26);
    ad_assert(strchr(escaped_chars, escape_char) == 0); // escape_char may not be included in chars_to_escape

    for (i = 0; str[i]; ++i) {
        if (str[i] == escape_char) {
            if (str[i+1] == escape_char) {
                buffer[j++] = escape_char;
            }
            else {
                int idx = str[i+1]-'A';

                ad_assert(idx >= 0 && idx<escaped_chars_len);
                buffer[j++] = escaped_chars[idx];
            }
            ++i;
        }
        else {
            buffer[j++] = str[i];
        }
    }
    buffer[j] = 0;

    return buffer;
}



/********************************************************************************************
                    String streams
********************************************************************************************/

struct GBS_strstruct {
    char *GBS_strcat_data;
    long  GBS_strcat_data_size;
    long  GBS_strcat_pos;
};

static struct GBS_strstruct *last_used = 0;

void *GBS_stropen(long init_size)   { /* opens a memory file */
    struct GBS_strstruct *strstr;

    if (last_used && last_used->GBS_strcat_data_size >= init_size) {
        strstr    = last_used;
        last_used = 0;
    }
    else {
#if defined(DEBUG) && 0
        printf("allocating new GBS_strstruct (size = %li)\n", init_size);
#endif /* DEBUG */
        strstr                       = (struct GBS_strstruct *)malloc(sizeof(struct GBS_strstruct));
        strstr->GBS_strcat_data_size = init_size;
        strstr->GBS_strcat_data      = (char *)malloc((size_t)strstr->GBS_strcat_data_size);
    }

    strstr->GBS_strcat_pos     = 0;
    strstr->GBS_strcat_data[0] = 0;

    return (void *)strstr;
}

char *GBS_strclose(void *strstruct)
     /* returns a char* copy of the memory file */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    long                  length = strstr->GBS_strcat_pos;
    char                 *str    = (char*)malloc(length+1);

    memcpy(str, strstr->GBS_strcat_data, length+1); /* copy with 0 */

    if (last_used) {
        if (last_used->GBS_strcat_data_size < strstr->GBS_strcat_data_size) { /* last_used is smaller -> keep this */
            struct GBS_strstruct *tmp = last_used;
            last_used                 = strstr;
            strstr                    = tmp;
        }
#if defined(DEBUG) && 0
        printf("freeing GBS_strstruct (size = %li)\n", strstr->GBS_strcat_data_size);
#endif /* DEBUG */
        free(strstr->GBS_strcat_data);
        free(strstr);
    }
    else {
        last_used = strstr;
    }

    return str;

    /*     old version: */
    /*     char                 *str; */
    /*     struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct; */
    /*     if (optimize) { */
    /*         str = GB_STRDUP(strstr->GBS_strcat_data); */
    /*         free(strstr->GBS_strcat_data); */
    /*     }else{ */
    /*         str = strstr->GBS_strcat_data; */
    /*     } */
    /*     free((char *)strstr); */
    /*     return str; */
}

GB_CPNTR GBS_mempntr(void *strstruct)   /* returns the memory file */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    return strstr->GBS_strcat_data;
}

long GBS_memoffset(void *strstruct) /* returns the memory file */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    return strstr->GBS_strcat_pos;
}

/** Removes byte_count characters at the tail of a memfile */
void GBS_str_cut_tail(void *strstruct, int byte_count){
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    strstr->GBS_strcat_pos -= byte_count;
    if (strstr->GBS_strcat_pos < 0) strstr->GBS_strcat_pos = 0;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}

void gbs_strensure_mem(void *strstruct,long len){
    struct GBS_strstruct *strstr     = (struct GBS_strstruct *)strstruct;
    if (strstr->GBS_strcat_pos + len + 2 >= strstr->GBS_strcat_data_size) {
        strstr->GBS_strcat_data_size = (strstr->GBS_strcat_pos+len+2)*3/2;
        strstr->GBS_strcat_data      = (char *)realloc((MALLOC_T)strstr->GBS_strcat_data,(size_t)strstr->GBS_strcat_data_size);
#if defined(DEBUG) && 0
        printf("re-allocated GBS_strstruct to size = %li\n", strstr->GBS_strcat_data_size);
#endif /* DEBUG */
    }
}

void GBS_strcat(void *strstruct,const char *ptr)    /* this function adds many strings. first create a strstruct with gbs_open */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    long                  len    = strlen(ptr);

    gbs_strensure_mem(strstruct,len);
    GB_MEMCPY(strstr->GBS_strcat_data+strstr->GBS_strcat_pos,ptr,(int)len);
    strstr->GBS_strcat_pos += len;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos]  = 0;
}

void GBS_strncat(void *strstruct,const char *ptr,long len)  /* this function adds many strings. first create a strstruct with gbs_open */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    gbs_strensure_mem(strstruct,len+2);
    GB_MEMCPY(strstr->GBS_strcat_data+strstr->GBS_strcat_pos,ptr,(int)len);
    strstr->GBS_strcat_pos += len;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}



void GBS_strnprintf(void *strstruct, long len, const char *templat, ...) {
    /* goes to header: __attribute__((format(printf, 3, 4)))  */

    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    char                 *buffer;
    int                   psize;

    va_list parg;
    va_start(parg,templat);
    gbs_strensure_mem(strstruct,len+2);

    buffer = strstr->GBS_strcat_data+strstr->GBS_strcat_pos;

#ifdef LINUX
    psize = vsnprintf(buffer,len,templat,parg);
#else
    psize = vsprintf(buffer,templat,parg);
#endif
    if (psize == -1 || psize>len) {
        ad_assert(0);
        GB_CORE;
    }

    strstr->GBS_strcat_pos += strlen(buffer);
}



void GBS_chrcat(void *strstruct,char ch)    /* this function adds many strings.
                                               The first call should be a null pointer */
{
    struct GBS_strstruct *strstr = (struct GBS_strstruct *)strstruct;
    if (strstr->GBS_strcat_pos + 3 >= strstr->GBS_strcat_data_size) {
        strstr->GBS_strcat_data_size = (strstr->GBS_strcat_pos+3)*3/2;
        strstr->GBS_strcat_data = (char *)realloc((MALLOC_T)strstr->GBS_strcat_data,
                                                  (size_t)strstr->GBS_strcat_data_size);
    }
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos++] = ch;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}

void GBS_intcat(void *strstruct,long val)
{
    char buffer[200];
    long len = sprintf(buffer,"%li",val);
    GBS_strncat(strstruct,buffer, len);
}

void GBS_floatcat(void *strstruct,double val)
{
    char buffer[200];
    long len = sprintf(buffer,"%f",val);
    GBS_strncat(strstruct,buffer, len);
}



/********************************************************************************************
                    String Replace
********************************************************************************************/
int GBS_reference_not_found;

GB_ERROR gbs_build_replace_string(void *strstruct,
                                  char *bar,char *wildcards, long max_wildcard,
                                  char **mwildcards, long max_mwildcard, GBDATA *gb_container)
{
    register char *p,c,d;
    int wildcardcnt = 0;
    int mwildcardcnt = 0;
    int wildcard_num;

    char    *entry;
    p = bar;


    wildcardcnt = 0;
    mwildcardcnt = 0;

    p = bar;
    while ( (c=*(p++)) ) {
        switch (c){
            case GBS_MWILD: case GBS_WILD:
                d = *(p++);
                if (gb_container && (d=='(')) { /* if a gbcont then replace till ')' */
                    GBDATA *gb_entry;
                    char    *klz;
                    char    *psym;
                    klz = gbs_search_second_bracket(p);
                    if (klz) {          /* reference found: $(gbd) */
                        int seperator = 0;
                        *klz = 0;
                        psym = strpbrk(p,"#|:");
                        if (psym) {
                            seperator = *psym;
                            *psym =0;
                        }
                        if (*p){
                            gb_entry = GB_search(gb_container,p,GB_FIND);
                        }else{
                            gb_entry = gb_container;
                        }
                        if (psym) *psym = seperator;

                        if (!gb_entry || gb_entry == gb_container) {
                            GBS_reference_not_found = 1;
                            entry=GB_STRDUP("");
                        }else{
                            entry = GB_read_as_string(gb_entry);
                        }
                        if (entry) {
                            char *h;
                            switch(seperator) {
                                case ':':
                                    h = GBS_string_eval(
                                                        entry,psym+1,gb_container);
                                    if (h){
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }else{
                                        return GB_get_error();
                                    }
                                    break;
                                case '|':
                                    h = GB_command_interpreter(GB_get_root(gb_container), entry,psym+1,gb_container, 0);
                                    if (h){
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }else{
                                        return GB_get_error();
                                    }
                                    break;
                                case '#':
                                    if (!gb_entry){
                                        GBS_strcat(strstruct,psym+1);
                                        break;
                                    }
                                default:
                                    GBS_strcat(strstruct,entry);
                                    break;
                            }
                            free(entry);
                        }
                        *klz = ')';
                        p = klz+1;
                        break;
                    }
                    c = '*';
                }else{
                    wildcard_num = d - '1';
                    if (c == GBS_WILD) {
                        c = '?';
                        if ( (wildcard_num<0)||(wildcard_num>9) ) {
                            p--;            /* use this character */
                            wildcard_num = wildcardcnt++;
                        }
                        if (wildcard_num>=max_wildcard) {
                            GBS_chrcat(strstruct,c);
                        }else{
                            GBS_chrcat(strstruct,wildcards[wildcard_num]);
                        }
                    }else{
                        c = '*';
                        if ( (wildcard_num<0)||(wildcard_num>9) ) {
                            p--;            /* use this character */
                            wildcard_num = mwildcardcnt++;
                        }
                        if (wildcard_num>=max_mwildcard) {
                            GBS_chrcat(strstruct,c);
                        }else{
                            GBS_strcat(strstruct,mwildcards[wildcard_num]);
                        }
                    }
                    break;
                }
                GBS_chrcat(strstruct,c);
                GBS_chrcat(strstruct,d);
                break;
            default:
                GBS_chrcat(strstruct,c);
        }       /* switch c */
    }
    return 0;
}

char *GBS_string_eval(const char *insource, const char *icommand, GBDATA *gb_container)
     /* GBS_string_eval replaces substrings in source
        Syntax: command = "oliver=olli:peter=peti"
        The string is GB_STRDUPped !
        *   is a wildcard for any number of character
        ?   is a wildcard for exactly one character

        To reference to the wildcards on the left side of the '='
        use ? and *, to reference in a different order use:
            *0 to reference to the first occurence of *
            *1          second
            ...
            *9

        if the last and first characters of the search string are no '*' wildcards then
        the replace is repeated as many times as possible
        '\' is the escape character: e.g. \n is newline; '\\' is '\'; '\=' is '='; ....

    eg:
        print first three characters of first word and the whole second word:

        *(arb_key)  is the value of the a database entry arb key

    */

{
    register char *source;      /* pointer into the current string when parsed */
    register char *search;      /* pointer into the current command when parsed */
    register char *p;       /* short live pointer */
    register char c;
    register char *already_transferred; /* point into 'in' string to non parsed position */

    char    wildcard[40];
    char    *mwildcard[10];
    GB_ERROR error;

    long    i;
    long    max_wildcard;
    long    max_mwildcard;


    char    *start_of_wildcard;
    char    what_wild_card;

    char    *start_match;

    char    *doppelpunkt;

    char    *bar;
    char    *in;
    char    *nextdp;
    void    *strstruct;
    char *command;

    if (!icommand || !icommand[0]) return GB_STRDUP(insource);

    command = gbs_compress_command(icommand);
    in = GB_STRDUP(insource);               /* copy insource to allow to destroy it */

    for (doppelpunkt = command; doppelpunkt; doppelpunkt = nextdp) {    /* loop over command string */
        /* in is in , strstruct is out */
        max_wildcard = 0;
        max_mwildcard = 0;
        nextdp = strchr(doppelpunkt, GBS_SEP);
        if (nextdp){
            *(nextdp++) = 0;
        }
        if (!doppelpunkt[0]) {                      /* empty command -> search next */
            continue;
        }

        bar = strchr(doppelpunkt+1, GBS_SET);               /* Parse the command string !!!! */
        if (bar) {
            *(bar++) = 0;
        } else {
            GB_export_error("SRT ERROR: no '=' found in command '%s' (position > %i)", icommand,doppelpunkt-command+1);
            free(command);
            free(in);
            return 0;
        }

        already_transferred = in;
        strstruct = GBS_stropen(1000);          /* create output stream */

        if ( (!*in) && doppelpunkt[0] == GBS_MWILD && doppelpunkt[1] == 0) {    /* empty string -> pars myself */
            /* * matches empty string !!!!  */
            mwildcard[max_mwildcard++] = GB_STRDUP("");
            gbs_build_replace_string(strstruct,bar,wildcard, max_wildcard,
                                     mwildcard, max_mwildcard,gb_container);
            goto gbs_pars_unsuccessfull;    /* successfull search*/
        }

        for (source = in;*source; ) {               /* loop over string */
            search = doppelpunkt;

            start_match = 0;                /* match string for '*' */
            while ( (c = *(search++))  ) {          /* search matching command */
                switch (c) {
                    case GBS_MWILD:
                        if (!start_match) start_match = source;

                        start_of_wildcard = search;
                        if ( !(c = *(search++) ) ) {    /* last character is a wildcard -> that was it */
                            mwildcard[max_mwildcard++] = GB_STRDUP(source);
                            source += strlen(source);
                            goto gbs_pars_successfull;      /* successfull search and end wildcard*/
                        }
                        while ( (c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD );   /* search the next wildcardstring */
                        search--;                   /* back one character */
                        *search = 0;
                        what_wild_card = c;
                        p = GBS_find_string(source,start_of_wildcard,0);
                        if (!p){                    /* string not found -> unsuccessful search */
                            goto gbs_pars_unsuccessfull;
                        }
                        c = *p;                     /* set wildcard */
                        *p = 0;
                        mwildcard[max_mwildcard++] = GB_STRDUP(source);
                        *p = c;
                        source = p + strlen(start_of_wildcard);         /* we parsed it */
                        *search = what_wild_card;
                        break;

                    case GBS_WILD:
                        if ( !source[0] ) {
                            goto gbs_pars_unsuccessfull;
                        }
                        if (!start_match) start_match = source;
                        wildcard[max_wildcard++] = *(source++);
                        break;
                    default:
                        if (start_match) {
                            if (c != *(source++)) {
                                goto gbs_pars_unsuccessfull;
                            }
                            break;
                        }else{
                            char *buf1;
                            buf1 = search-1;
                            while ( (c=*(search++)) && c!=GBS_MWILD && c!=GBS_WILD );   /* search the next wildcardstring */
                            search--;                   /* back one character */
                            *search = 0;
                            what_wild_card = c;
                            p = GBS_find_string(source,buf1,0);
                            if (!p){                    /* string not found -> unsuccessful search */
                                goto gbs_pars_unsuccessfull;
                            }
                            start_match = p;
                            source = p + strlen(buf1);          /* we parsed it */
                            *search = what_wild_card;
                        }
                        break;
                } /* switch */
            } /* while */           /* search matching command */

        gbs_pars_successfull:   /* now we got   source: pointer to end of match
                                   start_match: pointer to start of match
                                   in:  pointer to the entire string
                                   already_transferred: pointer to the start of the unparsed string
                                   bar: the replace string
                                */
            /* now look for the replace string */
            GBS_strncat(strstruct,already_transferred,start_match-already_transferred); /* cat old data */
            error = gbs_build_replace_string(strstruct,bar,wildcard, max_wildcard,      /* do the command */
                                             mwildcard, max_mwildcard,gb_container);
            already_transferred = source;

            for (i = 0; i < max_mwildcard; i++){
                free (mwildcard[i]);
                mwildcard[i] = 0;
            }
            max_wildcard = 0; max_mwildcard = 0;

            if (error) {
                free(GBS_strclose(strstruct));
                free(command);
                free(in);
                GB_export_error("%s",error);
                return 0;
            }
        }       /* while parsing */
    gbs_pars_unsuccessfull:
        GBS_strcat(strstruct,already_transferred);  /* cat the rest data */

        for (i = 0; i < max_mwildcard; i++){
            free (mwildcard[i]);
            mwildcard[i] = 0;
        }
        max_wildcard = 0; max_mwildcard = 0;

        p = GBS_strclose(strstruct);
        if (!strcmp(p,in)){ /* nothing changed */
            free(p);
        }else{
            free(in);
            if (nextdp) {       /* new command in line */
                in = p;
            }else{
                in = GB_STRDUP(p);
                free(p);
            }
        }
    }
    free(command);
    return in;
}

char *GBS_eval_env(const char *p){
    char *ka;
    char *kz;
    const char *genv;
    char *h;
    char *p2 = GB_STRDUP(p);
    char *eval;

    while ( (ka = GBS_find_string(p2,"$(",0)) ){
        kz = strchr(ka,')');
        if (!kz) {
            GB_export_error("missing ')' for enviroment '%s'",p2);
            return 0;
        }
        *kz = 0;
        genv = GB_getenv(ka+2);
        if (!genv) {
            genv = "";
        }
        eval = (char *)GB_calloc(sizeof(char),strlen(genv) + strlen(ka) + 10);
        sprintf(eval,"%s)=%s",ka,genv);
        *kz = ')';
        h=GBS_string_eval(p2,eval,0);
        free(eval);
        free(p2);
        p2 = h;
    }

    return p2;
}

/********************************************************************************************
            Create an entry representing pt-server 'i'
            (reads from arb_tcp.dat)
********************************************************************************************/

char *GBS_ptserver_id_to_choice(int i) {
    char  search_for[256];
    char  choice[256];
    char *fr;
    char *file;
    char *server;
    char empty[] = "";

    sprintf(search_for,"ARB_PT_SERVER%i",i);

    server = GBS_read_arb_tcp(search_for);
    if (!server) return 0;

    fr               = server;
    file             = server;  /* i got the machine name of the server */
    if (*file) file += strlen(file)+1; /* now i got the command string */
    if (*file) file += strlen(file)+1; /* now i got the file */

    if (strrchr(file,'/'))              file   = strrchr(file,'/')-1;
    if (!(server = strtok(server,":"))) server = empty;

    sprintf(choice,"%-8s: %s",server,file+2);
    free(fr);

    return strdup(choice);
}


/********************************************************************************************
            Find an entry in the $ARBHOME/lib/arb_tcp.dat file
********************************************************************************************/

char *GBS_read_arb_tcp(const char *env)
{
    char       *filename;
    char       *result = 0;

    if (strchr(env,':')){
        return GB_STRDUP(env);
    }

    filename = GBS_find_lib_file("arb_tcp.dat","", 1);
    if (!filename) {
        GB_export_error("File $ARBHOME/lib/arb_tcp.dat not found");
    }
    else {
        FILE       *arb_tcp = fopen(filename,"r");
        int         envlen  = strlen(env);
        const char *user    = GB_getenvUSER();

        if (!user) {
            GB_export_error("Enviroment Variable 'USER' not found");
            GB_print_error();
        }
        else {
            int   envuserlen = strlen(user)+envlen+1;
            char *envuser    = (char *)GB_calloc(sizeof(char),envuserlen+1);
            char  buffer[256];
            char *p;
            char *nl;
            char *tok1;

            sprintf(envuser,"%s:%s",user,env);

            for (p = fgets(buffer,255,arb_tcp);
                 p;
                 p = fgets(p,255,arb_tcp))
            {
                if (!strncmp(envuser,buffer,envuserlen)) {
                    envlen = envuserlen;
                    break;
                }
                if (!strncmp(env,buffer,envlen)) break;
            }

            if (p) {
                if ( (nl = strchr(p,'\n')) ) *nl = 0;
                p = p+envlen;
                while ((*p == ' ') || (*p == '\t')) p++;
                p = GBS_eval_env(p);
                strncpy(buffer,p,200);
                free(p);
                p = buffer;

                tok1 = strtok(p," \t");
                p = buffer;
                while (tok1) {
                    int len = strlen(tok1);
                    memmove(p, tok1, len+1);
                    p += len+1;
                    tok1 = strtok(0," \t");
                }

                result = (char*)GB_calloc(sizeof(char),p-buffer);
                GB_MEMCPY(result,buffer,p-buffer);
            }
            else {
                GB_export_error("Missing '%s' in '%s'",env,filename);
            }

            free(envuser);
        }
        fclose(arb_tcp);
    }
    free(filename);

    return result;
}

char* GBS_find_lib_file(const char *filename,const char *libprefix, int warn_when_not_found)
     /*
 * Searches in $CURRENTDIR $HOME $ARBHOME/lib
 */
{
    char            buffer[256];
    const char           *arbhome;
    const char           *home;
    FILE           *in;

    arbhome = GB_getenvARBHOME();
    home = GB_getenvHOME();

    in = fopen(filename, "r");
    if (in){
        fclose(in);
        return GB_STRDUP(filename);
    }
    if (filename[0] != '.') {
        if (strrchr(filename,'/')) filename = strrchr(filename,'/')+1;
    }

    sprintf(buffer, "%s/%s", home, filename);
    in = fopen(buffer, "r");
    if (in){
        fclose(in);
        return GB_STRDUP(buffer);
    }

    if (strrchr(filename,'/')) filename = strrchr(filename,'/')+1;
    if (filename[0] == '.') filename++;     /* no '.' in lib */

    sprintf(buffer, "%s/lib/%s%s", arbhome, libprefix, filename);
    in = fopen(buffer, "r");
    if (in){
        fclose(in);
        return GB_STRDUP(buffer);
    }

    if (warn_when_not_found) {
        fprintf(stderr, "WARNING dont know where to find %s\n", filename);
        fprintf(stderr, "   searched in .\n");
        fprintf(stderr, "   searched in $(HOME)     (==%s)\n", home);
        fprintf(stderr, "   searched in $(ARBHOME)/lib/%s   (==%s)\n", libprefix, arbhome);
    }
    return 0;
}



/********************************************************************************************
                    some simple find procedures
********************************************************************************************/

char **GBS_read_dir(const char *dir, const char *filter)
     /* read the content of the directory,
        if dir        == NULL then set dir to $ARBHOME/lib
    */
{
    char    dirbuffer[1024];
    char    command[1024];
    char    sin[256];
    char    **result=0;
    long    resultsize=50;
    long    resultptr=0;
    FILE    *ls;

#if defined(DEVEL_RALF)
#warning rewrite me
#endif /* DEVEL_RALF */

#ifdef DEBUG
    printf("dir='%s' filter='%s'\n",dir, filter);
#endif

    if (!dir) {
        dir = dirbuffer;
        sprintf(dirbuffer,"%s/lib",GB_getenvARBHOME());
    }
    if (filter) {
        sprintf(command,"ls %s/%s",dir,filter);
    }else{
        sprintf(command,"ls %s",dir);
    }
    ls = popen(command,"r");
    if (ls) {
        result = (char **)malloc((size_t)(sizeof(char *)*resultsize));
        while (
               sin[0] = 0,
               fscanf(ls,"%s\n",sin),
               sin[0]
               )
        {
            int len = strlen(sin);
            if (len>0) {
                if (sin[len-1] == ':') { // stop at first subdirectory
                    break;
                }
                else {
                    if (resultptr>=resultsize-1){
                        resultsize*=2;
                        result = (char **)realloc((MALLOC_T)result,(size_t)(sizeof(char *)*resultsize));
                    }
                    result[resultptr++] = GB_STRDUP(sin);
                }
            }
        }
        result[resultptr] = 0;
        fclose(ls);
    }
    return result;
}

GB_ERROR GBS_free_names(char **names)
{
    return GBT_free_names(names);
}

long GBS_gcgchecksum( const char *seq )
     /* GCGchecksum */
{
    register long  i, check = 0, count = 0;
    long seqlen = strlen(seq);
    for (i = 0; i < seqlen; i++) {
        count++;
        check += count * toupper(seq[i]);
        if (count == 57) count = 0;
    }
    check %= 10000;

    return check;
}

/* Table of CRC-32's of all single byte values (made by makecrc.c of ZIP source) */
const unsigned long crctab[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

long GB_checksum(const char *seq, long length, int ignore_case , const char *exclude) /* RALF: 02-12-96 */
     /*
      * CRC32checksum: modified from CRC-32 algorithm found in ZIP compression source
      * if ignore_case == true -> treat all characters as uppercase-chars (applies to exclude too)
      */
{
    register unsigned long c = 0xffffffffL;
    register long   n = length;
    register int    i;
    int tab[256];

    for (i=0;i<256;i++) {
        tab[i] = ignore_case ? toupper(i) : i;
    }

    if (exclude) {
        while (1) {
            int k  = *(unsigned char *)exclude++;
            if (!k) break;
            tab[k] = 0;
            if (ignore_case) tab[toupper(k)] = tab[tolower(k)] = 0;
        }
    }

    while (n--) {
        i = tab[*(const unsigned char *)seq++];
        if (i) {
            c = crctab[((int) c ^ i) & 0xff] ^ (c >> 8);
        }
    }
    c = c ^ 0xffffffffL;
    return c;
}

long GBS_checksum(const char *seq, int ignore_case, const char *exclude)
     /* if 'ignore_case' == true -> treat all characters as uppercase-chars (applies to 'exclude' too) */
{
    return GB_checksum(seq, strlen(seq), ignore_case, exclude);
}

/* extract all words in a text that:
    1. minlen < 1.0  contain more than minlen*len_of_text characters that also exists in chars
    2. minlen > 1.0  contain more than minlen characters that also exists in chars
*/

#ifdef __cplusplus
extern "C"
#endif

long GB_merge_sort_strcmp(void *v0, void *v1, char *not_used) {
    GBUSE(not_used);
    return strcmp((const char *)v0, (const char *)v1);
}

char *GBS_extract_words( const char *source,const char *chars, float minlen, GB_BOOL sort_output ) {
    char *s = GB_STRDUP(source);
    char **ps = (char **)GB_calloc(sizeof(char *), (strlen(source)>>1) + 1);
    void *strstruct = GBS_stropen(1000);
    char *f = s;
    int count = 0;
    char *p;
    register char *h;
    register int cnt;
    int len;

    int iminlen = (int) (minlen+.5);
    while ( (p = strtok(f," \t,;:|")) ) {
        f = 0;
        cnt = 0;
        len = strlen(p);
        for (h=p;*h;h++) {
            if (strchr(chars,*h)) cnt++;
        }

        if (minlen == 1.0) {
            if (cnt != len) continue;
        }else  if (minlen > 1.0) {
            if (cnt < iminlen) continue;
        }else{
            if (len < 3 || cnt < minlen*len) continue;
        }
        ps[count] = p;
        count ++;
    }
    if (sort_output) {
        GB_mergesort((void **)ps,0,count,GB_merge_sort_strcmp,0);
    }
    for (cnt = 0;cnt<count;cnt++) {
        if (cnt) {
            GBS_chrcat(strstruct,' ');
        }
        GBS_strcat(strstruct,ps[cnt]);
    }

    free((char *)ps);
    free(s);
    return GBS_strclose(strstruct);
}

int GBS_do_core(void)
{
    static char *flagfile = 0;
    static int result;
    FILE *f;
    if (flagfile) return result;
    flagfile = GBS_eval_env("$(ARBHOME)/do_core");
    f = fopen(flagfile,"r");
    if (f) {
        fclose(f);
        result = 1;
    }else{
        result = 0;
    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif

    void gb_error_to_stderr(const char *msg) {
        fprintf(stderr, "%s\n", msg);
    }

#ifdef __cplusplus
}
#endif

gb_error_handler_type gb_error_handler = gb_error_to_stderr;

NOT4PERL void GB_install_error_handler(gb_error_handler_type aw_message_handler){
    gb_error_handler = aw_message_handler;
}

void GB_internal_error(const char *templat, ...) {
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;
    GB_CSTR message;
    GB_CSTR full_message;

    va_start(parg, templat);
    message = gbs_vglobal_string(templat, parg, 0);
    va_end(parg);

    full_message = GBS_global_string("Internal ARB Error: %s", message);
    gb_error_handler(full_message);
    gb_error_handler("  ARB may be unstable now (due to this error).\n"
                     "  Consider saving your database (maybe under a different name),\n"
                     "  try to fix the error and restart ARB.");

    if (GBS_do_core()) {
        GB_CORE;
    }
#if defined(DEBUG)
    else {
        gb_assert(0);
        fprintf(stderr,"Debug file %s not found -> continuing operation \n",
                "$(ARBHOME)/do_core");
    }
#endif /* DEBUG */
}

void GB_warning( const char *templat, ...) {    /* max 4000 characters */
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;

    if ( gb_warning_func ) {
        char buffer[4000];memset(&buffer[0],0,4000);
        va_start(parg,templat);
        vsprintf(buffer,templat,parg);
        gb_warning_func(buffer);
    }else{
        va_start(parg,templat);
        vfprintf(stdout,templat,parg);
        fprintf(stdout,"\n");
    }
}

NOT4PERL void GB_install_warning(gb_warning_func_type warn){
    gb_warning_func = warn;
}

void GB_information( const char *templat, ...) {    /* max 4000 characters */
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;

    if ( gb_information_func ) {
        char buffer[4000];memset(&buffer[0],0,4000);
        va_start(parg,templat);
        vsprintf(buffer,templat,parg);
        gb_information_func(buffer);
    }else{
        va_start(parg,templat);
        vfprintf(stdout,templat,parg);
        fprintf(stdout,"\n");
    }
}

NOT4PERL void GB_install_information(gb_information_func_type info){
    gb_information_func = info;
}


int GB_status( double val) {
    int result = 0;
    if ( gb_status_func ) {
        result = gb_status_func(val);
    }
    else {
        char buffer[100];
        int i;
        static int lastv = 0;
        int v = (int)(val*80);
        if (v == lastv) return 0;
        lastv = v;
        for (i=0;i<v;i++) buffer[i] = '+';
        for (;i<80;i++) buffer[i] = '-';
        buffer[i] = 0;
        fprintf(stdout,"%s\n",buffer);
    }
    return result;
}

NOT4PERL void GB_install_status(gb_status_func_type func){
    gb_status_func = func;
}


int GB_status2( const char *templat, ... ) {
    /* goes to header: __attribute__((format(printf, 1, 2)))  */

    va_list parg;

    if ( gb_status_func2 ) {
char    buffer[4000];memset(&buffer[0],0,4000);
        va_start(parg,templat);
        vsprintf(buffer,templat,parg);
        return gb_status_func2(buffer);
    }else{
        va_start(parg,templat);
        vfprintf(stdout,templat,parg);
        fprintf(stdout,"\n");
        return 0;
    }
}

NOT4PERL void GB_install_status2(gb_status_func2_type func2){
    gb_status_func2 = func2;
}



/************************************************************
 *      Regular Expressions string/substition
 ************************************************************/
void gbs_regerror(int en){
    regerrno = en;
    switch(regerrno){
        case 11:    GB_export_error("Range endpoint too large.");break;
        case 16:    GB_export_error("Bad number.");break;
        case 25:    GB_export_error("``\\digit'' out of range.");break;
        case 36:    GB_export_error("Illegal or missing delimiter");break;
        case 41:    GB_export_error("No remembered search string");break;
        case 42:    GB_export_error("(~) imbalance.");break;
        case 43:    GB_export_error("Too many (");break;
        case 44:    GB_export_error("More than 2 numbers given in {~}");break;
        case 45:    GB_export_error("} expected after \\");break;
        case 46:    GB_export_error("First number exceeds second in {~}");break;
        case 49:    GB_export_error("[ ] imbalance");break;
    }
}



/****** expects '/term/by/' *******/

#ifdef NO_REGEXPR
/** regexpr = '/regexpr/' */

GB_CPNTR GBS_regsearch(const char *in, const char *regexprin){
    /* search the beginning first match */
    static char  expbuf[8000];
    static char *regexpr = 0;
    char        *res;
    int          rl      = strlen(regexprin)-2;
    if (regexprin[0] != '/' || regexprin[rl+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/'");
        return 0;
    }
    if (regexpr && !strncmp( regexpr, regexprin+1, rl))
        goto already_compiled;
    if (regexpr) free(regexpr);
    regexpr = GB_STRDUP(regexprin+1);
    regexpr[rl] = 0;

    regerrno = 0;
    res = compile(regexpr,&expbuf[0],&expbuf[8000],0);
    if (!res|| regerrno){
        gbs_regerror(regerrno);
        return 0;
    }
 already_compiled:
    if (step((char *)in,expbuf)) return loc1;
    return 0;
}

char *GBS_regreplace(const char *in, const char *regexprin, GBDATA *gb_species){
    static char  expbuf[8000];
    char        *regexpr;
    char        *subs;
    void        *out;
    char        *res;
    const char  *loc;
    int          rl = strlen(regexprin)-2;
    GBUSE(gb_species);
    if (regexprin[0] != '/' || regexprin[rl+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/replace/'");
        return 0;
    }
    /* Copy regexpr and remove leading + trailing '/' */
    regexpr = GB_STRDUP(regexprin+1);
    regexpr[rl] = 0;

    /* Search seperating '/' */
    subs                                                    = strrchr(regexpr,'/');
    while (subs && subs > regexpr && subs[-1] == '\\') subs = strrchr(subs,'/');
    if (!subs || subs == regexpr){
        free(regexpr);
        GB_export_error("no '/' found in regexpr"); /* dont change this error message (or change it in adquery/GB_command_interpreter too) */
        return 0;
    }
    *(subs++) = 0;
    regerrno = 0;
    res = compile(regexpr,expbuf,&expbuf[8000],0);
    if (!res || regerrno){
        gbs_regerror(regerrno);
        free(regexpr);
        return 0;
    }
    loc = in;
    out = GBS_stropen(10000);
    while ( step((char *)loc,expbuf)){      /* found a match */
        char *s;
        int c;
        GBS_strncat(out,loc,loc1-loc);  /* prefix */
        s = subs;
        while ( (c = *(s++)) ){
            if (c== '\\'){
                c = *(s++);
                if (!c) { s--; break; }
                switch (c){
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    default: break;
                }
            }
            GBS_chrcat(out,c);
        }
        loc = loc2;
    }
    GBS_strcat(out,loc);        /* copy rest */
    free ( regexpr);
    return GBS_strclose(out);
}
#else

/** regexpr = '/regexpr/' */
GB_CPNTR gb_compile_regexpr(const char *regexprin,char **subsout){
    static char *expbuf = 0;
    static char *old_reg_expr = 0;
    char *regexpr;
    char *subs;

    int rl = strlen(regexprin)-2;
    if (regexprin[0] != '/' || regexprin[rl+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/replace/'");
        return 0;
    }

    /* Copy regexpr and remove leading + trailing '/' */
    regexpr = GB_STRDUP(regexprin+1);
    regexpr[rl] = 0;
    if (subsout){
        /* Search seperating '/' */
        subs = strrchr(regexpr,'/');
        *subsout = 0;
        while (subs && subs > regexpr && subs[-1] == '\\') subs = strrchr(subs,'/');
        if (!subs || subs == regexpr){
            free(regexpr);
            GB_export_error("no '/' found in regexpr");
            return 0;
        }
        *(subs++) = 0;
        *subsout = subs;
    }

    regerrno = 0;
    if (!old_reg_expr || strcmp (old_reg_expr ,regexpr)){
        if (expbuf) free(expbuf);
        expbuf = compile(regexpr,0,0);
    }
    if (old_reg_expr) free(old_reg_expr);
    old_reg_expr = regexpr;
    regexpr = 0;

    if (regerrno){
        gbs_regerror(regerrno);
        return 0;
    }
    return expbuf;
}

GB_CPNTR GBS_regsearch(const char *in, const char *regexprin){
    char *expbuf;

    expbuf = gb_compile_regexpr(regexprin,0);
    if (!expbuf) return 0;
    if (step((char *)in,expbuf)) return loc1;
    return 0;
}

char *GBS_regreplace(const char *in, const char *regexprin, GBDATA *gb_species){
    char *expbuf = 0;
    char *subs;
    void *out;
    const char *loc;
    GBUSE(gb_species);
    expbuf = gb_compile_regexpr(regexprin,&subs);
    if (!expbuf) return 0;

    loc = in;
    out = GBS_stropen(10000);
    while ( step((char *)loc,expbuf)){      /* found a match */
        char *s;
        int c;
        GBS_strncat(out,loc,loc1-loc);  /* prefix */
        s = subs;
        while (c = *(s++)){
            if (c== '\\'){
                c = *(s++);
                if (!c) { s--; break; }
                switch (c){
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    default: break;
                }
            }else if (c=='$'){
                c = *(s++);
                if (!c) { s--; break; }
                if (c>='0' && c <'0' + nbra){
                    c -= '0';
                    GBS_strncat(out,braslist[c], braelist[c] - braslist[c]);
                    continue;
                }

            }
            GBS_chrcat(out,c);
        }
        loc = loc2;
    }
    GBS_strcat(out,loc);        /* copy rest */
    return GBS_strclose(out);
}


#endif

char *GBS_regmatch(const char *in, const char *regexprin) {
    /* returns the first match */
    GB_CPNTR found = GBS_regsearch(in, regexprin);
    if (found) {
        int   length   = loc2-loc1;
        char *result   = (char*)malloc(length+1);
        memcpy(result, loc1, length);
        result[length] = 0;

        return result;
    }
    return 0;
}

GB_ERROR g_bs_add_value_tag_to_hash(GBDATA *gb_main, GB_HASH *hash, char *tag, char *value,const char *rtag, const char *srt, const char *aci, GBDATA *gbd){
    char    *p;
    GB_HASH *sh;
    char    *to_free = 0;
    if (rtag && GBS_string_cmp(tag,rtag,0) == 0){
        if (srt) {
            value = to_free = GBS_string_eval(value,srt,gbd);
        }else if (aci){
            value = to_free = GB_command_interpreter(gb_main,value,aci,gbd, 0);
        }
        if (!value) return GB_get_error();
    }

    p=value; while ( (p = strchr(p,'[')) ) *p =  '{'; /* replace all '[' by '{' */
    p=value; while ( (p = strchr(p,']')) ) *p =  '}'; /* replace all ']' by '}' */

    sh = (GB_HASH *)GBS_read_hash(hash,value);
    if (!sh){
        sh = GBS_create_hash(10,1); /* Tags are case independent */
        GBS_write_hash(hash,value,(long)sh);
    }

    GBS_write_hash(sh,tag,1);
    if (to_free) free(to_free);
    return 0;
}


GB_ERROR g_bs_convert_string_to_tagged_hash(GB_HASH *hash, char *s,char *default_tag,const char *del,
                                            GBDATA *gb_main, const char *rtag,const char *srt, const char *aci, GBDATA *gbd){
    char *se;           /* string end */
    char *sa;           /* string anfang and tag end */
    char *ts;           /* tag start */
    char *t;
    GB_ERROR error = 0;
    while (s){
        if (strlen(s)<=0) break;
        ts = strchr(s,'[');
        if (!ts){
            error = g_bs_add_value_tag_to_hash(gb_main,hash,default_tag,s,rtag,srt,aci,gbd); /* no tag found, use default tag */
            if (error) return error;
            break;
        }else{
            *(ts++) = 0;
        }
        sa = strchr(ts,']');
        if (sa){
            *sa++ = 0;
            while (*sa == ' ') sa++;
        }else{
            error = g_bs_add_value_tag_to_hash(gb_main,hash,default_tag,s,rtag,srt,aci,gbd); /* no tag found, use default tag */
            if (error) return error;
            break;
        }
        se = strchr(sa,'[');
        if (se) {
            while (se>sa && se[-1] == ' ') se--;
            *(se++) = 0;
        }
        for (t = strtok(ts,","); t; t = strtok(0,",")){
            if (del && !GBS_string_cmp(t,del,0)) continue; /* test, whether to delete */
            if (strlen(sa) == 0) continue;
            error = g_bs_add_value_tag_to_hash(gb_main,hash,t,sa,rtag,srt,aci,gbd); /* tag found, use  tag */
            if (error) return error;
        }
        s = se;
    }
    return error;
}

void *g_bs_merge_result;
void *g_bs_merge_sub_result;
GB_HASH *g_bs_collect_tags_hash;

#ifdef __cplusplus
extern "C" {
#endif

    static long g_bs_merge_tags(const char *tag, long val){
        GBS_strcat(   g_bs_merge_sub_result, tag    );
        GBS_strcat(   g_bs_merge_sub_result, ","    );
        return val;
    }

    static long g_bs_sort_fields_of_hash(const char *key0,long val0,const char *key1,long val1){
        GBUSE(val0);GBUSE(val1);
        return strcmp(key0,key1);
    }

    static long g_bs_read_tagged_hash(const char *value, long subhash){
        char *str;
        static int counter = 0;
        g_bs_merge_sub_result = GBS_stropen(100);
        GBS_hash_do_sorted_loop((GB_HASH *)subhash,g_bs_merge_tags,g_bs_sort_fields_of_hash);
        GBS_free_hash((GB_HASH *)subhash); /* array of tags */
        GBS_intcat(g_bs_merge_sub_result,counter++); /* create a unique number */

        str = GBS_strclose(g_bs_merge_sub_result);
        GBS_write_hash(g_bs_collect_tags_hash,str,(long)strdup(value)); /* send output to new hash for sorting */

        free(str);
        return 0;
    }

    static long g_bs_read_final_hash(const char *tag, long value){
        char *lk = strrchr(tag,',');
        if (lk) {           /* remove number at end */
            *lk = 0;
            GBS_strcat(g_bs_merge_result," [");
            GBS_strcat(g_bs_merge_result,tag);
            GBS_strcat(g_bs_merge_result,"] ");
        }
        GBS_strcat(g_bs_merge_result,(char *)value);
        return value;
    }

#ifdef __cplusplus
}
#endif


static char *g_bs_get_string_of_tag_hash(GB_HASH *tag_hash){
    g_bs_merge_result = GBS_stropen(256);
    g_bs_collect_tags_hash = GBS_create_hash(1024,1);

    GBS_hash_do_sorted_loop(tag_hash,g_bs_read_tagged_hash,g_bs_sort_fields_of_hash); /* move everything into g_bs_collect_tags_hash */
    GBS_hash_do_sorted_loop(g_bs_collect_tags_hash,g_bs_read_final_hash,g_bs_sort_fields_of_hash);

    GBS_free_hash_free_pointer(g_bs_collect_tags_hash);
    return GBS_strclose(g_bs_merge_result);
}

/* Create a tagged string from two tagged strings:
                 * a tagged string is '[tag,tag,tag] string'
                 * if s2 than delete all tags replace1 in string1
                 * if s1 than delete all tags replace2 in string2 */

char *GBS_merge_tagged_strings(const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2){
    char *str1 = GB_STRDUP(s1);
    char *str2 = GB_STRDUP(s2);
    char *t1 = GBS_string_2_key(tag1);
    char *t2 = GBS_string_2_key(tag2);
    char *result = 0;
    GB_ERROR error = 0;
    GB_HASH *hash = GBS_create_hash(16,0);
    if (!strlen(s1)) replace2 = 0;
    if (!strlen(s2)) replace1 = 0;
    if (replace1 && replace1[0] == 0) replace1 = 0;
    if (replace2 && replace2[0] == 0) replace2 = 0;
    error = g_bs_convert_string_to_tagged_hash(hash,str1,t1,replace1,0,0,0,0,0);
    if (!error) error = g_bs_convert_string_to_tagged_hash(hash,str2,t2,replace2,0,0,0,0,0);
    if (!error){
        result = g_bs_get_string_of_tag_hash(hash);
    }
    GBS_free_hash(hash);
    free(t2);
    free(t1);
    free(str1);
    free(str2);
    return result;
}

char *GBS_string_eval_tagged_string(GBDATA *gb_main,const char *s, const char *dt, const char *tag, const char *srt, const char *aci, GBDATA *gbd){
    char *str = GB_STRDUP(s);
    char *default_tag = GBS_string_2_key(dt);
    GB_HASH *hash = GBS_create_hash(16,0);
    GB_ERROR error = 0;
    char *result = 0;
    error = g_bs_convert_string_to_tagged_hash(hash,str,default_tag,0,gb_main,tag,srt,aci,gbd);

    if (!error){
        result = g_bs_get_string_of_tag_hash(hash);
    }
    GBS_free_hash(hash);
    free(default_tag);
    free(str);
    return result;
}


char *GB_read_as_tagged_string(GBDATA *gbd, const char *tagi){
    char *s;
    char *tag;
    char *buf;
    char *se;           /* string end */
    char *sa;           /* string anfang and tag end */
    char *ts;           /* tag start */
    char *t;

    buf = s = GB_read_as_string(gbd);
    if (!s) return s;
    if (!tagi) return s;
    if (!strlen(tagi)) return s;

    tag = GBS_string_2_key(tagi);

    while(s){
        ts = strchr(s,'[');
        if (!ts)    goto notfound;      /* no tag */

        *(ts++) = 0;

        sa = strchr(ts,']');
        if (!sa) goto notfound;

        *sa++ = 0;
        while (*sa == ' ') sa++;

        se = strchr(sa,'[');
        if (se) {
            while (se>sa && se[-1] == ' ') se--;
            *(se++) = 0;
        }
        for (t = strtok(ts,","); t; t = strtok(0,",")){
            if (!GBS_string_cmp(t,tag,0)) {
                s = strdup(sa);
                free(buf);
                goto found;
            }
        }
        s = se;
    }
 notfound:
    /* Nothing found */
    free(buf); s = 0;
 found:
    free(tag);
    return s;
}


GBDATA_SET *GB_create_set(int size){
    GBDATA_SET *set = (GBDATA_SET *)GB_calloc(sizeof(GBDATA_SET),1);
    set->nitems = 0;
    set->malloced_items = size;
    set->items = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)set->malloced_items);
    return set;
}

void GB_add_set(GBDATA_SET *set, GBDATA *item){
    if (set->nitems >= set->malloced_items){
        set->malloced_items *= 2;
        set->items = (GBDATA **)realloc((char *)set->items,(size_t)(sizeof(void *) * set->malloced_items));
    }
    set->items[set->nitems++] = item;
}

void GB_delete_set(GBDATA_SET *set){
    GB_FREE(set->items);
    GB_FREE(set);
}

/* be CAREFUL : this function is used to save ARB ASCII database (i.e. properties)
 * used as well to save perl macros
 *
 * when changing GBS_fwrite_string -> GBS_fread_string needs to be fixed as well
 *
 * always keep in mind, that many users have databases/macros written with older
 * versions of this function. They MUST load proper!!!
 */
GB_ERROR GBS_fwrite_string(const char *strngi,FILE *out){
    register unsigned char *strng = (unsigned char *)strngi;
    register int c;
    putc('"',out);

    while ( (c= *strng++) ) {
        if (c < 32) {
            putc('\\',out);
            if (c == '\n')
                putc('n',out);
            else if (c == '\t')
                putc('t',out);
            else if ( c<25 ) {
                putc(c+'@',out); /* characters ASCII 0..24 encoded as \@..\X    (\n and \t are done above) */
            }else{
                putc(c+('0'-25),out);/* characters ASCII 25..31 encoded as \0..\6 */
            }
        }else if (c == '"'){
            putc('\\',out);
            putc('"',out);
        }else if (c == '\\'){
            putc('\\',out);
            putc('\\',out);
        }else{
            putc(c,out);
        }
    }
    putc('"',out);
    return 0;
}

/*  Read a string from a file written by GBS_fwrite_string,
 *  Searches first '"'
 *
 *  WARNING : changing this function affects perl-macro execution (read warnings for GBS_fwrite_string)
 *  any changes should be done in GBS_fconvert_string too.
 */

char *GBS_fread_string(FILE *in) {
    void         *strstr = GBS_stropen(1024);
    register int  x;

    while ((x = getc(in)) != '"' ) if (x == EOF) break; /* Search first '"' */

    if (x != EOF) {
        while ((x = getc(in)) != '"' ){
            if (x == EOF) break;
            if (x == '\\'){
                x = getc(in); if (x==EOF) break;
                if (x == 'n') {
                    GBS_chrcat(strstr,'\n');
                    continue;
                }
                if (x == 't') {
                    GBS_chrcat(strstr,'\t');
                    continue;
                }
                if (x>='@' && x <='@'+ 25) {
                    GBS_chrcat(strstr,x-'@');
                    continue;
                }
                if (x>='0' && x <='9') {
                    GBS_chrcat(strstr,x-('0'-25));
                    continue;
                }
                /* all other backslashes are simply skipped */
            }
            GBS_chrcat(strstr,x);
        }
    }
    return GBS_strclose(strstr);
}

/* does similiar decoding as GBS_fread_string but works directly on an existing buffer
 * (WARNING : GBS_fconvert_string is used by gb_read_file which reads ARB ASCII databases!!)
 *
 * inserts \0 behind decoded string (removes the closing '"')
 * returns a pointer behind the end (") of the _encoded_ string
 * returns NULL if a 0-character is found
 */
char *GBS_fconvert_string(char *buffer) {
    char         *t = buffer;
    char         *f = buffer;
    register int  x;

    ad_assert(f[-1] == '"');
    /* the opening " has already been read */

    while ((x = *f++) != '"') {
        if (!x) break;

        if (x == '\\') {
            x = *f++;
            if (!x) break;

            if (x == 'n') {
                *t++ = '\n';
                continue;
            }
            if (x == 't') {
                *t++ = '\t';
                continue;
            }
            if (x>='@' && x <='@'+ 25) {
                *t++ = x-'@';
                continue;
            }
            if (x>='0' && x <='9') {
                *t++ = x-('0'-25);
                continue;
            }
            /* all other backslashes are simply skipped */
        }
        *t++ = x;
    }

    if (!x) return 0;           // error (string should not contain 0-character)
    ad_assert(x == '"');

    t[0] = 0;
    return f;
}

char *GBS_replace_tabs_by_spaces(const char *text){
    int tlen = strlen(text);
    void *mfile = GBS_stropen(tlen * 3/2);
    int tabpos = 0;
    int c;
    while ((c=*(text++))) {
        if (c == '\t'){
            int ntab = (tabpos + 8) & 0xfffff8;
            while(tabpos < ntab){
                GBS_chrcat(mfile,' ');
                tabpos++;
            }
            continue;
        }
        tabpos ++;
        if (c== '\n'){
            tabpos = 0;
        }
        GBS_chrcat(mfile,c);
    }
    return GBS_strclose(mfile);
}

int GBS_strscmp(const char *s1, const char *s2) {
    int    cmp = 0;
    size_t idx = 0;
    while (!cmp) {
        if (!s1[idx] || !s2[idx]) break;
        cmp = s1[idx] - s2[idx];
        ++idx;
    }
    return cmp;
}
