// ============================================================ //
//                                                              //
//   File      : refentries.h                                   //
//   Purpose   : functions for fields containing references to  //
//               other item (as names)                          //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef REFENTRIES_H
#define REFENTRIES_H

#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef AWT_HXX
#include "awt.hxx"
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef DBITEM_SET_H
#include <dbitem_set.h>
#endif


class AW_root;
class AW_window;

namespace RefEntries {

    typedef ARB_ERROR (*referred_item_handler)(GBDATA *gb_main, const DBItemSet& referred);                // called with all referred items

    class ReferringEntriesHandler {
        GBDATA *gb_main;
        ad_item_selector itemtype;

    public:
        ReferringEntriesHandler(GBDATA *gb_main_, const ad_item_selector& itemtype_)
            : gb_main(gb_main_), 
              itemtype(itemtype_)
        {
        }

        GBDATA *get_gbmain() const { return gb_main; }
        const ad_item_selector& get_referring_item() const { return itemtype; }


        ARB_ERROR with_all_referred_items(GBDATA *gb_item, const char *refs_field, bool error_if_unknown_ref, referred_item_handler cb);
        ARB_ERROR with_all_referred_items(AWT_QUERY_RANGE range, const char *refs_field, bool error_if_unknown_ref, referred_item_handler cb);
    };

    // --------------------------
    //      GUI related below

    typedef void (*client_area_builder)(AW_window *aw_reh); // callback to build client area in RefEntriesHandler window

    AW_window *create_refentries_window(AW_root *awr, ReferringEntriesHandler *reh, const char *window_id, const char *title, const char *help, client_area_builder build_client_area, const char *action, referred_item_handler action_cb);
    void create_refentries_awars(AW_root *aw_root, AW_default aw_def);
};

#else
#error refentries.h included twice
#endif // REFENTRIES_H
