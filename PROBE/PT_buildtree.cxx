#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <unistd.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"

POS_TREE *build_pos_tree (POS_TREE *pt, int anfangs_pos, int apos, int RNS_nr, unsigned int end)
{
    static POS_TREE       *pthelp, *pt_next;
    unsigned int i,j;
    int          height = 0, anfangs_apos_ref, anfangs_rpos_ref, RNS_nr_ref;
    pthelp = pt;
    i = anfangs_pos;
    while (PT_read_type(pthelp) == PT_NT_NODE) {    /* now we got an inner node */
        if ((pt_next = PT_read_son_stage_1(psg.ptmain,pthelp, (PT_BASES)psg.data[RNS_nr].data[i])) == NULL) {
                            // there is no son of that type -> simply add the new son to that path //
            if (pthelp == pt) { // now we create a new root structure (size will change)
                PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
                return pthelp;  // return the new root
            } else {
                PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
                return pt;  // return the old root
            }
        } else {            // go down the tree
            pthelp = pt_next;
            height++;
            i++;
            if (i >= end) {     // end of sequence reached -> change node to chain and add
                        // should never be reached, because of the terminal symbol
                        // of each sequence
                if (PT_read_type(pthelp) == PT_NT_CHAIN)
                    pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr,apos,anfangs_pos);
                // if type == node then forget it
                return pt;
            }
        }
    }
            // type == leaf or chain
    if (PT_read_type(pthelp) == PT_NT_CHAIN) {      // old chain reached
        pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr, apos, anfangs_pos);
        return pt;
    }
    anfangs_rpos_ref = PT_read_rpos(psg.ptmain,pthelp); // change leave to node and create two sons
    anfangs_apos_ref = PT_read_apos(psg.ptmain,pthelp);
    RNS_nr_ref = PT_read_name(psg.ptmain,pthelp);
    j = anfangs_rpos_ref + height;

    while (psg.data[RNS_nr].data[i] == psg.data[RNS_nr_ref].data[j]) {  /* creates nodes until sequences are different */
                                        // type != nt_node
        if (PT_read_type(pthelp) == PT_NT_CHAIN) {          /* break */
            pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr, apos, anfangs_pos);
            return pt;
        }
        if (height >= PT_POS_TREE_HEIGHT) {
            if (PT_read_type(pthelp) == PT_NT_LEAF) {
                pthelp = PT_leaf_to_chain(psg.ptmain,pthelp);
            }
            pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr, apos, anfangs_pos);
            return pt;
        }
        if ( ((i + 1)>= end) && (j + 1 >= (unsigned)(psg.data[RNS_nr_ref].size))) { /* end of both sequences */
            return pt;
        }
        pthelp = PT_change_leaf_to_node(psg.ptmain,pthelp); /* change tip to node and append two new leafs */
        if (i + 1 >= end) {                 /* end of source sequence reached   */
            pthelp = PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr_ref].data[j],
                    anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
            return pt;
        }
        if (j + 1 >= (unsigned)(psg.data[RNS_nr_ref].size)) {       /* end of reference sequence */
            pthelp = PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
            return pt;
        }
        pthelp = PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
                    // dummy leaf just to create a new node; may become a chain
        i++;
        j++;
        height++;
    }
    if (height >= PT_POS_TREE_HEIGHT) {
        if (PT_read_type(pthelp) == PT_NT_LEAF)
            pthelp = PT_leaf_to_chain(psg.ptmain,pthelp);
        pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr, apos, anfangs_pos);
        return pt;
    }
    if (PT_read_type(pthelp) == PT_NT_CHAIN) {
        pthelp = PT_add_to_chain(psg.ptmain,pthelp, RNS_nr, apos, anfangs_pos);
    } else {
        pthelp = PT_change_leaf_to_node(psg.ptmain,pthelp); /* Blatt loeschen */
        PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);  /* zwei neue Blaetter */
        PT_create_leaf(psg.ptmain,&pthelp, (PT_BASES)psg.data[RNS_nr_ref].data[j], anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
    }
    return pt;
}
/* get the absolute alignment position */
static GB_INLINE void get_abs_align_pos(char *seq, int &pos)
{
    register int c;
    int q_exists = 0;
    while ( pos){
        c = seq[pos];
        if ( c== '.'){
            q_exists = 1; pos--; continue;
        }
        if (c == '-'){
            pos--; continue;
        }
        break;
    }
    pos += q_exists;
}



long PTD_save_partial_tree(FILE *out,PTM2 *ptmain,POS_TREE * node,char *partstring,int partsize,long pos, long *ppos)
{
    if (partsize) {
        POS_TREE *son = PT_read_son(ptmain,node,(enum PT_bases_enum)partstring[0]);
        if (son) {
            pos = PTD_save_partial_tree(out,ptmain,son,partstring+1,partsize-1,pos,ppos);
        }
    }else{
        PTD_clear_fathers(ptmain,node);
        long r_pos;
        int blocked;
        blocked = 1;
        while (blocked) {
            blocked = 0;
            printf("*************** pass %li *************\n",pos);
            r_pos = PTD_write_leafs_to_disk(out,ptmain,node,pos,ppos,&blocked);
            if (r_pos > pos) pos = r_pos;
        }
    }
    return pos;
}

inline int ptd_string_shorter_than(const char *s, int len) {
    int i;
    for (i=0;i<len;i++){
        if (!s[i]) {
            return 1;
        }
    }
    return 0;
}

/*initialize tree and call the build pos tree procedure*/
void
enter_stage_1_build_tree(PT_main * main,char *tname)
{
    main = main;
    POS_TREE       *pt = NULL;
    int             i, j, abs_align_pos;
    static  int psize;
    char        *align_abs;
    pt = 0;
    char *t2name;

    FILE *out;                      /* write params */
    long    pos;
    long    last_obj;

    if (unlink(tname) ) {
        if (GB_size_of_file(tname)>=0) {
            fprintf(stderr,"Cannot remove %s\n",tname);
            exit(EXIT_FAILURE);
        }
    }

    t2name = (char *)calloc(sizeof(char),strlen(tname) + 2);
    sprintf(t2name,"%s%%",tname);
    out = fopen(t2name,"w");                /* open file for output */
    if (!out) {
        fprintf(stderr,"Cannot open %s\n",t2name);
        exit(EXIT_FAILURE);
    }
    GB_set_mode_of_file(t2name,0666);
    putc(0,out);        /* disable zero father */
    pos = 1;

    psg.ptmain = PT_init(PT_B_MAX);
    psg.ptmain->stage1 = 1;             /* enter stage 1 */

    pt = PT_create_leaf(psg.ptmain,0,PT_N,0,0,0);       /* create main node */
    pt = PT_change_leaf_to_node(psg.ptmain,pt);
    psg.stat.cut_offs = 0;                  /* statistic information */
    GB_begin_transaction(psg.gb_main);

    char partstring[256];
    int  partsize = 0;
    int  pass0    = 0;
    int  passes   = 1;

    {
        ulong total_size = psg.char_count;

        printf("Overall bases: %li\n", total_size);

        while (1) {
            ulong estimated_kb = (total_size/1024)*35; /* value by try and error; 35 bytes per base*/
            printf("Estimated memory usage for %i passes: %lu k\n", passes, estimated_kb);

            if (estimated_kb <= physical_memory) break;

            total_size /= 4;
            partsize ++;
            passes     *= 5;
        }

        //     while ( ulong(total_size*35/1024) > physical_memory) {  /* value by try and error; 35 bytes per base*/
        //         total_size /= 4;
        //         partsize ++;
        //         passes *=5;
        //     }
    }

    printf ("Tree Build: %li bases in %i passes\n",psg.char_count, passes);

    PT_init_base_string_counter(partstring,PT_N,partsize);
    while (!PT_base_string_counter_eof(partstring)) {
        ++pass0;
        printf("\n Starting pass %i(%i)\n", pass0, passes);

        for (i = 0; i < psg.data_count; i++) {
            align_abs = probe_read_alignment(i, &psize);

            if ((i % 1000) == 0) printf("%i(%i)\t cutoffs:%i\n", i, psg.data_count, psg.stat.cut_offs);
            abs_align_pos = psize-1;
            for (j = psg.data[i].size - 1; j >= 0; j--, abs_align_pos--) {
                get_abs_align_pos(align_abs, abs_align_pos); // may result in neg. abs_align_pos (seems to happen if sequences are short < 214bp )
                if (abs_align_pos < 0) break; // -> in this case abort

                if ( partsize && (*partstring!= psg.data[i].data[j] || strncmp(partstring,psg.data[i].data+j,partsize)) ) continue;
                if (ptd_string_shorter_than(psg.data[i].data+j,9)) continue;

                pt = build_pos_tree(pt, j, abs_align_pos, i, psg.data[i].size);
            }
            free(align_abs);
        }
        pos = PTD_save_partial_tree(out, psg.ptmain, pt, partstring, partsize, pos, &last_obj);
#ifdef PTM_DEBUG
        PTM_debug_mem();
        PTD_debug_nodes();
#endif
        PT_inc_base_string_count(partstring,PT_N,PT_B_MAX,partsize);
    }
    if (partsize){
        pos = PTD_save_partial_tree(out, psg.ptmain, pt, 0, 0, pos, &last_obj);
#ifdef PTM_DEBUG
        PTM_debug_mem();
        PTD_debug_nodes();
#endif
    }
    PTD_put_int(out,last_obj);

    GB_commit_transaction(psg.gb_main);
    fclose(out);
    if (GB_rename_file(t2name,tname) ) {
        GB_print_error();
    }

    if (GB_set_mode_of_file(tname,00666)) {
        GB_print_error();
    }
    psg.pt = pt;
}

/* load tree from disk */
void enter_stage_3_load_tree(PT_main * main,char *tname)
{
    main = main;
    psg.ptmain = PT_init(PT_B_MAX);
    psg.ptmain->stage3 = 1;             /* enter stage 3 */

    FILE *in;
    long size = GB_size_of_file(tname);
    if (size<0) {
        fprintf(stderr,"PT_SERVER: Error while opening file %s\n",tname);
        exit(EXIT_FAILURE);
    }

    printf ("PT_SERVER: Reading Tree %s of size %li from disk\n",tname,size);
    in = fopen(tname,"r");
    if (!in) {
        fprintf(stderr,"PT_SERVER: Error while opening file %s\n",tname);
        exit(EXIT_FAILURE);
    }
    PTD_read_leafs_from_disk(tname,psg.ptmain,&psg.pt);
    fclose(in);
}

#define DEBUG_MAX_CHAIN_SIZE 1000
#define DEBUG_TREE_DEEP PT_POS_TREE_HEIGHT+50

struct PT_debug_struct {
    int chainsizes[DEBUG_MAX_CHAIN_SIZE][DEBUG_TREE_DEEP];
    int chainsizes2[DEBUG_MAX_CHAIN_SIZE];
    int splits[DEBUG_TREE_DEEP][PT_B_MAX];
    int nodes[DEBUG_TREE_DEEP];
    int tips[DEBUG_TREE_DEEP];
    int chains[DEBUG_TREE_DEEP];
    int chaincount;
    } *ptds;

int PT_chain_debug(int name, int apos, int rpos, long height)
    {
    psg.height++;
    name = name;
    apos = apos;
    rpos = rpos;
    height = height;
    return 0;
}
void PT_analyse_tree(POS_TREE *pt,int height)
    {
    PT_NODE_TYPE type;
    POS_TREE *pt_help;
    int i;
    int basecnt;
    type = PT_read_type(pt);
    switch (type) {
        case PT_NT_NODE:
            ptds->nodes[height]++;
            basecnt = 0;
            for (i=PT_QU; i<PT_B_MAX; i++) {
                if ((pt_help = PT_read_son(psg.ptmain,pt, (PT_BASES)i)))
                {
                    basecnt++;
                    PT_analyse_tree(pt_help,height+1);
                };
            }
            ptds->splits[height][basecnt]++;
            break;
        case PT_NT_LEAF:
            ptds->tips[height]++;
            break;
        default:
            ptds->chains[height]++;
            psg.height = 0;
            PT_read_chain(psg.ptmain,pt, PT_chain_debug, height);
            if (psg.height >= DEBUG_MAX_CHAIN_SIZE) psg.height = DEBUG_MAX_CHAIN_SIZE;
            ptds->chainsizes[psg.height][height]++;
            ptds->chainsizes2[psg.height]++;
            ptds->chaincount++;
            if (ptds->chaincount<20) {
                printf("\n\n\n\n");
                PT_read_chain(psg.ptmain,pt, PTD_chain_print, 0);
            }
            break;
    };
}

void PT_debug_tree(void)    // show various debug information about the tree
    {
    ptds = (struct PT_debug_struct *)calloc(sizeof(struct PT_debug_struct),1);
    PT_analyse_tree(psg.pt,0);
    int i,j,k;
    int sum = 0;            // sum of chains
    for (i=0;i<DEBUG_TREE_DEEP;i++ ) {
        k = ptds->nodes[i];
        if (k){
            sum += k;
            printf("nodes at deep %i: %i        sum %i\t\t",i,k,sum);
            for (j=0;j<PT_B_MAX; j++) {
                k =     ptds->splits[i][j];
                printf("%i:%i\t",j,k);
            }
            printf("\n");
        }
    }
    sum = 0;
    for (i=0;i<DEBUG_TREE_DEEP;i++ ) {
        k = ptds->tips[i];
        sum += k;
        if (k)  printf("tips at deep %i: %i     sum %i\n",i,k,sum);
    }
    sum = 0;
    for (i=0;i<DEBUG_TREE_DEEP;i++ ) {
        k = ptds->chains[i];
        if (k)  {
            sum += k;
            printf("chains at deep %i: %i       sum %i\n",i,k,sum);
        }
    }
    sum = 0;
    int sume = 0;           // sum of chain entries
    for (i=0;i<DEBUG_MAX_CHAIN_SIZE;i++ ) {
            k =     ptds->chainsizes2[i];
            sum += k;
            sume += i*k;
            if (k) printf("chain of size %i occur %i    sc %i   sce %i\n",i,k,sum,sume);
    }
#if 0
    sum = 0;
    for (i=0;i<DEBUG_MAX_CHAIN_SIZE;i++ ) {
        for (j=0;j<DEBUG_TREE_DEEP;j++ ) {
            k =     ptds->chainsizes[i][j];
            sum += k;
            if (k) printf("chain of size %i at deep %i occur %i     sum:%i\n",i,j,k,sum);
        }
    }
#endif
}
