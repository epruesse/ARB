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
#ifndef ARB_STRING_H
#include <arb_string.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef DBITEM_SET_H
#include <dbitem_set.h>
#endif
#ifndef ITEMS_H
#include <items.h>
#endif

#define re_assert(cond) arb_assert(cond)


namespace RefEntries {

    typedef ARB_ERROR (*referred_item_handler)(GBDATA *gb_main, const DBItemSet& referred);                // called with all referred items

    class RefSelector : virtual Noncopyable {
        char *field; // name of field containing references
        char *aci;   // postprocess expression for field-content

        bool error_if_field_missing;
        bool error_if_ref_unknown;
    public:
        RefSelector(const char *field_, const char *aci_, bool error_if_field_missing_, bool error_if_ref_unknown_)
            : field(ARB_strdup(field_)),
              aci(ARB_strdup(aci_)),
              error_if_field_missing(error_if_field_missing_),
              error_if_ref_unknown(error_if_ref_unknown_)
        {}
        ~RefSelector() {
            free(aci);
            free(field);
        }

        const char *get_refs(ItemSelector& itemtype, GBDATA *gb_item) const;
        char *filter_refs(const char *refs, GBDATA *gb_item) const;

        bool ignore_unknown_refs() const { return !error_if_ref_unknown; }
        const char *get_field() const { return field; }
    };

    class ReferringEntriesHandler {
        GBDATA        *gb_main;
        ItemSelector&  itemtype;

    public:
        ReferringEntriesHandler(GBDATA *gb_main_, ItemSelector& itemtype_)
            : gb_main(gb_main_),
              itemtype(itemtype_)
        {
            re_assert(&itemtype);
        }

        GBDATA *get_gbmain() const { return gb_main; }
        ItemSelector& get_referring_item() const { return itemtype; }


        ARB_ERROR with_all_referred_items(GBDATA *gb_item, const RefSelector& refsel, referred_item_handler cb);
        ARB_ERROR with_all_referred_items(QUERY_RANGE range, const RefSelector& refsel, referred_item_handler cb);
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
