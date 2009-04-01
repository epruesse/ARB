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
#include <execinfo.h>
#include <signal.h>

#ifdef NO_REGEXPR
int regerrno;
#   define INIT      char *sp = instring;
#   define GETC()    (*sp++)
#   define PEEKC()   (*sp)
#   define UNGETC(c) (--sp)
#   define RETURN(c) return c;
#   define ERROR(c)  gbs_regerror(c)
#   include "adregexp.h"
#else
#   include <regexpr.h>
#endif


/********************************************************************************************
                    directory handling
********************************************************************************************/

GB_ERROR gb_scan_directory(char *basename, struct gb_scandir *sd) { /* goes to header: __ATTR__USERESULT */
    /* look for quick saves (basename = yyy/xxx no arb ending !!!!) */
    char *path = strdup(basename);
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

       'mask' may contain wildcards (*?) or
       it may be a regular expression ('/regexp/')
    */

    DIR           *dirp;
    struct dirent *dp;
    struct stat    st;
    char          *result = 0;
    char           buffer[GB_PATH_MAX];

    dirp = opendir(dir);
    if (dirp) {
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
            if (GBS_string_matches_regexp(dp->d_name,mask,0)) {
                sprintf(buffer,"%s/%s",dir,dp->d_name);
                if (stat(buffer,&st) == 0  && S_ISREG(st.st_mode)) { // regular file ?
                    if (filename_only) strcpy(buffer, dp->d_name);
                    if (result) {
                        freeset(result, GBS_global_string_copy("%s*%s", result, buffer));
                    }
                    else {
                        result = strdup(buffer);
                    }
                }
            }
        }
        closedir(dirp);
    }

    return result;
}

char *GB_find_latest_file(const char *dir,const char *mask) {
    /* 'mask' may contain wildcards (*?) or it may be a regular expression ('/regexp/') */
    
    DIR           *dirp;
    struct dirent *dp;
    char           buffer[GB_PATH_MAX];
    struct stat    st;
    GB_ULONG       newest = 0;
    char          *result = 0;

    dirp = opendir(dir);
    if (dirp) {
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)){
            if (GBS_string_matches_regexp(dp->d_name,mask,0)){
                sprintf(buffer,"%s/%s",dir,dp->d_name);
                if (stat(buffer,&st) == 0){
                    if ((GB_ULONG)st.st_mtime > newest){
                        newest = st.st_mtime;
                        freedup(result, dp->d_name);
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

#if defined(DEVEL_RALF)
#warning redesign GB_export_error et al
/* To clearly distinguish between the two ways of error handling
 * (which are: return GB_ERROR
 *  and:       export the error)
 * 
 * GB_export_error() shall only export, not return the error message.
 * if only used for formatting GBS_global_string shall be used.
 *
 * GB_export_IO_error() shall not export and be renamed into GB_IO_error()
 *
 * GB_export_error() shall fail if there is already an exported error
 * (maybe always remember a stack trace of last error export (try whether copy of backtrace-array works))
 *
 * use GB_get_error() to import AND clear the error
 */
#endif /* DEVEL_RALF */

static char *GB_error_buffer = 0;

GB_ERROR GB_export_error(const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(1) */

    char     buffer[GBS_GLOBAL_STRING_SIZE];
    char    *p  = buffer;
    va_list  parg;
    memset(buffer,0,1000);
    sprintf (buffer,"ARB ERROR: ");
    p          += strlen(p);
    va_start(parg,templat);

    vsprintf(p,templat,parg);

    freedup(GB_error_buffer, buffer);
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
            if (filename) sprintf(buffer, "ARB ERROR: While %s '%s': %s", action, filename, error_message);
            else sprintf(buffer, "ARB ERROR: While %s <unknown file>: %s", action, error_message);
        }
        else {
            if (filename) sprintf(buffer, "ARB ERROR: Concerning '%s': %s", filename, error_message);
            else sprintf(buffer, "ARB ERROR: %s", error_message);
        }

        freedup(GB_error_buffer, buffer);
    }
    return GB_error_buffer;
}

GB_ERROR GB_print_error() {
    if (GB_error_buffer){
        fflush(stdout);
        fprintf(stderr,"%s\n",GB_error_buffer);
    }
    return GB_error_buffer;
}

GB_ERROR GB_get_error() {
    return GB_error_buffer;
}

GB_ERROR GB_expect_error() {
    return GB_error_buffer
        ? GB_error_buffer
        : "Expected error, but no error message found (program logic error)";
}

void GB_clear_error() {         /* clears the error buffer */
    freeset(GB_error_buffer, 0);
}

#if defined(DEVEL_RALF)
#warning search for 'GBS_global_string.*error' and replace with GB_failedTo_error
#endif /* DEVEL_RALF */
GB_ERROR GB_failedTo_error(const char *do_something, const char *special, GB_ERROR error) {
    if (error) {
        if (special) {
            error = GBS_global_string("Failed to %s '%s'.\n(Reason: %s)", do_something, special, error);
        }
        else {
            error = GBS_global_string("Failed to %s.\n(Reason: %s)", do_something, error);
        }
    }
    return error;
}

/* -------------------------------------------------------------------------------- */

#ifdef LINUX
# define HAVE_VSNPRINTF
#endif

#ifdef HAVE_VSNPRINTF
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsnprintf(buffer, bufsize, templat, parg);
#else
# define PRINT2BUFFER(buffer, bufsize, templat, parg) vsprintf(buffer, templat, parg);
#endif

#define PRINT2BUFFER_CHECKED(printed, buffer, bufsize, templat, parg)   \
    (printed) = PRINT2BUFFER(buffer, bufsize, templat, parg);           \
    if ((printed) < 0 || (size_t)(printed) >= (bufsize)) {              \
        GBK_terminate("Internal buffer overflow (size=%zu, used=%zi)\n", \
                     (bufsize), (printed));                             \
    }
    
/* -------------------------------------------------------------------------------- */

#if defined(DEBUG)
#if defined(DEVEL_RALF)
/* #define TRACE_BUFFER_USAGE */
#endif /* DEBUG */
#endif /* DEVEL_RALF */

#define GLOBAL_STRING_BUFFERS 4

static size_t last_global_string_size = 0;

static GB_CSTR gbs_vglobal_string(const char *templat, va_list parg, int allow_reuse)
{
    static char buffer[GLOBAL_STRING_BUFFERS][GBS_GLOBAL_STRING_SIZE+2]; // serveral buffers - used alternately
    static int  idx                             = 0;
    static char lifetime[GLOBAL_STRING_BUFFERS] = { };
    static char nextIdx[GLOBAL_STRING_BUFFERS] = { };

    int my_idx;
    int psize;

    if (nextIdx[0] == 0) { // initialize nextIdx
        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            nextIdx[my_idx] = (my_idx+1)%GLOBAL_STRING_BUFFERS;
        }
    }

    if (allow_reuse == -1) { /* called from GBS_reuse_buffer */
        /* buffer to reuse is passed in 'templat' */

        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            if (buffer[my_idx] == templat) {
                lifetime[my_idx] = 0;
#if defined(TRACE_BUFFER_USAGE)
                printf("Reusing buffer #%i\n", my_idx);
#endif /* TRACE_BUFFER_USAGE */
                if (nextIdx[my_idx] == idx) idx = my_idx;
                return 0;
            }
#if defined(TRACE_BUFFER_USAGE)
            else {
                printf("(buffer to reuse is not buffer #%i (%p))\n", my_idx, buffer[my_idx]);
            }
#endif /* TRACE_BUFFER_USAGE */
        }
        for (my_idx = 0; my_idx<GLOBAL_STRING_BUFFERS; my_idx++) {
            printf("buffer[%i]=%p\n", my_idx, buffer[my_idx]);
        }
        ad_assert(0);       /* GBS_reuse_buffer called with illegal buffer */
        return 0;
    }

    if (lifetime[idx] == 0) {
        my_idx = idx;
    }
    else {
        for (my_idx = nextIdx[idx]; lifetime[my_idx]>0; my_idx = nextIdx[my_idx]) {
#if defined(TRACE_BUFFER_USAGE)
            printf("decreasing lifetime[%i] (%i->%i)\n", my_idx, lifetime[my_idx], lifetime[my_idx]-1);
#endif /* TRACE_BUFFER_USAGE */
            lifetime[my_idx]--;
        }
    }

    PRINT2BUFFER_CHECKED(psize, buffer[my_idx], GBS_GLOBAL_STRING_SIZE, templat, parg);

#if defined(TRACE_BUFFER_USAGE)
    printf("Printed into global buffer #%i ('%s')\n", my_idx, buffer[my_idx]);
#endif /* TRACE_BUFFER_USAGE */

    last_global_string_size = psize;

    if (!allow_reuse) {
        idx           = my_idx;
        lifetime[idx] = 1;
    }
#if defined(TRACE_BUFFER_USAGE)
    else {
        printf("Allow reuse of buffer #%i\n", my_idx);
    }
#endif /* TRACE_BUFFER_USAGE */

    return buffer[my_idx];
}

static char *gbs_vglobal_string_copy(const char *templat, va_list parg) {
    GB_CSTR gstr = gbs_vglobal_string(templat, parg, 1);
    return GB_strduplen(gstr, last_global_string_size);
}

void GBS_reuse_buffer(GB_CSTR global_buffer) {
    /* If you've just shortely used a buffer, you can put it back here */
    gbs_vglobal_string(global_buffer, 0, -1);
}

GB_CSTR GBS_global_string(const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(1)  */

    va_list parg;
    GB_CSTR result;

    va_start(parg,templat);
    result = gbs_vglobal_string(templat, parg, 0);
    va_end(parg);

    return result;
}

char *GBS_global_string_copy(const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(1)  */

    va_list parg;
    char *result;

    va_start(parg,templat);
    result = gbs_vglobal_string_copy(templat, parg);
    va_end(parg);

    return result;
}

#if defined(DEVEL_RALF)
#warning search for '\b(sprintf)\b\s*\(' and replace by GBS_global_string_to_buffer
#endif /* DEVEL_RALF */

const char *GBS_global_string_to_buffer(char *buffer, size_t bufsize, const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(3)  */

    va_list parg;
    int     psize;

    gb_assert(buffer);
    va_start(parg,templat);
    PRINT2BUFFER_CHECKED(psize, buffer, bufsize, templat, parg);
    va_end(parg);

    return buffer;
}

size_t GBS_last_global_string_size() {
    return last_global_string_size;
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
    return strdup(buf);
}

char *GBS_string_2_key(const char *str) /* converts any string to a valid key */
{
    return GBS_string_2_key_with_exclusions(str, "");
}

void gbs_uppercase(char *str)
{
    char c;
    while( (c=*str) )   {
        if ( (c<='z') && (c>='a')) *str = c - 'a' + 'A';
        str++;
    }
}

void gbs_memcopy(char *dest, const char *source, long len)
{
    long        i;
    const char *s;
    char       *d;

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
    memcpy(dest,source,(int)len);
    return dest;
}

GB_ERROR GB_check_key(const char *key) { /* goes to header: __ATTR__USERESULT */
    /* test whether all characters are letters, numbers or _ */
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
GB_ERROR GB_check_link_name(const char *key) { /* goes to header: __ATTR__USERESULT */
    /* test whether all characters are letters, numbers or _ */
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
GB_ERROR GB_check_hkey(const char *key) { /* goes to header: __ATTR__USERESULT */
    /* test whether all characters are letters, numbers or _ */
    /* additionally allow '/' and '->' for hierarchical keys */
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

#define UPPERCASE(c) if ( (c>='a') && (c<='z')) c+= 'A'-'a'

/* search a substring in another string
    match_mode == 0     -> exact match
    match_mode == 1     -> a==A 
    match_mode == 2     -> a==a && a==?
    match_mode == else  -> a==A && a==?
*/

GB_CSTR GBS_find_string(GB_CSTR str, GB_CSTR substr, int match_mode) {
    const char *p1, *p2;
    char        b;
    
    switch (match_mode) {

        case 0: /* exact match */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2)   return (char *)str;
            break;

        case 1: /* a==A */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b)) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
        case 2: /* a==a && a==? */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (b == *p1 || (b=='?')) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;

        default: /* a==A && a==? */
            for (p1 = str, p2 = substr; *p1;) {
                if (!(b = *p2)) {
                    return (char *)str;
                } else {
                    if (toupper(*p1) == toupper(b) || (b=='?') ) {
                        p1++;
                        p2++;
                    } else {
                        p2 = substr;
                        p1 = (++str);
                    }
                }
            }
            if (!*p2) return (char *)str;
            break;
    }
    return 0;
}

GB_BOOL GBS_string_matches_regexp(const char *str,const char *search,GB_CASE case_sens) {
    /* same as GBS_string_matches except '/regexpr/' matches regexpr  */
    if (search[0] == '/') {
        if (search[strlen(search)-1] == '/'){
#if defined(DEVEL_RALF)
#warning check case-handling in GBS_regsearch
#endif /* DEVEL_RALF */
            return GBS_regsearch(str,search) ? GB_TRUE : GB_FALSE;
        }
    }
    return GBS_string_matches(str,search,case_sens);
}

GB_BOOL GBS_string_matches(const char *str, const char *search, GB_CASE case_sens)
/* Wildcards in 'search' string:
 *      ?   one character
 *      *   serveral characters
 *
 * if 'case_sens' == GB_IGNORE_CASE -> change all letters to uppercase
 * 
 * returns GB_TRUE if strings are equal, GB_FALSE otherwise
 */
{
    const char *p1,*p2;
    char        a,b,*d;
    long        i;
    char        fsbuf[256];

    p1 = str;
    p2 = search;
    while (1) {
        a = *p1;
        b = *p2;
        if (b == '*')   {
            if (!p2[1]) break; /* '*' also matches nothing */
            i = 0;
            d = fsbuf;
            for (p2++; (b=*p2)&&(b!='*'); ) {
                *(d++) = b;
                p2++;
                i++;
                if (i > 250) break;
            }
            if (*p2 != '*' ) {
                p1 += strlen(p1)-i;     /* check the end of the string */
                if (p1 < str) return GB_FALSE;
                p2 -= i;
            }
            else {
                *d  = 0;
                p1  = GBS_find_string(p1,fsbuf,2+(case_sens == GB_IGNORE_CASE)); // match with '?' wildcard
                if (!p1) return GB_FALSE;
                p1 += i;
            }
            continue;
        }

        if (!a) return b ? GB_FALSE : GB_TRUE;
        if (a != b) {
            if (b != '?') {
                if (!b) return a ? GB_FALSE : GB_TRUE;
                if (case_sens == GB_IGNORE_CASE) {
                    a = toupper(a);
                    b = toupper(b);
                    if (a != b) return GB_FALSE;
                }
                else {
                    return GB_FALSE;
                }
            }
        }
        p1++;
        p2++;
    }
    return GB_TRUE;
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
    char *result,*s,*d;
    int   ch;

    s = d = result = strdup(com);
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
    char *result,*s,*d;
    int   ch;

    s = d = result = strdup(com);
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

#if defined(DEBUG)
// # define DUMP_STRSTRUCT_MEMUSE
#endif /* DEBUG */


struct GBS_strstruct {
    char *GBS_strcat_data;
    long  GBS_strcat_data_size;
    long  GBS_strcat_pos;
};

static struct GBS_strstruct *last_used = 0;

struct GBS_strstruct *GBS_stropen(long init_size)   { /* opens a memory file */
    struct GBS_strstruct *strstr;

    if (last_used && last_used->GBS_strcat_data_size >= init_size) {
        strstr    = last_used;
        last_used = 0;
    }
    else {
#if defined(DUMP_STRSTRUCT_MEMUSE)
        printf("allocating new GBS_strstruct (size = %li)\n", init_size);
#endif /* DUMP_STRSTRUCT_MEMUSE */
        strstr                       = (struct GBS_strstruct *)malloc(sizeof(struct GBS_strstruct));
        strstr->GBS_strcat_data_size = init_size;
        strstr->GBS_strcat_data      = (char *)malloc((size_t)strstr->GBS_strcat_data_size);
    }

    strstr->GBS_strcat_pos     = 0;
    strstr->GBS_strcat_data[0] = 0;

    return strstr;
}

char *GBS_strclose(struct GBS_strstruct *strstr) {
    /* returns a char* copy of the memory file */
    
    long  length = strstr->GBS_strcat_pos;
    char *str    = (char*)malloc(length+1);

    gb_assert(str);

    memcpy(str, strstr->GBS_strcat_data, length+1); /* copy with 0 */
    GBS_strforget(strstr);

    return str;
}

void GBS_strforget(struct GBS_strstruct *strstr) {
    if (last_used) {
        if (last_used->GBS_strcat_data_size < strstr->GBS_strcat_data_size) { /* last_used is smaller -> keep this */
            struct GBS_strstruct *tmp = last_used;
            last_used                 = strstr;
            strstr                    = tmp;
        }
    }
    else {
        static short oversized_counter = 0;

        if (strstr->GBS_strcat_pos*10 < strstr->GBS_strcat_data_size) oversized_counter++;
        else oversized_counter = 0;

        if (oversized_counter<10) {
            // keep strstruct for next call
            last_used = strstr;
            strstr    = 0;
        }
        // otherwise the current strstruct was oversized 10 times -> free it
    }

    if (strstr) {
#if defined(DUMP_STRSTRUCT_MEMUSE)
        printf("freeing GBS_strstruct (size = %li)\n", strstr->GBS_strcat_data_size);
#endif /* DUMP_STRSTRUCT_MEMUSE */
        free(strstr->GBS_strcat_data);
        free(strstr);
    }
}

GB_BUFFER GBS_mempntr(struct GBS_strstruct *strstr) {
    /* returns the memory file */
    return strstr->GBS_strcat_data;
}

long GBS_memoffset(struct GBS_strstruct *strstr) {
    /* returns the offset into the memory file */
    return strstr->GBS_strcat_pos;
}

void GBS_str_cut_tail(struct GBS_strstruct *strstr, int byte_count){
    /* Removes byte_count characters at the tail of a memfile */
    strstr->GBS_strcat_pos -= byte_count;
    if (strstr->GBS_strcat_pos < 0) strstr->GBS_strcat_pos = 0;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}

static void gbs_strensure_mem(struct GBS_strstruct *strstr,long len) {
    if (strstr->GBS_strcat_pos + len + 2 >= strstr->GBS_strcat_data_size) {
        strstr->GBS_strcat_data_size = (strstr->GBS_strcat_pos+len+2)*3/2;
        strstr->GBS_strcat_data      = (char *)realloc((MALLOC_T)strstr->GBS_strcat_data,(size_t)strstr->GBS_strcat_data_size);
#if defined(DUMP_STRSTRUCT_MEMUSE)
        printf("re-allocated GBS_strstruct to size = %li\n", strstr->GBS_strcat_data_size);
#endif /* DUMP_STRSTRUCT_MEMUSE */
    }
}

void GBS_strcat(struct GBS_strstruct *strstr,const char *ptr) {
    /* append string to strstruct */
    long len = strlen(ptr);

    gbs_strensure_mem(strstr,len);
    memcpy(strstr->GBS_strcat_data+strstr->GBS_strcat_pos,ptr,(int)len);
    strstr->GBS_strcat_pos += len;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos]  = 0;
}

void GBS_strncat(struct GBS_strstruct *strstr,const char *ptr,long len) {
    /* append some bytes string to strstruct
     * (caution : copies zero byte and things behind!)
     */
    gbs_strensure_mem(strstr,len+2);
    memcpy(strstr->GBS_strcat_data+strstr->GBS_strcat_pos,ptr,(int)len);
    strstr->GBS_strcat_pos += len;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}



void GBS_strnprintf(struct GBS_strstruct *strstr, long len, const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(3)  */
    char    *buffer;
    int      psize;
    va_list  parg;
    
    va_start(parg,templat);
    gbs_strensure_mem(strstr,len+2);

    buffer = strstr->GBS_strcat_data+strstr->GBS_strcat_pos;

#ifdef LINUX
    psize = vsnprintf(buffer,len,templat,parg);
#else
    psize = vsprintf(buffer,templat,parg);
#endif

    assert_or_exit(psize >= 0 && psize <= len);
    strstr->GBS_strcat_pos += psize;
}

void GBS_chrcat(struct GBS_strstruct *strstr,char ch) {
    gbs_strensure_mem(strstr, 1);
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos++] = ch;
    strstr->GBS_strcat_data[strstr->GBS_strcat_pos] = 0;
}

void GBS_intcat(struct GBS_strstruct *strstr,long val) {
    char buffer[200];
    long len = sprintf(buffer,"%li",val);
    GBS_strncat(strstr, buffer, len);
}

void GBS_floatcat(struct GBS_strstruct *strstr,double val) {
    char buffer[200];
    long len = sprintf(buffer,"%f",val);
    GBS_strncat(strstr, buffer, len);
}

/********************************************************************************************
                    String Replace
********************************************************************************************/
int GBS_reference_not_found;

ATTRIBUTED(__ATTR__USERESULT,
           static GB_ERROR gbs_build_replace_string(void *strstruct,
                                                    char *bar,char *wildcards, long max_wildcard,
                                                    char **mwildcards, long max_mwildcard, GBDATA *gb_container))
{
    char *p,c,d;
    int   wildcardcnt  = 0;
    int   mwildcardcnt = 0;
    int   wildcard_num;
    char *entry;

    p = bar;

    wildcardcnt  = 0;
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
                            entry = strdup("");
                        }else{
                            entry = GB_read_as_string(gb_entry);
                        }
                        if (entry) {
                            char *h;
                            switch(seperator) {
                                case ':':
                                    h = GBS_string_eval(entry,psym+1,gb_container);
                                    if (h){
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }else{
                                        return GB_get_error();
                                    }
                                    break;
                                case '|':
                                    h = GB_command_interpreter(GB_get_root(gb_container), entry,psym+1,gb_container, 0);
                                    if (h) {
                                        GBS_strcat(strstruct,h);
                                        free(h);
                                    }
                                    else {
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
                    GBS_chrcat(strstruct,c);
                    GBS_chrcat(strstruct,d);
                }
                else {
                    wildcard_num = d - '1';
                    if (c == GBS_WILD) {
                        c = '?';
                        if ( (wildcard_num<0)||(wildcard_num>9) ) {
                            p--;            /* use this character */
                            wildcard_num = wildcardcnt++;
                        }
                        if (wildcard_num>=max_wildcard) {
                            GBS_chrcat(strstruct,c);
                        }
                        else {
                            GBS_chrcat(strstruct,wildcards[wildcard_num]);
                        }
                    }
                    else {
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
                }
                break;
            default:
                GBS_chrcat(strstruct,c);
                break;
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
            *0 to reference to the first occurrence of *
            *1          second
            ...
            *9

        if the last and first characters of the search string are no '*' wildcards then
        the replace is repeated as many times as possible
        '\' is the escape character: e.g. \n is newline; '\\' is '\'; '\=' is '='; ....

        eg:
        print first three characters of first word and the whole second word:

        *(arb_key)          is the value of the a database entry arb key
        *(arb_key#string)   value of the database entry or 'string' if the entry does not exist
        *(arb_key\:SRT)     runs SRT recursively on the value of the database entry
        *([arb_key]|ACI)    runs the ACI command interpreter on the value of the database entry (or on an empty string)

     */

{
    GB_CSTR  source;            /* pointer into the current string when parsed */
    char    *search;            /* pointer into the current command when parsed */
    GB_CSTR  p;                 /* short live pointer */
    char     c;
    GB_CSTR  already_transferred; /* point into 'in' string to non parsed position */

    char      wildcard[40];
    char     *mwildcard[10];
    GB_ERROR  error;

    long i;
    long max_wildcard;
    long max_mwildcard;

    char *start_of_wildcard;
    char  what_wild_card;

    GB_CSTR start_match;

    char *doppelpunkt;

    char *bar;
    char *in;
    char *nextdp;
    void *strstruct;
    char *command;

    if (!icommand || !icommand[0]) return strdup(insource);

    command = gbs_compress_command(icommand);
    in = strdup(insource);               /* copy insource to allow to destroy it */

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
            GB_export_error("SRT ERROR: no '=' found in command '%s' (position > %zi)", icommand, doppelpunkt-command+1);
            free(command);
            free(in);
            return 0;
        }

        already_transferred = in;
        strstruct = GBS_stropen(1000);          /* create output stream */

        if ( (!*in) && doppelpunkt[0] == GBS_MWILD && doppelpunkt[1] == 0) {    /* empty string -> pars myself */
            /* * matches empty string !!!!  */
            mwildcard[max_mwildcard++] = strdup("");
            gbs_build_replace_string(strstruct,bar,wildcard, max_wildcard, mwildcard, max_mwildcard,gb_container);
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
                            mwildcard[max_mwildcard++] = strdup(source);
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
                        mwildcard[max_mwildcard++] = GB_strpartdup(source, p-1);
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

            for (i = 0; i < max_mwildcard; i++) {
                freeset(mwildcard[i], 0);
            }
            max_wildcard  = 0;
            max_mwildcard = 0;

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
            freeset(mwildcard[i], 0);
        }
        max_wildcard  = 0;
        max_mwildcard = 0;

        freeset(in, GBS_strclose(strstruct));
    }
    free(command);
    return in;
}

char *GBS_eval_env(GB_CSTR p){
    GB_ERROR  error = 0;
    GB_CSTR   ka;
    void     *out   = GBS_stropen(1000);

    while ((ka = GBS_find_string(p,"$(",0))) {
        GB_CSTR kz = strchr(ka,')');
        if (!kz) {
            error = GBS_global_string("missing ')' for envvar '%s'", p);
            break;
        }
        else {
            char *envvar = GB_strpartdup(ka+2, kz-1);
            int   len    = ka-p;

            if (len) GBS_strncat(out, p, len);

            GB_CSTR genv = GB_getenv(envvar);
            if (genv) GBS_strcat(out, genv);

            p = kz+1;
            free(envvar);
        }
    }

    if (error) {
        GB_export_error(error);
        free(GBS_strclose(out));
        return 0;
    }

    GBS_strcat(out, p);         // copy rest
    return GBS_strclose(out);
}

char *GBS_find_lib_file(const char *filename, const char *libprefix, int warn_when_not_found) {
    /* Searches files in current dir, $HOME, $ARBHOME/lib/libprefix */

    char *result = 0;

    if (GB_is_readablefile(filename)) {
        result = strdup(filename);
    }
    else {
        const char *slash = strrchr(filename, '/'); // look for last slash

        if (slash && filename[0] != '.') { // have absolute path
            filename = slash+1; // only use filename part
            slash    = 0;
        }

        const char *fileInHome = GB_concat_full_path(GB_getenvHOME(), filename);

        if (fileInHome && GB_is_readablefile(fileInHome)) {
            result = strdup(fileInHome);
        }
        else {
            if (slash) filename = slash+1; // now use filename only, even if path starts with '.'

            const char *fileInLib = GB_path_in_ARBLIB(libprefix, filename);

            if (fileInLib && GB_is_readablefile(fileInLib)) {
                result = strdup(fileInLib);
            }
            else {
                if (warn_when_not_found) {
                    GB_warning("Don't know where to find '%s'\n"
                               "  searched in '.'\n"
                               "  searched in $(HOME) (for '%s')\n"
                               "  searched in $(ARBHOME)/lib/%s (for '%s')\n", 
                               filename, fileInHome, libprefix, fileInLib);
                }
            }
        }
    }

    return result;
}



/* *******************************************************************************************
   some simple find procedures
********************************************************************************************/

char **GBS_read_dir(const char *dir, const char *mask) {
    /* Return names of files in directory 'dir'.
     * Filter through 'mask' (mask == NULL -> return all files).
     *
     * Result is a NULL terminated array of char* (sorted alphanumerically) 
     * Use GBT_free_names() to free the result.
     * 
     * In case of error, result is NULL and error is exported
     *
     * Special case: If 'dir' is the name of a file, return an array with file as only element
     */

    gb_assert(dir);             // dir == NULL was allowed before 12/2008, forbidden now!

    char  *fulldir   = nulldup(GB_get_full_path(dir));
    DIR   *dirstream = opendir(fulldir);
    char **names     = NULL;

    if (!dirstream) {
        if (GB_is_readablefile(fulldir)) {
            names    = malloc(2*sizeof(*names));
            names[0] = strdup(fulldir);
            names[1] = NULL;
        }
        else {
            char *lslash = strrchr(fulldir, '/');

            if (lslash) {
                char *name;
                lslash[0] = 0;
                name      = lslash+1;
                if (GB_is_directory(fulldir)) {
                    names = GBS_read_dir(fulldir, name);
                }
                lslash[0] = '/';
            }

            if (!names) GB_export_error("can't read directory '%s'", fulldir);
        }
    }
    else {
        if (mask == NULL) mask = "*";

        int   allocated = 100;
        int   entries   = 0;
        names           = malloc(100*sizeof(*names));

        struct dirent *entry;
        while ((entry = readdir(dirstream)) != 0) {
            const char *name = entry->d_name;

            if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
                ; // skip '.' and '..'
            }
            else {
                if (GBS_string_matches_regexp(name, mask, GB_MIND_CASE)) {
                    if (entries == allocated) {
                        allocated += allocated>>1; // * 1.5
                        names  = realloc(names, allocated*sizeof(*names));
                    }
                    names[entries++] = strdup(GB_concat_path(fulldir, name));
                }
            }
        }

        names          = realloc(names, (entries+1)*sizeof(*names));
        names[entries] = NULL;

        GB_sort((void**)names, 0, entries, GB_string_comparator, 0);

        closedir(dirstream);
    }

    free(fulldir);

    return names;  
}

long GBS_gcgchecksum( const char *seq )
/* GCGchecksum */
{
    long i;
    long check  = 0;
    long count  = 0;
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
const uint32_t crctab[] = {
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

uint32_t GB_checksum(const char *seq, long length, int ignore_case , const char *exclude) /* RALF: 02-12-96 */
     /*
      * CRC32checksum: modified from CRC-32 algorithm found in ZIP compression source
      * if ignore_case == true -> treat all characters as uppercase-chars (applies to exclude too)
      */
{
    unsigned long c = 0xffffffffL;
    long   n = length;
    int    i;
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

uint32_t GBS_checksum(const char *seq, int ignore_case, const char *exclude)
     /* if 'ignore_case' == true -> treat all characters as uppercase-chars (applies to 'exclude' too) */
{
    return GB_checksum(seq, strlen(seq), ignore_case, exclude);
}

/* extract all words in a text that:
    1. minlen < 1.0  contain more than minlen*len_of_text characters that also exists in chars
    2. minlen > 1.0  contain more than minlen characters that also exists in chars
*/

char *GBS_extract_words( const char *source,const char *chars, float minlen, GB_BOOL sort_output ) {
    char  *s         = strdup(source);
    char **ps        = (char **)GB_calloc(sizeof(char *), (strlen(source)>>1) + 1);
    void  *strstruct = GBS_stropen(1000);
    char  *f         = s;
    int    count     = 0;
    char  *p;
    char  *h;
    int    cnt;
    int    len;
    int    iminlen   = (int) (minlen+.5);
    
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
        GB_sort((void **)ps, 0, count, GB_string_comparator, 0);
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

/* ----------------------- */
/*      Error handler      */

static void gb_error_to_stderr(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static gb_error_handler_type gb_error_handler = gb_error_to_stderr;

NOT4PERL void GB_install_error_handler(gb_error_handler_type aw_message_handler){
    gb_error_handler = aw_message_handler;
}

/* --------------------- */
/*      Backtracing      */

#define MAX_BACKTRACE 66

void GBK_dump_backtrace(FILE *out, GB_ERROR error) {
    void   *array[MAX_BACKTRACE];
    size_t  size = backtrace(array, MAX_BACKTRACE); // get void*'s for all entries on the stack

    if (!out) out = stderr;

    // print out all the frames to out
    fprintf(out, "\n-------------------- ARB-backtrace for '%s':\n", error);
    backtrace_symbols_fd(array, size, fileno(out));
    if (size == MAX_BACKTRACE) fputs("[stack truncated to avoid deadlock]\n", out);
    fputs("-------------------- End of backtrace\n", out);
    fflush(out);
}

/* ----------------------- */
/*      catch SIGSEGV      */

static void sigsegv_handler_exit(int sig) {
    fprintf(stderr, "[Terminating with signal %i]\n", sig);
    exit(EXIT_FAILURE);
}
void sigsegv_handler_dump(int sig) {
    /* sigsegv_handler is intentionally global, to show it in backtrace! */
    GBK_dump_backtrace(stderr, GBS_global_string("received signal %i", sig));
    sigsegv_handler_exit(sig);
}

void GBK_install_SIGSEGV_handler(GB_BOOL install) {
    signal(SIGSEGV, install ? sigsegv_handler_dump : sigsegv_handler_exit);
}

/* ------------------------------------------- */
/*      Error/notification functions           */

void GB_internal_error(const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(1)  */
    va_list parg;

    /* Use GB_internal_error, when something goes badly wrong
     * but you want to give the user a chance to save his database
     * 
     * Note: it's NOT recommended to use this function!
     */

    va_start(parg, templat);
    GB_CSTR message = gbs_vglobal_string(templat, parg, 0);
    va_end(parg);

    char *full_message = GBS_global_string_copy("Internal ARB Error: %s", message);
    gb_error_handler(full_message);
    gb_error_handler("ARB is most likely unstable now (due to this error).\n"
                     "If you've made changes to the database, consider to save it using a different name.\n"
                     "Try to fix the cause of the error and restart ARB.");

#ifdef ASSERTION_USED
    fprintf(stderr, full_message);
    gb_assert(0);                             // internal errors shall not happen, go fix it
#else
    GBK_dump_backtrace(stderr, full_message);
#endif

    free(full_message);
}

void GBK_terminate(const char *templat, ...) {
    /* goes to header: __ATTR__FORMAT(1)  */

    /* GBK_terminate is the emergency exit!
     * only used if no other way to recover
     */
    
    va_list parg;

    va_start(parg,templat);
    GB_CSTR error = gbs_vglobal_string(templat, parg, 0);
    va_end(parg);

    fprintf(stderr, "Error: '%s'\n", error);
    fputs("Can't continue - terminating..\n", stderr);
    GBK_dump_backtrace(stderr, "GBK_terminate");

    fflush(stderr);
    ARB_SIGSEGV(0); // GBK_terminate shall not be called, fix reason (crash in DEBUG and NDEBUG version)
    exit(EXIT_FAILURE);
}

GB_ERROR GBK_assert_msg(const char *assertion, const char *file, int linenr) {
    return GBS_global_string("assertion '%s' failed in %s #%i", assertion, file, linenr);
}

void GB_warning(const char *templat, ...) { 
    /* goes to header: __ATTR__FORMAT(1)  */

    /* If program uses GUI, the message is printed via aw_message, otherwise it goes to stdout
     * see also : GB_information
     */

    va_list parg;

    va_start(parg,templat);
    char *message = gbs_vglobal_string_copy(templat, parg);
    va_end(parg);

    if (gb_warning_func) {
        gb_warning_func(message);
    }
    else {
        fputs(message, stdout);
        fputc('\n', stdout);
    }
    free(message);
}

NOT4PERL void GB_install_warning(gb_warning_func_type warn){
    gb_warning_func = warn;
}

void GB_information( const char *templat, ...) {    /* max 4000 characters */
    /* goes to header: __ATTR__FORMAT(1)  */

    /* this message is always printed to stdout (regardless whether program uses GUI or not)
     * see also : GB_warning
     */

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

    /* if program uses GUI this uses aw_status(),
     * otherwise it uses simple status to stdout
     *
     * return value : 0 = ok, 1 = userAbort
     */

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
    /* goes to header: __ATTR__FORMAT(1)  */
    
    /* return value : 0 = ok, 1 = userAbort */

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


GB_CSTR GBS_regsearch(GB_CSTR in, const char *regexprin){
    /* search the beginning first match
     * returns the position or NULL
     */
    static char  expbuf[8000];
    static char *regexpr = 0;
    char        *res;
    int          rl      = strlen(regexprin)-2;
    
    if (regexprin[0] != '/' || regexprin[rl+1] != '/') {
        GB_export_error("RegExprSyntax: '/searchterm/'");
        GB_print_error();
        return 0;
    }

    if (!regexpr || strncmp(regexpr, regexprin+1, rl) != 0) { // first or new regexpr
        freedup(regexpr, regexprin+1);
        regexpr[rl] = 0;
        regerrno    = 0;
        
        res = compile(regexpr,&expbuf[0],&expbuf[8000],0);
        if (!res|| regerrno){
            gbs_regerror(regerrno);
            return 0;
        }
    }

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
    regexpr = strdup(regexprin+1);
    regexpr[rl] = 0;

    /* Search seperating '/' */
    subs                                                    = strrchr(regexpr,'/');
    while (subs && subs > regexpr && subs[-1] == '\\') subs = strrchr(subs,'/');
    if (!subs || subs == regexpr){
        free(regexpr);

        /* dont change this error message (or change it in adquery/GB_command_interpreter too) : */
        GB_export_error("no '/' found in regexpr");
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

#error sorry.. got no system with regexp.h to fix code below -- ralf May 2008

/** regexpr = '/regexpr/' */
static GB_CSTR gb_compile_regexpr(GB_CSTR regexprin,char **subsout){
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
    if (!old_reg_expr || strcmp(old_reg_expr ,regexpr) != 0) {
        freeset(expbuf, compile(regexpr,0,0));
    }

    reassign(old_reg_expr, regexpr);

    if (regerrno){
        gbs_regerror(regerrno);
        return 0;
    }
    return expbuf;
}

GB_CSTR GBS_regsearch(GB_CSTR in, GB_CSTR regexprin){
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
    GB_CSTR found = GBS_regsearch(in, regexprin);
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
    if (rtag && strcmp(tag,rtag) == 0){ 
        if (srt) {
            value = to_free = GBS_string_eval(value,srt,gbd);
        }
        else if (aci) {
            value = to_free = GB_command_interpreter(gb_main,value,aci,gbd, 0);
        }
        if (!value) return GB_get_error();
    }

    p=value; while ( (p = strchr(p,'[')) ) *p =  '{'; /* replace all '[' by '{' */
    p=value; while ( (p = strchr(p,']')) ) *p =  '}'; /* replace all ']' by '}' */

    sh = (GB_HASH *)GBS_read_hash(hash,value);
    if (!sh){
        sh = GBS_create_hash(10, GB_IGNORE_CASE); /* Tags are case independent */
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
            if (del && strcmp(t,del) == 0) continue; /* test, whether to delete */
            if (sa[0] == 0) continue;
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

    static long g_bs_merge_tags(const char *tag, long val) {
        GBS_strcat(g_bs_merge_sub_result, tag);
        GBS_strcat(g_bs_merge_sub_result, ",");
        return val;
    }

    static long g_bs_read_tagged_hash(const char *value, long subhash){
        char *str;
        static int counter = 0;
        g_bs_merge_sub_result = GBS_stropen(100);
        GBS_hash_do_sorted_loop((GB_HASH *)subhash, g_bs_merge_tags, GBS_HCF_sortedByKey);
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
    g_bs_collect_tags_hash = GBS_create_hash(1024, GB_IGNORE_CASE);

    GBS_hash_do_sorted_loop(tag_hash, g_bs_read_tagged_hash, GBS_HCF_sortedByKey);     /* move everything into g_bs_collect_tags_hash */
    GBS_hash_do_sorted_loop(g_bs_collect_tags_hash, g_bs_read_final_hash, GBS_HCF_sortedByKey);

    GBS_free_hash_free_pointer(g_bs_collect_tags_hash);
    return GBS_strclose(g_bs_merge_result);
}

/* Create a tagged string from two tagged strings:
                 * a tagged string is '[tag,tag,tag] string'
                 * if s2 than delete all tags replace1 in string1
                 * if s1 than delete all tags replace2 in string2 */

char *GBS_merge_tagged_strings(const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2){
    char *str1 = strdup(s1);
    char *str2 = strdup(s2);
    char *t1 = GBS_string_2_key(tag1);
    char *t2 = GBS_string_2_key(tag2);
    char *result = 0;
    GB_ERROR error = 0;
    GB_HASH *hash = GBS_create_hash(16, GB_MIND_CASE);
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
    char *str = strdup(s);
    char *default_tag = GBS_string_2_key(dt);
    GB_HASH *hash = GBS_create_hash(16, GB_MIND_CASE);
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
            if (strcmp(t,tag) == 0) {
                s = strdup(sa);
                free(buf);
                goto found;
            }
        }
        s = se;
    }
 notfound:
    /* Nothing found */
    free(buf);
    s = 0;
 found:
    free(tag);
    return s;
}


GBDATA_SET *GB_create_set(int items){
    GBDATA_SET *set = (GBDATA_SET *)GB_calloc(sizeof(GBDATA_SET),1);
    set->nitems = 0;
    set->malloced_items = items;
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
    free(set->items);
    free(set);
}

/* be CAREFUL : this function is used to save ARB ASCII database (i.e. properties)
 * used as well to save perl macros
 *
 * when changing GBS_fwrite_string -> GBS_fread_string needs to be fixed as well
 *
 * always keep in mind, that many users have databases/macros written with older
 * versions of this function. They MUST load proper!!!
 */
void GBS_fwrite_string(const char *strngi,FILE *out){
    unsigned char *strng = (unsigned char *)strngi;
    int            c;
    
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
}

/*  Read a string from a file written by GBS_fwrite_string,
 *  Searches first '"'
 *
 *  WARNING : changing this function affects perl-macro execution (read warnings for GBS_fwrite_string)
 *  any changes should be done in GBS_fconvert_string too.
 */

char *GBS_fread_string(FILE *in) {
    void *strstr = GBS_stropen(1024);
    int   x;

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
    char *t = buffer;
    char *f = buffer;
    int   x;

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

const char *GBS_readable_size(unsigned long long size) {
    /* return human readable size information */
    /* returned string is maximal 7 characters long */

    if (size<1000) return GBS_global_string("%llu b", size);

    const char *units = "kMGTPEZY"; // kilo, Mega, Giga, Tera, ... should be enough forever
    int i;

    for (i = 0; units[i]; ++i) {
        char unit = units[i];
        if (size<1000*1024) {
            double amount = size/(double)1024;
            if (amount<10.0)  return GBS_global_string("%4.2f %cb", amount+0.005, unit);
            if (amount<100.0) return GBS_global_string("%4.1f %cb", amount+0.05, unit);
            return GBS_global_string("%i %cb", (int)(amount+0.5), unit);
        }
        size /= 1024; // next unit
    }
    ad_assert(0);
    return "<much>";
}

