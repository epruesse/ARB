
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include <struct_man.h>
#include "probe.h"
#include "probe_tree.hxx"
#include "pt_prototypes.h"
#include <arbdbt.h>
/* if chain is reached copy data in locs structure */


GB_INLINE double ptnd_get_wmismatch(PT_pdc *pdc, char *probe, int pos, char ref)
{
    int	base;
    int	complement;
    double	max_bind;
    double	new_bind;
    base = probe[pos];
    complement = psg.complement[base];
    max_bind = pdc->bond[  (complement-(int)PT_A)*4 + base-(int)PT_A].val;
    new_bind = pdc->bond[  (complement-(int)PT_A)*4 + ref-(int)PT_A].val;
    return (max_bind - new_bind);
}

int PT_chain_print(int name, int apos, int rpos, long ilocs)
{
    PT_probematch *ml;
    char *probe = psg.probe;
    int mismatches = psg.mismatches;
    double wmismatches = psg.wmismatches;
    double	h;
    int N_mismatches = psg.N_mismatches;
    PT_local *locs = (PT_local *)ilocs;
    int	height;
    int	base;
    int	ref;
    int	pos;
    if (psg.probe) {
        pos = rpos+psg.height;
        height = psg.height;
        while ( (base=probe[height]) && ( ref = psg.data[name].data[pos] ) ) {
            if (ref == PT_N || base == PT_N)
                N_mismatches++;
            else
                if (ref != base){
                    mismatches++;
                    if (locs->pdc) {
                        h = ptnd_get_wmismatch(locs->pdc, probe, height, ref);
                        wmismatches += psg.pos_to_weight[height]*h;
                    }
                }
            height ++;pos++;
        }
        while ( (base = probe[height]) ) {
            N_mismatches++;
            height++;
        }
        if (locs->sort_by != PT_MATCH_TYPE_INTEGER) {
            if (psg.w_N_mismatches[N_mismatches] + (int)(wmismatches+.5) > psg.deep) return 0;
        }else{
            if (psg.w_N_mismatches[N_mismatches]+mismatches>psg.deep) return 0;
        }
    }

    ml = create_PT_probematch();

    ml->name         = name;
    ml->b_pos        = apos;
    ml->g_pos        = -1;
    ml->rpos         = rpos;
    ml->wmismatches  = wmismatches;
    ml->mismatches   = mismatches;
    ml->N_mismatches = N_mismatches;
    ml->sequence     = psg.main_probe;
    ml->reversed     = psg.reversed ? 1 : 0;

    aisc_link((struct_dllpublic_ext*)&(locs->ppm),(struct_dllheader_ext*)ml);

    return 0;
}

/* go down the tree to chains and leafs; copy names, positions and mismatches in locs structure */
int read_names_and_pos(PT_local *locs, POS_TREE *pt)
{
    int base;
    int	error;
    int	name, pos, rpos;
    PT_probematch *ml;

    if (pt == NULL)
        return 0;
    if (locs->ppm.cnt > PT_MAX_MATCHES) {
        locs->matches_truncated = 1;
        return 1;
    }
    if (PT_read_type(pt) == PT_NT_LEAF) {
        name = PT_read_name(psg.ptmain,pt);
        pos  = PT_read_apos(psg.ptmain,pt);
        rpos = PT_read_rpos(psg.ptmain,pt);

        ml = create_PT_probematch();

        ml->name         = name;
        ml->b_pos        = pos;
        ml->g_pos        = -1;
        ml->rpos         = rpos;
        ml->mismatches   = psg.mismatches;
        ml->wmismatches  = psg.wmismatches;
        ml->N_mismatches = psg.N_mismatches;
        ml->sequence     = psg.main_probe;
        ml->reversed     = psg.reversed ? 1 : 0;

        aisc_link((struct_dllpublic_ext*)&(locs->ppm),(struct_dllheader_ext*)ml);

        return 0;
    }else
        if (PT_read_type(pt) == PT_NT_CHAIN) {
            psg.probe = 0;
            if (PT_read_chain(psg.ptmain,pt, PT_chain_print, (long)locs)) {
                error = 1;
                return 1;
            }
        }else{
            for (base = PT_QU; base< PT_B_MAX; base++) {
                error = read_names_and_pos(locs, PT_read_son(psg.ptmain,pt,(PT_BASES)base));
                if (error) return error;
            }

            return 0;
        }
    return 0;
}

/* search down the tree to find matching species for the given probe */
int get_info_about_probe(PT_local *locs, char *probe, POS_TREE *pt, int mismatches, double wmismatches, int N_mismatches, int height){
    int	name, pos;
    int	i;
    int	base;
    POS_TREE	*pthelp;
    int	newmis;
    double	newwmis;
    double	h;
    int	new_N_mis;
    int	error;
    if (!pt) return 0;
    if (locs->sort_by != PT_MATCH_TYPE_INTEGER) {
        if (psg.w_N_mismatches[N_mismatches] + (int)(wmismatches+ 0.5) > psg.deep) return 0;
    }else{
        if (psg.w_N_mismatches[N_mismatches]+mismatches>psg.deep) return 0;
    }
    if (PT_read_type(pt) == PT_NT_NODE && probe[height] ) {
        for (i=PT_N; i<PT_B_MAX; i++) {
            if ( (pthelp = PT_read_son(psg.ptmain,pt, (PT_BASES)i))){
                new_N_mis = N_mismatches;
                base = probe[height];
                if (base == PT_N || i == PT_N) {
                    newmis = mismatches;
                    newwmis = wmismatches;
                    new_N_mis = N_mismatches + 1;
                }else 	if (i != base) {
                    if (locs->pdc) {
                        h = ptnd_get_wmismatch(locs->pdc, probe, height, i);
                        newwmis = wmismatches + psg.pos_to_weight[height] * h;
                    }else{
                        newwmis = wmismatches;
                    }
                    newmis = mismatches+1;
                }else{
                    newmis = mismatches;
                    newwmis = wmismatches;
                }
                error = get_info_about_probe(locs,probe,pthelp,newmis,newwmis, new_N_mis, height+1);
                if (error) return error;
            }
        }
        return 0;
    }
    psg.mismatches = mismatches;
    psg.wmismatches = wmismatches;
    psg.N_mismatches = N_mismatches;
    if (probe[height]) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            pos = PT_read_rpos(psg.ptmain,pt) + height;
            name = PT_read_name(psg.ptmain,pt);
            if (pos + (int)(strlen(probe+height)) >= psg.data[name].size)	/* end of sequence */
                return 0;

            while ((base = probe[height])) {
                i = psg.data[name].data[pos];
                if (i == PT_N || base == PT_N){
                    psg.N_mismatches = psg.N_mismatches + 1;
                }else{
                    if (i != base){
                        psg.mismatches++;
                        if (locs->pdc) {
                            h = ptnd_get_wmismatch(locs->pdc, probe, height, i);
                            psg.wmismatches += psg.pos_to_weight[height] * h;
                        }
                    }
                }
                pos++;
                height++;
            }
        } else {		/* chain */
            psg.probe = probe;
            psg.height = height;
            PT_read_chain(psg.ptmain,pt, PT_chain_print, (long)locs);
            return 0;
        }
        if (locs->sort_by != PT_MATCH_TYPE_INTEGER) {
            if (psg.w_N_mismatches[psg.N_mismatches] + (int)(psg.wmismatches+.5) > psg.deep) return 0;
        }else{
            if (psg.w_N_mismatches[psg.N_mismatches]+psg.mismatches>psg.deep) return 0;
        }
    }
    return read_names_and_pos(locs, pt);
}


#ifdef __cplusplus
extern "C" {
#endif

    static long pt_sort_compare_match(void *PT_probematch_ptr1, void *PT_probematch_ptr2, char*) {
        PT_probematch *mach1 = (PT_probematch*)PT_probematch_ptr1;
        PT_probematch *mach2 = (PT_probematch*)PT_probematch_ptr2;

        if (psg.deep<0) {
            if (mach1->dt > mach2->dt) return 1;
            if (mach1->dt < mach2->dt) return -1;
        }
        if (psg.sort_by != PT_MATCH_TYPE_INTEGER){
            if (mach1->wmismatches > mach2->wmismatches) return 1;
            if (mach1->wmismatches < mach2->wmismatches) return -1;
        }
        if (mach1->mismatches > mach2->mismatches) return 1;
        if (mach1->mismatches < mach2->mismatches) return -1;
        if (mach1->N_mismatches > mach2->N_mismatches) return 1;
        if (mach1->N_mismatches < mach2->N_mismatches) return -1;
        if (mach1->wmismatches > mach2->wmismatches) return 1;
        if (mach1->wmismatches < mach2->wmismatches) return -1;
        if (mach1->b_pos > mach2->b_pos) return 1;
        if (mach1->b_pos < mach2->b_pos) return -1;
        if (mach1->name > mach2->name) return 1;
        if (mach1->name < mach2->name) return -1;
        return 0;
    }

#ifdef __cplusplus
}
#endif

void pt_sort_match_list(PT_local * locs)
{
    PT_probematch	**my_list;
    int 		list_len;
    PT_probematch	*match;
    int		i;
    if (!locs->pm) return;
    psg.sort_by = locs->sort_by;
    list_len = get_MATCHLIST_CNT(locs->pm);
    if (list_len <= 1) return;
    my_list = (PT_probematch **)calloc(sizeof(void *),list_len);
    for (	i=0,	match = locs->pm;
            match;
            i++,match=match->next	){
        my_list[i] = match;
    }
    GB_mergesort((void **)my_list,0,list_len,pt_sort_compare_match,0 );
    for (i=0;i<list_len;i++) {
        aisc_unlink((struct_dllheader_ext*)my_list[i]);
        aisc_link((struct_dllpublic_ext*)&(locs->ppm),(struct_dllheader_ext*)my_list[i]);
    }
    free((char *)my_list);
}
/* mirror a probe */
char *reverse_probe(char *probe, int probe_length)
{
    int	i,j;
    char	*rev_probe;
    if (!probe_length) probe_length = strlen(probe);
    rev_probe = (char *)malloc(probe_length * sizeof(char)+1);
    j = probe_length - 1;
    for (i=0; i< probe_length; i++)
        rev_probe[j--] = probe[i];
    rev_probe[probe_length] = '\0';
    return rev_probe;
}
int PT_complement(int base)
{
    switch(base) {
        case PT_A:	return PT_T;
        case PT_C:	return PT_G;
        case PT_G:	return PT_C;
        case PT_T:	return PT_A;
        default:	return base;
    }
}
/* build the complement of a probe */
void complement_probe(char *probe, int probe_length)
{
    int	i;
    if (!probe_length) probe_length = strlen(probe);
    for (i=0; i< probe_length; i++){
        probe[i] = PT_complement(probe[i]);
    }
}

double calc_position_wmis(int pos, int seq_len, double y1, double y2)
{
    return (double)(((double)(pos * (seq_len - 1 - pos)) / (double)((seq_len - 1) * (seq_len - 1)))* (double)(y2*4.0) + y1);
}

void pt_build_pos_to_weight(PT_MATCH_TYPE type, const char *sequence){
    delete [] psg.pos_to_weight;
    int slen = strlen(sequence);
    psg.pos_to_weight = new double[slen+1];
    int p;
    for (p=0;p<slen;p++){
        if (type == PT_MATCH_TYPE_WEIGHTED_PLUS_POS){
            psg.pos_to_weight[p] = calc_position_wmis(p, slen, 0.3, 1.0);
        }else{
            psg.pos_to_weight[p] = 1.0;
        }
    }
    psg.pos_to_weight[slen] = 0;
}

/* find out where a given probe matches */
extern "C" int probe_match(PT_local * locs, aisc_string probestring)
{
    // IDP probe match

    PT_probematch *ml;
    char          *rev_pro;
    if (locs->pm_sequence) free(locs->pm_sequence);
    locs->pm_sequence       = psg.main_probe = strdup(probestring);
    compress_data(probestring);
    while ((ml = locs->pm)) destroy_PT_probematch(ml);
    locs->matches_truncated = 0;

    {
        int probe_len = strlen(probestring);
        if ((probe_len - 2*locs->pm_max) < MIN_PROBE_LENGTH) {
            if (probe_len >= MIN_PROBE_LENGTH) {
                int max_pos_mismatches = (probe_len-MIN_PROBE_LENGTH)/2;
                if (max_pos_mismatches>0) {
                    if (max_pos_mismatches>1) {
                        pt_export_error(locs, GBS_global_string("Max. %i mismatches are allowed for probes of length %i", max_pos_mismatches, probe_len));
                    }
                    else {
                        pt_export_error(locs, GBS_global_string("Max. 1 mismatch is allowed for probes of length %i", probe_len));
                    }
                }
                else {
                    pt_export_error(locs, "No mismatches allowed for that short probes.");
                }
            }
            else {
                pt_export_error(locs, GBS_global_string("Min. probe length is %i", MIN_PROBE_LENGTH));
            }
            return 0;
        }
    }
    set_table_for_PT_N_mis();
    if (locs->pm_complement) {
        complement_probe(probestring,0);
    }
    psg.reversed = 0;

    if (locs->pm_sequence) free(locs->pm_sequence);
    locs->pm_sequence = psg.main_probe = strdup(probestring);

    psg.deep = locs->pm_max;
    pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by,probestring);

    if (psg.deep >=0 ){
        get_info_about_probe(locs, probestring, psg.pt, 0, 0.0, 0, 0);
    }else{
        ptnd_new_match(locs,	probestring);
    }
    if (locs->pm_reversed) {
        psg.reversed = 1;
        rev_pro = reverse_probe(probestring,0);
        complement_probe(rev_pro,0 );
        if (locs->pm_csequence) free(locs->pm_csequence);
        locs->pm_csequence = psg.main_probe = strdup(rev_pro);
        if (psg.deep >=0 ){
            get_info_about_probe(locs, rev_pro, psg.pt, 0, 0.0, 0, 0);
        }else{
            ptnd_new_match(locs,	rev_pro);
        }
        free(rev_pro);
    }
    pt_sort_match_list(locs);
    free(probestring);
    return 0;
}

struct format_props {
    bool show_mismatches;       // whether to show 'mis' and 'N_mis'
    bool show_ecoli;            // whether to show 'ecoli' column
    bool show_gpos;             // whether to show 'gpos' column

    int name_width;             // width of 'name' column
    int gene_or_full_width;     // width of 'genename' or 'fullname' column
    int pos_width;              // max. width of pos column
    int gpos_width;             // max. width of gpos column
    int ecoli_width;            // max. width of ecoli column

    int rev_width() const { return 3; }
    int mis_width() const { return 3; }
    int N_mis_width() const { return 5; }
    int wmis_width() const { return 4; }
};

inline void set_max(const char *str, int &curr_max) {
    if (str) {
        int len = strlen(str);
        if (len>curr_max) curr_max = len;
    }
}

static format_props detect_format_props(PT_local *locs, bool show_gpos) {
    PT_probematch *ml = locs->pm; // probe matches
    format_props   format;

    format.show_mismatches = (ml->N_mismatches >= 0);
    format.show_ecoli      = psg.ecoli; // display only if there is ecoli
    format.show_gpos       = show_gpos; // display only for gene probe matches

    // minumum values (caused by header widths) :
    format.name_width         = gene_flag ? 8 : 4; // 'organism' or 'name'
    format.gene_or_full_width = 8; // 'genename' or 'fullname'
    format.pos_width          = 3; // 'pos'
    format.gpos_width         = 4; // 'gpos'
    format.ecoli_width        = 5; // 'ecoli'

    for (PT_probematch *ml = locs->pm; ml; ml = ml->next) {
        set_max(virt_name(ml), format.name_width);
        set_max(virt_fullname(ml), format.gene_or_full_width);
        set_max(GBS_global_string("%i", ml->b_pos), format.pos_width);
        if (show_gpos) set_max(GBS_global_string("%i", ml->g_pos), format.gpos_width);
        if (format.show_ecoli) set_max(GBS_global_string("%li", PT_abs_2_rel(ml->b_pos)), format.ecoli_width);
    }

    return format;
}

inline void cat_internal(void *memfile, int len, const char *text, int width, char spacer, bool align_left) {
    if (len == width) {
        GBS_strcat(memfile, text); // text has exact len
    }
    else if (len > width) { // text to long
        char buf[width+1];
        memcpy(buf, text, width);
        buf[width] = 0;
        GBS_strcat(memfile, buf);
    }
    else {                      // text is too short -> insert spaces
        int  spaces = width-len;
        pt_assert(spaces>0);
        char sp[spaces+1];
        memset(sp, spacer, spaces);
        sp[spaces]  = 0;

        if (align_left) {
            GBS_strcat(memfile, text);
            GBS_strcat(memfile, sp);
        }
        else {
            GBS_strcat(memfile, sp);
            GBS_strcat(memfile, text);
        }
    }
    GBS_chrcat(memfile, ' '); // one space behind each column
}
inline void cat_spaced_left (void *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, ' ', true); }
inline void cat_spaced_right(void *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, ' ', false); }
inline void cat_dashed_left (void *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, '-', true); }
inline void cat_dashed_right(void *memfile, const char *text, int width) { cat_internal(memfile, strlen(text), text, width, '-', false); }

static const char *get_match_info_formatted(PT_probematch  *ml, const format_props& format)
{
    int    pr_pos,al_pos;
    int    pr_len = strlen(ml->sequence);
    double wmis   = 0.0;
    double h;
    int    a,b;

    PT_local *locs = (PT_local *)ml->mh.parent->parent;
    PT_pdc   *pdc  = locs->pdc;

    char *ref    = (char *)calloc(sizeof(char),21+pr_len);
    memset(ref,'.',10);
    for (pr_pos  = 8, al_pos = ml->rpos-1;
         pr_pos >= 0 && al_pos >=0;
         pr_pos--, al_pos--)
    {
        if (!psg.data[ml->name].data[al_pos]) break;
        ref[pr_pos] = psg.data[ml->name].data[al_pos];
    }
    ref[9] = '-';

    pt_build_pos_to_weight((PT_MATCH_TYPE)locs->sort_by,ml->sequence);

    for (pr_pos = 0, al_pos = ml->rpos;
         pr_pos < pr_len && al_pos < psg.data[ml->name].size;
         pr_pos++, al_pos++)
    {
        if ((a=ml->sequence[pr_pos]) == (b=psg.data[ml->name].data[al_pos])) {
            ref[pr_pos+10] = '=';
        }
        else {
            ref[pr_pos+10] = b;
            if (pdc && a >= PT_A && a <=PT_T && b>= PT_A && b<=PT_T) {
                h = ptnd_check_split(pdc,ml->sequence, pr_pos,b);
                if (h<0.0) {
                    h = -h;
                }
                else {
                    ref[pr_pos+10] = " nacgu"[b];
                }
                wmis += psg.pos_to_weight[pr_pos] * h;
            }
        }

    }

    for (pr_pos = 0, al_pos = ml->rpos+pr_len;
         pr_pos < 9 && al_pos < psg.data[ml->name].size;
         pr_pos++, al_pos++)
    {
        ref[pr_pos+11+pr_len] = psg.data[ml->name].data[al_pos];
    }
    ref[10+pr_len] = '-';
    PT_base_2_string(ref,0);

    void *memfile = GBS_stropen(256);
    GBS_strcat(memfile, "  ");

    cat_spaced_left(memfile, virt_name(ml), format.name_width);
    cat_spaced_left(memfile, virt_fullname(ml), format.gene_or_full_width);

    if (format.show_mismatches) {
        cat_spaced_right(memfile, GBS_global_string("%i", ml->mismatches), format.mis_width());
        cat_spaced_right(memfile, GBS_global_string("%i", ml->N_mismatches), format.N_mis_width());
    }
    cat_spaced_right(memfile, GBS_global_string("%.1f", wmis), format.wmis_width());
    cat_spaced_right(memfile, GBS_global_string("%i", ml->b_pos), format.pos_width);
    if (format.show_gpos) {
        cat_spaced_right(memfile, GBS_global_string("%i", ml->g_pos), format.gpos_width);
    }
    if (format.show_ecoli) {
        cat_spaced_right(memfile, GBS_global_string("%li", PT_abs_2_rel(ml->b_pos)), format.ecoli_width);
    }
    cat_spaced_left(memfile, GBS_global_string("%i", ml->reversed), format.rev_width());

    GBS_strcat(memfile, ref);

    free(ref);
    return GBS_strclose(memfile);
}

static const char *get_match_hinfo_formatted(PT_probematch *ml, const format_props& format) {
    // Set header of result
    const char *result = 0;
    if (!ml) {
        result = "There are no targets";
    }
    else {
        void *memfile = GBS_stropen(500);
        GBS_strcat(memfile, "    "); // one space more than in get_match_info_formatted()

        cat_dashed_left(memfile, gene_flag ? "organism" : "name", format.name_width);
        cat_dashed_left(memfile, gene_flag ? "genename" : "fullname", format.gene_or_full_width);

        if (format.show_mismatches) {
            cat_dashed_right(memfile, "mis",   format.mis_width());
            cat_dashed_right(memfile, "N_mis", format.N_mis_width());
        }
        cat_dashed_right(memfile, "wmis", format.wmis_width());
        cat_dashed_right(memfile, "pos", format.pos_width);
        if (format.show_gpos) {
            cat_dashed_right(memfile, "gpos", format.gpos_width);
        }
        if (format.show_ecoli) {
            cat_dashed_right(memfile, "ecoli", format.ecoli_width);
        }
        cat_dashed_left(memfile, "rev", format.rev_width());

        if (ml->N_mismatches >= 0) { //
            char *seq = strdup(ml->sequence);
            PT_base_2_string(seq,0);

            GBS_strcat(memfile, "         '");
            GBS_strcat(memfile, seq);
            GBS_strcat(memfile, "'");

            free(seq);
        }

        static char *result = 0;
        if (result) free(result);
        result = GBS_strclose(memfile);

        return result;
    }
    pt_assert(result);
    return result;
}

static void gene_rel_2_abs(PT_probematch *ml) {
    // after gene probe match all positions are gene-relative.
    // gene_rel_2_abs() makes them genome-absolute.

    GB_transaction ta(psg.gb_main);

    for (; ml; ml = ml->next) {
        probe_input_data&  pid    = psg.data[ml->name];
        GBDATA            *gb_pos = GB_find(pid.gbd, "abspos", 0, down_level);

        if (gb_pos) {
            long gene_pos  = GB_read_int(gb_pos);
            ml->g_pos      = ml->b_pos;
            ml->b_pos     += gene_pos;
        }
        else {
            fprintf(stderr, "Error in gene-pt-server: gene w/o position info\n");
            pt_assert(gb_pos);
        }
    }
}

/* Create a big output string:	header\001name\001info\001name\001info....\000 */
extern "C" bytestring *match_string(PT_local *locs) {
    static bytestring bs = {0,0};
    void          *memfile;
    PT_probematch *ml;

    free(bs.data);

    char empty[] = "";
    bs.data      = empty;
    memfile      = GBS_stropen(50000);

    if (locs->pm) {
        if (gene_flag) gene_rel_2_abs(locs->pm);

        format_props format = detect_format_props(locs, gene_flag);

        GBS_strcat(memfile, get_match_hinfo_formatted(locs->pm, format));
        GBS_chrcat(memfile,(char)1);

        for (ml = locs->pm; ml ; ml = ml->next){
            GBS_strcat(memfile, virt_name(ml));
            GBS_chrcat(memfile,(char)1);
            GBS_strcat(memfile, get_match_info_formatted(ml, format));
            GBS_chrcat(memfile,(char)1);
        }
    }

    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}




/* Create a big output string:	header\001name\001#mismatch\001name\001#mismatch....\000 */
extern "C" bytestring *MP_match_string(PT_local *locs){
    static bytestring bs = {0,0};
    char buffer[50];
    char buffer1[50];
    void *memfile;
    PT_probematch *ml;

    delete bs.data;
    bs.data = 0;
    memfile = GBS_stropen(50000);

    for (ml = locs->pm; ml ; ml = ml->next)
    {
        GBS_strcat(memfile, virt_name(ml));
        GBS_chrcat(memfile,(char)1);
        sprintf(buffer, "%2i", ml->mismatches);
        GBS_strcat(memfile, buffer);
        GBS_chrcat(memfile,(char)1);
        sprintf(buffer1, "%1.1f", ml->wmismatches);
        GBS_strcat(memfile, buffer1);
        GBS_chrcat(memfile,(char)1);
    }
    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}


/* Create a big output string:	001name\001name\....\000 */
extern "C" bytestring *MP_all_species_string(PT_local *){
    static bytestring bs = {0,0};
    void *memfile;
    int i;

    delete bs.data;
    bs.data = 0;
    memfile = GBS_stropen(1000);

    for (i = 0; i < psg.data_count; i++)
    {
        GBS_strcat(memfile, psg.data[i].name);
        GBS_chrcat(memfile,(char)1);
    }

    bs.data = GBS_strclose(memfile);
    bs.size = strlen(bs.data)+1;
    return &bs;
}



extern "C" int MP_count_all_species(PT_local *)
{
    return psg.data_count;
}



