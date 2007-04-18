#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <names_server.h>
#include <names_client.h>
#include "names.h"
#include <arbdb.h>
#include <names_prototypes.h>
#include <server.h>
#include <client.h>
#include <servercntrl.h>
#include <struct_man.h>

#include <list>
#include <string>

using namespace std;

// --------------------------------------------------------------------------------

#if defined(DEBUG)
// #define DUMP_NAME_CREATION
#endif // DEBUG

#define UPPERCASE(c) do{ (c) = toupper(c); }while(0)

// --------------------------------------------------------------------------------

struct AN_gl_struct  AN_global;
AN_main             *aisc_main; /* muss so heissen */

const int SERVER_VERSION = 4;

// --------------------------------------------------------------------------------

inline char *an_strlwr(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
    return str;
}

inline int an_strnicmp(const char *s1, const char *s2, int len) {
    int cmp = 0;

    for (int i = 0; i<len; i++) {
        cmp = tolower(s1[i])-tolower(s2[i]);
        if (cmp || !s1[i]) break;      // different or string-end reached in both strings
    }

    return cmp;
}

inline int an_stricmp(const char *s1, const char *s2) {
    int cmp = 0;

    for (int i = 0; ; i++) {
        cmp = tolower(s1[i])-tolower(s2[i]);
        if (cmp || !s1[i]) break;      // different or string-end reached in both strings
    }

    return cmp;
}


static AN_revers *lookup_an_revers(AN_main *main, const char *shortname) {
    char      *key        = an_strlwr(strdup(shortname));
    AN_revers *an_reverse = (AN_revers*)aisc_find_lib((struct_dllpublic_ext*)&(main->prevers), key);
    
    free(key);

    return an_reverse;
}

static AN_shorts *lookup_an_shorts(AN_main *main, const char *identifier) {
    // 'identifier' is either '*acc*add_id' or 'name1*name2*S' (see get_short() for details)
    // 'add_id' is the value of an additional DB field and may be empty.

    char      *key       = an_strlwr(strdup(identifier));
    AN_shorts *an_shorts = (AN_shorts*)aisc_find_lib((struct_dllpublic_ext*)&(main->pnames), key);

    free(key);

    return an_shorts;
}

static void an_add_short(AN_local *locs, const char *new_name,
                         const char *parsed_name, const char *parsed_sym,
                         const char *shrt, const char *acc, const char *add_id)
{
    AN_shorts *an_shorts;
    AN_revers *an_revers;
    char        *full_name;
    locs = locs;

    if (strlen(parsed_sym)){
        full_name = (char *)calloc(sizeof(char),
                                   strlen(parsed_name) + strlen(" sym")+1);
        sprintf(full_name,"%s sym",parsed_name);
    }else{
        full_name = strdup(parsed_name);
    }

    an_shorts = create_AN_shorts();
    an_revers = create_AN_revers();

    an_shorts->mh.ident  = an_strlwr(strdup(new_name));
    an_shorts->shrt      = strdup(shrt);
    an_shorts->full_name = strdup(full_name);
    an_shorts->acc       = strdup(acc);
    an_shorts->add_id    = strdup(add_id);

    aisc_link((struct_dllpublic_ext*)&(aisc_main->pnames),(struct_dllheader_ext*)an_shorts);

    an_revers->mh.ident  = an_strlwr(strdup(shrt));
    an_revers->full_name = full_name;
    an_revers->acc       = strdup(acc);
    an_revers->add_id    = strdup(add_id);

    aisc_link((struct_dllpublic_ext*)&(aisc_main->prevers),(struct_dllheader_ext*)an_revers);

    aisc_main->touched = 1;
}

static void an_remove_short(AN_shorts *an_shorts) {
    /* this should only be used to remove illegal entries from name-server.
       normally removing names does make problems - so use it very rarely */

    AN_revers *an_revers = lookup_an_revers(aisc_main, an_shorts->shrt);

    if (an_revers) {
        aisc_unlink((struct_dllheader_ext*)an_revers);

        free(an_revers->mh.ident);
        free(an_revers->full_name);
        free(an_revers->acc);
        free(an_revers);
    }

    aisc_unlink((struct_dllheader_ext*)an_shorts);

    free(an_shorts->mh.ident);
    free(an_shorts->shrt);
    free(an_shorts->full_name);
    free(an_shorts->acc);
    free(an_shorts);
}




static AN_shorts *an_find_shrt(AN_shorts *sin,char *search)
{
    while (sin) {
        if (!an_strnicmp(sin->shrt,search,3)) {
            return sin;
        }
        sin = sin->next;
    }
    return 0;
}

static char *nas_string_2_key(const char *str)
// converts any string to a valid key
{
#if defined(DUMP_NAME_CREATION)
    const char *org_str = str;
#endif // DUMP_NAME_CREATION
    
    char buf[GB_KEY_LEN_MAX+1];
    int  i;
    int  c;
    for (i=0;i<GB_KEY_LEN_MAX;) {
        c                        = *(str++);
        if (!c) break;
        if (isalpha(c)) buf[i++] = c;
        // name should not contain _'s (not compatible with FastDNAml)
        //else if (c==' ' || c=='_') buf[i++] = '_';
    }
    for (;i<GB_KEY_LEN_MIN;i++) buf[i] = '0';
    buf[i] = 0;
#if defined(DUMP_NAME_CREATION)
    printf("nas_string_2_key('%s') = '%s'\n", org_str, buf);
#endif // DUMP_NAME_CREATION
    return strdup(buf);
}

static char *nas_remove_small_vocals(const char *str) {
#if defined(DUMP_NAME_CREATION)
    const char *org_str = str;
#endif // DUMP_NAME_CREATION
    char buf[GB_KEY_LEN_MAX+1];
    int i;
    int c;

    for (i=0; i<GB_KEY_LEN_MAX; ) {
        c = *str++;
        if (!c) break;
        if (strchr("aeiouy", c)==0) {
            buf[i++] = c;
        }
    }
    for (; i<GB_KEY_LEN_MIN; i++) buf[i] = '0';
    buf[i] = 0;
#if defined(DUMP_NAME_CREATION)
    printf("nas_remove_small_vocals('%s') = '%s'\n", org_str, buf);
#endif // DUMP_NAME_CREATION
    return strdup(buf);
}

static void an_complete_shrt(char *shrt, const char *rest_of_full) {
    int len = strlen(shrt);

    while (len<5) {
        char c = *rest_of_full++;

        if (!c) break;
        shrt[len++] = c;
    }

    while (len<GB_KEY_LEN_MIN) {
        shrt[len++] = '0';
    }

    shrt[len] = 0;
}

static void an_autocaps(char *str) {
    // automatically capitalizes a string if it is completely up- or downcase

    bool is_single_case = true;
    {
        bool seen_upper_case = false;
        bool seen_lower_case = false;
        for (int i = 0; str[i] && is_single_case; i++) {
            char c = str[i];
            if (isalpha(c)) {
                if (islower(c)) seen_lower_case = true;
                else seen_upper_case            = true;

                if (seen_lower_case == seen_upper_case) { // both cases occurred
                    is_single_case = false;
                }
            }
        }
    }

    if (is_single_case) {
        bool next_is_capital = true;

        for (int i = 0; str[i]; i++) {
            char c = str[i];
            if (isalnum(c)) {
                if (next_is_capital) {
                    str[i]          = toupper(c);
                    next_is_capital = false;
                }
                else {
                    str[i] = tolower(c);
                }
            }
            else {
                next_is_capital = true;
            }
        }
    }
}

static char *an_get_short(AN_shorts *shorts, dll_public *parent, const char *full){
    AN_shorts *look;

    gb_assert(full);

    if (full[0]==0) {
        return strdup("Xxx");
    }

    char *result = 0;
    char *full1  = strdup(full);
    an_autocaps(full1);

    char *full2 = nas_string_2_key(full1);

    look = (AN_shorts *)aisc_find_lib((struct_dllpublic_ext*)parent, full2);
    if (look) {                 /* name is already known */
        free(full2);
        free(full1);
        return strdup(look->shrt);
    }

    char *full3 = 0;
    char  shrt[10];
    int   len2, len3;
    int   p1, p2, p3;

    // try first three letters:

    strncpy(shrt,full2,3);
    UPPERCASE(shrt[0]);
    shrt[3] = 0;

    look = an_find_shrt(shorts,shrt);
    if (!look) {
        len2   = strlen(full2);
        an_complete_shrt(shrt, len2>=3 ? full2+3 : "");
        goto found_short;
    }

    // generate names from first char + consonants:

    full3 = nas_remove_small_vocals(full2);
    len3 = strlen(full3);

    for (p1=1; p1<(len3-1); p1++) {
        shrt[1] = full3[p1];
        for (p2=p1+1; p2<len3; p2++) {
            shrt[2] = full3[p2];
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full3+p2+1);
                goto found_short;
            }
        }
    }

    // generate names from first char + all characters:

    len2 = strlen(full2);
    for (p1=1; p1<(len2-1); p1++) {
        shrt[1] = full2[p1];
        for (p2=p1+1; p2<len2; p2++) {
            shrt[2] = full2[p2];
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full2+p2+1);
                goto found_short;
            }
        }
    }

    // generate names containing first char + characters from name + one digit:

    for (p1=1; p1<len2; p1++) {
        shrt[1] = full2[p1];
        for (p2=0; p2<=9; p2++) {
            shrt[2] = '0'+p2;
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full2+p1+1);
                goto found_short;
            }
        }
    }

    // generate names containing first char + two digits:

    for (p1=1; p1<=99; p1++) {
        shrt[1] = '0'+(p1/10);
        shrt[2] = '0'+(p1%10);
        look = an_find_shrt(shorts, shrt);
        if (!look) {
            an_complete_shrt(shrt, full2+1);
            goto found_short;
        }
    }

    // generate names containing 1 random character + 2 digits:

    for (p1='A'; p1<='Z'; p1++) {
        shrt[0] = p1;
        for (p2=0; p2<=99; p2++) {
            shrt[1] = '0'+(p2/10);
            shrt[2] = '0'+(p2%10);
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full2);
                goto found_short;
            }
        }
    }

    // generate names containing 2 random characters + 1 digit:

    for (p1='A'; p1<='Z'; p1++) {
        shrt[0] = p1;
        for (p2='a'; p2<='z'; p2++) {
            shrt[1] = p2;
            for (p3=0; p3<=9; p3++) {
                shrt[2] = '0'+p3;
                look = an_find_shrt(shorts, shrt);
                if (!look) {
                    an_complete_shrt(shrt, full2);
                    goto found_short;
                }
            }
        }
    }

    // generate names containing 3 random characters:

    for (p1='A'; p1<='Z'; p1++) {
        shrt[0] = p1;
        for (p2='a'; p2<='z'; p2++) {
            shrt[1] = p2;
            for (p3='a'; p3<='z'; p3++) {
                shrt[2] = p3;
                look = an_find_shrt(shorts, shrt);
                if (!look) {
                    an_complete_shrt(shrt, full2);
                found_short:
                    result = shrt;
                    goto done;
                }
            }
        }
    }

 done: 
    
    if (result) {
#if defined(DUMP_NAME_CREATION)
        if (isdigit(result[0]) || isdigit(result[1])) {
            printf("generated new short-name '%s' for full-name '%s' full2='%s' full3='%s'\n", shrt, full, full2, full3);
        }
#endif    // DUMP_NAME_CREATION

        look           = create_AN_shorts();
        look->mh.ident = strdup(full2);
        look->shrt     = strdup(result);
        aisc_link((struct_dllpublic_ext*)parent,(struct_dllheader_ext*)look);
        
        aisc_main->touched = 1;
    }
    else {
        printf("ARB_name_server: Failed to make a short-name for '%s'\n", full);
    }

    free(full3);
    free(full2);
    free(full1);

    return strdup(result);
}

// --------------------------------------------------------------------------------

static const char *default_full_name = "No name";

class NameInformation {
    const char *full_name;

    char *parsed_name;
    char *parsed_sym;
    char *parsed_acc;
    char *parsed_add_id;

    char *first_name;
    char *rest_of_name;

    char *id;

public:
    NameInformation(AN_local *locs);
    ~NameInformation();

    const char *get_id() const { return id; }
    const char *get_full_name() const { return full_name; }
    const char *get_first_name() const { return first_name; }
    const char *get_rest_of_name() const { return rest_of_name; }

    void add_short(AN_local *locs, const char *shrt) const {
        if (strlen(parsed_name)>3) {
            an_add_short(locs, id, parsed_name, parsed_sym, shrt, parsed_acc, parsed_add_id);
        }
    }
};

static bool stralnum(const char *str) {
    bool nonalnum = false;
    for (char c = *str++; c; c = *str++) {
        if (!isalnum(c)) {
            nonalnum = true;
            break;
        }
    }
    return !nonalnum;
}

static char *make_alnum(const char *str) {
    // returns a heap-copy containing all alphanumeric characters of 'str'

    char *newStr = (char*)malloc(strlen(str)+1);
    int   n      = 0;

    for (int p = 0; str[p]; ++p) {
        if (isalnum(str[p])) newStr[n++] = str[p];
    }
    newStr[n] = 0;
    return newStr;
}
static char *make_alpha(const char *str) {
    // returns a heap-copy containing all alpha characters of 'str'

    char *newStr = (char*)malloc(strlen(str)+1);
    int   n      = 0;

    for (int p = 0; str[p]; ++p) {
        if (isalpha(str[p])) newStr[n++] = str[p];
    }
    newStr[n] = 0;
    return newStr;
}

#if defined(DEBUG)
#define assert_alnum(s) gb_assert(stralnum(s))
#else
#define assert_alnum(s)
#endif // DEBUG

NameInformation::NameInformation(AN_local *locs) {
    full_name = locs->full_name;
    if (!full_name || !full_name[0]) full_name = default_full_name;

    parsed_name = GBS_string_eval(full_name,
                                  "\t= :\"= :'= :" // replace TABs and quotes by space
                                  "sp.=species:spec.=species:SP.=SPECIES:SPEC.=SPECIES:" // replace common abbreviations of 'species' 
                                  ".= :" // replace dots by spaces
                                  "  = :" // multiple spaces -> 1 space
                                  "* * *=*1 *2" // skip all beyond 2nd word
                                  ,0);
    an_autocaps(parsed_name);

    parsed_sym = GBS_string_eval(full_name, "\t= :* * *sym*=S",0);
    if (strlen(parsed_sym)>1) {
        free(parsed_sym);
        parsed_sym = strdup("");
    }

    parsed_acc      = make_alnum(locs->acc);
    parsed_add_id   = make_alnum(locs->add_id);
    first_name      = GBS_string_eval(parsed_name,"* *=*1",0);
    rest_of_name    = make_alnum(parsed_name+strlen(first_name));

    {
        char *first_name_an = make_alnum(first_name);
        free(first_name);
        first_name = first_name_an;
    }

    assert_alnum(parsed_acc);
    assert_alnum(first_name);
    assert_alnum(rest_of_name);

    UPPERCASE(rest_of_name[0]);

    // build id

    id = (strlen(parsed_acc)+strlen(parsed_add_id))
        ? GBS_global_string_copy("*%s*%s", parsed_acc, parsed_add_id)
        : GBS_global_string_copy("%s*%s*%s", first_name, rest_of_name, parsed_sym);
}

NameInformation::~NameInformation() {
    free(id);
    
    free(rest_of_name);
    free(first_name);

    free(parsed_add_id);
    free(parsed_acc);
    free(parsed_sym);
    free(parsed_name);
}

// --------------------------------------------------------------------------------
// AISC functions

extern "C" int del_short(AN_local *locs)
/* forget about a short name */
{
    NameInformation  info(locs);
    int              removed   = 0;
    AN_shorts       *an_shorts = lookup_an_shorts(aisc_main, info.get_id());

    if (an_shorts) {
        an_remove_short(an_shorts);
        removed = 1;
    }

    return removed;
}

static GB_HASH *nameModHash = 0; // key = default name; value = max. counter tested

extern "C" aisc_string get_short(AN_local *locs)
/* get the short name from the previously set names */
{
    static char *shrt = 0;
    if (shrt) {
        free(shrt);
        shrt = 0;
    }

    NameInformation  info(locs);
    AN_shorts       *an_shorts = lookup_an_shorts(aisc_main, info.get_id());

    if (an_shorts) {            // we already have a short name
        bool recreate = false;

        if (!stralnum(an_shorts->shrt)) { // contains non-alphanumeric characters
            recreate = true;
        }
        else if (strcmp(an_shorts->full_name, default_full_name) == 0 && // fullname in name server is default_full_name
                 strcmp(info.get_full_name(), an_shorts->full_name) != 0) // and differs from current
        {
            recreate = true;
        }
        if (recreate) {
            an_remove_short(an_shorts);
            an_shorts = 0;
        }
        else {
            shrt = strdup(an_shorts->shrt);
        }
    }
    if (!shrt) { /* now there is no short name (or an illegal one) */
        char *first_advice=0,*second_advice=0;

        if (locs->advice[0] && !stralnum(locs->advice)) { // bad advice 
            locs->advice[0] = 0; // delete it
        }

        if (locs->advice[0]) {
            char *advice = make_alpha(locs->advice);

            first_advice = strdup(advice);
            if (strlen(advice) > 3) {
                second_advice = strdup(advice+3);
                first_advice[3] = 0;
            }
        }

        if (!first_advice) first_advice = strdup("Xxx");
        if (!second_advice) second_advice = strdup("Yyyyy");

        char *first_short;
        int   first_len;
        {
            const char *first_name = info.get_first_name();
            first_short      = first_name[0]
                ? an_get_short(aisc_main->shorts1, &(aisc_main->pshorts1), first_name)
                : strdup(first_advice);

            gb_assert(first_short);
            if (first_short[0] == 0) { // empty?
                free(first_short);
                first_short = strdup("Xxx");
            }
            first_len = strlen(first_short);
        }

        char *second_short = (char*)calloc(10, 1);
        int   second_len;
        {
            const char *rest_of_name = info.get_rest_of_name();
            int         restlen      = strlen(rest_of_name);

            if (!restlen) {
                rest_of_name = second_advice;
                restlen      = strlen(rest_of_name);
                if (!restlen) {
                    rest_of_name = "Yyyyy";
                    restlen      = 5;
                }
            }

            second_short[0] = 0;

            if (restlen<5 && first_len>3) {
                strcpy(second_short, first_short+3); // take additional characters from first_short
                second_short[5-restlen] = 0; // but not too many
            }

            char *strend = strchr(second_short, 0);
            strncpy(strend, rest_of_name, 8);
            second_len   = strlen(second_short);
        }

        if (first_len>3) {
            first_short[3] = 0;
            first_len      = 3;;
        }

        int both_len = first_len+second_len;
        if (both_len<8) {
            char *new_second  = GBS_global_string_copy("%s00000000", second_short);
            free(second_short);
            second_short      = new_second;
            second_len       += 8;
            both_len         += 8;
        }

        if (both_len>8) {
            second_len               = 8-first_len;
            second_short[second_len] = 0;
            both_len                 = 8;
        }

        char test_short[9];
        sprintf(test_short,"%s%s", first_short, second_short);
        
        gb_assert(size_t(both_len) == strlen(test_short));
        gb_assert(second_len>=5 && second_len <= 8);

        if (lookup_an_revers(aisc_main, test_short)) {
            if (!nameModHash) nameModHash = GBS_create_hash(1000, 1);

            char *test_short_dup = strdup(test_short);
            long  count          = GBS_read_hash(nameModHash, test_short);
            if (count<2) count   = 2; // first name modification uses number 2

            int  printOffset = both_len;
            bool foundUnused = false;

            // first create alternate name with digits only
            {
                int digLimit[6] = { 0, 9, 99, 999, 9999, 99999 };
                for (int digits = 1; !foundUnused && digits <= 5; ++digits) {
                    int maxOffset = 8-digits;
                    int limit     = digLimit[digits];

                    if (printOffset>maxOffset) printOffset = maxOffset;
                
                    char *printAt = test_short+printOffset;

                    for (; !foundUnused && count <= limit; ++count) {
                        int printed = sprintf(printAt, "%li", count);
                        gb_assert((printed+printOffset) <= 8);
                        if (!lookup_an_revers(aisc_main, test_short)) foundUnused = true; // name does not exist
                    }
                }
            }

            // if no unused name found, create one with alpha-chars
            if (!foundUnused) {
                strcpy(test_short, test_short_dup);

                long        count2  = count-100000; // 100000 numbers were used above
                char       *printAt = test_short+3;
                const char *base36  = "0123456789abcdefghijklmnopqrstuvwxyz";

                printAt[5] = 0;

                for (; !foundUnused && count2<16796160; ++count2) { // 16796160 = 36^4*10
                    // now print count2 converted to base 36

                    int c = count2;
                    for (int pos = 0; pos<5; ++pos) {
                        int nextc = c/36;
                        int rest  = c-36*nextc;

                        printAt[4-pos] = base36[rest];
                        c              = nextc;

                        gb_assert(pos != 4 || c == 0);
                    }

                    if (!lookup_an_revers(aisc_main, test_short)) foundUnused = true; // name does not exist
                }

                count = count2+100000;
            }

            gb_assert(foundUnused);
            GBS_write_hash(nameModHash, test_short_dup, count);

            free(test_short_dup);
        }

        assert_alnum(test_short);

        shrt = strdup(test_short);
        info.add_short(locs, shrt);

        free(first_short);
        free(second_short);
        free(first_advice);
        free(second_advice);
    }

    assert_alnum(shrt);
    return shrt;
}

extern "C" int server_save(AN_main *main, int dummy)
{
    FILE        *file;
    int error;
    char        *sec_name;
    dummy = dummy;
    if (main->touched) {
        int server_date = GB_time_of_file(main->server_file);
        if (server_date>main->server_filedate) {
            printf("Another nameserver changed '%s' - your changes are lost.\n", main->server_file);
        }
        else {
            sec_name = (char *)calloc(sizeof(char), strlen(main->server_file)+2);
            sprintf(sec_name,"%s%%", main->server_file);
            printf("Saving '%s'..\n", main->server_file);
            file     = fopen(sec_name,"w");
            if (!file) {
                fprintf(stderr,"ERROR cannot save file '%s'\n",sec_name);
            }
            else { 
                error = save_AN_main(main,file);
                fclose(file);
                if (!error) {
                    if (GB_rename_file(sec_name, main->server_file)) {
                        GB_print_error();
                    }
                    else {
                        main->touched = 0;
                    }
                }
            }
            free(sec_name);
            main->server_filedate = GB_time_of_file(main->server_file);
        }        
    }
    else {
        printf("No changes to ARB_name_server data.\n");
    }

    return 0;
}

#if defined(DEBUG) && 0
static void check_list(AN_shorts *start) {
    int count = 0;
    while (++count) {
        start = start->next;
        if (!start) {
            fprintf(stderr, "<list has %i elements>\n", count);
            return;
        }
    }

    fprintf(stderr, "<ERROR - list is looped>\n");
    gb_assert(0);
}
#endif // DEBUG

static void check_for_case_error(AN_main *main) {
    // test for duplicated names or name parts (only differing in case)
    // such names were created by old name server versions

    bool case_error_occurred = false;
    int  idents_changed      = 0;

    // first check name parts
    for (AN_shorts *shrt = main->shorts1; shrt; ) {
        AN_shorts *next  = shrt->next;
        AN_shorts *found = an_find_shrt(main->shorts1, shrt->shrt);
        if (found != shrt) {
            fprintf(stderr, "- Correcting error in name-database: '%s' equals '%s'\n",
                    found->shrt, shrt->shrt);
            an_remove_short(shrt);
            case_error_occurred = true;
        }
        shrt = next;
    }

    // then check full short-names
    for (AN_shorts *shrt = main->names; shrt; ) {
        AN_shorts *next  = shrt->next;
        AN_revers *found = lookup_an_revers(main, shrt->shrt);

        if (found && (shrt->acc && found->acc && an_stricmp(shrt->acc, found->acc) != 0)) {
            fprintf(stderr, "- Correcting error in name-database: '%s' equals '%s' (but acc differs)\n",
                    found->mh.ident, shrt->shrt);

            an_remove_short(shrt);
            case_error_occurred = true;
        }
        else if (found && (shrt->add_id && found->add_id && an_stricmp(shrt->add_id, found->add_id) != 0)) {
            fprintf(stderr, "- Correcting error in name-database: '%s' equals '%s' (but add_id differs)\n",
                    found->mh.ident, shrt->shrt);

            an_remove_short(shrt);
            case_error_occurred = true;
        }
        else {
            AN_shorts *self_find = lookup_an_shorts(main, shrt->mh.ident);
            if (!self_find) { // stored with wrong key (not lowercase)
                aisc_unlink((struct_dllheader_ext*)shrt);
                an_strlwr(shrt->mh.ident);
                aisc_link((struct_dllpublic_ext*)&(main->pnames), (struct_dllheader_ext*)shrt);
                main->touched = 1;

                case_error_occurred = true;
                idents_changed++;
            }
            else if (self_find != shrt) {
                fprintf(stderr, "- Correcting error in name-database: '%s' equals '%s' (case-difference in full_name or acc)\n",
                        shrt->mh.ident, self_find->mh.ident);
                an_remove_short(shrt);
                case_error_occurred = true;
            }
        }

        shrt = next;
    }

    if (case_error_occurred) {
        int regen_name_parts = 0;
        int deleted_names    = 0;

        // now capitalize all name parts
        {
            list<string> idents_to_recreate;

            for (AN_shorts *shrt =  main->shorts1; shrt; ) {
                char      *cap_name = strdup(shrt->shrt);
                an_autocaps(cap_name);

                if (strcmp(cap_name, shrt->shrt) != 0) {
                    idents_to_recreate.push_back(shrt->mh.ident);
                }
                free(cap_name);
                
                AN_shorts *next = shrt->next;
                an_remove_short(shrt);
                shrt = next;
            }

            list<string>::const_iterator end = idents_to_recreate.end();
            for (list<string>::const_iterator i = idents_to_recreate.begin(); i != end; ++i) {
                const char *ident = i->c_str();
                an_get_short(main->shorts1, &(main->pshorts1), ident);
                regen_name_parts++;
            }
        }

        // now capitalize all short names
        for (AN_shorts *shrt =  main->names; shrt; ) {
            AN_shorts *next     = shrt->next;
            char      *cap_name = strdup(shrt->shrt);
            an_autocaps(cap_name);

            if (strcmp(cap_name, shrt->shrt) != 0) {
                // fprintf(stderr, "Deleting entry '%s'\n", shrt->shrt);
                an_remove_short(shrt);
                deleted_names++;
            }
            shrt = next;
            free(cap_name);
        }

        if (idents_changed)   fprintf(stderr, "* Changed case of %i identifiers.\n", idents_changed);
        if (regen_name_parts) fprintf(stderr, "* Regenerated %i prefix names.\n", regen_name_parts);
        if (deleted_names)    fprintf(stderr, "* Removed %i names with wrong case.\n"
                                      "=> This leads to name changes when generating new names (which is recommended now).\n", deleted_names);
    }
}

static void check_for_illegal_chars(AN_main *main) {
    // test for names containing illegal characters
    int illegal_names = 0;

    // first check name parts
    for (AN_shorts *shrt = main->shorts1; shrt; ) {
        AN_shorts *next = shrt->next;
        if (!stralnum(shrt->shrt)) {
            fprintf(stderr, "- Fixing illegal chars in '%s'\n", shrt->shrt);
            an_remove_short(shrt);
            illegal_names++;
        }
        shrt = next;
    }
    // then check full short-names
    for (AN_shorts *shrt = main->names; shrt; ) {
        AN_shorts *next  = shrt->next;
        if (!stralnum(shrt->shrt)) {
            fprintf(stderr, "- Fixing illegal chars in '%s'\n", shrt->shrt);
            an_remove_short(shrt);
            illegal_names++;
        }
        shrt = next;
    }
    
    if (illegal_names>0) {
        fprintf(stderr, "* Removed %i names containing illegal characters.\n"
                "=> This leads to name changes when generating new names (which is recommended now).\n", illegal_names);
    }
}

static GB_ERROR server_load(AN_main *main)
{
    FILE      *file;
    GB_ERROR   error = 0;
    AN_shorts *shrt;
    AN_revers *revers;

    fprintf(stderr, "Starting ARB_name_server..\n");

    file = fopen(main->server_file,"r");
    if (!file) {
        error = GBS_global_string("No such file '%s'", main->server_file);
    }
    else {
        fprintf(stderr, "* Loading %s\n", main->server_file);
        int err_code = load_AN_main(main,file);
        if (err_code) {
            error = GBS_global_string("Error #%i while loading '%s'", err_code, main->server_file);
        }
    }

    if (!error) {
        fprintf(stderr, "* Parsing data\n");
        long nameCount =  0;
        for (shrt = main->names; shrt; shrt = shrt->next) {
            revers            = create_AN_revers();
            revers->mh.ident  = an_strlwr(strdup(shrt->shrt));
            revers->full_name = strdup(shrt->full_name);
            revers->acc       = strdup(shrt->acc);
            revers->add_id    = shrt->add_id ? strdup(shrt->add_id) : 0;
            aisc_link((struct_dllpublic_ext*)&(main->prevers),(struct_dllheader_ext*)revers);
            nameCount++;
        }

        int namesDBversion = main->dbversion; // version used to save names.dat
        fprintf(stderr, "* Loaded NAMEDB v%i (contains %li names)\n", namesDBversion, nameCount);

        if (namesDBversion < SERVER_VERSION) {
            if (namesDBversion<4) {
                fprintf(stderr, "* Checking for case-errors\n");
                check_for_case_error(main);
                
                fprintf(stderr, "* Checking for illegal characters\n");
                check_for_illegal_chars(main);
            }

            fprintf(stderr, "* NAMEDB version upgrade %i -> %i\n", namesDBversion, SERVER_VERSION);
            main->dbversion = SERVER_VERSION;
            main->touched   = 1; // force save
        }

        fprintf(stderr, "ARB_name_server is up.\n");
        main->server_filedate = GB_time_of_file(main->server_file);
    }
    else {
        main->server_filedate = -1;
    }
    return error;
}

int names_server_shutdown(int exitcode) {
    aisc_server_shutdown_and_exit(AN_global.server_communication, exitcode); // never returns
    return 0;
}

int names_server_save(void) {
    server_save(aisc_main, 0);
    return 0;
}

extern "C" int server_shutdown(AN_main *pm, aisc_string passwd){
    /** passwdcheck **/
    if( strcmp(passwd, "ldfiojkherjkh") ) return 1;
    printf("\narb_name_server: I got the shutdown message.\n");

    /** shoot clients **/
    aisc_broadcast(AN_global.server_communication, 0,
                   "SERVER SHUTDOWN BY ADMINISTRATOR!\n");

    /** shutdown **/
    printf("ARB_name_server: server shutdown by administrator\n");
    names_server_shutdown(0);   // never returns!
    pm = pm;
    return 0;
}

static void usage(const char *exeName, const char *err) __attribute__((noreturn));
static void usage(const char *exeName, const char *err) {
    printf("ARB nameserver v%i\n"
           "Usage: %s command server-parameters\n"
           "command = -boot\n"
           "          -kill\n"
           "          -look\n"
           , SERVER_VERSION, exeName);
    arb_print_server_params();
    if (err) printf("Error: %s\n", err);
    exit(-1);
}

int main(int argc,char **argv)
{
    char              *name;
    int                i;
    struct Hs_struct  *so;
    struct arb_params *params;

    params                 = arb_trace_argv(&argc,argv);
    const char *executable = argv[0];

    if (!params->default_file) usage(executable, "Missing default file");

    if (argc==1) { // default command is '-look'
        char flag[]="-look";
        argv[1] = flag;
        argc = 2;
    }

    if (argc!=2) usage(executable, "Too many parameters");

    aisc_main = create_AN_main();
    /***** try to open com with any other pb server ******/
    if (params->tcp) {
        name = params->tcp;
    }else{
        if( !(name=(char *)GBS_read_arb_tcp("ARB_NAME_SERVER")) ){
            GB_print_error();
            exit(-1);
        }else{
            name=strdup(name);
        }
    }

    AN_global.cl_link = (aisc_com *)aisc_open(name, (long *)&AN_global.cl_main,AISC_MAGIC_NUMBER);

    if (AN_global.cl_link) {
        if( !strcmp(argv[1],"-look")) {
            printf("ARB_name_server: No client - terminating.\n");
            aisc_close(AN_global.cl_link);AN_global.cl_link = 0;
            exit(0);
        }

        printf("There is another active nameserver. I try to kill it..\n");
        aisc_nput(AN_global.cl_link, AN_MAIN, AN_global.cl_main,
                  MAIN_SHUTDOWN, "ldfiojkherjkh",
                  NULL);
        aisc_close(AN_global.cl_link);AN_global.cl_link=0;
        sleep(1);
    }
    if( ((strcmp(argv[1],"-kill")==0)) ||
        ((argc==3) && (strcmp(argv[2],"-kill")==0))){
        printf("ARB_name_server: Now I kill myself!\n");
        exit(0);
    }
    for (i=0, so=0; (i<MAX_TRY) && (!so); i++){
        so = open_aisc_server(name, NAME_SERVER_SLEEP*1000L ,0);
        if (!so) sleep(RETRY_SLEEP);
    }
    if (!so) {
        printf("AN_SERVER: Gave up on opening the communication socket!\n");
        exit(0);
    }
    AN_global.server_communication = so;

    aisc_main->server_file     = strdup(params->default_file);
    aisc_main->server_filedate = GB_time_of_file(aisc_main->server_file);

    GB_ERROR error = server_load(aisc_main);

    if (!error) {
        const char *field     = params->field;
        if (field == 0) field = ""; // default to no field
        
        if (aisc_main->add_field == 0) {
            printf("* add. field not defined yet -> using '%s'\n", field);
            aisc_main->add_field = strdup(field);
            aisc_main->touched   = 1;
        }
        else {
            if (strcmp(aisc_main->add_field, field) != 0) {
                if (aisc_main->add_field[0]) {
                    error = GBS_global_string("This nameserver has to be started with -f%s", aisc_main->add_field);
                }
                else {
                    error = "This nameserver has to be started w/o -f option";
                }
            }
        }
    }

    long accept_calls_init = NAME_SERVER_TIMEOUT/long(NAME_SERVER_SLEEP);
    long accept_calls      = accept_calls_init;
    bool isTimeout         = true;

    if (!error && aisc_main->touched) server_save(aisc_main, 0);

    while (!error && accept_calls>0) {
        aisc_accept_calls(so);

        if (aisc_main->ploc_st.cnt <=0) { // timeout passed and no clients
            accept_calls--;

            long server_date = GB_time_of_file(aisc_main->server_file);
            if (server_date == 0 && aisc_main->server_filedate != 0) {
                fprintf(stderr, "ARB_name_server data has been deleted.\n");
                accept_calls = 0;
                isTimeout    = false;
            }
            if (server_date>aisc_main->server_filedate) {
                fprintf(stderr, "ARB_name_server data changed on disc.\n");
                accept_calls = 0;
                isTimeout    = false;
            }
            else if (aisc_main->touched) {
                server_save(aisc_main,0);
                accept_calls = accept_calls_init;
            }
        }
        else { // got a client
            accept_calls = accept_calls_init;
        }
    }

    if (error) {
        fprintf(stderr, "Error in ARB_name_server: %s\n", error);
    }
    else if (accept_calls == 0) {
        if (isTimeout) {
            fprintf(stderr, "Been idle for %i minutes.\n", int(NAME_SERVER_TIMEOUT/60));
        }
    }

    printf("ARB_name_server terminating...\n");
    names_server_shutdown(error ? EXIT_FAILURE : EXIT_SUCCESS); // never returns
}
