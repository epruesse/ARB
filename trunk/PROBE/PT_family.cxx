#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <PT_server.h>
#include <struct_man.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"
#include <arbdbt.h>


// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_family_list *family)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(family)); }

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Increment match_count for a matched chain */

int mark_all_matches_chain_handle(int name, int /*pos*/, int /*rpos*/, long /*clientdata*/)
{
    psg.data[name].stat.match_count++;
    return 0;
}
/* Increment match_count for every match */
int mark_all_matches( PT_local  *locs,
                      POS_TREE  *pt,
                      char      *probe,
                      int       length,
                      int       mismatches,
                      int       height,
                      int	max_mismatches)
{
    int             ref_pos, ref2_pos, ref_name;
    int        base;
    int             type_of_node;
    POS_TREE        *pt_son;
    if (pt == NULL || mismatches > max_mismatches) {
        return 0;
    }
    type_of_node = PT_read_type(pt);
    if ((type_of_node = PT_read_type(pt)) != PT_NT_NODE) {
        /*
         * Found unique solution
         */
        if (type_of_node == PT_NT_LEAF) {
            ref2_pos = PT_read_rpos(psg.ptmain,pt);
            ref_pos = ref2_pos + height;
            ref_name = PT_read_name(psg.ptmain,pt);
            /*
             * Check rest of probe
             */
            while (mismatches <= max_mismatches && *probe) {
                if (psg.data[ref_name].data[ref_pos++] != *probe) {
                    mismatches++;
                }
                probe++;
                height++;
            }
            /*
             * Increment count if probe matches
             */
            if (mismatches <= max_mismatches)
                psg.data[ref_name].stat.match_count++;
            return 0;
        } else {        /* type_of_node == CHAIN !! */
            psg.mismatches = mismatches;
            psg.height = height;
            psg.length = length;
            psg.probe = probe;
            PT_read_chain(psg.ptmain,pt,mark_all_matches_chain_handle, (long) locs);
            return 0;
        }
    }
    for (base = PT_N; base < PT_B_MAX; base++) {
        if ( (pt_son = PT_read_son(psg.ptmain, pt, (PT_BASES)base))) {
            if (*probe) {
                if (*probe != base) {
                    mark_all_matches(locs, pt_son, probe+1, length,
                                     mismatches + 1, height + 1, max_mismatches);
                } else {
                    mark_all_matches(locs, pt_son, probe+1, length,
                                     mismatches, height + 1, max_mismatches);
                }
            } else {
                mark_all_matches(locs, pt_son, probe, length,
                                 mismatches, height + 1, max_mismatches);
            }
        }
    }
    return 0;
}

/* Clear all informations in psg.data[i].stat */
void clear_statistic(){
    int i;
    for (i = 0; i < psg.data_count; i++)
        memset((char *) &psg.data[i].stat,0,sizeof(struct probe_statistic));
}


/* Calculate the statistic informations for the family */
void make_match_statistic(int probe_len){
    int i;
    /*
     * compute statistic for all species in family
     */
    for (i = 0; i < psg.data_count; i++) {
        int all_len = psg.data[i].size - probe_len + 1;
        if (all_len <= 0){
            psg.data[i].stat.rel_match_count = 0;
        }else{
            psg.data[i].stat.rel_match_count = psg.data[i].stat.match_count / (double) (all_len);
        }
    }
}



#ifdef __cplusplus
extern "C" {
#endif

    /* Compare function for GB_mergesort() */
    static long compare_probe_input_data0(void *pid_ptr1, void *pid_ptr2, char*) {
        struct probe_input_data *d1 = (struct probe_input_data*)pid_ptr1;
        struct probe_input_data *d2 = (struct probe_input_data*)pid_ptr2;

        if (d1->stat.match_count < d2->stat.match_count) return 1;
        if (d1->stat.match_count == d2->stat.match_count) return 0;
        return -1;
    }

    static long compare_probe_input_data1(void *pid_ptr1, void *pid_ptr2, char*) {
        struct probe_input_data *d1 = (struct probe_input_data*)pid_ptr1;
        struct probe_input_data *d2 = (struct probe_input_data*)pid_ptr2;

        if (d1->stat.rel_match_count < d2->stat.rel_match_count) return 1;
        if (d1->stat.rel_match_count == d2->stat.rel_match_count) return 0;
        return -1;
    }

#ifdef __cplusplus
}
#endif

/*  Make sortet list of family members */
int make_PT_family_list(PT_local *locs)
{
    struct probe_input_data **my_list;
    PT_family_list *fl;
    int i;
    /*
     * Sort the data
     */
    my_list = (struct probe_input_data**) calloc(sizeof(void *),psg.data_count);
    for (i = 0; i < psg.data_count; i++)	my_list[i] = &psg.data[i];
    if (locs->sort_type == 0){
        GB_mergesort((void **) my_list,0,psg.data_count, compare_probe_input_data0,0);
    }else{
        GB_mergesort((void **) my_list,0,psg.data_count, compare_probe_input_data1,0);
    }
	/*
	 * destroy old list
	 */
    while(locs->fl)	destroy_PT_family_list(locs->fl);
    /*
     * build new list
     */
    for (i = 0; i < psg.data_count; i++) {
        if (my_list[i]->stat.match_count == 0) continue;
        fl = create_PT_family_list();
        fl->name = strdup(my_list[i]->name);
        fl->matches = my_list[i]->stat.match_count;
        fl->rel_matches = my_list[i]->stat.rel_match_count;
        aisc_link(&locs->pfl, fl);
    }
    free((char *)my_list);
    return 0;
}

/* Check the probe for inconsitencies */
int probe_is_ok(char *probe, int probe_len, char first_c, char second_c)
{
    int i;
    if (probe_len < 2 || probe[0] != first_c || probe[1] != second_c)
        return 0;
    for (i = 2; i < probe_len; i++)
        if (probe[i] < PT_A || probe[i] > PT_T)
            return 0;
    return 1;
}
/* make sorted list of family members of species */
extern "C" int find_family(PT_local *locs, bytestring *species)
{
    char first_c, second_c;
    char *sequence, *probe;
    int sequence_len;
    int probe_len = locs->pr_len;
    int mismatch_nr = locs->mis_nr;
    clear_statistic();
    sequence = species->data;
    sequence_len = probe_compress_sequence(sequence);
    /*
     * search for sorted probes
     */
	/*
	 * find all "*"
	 */
	if (locs->find_type == 0) {
		for (first_c = PT_A; first_c <= PT_T; first_c++)
			for (second_c = PT_A; second_c <= PT_T; second_c++)
				for (probe=sequence;probe<sequence+sequence_len-probe_len;probe++)
					if (probe_is_ok(probe,probe_len,first_c,second_c))
						mark_all_matches(locs,psg.pt,probe,probe_len,0,0,mismatch_nr);
	}
	/*
	 * find only "a*"
	 */
	else {
		first_c = PT_A;
		for (second_c = PT_A; second_c <= PT_T; second_c++)
			for (probe=sequence;probe<sequence+sequence_len-probe_len;probe++)
				if (probe_is_ok(probe,probe_len,first_c,second_c))
					mark_all_matches(locs,psg.pt,probe,probe_len,0,0,mismatch_nr);
	}

    make_match_statistic(locs->pr_len);
    make_PT_family_list(locs);
    free(species->data);
    return 0;
}
