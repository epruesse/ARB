// =============================================================== //
//                                                                 //
//   File      : ad_save_load.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <arb_file.h>
#include <arb_diff.h>
#include <arb_zfile.h>
#include <arb_defs.h>
#include <arb_strbuf.h>

#include "gb_key.h"
#include "gb_map.h"
#include "gb_load.h"
#include "ad_io_inline.h"

GB_MAIN_TYPE *gb_main_array[GB_MAIN_ARRAY_SIZE];

char *gb_findExtension(char *path) {
    char *punkt = strrchr(path, '.');
    if (punkt) {
        char *slash = strchr(punkt, '/');
        if (slash) punkt = 0;   //  slash after '.' -> no extension
    }
    return punkt;
}


/* CAUTION!!!
 *
 * The following functions (gb_quicksaveName, gb_oldQuicksaveName, gb_mapfile_name, gb_overwriteName)
 * use static buffers for the created filenames.
 *
 * So you have to make sure, to use only one instance of every of these
 * functions or to dup the string before using the second instance
 */

inline char *STATIC_BUFFER(SmartCharPtr& strvar, int minlen) {
    gb_assert(minlen > 0);
    if (strvar.isNull() || (strlen(&*strvar) < (size_t)(minlen-1))) {
        strvar = (char*)GB_calloc(minlen, 1);
    }
    return &*strvar;
}

GB_CSTR gb_oldQuicksaveName(GB_CSTR path, int nr) {
    static SmartCharPtr Qname;

    size_t  len   = strlen(path);
    char   *qname = STATIC_BUFFER(Qname, len+15);
    strcpy(qname, path);

    char *ext     = gb_findExtension(qname);
    if (!ext) ext = qname + len;

    if (nr==-1) sprintf(ext, ".arb.quick?");
    else    sprintf(ext, ".arb.quick%i", nr);

    return qname;
}

GB_CSTR gb_quicksaveName(GB_CSTR path, int nr) {
    static SmartCharPtr Qname;

    char *qname = STATIC_BUFFER(Qname, strlen(path)+4);
    strcpy(qname, path);

    char *ext     = gb_findExtension(qname);
    if (!ext) ext = qname + strlen(qname);

    if (nr==-1) sprintf(ext, ".a??");
    else    sprintf(ext, ".a%02i", nr);

    return qname;
}

GB_CSTR gb_mapfile_name(GB_CSTR path) {
    static SmartCharPtr Mapname;

    char *mapname = STATIC_BUFFER(Mapname, strlen(path)+4+1);
    strcpy(mapname, path);

    char *ext     = gb_findExtension(mapname);
    if (!ext) ext = mapname + strlen(mapname);

    strcpy(ext, ".ARM");

    return mapname;
}

GB_CSTR GB_mapfile(GBDATA *gb_main) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    return gb_mapfile_name(Main->path);
}

static GB_CSTR gb_overwriteName(GB_CSTR path) {
    static SmartCharPtr Oname;

    int   len   = strlen(path);
    char *oname = STATIC_BUFFER(Oname, len+2);

    strcpy(oname, path);
    strcpy(oname+len, "~");                         // append ~

    return oname;
}

static GB_CSTR gb_reffile_name(GB_CSTR path) {
    static SmartCharPtr Refname;

    size_t  len     = strlen(path);
    char   *refname = STATIC_BUFFER(Refname, len+4+1);
    memcpy(refname, path, len+1);

    const char *ext        = gb_findExtension(refname);
    size_t      ext_offset = ext ? (size_t)(ext-refname) : len;

    strcpy(refname+ext_offset, ".ARF");

    return refname;
}

static char *gb_full_path(const char *path) {
    char *res = 0;

    if (path[0] == '/') res = strdup(path);
    else {
        const char *cwd = GB_getcwd();

        if (path[0] == 0) res = strdup(cwd);
        else res              = GBS_global_string_copy("%s/%s", cwd, path);
    }
    return res;
}

static GB_ERROR gb_delete_reference(const char *master) {
    GB_ERROR    error      = 0;
    char       *fullmaster = gb_full_path(master);
    const char *fullref    = gb_reffile_name(fullmaster);

    GB_unlink_or_warn(fullref, &error);

    free(fullmaster);
    return error;
}

static GB_ERROR gb_create_reference(const char *master) {
    char       *fullmaster = gb_full_path(master);
    const char *fullref    = gb_reffile_name(fullmaster);
    GB_ERROR    error      = 0;
    FILE       *out        = fopen(fullref, "w");

    if (out) {
        fprintf(out, "***** The following files may be a link to %s ********\n", fullmaster);
        fclose(out);
        error = GB_failedTo_error("create reference file", NULL, GB_set_mode_of_file(fullref, 00666));
    }
    else {
        error = GBS_global_string("Cannot create reference file '%s'\n"
                                  "Your database was saved, but you should check "
                                  "write permissions in the destination directory!", fullref);
    }

    free(fullmaster);
    return error;
}

static GB_ERROR gb_add_reference(const char *master, const char *changes) {
    char       *fullmaster  = gb_full_path(master);
    char       *fullchanges = gb_full_path(changes);
    const char *fullref     = gb_reffile_name(fullmaster);
    GB_ERROR    error       = 0;
    FILE       *out         = fopen(fullref, "a");

    if (out) {
        fprintf(out, "%s\n", fullchanges);
        fclose(out);
        error = GB_failedTo_error("append to reference files", NULL, GB_set_mode_of_file(fullref, 00666));
    }
    else {
        error = GBS_global_string("Cannot add your file '%s'\n"
                                  "to the list of references of '%s'.\n"
                                  "Please ask the owner of that file not to delete it\n"
                                  "or save the entire database (that's recommended!)",
                                  fullchanges, fullref);
    }

    free(fullchanges);
    free(fullmaster);

    return error;
}

static GB_ERROR gb_remove_quick_saved(GB_MAIN_TYPE *Main, const char *path) {
    int i;
    GB_ERROR error = 0;

    for (i=0; i<GB_MAX_QUICK_SAVE_INDEX && !error; i++) GB_unlink_or_warn(gb_quicksaveName(path, i), &error);
    for (i=0; i<10 && !error; i++) GB_unlink_or_warn(gb_oldQuicksaveName(path, i), &error);
    if (Main) Main->qs.last_index = -1;

    RETURN_ERROR(error);
}

static GB_ERROR gb_remove_all_but_main(GB_MAIN_TYPE *Main, const char *path) {
    GB_ERROR error = gb_remove_quick_saved(Main, path);
    if (!error) GB_unlink_or_warn(gb_mapfile_name(path), &error); // delete old mapfile

    RETURN_ERROR(error);
}

static GB_ERROR deleteSuperfluousQuicksaves(GB_MAIN_TYPE *Main) {
    int       cnt   = 0;
    int       i;
    char     *path  = Main->path;
    GB_ERROR  error = 0;

    for (i=0; i <= GB_MAX_QUICK_SAVE_INDEX; i++) {
        GB_CSTR qsave = gb_quicksaveName(path, i);
        if (GB_is_regularfile(qsave)) cnt++;
    }

    for (i=0; cnt>GB_MAX_QUICK_SAVES && i <= GB_MAX_QUICK_SAVE_INDEX && !error; i++) {
        GB_CSTR qsave = gb_quicksaveName(path, i);
        if (GB_is_regularfile(qsave)) {
            if (GB_unlink(qsave)<0) error = GB_await_error();
            else cnt--;
        }
    }

    return error;
}

static GB_ERROR renameQuicksaves(GB_MAIN_TYPE *Main) {
    /* After hundred quicksaves rename the quicksave-files to a00...a09
     * to keep MS-DOS-compatibility (8.3)
     * Note: This has to be called just before saving.
     */

    GB_ERROR error = deleteSuperfluousQuicksaves(Main);
    if (!error) {
        const char *path = Main->path;
        int         i;
        int         j;

        for (i=0, j=0; i <= GB_MAX_QUICK_SAVE_INDEX; i++) {
            GB_CSTR qsave = gb_quicksaveName(path, i);

            if (GB_is_regularfile(qsave)) {
                if (i!=j) {                         // otherwise the filename is correct
                    char    *qdup = strdup(qsave);
                    GB_CSTR  qnew = gb_quicksaveName(path, j);

                    if (error) GB_warning(error);
                    error = GB_rename_file(qdup, qnew);
                    free(qdup);
                }

                j++;
            }
        }

        gb_assert(j <= GB_MAX_QUICK_SAVES);
        Main->qs.last_index = j-1;
    }

    return error;
}

// ------------------------
//      Ascii to Binary

long gb_ascii_2_bin(const char *source, GBENTRY *gbe) {
    const char *s = source;
    char        c = *(s++);

    A_TO_I(c);
    gbe->flags.compressed_data = c;

    long size;
    if (*s == ':') {
        size = 0;
        s++;
    }
    else {
        long i, k;
        for (i = 0, k = 8; k && (c = *(s++)); k--) {
            A_TO_I(c);
            i = (i<<4)+c;
        }
        size = i;
    }
    source = s;

    long len = 0;
    while ((c = *(s++))) {
        if ((c == '.') || (c=='-')) {
            len++;
            continue;
        }
        if ((c == ':') || (c=='=')) {
            len += 2;
            continue;
        }
        if (!(c = *(s++))) {
            return 1;
        };
        len++;
    }

    GBENTRY_memory storage(gbe, size, len);

    char *d = storage;
    s       = source;

    while ((c = *(s++))) {
        if (c == '.') {
            *(d++)=0;
            continue;
        }
        if (c == ':') {
            *(d++)=0;
            *(d++)=0;
            continue;
        }
        if (c == '-') {
            *(d++) = 0xff;
            continue;
        }
        if (c == '=') {
            *(d++) = 0xff;
            *(d++) = 0xff;
            continue;
        }
        A_TO_I(c);
        long i = c << 4;
        c = *(s++);
        A_TO_I(c);
        *(d++) = (char)(i + c);
    }

    return 0;
}
// ------------------------
//      Binary to Ascii

#define GB_PUT(c, out) do { if (c>=10) c+='A'-10; else c += '0'; *(out++) = (char)c; } while (0)

static GB_BUFFER gb_bin_2_ascii(GBENTRY *gbe) {
    signed char   *s, *out, c, mo;
    unsigned long  i;
    int            j;
    char          *buffer;
    int            k;

    const char *source     = gbe->data();
    long        len        = gbe->memsize();
    long        xtended    = gbe->size();
    int         compressed = gbe->flags.compressed_data;

    buffer = GB_give_buffer(len * 2 + 10);
    out = (signed char *)buffer;
    s = (signed char *)source, mo = -1;

    GB_PUT(compressed, out);
    if (!xtended) {
        *(out++) = ':';
    }
    else {
        for (i = 0xf0000000, j=28; j>=0; j-=4, i=i>>4) {
            k = (int)((xtended & i)>>j);
            GB_PUT(k, out);
        }
    }
    for (i = len; i; i--) {
        if (!(c = *(s++))) {
            if ((i > 1) && !*s) {
                *(out++) = ':';
                s ++;
                i--;
                continue;
            }
            *(out++) = '.';
            continue;
        }
        if (c == mo) {
            if ((i > 1) && (*s == -1)) {
                *(out++) = '=';
                s ++;
                i--;
                continue;
            }
            *(out++) = '-';
            continue;
        }
        j = ((unsigned char) c) >> 4;
        GB_PUT(j, out);
        j = c & 15;
        GB_PUT(j, out);
    }
    *(out++) = 0;
    return buffer;
}

// -------------------------
//      Write Ascii File

#define GB_PUT_OUT(c, out) do { if (c>=10) c+='A'-10; else c += '0'; putc(c, out); } while (0)

static bool gb_write_childs(FILE *out, GBCONTAINER *gbc, GBCONTAINER*& gb_writeFrom, GBCONTAINER *gb_writeTill, int indent);

static bool gb_write_one_child(FILE *out, GBDATA *gb, GBCONTAINER*& gb_writeFrom, GBCONTAINER *gb_writeTill, int indent) {
    /*! parameters same as for gb_write_childs()
     * The differences between both functions are:
     * - gb_write_one_child can be called for entries and writes container tags if called with a container
     * - gb_write_childs can only be called with containers and does NOT write enclosing container tags
     */
    {
        const char *key = GB_KEY(gb);
        if (!strcmp(key, GB_SYSTEM_FOLDER)) return true;   // do not save system folder

        if (!gb_writeFrom) {
            for (int i=indent; i--;) putc('\t', out);
            fprintf(out, "%s\t", key);
            if ((int)strlen(key) < 8) {
                putc('\t', out);
            }
        }
    }

    if (!gb_writeFrom) {
        if (gb->flags.security_delete ||
            gb->flags.security_write ||
            gb->flags.security_read ||
            gb->flags2.last_updated)
        {
            putc(':', out);
            char c;
            c= gb->flags.security_delete; GB_PUT_OUT(c, out);
            c= gb->flags.security_write;  GB_PUT_OUT(c, out);
            c= gb->flags.security_read;   GB_PUT_OUT(c, out);
            fprintf(out, "%u\t", gb->flags2.last_updated);
        }
        else {
            putc('\t', out);
        }
    }

    if (gb->is_container()) {
        if (!gb_writeFrom) {
            fprintf(out, "%%%c (%%\n", GB_read_flag(gb) ? '$' : '%');
            bool closeTags = gb_write_childs(out, gb->as_container(), gb_writeFrom, gb_writeTill, indent+1);
            if (!closeTags) return false;
        }
        if (gb_writeFrom && gb_writeFrom == gb->as_container()) gb_writeFrom = NULL;
        if (gb_writeTill && gb_writeTill == gb->as_container()) {
            return false;
        }
        if (!gb_writeFrom) {
            for (int i=indent+1; i--;) putc('\t', out);
            fprintf(out, "%%) /*%s*/\n\n", GB_KEY(gb));
        }
    }
    else {
        GBENTRY *gbe = gb->as_entry();
        switch (gbe->type()) {
            case GB_STRING: {
                GB_CSTR strng = GB_read_char_pntr(gbe);
                if (!strng) {
                    strng = "<entry was broken - replaced during ASCIIsave/arb_repair>";
                    GB_warningf("- replaced broken DB entry %s (data lost)\n", GB_get_db_path(gbe));
                }
                if (*strng == '%') {
                    putc('%', out);
                    putc('s', out);
                    putc('\t', out);
                }
                GBS_fwrite_string(strng, out);
                putc('\n', out);
                break;
            }
            case GB_LINK: {
                GB_CSTR strng = GB_read_link_pntr(gbe);
                if (*strng == '%') {
                    putc('%', out);
                    putc('l', out);
                    putc('\t', out);
                }
                GBS_fwrite_string(strng, out);
                putc('\n', out);
                break;
            }
            case GB_INT:
                fprintf(out, "%%i %li\n", GB_read_int(gbe));
                break;
            case GB_FLOAT:
                fprintf(out, "%%f %g\n", GB_read_float(gbe));
                break;
            case GB_BITS:
                fprintf(out, "%%I\t\"%s\"\n",
                        GB_read_bits_pntr(gbe, '-', '+'));
                break;
            case GB_BYTES: {
                const char *s = gb_bin_2_ascii(gbe);
                fprintf(out, "%%Y\t%s\n", s);
                break;
            }
            case GB_INTS: {
                const char *s = gb_bin_2_ascii(gbe);
                fprintf(out, "%%N\t%s\n", s);
                break;
            }
            case GB_FLOATS: {
                const char *s = gb_bin_2_ascii(gbe);
                fprintf(out, "%%F\t%s\n", s);
                break;
            }
            case GB_DB:
                gb_assert(0);
                break;
            case GB_BYTE:
                fprintf(out, "%%y %i\n", GB_read_byte(gbe));
                break;
            default:
                fprintf(stderr,
                        "ARBDB ERROR Key \'%s\' is of unknown type\n",
                        GB_KEY(gbe));
                fprintf(out, "%%%% (%% %%) /* unknown type */\n");
                break;
        }
    }
    return true;
}

static bool gb_write_childs(FILE *out, GBCONTAINER *gbc, GBCONTAINER*& gb_writeFrom, GBCONTAINER *gb_writeTill, int indent) {
    /*! write database entries to stream (used by ASCII database save)
     * @param out           output stream
     * @param gbc           container to write (including all subentries if gb_writeFrom and gb_writeTill are NULL).
     * @param gb_writeFrom  != NULL -> assume opening tag of container gb_writeFrom and all its subentries have already been written.
     *                                 Save closing tag and all following containers.
     * @param gb_writeTill  != NULL -> write till opening tag of container gb_writeTill
     * @param indent        indentation for container gbc
     * @return true if closing containers shall be written
     */

    gb_assert(gb_writeFrom == NULL || gb_writeTill == NULL); // not supported (yet)

    for (GBDATA *gb = GB_child(gbc); gb; gb = GB_nextChild(gb)) {
        if (gb->flags.temporary) continue;

        bool closeTags = gb_write_one_child(out, gb, gb_writeFrom, gb_writeTill, indent);
        if (!closeTags) return false;
    }

    return true;
}

// -------------------------
//      Read Binary File

long gb_read_bin_error(FILE *in, GBDATA *gbd, const char *text) {
    long p = (long)ftell(in);
    GB_export_errorf("%s in reading GB_file (loc %li=%lX) reading %s\n",
                     text, p, p, GB_KEY(gbd));
    GB_print_error();
    return 0;
}

// --------------------------
//      Write Binary File

static int gb_is_writeable(gb_header_list *header, GBDATA *gbd, long version, long diff_save) {
    /* Test whether to write any data to disc.
     *
     * version 1       write only latest data
     * version 2       write deleted entries two (which are not already stored to master file !!!
     *
     * try to avoid to access gbd (to keep it swapped out)
     */
    if (version == 2 && header->flags.changed==GB_DELETED) return 1;    // save delete flag
    if (!gbd) return 0;
    if (diff_save) {
        if (!header->flags.ever_changed) return 0;
        if (!gbd->ext || (gbd->ext->update_date<diff_save && gbd->ext->creation_date < diff_save))
            return 0;
    }
    if (gbd->flags.temporary) return 0;
    return 1;
}

static int gb_write_bin_sub_containers(FILE *out, GBCONTAINER *gbc, long version, long diff_save, int is_root);

static bool seen_corrupt_data = false;

static long gb_write_bin_rek(FILE *out, GBDATA *gbd, long version, long diff_save, long index_of_master_file) {
    int          i;
    GBCONTAINER *gbc  = 0;
    GBENTRY     *gbe  = 0;
    long         size = 0;
    GB_TYPES     type = gbd->type();

    if (type == GB_DB) {
        gbc = gbd->as_container();
    }
    else {
        gbe = gbd->as_entry();
        if (type == GB_STRING || type == GB_STRING_SHRT) {
            size = gbe->size();
            if (!gbe->flags.compressed_data && size < GBTUM_SHORT_STRING_SIZE) {
                const char *data = gbe->data();
                size_t      len  = strlen(data); // w/o zero-byte!

                if ((long)len == size) {
                    type = GB_STRING_SHRT;
                }
                else {
                    // string contains zero-byte inside data or misses trailing zero-byte
                    type              = GB_STRING; // fallback to safer type
                    seen_corrupt_data = true;
                    GB_warningf("Corrupted entry detected:\n"
                                "entry: '%s'\n"
                                "data:  '%s'",
                                GB_get_db_path(gbe),
                                data);
                }
            }
            else {
                type = GB_STRING;
            }
        }
    }

    i =     (type<<4)
        +   (gbd->flags.security_delete<<1)
        +   (gbd->flags.security_write>>2);
    putc(i, out);

    i =     ((gbd->flags.security_write &3) << 6)
        +   (gbd->flags.security_read<<3)
        +   (gbd->flags.compressed_data<<2)
        +   ((GB_ARRAY_FLAGS(gbd).flags&1)<<1)
        +   (gbd->flags.unused);
    putc(i, out);

    gb_put_number(GB_ARRAY_FLAGS(gbd).key_quark, out);

    if (diff_save) {
        gb_put_number(index_of_master_file, out);
    }

    i = gbd->flags2.last_updated;
    putc(i, out);

    if (type == GB_STRING_SHRT) {
        const char *data = gbe->data();
        gb_assert((long)strlen(data) == size);

        i = fwrite(data, size+1, 1, out);
        return i <= 0 ? -1 : 0;
    }

    switch (type) {
        case GB_DB:
            i = gb_write_bin_sub_containers(out, gbc, version, diff_save, 0);
            return i;

        case GB_INT: {
            GB_UINT4 buffer = (GB_UINT4)htonl(gbe->info.i);
            if (!fwrite((char *)&buffer, sizeof(float), 1, out)) return -1;
            return 0;
        }
        case GB_FLOAT:
            if (!fwrite((char *)&gbe->info.i, sizeof(float), 1, out)) return -1;
            return 0;
        case GB_LINK:
        case GB_BITS:
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            size = gbe->size();
            // fall-through
        case GB_STRING: {
            long memsize = gbe->memsize();
            gb_put_number(size, out);
            gb_put_number(memsize, out);
            i = fwrite(gbe->data(), (size_t)memsize, 1, out);
            if (memsize && !i) return -1;
            return 0;
        }
        case GB_BYTE:
            putc((int)(gbe->info.i), out);
            return 0;
        default:
            gb_assert(0); // unknown type
            return -1;
    }
}

static int gb_write_bin_sub_containers(FILE *out, GBCONTAINER *gbc, long version, long diff_save, int is_root) {
    gb_header_list *header;
    uint32_t        i, index;

    header = GB_DATA_LIST_HEADER(gbc->d);
    gb_assert(gbc->d.nheader >= 0);
    for (i=0, index = 0; index < (uint32_t)gbc->d.nheader; index++) {
        if (gb_is_writeable(&(header[index]), GB_HEADER_LIST_GBD(header[index]), version, diff_save)) i++;
    }

    if (!is_root) {
        gb_put_number(i, out);
    }
    else {
        gb_write_out_uint32(i, out);
    }

    uint32_t counter = 0;
    for (index = 0; index < (uint32_t)gbc->d.nheader; index++) {
        GBDATA *h_gbd;

        if (header[index].flags.changed == GB_DELETED_IN_MASTER) {  // count deleted items in master, because of index renaming
            counter ++;
            continue;
        }

        h_gbd = GB_HEADER_LIST_GBD(header[index]);

        if (!gb_is_writeable(&(header[index]), h_gbd, version, diff_save)) {
            if (version <= 1 && header[index].flags.changed == GB_DELETED) {
                header[index].flags.changed = GB_DELETED_IN_MASTER; // mark deleted in master
            }
            continue;
        }

        if (h_gbd) {
            i = (int)gb_write_bin_rek(out, h_gbd, version, diff_save, index-counter);
            if (i) return i;
        }
        else {
            if (header[index].flags.changed == GB_DELETED) {
                putc(0, out);
                putc(1, out);
                gb_put_number(index - counter, out);
            }
        }
    }
    return 0;
}

static int gb_write_bin(FILE *out, GBCONTAINER *gbc, uint32_t version) {
    /* version 1 write master arb file
     * version 2 write slave arb file (aka quick save file)
     */

    gb_assert(!seen_corrupt_data);

    int           diff_save = 0;
    GB_MAIN_TYPE *Main      = GBCONTAINER_MAIN(gbc);

    gb_write_out_uint32(GBTUM_MAGIC_NUMBER, out);
    fprintf(out, "\n this is the binary version of the gbtum data file version %li\n", (long)version);
    putc(0, out);
    fwrite("vers", 4, 1, out);
    gb_write_out_uint32(0x01020304, out);
    gb_write_out_uint32(version, out);
    fwrite("keys", 4, 1, out);

    for (long i=1; i<Main->keycnt; i++) {
        gb_Key &KEY = Main->keys[i];
        if (KEY.nref>0) {
            gb_put_number(KEY.nref, out);
            fputs(KEY.key, out);
        }
        else {
            putc(0, out);       // 0 nref
            putc(1, out);       // empty key
        }
        putc(0, out);

    }
    putc(0, out);
    putc(0, out);
    fwrite("time", 4, 1, out); {
        unsigned int k;
        for (k=0; k<Main->last_updated; k++) {
            fprintf(out, "%s", Main->dates[k]);
            putc(0, out);
        }
    }
    putc(0, out);
    fwrite("data", 4, 1, out);
    if (version == 2) diff_save = (int)Main->last_main_saved_transaction+1;

    return gb_write_bin_sub_containers(out, gbc, version, diff_save, 1);
}

// ----------------------
//      save database

GB_ERROR GB_save(GBDATA *gb, const char *path, const char *savetype)
    /*! save database
     * @param gb database root
     * @param path filename (if NULL -> use name stored in DB; otherwise store name in DB)
     * @param savetype @see GB_save_as()
     */
{
    if (path && strchr(savetype, 'S') == 0) // 'S' dumps to stdout -> do not change path
    {
        freedup(GB_MAIN(gb)->path, path);
    }
    return GB_save_as(gb, path, savetype);
}

GB_ERROR GB_create_parent_directory(const char *path) {
    GB_ERROR error = NULL;
    char *parent;
    GB_split_full_path(path, &parent, NULL, NULL, NULL);
    if (parent) {
        if (!GB_is_directory(parent)) error = GB_create_directory(parent);
        free(parent);
    }
    return error;
}

GB_ERROR GB_create_directory(const char *path) {
    GB_ERROR error = 0;
    if (!GB_is_directory(path)) {
        error = GB_create_parent_directory(path);
        if (!error) {
            int res = mkdir(path, ACCESSPERMS);
            if (res) error = GB_IO_error("creating directory", path);
        }
        error = GB_failedTo_error("GB_create_directory", path, error);
    }
    return error;
}

GB_ERROR GB_save_in_arbprop(GBDATA *gb, const char *path, const char *savetype) {
    /*! save database inside arb-properties-directory.
     * Automatically creates subdirectories as needed.
     * @param gb database root
     * @param path filename (if NULL -> use name stored in DB)
     * @param savetype @see GB_save_as()
     */

    char     *fullname = strdup(GB_path_in_arbprop(path ? path : GB_MAIN(gb)->path));
    GB_ERROR  error    = GB_create_parent_directory(fullname);
    if (!error) error = GB_save_as(gb, fullname, savetype);
    free(fullname);

    return error;
}

GB_ERROR GB_MAIN_TYPE::check_saveable(const char *new_path, const char *flags) const {
    /* Check wether file can be stored at destination
     *  'f' in flags means 'force' => ignores main->disabled_path
     *  'q' in flags means 'quick save'
     *  'n' in flags means destination must be empty
     */

    GB_ERROR error = NULL;
    if (is_client()) {
        error = "You cannot save a remote database,\nplease use save button in master program";
    }
    else if (opentype == gb_open_read_only_all) {
        error = "Database is read only";
    }
    else if (strchr(new_path, ':')) {
        error = "Your database name may not contain a ':' character\nChoose a different name";
    }
    else {
        char *fullpath = gb_full_path(new_path);
        if (disabled_path && !strchr(flags, 'f')) {
            if (GBS_string_matches(fullpath, disabled_path, GB_MIND_CASE)) {
                error = GBS_global_string("You are not allowed to save your database in this directory,\n"
                                          "Please select 'save as' and save your data to a different location");
            }
        }

        if (!error) {
            // check whether destination directory exists
            char *lslash = strrchr(fullpath, '/');
            if (lslash) {
                lslash[0] = 0;
                if (!GB_is_directory(fullpath)) {
                    error = GBS_global_string("Directory '%s' doesn't exist", fullpath);
                }
                lslash[0] = '/';
            }
        }
        free(fullpath);
    }
    
    if (!error && !strchr(flags, 'q')) {
        long mode = GB_mode_of_link(new_path);
        if (mode >= 0 && !(mode & S_IWUSR)) { // no write access -> looks like a master file
            error = GBS_global_string("Your selected file '%s'\n"
                                      "already exists and is write protected!\n"
                                      "This happens e.g. if your file is a MASTER ARB FILE which is\n"
                                      "used by multiple quicksaved databases.\n"
                                      "If you want to save it nevertheless, delete it first, but\n"
                                      "note that doing this will render all these quicksaves useless!",
                                      new_path);
        }
    }
    
    if (!error && strchr(flags, 'n') && GB_time_of_file(new_path)) {
        error = GBS_global_string("Your destination file '%s' already exists.\n"
                                  "Delete it manually!", new_path);
    }

#if (MEMORY_TEST==1)
    if (!error && strchr(flags, 'm')) {
        error = "It's impossible to save mapfiles (ARBDB is MEMORY_TEST mode 1)";
    }
#endif

    return error;
}

static GB_ERROR protect_corruption_error(const char *savepath) {
    GB_ERROR error = NULL;
    gb_assert(seen_corrupt_data);
    if (strstr(savepath, "CORRUPTED") == 0) {
        error = "Severe error: Corrupted data detected during save\n"
            "ARB did NOT save your database!\n"
            "Advices:\n"                                                    //|
            "* If your previous (quick)save was not long ago, your savest\n"
            "  option is to drop the changes since then, by reloading the not\n"
            "  corrupted database and redo your changes. If you can reproduce\n"
            "  the bug that corrupted the entries, please report it!\n"
            "* If that is no option (because too much work would be lost)\n"
            "  you can force saving the corrupted database by adding the text\n"
            "  'CORRUPTED' to the database name. After doing that, do NOT\n"
            "  quit ARB, instead try to find and fix all corrupted entries\n"
            "  that were listed below. Manually enter their original values\n"
            "  (in case you want to lookup or copy&paste some values, you may\n"
            "   open the last saved version of this database using\n"
            "   'Start second database').\n"
            "  Saving the database again will show all remaining unfixed\n"
            "  entries. If no more corrupted entries show up, you can safely\n"
            "  continue to work with that database.";
    }
    else {
        GB_warning("Warning: Saved corrupt database");
    }
    seen_corrupt_data = false;
    return error;
}

#define SUPPORTED_COMPRESSION_FLAGS "zBx"

const char *GB_get_supported_compression_flags(bool verboose) {
    if (verboose) {
        GBS_strstruct doc(50);
        for (int f = 0; SUPPORTED_COMPRESSION_FLAGS[f]; ++f) {
            if (f) doc.cat(", ");
            switch (SUPPORTED_COMPRESSION_FLAGS[f]) {
                // Note: before changing produced format, see callers (esp. AWT_insert_DBcompression_selector)
                case 'z': doc.cat("z=gzip"); break;
                case 'B': doc.cat("B=bzip2"); break;
                case 'x': doc.cat("x=xz"); break;
                default:
                    gb_assert(0); // undocumented flag
                    break;
            }
        }
        return GBS_static_string(doc.get_data());
    }
    return SUPPORTED_COMPRESSION_FLAGS;
}


class ArbDBWriter : virtual Noncopyable {
    GB_MAIN_TYPE *Main;

    GB_ERROR error;

    FILE *out;
    bool  saveASCII;
    bool  saveMapfile;

    char       *given_path;
    const char *as_path;
    char       *sec_path;
    char       *mappath;
    char       *sec_mappath;

    struct Levels {
        int security;
        int transaction;

        void readFrom(GB_MAIN_TYPE *main) {
            security    = main->security_level;
            transaction = main->get_transaction_level();
        }
        void writeTo(GB_MAIN_TYPE *main) {
            main->security_level       = security;
            if (main->transaction_level>0) {
                GB_commit_transaction(main->root_container);
            }
            if (transaction) {
                GB_begin_transaction(main->root_container);
            }
            gb_assert(main->transaction_level == transaction || main->transaction_level == -1);
        }

    };

    Levels save; // wanted levels during save (i.e. inside saveFromTill/finishSave)

    bool finishCalled; // @@@ replace by state-enum
    bool dump_to_stdout;
    bool outOfOrderSave;
    bool deleteQuickAllowed;

public:
    ArbDBWriter(GB_MAIN_TYPE *Main_)
        : Main(Main_),
          error(NULL),
          out(NULL),
          saveASCII(false),
          saveMapfile(false),
          given_path(NULL),
          sec_path(NULL),
          mappath(NULL),
          sec_mappath(NULL),
          finishCalled(false),
          dump_to_stdout(false),
          outOfOrderSave(false),
          deleteQuickAllowed(false)
    {
    }
    ~ArbDBWriter() {
        gb_assert(finishCalled); // you have to call finishSave()! (even in error-case)

        free(sec_mappath);
        free(mappath);
        free(sec_path);
        free(given_path);
    }

    GB_ERROR startSaveAs(const char *given_path_, const char *savetype) {
        gb_assert(!error);

        given_path = nulldup(given_path_);
        as_path    = given_path;

        if (strchr(savetype, 'a'))      saveASCII = true;
        else if (strchr(savetype, 'b')) saveASCII = false;
        else error                                = GBS_global_string("Invalid savetype '%s' (expected 'a' or 'b')", savetype);

        if (!error) {
            if (!as_path) as_path              = Main->path;
            if (!as_path || !as_path[0]) error = "Please specify a savename";
            else error                         = Main->check_saveable(as_path, savetype);
        }

        FileCompressionMode compressMode = ZFILE_UNCOMPRESSED;
        if (!error) {
            struct {
                char                flag;
                FileCompressionMode mode;
            } supported[] = {
                { 'z', ZFILE_GZIP },
                { 'B', ZFILE_BZIP2 },
                { 'x', ZFILE_XZ },
                // Please document new flags in GB_save_as() below.
            };

            STATIC_ASSERT(ARRAY_ELEMS(supported) == ZFILE_REAL_CMODES); // (after adding a new FileCompressionMode it should be supported here)

            for (size_t comp = 0; !error && comp<ARRAY_ELEMS(supported); ++comp) {
                if (strchr(savetype, supported[comp].flag)) {
                    if (compressMode == ZFILE_UNCOMPRESSED) {
                        compressMode = supported[comp].mode;
                    }
                    else {
                        error = "Multiple compression modes specified";
                    }
                }
                gb_assert(strchr(SUPPORTED_COMPRESSION_FLAGS, supported[comp].flag)); // flag gets not tested -> add to SUPPORTED_COMPRESSION_FLAGS
            }
        }

        if (!error && Main->transaction_level>1) {
            error = "Save only allowed with transaction_level <= 1";
        }

        save.readFrom(Main); // values for error case
        if (!error) {
            // unless saving to a fifo, we append ~ to the file name we write to,
            // and move that file if and when everything has gone well
            sec_path       = strdup(GB_is_fifo(as_path) ? as_path : gb_overwriteName(as_path));
            dump_to_stdout = strchr(savetype, 'S');
            out            = dump_to_stdout ? stdout : ARB_zfopen(sec_path, "w", compressMode, error);

            if (!out) {
                gb_assert(error);
                error = GBS_global_string("While saving database '%s': %s", sec_path, error);
            }
            else {
                save.security    = 7;
                save.transaction = 1;

                seen_corrupt_data = false;

                outOfOrderSave     = strchr(savetype, 'f');
                deleteQuickAllowed = !outOfOrderSave && !dump_to_stdout;
                {
                    if (saveASCII) {
                        fprintf(out, "/*ARBDB ASCII*/\n");
                    }
                    else {
                        saveMapfile = strchr(savetype, 'm');
                    }
                }
            }
        }

        return error;
    }

private:
    void writeContainerOrChilds(GBCONTAINER *top, GBCONTAINER *from, GBCONTAINER *till) {
        if (top == Main->root_container) {
            gb_write_childs(out, top, from, till, 0);
        }
        else {
            int root_indent = 0;
            {
                GBDATA *stepUp = top;
                while (stepUp != Main->root_container) {
                    stepUp = stepUp->get_father();
                    ++root_indent;
                    gb_assert(stepUp); // fails if 'top' is not member of 'Main'
                }
                --root_indent; // use 0 for direct childs of root_container
            }
            gb_assert(root_indent>=0);
            gb_write_one_child(out, top, from, till, root_indent);
        }
    }
public:

    GB_ERROR saveFromTill(GBDATA *gb_from, GBDATA *gb_till) { // @@@ pass containers
        Levels org; org.readFrom(Main);
        save.writeTo(Main); // set save transaction-state and security_level

        gb_assert(!error);
        if (saveASCII) {
            if (gb_from == gb_till) {
                writeContainerOrChilds(gb_from->as_container(), NULL, NULL);
            }
            else {
                bool from_is_ancestor = false;
                bool till_is_ancestor = false;

                GBDATA *gb_from_ancestor = GB_get_father(gb_from);
                GBDATA *gb_till_ancestor = GB_get_father(gb_till);

                while (gb_from_ancestor || gb_till_ancestor) {
                    if (gb_from_ancestor && gb_from_ancestor == gb_till) till_is_ancestor = true;
                    if (gb_till_ancestor && gb_till_ancestor == gb_from) from_is_ancestor = true;

                    if (gb_from_ancestor) gb_from_ancestor = GB_get_father(gb_from_ancestor);
                    if (gb_till_ancestor) gb_till_ancestor = GB_get_father(gb_till_ancestor);
                }

                if (from_is_ancestor) {
                    writeContainerOrChilds(gb_from->as_container(), NULL, gb_till->as_container());
                }
                else if (till_is_ancestor) {
                    writeContainerOrChilds(gb_till->as_container(), gb_from->as_container(), NULL);
                }
                else {
                    error = "Invalid use (gb_from has to be ancestor of gb_till or vv.)";
                }
            }
        }
        else { // save binary (performed in finishSave)
            if (gb_from != Main->root_container || gb_till != Main->root_container) {
                error = "streamed saving only supported for ascii database format";
            }
        }

        org.writeTo(Main); // restore original transaction-state and security_level
        return error;
    }
    GB_ERROR finishSave() {
        Levels org; org.readFrom(Main);
        save.writeTo(Main); // set save transaction-state and security_level

        finishCalled = true;

        if (out) {
            int result = 0;
            if (saveASCII) {
                freedup(Main->qs.quick_save_disabled, "Database saved in ASCII mode");
                if (deleteQuickAllowed) error = gb_remove_all_but_main(Main, as_path);
            }
            else {
                mappath = strdup(gb_mapfile_name(as_path));
                if (saveMapfile) {
                    // it's necessary to save the mapfile FIRST,
                    // cause this re-orders all GB_CONTAINERs containing NULL-entries in their header
                    sec_mappath = strdup(gb_overwriteName(mappath));
                    error       = gb_save_mapfile(Main, sec_mappath);
                }
                else GB_unlink_or_warn(mappath, &error); // delete old mapfile
                if (!error) result |= gb_write_bin(out, Main->root_container, 1);
            }

            // org.writeTo(Main); // Note: was originally done here

            if (result != 0) {
                error = GB_IO_error("writing", sec_path);
                if (!dump_to_stdout) {
                    GB_ERROR close_error   = ARB_zfclose(out, sec_path);
                    if (close_error) error = GBS_global_string("%s\n(close reports: %s)", error, close_error);
                }
            }
            else {
                if (!dump_to_stdout) error = ARB_zfclose(out, sec_path);
            }

            if (!error && seen_corrupt_data) {
                error = protect_corruption_error(as_path);
            }

            if (!error && !saveASCII) {
                if (!outOfOrderSave) freenull(Main->qs.quick_save_disabled); // delete reason, why quicksaving was disallowed
                if (deleteQuickAllowed) error = gb_remove_quick_saved(Main, as_path);
            }

            if (!dump_to_stdout) {
                if (error) {
                    if (sec_mappath) GB_unlink_or_warn(sec_mappath, NULL);
                    GB_unlink_or_warn(sec_path, NULL);
                }
                else {
                    bool unlinkMapfiles = false;
                    if (strcmp(sec_path, as_path) != 0) {
                        error = GB_rename_file(sec_path, as_path);
                    }

                    if (error) {
                        unlinkMapfiles = true;
                    }
                    else if (sec_mappath) {
                        error             = GB_rename_file(sec_mappath, mappath);
                        if (!error) error = GB_set_mode_of_file(mappath, GB_mode_of_file(as_path)); // set mapfile to same mode ...
                        if (!error) error = GB_set_time_of_file(mappath, GB_time_of_file(as_path)); // ... and same time as DB file
                        if (error) {
                            GB_warningf("Error: %s\n[Falling back to non-fastload-save]", error);
                            error          = 0;
                            unlinkMapfiles = true;
                        }
                    }

                    if (unlinkMapfiles) {
                        GB_unlink_or_warn(sec_mappath, NULL);
                        GB_unlink_or_warn(mappath, NULL);
                    }

                    if (!error) {
                        error = Main->qs.quick_save_disabled == 0
                            ? gb_create_reference(as_path)
                            : gb_delete_reference(as_path);
                    }
                }
            }

            if (!error && !outOfOrderSave) {
                Main->last_saved_transaction      = GB_read_clock(Main->root_container);
                Main->last_main_saved_transaction = GB_read_clock(Main->root_container);
                Main->last_saved_time             = GB_time_of_day();
            }
        }

        org.writeTo(Main); // restore original transaction-state and security_level
        return error;
    }
};

GB_ERROR GB_MAIN_TYPE::save_as(const char *as_path, const char *savetype) {
    ArbDBWriter dbwriter(this);
    GB_ERROR    error = dbwriter.startSaveAs(as_path, savetype);
    if (!error) error = dbwriter.saveFromTill(root_container, root_container);
    error = dbwriter.finishSave();
    return error;
}

GB_ERROR GB_save_as(GBDATA *gbd, const char *path, const char *savetype) {
    /*! Save whole database
     *
     * @param gbd database root
     * @param path filename (if NULL -> use name stored in DB)
     * @param savetype
     *          'a' ascii
     *          'b' binary
     *          'm' save mapfile (only together with binary)
     *
     *          'f' force saving even in disabled path to a different directory (out of order save)
     *          'S' save to stdout (used in arb_2_ascii when saving to stdout; also useful for debugging)
     *
     *          Extra compression flags:
     *          'z' stream through gzip/pigz
     *          'B' stream through bzip2
     *          'x' stream through xz
     */

    GB_ERROR error = NULL;

    gb_assert(savetype);

    if (!gbd) {
        error = "got no DB";
    }
    else {
        error = GB_MAIN(gbd)->save_as(path, savetype);
    }

    RETURN_ERROR(error);
}

GB_ERROR GB_MAIN_TYPE::check_quick_save() const {
    /* is quick save allowed?
     * return NULL if allowed, error why disallowed otherwise.
     */

    if (qs.quick_save_disabled) {
        return GBS_global_string("Save Changes Disabled, because\n"
                                 "    '%s'\n"
                                 "    Save whole database using binary mode first",
                                 qs.quick_save_disabled);
    }
    return NULL;
}

GB_ERROR GB_delete_database(GB_CSTR filename) {
    GB_ERROR error = 0;

    if (GB_unlink(filename)<0) error = GB_await_error();
    else error                       = gb_remove_all_but_main(0, filename);

    return error;
}

GB_ERROR GB_MAIN_TYPE::save_quick_as(const char *as_path) {
    GB_ERROR error = NULL;
    if (!as_path || !strlen(as_path)) {
        error = "Please specify a file name";
    }
    else if (strcmp(as_path, path) == 0) {    // same name (no rename)
        error = save_quick(as_path);
    }
    else {
        error = check_quick_saveable(as_path, "bn");

        if (!error) {
            FILE *fmaster = fopen(path, "r");         // old master !!!!
            if (!fmaster) {                                 // Oh no, where is my old master
                error = GBS_global_string("Save Changes is missing master ARB file '%s',\n"
                                          "    save database first", path);
            }
            else {
                fclose(fmaster);
            }
        }
        if (!error) {
            if (GB_unlink(as_path)<0) { // delete old file
                error = GBS_global_string("File '%s' already exists and could not be deleted\n"
                                          "(Reason: %s)",
                                          as_path, GB_await_error());
            }
        }
        if (!error) {
            char *org_master = S_ISLNK(GB_mode_of_link(path))
                ? GB_follow_unix_link(path)
                : strdup(path);

            error = gb_remove_all_but_main(this, as_path);
            if (!error) {
                long mode = GB_mode_of_file(org_master);
                if (mode & S_IWUSR) {
                    GB_ERROR sm_error = GB_set_mode_of_file(org_master, mode & ~(S_IWUSR | S_IWGRP | S_IWOTH));
                    if (sm_error) {
                        GB_warningf("%s\n"
                                    "Ask the owner to remove write permissions from that master file.\n"
                                    "NEVER delete or change it, otherwise your quicksaves will be rendered useless!", 
                                    sm_error);
                    }
                }
                char *full_path_of_source;
                if (strchr(as_path, '/') || strchr(org_master, '/')) {
                    // dest or source in different directory
                    full_path_of_source = gb_full_path(org_master);
                }
                else {
                    full_path_of_source = strdup(org_master);
                }

                error = GB_symlink(full_path_of_source, as_path);
                if (!error) {
                    if ((uid_t)GB_getuid_of_file(full_path_of_source) != getuid()) {
                        GB_warningf("**** WARNING ******\n"
                                    "   You are using the file '%s' \n"
                                    "   as reference for your saved changes.\n"
                                    "   That file is owned by ANOTHER USER.\n"
                                    "   If that user deletes or overwrites that file, your saved\n"
                                    "   changes will get useless (=they will be lost)!\n"
                                    "   You should only 'save changes as' if you understand what that means.\n"
                                    "   Otherwise use 'Save whole database as' NOW!", full_path_of_source);
                    }

                    GB_ERROR warning = gb_add_reference(full_path_of_source, as_path);
                    if (warning) GB_warning(warning);

                    freedup(path, as_path);                      // Symlink created -> rename allowed

                    qs.last_index = -1; // Start with new quicks (next index will be 0)
                    error = save_quick(as_path);
                }
                free(full_path_of_source);
            }
            free(org_master);
        }
    }

    RETURN_ERROR(error);
}

GB_ERROR GB_save_quick_as(GBDATA *gbd, const char *path) {
    return GB_MAIN(gbd)->save_quick_as(path);
}

GB_ERROR GB_MAIN_TYPE::save_quick(const char *refpath) {
    GB_ERROR error = check_quick_saveable(refpath, "q");

    if (!error && refpath && strcmp(refpath, path) != 0) {
        error = GBS_global_string("master file rename '%s'!= '%s',\n"
                                  "save database first", refpath, path);
    }
    if (!error) {
        FILE *fmaster = fopen(path, "r");

        if (!fmaster) {
            error = GBS_global_string("Quick save is missing master ARB file '%s',\n"
                                      "save database first", refpath);
        }
        else {
            fclose(fmaster);
        }
    }
    if (!error && is_client()) error = "You cannot save a remote database";
    if (!error) {
        qs.last_index++;
        if (qs.last_index > GB_MAX_QUICK_SAVE_INDEX) renameQuicksaves(this);

        GB_CSTR qck_path = gb_quicksaveName(path, qs.last_index);
        GB_CSTR sec_path = gb_overwriteName(qck_path);

        FILE *out       = fopen(sec_path, "w");
        if (!out) error = GBS_global_string("Cannot save file to '%s'", sec_path);
        else {
            long erg;
            {
                const int org_security_level    = security_level;
                int       org_transaction_level = get_transaction_level();

                if (!org_transaction_level) transaction_level = 1;
                else {
                    if (org_transaction_level> 0) {
                        GB_commit_transaction(root_container);
                        GB_begin_transaction(root_container);
                    }
                }

                security_level    = 7;
                seen_corrupt_data = false;

                erg = gb_write_bin(out, root_container, 2);

                security_level    = org_security_level;
                transaction_level = org_transaction_level;
            }

            erg |= fclose(out);

            if (erg!=0) error = GBS_global_string("Cannot write to '%s'", sec_path);
            else {
                if (seen_corrupt_data) {
                    gb_assert(!error);
                    error = protect_corruption_error(qck_path);
                }
                if (!error) error = GB_rename_file(sec_path, qck_path);
                if (error) GB_unlink_or_warn(sec_path, NULL);
            }
        }

        if (error) qs.last_index--; // undo index increment
        else {
            last_saved_transaction = GB_read_clock(root_container);
            last_saved_time        = GB_time_of_day();

            error = deleteSuperfluousQuicksaves(this);
        }
    }

    RETURN_ERROR(error);
}

GB_ERROR GB_save_quick(GBDATA *gbd, const char *refpath) {
    return GB_MAIN(gbd)->save_quick(refpath);
}


void GB_disable_path(GBDATA *gbd, const char *path) {
    // disable directories for save
    freeset(GB_MAIN(gbd)->disabled_path, path ? GBS_eval_env(path) : NULL);
}

long GB_last_saved_clock(GBDATA *gb_main) {
    return GB_MAIN(gb_main)->last_saved_transaction;
}

GB_ULONG GB_last_saved_time(GBDATA *gb_main) {
    return GB_MAIN(gb_main)->last_saved_time;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

#define SAVE_AND_COMPARE(gbd, save_as, savetype, compare_with) \
    TEST_EXPECT_NO_ERROR(GB_save_as(gbd, save_as, savetype));  \
    TEST_EXPECT_FILES_EQUAL(save_as, compare_with)

static GB_ERROR modify_db(GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    GB_ERROR  error          = NULL;
    GBDATA   *gb_container   = GB_create_container(gb_main, "container");
    if (!gb_container) error = GB_await_error();
    else {
        GBDATA *gb_entry     = GB_create(gb_container, "str", GB_STRING);
        if (!gb_entry) error = GB_await_error();
        else    error        = GB_write_string(gb_entry, "text");
        // else    error        = GB_write_string(gb_entry, "bla"); // provoke error in file compare
    }
    return error;
}

// #define TEST_AUTO_UPDATE // uncomment to auto-update binary and quicksave testfiles (needed once after changing ascii testfile or modify_db())

#define TEST_loadsave_CLEANUP() TEST_EXPECT_ZERO(system("rm -f [abr]2[ab]*.* master.* slave.* renamed.* fast.* fast2a.* TEST_loadsave.ARF"))

void TEST_SLOW_loadsave() {
    GB_shell shell;
    TEST_loadsave_CLEANUP();

    // test non-existing DB
    TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_open("nonexisting.arb", "r"), "'nonexisting.arb' not found");
    TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GB_open("nonexisting.arb", "rw"), "'nonexisting.arb' not found");
    {
        GBDATA *gb_new;
        TEST_EXPECT_RESULT__NOERROREXPORTED(gb_new = GB_open("nonexisting.arb", "w")); // create new DB
        GB_close(gb_new);
    }

    // the following DBs have to be provided in directory ../UNIT_TESTER/run
    const char *bin_db = "TEST_loadsave.arb";
    const char *asc_db = "TEST_loadsave_ascii.arb";

    GBDATA *gb_asc;
    TEST_EXPECT_RESULT__NOERROREXPORTED(gb_asc = GB_open(asc_db, "rw"));

#if defined(TEST_AUTO_UPDATE)
    TEST_EXPECT_NO_ERROR(GB_save_as(gb_asc, bin_db, "b"));
#endif // TEST_AUTO_UPDATE

    GBDATA *gb_bin;
    TEST_EXPECT_RESULT__NOERROREXPORTED(gb_bin = GB_open(bin_db, "rw"));

    // test ASCII / BINARY compatibility
    SAVE_AND_COMPARE(gb_asc, "a2a.arb", "a", asc_db);
    SAVE_AND_COMPARE(gb_asc, "a2b.arb", "b", bin_db);
    SAVE_AND_COMPARE(gb_bin, "b2a.arb", "a", asc_db);
    SAVE_AND_COMPARE(gb_bin, "b2b.arb", "b", bin_db);

    // test extra database stream compression
    const char *compFlag = SUPPORTED_COMPRESSION_FLAGS;

    int successful_compressed_saves = 0;
    for (int c = 0; compFlag[c]; ++c) {
        for (char dbtype = 'a'; dbtype<='b'; ++dbtype) {
            TEST_ANNOTATE(GBS_global_string("dbtype=%c compFlag=%c", dbtype, compFlag[c]));
            char *zipd_db = GBS_global_string_copy("a2%c_%c.arb", dbtype, compFlag[c]);

            char savetype[] = "??";
            savetype[0]     = dbtype;
            savetype[1]     = compFlag[c];

            GB_ERROR error = GB_save_as(gb_asc, zipd_db, savetype);
            if (error && strstr(error, "failed with exitcode=127")) {
                fprintf(stderr, "Assuming compression utility for flag '%c' is not installed\n", compFlag[c]);
            }
            else {
                TEST_EXPECT_NO_ERROR(error);

                // reopen saved database, save again + compare
                {
                    GBDATA *gb_reloaded = GB_open(zipd_db, "rw");
                    TEST_REJECT_NULL(gb_reloaded); // reading compressed database failed

                    SAVE_AND_COMPARE(gb_reloaded, "r2b.arb", "b", bin_db); // check binary content
                    SAVE_AND_COMPARE(gb_reloaded, "r2a.arb", "a", asc_db); // check ascii content

                    GB_close(gb_reloaded);
                }
                successful_compressed_saves++;
            }

            free(zipd_db);
        }
    }

    TEST_EXPECT(successful_compressed_saves>=2); // at least gzip and bzip2 should be installed

#if (MEMORY_TEST == 0)
    {
        GBDATA *gb_nomap;
        TEST_EXPECT_RESULT__NOERROREXPORTED(gb_nomap = GB_open(bin_db, "rw"));
        TEST_EXPECT_NO_ERROR(GB_save_as(gb_nomap, "fast.arb", "bm"));
        TEST_EXPECT(GB_is_regularfile("fast.ARM")); // assert map file has been saved
        TEST_EXPECT_EQUAL(GB_time_of_file("fast.ARM"), GB_time_of_file("fast.arb"));
        GB_close(gb_nomap);
    }
    {
        // open DB with mapfile
        GBDATA *gb_map;
        TEST_EXPECT_RESULT__NOERROREXPORTED(gb_map = GB_open("fast.arb", "rw"));
        // SAVE_AND_COMPARE(gb_map, "fast2b.arb", "b", bin_db); // fails now (because 3 keys have different key-ref-counts)
        // Surprise: these three keys are 'tmp', 'message' and 'pending'
        // (key-ref-counts include temporary entries, but after saving they vanish and remain wrong)
        SAVE_AND_COMPARE(gb_map, "fast2a.arb", "a", asc_db); // using ascii avoid that problem (no keys stored there)

        GB_close(gb_map);
    }
    {
        // test alloc/free (no real test, just call it for code coverage)
        char *small_block = (char*)gbm_get_mem(30, 5);
        gbm_free_mem(small_block, 30, 5);

        char *big_block = (char*)gbm_get_mem(3000, 6);
        gbm_free_mem(big_block, 3000, 6);
    }
#endif

    {
        // test opening saved DBs
        GBDATA *gb_a2b = GB_open("a2b.arb", "rw"); TEST_REJECT_NULL(gb_a2b);
        GBDATA *gb_b2b = GB_open("b2b.arb", "rw"); TEST_REJECT_NULL(gb_b2b);

        // modify ..
        TEST_EXPECT_NO_ERROR(modify_db(gb_a2b));
        TEST_EXPECT_NO_ERROR(modify_db(gb_b2b));

        // .. and quicksave
        TEST_EXPECT_NO_ERROR(GB_save_quick(gb_a2b, "a2b.arb"));
        TEST_EXPECT_NO_ERROR(GB_save_quick(gb_b2b, "b2b.arb"));

#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE("a2b.a00", "TEST_loadsave_quick.a00");
#endif // TEST_AUTO_UPDATE

        TEST_EXPECT_FILES_EQUAL("TEST_loadsave_quick.a00", "a2b.a00");
        TEST_EXPECT_FILES_EQUAL("a2b.a00", "b2b.a00");

        TEST_EXPECT_NO_ERROR(GB_save_quick_as(gb_a2b, "a2b.arb"));

        // check wether quicksave can be disabled
        GB_disable_quicksave(gb_a2b, "test it");

        TEST_EXPECT_ERROR_CONTAINS(GB_save_quick(gb_a2b, "a2b.arb"), "Save Changes Disabled");
        TEST_EXPECT_ERROR_CONTAINS(GB_save_quick_as(gb_a2b, "a2b.arb"), "Save Changes Disabled");

        const char *mod_db = "a2b_modified.arb";
        TEST_EXPECT_NO_ERROR(GB_save_as(gb_a2b, mod_db, "a")); // save modified DB (now ascii to avoid key-ref-problem)
        // test loading quicksave
        {
            GBDATA *gb_quickload = GB_open("a2b.arb", "rw"); // load DB which has a quicksave
            SAVE_AND_COMPARE(gb_quickload, "a2b_quickloaded.arb", "a", mod_db); // use ascii version (binary has key-ref-diffs)
            GB_close(gb_quickload);
        }

        {
            // check master/slave DBs
            TEST_EXPECT_NO_ERROR(GB_save_as(gb_b2b, "master.arb", "b"));

            GBDATA *gb_master = GB_open("master.arb", "rw"); TEST_REJECT_NULL(gb_master);
            TEST_EXPECT_NO_ERROR(modify_db(gb_master));

            TEST_EXPECT_NO_ERROR(GB_save_quick(gb_master, "master.arb"));
            TEST_EXPECT_NO_ERROR(GB_save_quick_as(gb_master, "master.arb"));

            TEST_EXPECT_ERROR_CONTAINS(GB_save_quick(gb_master, "renamed.arb"), "master file rename"); // quicksave with wrong name

            // check if master gets protected by creating slave-DB
            TEST_EXPECT_NO_ERROR(GB_save_as(gb_master, "master.arb", "b")); // overwrite
            TEST_EXPECT_NO_ERROR(GB_save_quick_as(gb_master, "slave.arb")); // create slave -> master now protected
            TEST_EXPECT_ERROR_CONTAINS(GB_save_as(gb_master, "master.arb", "b"), "already exists and is write protected"); // overwrite should fail now

            {
                GBDATA *gb_slave = GB_open("slave.arb", "rw"); TEST_REJECT_NULL(gb_slave); // load slave DB
                TEST_EXPECT_NO_ERROR(modify_db(gb_slave));
                TEST_EXPECT_NO_ERROR(GB_save_quick(gb_slave, "slave.arb"));
                TEST_EXPECT_NO_ERROR(GB_save_quick_as(gb_slave, "renamed.arb"));
                GB_close(gb_slave);
            }
            GB_close(gb_master);
        }

        // test various error conditions:

        TEST_EXPECT_ERROR_CONTAINS(GB_save_as(gb_b2b, "",        "b"),   "specify a savename"); // empty name
        TEST_EXPECT_ERROR_CONTAINS(GB_save_as(gb_b2b, "b2b.arb", "bzB"), "Multiple compression modes");

        TEST_EXPECT_NO_ERROR(GB_set_mode_of_file(mod_db, 0444)); // write-protect
        TEST_EXPECT_ERROR_CONTAINS(GB_save_as(gb_b2b, mod_db, "b"), "already exists and is write protected"); // try to overwrite write-protected DB

        TEST_EXPECT_ERROR_CONTAINS(GB_save_quick_as(gb_b2b, NULL), "specify a file name"); // no name
        TEST_EXPECT_ERROR_CONTAINS(GB_save_quick_as(gb_b2b, ""), "specify a file name"); // empty name

        GB_close(gb_b2b);
        GB_close(gb_a2b);
    }

    GB_close(gb_asc);
    GB_close(gb_bin);

    TEST_loadsave_CLEANUP();
}

#define TEST_quicksave_CLEANUP() TEST_EXPECT_ZERO(system("rm -f min_bin.a[0-9]* min_bin.ARF"))

inline bool quicksave_exists(int i) {
    const char *qsformat = "min_bin.a%02i";
    return GB_is_regularfile(GBS_global_string(qsformat, (i)));
}
inline bool quicksave_missng(int i)          { return !quicksave_exists(i); }
inline bool is_first_quicksave(int i)        { return quicksave_exists(i) && !quicksave_exists(i-1); }
inline bool is_last_quicksave(int i)         { return quicksave_exists(i) && !quicksave_exists(i+1); }
inline bool quicksaves_range(int from, int to) { return is_first_quicksave(from) && is_last_quicksave(to); }

#define TEST_QUICK_RANGE(s,e) TEST_EXPECT(quicksaves_range(s,e))
#define TEST_QUICK_GONE(i)    TEST_EXPECT(quicksave_missng(i))

void TEST_SLOW_quicksave_names() {
    // check quicksave delete and wrap around
    TEST_quicksave_CLEANUP();
    const char *bname = "min_bin.arb";

    GB_shell shell;
    
#if 0
    {
        // update min_bin.arb from min_ascii.arb
        const char *aname    = "min_ascii.arb";
        GBDATA     *gb_ascii = GB_open(aname, "rw"); TEST_REJECT_NULL(gb_ascii);

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_ascii, bname, "b"));
        GB_close(gb_ascii);
    }
#endif
    GBDATA *gb_bin = GB_open(bname, "rw"); TEST_REJECT_NULL(gb_bin);
    for (int i = 0; i <= 100; ++i) {
        TEST_EXPECT_NO_ERROR(GB_save_quick(gb_bin, bname));
        switch (i) {
            case  0: TEST_QUICK_RANGE( 0,  0); break;
            case  1: TEST_QUICK_RANGE( 0,  1); break;
            case 10: TEST_QUICK_RANGE( 1, 10); break;
            case 98: TEST_QUICK_RANGE(89, 98); break;
            case 99: TEST_QUICK_RANGE(90, 99);
                TEST_QUICK_GONE(0);    // should not exist yet
                break;
            case 100: TEST_QUICK_RANGE(0, 9);
                TEST_QUICK_GONE((i-8));
                TEST_QUICK_GONE((i-1));
                TEST_QUICK_GONE(i);
                break;
        }
        if (i == 10) {
            // speed-up-hack
            GB_MAIN_TYPE *Main   = GB_MAIN(gb_bin);
            i                   += 78; // -> 88 (afterwards run 10 times w/o checks to fake correct state)
            Main->qs.last_index += 78;
        }
    }

    GB_close(gb_bin);
    
    TEST_quicksave_CLEANUP();
}

void TEST_db_filenames() {
    TEST_EXPECT_EQUAL(gb_quicksaveName("nosuch.arb", 0), "nosuch.a00");
    TEST_EXPECT_EQUAL(gb_quicksaveName("nosuch", 1), "nosuch.a01");
}

void TEST_SLOW_corruptedEntries_saveProtection() {
    // see #499 and #501
    GB_shell shell;

    const char *name_NORMAL[] = {
        "corrupted.arb",
        "corrupted2.arb",
    };
    const char *name_CORRUPTED[] = {
        "corrupted_CORRUPTED.arb",
        "corrupted2_CORRUPTED.arb",
    };

    const char *quickname           = "corrupted.a00";
    const char *quickname_CORRUPTED = "corrupted_CORRUPTED.a00";
    const char *quickname_unwanted  = "corrupted_CORRUPTED.a01";

    const char **name = name_NORMAL;

    const char *INITIAL_VALUE = "initial value";
    const char *CHANGED_VALUE = "changed";

    GB_unlink("*~");
    GB_unlink(quickname_unwanted);

    for (int corruption = 0; corruption<=3; ++corruption) {
        TEST_ANNOTATE(GBS_global_string("corruption level %i", corruption));

        GB_unlink(name[0]);

        // create simple DB
        {
            GBDATA *gb_main = GB_open(name[0], "cwr");
            TEST_REJECT_NULL(gb_main);

            {
                GB_transaction ta(gb_main);

                GBDATA *gb_entry = GB_create(gb_main, "sth", GB_STRING);
                TEST_REJECT_NULL(gb_entry);
                TEST_EXPECT_NO_ERROR(GB_write_string(gb_entry, INITIAL_VALUE));

                GBDATA *gb_other = GB_create(gb_main, "other", GB_INT);
                TEST_REJECT_NULL(gb_other);
                TEST_EXPECT_NO_ERROR(GB_write_int(gb_other, 4711));
            }

            TEST_EXPECT_NO_ERROR(GB_save(gb_main, NULL, "b"));
            GB_close(gb_main);
        }

        // reopen DB, change the entry, quick save + full save with different name
        {
            GBDATA *gb_main = GB_open(name[0], "wr");
            TEST_REJECT_NULL(gb_main);

            {
                GB_transaction ta(gb_main);

                GBDATA *gb_entry = GB_entry(gb_main, "sth");
                TEST_REJECT_NULL(gb_entry);

                const char *content = GB_read_char_pntr(gb_entry);
                TEST_EXPECT_EQUAL(content, INITIAL_VALUE);

                TEST_EXPECT_NO_ERROR(GB_write_string(gb_entry, CHANGED_VALUE));

                content = GB_read_char_pntr(gb_entry);
                TEST_EXPECT_EQUAL(content, CHANGED_VALUE);

                // now corrupt the DB entry:
                if (corruption>0) {
                    char *illegal_access = (char*)content;
                    illegal_access[2]    = 0;

                    if (corruption>1) {
                        gb_entry = GB_create(gb_main, "sth", GB_STRING);
                        TEST_REJECT_NULL(gb_entry);
                        TEST_EXPECT_NO_ERROR(GB_write_string(gb_entry, INITIAL_VALUE));

                        if (corruption>2) {
                            // fill rest of string with zero bytes (similar to copying a truncated string into calloced memory)
                            int len = strlen(CHANGED_VALUE);
                            for (int i = 3; i<len; ++i) {
                                illegal_access[i] = 0;
                            }
                        }
                    }

// #define PERFORM_DELETE
#ifdef PERFORM_DELETE
                    // delete "other"
                    GBDATA *gb_other = GB_entry(gb_main, "other");
                    TEST_REJECT_NULL(gb_other);
                    TEST_EXPECT_NO_ERROR(GB_delete(gb_other));
#endif
                }
            }

            GB_ERROR quick_error = GB_save_quick(gb_main, name[0]);
            TEST_REJECT(seen_corrupt_data);
            if (corruption) {
                TEST_EXPECT_CONTAINS(quick_error, "Corrupted data detected during save");
                quick_error = GB_save_quick_as(gb_main, name_CORRUPTED[0]); // save with special name (as user should do)
                TEST_REJECT(seen_corrupt_data);
            }
            TEST_REJECT(quick_error);

            GB_ERROR full_error = GB_save(gb_main, name[1], "b");
            TEST_REJECT(seen_corrupt_data);
            if (corruption) {
                TEST_EXPECT_CONTAINS(full_error, "Corrupted data detected during save");
                full_error  = GB_save(gb_main, name_CORRUPTED[1], "b"); // save with special name (as user should do)
                TEST_REJECT(seen_corrupt_data);
                name = name_CORRUPTED; // from now on use these names (for load and save)
            }
            TEST_REJECT(full_error);

            GB_close(gb_main);
        }

        for (int full = 0; full<2; ++full) {
            TEST_ANNOTATE(GBS_global_string("corruption level %i / full=%i", corruption, full));

            // reopen DB (full==0 -> load quick save; ==1 -> load full save)
            GBDATA *gb_main = GB_open(name[full], "r");
            TEST_REJECT_NULL(gb_main);

            if (gb_main) {
                {
                    GB_transaction ta(gb_main);

                    GBDATA *gb_entry = GB_entry(gb_main, "sth");
                    TEST_REJECT_NULL(gb_entry);

                    const char *content = GB_read_char_pntr(gb_entry);

                    switch (corruption) {
                        case 0:
                            TEST_EXPECT_EQUAL(content, CHANGED_VALUE);
                            break;
                        default:
                            TEST_EXPECT_EQUAL(content, "ch");
                            break;
                    }

                    // check 2nd entry
                    gb_entry = GB_nextEntry(gb_entry);
                    if (corruption>1) {
                        TEST_REJECT_NULL(gb_entry);

                        content = GB_read_char_pntr(gb_entry);
                        TEST_EXPECT_EQUAL(content, INITIAL_VALUE);
                    }
                    else {
                        TEST_REJECT(gb_entry);
                    }

                    // check int entry
                    GBDATA *gb_other = GB_entry(gb_main, "other");
#if defined(PERFORM_DELETE)
                    bool    deleted  = corruption>0;
#else // !defined(PERFORM_DELETE)
                    bool    deleted  = false;
#endif

                    if (deleted) {
                        TEST_REJECT(gb_other);
                    }
                    else {
                        TEST_REJECT_NULL(gb_other);
                        TEST_EXPECT_EQUAL(GB_read_int(gb_other), 4711);
                    }
                }

                GB_close(gb_main);
            }
            GB_unlink(name_NORMAL[full]);
            GB_unlink(name_CORRUPTED[full]);
        }
        GB_unlink(quickname);
        GB_unlink(quickname_CORRUPTED);

        TEST_REJECT(GB_is_regularfile(quickname_unwanted));

        name = name_NORMAL; // restart with normal names
    }
}

// @@@ move code into other module (to avoid extra include and to test w/o knowing ArbDBWriter)
#include "arbdbt.h"

void TEST_streamed_ascii_save_asUsedBy_silva_pipeline() {
    GB_shell shell;

    const char *loadname = "TEST_loadsave_ascii.arb";
    const char *savename = "TEST_streamsaved.arb";

    {
        GBDATA *gb_main1 = GB_open(loadname, "r");  TEST_REJECT_NULL(gb_main1);
        GBDATA *gb_main2 = GB_open(loadname, "rw"); TEST_REJECT_NULL(gb_main2);

        // delete all species from DB2
        GBDATA *gb_species_data2 = NULL;
        {
            GB_transaction ta(gb_main2);
            GB_push_my_security(gb_main2);
            gb_species_data2 = GBT_get_species_data(gb_main2);
            for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data2);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                TEST_EXPECT_NO_ERROR(GB_delete(gb_species));
            }
            GB_pop_my_security(gb_main2);
        }

        for (int saveWhileTransactionOpen = 0; saveWhileTransactionOpen<=1; ++saveWhileTransactionOpen) {
            {
                ArbDBWriter writer(GB_MAIN(gb_main2));
                TEST_EXPECT_NO_ERROR(writer.startSaveAs(savename, "a"));
                TEST_EXPECT_NO_ERROR(writer.saveFromTill(gb_main2, gb_species_data2));

                {
                    GB_transaction ta1(gb_main1);
                    GB_transaction ta2(gb_main2);

                    for (GBDATA *gb_species1 = GBT_first_species(gb_main1);
                         gb_species1;
                         gb_species1 = GBT_next_species(gb_species1))
                    {
                        GBDATA *gb_species2 = GB_create_container(gb_species_data2, "species");
                        if (!gb_species2) {
                            TEST_EXPECT_NO_ERROR(GB_await_error());
                        }
                        else {
                            TEST_EXPECT_NO_ERROR(GB_copy_with_protection(gb_species2, gb_species1, true));
                            GB_write_flag(gb_species2, GB_read_flag(gb_species1)); // copy marked flag
                        }

                        if (!saveWhileTransactionOpen) GB_commit_transaction(gb_main2);
                        TEST_EXPECT_NO_ERROR(writer.saveFromTill(gb_species2, gb_species2));
                        if (!saveWhileTransactionOpen) GB_begin_transaction(gb_main2);

                        GB_push_my_security(gb_main2);
                        TEST_EXPECT_NO_ERROR(GB_delete(gb_species2));
                        GB_pop_my_security(gb_main2);
                    }
                }

                TEST_EXPECT_NO_ERROR(writer.saveFromTill(gb_species_data2, gb_main2));
                TEST_EXPECT_NO_ERROR(writer.finishSave());
            }

            // test file content
            TEST_EXPECT_TEXTFILES_EQUAL(savename, loadname);
            TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(savename));
        }

        GB_close(gb_main2);
        GB_close(gb_main1);
    }

    // @@@ remove result file(s)
}

#endif // UNIT_TESTS
