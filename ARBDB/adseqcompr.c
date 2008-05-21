#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adlocal.h>
#include <arbdbt.h>

/* -------------------------------------------------------------------------------- */

#define MAX_SEQUENCE_PER_MASTER 50 /* was 18 till May 2008 */

#if defined(DEBUG)
/* dont do optimize, only create tree and save to DB */
/* #define SAVE_COMPRESSION_TREE_TO_DB */
#endif /* DEBUG */

/* -------------------------------------------------------------------------------- */

typedef struct gb_seq_compr_tree {
#ifdef FAKE_VTAB_PTR
    virtualTable *dummy_virtual; /* simulate pointer to virtual-table used in AP_tree */
#endif
    GBT_TREE_ELEMENTS(struct gb_seq_compr_tree);

    int index;                  /* master(inner nodes) or sequence(leaf nodes) index */
    int sons;                   /* sons with sequence or masters (in subtree) */
} GB_CTREE;

typedef struct {
    int            len;
    char           used[256];
    unsigned char *con[256];
} GB_Consensus;

typedef struct {
    GBDATA *gbd;
    int     master;
} GB_Sequence;

typedef struct {
    GBDATA *gbd;
    int     master;
} GB_Master;

/* -------------------------------------------------------------------------------- */

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
    int            i;
    int            li;
    int            c;
    unsigned char *s;
    int            last;
    unsigned char *p;
    int            eq_count;
    const int      max_priority = 255/MAX_SEQUENCE_PER_MASTER; /* No overflow possible */

    ad_assert(max_priority >= 1); 

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
    int pos;
    unsigned char *s;
    unsigned char *max = (unsigned char *)GB_calloc(sizeof(char), gcon->len);
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
        if (ctree->index >= 0) {
            GB_CSTR data = GB_read_char_pntr(seqs[ctree->index].gbd);
            long    len  = GB_read_string_count(seqs[ctree->index].gbd);
            g_b_Consensus_add(gcon,(unsigned char *)data,len);
        }
    }
    else if (ctree->index<0){
        g_b_put_sequences_in_container(ctree->leftson,seqs,masters,gcon);
        g_b_put_sequences_in_container(ctree->rightson,seqs,masters,gcon);
    }
    else {
        GB_CSTR data = GB_read_char_pntr(masters[ctree->index]->gbd);
        long    len  = GB_read_string_count(masters[ctree->index]->gbd);
        g_b_Consensus_add(gcon,(unsigned char *)data,len);
    }
}

void g_b_create_master(GB_CTREE *node, GB_Sequence *seqs, GB_Master **masters,
                       int masterCount, int *builtMasters, int my_master,
                       const char *ali_name, long seq_len, int *aborted)
{
    if (*aborted) {
        return;
    }
    
    if (node->is_leaf){
        if (node->index >= 0) {
            GBDATA *gb_data = GBT_read_sequence(node->gb_node, ali_name);

            seqs[node->index].gbd    = gb_data;
            seqs[node->index].master = my_master;
        }
    }
    else {
        if (node->index>=0){
            masters[node->index]->master = my_master;
            my_master = node->index;
        }
        g_b_create_master(node->leftson,seqs,masters,masterCount,builtMasters,my_master,ali_name,seq_len, aborted);
        g_b_create_master(node->rightson,seqs,masters,masterCount,builtMasters,my_master,ali_name,seq_len, aborted);
        if (node->index>=0 && !*aborted) { /* build me */
            char         *data;
            GB_Consensus *gcon = g_b_new_Consensus(seq_len);

            g_b_put_sequences_in_container(node->leftson,seqs,masters,gcon);
            g_b_put_sequences_in_container(node->rightson,seqs,masters,gcon);

            data = g_b_Consensus_get_sequence(gcon);
            
            GB_write_string(masters[node->index]->gbd,data);
            GB_write_security_write(masters[node->index]->gbd,7);

            g_b_delete_Consensus(gcon);
            free(data);

            (*builtMasters)++;
            *aborted |= GB_status(*builtMasters/(double)masterCount);
        }
    }
}

/* ------------------------------------- */
/*      distribute master sequences      */
/* ------------------------------------- */

static void subtract_sons_from_tree(GB_CTREE *node, int subtract) {
    while (node) {
        node->sons -= subtract;
        node        = node->father;
    }
}

static int set_masters_with_sons(GB_CTREE *node, int wantedSons, int *mcount) {
    if (!node->is_leaf) {
        if (node->sons == wantedSons) {
            /* insert new master */
            ad_assert(node->index == -1);
            node->index = *mcount;
            (*mcount)++;

            subtract_sons_from_tree(node->father, node->sons-1);
            node->sons = 1;
        }
        else if (node->sons>wantedSons) {
            int lMax = set_masters_with_sons(node->leftson,  wantedSons, mcount);
            int rMax = set_masters_with_sons(node->rightson, wantedSons, mcount);

            int maxSons = lMax<rMax ? rMax : lMax;
            if (node->sons <= MAX_SEQUENCE_PER_MASTER && node->sons>maxSons) {
                maxSons = node->sons;
            }
            return maxSons;
        }
    }
    return node->sons <= MAX_SEQUENCE_PER_MASTER ? node->sons : 0;
}

static int maxCompressionSteps(GB_CTREE *node) {
    if (node->is_leaf) {
        return 0;
    }

    int left  = maxCompressionSteps(node->leftson);
    int right = maxCompressionSteps(node->rightson);

#if defined(SAVE_COMPRESSION_TREE_TO_DB)
    if (node->name) {
        free(node->name);
        node->name = 0;
    }

    if (node->index2 != -1) {
        node->name = GBS_global_string_copy("master_%03i", node->index2);
    }
#endif /* SAVE_COMPRESSION_TREE_TO_DB */

    return (left>right ? left : right) + (node->index == -1 ? 0 : 1);
}

static int init_indices_and_count_sons(GB_CTREE *node, int *scount, const char *ali_name) {
    if (node->is_leaf){
        if (node->gb_node ==0 || !GBT_read_sequence(node->gb_node,(char *)ali_name)) {
            node->index = -1;
            node->sons  = 0;
        }
        else {
            node->index = *scount;
            node->sons  = 1;
            (*scount)++;
        }
    }
    else {
        node->index = -1;
        node->sons  = 
            init_indices_and_count_sons(node->leftson, scount, ali_name) +
            init_indices_and_count_sons(node->rightson, scount, ali_name);
    }
    return node->sons;
}

static void distribute_masters(GB_CTREE *tree, int *mcount, int *max_masters) {
    int wantedSons = MAX_SEQUENCE_PER_MASTER;
    while (wantedSons >= 2) {
        int maxSons = set_masters_with_sons(tree, wantedSons, mcount);
        wantedSons = maxSons;
    }
    ad_assert(tree->sons == 1);

    ad_assert(tree->index != -1);
    *max_masters = maxCompressionSteps(tree);
}

/* -------------------------------------------------------------------------------- */

static GB_INLINE long g_b_read_number2(const unsigned char **s) {
    unsigned int c0,c1,c2,c3,c4;
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
    int j;
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
    int            rest = 0;
    unsigned char *d;
    int            i,cs,cm;
    int            last;
    long           len  = seq_len;
    
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
                                     GBQUARK q, const char *seq, int seq_len, long *memsize)
{
    long  size;
    char *is  = gb_compress_seq_by_master(master,master_len,master_index,q,seq,seq_len,&size,GB_COMPRESSION_LAST);
    char *res = gb_compress_data(gbd,0,is,size,memsize, ~(GB_COMPRESSION_DICTIONARY|GB_COMPRESSION_SORTBYTES|GB_COMPRESSION_RUNLENGTH),GB_TRUE);
    return res;
}

static GB_ERROR compress_sequence_tree(GBDATA *gb_main, GB_CTREE *tree, const char *ali_name){
    GB_ERROR error      = 0;
    long     ali_len    = GBT_get_alignment_len(gb_main, ali_name);
    int      main_clock = GB_read_clock(gb_main);

    if (ali_len<0){
        error = GB_export_error("Alignment '%s' is not a valid alignment (len=%li)", ali_name, ali_len);
    }
    else {
        int leafcount = g_b_count_leafs(tree);
        if (!leafcount) {
            error = GB_export_error("Tree contains no sequences with data in '%s'", ali_name);
        }
        else {
            /* Distribute masters in tree */
            int mastercount   = 0;
            int max_compSteps = 0; // in one branch
            int seqcount      = 0;

            GB_status2("Create master sequences");
            GB_status(0);

            init_indices_and_count_sons(tree, &seqcount, ali_name);
            distribute_masters(tree, &mastercount, &max_compSteps);

#if defined(SAVE_COMPRESSION_TREE_TO_DB)
            {
                error = GBT_link_tree((GBT_TREE*)tree, gb_main, 0, NULL, NULL);
                if (!error) error = GBT_write_tree(gb_main,0,"tree_compression_new",(GBT_TREE*)tree);
                GB_information("Only generated compression tree (do NOT save DB anymore)");
                return error;
            }
#endif /* SAVE_COMPRESSION_TREE_TO_DB */

            // detect degenerated trees
            {
                int min_masters   = ((seqcount-1)/MAX_SEQUENCE_PER_MASTER)+1;
                int min_compSteps = 1;
                {
                    int m = min_masters;
                    while (m>1) {
                        m            = ((m-1)/MAX_SEQUENCE_PER_MASTER)+1;
                        min_masters += m;
                        min_compSteps++;
                    }
                }

                int acceptable_masters   = (3*min_masters)/2; // accept 50% overhead
                int acceptable_compSteps = 11*min_compSteps; // accept 1000% overhead

                if (mastercount>acceptable_masters || max_compSteps>acceptable_compSteps) {
                    GB_warning("Tree is ill-suited for compression (cause of deep branches)\n"
                               "                    Used tree   Optimal tree   Overhead\n"
                               "Compression steps       %5i          %5i      %4i%% (speed)\n"
                               "Master sequences        %5i          %5i      %4i%% (size)\n"
                               "If you like to restart with a better tree,\n"
                               "press 'Abort' to stop compression",
                               max_compSteps, min_compSteps, (100*max_compSteps)/min_compSteps-100,
                               mastercount, min_masters, (100*mastercount)/min_masters-100);
                }
            }

            ad_assert(mastercount>0);

            {
                GBDATA              *gb_master_ali     = 0;
                GBDATA              *old_gb_master_ali = 0;
                GB_Sequence         *seqs              = 0;
                GB_MAIN_TYPE        *Main              = GB_MAIN(gb_main);
                GBQUARK              ali_quark         = gb_key_2_quark(Main,ali_name);
                unsigned long long   sumorg            = 0;
                unsigned long long   sumold            = 0;
                unsigned long long   sumnew            = 0;
                GB_Master          **masters           = (GB_Master **)GB_calloc(sizeof(*masters),leafcount);
                int                  si;

                {
                    char *masterfoldername = GBS_global_string_copy("%s/@master_data/@%s",GB_SYSTEM_FOLDER,ali_name);
                    old_gb_master_ali      = GB_search(gb_main, masterfoldername,GB_FIND);
                    free(masterfoldername);
                }

                // create masters
                if (!error) {
                    {
                        char   *master_data_name = GBS_global_string_copy("%s/@master_data",GB_SYSTEM_FOLDER);
                        char   *master_name      = GBS_global_string_copy("@%s",ali_name);
                        GBDATA *gb_master_data   = gb_search(gb_main, master_data_name,GB_CREATE_CONTAINER,1);

                        /* create a master container, the old is deleted as soon as all sequences are compressed by the new method*/
                        gb_master_ali = gb_create_container(gb_master_data, master_name);
                        GB_write_security_delete(gb_master_ali,7);
                
                        free(master_name);
                        free(master_data_name);
                    }
                    for (si = 0; si<mastercount; si++) {
                        masters[si]      = (GB_Master *)GB_calloc(sizeof(GB_Master),1);
                        masters[si]->gbd = gb_create(gb_master_ali,"@master",GB_STRING);
                    }
                    seqs = (GB_Sequence *)GB_calloc(sizeof(*seqs),leafcount);
                    {
                        int builtMasters = 0;
                        int aborted      = 0;
                        GB_status2("Building %i master sequences", mastercount);
                        g_b_create_master(tree,seqs,masters,mastercount,&builtMasters,-1,ali_name,ali_len, &aborted);
                        if (aborted) error = "User abort";
                    }
                }

                /* Compress sequences in tree */
                if (!error) {
                    GB_status2("Compressing %i sequences in tree", seqcount);
                    GB_status(0);

                    for (si=0;si<seqcount;si++){
                        int        mi     = seqs[si].master;
                        GB_Master *master = masters[mi];
                        GBDATA    *gbd    = seqs[si].gbd;

                        if (GB_read_clock(gbd) >= main_clock){
                            GB_warning("A Species seems to be more than once in the tree");
                        }
                        else {
                            char *seq        = GB_read_string(gbd);
                            int   seq_len    = GB_read_string_count(gbd);
                            long  sizen      = GB_read_memuse(gbd);
                            char *seqm       = GB_read_string(master->gbd);
                            int   master_len = GB_read_string_count(master->gbd);
                            long  sizes;
                            char *ss         = gb_compress_sequence_by_master(gbd,seqm,master_len,mi,ali_quark,seq,seq_len,&sizes);

                            gb_write_compressed_pntr(gbd,ss,sizes,seq_len);
                            sizes = GB_read_memuse(gbd); // check real usage

                            sumnew += sizes;
                            sumold += sizen;
                            sumorg += seq_len;
                            
                            GB_FREE(seqm);
                            GB_FREE(seq);
                        }

                        if (GB_status((si+1)/(double)seqcount)) {
                            error = "User abort";
                            break;
                        }
                    }
                }

                /* Compress rest of sequences */
                if (!error) {
                    int pass; /* pass 1 : count species to compress, pass 2 : compress species */
                    int speciesNotInTree = 0;

                    GB_status2("Compressing sequences NOT in tree");
                    GB_status(0);

                    for (pass = 1; pass <= 2; ++pass) {
                        GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
                        GBDATA *gb_species;
                        int     count           = 0;

                        for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
                             gb_species;
                             gb_species = GBT_next_species(gb_species))
                        {
                            GBDATA *gbd = GBT_read_sequence(gb_species,ali_name);

                            if(!gbd) continue;
                            if (GB_read_clock(gbd) >= main_clock) continue; /* Compress only those which are not compressed by masters */
                            count++;
                            if (pass == 2) {
                                char *data    = GB_read_string(gbd);
                                long  seq_len = GB_read_string_count(gbd);
                                long  size    = GB_read_memuse(gbd);

                                GB_write_string(gbd,"");
                                GB_write_string(gbd,data);
                                GB_FREE(data);

                                sumold += size;
                                
                                size = GB_read_memuse(gbd);
                                sumnew += size;
                                sumorg += seq_len;

                                if (GB_status(count/(double)speciesNotInTree)) {
                                    error = "User abort";
                                    break;
                                }
                            }
                        }
                        if (pass == 1) {
                            speciesNotInTree = count;
                            if (GB_status2("Compressing %i sequences NOT in tree", speciesNotInTree)) {
                                error = "User abort";
                                break;
                            }
                        }
                    }
                }

                if (!error) {
                    GB_status2("Compressing %i master-sequences", mastercount);
                    GB_status(0);

                    /* Compress all masters */
                    for (si=0;si<mastercount;si++){
                        int mi = masters[si]->master;

                        if (mi>0) { /*  master available */
                            GBDATA *gbd = masters[si]->gbd;
                            
                            ad_assert(mi>si); /* we don't want a rekursion, because we cannot uncompress sequence compressed masters, Main->gb_master_data is wrong */

                            if (gb_read_nr(gbd) != si) { /* Check database */
                                GB_internal_error("Sequence Compression: Master Index Conflict");
                                error = GB_export_error("Sequence Compression: Master Index Conflict");
                                break;
                            }

                            {
                                GB_Master *master     = masters[mi];
                                char      *seqm       = GB_read_string(master->gbd);
                                int        master_len = GB_read_string_count(master->gbd);
                                char      *seq        = GB_read_string(gbd);
                                int        seq_len    = GB_read_string_count(gbd);
                                long       sizes;
                                char      *ss         = gb_compress_sequence_by_master(gbd,seqm,master_len,mi,ali_quark,seq,seq_len,&sizes);

                                gb_write_compressed_pntr(gbd,ss,sizes,seq_len);
                                sumnew += sizes;

                                free(seq);
                                free(seqm);
                            }

                            if (GB_status((si+1)/(double)mastercount)) {
                                error = "User abort";
                                break;
                            }
                        }
                        else { // count size of top master
                            GBDATA *gbd  = masters[si]->gbd;
                            sumnew      += GB_read_memuse(gbd); 
                        }
                    }

                    // count size of old master data
                    if (!error) {
                        GBDATA *gb_omaster;
                        for (gb_omaster = GB_entry(old_gb_master_ali, "@master");
                             gb_omaster;
                             gb_omaster = GB_nextEntry(gb_omaster))
                        {
                            long size  = GB_read_memuse(gb_omaster);
                            sumold    += size;
                        }
                    }

                    if (!error) {
                        char *sizeOrg = GB_STRDUP(GBS_readable_size(sumorg));
                        char *sizeOld = GB_STRDUP(GBS_readable_size(sumold));
                        char *sizeNew = GB_STRDUP(GBS_readable_size(sumnew));

                        GB_warning("Alignment '%s':\n"
                                   "    Uncompressed data:   %7s\n"
                                   "    Old compressed data: %7s = %6.2f%%\n"
                                   "    New compressed data: %7s = %6.2f%%",
                                   ali_name, sizeOrg,
                                   sizeOld, (100.0*sumold)/sumorg,
                                   sizeNew, (100.0*sumnew)/sumorg);

                        free(sizeNew);
                        free(sizeOld);
                        free(sizeOrg);
                    }
                }


                if (!error) {
                    if (old_gb_master_ali){
                        error = GB_delete(old_gb_master_ali);
                    }
                    Main->keys[ali_quark].gb_master_ali = gb_master_ali;
                }

                // free data
                GB_FREE(seqs);                
                for (si=0;si<mastercount;si++)GB_FREE(masters[si]);
                GB_FREE(masters);
            }
        }
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
        if (!error) error = compress_sequence_tree(gb_main,ctree,ali_name);
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

#if defined(SAVE_COMPRESSION_TREE_TO_DB)
    error = "fake error";
#endif /* SAVE_COMPRESSION_TREE_TO_DB */

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
    const signed char *source = (signed char *)s;
    char              *dest;
    const char        *m      = master;
    unsigned int       c;
    int                j;
    int                i;
    char              *buffer;

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
            if (c==0) {
                memcpy(dest, m, -j);
                dest+= -j;
                m+= -j;
            } else {
                memset(dest, c, -j);
                dest+= -j;
            	m+= -j;
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
