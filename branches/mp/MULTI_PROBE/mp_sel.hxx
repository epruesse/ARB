// ============================================================ //
//                                                              //
//   File      : mp_sel.hxx                                     //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2012   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef MP_SEL_HXX
#define MP_SEL_HXX

#ifndef MPDEFS_H
#include "mpdefs.h"
#endif
#ifndef AW_SELECT_HXX
#include <aw_select.hxx>
#endif

#define IPL_ENTRYSIZE (1+1 + 1+1 + 6+1 + 30+1)
inline const char *formatInputProbeListEntry(int quality, int singleMismatches, int ecoliPos, const char *probeString) {
    static char buffer[IPL_ENTRYSIZE+1];
#if defined(DEBUG)
    int printed = 
#endif
        sprintf(buffer, "%1d#%1d#%6d#%s", quality, singleMismatches, ecoliPos, probeString);
    mp_assert(printed<IPL_ENTRYSIZE);
    return buffer;
}

inline void sortNrefresh_selected_list() {
    selected_list->sort(false, true);
    selected_list->update();
}

inline void addTo_selected_list(const char *entry) {
    selected_list->insert(entry, entry);
    sortNrefresh_selected_list();
    selected_list->set_awar_value(entry);
}

#else
#error mp_sel.hxx included twice
#endif // MP_SEL_HXX
