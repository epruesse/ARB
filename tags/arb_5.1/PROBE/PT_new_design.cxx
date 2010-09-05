#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <PT_server.h>
#include <PT_server_prototypes.h>
#include <struct_man.h>
#include "probe.h"
#include "probe_tree.hxx"
#include <arbdbt.h>
#include <inline.h>

#ifdef P_
#error P_ already defined
#endif

#include "pt_prototypes.h"

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_tprobes *tprobe)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(tprobe)); }
inline void aisc_link(dll_public *dll, PT_probeparts *parts) { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(parts)); }
inline void aisc_link(dll_public *dll, PT_probematch *match) { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(match)); }

extern "C" {
    int pt_init_bond_matrix(PT_pdc *THIS)
    {
        THIS->bond[0].val  = 0.0;
        THIS->bond[1].val  = 0.0;
        THIS->bond[2].val  = 0.5;
        THIS->bond[3].val  = 1.1;
        THIS->bond[4].val  = 0.0;
        THIS->bond[5].val  = 0.0;
        THIS->bond[6].val  = 1.5;
        THIS->bond[7].val  = 0.0;
        THIS->bond[8].val  = 0.5;
        THIS->bond[9].val  = 1.5;
        THIS->bond[10].val = 0.4;
        THIS->bond[11].val = 0.9;
        THIS->bond[12].val = 1.1;
        THIS->bond[13].val = 0.0;
        THIS->bond[14].val = 0.9;
        THIS->bond[15].val = 0.0;

        return 0;
    }
}
struct ptnd_loop_com {
    PT_pdc        *pdc;
    PT_local      *locs;
    PT_probeparts *parts;
    int            mishits;
    int            new_match; /* match or design the probe: 1 match   0 design */
    double         sum_bonds; /* sum of bond of longest non mismatch string */
    double         dt;     /* sum of mismatches */
} ptnd;


static int ptnd_compare_quality(const void *PT_tprobes_ptr1, const void *PT_tprobes_ptr2, void *) {
    const PT_tprobes *tprobe1 = (const PT_tprobes*)PT_tprobes_ptr1;
    const PT_tprobes *tprobe2 = (const PT_tprobes*)PT_tprobes_ptr2;

    // sort best quality first
    if (tprobe1->quality < tprobe2->quality) return 1;
    if (tprobe1->quality > tprobe2->quality) return -1;
    return 0;
}

static int ptnd_compare_sequence(const void *PT_tprobes_ptr1, const void *PT_tprobes_ptr2, void*) {
    const PT_tprobes *tprobe1 = (const PT_tprobes*)PT_tprobes_ptr1;
    const PT_tprobes *tprobe2 = (const PT_tprobes*)PT_tprobes_ptr2;

    return strcmp(tprobe1->sequence,tprobe2->sequence);
}

static void ptnd_sort_probes_by(PT_pdc *pdc, int mode)  /* mode 0 quality, mode 1 sequence */
{
    PT_tprobes **my_list;
    int          list_len;
    PT_tprobes  *tprobe;
    int          i;

    if (!pdc->tprobes) return;
    list_len = get_TPROBE_CNT(pdc->tprobes);
    if (list_len <= 1) return;
    my_list = (PT_tprobes **)calloc(sizeof(void *),list_len);
    for (i=0, tprobe = pdc->tprobes;
         tprobe;
         i++,tprobe=tprobe->next)
    {
        my_list[i] = tprobe;
    }
    switch (mode) {
        case 0:
            GB_sort((void **)my_list, 0, list_len, ptnd_compare_quality, 0);
            break;
        case 1:
            GB_sort((void **)my_list, 0, list_len, ptnd_compare_sequence, 0);
            break;
    }

    for (i=0;i<list_len;i++) {
        aisc_unlink(reinterpret_cast<dllheader_ext*>(my_list[i]));
        aisc_link(&pdc->ptprobes, my_list[i]);
    }
    free((char *)my_list);
}
static void ptnd_probe_delete_all_but(PT_pdc *pdc, int count)
{
    PT_tprobes *tprobe;
    int         i;

    for (i=0,tprobe = pdc->tprobes;
         tprobe && i< count;
         i++,tprobe = tprobe->next ) ;
    
    if (tprobe) {
        while(tprobe->next) {
            destroy_PT_tprobes( tprobe->next );
        }
    }
}
static int pt_get_gc_content(char *probe)
{
    int gc = 0;
    while (*probe){
        if (    *probe == PT_G ||
                *probe == PT_C) {
            gc++;
        }
        probe ++;
    }
    return gc;
}
static double pt_get_temperature(const char *probe)
{
    int i;
    double t = 0;
    while (( i=*(probe++) )) {
        if (i == PT_G || i == PT_C) t+=4.0;else t+= 2.0;
    }
    return t;
}

#if 0
int ptnd_check_pure(char *probe)
{
    int i;
    while (( i=*(probe++) )) {
        if (    i < PT_A || i > PT_T) return 1;
    }
    return 0;
}
#endif

static void ptnd_calc_quality(PT_pdc *pdc) {
    PT_tprobes *tprobe;
    int         i;

    for (tprobe = pdc->tprobes;
         tprobe;
         tprobe = tprobe->next)
    {
        for (i=0; i< PERC_SIZE-1; i++) {
            if (tprobe->perc[i] > tprobe->mishit) break;
        }
        tprobe->quality = ((double)tprobe->groupsize * i) + 1000.0/(1000.0 + tprobe->perc[i]) ;
    }
}
/***********************************************************************
                        check the bond val for a probe
***********************************************************************/
static double ptnd_check_max_bond( PT_pdc *pdc, char base)
{
    int complement = PT_complement(base);
    return pdc->bond[  (complement-(int)PT_A)*4 + base-(int)PT_A].val;
}

double ptnd_check_split(PT_pdc *pdc, char *probe, int pos, char ref) {
    int    base       = probe[pos];
    int    complement = PT_complement(base);
    double max_bind   = pdc->bond[  (complement-(int)PT_A)*4 + base-(int)PT_A].val;
    double new_bind   = pdc->bond[  (complement-(int)PT_A)*4 + ref-(int)PT_A].val;
    
    if ( new_bind < pdc->split)
        return new_bind-max_bind; /* negative values indicate split */
    /* this mismatch splits the probe in several domains */
    return (max_bind - new_bind);
}


/***********************************************************************
                        primary search for probes
***********************************************************************/
/* count all mishits for a probe */

struct ptnd_chain_count_mishits {
    int operator()(int name, int apos, int rpos) {
        char *probe = psg.probe;
        if (apos>psg.apos) psg.apos = apos;
        if (psg.data[name].is_group) return 0;              /* dont count group or neverminds */
        if (probe) {
            rpos+=psg.height;
            while (*probe && psg.data[name].data[rpos]) {
                if (psg.data[name].data[rpos] != *(probe))
                    return 0;
                probe++; rpos++;
            }
        }
        ptnd.mishits++;
        return 0;
    }
};

/* go down the tree to chains and leafs; count the species that are in the non member group */
static int ptnd_count_mishits2(POS_TREE *pt)
{
    int base;
    int name;
    int mishits = 0;

    if (pt == NULL)
        return 0;
    if (PT_read_type(pt) == PT_NT_LEAF) {
        name = PT_read_name(psg.ptmain,pt);
        int apos = PT_read_apos(psg.ptmain,pt);
        if (apos>psg.apos) psg.apos = apos;
        if (!psg.data[name].is_group)   return 1;
        return 0;
    }else if (PT_read_type(pt) == PT_NT_CHAIN) {
        psg.probe = 0;
        ptnd.mishits = 0;
        PT_read_chain(psg.ptmain,pt, ptnd_chain_count_mishits());
        return ptnd.mishits;
    }else{
        for (base = PT_QU; base< PT_B_MAX; base++) {
            mishits += ptnd_count_mishits2(PT_read_son(psg.ptmain,pt,(PT_BASES)base));
        }
        return mishits;
    }
}
extern "C" char *get_design_info(PT_tprobes  *tprobe)
{
    char   *buffer = (char *)GB_give_buffer(2000);
    char   *probe  = (char *)GB_give_buffer2(tprobe->seq_len + 10);
    PT_pdc *pdc    = (PT_pdc *)tprobe->mh.parent->parent;
    char   *p;
    int     i;
    int     sum;

    p = buffer;

    // target
    strcpy(probe,tprobe->sequence);
    PT_base_2_string(probe,0);  /* convert probe to real ASCII */
    sprintf(p, "%-*s", pdc->probelen+1, probe);
    p += strlen(p);

    int apos = tprobe->apos;
    int c    = 0;
    int cs   = '=';
    { // search nearest hit
        for (c=0;c<ALPHA_SIZE;c++) {
            if (!pdc->pos_groups[c]) { // new group
                pdc->pos_groups[c] = tprobe->apos;
                break;
            }
            int dist = tprobe->apos - pdc->pos_groups[c];
            if (dist<0) dist = -dist;
            if ( dist < pdc->probelen) {
                apos = tprobe->apos - pdc->pos_groups[c];
                if (apos > 0) {
                    cs = '+';
                }else{
                    apos = -apos; cs = '-';
                }
                break;
            }
        }
    }

    // le apos ecol
    sprintf(p,"%2i %c%c%6i %4li ", tprobe->seq_len, c+'A', cs, apos, PT_abs_2_rel(tprobe->apos));
    p += strlen(p);

    // grps
    sprintf(p,"%4i ",tprobe->groupsize);
    p += strlen(p);

    // G+C
    sprintf(p,"%-4.1f ",((double)pt_get_gc_content(tprobe->sequence))/tprobe->seq_len*100.0);
    p += strlen(p);

    // 4GC+2AT
    sprintf(p,"%-7.1f ", pt_get_temperature(tprobe->sequence));
    p += strlen(p);

    // probe string
    probe = reverse_probe(tprobe->sequence,0);
    complement_probe(probe,0);
    PT_base_2_string(probe,0);  /* convert probe to real ASCII */
    sprintf(p,"%-*s |", pdc->probelen, probe);
    free(probe);
    p += strlen(p);

    // non-group hits by temp. decrease
    for (sum=i=0;i<PERC_SIZE;i++) {
        sum+= tprobe->perc[i];
        sprintf(p,"%3i,",sum);
        p += strlen(p);
    }

    return buffer;
}


extern "C" char *get_design_hinfo(PT_tprobes  *tprobe) {
    char   *buffer = (char *)GB_give_buffer(2000);
    char   *s      = buffer;
    PT_pdc *pdc;

    if (!tprobe) {
        return (char*)"Sorry, there are no probes for your selection !!!";
    }
    pdc = (PT_pdc *)tprobe->mh.parent->parent;
    sprintf(buffer,
            "Probe design Parameters:\n"
            "Length of probe    %4i\n"
            "Temperature        [%4.1f -%4.1f]\n"
            "GC-Content         [%4.1f -%4.1f]\n"
            "E.Coli Position    [%4i -%4i]\n"
            "Max Non Group Hits  %4i\n"
            "Min Group Hits      %4.0f%%\n",
            pdc->probelen,
            pdc->mintemp,pdc->maxtemp,
            pdc->min_gc*100.0,pdc->max_gc*100.0,
            pdc->minpos, pdc->maxpos,
            pdc->mishit, pdc->mintarget*100.0);

    s += strlen(s);

    sprintf(s,"%-*s", pdc->probelen+1, "Target");
    s += strlen(s);

    sprintf(s,"%2s %8s %4s ","le","apos","ecol");
    s += strlen(s);

    sprintf(s,"%4s ","grps"); // groupsize
    s += strlen(s);

    sprintf(s,"%-4s %-7s %-*s | "," G+C","4GC+2AT", pdc->probelen, "Probe sequence");
    s += strlen(s);

    sprintf(s, "Decrease T by n*.3C -> probe matches n non group species");
    // s += strlen(s);

    return buffer;
}

/* search down the tree to find matching species for the given probe */
static int ptnd_count_mishits(char *probe, POS_TREE *pt,int height)
{
    int       name;
    int       i;
    POS_TREE *pthelp;
    int       pos;
    int       mishits;

    if (!pt) return 0;
    mishits = 0;
    if (PT_read_type(pt) == PT_NT_NODE && *probe) {
        for (i=PT_A; i<PT_B_MAX; i++) {
            if (i!= *probe) continue;
            if (( pthelp = PT_read_son(psg.ptmain,pt, (PT_BASES)i) ))
                mishits += ptnd_count_mishits(probe+1,pthelp, height+1);
        }
        return mishits;
    }
    if (*probe) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            pos = PT_read_rpos(psg.ptmain,pt) + height;
            name = PT_read_name(psg.ptmain,pt);
            if (pos + (int)(strlen(probe)) >= psg.data[name].size)              /* after end */
                return 0;

            while (*probe) {
                if (psg.data[name].data[pos++] != *(probe++))
                    return 0;
            }
        } else {                /* chain */
            psg.probe = probe;
            psg.height = height;
            ptnd.mishits = 0;
            PT_read_chain(psg.ptmain,pt, ptnd_chain_count_mishits());
            return ptnd.mishits;
        }
    }
    return ptnd_count_mishits2(pt);
}

static void ptnd_first_check(PT_pdc *pdc)       /* checks the direkt mishits */
{
    PT_tprobes  *tprobe, *tprobe_next;
    for (       tprobe = pdc->tprobes;
                tprobe;
                tprobe = tprobe_next ) {
        tprobe_next = tprobe->next;
        psg.apos = 0;
        tprobe->mishit = ptnd_count_mishits(tprobe->sequence,psg.pt,0);
        tprobe->apos = psg.apos;
        if (tprobe->mishit > pdc->mishit) {
            destroy_PT_tprobes(tprobe);
        }
    }
}
/***********************************************************************
                        Check the probes Position
***********************************************************************/

static void ptnd_check_position(PT_pdc *pdc)    /* checks the direkt mishits */
{
    PT_tprobes  *tprobe, *tprobe_next;
    if ( pdc->minpos == pdc->maxpos) return;

    for (       tprobe = pdc->tprobes;
                tprobe;
                tprobe = tprobe_next ) {
        tprobe_next = tprobe->next;
        long relpos = PT_abs_2_rel(tprobe->apos);
        if (relpos < pdc->minpos || relpos > pdc->maxpos) {
            destroy_PT_tprobes(tprobe);
        }
    }
}
/***********************************************************************
                        check the average bond size
***********************************************************************/
static void ptnd_check_bonds(PT_pdc *pdc, int match)    /* checks probe hairpin bonds */
{
    PT_tprobes  *tprobe, *tprobe_next;
    int i;
    double      sbond;
    for (       tprobe = pdc->tprobes;
                tprobe;
                tprobe = tprobe_next ) {
        tprobe_next = tprobe->next;
        tprobe->seq_len = strlen(tprobe->sequence);
        sbond = 0.0;
        for (i=0;i<tprobe->seq_len;i++) {
            sbond += ptnd_check_max_bond(pdc,tprobe->sequence[i]);
        }
        tprobe->sum_bonds = sbond;
    }
    match = match;
}
/***********************************************************************
                        split the probes in probeparts
***********************************************************************/
static void ptnd_cp_tprobe_2_probepart(PT_pdc *pdc)
{
    PT_tprobes    *tprobe;
    PT_probeparts *parts;
    int            pos;
    int            probelen;

    while (pdc->parts) destroy_PT_probeparts(pdc->parts);
    for (       tprobe = pdc->tprobes;
                tprobe;
                tprobe = tprobe->next ) {
        probelen = strlen(tprobe->sequence);
        probelen -= DOMAIN_MIN_LENGTH;
        for (pos = 0; pos < probelen; pos ++ ) {
            parts = create_PT_probeparts();
            parts->sequence = strdup(tprobe->sequence+pos);
            parts->source = tprobe;
            parts->start = pos;
            aisc_link(&pdc->pparts, parts);
        }
    }
}
static void ptnd_duplikate_probepart_rek(PT_pdc *pdc, char *insequence, int deep, double dt,double sum_bonds, PT_probeparts *parts)
{
    PT_probeparts *newparts;
    int            base;
    int            i;
    double         max_bind;
    double         split;
    double         ndt,nsum_bonds;
    char          *sequence;

    sequence = strdup(insequence);
    if (deep >= PT_PART_DEEP) { /* now do it */
        newparts = create_PT_probeparts();
        newparts->sequence = sequence;
        newparts->source = parts->source;
        newparts->dt = dt;
        newparts->start = parts->start;
        newparts->sum_bonds = sum_bonds;
        aisc_link(&pdc->pdparts, newparts);
        return;
    }
    base = sequence[deep];
    max_bind = ptnd_check_max_bond(pdc, base);
    for (i = PT_A; i <=PT_T; i++) {
        sequence[deep] = i;
        if ( (split = ptnd_check_split(pdc, insequence, deep, i)) < 0.0) continue;
        /* this mismatch splits the probe in several domains */
        ndt = split + dt;
        nsum_bonds = sum_bonds+max_bind-split;
        ptnd_duplikate_probepart_rek(pdc,sequence,deep+1,ndt,nsum_bonds,parts);
    }
    free(sequence);
}
static void ptnd_duplikate_probepart(PT_pdc *pdc)
{
    PT_probeparts       *parts;
    for (parts = pdc->parts; parts; parts = parts->next)
        ptnd_duplikate_probepart_rek(pdc,parts->sequence,0,parts->dt,0.0, parts);

    while (( parts = pdc->parts ))
        destroy_PT_probeparts(parts);   /* delete the source */

    while (( parts = pdc->dparts ))
    {
        aisc_unlink((struct_dllheader_ext*)parts);
        aisc_link(&pdc->pparts, parts);
    }
}
/***********************************************************************
                sort the parts and check for duplicated parts
***********************************************************************/

static int ptnd_compare_parts(const void *PT_probeparts_ptr1, const void *PT_probeparts_ptr2, void*) {
    const PT_probeparts *tprobe1 = (const PT_probeparts*)PT_probeparts_ptr1;
    const PT_probeparts *tprobe2 = (const PT_probeparts*)PT_probeparts_ptr2;

    return strcmp(tprobe1->sequence,tprobe2->sequence);
}

static void ptnd_sort_parts(PT_pdc *pdc)
{
    PT_probeparts **my_list;
    int             list_len;
    PT_probeparts  *tprobe;
    int             i;

    if (!pdc->parts) return;
    list_len = get_TPROBEPARTS_CNT(pdc->parts);
    if (list_len <= 1) return;
    my_list = (PT_probeparts **)calloc(sizeof(void *),list_len);
    for (       i=0,    tprobe = pdc->parts;
                tprobe;
                i++,tprobe=tprobe->next     ) {
        my_list[i] = tprobe;
    }
    GB_sort((void **)my_list, 0, list_len, ptnd_compare_parts, 0);

    for (i=0;i<list_len;i++) {
        aisc_unlink((struct_dllheader_ext*)my_list[i]);
        aisc_link(&pdc->pparts, my_list[i]);
    }
    free((char *)my_list);
}
static void ptnd_remove_duplikated_probepart(PT_pdc *pdc)
{
    PT_probeparts       *parts, *parts_next;
    for (       parts = pdc->parts;
                parts;
                parts = parts_next) {
        parts_next = parts->next;
        if (parts_next) {
            if ( (parts->source== parts_next->source) && !strcmp(parts->sequence, parts_next->sequence)) {      /* equal sequence */
                if (parts->dt < parts_next->dt) {       /* delete higher dt */
                    destroy_PT_probeparts(parts_next);
                    parts_next = parts;
                }else{
                    destroy_PT_probeparts(parts);
                }
            }
        }
    }
}
/***********************************************************************
        test the probe parts, search the longest non mismatch string
***********************************************************************/
static void ptnd_check_part_inc_dt(PT_pdc *pdc, PT_probeparts *parts,
                                   int name, int apos, int rpos,
                                   double dt,double sum_bonds)
{
    PT_tprobes *tprobe = parts->source;
    double      ndt;
    int         pos;
    int         start;
    double      h;
    char       *probe;
    int         split;

    if (( start = parts->start )) {             /* look backwards */
        probe = parts->source->sequence;
        pos = rpos-1;
        start--;                        /* test the base left of start */
        split = 0;
        while (start>=0) {
            if (pos<0) break;   /* out of sight */
            h = ptnd_check_split(ptnd.pdc, probe, start,psg.data[name].data[pos]);
            if (h>0.0 && !split)        return; /* there is a longer part matching this */
            dt -= h;
            start --;
            pos --;
            split = 1;
        }
    }
    ndt = (dt * pdc->dt + (tprobe->sum_bonds - sum_bonds)*pdc->dte ) / tprobe->seq_len;
    pos = (int)ndt;

    pt_assert(pos >= 0);

    if (pos >= PERC_SIZE) return; /* out of observation */
    tprobe->perc[pos] ++;
    if (ptnd.new_match) {                       /* save the result in probematch */
        PT_probematch *match;
        if (psg.data[name].match) {
            if (psg.data[name].match->dt < ndt) return;
            /* there is a better hit for that sequence */
            match = psg.data[name].match;
        }else{
            match = create_PT_probematch();
            aisc_link(&ptnd.locs->ppm, match);
            psg.data[name].match = match;
        }
        match->name = name;
        match->b_pos = apos - parts->start;     /* thats not correct !!! */
        match->rpos = rpos-parts->start;
        match->N_mismatches = -1;       /* there are no mismatches in this mode */
        match->mismatches = -1;
        match->wmismatches = dt;        /* only weighted mismatches (maybe) */
        match->dt = ndt;
        match->sequence = psg.main_probe;
        match->reversed = psg.reversed;
    }
}
static int ptnd_check_inc_mode(PT_pdc *pdc ,PT_probeparts *parts,double dt,double sum_bonds)
{
    PT_tprobes *tprobe = parts->source;
    double      ndt    = (dt * pdc->dt + (tprobe->sum_bonds - sum_bonds)*pdc->dte ) / tprobe->seq_len;
    int         pos    = (int)ndt;

    pt_assert(pos >= 0);

    if (pos >= PERC_SIZE) return 1; /* out of observation */
    return 0;
}

struct ptnd_chain_check_part {
  ptnd_chain_check_part(int s) : split(s) {}
  int split;
  int operator() (int name, int apos, int rpos)
{
    char   *probe  = psg.probe;
    int     height = psg.height;
    double  sbond  = ptnd.sum_bonds;
    double  dt     = ptnd.dt;
    double  h      = 1.0;
    int     pos;
    int     base;

    if (!ptnd.new_match && psg.data[name].is_group) return 0;           /* dont count group or neverminds */
    if (probe) {
        pos = rpos+psg.height;
        while (probe[height] && (base = psg.data[name].data[pos])) {
            if (!split && (h = ptnd_check_split(ptnd.pdc, probe, height,
                                                base) < 0.0) ) {
                dt -= h;
                split = 1;
            }else{
                dt += h;
                sbond += ptnd_check_max_bond(ptnd.pdc,probe[height]) - h;
            }
            height++; pos++;
        }
    }
    ptnd_check_part_inc_dt(ptnd.pdc,ptnd.parts,name,apos,rpos,dt,sbond);
    return 0;
}
};

/* go down the tree to chains and leafs; check all  */
static void ptnd_check_part_all(POS_TREE *pt, double dt, double sum_bonds)
{
    int base;
    int name, apos, rpos;

    if (pt == NULL)
        return;
    if (PT_read_type(pt) == PT_NT_LEAF) {
        name = PT_read_name(psg.ptmain,pt);
        if (!ptnd.new_match && psg.data[name].is_group)         return;
        rpos = PT_read_rpos(psg.ptmain,pt);
        apos = PT_read_apos(psg.ptmain,pt);
        ptnd_check_part_inc_dt( ptnd.pdc, ptnd.parts,
                                name,apos,rpos,  dt, sum_bonds);
    }else if (PT_read_type(pt) == PT_NT_CHAIN) {
        psg.probe = 0;
        ptnd.dt = dt;
        ptnd.sum_bonds = sum_bonds;
        PT_read_chain(psg.ptmain,pt, ptnd_chain_check_part(0));
    }else{
        for (base = PT_QU; base< PT_B_MAX; base++) {
            ptnd_check_part_all(PT_read_son(psg.ptmain,pt,(PT_BASES)base),dt,sum_bonds);
        }
    }
}
/* search down the tree to find matching species for the given probe */
static void ptnd_check_part(char *probe, POS_TREE *pt,int  height, double dt, double sum_bonds, int split)
{
    int       name;
    int       i;
    POS_TREE *pthelp;
    int       rpos,apos,pos;
    double    ndt,nsum_bonds,h;
    int       nsplit;
    int       ref;

    if (!pt) return;
    if (dt/ptnd.parts->source->seq_len > PERC_SIZE) return;     /* out of scope */
    if (PT_read_type(pt) == PT_NT_NODE && probe[height]) {
        if (split && ptnd_check_inc_mode(ptnd.pdc ,ptnd.parts, dt, sum_bonds)) return;
        for (i=PT_A; i<PT_B_MAX; i++) {
            if (( pthelp = PT_read_son(psg.ptmain,pt, (PT_BASES)i) ))
            {
                nsplit = split;
                nsum_bonds = sum_bonds;
                if (height < PT_PART_DEEP) {
                    if ( i != probe[height] ) continue;
                    ndt = dt;
                }else{
                    if (split) {
                        h = ptnd_check_split(ptnd.pdc, probe, height,i);
                        if (h>0.0) ndt = dt+h; else ndt = dt-h;
                    }else       if ((h = ptnd_check_split(ptnd.pdc, probe, height,i)) < 0.0 ) {
                        ndt = dt - h;
                        nsplit = 1;
                    }else{
                        ndt = dt + h;
                        nsum_bonds +=
                            ptnd_check_max_bond(ptnd.pdc,probe[height]) - h;
                    }
                }
                if (nsplit && height <= DOMAIN_MIN_LENGTH) continue;
                ptnd_check_part(probe,pthelp,height+1,ndt,nsum_bonds,nsplit);
            }
        }
        return;
    }
    if (probe[height]) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            name = PT_read_name(psg.ptmain,pt);
            if (!ptnd.new_match && psg.data[name].is_group)     return;
            rpos = PT_read_rpos(psg.ptmain,pt);
            apos = PT_read_apos(psg.ptmain,pt);
            pos = rpos + height;
            if (pos + (int)(strlen(probe+height)) >= psg.data[name].size)               /* after end */
                return;
            while (probe[height] && (ref = psg.data[name].data[pos])) {
                if (split) {
                    h = ptnd_check_split(ptnd.pdc, probe, height,ref);
                    if (h<0.0) dt -= h; else dt += h;
                }else if ( (h = ptnd_check_split(ptnd.pdc, probe, height,
                                                 ref)) < 0.0 ) {
                    dt -= h;
                    split = 1;
                }else{
                    dt += h;
                    sum_bonds += ptnd_check_max_bond(ptnd.pdc,probe[height]) - h;
                }
                height++; pos++;
            }
            ptnd_check_part_inc_dt(     ptnd.pdc,ptnd.parts,
                                        name, apos, rpos, dt, sum_bonds);
            return;
        } else {                /* chain */
            psg.probe = probe;
            psg.height = height;
            ptnd.dt = dt;
            ptnd.sum_bonds = sum_bonds;
            PT_read_chain(psg.ptmain,pt, ptnd_chain_check_part(split));
            return;
        }
    }
    ptnd_check_part_all(pt,dt,sum_bonds);
}
static void ptnd_check_probepart(PT_pdc *pdc)
{
    PT_probeparts       *parts;
    ptnd.pdc = pdc;
    for (       parts = pdc->parts;
                parts;
                parts = parts->next) {
        ptnd.parts = parts;
        ptnd_check_part(parts->sequence, psg.pt, 0, parts->dt, parts->sum_bonds,0);
    }
}
/***********************************************************************
                        search for possible probes
***********************************************************************/
inline int ptnd_check_tprobe(PT_pdc *pdc, const char *probe, int len)
{
    int occ[PT_B_MAX] = { 0, 0, 0, 0, 0, 0 };

    int count = 0;
    while (count<len) {
        occ[safeCharIndex(probe[count++])]++;
    }
    int gc = occ[PT_G]+occ[PT_C];
    int at = occ[PT_A]+occ[PT_T];

    int all = gc+at;
    
    if (all < len) return 1; // there were some unwanted characters ('N' or '.')
    if (all < pdc->probelen) return 1;

    if (gc < pdc->min_gc*len) return 1;
    if (gc > pdc->max_gc*len) return 1;

    double temp = at*2 + gc*4;
    if (temp< pdc->mintemp) return 1;
    if (temp>pdc->maxtemp) return 1;

    return 0;
}

static long ptnd_build_probes_collect(const char *probe, long count, void*) {
    PT_tprobes *tprobe;
    if (count >= ptnd.locs->group_count*ptnd.pdc->mintarget) {
        tprobe = create_PT_tprobes();
        tprobe->sequence = strdup(probe);
        tprobe->temp = pt_get_temperature(probe);
        tprobe->groupsize = (int)count;
        aisc_link(&ptnd.pdc->ptprobes, tprobe);
    }
    return count;
}

inline void PT_incr_hash(GB_HASH *hash, char *sequence, int len) {
    char c        = sequence[len];
    sequence[len] = 0;

    pt_assert(strlen(sequence) == (size_t)len);
    
    GBS_incr_hash(hash, sequence);

    sequence[len] = c;
}

static void ptnd_add_sequence_to_hash(PT_pdc *pdc, GB_HASH *hash, char *sequence, int seq_len, int probe_len, char *prefix, int prefix_len) {
    int pos;
    if (*prefix) { // partition search, else very large hash tables (>60 mbytes)
        for (pos = seq_len-probe_len; pos >= 0; pos--, sequence++) {
            if ((*prefix!= *sequence || strncmp(sequence,prefix,prefix_len)) ) continue;
            if (!ptnd_check_tprobe(pdc,sequence, probe_len)) {
                PT_incr_hash(hash, sequence, probe_len);
            }
        }
    }
    else {
        for (pos = seq_len-probe_len; pos >= 0; pos--, sequence++) {
            if (!ptnd_check_tprobe(pdc,sequence, probe_len)) {
                PT_incr_hash(hash, sequence, probe_len);
            }
        }
    }
}

static long ptnd_collect_hash(const char *key, long val, void *gb_hash) {
    pt_assert(val);
    GBS_incr_hash((GB_HASH*)gb_hash, key);
    return val;
}

#if defined(DEVEL_RALF)
static long avoid_hash_warning(const char *, long , void*) { return 0; }
#endif // DEVEL_RALF

static void ptnd_build_tprobes(PT_pdc *pdc, int group_count) {
    int           *group_idx    = new int[group_count];
    unsigned long  datasize     = 0;                    // of marked species/genes
    unsigned long  maxseqlength = 0;                    // of marked species/genes

    // get group indices and count datasize of marked species
    {
        int used_idx = 0;
        for (int name = 0; name < psg.data_count; name++) {
            if(psg.data[name].is_group == 1) {
                group_idx[used_idx++]                  = name; // store marked group indices
                unsigned long size                     = psg.data[name].size;
                datasize                              += size;
                if (datasize<size) datasize            = ULONG_MAX; // avoid overflow!
                if (size > maxseqlength) maxseqlength  = size;
            }
        }
        pt_assert(used_idx == group_count);
    }

    unsigned long outer_hash_size;
    int           partsize = 0; // no partitions
    {
        const unsigned long max_allowed_hash_size = 1000000; // approx.
        const unsigned long max_allowed_tprobes   = (unsigned long)(max_allowed_hash_size*0.66); // results in 66% fill rate for hash (which is much!)

        // tests found about 5-8% tprobes (in relation to datasize) -> we use 10% here
        unsigned long estimated_no_of_tprobes = (unsigned long)((datasize-pdc->probelen+1)*0.10+0.5);

        while (estimated_no_of_tprobes > max_allowed_tprobes) {
            partsize++;
            estimated_no_of_tprobes /= 4; // 4 different bases
        }

        outer_hash_size = (unsigned long)(estimated_no_of_tprobes*1.5);

#if defined(DEBUG)
        printf("marked=%i datasize=%lu partsize=%i estimated_no_of_tprobes=%lu outer_hash_size=%lu\n",
               group_count, datasize, partsize, estimated_no_of_tprobes, outer_hash_size);
#endif // DEBUG
    }

#if defined(DEBUG)
    GBS_clear_hash_statistic_summary("outer");
#endif // DEBUG

    const int hash_multiply = 4; // hash fill ratio is 1/hash_multiply

    char partstring[256];
    PT_init_base_string_counter(partstring,PT_A,partsize);
    while (!PT_base_string_counter_eof(partstring)) {
#if defined(DEBUG)
        fputs("partition='", stdout);
        for (int i = 0; i<partsize; ++i) {
            putchar("ACGT"[partstring[i]-PT_A]);
        }
        fputs("'\n", stdout);
#endif // DEBUG

        GB_HASH *hash_outer = GBS_create_hash(outer_hash_size, GB_MIND_CASE); // count in how many groups/sequences the tprobe occurs (key = tprobe, value = counter)

#if defined(DEBUG)
        GBS_clear_hash_statistic_summary("inner");
#endif // DEBUG

        // for (name = 0; name < psg.data_count; name++) {
        // if(psg.data[name].is_group != 1) continue;
        for (int g = 0; g<group_count; ++g) {
            int      name             = group_idx[g];
            long     possible_tprobes = psg.data[name].size-pdc->probelen+1;
            GB_HASH *hash_one         = GBS_create_hash(possible_tprobes*hash_multiply, GB_MIND_CASE); // count tprobe occurrences for one group/sequence
            ptnd_add_sequence_to_hash(pdc, hash_one, psg.data[name].data, psg.data[name].size, pdc->probelen, partstring, partsize);
            GBS_hash_do_loop(hash_one, ptnd_collect_hash, hash_outer); // merge hash_one into hash
#if defined(DEBUG)
            GBS_calc_hash_statistic(hash_one, "inner", 0);
#endif // DEBUG
            GBS_free_hash(hash_one);
        }
        PT_sequence *seq;
        for ( seq = pdc->sequences; seq; seq = seq->next) {
            long     possible_tprobes = seq->seq.size-pdc->probelen+1;
            GB_HASH *hash_one         = GBS_create_hash(possible_tprobes*hash_multiply, GB_MIND_CASE); // count tprobe occurrences for one group/sequence
            ptnd_add_sequence_to_hash(pdc, hash_one, seq->seq.data, seq->seq.size, pdc->probelen, partstring, partsize);
            GBS_hash_do_loop(hash_one, ptnd_collect_hash, hash_outer); // merge hash_one into hash
#if defined(DEBUG)
            GBS_calc_hash_statistic(hash_one, "inner", 0);
#endif // DEBUG
            GBS_free_hash(hash_one);
        }

#if defined(DEBUG)
        GBS_print_hash_statistic_summary("inner");
#endif // DEBUG

        GBS_hash_do_loop(hash_outer, ptnd_build_probes_collect, NULL);

#if defined(DEBUG)
        GBS_calc_hash_statistic(hash_outer, "outer", 1);
#if defined(DEVEL_RALF)
        GBS_hash_do_loop(hash_outer, avoid_hash_warning, NULL); // hash is known to be overfilled - avoid assertion
#endif // DEVEL_RALF
#endif // DEBUG
        
        GBS_free_hash(hash_outer);
        PT_inc_base_string_count(partstring,PT_A,PT_B_MAX,partsize);
    }
#if defined(DEBUG)
    GBS_print_hash_statistic_summary("outer");
#endif // DEBUG
}

#if 0
static void ptnd_print_probes(PT_pdc *pdc) {
    PT_tprobes *tprobe;
    char        buffer[1024];
    int         i;

    for (       tprobe = pdc->tprobes;
                tprobe;
                tprobe = tprobe->next ) {
        strncpy(buffer,tprobe->sequence,250);
        buffer[250] = 0;
        PT_base_2_string(buffer,0);
        printf("seq: %s group %i  mishits: %i ",buffer,tprobe->groupsize,tprobe->mishit);
        for (i=0;i<PERC_SIZE;i++) {
            printf("%3i,",tprobe->perc[i]);
        }
        printf("\n");
    }
}
#endif

extern "C" int PT_start_design(PT_pdc *pdc, int /*dummy*/) {

    //  IDP probe design

    PT_local *locs   = (PT_local*)pdc->mh.parent->parent;
    ptnd.new_match   = 0;
    ptnd.locs        = locs;
    ptnd.pdc         = pdc;

    const char *error;
    {
        char *unknown_names = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error); /* read the names */
        if (unknown_names) free(unknown_names); // unused here (handled by PT_unknown_names)
    }

    if (error) {
        pt_export_error(locs, error);
        return 0;
    }

    PT_sequence *seq;
    for ( seq = pdc->sequences; seq; seq = seq->next) { // Convert all external sequence to internal format
        seq->seq.size = probe_compress_sequence(seq->seq.data, seq->seq.size);
        locs->group_count++;
    }

    if (locs->group_count <=0) {
        pt_export_error(locs, GBS_global_string("No %s marked - no probes designed",
                                                gene_flag ? "genes" : "species"));
        return 0;
    }

    while (pdc->tprobes) destroy_PT_tprobes(pdc->tprobes);
    ptnd_build_tprobes(pdc, locs->group_count);
    while (pdc->sequences) destroy_PT_sequence(pdc->sequences);
    ptnd_sort_probes_by(pdc,1);
    ptnd_first_check(pdc);
    ptnd_check_position(pdc);
    ptnd_check_bonds(pdc,ptnd.new_match);
    ptnd_cp_tprobe_2_probepart(pdc);
    ptnd_duplikate_probepart(pdc);
    ptnd_sort_parts(pdc);
    ptnd_remove_duplikated_probepart(pdc);
    ptnd_check_probepart(pdc);
    while (pdc->parts) destroy_PT_probeparts(pdc->parts);
    ptnd_calc_quality(pdc);
    ptnd_sort_probes_by(pdc,0);
    ptnd_probe_delete_all_but(pdc,pdc->clipresult);
    /* ptnd_print_probes(pdc); */

    return 0;
}

void ptnd_new_match(PT_local * locs, char *probestring)
{
    PT_pdc     *pdc = locs->pdc;
    PT_tprobes *tprobe;

    ptnd.locs      = locs;
    ptnd.pdc       = pdc;
    ptnd.new_match = 1;

    if (!pdc) return; /* no config */

    tprobe           = create_PT_tprobes();
    tprobe->sequence = strdup(probestring);

    aisc_link(&pdc->ptprobes, tprobe);
    ptnd_check_bonds(pdc,ptnd.new_match);
    ptnd_cp_tprobe_2_probepart(pdc);
    ptnd_duplikate_probepart(pdc);
    ptnd_sort_parts(pdc);
    ptnd_remove_duplikated_probepart(pdc);
    ptnd_check_probepart(pdc);
    
    while (pdc->parts)                  destroy_PT_probeparts(pdc->parts);
    while (( tprobe = pdc->tprobes ))   destroy_PT_tprobes(tprobe);
}
