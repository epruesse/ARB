/*!
 * \brief ptpan_legacy.cxx
 *
 * \date 22.02.2011
 * \author Tilo Eissler
 *
 * Contributors: Chris Hodges, Jörg Böhnel
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>

#include "ptpan_legacy.h"

#include <PT_server_prototypes.h>
#include <struct_man.h>
#include <arbdb.h>
#include <arb_strbuf.h>
#include <arb_defs.h>

// all the stuff required for integration as PT_server replacement

/* /// "AllocPTPanLegacy()" */
struct PTPanLegacy * AllocPTPanLegacy() {

#ifdef DEBUG
    printf("Init PTPanLegacy struct ...\n");
#endif

    struct PTPanLegacy *pl = (struct PTPanLegacy *) calloc(1,
            sizeof(struct PTPanLegacy));
    if (!pl) {
        printf("Error allocating PTPanLegacyGlobal!\n");
        return (NULL);
    }
    pl->pl_pt = NULL;
    pl->pl_SearchPrefs = NULL;
    pl->pl_ComSocket = NULL;
    pl->pl_GbShell = NULL;
    pl->pl_ServerName = NULL;

    return (pl);
}
/* \\\ */

/* /// "FreePTPanGlobal()" */
void FreePTPanLegacy(struct PTPanLegacy *pl) {
    free(pl);
    pl = NULL;
}
/* \\\ */

// overloaded functions to avoid problems with type-punning:
inline void aisc_link(dll_public *dll, PT_tprobes *tprobe) {
    aisc_link(reinterpret_cast<dllpublic_ext*>(dll),
            reinterpret_cast<dllheader_ext*>(tprobe));
}
inline void aisc_link(dll_public *dll, PT_probeparts *parts) {
    aisc_link(reinterpret_cast<dllpublic_ext*>(dll),
            reinterpret_cast<dllheader_ext*>(parts));
}
inline void aisc_link(dll_public *dll, PT_probematch *match) {
    aisc_link(reinterpret_cast<dllpublic_ext*>(dll),
            reinterpret_cast<dllheader_ext*>(match));
}
inline void aisc_link(dll_public *dll, PT_family_list *family) {
    aisc_link(reinterpret_cast<dllpublic_ext*>(dll),
            reinterpret_cast<dllheader_ext*>(family));
}

/* /// "pt_init_bond_matrix()" */
extern "C" int pt_init_bond_matrix(PT_pdc *THIS) {
    THIS->bond[0].val = 0.0;
    THIS->bond[1].val = 0.0;
    THIS->bond[2].val = 0.5;
    THIS->bond[3].val = 1.1;
    THIS->bond[4].val = 0.0;
    THIS->bond[5].val = 0.0;
    THIS->bond[6].val = 1.5;
    THIS->bond[7].val = 0.0;
    THIS->bond[8].val = 0.5;
    THIS->bond[9].val = 1.5;
    THIS->bond[10].val = 0.4;
    THIS->bond[11].val = 0.9;
    THIS->bond[12].val = 1.1;
    THIS->bond[13].val = 0.0;
    THIS->bond[14].val = 0.9;
    THIS->bond[15].val = 0.0;
    return 0;
}
/* \\\ */

/*!
 * \brief Convert bond matrix
 */
void convertBondMatrix(PT_pdc *pdc, struct MismatchWeights *mw) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    const AbstractAlphabetSpecifics* as = pl->pl_pt->getAlphabetSpecifics();
    int start_code = as->wildcard_code + 1;
    for (int query = start_code; query < as->alphabet_size; ++query) {
        for (int species = start_code; species < as->alphabet_size; ++species) {
            int rowIdx = (as->complement_table[query] - start_code)
                    * (as->alphabet_size - 1);
            int maxIdx = rowIdx + query - start_code;
            int newIdx = rowIdx + species - start_code;

            double max_bind = pdc->bond[maxIdx].val;
            double new_bind = pdc->bond[newIdx].val;

            mw->mw_Replace[query * as->alphabet_size + species] = max_bind
                    - new_bind;
        }
    }
#if defined(DEBUG)
    printf("Current bond values:\n");
    for (int y = 0; y < (as->alphabet_size - 1); y++) {
        for (int x = 0; x < (as->alphabet_size - 1); x++) {
            printf("%5.2f", pdc->bond[y * (as->alphabet_size - 1) + x].val);
        }
        printf("\n");
    }
    printf("Current Replace Matrix:\n");
    for (int query = start_code; query < as->alphabet_size; ++query) {
        for (int species = start_code; species < as->alphabet_size; ++species) {
            printf("%5.2f",
                    mw->mw_Replace[query * as->alphabet_size + species]);
        }
        printf("\n");
    }
#endif // DEBUG
}

/*!
 * \brief Function to obtain gc content
 *
 * NOTE: use an existing ARB function instead! if there is one already
 */
static int ptpan_get_gc_content(char *probe) {
    int gc = 0;
    while (*probe) {
        switch (*probe) {
        case 'G':
        case 'g':
        case 'C':
        case 'c':
            gc++;
            break;
        default:
            break;
        }
        probe++;
    }
    return gc;
}

/* /// "get_design_info()" */
char *get_design_info(PT_tprobes *tprobe) {
#ifdef DEBUG
    printf("EXTERN: get_design_info\n");
#endif

    STRPTR buffer = (STRPTR) GB_give_buffer(2000);

    UWORD posgroup = 0;
    char possep = '=';
    LONG alignpos = tprobe->apos;

    PT_pdc *pdc = (PT_pdc *) tprobe->mh.parent->parent;
    STRPTR outptr = buffer;

    // find variable that is closest to the hit
    for (posgroup = 0; posgroup < 26; ++posgroup) {
        LONG dist;
        // see, if this group has been defined yet
        if (!(pdc->pos_groups[posgroup])) {
            pdc->pos_groups[posgroup] = tprobe->apos;
            break;
        }
        dist = tprobe->apos - pdc->pos_groups[posgroup];
        if ((dist >= 0) && (dist < pdc->probelen)) {
            alignpos = dist;
            possep = '+';
            break;
        }
        if ((dist < 0) && (dist > -pdc->probelen)) {
            alignpos = -dist;
            possep = '-';
            break;
        }
    }
    // generate output
    sprintf(
            buffer,
            "%s %2ld %c%c%6ld %4ld %6ld %-4.1f %-7.1f",
            tprobe->sequence,
            (ULONG) tprobe->seq_len,
            'A' + posgroup,
            possep,
            alignpos + 1,
            (ULONG) tprobe->get_r_pos + 1,
            (ULONG) tprobe->groupsize,
            (((double) ptpan_get_gc_content(tprobe->sequence)) / tprobe->seq_len
                    * 100.0), tprobe->temp);

    outptr += strlen(buffer);

    *outptr++ = ' ';
    LONG cnt;
    STRPTR srcptr = &tprobe->sequence[tprobe->seq_len];
    PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    const AbstractAlphabetSpecifics *as = pl->pl_pt->getAlphabetSpecifics();
    for (cnt = 0; cnt < tprobe->seq_len; cnt++) {
        if (as) {
            *outptr++ =
                    as->decompress_table[as->complement_table[as->compress_table[(int) *--srcptr]]];
        }
    }
    *outptr++ = ' ';
    *outptr++ = '|';

    ULONG sum = 0;
    for (cnt = 0; cnt < DECR_TEMP_CNT; ++cnt) {
        sum += tprobe->perc[cnt];
        sprintf(outptr, "%3ld;", sum);
        outptr += strlen(outptr);
    }
    *outptr = 0;
    return buffer;
}
/* \\\ */

/* /// "get_design_hinfo()" */
char *get_design_hinfo(PT_tprobes *tprobe) {
#ifdef DEBUG
    printf("EXTERN: get_design_hinfo\n");
#endif

    if (!tprobe) {
        return ((STRPTR) "Sorry, there are no probes for your selection!!!");
    }
    PT_pdc *pdc = (PT_pdc *) tprobe->mh.parent->parent;

    STRPTR buffer = (STRPTR) GB_give_buffer(2000);
    char *s = buffer;
    sprintf(s, "Probe design Parameters:\n"
            "Length of probe    %4i\n"
            "Temperature        [%4.1f -%4.1f ]\n"
            "GC-Content         [%4.1f%%-%4.1f%%]\n", pdc->probelen,
            pdc->mintemp, pdc->maxtemp, pdc->min_gc * 100.0,
            pdc->max_gc * 100.0);
    s += strlen(s);

    if (pdc->min_ecolipos == -1) {
        if (pdc->max_ecolipos == -1) {
            sprintf(s, "E.Coli Position    [any]\n");
        } else {
            sprintf(s, "E.Coli Position    [<= %4i]\n", pdc->max_ecolipos);
        }
    } else {
        if (pdc->max_ecolipos == -1) {
            sprintf(s, "E.Coli Position    [>= %4i]\n", pdc->min_ecolipos);
        } else {
            sprintf(s, "E.Coli Position    [%4i -%4i]\n", pdc->min_ecolipos,
                    pdc->max_ecolipos);
        }
    }
    s += strlen(s);

    sprintf(s, "Max Non Group Hits  %4i\n"
            "Min Group Hits      %4.0f%%\n", pdc->mishit,
            pdc->mintarget * 100.0);
    s += strlen(s);

    sprintf(s, "%-*s", pdc->probelen + 1, "Target");
    s += strlen(s);

    sprintf(s, "%2s %8s %4s ", "le", "apos", "ecol");
    s += strlen(s);

    sprintf(s, "%6s ", "  grps"); // groupsize
    s += strlen(s);

    sprintf(s, "%-4s %-7s %-*s | ", " G+C", "4GC+2AT", pdc->probelen,
            "Probe sequence");
    s += strlen(s);

    sprintf(
            s,
            "# of non-group hits for increasing ProbeMatch weighted mismatch values");
    // [max distance wmis = (column_nr - 1)*0.2]

    return buffer;
}
/* \\\ */

/* /// "markEntryGroup()" */
std::set<std::string> * markEntryGroup(STRPTR specnames) {
    if (!*specnames) {
        return (0); // string was empty!
    }

    std::string names(specnames);
    std::set<std::string> *splitted = new std::set<std::string>();

    boost::split(*splitted, names, boost::is_any_of("#"),
            boost::token_compress_on);
#ifdef DEBUG
    printf("Markcount %ld\n", splitted->size());
#endif
    return splitted;
}
/* \\\ */

/* /// "PT_start_design()" */
int PT_start_design(PT_pdc *pdc, int /* dummy */) {
    struct timeval tsStart, tsNow;
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
#ifndef DEBUG
    if (pl->pl_pt->isVerbose()) {
#endif
    printf("Start ProbeDesign ...\n");
    gettimeofday(&tsStart, NULL);
#ifndef DEBUG
}
#endif

    if (pdc->names.data && (*pdc->names.data != 0)) {

        MismatchWeights *mw = MismatchWeights::cloneMismatchWeightMatrix(
                pl->pl_pt->getAlphabetSpecifics()->mismatch_weights);
        convertBondMatrix(pdc, mw);

        DesignQuery dq = DesignQuery(mw, markEntryGroup(pdc->names.data));
        dq.dq_ProbeLength = pdc->probelen;
        dq.dq_MinGroupHits = (ULONG) (pdc->mintarget
                * (double) dq.dq_DesginIds->size() + 0.5);
        dq.dq_MaxNonGroupHits = pdc->mishit;
        dq.dq_MinTemp = pdc->mintemp;
        dq.dq_MaxTemp = pdc->maxtemp;
        dq.dq_MinGC = (ULONG) (pdc->min_gc * (double) pdc->probelen);
        dq.dq_MaxGC = (ULONG) (pdc->max_gc * (double) pdc->probelen + 0.5);
        dq.dq_MinPos = pdc->min_ecolipos;
        dq.dq_MaxPos = pdc->max_ecolipos;
        dq.dq_MaxHits = pdc->clipresult;
        dq.dq_SortMode = 0;

        //#ifdef DEBUG
        //    std::set<std::string>::const_iterator it;
        //    for (it = dq.dq_DesginIds->begin(); it != dq.dq_DesginIds->end(); it++) {
        //        printf("%s\n", it->data());
        //    }
        //#endif

        // now design probes ...
        boost::ptr_vector<DesignHit> results = pl->pl_pt->designProbes(dq);
        boost::ptr_vector<DesignHit>::const_iterator dh;
        for (dh = results.begin(); dh != results.end(); dh++) {
            // fill legacy structure
            PT_tprobes *tprobe = create_PT_tprobes();
            tprobe->sequence = strdup(dh->dh_ProbeSeq.data());
            tprobe->seq_len = dq.dq_ProbeLength;
            tprobe->temp = dh->dh_Temp;
            tprobe->groupsize = dh->dh_GroupHits;
            tprobe->mishit = dh->dh_NonGroupHits;
            tprobe->apos = dh->dh_relpos;
            tprobe->get_r_pos = (int) dh->dh_ref_pos;
            tprobe->quality = dh->dh_Quality;
            for (ULONG cnt = 0; cnt < DECR_TEMP_CNT; cnt++) {
                tprobe->perc[cnt] = dh->dh_NonGroupHitsPerc[cnt];
            }
            aisc_link((dllpublic_ext *) &(pdc->ptprobes),
                    (dllheader_ext *) tprobe);
        }
    }

#ifndef DEBUG
    if (pl->pl_pt->isVerbose()) {
#endif
    gettimeofday(&tsNow, NULL);
    printf("... done in %li seconds.\n", (tsNow.tv_sec - tsStart.tv_sec));
#ifndef DEBUG
}
#endif

    return 0;
}
/* \\\ */

/* get the name with a virtual function */
/* /// "virt_name()" */
STRPTR virt_name(PT_probematch *ml) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    return const_cast<STRPTR>(pl->pl_pt->getEntryId(ml->name));
}
/* \\\ */

/* /// "virt_fullname()" */
STRPTR virt_fullname(PT_probematch *ml) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    if (PTPanLegacyGlobalPtr->pl_pt->containsFeatures()) {
        return ml->psequence;
    }
    return const_cast<STRPTR>(pl->pl_pt->getEntryFullName(ml->name));
}
/* \\\ */

/*
 * called by: AISC
 */
/* /// "PT_unknown_names()" */
bytestring *PT_unknown_names(PT_pdc *pdc) {
#ifdef DEBUG
    printf("EXTERN: PT_unknown_names\n");
    // printf("'%s'\n", pdc->names.data);
#endif

    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;

    std::string names(pdc->names.data);
    if (names.empty()) {
        freeset(pl->pl_UnknownSpecies.data, (STRPTR) malloc(2));
        *pl->pl_UnknownSpecies.data = 0;
        pl->pl_UnknownSpecies.size = 1;

    } else {

        std::vector<std::string> splitted;

        boost::split(splitted, names, boost::is_any_of("#"),
                boost::token_compress_on);
        if (!splitted.empty()) {
#ifdef DEBUG
            printf("found %ld ids after splitting\n", splitted.size());
            if (splitted.size() == 1) {
                printf("'%s'\n", splitted[0].c_str());
            }
#endif

            std::ostringstream oss;
            std::vector<std::string>::const_iterator it;
            for (it = splitted.begin(); it != splitted.end(); it++) {
                if (!pl->pl_pt->containsEntry(*it)) {
                    oss << *it;
                }
            }
            std::string unknown_names = oss.str();
            freeset(pl->pl_UnknownSpecies.data,
                    (STRPTR) malloc(unknown_names.size() + 1));
            pl->pl_UnknownSpecies.data[unknown_names.size()] = 0;
            pl->pl_UnknownSpecies.size = unknown_names.size() + 1;

        } else {
            // THIS IS AN ERROR CASE
        }
    }
    return (&pl->pl_UnknownSpecies);
}
/* \\\ */

/* /// "ff_find_family()" */
int find_family(PT_family *ffinder, bytestring *species) {
    struct timeval tsStart, tsNow;
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
#ifndef DEBUG
    if (pl->pl_pt->isVerbose()) {
#endif
    printf("Start SimilaritySearch (= FindFamily) ...\n");
    gettimeofday(&tsStart, NULL);
#ifndef DEBUG
}
#endif
    // destroy old list
    while (ffinder->fl) {
        destroy_PT_family_list(ffinder->fl);
    }

    SimilaritySearchQuery ssq = SimilaritySearchQuery(species->data);
    ssq.ssq_ComplementMode = ffinder->complement;
    ssq.ssq_NumMaxMismatches = (double) ffinder->mis_nr;
    ssq.ssq_WindowSize = ffinder->pr_len;
    ssq.ssq_SearchMode = ffinder->find_type;
    ssq.ssq_SortType = ffinder->sort_type;
    ssq.ssq_RangeStart = ffinder->range_start;
    ssq.ssq_RangeEnd = ffinder->range_end;

    boost::ptr_vector<SimilaritySearchHit> values = pl->pl_pt->similaritySearch(
            ssq);
    ULONG count = 0; // for truncating
    boost::ptr_vector<SimilaritySearchHit>::const_iterator ssh;
    for (ssh = values.begin(); ssh != values.end() && count < ffinder->sort_max;
            ssh++) {
        PT_family_list *fl = create_PT_family_list();
        fl->name = strdup(ssh->ssh_Entry->pe_Name);
        fl->matches = ssh->ssh_Matches;
        fl->rel_matches = ssh->ssh_RelMatches;
        aisc_link(&ffinder->pfl, fl);
        count++;
    }
    ffinder->list_size = count;
    free(species->data);

#ifndef DEBUG
    if (pl->pl_pt->isVerbose()) {
#endif
    gettimeofday(&tsNow, NULL);
    printf("... done in %li seconds.\n", (tsNow.tv_sec - tsStart.tv_sec));
#ifndef DEBUG
}
#endif
    return 0;
}
/* \\\ */

/* /// "get_match_overlay()" */
//Peter Pan seems to store the already formated string in ml->sequence, so no further transformation is required
char *get_match_overlay(PT_probematch *ml) {
    if (PTPanLegacyGlobalPtr->pl_pt->containsFeatures()) {
        return &ml->psequence[ml->rpos + 1];
    }
    return ml->psequence;
}
/* \\\ */

// TODO I know, the following stuff is just a copy,
// but currently, I cannot access it and it's not my work
// to move it to a lib (TE 24.03.2011)
struct format_props {
    bool show_mismatches; // whether to show 'mis' and 'N_mis'
    bool show_ecoli; // whether to show 'ecoli' column
    bool show_gpos; // whether to show 'gpos' column

    int name_width; // width of 'name' column
    int gene_or_full_width; // width of 'genename' or 'fullname' column
    int pos_width; // max. width of pos column
    int gpos_width; // max. width of gpos column
    int ecoli_width; // max. width of ecoli column

    int rev_width() const {
        return 3;
    }
    int mis_width() const {
        return 3;
    }
    int N_mis_width() const {
        return 5;
    }
    int wmis_width() const {
        return 4;
    }
};

inline void set_max(const char *str, int &curr_max) {
    if (str) {
        int len = strlen(str);
        if (len > curr_max)
            curr_max = len;
    }
}

// I adopted this one to PTPan
static format_props detect_format_props(PT_local *locs, bool show_gpos) {
    PT_probematch *ml = locs->pm; // probe matches
    format_props format;

    format.show_mismatches = (ml->N_mismatches >= 0);
    format.show_ecoli = PTPanLegacyGlobalPtr->pl_pt->containsReferenceEntry(); // display only if there is ecoli
    format.show_gpos = show_gpos; // display only for gene probe matches

    // minimum values (caused by header widths) :
    // OLD format.name_width = gene_flag ? 8 : 4; // 'organism' or 'name'
    format.name_width = 4; // 'name'
    format.gene_or_full_width = 8; // 'genename' or 'fullname'
    format.pos_width = 3; // 'pos'
    format.gpos_width = 4; // 'gpos'
    format.ecoli_width = 5; // 'ecoli'

    const PTPanReferenceEntry *ref_entry =
            PTPanLegacyGlobalPtr->pl_pt->getReferenceEntry();
    for (; ml; ml = ml->next) {
        set_max(virt_name(ml), format.name_width);
        set_max(virt_fullname(ml), format.gene_or_full_width);
        set_max(GBS_global_string("%i", info2bio(ml->b_pos)), format.pos_width);
        if (show_gpos) {
            set_max(GBS_global_string("%i", info2bio(ml->g_pos)),
                    format.gpos_width);
        }
        if (format.show_ecoli) {
            set_max(
                    GBS_global_string("%li",
                            ref_entry->pre_ReferenceBaseTable[ml->b_pos + 1]),
                    format.ecoli_width);
        }
    }

    return format;
}

inline void cat_internal(GBS_strstruct *memfile, int len, const char *text,
        int width, char spacer, bool align_left) {
    if (len == width) {
        GBS_strcat(memfile, text); // text has exact len
    } else if (len > width) { // text to long
        char buf[width + 1];
        memcpy(buf, text, width);
        buf[width] = 0;
        GBS_strcat(memfile, buf);
    } else { // text is too short -> insert spaces
        int spaces = width - len;
        arb_assert(spaces > 0);
        char sp[spaces + 1];
        memset(sp, spacer, spaces);
        sp[spaces] = 0;

        if (align_left) {
            GBS_strcat(memfile, text);
            GBS_strcat(memfile, sp);
        } else {
            GBS_strcat(memfile, sp);
            GBS_strcat(memfile, text);
        }
    }
    GBS_chrcat(memfile, ' '); // one space behind each column
}
inline void cat_spaced_left(GBS_strstruct *memfile, const char *text,
        int width) {
    cat_internal(memfile, strlen(text), text, width, ' ', true);
}
inline void cat_spaced_right(GBS_strstruct *memfile, const char *text,
        int width) {
    cat_internal(memfile, strlen(text), text, width, ' ', false);
}
inline void cat_dashed_left(GBS_strstruct *memfile, const char *text,
        int width) {
    cat_internal(memfile, strlen(text), text, width, '-', true);
}
inline void cat_dashed_right(GBS_strstruct *memfile, const char *text,
        int width) {
    cat_internal(memfile, strlen(text), text, width, '-', false);
}

/* /// "match_string()" */
/* Create a big output string:  header\001name\001info\001name\001info....\000 */
bytestring * match_string(PT_local *locs) {
    PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    PT_probematch *ml;
    LONG entryCount = 0;

#ifdef DEBUG
    printf("EXTERN: match_string\n");
#endif

    if (pl->pl_ResultString.size > 1) {
        free(pl->pl_ResultString.data); // free old memory
    }
    for (ml = locs->pm; ml; ml = ml->next) { // count number of entries
        ++entryCount;
    }

    long init_size = (entryCount + 1) * 150; // 150 bytes per entry seems to be a good estimation
    // plus enough space for the header!!! (that's the +1)
    // ULONG length = 0;
    // STRPTR outptr = (STRPTR) malloc(init_size);
    // STRPTR srcptr = outptr;

    if (locs->pm) {
        bool gene_flag = false;
        if (pl->pl_pt->containsFeatures()) {
            gene_flag = true;
        }

        format_props format = detect_format_props(locs, gene_flag);

        GBS_strstruct *memfile = GBS_stropen(init_size);
        GBS_strcat(memfile, "    "); // one space more than in get_match_info_formatted()

        cat_dashed_left(memfile, gene_flag ? "organism" : "name",
                format.name_width);
        cat_dashed_left(memfile, gene_flag ? "genename" : "fullname",
                format.gene_or_full_width);

        if (format.show_mismatches) {
            cat_dashed_right(memfile, "mis", format.mis_width());
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

        if (locs->pm->N_mismatches >= 0) {
            char *seq =
                    locs->pm->reversed ? locs->pm_csequence : locs->pm_sequence;

            GBS_strcat(memfile, "         '");
            GBS_strcat(memfile, seq);
            GBS_strcat(memfile, "'");
        }
        GBS_chrcat(memfile, (char) 1);

        const PTPanReferenceEntry *ref_entry =
                PTPanLegacyGlobalPtr->pl_pt->getReferenceEntry();

        for (ml = locs->pm; ml; ml = ml->next) {
            GBS_strcat(memfile, virt_name(ml));
            GBS_chrcat(memfile, (char) 1);
            GBS_strcat(memfile, "  ");

            cat_spaced_left(memfile, virt_name(ml), format.name_width);
            cat_spaced_left(memfile, virt_fullname(ml),
                    format.gene_or_full_width);

            if (format.show_mismatches) {
                cat_spaced_right(memfile,
                        GBS_global_string("%i", ml->mismatches),
                        format.mis_width());
                cat_spaced_right(memfile,
                        GBS_global_string("%i", ml->N_mismatches),
                        format.N_mis_width());
            }
            cat_spaced_right(memfile,
                    GBS_global_string("%.1f", ml->wmismatches),
                    format.wmis_width());
            cat_spaced_right(memfile,
                    GBS_global_string("%i", info2bio(ml->b_pos)),
                    format.pos_width);
            if (format.show_gpos) {
                cat_spaced_right(memfile,
                        GBS_global_string("%i", info2bio(ml->g_pos)),
                        format.gpos_width);
            }
            if (format.show_ecoli && ref_entry) {
                cat_spaced_right(
                        memfile,
                        GBS_global_string(
                                "%li",
                                ref_entry->pre_ReferenceBaseTable[ml->b_pos + 1]),
                        format.ecoli_width);
            }
            cat_spaced_left(memfile, GBS_global_string("%i", ml->reversed),
                    format.rev_width());

            char *ref = get_match_overlay(ml);
            GBS_strcat(memfile, ref);

            GBS_chrcat(memfile, (char) 1);
        }

        // for us to avoid global struct!
        pl->pl_ResultString.data = GBS_strclose(memfile);
    } else {
        pl->pl_ResultString.data = "";
    }
    pl->pl_ResultString.size = strlen(pl->pl_ResultString.data) + 1;
#ifdef DEBUG
    // printf("== match_string: %s\n", pg->pg_ResultString.data);
    printf(
            "%li entries used %i bytes (<= %i MB) of buffer: %5.2f byte per entry\n",
            entryCount, pl->pl_ResultString.size,
            (pl->pl_ResultString.size >> 20) + 1,
            (double) pl->pl_ResultString.size / (double) entryCount);
#endif
    return (&pl->pl_ResultString);
}
/* \\\ */

/*!
 * \brief Create list of species where probe matches and #mismatches (for multiprobe)
 *
 * Format: name^1#mismatch^1name^1#mismatch....^0
 *         (where ^0 and ^1 are ASCII 0 and 1)
 *
 * Implements server function 'MP_MATCH_STRING'
 */
bytestring * MP_match_string(PT_local *locs) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;

#ifdef DEBUG
    printf("EXTERN: MP_match_string\n");
#endif
    // free old memory
    if (pl->pl_ResultMString.data) {
        free(pl->pl_ResultMString.data);
        pl->pl_ResultMString.size = 0;
    }

    LONG buflen = 100;
    ULONG entry_count = pl->pl_pt->countEntries();
    if (entry_count > 0) {
        buflen = entry_count * 25;
#ifdef DEBUG
        printf("buffer size: %lu\n", buflen);
#endif
    }
    STRPTR outptr = (STRPTR) malloc(buflen);
    pl->pl_ResultMString.data = outptr;

    buflen--; // space for termination byte

    ULONG byte_count = 0;
    // add each entry to the list
    PT_probematch *ml;
    STRPTR srcptr;
    ULONG real_hits = 0;
    for (ml = locs->pm; ml; ml = ml->next) {
        real_hits++;
        /* add the name */
        srcptr = virt_name(ml);
        while ((--buflen > 0) && (*outptr++ = *srcptr++)) {
            byte_count++;
        }
        *outptr = (char) 1;
        outptr++;
        byte_count++;
        --buflen;
        if (buflen <= 0) {
            printf(
                    "ERROR: buffer too small - see function MP_match_string(...) in file ptpan_legacy.cxx\n");
            break;
        }

        /* and and the mismatch and wmismatch count */
        char tempBuffer[128];
        sprintf(tempBuffer, "%2i\1%1.1f", ml->mismatches, ml->wmismatches);
        srcptr = tempBuffer;
        while ((--buflen > 0) && (*outptr++ = *srcptr++)) {
            byte_count++;
        }
        *outptr = (char) 1;
        outptr++;
        byte_count++;
        --buflen;
        if (buflen <= 0) {
            printf(
                    "ERROR: buffer too small - see function MP_match_string(...) in file ptpan_legacy.cxx\n");
            break;
        }
    }
    if (byte_count > 0) {
        /* terminate string */
        outptr[-1] = 0;

        printf("%ld\n", byte_count);
        pl->pl_ResultMString.size = byte_count;
        /* free unused memory */
        outptr = (STRPTR) realloc(pl->pl_ResultMString.data, byte_count);
        if (!outptr) {
            printf("realloc failed!\n");
        }
        pl->pl_ResultMString.data = outptr;
    } else {
        pl->pl_ResultMString.data = (STRPTR) realloc(pl->pl_ResultMString.data,
                1);
        pl->pl_ResultMString.size = 0;
        pl->pl_ResultMString.data[0] = 0;
    }
#ifdef DEBUG
    //printf("\n\"");
    //ULONG i;
    //for (i = 0; i < byte_count; i++) {
    //printf("%c", outptr[i]);
    //fflush(stdout);
    //}
    //printf("\"\n%lu\n", i);
    //fflush(stdout);
    printf(
            "%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n",
            real_hits, byte_count, byte_count >> 20,
            (double) byte_count / (double) real_hits);
#endif
    return (&pl->pl_ResultMString);
}

/*!
 * \brief Create list of all species known to PTPan server
 *
 * Format: ^1name^1name....^0
 *         (where ^0 and ^1 are ASCII 0 and 1)
 *
 * Implements server function 'MP_ALL_SPECIES_STRING'
 */
bytestring * MP_all_species_string(PT_local *) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
#ifdef DEBUG
    printf("EXTERN: MP_all_species_string\n");
#endif
    /* free old memory */
    if (pl->pl_EntriesString.data) {
        free(pl->pl_EntriesString.data);
    }

    LONG buflen = 2;
    ULONG entry_count = pl->pl_pt->countEntries();
    if (entry_count > 0) {
        buflen = entry_count * 20;
    }

    STRPTR outptr = (STRPTR) calloc(buflen, 1);
    pl->pl_EntriesString.data = outptr;

    buflen--; /* space for termination byte */

    LONG byte_count = 0;
    STRPTR srcptr;
    /* add each entry to the list */
    struct PTPanEntry *ps = (struct PTPanEntry *) pl->pl_pt->getFirstEntry();
    if (ps) {
        while (ps->pe_Node.ln_Succ) {
            // add the name
            srcptr = ps->pe_Name;
            while ((--buflen > 0) && (*outptr++ = *srcptr++)) {
                byte_count++;
            }
            *outptr++ = (char) 1;
            byte_count++;
            --buflen;
            if (buflen <= 0) {
                printf(
                        "ERROR: buffer too small - see function MP_all_species_string(...) in file ptpan_legacy.cxx\n");
                break;
            }
            ps = (struct PTPanEntry *) ps->pe_Node.ln_Succ;
        }
    }
    /* terminate string */
    outptr[-1] = 0;

    pl->pl_EntriesString.size = byte_count;
    // free unused memory
    pl->pl_EntriesString.data = (STRPTR) realloc(pl->pl_EntriesString.data,
            pl->pl_EntriesString.size);

#ifdef DEBUG
    //for (ULONG i = 0; i < byte_count; i++) {
    //printf("%c", pl->pl_EntriesString.data[i]);
    //}
    //printf("\n");
    printf(
            "%li entries used %li bytes (%li MB) of buffer: %5.2f byte per entry\n",
            entry_count, (byte_count), (byte_count) >> 20,
            (double) (byte_count) / (double) entry_count);
#endif
    return (&pl->pl_EntriesString);
}

/* /// "MP_count_all_species()" */
int MP_count_all_species(PT_local *) {
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    printf("EXTERN: MP_count_all_species\n");
    return (pl->pl_pt->countEntries());
}
/* \\\ */

/*
 * /// "create_match_list()"
 *
 * \param limit 0 means unlimited
 */
ULONG create_match_list(const SearchQuery& sq,
        const boost::ptr_vector<QueryHit>& results, ULONG limit) {
#ifdef DEBUG
    printf("create_match_list(...) for %ld results\n", results.size());
#endif
    ULONG done = 0;
    if (results.size() > 0) {
        struct PTPanEntry *pe;
        boost::ptr_vector<QueryHit>::const_iterator hit;
        ULONG query_length = sq.sq_Query.size();

        if (PTPanLegacyGlobalPtr->pl_pt->containsFeatures()) {

            for (hit = results.begin(); hit != results.end(); hit++) {
                pe = hit->qh_Entry;
                if (pe->pe_FeatureContainer) {
                    ULONG pos = hit->qh_AbsPos - pe->pe_AbsOffset;

                    std::vector<PTPanFeature*> results =
                            pe->pe_FeatureContainer->getFeature(pos);

                    if (!results.empty()) {
                        std::vector<PTPanFeature*>::const_iterator it;
                        for (it = results.begin(); it != results.end(); it++) {

                            // check if hit crosses feature border!
                            if ((*it)->inSameRange(
                                    pos,
                                    query_length + hit->qh_InsertCount
                                            - hit->qh_DeleteCount)) {

                                PT_probematch *ml = create_PT_probematch();
                                done++;
                                ml->name = pe->pe_Num;
                                // UGLY PROBLEM: we need a second identifier to hand over the
                                // Feature pointer or pointer to name!!
                                // UGLY HACK: use  ml->rpos for other purpose:
                                ml->rpos = strlen((*it)->pf_name);
                                ml->b_pos = hit->qh_relpos;
                                std::pair<ULONG, LONG> borders =
                                        (*it)->getBorderDistance(
                                                pos,
                                                query_length
                                                        + hit->qh_InsertCount
                                                        - hit->qh_DeleteCount);
                                //ml->g_pos = (*it)->getPositionInFeature(pos);
                                ml->g_pos = borders.first;
                                // we don't need this: ml->rpos = pos; so we use it for another purpose!

                                ml->wmismatches = (double) hit->qh_WMisCount;
                                ml->mismatches = hit->qh_ReplaceCount
                                        + hit->qh_InsertCount
                                        + hit->qh_DeleteCount;
                                ml->N_mismatches = hit->qh_nmismatch;
                                ml->psequence = (STRPTR) calloc(
                                        ml->rpos + 1 + 21 + query_length, 0x01);
                                memcpy(ml->psequence, (*it)->pf_name, ml->rpos);
                                memcpy(&ml->psequence[ml->rpos + 1],
                                        hit->qh_seqout.data(),
                                        20 + query_length);

                                if (borders.first < 9) {
                                    for (int i = 0; i < 9 - borders.first;
                                            i++) {
                                        ml->psequence[ml->rpos + 1 + i] = '.';
                                    }
                                }
                                if (borders.second < 9) {
                                    for (int i = 8; i > borders.second; i--) {
                                        ml->psequence[ml->rpos + 1 + 11
                                                + query_length + i] = '.';
                                    }
                                }

                                ml->reversed =
                                        (hit->qh_Flags & QHF_REVERSED) ? 1 : 0;

                                aisc_link(
                                        (dllpublic_ext *) &(PTPanLegacyGlobalPtr->pl_SearchPrefs->ppm),
                                        (dllheader_ext *) ml);

                                if (limit != 0 && done >= limit) {
                                    return done;
                                }
                            }
                        }
                    }
                }
            }

        } else {
            std::size_t length = 0;
            for (hit = results.begin(); hit != results.end(); hit++) {
                pe = hit->qh_Entry;
                PT_probematch *ml = create_PT_probematch();
                done++;
                ml->name = pe->pe_Num;
                ml->b_pos = hit->qh_relpos;
                // we don't neeed this: ml->rpos = qh->qh_AbsPos - pe->pe_AbsOffset;
                ml->wmismatches = (double) hit->qh_WMisCount;
                ml->mismatches = hit->qh_ReplaceCount + hit->qh_InsertCount
                        + hit->qh_DeleteCount;
                ml->N_mismatches = hit->qh_nmismatch;
                length = hit->qh_seqout.size();
                ml->psequence = (STRPTR) malloc(length + 1);
                memcpy(ml->psequence, hit->qh_seqout.data(), length);
                ml->psequence[length] = 0;
                ml->reversed = (hit->qh_Flags & QHF_REVERSED) ? 1 : 0;

                aisc_link(
                        (dllpublic_ext *) &(PTPanLegacyGlobalPtr->pl_SearchPrefs->ppm),
                        (dllheader_ext *) ml);

                if (limit != 0 && done >= limit) {
                    break;
                }
            }
        }
    }
    return done;
}
/* \\\ */

/* /// "probe_match()" */
int probe_match(PT_local *locs, aisc_string probestring) {

    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    pl->pl_SearchPrefs = locs;
    // f PDC structure is missing, create it and
    // set flag to delete it in this method again!
    bool pdc_empty = false;
    if (!locs->pdc) {
        locs->pdc = create_PT_pdc();
        pdc_empty = true;
    }

    //#ifdef DEBUG
    //// find out where a given probe matches
    //      printf(
    //          "Search request for %s (errs = %d, compl = %d, rev = %d, weight = %d)\n",
    //                probestring, locs->pm_max,
    //                locs->pm_complement,
    //                locs->pm_reversed, locs->sort_by);
    //#endif

    // free the old sequence
    if (locs->pm_sequence) {
        free(locs->pm_sequence);
    }
    const AbstractAlphabetSpecifics *as = pl->pl_pt->getAlphabetSpecifics();
    locs->pm_sequence = as->filterSequence(probestring);

    /* do we need to check the complement instead of the normal one? */
    if (locs->pm_complement) {
        as->complementSequence(locs->pm_sequence);
    }

    // do we need to look at the reversed sequence as well?
    if (locs->pm_reversed) {
        if (locs->pm_csequence) {
            free(locs->pm_csequence);
        }
        locs->pm_csequence = strdup(locs->pm_sequence);
        as->reverseSequence(locs->pm_csequence);
        as->complementSequence(locs->pm_csequence);
    }

    // clear all old matches
    PT_probematch *ml;
    while ((ml = locs->pm)) {
        destroy_PT_probematch(ml);
    }

    // check, if the probe string is too short
    if (strlen(locs->pm_sequence) + (2 * locs->pm_max) < MIN_PROBE_LENGTH) {
        if (locs->ls_error) {
            free(locs->ls_error);
        }
        locs->ls_error = strdup((STRPTR) "error: probe too short!!\n");
        if (pdc_empty && locs->pdc) {
            destroy_PT_pdc(locs->pdc);
            locs->pdc = NULL;
        }
        free(probestring);
        return (0);
    }

    // allocate query that configures and holds all the merged results
    struct MismatchWeights* mws = MismatchWeights::cloneMismatchWeightMatrix(
            as->mismatch_weights);
    // for wmis calculation and/or for error counting
    if (!pdc_empty) {
        convertBondMatrix(locs->pdc, mws);
    }
    SearchQuery sq = SearchQuery(mws); // note: SearchQuery take ownership of MismatchWeights!
    if (locs->sort_by) {
        sq.sq_WeightedSearchMode = true;
    } else {
        sq.sq_WeightedSearchMode = false;
    }

#ifdef DEBUG
    printf("Current Replace Matrix:\n");
    int start_code = as->wildcard_code;
    for (int query = start_code; query < as->alphabet_size; ++query) {
        for (int species = start_code; species < as->alphabet_size; ++species) {
            printf(
                    "%5.2f",
                    sq.sq_MismatchWeights->mw_Replace[query * as->alphabet_size
                            + species]);
        }
        printf("\n");
    }
#endif
    sq.sq_Query = std::string(locs->pm_sequence);
    sq.sq_MaxErrors = (float) locs->pm_max;
    if (locs->pm_reversed) {
        // include reversed-complement search as well
        sq.sq_Reversed = true;
    } else {
        sq.sq_Reversed = false;
    }
    if (sq.sq_MaxErrors > 0) {
        sq.sq_AllowReplace = true; // N-mismatches are listed anyways!
        sq.sq_AllowInsert = true;
        sq.sq_AllowDelete = true;
    }
    sq.sq_KillNSeqsAt = sq.sq_Query.size() / 3; // TODO FIXME limit number of N-mismatches (locs->pm_nmatches_ignored, locs->pm_nmatches_limit)
    sq.sq_MinorMisThres = locs->pdc->split;
    if (locs->sort_by == 0) { // 0 is non-weighted
        sq.sq_WeightedSearchMode = false;
    } else {
        sq.sq_WeightedSearchMode = true;
    }

#ifdef DEBUG
    printf("Prepared everything, now search!\n");
#endif
    boost::ptr_vector<QueryHit> results = pl->pl_pt->searchTree(sq, true, true);
    ULONG done = create_match_list(sq, results, locs->pm_max_hits);

#ifdef DEBUG
    if (done) {
        printf("done with search!\n");
    }
#endif

    // Just to be secure, we clear the PDC structure
    // but only if we created it!
    if (pdc_empty && locs->pdc) {
        destroy_PT_pdc(locs->pdc);
        locs->pdc = NULL;
    }
    free(probestring); // is this really required??
    return 0;
}
/* \\\ */

/* /// "PT_find_exProb()" */
int PT_find_exProb(PT_exProb *pep, int dummy_1x) {
#ifdef DEBUG
    printf("EXTERN: PT_find_exProb\n");
#endif
    // free old result
    freenull(pep->result);
    // check for restart
    if (pep->restart) {
        if (pep->next_probe.data) {
            freenull(pep->next_probe.data);
            pep->next_probe.size = 0;
        }
        pep->restart = 0;
    }
    struct PTPanLegacy *pl = PTPanLegacyGlobalPtr;
    // TODO FIXME use temporary storage and getAllProbes(ULONG length) instead
    // may use a counter indicating next not yet retrieved!?
    // or index somehow!?
    STRPTR outptr = pl->pl_pt->getProbes(pep->next_probe.data, pep->plength,
            pep->numget);
    if (pep->next_probe.data) {
        freenull(pep->next_probe.data);
        pep->next_probe.size = 0;
    }
    if (outptr) {
        STRPTR tmp_outptr = outptr;
        while (++tmp_outptr) {
            ;
        }
        pep->next_probe.data = (char *) calloc(pep->plength + 1, 1);
        for (int i = pep->plength - 1; i >= 0; i--) {
            pep->next_probe.data[i] = *tmp_outptr;
            --tmp_outptr;
        }
        pep->next_probe.size = pep->plength;
        pep->result = outptr;
    } else {
        pep->result = strdup(""); // empty string
    }
    return (0);
}
/* \\\ */
