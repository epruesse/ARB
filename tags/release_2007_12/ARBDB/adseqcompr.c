#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */

#include <adlocal.h>
/* #include <arbdb.h> */
#include <arbdbt.h>
#include "adseqcompr.h"
#define MAX_SEQUENCE_PER_MASTER 10

double g_b_number_of_sequences_to_compress;
int g_b_counter_of_sequences_to_compress;

GB_Consensus *g_b_new_Consensus(long len){
    GB_Consensus *gcon = (GB_Consensus *)GB_calloc(sizeof(GB_Consensus),1);
    int i;
    unsigned char *data = (unsigned char *)GB_calloc(sizeof(char)*256,len);
    gcon->len = len;

    for (i=0;i<256;i++){
        gcon->con[i] = data + len*i;
    }
    return gcon;
}


void g_b_delete_Consensus(GB_Consensus *gcon){
    free((char *)gcon->con[0]);
    free((char *)gcon);
}


void g_b_Consensus_add(GB_Consensus *gcon, unsigned char *seq, long seq_len){
    register int i;
    register int li;
    register int c;
    register unsigned char *s;
    register int last;
    register unsigned char *p;
    int eq_count;
    const int max_priority = 255/MAX_SEQUENCE_PER_MASTER; /* No overflow possible */

    if (seq_len > gcon->len) seq_len = gcon->len;

    /* Search for runs  */
    s = seq;
    last = 0;
    eq_count = 0;
    for (li = i = 0; i < seq_len; i++ ){
        c = *(s++);
        if (c == last){
            continue;
        }else{
        inc_hits:
            eq_count = i-li;
            gcon->used[c] = 1;
            p = gcon->con[last];
            last = c;
            if (eq_count <= GB_RUNLENGTH_SIZE) {
                c = max_priority;
                while (li < i) p[li++] += c;
            }else{
                c = max_priority * ( GB_RUNLENGTH_SIZE ) / eq_count;
                if (c){
                    while (li < i) p[li++] += c;
                }else{
                    while (li < i) p[li++] |= 1;
                }
            }
        }
    }
    if (li<seq_len){
        c = last;
        i = seq_len;
        goto inc_hits;
    }
}

char *g_b_Consensus_get_sequence(GB_Consensus *gcon){
    register int pos;
    register unsigned char *s;
    register unsigned char *max = (unsigned char *)GB_calloc(sizeof(char), gcon->len);
    int c;
    char *seq = (char *)GB_calloc(sizeof(char),gcon->len+1);

    memset(seq,'@',gcon->len);

    for (c = 1; c<256;c++){ /* Find maximum frequency of non run */
        if (!gcon->used[c]) continue;
        s = gcon->con[c];
        for (pos= 0;pos<gcon->len;pos++){
            if (*s > max[pos]) {
                max[pos] = *s;
                seq[pos] = c;
            }
            s++;
        }
    }
    free((char *)max);
    return seq;
}


int g_b_count_leafs(GB_CTREE *node){
    if (node->is_leaf) return 1;
    node->gb_node = 0;
    return (g_b_count_leafs(node->leftson) + g_b_count_leafs(node->rightson));
}

void g_b_put_sequences_in_container(GB_CTREE *ctree,GB_Sequence *seqs,GB_Master **masters,GB_Consensus *gcon){
    if (ctree->is_leaf){
        char *data;
        long len;
        if (ctree->index<0) return;
        g_b_counter_of_sequences_to_compress++;
        GB_status(g_b_counter_of_sequences_to_compress / g_b_number_of_sequences_to_compress);
        data = GB_read_char_pntr(seqs[ctree->index].gbd);
        len = GB_read_string_count(seqs[ctree->index].gbd);
        g_b_Consensus_add(gcon,(unsigned char *)data,len);
        return;
    }
    if (ctree->index<0){
        g_b_put_sequences_in_container(ctree->leftson,seqs,masters,gcon);
        g_b_put_sequences_in_container(ctree->rightson,seqs,masters,gcon);
        return;
    }
    {
        char *data = GB_read_char_pntr(masters[ctree->index]->gbd);
        long len = GB_read_string_count(masters[ctree->index]->gbd);
        g_b_Consensus_add(gcon,(unsigned char *)data,len);
    }
}

void g_b_create_master( GB_CTREE *node,
                        GB_Sequence *seqs,
                        GB_Master **masters,
                        int my_master,
                        const char *ali_name, long seq_len){
    if (node->is_leaf){
        GBDATA *gb_data;
        if (node->index < 0) return;
        gb_data = GBT_read_sequence(node->gb_node,(char *)ali_name);
        seqs[node->index].gbd = gb_data;
        seqs[node->index].master = my_master;
        return;
    }
    if (node->index>=0){
        masters[node->index]->master = my_master;
        my_master = node->index;
    }
    g_b_create_master(node->leftson,seqs,masters,my_master,ali_name,seq_len);
    g_b_create_master(node->rightson,seqs,masters,my_master,ali_name,seq_len);
    if (node->index>=0){        /* build me */
        char *data;
        GB_Consensus *gcon = g_b_new_Consensus(seq_len);
        g_b_put_sequences_in_container(node->leftson,seqs,masters,gcon);
        g_b_put_sequences_in_container(node->rightson,seqs,masters,gcon);
        data = g_b_Consensus_get_sequence(gcon);
        GB_write_string(masters[node->index]->gbd,data);
        GB_write_security_write(masters[node->index]->gbd,7);

        g_b_delete_Consensus(gcon);
        free(data);
    }
}

int g_b_set_masters_and_set_leafs(GB_CTREE *node,
                                  GB_Sequence *seqs,    int *scount,
                                  GB_Master **masters,  int *mcount,
                                  char *ali_name,
                                  GBDATA            *master_folder ){
    int sumsons;
    if (node->is_leaf){
        if (node->gb_node ==0 || !GBT_read_sequence(node->gb_node,(char *)ali_name)){
            node->index = -1;
            return 0;
        }

        node->index = *scount;
        (*scount)++;
        return 1;
    }
    node->index = -1;
    sumsons =
        g_b_set_masters_and_set_leafs( node->leftson,   seqs,scount,masters,mcount,ali_name, master_folder) +
        g_b_set_masters_and_set_leafs( node->rightson,  seqs,scount,masters,mcount,ali_name, master_folder);
    if (sumsons < MAX_SEQUENCE_PER_MASTER && node->father){
        return sumsons;
    }
    /* Now we should create a master, there are enough sons */
    node->index = *mcount;
    masters[*mcount] = (GB_Master *)GB_calloc(sizeof(GB_Master),1);
    masters[*mcount]->gbd = gb_create(master_folder,"@master",GB_STRING);
    (*mcount)++;
    return 1;
}

static GB_INLINE long g_b_read_number2(const unsigned char **s) {
    register unsigned int c0,c1,c2,c3,c4;
    c0 = (*((*s)++));
    if (c0 & 0x80){
        c1 = (*((*s)++));
        if (c0 & 0x40) {
            c2 = (*((*s)++));
            if (c0 & 0x20) {
                c3 = (*((*s)++));
                if (c0 &0x10) {
                    c4 = (*((*s)++));
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

static GB_INLINE void g_b_put_number2(int i, unsigned char **s) {
    register int j;
    if (i< 0x80 ){ *((*s)++)=i;return; }
    if (i<0x4000) {
        j = (i>>8) | 0x80;
        *((*s)++)=j;
        *((*s)++)=i;
    }else if (i<0x200000) {
        j = (i>>16) | 0xC0;
        *((*s)++)=j;
        j = (i>>8);
        *((*s)++)=j;
        *((*s)++)=i;
    } else if (i<0x10000000) {
        j = (i>>24) | 0xE0;
        *((*s)++)=j;
        j = (i>>16);
        *((*s)++)=j;
        j = (i>>8);
        *((*s)++)=j;
        *((*s)++)=i;
    }
}


char *gb_compress_seq_by_master(const char *master,int master_len,int master_index,
                                GBQUARK q, const char *seq, long seq_len,
                                long *memsize, int old_flag){
    unsigned char *buffer;
    int rest = 0;
    register unsigned char *d;
    register int i,cs,cm;
    int last;
    long len = seq_len;
    d = buffer = (unsigned char *)GB_give_other_buffer(seq,seq_len);

    if (seq_len > master_len){
        rest = seq_len - master_len;
        len = master_len;
    }

    last = -1000;       /* Convert Sequence relative to Master */
    for( i = len; i>0; i--){
        cm = *(master++);
        cs = *(seq++);
        if (cm==cs && cs != last){
            *(d++) = 0;
            last = 1000;
        }else{
            *(d++) = cs;
            last = cs;
        }
    }
    for( i = rest; i>0; i--){
        *(d++) = *(seq++);
    }

    {               /* Append run length compression method */
        unsigned char *buffer2;
        unsigned char *dest2;
        buffer2 = dest2 = (unsigned char *)GB_give_other_buffer((char *)buffer,seq_len+100);
        *(dest2++) = GB_COMPRESSION_SEQUENCE | old_flag;

        g_b_put_number2(master_index,&dest2); /* Tags */
        g_b_put_number2(q,&dest2);

        gb_compress_equal_bytes_2((char *)buffer,seq_len,memsize,(char *)dest2); /* append runlength compressed sequences to tags */

        *memsize = *memsize + (dest2-buffer2);
        return (char *)buffer2;
    }
}

char *gb_compress_sequence_by_master(GBDATA *gbd,const char *master,int master_len,int master_index,
                                     GBQUARK q, const char *seq, int seq_len, long *memsize){
    char *is;
    char *res;
    long size;
    is = gb_compress_seq_by_master(master,master_len,master_index,q,seq,seq_len,&size,GB_COMPRESSION_LAST);
    res = gb_compress_data(gbd,0,is,size,memsize, ~(GB_COMPRESSION_DICTIONARY|GB_COMPRESSION_SORTBYTES|GB_COMPRESSION_RUNLENGTH),GB_TRUE);
    return res;
}

NOT4PERL GB_ERROR GBT_compress_sequence_tree(GBDATA *gb_main, GB_CTREE *tree, const char *ali_name){
    GB_ERROR error = 0;
    char *masterfoldername = 0;
    int leafcount = 0;
    int mastercount = 0;
    int seqcount = 0;
    int main_clock;
    GB_Sequence *seqs = 0;
    GB_Master **masters = 0;
    GBDATA *gb_master_ali;
    GBDATA *old_gb_master_ali;
    int q;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    q = gb_key_2_quark(Main,ali_name);

    do {
        long seq_len = GBT_get_alignment_len(gb_main,(char *)ali_name);
        main_clock = GB_read_clock(gb_main);
        if (seq_len<0){
            error = GB_export_error("Alignment '%s' is not a valid alignment",ali_name);
            return error;
        }
        leafcount = g_b_count_leafs(tree);
        if (!leafcount) return 0;

        g_b_number_of_sequences_to_compress = (1 + leafcount) * (2.0 + 1.0 / MAX_SEQUENCE_PER_MASTER);
        g_b_counter_of_sequences_to_compress = 0;
        seqs = (GB_Sequence *)GB_calloc(sizeof(GB_Sequence),leafcount);
        masters = (GB_Master **)GB_calloc(sizeof(void *),leafcount);
        masterfoldername = GBS_global_string_copy("%s/@master_data/@%s",GB_SYSTEM_FOLDER,ali_name);
        old_gb_master_ali = GB_search( gb_main, masterfoldername,GB_FIND);
        free(masterfoldername);
        {
            char *master_data_name = GBS_global_string_copy("%s/@master_data",GB_SYSTEM_FOLDER);
            char *master_name = GBS_global_string_copy("@%s",ali_name);
            GBDATA *gb_master_data = gb_search( gb_main, master_data_name,GB_CREATE_CONTAINER,1);
            gb_master_ali = gb_create_container( gb_master_data, master_name);       /* create a master container always,
                                                                                        the old is deleted as soon as all sequences are
                                                                                        compressed by the new method*/
            GB_write_security_delete(gb_master_ali,7);
            free(master_name);
            free(master_data_name);
        }
        GB_status(0);
        g_b_set_masters_and_set_leafs(tree,seqs,&seqcount,masters,&mastercount,(char *)ali_name,gb_master_ali);
        g_b_create_master(tree,seqs,masters,-1,ali_name,seq_len);
    } while(0);

    /* Now compress everything !!! */
    {
        long sizes =0 ;
        long sizen = 0;
        char *seq;
        int seq_len;
        int si;
        char *ss;
        long sumorg = 0;
        long sumold = 0;
        long sumnew = 0;

        /* Compress sequences in tree */
        for (si=0;si<seqcount;si++){
            int mi = seqs[si].master;
            GB_Master *master = masters[mi];
            GBDATA *gbd = seqs[si].gbd;

            char *seqm = GB_read_string(master->gbd);
            int master_len = GB_read_string_count(master->gbd);
            if (GB_read_clock(gbd) >= main_clock){
                GB_warning("A Species seems to be more than once in the tree");
                continue; /* Check for sequences which are more than once in the tree */
            }

            seq = GB_read_string(gbd);
            seq_len = GB_read_string_count(gbd);
            sizen = GB_read_memuse(gbd);

            ss = gb_compress_sequence_by_master(gbd,seqm,master_len,mi,q,seq,seq_len,&sizes);
            gb_write_compressed_pntr(gbd,ss,sizes,seq_len);

            GB_status( (++g_b_counter_of_sequences_to_compress) / g_b_number_of_sequences_to_compress);

            sumorg += seq_len;
            sumold += sizen;
            sumnew+= sizes;
            GB_FREE(seqm);
            GB_FREE(seq);
        }
        GB_FREE(seqs);
        /* Compress rest of sequences */
        {
            GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
            GBDATA *gb_species;
            for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
                 gb_species;
                 gb_species = GBT_next_species(gb_species)){
                GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
                char *data;
                if(!gb_data) continue;
                if (GB_read_clock(gb_data) >= main_clock) continue; /* Compress only those which are not compressed by masters */
                data = GB_read_string(gb_data);
                GB_write_string(gb_data,"");
                GB_write_string(gb_data,data);
                GB_FREE(data);
            }
        }

        /* Compress all masters */
        for (si=0;si<mastercount;si++){
            int mi = masters[si]->master;
            GB_Master *master;
            GBDATA *gbd;
            char *seqm;
            int master_len;

            if (mi>0) {     /*  master available */
                ad_assert(mi>si);   /* we don't want a rekursion, because we cannot uncompress sequence compressed masters, Main->gb_master_data is wrong */
                master = masters[mi];
                gbd = masters[si]->gbd;
                seqm = GB_read_string(master->gbd);
                master_len = GB_read_string_count(master->gbd);
                if (gb_read_nr(gbd) != si) { /* Check database */
                    GB_internal_error("Sequence Compression: Master Index Conflict");
                    error = GB_export_error("Sequence Compression: Master Index Conflict");
                    break;
                }

                seq = GB_read_string(gbd);
                seq_len = GB_read_string_count(gbd);
                sizen = GB_read_memuse(gbd);
                ss = gb_compress_sequence_by_master(gbd,seqm,master_len,mi,q,seq,seq_len,&sizes);
                gb_write_compressed_pntr(gbd,ss,sizes,seq_len);

                g_b_counter_of_sequences_to_compress++;
                GB_status(g_b_counter_of_sequences_to_compress / g_b_number_of_sequences_to_compress);

                sumnew+= sizes;
                free(seqm);
                free(seq);
            }
            GB_FREE(masters[si]);

        }
        GB_FREE(masters);
        GB_warning("Alignment '%s': Sum Orig %6lik Old %5lik New %5lik",ali_name,sumorg/1024,sumold/1024,sumnew/1024);

    }
    if(!error){
        if (old_gb_master_ali){
            error = GB_delete(old_gb_master_ali);
        }
        Main->keys[q].gb_master_ali = gb_master_ali;
    }
    return error;
}

/* Compress sequences, call only outside a transaction */
GB_ERROR GBT_compress_sequence_tree2(GBDATA *gb_main, const char *tree_name, const char *ali_name){
    GB_ERROR      error     = 0;
    GB_CTREE     *ctree;
    GB_UNDO_TYPE  undo_type = GB_get_requested_undo_type(gb_main);
    GB_MAIN_TYPE *Main      = GB_MAIN(gb_main);
    char         *to_free   = 0;
    
    if (Main->transaction>0){
        GB_internal_error("Internal Error: Compress Sequences called during a running transaction");
        return GB_export_error("Internal Error: Compress Sequences called during a running transaction");
    }
    
    GB_request_undo_type(gb_main,GB_UNDO_KILL);
    GB_begin_transaction(gb_main);
    GB_push_my_security(gb_main);

    if (!tree_name || !strlen(tree_name)) {
        to_free   = GBT_find_largest_tree(gb_main);
        tree_name = to_free;
    }
    ctree = (GB_CTREE *)GBT_read_tree(gb_main,(char *)tree_name,-sizeof(GB_CTREE));
    if (!ctree) {
        error = GB_export_error("Tree %s not found in database",tree_name);
    }
    else {
        error             = GBT_link_tree((GBT_TREE *)ctree, gb_main, GB_FALSE, 0, 0);
        if (!error) error = GBT_compress_sequence_tree(gb_main,ctree,ali_name);
        GBT_delete_tree((GBT_TREE *)ctree);
    }
    
    GB_pop_my_security(gb_main);
    if (error) {
        GB_abort_transaction(gb_main);
    }
    else {
        GB_commit_transaction(gb_main);
        GB_disable_quicksave(gb_main,"Database optimized");
    }
    GB_request_undo_type(gb_main,undo_type);
    
    if (to_free) free(to_free);
    return error;
}

void GBT_compression_test(void *dummy, GBDATA *gb_main) {
    GB_ERROR  error     = 0;
    char     *tree_name = 0;
    char     *ali_name;
    GBDATA   *gb_tree_name;

    GBUSE(dummy);

    GB_begin_transaction(gb_main);

    ali_name     = GBT_get_default_alignment(gb_main);
    gb_tree_name = GB_search(gb_main, "/focus/tree_name", GB_FIND);
    if (!gb_tree_name) {
        error = "Can't detect current treename";
    }
    else {
        tree_name = GB_read_string(gb_tree_name);
    }

    GB_commit_transaction(gb_main);

    if (!error) {
        printf("Recompression data in alignment '%s' using tree '%s'\n", ali_name, tree_name);
        error = GBT_compress_sequence_tree2(gb_main, tree_name, ali_name);
    }
    if (error) {
        GB_warning("%s",error);
    }

    free(tree_name);
    free(ali_name);
}



/* ******************** Decompress Sequences ******************** */
/* ******************** Decompress Sequences ******************** */
/* ******************** Decompress Sequences ******************** */
char *g_b_uncompress_single_sequence_by_master(const char *s, const char *master, long size)
{
    register const signed char *source = (signed char *)s;
    register char *dest;
    register const char *m = master;
    register unsigned int c;
    register int j;
    int i;
    char *buffer;

    dest = buffer = GB_give_other_buffer((char *)source,size);

    for (i=size;i;) {
        j = *(source++);
        if (j>0) {      /* uncompressed data block */
            if (j>i) j=i;
            i -= j;
            for (;j;j--) {
                c = *(source++);
                if (!c) c = *m;
                *(dest++) = c;
                m++;
            }
        }else{          /* equal bytes compressed */
            if (!j) break;  /* end symbol */
            if (j== -122) {
                j = *(source++) & 0xff;
                j |= ((*(source++)) <<8) &0xff00;
                j = -j;
            }
            c = *(source++);
            i += j;
            if (i<0) {
                GB_internal_error("Internal Error: Missing end in data");
                j += -i;
                i = 0;
            }
            if (c==0){
                for (;j<-4;j+=4){ /* copy 4 bytes at a time */
                    int a,b;
                    a = m[0];
                    b = m[1];   dest[0] = a;
                    a = m[2];   dest[1] = b;
                    b = m[3];   dest[2] = a;
                    dest[3] = b;
                    m+= 4;
                    dest += 4;
                }
                for (;j;j++) *(dest++) = *(m++);
            }else{
                m -= j;
                if (j<-16){     /* set multiple bytes */
                    int k;
                    c &= 0xff;  /* copy c to upper bytes */
                    c |= (c<<8);
                    c |= (c<<16);
                    j = -j;
                    if ( ((long)dest) & 1) {
                        *(dest++) = c;
                        j--;
                    }
                    if ( ((long)dest) &2){
                        *((short *)dest) = c;
                        dest += 2;
                        j-=2;
                    }

                    k = j&3;
                    j = j>>2;
                    for (;j;j--){
                        *((GB_UINT4 *)dest) = c;
                        dest += sizeof(GB_UINT4);
                    }
                    j = k;
                    for (;j;j--) *(dest++) = c;
                }else{
                    for (;j;j++) *(dest++) = c;
                }
            }
        }
    }
    *(dest++) = 0;      /* NULL of NULL terminated string */
    ad_assert(dest - buffer == size);
    return buffer;
}

char *gb_uncompress_by_sequence(GBDATA *gbd, const char *ss,long size, GB_ERROR *error){
    char *dest = 0;

    *error = 0;
    if (!GB_FATHER(gbd)) {
        *error = "Cannot uncompress this sequence: Sequence has no father";
    }
    else {
        GB_MAIN_TYPE *Main    = GB_MAIN(gbd);
        GBDATA       *gb_main = (GBDATA*)Main->data;
        char         *to_free = gb_check_out_buffer(ss); /* Remove 'ss' from memory management, otherwise load_single_key_data() may destroy it */
        int           index;
        GBQUARK       quark;

        {
            const unsigned char *s = (const unsigned char *)ss;

            index = g_b_read_number2(&s);
            quark = g_b_read_number2(&s);

            ss = (const char *)s;
        }

        if (!Main->keys[quark].gb_master_ali){
            gb_load_single_key_data(gb_main,quark);
        }

        if (!Main->keys[quark].gb_master_ali){
            *error = "Cannot uncompress this sequence: Cannot find a master sequence";
        }
        else {
            GBDATA *gb_master = gb_find_by_nr(Main->keys[quark].gb_master_ali,index);
            if (!gb_master){
                dest = (char*)GB_get_error();
            }
            else {
                long        master_size = GB_read_string_count(gb_master); // @@@ why unused ?
                const char *master      = GB_read_char_pntr(gb_master); /* make sure that this is not a buffer !!! */

                dest = g_b_uncompress_single_sequence_by_master(ss, master, size);
            }
        }
        free(to_free);
    }
    
    return dest;
}
