#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
/* #include <malloc.h> */
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*#include "arbdb.h"*/
#include "adlocal.h"
#include "admap.h"

GB_MAIN_TYPE *gb_main_array[GB_MAIN_ARRAY_SIZE];

/********************************************************************************************
            Versions:
        ASCII

        V0  - 20.6.95
        V1  Full save
        V2  Differential save
********************************************************************************************/

GB_CPNTR gb_findExtension(GB_CSTR path)
{
    char *punkt = strrchr(path, '.');
    char *slash;
    if (!punkt) return 0;
    slash = strchr(punkt,'/');
    return slash? 0:punkt;
}


/*
 * CAUTION!!!
 *
 * The following functions (quicksaveName, oldQuicksaveName, mapfile_name, gb_overwriteName)
 * use static buffers for the created filenames.
 *
 * So you have to make sure, to use only one instance of every of these
 * functions or to dup the string before using the second instance
 *
 */



GB_CSTR gb_oldQuicksaveName(GB_CSTR path, int nr)
{
    static char *qname = 0;
    char *ext;

    STATIC_BUFFER(qname,strlen(path)+10);

    strcpy(qname,path);
    ext = gb_findExtension(qname);
    if (!ext) ext = qname + strlen(qname);

    if (nr==-1) sprintf(ext,".arb.quick?");
    else    sprintf(ext,".arb.quick%i", nr);

    return qname;
}

GB_CSTR gb_quicksaveName(GB_CSTR path,int nr)
{
    static char *qname = 0;
    char *ext;

    STATIC_BUFFER(qname,strlen(path)+4);

    strcpy(qname,path);
    ext = gb_findExtension(qname);
    if (!ext){
        ext = qname + strlen(qname);
    }

    if (nr==-1) sprintf(ext,".a??");
    else    sprintf(ext,".a%02i", nr);

    return qname;
}

GB_CSTR gb_mapfile_name(GB_CSTR path)
{
    static char *mapname = 0;
    char *ext;

    STATIC_BUFFER(mapname,strlen(path)+4);

    strcpy(mapname,path);
    ext = gb_findExtension(mapname);
    if (!ext) ext = mapname + strlen(mapname);

    strcpy(ext,".ARM");

    return mapname;
}

GB_CSTR gb_overwriteName(GB_CSTR path)
{
    static char *oname = 0;
    int          len   = strlen(path);

    STATIC_BUFFER(oname, len+2);
    strcpy(oname,path);
    strcpy(oname+len, "~");     // append ~

    return oname;
}

GB_CSTR gb_reffile_name(GB_CSTR path){
    static char *refname = 0;
    char *ext;
    STATIC_BUFFER(refname,  strlen(path)+4+1);

    strcpy(refname,path);
    ext = gb_findExtension(refname);
    if (!ext) ext = refname + strlen(refname);
    strcpy(ext,".ARF");
    return refname;
}

GB_ERROR gb_delete_reference(const char *master){
    char *fullmaster = gb_full_path(master);
    const char *fullref = gb_reffile_name(fullmaster);
    free(fullmaster);
    if ( GB_unlink(fullref) <0){
        return GB_export_error("Cannot delete file '%s'",fullref);
    }
    return 0;
}

GB_ERROR gb_create_reference(const char *master){
    char *fullmaster = gb_full_path(master);
    const char *fullref = gb_reffile_name(fullmaster);
    GB_ERROR error = 0;
    FILE *out = fopen(fullref,"w");
    if (out){
        fprintf(out,"***** The following files may be a link to %s ********\n",fullmaster);
        fclose(out);
        GB_set_mode_of_file(fullref,00666);
    }else{
        error = GB_export_error("WARNING: Cannot create file '%s'\n"
                                "   Your database is saved, but you should check write permissions\n"
                                "   in the destination directory",
                                fullref);
    }

    free(fullmaster);
    return error;
}

GB_ERROR gb_add_reference(char *master, char *changes){
    char *fullmaster = gb_full_path(master);
    char *fullchanges = gb_full_path(changes);
    const char *fullref = gb_reffile_name(fullmaster);
    GB_ERROR    error = 0;
    FILE *out = fopen(fullref,"a");
    if (out){
        fprintf(out,"%s\n",fullchanges);
        fclose(out);
        GB_set_mode_of_file(fullref,00666);
    }else{
        error = GB_export_error("Cannot add your file '%s'\n"
                                "   to the list of references of '%s'\n"
                                "   Please ask the owner of that file not to delete it\n"
                                "   or save the entire database (that's recommended!)",
                                fullchanges,fullref);
    }

    free(fullmaster);
    free(fullchanges);
    return error;
}
GB_ERROR gb_remove_quick_saved(GB_MAIN_TYPE *Main, const char *path)
{
    int i;
    GB_ERROR error = 0;

    for (i=0; i< GB_MAX_QUICK_SAVE_INDEX; i++)
    {
        if (GB_unlink(gb_quicksaveName(path,i))<0)
            error = GB_get_error();
    }

    for (i=0; i< 10; i++)
    {
        if (GB_unlink(gb_oldQuicksaveName(path,i))<0)
            error = GB_get_error();
    }

    if (Main) {
        Main->qs.last_index = -1;
    }
    return error;
}

GB_ERROR gb_remove_all_but_main(GB_MAIN_TYPE *Main, const char *path){
    GB_ERROR error = 0;
    error = gb_remove_quick_saved(Main,path);
    if (!error){
        if (GB_unlink(gb_mapfile_name(path))<0){
            error = GB_get_error();
        }
    }                 /* delete old mapfile */
    return error;
}

/*
 *  If we did a hundred quicksaves, we rename the quicksave-files to a00...a09
 *  to keep MS-DOS-compatibility
 */
static GB_ERROR renameQuicksaves(GB_MAIN_TYPE *Main)
{
    int i,
        j;
    GB_ERROR error = NULL;
    const char *path = Main->path;

    while(1)
    {
        for (i=0,j=0; i<GB_MAX_QUICK_SAVE_INDEX; i++)
        {
            GB_CSTR qsave = gb_quicksaveName(path,i);

            if (GB_is_regularfile(qsave))
            {
                if (i!=j)         /* otherwise the filename is correct */
                {
                    char *qdup = GB_STRDUP((char *)qsave);
                    GB_CSTR qnew = gb_quicksaveName(path,j);

                    GB_rename_file(qdup,qnew);
                    free(qdup);
                }

                j++;
            }
        }

        if (j<=GB_MAX_QUICK_SAVES) break; /* less or equal than GB_MAX_QUICK_SAVES files -> ok */

        i = 0;
        while (j>GB_MAX_QUICK_SAVES) /* otherwise delete files from lower numbers till there
                                      * are only GB_MAX_QUICK_SAVES left */
        {
            GB_CSTR qsave = gb_quicksaveName(path,i);

            if (GB_is_regularfile(qsave)) remove(qsave);
            j--;
            i++;
        }
    }

    Main->qs.last_index = j-1;

    return error;
}

static GB_ERROR deleteSuperfluousQuicksaves(GB_MAIN_TYPE *Main)
{
    int cnt = 0,
        i;
    char *path = Main->path;

    for (i=0; i<GB_MAX_QUICK_SAVE_INDEX; i++)
    {
        GB_CSTR qsave = gb_quicksaveName(path,i);

        if (GB_is_regularfile(qsave))
            cnt++;
    }

    for (i=0; cnt>GB_MAX_QUICK_SAVES && i<GB_MAX_QUICK_SAVE_INDEX; i++)
    {
        GB_CSTR qsave = gb_quicksaveName(path,i);

        if (GB_is_regularfile(qsave))
        {
            cnt--;
            GB_unlink(qsave);
        }
    }

    return NULL;
}




/********************************************************************************************
                    Ascii to  Binary
********************************************************************************************/


long gb_ascii_2_bin(const char *source,GBDATA *gbd)
{
    long size,memsize;

    register const char   *s;
    register char *d,c;
    register long   len,k;
    register    long i;
    len = 0;
    s = source;

    c=*(s++);
    A_TO_I(c);
    gbd->flags.compressed_data = c;

    if (*s == ':') {
        size = 0;
        s++;
    }else{
        for (i=0,k = 8;k && (c = *(s++));k--) {
            A_TO_I(c);
            i = (i<<4)+c;
        }
        size = i;
    }
    source = s;

    while ( (c = *(s++)) ) {
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

    GB_SETSMDMALLOC(gbd,size,memsize,0);
    d = GB_GETDATA(gbd);
    s = source;
    while ( (c = *(s++)) ) {
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
            *(d++)= 0xff;
            continue;
        }
        if (c == '=') {
            *(d++)= 0xff;
            *(d++)= 0xff;
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
/********************************************************************************************
                    Binary to  Ascii
********************************************************************************************/

#define GB_PUT(c,out) if(c>=10) c+='A'-10; else c+= '0'; *(out++) = (char)c;

GB_CPNTR gb_bin_2_ascii(GBDATA *gbd)
{
    register signed char  *s, *out, c, mo;
    register unsigned long    i;
    register    int j;
    char *buffer;
    int k;

    char *source = GB_GETDATA(gbd);
    long len = GB_GETMEMSIZE(gbd);
    long xtended = GB_GETSIZE(gbd);
    int compressed = gbd->flags.compressed_data;

    buffer = GB_give_buffer(len * 2 + 10);
    out = (signed char * )buffer;
    s = (signed char *)source, mo = -1;

    GB_PUT(compressed,out);
    if (!xtended) {
        *(out++) = ':';
    }else{
        for (i = 0xf0000000,j=28;j>=0;j-=4,i=i>>4) {
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






/********************************************************************************************
                    Write Ascii File
********************************************************************************************/
#define GB_PUT_OUT(c,out) if(c>=10) c+='A'-10; else c+= '0'; putc(c,out);

long gb_test_sub(GBDATA *gbd)
{
    gbd = gbd;
    return 0;
}

/* only used for saving ASCII database */
long gb_write_rek(FILE *out,GBCONTAINER *gbc,long deep,long big_hunk)
{
    register long i;
    register char *s;
    register char c;
    register GBDATA *gb;
    char    *strng;
    char *key;
    /*int index;*/
    for (gb = GB_find((GBDATA *)gbc,0,0,down_level);
         gb;
         gb = GB_find(gb,0,0,this_level|search_next)) {
        if (gb->flags.temporary) continue;
        key = GB_KEY(gb);
        if (!strcmp(key,GB_SYSTEM_FOLDER)) continue;    /* do not save system folder */
        for (i=deep;i--;) putc('\t',out);
        fprintf(out,"%s\t",key);
        if ((int)strlen(key) < 8){
            putc('\t',out);
        }
        if (    gb->flags.security_delete ||
                gb->flags.security_write ||
                gb->flags.security_read ||
                gb->flags2.last_updated ){
            putc(':',out);
            c = gb->flags.security_delete;
            GB_PUT_OUT(c,out);
            c = gb->flags.security_write;
            GB_PUT_OUT(c,out);
            c = gb->flags.security_read;
            GB_PUT_OUT(c,out);
            fprintf(out,"%i\t",gb->flags2.last_updated);
        }else{
            putc('\t',out);
        }
        switch (GB_TYPE(gb)) {
            case    GB_STRING:
                strng = GB_read_char_pntr(gb);
                if (*strng == '%') {
                    putc('%',out);
                    putc('s',out);
                    putc('\t',out);
                }
                GBS_fwrite_string(strng,out);
                putc('\n',out);
                break;
            case    GB_LINK:
                strng = GB_read_link_pntr(gb);
                if (*strng == '%') {
                    putc('%',out);
                    putc('l',out);
                    putc('\t',out);
                }
                GBS_fwrite_string(strng,out);
                putc('\n',out);
                break;
            case GB_INT:
                fprintf(out,"%%i %li\n",GB_read_int(gb));
                break;
            case GB_FLOAT:
                fprintf(out,"%%f %g\n",GB_read_float(gb));
                break;
            case GB_BITS:
                fprintf(out,"%%I\t\"%s\"\n",
                        GB_read_bits_pntr(gb,'-','+'));
                break;
            case GB_BYTES:
                s = gb_bin_2_ascii(gb);
                fprintf(out,"%%Y\t%s\n",s);
                break;
            case GB_INTS:
                s = gb_bin_2_ascii(gb);
                fprintf(out,"%%N\t%s\n",s);
                break;
            case GB_FLOATS:
                s = gb_bin_2_ascii(gb);
                fprintf(out,"%%F\t%s\n",s);
                break;
            case GB_DB:
                fprintf(out, "%%%% (%%\n");
                gb_write_rek(out, (GBCONTAINER *)gb, deep + 1, big_hunk);
                for (i=deep+1;i--;) putc('\t',out);
                fprintf(out, "%%) /*%s*/\n\n", GB_KEY(gb));
                break;
            case GB_BYTE:
                fprintf(out,"%%y %i\n",GB_read_byte(gb));
                break;
            default:
                fprintf(stderr,
                        "ARBDB ERROR Key \'%s\' is of unknown type\n",
                        GB_KEY(gb));
                fprintf(out, "%%%% (%% %%) /* unknown type */\n");
                break;
        }
    }/*while*/
    return 0;
}




/********************************************************************************************
                    Read Binary File
********************************************************************************************/

long    gb_read_in_long(FILE *in,long reversed)
{
    long sdata = 0;
    register char *p = (char *)&sdata;

    if (!reversed) {
        *(p++) = getc(in);
        *(p++) = getc(in);
        *(p++) = getc(in);
        *(p) = getc(in);
        return sdata;
    }
    p[3] = getc(in);
    p[2] = getc(in);
    p[1] = getc(in);
    p[0] = getc(in);
    return sdata;
}


long gb_read_number(FILE *in) {
    register unsigned int c0,c1,c2,c3,c4;
    c0 = getc(in);
    if (c0 & 0x80){
        c1 = getc(in);
        if (c0 & 0x40) {
            c2 = getc(in);
            if (c0 & 0x20) {
                c3 = getc(in);
                if (c0 &0x10) {
                    c4 = getc(in);
                    return c4 | (c3<<8) | (c2<<16) | (c1<<8);
                }else{
                    return (c3) | (c2<<8 ) | (c1<<16) | ((c0 & 0x0f)<<24);
                }
            }else{
                return (c2) | (c1<<8) | ((c0 & 0x1f)<<16);
            }
        }else{
            return (c1) | ((c0 & 0x3f)<<8);
        }
    }else{
        return c0;
    }
}

void gb_put_number(long i, FILE *out) {
    register long j;
    if (i< 0x80 ){ putc((int)i,out);return; }
    if (i<0x4000) {
        j = (i>>8) | 0x80;
        putc((int)j,out);
        putc((int)i,out);
        return;
    }
    if (i<0x200000) {
        j = (i>>16) | 0xC0;
        putc((int)j,out);
        j = (i>>8);
        putc((int)j,out);
        putc((int)i,out);
        return;
    }
    if (i<0x10000000) {
        j = (i>>24) | 0xE0;
        putc((int)j,out);
        j = (i>>16);
        putc((int)j,out);
        j = (i>>8);
        putc((int)j,out);
        putc((int)i,out);
        return;
    }
}

long gb_read_bin_error(FILE *in,GBDATA *gbd,const char *text)
{
    long p = (long)ftell(in);
    GB_export_error("%s in reading GB_file (loc %li=%lX) reading %s\n",
                    text,p,p,GB_KEY(gbd));
    GB_print_error();
    return 0;
}




/********************************************************************************************
                    Write Binary File
********************************************************************************************/

long    gb_write_out_long(long data, FILE *out)
{
    long sdata[1];
    register char *p = (char *)sdata,c;
    sdata[0] = data;
    c = *(p++);
    putc(c,out);
    c = *(p++);
    putc(c,out);
    c = *(p++);
    putc(c,out);
    c = *(p);
    putc(c,out);
    return 0;
}
/** Test whether to write any data to disc.
    version 1       write only latest data
    version 2       write deleted entries two (which are not already stored to master file !!!
    try to avoid to access gbd, keep it swap out
*/


int gb_is_writeable(struct gb_header_list_struct *header, GBDATA *gbd, long version, long diff_save)
{
    if (version == 2 && header->flags.changed==gb_deleted) return 1;    /* save delete flag */
    if (!gbd) return 0;
    if (diff_save) {
        if (!header->flags.ever_changed) return 0;
        if (!gbd->ext || (gbd->ext->update_date<diff_save && gbd->ext->creation_date < diff_save))
            return 0;
    }
    if (gbd->flags.temporary) return 0;
    return 1;
}

int gb_write_bin_sub_containers(FILE *out,GBCONTAINER *gbc,long version,long diff_save, int is_root){
    struct gb_header_list_struct *header;
    register int i,index;
    int counter;
    header = GB_DATA_LIST_HEADER(gbc->d);
    for (i=0, index = 0; index < gbc->d.nheader; index++) {
        if (gb_is_writeable(&(header[index]),GB_HEADER_LIST_GBD(header[index]),version,diff_save)) i++;
    }

    if (!is_root){
        gb_put_number(i,out);
    }else{
        gb_write_out_long(i,out);
    }

    counter = 0;
    for (index = 0; index < gbc->d.nheader; index++){
        GBDATA *h_gbd;

        if (header[index].flags.changed == gb_deleted_in_master){   /* count deleted items in master, because of index renaming */
            counter ++;
            continue;
        }

        h_gbd = GB_HEADER_LIST_GBD(header[index]);

        if(!gb_is_writeable(&(header[index]),h_gbd,version,diff_save))
        {   /* mark deleted in master */
            if (version <= 1 && header[index].flags.changed == gb_deleted) {
                header[index].flags.changed = gb_deleted_in_master;
            }
            continue;
        }

        if (h_gbd) {
            i = (int)gb_write_bin_rek(out,h_gbd,version,diff_save,index-counter);
            if (i) return i;
        }else{
            if (header[index].flags.changed == gb_deleted ) {
                putc(0,out);
                putc(1,out);
                gb_put_number(index - counter,out);
            }
        }
    }
    return 0;
}

long gb_write_bin_rek(FILE *out,GBDATA *gbd,long version, long diff_save, long index_of_master_file)
{
    register int i;
    GBCONTAINER *gbc =0;
    long    memsize =0;
    int type;

    if ((type=GB_TYPE(gbd)) == GB_DB) {
        gbc = (GBCONTAINER *)gbd;
    }else   if (    ( type == GB_STRING) &&
                    !gbd->flags.compressed_data &&
                    (memsize = GB_GETMEMSIZE(gbd)) < GBTUM_SHORT_STRING_SIZE){
        type = GB_STRING_SHRT;
    }

    i =     (type<<4)
        +   (gbd->flags.security_delete<<1)
        +   (gbd->flags.security_write>>2);
    putc(i,out);

    i =     ((gbd->flags.security_write &3) <<6)
        +   (gbd->flags.security_read<<3)
        +   (gbd->flags.compressed_data<<2)
        +   ((GB_ARRAY_FLAGS(gbd).flags&1)<<1)
        +   (gbd->flags.unused);
    putc(i,out);

    gb_put_number(GB_ARRAY_FLAGS(gbd).key_quark,out);

    if (diff_save) {
        gb_put_number(index_of_master_file,out);
    }

    i = gbd->flags2.last_updated;
    putc(i,out);

    switch(type) {
        case    GB_STRING_SHRT:
            if (memsize) {
                i = fwrite(GB_GETDATA(gbd),(size_t)memsize,1,out);
                if (i<=0){
                    return -1;
                }
            }else{
                putc(0,out);
            }
            return 0;

        case    GB_DB:
            i = gb_write_bin_sub_containers(out,gbc,version,diff_save,0);
            return i;

        case    GB_INT:{
            GB_UINT4 buffer = (GB_UINT4)htonl(gbd->info.i);
            if (!fwrite((char *)&buffer,sizeof(float),1,out)) return -1;
            return 0;
        }
        case    GB_FLOAT:
            if (!fwrite((char *)&gbd->info.i,sizeof(float),1,out)) return -1;
            return 0;
        case    GB_STRING:
        case    GB_LINK:
        case    GB_BITS:
        case    GB_BYTES:
        case    GB_INTS:
        case    GB_FLOATS:
            memsize = GB_GETMEMSIZE(gbd);
            gb_put_number(GB_GETSIZE(gbd),out);
            gb_put_number(memsize,out);
            i = fwrite(GB_GETDATA(gbd),(size_t)memsize,1,out);
            if (memsize && !i ) return -1;
            return 0;
        case    GB_BYTE:
            putc((int)(gbd->info.i),out);
            return 0;
        default:
            return 0;
    }
}


/**     version 1 write master arb file
        version 2 write slave arb file */
int gb_write_bin(FILE *out,GBDATA *gbd,long version){
    long i;
    GBCONTAINER *gbc = (GBCONTAINER *)gbd;
    int diff_save = 0;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbc);

    gb_write_out_long(GBTUM_MAGIC_NUMBER,out);
    fprintf(out,"\n this is the binary version of the gbtum data file version %li\n",version);
    putc(0,out);
    fwrite("vers",4,1,out);
    gb_write_out_long(0x01020304,out);
    gb_write_out_long(version,out);
    fwrite("keys",4,1,out);

    for (i=1;i<Main->keycnt;i++) {
        if (Main->keys[i].nref>0) {
            gb_put_number(Main->keys[i].nref,out);
            fprintf(out,"%s",Main->keys[i].key);
        }else{
            putc(0,out);        /* 0 nref */
            putc(1,out);        /* empty key */
        }
        putc(0,out);

    }
    putc(0,out);
    putc(0,out);
    fwrite("time",4,1,out);{
        unsigned int k;
        for (k=0;k<Main->last_updated;k++) {
            fprintf(out,"%s",Main->dates[k]);
            putc(0,out);
        }
    }
    putc(0,out);
    fwrite("data",4,1,out);
    if (version == 2) diff_save = (int)Main->last_main_saved_transaction+1;

    return gb_write_bin_sub_containers(out,gbc,version,diff_save,1);
}

/********************************************************************************************
                    SAVE DATABASE
********************************************************************************************/


GB_ERROR GB_save(GBDATA *gb,const char *path,const char *savetype)
     /*
      * savetype 'a'    ascii
      *          'aS'   dump to stdout
      *          'b'    binary
      *          'bm'   binary + mapfile
      *          0      ascii
      */
{
    if (path && strchr(savetype, 'S') == 0) // 'S' dumps to stdout -> do not change path
    {
        free(GB_MAIN(gb)->path);
        GB_MAIN(gb)->path = GB_STRDUP(path);
    }
    return GB_save_as(gb,path,savetype);
}

/*  -------------------------------------------------------  */
/*      GB_ERROR GB_create_directory(const char *path)       */
/*  -------------------------------------------------------  */
GB_ERROR GB_create_directory(const char *path) {
    char     *mkdir          = GB_give_buffer(1024);
    GB_ERROR  error          = 0;
    sprintf(mkdir, "mkdir -p %s", path);
    if (system(mkdir)) error = GB_export_error("Cannot create directory %s",path);
    /* @@@ FIXME: use POSIX command instead of system call!  */
    return error;
}

GB_ERROR GB_save_in_home(GBDATA *gb,const char *path,const char *savetype)
     /*
      * savetype 'a'    ascii
      *      'b'    binary
      *      'bm'   binary + mapfile
      *      0=ascii
      *
      *      creates subdirectories too
      */
{
    GB_ERROR error = 0;
    char *buffer;
    const char *env;
    char *slash;
    /*     char *buf2; */

    env = GB_getenvHOME();
    if(!path) path = GB_MAIN(gb)->path;

    buffer = (char *)GB_calloc(sizeof(char),strlen(env)+ strlen(path) + 2);

    sprintf(buffer,"%s/%s",env,path);
    slash             = strrchr(buffer,'/');
    *slash            = 0;
    /*     buf2 = GB_give_buffer(1024); */
    /*     sprintf(buf2,"mkdir -p %s",buffer); */
    /*     if (system(buf2)) error = GB_export_error("Cannot create directory %s",buffer); */
    error             = GB_create_directory(buffer);
    *slash            = '/';
    if (!error) error = GB_save_as(gb,buffer,savetype);
    if (buffer) free(buffer);
    return error;
}


/** Save whole database */
GB_ERROR GB_save_as(GBDATA *gb,const char *path,const char *savetype)
     /*
      * savetype    'a' ascii
      *         'b' binary
      *         'm' save mapfile too (only with binary)
      *         'f' force saving even in disabled path to a different directory (out of order save)
      *                      'S' save to stdout (for debugging)
      *          0=ascii
      *
      */
{
    FILE *out;
    long erg =0;
    int slevel;
    int translevel;
    char    *sec_path = NULL;
    GB_CSTR map_path = NULL;
    GB_MAIN_TYPE *Main = GB_MAIN(gb);
    int deleteQuickAllowed = savetype==NULL || strchr(savetype, 'f')==NULL;
    int dump_to_stdout = 0;

    if (gb == NULL) return NULL;
    gb = (GBDATA *)Main->data;

    if (path == NULL) path = Main->path;

    if (!path || !strlen(path) )
    {
        GB_export_error("Please specify a file name");
        goto error;
    }

    if (gb_check_saveable(gb,path,savetype)) goto error;

    sec_path = GB_STRDUP(gb_overwriteName(path));

    if (strchr(savetype,'S')) {
        out = stdout;
        dump_to_stdout = 1;
    }
    else if ((out = fopen(sec_path, "w")) == NULL)
    {
        GB_export_IO_error("saving",sec_path);
/*         printf(" file %s could not be opened for writing \n", sec_path); */
/*         GB_export_error("ARBDB ERROR: Cannot save file to '%s'",sec_path); */
        goto error;
    }

    slevel = Main->security_level;
    translevel = Main->transaction;

    if (!translevel) Main->transaction = 1;
    else
    {
        if (translevel> 0 )
        {
            GB_commit_transaction(gb);
            GB_begin_transaction(gb);
        }
    }

    Main->security_level = 7;

    if (!savetype || strchr(savetype,'a'))          /* ASCII */
    {
        fprintf(out,"/*ARBDB ASCII*/\n");
        erg = gb_write_rek(out,(GBCONTAINER *)gb,0,1);
        if (erg==0)
        {
            if (Main->qs.quick_save_disabled) free(Main->qs.quick_save_disabled);
            Main->qs.quick_save_disabled = strdup("Database saved in ASCII mode");
            if (deleteQuickAllowed && gb_remove_all_but_main(Main,path)) goto error;
        }
    }
    else if (strchr(savetype,'b'))              /* binary */
    {
        /* it's necessary to save the mapfile FIRST, cause this re-orders all GB_CONTAINERs
         * containing NULL-entries in their header */

        erg = 0;
        if (strchr(savetype,'m'))
        {
            map_path = gb_mapfile_name(path);
            erg |= gb_save_mapfile(Main,map_path);          /* save mapfile */
            erg |= gb_write_bin(out,gb,1);                  /* save binary */
        }
        else
        {
            GB_unlink(gb_mapfile_name(path));                  /* delete old mapfile */
            erg |= gb_write_bin(out,gb,1);                  /* save binary */
        }

        if (erg==0){
            if (!strchr(savetype,'f')){             /* allow quick saving unless saved in 'f' mode */
                if (Main->qs.quick_save_disabled) free(Main->qs.quick_save_disabled);
                Main->qs.quick_save_disabled = 0; /* and allow quicksaving */
            }
            if (deleteQuickAllowed && gb_remove_quick_saved(Main,path)) goto error;
        }
    }

    Main->security_level = slevel;
    Main->transaction = translevel;
    erg |= fclose(out);

    if (erg) {
        GB_export_IO_error("writing", sec_path);
/*         GB_export_error("ARBDB: Write Error, system errno = '%i', see console", errno); */
/*         perror("Write Error"); */
        goto error;
    }


    if (!dump_to_stdout) {
        if (GB_rename_file(sec_path, path)==0){
            if (map_path){
                GB_CSTR overwrite_map_path = gb_overwriteName(map_path);
                long path_mode = GB_mode_of_file(path);

                if (GB_rename_file(overwrite_map_path, map_path)==0)
                {
                    GB_set_mode_of_file(map_path,path_mode);        /* set mapfile to same mode as binary file */
                }
                else /* if we cannot rename the mapfile, then we delete it */
                {
                    GB_unlink(overwrite_map_path);
                    goto error;
                }
            }
            if (Main->qs.quick_save_disabled == 0){     /* do we need an ARF file ?? */
                gb_create_reference(path);
            }else{
                gb_delete_reference(path);              /* delete old ARF file */
            }
        }else{
            goto error;
        }
    }
    free(sec_path);sec_path = 0;
    if (!strchr(savetype,'f')){ /* reset values unless out of order save */
        Main->last_saved_transaction = GB_read_clock(gb);
        Main->last_main_saved_transaction = GB_read_clock(gb);
        Main->last_saved_time = GB_time_of_day();
    }
    return NULL;

 error:

    if (sec_path) free(sec_path);
    return GB_get_error();
}

/** is quick save allowed ??? */
GB_ERROR gb_check_quick_save(GBDATA *gb_main, char *refpath)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GBUSE(refpath);
    gb_main = (GBDATA *)Main->data;

    if (Main->qs.quick_save_disabled) {
        return GB_export_error("Save Changes Disabled, because\n"
                               "    '%s'\n"
                               "    Save whole database using binary mode first",
                               Main->qs.quick_save_disabled);
    }

    return 0;
}

GB_ERROR GB_delete_database(GB_CSTR filename)
{
    if (GB_unlink(filename)==-1)
    {
        return GB_get_error();
    }

    if (gb_remove_all_but_main(0,filename)){
        return GB_get_error();
    }
    return NULL;
}

GB_ERROR GB_save_quick_as(GBDATA *gb_main, char *path)
{
    FILE *fmaster;
    long mode;
    char *org_master;
    char *full_path_of_source;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);

    gb_main = (GBDATA *)Main->data;

    if (!path || !strlen(path) ) {
        return GB_export_error("Please specify a file name");
    }

    if (!strcmp(path,Main->path)) return GB_save_quick(gb_main,path);   /* No rename */
    if (gb_check_quick_save(gb_main,path)) return GB_get_error();
    if (gb_check_saveable(gb_main,path,"bn")) return GB_get_error();

    fmaster = fopen( Main->path, "r" ); /* old master !!!! */
    if (!fmaster){      /* Oh no, where is my old master */
        return GB_export_error("Save Changes is missing master ARB file '%s',\n"
                               "    save database first", Main->path);
    }
    fclose(fmaster);
    if (GB_unlink(path)<0){
        return GB_export_error("File '%s' already exists and cannot be deleted",path);
    };      /* delete old file */

    mode = GB_mode_of_link(Main->path); /* old master */
    if (S_ISLNK(mode)){
        org_master = GB_follow_unix_link(Main->path);
    }else{
        org_master = GB_STRDUP(Main->path);
    }

    if( gb_remove_all_but_main(Main,path))
    {
        free(org_master);
        return GB_get_error();
    }

    mode = GB_mode_of_file(org_master);
    if (mode & S_IWUSR) {
        if (GB_set_mode_of_file(org_master,mode & ~(S_IWUSR | S_IWGRP | S_IWOTH))){
            GB_warning( "!!!!!!!!! WARNING !!!!!!!\n"
                        "   Cannot set mode of file '%s'\n"
                        "   NEVER, NEVER delete or change file\n"
                        "        '%s'\n"
                        "   Best: Ask your system administrator\n"
                        "   to remove write permissions.",
                        org_master,org_master);
        }
    }
    if (strchr(path,'/') || strchr(org_master,'/') ){
        /* dest or source in different directory */
        full_path_of_source = gb_full_path(org_master);
    }else{
        full_path_of_source = GB_STRDUP(org_master);
    }

    if (GB_symlink(full_path_of_source,path)){
        free(org_master);
        free(full_path_of_source);
        return GB_get_error();
    }
    if (GB_getuid_of_file(full_path_of_source) != GB_getuid()){
        GB_warning( "**** WARNING ******\n"
                    "   You now using a file '%s'\n"
                    "   which is owned by another user\n"
                    "   Please ask him not to delete this file\n"
                    "   If you don't trust him, don't save changes but\n"
                    "   the WHOLE database",full_path_of_source);
    }
    if (gb_add_reference(full_path_of_source,path)){
        GB_warning("%s",GB_get_error());
    }

    free(Main->path);       /* Symlink created -> rename allowed */
    Main->path = GB_STRDUP(path);

    free(full_path_of_source);
    free(org_master);

    Main->qs.last_index = 0;        /* Start with new quicks */
    return GB_save_quick(gb_main,path);
}


GB_ERROR GB_save_quick(GBDATA *gb, char *refpath)
{
    FILE *out;
    GB_CSTR path;
    GB_CSTR sec_path;
    long erg;
    int slevel;
    int translevel;
    FILE *fmaster;
    GB_MAIN_TYPE *Main = GB_MAIN(gb);

    gb = (GBDATA *)Main->data;
    if (gb_check_quick_save(gb,refpath)) return GB_get_error();
    if (gb_check_saveable(gb,refpath,"q")) return GB_get_error();

    if (refpath && strcmp(refpath, Main->path) )
    {
        return GB_export_error("Internal ARB Error, master file rename '%s'!= '%s',\n"
                               "    save database first", refpath,Main->path);
    }

    fmaster = fopen( Main->path, "r");

    if (!fmaster) /* Oh no, where is my old master */
    {
        return GB_export_error("Quick save is missing master ARB file '%s',\n"
                               "    save database first", refpath);
    }
    fclose(fmaster);

    if (Main->local_mode == GB_FALSE)
    {
        GB_export_error("You cannot save a remote database");
        GB_print_error();
        return GB_get_error();
    }

    Main->qs.last_index++;
    if (Main->qs.last_index >= GB_MAX_QUICK_SAVE_INDEX) renameQuicksaves(Main);

    path = gb_quicksaveName(Main->path, Main->qs.last_index);
    sec_path = gb_overwriteName(path);

    if ((out = fopen(sec_path, "w")) == NULL) return GB_export_error("ARBDB ERROR: Cannot save file to '%s'",sec_path);

    slevel = Main->security_level;
    translevel = Main->transaction;

    if (!translevel)
        Main->transaction = 1;
    else
    {
        if (translevel> 0 )
        {
            GB_commit_transaction(gb);
            GB_begin_transaction(gb);
        }
    }

    Main->security_level = 7;
    erg = gb_write_bin(out,gb,2);

    Main->security_level = slevel;
    Main->transaction = translevel;
    erg |= fclose(out);
    if (erg!=0) return GB_export_error("Cannot write to '%s'",sec_path); /* was: '%s%%'",path);*/

    if (GB_rename_file(sec_path,path)) return GB_get_error();

    Main->last_saved_transaction = GB_read_clock(gb);
    Main->last_saved_time = GB_time_of_day();

    deleteSuperfluousQuicksaves(Main);

    return NULL;
}

/********************************************************************************************
                    DISABLE DIRECTORIES
********************************************************************************************/

/** returns 0 if user is allowed to save */

char *gb_full_path(const char *path) {
    const char *cwd;
    char *res;
    if (path[0] == '/') return GB_STRDUP(path);

    cwd = GB_getcwd();

    if (path[0] == 0) return GB_STRDUP(cwd);
    res = GB_STRDUP(GBS_global_string("%s/%s",cwd,path));

    return res;
}


/* Check wether file can be stored at destination
 * a 'f' in flags means 'force', disables main->disabled_path
 * a 'q' in flags means 'quick save'
 * a 'n' in flags means destination must be empty */

GB_ERROR gb_check_saveable(GBDATA *gbd,const char *path,const char *flags){
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    char *fullpath;
    long mode;
    if (Main->local_mode == GB_FALSE) {
        GB_export_error("You cannot save a remote database, please use save button in master program");
        GB_print_error();
        return GB_get_error();
    }
    if (Main->opentype == gb_open_read_only_all) {
        GB_export_error("Database is read only");
        GB_print_error();
        return GB_get_error();
    }
    if (strchr(path,':')){
        return GB_export_error( "Your database name must not contain a ':' character\n"
                                "   Choose a different name");
    }

    fullpath = gb_full_path(path);
    if (Main->disabled_path && !strchr(flags,'f') ) {
        if( ! GBS_string_cmp(fullpath,Main->disabled_path,0) ) {
            free(fullpath);
            return GB_export_error(
                                   "You are not allowed to save your database in this directory,\n"
                                   "    Please select 'save as' and save your data to a different location");
        }
    }

    // check whether destination directory exists
    {
        char *lslash = strrchr(fullpath, '/');
        if (lslash) {
            GB_ERROR err = 0;

            lslash[0] = 0;
            if (!GB_is_directory(fullpath)) {
                err = GB_export_error("Directory '%s' doesn't exist", fullpath);
            }
            lslash[0] = '/';

            if (err) return err;
        }
    }

    free(fullpath);

    if ( !strchr(flags,'q')){
        mode = GB_mode_of_link(path);
        if (mode >=0 && !(mode & S_IWUSR)){ /* no write access -> lookes like a master file */
            return GB_export_error(
                                   "Your selected file '%s' already exists and is write protected\n"
                                   "    It looks like that your file is a master arb file which is\n"
                                   "    used by some xxx.a?? quicksave databases\n"
                                   "    If you want to save it nevertheless, delete it first !!!",
                                   path);
        }
    }
    if ( strchr(flags,'n')){
        if (GB_time_of_file(path)){
            return GB_export_error( "Your destination file '%s' already exists.\n"
                                    "\tDelete it by hand first",path );
        }
    }
    return 0;
}

GB_ERROR GB_disable_path(GBDATA *gbd,const char *path){
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    if (Main->disabled_path) free(Main->disabled_path);
    if (path) {
        Main->disabled_path = GBS_eval_env(path);
    }else{
        Main->disabled_path = NULL;
    }
    return 0;
}




