// =============================================================== //
//                                                                 //
//   File      : PT_buildtree.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"
#include <PT_server_prototypes.h>
#include "probe_tree.h"
#include "pt_prototypes.h"

#include <unistd.h>

POS_TREE *build_pos_tree (POS_TREE *pt, int anfangs_pos, int apos, int RNS_nr, unsigned int end)
{
    static POS_TREE       *pthelp, *pt_next;
    unsigned int i, j;
    int          height = 0, anfangs_apos_ref, anfangs_rpos_ref, RNS_nr_ref;
    pthelp = pt;
    i = anfangs_pos;
    while (PT_read_type(pthelp) == PT_NT_NODE) {    // now we got an inner node
        if ((pt_next = PT_read_son_stage_1(psg.ptmain, pthelp, (PT_BASES)psg.data[RNS_nr].data[i])) == NULL) {
                            // there is no son of that type -> simply add the new son to that path //
            if (pthelp == pt) { // now we create a new root structure (size will change)
                PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
                return pthelp;  // return the new root
            }
            else {
                PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
                return pt;  // return the old root
            }
        }
        else {            // go down the tree
            pthelp = pt_next;
            height++;
            i++;
            if (i >= end) {     // end of sequence reached -> change node to chain and add
                        // should never be reached, because of the terminal symbol
                        // of each sequence
                if (PT_read_type(pthelp) == PT_NT_CHAIN)
                    pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
                // if type == node then forget it
                return pt;
            }
        }
    }
            // type == leaf or chain
    if (PT_read_type(pthelp) == PT_NT_CHAIN) {      // old chain reached
        pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
        return pt;
    }
    anfangs_rpos_ref = PT_read_rpos(psg.ptmain, pthelp); // change leave to node and create two sons
    anfangs_apos_ref = PT_read_apos(psg.ptmain, pthelp);
    RNS_nr_ref = PT_read_name(psg.ptmain, pthelp);
    j = anfangs_rpos_ref + height;

    while (psg.data[RNS_nr].data[i] == psg.data[RNS_nr_ref].data[j]) {  // creates nodes until sequences are different
                                        // type != nt_node
        if (PT_read_type(pthelp) == PT_NT_CHAIN) {          // break
            pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
            return pt;
        }
        if (height >= PT_POS_TREE_HEIGHT) {
            if (PT_read_type(pthelp) == PT_NT_LEAF) {
                pthelp = PT_leaf_to_chain(psg.ptmain, pthelp);
            }
            pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
            return pt;
        }
        if (((i + 1) >= end) && (j + 1 >= (unsigned)(psg.data[RNS_nr_ref].size))) { // end of both sequences
            return pt;
        }
        pthelp = PT_change_leaf_to_node(psg.ptmain, pthelp); // change tip to node and append two new leafs
        if (i + 1 >= end) {                 // end of source sequence reached
            pthelp = PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr_ref].data[j],
                    anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
            return pt;
        }
        if (j + 1 >= (unsigned)(psg.data[RNS_nr_ref].size)) {       // end of reference sequence
            pthelp = PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr);
            return pt;
        }
        pthelp = PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
                    // dummy leaf just to create a new node; may become a chain
        i++;
        j++;
        height++;
    }
    if (height >= PT_POS_TREE_HEIGHT) {
        if (PT_read_type(pthelp) == PT_NT_LEAF)
            pthelp = PT_leaf_to_chain(psg.ptmain, pthelp);
        pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
        return pt;
    }
    if (PT_read_type(pthelp) == PT_NT_CHAIN) {
        pthelp = PT_add_to_chain(psg.ptmain, pthelp, RNS_nr, apos, anfangs_pos);
    }
    else {
        pthelp = PT_change_leaf_to_node(psg.ptmain, pthelp); // Blatt loeschen
        PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr].data[i], anfangs_pos, apos, RNS_nr); // zwei neue Blaetter
        PT_create_leaf(psg.ptmain, &pthelp, (PT_BASES)psg.data[RNS_nr_ref].data[j], anfangs_rpos_ref, anfangs_apos_ref, RNS_nr_ref);
    }
    return pt;
}

inline void get_abs_align_pos(char *seq, int &pos)
{
    // get the absolute alignment position
    int q_exists = 0;
    if (pos > 3) {
        pos-=3;
        while (pos > 0) {
            uint32_t c = *((uint32_t*)(seq+pos));
            if (c == 0x2E2E2E2E) {
                q_exists = 1;
                pos-=4;
                continue;
            }
            if (c == 0x2D2D2D2D) {
                pos-=4;
                continue;
            }
            break;
        }
        pos+=3;
    }
    while (pos) {
        unsigned char c = seq[pos];
        if (c == '.') {
            q_exists = 1;
            pos--;
            continue;
        }
        if (c == '-') {
            pos--;
            continue;
        }
        break;
    }
    pos+=q_exists;
}

long PTD_save_partial_tree(FILE *out, PTM2 *ptmain, POS_TREE * node, char *partstring, int partsize, long pos, long *ppos, ARB_ERROR& error) {
    if (partsize) {
        POS_TREE *son = PT_read_son(ptmain, node, (enum PT_bases_enum)partstring[0]);
        if (son) {
            pos = PTD_save_partial_tree(out, ptmain, son, partstring+1, partsize-1, pos, ppos, error);
        }
    }
    else {
        PTD_clear_fathers(ptmain, node);
        long r_pos;
        int blocked;
        blocked = 1;
        while (blocked && !error) {
            blocked = 0;
            printf("*************** pass %li *************\n", pos);
            fflush(stdout);
            r_pos = PTD_write_leafs_to_disk(out, ptmain, node, pos, ppos, &blocked, error);
            if (r_pos > pos) pos = r_pos;
        }
    }
    return pos;
}

inline int ptd_string_shorter_than(const char *s, int len) {
    int i;
    for (i=0; i<len; i++) {
        if (!s[i]) {
            return 1;
        }
    }
    return 0;
}

ARB_ERROR enter_stage_1_build_tree(PT_main * , char *tname) { // __ATTR__USERESULT
    // initialize tree and call the build pos tree procedure

    ARB_ERROR error;

    if (unlink(tname)) {
        if (GB_size_of_file(tname) >= 0) {
            error = GBS_global_string("Cannot remove %s\n", tname);
        }
    }

    if (!error) {
        char *t2name = (char *)calloc(sizeof(char), strlen(tname) + 2);
        sprintf(t2name, "%s%%", tname);
        
        FILE *out = fopen(t2name, "w");
        if (!out) {
            error = GBS_global_string("Cannot open %s\n", t2name);
        }
        else {
            POS_TREE *pt = NULL;
            
            {
                GB_ERROR sm_error = GB_set_mode_of_file(t2name, 0666);
                if (sm_error) {
                    GB_warningf("%s\nOther users might get problems when they try to access this file.", sm_error);
                }
            }

            putc(0, out);       // disable zero father
            long pos = 1;

            // now temp file exists -> trigger ptserver-selectionlist-update in all
            // ARB applications by writing to log
            GBS_add_ptserver_logentry(GBS_global_string("Calculating probe tree (%s)", tname));

            psg.ptmain = PT_init();
            psg.ptmain->stage1 = 1;             // enter stage 1

            pt = PT_create_leaf(psg.ptmain, NULL, PT_N, 0, 0, 0);  // create main node
            pt = PT_change_leaf_to_node(psg.ptmain, pt);
            psg.stat.cut_offs = 0;                  // statistic information
            GB_begin_transaction(psg.gb_main);

            long last_obj = 0;
            char partstring[256];
            int  partsize = 0;
            int  pass0    = 0;
            int  passes   = 1;

            {
                ULONG total_size = psg.char_count;

                printf("Overall bases: %li\n", total_size);

                while (1) {
#ifdef ARB_64
                    ULONG estimated_kb = (total_size/1024)*55;  // value by try and error (for gene server)
                    // TODO: estimated_kb depends on 32/64 bit...
#else
                    ULONG estimated_kb = (total_size/1024)*35; // value by try and error; 35 bytes per base
#endif
                    printf("Estimated memory usage for %i passes: %lu k\n", passes, estimated_kb);

                    if (estimated_kb <= physical_memory) break;

                    total_size /= 4;
                    partsize ++;
                    passes     *= 5;
                }
            }

            printf ("Tree Build: %li bases in %i passes\n", psg.char_count, passes);

            PT_init_base_string_counter(partstring, PT_N, partsize);
            while (!PT_base_string_counter_eof(partstring)) {
                ++pass0;
                printf("\n Starting pass %i(%i)\n", pass0, passes);

                for (int i = 0; i < psg.data_count; i++) {
                    int   psize;
                    char *align_abs = probe_read_alignment(i, &psize);

                    if ((i % 1000) == 0) {
                        printf("%i(%i)\t cutoffs:%i\n", i, psg.data_count, psg.stat.cut_offs);
                        fflush(stdout);
                    }

                    int abs_align_pos = psize-1;
                    for (int j = psg.data[i].size - 1; j >= 0; j--, abs_align_pos--) {
                        get_abs_align_pos(align_abs, abs_align_pos); // may result in neg. abs_align_pos (seems to happen if sequences are short < 214bp )
                        if (abs_align_pos < 0) break; // -> in this case abort

                        if (partsize && (*partstring != psg.data[i].data[j] || strncmp(partstring, psg.data[i].data+j, partsize))) continue;
                        if (ptd_string_shorter_than(psg.data[i].data+j, 9)) continue;

                        pt = build_pos_tree(pt, j, abs_align_pos, i, psg.data[i].size);
                    }
                    free(align_abs);
                }
                pos = PTD_save_partial_tree(out, psg.ptmain, pt, partstring, partsize, pos, &last_obj, error);
                if (error) break;

#ifdef PTM_DEBUG
                PTM_debug_mem();
                PTD_debug_nodes();
#endif
                PT_inc_base_string_count(partstring, PT_N, PT_B_MAX, partsize);
            }

            if (!error) {
                if (partsize) {
                    pos = PTD_save_partial_tree(out, psg.ptmain, pt, NULL, 0, pos, &last_obj, error);
#ifdef PTM_DEBUG
                    PTM_debug_mem();
                    PTD_debug_nodes();
#endif
                }
            }
            if (!error) {
                bool need64bit                        = false;  // does created db need a 64bit ptserver ?
#ifdef ARB_64
                if (last_obj >= 0xffffffff) need64bit = true;   // last_obj is bigger than int
#else
                if (last_obj <= 0) {                            // overflow ?
                    GBK_terminate("Overflow - out of memory");
                }
#endif

                // write information about database
                long info_pos = pos;
                PTD_put_int(out, PT_SERVER_MAGIC);              // marker to identify PT-Server file
                fputc(PT_SERVER_VERSION, out);                  // version of PT-Server file
                pos += 4+1;

                // as last element of info block, write it's size (2byte)
                long info_size = pos-info_pos;
                PTD_put_short(out, info_size);
                pos += 2;

                // save DB footer (which is the entry point on load)

                if (need64bit) {                                // last_obj is bigger than int
#ifdef ARB_64
                    PTD_put_longlong(out, last_obj);            // write last_obj as long long (64 bit)
                    PTD_put_int(out, 0xffffffff);               // write 0xffffffff at the end to signalize 64bit ptserver is needed
#else
                    pt_assert(0);
#endif
                }
                else {
                    PTD_put_int(out, last_obj);                 // last_obj fits into an int -> store it as usual (compatible to old unversioned format)
                }
            }
            if (error) {
                GB_abort_transaction(psg.gb_main);
                fclose(out);

                int res = GB_unlink(t2name);
                if (res == -1) fputs(GB_await_error(), stderr);
            }
            else {
                GB_commit_transaction(psg.gb_main);
                fclose(out);

                error = GB_rename_file(t2name, tname);
                if (!error) {
                    GB_ERROR sm_error = GB_set_mode_of_file(tname, 00666);
                    if (sm_error) GB_warning(sm_error);

                    psg.pt = pt;
                }
            }
        }
        free(t2name);
    }
    return error;
}

ARB_ERROR enter_stage_3_load_tree(PT_main *, const char *tname) { // __ATTR__USERESULT
    // load tree from disk
    ARB_ERROR error;
    
    psg.ptmain         = PT_init();
    psg.ptmain->stage3 = 1;                         // enter stage 3

    long size = GB_size_of_file(tname);
    if (size<0) {
        error = GB_IO_error("stat", tname);
    }
    else {
        printf("- reading Tree %s of size %li from disk\n", tname, size);
        FILE *in = fopen(tname, "r");
        if (!in) {
            error = GB_IO_error("read", tname);
        }
        else {
            error = PTD_read_leafs_from_disk(tname, psg.ptmain, &psg.pt);
            fclose(in);
        }
    }

    return error;
}

