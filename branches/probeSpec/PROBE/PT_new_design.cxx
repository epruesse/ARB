// =============================================================== //
//                                                                 //
//   File      : PT_new_design.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "probe.h"
#include <PT_server_prototypes.h>
#include <struct_man.h>
#include "probe_tree.h"
#include <arb_str.h>
#include <arb_defs.h>
#include <arb_sort.h>
#include "pt_prototypes.h"
#include <climits>
#include <set>
#include <stdint.h>


// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_tprobes *tprobe)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(tprobe)); }
inline void aisc_link(dll_public *dll, PT_probeparts *parts) { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(parts)); }
inline void aisc_link(dll_public *dll, PT_probematch *match) { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(match)); }

extern "C" {
    int pt_init_bond_matrix(PT_local *THIS)
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

static struct ptnd_loop_com {
    PT_pdc        *pdc;
    PT_local      *locs;
    PT_probeparts *parts;
    int            mishits;
    double         sum_bonds; // sum of bond of longest non mismatch string
    double         dt;     // sum of mismatches
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

    return strcmp(tprobe1->sequence, tprobe2->sequence);
}

static void ptnd_sort_probes_by(PT_pdc *pdc, int mode)  // mode 0 quality, mode 1 sequence
{
    PT_tprobes **my_list;
    int          list_len;
    PT_tprobes  *tprobe;
    int          i;

    if (!pdc->tprobes) return;
    list_len = pdc->tprobes->get_count();
    if (list_len <= 1) return;
    my_list = (PT_tprobes **)calloc(sizeof(void *), list_len);
    for (i=0, tprobe = pdc->tprobes;
         tprobe;
         i++, tprobe=tprobe->next)
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

    for (i=0; i<list_len; i++) {
        aisc_unlink(reinterpret_cast<dllheader_ext*>(my_list[i]));
        aisc_link(&pdc->ptprobes, my_list[i]);
    }
    free(my_list);
}
static void ptnd_probe_delete_all_but(PT_pdc *pdc, int count)
{
    PT_tprobes *tprobe;
    int         i;

    for (i=0, tprobe = pdc->tprobes;
         tprobe && i< count;
         i++, tprobe = tprobe->next) ;

    if (tprobe) {
        while (tprobe->next) {
            destroy_PT_tprobes(tprobe->next);
        }
    }
}
static int pt_get_gc_content(char *probe)
{
    int gc = 0;
    while (*probe) {
        if (*probe == PT_G ||
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
    while ((i=*(probe++))) {
        if (i == PT_G || i == PT_C) t+=4.0; else t += 2.0;
    }
    return t;
}

static void ptnd_calc_quality(PT_pdc *pdc) {
    PT_tprobes *tprobe;
    int         i;

    for (tprobe = pdc->tprobes;
         tprobe;
         tprobe = tprobe->next)
    {
        if (pdc->mismatches == 0)
        {
          for (i=0; i< PERC_SIZE-1; i++) {
              if (tprobe->perc[i] > tprobe->mishit) break;
          }
          tprobe->quality = ((double)tprobe->groupsize * i) + 1000.0/(1000.0 + tprobe->perc[i]);
        }
        else
        {
          int nMismatchWithIncreasingTemp = 0;

          for (i = 0 ; i < PERC_SIZE - 1 ; i++)
          {
              nMismatchWithIncreasingTemp += tprobe->perc[i];
              if (nMismatchWithIncreasingTemp > tprobe->mishit) break;
          }

          tprobe->quality = ((double)tprobe->groupsize) / (1 + tprobe->mishit) + 1000.0/(1000.0 + nMismatchWithIncreasingTemp);
        }
    }
}

static double ptnd_check_max_bond(PT_local *locs, char base) {
    //! check the bond val for a probe

    int complement = PT_complement(base);
    return locs->bond[(complement-(int)PT_A)*4 + base-(int)PT_A].val;
}

double ptnd_check_split(PT_local *locs, char *probe, int pos, char ref) {
    int    base       = probe[pos];
    int    complement = PT_complement(base);
    double max_bind   = locs->bond[(complement-(int)PT_A)*4 + base-(int)PT_A].val;
    double new_bind   = locs->bond[(complement-(int)PT_A)*4 + ref-(int)PT_A].val;

    // if (new_bind < locs->pdc->split)
    if (new_bind < locs->split)
        return new_bind-max_bind; // negative values indicate split
    // this mismatch splits the probe in several domains
    return (max_bind - new_bind);
}


// ----------------------------------
//      primary search for probes

struct ptnd_chain_count_mishits {
    int operator()(const DataLoc& probeLoc) {
        // count all mishits for a probe

        char *probe = psg.probe;
        psg.abs_pos.announce(probeLoc.apos);

        const probe_input_data& pid = psg.data[probeLoc.name];

        if (pid.outside_group()) {
            if (probe) {
                int rpos = probeLoc.rpos + psg.height;
                while (*probe && pid.get_data()[rpos]) {
                    if (pid.get_data()[rpos] != *(probe)) return 0;
                    probe++;
                    rpos++;
                }
            }
            ptnd.mishits++;
        }
        return 0;
    }
};

static int ptnd_count_mishits2(POS_TREE *pt) {
    //! go down the tree to chains and leafs; count the species that are in the non member group
    if (pt == NULL)
        return 0;

    if (PT_read_type(pt) == PT_NT_LEAF) {
        DataLoc loc(pt);
        psg.abs_pos.announce(loc.apos);
        return psg.data[loc.name].outside_group();
    }

    if (PT_read_type(pt) == PT_NT_CHAIN) {
        psg.probe = 0;
        ptnd.mishits = 0;
        PT_forwhole_chain(pt, ptnd_chain_count_mishits());
        return ptnd.mishits;
    }

    int mishits = 0;
    for (int base = PT_QU; base< PT_B_MAX; base++) {
        mishits += ptnd_count_mishits2(PT_read_son(pt, (PT_BASES)base));
    }
    return mishits;
}

inline char hitgroup_idx2char(int idx) {
    const int firstSmall = ALPHA_SIZE/2;
    int c;
    if (idx >= firstSmall) {
        c = 'a'+(idx-firstSmall);
    }
    else {
        c = 'A'+idx;
    }
    pt_assert(isalpha(c));
    return c;
}

char *get_design_info(PT_tprobes  *tprobe) {
    char   *buffer = (char *)GB_give_buffer(2000);
    char   *probe  = (char *)GB_give_buffer2(tprobe->seq_len + 10);
    PT_pdc *pdc    = (PT_pdc *)tprobe->mh.parent->parent;
    char   *p;
    int     i;
    int     sum;

    p = buffer;

    // target
    strcpy(probe, tprobe->sequence);
    PT_base_2_string(probe); // convert probe to real ASCII
    sprintf(p, "%-*s", pdc->probelen+1, probe);
    p += strlen(p);

    {
        int apos = info2bio(tprobe->apos);
        int c;
        int cs   = 0;

        // search nearest previous hit
        for (c=0; c<ALPHA_SIZE; c++) {
            if (!pdc->pos_groups[c]) { // new group
                pdc->pos_groups[c] = apos;
                cs                 = '=';
                break;
            }
            int dist = abs(apos - pdc->pos_groups[c]);
            if (dist < pdc->probelen) {
                apos = apos - pdc->pos_groups[c];
                if (apos > 0) {
                    cs = '+';
                    break;
                }
                cs = '-';
                apos = -apos;
                break;
            }
        }

        if (cs) {
            c  = hitgroup_idx2char(c);
        }
        else {
            // ran out of pos_groups
            c  = '?';
            cs = ' ';
        }

        // le apos ecol
        p += sprintf(p, "%2i %c%c%6i %4li ", tprobe->seq_len, c, cs,
                     apos,
                     PT_abs_2_rel(tprobe->apos+1)); // ecoli-bases inclusive apos ("bases before apos+1")
    }
    // grps
    p += sprintf(p, "%4i ", tprobe->groupsize);

    // mis
    sprintf(p,"%4i ",tprobe->mishit);
    p += strlen(p);

    // G+C
    p += sprintf(p, "%-4.1f ", ((double)pt_get_gc_content(tprobe->sequence))/tprobe->seq_len*100.0);

    // 4GC+2AT
    p += sprintf(p, "%-7.1f ", pt_get_temperature(tprobe->sequence));

    // probe string
    probe  = reverse_probe(tprobe->sequence);
    complement_probe(probe);
    PT_base_2_string(probe); // convert probe to real ASCII
    p     += sprintf(p, "%-*s |", pdc->probelen, probe);
    free(probe);

    // non-group hits by temp. decrease
    for (sum=i=0; i<PERC_SIZE; i++) {
        sum += tprobe->perc[i];
        p   += sprintf(p, "%3i,", sum);
    }

    return buffer;
}

#if defined(WARN_TODO)
#warning fix usage of strlen in get_design_info and add assertion vs buffer overflow
#endif

char *get_design_hinfo(PT_tprobes  *tprobe) {
    char   *buffer = (char *)GB_give_buffer(2000);
    char   *s      = buffer;
    PT_pdc *pdc;

    if (!tprobe) {
        return (char*)"Sorry, there are no probes for your selection !!!";
    }
    pdc = (PT_pdc *)tprobe->mh.parent->parent;

    {
        char *ecolipos = NULL;
        if (pdc->min_ecolipos == -1) {
            if (pdc->max_ecolipos == -1) {
                ecolipos = strdup("any");
            }
            else {
                ecolipos = GBS_global_string_copy("<= %i", pdc->max_ecolipos);
            }
        }
        else {
            if (pdc->max_ecolipos == -1) {
                ecolipos = GBS_global_string_copy(">= %i", pdc->min_ecolipos);
            }
            else {
                ecolipos = GBS_global_string_copy("%4i -%4i", pdc->min_ecolipos, pdc->max_ecolipos);
            }
        }

        sprintf(buffer,
                "Probe design Parameters:\n"
                "Length of probe    %4i\n"
                "Temperature        [%4.1f -%4.1f]\n"
                "GC-Content         [%4.1f -%4.1f]\n"
                "E.Coli Position    [%s]\n"
                "Max Non Group Hits   %4i\n"
                "Min Group Hits       %4.0f%%\n"
                "Allowable mismatches %4i\n",
                pdc->probelen,
                pdc->mintemp, pdc->maxtemp,
                pdc->min_gc*100.0, pdc->max_gc*100.0,
                ecolipos,
                pdc->mishit, pdc->mintarget*100.0,
                pdc->mismatches);

        free(ecolipos);
    }

    s += strlen(s);

    sprintf(s, "%-*s", pdc->probelen+1, "Target");
    s += strlen(s);

    sprintf(s, "%2s %8s %4s ", "le", "apos", "ecol");
    s += strlen(s);

    sprintf(s, "%4s ", "grps"); // groupsize
    s += strlen(s);

    sprintf(s,"%4s ","mis"); // mishits
    s += strlen(s);

    sprintf(s, "%-4s %-7s %-*s | ", " G+C", "4GC+2AT", pdc->probelen, "Probe sequence");
    s += strlen(s);

    sprintf(s, "Decrease T by n*.3C -> probe matches n non group species");

    return buffer;
}

static int ptnd_count_mishits(char *probe, POS_TREE *pt, int height) {
    //! search down the tree to find matching species for the given probe
    if (!pt) return 0;
    if (PT_read_type(pt) == PT_NT_NODE && *probe) {
        int mishits = 0;
        for (int i=PT_A; i<PT_B_MAX; i++) {
            if (i != *probe) continue;
            POS_TREE *pthelp = PT_read_son(pt, (PT_BASES)i);
            if (pthelp) mishits += ptnd_count_mishits(probe+1, pthelp, height+1);
        }
        return mishits;
    }
    if (*probe) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            const DataLoc loc(pt);
            int           pos = loc.rpos+height;

            if (pos + (int)(strlen(probe)) >= psg.data[loc.name].get_size())              // after end
                return 0;

            while (*probe) {
                if (psg.data[loc.name].get_data()[pos++] != *(probe++))
                    return 0;
            }
        }
        else {                // chain
            psg.probe = probe;
            psg.height = height;
            ptnd.mishits = 0;
            PT_forwhole_chain(pt, ptnd_chain_count_mishits());
            return ptnd.mishits;
        }
    }
    return ptnd_count_mishits2(pt);
}

static void ptnd_first_check(PT_pdc *pdc) {
    //! Check for direct mishits
    PT_tprobes  *tprobe, *tprobe_next;

    for (tprobe = pdc->tprobes;
         tprobe;
         tprobe = tprobe_next)
    {
        int nScaleMisHitTest = 1;

        tprobe_next = tprobe->next;

        psg.abs_pos.clear();

        if (pdc->mismatches == 0)
        {
            tprobe->mishit = ptnd_count_mishits(tprobe->sequence,psg.pt,0);
        }
        else
        {
            PT_probematch*  pm;
            aisc_string     seq;
            PT_local*       locs = (PT_local*)pdc->mh.parent->parent;
            std::set<int>   MatchSet;
            int             MatchKey;

            locs->pm_max      = pdc->mismatches;
            seq               = strdup(tprobe->sequence);
            tprobe->mishit    = 0;
            tprobe->groupsize = 0;

            PT_base_2_string(seq);

            probe_match(locs, seq);

            for (pm = locs->pm ; pm != 0 ; pm = pm->next)
            {
                psg.abs_pos.announce(pm->b_pos);

                // create a key out of the species id and check if it is in the
                // MatchSet. If not add it and count the match stats otherwise
                // ignore it.
                MatchKey = pm->name;

                if (MatchSet.find(MatchKey) == MatchSet.end())
                {
                    MatchSet.insert(MatchKey);

                    switch (pm->is_member)
                    {
                        case 1:
                        {
                            tprobe->groupsize++;
                            break;
                        }

                        case 0:
                        default:
                        {
                            tprobe->mishit++;
                            break;
                        }
                    }
                }
            }
        }

        if (tprobe->mishit > pdc->mishit)
        {
            destroy_PT_tprobes(tprobe);
        }
    }

    psg.abs_pos.clear();
}

static void ptnd_check_position(PT_pdc *pdc) {
    //! Check the probes position.
    PT_tprobes *tprobe, *tprobe_next;
    // if (pdc->min_ecolipos == pdc->max_ecolipos) return; // @@@ wtf was this for?

    for (tprobe = pdc->tprobes; tprobe; tprobe = tprobe_next) {
        tprobe_next = tprobe->next;
        long relpos = PT_abs_2_rel(tprobe->apos+1);
        if (relpos < pdc->min_ecolipos || (relpos > pdc->max_ecolipos && pdc->max_ecolipos != -1)) {
            destroy_PT_tprobes(tprobe);
        }
    }
}

static void ptnd_check_bonds(PT_local *locs) {
    /*! check the average bond size.
     *
     * @TODO  checks probe hairpin bonds
     */

    PT_pdc *pdc = locs->pdc;
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; ) {
        PT_tprobes *tprobe_next = tprobe->next;
        tprobe->seq_len = strlen(tprobe->sequence);
        double sbond = 0.0;
        for (int i=0; i<tprobe->seq_len; i++) {
            sbond += ptnd_check_max_bond(locs, tprobe->sequence[i]);
        }
        tprobe->sum_bonds = sbond;

        tprobe = tprobe_next;
    }
}

static void ptnd_cp_tprobe_2_probepart(PT_pdc *pdc) {
    //! split the probes into probeparts
    while (pdc->parts) destroy_PT_probeparts(pdc->parts);
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
        int probelen = strlen(tprobe->sequence)-DOMAIN_MIN_LENGTH;
        for (int pos = 0; pos < probelen; pos ++) {
            PT_probeparts *parts = create_PT_probeparts();

            parts->sequence = strdup(tprobe->sequence+pos);
            parts->source   = tprobe;
            parts->start    = pos;

            aisc_link(&pdc->pparts, parts);
        }
    }
}

static void ptnd_duplicate_probepart_rek(PT_local *locs, char *insequence, int deep, double dt, double sum_bonds, PT_probeparts *parts) {
    PT_probeparts *newparts;
    int            base;
    int            i;
    double         max_bind;
    double         split;
    double         ndt, nsum_bonds;
    char          *sequence;
    PT_pdc        *pdc = locs->pdc;

    sequence = strdup(insequence);
    if (deep >= PT_PART_DEEP) { // now do it
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
    max_bind = ptnd_check_max_bond(locs, base);
    for (i = PT_A; i <= PT_T; i++) {
        sequence[deep] = i;
        if ((split = ptnd_check_split(locs, insequence, deep, i)) < 0.0) continue;
        // this mismatch splits the probe in several domains
        ndt = split + dt;
        nsum_bonds = sum_bonds+max_bind-split;
        ptnd_duplicate_probepart_rek(locs, sequence, deep+1, ndt, nsum_bonds, parts);
    }
    free(sequence);
}

static void ptnd_duplicate_probepart(PT_local *locs) {
    PT_probeparts *parts;
    PT_pdc        *pdc = locs->pdc;
    for (parts = pdc->parts; parts; parts = parts->next)
        ptnd_duplicate_probepart_rek(locs, parts->sequence, 0, parts->dt, 0.0, parts);

    while ((parts = pdc->parts))
        destroy_PT_probeparts(parts);   // delete the source

    while ((parts = pdc->dparts))
    {
        aisc_unlink((dllheader_ext*)parts);
        aisc_link(&pdc->pparts, parts);
    }
}

static int ptnd_compare_parts(const void *PT_probeparts_ptr1, const void *PT_probeparts_ptr2, void*) {
    const PT_probeparts *tprobe1 = (const PT_probeparts*)PT_probeparts_ptr1;
    const PT_probeparts *tprobe2 = (const PT_probeparts*)PT_probeparts_ptr2;

    return strcmp(tprobe1->sequence, tprobe2->sequence);
}

static void ptnd_sort_parts(PT_pdc *pdc) {
    //! sort the parts and check for duplicated parts

    PT_probeparts **my_list;
    int             list_len;
    PT_probeparts  *tprobe;
    int             i;

    if (!pdc->parts) return;
    list_len = pdc->parts->get_count();
    if (list_len <= 1) return;
    my_list = (PT_probeparts **)calloc(sizeof(void *), list_len);
    for (i=0,           tprobe = pdc->parts;
                tprobe;
                i++, tprobe=tprobe->next) {
        my_list[i] = tprobe;
    }
    GB_sort((void **)my_list, 0, list_len, ptnd_compare_parts, 0);

    for (i=0; i<list_len; i++) {
        aisc_unlink((dllheader_ext*)my_list[i]);
        aisc_link(&pdc->pparts, my_list[i]);
    }
    free(my_list);
}
static void ptnd_remove_duplicated_probepart(PT_pdc *pdc)
{
    PT_probeparts       *parts, *parts_next;
    for (parts = pdc->parts;
                parts;
                parts = parts_next) {
        parts_next = parts->next;
        if (parts_next) {
            if ((parts->source == parts_next->source) && !strcmp(parts->sequence, parts_next->sequence)) {      // equal sequence
                if (parts->dt < parts_next->dt) {       // delete higher dt
                    destroy_PT_probeparts(parts_next);
                    parts_next = parts;
                }
                else {
                    destroy_PT_probeparts(parts);
                }
            }
        }
    }
}

static void ptnd_check_part_inc_dt(PT_pdc *pdc, PT_probeparts *parts, const DataLoc& matchLoc, double dt, double sum_bonds) {
    //! test the probe parts, search the longest non mismatch string

    PT_tprobes *tprobe = parts->source;
    int         start  = parts->start;
    if (start) {               // look backwards
        char *probe = parts->source->sequence;
        int   pos   = matchLoc.rpos-1;
        start--;                        // test the base left of start

        bool split = false;
        while (start>=0) {
            if (pos<0) break;   // out of sight

            double h = ptnd_check_split(ptnd.locs, probe, start, psg.data[matchLoc.name].get_data()[pos]);
            if (h>0.0 && !split) return; // there is a longer part matching this

            dt -= h;

            start--;
            pos--;
            split = true;
        }
    }
    double ndt = (dt * pdc->dt + (tprobe->sum_bonds - sum_bonds)*pdc->dte) / tprobe->seq_len;
    int    pos = (int)ndt;

    pt_assert(pos >= 0);

    if (pos >= PERC_SIZE) return; // out of observation
    tprobe->perc[pos] ++;
}
static int ptnd_check_inc_mode(PT_pdc *pdc, PT_probeparts *parts, double dt, double sum_bonds)
{
    PT_tprobes *tprobe = parts->source;
    double      ndt    = (dt * pdc->dt + (tprobe->sum_bonds - sum_bonds)*pdc->dte) / tprobe->seq_len;
    int         pos    = (int)ndt;

    pt_assert(pos >= 0);

    if (pos >= PERC_SIZE) return 1; // out of observation
    return 0;
}

struct ptnd_chain_check_part {
    int split;

    ptnd_chain_check_part(int s) : split(s) {}

    int operator() (const DataLoc& partLoc) {
        if (psg.data[partLoc.name].outside_group()) {
            char   *probe = psg.probe;
            double  sbond = ptnd.sum_bonds;
            double  dt    = ptnd.dt;

            if (probe) {
                int    pos    = partLoc.rpos+psg.height;
                double h      = 1.0;
                int    height = psg.height;
                int    base;

                while (probe[height] && (base = psg.data[partLoc.name].get_data()[pos])) {
                    if (!split && (h = ptnd_check_split(ptnd.locs, probe, height, base) < 0.0)) {
                        dt -= h;
                        split = 1;
                    }
                    else {
                        dt += h;
                        sbond += ptnd_check_max_bond(ptnd.locs, probe[height]) - h;
                    }
                    height++; pos++;
                }
            }
            ptnd_check_part_inc_dt(ptnd.pdc, ptnd.parts, partLoc, dt, sbond);
        }
        return 0;
    }
};

static void ptnd_check_part_all(POS_TREE *pt, double dt, double sum_bonds) {
    /*! go down the tree to chains and leafs;
     * check all (for what?)
     */

    if (pt) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            // @@@ dupped code is in ptnd_chain_check_part::operator()
            DataLoc loc(pt);
            if (psg.data[loc.name].outside_group()) {
                ptnd_check_part_inc_dt(ptnd.pdc, ptnd.parts, loc, dt, sum_bonds);
            }
        }
        else if (PT_read_type(pt) == PT_NT_CHAIN) {
            psg.probe = 0;
            ptnd.dt = dt;
            ptnd.sum_bonds = sum_bonds;
            PT_forwhole_chain(pt, ptnd_chain_check_part(0));
        }
        else {
            for (int base = PT_QU; base< PT_B_MAX; base++) {
                ptnd_check_part_all(PT_read_son(pt, (PT_BASES)base), dt, sum_bonds);
            }
        }
    }
}
static void ptnd_check_part(char *probe, POS_TREE *pt, int  height, double dt, double sum_bonds, int split) {
    //! search down the tree to find matching species for the given probe

    if (!pt) return;
    if (dt/ptnd.parts->source->seq_len > PERC_SIZE) return;     // out of scope
    if (PT_read_type(pt) == PT_NT_NODE && probe[height]) {
        if (split && ptnd_check_inc_mode(ptnd.pdc, ptnd.parts, dt, sum_bonds)) return;
        for (int i=PT_A; i<PT_B_MAX; i++) {
            POS_TREE *pthelp = PT_read_son(pt, (PT_BASES)i);
            if (pthelp) {
                int    nsplit     = split;
                double nsum_bonds = sum_bonds;
                double ndt;

                if (height < PT_PART_DEEP) {
                    if (i != probe[height]) continue;
                    ndt = dt;
                }
                else {
                    if (split) {
                        double h = ptnd_check_split(ptnd.locs, probe, height, i);
                        if (h>0.0) ndt = dt+h; else ndt = dt-h;
                    }
                    else {
                        double h = ptnd_check_split(ptnd.locs, probe, height, i);
                        if (h<0.0) {
                            ndt = dt - h;
                            nsplit = 1;
                        }
                        else {
                            ndt = dt + h;
                            nsum_bonds += ptnd_check_max_bond(ptnd.locs, probe[height]) - h;
                        }
                    }
                }
                if (nsplit && height <= DOMAIN_MIN_LENGTH) continue;
                ptnd_check_part(probe, pthelp, height+1, ndt, nsum_bonds, nsplit);
            }
        }
        return;
    }
    if (probe[height]) {
        if (PT_read_type(pt) == PT_NT_LEAF) {
            // @@@ dupped code is in ptnd_chain_check_part::operator()
            const DataLoc loc(pt);
            if (psg.data[loc.name].outside_group()) {
                int pos = loc.rpos + height;
                if (pos + (int)(strlen(probe+height)) >= psg.data[loc.name].get_size())               // after end
                    return;

                int ref;
                while (probe[height] && (ref = psg.data[loc.name].get_data()[pos])) {
                    if (split) {
                        double h = ptnd_check_split(ptnd.locs, probe, height, ref);
                        if (h<0.0) dt -= h; else dt += h;
                    }
                    else {
                        double h = ptnd_check_split(ptnd.locs, probe, height, ref);
                        if (h<0.0) {
                            dt -= h;
                            split = 1;
                        }
                        else {
                            dt += h;
                            sum_bonds += ptnd_check_max_bond(ptnd.locs, probe[height]) - h;
                        }
                    }
                    height++; pos++;
                }
                ptnd_check_part_inc_dt(ptnd.pdc, ptnd.parts, loc, dt, sum_bonds);
            }
            return;
        }
        else {                // chain
            psg.probe = probe;
            psg.height = height;
            ptnd.dt = dt;
            ptnd.sum_bonds = sum_bonds;
            PT_forwhole_chain(pt, ptnd_chain_check_part(split));
            return;
        }
    }
    ptnd_check_part_all(pt, dt, sum_bonds);
}
static void ptnd_check_probepart(PT_pdc *pdc)
{
    PT_probeparts       *parts;
    ptnd.pdc = pdc;
    for (parts = pdc->parts;
                parts;
                parts = parts->next) {
        ptnd.parts = parts;
        ptnd_check_part(parts->sequence, psg.pt, 0, parts->dt, parts->sum_bonds, 0);
    }
}
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
    if (count >= ptnd.locs->group_count*ptnd.pdc->mintarget) {
        PT_tprobes *tprobe = create_PT_tprobes();
        tprobe->sequence = strdup(probe);
        tprobe->temp = pt_get_temperature(probe);
        tprobe->groupsize = (int)count;
        aisc_link(&ptnd.pdc->ptprobes, tprobe);
    }
    return count;
}

inline void PT_incr_hash(GB_HASH *hash, const char *sequence, int len) {
    char c        = sequence[len];
    const_cast<char*>(sequence)[len] = 0;

    pt_assert(strlen(sequence) == (size_t)len);

    GBS_incr_hash(hash, sequence);

    const_cast<char*>(sequence)[len] = c;
}

static void ptnd_add_sequence_to_hash(PT_pdc *pdc, GB_HASH *hash, const char *sequence, int seq_len, int probe_len, char *prefix, int prefix_len) {
    int pos;
    if (*prefix) { // partition search, else very large hash tables (>60 mbytes)
        for (pos = seq_len-probe_len; pos >= 0; pos--, sequence++) {
            if ((*prefix != *sequence || strncmp(sequence, prefix, prefix_len))) continue;
            if (!ptnd_check_tprobe(pdc, sequence, probe_len)) {
                PT_incr_hash(hash, sequence, probe_len);
            }
        }
    }
    else {
        for (pos = seq_len-probe_len; pos >= 0; pos--, sequence++) {
            if (!ptnd_check_tprobe(pdc, sequence, probe_len)) {
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

#if defined(DEBUG)
#if defined(DEVEL_RALF)
static long avoid_hash_warning(const char *, long,  void*) { return 0; }
#endif // DEVEL_RALF
#endif

static void ptnd_build_tprobes(PT_pdc *pdc, int group_count) {
    // search for possible probes

    int           *group_idx    = new int[group_count];
    unsigned long  datasize     = 0;                    // of marked species/genes
    unsigned long  maxseqlength = 0;                    // of marked species/genes

    // get group indices and count datasize of marked species
    {
        int used_idx = 0;
        for (int name = 0; name < psg.data_count; name++) {
            const probe_input_data& pid = psg.data[name];

            if (pid.inside_group()) {
                group_idx[used_idx++] = name;                  // store marked group indices

                unsigned long size  = pid.get_size();
                datasize           += size;

                if (size<1 || size<(unsigned long)pdc->probelen) {
                    fprintf(stderr, "Warning: impossible design request for '%s' (contains only %lu bp)\n", pid.get_name(), size);
                }

                if (datasize<size) datasize           = ULONG_MAX;  // avoid overflow!
                if (size > maxseqlength) maxseqlength = size;
            }
        }
        pt_assert(used_idx == group_count);
    }

    int           partsize = 0;                     // no partitions
    unsigned long estimated_no_of_tprobes;
    {
        const unsigned long max_allowed_hash_size = 1000000; // approx.
        const unsigned long max_allowed_tprobes   = (unsigned long)(max_allowed_hash_size*0.66); // results in 66% fill rate for hash (which is much!)

        // tests found about 5-8% tprobes (in relation to datasize) -> we use 10% here
        estimated_no_of_tprobes = (unsigned long)((datasize-pdc->probelen+1)*0.10+0.5);

        while (estimated_no_of_tprobes > max_allowed_tprobes) {
            partsize++;
            estimated_no_of_tprobes /= 4; // 4 different bases
        }

#if defined(DEBUG)
        printf("marked=%i datasize=%lu partsize=%i estimated_no_of_tprobes=%lu\n",
               group_count, datasize, partsize, estimated_no_of_tprobes);
#endif // DEBUG
    }

#if defined(DEBUG)
    GBS_clear_hash_statistic_summary("outer");
#endif // DEBUG

    const int hash_multiply = 2; // resulting hash fill ratio is 1/(2*hash_multiply)

    char partstring[256];
    PT_init_base_string_counter(partstring, PT_A, partsize);

    while (!PT_base_string_counter_eof(partstring)) {
#if defined(DEBUG)
        fputs("partition='", stdout);
        for (int i = 0; i<partsize; ++i) {
            putchar("ACGT"[partstring[i]-PT_A]);
        }
        fputs("'\n", stdout);
#endif // DEBUG

        GB_HASH *hash_outer = GBS_create_hash(estimated_no_of_tprobes, GB_MIND_CASE); // count in how many groups/sequences the tprobe occurs (key = tprobe, value = counter)

#if defined(DEBUG)
        GBS_clear_hash_statistic_summary("inner");
#endif // DEBUG

        for (int g = 0; g<group_count; ++g) {
            int  name             = group_idx[g];
            long possible_tprobes = psg.data[name].get_size()-pdc->probelen+1;

            if (possible_tprobes<1) possible_tprobes = 1; // avoid wrong hash-size if no/not enough data

            GB_HASH *hash_one         = GBS_create_hash(possible_tprobes*hash_multiply, GB_MIND_CASE); // count tprobe occurrences for one group/sequence
            ptnd_add_sequence_to_hash(pdc, hash_one, psg.data[name].get_data(), psg.data[name].get_size(), pdc->probelen, partstring, partsize);
            GBS_hash_do_loop(hash_one, ptnd_collect_hash, hash_outer); // merge hash_one into hash
#if defined(DEBUG)
            GBS_calc_hash_statistic(hash_one, "inner", 0);
#endif // DEBUG
            GBS_free_hash(hash_one);
        }
        PT_sequence *seq;
        for (seq = pdc->sequences; seq; seq = seq->next) {
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
        PT_inc_base_string_count(partstring, PT_A, PT_B_MAX, partsize);
    }
    delete [] group_idx;
#if defined(DEBUG)
    GBS_print_hash_statistic_summary("outer");
#endif // DEBUG
}

int PT_start_design(PT_pdc *pdc, int /* dummy */) {

    //  IDP probe design

    PT_local *locs = (PT_local*)pdc->mh.parent->parent;

    ptnd.locs = locs;
    ptnd.pdc  = pdc;

    const char *error;
    {
        char *unknown_names = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error); // read the names
        if (unknown_names) free(unknown_names); // unused here (handled by PT_unknown_names)
    }

    if (error) {
        pt_export_error(locs, error);
        return 0;
    }

    PT_sequence *seq;
    for (seq = pdc->sequences; seq; seq = seq->next) {  // Convert all external sequence to internal format
        seq->seq.size = probe_compress_sequence(seq->seq.data, seq->seq.size);
        locs->group_count++;
    }

    if (locs->group_count <= 0) {
        pt_export_error(locs, GBS_global_string("No %s marked - no probes designed",
                                                gene_flag ? "genes" : "species"));
        return 0;
    }

    while (pdc->tprobes) destroy_PT_tprobes(pdc->tprobes);
    ptnd_build_tprobes(pdc, locs->group_count);
    while (pdc->sequences) destroy_PT_sequence(pdc->sequences);
    ptnd_sort_probes_by(pdc, 1);
    ptnd_first_check(pdc);
    ptnd_check_position(pdc);
    ptnd_check_bonds(locs);
    ptnd_cp_tprobe_2_probepart(pdc);
    ptnd_duplicate_probepart(locs);
    ptnd_sort_parts(pdc);
    ptnd_remove_duplicated_probepart(pdc);
    ptnd_check_probepart(pdc);
    while (pdc->parts) destroy_PT_probeparts(pdc->parts);
    ptnd_calc_quality(pdc);
    ptnd_sort_probes_by(pdc, 0);
    ptnd_probe_delete_all_but(pdc, pdc->clipresult);

    return 0;
}
