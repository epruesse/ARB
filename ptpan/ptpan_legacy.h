/*!
 * \file ptpan_legacy.h
 *
 * \date 09.02.2011
 * \author Tilo Eissler
 */

#ifndef PTPAN_LEGACY_H_
#define PTPAN_LEGACY_H_

#include "ptpan_tree.h"

#include <PT_server.h>
#include <PT_com.h>

/* minimum length of probe for searching */
#define MIN_PROBE_LENGTH 5

// global pointer to PTPan legacy struct
extern struct PTPanLegacy *PTPanLegacyGlobalPtr;

class GB_shell;

/*!
 * \brief Struct with the required legaxy pointer
 */
struct PTPanLegacy {
    // pointer to PTPanTree
    PtpanTree *pl_pt;

    /* communication stuff */
    STRPTR pl_ServerName; /* hostname of server */
    struct Hs_struct *pl_ComSocket; /* the communication socket */
    GB_shell *pl_GbShell;

    /* output stuff */
    PT_local *pl_SearchPrefs; /* search prefs from GUI */
    bytestring pl_ResultString; /* complete merged output string (header/name/format) */
    bytestring pl_ResultMString; /* complete merged output string (name/mismatch/wmis) */
    bytestring pl_EntriesString; /* complete merged entries string (entries) */
    bytestring pl_UnknownSpecies; /* string containing the unknown entries */

};

struct PTPanLegacy * AllocPTPanLegacy();
void FreePTPanLegacy(struct PTPanLegacy *plg);

void convertBondMatrix(PT_pdc *pdc, struct MismatchWeights *mw);
std::set<std::string> * markEntryGroup(STRPTR specnames);

int server_shutdown(PT_main *, aisc_string passwd);
int broadcast(PT_main *main, int dummy_1x);

extern "C" int pt_init_bond_matrix(PT_pdc * THIS);
char *get_design_info(PT_tprobes * tprobe);
char *get_design_hinfo(PT_tprobes * tprobe);
int PT_start_design(PT_pdc *pdc, int dummy_1x);

STRPTR virt_name(PT_probematch * ml);
STRPTR virt_fullname(PT_probematch * ml);
bytestring *PT_unknown_names(PT_pdc * pdc);

int find_family(PT_family *ffinder, bytestring *species);

char *get_match_overlay(PT_probematch * ml);
int probe_match(PT_local *locs, aisc_string probestring);
bytestring *match_string(PT_local * locs);
bytestring *MP_match_string(PT_local * locs);
bytestring *MP_all_species_string(PT_local *);
int MP_count_all_species(PT_local *);

int PT_find_exProb(PT_exProb * pep, int dummy_1x);

#endif /* PTPAN_LEGACY_H_ */
