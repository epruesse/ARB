// =============================================================== //
//                                                                 //
//   File      : admap.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_map.h"
#include "gb_storage.h"
#include "gb_index.h"

#define ADMAP_BYTE_ORDER    0x01020304
#define GB_MAX_MAPPED_FILES 10

#ifdef DIGITAL
#   define ALIGN_BITS 3
#else
#   define ALIGN_BITS 2                             // if ALIGN_BITS == 3 we get problems when writing pointer-arrays
#endif

#define ALIGN(size)     (((((size)-1)>>ALIGN_BITS)+1)<<ALIGN_BITS)
#define PTR_DIFF(p1,p2) ((char*)(p1)-(char*)(p2))

struct gbdata_offset
{
    GBDATA *gbd;
    long    index;                                  // new index
    long    offset;                                 // offset in mapfile (initialized with -1)
};

struct gbdByKey {                                   // one for each diff. keyQuark
    int            cnt;
    gbdata_offset *gbdoff;
};

static gbdByKey *gb_gbk = NULL;


inline long MAKEREL(long rel_to, long offset) {
    gb_assert(rel_to);
    return offset ? offset-rel_to : 0;
}

// ---------------------------
//      sort and binsearch

#define cmp(h1,h2)  ((long)(h1).gbd - (long)(h2).gbd)

#define swap(h1,h2)                         \
    do {                                    \
        struct gbdata_offset xxx = (h1);    \
        (h1) = (h2);                        \
        (h2) = xxx;                         \
    } while(0)

static void downheap(struct gbdata_offset *heap, int idx, int num)
{
    int idx2  = idx<<1;
    int idx21 = idx2+1;
    gb_assert(idx>=1);

    if (idx2>num)
        return;                     // no lson -> done

    if (cmp(heap[idx2],heap[idx])>0)            // lson is bigger than actual
    {
        if (idx21 <= num &&             // rson exists
            cmp(heap[idx2],heap[idx21])<0)  // lson is smaller than rson
        {
            swap(heap[idx],heap[idx21]);
            downheap(heap,idx21,num);
        }
        else
        {
            swap(heap[idx],heap[idx2]);
            downheap(heap,idx2,num);
        }
    }else if (idx21 <= num &&               // rson exists
              cmp(heap[idx],heap[idx21])<0)         // rson is bigger than actual
    {
        swap(heap[idx],heap[idx21]);
        downheap(heap,idx21,num);
    }
}

static void sort_gbdata_offsets(struct gbdata_offset *gbdo, int num)
{
    int i;
    struct gbdata_offset *heap = gbdo-1;
#if defined(DEBUG)
    int onum = num;
#endif // DEBUG

    gb_assert(gbdo!=NULL);
    gb_assert(num>=1);

    for (i=num/2; i>=1; i--)
        downheap(heap,i,num); // make heap

    while(num>1)    // sort heap
    {
        struct gbdata_offset big = heap[1];

        heap[1] = heap[num];
        downheap(heap,1,num-1);
        heap[num] = big;
        num--;
    }

#ifdef DEBUG
    for (i=1; i<onum; i++) { // test if sorted
        gb_assert(cmp(heap[i],heap[i+1])<0);
    }
#endif
}

static struct gbdata_offset *find_gbdata_offset(GBQUARK quark, GBDATA *gbd){
    struct gbdata_offset *gbdo = gb_gbk[quark].gbdoff;
    long l=0,
        h=gb_gbk[quark].cnt-1,
        m;

    gb_assert(h>=l);
    gb_assert(gbdo!=NULL);

    while (1){
        long cmpres;

        m = (l+h)>>1;
        cmpres = (long)gbd - (long)gbdo[m].gbd;

        if (cmpres == 0){   // equal m
            return &gbdo[m];
        }
        else {
            if (l==h) break;
            if (cmpres < 0) h = m;
            else        l = m+1;
        }
    }

    printf("not found(1): gbd=%lx\n",(long)gbd);
    gb_assert(0);   // should never occur
    return 0;
}

static long getrel_GBDATA(long rel_to, GBDATA *gbd) {
    // calcs offset of 'gbd' in mapfile _relative_ to offset 'rel_to'

    if (gbd) {
        GBQUARK quark = gbd->rel_father ? GB_KEY_QUARK(gbd) : 0; // cause Main->data->father==NULL !!
        struct gbdata_offset *gbdo = gb_gbk[quark].gbdoff;
        int l=0,
            h=gb_gbk[quark].cnt-1,
            m;

        gb_assert(h>=l);
        gb_assert(gbdo!=NULL);

        while (1)
        {
            long cmpres;

            m = (l+h)>>1;
            cmpres = (long)gbd - (long)gbdo[m].gbd;

            if (cmpres == 0){   // equal m
                return MAKEREL(rel_to,gbdo[m].offset);
            }
            else {
                if (l==h) break;

                if (cmpres < 0) h = m;
                else        l = m+1;
            }
        }

        printf("not found(2): gbd=%lx\n",(long)gbd);
        gb_assert(0);   // should never occur
    }

    return 0;
}

#undef cmp
#undef swap

/* ********************************************************
   write - routines
   ******************************************************** */

static int writeError;

void ftwrite_aligned(const void *ptr, size_t ali_siz, FILE *fil) {
    gb_assert(ali_siz == ALIGN(ali_siz));
    if (!writeError && fwrite((const char *)ptr, 1, ali_siz, fil) != ali_siz) {
        writeError = 1;
    }
}

static char alignment_bytes[ALIGN(1)] = { 0 }; // zero-filled buffer with maximum alignment size


size_t ftwrite_unaligned(const void *ptr, size_t unali_siz, FILE *fil) {
    // ftwrite_unaligned does the same as ftwrite_aligned,
    // but does not access uninitialized memory (that's better for valgrind)

    if (!writeError) {
        size_t ali_siz   = ALIGN(unali_siz);
        size_t pad_bytes = ali_siz-unali_siz;

        if (fwrite((const char*)(ptr), 1, unali_siz, fil) == unali_siz) {
            if (pad_bytes == 0 || fwrite(alignment_bytes, 1, pad_bytes, fil) == pad_bytes) {
                return ali_siz; // success -> return size written
            }
        }
    }
    return 0; // failure
}

static long write_IE(struct gb_if_entries *ie, FILE *out, long *offset) {
    // parameters mean the same as in write_GBDATA
    long ieoffset = ie ? *offset : 0;

    while (ie)
    {
        struct gb_if_entries copy;
        size_t               copy_size;

        if (out) {
            copy.rel_ie_gbd = (GB_REL_GBDATA) getrel_GBDATA(*offset,GB_IF_ENTRIES_GBD(ie));
            copy.rel_ie_next = (GB_REL_IFES)( ie->rel_ie_next ? ALIGN(sizeof(copy)) : 0);

            copy_size = ftwrite_unaligned(&copy,sizeof(copy),out);
        }
        else {
            copy_size = ALIGN(sizeof(copy));
        }

        *offset += copy_size;
        ie = GB_IF_ENTRIES_NEXT(ie);
    }

    return ieoffset;
}

static long write_IFS(struct gb_index_files_struct *ifs, FILE *out, long *offset) {
    // parameters mean the same as in write_GBDATA
    long    ifsoffset,
        nextoffset,
        entriesoffset;

    if (!ifs) return 0;

    nextoffset = write_IFS(GB_INDEX_FILES_NEXT(ifs), out, offset);

    // entries

    {
        GB_REL_IFES *ie     = GB_INDEX_FILES_ENTRIES(ifs);
        GB_REL_IFES *iecopy;
        size_t       iesize = ALIGN((size_t)(ifs->hash_table_size)*sizeof(*ie));
        int          idx;

        gb_assert(ALIGN(sizeof(*ie))==sizeof(*ie));

        iecopy = (GB_REL_IFES *)malloc(iesize);
        memcpy(iecopy,ie,iesize);

        // write index entries an calc absolute offsets

        for (idx=0; idx<ifs->hash_table_size; idx++) {
            iecopy[idx] = (GB_REL_IFES) write_IE(GB_ENTRIES_ENTRY(ie,idx),out,offset);
        }

        // convert to relative offsets and write them

        entriesoffset = *offset;
        for (idx=0; idx<ifs->hash_table_size; idx++) {
            iecopy[idx] = (GB_REL_IFES)MAKEREL(entriesoffset,(long)iecopy[idx]);
        }

        if (out) ftwrite_aligned(iecopy,iesize,out);
        *offset += iesize;

        free(iecopy);
    }

    // ifs

    {
        struct gb_index_files_struct ifscopy = *ifs;
        size_t ifscopy_size;

        ifsoffset = *offset;

        ifscopy.rel_if_next = (GB_REL_IFS)MAKEREL(ifsoffset,nextoffset);
        ifscopy.rel_entries = (GB_REL_PIFES)MAKEREL(ifsoffset,entriesoffset);

        if (out) ifscopy_size = ftwrite_unaligned(&ifscopy, sizeof(ifscopy), out);
        else     ifscopy_size = ALIGN(sizeof(ifscopy));

        *offset += ifscopy_size;
    }

    return ifsoffset;
}

static void convertFlags4Save(struct gb_flag_types *flags, struct gb_flag_types2 * flags2, struct gb_flag_types3 *flags3)
{
    // GBUSE(flags3);
    flags->unused = 0;
    flags->user_flags = 0;
    gb_assert(flags->temporary==0);
    flags->saved_flags = 0;

    flags2->last_updated = 0;
    flags2->usr_ref = 0;
    flags2->folded_container = 0;
    flags2->update_in_server = 0;
    flags2->header_changed = 0;

    /*  if (flags3)
        {

        }
    */
}

static long write_GBDATA(GB_MAIN_TYPE *Main,GBDATA *gbd, GBQUARK quark, FILE *out, long *offset,GB_MAIN_IDX main_idx)
     /*
       if out==NULL -> only calculate size

       changes     'offset' according to size of written data
       returns     offset of GBDATA in mapfile
     */
{
    int  type = GB_TYPE(gbd);
    long gbdoffset;

    // GBUSE(Main);
    gb_assert(gbd->flags.temporary==0);

    if (type==GB_DB)    // CONTAINER
    {
        GBCONTAINER *gbc = (GBCONTAINER*)gbd, gbccopy = *gbc;
        long headeroffset, ifsoffset;

        // header

        {
            struct gb_header_list_struct *header, *headercopy;
            long headermemsize = ALIGN(gbc->d.headermemsize*sizeof(*header));
            int item, nitems = gbc->d.nheader;

            headeroffset = *offset;
            header = GB_DATA_LIST_HEADER(gbc->d);
            gb_assert(PTR_DIFF(&(header[1]),&(header[0]))==sizeof(*header));    // @@@@

            if (headermemsize){      // if container is non-empty

                if (out){
                    int valid=0;    // no of non-temporary items
                    headercopy = (struct gb_header_list_struct*) malloc(headermemsize);
                    gb_assert(sizeof(*headercopy)==ALIGN(sizeof(*headercopy)));
                    memset(headercopy,0x0,headermemsize);

                    for (item=0; item<nitems; item++)
                    {
                        GBDATA *gbd2 = GB_HEADER_LIST_GBD(header[item]);
                        long hs_offset;

                        if (!gbd2 || gbd2->flags.temporary) continue;

                        hs_offset = headeroffset + PTR_DIFF(&(headercopy[valid]), &(headercopy[0]));

                        headercopy[valid].flags = header[item].flags;
                        headercopy[valid].flags.flags &= 1;
                        headercopy[valid].flags.changed = 0;
                        headercopy[valid].flags.ever_changed = 0;
                        headercopy[valid].rel_hl_gbd = (GB_REL_GBDATA)getrel_GBDATA(hs_offset, gbd2);

                        /* printf("header[%i->%i].rel_hl_gbd = %li\n", item,valid,
                           headercopy[valid].rel_hl_gbd); */

                        gb_assert(headercopy[valid].rel_hl_gbd != 0);
                        valid++;
                    }


                    gbccopy.d.size          = gbccopy.d.nheader = valid;
                    gbccopy.d.headermemsize = valid;

                    headermemsize = ALIGN(valid * sizeof(*header));
                    ftwrite_aligned(headercopy, headermemsize, out);
                    free(headercopy);

                }
                else {                              // Calc new indices and size of header
                    int valid = 0;                  // no of non-temporary items
                    for (item=0; item<nitems; item++)
                    {
                        GBDATA *gbd2 = GB_HEADER_LIST_GBD(header[item]);
                        struct gbdata_offset *dof;
                        if (!gbd2 || gbd2->flags.temporary) continue;
                        dof = find_gbdata_offset(header[item].flags.key_quark, gbd2);
                        dof->index = valid;
                        valid++;
                    }
                    gb_assert((size_t)headermemsize >= valid * sizeof(*header));
                    headermemsize = ALIGN(valid * sizeof(*header));
                }
            }
            else {
                gb_assert(header==0);
                headeroffset=0;
            }

            *offset += headermemsize;
        }

        // ifs

        ifsoffset = write_IFS(GBCONTAINER_IFS(gbc),out,offset);

        // gbc

        gbdoffset = *offset;
        {
            size_t gbccopy_size;
            if (out) {
                struct gbdata_offset *dof = find_gbdata_offset(quark, (GBDATA *)gbc);
                gbccopy.index = dof->index;
                gb_assert(dof->index <= gbc->index); // very simple check

                gbccopy.rel_father = (GB_REL_CONTAINER)getrel_GBDATA(gbdoffset,(GBDATA*)GB_FATHER(gbc));

                gbccopy.ext = NULL;
                convertFlags4Save(&(gbccopy.flags),&(gbccopy.flags2),&(gbccopy.flags3));
                gbccopy.d.rel_header = (GB_REL_HLS)MAKEREL( gbdoffset+PTR_DIFF(&(gbc->d), gbc),headeroffset);
                // rel_header is relative to gbc->d !!!
                gbccopy.main_idx = main_idx;
                gbccopy.index_of_touched_one_son = 0;
                gbccopy.header_update_date = 0;
                gbccopy.rel_ifs = (GB_REL_IFS)MAKEREL(gbdoffset,ifsoffset);

                gbccopy_size = ftwrite_unaligned(&gbccopy, sizeof(gbccopy), out);
            }
            else {
                gbccopy_size = ALIGN(sizeof(gbccopy));
            }
            *offset += gbccopy_size;
        }
    }
    else        // GBDATA
    {
        int     ex = gbd->flags2.extern_data;
        GBDATA  gbdcopy = *gbd; // make copy to avoid change of mem

        if (ex) {
            long   exoffset = *offset;
            size_t ex_size;

            if (out) ex_size = ftwrite_unaligned(GB_EXTERN_DATA_DATA(gbd->info.ex), gbdcopy.info.ex.memsize, out);
            else ex_size     = ALIGN(gbdcopy.info.ex.memsize);;

            *offset                  += ex_size;
            gbdcopy.info.ex.rel_data  = (GB_REL_STRING)MAKEREL(*offset+PTR_DIFF(&(gbd->info),gbd), exoffset);
        }

        gbdoffset = *offset;

        {
            size_t gbdcopy_size;
            if (out) {
                struct gbdata_offset *dof = find_gbdata_offset(quark, gbd);
                gbdcopy.index = dof->index;
                gb_assert(dof->index <= gbd->index); // very simple check

                gbdcopy.rel_father  = (GB_REL_CONTAINER)getrel_GBDATA(gbdoffset,(GBDATA*)GB_FATHER(gbd));
                gbdcopy.ext         = NULL;
                gbdcopy.server_id   = GBTUM_MAGIC_NUMBER;
                convertFlags4Save(&(gbdcopy.flags),&(gbdcopy.flags2),NULL);
                gbdcopy.cache_index = 0;
                gbdcopy_size        = ftwrite_unaligned(&gbdcopy, sizeof(gbdcopy), out);
            }
            else {
                gbdcopy_size = ALIGN(sizeof(gbdcopy));
            }
            *offset += gbdcopy_size;
        }
    }

    return gbdoffset;
}

static long writeGbdByKey(GB_MAIN_TYPE *Main, gbdByKey *gbk, FILE *out, GB_MAIN_IDX main_idx)
{
    int idx;
    int idx2;
    long offset = ALIGN(sizeof(struct gb_map_header));

    for (idx=0; idx<Main->keycnt; idx++)
    {
        for (idx2=0; idx2<gbk[idx].cnt; idx2++)
        {
#if defined(ASSERTION_USED)
            long gboffset =
#endif // ASSERTION_USED
                write_GBDATA(Main,gbk[idx].gbdoff[idx2].gbd,idx,out, &offset,main_idx);
            gb_assert(gboffset == gbk[idx].gbdoff[idx2].offset);
        }
    }

    return offset;
}

static long calcGbdOffsets(GB_MAIN_TYPE *Main, gbdByKey *gbk)
{
    int idx;
    int idx2;
    long offset = ALIGN(sizeof(struct gb_map_header));

    for (idx=0; idx<Main->keycnt; idx++)
    {
        for (idx2=0; idx2<gbk[idx].cnt; idx2++)
        {
            gbk[idx].gbdoff[idx2].offset =
                write_GBDATA(Main,gbk[idx].gbdoff[idx2].gbd,idx, NULL, &offset,0);
        }
    }

    return offset;
}

/* ********************************************************
   handle gbdByKey
   ******************************************************** */

static void scanGbdByKey(GB_MAIN_TYPE *Main, GBDATA *gbd, gbdByKey *gbk)
{
    GBQUARK quark;

    if (gbd->flags.temporary) return;

    if (GB_TYPE(gbd) == GB_DB)  // CONTAINER
    {
        int         idx;
        GBCONTAINER     *gbc = (GBCONTAINER *)gbd;
        GBDATA      *gbd2;

        for (idx=0; idx < gbc->d.nheader; idx++)
            if ((gbd2=GBCONTAINER_ELEM(gbc,idx))!=NULL)
                scanGbdByKey(Main,gbd2,gbk);
    }

    quark = GB_KEY_QUARK(gbd);

#if defined(DEBUG)
    if (quark == 0) {
        printf("KeyQuark==0 found:\n");
        GB_dump_db_path(gbd);
    }
#endif // DEBUG
    gb_assert(gbk[quark].gbdoff!=0);

    gbk[quark].gbdoff[ gbk[quark].cnt ].gbd = gbd;
    gbk[quark].gbdoff[ gbk[quark].cnt ].offset = -1; // -1 is impossible as offset in file
    gbk[quark].cnt++;
}

static gbdByKey *createGbdByKey(GB_MAIN_TYPE *Main)
{
    int       idx;
    gbdByKey *gbk = (gbdByKey*)GB_calloc(Main->keycnt, sizeof(*gbk));

    if (!gbk) goto err1;

    for (idx=0; idx<Main->keycnt; idx++) {
        gbk[idx].cnt = 0;

        if (Main->keys[idx].key && Main->keys[idx].nref>0) {
            gbk[idx].gbdoff  = (struct gbdata_offset *) GB_calloc((size_t)Main->keys[idx].nref, sizeof(*(gbk[idx].gbdoff)));
            if (!gbk[idx].gbdoff) goto err2;
        }
    }

    gbk[0].gbdoff = (struct gbdata_offset *)GB_calloc(1,sizeof(*(gbk[0].gbdoff))); // @@@ FIXME : this is maybe allocated twice (5 lines above and here), maybe idx == 0 is special ?

    scanGbdByKey(Main,(GBDATA*)Main->data,gbk);

    for (idx=0; idx<Main->keycnt; idx++)
        if (gbk[idx].cnt)
            sort_gbdata_offsets(gbk[idx].gbdoff,gbk[idx].cnt);

    return gbk;

    // error handling:

 err2:  while (idx>=0)
 {
     free(gbk[idx].gbdoff);
     idx--;
 }
    free(gbk);
 err1:  GB_memerr();
    return NULL;
}

static void freeGbdByKey(GB_MAIN_TYPE *Main, gbdByKey *gbk)
{
    int idx;

    for (idx=0; idx<Main->keycnt; idx++) free(gbk[idx].gbdoff);
    free(gbk);
}

/* ********************************************************
   save
   ******************************************************** */

int gb_save_mapfile(GB_MAIN_TYPE *Main, GB_CSTR path)
{
    struct gb_map_header mheader;
    FILE *out;
    long calcOffset,
        writeOffset;
    GB_MAIN_IDX main_idx;
    GB_CSTR opath = gb_overwriteName(path);

    gb_gbk = createGbdByKey(Main);
    if (!gb_gbk) goto error;

    calcOffset = calcGbdOffsets(Main,gb_gbk);

    // write file

    out = fopen(opath, "w");
    writeError = out==NULL;     // global flag

    gb_assert(ADMAP_ID_LEN <= strlen(ADMAP_ID));
    memset(&mheader,0,sizeof(mheader));
    strcpy(mheader.mapfileID,ADMAP_ID); // header

    mheader.version = ADMAP_VERSION;
    mheader.byte_order = ADMAP_BYTE_ORDER;
    main_idx  = gb_make_main_idx(Main); // Generate a new main idx
    mheader.main_idx = main_idx;

    mheader.main_data_offset = getrel_GBDATA(1,(GBDATA*)Main->data)+1;

    ftwrite_unaligned(&mheader, sizeof(mheader), out);

    gb_assert(GB_FATHER(Main->data)==Main->dummy_father);
    SET_GB_FATHER(Main->data,NULL);
    writeOffset = writeGbdByKey(Main,gb_gbk,out,main_idx);
    SET_GB_FATHER(Main->data,Main->dummy_father);

    gb_assert(calcOffset==writeOffset);

    freeGbdByKey(Main,gb_gbk);
    gb_gbk = NULL;

    fclose(out);
    if (!writeError) return 0;  // no error

 error:

    GB_export_errorf("Error while saving FastLoad-File '%s'", opath);
    GB_unlink_or_warn(opath, NULL);

    return -1;
}

int gb_is_valid_mapfile(const char *path, struct gb_map_header *mheader, int verbose) {
    /* Test whether mapfile is valid
     * returns
     *     -1   no map file found / MEMORY_TEST (don't use mapfile)
     *      0   mapfile error
     *      1   mapfile ok
     */

#if ( MEMORY_TEST == 1)
    // Don't map anything in memory debug mode
    return -1;
#else
    FILE *in = fopen(path,"r");
    if (in) {
        const char *error_form = 0;

        if (verbose) printf("ARB: Opening FastLoad File '%s' ...\n",path);
        fread((char *)mheader, sizeof(*mheader),1,in);
        fclose(in);

        if (strcmp(mheader->mapfileID,ADMAP_ID)!=0)     error_form = "'%s' is not a ARB-FastLoad-File";
        else if (mheader->version!=ADMAP_VERSION)       error_form = "FastLoad-File '%s' has wrong version";
        else if (mheader->byte_order!=ADMAP_BYTE_ORDER) error_form = "FastLoad-File '%s' has wrong byte order";

        if (error_form) {
            GB_export_errorf(error_form, path);
            GB_print_error();
            return 0;
        }

        return 1;
    }
    return -1;
#endif
}

/* The module admalloc.c must be able to determine whether a memory block
 * is inside the mapped file. So we store the location of the mapped file in
 * the following variables: 
 */
static char *fileMappedTo[GB_MAX_MAPPED_FILES];
static long  fileLen[GB_MAX_MAPPED_FILES];
static int   mappedFiles = 0;

GBDATA *gb_map_mapfile(const char *path)
{
    struct gb_map_header mheader;

    if (gb_is_valid_mapfile(path, &mheader, 1)>0)
    {
        char *mapped;
        mapped = GB_map_file(path, 1);

        if (mapped)
        {
            fileMappedTo[mappedFiles] = mapped;
            fileLen[mappedFiles++] = GB_size_of_file(path);
            gb_assert(mappedFiles<=GB_MAX_MAPPED_FILES);

            return (GBDATA*)(mapped+mheader.main_data_offset);
        }
    }

    return NULL;
}

int gb_isMappedMemory(char *mem)
{
    int file = 0;

    while (file<mappedFiles)
    {
        if (mem>=fileMappedTo[file] &&
            mem<(fileMappedTo[file]+fileLen[file])) return 1;
        file++;
    }

    return 0;
}

