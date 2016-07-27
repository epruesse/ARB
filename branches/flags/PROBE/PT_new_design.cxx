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

#include <arb_str.h>
#include <arb_defs.h>
#include <arb_sort.h>

#include <arb_strbuf.h>
#include <arb_strarray.h>

#include <climits>
#include <algorithm>
#include <map>
#include <arb_progress.h>

#if defined(DEBUG)
// # define DUMP_DESIGNED_PROBES
// # define DUMP_OLIGO_MATCHING
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

int pt_init_bond_matrix(PT_local *THIS) {
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

struct dt_bondssum {
    double dt;        // sum of mismatches
    double sum_bonds; // sum of bonds of longest non mismatch string

    dt_bondssum(double dt_, double sum_bonds_) : dt(dt_), sum_bonds(sum_bonds_) {}
};

class Oligo {
    const char *data;
    int         length;

public:
    Oligo() : data(NULL), length(0) {} // empty oligo
    Oligo(const char *data_, int length_) : data(data_), length(length_) {}

    char at(int offset) const {
        pt_assert(offset >= 0 && offset<length);
        return data[offset];
    }
    int size() const { return length; }

    Oligo suffix(int skip) const {
        return skip <= length ? Oligo(data+skip, length-skip) : Oligo();
    }

#if defined(DUMP_OLIGO_MATCHING)
    void dump(FILE *out) const {
        char *readable = readable_probe(data, length, 'T');
        fputs(readable, out);
        free(readable);
    }
#endif
};

class MatchingOligo {
    Oligo oligo;
    int   matched;       // how many positions matched
    int   lastSplit;

    bool   domain_found;
    double maxDomainBondSum;

    dt_bondssum linkage;

    MatchingOligo(const MatchingOligo& other, double strength) // expand by 1 (with a split)
        : oligo(other.oligo),
          matched(other.matched+1),
          domain_found(other.domain_found),
          maxDomainBondSum(other.maxDomainBondSum),
          linkage(other.linkage)
    {
        pt_assert(other.dangling());
        pt_assert(strength<0.0);

        lastSplit          = matched-1;
        linkage.dt        -= strength;
        linkage.sum_bonds  = 0.0;

        pt_assert(domainLength() == 0);
    }

    MatchingOligo(const MatchingOligo& other, double strength, double max_bond) // expand by 1 (binding)
        : oligo(other.oligo),
          matched(other.matched+1),
          domain_found(other.domain_found),
          maxDomainBondSum(other.maxDomainBondSum),
          linkage(other.linkage)
    {
        pt_assert(other.dangling());
        pt_assert(strength >= 0.0);

        lastSplit          = other.lastSplit;
        linkage.dt        += strength;
        linkage.sum_bonds += max_bond-strength;

        pt_assert(linkage.sum_bonds >= 0.0);

        if (domainLength() >= DOMAIN_MIN_LENGTH) {
            domain_found     = true;
            maxDomainBondSum = std::max(maxDomainBondSum, linkage.sum_bonds);
        }
    }

    void accept_rest_dangling() {
        pt_assert(dangling());
        lastSplit = matched+1;
        matched   = oligo.size();
        pt_assert(!dangling());
    }

    void optimal_bind_rest(const Splits& splits) { // @@@ slow -> optimize
        pt_assert(dangling());
        while (dangling()) {
            char   pc       = dangling_char();
            double strength = splits.check(pc, pc); // @@@ always 0.0?
            UNCOVERED();
            pt_assert(strength >= 0.0);

            double bondmax = splits.get_max_bond(pc);

            matched++;
            linkage.dt        += strength;
            linkage.sum_bonds += bondmax-strength;
        }
        if (domainLength() >= DOMAIN_MIN_LENGTH) {
            domain_found     = true;
            maxDomainBondSum = std::max(maxDomainBondSum, linkage.sum_bonds);
        }
        pt_assert(!dangling());
    }

public:
    explicit MatchingOligo(const Oligo& oligo_)
        : oligo(oligo_),
          matched(0),
          lastSplit(-1),
          domain_found(false),
          maxDomainBondSum(-1),
          linkage(0.0, 0.0)
    {}

    int dangling() const {
        int dangle_count = oligo.size()-matched;
        pt_assert(dangle_count >= 0);
        return dangle_count;
    }

    char dangling_char() const {
        pt_assert(dangling()>0);
        return oligo.at(matched);
    }

    MatchingOligo bind_against(char c, const Splits& splits) const {
        pt_assert(is_std_base_or_N(c));

        char pc = dangling_char();
        double strength;

        if (c == PT_N) {
            // we are checking outgroup-matches here => assume strongest possible bind versus N
            strength = -100.0;
            for (int base = PT_A; base < PT_BASES; ++base) {
                strength = std::max(strength, splits.check(pc, base));
            }
            pt_assert(strength>-100.0);
        }
        else {
            strength = splits.check(pc, c);
        }

        return strength<0.0
            ? MatchingOligo(*this, strength)
            : MatchingOligo(*this, strength, splits.get_max_bond(pc));
    }

    MatchingOligo dont_bind_rest() const {
        MatchingOligo accepted(*this);
        accepted.accept_rest_dangling();
        return accepted;
    }

    int domainLength() const { return matched-(lastSplit+1); }

    bool domainSeen() const { return domain_found; }
    bool domainPossible() const {
        pt_assert(!domainSeen()); // you are testing w/o need
        return (domainLength()+dangling()) >= DOMAIN_MIN_LENGTH;
    }

    bool completely_bound() const { return !dangling(); }

    int calc_centigrade_pos(const PT_tprobes *tprobe, const PT_pdc *const pdc) const {
        pt_assert(completely_bound());
        pt_assert(domainSeen());

        double ndt = (linkage.dt * pdc->dt + (tprobe->sum_bonds - maxDomainBondSum)*pdc->dte) / tprobe->seq_len;
        int    pos = (int)ndt;

        pt_assert(pos >= 0);
        return pos;
    }

    bool centigrade_pos_out_of_reach(const PT_tprobes *tprobe, const PT_pdc *const pdc, const Splits& splits) const {
        MatchingOligo optimum(*this);
        optimum.optimal_bind_rest(splits);

        if (!optimum.domainSeen()) return true; // no domain -> no centigrade position

        int centpos = optimum.calc_centigrade_pos(tprobe, pdc);
        return centpos >= PERC_SIZE;
    }

    const Oligo& get_oligo() const { return oligo; }

#if defined(DUMP_OLIGO_MATCHING)
    void dump(FILE *out) const {
        fputs("oligo='", out);
        oligo.dump(out);

        const char *domainState                = "impossible";
        if (domainSeen()) domainState          = "seen";
        else if (domainPossible()) domainState = "possible";

        fprintf(out, "' matched=%2i lastSplit=%2i domainLength()=%2i dangling()=%2i domainState=%s maxDomainBondSum=%f dt=%f sum_bonds=%f\n",
                matched, lastSplit, domainLength(), dangling(), domainState, maxDomainBondSum, linkage.dt, linkage.sum_bonds);
    }
#endif
};

static int ptnd_compare_quality(const void *vtp1, const void *vtp2, void *) {
    const PT_tprobes *tp1 = (const PT_tprobes*)vtp1;
    const PT_tprobes *tp2 = (const PT_tprobes*)vtp2;

    int cmp = double_cmp(tp2->quality, tp1->quality);         // high quality first
    if (!cmp) {
        cmp           = tp1->apos - tp2->apos;                // low abs.pos first
        if (!cmp) cmp = strcmp(tp1->sequence, tp2->sequence); // alphabetically by probe
    }
    return cmp;
}

static int ptnd_compare_sequence(const void *vtp1, const void *vtp2, void*) {
    const PT_tprobes *tp1 = (const PT_tprobes*)vtp1;
    const PT_tprobes *tp2 = (const PT_tprobes*)vtp2;

    return strcmp(tp1->sequence, tp2->sequence);
}

enum ProbeSortMode {
    PSM_QUALITY,
    PSM_SEQUENCE,
};

static void sort_tprobes_by(PT_pdc *pdc, ProbeSortMode mode) {
    if (pdc->tprobes) {
        int list_len = pdc->tprobes->get_count();
        if (list_len > 1) {
            PT_tprobes **my_list = (PT_tprobes **)calloc(sizeof(void *), list_len);
            {
                PT_tprobes *tprobe;
                int         i;

                for (i = 0, tprobe = pdc->tprobes; tprobe; i++, tprobe = tprobe->next) {
                    my_list[i] = tprobe;
                }
            }

            switch (mode) {
                case PSM_QUALITY:
                    GB_sort((void **)my_list, 0, list_len, ptnd_compare_quality, 0);
                    break;

                case PSM_SEQUENCE:
                    GB_sort((void **)my_list, 0, list_len, ptnd_compare_sequence, 0);
                    break;
            }

            for (int i=0; i<list_len; i++) {
                aisc_unlink(reinterpret_cast<dllheader_ext*>(my_list[i]));
                aisc_link(&pdc->ptprobes, my_list[i]);
            }

            free(my_list);
        }
    }
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

static void tprobes_sumup_perc_and_calc_quality(PT_pdc *pdc) {
    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
        int sum = 0;
        int i;
        for (i=0; i<PERC_SIZE; ++i) {
            sum             += tprobe->perc[i];
            tprobe->perc[i]  = sum;
        }

        // pt_assert(tprobe->perc[0] == tprobe->mishits); // OutgroupMatcher and count_mishits_for_matched do not agree!
        // @@@ found one case where it differs by 1 (6176 in perc, 6177 in mishits). Can this be caused somehow by N or IUPAC in seq?

        int limit = 2*tprobe->mishits;
        for (i=0; i<(PERC_SIZE-1); ++i) {
            if (tprobe->perc[i]>limit) break;
        }

        tprobe->quality = ((double)tprobe->groupsize * i) + 1000.0/(1000.0 + tprobe->perc[i]);
    }
}

static char hitgroup_idx2char(int idx) {
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

inline int get_max_probelen(const PT_pdc *pdc) {
    return pdc->max_probelen == -1 ? pdc->min_probelen : pdc->max_probelen;
}

inline int shown_apos(const PT_tprobes *tprobe) { return info2bio(tprobe->apos); }
inline int shown_ecoli(const PT_tprobes *tprobe) { return PT_abs_2_ecoli_rel(tprobe->apos+1); }
inline int shown_qual(const PT_tprobes *tprobe) { return int(tprobe->quality+0.5); }

class PD_formatter {
    int width[PERC_SIZE]; // of centigrade list columns
    int maxprobelen;

    int apos_width;
    int ecol_width;
    int qual_width;
    int grps_width;

    static inline void track_max(int& tracked, int val) { tracked = std::max(tracked, val); }

    void collect(const int *perc) { for (int i = 0; i<PERC_SIZE; ++i) width[i] = std::max(width[i], perc[i]); }
    void collect(const PT_tprobes *tprobe) {
        collect(tprobe->perc);
        track_max(apos_width, shown_apos(tprobe));
        track_max(ecol_width, shown_ecoli(tprobe));
        track_max(qual_width, shown_qual(tprobe));
        track_max(grps_width, shown_qual(tprobe));
        track_max(maxprobelen, tprobe->seq_len);
    }

    void clear() {
        for (int i = 0; i<PERC_SIZE; ++i) width[i] = 0;

        apos_width = 0;
        ecol_width = 0;
        qual_width = 0;
        grps_width = 0;

        maxprobelen = 0;
    }

    static inline int width4num(int num) {
        pt_assert(num >= 0);

        int digits = 0;
        while (num) {
            ++digits;
            num /= 10;
        }
        return digits ? digits : 1;
    }

public:
    PD_formatter() { clear(); }
    PD_formatter(const PT_pdc *pdc) {
        clear();

        // collect max values for output columns:
        for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) collect(tprobe);

        // convert max.values to needed print-width:
        for (int i = 0; i<PERC_SIZE; ++i) width[i] = width4num(width[i]);

        apos_width = std::max(width4num(apos_width), 2);
        ecol_width = std::max(width4num(ecol_width), 4);
        qual_width = std::max(width4num(qual_width), 4);
        grps_width = std::max(width4num(grps_width), 4);
    }

    int sprint_centigrade_list(char *buffer, const int *perc) const {
        // format centigrade-decrement-list
        int printed = 0;
        for (int i = 0; i<PERC_SIZE; ++i) {
            buffer[printed++]  = ' ';
            printed += perc[i]
                ? sprintf(buffer+printed, "%*i", width[i], perc[i])
                : sprintf(buffer+printed, "%*s", width[i], "-");
        }
        return printed;
    }

    int get_max_designed_len() const { return maxprobelen; }

    int get_apos_width() const { return apos_width; }
    int get_ecol_width() const { return ecol_width; }
    int get_qual_width() const { return qual_width; }
    int get_grps_width() const { return grps_width; }
};

typedef std::map<const PT_pdc*const, PD_formatter> PD_Formatter4design;
static PD_Formatter4design format4design;

inline const PD_formatter& get_formatter(const PT_pdc *pdc) {
    PD_Formatter4design::iterator found = format4design.find(pdc);
    if (found == format4design.end()) {
        format4design[pdc] = PD_formatter(pdc);
        found              = format4design.find(pdc);
    }
    pt_assert(found != format4design.end());

    return found->second;
}
inline void erase_formatter(const PT_pdc *pdc) { format4design.erase(pdc); }

char *get_design_info(const PT_tprobes *tprobe) {
    pt_assert(tprobe);

    const int            BUFFERSIZE = 2000;
    char                *buffer     = (char *)GB_give_buffer(BUFFERSIZE);
    PT_pdc              *pdc        = (PT_pdc *)tprobe->mh.parent->parent;
    char                *p          = buffer;
    const PD_formatter&  formatter  = get_formatter(pdc);

    // target
    {
        int   len   = tprobe->seq_len;
        char *probe = (char *)GB_give_buffer2(len+1);
        strcpy(probe, tprobe->sequence);

        probe_2_readable(probe, len); // convert probe to real ASCII
        p += sprintf(p, "%-*s", formatter.get_max_designed_len()+1, probe);
    }

    {
        int apos = shown_apos(tprobe);
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
            if (dist < pdc->min_probelen) {
                apos = apos - pdc->pos_groups[c];
                if (apos >= 0) {
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
        p += sprintf(p, "%2i ", tprobe->seq_len);
        p += sprintf(p, "%c%c%*i ", c, cs, formatter.get_apos_width(), apos);
        p += sprintf(p, "%*i ", formatter.get_ecol_width(), shown_ecoli(tprobe)); // ecoli-bases inclusive apos ("bases before apos+1")
    }

    p += sprintf(p, "%*i ", formatter.get_qual_width(), shown_qual(tprobe));
    p += sprintf(p, "%*i ", formatter.get_grps_width(), tprobe->groupsize);
    p += sprintf(p, "%5.1f", ((double)pt_get_gc_content(tprobe->sequence))/tprobe->seq_len*100.0); // G+C
    p += sprintf(p, "%5.1f ", pt_get_temperature(tprobe->sequence));                               // temperature

    // probe string
    {
        char *probe = create_reversed_probe(tprobe->sequence, tprobe->seq_len);
        complement_probe(probe, tprobe->seq_len);
        probe_2_readable(probe, tprobe->seq_len); // convert probe to real ASCII
        p += sprintf(p, "%*s |", formatter.get_max_designed_len(), probe);
        free(probe);
    }

    // non-group hits by temp. decrease
    p += formatter.sprint_centigrade_list(p, tprobe->perc);
    if (!tprobe->next) erase_formatter(pdc); // erase formatter when done with last probe

    pt_assert((p-buffer)<BUFFERSIZE);
    return buffer;
}

char *get_design_hinfo(const PT_pdc *pdc) {
    const int  BUFFERSIZE = 2000;
    char      *buffer     = (char *)GB_give_buffer(BUFFERSIZE);
    char      *s          = buffer;

    const PD_formatter& formatter = get_formatter(pdc);

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
                ecolipos = GBS_global_string_copy("%4i -%5i", pdc->min_ecolipos, pdc->max_ecolipos);
            }
        }

        char *mishit_annotation = NULL;
        if (pdc->min_rj_mishits<INT_MAX) {
            mishit_annotation = GBS_global_string_copy(" (lowest rejected nongroup hits: %i)", pdc->min_rj_mishits);
        }

        char *coverage_annotation = NULL;
        if (pdc->max_rj_coverage>0.0) {
            coverage_annotation = GBS_global_string_copy(" (max. rejected coverage: %.0f%%)", pdc->max_rj_coverage*100.0);
        }

        char *probelen_info;
        if (get_max_probelen(pdc) == pdc->min_probelen) {
            probelen_info = GBS_global_string_copy("%i", pdc->min_probelen);
        }
        else {
            probelen_info = GBS_global_string_copy("%i-%i", pdc->min_probelen, get_max_probelen(pdc));
        }

        s += sprintf(s,
                     "Probe design parameters:\n"
                     "Length of probe    %s\n"
                     "Temperature        [%4.1f -%5.1f]\n"
                     "GC-content         [%4.1f -%5.1f]\n"
                     "E.Coli position    [%s]\n"
                     "Max. nongroup hits %i%s\n"
                     "Min. group hits    %.0f%%%s\n",
                     probelen_info,
                     pdc->mintemp, pdc->maxtemp,
                     pdc->min_gc*100.0, pdc->max_gc*100.0,
                     ecolipos,
                     pdc->max_mishits,        mishit_annotation   ? mishit_annotation   : "",
                     pdc->min_coverage*100.0, coverage_annotation ? coverage_annotation : "");

        free(probelen_info);
        free(coverage_annotation);
        free(mishit_annotation);

        free(ecolipos);
    }

    if (pdc->tprobes) {
        int maxprobelen = formatter.get_max_designed_len();

        s += sprintf(s, "%-*s", maxprobelen+1, "Target");
        s += sprintf(s, "le ");
        s += sprintf(s, "%*s ", formatter.get_apos_width()+2, "apos");
        s += sprintf(s, "%*s ", formatter.get_ecol_width(), "ecol");
        s += sprintf(s, "%*s ", formatter.get_qual_width(), "qual");
        s += sprintf(s, "%*s ", formatter.get_grps_width(), "grps");
        s += sprintf(s, "  G+C temp ");
        s += sprintf(s, "%*s | ", maxprobelen, maxprobelen<14 ? "Probe" : "Probe sequence");
        s += sprintf(s, "Decrease T by n*.3C -> probe matches n non group species");
    }
    else {
        s += sprintf(s, "No probes found!");
        erase_formatter(pdc);
    }

    pt_assert((s-buffer)<BUFFERSIZE);

    return buffer;
}

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
    // tracks minimum rejected mishits in 'pdc->min_rj_mishits'
    int min_rej_mishit_amount = INT_MAX;

    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; ) {
        PT_tprobes *tprobe_next = tprobe->next;

        psg.abs_pos.clear();
        tprobe->mishits = count_mishits_for_matched(tprobe->sequence, psg.TREE_ROOT2(), 0);
        tprobe->apos    = psg.abs_pos.get_most_used();
        if (tprobe->mishits > pdc->max_mishits) {
            min_rej_mishit_amount = std::min(min_rej_mishit_amount, tprobe->mishits);
            destroy_PT_tprobes(tprobe);
        }
        tprobe = tprobe_next;
    }
    psg.abs_pos.clear();

    pdc->min_rj_mishits = std::min(pdc->min_rj_mishits, min_rej_mishit_amount);
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

static size_t tprobes_calculate_bonds(PT_local *locs) {
    /*! check the average bond size.
     *  returns number of tprobes
     *
     * @TODO  checks probe hairpin bonds
     */

    PT_pdc *pdc   = locs->pdc;
    size_t  count = 0;

    MaxBond mbond(locs->bond);

    for (PT_tprobes *tprobe = pdc->tprobes; tprobe; ) {
        PT_tprobes *tprobe_next = tprobe->next;
        tprobe->seq_len = strlen(tprobe->sequence);
        double sbond = 0.0;
        for (int i=0; i<tprobe->seq_len; i++) {
            sbond += mbond.get_max_bond(tprobe->sequence[i]);
        }
        tprobe->sum_bonds = sbond;

        ++count;
        tprobe = tprobe_next;
    }
    return count;
}

class CentigradePos { // track minimum centigrade position for each species
    typedef std::map<int, int> Pos4Name;

    Pos4Name cpos; // key = outgroup-species-id; value = min. centigrade position for (partial) probe

public:

    static bool is_valid_pos(int pos) { return pos >= 0 && pos<PERC_SIZE; }

    void set_pos_for(int name, int pos) {
        pt_assert(is_valid_pos(pos)); // otherwise it should not be put into CentigradePos
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
            pt_assert(is_valid_pos(pos)); // otherwise it should not be put into CentigradePos
            centigrade_hits[pos]++;
        }
    }

    bool empty() const { return cpos.empty(); }
};

class OutgroupMatcher : virtual Noncopyable {
    const PT_pdc *const pdc;

    Splits splits;

    PT_tprobes    *currTprobe;
    CentigradePos  result;

    bool only_bind_behind_dot; // true for suffix-matching

    void uncond_announce_match(int centPos, const AbsLoc& loc) {
        pt_assert(loc.get_pid().outside_group());
#if defined(DUMP_OLIGO_MATCHING)
        fprintf(stderr, "announce_match centPos=%2i loc", centPos);
        loc.dump(stderr);
        fflush_all();
#endif
        result.set_pos_for(loc.get_name(), centPos);
    }

    bool location_follows_dot(const ReadableDataLoc& loc) const { return loc[-1] == PT_QU; }
    bool location_follows_dot(const DataLoc& loc) const { return location_follows_dot(ReadableDataLoc(loc)); } // very expensive @@@ optimize using relpos
    bool location_follows_dot(const AbsLoc& loc) const { return location_follows_dot(ReadableDataLoc(DataLoc(loc))); } // very very expensive

    template<typename LOC>
    bool acceptable_location(const LOC& loc) const {
        return loc.get_pid().outside_group() &&
            (only_bind_behind_dot ? location_follows_dot(loc) : true);
    }

    template<typename LOC>
    void announce_match_if_acceptable(int centPos, const LOC& loc) {
        if (acceptable_location(loc)) {
            uncond_announce_match(centPos, loc);
        }
    }
    void announce_match_if_acceptable(int centPos, POS_TREE2 *pt) {
        switch (pt->get_type()) {
            case PT2_NODE:
                for (int i = PT_QU; i<PT_BASES; ++i) {
                    POS_TREE2 *ptson = PT_read_son(pt, (PT_base)i);
                    if (ptson) {
                        announce_match_if_acceptable(centPos, ptson);
                    }
                }
                break;

            case PT2_LEAF:
                announce_match_if_acceptable(centPos, DataLoc(pt));
                break;

            case PT2_CHAIN: {
                ChainIteratorStage2 iter(pt);
                while (iter) {
                    announce_match_if_acceptable(centPos, iter.at());
                    ++iter;
                }
                break;
            }
        }
    }

    template <typename PT_OR_LOC>
    void announce_possible_match(const MatchingOligo& oligo, PT_OR_LOC pt_or_loc) {
        pt_assert(oligo.domainSeen());
        pt_assert(!oligo.dangling());

        int centPos = oligo.calc_centigrade_pos(currTprobe, pdc);
        if (centPos<PERC_SIZE) {
            announce_match_if_acceptable(centPos, pt_or_loc);
        }
    }

    bool might_reach_centigrade_pos(const MatchingOligo& oligo) const {
        return !oligo.centigrade_pos_out_of_reach(currTprobe, pdc, splits);
    }

    void bind_rest(const MatchingOligo& oligo, const ReadableDataLoc& loc, const int height) {
        // unconditionally bind rest of oligo versus sequence

        pt_assert(oligo.domainSeen());                // entry-invariant for bind_rest()
        pt_assert(oligo.dangling());                  // otherwise ReadableDataLoc is constructed w/o need (very expensive!)
        pt_assert(might_reach_centigrade_pos(oligo)); // otherwise ReadableDataLoc is constructed w/o need (very expensive!)
        pt_assert(loc.get_pid().outside_group());     // otherwise we are not interested in the result

        if (loc[height]) {
            MatchingOligo more = oligo.bind_against(loc[height], splits);
            pt_assert(more.domainSeen()); // implied by oligo.domainSeen()
            if (more.dangling()) {
                if (might_reach_centigrade_pos(more)) {
                    bind_rest(more, loc, height+1);
                }
            }
            else {
                announce_possible_match(more, loc);
            }
        }
        else {
            MatchingOligo all = oligo.dont_bind_rest();
            announce_possible_match(all, loc);
        }
    }
    void bind_rest_if_outside_group(const MatchingOligo& oligo, const DataLoc& loc, const int height) {
        if (loc.get_pid().outside_group() && might_reach_centigrade_pos(oligo)) {
            bind_rest(oligo, ReadableDataLoc(loc), height);
        }
    }
    void bind_rest_if_outside_group(const MatchingOligo& oligo, const AbsLoc&  loc, const int height) {
        if (loc.get_pid().outside_group() && might_reach_centigrade_pos(oligo)) {
            bind_rest(oligo, ReadableDataLoc(DataLoc(loc)), height);
        }
    }

    void bind_rest(const MatchingOligo& oligo, POS_TREE2 *pt, const int height) {
        // unconditionally bind rest of oligo versus complete index-subtree
        pt_assert(oligo.domainSeen()); // entry-invariant for bind_rest()

        if (oligo.dangling()) {
            if (might_reach_centigrade_pos(oligo)) {
                switch (pt->get_type()) {
                    case PT2_NODE: {
                        POS_TREE2 *ptdotson = PT_read_son(pt, PT_QU);
                        if (ptdotson) {
                            MatchingOligo all = oligo.dont_bind_rest();
                            announce_possible_match(all, ptdotson);
                        }

                        for (int i = PT_N; i<PT_BASES; ++i) {
                            POS_TREE2 *ptson = PT_read_son(pt, (PT_base)i);
                            if (ptson) {
                                bind_rest(oligo.bind_against(i, splits), ptson, height+1);
                            }
                        }
                        break;
                    }
                    case PT2_LEAF:
                        bind_rest_if_outside_group(oligo, DataLoc(pt), height);
                        break;

                    case PT2_CHAIN:
                        ChainIteratorStage2 iter(pt);
                        while (iter) {
                            bind_rest_if_outside_group(oligo, iter.at(), height);
                            ++iter;
                        }
                        break;
                }
            }
        }
        else {
            announce_possible_match(oligo, pt);
        }
    }

    void bind_till_domain(const MatchingOligo& oligo, const ReadableDataLoc& loc, const int height) {
        pt_assert(!oligo.domainSeen() && oligo.domainPossible()); // entry-invariant for bind_till_domain()
        pt_assert(oligo.dangling());                              // otherwise ReadableDataLoc is constructed w/o need (very expensive!)
        pt_assert(might_reach_centigrade_pos(oligo));             // otherwise ReadableDataLoc is constructed w/o need (very expensive!)
        pt_assert(loc.get_pid().outside_group());                 // otherwise we are not interested in the result

        if (is_std_base_or_N(loc[height])) { // do not try to bind domain versus dot
            MatchingOligo more = oligo.bind_against(loc[height], splits);
            if (more.dangling()) {
                if (might_reach_centigrade_pos(more)) {
                    if      (more.domainSeen())     bind_rest       (more, loc, height+1);
                    else if (more.domainPossible()) bind_till_domain(more, loc, height+1);
                }
            }
            else if (more.domainSeen()) {
                announce_possible_match(more, loc);
            }
        }
    }
    void bind_till_domain_if_outside_group(const MatchingOligo& oligo, const DataLoc& loc, const int height) {
        if (loc.get_pid().outside_group() && might_reach_centigrade_pos(oligo)) {
            bind_till_domain(oligo, ReadableDataLoc(loc), height);
        }
    }
    void bind_till_domain_if_outside_group(const MatchingOligo& oligo, const AbsLoc&  loc, const int height) {
        if (loc.get_pid().outside_group() && might_reach_centigrade_pos(oligo)) {
            bind_till_domain(oligo, ReadableDataLoc(DataLoc(loc)), height);
        }
    }

    void bind_till_domain(const MatchingOligo& oligo, POS_TREE2 *pt, const int height) {
        pt_assert(!oligo.domainSeen() && oligo.domainPossible()); // entry-invariant for bind_till_domain()
        pt_assert(oligo.dangling());

        if (might_reach_centigrade_pos(oligo)) {
            switch (pt->get_type()) {
                case PT2_NODE:
                    for (int i = PT_A; i<PT_BASES; ++i) {
                        POS_TREE2 *ptson = PT_read_son(pt, (PT_base)i);
                        if (ptson) {
                            MatchingOligo sonOligo = oligo.bind_against(i, splits);

                            if      (sonOligo.domainSeen())     bind_rest       (sonOligo, ptson, height+1);
                            else if (sonOligo.domainPossible()) bind_till_domain(sonOligo, ptson, height+1);
                        }
                    }
                    break;

                case PT2_LEAF:
                    bind_till_domain_if_outside_group(oligo, DataLoc(pt), height);
                    break;

                case PT2_CHAIN:
                    ChainIteratorStage2 iter(pt);
                    while (iter) {
                        bind_till_domain_if_outside_group(oligo, iter.at(), height);
                        ++iter;
                    }
                    break;
            }
        }
    }

    void reset() { result = CentigradePos(); }

public:
    OutgroupMatcher(PT_local *locs_, PT_pdc *pdc_)
        : pdc(pdc_),
          splits(locs_),
          currTprobe(NULL),
          only_bind_behind_dot(false)
    {}

    void calculate_outgroup_matches(PT_tprobes& tprobe) {
        LocallyModify<PT_tprobes*> assign_tprobe(currTprobe, &tprobe);
        pt_assert(result.empty());

        MatchingOligo  fullProbe(Oligo(tprobe.sequence, tprobe.seq_len));
        POS_TREE2     *ptroot = psg.TREE_ROOT2();

        pt_assert(!fullProbe.domainSeen());
        pt_assert(fullProbe.domainPossible()); // probe length too short (probes shorter than DOMAIN_MIN_LENGTH always report 0 outgroup hits -> wrong result)

        arb_progress progress(tprobe.seq_len);

        only_bind_behind_dot = false;
        bind_till_domain(fullProbe, ptroot, 0);
        ++progress;

        // match all suffixes of probe
        // - detect possible partial matches at start of alignment (and behind dots inside the sequence)
        only_bind_behind_dot = true;
        for (int off = 1; off<tprobe.seq_len; ++off) {
            MatchingOligo probeSuffix(fullProbe.get_oligo().suffix(off));
            if (!probeSuffix.domainPossible()) break; // abort - no smaller suffix may create a domain
            bind_till_domain(probeSuffix, ptroot, 0);
            ++progress;
        }

        result.summarize_centigrade_hits(tprobe.perc);
        progress.done();

        reset();
    }
};

class ProbeOccurrance {
    const char *seq;

public:
    ProbeOccurrance(const char *seq_) : seq(seq_) {}

    const char *sequence() const { return seq; }

    void forward() { ++seq; }

    bool less(const ProbeOccurrance& other, size_t len) const {
        const char *mine = sequence();
        const char *his  = other.sequence();

        int cmp = 0;
        for (size_t i = 0; i<len && !cmp; ++i) {
            cmp = mine[i]-his[i];
        }
        return cmp<0;
    }
};

class ProbeIterator {
    ProbeOccurrance cursor;

    size_t probelen;
    size_t rest;   // size of seqpart behind cursor
    size_t gc, at; // count G+C and A+T

    bool has_only_std_bases() const { return (gc+at) == probelen; }

    bool at_end() const { return !rest; }

    PT_base base_at(size_t offset) const {
        pt_assert(offset < probelen);
        return PT_base(cursor.sequence()[offset]);
    }

    void forget(PT_base b) {
        switch (b) {
            case PT_A: case PT_T: pt_assert(at>0); --at; break;
            case PT_C: case PT_G: pt_assert(gc>0); --gc; break;
            default : break;
        }
    }
    void count(PT_base b) {
        switch (b) {
            case PT_A: case PT_T: ++at; break;
            case PT_C: case PT_G: ++gc; break;
            default : break;
        }
    }

    void init_counts() {
        at = gc = 0;
        for (size_t i = 0; i<probelen; ++i) {
            count(base_at(i));
        }
    }

public:
    ProbeIterator(const char *seq, size_t seqlen, size_t probelen_)
        : cursor(seq),
          probelen(probelen_),
          rest(seqlen-probelen)
    {
        pt_assert(seqlen >= probelen);
        init_counts();
    }

    bool next() {
        if (at_end()) return false;
        forget(base_at(0));
        cursor.forward();
        --rest;
        count(base_at(probelen-1));
        return true;
    }

    int get_temperature() const { return 2*at + 4*gc; }

    bool gc_in_wanted_range(PT_pdc *pdc) const {
        return pdc->min_gc*probelen <= gc && gc <= pdc->max_gc*probelen;
    }
    bool temperature_in_wanted_range(PT_pdc *pdc) const {
        int temp             = get_temperature();
        return pdc->mintemp <= temp && temp <= pdc->maxtemp;
    }

    bool feasible(PT_pdc *pdc) const {
        return has_only_std_bases() &&
            gc_in_wanted_range(pdc) &&
            temperature_in_wanted_range(pdc);
    }

    const ProbeOccurrance& occurance() const { return cursor; }

    void dump(FILE *out) const {
        char *probe = readable_probe(cursor.sequence(), probelen, 'T');
        fprintf(out, "probe='%s' probelen=%zu at=%zu gc=%zu\n", probe, probelen, at, gc);
        free(probe);
    }
};

class PO_Less {
    size_t probelen;
public:
    PO_Less(size_t probelen_) : probelen(probelen_) {}
    bool operator()(const ProbeOccurrance& a, const ProbeOccurrance& b) const { return a.less(b, probelen); }
};

struct SCP_Less {
    bool operator()(const SmartCharPtr& p1, const SmartCharPtr& p2) { return &*p1 < &*p2; } // simply compare addresses
};

class ProbeCandidates {
    typedef std::set<ProbeOccurrance, PO_Less>      Candidates;
    typedef std::map<ProbeOccurrance, int, PO_Less> CandidateHits;
    typedef std::set<SmartCharPtr, SCP_Less>        SeqData;

    size_t        probelen;
    CandidateHits candidateHits;
    SeqData       data; // locks SmartPtrs to input data (ProbeOccurrances point inside their data)

public:
    ProbeCandidates(size_t probelen_)
        : probelen(probelen_),
          candidateHits(probelen)
    {}

    void generate_for_sequence(PT_pdc *pdc, const SmartCharPtr& seq, size_t bp) {
        arb_progress base_progress(2*bp);
        if (bp >= probelen) {
            // collect all probe candidates for current sequence
            Candidates candidates(probelen);
            {
                data.insert(seq);
                ProbeIterator probe(&*seq, bp, probelen);
                do {
                    if (probe.feasible(pdc)) {
                        candidates.insert(probe.occurance());
                        ++base_progress;
                    }
                } while (probe.next());
            }

            // increment overall hitcount for each found candidate
            for (Candidates::iterator c = candidates.begin(); c != candidates.end(); ++c) {
                candidateHits[*c]++;
                ++base_progress;
            }
        }
        base_progress.done();
    }

    void create_tprobes(PT_pdc *pdc, int ingroup_size) {
        // tracks maximum rejected target-group coverage
        int min_ingroup_hits     = (ingroup_size * pdc->min_coverage + .5);
        int max_rej_ingroup_hits = 0;

        for (CandidateHits::iterator c = candidateHits.begin(); c != candidateHits.end(); ++c) {
            int ingroup_hits = c->second;
            if (ingroup_hits >= min_ingroup_hits) {
                const ProbeOccurrance& candi = c->first;

                PT_tprobes *tprobe = create_PT_tprobes();

                tprobe->sequence  = GB_strndup(candi.sequence(), probelen);
                tprobe->temp      = pt_get_temperature(tprobe->sequence);
                tprobe->groupsize = ingroup_hits;

                aisc_link(&pdc->ptprobes, tprobe);
            }
            else {
                max_rej_ingroup_hits = std::max(max_rej_ingroup_hits, ingroup_hits);
            }
        }

        double max_rejected_coverage = max_rej_ingroup_hits/double(ingroup_size);
        pdc->max_rj_coverage         = std::max(pdc->max_rj_coverage, max_rejected_coverage);
    }
};

class DesignTargets : virtual Noncopyable {
    PT_pdc *pdc;

    int targets;       // overall target sequence count
    int known_targets; // known target sequences
    int added_targets; // added (=unknown) target sequences

    long datasize;     // overall bp of target sequences

    int min_probe_length; // min. designed probe length

    int *known2id;

    ARB_ERROR error;

    void announce_seq_bp(long bp) {
        pt_assert(!error);
        if (bp<long(min_probe_length)) {
            error = GBS_global_string("Sequence contains only %lu bp. Impossible design request for", bp);
        }
        else {
            datasize                  += bp;
            if (datasize<bp) datasize  = ULONG_MAX;
        }
    }

    void scan_added_targets() {
        added_targets = 0;
        for (PT_sequence *seq = pdc->sequences; seq && !error; seq = seq->next) {
            announce_seq_bp(seq->seq.size);
            ++added_targets;
        }
        if (error) error = GBS_global_string("%s one of the added sequences", error.deliver());
    }
    void scan_known_targets(int expected_known_targets) {
        known2id      = new int[expected_known_targets];
        known_targets = 0;

        for (int id = 0; id < psg.data_count && !error; ++id) {
            const probe_input_data& pid = psg.data[id];
            if (pid.inside_group()) {
                announce_seq_bp(pid.get_size());
                known2id[known_targets++] = id;
                pt_assert(known_targets <= expected_known_targets);
                if (error) error = GBS_global_string("%s species '%s'", error.deliver(), pid.get_shortname());
            }
        }
    }

public:
    DesignTargets(PT_pdc *pdc_, int expected_known_targets)
        : pdc(pdc_),
          targets(0),
          known_targets(0),
          datasize(0),
          min_probe_length(pdc->min_probelen),
          known2id(NULL)
    {
        scan_added_targets(); // calc added_targets
        if (!error) {
            scan_known_targets(expected_known_targets);
            targets = known_targets+added_targets;
        }
    }
    ~DesignTargets() {
        delete [] known2id;
    }
    ARB_ERROR& get_error() { return error; } // needs to be checked after construction!

    long get_datasize() const { return datasize; }
    int get_count() const { return targets; }
    int get_known_count() const { return known_targets; }
    int get_added_count() const { return added_targets; }

    void generate(ProbeCandidates& candidates) {
        arb_progress progress(get_count());
        for (int k = 0; k<known_targets; ++k) {
            const probe_input_data& pid = psg.data[known2id[k]];
            candidates.generate_for_sequence(pdc, pid.get_dataPtr(), pid.get_size());
            ++progress;
        }
        for (PT_sequence *seq = pdc->sequences; seq; seq = seq->next) {
            candidates.generate_for_sequence(pdc, GB_strndup(seq->seq.data, seq->seq.size), seq->seq.size);
            ++progress;
        }
    }
};

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

    erase_formatter(pdc);
    while (pdc->tprobes) destroy_PT_tprobes(pdc->tprobes);

    for (PT_sequence *seq = pdc->sequences; seq; seq = seq->next) {              // Convert all external sequence to internal format
        seq->seq.size = probe_compress_sequence(seq->seq.data, seq->seq.size-1); // no longer convert final zero-byte (PT_QU gets auto-appended)
    }

    ARB_ERROR error;
    int       unknown_species_count = 0;
    {
        char *unknown_names = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error); // read the names
        if (unknown_names) {
            ConstStrArray names;
            GBT_splitNdestroy_string(names, unknown_names, "#", true);
            pt_assert(!unknown_names);
            unknown_species_count = names.size();
        }
    }

    if (!error) {
        DesignTargets targets(pdc, locs->group_count); // here locs->group_count is amount of known marked species/genes
        error = targets.get_error();

        if (!error) {
            locs->group_count = targets.get_count();
            if (locs->group_count <= 0) {
                error = GBS_global_string("No %s marked - no probes designed", gene_flag ? "genes" : "species");
            }
            else if (targets.get_added_count() != unknown_species_count) {
                if (gene_flag) { // cannot add genes
                    pt_assert(targets.get_added_count() == 0); // cannot, but did add?!
                    error = GBS_global_string("Cannot design probes for %i unknown marked genes", unknown_species_count);
                }
                else {
                    int added = targets.get_added_count();
                    error     = GBS_global_string("Got %i unknown marked species, but %i custom sequence%s added (has to match)",
                                                  unknown_species_count,
                                                  added,
                                                  added == 1 ? " was" : "s were");
                }
            }
            else if (pdc->min_probelen < MIN_DESIGN_PROBE_LENGTH) {
                error = GBS_global_string("Specified min. probe length %i is below the min. allowed probe length of %i", pdc->min_probelen, MIN_DESIGN_PROBE_LENGTH);
            }
            else if (get_max_probelen(pdc) < pdc->min_probelen) {
                error = GBS_global_string("Max. probe length %i is below the specified min. probe length of %i", get_max_probelen(pdc), pdc->min_probelen);
            }
            else {
                // search for possible probes
                if (!error) {
                    int min_probelen = pdc->min_probelen;
                    int max_probelen = get_max_probelen(pdc);

                    arb_progress progress("Searching probe candidates", max_probelen-min_probelen+1);
                    for (int len = min_probelen; len <= max_probelen; ++len) {
                        ProbeCandidates candidates(len);

                        targets.generate(candidates);
                        pt_assert(!targets.get_error());

                        candidates.create_tprobes(pdc, targets.get_count());
                        ++progress;
                    }
                }

                while (pdc->sequences) destroy_PT_sequence(pdc->sequences);

                if (!error) {
                    sort_tprobes_by(pdc, PSM_SEQUENCE);
                    remove_tprobes_with_too_many_mishits(pdc);
                    remove_tprobes_outside_ecoli_range(pdc);
                    size_t tprobes = tprobes_calculate_bonds(locs);
                    DUMP_TPROBES("after tprobes_calculate_bonds", pdc);

                    if (tprobes) {
                        arb_progress    progress(GBS_global_string("Calculating probe quality for %zu probe candidates", tprobes), tprobes);
                        OutgroupMatcher om(locs, pdc);
                        for (PT_tprobes *tprobe = pdc->tprobes; tprobe; tprobe = tprobe->next) {
                            om.calculate_outgroup_matches(*tprobe);
                            ++progress;
                        }
                    }
                    else {
                        fputs("No probe candidates found\n", stdout);
                    }

                    tprobes_sumup_perc_and_calc_quality(pdc);
                    sort_tprobes_by(pdc, PSM_QUALITY);
                    clip_tprobes(pdc, pdc->clipresult);
                }
            }
        }
    }

    pt_export_error_if(locs, error);
    return 0;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static const char *concat_iteration(PrefixIterator& prefix) {
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
