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

#include "gb_map.h"
#include "gb_load.h"
#include "gb_storage.h"

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

typedef SmartMallocPtr(char) SmartCharPtr;
 
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

GB_CSTR gb_overwriteName(GB_CSTR path) {
    static SmartCharPtr Oname;

    int   len   = strlen(path);
    char *oname = STATIC_BUFFER(Oname, len+2);

    strcpy(oname, path);
    strcpy(oname+len, "~");                         // append ~

    return oname;
}

GB_CSTR gb_reffile_name(GB_CSTR path) {
    static SmartCharPtr Refname;

    size_t  len     = strlen(path);
    char   *refname = STATIC_BUFFER(Refname, len+4+1);
    memcpy(refname, path, len+1);

    const char *ext        = gb_findExtension(refname);
    size_t      ext_offset = ext ? (size_t)(ext-refname) : len;

    strcpy(refname+ext_offset, ".ARF");

    return refname;
}

GB_ERROR gb_delete_reference(const char *master) {
    GB_ERROR    error      = 0;
    char       *fullmaster = gb_full_path(master);
    const char *fullref    = gb_reffile_name(fullmaster);

    GB_unlink_or_warn(fullref, &error);

    free(fullmaster);
    return error;
}

GB_ERROR gb_create_reference(const char *master) {
    char       *fullmaster = gb_full_path(master);
    const char *fullref    = gb_reffile_name(fullmaster);
    GB_ERROR    error      = 0;
    FILE       *out        = fopen(fullref, "w");

    if (out) {
        fprintf(out, "***** The following files may be a link to %s ********\n", fullmaster);
        fclose(out);
        error = GB_set_mode_of_file(fullref, 00666);
    }
    else {
        error = GBS_global_string("Cannot create reference file '%s'\n"
                                  "Your database was saved, but you should check "
                                  "write permissions in the destination directory!", fullref);
    }

    free(fullmaster);
    return error;
}

GB_ERROR gb_add_reference(const char *master, const char *changes) {
    char       *fullmaster  = gb_full_path(master);
    char       *fullchanges = gb_full_path(changes);
    const char *fullref     = gb_reffile_name(fullmaster);
    GB_ERROR    error       = 0;
    FILE       *out         = fopen(fullref, "a");

    if (out) {
        fprintf(out, "%s\n", fullchanges);
        fclose(out);
        error = GB_set_mode_of_file(fullref, 00666);
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

    gb_assert(!GB_have_error()); // dont export
    return error;
}

GB_ERROR gb_remove_all_but_main(GB_MAIN_TYPE *Main, const char *path) {
    GB_ERROR error = gb_remove_quick_saved(Main, path);
    if (!error) GB_unlink_or_warn(gb_mapfile_name(path), &error); // delete old mapfile

    gb_assert(!GB_have_error());                    // dont export
    return error;
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

long gb_ascii_2_bin(const char *source, GBDATA *gbd) {
    const char *s = source;

    long len = 0;
    char c   = *(s++);

    long  size;
    long  memsize;
    char *d;
    long  k;
    long  i;

    A_TO_I(c);
    gbd->flags.compressed_data = c;

    if (*s == ':') {
        size = 0;
        s++;
    }
    else {
        for (i=0, k = 8; k && (c = *(s++)); k--) {
            A_TO_I(c);
            i = (i<<4)+c;
        }
        size = i;
    }
    source = s;

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

    memsize = len;

    GB_SETSMDMALLOC_UNINITIALIZED(gbd, size, memsize);
    d = GB_GETDATA(gbd);
    s = source;
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
        i = c << 4;
        c = *(s++);
        A_TO_I(c);
        *(d++) = (char)(i + c);
    }
    return 0;
}
// ------------------------
//      Binary to Ascii

#define GB_PUT(c, out) do { if (c>=10) c+='A'-10; else c += '0'; *(out++) = (char)c; } while (0)

GB_BUFFER gb_bin_2_ascii(GBDATA *gbd)
{
    signed char   *s, *out, c, mo;
    unsigned long  i;
    int            j;
    char          *buffer;
    int            k;

    char *source = GB_GETDATA(gbd);
    long len = GB_GETMEMSIZE(gbd);
    long xtended = GB_GETSIZE(gbd);
    int compressed = gbd->flags.compressed_data;

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

static void gb_write_rek(FILE *out, GBCONTAINER *gbc, long deep, long big_hunk) {
    // used by ASCII database save
    long     i;
    char    *s;
    char     c;
    GBDATA  *gb;
    GB_CSTR  strng;
    char    *key;

    for (gb = GB_child((GBDATA *)gbc); gb; gb = GB_nextChild(gb)) {
        if (gb->flags.temporary) continue;
        key = GB_KEY(gb);
        if (!strcmp(key, GB_SYSTEM_FOLDER)) continue;   // do not save system folder
        for (i=deep; i--;) putc('\t', out);
        fprintf(out, "%s\t", key);
        if ((int)strlen(key) < 8) {
            putc('\t', out);
        }
        if (gb->flags.security_delete ||
                gb->flags.security_write ||
                gb->flags.security_read ||
                gb->flags2.last_updated) {
            putc(':', out);
            c = gb->flags.security_delete;
            GB_PUT_OUT(c, out);
            c = gb->flags.security_write;
            GB_PUT_OUT(c, out);
            c = gb->flags.security_read;
            GB_PUT_OUT(c, out);
            fprintf(out, "%i\t", gb->flags2.last_updated);
        }
        else {
            putc('\t', out);
        }
        switch (GB_TYPE(gb)) {
            case    GB_STRING:
                strng = GB_read_char_pntr(gb);
                if (!strng) {
                    strng = "<entry was broken - replaced during ASCIIsave/arb_repair>";
                    GB_warningf("- replaced broken DB entry %s (data lost)\n", GB_get_db_path(gb));
                }
                if (*strng == '%') {
                    putc('%', out);
                    putc('s', out);
                    putc('\t', out);
                }
                GBS_fwrite_string(strng, out);
                putc('\n', out);
                break;
            case    GB_LINK:
                strng = GB_read_link_pntr(gb);
                if (*strng == '%') {
                    putc('%', out);
                    putc('l', out);
                    putc('\t', out);
                }
                GBS_fwrite_string(strng, out);
                putc('\n', out);
                break;
            case GB_INT:
                fprintf(out, "%%i %li\n", GB_read_int(gb));
                break;
            case GB_FLOAT:
                fprintf(out, "%%f %g\n", GB_read_float(gb));
                break;
            case GB_BITS:
                fprintf(out, "%%I\t\"%s\"\n",
                        GB_read_bits_pntr(gb, '-', '+'));
                break;
            case GB_BYTES:
                s = gb_bin_2_ascii(gb);
                fprintf(out, "%%Y\t%s\n", s);
                break;
            case GB_INTS:
                s = gb_bin_2_ascii(gb);
                fprintf(out, "%%N\t%s\n", s);
                break;
            case GB_FLOATS:
                s = gb_bin_2_ascii(gb);
                fprintf(out, "%%F\t%s\n", s);
                break;
            case GB_DB:
                fprintf(out, "%%%% (%%\n");
                gb_write_rek(out, (GBCONTAINER *)gb, deep + 1, big_hunk);
                for (i=deep+1; i--;) putc('\t', out);
                fprintf(out, "%%) /*%s*/\n\n", GB_KEY(gb));
                break;
            case GB_BYTE:
                fprintf(out, "%%y %i\n", GB_read_byte(gb));
                break;
            default:
                fprintf(stderr,
                        "ARBDB ERROR Key \'%s\' is of unknown type\n",
                        GB_KEY(gb));
                fprintf(out, "%%%% (%% %%) /* unknown type */\n");
                break;
        }
    }
}

// -------------------------
//      Read Binary File

long gb_read_in_long(FILE *in, long reversed) {
    long  sdata = 0;
    char *p     = (char *)&sdata;

    if (!reversed) {
        *(p++) = getc(in);
        *(p++) = getc(in);
        *(p++) = getc(in);
        *(p) = getc(in);
    }
    else {
        p[3] = getc(in);
        p[2] = getc(in);
        p[1] = getc(in);
        p[0] = getc(in);
    }
    return sdata;
}


long gb_read_number(FILE *in) {
    unsigned int c0, c1, c2, c3, c4;
    c0 = getc(in);
    if (c0 & 0x80) {
        c1 = getc(in);
        if (c0 & 0x40) {
            c2 = getc(in);
            if (c0 & 0x20) {
                c3 = getc(in);
                if (c0 &0x10) {
                    c4 = getc(in);
                    return c4 | (c3<<8) | (c2<<16) | (c1<<8);
                }
                else {
                    return (c3) | (c2<<8) | (c1<<16) | ((c0 & 0x0f)<<24);
                }
            }
            else {
                return (c2) | (c1<<8) | ((c0 & 0x1f)<<16);
            }
        }
        else {
            return (c1) | ((c0 & 0x3f)<<8);
        }
    }
    else {
        return c0;
    }
}

void gb_put_number(long i, FILE *out) {
    long j;
    if (i< 0x80) { putc((int)i, out); return; }
    if (i<0x4000) {
        j = (i>>8) | 0x80;
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    if (i<0x200000) {
        j = (i>>16) | 0xC0;
        putc((int)j, out);
        j = (i>>8);
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    if (i<0x10000000) {
        j = (i>>24) | 0xE0;
        putc((int)j, out);
        j = (i>>16);
        putc((int)j, out);
        j = (i>>8);
        putc((int)j, out);
        putc((int)i, out);
        return;
    }
    gb_assert(0); // overflow
}

long gb_read_bin_error(FILE *in, GBDATA *gbd, const char *text) {
    long p = (long)ftell(in);
    GB_export_errorf("%s in reading GB_file (loc %li=%lX) reading %s\n",
                     text, p, p, GB_KEY(gbd));
    GB_print_error();
    return 0;
}

// --------------------------
//      Write Binary File

long gb_write_out_long(long data, FILE *out) {
    long sdata[1];
    char *p = (char *)sdata, c;
    sdata[0] = data;
    c = *(p++);
    putc(c, out);
    c = *(p++);
    putc(c, out);
    c = *(p++);
    putc(c, out);
    c = *(p);
    putc(c, out);
    return 0;
}


int gb_is_writeable(gb_header_list *header, GBDATA *gbd, long version, long diff_save) {
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

int gb_write_bin_sub_containers(FILE *out, GBCONTAINER *gbc, long version, long diff_save, int is_root) {
    gb_header_list *header;
    int             i, index;
    int             counter;

    header = GB_DATA_LIST_HEADER(gbc->d);
    for (i=0, index = 0; index < gbc->d.nheader; index++) {
        if (gb_is_writeable(&(header[index]), GB_HEADER_LIST_GBD(header[index]), version, diff_save)) i++;
    }

    if (!is_root) {
        gb_put_number(i, out);
    }
    else {
        gb_write_out_long(i, out);
    }

    counter = 0;
    for (index = 0; index < gbc->d.nheader; index++) {
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

long gb_write_bin_rek(FILE *out, GBDATA *gbd, long version, long diff_save, long index_of_master_file)
{
    int          i;
    GBCONTAINER *gbc     = 0;
    long         size    = 0;
    int          type    = GB_TYPE(gbd);

    if (type == GB_DB) {
        gbc = (GBCONTAINER *)gbd;
    }
    else if (type == GB_STRING || type == GB_STRING_SHRT) {
        size = GB_GETSIZE(gbd);
        if (!gbd->flags.compressed_data && size < GBTUM_SHORT_STRING_SIZE) {
            type = GB_STRING_SHRT;
        }
        else {
            type = GB_STRING;
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
        const char *data = GB_GETDATA(gbd);
        size_t      len  = strlen(data); // w/o zero-byte!

        if ((long)len == size) {
            i = fwrite(data, len+1, 1, out);

            return i <= 0 ? -1 : 0;
        }
        // string contains zero-byte inside data or misses trailing zero-byte
        type = GB_STRING; // fallback to safer type
    }

    switch (type) {
        case GB_DB:
            i = gb_write_bin_sub_containers(out, gbc, version, diff_save, 0);
            return i;

        case GB_INT: {
            GB_UINT4 buffer = (GB_UINT4)htonl(gbd->info.i);
            if (!fwrite((char *)&buffer, sizeof(float), 1, out)) return -1;
            return 0;
        }
        case GB_FLOAT:
            if (!fwrite((char *)&gbd->info.i, sizeof(float), 1, out)) return -1;
            return 0;
        case GB_LINK:
        case GB_BITS:
        case GB_BYTES:
        case GB_INTS:
        case GB_FLOATS:
            size = GB_GETSIZE(gbd);
            // fall-through
        case GB_STRING: {
            long memsize = GB_GETMEMSIZE(gbd);
            gb_put_number(size, out);
            gb_put_number(memsize, out);
            i = fwrite(GB_GETDATA(gbd), (size_t)memsize, 1, out);
            if (memsize && !i) return -1;
            return 0;
        }
        case GB_BYTE:
            putc((int)(gbd->info.i), out);
            return 0;
        default:
            gb_assert(0); // unknown type
            return -1;
    }
}


int gb_write_bin(FILE *out, GBDATA *gbd, long version) {
    /* version 1 write master arb file
     * version 2 write slave arb file
     */
    long          i;
    GBCONTAINER  *gbc       = (GBCONTAINER *)gbd;
    int           diff_save = 0;
    GB_MAIN_TYPE *Main      = GBCONTAINER_MAIN(gbc);

    gb_write_out_long(GBTUM_MAGIC_NUMBER, out);
    fprintf(out, "\n this is the binary version of the gbtum data file version %li\n", version);
    putc(0, out);
    fwrite("vers", 4, 1, out);
    gb_write_out_long(0x01020304, out);
    gb_write_out_long(version, out);
    fwrite("keys", 4, 1, out);

    for (i=1; i<Main->keycnt; i++) {
        if (Main->keys[i].nref>0) {
            gb_put_number(Main->keys[i].nref, out);
            fprintf(out, "%s", Main->keys[i].key);
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
     /* savetype 'a'    ascii
      *          'aS'   dump to stdout
      *          'b'    binary
      *          'bm'   binary + mapfile
      *          0      ascii
      */
{
    if (path && strchr(savetype, 'S') == 0) // 'S' dumps to stdout -> do not change path
    {
        freedup(GB_MAIN(gb)->path, path);
    }
    return GB_save_as(gb, path, savetype);
}

GB_ERROR GB_create_directory(const char *path) {
    GB_ERROR error = 0;
    if (!GB_is_directory(path)) {
        char *parent;
        GB_split_full_path(path, &parent, NULL, NULL, NULL);
        if (parent) {
            if (!GB_is_directory(parent)) error = GB_create_directory(parent);
            free(parent);
        }

        if (!error) {
            int res = mkdir(path, ACCESSPERMS);
            if (res) error = GB_IO_error("creating directory", path);
        }

        gb_assert(error || GB_is_directory(path));

        error = GB_failedTo_error("GB_create_directory", path, error);
    }
    return error;
}

GB_ERROR GB_save_in_home(GBDATA *gb, const char *path, const char *savetype) {
    /* savetype
     *      'a'    ascii
     *      'b'    binary
     *      'bm'   binary + mapfile
     *      0=ascii
     *
     * automatically creates subdirectories
     */
    GB_ERROR    error = 0;
    char       *buffer;
    const char *env;
    char       *slash;

    env = GB_getenvHOME();
    if (!path) path = GB_MAIN(gb)->path;

    buffer = (char *)GB_calloc(sizeof(char), strlen(env) + strlen(path) + 2);

    sprintf(buffer, "%s/%s", env, path);
    slash             = strrchr(buffer, '/');
    *slash            = 0;
    error             = GB_create_directory(buffer);
    *slash            = '/';
    if (!error) error = GB_save_as(gb, buffer, savetype);
    if (buffer) free(buffer);
    return error;
}

GB_ERROR GB_save_as(GBDATA *gb, const char *path, const char *savetype) {
    /* Save whole database
     *
     * savetype
     *          'a' ascii
     *          'b' binary
     *          'm' save mapfile (only together with binary)
     *          'f' force saving even in disabled path to a different directory (out of order save)
     *          'S' save to stdout (for debugging)
     */

    GB_ERROR error = NULL;

    gb_assert(savetype);

    if (!gb) {
        error = "got no db";
    }
    else {
        bool saveASCII;
        if (strchr(savetype, 'a')) saveASCII      = true;
        else if (strchr(savetype, 'b')) saveASCII = false;
        else error = GBS_global_string("Invalid savetype '%s' (expected 'a' or 'b')", savetype);

        if (!error) {
            GB_MAIN_TYPE *Main = GB_MAIN(gb);
            gb                 = (GBDATA*)Main->data;
            if (!path) path    = Main->path;

            if (!path || !path[0]) error = "Please specify a savename";
            else error                   = gb_check_saveable(gb, path, savetype);

            if (!error) {
                char *mappath        = NULL;
                char *sec_path       = strdup(gb_overwriteName(path));
                char *sec_mappath    = NULL;
                bool  dump_to_stdout = strchr(savetype, 'S');
                FILE *out            = dump_to_stdout ? stdout : fopen(sec_path, "w");

                if (!out) error = GB_IO_error("saving", sec_path);
                else {
                    int slevel     = Main->security_level;
                    int translevel = Main->transaction;

                    if (!translevel) Main->transaction = 1;
                    else {
                        if (translevel> 0) {
                            GB_commit_transaction(gb);
                            GB_begin_transaction(gb);
                        }
                    }
                    Main->security_level = 7;

                    bool outOfOrderSave     = strchr(savetype, 'f');
                    bool deleteQuickAllowed = !outOfOrderSave;
                    {
                        int result = 0;

                        if (saveASCII) {
                            fprintf(out, "/*ARBDB ASCII*/\n");
                            gb_write_rek(out, (GBCONTAINER *)gb, 0, 1);
                            freedup(Main->qs.quick_save_disabled, "Database saved in ASCII mode");
                            if (deleteQuickAllowed) error = gb_remove_all_but_main(Main, path);
                        }
                        else {                      // save binary
                            mappath = strdup(gb_mapfile_name(path));
                            if (strchr(savetype, 'm')) {
                                // it's necessary to save the mapfile FIRST,
                                // cause this re-orders all GB_CONTAINERs containing NULL-entries in their header
                                sec_mappath = strdup(gb_overwriteName(mappath));
                                error       = gb_save_mapfile(Main, sec_mappath);
                            }
                            else GB_unlink_or_warn(mappath, &error); // delete old mapfile
                            if (!error) result |= gb_write_bin(out, gb, 1);
                        }

                        Main->security_level = slevel;
                        Main->transaction = translevel;

                        if (!dump_to_stdout) result |= fclose(out);
                        if (result != 0) error = GB_IO_error("writing", sec_path);
                    }

                    if (!error && !saveASCII) {
                        if (!outOfOrderSave) freenull(Main->qs.quick_save_disabled); // delete reason, why quicksaving was disallowed
                        if (deleteQuickAllowed) error = gb_remove_quick_saved(Main, path);
                    }

                    if (!dump_to_stdout) {
                        if (error) {
                            if (sec_mappath) GB_unlink_or_warn(sec_mappath, NULL);
                            GB_unlink_or_warn(sec_path, NULL);
                        }
                        else {
                            bool unlinkMapfiles       = false;
                            error                     = GB_rename_file(sec_path, path);
                            if (error) unlinkMapfiles = true;
                            else if (sec_mappath) {
                                error             = GB_rename_file(sec_mappath, mappath);
                                if (!error) error = GB_set_mode_of_file(mappath, GB_mode_of_file(path)); // set mapfile to same mode as DB file
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
                                    ? gb_create_reference(path)
                                    : gb_delete_reference(path);
                            }
                        }
                    }

                    if (!error && !outOfOrderSave) {
                        Main->last_saved_transaction      = GB_read_clock(gb);
                        Main->last_main_saved_transaction = GB_read_clock(gb);
                        Main->last_saved_time             = GB_time_of_day();
                    }
                }

                free(sec_path);
                free(mappath);
                free(sec_mappath);
            }
        }
    }

    gb_assert(!GB_have_error());                    // dont export
    return error;
}

static GB_ERROR gb_check_quick_save(GBDATA *gb_main) {
    /* is quick save allowed?
     * return NULL if allowed, error why disallowed otherwise.
     */

    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    if (Main->qs.quick_save_disabled) {
        return GBS_global_string("Save Changes Disabled, because\n"
                                 "    '%s'\n"
                                 "    Save whole database using binary mode first",
                                 Main->qs.quick_save_disabled);
    }

    return NULL;
}

GB_ERROR GB_delete_database(GB_CSTR filename) {
    GB_ERROR error = 0;

    if (GB_unlink(filename)<0) error = GB_await_error();
    else error                       = gb_remove_all_but_main(0, filename);

    return error;
}

GB_ERROR GB_save_quick_as(GBDATA *gb_main, const char *path) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    gb_main            = (GBDATA *)Main->data;

    GB_ERROR error = NULL;

    if (!path || !strlen(path)) {
        error = "Please specify a file name";
    }
    else if (strcmp(path, Main->path) == 0) {    // same name (no rename)
        error = GB_save_quick(gb_main, path);
    }
    else {
        error             = gb_check_quick_save(gb_main);
        if (!error) error = gb_check_saveable(gb_main, path, "bn");

        if (!error) {
            FILE *fmaster = fopen(Main->path, "r");         // old master !!!!
            if (!fmaster) {                                 // Oh no, where is my old master
                error = GBS_global_string("Save Changes is missing master ARB file '%s',\n"
                                          "    save database first", Main->path);
            }
            fclose(fmaster);
        }
        if (!error) {
            if (GB_unlink(path)<0) { // delete old file
                error = GBS_global_string("File '%s' already exists and could not be deleted\n"
                                          "(Reason: %s)",
                                          path, GB_await_error());
            }
        }
        if (!error) {
            char *org_master = S_ISLNK(GB_mode_of_link(Main->path))
                ? GB_follow_unix_link(Main->path)
                : strdup(Main->path);

            error = gb_remove_all_but_main(Main, path);
            if (!error) {
                long mode = GB_mode_of_file(org_master);
                if (mode & S_IWUSR) {
                    GB_ERROR sm_error = GB_set_mode_of_file(org_master, mode & ~(S_IWUSR | S_IWGRP | S_IWOTH));
                    if (sm_error) {
                        GB_warningf("%s\n"
                                    "Ask your admin to remove write permissions from that master file.\n"
                                    "NEVER delete or change it, otherwise your quicksaves will be rendered useless!", 
                                    sm_error);
                    }
                }
                char *full_path_of_source;
                if (strchr(path, '/') || strchr(org_master, '/')) {
                    // dest or source in different directory
                    full_path_of_source = gb_full_path(org_master);
                }
                else {
                    full_path_of_source = strdup(org_master);
                }

                error = GB_symlink(full_path_of_source, path);
                if (!error) {
                    if ((uid_t)GB_getuid_of_file(full_path_of_source) != getuid()) {
                        GB_warningf("**** WARNING ******\n"
                                    "   You now using a file '%s'\n"
                                    "   which is owned by another user\n"
                                    "   Please ask him not to delete this file\n"
                                    "   If you don't trust him, don't save changes but\n"
                                    "   the WHOLE database", full_path_of_source);
                    }

                    GB_ERROR warning = gb_add_reference(full_path_of_source, path);
                    if (warning) GB_warning(warning);

                    freedup(Main->path, path);                      // Symlink created -> rename allowed

                    Main->qs.last_index = 0;            // Start with new quicks
                    error = GB_save_quick(gb_main, path);
                }
                free(full_path_of_source);
            }
            free(org_master);
        }
    }

    gb_assert(!GB_have_error());                    // dont export
    return error;
}

GB_ERROR GB_save_quick(GBDATA *gb, const char *refpath) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb);
    gb                 = (GBDATA *)Main->data;

    GB_ERROR error    = gb_check_quick_save(gb);
    if (!error) error = gb_check_saveable(gb, refpath, "q");

    if (!error && refpath && strcmp(refpath, Main->path) != 0) {
        error = GBS_global_string("master file rename '%s'!= '%s',\n"
                                  "save database first", refpath, Main->path);
    }
    if (!error) {
        FILE *fmaster = fopen(Main->path, "r");

        if (!fmaster) {
            error = GBS_global_string("Quick save is missing master ARB file '%s',\n"
                                      "save database first", refpath);
        }
        fclose(fmaster);
    }
    if (!error && !Main->local_mode) error = "You cannot save a remote database";
    if (!error) {
        Main->qs.last_index++;
        if (Main->qs.last_index > GB_MAX_QUICK_SAVE_INDEX) renameQuicksaves(Main);

        GB_CSTR path     = gb_quicksaveName(Main->path, Main->qs.last_index);
        GB_CSTR sec_path = gb_overwriteName(path);

        FILE *out = fopen(sec_path, "w");
        if (!out) error = GBS_global_string("Cannot save file to '%s'", sec_path);
        else {
            long erg;
            {
                int slevel     = Main->security_level;
                int translevel = Main->transaction;

                if (!translevel) Main->transaction = 1;
                else {
                    if (translevel> 0) {
                        GB_commit_transaction(gb);
                        GB_begin_transaction(gb);
                    }
                }

                Main->security_level = 7;

                erg = gb_write_bin(out, gb, 2);

                Main->security_level = slevel;
                Main->transaction    = translevel;
            }

            erg |= fclose(out);

            if (erg!=0) error = GBS_global_string("Cannot write to '%s'", sec_path);
            else {
                error = GB_rename_file(sec_path, path);
                if (!error) {
                    Main->last_saved_transaction = GB_read_clock(gb);
                    Main->last_saved_time        = GB_time_of_day();

                    error = deleteSuperfluousQuicksaves(Main);
                }
            }
        }
    }

    gb_assert(!GB_have_error());                    // dont export
    return error;
}

char *gb_full_path(const char *path) {
    char *res = 0;

    if (path[0] == '/') res = strdup(path);
    else {
        const char *cwd = GB_getcwd();

        if (path[0] == 0) res = strdup(cwd);
        else res              = GBS_global_string_copy("%s/%s", cwd, path);
    }
    return res;
}


GB_ERROR gb_check_saveable(GBDATA *gbd, const char *path, const char *flags) {
    /* Check wether file can be stored at destination
     *  'f' in flags means 'force' => ignores main->disabled_path
     *  'q' in flags means 'quick save'
     *  'n' in flags means destination must be empty
     */

    GB_MAIN_TYPE *Main  = GB_MAIN(gbd);
    GB_ERROR      error = NULL;

    if (!Main->local_mode) {
        error = "You cannot save a remote database, please use save button in master program";
    }
    else if (Main->opentype == gb_open_read_only_all) {
        error = "Database is read only";
    }
    else if (strchr(path, ':')) {
        error = "Your database name may not contain a ':' character\nChoose a different name";
    }
    else {
        char *fullpath = gb_full_path(path);
        if (Main->disabled_path && !strchr(flags, 'f')) {
            if (GBS_string_matches(fullpath, Main->disabled_path, GB_MIND_CASE)) {
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
        long mode = GB_mode_of_link(path);
        if (mode >= 0 && !(mode & S_IWUSR)) { // no write access -> looks like a master file
            error = GBS_global_string("Your selected file '%s' already exists and is write protected\n"
                                      "It looks like that your file is a master arb file which is\n"
                                      "used by some quicksaved databases\n"
                                      "If you want to save it nevertheless, delete it first, but\n"
                                      "note that doing this will render all these quicksaves useless!",
                                      path);
        }
    }
    
    if (!error && strchr(flags, 'n') && GB_time_of_file(path)) {
        error = GBS_global_string("Your destination file '%s' already exists.\n"
                                  "Delete it manually!", path);
    }
    
    return error;
}

void GB_disable_path(GBDATA *gbd, const char *path) {
    // disable directories for save
    freeset(GB_MAIN(gbd)->disabled_path, path ? GBS_eval_env(path) : NULL);
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)

#include <test_unit.h>

#define SAVE_AND_COMPARE(gbd, save_as, savetype, compare_with) \
    TEST_ASSERT_NO_ERROR(GB_save_as(gbd, save_as, savetype));  \
    TEST_ASSERT_FILES_EQUAL(save_as, compare_with)

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
#define TEST_copy(src, dst) TEST_ASSERT(system(GBS_global_string("cp '%s' '%s'", src, dst)) == 0)

#define TEST_loadsave_CLEANUP() TEST_ASSERT(system("rm -f [ab]2[ab]*.* master.* slave.* renamed.* fast.* fast2b.* TEST_loadsave.ARF") == 0)

void TEST_SLOW_loadsave() {
    TEST_loadsave_CLEANUP();

    // test non-existing DB
    TEST_ASSERT_NORESULT__ERROREXPORTED(GB_open("nonexisting.arb", "r"));
    TEST_ASSERT_NORESULT__ERROREXPORTED(GB_open("nonexisting.arb", "rw"));
    {
        GBDATA *gb_new;
        TEST_ASSERT_RESULT__NOERROREXPORTED(gb_new = GB_open("nonexisting.arb", "w")); // create new DB
        GB_close(gb_new);
    }

    // the following DBs have to be provided in directory ../UNIT_TESTER/run
    const char *bin_db = "TEST_loadsave.arb";
    const char *asc_db = "TEST_loadsave_ascii.arb";

    GBDATA *gb_asc;
    TEST_ASSERT_RESULT__NOERROREXPORTED(gb_asc = GB_open(asc_db, "rw"));

#if defined(TEST_AUTO_UPDATE)
    TEST_ASSERT_NO_ERROR(GB_save_as(gb_asc, bin_db, "b")); 
#endif // TEST_AUTO_UPDATE
    
    GBDATA *gb_bin;
    TEST_ASSERT_RESULT__NOERROREXPORTED(gb_bin = GB_open(bin_db, "rw"));

    // test ASCII / BINARY compatibility
    SAVE_AND_COMPARE(gb_asc, "a2a.arb", "a", asc_db);
    SAVE_AND_COMPARE(gb_asc, "a2b.arb", "b", bin_db);
    SAVE_AND_COMPARE(gb_bin, "b2a.arb", "a", asc_db);
    SAVE_AND_COMPARE(gb_bin, "b2b.arb", "b", bin_db);

#if (MEMORY_TEST == 0)
    {
        GBDATA *gb_nomap;
        TEST_ASSERT_RESULT__NOERROREXPORTED(gb_nomap = GB_open(bin_db, "rw"));
        TEST_ASSERT_NO_ERROR(GB_save_as(gb_nomap, "fast.arb", "bm"));
        TEST_ASSERT(GB_is_regularfile("fast.ARM")); // assert map file has been saved
        GB_close(gb_nomap);
    }
    {
        // open DB with mapfile
        GBDATA *gb_map;
        TEST_ASSERT_RESULT__NOERROREXPORTED(gb_map = GB_open("fast.arb", "rw"));
        SAVE_AND_COMPARE(gb_map, "fast2b.arb", "b", bin_db);
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
        GBDATA *gb_a2b = GB_open("a2b.arb", "rw"); TEST_ASSERT(gb_a2b);
        GBDATA *gb_b2b = GB_open("b2b.arb", "rw"); TEST_ASSERT(gb_b2b);

        // modify ..
        TEST_ASSERT_NO_ERROR(modify_db(gb_a2b));
        TEST_ASSERT_NO_ERROR(modify_db(gb_b2b));

        // .. and quicksave
        TEST_ASSERT_NO_ERROR(GB_save_quick(gb_a2b, "a2b.arb"));
        TEST_ASSERT_NO_ERROR(GB_save_quick(gb_b2b, "b2b.arb"));

#if defined(TEST_AUTO_UPDATE)
        TEST_copy("a2b.a00", "TEST_loadsave_quick.a00");
#endif // TEST_AUTO_UPDATE

        TEST_ASSERT_FILES_EQUAL("TEST_loadsave_quick.a00", "a2b.a00");
        TEST_ASSERT_FILES_EQUAL("a2b.a00", "b2b.a00");

        TEST_ASSERT_NO_ERROR(GB_save_quick_as(gb_a2b, "a2b.arb"));

        // check wether quicksave can be disabled
        GB_disable_quicksave(gb_a2b, "test it");

        TEST_ASSERT_ERROR(GB_save_quick(gb_a2b, "a2b.arb"));
        TEST_ASSERT_ERROR(GB_save_quick_as(gb_a2b, "a2b.arb"));

        const char *mod_db = "a2b_modified.arb";
        TEST_ASSERT_NO_ERROR(GB_save_as(gb_a2b, mod_db, "b")); // save modified DB
        // test loading quicksave
        {
            GBDATA *gb_quicksaved = GB_open("a2b.arb", "rw"); // this DB has a quicksave
            SAVE_AND_COMPARE(gb_quicksaved, "a2b.arb", "b", mod_db);
            GB_close(gb_quicksaved);
        }


        // check quicksave delete and wrap around
        for (int i = 1; i <= 100; ++i) {
            GB_save_quick(gb_b2b, "b2b.arb");
            switch (i) {
                case 1: {
                    TEST_ASSERT(GB_is_regularfile("b2b.a00"));
                    TEST_ASSERT(GB_is_regularfile("b2b.a01"));
                    TEST_ASSERT(!GB_is_regularfile("b2b.a02")); // should not exist yet
                    break;
                }
                case 10: {
                    TEST_ASSERT(GB_is_regularfile("b2b.a01"));
                    TEST_ASSERT(GB_is_regularfile("b2b.a10"));
                    TEST_ASSERT(!GB_is_regularfile("b2b.a00")); // should no longer exist
                    TEST_ASSERT(!GB_is_regularfile("b2b.a11")); // should not exist yet

                    // speed-up-hack
                    GB_MAIN_TYPE *Main   = GB_MAIN(gb_b2b);
                    i                   += 78;
                    Main->qs.last_index += 78;
                    break;
                }
                case 98:  {
                    TEST_ASSERT(GB_is_regularfile("b2b.a89"));
                    TEST_ASSERT(GB_is_regularfile("b2b.a98"));
                    TEST_ASSERT(!GB_is_regularfile("b2b.a88")); // should no longer exist
                    TEST_ASSERT(!GB_is_regularfile("b2b.a99")); // should not exist yet
                    break;
                }
                case 99:  {
                    TEST_ASSERT(GB_is_regularfile("b2b.a90"));
                    TEST_ASSERT(GB_is_regularfile("b2b.a99"));
                    TEST_ASSERT(!GB_is_regularfile("b2b.a89")); // should no longer exist
                    TEST_ASSERT(!GB_is_regularfile("b2b.a00")); // should not exist yet
                    break;
                }
                case 100:  {
                    TEST_ASSERT(GB_is_regularfile("b2b.a00"));
                    TEST_ASSERT(GB_is_regularfile("b2b.a09"));
                    TEST_ASSERT(!GB_is_regularfile("b2b.a98")); // should no longer exist
                    TEST_ASSERT(!GB_is_regularfile("b2b.a99")); // should no longer exist
                    TEST_ASSERT(!GB_is_regularfile("b2b.a10")); // should not exist yet
                    break;
                }
            }
        }

        {
            // check master/slave DBs
            TEST_ASSERT_NO_ERROR(GB_save_as(gb_b2b, "master.arb", "b"));

            GBDATA *gb_master = GB_open("master.arb", "rw"); TEST_ASSERT(gb_master);
            TEST_ASSERT_NO_ERROR(modify_db(gb_master));

            TEST_ASSERT_NO_ERROR(GB_save_quick(gb_master, "master.arb"));
            TEST_ASSERT_NO_ERROR(GB_save_quick_as(gb_master, "master.arb"));

            TEST_ASSERT_ERROR(GB_save_quick(gb_master, "renamed.arb")); // quicksave with wrong name

            // check if master gets protected by creating slave-DB
            TEST_ASSERT_NO_ERROR(GB_save_as(gb_master, "master.arb", "b")); // overwrite
            TEST_ASSERT_NO_ERROR(GB_save_quick_as(gb_master, "slave.arb")); // create slave -> master now protected
            TEST_ASSERT_ERROR(GB_save_as(gb_master, "master.arb", "b")); // overwrite should fail now

            {
                GBDATA *gb_slave = GB_open("slave.arb", "rw"); TEST_ASSERT(gb_slave); // load slave DB
                TEST_ASSERT_NO_ERROR(modify_db(gb_slave));
                TEST_ASSERT_NO_ERROR(GB_save_quick(gb_slave, "slave.arb"));
                TEST_ASSERT_NO_ERROR(GB_save_quick_as(gb_slave, "renamed.arb"));
                GB_close(gb_slave);
            }
            GB_close(gb_master);
        }

        // test various error conditions:

        TEST_ASSERT_ERROR(GB_save_as(gb_b2b, "", "b")); // empty name

        TEST_ASSERT_NO_ERROR(GB_set_mode_of_file(mod_db, 0444)); // write-protect
        TEST_ASSERT_ERROR(GB_save_as(gb_b2b, mod_db, "b")); // try to overwrite write-protected DB

        TEST_ASSERT_ERROR(GB_save_quick_as(gb_b2b, NULL)); // no name
        TEST_ASSERT_ERROR(GB_save_quick_as(gb_b2b, "")); // empty name

        GB_close(gb_b2b);
        GB_close(gb_a2b);
    }

    GB_close(gb_asc);
    GB_close(gb_bin);

    TEST_loadsave_CLEANUP();
}

void TEST_db_filenames() {
    TEST_ASSERT_EQUAL(gb_quicksaveName("nosuch.arb", 0), "nosuch.a00");
    TEST_ASSERT_EQUAL(gb_quicksaveName("nosuch", 1), "nosuch.a01");
}

#endif
