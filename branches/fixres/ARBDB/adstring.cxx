// =============================================================== //
//                                                                 //
//   File      : adstring.cxx                                      //
//   Purpose   : various string functions                          //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arb_backtrace.h>
#include <arb_strbuf.h>
#include <arb_sort.h>
#include <arb_defs.h>
#include <arb_str.h>

#include "gb_key.h"

#include <SigHandler.h>

#include <execinfo.h>

#include <cstdarg>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <setjmp.h>

#include <valgrind.h>

static char *GBS_string_2_key_with_exclusions(const char *str, const char *additional) {
    // converts any string to a valid key (all chars in 'additional' are additionally allowed)
    char buf[GB_KEY_LEN_MAX+1];
    int i;
    int c;
    for (i=0; i<GB_KEY_LEN_MAX;) {
        c = *(str++);
        if (!c) break;

        if (c==' ' || c == '_') {
            buf[i++]  = '_';
        }
        else if (isalnum(c) || strchr(additional, c) != 0) {
            buf[i++]  = c;
        }
    }
    for (; i<GB_KEY_LEN_MIN; i++) buf[i] = '_';
    buf[i] = 0;
    return ARB_strdup(buf);
}

char *GBS_string_2_key(const char *str) // converts any string to a valid key
{
    return GBS_string_2_key_with_exclusions(str, "");
}

char *GB_memdup(const char *source, size_t len) {
    char *dest = (char *)ARB_alloc(len);
    memcpy(dest, source, len);
    return dest;
}

GB_ERROR GB_check_key(const char *key) { // goes to header: __ATTR__USERESULT
    // test whether all characters are letters, numbers or _
    int  i;
    long len;

    if (!key || key[0] == 0) return "Empty key is not allowed";
    len = strlen(key);
    if (len>GB_KEY_LEN_MAX) return GBS_global_string("Invalid key '%s': too long", key);
    if (len < GB_KEY_LEN_MIN) return GBS_global_string("Invalid key '%s': too short", key);

    for (i = 0; key[i]; ++i) {
        char c = key[i];
        if ((c>='a') && (c<='z')) continue;
        if ((c>='A') && (c<='Z')) continue;
        if ((c>='0') && (c<='9')) continue;
        if (c=='_') continue;
        return GBS_global_string("Invalid character '%c' in '%s'; allowed: a-z A-Z 0-9 '_' ", c, key);
    }

    return 0;
}
GB_ERROR GB_check_link_name(const char *key) { // goes to header: __ATTR__USERESULT
    // test whether all characters are letters, numbers or _
    int  i;
    long len;

    if (!key || key[0] == 0) return GB_export_error("Empty key is not allowed");
    len = strlen(key);
    if (len>GB_KEY_LEN_MAX) return GB_export_errorf("Invalid key '%s': too long", key);
    if (len < 1) return GB_export_errorf("Invalid key '%s': too short", key); // here it differs from GB_check_key

    for (i = 0; key[i]; ++i) {
        char c = key[i];
        if ((c>='a') && (c<='z')) continue;
        if ((c>='A') && (c<='Z')) continue;
        if ((c>='0') && (c<='9')) continue;
        if (c=='_') continue;
        return GB_export_errorf("Invalid character '%c' in '%s'; allowed: a-z A-Z 0-9 '_' ", c, key);
    }

    return 0;
}
GB_ERROR GB_check_hkey(const char *key) { // goes to header: __ATTR__USERESULT
    // test whether all characters are letters, numbers or _
    // additionally allow '/' and '->' for hierarchical keys
    GB_ERROR err = 0;

    if (!key || key[0] == 0) {
        err = "Empty key is not allowed";
    }
    else if (!strpbrk(key, "/-")) {
        err = GB_check_key(key);
    }
    else {
        char *key_copy = ARB_strdup(key);
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
                        err = GBS_global_string("'>' expected after '-' in '%s'", key);
                    }
                    start = key_end+2;
                }
                else {
                    gb_assert(c == '/');
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

// ---------------------------
//      escape characters

char *GBS_remove_escape(char *com)  // \ is the escape character

{
    char *result, *s, *d;
    int   ch;

    s = d = result = ARB_strdup(com);
    while ((ch = *(s++))) {
        switch (ch) {
            case '\\':
                ch = *(s++); if (!ch) { s--; break; };
                switch (ch) {
                    case 'n':   *(d++) = '\n'; break;
                    case 't':   *(d++) = '\t'; break;
                    case '0':   *(d++) = '\0'; break;
                    default:    *(d++) = ch; break;
                }
                break;
            default:
                *(d++) = ch;
        }
    }
    *d = 0;
    return result;
}

// ----------------------------------------------
//      escape/unescape characters in strings

char *GBS_escape_string(const char *str, const char *chars_to_escape, char escape_char) {
    /*! escape characters in 'str'
     *
     * uses a special escape-method, which eliminates all 'chars_to_escape' completely
     * from str (this makes further processing of the string more easy)
     *
     * @param str string to escape
     *
     * @param escape_char is the character used for escaping. For performance reasons it
     * should be a character rarely used in 'str'.
     *
     * @param chars_to_escape may not contain 'A'-'Z' (these are used for escaping)
     * and it may not be longer than 26 bytes
     *
     * @return heap copy of escaped string
     *
     * Inverse of GBS_unescape_string()
     */

    int   len    = strlen(str);
    char *buffer = (char*)ARB_alloc(2*len+1);
    int   j      = 0;
    int   i;

    gb_assert(strlen(chars_to_escape)              <= 26);
    gb_assert(strchr(chars_to_escape, escape_char) == 0); // escape_char may not be included in chars_to_escape

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

                gb_assert(found[0]<'A' || found[0]>'Z'); // illegal character in chars_to_escape
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
    //! inverse of GB_escape_string() - for params see there

    int   len    = strlen(str);
    char *buffer = (char*)ARB_alloc(len+1);
    int   j      = 0;
    int   i;

#if defined(ASSERTION_USED)
    int escaped_chars_len = strlen(escaped_chars);
#endif // ASSERTION_USED

    gb_assert(strlen(escaped_chars)              <= 26);
    gb_assert(strchr(escaped_chars, escape_char) == 0); // escape_char may not be included in chars_to_escape

    for (i = 0; str[i]; ++i) {
        if (str[i] == escape_char) {
            if (str[i+1] == escape_char) {
                buffer[j++] = escape_char;
            }
            else {
                int idx = str[i+1]-'A';

                gb_assert(idx >= 0 && idx<escaped_chars_len);
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

char *GBS_eval_env(GB_CSTR p) {
    GB_ERROR       error = 0;
    GB_CSTR        ka;
    GBS_strstruct *out   = GBS_stropen(1000);

    while ((ka = GBS_find_string(p, "$(", 0))) {
        GB_CSTR kz = strchr(ka, ')');
        if (!kz) {
            error = GBS_global_string("missing ')' for envvar '%s'", p);
            break;
        }
        else {
            char *envvar = ARB_strpartdup(ka+2, kz-1);
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
        GBS_strforget(out);
        return 0;
    }

    GBS_strcat(out, p);         // copy rest
    return GBS_strclose(out);
}

long GBS_gcgchecksum(const char *seq)
// GCGchecksum
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

// Table of CRC-32's of all single byte values (made by makecrc.c of ZIP source)
uint32_t crctab[] = {
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

uint32_t GB_checksum(const char *seq, long length, int ignore_case,  const char *exclude) // RALF: 02-12-96
{
    /* CRC32checksum: modified from CRC-32 algorithm found in ZIP compression source
     * if ignore_case == true -> treat all characters as uppercase-chars (applies to exclude too)
     */

    unsigned long c = 0xffffffffL;
    long          n = length;
    int           i;
    int           tab[256];

    for (i=0; i<256; i++) {
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
     // if 'ignore_case' == true -> treat all characters as uppercase-chars (applies to 'exclude' too)
{
    return GB_checksum(seq, strlen(seq), ignore_case, exclude);
}

/* extract all words in a text that:
    1. minlen < 1.0  contain more than minlen*len_of_text characters that also exists in chars
    2. minlen > 1.0  contain more than minlen characters that also exists in chars
*/

char *GBS_extract_words(const char *source, const char *chars, float minlen, bool sort_output) {
    char           *s         = ARB_strdup(source);
    char          **ps        = ARB_calloc<char*>((strlen(source)>>1) + 1);
    GBS_strstruct  *strstruct = GBS_stropen(1000);
    char           *f         = s;
    int             count     = 0;
    char           *p;
    char           *h;
    int             cnt;
    int             len;
    int             iminlen   = (int) (minlen+.5);

    while ((p = strtok(f, " \t,;:|"))) {
        f = 0;
        cnt = 0;
        len = strlen(p);
        for (h=p; *h; h++) {
            if (strchr(chars, *h)) cnt++;
        }

        if (minlen == 1.0) {
            if (cnt != len) continue;
        }
        else if (minlen > 1.0) {
            if (cnt < iminlen) continue;
        }
        else {
            if (len < 3 || cnt < minlen*len) continue;
        }
        ps[count] = p;
        count ++;
    }
    if (sort_output) {
        GB_sort((void **)ps, 0, count, GB_string_comparator, 0);
    }
    for (cnt = 0; cnt<count; cnt++) {
        if (cnt) {
            GBS_chrcat(strstruct, ' ');
        }
        GBS_strcat(strstruct, ps[cnt]);
    }

    free(ps);
    free(s);
    return GBS_strclose(strstruct);
}


size_t GBS_shorten_repeated_data(char *data) {
    // shortens repeats in 'data'
    // This function modifies 'data'!!
    // e.g. "..............................ACGT....................TGCA"
    // ->   ".{30}ACGT.{20}TGCA"

#if defined(DEBUG)
    size_t  orgLen    = strlen(data);
#endif // DEBUG
    char   *dataStart = data;
    char   *dest      = data;
    size_t  repeat    = 1;
    char    last      = *data++;

    while (last) {
        char curr = *data++;
        if (curr == last) {
            repeat++;
        }
        else {
            if (repeat >= 5) {
                dest += sprintf(dest, "%c{%zu}", last, repeat); // insert repeat count
            }
            else {
                size_t r;
                for (r = 0; r<repeat; r++) *dest++ = last; // insert plain
            }
            last   = curr;
            repeat = 1;
        }
    }

    *dest = 0;

#if defined(DEBUG)

    gb_assert(strlen(dataStart) <= orgLen);
#endif // DEBUG
    return dest-dataStart;
}


// -------------------------------------------
//      helper function for tagged fields

static GB_ERROR g_bs_add_value_tag_to_hash(GBDATA *gb_main, GB_HASH *hash, char *tag, char *value, const char *rtag, const char *srt, const char *aci, GBDATA *gbd) {
    char *to_free = NULL;

    if (rtag && strcmp(tag, rtag) == 0) {
        if (srt) {
            value = to_free = GBS_string_eval(value, srt, gbd);
        }
        else if (aci) {
            value = to_free = GB_command_interpreter(gb_main, value, aci, gbd, 0);
        }
        if (!value) return GB_await_error();
    }

    {
        char *p;
        p = value; while ((p = strchr(p, '['))) *p =   '{'; // replace all '[' by '{'
        p = value; while ((p = strchr(p, ']'))) *p =   '}'; // replace all ']' by '}'
    }

    GB_HASH *sh = (GB_HASH *)GBS_read_hash(hash, value);
    if (!sh) {
        sh = GBS_create_hash(10, GB_IGNORE_CASE);        // Tags are case independent
        GBS_write_hash(hash, value, (long)sh);
    }

    GBS_write_hash(sh, tag, 1);
    free(to_free);
    return NULL;
}


static GB_ERROR g_bs_convert_string_to_tagged_hash(GB_HASH *hash, char *s, char *default_tag, const char *del,
                                                   GBDATA *gb_main, const char *rtag, const char *srt, const char *aci, GBDATA *gbd) {
    char *se;           // string end
    char *sa;           // string start and tag end
    char *ts;           // tag start
    char *t;
    GB_ERROR error = 0;
    while (s && s[0]) {
        ts = strchr(s, '[');
        if (!ts) {
            error = g_bs_add_value_tag_to_hash(gb_main, hash, default_tag, s, rtag, srt, aci, gbd); // no tag found, use default tag
            if (error) break;
            break;
        }
        else {
            *(ts++) = 0;
        }
        sa = strchr(ts, ']');
        if (sa) {
            *sa++ = 0;
            while (*sa == ' ') sa++;
        }
        else {
            error = g_bs_add_value_tag_to_hash(gb_main, hash, default_tag, s, rtag, srt, aci, gbd); // no tag found, use default tag
            if (error) break;
            break;
        }
        se = strchr(sa, '[');
        if (se) {
            while (se>sa && se[-1] == ' ') se--;
            *(se++) = 0;
        }
        for (t = strtok(ts, ","); t; t = strtok(0, ",")) {
            if (del && strcmp(t, del) == 0) continue; // test, whether to delete
            if (sa[0] == 0) continue;
            error = g_bs_add_value_tag_to_hash(gb_main, hash, t, sa, rtag, srt, aci, gbd); // tag found, use tag
            if (error) break;
        }
        s = se;
    }
    return error;
}

static long g_bs_merge_tags(const char *tag, long val, void *cd_sub_result) {
    GBS_strstruct *sub_result = (GBS_strstruct*)cd_sub_result;

    GBS_strcat(sub_result, tag);
    GBS_strcat(sub_result, ",");

    return val;
}

static long g_bs_read_tagged_hash(const char *value, long subhash, void *cd_g_bs_collect_tags_hash) {
    static int counter = 0;

    GBS_strstruct *sub_result = GBS_stropen(100);
    GBS_hash_do_sorted_loop((GB_HASH *)subhash, g_bs_merge_tags, GBS_HCF_sortedByKey, sub_result);
    GBS_intcat(sub_result, counter++); // create a unique number

    char *str = GBS_strclose(sub_result);

    GB_HASH *g_bs_collect_tags_hash = (GB_HASH*)cd_g_bs_collect_tags_hash;
    GBS_write_hash(g_bs_collect_tags_hash, str, (long)ARB_strdup(value)); // send output to new hash for sorting

    free(str);
    return subhash;
}

static long g_bs_read_final_hash(const char *tag, long value, void *cd_merge_result) {
    GBS_strstruct *merge_result = (GBS_strstruct*)cd_merge_result;

    char *lk = const_cast<char*>(strrchr(tag, ','));
    if (lk) {           // remove number at end
        *lk = 0;
        GBS_strcat(merge_result, " [");
        GBS_strcat(merge_result, tag);
        GBS_strcat(merge_result, "] ");
    }
    GBS_strcat(merge_result, (char *)value);
    return value;
}

static char *g_bs_get_string_of_tag_hash(GB_HASH *tag_hash) {
    GBS_strstruct *merge_result      = GBS_stropen(256);
    GB_HASH       *collect_tags_hash = GBS_create_dynaval_hash(512, GB_IGNORE_CASE, GBS_dynaval_free);

    GBS_hash_do_sorted_loop(tag_hash, g_bs_read_tagged_hash, GBS_HCF_sortedByKey, collect_tags_hash);     // move everything into collect_tags_hash
    GBS_hash_do_sorted_loop(collect_tags_hash, g_bs_read_final_hash, GBS_HCF_sortedByKey, merge_result);

    GBS_free_hash(collect_tags_hash);
    return GBS_strclose(merge_result);
}

static long g_bs_free_hash_of_hashes_elem(const char */*key*/, long val, void *) {
    GB_HASH *hash = (GB_HASH*)val;
    if (hash) GBS_free_hash(hash);
    return 0;
}
static void g_bs_free_hash_of_hashes(GB_HASH *hash) {
    GBS_hash_do_loop(hash, g_bs_free_hash_of_hashes_elem, NULL);
    GBS_free_hash(hash);
}

char *GBS_merge_tagged_strings(const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2) {
    /* Create a tagged string from two tagged strings:
     * a tagged string is something like '[tag,tag,tag] string [tag] string [tag,tag] string'
     *
     * if 's2' is not empty, then delete tag 'replace1' in 's1'
     * if 's1' is not empty, then delete tag 'replace2' in 's2'
     *
     * if result is NULL, an error has been exported.
     */

    char     *str1   = ARB_strdup(s1);
    char     *str2   = ARB_strdup(s2);
    char     *t1     = GBS_string_2_key(tag1);
    char     *t2     = GBS_string_2_key(tag2);
    GB_HASH  *hash   = GBS_create_hash(16, GB_MIND_CASE);

    if (!s1[0]) replace2 = NULL;
    if (!s2[0]) replace1 = NULL;

    if (replace1 && !replace1[0]) replace1 = NULL;
    if (replace2 && !replace2[0]) replace2 = NULL;

    GB_ERROR error    = g_bs_convert_string_to_tagged_hash(hash, str1, t1, replace1, 0, 0, 0, 0, 0);
    if (!error) error = g_bs_convert_string_to_tagged_hash(hash, str2, t2, replace2, 0, 0, 0, 0, 0);

    char *result = NULL;
    if (!error) {
        result = g_bs_get_string_of_tag_hash(hash);
    }
    else {
        GB_export_error(error);
    }

    g_bs_free_hash_of_hashes(hash);

    free(t2);
    free(t1);
    free(str2);
    free(str1);

    return result;
}

char *GBS_string_eval_tagged_string(GBDATA *gb_main, const char *s, const char *dt, const char *tag, const char *srt, const char *aci, GBDATA *gbd) {
    /* if 's' is untagged, tag it with default tag 'dt'.
     * if 'tag' is != NULL -> apply 'srt' or 'aci' to that part of the  content of 's', which is tagged with 'tag'
     *
     * if result is NULL, an error has been exported.
     */

    char     *str         = ARB_strdup(s);
    char     *default_tag = GBS_string_2_key(dt);
    GB_HASH  *hash        = GBS_create_hash(16, GB_MIND_CASE);
    char     *result      = 0;
    GB_ERROR  error       = g_bs_convert_string_to_tagged_hash(hash, str, default_tag, 0, gb_main, tag, srt, aci, gbd);

    if (!error) {
        result = g_bs_get_string_of_tag_hash(hash);
    }
    else {
        GB_export_error(error);
    }

    g_bs_free_hash_of_hashes(hash);
    free(default_tag);
    free(str);

    return result;
}


char *GB_read_as_tagged_string(GBDATA *gbd, const char *tagi) {
    char *s;
    char *tag;
    char *buf;
    char *se;           // string end
    char *sa;           // string anfang and tag end
    char *ts;           // tag start
    char *t;

    buf = s = GB_read_as_string(gbd);
    if (!s) return s;
    if (!tagi) return s;
    if (!strlen(tagi)) return s;

    tag = GBS_string_2_key(tagi);

    while (s) {
        ts = strchr(s, '[');
        if (!ts)    goto notfound;      // no tag

        *(ts++) = 0;

        sa = strchr(ts, ']');
        if (!sa) goto notfound;

        *sa++ = 0;
        while (*sa == ' ') sa++;

        se = strchr(sa, '[');
        if (se) {
            while (se>sa && se[-1] == ' ') se--;
            *(se++) = 0;
        }
        for (t = strtok(ts, ","); t; t = strtok(0, ",")) {
            if (strcmp(t, tag) == 0) {
                s = ARB_strdup(sa);
                free(buf);
                goto found;
            }
        }
        s = se;
    }
 notfound :
    // Nothing found
    free(buf);
    s = 0;
 found :
    free(tag);
    return s;
}


/* be CAREFUL : this function is used to save ARB ASCII database (i.e. properties)
 * used as well to save perl macros
 *
 * when changing GBS_fwrite_string -> GBS_fread_string needs to be fixed as well
 *
 * always keep in mind, that many users have databases/macros written with older
 * versions of this function. They MUST load proper!!!
 */
void GBS_fwrite_string(const char *strngi, FILE *out) {
    unsigned char *strng = (unsigned char *)strngi;
    int            c;

    putc('"', out);

    while ((c = *strng++)) {
        if (c < 32) {
            putc('\\', out);
            if (c == '\n')
                putc('n', out);
            else if (c == '\t')
                putc('t', out);
            else if (c<25) {
                putc(c+'@', out); // characters ASCII 0..24 encoded as \@..\X    (\n and \t are done above)
            }
            else {
                putc(c+('0'-25), out); // characters ASCII 25..31 encoded as \0..\6
            }
        }
        else if (c == '"') {
            putc('\\', out);
            putc('"', out);
        }
        else if (c == '\\') {
            putc('\\', out);
            putc('\\', out);
        }
        else {
            putc(c, out);
        }
    }
    putc('"', out);
}

/*  Read a string from a file written by GBS_fwrite_string,
 *  Searches first '"'
 *
 *  WARNING : changing this function affects perl-macro execution (read warnings for GBS_fwrite_string)
 *  any changes should be done in GBS_fconvert_string too.
 */

static char *GBS_fread_string(FILE *in) { // @@@ should be used when reading things written by GBS_fwrite_string, but it's unused!
    GBS_strstruct *strstr = GBS_stropen(1024);
    int            x;

    while ((x = getc(in)) != '"') if (x == EOF) break;  // Search first '"'

    if (x != EOF) {
        while ((x = getc(in)) != '"') {
            if (x == EOF) break;
            if (x == '\\') {
                x = getc(in); if (x==EOF) break;
                if (x == 'n') {
                    GBS_chrcat(strstr, '\n');
                    continue;
                }
                if (x == 't') {
                    GBS_chrcat(strstr, '\t');
                    continue;
                }
                if (x>='@' && x <= '@' + 25) {
                    GBS_chrcat(strstr, x-'@');
                    continue;
                }
                if (x>='0' && x <= '9') {
                    GBS_chrcat(strstr, x-('0'-25));
                    continue;
                }
                // all other backslashes are simply skipped
            }
            GBS_chrcat(strstr, x);
        }
    }
    return GBS_strclose(strstr);
}

/* does similar decoding as GBS_fread_string but works directly on an existing buffer
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

    gb_assert(f[-1] == '"');
    // the opening " has already been read

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
            if (x>='@' && x <= '@' + 25) {
                *t++ = x-'@';
                continue;
            }
            if (x>='0' && x <= '9') {
                *t++ = x-('0'-25);
                continue;
            }
            // all other backslashes are simply skipped
        }
        *t++ = x;
    }

    if (!x) return 0;           // error (string should not contain 0-character)
    gb_assert(x == '"');

    t[0] = 0;
    return f;
}

char *GBS_replace_tabs_by_spaces(const char *text) {
    int            tlen   = strlen(text);
    GBS_strstruct *mfile  = GBS_stropen(tlen * 3/2 + 1);
    int            tabpos = 0;
    int            c;

    while ((c=*(text++))) {
        if (c == '\t') {
            int ntab = (tabpos + 8) & 0xfffff8;
            while (tabpos < ntab) {
                GBS_chrcat(mfile, ' ');
                tabpos++;
            }
            continue;
        }
        tabpos ++;
        if (c == '\n') {
            tabpos = 0;
        }
        GBS_chrcat(mfile, c);
    }
    return GBS_strclose(mfile);
}

char *GBS_trim(const char *str) {
    // trim whitespace at beginning and end of 'str'
    const char *whitespace = " \t\n";
    while (str[0] && strchr(whitespace, str[0])) str++;

    const char *end = strchr(str, 0)-1;
    while (end >= str && strchr(whitespace, end[0])) end--;

    return ARB_strpartdup(str, end);
}

static char *dated_info(const char *info) {
    char *dated_info = 0;
    time_t      date;
    if (time(&date) != -1) {
        char *dstr = ctime(&date);
        char *nl   = strchr(dstr, '\n');

        if (nl) nl[0] = 0; // cut off LF

        dated_info = GBS_global_string_copy("%s: %s", dstr, info);
    }
    else {
        dated_info = ARB_strdup(info);
    }
    return dated_info;
}

char *GBS_log_dated_action_to(const char *comment, const char *action) {
    /*! appends 'action' prefixed by current timestamp to 'comment'
     * @param comment may be NULL. otherwise '\n' is appended (if not already there)
     * @param action may NOT be NULL. is prepended with current date. '\n' is appended (if not already there)
     */
    size_t clen = comment ? strlen(comment) : 0;
    size_t alen = strlen(action);

    GBS_strstruct *new_comment = GBS_stropen(clen+alen+100);

    if (comment) {
        GBS_strcat(new_comment, comment);
        if (clen == 0 || comment[clen-1] != '\n') GBS_chrcat(new_comment, '\n');
    }

    char *dated_action = dated_info(action);
    GBS_strcat(new_comment, dated_action);
    GBS_chrcat(new_comment, '\n');

    free(dated_action);

    return GBS_strclose(new_comment);
}

const char *GBS_funptr2readable(void *funptr, bool stripARBHOME) {
    // only returns module and offset for static functions :-(
    char       **funNames     = backtrace_symbols(&funptr, 1);
    const char  *readable_fun = funNames[0];

    if (stripARBHOME) {
        const char *ARBHOME = GB_getenvARBHOME();
        if (ARB_strBeginsWith(readable_fun, ARBHOME)) {
            readable_fun += strlen(ARBHOME)+1; // +1 hides slash behind ARBHOME
        }
    }
    return readable_fun;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

// #define TEST_TEST_MACROS

#ifdef ENABLE_CRASH_TESTS
static void provokesegv() { raise(SIGSEGV); }
static void dont_provokesegv() {}
# if defined(ASSERTION_USED)
static void failassertion() { gb_assert(0); }
#  if defined(TEST_TEST_MACROS)
static void dont_failassertion() {}
#  endif
static void provokesegv_does_not_fail_assertion() {
    // provokesegv does not raise assertion
    // -> the following assertion fails
    TEST_EXPECT_CODE_ASSERTION_FAILS(provokesegv);
}
# endif
#endif

void TEST_signal_tests() {
    // check whether we can test that no SEGV or assertion failure happened
    TEST_EXPECT_NO_SEGFAULT(dont_provokesegv);

    // check whether we can test for SEGV and assertion failures
    TEST_EXPECT_SEGFAULT(provokesegv);
    TEST_EXPECT_CODE_ASSERTION_FAILS(failassertion);

    // tests whether signal suppression works multiple times (by repeating tests)
    TEST_EXPECT_CODE_ASSERTION_FAILS(failassertion);
    TEST_EXPECT_SEGFAULT(provokesegv);

    // test whether SEGV can be distinguished from assertion
    TEST_EXPECT_CODE_ASSERTION_FAILS(provokesegv_does_not_fail_assertion);

    // The following section is disabled, because it will
    // provoke test warnings (to test these warnings).
    // (enable it when changing any of these TEST_..-macros used here)
#if defined(TEST_TEST_MACROS)
    TEST_EXPECT_NO_SEGFAULT__WANTED(provokesegv);

    TEST_EXPECT_SEGFAULT__WANTED(dont_provokesegv);
    TEST_EXPECT_SEGFAULT__UNWANTED(provokesegv);
#if defined(ASSERTION_USED)
    TEST_EXPECT_SEGFAULT__UNWANTED(failassertion);
#endif

    TEST_EXPECT_CODE_ASSERTION_FAILS__WANTED(dont_failassertion);
    TEST_EXPECT_CODE_ASSERTION_FAILS__UNWANTED(failassertion);
    TEST_EXPECT_CODE_ASSERTION_FAILS__UNWANTED(provokesegv_does_not_fail_assertion);
#endif
}

#define EXPECT_CONTENT(content) TEST_EXPECT_EQUAL(GBS_mempntr(strstr), content)

void TEST_GBS_strstruct() {
    {
        GBS_strstruct *strstr = GBS_stropen(1000); EXPECT_CONTENT("");

        GBS_chrncat(strstr, 'b', 3);        EXPECT_CONTENT("bbb");
        GBS_intcat(strstr, 17);             EXPECT_CONTENT("bbb17");
        GBS_chrcat(strstr, '_');            EXPECT_CONTENT("bbb17_");
        GBS_floatcat(strstr, 3.5);          EXPECT_CONTENT("bbb17_3.500000");

        TEST_EXPECT_EQUAL(GBS_memoffset(strstr), 14);
        GBS_str_cut_tail(strstr, 13);       EXPECT_CONTENT("b");
        GBS_strcat(strstr, "utter");        EXPECT_CONTENT("butter");
        GBS_strncat(strstr, "flying", 3);   EXPECT_CONTENT("butterfly");

        GBS_strnprintf(strstr, 200, "%c%s", ' ', "flutters");
        EXPECT_CONTENT("butterfly flutters");

        GBS_strforget(strstr);
    }
    {
        // re-alloc smaller
        GBS_strstruct *strstr = GBS_stropen(500); EXPECT_CONTENT("");
        GBS_strforget(strstr);
    }

    // trigger downsize of oversized block
    for (int i = 0; i<12; ++i) {
        GBS_strstruct *strstr = GBS_stropen(10);
        GBS_strforget(strstr);
    }

    {
        GBS_strstruct *strstr     = GBS_stropen(10);
        size_t         oldbufsize = strstr->get_buffer_size();
        GBS_chrncat(strstr, 'x', 20);               // trigger reallocation of buffer

        TEST_EXPECT_DIFFERENT(oldbufsize, strstr->get_buffer_size()); // did we reallocate?
        EXPECT_CONTENT("xxxxxxxxxxxxxxxxxxxx");
        GBS_strforget(strstr);
    }
}

#define TEST_SHORTENED_EQUALS(Long,Short) do {  \
        char *buf = ARB_strdup(Long);           \
        GBS_shorten_repeated_data(buf);         \
        TEST_EXPECT_EQUAL(buf, Short);          \
        free(buf);                              \
    } while(0)

void TEST_GBS_shorten_repeated_data() {
    TEST_SHORTENED_EQUALS("12345", "12345"); 
    TEST_SHORTENED_EQUALS("aaaaaaaaaaaabc", "a{12}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaaaaaaabc", "a{11}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaaaaaabc", "a{10}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaaaaabc", "a{9}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaaaabc", "a{8}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaaabc", "a{7}bc"); 
    TEST_SHORTENED_EQUALS("aaaaaabc", "a{6}bc"); 
    TEST_SHORTENED_EQUALS("aaaaabc", "a{5}bc"); 
    TEST_SHORTENED_EQUALS("aaaabc", "aaaabc"); 
    TEST_SHORTENED_EQUALS("aaabc", "aaabc"); 
    TEST_SHORTENED_EQUALS("aabc", "aabc"); 
    TEST_SHORTENED_EQUALS("", "");
}

static const char *hkey_format[] = {
    "/%s/bbb/ccc",
    "/aaa/%s/ccc",
    "/aaa/bbb/%s",
};

inline const char *useInHkey(const char *fragment, size_t pos) {
    return GBS_global_string(hkey_format[pos], fragment);
}

#define TEST_IN_HKEYS_USING_EXPECT_NO_ERROR(use) do {                   \
        for (size_t i = 0; i<ARRAY_ELEMS(hkey_format); ++i) {           \
            TEST_EXPECT_NO_ERROR(GB_check_hkey(useInHkey(use, i)));     \
        }                                                               \
    } while(0)

#define TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(use,contains) do {                    \
        for (size_t i = 0; i<ARRAY_ELEMS(hkey_format); ++i) {                           \
            TEST_EXPECT_ERROR_CONTAINS(GB_check_hkey(useInHkey(use, i)), contains);     \
        }                                                                               \
    } while(0)


void TEST_DB_key_checks() {
    // plain keys
    const char *shortest  = "ab";
    const char *too_long  = "ab345678901234567890123456789012345678901234567890123456789012345";
    const char *too_short = shortest+1;
    const char *longest   = too_long+1;

    const char *empty = "";
    const char *slash = "sub/key";
    const char *comma = "no,key";
    const char *minus = "no-key";

    TEST_EXPECT_NO_ERROR(GB_check_key(shortest));
    TEST_EXPECT_NO_ERROR(GB_check_key(longest));

    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(too_short), "too short");
    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(too_long),  "too long");
    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(empty),     "not allowed");

    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(slash), "Invalid character");
    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(comma), "Invalid character");
    TEST_EXPECT_ERROR_CONTAINS(GB_check_key(minus), "Invalid character");

    // hierarchical keys
    TEST_IN_HKEYS_USING_EXPECT_NO_ERROR(shortest);
    TEST_IN_HKEYS_USING_EXPECT_NO_ERROR(longest);

    TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(too_short, "too short");
    TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(too_long,  "too long");
    TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(empty,     "not allowed");

    TEST_IN_HKEYS_USING_EXPECT_NO_ERROR(slash);
    TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(comma, "Invalid character ','");
    TEST_IN_HKEYS_USING_EXPECT_ERROR_CONTAINS(minus, "'>' expected after '-'");
}

#define TEST_STRING2KEY(str,expected) do {              \
        char *as_key = GBS_string_2_key(str);           \
        TEST_EXPECT_EQUAL(as_key, expected);            \
        TEST_EXPECT_NO_ERROR(GB_check_key(as_key));     \
        free(as_key);                                   \
    } while(0)

void TEST_DB_key_generation() {
    TEST_STRING2KEY("abc", "abc");
    TEST_STRING2KEY("a b c", "a_b_c");

    // invalid chars
    TEST_STRING2KEY("string containing \"double-quotes\", 'quotes' and other:shit!*&^@!%@(",
                    "string_containing_doublequotes_quotes_and_othershit");

    // length tests
    TEST_STRING2KEY("a", "a_");                                                          // too short
    TEST_STRING2KEY("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", // too long
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
}

#define TEST_MERGE_TAGGED(t1,t2,r1,r2,s1,s2,expected) do {               \
        char *result = GBS_merge_tagged_strings(s1, t1, r1, s2, t2, r2); \
        TEST_EXPECT_EQUAL(result, expected);                             \
        free(result);                                                    \
    } while(0)

#define TEST_MERGE_TAGGED__BROKEN(t1,t2,r1,r2,s1,s2,expected,got) do {   \
        char *result = GBS_merge_tagged_strings(s1, t1, r1, s2, t2, r2); \
        TEST_EXPECT_EQUAL__BROKEN(result, expected, got);                \
        free(result);                                                    \
    } while(0)

void TEST_merge_tagged_strings() {
    // merge two fields:
    TEST_MERGE_TAGGED("S",   "D",   "", "", "source", "dest", " [D_] dest [S_] source");   // @@@ elim leading space?
    TEST_MERGE_TAGGED("SRC", "DST", "", 0,  "source", "dest", " [DST] dest [SRC] source");
    TEST_MERGE_TAGGED("SRC", "DST", 0,  "", "source", "dest", " [DST] dest [SRC] source");
    TEST_MERGE_TAGGED("SRC", "DST", 0,  0,  "sth",    "sth",  " [DST,SRC] sth");

    // update fields:
    TEST_MERGE_TAGGED("SRC", "DST", 0, "SRC", "newsource", " [DST] dest [SRC] source", " [DST] dest [SRC] newsource");
    TEST_MERGE_TAGGED("SRC", "DST", 0, "SRC", "newsource", " [DST,SRC] sth",           " [DST] sth [SRC] newsource");
    TEST_MERGE_TAGGED("SRC", "DST", 0, "SRC", "sth",       " [DST] sth [SRC] source",  " [DST,SRC] sth");

    // append (opposed to update this keeps old entries with same tag; useless?)
    TEST_MERGE_TAGGED("SRC", "DST", 0, 0, "newsource", "[DST] dest [SRC] source", " [DST] dest [SRC] newsource [SRC] source");
    TEST_MERGE_TAGGED("SRC", "DST", 0, 0, "newsource", "[DST,SRC] sth",           " [DST,SRC] sth [SRC] newsource");
    TEST_MERGE_TAGGED("SRC", "DST", 0, 0, "sth",       "[DST] sth [SRC] source",  " [DST,SRC] sth [SRC] source");

    // merge three fields:
    TEST_MERGE_TAGGED("OTH", "DST", 0, 0, "oth",    " [DST] dest [SRC] source", " [DST] dest [OTH] oth [SRC] source");
    TEST_MERGE_TAGGED("OTH", "DST", 0, 0, "oth",    " [DST,SRC] sth",           " [DST,SRC] sth [OTH] oth");
    TEST_MERGE_TAGGED("OTH", "DST", 0, 0, "sth",    " [DST,SRC] sth",           " [DST,OTH,SRC] sth");
    TEST_MERGE_TAGGED("OTH", "DST", 0, 0, "dest",   " [DST] dest [SRC] source", " [DST,OTH] dest [SRC] source");
    TEST_MERGE_TAGGED("OTH", "DST", 0, 0, "source", " [DST] dest [SRC] source", " [DST] dest [OTH,SRC] source");

    // same tests as in section above, but vv
    TEST_MERGE_TAGGED("DST", "OTH", 0, 0, " [DST] dest [SRC] source", "oth",    " [DST] dest [OTH] oth [SRC] source");
    TEST_MERGE_TAGGED("DST", "OTH", 0, 0, " [DST,SRC] sth",           "oth",    " [DST,SRC] sth [OTH] oth");
    TEST_MERGE_TAGGED("DST", "OTH", 0, 0, " [DST,SRC] sth",           "sth",    " [DST,OTH,SRC] sth");
    TEST_MERGE_TAGGED("DST", "OTH", 0, 0, " [DST] dest [SRC] source", "dest",   " [DST,OTH] dest [SRC] source");
    TEST_MERGE_TAGGED("DST", "OTH", 0, 0, " [DST] dest [SRC] source", "source", " [DST] dest [OTH,SRC] source");
}

void TEST_date_stamping() {
    {
        char *dated = GBS_log_dated_action_to("comment", "action");
        TEST_EXPECT_CONTAINS(dated, "comment\n");
        TEST_EXPECT_CONTAINS(dated, "action\n");
        free(dated);
    }
    {
        char *dated = GBS_log_dated_action_to("", "action");
        TEST_EXPECT_EQUAL(dated[0], '\n');
        TEST_EXPECT_CONTAINS(dated, "action\n");
        free(dated);
    }
    {
        char *dated = GBS_log_dated_action_to(NULL, "action");
        TEST_EXPECT_DIFFERENT(dated[0], '\n');
        TEST_EXPECT_CONTAINS(dated, "action\n");
        free(dated);
    }
}
TEST_PUBLISH(TEST_date_stamping);

#endif // UNIT_TESTS

