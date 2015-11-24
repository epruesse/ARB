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
#include <arb_str.h>
#include <arb_file.h>
#include <arb_defs.h>
#include <arb_progress.h>

#include "gb_key.h"
#include "gb_localdata.h"
#include "gb_map.h"
#include "gb_load.h"
#include "ad_io_inline.h"

static int gb_verbose_mode = 0;
void GB_set_verbose() {
    gb_verbose_mode = 1;
}

#define FILESIZE_GRANULARITY 1024 // progress will work with DB files up to 2Tb

// ---------------------------------------------------
//      helper code to read ascii file in portions

#ifdef UNIT_TESTS // UT_DIFF
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

static char *getLine(Reader *r) {
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
            return 0;               // report "eof seen"

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

static const char *getToken(char **line) {
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
            gb_assert(i<ALLOWED_DATES);
            Main->dates[i]     = strdup("unknown date");
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
                const char *name = getToken(&line);

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
                    const char *protection = NULL;
                    if (line[0] == ':') {           // protection level
                        protection = getToken(&line);
                    }

                    bool readAsString = true;
                    if (line[0] == '%') {
                        readAsString = false;

                        const char *type = getToken(&line);

                        if (type[1] == 0 || type[2] != 0) {
                            error = GBS_global_string("Syntax error in type '%s' (expected %% and 1 more character)", type);
                        }
                        else {
                            if (type[1] == '%' || type[1] == '$') {   // container
                                if (line[0] == '(' && line[1] == '%') {
                                    char        *cont_name = strdup(name);
                                    GBCONTAINER *gbc       = gb_make_container(gb_parent, cont_name, -1, 0);

                                    char *protection_copy = nulldup(protection);
                                    bool  marked          = type[1] == '$';
                                    error                 = gb_parse_ascii_rek(r, gbc, cont_name);
                                    // CAUTION: most buffer variables are invalidated NOW!!!

                                    if (!error) error = set_protection_level(Main, gbc, protection_copy);
                                    if (marked) GB_write_flag(gbc, 1);

                                    free(protection_copy);
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
                                    gb_assert(gb_type != GB_NONE);

                                    GBENTRY *gb_new    = gb_make_entry(gb_parent, name, -1, 0, gb_type);
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

static GB_ERROR gb_read_ascii(const char *path, GBCONTAINER *gbc) {
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
        in              = fopen(path, "r");
        if (!in) error  = GBS_global_string("Can't open '%s'", path);
        else close_file = 1;
    }

    if (!error) {
        Reader *r = openReader(in);

        GB_search(gbc, GB_SYSTEM_FOLDER, GB_CREATE_CONTAINER); // Switch to Version 3

        error = gb_parse_ascii(r, gbc);

        GB_ERROR cl_error = closeReader(r);
        if (!error) error = cl_error;
    }

    if (close_file) fclose(in);
    return error;
}

// --------------------------
//      Read binary files

static long gb_recover_corrupt_file(GBCONTAINER *gbc, FILE *in, GB_ERROR recovery_reason, bool loading_quick_save) {
    // search pattern dx xx xx xx string 0
    // returns 0 if recovery was able to resync
    static FILE *old_in = 0;
    static unsigned char *file = 0;
    static long size = 0;
    if (!GBCONTAINER_MAIN(gbc)->allow_corrupt_file_recovery) {
        if (!recovery_reason) { recovery_reason = GB_await_error(); }
        char       *reason         = strdup(recovery_reason);
        const char *located_reason = GBS_global_string("%s (inside '%s')", reason, GB_get_db_path(gbc));

        if (loading_quick_save) {
            GB_export_error(located_reason);
        }
        else {
            GB_export_errorf("%s\n"
                             "(parts of your database might be recoverable using 'arb_repair yourDB.arb newName.arb')\n",
                             located_reason);
        }
        free(reason);
        return -1;
    }

    if (GB_is_fifo(in)) {
        GB_export_error("Unable to recover from corrupt file (Reason: cannot recover from stream)\n"
                        "Note: if the file is a compressed arb-file, uncompress it manually and retry.");
        return -1;
    }

    long pos = ftell(in);
    if (old_in != in) {
        file   = (unsigned char *)GB_map_FILE(in, 0);
        old_in = in;
        size   = GB_size_of_FILE(in);
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
                gb_local->search_system_folder = true;
                return fseek(in, pos, 0);
            }
        }
    }
    return -1;          // no short string found
}

// ----------------------------------------
// #define DEBUG_READ
#if defined(DEBUG_READ)

static void DEBUG_DUMP_INDENTED(long deep, const char *s) {
    printf("%*s%s\n", (int)deep, "", s);
}

#else
#define DEBUG_DUMP_INDENTED(d, s)
#endif // DEBUG_READ
// ----------------------------------------

static long gb_read_bin_rek_V2(FILE *in, GBCONTAINER *gbc_dest, long nitems, long version, 
                               long reversed, long deep, arb_progress& progress) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbc_dest);

    DEBUG_DUMP_INDENTED(deep, GBS_global_string("Reading container with %li items", nitems));

    progress.inc_to(ftell(in)/FILESIZE_GRANULARITY);
    if (progress.aborted()) {
        GB_export_error(progress.error_if_aborted());
        return -1;
    }

    gb_create_header_array(gbc_dest, (int)nitems);
    gb_header_list *header = GB_DATA_LIST_HEADER(gbc_dest->d);
    if (deep == 0 && GBCONTAINER_MAIN(gbc_dest)->allow_corrupt_file_recovery) {
        GB_warning("Now reading to end of file (trying to recover data from broken database)");
        nitems = 10000000; // read forever at highest level
    }

    bool is_quicksave = version != 1;

    for (long item = 0; item<nitems; item++) {
        long type = getc(in);
        DEBUG_DUMP_INDENTED(deep, GBS_global_string("Item #%li/%li type=%02lx (filepos=%08lx)", item+1, nitems, type, ftell(in)-1));
        if (type == EOF) {
            if (GBCONTAINER_MAIN(gbc_dest)->allow_corrupt_file_recovery) {
                GB_export_error("End of file reached (no error)");
            }
            else {
                GB_export_error("Unexpected end of file seen");
            }
            return -1;
        }
        if (!type) {
            int func;
            if (version == 1) { // master file
                if (gb_recover_corrupt_file(gbc_dest, in, "Unknown DB type 0 (DB version=1)", is_quicksave)) return -1;
                continue;
            }
            func = getc(in);
            switch (func) {
                case 1: {    //  delete entry
                    int index = (int)gb_get_number(in);
                    if (index >= gbc_dest->d.nheader) {
                        gb_create_header_array(gbc_dest, index+1);
                        header = GB_DATA_LIST_HEADER(gbc_dest->d);
                    }

                    GBDATA *gb2 = GB_HEADER_LIST_GBD(header[index]);
                    if (gb2) {
                        gb_delete_entry(gb2);
                    }
                    else {
                        header[index].flags.set_change(GB_DELETED);
                    }

                    break;
                }
                default:
                    if (gb_recover_corrupt_file(gbc_dest, in, GBS_global_string("Unknown func=%i", func), is_quicksave)) return -1;
                    continue;
            }
            continue;
        }

        long    security = getc(in);
        long    type2    = (type>>4)&0xf;
        GBQUARK key      = (GBQUARK)gb_get_number(in);

        if (key >= Main->keycnt || !quark2key(Main, key)) {
            const char *reason = GBS_global_string("database entry with unknown field quark %i", key);
            if (gb_recover_corrupt_file(gbc_dest, in, reason, is_quicksave)) return -1;
            continue;
        }

        DEBUG_DUMP_INDENTED(deep, GBS_global_string("key='%s' type2=%li", quark2key(Main, key), type2));

        GBENTRY     *gbe = NULL;
        GBCONTAINER *gbc = NULL;
        GBDATA      *gbd = NULL;

        {
            int index;
            if (version == 2) {
                index = (int)gb_get_number(in);
                if (index >= gbc_dest->d.nheader) {
                    gb_create_header_array(gbc_dest, index+1);
                    header = GB_DATA_LIST_HEADER(gbc_dest->d);
                }

                if (index >= 0 && (gbd = GB_HEADER_LIST_GBD(header[index]))!=NULL) {
                    if ((gbd->type() == GB_DB) != (type2 == GB_DB)) {
                        GB_internal_error("Type changed, you may loose data");
                        gb_delete_entry(gbd);
                        SET_GB_HEADER_LIST_GBD(header[index], NULL);
                    }
                    else {
                        if (type2 == GB_DB) {
                            gbc = gbd->as_container();
                        }
                        else {
                            gbe = gbd->as_entry();
                            gbe->free_data();
                        }
                    }
                }
            }
            else index = -1;

            if (!gbd) {
                if (type2 == (long)GB_DB) {
                    gbd = gbc = gb_make_container(gbc_dest, NULL, index, key);
                }
                else {
                    gbd = gbe = gb_make_entry(gbc_dest, NULL, index, key, (GB_TYPES)type2);
                    gbe->index_check_out();
                }
            }
        }

        gb_assert(implicated(gbe, gbe == gbd));
        gb_assert(implicated(gbc, gbc == gbd));
        gb_assert(implicated(gbd, contradicted(gbe, gbc)));

        if (version == 2) {
            gbd->create_extended();
            gbd->touch_creation_and_update(Main->clock);
            header[gbd->index].flags.ever_changed = 1;
        }
        else {
            Main->keys[key].nref_last_saved++;
        }

        gbd->flags.security_delete     = type >> 1;
        gbd->flags.security_write      = ((type&1) << 2) + (security >> 6);
        gbd->flags.security_read       = security >> 3;
        gbd->flags.compressed_data     = security >> 2;
        header[gbd->index].flags.flags = (int)((security >> 1) & 1);
        gbd->flags.unused              = security >> 0;
        gbd->flags2.last_updated       = getc(in);

        switch (type2) {
            case GB_INT:
                {
                    GB_UINT4 buffer;
                    if (!fread((char*)&buffer, sizeof(GB_UINT4), 1, in)) {
                        GB_export_error("File too short, seems truncated");
                        return -1;
                    }
                    gbe->info.i = ntohl(buffer);
                    break;
                }

            case GB_FLOAT:
                if (!fread((char*)&gbe->info.i, sizeof(float), 1, in)) {
                    GB_export_error("File too short, seems truncated");
                    return -1;
                }
                break;
            case GB_STRING_SHRT: {
                long  i    = GB_give_buffer_size();
                char *buff = GB_give_buffer(GBTUM_SHORT_STRING_SIZE+2);
                char *p    = buff;

                long size = 0;
                while (1) {
                    for (; size<i; size++) {
                        if (!(*(p++) = getc(in))) goto shrtstring_fully_loaded;
                    }
                    i = i*3/2;
                    buff = GB_increase_buffer(i);
                    p = buff + size;
                }
                  shrtstring_fully_loaded :
                gbe->insert_data(buff, size, size+1);
                break;
            }
            case GB_STRING:
            case GB_LINK:
            case GB_BITS:
            case GB_BYTES:
            case GB_INTS:
            case GB_FLOATS: {
                long size    = gb_get_number(in);
                long memsize = gb_get_number(in);

                DEBUG_DUMP_INDENTED(deep, GBS_global_string("size=%li memsize=%li", size, memsize));

                GBENTRY_memory storage(gbe, size, memsize);

                long i = fread(storage, 1, (size_t)memsize, in);
                if (i!=memsize) {
                    gb_read_bin_error(in, gbe, "Unexpected EOF found");
                    return -1;
                }
                break;
            }
            case GB_DB: {
                long size = gb_get_number(in);
                // gbc->d.size  is automatically incremented
                if (gb_read_bin_rek_V2(in, gbc, size, version, reversed, deep+1, progress)) {
                    if (!GBCONTAINER_MAIN(gbc_dest)->allow_corrupt_file_recovery) {
                        return -1;
                    }
                }
                break;
            }
            case GB_BYTE:
                gbe->info.i = getc(in);
                break;
            default:
                gb_read_bin_error(in, gbd, "Unknown type");
                if (gb_recover_corrupt_file(gbc_dest, in, NULL, is_quicksave)) {
                    return GBCONTAINER_MAIN(gbc_dest)->allow_corrupt_file_recovery
                        ? 0                         // loading stopped
                        : -1;
                }

                continue;
        }
    }
    return 0;
}

static GBDATA *gb_search_system_folder_rek(GBDATA *gbd) {
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


static void gb_search_system_folder(GBDATA *gb_main) {
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

inline bool read_keyword(const char *expected_keyword, FILE *in, GBCONTAINER *gbc) {
    gb_assert(strlen(expected_keyword) == 4); // mandatory

    long val         = gb_read_in_uint32(in, 0);
    bool as_expected = strncmp((char*)&val, expected_keyword, 4) == 0;
    
    if (!as_expected) {
        gb_read_bin_error(in, gbc, GBS_global_string("keyword '%s' not found", expected_keyword));
    }
    return as_expected;
}

static long gb_read_bin(FILE *in, GBCONTAINER *gbc, bool allowed_to_load_diff, arb_progress& progress) {
    int   c = 1;
    long  i;
    long  error;
    long  j, k;
    long  version;
    long  nodecnt;
    long  first_free_key;
    char *buffer, *p;

    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbc);

    while (c && c != EOF) {
        c = getc(in);
    }
    if (c==EOF) {
        gb_read_bin_error(in, gbc, "First zero not found");
        return 1;
    }

    if (!read_keyword("vers", in, gbc)) return 1;

    // detect byte order in ARB file
    i = gb_read_in_uint32(in, 0);
    bool reversed;
    switch (i) {
        case 0x01020304: reversed = false; break;
        case 0x04030201: reversed = true; break;
        default:
            gb_read_bin_error(in, gbc, "keyword '^A^B^C^D' not found");
            return 1;
    }

    version = gb_read_in_uint32(in, reversed);
    if (version == 0) {
        gb_read_bin_error(in, gbc, "ARB Database version 0 no longer supported (rev [9647])");
        return 1;
    }
    if (version>2) {
        gb_read_bin_error(in, gbc, "ARB Database version > '2'");
        return 1;
    }

    if (version == 2 && !allowed_to_load_diff) {
        GB_export_error("This is not a primary arb file, please select the master"
                        " file xxx.arb");
        return 1;
    }

    if (!read_keyword("keys", in, gbc)) return 1;

    if (!Main->key_2_index_hash) Main->key_2_index_hash = GBS_create_hash(ALLOWED_KEYS, GB_MIND_CASE);

    first_free_key = 0;
    Main->free_all_keys();

    buffer = GB_give_buffer(256);
    while (1) {         // read keys
        long nrefs = 0;
        if (version) {
            nrefs = gb_get_number(in);
        }
        p = buffer;
        for (k=0; ; k++) {
            c = getc(in);
            if (!c) break;
            if (c==EOF) {
                gb_read_bin_error(in, gbc, "unexpected EOF while reading keys");
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

    if (!read_keyword("time", in, gbc)) return 1;
    
    for (j=0; j<(ALLOWED_DATES-1); j++) {         // read times
        p = buffer;
        for (k=0; k<256; k++) {
            c = getc(in);
            if (!c) break;
            if (c==EOF) {
                gb_read_bin_error(in, gbc, "unexpected EOF while reading times");
                return 1;
            }
            *(p++) = c;
        }
        *p = 0;
        if (p == buffer) break;
        freedup(Main->dates[j], buffer);
    }
    if (j>=(ALLOWED_DATES-1)) {
        gb_read_bin_error(in, gbc, "too many date entries");
        return 1;
    }
    Main->last_updated = (unsigned int)j;

    if (!read_keyword("data", in, gbc)) return 1;

    nodecnt = gb_read_in_uint32(in, reversed);
    GB_give_buffer(256);

    if (version==1) {                                     // teste auf map file falls version == 1
        long          mode = GB_mode_of_link(Main->path); // old master
        GB_CSTR       map_path;
        unsigned long time_of_db;

        if (S_ISLNK(mode)) {
            char *path2 = GB_follow_unix_link(Main->path);
            map_path    = gb_mapfile_name(path2);
            time_of_db  = GB_time_of_file(path2);
            free(path2);
        }
        else {
            map_path = gb_mapfile_name(Main->path);
            time_of_db  = GB_time_of_file(Main->path);
        }

        bool          mapped          = false;
        GB_ERROR      map_fail_reason = NULL;
        gb_map_header mheader;

        switch (gb_is_valid_mapfile(map_path, &mheader, 0)) {
            case -1: map_fail_reason = GBS_global_string("no FastLoad File '%s' found", map_path); break;
            case  0: map_fail_reason = GB_await_error(); break;
            case  1: {
                unsigned long time_of_map = GB_time_of_file(map_path);

                if (time_of_map != time_of_db) { // timestamp of mapfile differs
                    unsigned long diff = time_of_map>time_of_db ? time_of_map-time_of_db : time_of_db-time_of_map;
                    fprintf(stderr, "Warning: modification times of DB and fastload file differ (DB=%lu fastload=%lu diff=%lu)\n",
                            time_of_db, time_of_map, diff);

                    if (diff>5) {
                        map_fail_reason = "modification times of DB and fastload file differ (too much)";
                    }
                    else {
                        fprintf(stderr, "(accepting modification time difference of %lu seconds)\n", diff);
                    }
                }

                if (!map_fail_reason) {
                    if (gb_main_array[mheader.main_idx]==NULL) {
                        GBCONTAINER *new_gbc = (GBCONTAINER*)gb_map_mapfile(map_path);

                        if (new_gbc) {
                            GBCONTAINER *father  = GB_FATHER(gbc);
                            GB_MAIN_IDX  new_idx = mheader.main_idx;
                            GB_MAIN_IDX  old_idx = father->main_idx;

                            long gbc_index = gbc->index;

                            GB_commit_transaction((GBDATA*)gbc);

                            gb_assert(new_gbc->main_idx == new_idx);
                            gb_assert((new_idx % GB_MAIN_ARRAY_SIZE) == new_idx);

                            gb_main_array[new_idx] = Main;

                            gbm_free_mem(Main->root_container, sizeof(GBCONTAINER), quark2gbmindex(Main, 0));

                            Main->root_container = new_gbc;
                            father->main_idx     = new_idx;

                            SET_GBCONTAINER_ELEM(father, gbc_index, NULL); // unlink old main-entry

                            gbc = new_gbc;
                            SET_GB_FATHER(gbc, father);
                        
                            SET_GBCONTAINER_ELEM(father, gbc->index, (GBDATA*)gbc); // link new main-entry

                            gb_main_array[old_idx]    = NULL;

                            GB_begin_transaction((GBDATA*)gbc);
                            mapped = true;
                        }
                    }
                    else {
                        map_fail_reason = GBS_global_string("FastLoad-File index conflict (%s, %i)", map_path, mheader.main_idx);
                    }
                }
            
                break;
            }
            default: gb_assert(0); break;
        }

        gb_assert(mapped || map_fail_reason);

        if (mapped) {
            Main->mapped = true;
            return 0; // succeded loading mapfile -> no need to load normal DB file
        }
        GB_informationf("ARB: %s => loading entire DB", map_fail_reason);
    }

    // load binary DB file

    switch (version) {
        case 2: // quick save file
            for (i=1; i < Main->keycnt; i++) {
                if (Main->keys[i].key) {
                    Main->keys[i].nref_last_saved = Main->keys[i].nref;
                }
            }

            if (Main->clock<=0) Main->clock++;
            // fall-through
        case 1: // master arb file
            error = gb_read_bin_rek_V2(in, gbc, nodecnt, version, reversed, 0, progress);
            break;
        default:
            GB_internal_errorf("Sorry: This ARB Version does not support database format V%li", version);
            error = 1;
    }

    if (gb_local->search_system_folder) {
        gb_search_system_folder(gbc);
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

static long gb_next_main_idx_for_mapfile = 0;

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

void GB_MAIN_TYPE::release_main_idx() { 
    if (dummy_father) {
        GB_MAIN_IDX idx = dummy_father->main_idx;
        gb_assert(gb_main_array[idx] == this);
        gb_main_array[idx] = NULL;
    }
}

GB_ERROR GB_MAIN_TYPE::login_remote(const char *db_path, const char *opent) {
    GB_ERROR error = NULL;

    i_am_server = false;
    c_link = gbcmc_open(db_path);
    if (!c_link) {
        error = GBS_global_string("There is no ARBDB server '%s', please start one or add a filename", db_path);
    }
    else {
        root_container->server_id = 0;

        remote_hash = GBS_create_numhash(GB_REMOTE_HASH_SIZE);
        error       = initial_client_transaction();

        if (!error) {
            root_container->flags2.folded_container = 1;

            if (strchr(opent, 't')) error      = gb_unfold(root_container, 0, -2);  // tiny
            else if (strchr(opent, 'm')) error = gb_unfold(root_container, 1, -2);  // medium (no sequence)
            else if (strchr(opent, 'b')) error = gb_unfold(root_container, 2, -2);  // big (no tree)
            else if (strchr(opent, 'h')) error = gb_unfold(root_container, -1, -2); // huge (all)
            else error                         = gb_unfold(root_container, 0, -2);  // tiny
        }
    }
    return error;
}

inline bool is_binary_db_id(int id) {
    return (id == 0x56430176)
        || (id == GBTUM_MAGIC_NUMBER)
        || (id == GBTUM_MAGIC_REVERSED);
}

static GBDATA *GB_login(const char *cpath, const char *opent, const char *user) {
    /*! open an ARB database
     *
     * @param cpath arb database. May be
     * - full path of database. (loads DB plus newest quicksave, if any quicksaves exist)
     * - full path of a quicksave file (loads DB with specific quicksave)
     * - ":" connects to a running ARB DB server (e.g. ARB_NT, arb_db_server)
     * @param opent contains one or more of the following characters:
     * - 'r' read
     * - 'w' write (w/o 'r' it overwrites existing database)
     * - 'c' create (if not found)
     * - 's'     read only ???
     * - 'D' looks for default in $ARBHOME/lib/arb_default if file is not found in $ARB_PROP
     *       (only works combined with mode 'c')
     * - memory usage:
     *   - 't' small memory usage
     *   - 'm' medium
     *   - 'b' big
     *   - 'h' huge
     * - 'R' allow corrupt file recovery + opening quicks with no master
     * - 'N' assume new database format (does not check whether to convert old->new compression)
     * @param user username
     *
     * @return DB root node (or NULL, in which case an error has been exported)
     *
     * @see GB_open() and GBT_open()
     */
    GBCONTAINER   *gbc;
    GB_MAIN_TYPE  *Main;
    gb_open_types  opentype;
    GB_CSTR        quickFile           = NULL;
    int            ignoreMissingMaster = 0;
    int            loadedQuickIndex    = -1;
    GB_ERROR       error               = 0;
    char          *path                = strdup(cpath?cpath:"");
    bool           dbCreated           = false;

    gb_assert(strchr(opent, 'd') == NULL); // mode 'd' is deprecated. You have to use 'D' and store your defaults inside ARBHOME/lib/arb_default

    opentype = (!opent || strchr(opent, 'w')) ? gb_open_all : gb_open_read_only_all;

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
                gb_scandir dir;
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

    error = GB_install_pid(1);
    if (error) {
        GB_export_error(error);
        return 0;
    }

    GB_init_gb();

    Main = new GB_MAIN_TYPE(path);
    Main->mark_as_server();

    if (strchr(opent, 'R')) Main->allow_corrupt_file_recovery = 1;

    ASSERT_RESULT(long, 0, gb_create_key(Main, "main", false));

    Main->dummy_father            = gb_make_container(NULL, NULL, -1, 0); // create "main"
    Main->dummy_father->main_idx  = gb_make_main_idx(Main);
    Main->dummy_father->server_id = GBTUM_MAGIC_NUMBER;
    gbc                           = gb_make_container(Main->dummy_father, NULL, -1, 0); // create "main"

    Main->root_container = gbc;

    error = gbcm_login(gbc, user);
    if (!error) {
        Main->opentype       = opentype;
        Main->security_level = 7;

        if (path && (strchr(opent, 'r'))) {
            if (strchr(path, ':')) {
                error = Main->login_remote(path, opent);
            }
            else {
                int read_from_stdin = strcmp(path, "-") == 0;

                GB_ULONG time_of_main_file = 0; long i;

                Main->mark_as_server();
                GB_begin_transaction(gbc);
                Main->clock      = 0;                   // start clock

                FILE *input = read_from_stdin ? stdin : fopen(path, "rb");
                if (!input && ignoreMissingMaster) {
                    goto load_quick_save_file_only;
                }

                if (!input) {
                    if (strchr(opent, 'c')) {
                        GB_disable_quicksave(gbc, "Database Created");

                        if (strchr(opent, 'D')) { // use default settings
                            GB_clear_error(); // with default-files gb_scan_directory (used above) has created an error, cause the path was a fake path
                        
                            gb_assert(!ARB_strBeginsWith(path, ".arb_prop/")); // do no longer pass path-prefix [deprecated!]  
                            char *found_path = GB_property_file(false, path);

                            if (!found_path) {
                                fprintf(stderr, "file %s not found\n", path);
                                dbCreated = true;
                            }
                            else {
                                freeset(path, found_path);
                                // cppcheck-suppress deallocuse (false positive; path is reassigned to non-NULL above)
                                input = fopen(path, "rb");
                            }
                        }
                        else {
                            dbCreated = true;
                        }

                        if (dbCreated) {
                            fprintf(stderr, "Created new database \"%s\".\n", path);
                        }
                    }
                    else {
                        error = GBS_global_string("Database '%s' not found", path);
                        gbc   = 0;
                    }
                }
                if (input) {
                    if (strchr(opent, 'D')) { // we are loading properties -> be verboose
                        fprintf(stderr, "Using properties from '%s'\n", path);
                    }
                    time_of_main_file = GB_time_of_file(path);

                    i = (input != stdin) ? gb_read_in_uint32(input, 0) : 0;

                    if (is_binary_db_id(i)) {
                        {
                            arb_progress progress("Loading database", GB_size_of_FILE(input)/FILESIZE_GRANULARITY);
                            i = gb_read_bin(input, gbc, false, progress);                  // read or map whole db
                            progress.done();
                        }
                        gbc = Main->root_container;
                        fclose(input);

                        if (i) {
                            if (Main->allow_corrupt_file_recovery) {
                                GB_print_error();
                                GB_clear_error();
                            }
                            else {
                                gbc   = 0;
                                error = GBS_global_string("Failed to load database '%s'\n"
                                                          "Reason: %s",
                                                          path,
                                                          GB_await_error());
                            }
                        }

                        if (gbc && quickFile) {
                            long     err;
                            GB_ERROR err_msg;
                          load_quick_save_file_only :
                            err     = 0;
                            err_msg = 0;

                            input = fopen(quickFile, "rb");

                            if (input) {
                                GB_ULONG time_of_quick_file = GB_time_of_file(quickFile);
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
                                i = gb_read_in_uint32(input, 0);
                                if (is_binary_db_id(i)) {
                                    {
                                        arb_progress progress("Loading quicksave", GB_size_of_FILE(input)/FILESIZE_GRANULARITY);
                                        err = gb_read_bin(input, gbc, true, progress);
                                        progress.done();
                                    }
                                    fclose (input);

                                    if (err) {
                                        err_msg = GBS_global_string("Loading failed (file corrupt?)\n"
                                                                    "[Fail-Reason: '%s']",
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
                                    gbc = 0;
                                }
                                else {
                                    GB_export_error(error);
                                    GB_print_error();
                                    GB_clear_error();
                                    error = 0;
                                    GB_disable_quicksave(gbc, "Couldn't load last quicksave (your latest changes are NOT included)");
                                }
                            }
                        }
                        Main->qs.last_index = loadedQuickIndex; // determines which # will be saved next
                    }
                    else {
                        if (input != stdin) fclose(input);
                        error = gb_read_ascii(path, gbc);
                        GB_disable_quicksave(gbc, "Sorry, I cannot save differences to ascii files\n"
                                             "  Save whole database in binary mode first");
                    }
                }
            }
        }
        else {
            GB_disable_quicksave(gbc, "Database not part of this process");
            Main->mark_as_server();
            GB_begin_transaction(gbc);
        }

        if (error) gbcm_logout(Main, user);
    }

    gb_assert(error || gbc);

    if (error) {
        gb_delete_dummy_father(Main->dummy_father);
        gbc = NULL;
        delete Main;

        GB_export_error(error);
    }
    else {
        GB_commit_transaction(gbc);
        {
            GB_begin_transaction(gbc); // New Transaction, should be quicksaveable
            if (!strchr(opent, 'N')) {               // new format
                gb_convert_V2_to_V3(gbc); // Compression conversion
            }
            error = gb_load_key_data_and_dictionaries(Main);
            if (!error) error = gb_resort_system_folder_to_top(Main->root_container);
            // @@@ handle error 
            GB_commit_transaction(gbc);
        }
        Main->security_level = 0;
        gbl_install_standard_commands(gbc);

        if (Main->is_server()) {
            GBT_install_message_handler(gbc);
        }
        if (gb_verbose_mode && !dbCreated) GB_informationf("ARB: Loading '%s' done\n", path);
    }
    free(path);
    return gbc;
}

GBDATA *GB_open(const char *path, const char *opent)
{
    const char *user;
    user = GB_getenvUSER();
    return GB_login(path, opent, user);
}

GB_ERROR GBT_check_arb_file(const char *name) { // goes to header: __ATTR__USERESULT
    /*! Checks whether a file is an arb file (or ':')
     *
     * @result NULL if it is an arb file
     */

    GB_ERROR error = NULL;
    if (!strchr(name, ':'))  { // don't check remote DB
        if (GB_is_regularfile(name)) {
            FILE *in = fopen(name, "rb");
            if (!in) {
                error = GBS_global_string("Cannot find file '%s'", name);
            }
            else {
                long i = gb_read_in_uint32(in, 0);

                if (!is_binary_db_id(i)) {
                    const int ASC_HEADER_SIZE = 15;
                    char      buffer[ASC_HEADER_SIZE+1];

                    size_t read_bytes = fread(buffer+4, 1, ASC_HEADER_SIZE-4, in); // 4 bytes have already been read as binary ID
                    if (read_bytes == (ASC_HEADER_SIZE-4)) {
                        buffer[ASC_HEADER_SIZE] = 0;

                        const char *ascii_header = "/*ARBDB ASCII*/";

                        *(uint32_t*)buffer = i; // insert these bytes
                        if (strcmp(buffer, ascii_header) != 0) {
                            *(uint32_t*)buffer = reverse_byteorder(i);
                            if (strcmp(buffer, ascii_header) != 0) {
                                error = GBS_global_string("'%s' is not an arb file", name);
                            }
                        }
                    }
                }
                fclose(in);
            }
        }
        else {
            error = GBS_global_string("'%s' is no file", name);
        }
    }
    return error;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

void TEST_io_number() {
    struct data {
        long  val;
        short size_expd;
    };

    data DATA[] = {
        { 0x0,      1 },
        { 0x1,      1 },
        { 0x7f,     1 },
                    
        { 0x80,     2 },
        { 0x81,     2 },
        { 0xff,     2 },
        { 0x100,    2 },
        { 0x1234,   2 },
        { 0x3fff,   2 },
                    
        { 0x4000,   3 },
        { 0x4001,   3 },
        { 0xffff,   3 },
        { 0x1fffff, 3 },

        { 0x200000, 4 },
        { 0x7fffff, 4 },
        { 0x800002, 4 },
        { 0xffffff, 4 },
        { 0xfffffff, 4 },

#if defined(ARB_64)
        // the following entries are negative on 32bit systems; see ad_io_inline.h@bit-hell
        { 0x10000000, 5 },
        { 0x7fffffff, 5 },
        { 0x80000000, 5 },
        { 0x80808080, 5 },
        { 0xffffffff, 5 },
#endif
    };

    const char *numbers   = "numbers.test";
    long        writeSize = 0;
    long        readSize  = 0;

    {
        FILE *out = fopen(numbers, "wb");
        TEST_REJECT_NULL(out);

        long lastPos = 0;
        for (size_t i = 0; i<ARRAY_ELEMS(DATA); ++i) {
            data& d = DATA[i];
            TEST_ANNOTATE(GBS_global_string("val=0x%lx", d.val));
            gb_put_number(d.val, out);

            long pos           = ftell(out);
            long bytes_written = pos-lastPos;
            TEST_EXPECT_EQUAL(bytes_written, d.size_expd);

            writeSize += bytes_written;

            lastPos = pos;
        }
        TEST_ANNOTATE(NULL);

        fclose(out);
    }

    {
        FILE *in = fopen(numbers, "rb");
        TEST_REJECT_NULL(in);

        long lastPos = 0;

        for (size_t i = 0; i<ARRAY_ELEMS(DATA); ++i) {
            data& d = DATA[i];
            TEST_ANNOTATE(GBS_global_string("val=0x%lx", d.val));

            long val = gb_get_number(in);
            TEST_EXPECT_EQUAL(val, d.val);

            long pos        = ftell(in);
            long bytes_read = pos-lastPos;
            TEST_EXPECT_EQUAL(bytes_read, d.size_expd);

            readSize += bytes_read;

            lastPos = pos;
        }
        TEST_ANNOTATE(NULL);

        fclose(in);
    }

    TEST_EXPECT_EQUAL(GB_size_of_file(numbers), writeSize);
    TEST_EXPECT_EQUAL(writeSize, readSize);

    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(numbers));
}

void TEST_GBDATA_size() {
    // protect GBDATA/GBENTRY/GBCONTAINER against unwanted changes
    GBENTRY fakeEntry;

#if defined(ARB_64)

    TEST_EXPECT_EQUAL(sizeof(GBDATA), 40);
    TEST_EXPECT_EQUAL(sizeof(fakeEntry.info), 24);  // @@@ SIZOFINTERN may be increased (to use unused space); that change would need new mapfile version
    TEST_EXPECT_EQUAL(sizeof(fakeEntry.cache_index), 4);
    // 40+24+4 = 68 (where/what are the missing 4 bytes?; they seem to be at the end of the struct; maybe padding-bytes)

    TEST_EXPECT_EQUAL(sizeof(GBENTRY), 72);
    TEST_EXPECT_EQUAL(sizeof(GBCONTAINER), 104);

#else // !defined(ARB_64)

    TEST_EXPECT_EQUAL(sizeof(GBDATA), 24);
    TEST_EXPECT_EQUAL(sizeof(fakeEntry.info), 12);
    TEST_EXPECT_EQUAL(sizeof(fakeEntry.cache_index), 4);
    // 24+12+4 = 40 (here nothing is missing)

    TEST_EXPECT_EQUAL(sizeof(GBENTRY), 40);
    TEST_EXPECT_EQUAL(sizeof(GBCONTAINER), 60);

#endif
}
TEST_PUBLISH(TEST_GBDATA_size);

#endif // UNIT_TESTS
