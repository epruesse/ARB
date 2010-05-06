// =============================================================== //
//                                                                 //
//   File      : ad_load.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cctype>
#include <ctime>
#include <netinet/in.h>
#include <sys/stat.h>

#include <arbdbt.h>

#include "gb_storage.h"
#include "gb_localdata.h"
#include "gb_map.h"
#include "gb_load.h"

static int gb_verbose_mode = 0;
void GB_set_verbose() {
    gb_verbose_mode = 1;
}

// ---------------------------------------------------
//      helper code to read ascii file in portions

#if (UNIT_TESTS == 1)
#define READING_BUFFER_SIZE 100
#else
#define READING_BUFFER_SIZE (1024*32)
#endif

struct ReadingBuffer {
    char          *data;
    ReadingBuffer *next;
    int            read_bytes;
};

static ReadingBuffer *unused_reading_buffers = 0;

#if defined(DEBUG)
// #define CHECK_RELEASED_BUFFERS
#endif // DEBUG

#if defined(CHECK_RELEASED_BUFFERS)
static int is_a_unused_reading_buffer(ReadingBuffer *rb) {
    if (unused_reading_buffers) {
        ReadingBuffer *check = unused_reading_buffers;
        for (; check; check = check->next) {
            if (check == rb) return 1;
        }
    }
    return 0;
}
#endif // CHECK_RELEASED_BUFFERS

static ReadingBuffer *allocate_ReadingBuffer() {
    ReadingBuffer *rb = (ReadingBuffer*)malloc(sizeof(*rb)+READING_BUFFER_SIZE);
    rb->data          = ((char*)rb)+sizeof(*rb);
    rb->next          = 0;
    rb->read_bytes    = 0;
    return rb;
}

static void release_ReadingBuffers(ReadingBuffer *rb) {
    ReadingBuffer *last = rb;

#if defined(CHECK_RELEASED_BUFFERS)
    gb_assert(!is_a_unused_reading_buffer(rb));
#endif // CHECK_RELEASED_BUFFERS
    while (last->next) {
        last = last->next;
#if defined(CHECK_RELEASED_BUFFERS)
        gb_assert(!is_a_unused_reading_buffer(last));
#endif // CHECK_RELEASED_BUFFERS
    }
    gb_assert(last && !last->next);
    last->next              = unused_reading_buffers;
    unused_reading_buffers  = rb;
}
static void free_ReadingBuffer(ReadingBuffer *rb) {
    if (rb) {
        if (rb->next) free_ReadingBuffer(rb->next);
        free(rb);
    }
}

static ReadingBuffer *read_another_block(FILE *in) {
    ReadingBuffer *buf = 0;
    if (unused_reading_buffers) {
        buf                    = unused_reading_buffers;
        unused_reading_buffers = buf->next;
        buf->next              = 0;
        buf->read_bytes        = 0;
    }
    else {
        buf = allocate_ReadingBuffer();
    }

    buf->read_bytes = fread(buf->data, 1, READING_BUFFER_SIZE, in);

    return buf;
}

// ---------------
//      Reader

struct Reader {
    FILE          *in;
    ReadingBuffer *first;                           // allocated
    GB_ERROR       error;

    ReadingBuffer *current;                         // only reference
    size_t         current_offset;                  // into 'current->data'

    char   *current_line;
    int     current_line_allocated;                 // whether 'current_line' was allocated
    size_t  current_line_size;                      // size of 'current_line' (valid if current_line_allocated == 1)
    size_t  line_number;

};

typedef unsigned long ReaderPos; // absolute position (relative to ReadingBuffer 'first')
#define NOPOS (-1UL)


static Reader *openReader(FILE *in) {
    Reader *r = (Reader*)malloc(sizeof(*r));

    gb_assert(unused_reading_buffers == 0);

    r->in    = in;
    r->error = 0;
    r->first = read_another_block(r->in);

    r->current_offset = 0;
    r->current        = r->first;

    r->current_line           = 0;
    r->current_line_allocated = 0;
    r->line_number            = 0;

    return r;
}

static void freeCurrentLine(Reader *r) {
    if (r->current_line_allocated && r->current_line) {
        free(r->current_line);
        r->current_line_allocated = 0;
    }
}

static GB_ERROR closeReader(Reader *r) {
    GB_ERROR error = r->error;

    free_ReadingBuffer(r->first);
    free_ReadingBuffer(unused_reading_buffers);
    unused_reading_buffers = 0;

    freeCurrentLine(r);
    free(r);

    return error;
}

static void releasePreviousBuffers(Reader *r) {
    /* Release all buffers before current position.
     * Warning: This invalidates all offsets!
     */
    ReadingBuffer *last_rel  = 0;
    ReadingBuffer *old_first = r->first;

    while (r->first != r->current) {
        last_rel = r->first;
        r->first = r->first->next;
    }

    if (last_rel) {
        last_rel->next = 0;     // avoid to release 'current'
        release_ReadingBuffers(old_first);
        r->first       = r->current;
    }
}

static char *getPointer(const Reader *r) {
    return r->current->data + r->current_offset;
}

static ReaderPos getPosition(const Reader *r) {
    ReaderPos      p = 0;
    ReadingBuffer *b = r->first;
    while (b != r->current) {
        gb_assert(b);
        p += b->read_bytes;
        b  = b->next;
    }
    p += r->current_offset;
    return p;
}

static int gotoNextBuffer(Reader *r) {
    if (!r->current->next) {
        if (r->current->read_bytes < READING_BUFFER_SIZE) { // eof
            return 0;
        }
        r->current->next = read_another_block(r->in);
    }

    r->current        = r->current->next;
    r->current_offset = 0;

    return r->current != 0;
}

static int movePosition(Reader *r, int offset) {
    int rest = r->current->read_bytes - r->current_offset - 1;

    gb_assert(offset >= 0);     // not implemented for negative offsets
    if (rest >= offset) {
        r->current_offset += offset;
        return 1;
    }

    if (gotoNextBuffer(r)) {
        offset -= rest+1;
        return offset == 0 ? 1 : movePosition(r, offset);
    }

    // in last buffer and position behind end -> position after last element
    // (Note: last buffer cannot be full - only empty)
    r->current_offset = r->current->read_bytes;
    return 0;
}

static int gotoChar(Reader *r, char lookfor) {
    const char *data  = r->current->data + r->current_offset;
    size_t      size  = r->current->read_bytes-r->current_offset;
    const char *found = (const char *)memchr(data, lookfor, size);

    if (found) {
        r->current_offset += (found-data);
        return 1;
    }

    if (gotoNextBuffer(r)) {
        return gotoChar(r, lookfor);
    }

    // in last buffer and char not found -> position after last element
    // (Note: last buffer cannot be full - only empty)
    r->current_offset = r->current->read_bytes;
    return 0;
}

char *getLine(Reader *r) {
    releasePreviousBuffers(r);

    {
        ReaderPos      start        = getPosition(r);
        ReadingBuffer *start_buffer = r->current;
        char          *start_ptr    = getPointer(r);
        int            eol_found    = gotoChar(r, '\n');

        // now current position is on EOL or EOF

        ReaderPos      eol        = getPosition(r);
        ReadingBuffer *eol_buffer = r->current;
        char          *eol_ptr    = getPointer(r);

        movePosition(r, 1);

        if (start_buffer == eol_buffer) { // start and eol in one ReadingBuffer -> no copy
            freeCurrentLine(r);
            r->current_line           = start_ptr;
            r->current_line_allocated = 0;
            eol_ptr[0]                = 0; // eos
        }
        else { // otherwise build a copy of the string
            size_t  line_length = eol-start+1;
            char   *bp;
            size_t  len;

            if (r->current_line_allocated == 0 || r->current_line_size < line_length) { // need alloc
                freeCurrentLine(r);
                r->current_line           = (char*)malloc(line_length);
                r->current_line_size      = line_length;
                r->current_line_allocated = 1;

                gb_assert(r->current_line);
            }

            // copy contents of first buffer
            bp            = r->current_line;
            len           = start_buffer->read_bytes - (start_ptr-start_buffer->data);
            memcpy(bp, start_ptr, len);
            bp           += len;
            start_buffer  = start_buffer->next;

            // copy contents of middle buffers
            while (start_buffer != eol_buffer) {
                memcpy(bp, start_buffer->data, start_buffer->read_bytes);
                bp           += start_buffer->read_bytes;
                start_buffer  = start_buffer->next;
            }

            // copy contents from last buffer
            len        = eol_ptr-start_buffer->data;
            memcpy(bp, start_buffer->data, len);
            bp        += len;
            bp[0]  = 0; // eos
        }

        if (!eol_found && r->current_line[0] == 0)
            return 0;               // signal eof

    }

    ++r->line_number;
    return r->current_line;
}

/* ----------------------------------------
 * ASCII format
 *
 * Versions:
 *
 * V0  - 20.6.95
 * V1  Full save
 * V2  Differential save
 * V3  May read from stdin. Skipped support for old ASCII format.
 */

static char *getToken(char **line) {
    char *token  = *line;
    (*line)     += strcspn(*line, " \t");

    if ((*line)[0]) {                               // sth follows behind token
        (*line)[0]  = 0;                            // terminate token
        (*line)++;
        (*line)    += strspn(*line, " \t");         // goto next token
    }

    return token;
}

static GB_ERROR set_protection_level(GB_MAIN_TYPE *Main, GBDATA *gbd, const char *p) {
    int      secr, secw, secd, lu;
    GB_ERROR error = 0;

    secr = secw = secd = 0;
    lu   = 0;

    if (p && p[0] == ':') {
        long i;

        secd = p[1]; A_TO_I(secd);
        secw = p[2]; A_TO_I(secw);
        secr = p[3]; A_TO_I(secr);

        if (secd<0 || secd>7) error = GBS_global_string("Illegal protection level %i", secd);
        else if (secw<0 || secw>7) error = GBS_global_string("Illegal protection level %i", secw);
        else if (secr<0 || secr>7) error = GBS_global_string("Illegal protection level %i", secr);

        lu = atoi(p+4);

        for (i=Main->last_updated; i<=lu; ++i) {
            Main->dates[i] = strdup("unknown date");
            Main->last_updated = lu+1;
        }
    }

    if (!error) {
        gbd->flags.security_delete = secd;
        gbd->flags.security_write  = secw;
        gbd->flags.security_read   = secr;
        gbd->flags2.last_updated   = lu;
    }

    return error;
}

static GB_ERROR gb_parse_ascii_rek(Reader *r, GBCONTAINER *gb_parent, const char *parent_name) {
    // if parent_name == 0 -> we are parsing at root-level
    GB_ERROR      error = 0;
    int           done  = 0;
    GB_MAIN_TYPE *Main  = GBCONTAINER_MAIN(gb_parent);

    while (!error && !done) {
        char *line = getLine(r);
        if (!line) break;

    rest :
        line += strspn(line, " \t"); // goto first non-whitespace

        if (line[0]) {          // not empty
            if (line[0] == '/' && line[1] == '*') { // comment
                char *eoc = strstr(line+2, "*/");
                if (eoc) {
                    line = eoc+2;
                    goto rest;
                }
                error = "expected '*/'";
            }
            else { // real content
                char *name = getToken(&line);

                if (name[0] == '%' && name[1] == ')' && !name[2]) { // close container
                    if (!parent_name) {
                        error = "Unexpected '%)' (not allowed outside container)";
                    }
                    else {
                        if (line[0] == '/' && line[1] == '*') { // comment at container-end
                            char *eoc  = strstr(line+2, "*/");
                            if (!eoc) {
                                error = "expected '*/'"; 
                            }
                            else {
                                line += 2;
                                *eoc  = 0;
                                if (strcmp(line, parent_name) != 0) {
                                    fprintf(stderr,
                                            "Warning: comment at end of container ('%s') does not match name of container ('%s').\n"
                                            "         (might be harmless if you've edited the file and did not care about these comments)\n",
                                            line, parent_name); 
                                }
                                line = eoc+2;
                            }
                        }
                        done = 1;
                    }
                }
                else {
                    char *protection = 0;
                    if (line[0] == ':') {           // protection level
                        protection = getToken(&line);
                    }

                    bool readAsString = true;
                    if (line[0] == '%') {
                        readAsString = false;

                        char *type   = getToken(&line);

                        if (type[1] == 0 || type[2] != 0) {
                            error = GBS_global_string("Syntax error in type '%s' (expected %% and 1 more character)", type);
                        }
                        else {
                            if (type[1] == '%') {   // container
                                if (line[0] == '(' && line[1] == '%') {
                                    char        *cont_name = strdup(name);
                                    GBCONTAINER *gbc       = gb_make_container(gb_parent, cont_name, -1, 0);

                                    protection = protection ? strdup(protection) : 0;
                                    error      = gb_parse_ascii_rek(r, gbc, cont_name);
                                    // caution: most buffer variables are invalidated NOW!

                                    if (!error) error = set_protection_level(Main, (GBDATA*)gbc, protection);

                                    free(protection);
                                    free(cont_name);
                                }
                                else {
                                    error = "Expected '(%' after '%%'";
                                }
                            }
                            else {
                                GB_TYPES gb_type = GB_NONE;

                                switch (type[1]) {
                                    case 'i': gb_type = GB_INT; break;
                                    case 'l': gb_type = GB_LINK; break;
                                    case 'y': gb_type = GB_BYTE; break;
                                    case 'f': gb_type = GB_FLOAT; break;
                                    case 'I': gb_type = GB_BITS; break;

                                    case 'Y': gb_type = GB_BYTES; break;
                                    case 'N': gb_type = GB_INTS; break;
                                    case 'F': gb_type = GB_FLOATS; break;
                                    case 's': gb_type = GB_STRING; readAsString = true; break; // special case (string starts with '%')

                                    default:
                                        error = GBS_global_string("Unknown type '%s'", type);
                                        break;
                                }

                                if (!error && !readAsString) {
                                    GBDATA *gb_new;

                                    gb_assert(gb_type != GB_NONE);
                                    gb_new = gb_make_entry(gb_parent, name, -1, 0, gb_type);
                                    if (!gb_new) error = GB_await_error();
                                    else {
                                        switch (type[1]) {
                                            case 'i': error = GB_write_int(gb_new, atoi(line)); break;
                                            case 'l': error = GB_write_link(gb_new, line); break;
                                            case 'y': error = GB_write_byte(gb_new, atoi(line)); break;
                                            case 'f': error = GB_write_float(gb_new, GB_atof(line)); break;
                                            case 'I': {
                                                int len = strlen(line);
                                                gb_assert(line[0] == '\"');
                                                gb_assert(line[len-1] == '\"');
                                                error   = GB_write_bits(gb_new, line+1, len-2, "-"); break;
                                            }

                                            case 'Y': if (gb_ascii_2_bin(line, gb_new)) error = "syntax error in byte-array"; break;
                                            case 'N': if (gb_ascii_2_bin(line, gb_new)) error = "syntax error in int-array"; break;
                                            case 'F': if (gb_ascii_2_bin(line, gb_new)) error = "syntax error in float-array"; break;

                                            default: gb_assert(0);  // forgot a case ?
                                        }

                                        if (!error) error = set_protection_level(Main, gb_new, protection);
                                    }
                                }
                            }
                        }
                    }
                    
                    if (readAsString) {
                        if (line[0] != '\"') {
                            error = GBS_global_string("Unexpected content '%s'", line);
                        }
                        else {                      // string entry
                            char *string_start = line+1;
                            char *end          = GBS_fconvert_string(string_start);

                            if (!end) error = "Cannot convert string (contains zero char)";
                            else {
                                GBDATA *gb_string     = gb_make_entry(gb_parent, name, -1, 0, GB_STRING);
                                if (!gb_string) error = GB_await_error();
                                else {
                                    error             = GB_write_string(gb_string, string_start);
                                    if (!error) error = set_protection_level(Main, gb_string, protection);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return error;
}

static GB_ERROR gb_parse_ascii(Reader *r, GBCONTAINER *gb_parent) {
    GB_ERROR error = gb_parse_ascii_rek(r, gb_parent, 0);
    if (error) {
        error = GBS_global_string("%s in line %zu", error, r->line_number);
    }
    return error;
}

GB_ERROR gb_read_ascii(const char *path, GBCONTAINER *gbd) {
    /* This loads an ACSII database
     * if path == "-" -> read from stdin
     */

    FILE     *in         = 0;
    GB_ERROR  error      = 0;
    int       close_file = 0;

    if (strcmp(path, "-") == 0) {
        in = stdin;
    }
    else {
        in              = fopen(path, "rt");
        if (!in) error  = GBS_global_string("Can't open '%s'", path);
        else close_file = 1;
    }

    if (!error) {
        Reader   *r        = openReader(in);
        GB_ERROR  cl_error = 0;

        GB_search((GBDATA *)gbd, GB_SYSTEM_FOLDER, GB_CREATE_CONTAINER); // Switch to Version 3

        error = gb_parse_ascii(r, gbd);

        cl_error = closeReader(r);
        if (!error) error = cl_error;
    }

    if (close_file) fclose(in);
    return error;
}

// --------------------------
//      Read binary files

long gb_read_bin_rek(FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed)
{
    long          item;
    long          type, type2;
    GBQUARK       key;
    char         *p;
    long          i;
    int           c;
    long          size;
    long          memsize;
    GBDATA       *gb2;
    GBCONTAINER  *gbc  = 0;
    long          security;
    char         *buff;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    gb_create_header_array(gbd, (int)nitems);

    for (item = 0; item<nitems; item++) {
        type = getc(in);
        security = getc(in);
        type2 = (type>>4)&0xf;
        key = getc(in);
        if (!key) {
            p = buff = GB_give_buffer(256);
            for (i=0; i<256; i++) {
                c = getc(in);
                *(p++) = c;
                if (!c) break;
                if (c==EOF) {
                    gb_read_bin_error(in, (GBDATA *)gbd, "Unexpected EOF found");
                    return -1;
                }
            }
            if (i>GB_KEY_LEN_MAX*2) {
                gb_read_bin_error(in, (GBDATA *)gbd, "Key to long");
                return -1;
            }
            if (type2 == (long)GB_DB) {
                gbc = gb_make_container(gbd, buff, -1, 0);
                gb2 = (GBDATA *)gbc;
            }
            else {
                gb2 = gb_make_entry(gbd, buff, -1, 0, (GB_TYPES)type2);
            }
        }
        else {
            if (type2 == (long)GB_DB) {
                gbc = gb_make_container(gbd, NULL, -1, key);
                gb2 = (GBDATA *)gbc;
            }
            else {
                gb2 = gb_make_entry(gbd, NULL, -1, (GBQUARK)key, (GB_TYPES)type2);
            }
            if (!Main->keys[key].key) {
                GB_internal_error("Some database fields have no field identifier -> setting to 'main'");
                gb_write_index_key(GB_FATHER(gbd), gbd->index, 0);
            }
        }
        gb2->flags.security_delete  = type >> 1;
        gb2->flags.security_write   = ((type&1) << 2) + (security >> 6);
        gb2->flags.security_read    = security >> 3;
        gb2->flags.compressed_data  = security >> 2;
        GB_ARRAY_FLAGS(((GBCONTAINER*)gb2)).flags = (int)((security >> 1) & 1);
        gb2->flags.unused       = security >> 0;
        gb2->flags2.last_updated = getc(in);

        switch (type2) {
            case GB_INT:
                {
                    GB_UINT4 buffer;
                    if (!fread((char*)&buffer, sizeof(GB_UINT4), 1, in)) {
                        GB_export_error("File too short, seems truncated");
                        return -1;
                    }
                    gb2->info.i = ntohl(buffer);
                    break;
                }
            case GB_FLOAT:
                gb2->info.i = 0;
                if (!fread((char*)&gb2->info.i, sizeof(float), 1, in)) {
                    return -1;
                }
                break;
            case GB_STRING_SHRT:
                p = buff = GB_give_buffer(GBTUM_SHORT_STRING_SIZE+2);
                for (size=0; size<=GBTUM_SHORT_STRING_SIZE; size++) {
                    if (!(*(p++) = getc(in))) break;
                }
                *p=0;
                GB_SETSMDMALLOC(gb2, size, size+1, buff);
                break;
            case GB_STRING:
            case GB_LINK:
            case GB_BITS:
            case GB_BYTES:
            case GB_INTS:
            case GB_FLOATS:
                size = gb_read_in_long(in, reversed);
                memsize =  gb_read_in_long(in, reversed);
                if (GB_CHECKINTERN(size, memsize)) {
                    GB_SETINTERN(gb2);
                    p = &(gb2->info.istr.data[0]);
                }
                else {
                    GB_SETEXTERN(gb2);
                    p = GB_give_buffer(memsize);
                }
                i = fread(p, 1, (size_t)memsize, in);
                if (i!=memsize) {
                    gb_read_bin_error(in, gb2, "Unexpected EOF found");
                    return -1;
                }
                GB_SETSMDMALLOC(gb2, size, memsize, p);
                break;
            case GB_DB:
                size = gb_read_in_long(in, reversed);
                // gbc->d.size  is automatically incremented
                memsize = gb_read_in_long(in, reversed);
                if (gb_read_bin_rek(in, gbc, size, version, reversed)) return -1;
                break;
            case GB_BYTE:
                gb2->info.i = getc(in);
                break;
            default:
                gb_read_bin_error(in, gb2, "Unknown type");
                return -1;
        }
    }
    return 0;
}


long gb_recover_corrupt_file(GBCONTAINER *gbd, FILE *in, GB_ERROR recovery_reason) {
    // search pattern dx xx xx xx string 0
    static FILE *old_in = 0;
    static unsigned char *file = 0;
    static long size = 0;
    long pos = ftell(in);
    if (!GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
        if (!recovery_reason) { recovery_reason = GB_await_error(); }
        GB_export_errorf("Aborting recovery (because recovery mode is disabled)\n"
                         "Error causing recovery: '%s'\n"
                         "Part of data may be recovered using 'arb_repair yourDB.arb newName.arb'\n"
                         "(Note: Recovery doesn't work if the error occurs while loading a quick save file)",
                         recovery_reason);
        return -1;
    }
    pos = ftell(in);
    if (old_in != in) {
        file = (unsigned char *)GB_map_FILE(in, 0);
        old_in = in;
        size = GB_size_of_FILE(in);
    }
    for (; pos<size-10; pos ++) {
        if ((file[pos] & 0xf0) == (GB_STRING_SHRT<<4)) {
            long s;
            int c;
            for (s = pos + 4; s<size && file[s]; s++) {
                c = file[s];
                if (! (isalnum(c) || isspace(c) || strchr("._;:,", c))) break;
            }
            if (s< size && s > pos+11 && !file[s]) {    // we found something
                gb_local->search_system_folder = 1;
                return fseek(in, pos, 0);
            }
        }
    }
    return -1;          // no short string found
}

// #define DEBUG_READ
#if defined(DEBUG_READ)

static void DEBUG_DUMP_INDENTED(long deep, const char *s) {
    printf("%*s%s\n", (int)deep, "", s);
}

#else

#define DEBUG_DUMP_INDENTED(d, s)

#endif // DEBUG_READ


long gb_read_bin_rek_V2(FILE *in, GBCONTAINER *gbd, long nitems, long version, long reversed, long deep)
{
    long          item;
    long          type, type2;
    GBQUARK       key;
    char         *p;
    long          i;
    long          size;
    long          memsize;
    int           index;
    GBDATA       *gb2;
    GBCONTAINER  *gbc;
    long          security;
    char         *buff;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    struct gb_header_list_struct *header;

    DEBUG_DUMP_INDENTED(deep, GBS_global_string("Reading container with %li items", nitems));

    gb_create_header_array(gbd, (int)nitems);
    header = GB_DATA_LIST_HEADER(gbd->d);
    if (deep == 0 && GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
        GB_warning("Read to end of file in recovery mode");
        nitems = 10000000; // read forever at highest level
    }

    for (item = 0; item<nitems; item++)
    {
        type = getc(in);
        DEBUG_DUMP_INDENTED(deep, GBS_global_string("Item #%li/%li type=%02lx (filepos=%08lx)", item+1, nitems, type, ftell(in)-1));
        if (type == EOF) {
            GB_export_error("Unexpected end of file seen");
            return -1;
        }
        if (!type) {
            int func;
            if (version == 1) { // master file
                if (gb_recover_corrupt_file(gbd, in, "Unknown DB type 0 (DB version=1)")) return -1;
                continue;
            }
            func = getc(in);
            switch (func) {
                case 1:     //  delete entry
                    index = (int)gb_read_number(in);
                    if (index >= gbd->d.nheader) {
                        gb_create_header_array(gbd, index+1);
                        header = GB_DATA_LIST_HEADER(gbd->d);
                    }
                    if ((gb2 = GB_HEADER_LIST_GBD(header[index]))!=NULL) {
                        gb_delete_entry(&gb2);
                    }
                    else {
                        header[index].flags.ever_changed = 1;
                        header[index].flags.changed      = GB_DELETED;
                    }

                    break;
                default:
                    if (gb_recover_corrupt_file(gbd, in, GBS_global_string("Unknown func=%i", func))) return -1;
                    continue;
            }
            continue;
        }

        security = getc(in);
        type2 = (type>>4)&0xf;
        key = (GBQUARK)gb_read_number(in);

        if (key >= Main->keycnt || !Main->keys[key].key) {
            GB_export_error("Inconsistent Database: Changing field identifier to 'main'");
            key = 0;
        }

        DEBUG_DUMP_INDENTED(deep, GBS_global_string("key='%s' type2=%li", Main->keys[key].key, type2));

        gb2 = NULL;
        gbc = NULL;
        if (version == 2) {
            index = (int)gb_read_number(in);
            if (index >= gbd->d.nheader) {
                gb_create_header_array(gbd, index+1);
                header = GB_DATA_LIST_HEADER(gbd->d);
            }

            if (index >= 0 && (gb2 = GB_HEADER_LIST_GBD(header[index]))!=NULL) {
                if ((GB_TYPE(gb2) == GB_DB) != (type2 == GB_DB)) {
                    GB_internal_error("Type changed, you may loose data");
                    gb_delete_entry(&gb2);
                    SET_GB_HEADER_LIST_GBD(header[index], NULL);
                }
                else {
                    if (type2 == GB_DB) gbc = (GBCONTAINER *)gb2;
                    else GB_FREEDATA(gb2);
                }
            }
        }
        else index = -1;

        if (!gb2) {
            if (type2 == (long)GB_DB) {
                gbc = gb_make_container(gbd, NULL, index, key);
                gb2 = (GBDATA *)gbc;
            }
            else {
                gb2 = gb_make_entry(gbd, NULL, index, key, (GB_TYPES)type2);
                GB_INDEX_CHECK_OUT(gb2);
            }
        }

        if (version == 2) {
            GB_CREATE_EXT(gb2);
            gb2->ext->update_date = gb2->ext->creation_date = Main->clock;
            header[gb2->index].flags.ever_changed = 1;
        }
        else {
            Main->keys[key].nref_last_saved++;
        }

        gb2->flags.security_delete  = type >> 1;
        gb2->flags.security_write   = ((type&1) << 2) + (security >> 6);
        gb2->flags.security_read    = security >> 3;
        gb2->flags.compressed_data  = security >> 2;
        header[gb2->index].flags.flags  = (int)((security >> 1) & 1);
        gb2->flags.unused       = security >> 0;
        gb2->flags2.last_updated = getc(in);

        switch (type2) {
            case GB_INT:
                {
                    GB_UINT4 buffer;
                    if (!fread((char*)&buffer, sizeof(GB_UINT4), 1, in)) {
                        GB_export_error("File too short, seems truncated");
                        return -1;
                    }
                    gb2->info.i = ntohl(buffer);
                    break;
                }

            case GB_FLOAT:
                if (!fread((char*)&gb2->info.i, sizeof(float), 1, in)) {
                    GB_export_error("File too short, seems truncated");
                    return -1;
                }
                break;
            case GB_STRING_SHRT:
                i = GB_give_buffer_size();
                p = buff = GB_give_buffer(GBTUM_SHORT_STRING_SIZE+2);
                size = 0;
                while (1) {
                    for (; size<i; size++) {
                        if (!(*(p++) = getc(in))) goto shrtstring_fully_loaded;
                    }
                    i = i*3/2;
                    buff = GB_increase_buffer(i);
                    p = buff + size;
                }
        shrtstring_fully_loaded :
                GB_SETSMDMALLOC(gb2, size, size+1, buff);
                break;
            case GB_STRING:
            case GB_LINK:
            case GB_BITS:
            case GB_BYTES:
            case GB_INTS:
            case GB_FLOATS:
                size = gb_read_number(in);
                memsize =  gb_read_number(in);
                DEBUG_DUMP_INDENTED(deep, GBS_global_string("size=%li memsize=%li", size, memsize));
                if (GB_CHECKINTERN(size, memsize)) {
                    GB_SETINTERN(gb2);
                    p = &(gb2->info.istr.data[0]);
                }
                else {
                    GB_SETEXTERN(gb2);
                    p = gbm_get_mem((size_t)memsize+1, GB_GBM_INDEX(gb2)); // ralf: added +1 because decompress ran out of this block
                }
                i = fread(p, 1, (size_t)memsize, in);
                if (i!=memsize) {
                    gb_read_bin_error(in, gb2, "Unexpected EOF found");
                    return -1;
                }
                GB_SETSMD(gb2, size, memsize, p);
                break;
            case GB_DB:
                size = gb_read_number(in);
                // gbc->d.size  is automatically incremented
                if (gb_read_bin_rek_V2(in, gbc, size, version, reversed, deep+1)) {
                    if (!GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
                        return -1;
                    }
                }
                break;
            case GB_BYTE:
                gb2->info.i = getc(in);
                break;
            default:
                gb_read_bin_error(in, gb2, "Unknown type");
                if (gb_recover_corrupt_file(gbd, in, NULL)) {
                    return GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery
                        ? 0                         // loading stopped
                        : -1;
                }

                continue;
        }
    }
    return 0;
}

GBDATA *gb_search_system_folder_rek(GBDATA *gbd) {
    GBDATA *gb2;
    GBDATA *gb_result = 0;
    for (gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {
        int type = GB_read_type(gb2);
        if (type != GB_DB) continue;
        if (!strcmp(GB_SYSTEM_FOLDER, GB_read_key_pntr(gb2))) {
            gb_result = gb2;
            break;
        }
    }
    return gb_result;
}


void gb_search_system_folder(GBDATA *gb_main) {
    /* Search a system folder within the database tree
     * and copy it to main level
     */
    GBDATA   *gb_oldsystem;
    GB_ERROR  error;
    GBDATA   *gb_system = GB_entry(gb_main, GB_SYSTEM_FOLDER);
    if (gb_system) return;

    GB_warning("Searching system information");
    gb_oldsystem = gb_search_system_folder_rek(gb_main);
    if (!gb_oldsystem) {
        GB_warning("!!!!! not found (bad)");
        return;
    }
    gb_system = GB_search(gb_main, GB_SYSTEM_FOLDER, GB_CREATE_CONTAINER);
    error = GB_copy(gb_system, gb_oldsystem);
    if (!error) error = GB_delete(gb_oldsystem);
    if (error) GB_warning(error);
    GB_warning("***** found (good)");
}

long gb_read_bin(FILE *in, GBCONTAINER *gbd, int diff_file_allowed)
{
    int   c = 1;
    long  i;
    long  error;
    long  j, k;
    long  version;
    long  reversed;
    long  nodecnt;
    long  first_free_key;
    char *buffer, *p;

    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);

    while (c && c != EOF) {
        c = getc(in);
    }
    if (c==EOF) {
        gb_read_bin_error(in, (GBDATA *)gbd, "First zero not found");
        return 1;
    }

    i = gb_read_in_long(in, 0);
    if (strncmp((char *)&i, "vers", 4)) {
        gb_read_bin_error(in, (GBDATA *)gbd, "keyword 'vers' not found");
        return 1;
    }
    i = gb_read_in_long(in, 0);
    switch (i) {
        case 0x01020304:
            reversed = 0;
            break;
        case 0x04030201:
            reversed = 1;
            break;
        default:
            gb_read_bin_error(in, (GBDATA *)gbd, "keyword '^A^B^C^D' not found");
            return 1;
    }
    version = gb_read_in_long(in, reversed);
    if (version >2) {
        gb_read_bin_error(in, (GBDATA *)gbd, "ARB Database version > '2'");
        return 1;
    }

    if (version == 2 && !diff_file_allowed) {
        GB_export_error("This is not a primary arb file, please select the master"
                        " file xxx.arb");
        return 1;
    }

    buffer = GB_give_buffer(256);
    i = gb_read_in_long(in, 0);
    if (strncmp((char *)&i, "keys", 4)) {
        gb_read_bin_error(in, (GBDATA *)gbd, "keyword 'keys' not found");
        return 1;
    }

    if (!Main->key_2_index_hash) Main->key_2_index_hash = GBS_create_hash(ALLOWED_KEYS, GB_MIND_CASE);

    first_free_key = 0;
    gb_free_all_keys(Main);

    while (1) {         // read keys
        long nrefs = 0;
        if (version) {
            nrefs = gb_read_number(in);
        }
        p = buffer;
        for (k=0; ; k++) {
            c = getc(in);
            if (!c) break;
            if (c==EOF) {
                gb_read_bin_error(in, (GBDATA *)gbd, "unexpected EOF while reading keys");
                return 1;
            }
            *(p++) = c;
        }
        *p = 0;

        if (k > GB_KEY_LEN_MAX) {
            printf("Warning: Key '%s' exceeds maximum keylength (%i)\n"
                   "         Please do NOT create such long keys!\n",
                   buffer, GB_KEY_LEN_MAX);
        }
        if (p == buffer) break;

        if (*buffer == 1) {                                                   // empty key
            long index = gb_create_key(Main, 0, false);

            Main->keys[index].key           = 0;
            Main->keys[index].nref          = 0;
            Main->keys[index].next_free_key = first_free_key;

            first_free_key = index;
        }
        else {
            long index = gb_create_key(Main, buffer, false);

            Main->keys[index].nref = nrefs;
        }
    }

    Main->first_free_key = first_free_key;

    i = gb_read_in_long(in, 0);
    if (strncmp((char *)&i, "time", 4)) {
        gb_read_bin_error(in, (GBDATA *)gbd, "keyword 'time' not found");
        return 1;
    }
    for (j=0; j<255; j++) {         // read times
        p = buffer;
        for (k=0; k<256; k++) {
            c = getc(in);
            if (!c) break;
            if (c==EOF) {
                gb_read_bin_error(in, (GBDATA *)gbd, "unexpected EOF while reading times");
                return 1;
            }
            *(p++) = c;
        }
        *p = 0;
        if (p == buffer) break;
        freedup(Main->dates[j], buffer);
    }
    if (j>=255) {
        gb_read_bin_error(in, (GBDATA *)gbd, "more than 255 dates are not allowed");
        return 1;
    }
    Main->last_updated = (unsigned int)j;

    i = gb_read_in_long(in, 0);
    if (strncmp((char *)&i, "data", 4)) {
        gb_read_bin_error(in, (GBDATA *)gbd, "keyword 'data' not found");
        return 0;
    }
    nodecnt = gb_read_in_long(in, reversed);
    GB_give_buffer(256);

    if (version==1) {                               // teste auf map file falls version == 1
        long    mode = GB_mode_of_link(Main->path); // old master
        GB_CSTR map_path;

        if (S_ISLNK(mode)) {
            char *path2 = GB_follow_unix_link(Main->path);
            map_path = gb_mapfile_name(path2);
            free(path2);
        }
        else {
            map_path = gb_mapfile_name(Main->path);
        }

        bool          mapped          = false;
        GB_ERROR      map_fail_reason = NULL;
        gb_map_header mheader;

        switch (gb_is_valid_mapfile(map_path, &mheader, 0)) {
            case -1: map_fail_reason = GBS_global_string("no FastLoad File '%s' found", map_path); break;
            case  0: map_fail_reason = GB_await_error(); break;
            case  1: {
                if (gb_main_array[mheader.main_idx]==NULL) {
                    GBCONTAINER *newGbd = (GBCONTAINER*)gb_map_mapfile(map_path);

                    if (newGbd) {
                        GBCONTAINER *father  = GB_FATHER(gbd);
                        GB_MAIN_IDX  new_idx = mheader.main_idx;
                        GB_MAIN_IDX  old_idx = father->main_idx;

                        GB_commit_transaction((GBDATA*)gbd);

                        gb_assert(newGbd->main_idx == new_idx);
                        gb_assert((new_idx % GB_MAIN_ARRAY_SIZE) == new_idx);

                        gb_main_array[new_idx] = Main;
                        Main->data = newGbd;
                        father->main_idx = new_idx;

                        gbd = newGbd;
                        SET_GB_FATHER(gbd, father);

                        gb_main_array[old_idx]    = NULL;

                        GB_begin_transaction((GBDATA*)gbd);
                        mapped = true;
                    }
                }
                else {
                    map_fail_reason = GBS_global_string("FastLoad-File index conflict (%s, %i)", map_path, mheader.main_idx);
                }
            
                break;
            }
            default: gb_assert(0); break;
        }

        gb_assert(mapped || map_fail_reason);

        if (mapped) return 0; // succeded loading mapfile -> no need to load normal DB file
        GB_informationf("ARB: %s => loading entire DB", map_fail_reason);
    }

    // load binary DB file

    switch (version) {
        case 0:
            error = gb_read_bin_rek(in, gbd, nodecnt, version, reversed);
            break;
        case 2:
            for (i=1; i < Main->keycnt; i++) {
                if (Main->keys[i].key) {
                    Main->keys[i].nref_last_saved = Main->keys[i].nref;
                }
            }

            if (Main->clock<=0) Main->clock++;
        case 1:
            error = gb_read_bin_rek_V2(in, gbd, nodecnt, version, reversed, 0);
            break;
        default:
            GB_internal_errorf("Sorry: This ARB Version does not support database format V%li", version);
            error = 1;
    }

    if (gb_local->search_system_folder) {
        gb_search_system_folder((GBDATA *)gbd);
    }

    switch (version) {
        case 2:
        case 1:
            for (i=1; i < Main->keycnt; i++) {
                if (Main->keys[i].key) {
                    Main->keys[i].nref = Main->keys[i].nref_last_saved;
                }
            }
            break;
        default:
            break;
    }

    return error;
}

// ----------------------
//      OPEN DATABASE

long gb_next_main_idx_for_mapfile = 0;

void GB_set_next_main_idx(long idx) {
    gb_next_main_idx_for_mapfile = idx;
}

GB_MAIN_IDX gb_make_main_idx(GB_MAIN_TYPE *Main) {
    static int initialized = 0;
    GB_MAIN_IDX idx;

    if (!initialized) {
        for (idx=0; idx<GB_MAIN_ARRAY_SIZE; idx++) gb_main_array[idx] = NULL;
        initialized = 1;
    }
    if (gb_next_main_idx_for_mapfile<=0) {
        while (1) {                                 // search for unused array index
            idx = (short)GB_random(GB_MAIN_ARRAY_SIZE);
            if (gb_main_array[idx]==NULL) break;
        }
    }
    else {
        idx = (short)gb_next_main_idx_for_mapfile;
        gb_next_main_idx_for_mapfile = 0;
    }

    gb_assert((idx%GB_MAIN_ARRAY_SIZE) == idx);

    gb_main_array[idx] = Main;
    return idx;
}

void gb_release_main_idx(GB_MAIN_TYPE *Main) {
    if (Main->dummy_father) {
        GB_MAIN_IDX idx = Main->dummy_father->main_idx;

        gb_assert(gb_main_array[idx] == Main);
        gb_main_array[idx] = NULL;
    }
}

GB_ERROR gb_login_remote(GB_MAIN_TYPE *Main, const char *path, const char *opent) {
    GBCONTAINER *gbd   = Main->data;
    GB_ERROR     error = NULL;

    Main->local_mode = false;
    Main->c_link     = gbcmc_open(path);

    if (!Main->c_link) {
        error = GBS_global_string("There is no ARBDB server '%s', please start one or add a filename", path);
    }
    else {
        gbd->server_id    = 0;
        Main->remote_hash = GBS_create_numhash(GB_REMOTE_HASH_SIZE);
        error             = gb_init_transaction(gbd); // login in server

        if (!error) {
            gbd->flags2.folded_container = 1;

            if (strchr(opent, 't')) error = gb_unfold(gbd, 0, -2);  // tiny
            else if (strchr(opent, 'm')) error = gb_unfold(gbd, 1, -2); // medium (no sequence)
            else if (strchr(opent, 'b')) error = gb_unfold(gbd, 2, -2); // big (no tree)
            else if (strchr(opent, 'h')) error = gb_unfold(gbd, -1, -2); // huge (all)
            else error                          = gb_unfold(gbd, 0, -2); // tiny
        }
    }
    return error;
}

GBDATA *GB_login(const char *cpath, const char *opent, const char *user) {
    /*! open an ARB database
     *
     * @param opent contains one or more of the following characters:
     * - 'r' read
     * - 'w' write (w/o 'r' it overwrites existing database)
     * - 'c' create (if not found)
     * - 's'     read only ???
     * - 'd' look for default (if create) in $ARBHOME/lib (any leading '.' is removed )
     * - 'D' look for default (if create) in $ARBHOME/lib/arb_default (any leading '.' is removed )
     * - memory usage:
     *   - 't' small memory usage
     *   - 'm' medium
     *   - 'b' big
     *   - 'h' huge
     * - 'R' allow corrupt file recovery + opening quicks with no master
     * - 'N' assume new database format (does not check whether to convert old->new compression)
     *
     * @return root node
     *
     * @see GB_open() and GBT_open()
     */
    GBCONTAINER   *gbd;
    FILE          *input;
    long           i;
    GB_MAIN_TYPE  *Main;
    gb_open_types  opentype;
    GB_CSTR        quickFile           = NULL;
    int            ignoreMissingMaster = 0;
    int            loadedQuickIndex    = -1;
    GB_ERROR       error               = 0;
    char          *path                = strdup(cpath);
    bool           dbCreated           = false;

    GBK_install_SIGSEGV_handler(true);

    if (!opent) opentype = gb_open_all;
    else if (strchr(opent, 'w')) opentype = gb_open_all;
    else if (strchr(opent, 's')) opentype = gb_open_read_only_all;
    else opentype = gb_open_read_only_all;

    if (strchr(path, ':')) {
        ; // remote access
    }
    else if (GBS_string_matches(path, "*.quick?", GB_MIND_CASE)) {
        char *ext = gb_findExtension(path);
        gb_assert(ext!=0);
        if (isdigit(ext[6]))
        {
            loadedQuickIndex = atoi(ext+6);
            strcpy(ext, ".arb");
            quickFile = gb_oldQuicksaveName(path, loadedQuickIndex);
            if (strchr(opent, 'R'))     ignoreMissingMaster = 1;
        }
    }
    else if (GBS_string_matches(path, "*.a??", GB_MIND_CASE)) {

        char *extension = gb_findExtension(path);

        if (isdigit(extension[2]) && isdigit(extension[3]))
        {
            loadedQuickIndex = atoi(extension+2);
            strcpy(extension, ".arb");
            quickFile = gb_quicksaveName(path, loadedQuickIndex);
            if (strchr(opent, 'R'))     ignoreMissingMaster = 1;
        }
        else {
            char *base = strdup(path);
            char *ext = gb_findExtension(base);
            {
                struct gb_scandir dir;
                ext[0] = 0;
                gb_scan_directory(base, &dir);

                loadedQuickIndex = dir.highest_quick_index;

                if (dir.highest_quick_index!=dir.newest_quick_index)
                {
                    GB_warning("The QuickSave-File with the highest index-number\n"
                               "is not the NEWEST of your QuickSave-Files.\n"
                               "If you didn't restore old QuickSave-File from a backup\n"
                               "please inform your system-administrator - \n"
                               "this may be a serious bug and you may loose your data.");
                }

                switch (dir.type)
                {
                    case GB_SCAN_NO_QUICK:
                        break;
                    case GB_SCAN_NEW_QUICK:
                        quickFile = gb_quicksaveName(path, dir.highest_quick_index);
                        break;
                    case GB_SCAN_OLD_QUICK:
                        quickFile = gb_oldQuicksaveName(path, dir.newest_quick_index);

                        break;
                }
            }

            free(base);
        }
    }

    if (gb_verbose_mode) {
        GB_informationf("ARB: Loading '%s'%s%s", path, quickFile ? " + Changes-File " : "", quickFile ? quickFile : "");
    }

    gbm_init_mem();
    GB_init_gb();

    if (GB_install_pid(1)) return 0;

    Main = gb_make_gb_main_type(path);
    Main->local_mode = true;

    if (strchr(opent, 'R')) Main->allow_corrupt_file_recovery = 1;

    gb_create_key(Main, "main", false);

    Main->dummy_father            = gb_make_container(NULL, 0, -1, 0); // create "main"
    Main->dummy_father->main_idx  = gb_make_main_idx(Main);
    Main->dummy_father->server_id = GBTUM_MAGIC_NUMBER;
    gbd                           = gb_make_container(Main->dummy_father, 0, -1, 0); // create "main"

    Main->data = gbd;
    gbcm_login(gbd, user);
    Main->opentype = opentype;
    Main->security_level = 7;

    if (path && (strchr(opent, 'r'))) {
        if (strchr(path, ':')) {
            error = gb_login_remote(Main, path, opent);
        }
        else {
            int read_from_stdin = strcmp(path, "-") == 0;

            GB_ULONG time_of_main_file  = 0;
            GB_ULONG time_of_quick_file = 0;
            Main->local_mode            = true;
            GB_begin_transaction((GBDATA *)gbd);
            Main->clock                 = 0;        // start clock

            if (read_from_stdin) input = stdin;
            else input                 = fopen(path, "rb");

            if (!input && ignoreMissingMaster) {
                goto load_quick_save_file_only;
            }

            if (!input) {
                if (strchr(opent, 'c')) {
                    GB_disable_quicksave((GBDATA *)gbd, "Database Created");

                    if (strchr(opent, 'd')||strchr(opent, 'D')) {
                        // use default settings
                        const char *pre;
                        char       *found_path;

                        GB_clear_error();           // with default-files gb_scan_directory (used above)
                        // creates an error, cause the path is a fake path

                        if (strchr(opent, 'd')) pre = "";
                        else                    pre = "arb_default/";

                        found_path = GBS_find_lib_file(path, pre, 0);
                        if (!found_path) {
                            fprintf(stderr, "file %s not found\n", path);
                            dbCreated = true;
                        }
                        else {
#if defined(DEBUG)
                            fprintf(stderr, "Using properties from %s\n", found_path);
#endif // DEBUG
                            freeset(path, found_path);
                            input = fopen(path, "rb");
                        }
                    }
                    else {
                        dbCreated = true;
                    }

                    if (dbCreated) printf(" database %s created\n", path);
                }
                else {
                    error = GBS_global_string("Database '%s' not found", path);
                    gbd   = 0;
                }
            }
            if (input) {
                time_of_main_file = GB_time_of_file(path);

                if (input != stdin) i = gb_read_in_long(input, 0);
                else i                = 0;

                if ((i == 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) {
                    i = gb_read_bin(input, gbd, 0);     // read or map whole db
                    gbd = Main->data;
                    fclose(input);

                    if (i) {
                        if (Main->allow_corrupt_file_recovery) {
                            GB_print_error();
                        }
                        else {
                            gbd   = 0;
                            error = GB_await_error();
                        }
                    }

                    if (gbd && quickFile) {
                        long     err;
                        GB_ERROR err_msg;
                    load_quick_save_file_only :
                        err     = 0;
                        err_msg = 0;

                        input = fopen(quickFile, "rb");

                        if (input) {
                            time_of_quick_file = GB_time_of_file(quickFile);
                            if (time_of_main_file && time_of_quick_file < time_of_main_file) {
                                const char *warning = GBS_global_string("Your main database file '%s' is newer than\n"
                                                                        "   the changes file '%s'\n"
                                                                        "   That is very strange and happens only if files where\n"
                                                                        "   moved/copied by hand\n"
                                                                        "   Your file '%s' may be an old relict,\n"
                                                                        "   if you ran into problems now,delete it",
                                                                        path, quickFile, quickFile);
                                GB_warning(warning);
                            }
                            i = gb_read_in_long(input, 0);
                            if ((i == 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED))
                            {
                                err = gb_read_bin(input, gbd, 1);
                                fclose (input);

                                if (err) {
                                    err_msg = GBS_global_string("Loading failed (file corrupt?)\n"
                                                                "[Fail-Reason: '%s']\n",
                                                                GB_await_error());
                                }
                            }
                            else {
                                err_msg = "Wrong file format (not a quicksave file)";
                                err     = 1;
                            }
                        }
                        else {
                            err_msg = "Can't open file";
                            err     = 1;
                        }

                        if (err) {
                            error = GBS_global_string("I cannot load your quick file '%s'\n"
                                                      "Reason: %s\n"
                                                      "\n"
                                                      "Note: you MAY restore an older version by running arb with:\n"
                                                      "      arb <name of quicksave-file>",
                                                      quickFile, err_msg);

                            if (!Main->allow_corrupt_file_recovery) {
                                gbd = 0;
                            }
                            else {
                                GB_export_error(error);
                                GB_print_error();
                                GB_clear_error();
                                error = 0;
                                GB_disable_quicksave((GBDATA*)gbd, "Couldn't load last quicksave (your latest changes are NOT included)");
                            }
                        }
                    }
                    Main->qs.last_index = loadedQuickIndex; // determines which # will be saved next
                }
                else {
                    if (input != stdin) fclose(input);
                    error = gb_read_ascii(path, gbd);
                    if (error) GB_warning(error);
                    GB_disable_quicksave((GBDATA *)gbd, "Sorry, I cannot save differences to ascii files\n"
                                         "  Save whole database in binary mode first");
                }
            }
        }
    }
    else {
        GB_disable_quicksave((GBDATA *)gbd, "Database not part of this process");
        Main->local_mode = true;
        GB_begin_transaction((GBDATA *)gbd);
    }

    gb_assert(error || gbd);

    if (error) {
        gb_release_main_idx(Main);
        GB_export_error(error);
        gbd = 0;
    }
    else {
        GB_commit_transaction((GBDATA *)gbd);
        {
            GB_begin_transaction((GBDATA *)gbd); // New Transaction, should be quicksaveable
            if (!strchr(opent, 'N')) {               // new format
                gb_convert_V2_to_V3((GBDATA *)gbd); // Compression conversion
            }
            error = gb_load_key_data_and_dictionaries((GBDATA *)Main->data);
            if (!error) error = GB_resort_system_folder_to_top((GBDATA *)Main->data);
            GB_commit_transaction((GBDATA *)gbd);
        }
        Main->security_level = 0;
        gbl_install_standard_commands((GBDATA *)gbd);

        if (Main->local_mode) {                     // i am the server
            GBT_install_message_handler((GBDATA *)gbd);
        }
        if (gb_verbose_mode && !dbCreated) GB_informationf("ARB: Loading '%s' done\n", path);
    }
    free(path);
    return (GBDATA *)gbd;
}

GBDATA *GB_open(const char *path, const char *opent)
{
    const char *user;
    user = GB_getenvUSER();
    return GB_login(path, opent, user);
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)

#include <test_unit.h>

void TEST_dummy() {
    // just here to trigger coverage analysis as long as there are no real tests
    // (much code from ad_load.cxx is used by ad_save_load.cxx)
}

#endif
