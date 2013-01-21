// =============================================================== //
//                                                                 //
//   File      : PT_new_design.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "pt_split.h"
#include <PT_server_prototypes.h>

#include <struct_man.h>
#include "probe_tree.h"
#include "PT_prefixIter.h"
#include "pt_probepart.h"

#include <arb_str.h>
#include <arb_defs.h>
#include <arb_sort.h>
#include <arb_strbuf.h>

#include <climits>

#if defined(DEBUG)
// #define DUMP_DESIGNED_PROBES 
#endif

#define MIN_DESIGN_PROBE_LENGTH DOMAIN_MIN_LENGTH

int Complement::calc_complement(int base) {
    switch (base) {
        case PT_A: return PT_T;
        case PT_C: return PT_G;
        case PT_G: return PT_C;
        case PT_T: return PT_A;
        default:   return base;
    }
}

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_tprobes *tprobe)   { aisc_link(reinterpret_cast<dllpublic_ext*>(dll), reinterpret_cast<dllheader_ext*>(tprobe)); }
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

struct dt_bondssum {
    double dt;        // sum of mismatches
    double sum_bonds; // sum of bonds of longest non mismatch string

    dt_bondssum(double dt_, double sum_bonds_) : dt(dt_), sum_bonds(sum_bonds_) {}
};

struct dt_bondssum_split : public dt_bondssum {
    bool split;

    dt_bondssum_split(const dt_bondssum& dtbs, bool split_) : dt_bondssum(dtbs), split(split_) {}
    dt_bondssum_split(double dt_, double sum_bonds_, bool split_) : dt_bondssum(dt_, sum_bonds_), split(split_) {}
};

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

static void sort_tprobes_by(PT_pdc *pdc, int mode)  // mode 0 quality, mode 1 sequence
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
static void clip_tprobes(PT_pdc *pdc, int count)
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

static void tprobes_calc_quality(PT_pdc *pdc) {
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
        int i;
        for (i=0; i< PERC_SIZE-1; i++) {
            if (tprobe->perc[i] > tprobe->mishit) break;
        }
        tprobe->quality = ((double)tprobe->groupsize * i) + 1000.0/(1000.0 + tprobe->perc[i]);
    }
}

static double ptnd_check_max_bond(const PT_local *locs, char base) {
    //! check the bond val for a probe

    int complement = get_complement(base);
    return locs->bond[(complement-(int)PT_A)*4 + base-(int)PT_A].val;
}

// ----------------------------------
//      primary search for probes

struct ptnd_chain_count_mishits {
    int         mishits;
    const char *probe;
    int         height;

    ptnd_chain_count_mishits() : mishits(0), probe(NULL), height(0) {}
    ptnd_chain_count_mishits(const char *probe_, int height_) : mishits(0), probe(probe_), height(height_) {}

    int operator()(const AbsLoc& probeLoc) {
        // count all mishits for a probe

        psg.abs_pos.announce(probeLoc.get_abs_pos());

        if (probeLoc.get_pid().outside_group()) {
            if (probe) {
                pt_assert(probe[0]); // if this case occurs, avoid entering this branch

                DataLoc         dataLoc(probeLoc);
                ReadableDataLoc readableLoc(dataLoc);
                for (int i = 0; probe[i] && readableLoc[height+i]; ++i) {
                    if (probe[i] != readableLoc[height+i]) return 0;
                }
            }
            mishits++;
        }
        return 0;
    }
};

static int count_mishits_for_all(POS_TREE2 *pt) {
    //! go down the tree to chains and leafs; count the species that are in the non member group
    if (pt == NULL)
        return 0;
    
    if (pt->is_leaf()) {
        DataLoc loc(pt);
        psg.abs_pos.announce(loc.get_abs_pos());
        return loc.get_pid().outside_group();
    }

    if (pt->is_chain()) {
        ptnd_chain_count_mishits counter;
        PT_forwhole_chain(pt, counter); // @@@ expand
        return counter.mishits;
    }

    int mishits = 0;
    for (int base = PT_QU; base< PT_BASES; base++) {
        mishits += count_mishits_for_all(PT_read_son(pt, (PT_base)base));
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

char *get_design_info(const PT_tprobes *tprobe) {
    const int  BUFFERSIZE = 2000;
    char      *buffer     = (char *)GB_give_buffer(BUFFERSIZE);
    PT_pdc    *pdc        = (PT_pdc *)tprobe->mh.parent->parent;
    char      *p          = buffer;

    // target
    {
        char *probe = (char *)GB_give_buffer2(tprobe->seq_len + 10);
        strcpy(probe, tprobe->sequence);

        probe_2_readable(probe, pdc->probelen); // convert probe to real ASCII
        p += sprintf(p, "%-*s", pdc->probelen+1, probe);
    }

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
                     PT_abs_2_ecoli_rel(tprobe->apos+1)); // ecoli-bases inclusive apos ("bases before apos+1")
    }

    p += sprintf(p, "%4i ", int(tprobe->quality+0.5));
    p += sprintf(p, "%4i ", tprobe->groupsize);
    p += sprintf(p, "%4.1f ", ((double)pt_get_gc_content(tprobe->sequence))/tprobe->seq_len*100.0); // G+C
    p += sprintf(p, "%7.1f ", pt_get_temperature(tprobe->sequence));                                // 4GC+2AT

    // probe string
    {
        char *probe  = create_reversed_probe(tprobe->sequence, tprobe->seq_len);
        complement_probe(probe, tprobe->seq_len);
        probe_2_readable(probe, tprobe->seq_len); // convert probe to real ASCII
        p     += sprintf(p, "%-*s |", pdc->probelen, probe);
        free(probe);
    }

    // non-group hits by temp. decrease
    int sum = 0;
    for (int i = 0; i<PERC_SIZE; i++) {
        sum += tprobe->perc[i];
        p   += sprintf(p, "%3i,", sum);
    }

    pt_assert((p-buffer)<BUFFERSIZE);

    return buffer;
}

#if defined(WARN_TODO)
#warning fix usage of strlen in get_design_info and add assertion vs buffer overflow
#endif

char *get_design_hinfo(const PT_tprobes *tprobe) {
    const int  BUFFERSIZE = 2000;
    char      *buffer     = (char *)GB_give_buffer(BUFFERSIZE);
    char      *s          = buffer;
    PT_pdc    *pdc;

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

        s += sprintf(s,
                     "Probe design Parameters:\n"
                     "Length of probe    %4i\n"
                     "Temperature        [%4.1f -%4.1f]\n"
                     "GC-Content         [%4.1f -%4.1f]\n"
                     "E.Coli Position    [%s]\n"
                     "Max Non Group Hits  %4i\n"
                     "Min Group Hits      %4.0f%%\n",
                     pdc->probelen,
                     pdc->mintemp, pdc->maxtemp,
                     pdc->min_gc*100.0, pdc->max_gc*100.0,
                     ecolipos,
                     pdc->mishit, pdc->mintarget*100.0);

        free(ecolipos);
    }

    s += sprintf(s, "%-*s", pdc->probelen+1, "Target");
    s += sprintf(s, "le     apos ecol qual grps  G+C 4GC+2AT ");
    s += sprintf(s, "%-*s | ", pdc->probelen, "Probe sequence");
    s += sprintf(s, "Decrease T by n*.3C -> probe matches n non group species");

    pt_assert((s-buffer)<BUFFERSIZE);

    return buffer;
}

static int count_mishits_for_matched(char *probe, POS_TREE2 *pt, int height) {
    //! search down the tree to find matching species for the given probe
    if (!pt) return 0;
    if (pt->is_node() && *probe) {
        int mishits = 0;
        for (int i=PT_A; i<PT_BASES; i++) {
            if (i != *probe) continue;
            POS_TREE2 *pthelp = PT_read_son(pt, (PT_base)i);
            if (pthelp) mishits += count_mishits_for_matched(probe+1, pthelp, height+1);
        }
        return mishits;
    }
    if (*probe) {
        if (pt->is_leaf()) {
            const ReadableDataLoc loc(pt);

            int pos = loc.get_rel_pos()+height;
            
            if (pos + (int)(strlen(probe)) >= loc.get_pid().get_size()) // after end // @@@ wrong check ? better return from loop below when ref is PT_QU
                return 0;

            for (int i = 0; probe[i] && loc[height+i]; ++i) {
                if (probe[i] != loc[height+i]) {
                    return 0;
                }
            }
        }
        else {                // chain
            ptnd_chain_count_mishits counter(probe, height);
            PT_forwhole_chain(pt, counter); // @@@ expand
            return counter.mishits;
        }
    }
    return count_mishits_for_all(pt);
}

static void remove_tprobes_with_too_many_mishits(PT_pdc *pdc) {
    //! Check for direct mishits

    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; ) {
        PT_tprobes *tprobe_next = tprobe->next;

        psg.abs_pos.clear();
        tprobe->mishit = count_mishits_for_matched(tprobe->sequence, psg.TREE_ROOT2(), 0);
        tprobe->apos   = psg.abs_pos.get_most_used();
        if (tprobe->mishit > pdc->mishit) {
            destroy_PT_tprobes(tprobe);
        }
        tprobe = tprobe_next;
    }
    psg.abs_pos.clear();
}

static void remove_tprobes_outside_ecoli_range(PT_pdc *pdc) {
    //! Check the probes position.
    // if (pdc->min_ecolipos == pdc->max_ecolipos) return; // @@@ wtf was this for?  

    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; ) {
        PT_tprobes *tprobe_next = tprobe->next;
        long relpos = PT_abs_2_ecoli_rel(tprobe->apos+1);
        if (relpos < pdc->min_ecolipos || (relpos > pdc->max_ecolipos && pdc->max_ecolipos != -1)) {
            destroy_PT_tprobes(tprobe);
        }
        tprobe = tprobe_next;
    }
}

static void tprobes_calculate_bonds(PT_local *locs) {
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

static void ptnd_cp_tprobe_2_probepart(Parts& parts, PT_pdc *pdc) {
    //! split the probes into probeparts
    pt_assert(parts.empty());
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
#if defined(DUMP_PROBEPARTS)
        SmartCharPtr tprobe_readable = readable_probe(tprobe->sequence, tprobe->seq_len, 'T');
#endif
        int probelen = strlen(tprobe->sequence)-DOMAIN_MIN_LENGTH;
        for (int pos = 0; pos < probelen; pos ++) {
            ProbePart part(SmartCharPtr(strdup(tprobe->sequence+pos)), *tprobe, pos);

#if defined(DUMP_PROBEPARTS)
            SmartCharPtr part_readable = part.get_readable_sequence();
            fprintf(stderr, "[tprobe='%s'] probepart='%s'\n", &*tprobe_readable, &*part_readable);
#endif
            parts.push_back(part);
        }
    }
}

static void ptnd_duplicate_probepart_rek(PT_local *locs, const Splits& splits, const char *insequence, int deep, const dt_bondssum& dtbs, ProbePart& part2dup, Parts& result) {
    char *sequence = strdup(insequence);

    if (deep >= PT_PART_DEEP) { // now do it
        ProbePart part(sequence, part2dup);
        part.set_dt_and_bonds(dtbs.dt, dtbs.sum_bonds);
        result.push_back(part);
    }
    else {
        int    base     = sequence[deep];
        double max_bind = ptnd_check_max_bond(locs, base);

        for (int i = PT_A; i <= PT_T; i++) {
            sequence[deep] = i;
            double split   = splits.check(insequence[deep], i);
            if (split >= 0.0) { // this mismatch splits the probe in several domains
                dt_bondssum dtbs_sub(split+dtbs.dt, dtbs.sum_bonds+max_bind-split);
                ptnd_duplicate_probepart_rek(locs, splits, sequence, deep+1, dtbs_sub, part2dup, result);
            }
        }
        free(sequence);
    }
}

static void ptnd_duplicate_probepart(PT_local *locs, const Splits& splits, Parts& parts) {
    Parts duplicates;
    for (PartsIter p = parts.begin(); p != parts.end(); ++p) {
        ProbePart& part = *p;
        ptnd_duplicate_probepart_rek(locs, splits, &*part.get_sequence(), 0, dt_bondssum(part.get_dt(), 0.0), part, duplicates);
    }
    parts = duplicates;
}

inline bool partsLess(const ProbePart& p1, const ProbePart& p2) {
    return p1.seq_less(p2);
}
inline void ptnd_sort_parts(Parts& parts) {
    parts.sort(partsLess);
}

static void ptnd_remove_duplicated_probepart(Parts& parts) {
    PartsIter end = parts.end();
    PartsIter p1  = parts.begin();

    while (p1 != end) {
        PartsIter p2 = p1;
        if (++p2 == end) break;

        if (p1->is_same_part_as(*p2)) {
            // delete higher dt
            if (p1->get_dt() < p2->get_dt()) {
                parts.erase(p2);
                p2 = p1;
            }
            else {
                parts.erase(p1);
            }
        }
        p1 = p2;
        ++p2;
    }
}

class CentigradePos {
    typedef std::map<int, int> Pos4Name;

    Pos4Name cpos; // key = outgroup-species-id; value = min. centigrade position for (partial) probe

public:

    void set_pos_for(int name, int pos) {
        Pos4Name::iterator found = cpos.find(name);
        if (found == cpos.end()) {
            cpos[name] = pos;
        }
        else if (pos<found->second) {
            found->second = pos;
        }
    }

    void summarize_centigrade_hits(int *centigrade_hits) const {
        for (int i = 0; i<PERC_SIZE; ++i) centigrade_hits[i] = 0;
        for (Pos4Name::const_iterator p = cpos.begin(); p != cpos.end(); ++p) {
            int pos  = p->second;
            pt_assert(pos<PERC_SIZE); // otherwise it should not be put into CentigradePos
            centigrade_hits[pos]++;
        }
    }
};

typedef std::map<PT_tprobes*, CentigradePos> TprobeCentPos;

class PartCheck_Traversal {
    const PT_local *const locs;
    const PT_pdc   *const pdc;

    Splits         splits;
    PartsIter      currPart;
    CentigradePos *currCentigrade; // = CentigradePos for tprobe of 'currPart'

    void setCentigradePos(int name, int centPos) {
        pt_assert(centPos >= 0 && centPos<PERC_SIZE);
        currCentigrade->set_pos_for(name, centPos);
    }

    int calc_centigrade_pos(const dt_bondssum& dtbs) const {
        const PT_tprobes *tprobe = &currPart->get_source();

        double ndt = (dtbs.dt * pdc->dt + (tprobe->sum_bonds - dtbs.sum_bonds)*pdc->dte) / tprobe->seq_len;
        int    pos = (int)ndt;

        pt_assert(pos >= 0);
        return pos;
    }
    bool centigrade_pos_outofscope(const dt_bondssum& dtbs) const {
        return calc_centigrade_pos(dtbs) >= PERC_SIZE;
    }

    void check_part_inc_dt_backwards(const AbsLoc& absLoc, const dt_bondssum& dtbs) {
        int start = currPart->get_start();
        pt_assert(start);

        // look backwards
        DataLoc matchLoc(absLoc);

        char *probe = currPart->get_source().sequence;
        int   pos   = matchLoc.get_rel_pos()-1;
        start--;                        // test the base left of start

        SmartCharPtr  seqPtr = matchLoc.get_pid().get_dataPtr();
        const char   *seq    = &*seqPtr;

        dt_bondssum_split ndtbss(dtbs, false);
        while (start>=0) {
            if (pos<0) break;   // out of sight

            double h = get_splits().check(probe[start], seq[pos]);
            if (h>0.0 && !ndtbss.split) return; // there is a longer part matching this

            ndtbss.dt -= h;

            start--;
            pos--;
            ndtbss.split = true;
        }

        int cpos = calc_centigrade_pos(ndtbss);
        if (cpos<PERC_SIZE) {
            setCentigradePos(absLoc.get_name(), cpos);
        }
    }

    void check_part_inc_dt_basic(const AbsLoc& absLoc, const dt_bondssum& dtbs) {
        pt_assert(!currPart->get_start());
        int cpos = calc_centigrade_pos(dtbs);
        if (cpos<PERC_SIZE) {
            setCentigradePos(absLoc.get_name(), cpos);
        }
    }

    void check_part_inc_dt(const AbsLoc& absLoc, const dt_bondssum& dtbs) {
        //! test the probe parts, search the longest non mismatch string
        // (Note: modifies 'dtbs')

        int start = currPart->get_start();
        if (start) check_part_inc_dt_backwards(absLoc, dtbs);
        else check_part_inc_dt_basic(absLoc, dtbs);
    }

    void chain_check_part_no_probe(const AbsLoc& absLoc, const dt_bondssum_split& dtbss) {
        pt_assert(absLoc.get_pid().outside_group());
        check_part_inc_dt(absLoc, dtbss);
    }

    void chain_check_part(const AbsLoc& absLoc, const dt_bondssum_split& dtbss, const char *probe, int height) {
        pt_assert(absLoc.get_pid().outside_group());
        dt_bondssum_split ndtbss(dtbss);

        pt_assert(probe);

        DataLoc partLoc(absLoc);

        int    spos = partLoc.get_rel_pos()+height;
        int    ppos = height;
        double h    = 1.0;
        int    base;

        SmartCharPtr  seqPtr = partLoc.get_pid().get_dataPtr();
        const char   *seq    = &*seqPtr;

        while (probe[ppos] && (base = seq[spos])) {
            h = get_splits().check(probe[ppos], base);

            if (ndtbss.split) {
                if (h<0.0) ndtbss.dt -= h; else ndtbss.dt += h;
            }
            else {
                if (h<0.0) {
                    ndtbss.dt    -= h;
                    ndtbss.split  = 1;
                }
                else {
                    ndtbss.dt        += h;
                    ndtbss.sum_bonds += ptnd_check_max_bond(locs, probe[ppos]) - h;
                }
            }
            ppos++; spos++;
        }
        check_part_inc_dt(absLoc, ndtbss);
    }

    void check_part_all(POS_TREE2 *pt, const dt_bondssum& dtbs) {
        /*! go down the tree to chains and leafs;
         * check all (for what?)
         */

        if (pt) {
            if (pt->is_leaf()) {
                // @@@ dupped code is in chain_check_part
                DataLoc loc(pt);
                if (loc.get_pid().outside_group()) {
                    check_part_inc_dt(loc, dtbs);
                }
            }
            else if (pt->is_chain()) {
                ChainIteratorStage2 entry(pt);
                do {
                    const AbsLoc& absLoc = entry.at();
                    if (absLoc.get_pid().outside_group()) {
                        chain_check_part_no_probe(entry.at(), dt_bondssum_split(dtbs, 0));
                    }
                } while (++entry);
            }
            else {
                for (int base = PT_QU; base< PT_BASES; base++) {
                    check_part_all(PT_read_son(pt, (PT_base)base), dtbs);
                }
            }
        }
    }

    void check_part(const char *probe, POS_TREE2 *pt, int height, const dt_bondssum_split& dtbss) {
        //! search down the tree to find matching species for the given probe

        if (!pt) return;
        if (dtbss.dt/currPart->get_source().seq_len > PERC_SIZE) return;     // out of scope
        if (pt->is_node() && probe[height]) {
            if (dtbss.split && centigrade_pos_outofscope(dtbss)) return;
            for (int i=PT_A; i<PT_BASES; i++) {
                POS_TREE2 *pthelp = PT_read_son(pt, (PT_base)i);
                if (pthelp) {
                    dt_bondssum_split ndtbss(dtbss);

                    if (height < PT_PART_DEEP) {
                        if (i != probe[height]) continue;
                    }
                    else {
                        double h = get_splits().check(probe[height], i);
                        if (ndtbss.split) {
                            if (h>0.0) ndtbss.dt += h; else ndtbss.dt -= h;
                        }
                        else {
                            if (h<0.0) {
                                ndtbss.dt    -= h;
                                ndtbss.split  = true;
                            }
                            else {
                                ndtbss.dt        += h;
                                ndtbss.sum_bonds += ptnd_check_max_bond(locs, probe[height]) - h;
                            }
                        }
                    }
                    if (!ndtbss.split || height > DOMAIN_MIN_LENGTH) {
                        check_part(probe, pthelp, height+1, ndtbss);
                    }
                }
            }
            return;
        }
        if (probe[height]) {
            if (pt->is_leaf()) {
                // @@@ dupped code is in chain_check_part
                const DataLoc loc(pt);
                if (loc.get_pid().outside_group()) {
                    int pos = loc.get_rel_pos() + height;
                    if (pos + (int)(strlen(probe+height)) >= loc.get_pid().get_size()) // after end
                        return;

                    int ref;

                    SmartCharPtr  seqPtr = loc.get_pid().get_dataPtr();
                    const char   *seq    = &*seqPtr;

                    dt_bondssum_split ndtbss(dtbss);

                    while (probe[height] && (ref = seq[pos])) {
                        double h = get_splits().check(probe[height], ref);
                        if (ndtbss.split) {
                            if (h<0.0) ndtbss.dt -= h; else ndtbss.dt += h;
                        }
                        else {
                            if (h<0.0) {
                                ndtbss.dt -= h;
                                ndtbss.split = 1;
                            }
                            else {
                                ndtbss.dt += h;
                                ndtbss.sum_bonds += ptnd_check_max_bond(locs, probe[height]) - h;
                            }
                        }
                        height++; pos++;
                    }
                    check_part_inc_dt(loc, ndtbss);
                }
            }
            else { // chain
                ChainIteratorStage2 entry(pt);
                do {
                    const AbsLoc& absLoc = entry.at();
                    if (absLoc.get_pid().outside_group()) {
                        chain_check_part(entry.at(), dtbss, probe, height);
                    }
                } while (++entry);
            }
        }
        else {
            check_part_all(pt, dtbss);
        }
    }

public:
    PartCheck_Traversal(PT_local *locs_, PT_pdc *pdc_)
        : locs(locs_),
          pdc(pdc_),
          splits(locs)
    {
    }

    const Splits& get_splits() const { return splits; }

    void check_probeparts(Parts& parts, TprobeCentPos& tcp) {
        currPart      = parts.begin();
        PartsIter end = parts.end();

        for (; currPart != end; ++currPart) {
            TprobeCentPos::iterator tcp4part = tcp.find(&currPart->get_source());
            pt_assert(tcp4part != tcp.end()); // missing CentigradePos for tprobe of 'currPart'

            currCentigrade = &tcp4part->second;

            dt_bondssum_split dtbss(currPart->get_dt(), currPart->get_sum_bonds(), 0);
            check_part(&*currPart->get_sequence(), psg.TREE_ROOT2(), 0, dtbss);
        }
    }
};

inline int ptnd_check_tprobe(PT_pdc *pdc, const char *probe, int len)
{
    int occ[PT_BASES] = { 0, 0, 0, 0, 0, 0 };

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

struct probes_collect_data {
    PT_pdc *pdc;
    long    min_count;

    probes_collect_data(PT_pdc *pdc_, int group_count)
        : pdc(pdc_),
          min_count(group_count*pdc->mintarget)
    {}
};

static long ptnd_build_probes_collect(const char *probe, long count, void *cl_collData) {
    probes_collect_data *collect_data = static_cast<probes_collect_data*>(cl_collData);

    if (count >= collect_data->min_count) {
        PT_tprobes *tprobe = create_PT_tprobes();
        tprobe->sequence = strdup(probe);
        tprobe->temp = pt_get_temperature(probe);
        tprobe->groupsize = (int)count;
        aisc_link(&collect_data->pdc->ptprobes, tprobe);
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

inline void ptnd_add_sequence_to_hash(PT_pdc *pdc, GB_HASH *hash, const char *sequence, int seq_len, int probe_len, const PrefixIterator& prefix) {
    for (int pos = seq_len-probe_len; pos >= 0; pos--, sequence++) {
        if (prefix.matches_at(sequence)) {
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
                    fprintf(stderr, "Warning: impossible design request for '%s' (contains only %lu bp)\n", pid.get_shortname(), size);
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

#if defined(DEBUG) && 0
    partsize = 2;
    fprintf(stderr, "OVERRIDE: forcing partsize=%i\n", partsize);
#endif

    PrefixIterator design4prefix(PT_A, PT_T, partsize);
    while (!design4prefix.done()) {
#if defined(DEBUG)
        {
            char *prefix = probe_2_readable(design4prefix.copy(), design4prefix.length());
            fprintf(stderr, "partition='%s'\n", prefix);
            free(prefix);
        }
#endif // DEBUG
        GB_HASH *hash_outer = GBS_create_hash(estimated_no_of_tprobes, GB_MIND_CASE); // count in how many groups/sequences the tprobe occurs (key = tprobe, value = counter)

#if defined(DEBUG)
        GBS_clear_hash_statistic_summary("inner");
#endif // DEBUG

        for (int g = 0; g<group_count; ++g) {
            const probe_input_data& pid = psg.data[group_idx[g]];

            long possible_tprobes = pid.get_size()-pdc->probelen+1;
            if (possible_tprobes<1) possible_tprobes = 1; // avoid wrong hash-size if no/not enough data

            GB_HASH      *hash_one = GBS_create_hash(possible_tprobes*hash_multiply, GB_MIND_CASE);    // count tprobe occurrences for one group/sequence
            SmartCharPtr  seqPtr   = pid.get_dataPtr();
            ptnd_add_sequence_to_hash(pdc, hash_one, &*seqPtr, pid.get_size(), pdc->probelen, design4prefix);

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
            ptnd_add_sequence_to_hash(pdc, hash_one, seq->seq.data, seq->seq.size, pdc->probelen, design4prefix);
            GBS_hash_do_loop(hash_one, ptnd_collect_hash, hash_outer); // merge hash_one into hash
#if defined(DEBUG)
            GBS_calc_hash_statistic(hash_one, "inner", 0);
#endif // DEBUG
            GBS_free_hash(hash_one);
        }

#if defined(DEBUG)
        GBS_print_hash_statistic_summary("inner");
#endif // DEBUG

        {
            probes_collect_data collData(pdc, group_count);
            GBS_hash_do_loop(hash_outer, ptnd_build_probes_collect, &collData);
        }

#if defined(DEBUG)
        GBS_calc_hash_statistic(hash_outer, "outer", 1);
#if defined(DEVEL_RALF)
        GBS_hash_do_loop(hash_outer, avoid_hash_warning, NULL); // hash is known to be overfilled - avoid assertion
#endif // DEVEL_RALF
#endif // DEBUG

        GBS_free_hash(hash_outer);
        ++design4prefix;
    }
    delete [] group_idx;
#if defined(DEBUG)
    GBS_print_hash_statistic_summary("outer");
#endif // DEBUG
}

#if defined(DUMP_DESIGNED_PROBES)
static void dump_tprobe(PT_tprobes *tprobe, int idx) {
    char *readable = readable_probe(tprobe->sequence, tprobe->seq_len, 'T');
    fprintf(stderr, "tprobe='%s' idx=%i len=%i\n", readable, idx, tprobe->seq_len);
    free(readable);
}
static void DUMP_TPROBES(const char *where, PT_pdc *pdc) {
    int idx = 0;
    fprintf(stderr, "dumping tprobes %s:\n", where);
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
        dump_tprobe(tprobe, idx++);
    }
}
#else
#define DUMP_TPROBES(a,b)
#endif

int PT_start_design(PT_pdc *pdc, int /* dummy */) {
    PT_local *locs = (PT_local*)pdc->mh.parent->parent;

    const char *error;
    {
        char *unknown_names = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error); // read the names
        if (unknown_names) free(unknown_names); // unused here (handled by PT_unknown_names)
    }

    if (error) {
        pt_export_error(locs, error);
        return 0;
    }

    PT_sequence *seq; // @@@ move into for loop?
    for (seq = pdc->sequences; seq; seq = seq->next) {  // Convert all external sequence to internal format
        seq->seq.size = probe_compress_sequence(seq->seq.data, seq->seq.size);
        locs->group_count++;
    }

    if (locs->group_count <= 0) {
        pt_export_error(locs, GBS_global_string("No %s marked - no probes designed", gene_flag ? "genes" : "species"));
        return 0;
    }

    if (pdc->probelen < MIN_DESIGN_PROBE_LENGTH) {
        pt_export_error(locs, GBS_global_string("Probe length %i is below the minimum probe length of %i", pdc->probelen, MIN_DESIGN_PROBE_LENGTH));
        return 0;
    }

    while (pdc->tprobes) destroy_PT_tprobes(pdc->tprobes);

    ptnd_build_tprobes(pdc, locs->group_count);
    while (pdc->sequences) destroy_PT_sequence(pdc->sequences);

    sort_tprobes_by(pdc, 1);
    remove_tprobes_with_too_many_mishits(pdc);
    remove_tprobes_outside_ecoli_range(pdc);
    tprobes_calculate_bonds(locs);
    DUMP_TPROBES("after tprobes_calculate_bonds", pdc);

    {
        Parts probeparts;
        ptnd_cp_tprobe_2_probepart(probeparts, pdc);

        PartCheck_Traversal pct(locs, pdc);

        ptnd_duplicate_probepart(locs, pct.get_splits(), probeparts);
        ptnd_sort_parts(probeparts);
        ptnd_remove_duplicated_probepart(probeparts);

        // track minimum outgroup centigrade position for each tprobe
        TprobeCentPos tcp;
        for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
            tcp[tprobe] = CentigradePos();
        }

        pct.check_probeparts(probeparts, tcp);

        // set perc[] of all tprobes
        for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
            TprobeCentPos::iterator cpos = tcp.find(tprobe);
            pt_assert(cpos != tcp.end());

            cpos->second.summarize_centigrade_hits(tprobe->perc);
        }
    }

    tprobes_calc_quality(pdc);
    sort_tprobes_by(pdc, 0);
    clip_tprobes(pdc, pdc->clipresult);

#if defined(DEBUG)
    sort_tprobes_by(pdc, 1); // @@@ remove me - just here make upcoming changes more obvious
#endif

    return 0;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

inline const char *concat_iteration(PrefixIterator& prefix) {
    static GBS_strstruct out(50);

    out.erase();

    while (!prefix.done()) {
        if (out.filled()) out.put(',');

        char *readable = probe_2_readable(prefix.copy(), prefix.length());
        out.cat(readable);
        free(readable);

        ++prefix;
    }

    return out.get_data();
}

void TEST_PrefixIterator() {
    // straight-forward permutation
    PrefixIterator p0(PT_A, PT_T, 0); TEST_EXPECT_EQUAL(p0.steps(), 1);
    PrefixIterator p1(PT_A, PT_T, 1); TEST_EXPECT_EQUAL(p1.steps(), 4);
    PrefixIterator p2(PT_A, PT_T, 2); TEST_EXPECT_EQUAL(p2.steps(), 16);
    PrefixIterator p3(PT_A, PT_T, 3); TEST_EXPECT_EQUAL(p3.steps(), 64);

    TEST_EXPECT_EQUAL(p1.done(), false);
    TEST_EXPECT_EQUAL(p0.done(), false);

    TEST_EXPECT_EQUAL(concat_iteration(p0), "");
    TEST_EXPECT_EQUAL(concat_iteration(p1), "A,C,G,U");
    TEST_EXPECT_EQUAL(concat_iteration(p2), "AA,AC,AG,AU,CA,CC,CG,CU,GA,GC,GG,GU,UA,UC,UG,UU");

    // permutation truncated at PT_QU
    PrefixIterator q0(PT_QU, PT_T, 0); TEST_EXPECT_EQUAL(q0.steps(), 1);
    PrefixIterator q1(PT_QU, PT_T, 1); TEST_EXPECT_EQUAL(q1.steps(), 6);
    PrefixIterator q2(PT_QU, PT_T, 2); TEST_EXPECT_EQUAL(q2.steps(), 31);
    PrefixIterator q3(PT_QU, PT_T, 3); TEST_EXPECT_EQUAL(q3.steps(), 156);
    PrefixIterator q4(PT_QU, PT_T, 4); TEST_EXPECT_EQUAL(q4.steps(), 781);

    TEST_EXPECT_EQUAL(concat_iteration(q0), "");
    TEST_EXPECT_EQUAL(concat_iteration(q1), ".,N,A,C,G,U");
    TEST_EXPECT_EQUAL(concat_iteration(q2),
                      ".,"
                      "N.,NN,NA,NC,NG,NU,"
                      "A.,AN,AA,AC,AG,AU,"
                      "C.,CN,CA,CC,CG,CU,"
                      "G.,GN,GA,GC,GG,GU,"
                      "U.,UN,UA,UC,UG,UU");
    TEST_EXPECT_EQUAL(concat_iteration(q3),
                      ".,"
                      "N.,"
                      "NN.,NNN,NNA,NNC,NNG,NNU,"
                      "NA.,NAN,NAA,NAC,NAG,NAU,"
                      "NC.,NCN,NCA,NCC,NCG,NCU,"
                      "NG.,NGN,NGA,NGC,NGG,NGU,"
                      "NU.,NUN,NUA,NUC,NUG,NUU,"
                      "A.,"
                      "AN.,ANN,ANA,ANC,ANG,ANU,"
                      "AA.,AAN,AAA,AAC,AAG,AAU,"
                      "AC.,ACN,ACA,ACC,ACG,ACU,"
                      "AG.,AGN,AGA,AGC,AGG,AGU,"
                      "AU.,AUN,AUA,AUC,AUG,AUU,"
                      "C.,"
                      "CN.,CNN,CNA,CNC,CNG,CNU,"
                      "CA.,CAN,CAA,CAC,CAG,CAU,"
                      "CC.,CCN,CCA,CCC,CCG,CCU,"
                      "CG.,CGN,CGA,CGC,CGG,CGU,"
                      "CU.,CUN,CUA,CUC,CUG,CUU,"
                      "G.,"
                      "GN.,GNN,GNA,GNC,GNG,GNU,"
                      "GA.,GAN,GAA,GAC,GAG,GAU,"
                      "GC.,GCN,GCA,GCC,GCG,GCU,"
                      "GG.,GGN,GGA,GGC,GGG,GGU,"
                      "GU.,GUN,GUA,GUC,GUG,GUU,"
                      "U.,"
                      "UN.,UNN,UNA,UNC,UNG,UNU,"
                      "UA.,UAN,UAA,UAC,UAG,UAU,"
                      "UC.,UCN,UCA,UCC,UCG,UCU,"
                      "UG.,UGN,UGA,UGC,UGG,UGU,"
                      "UU.,UUN,UUA,UUC,UUG,UUU");
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
