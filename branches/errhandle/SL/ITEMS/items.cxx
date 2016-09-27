// ============================================================ //
//                                                              //
//   File      : items.cxx                                      //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "items.h"
#include <arbdb.h>

GBDATA *MutableBoundItemSel::get_any_item() const {
    /*! get any item of this type
     * @return NULL of no item exists. Otherwise the "first item" is returned.
     */
    GB_transaction ta(gb_main);

    GBDATA *gb_container = selector.get_first_item_container(gb_main, NULL, QUERY_ALL_ITEMS);
    while (gb_container) {
        GBDATA *gb_item = selector.get_first_item(gb_container, QUERY_ALL_ITEMS);
        if (gb_item) return gb_item;

        gb_container = selector.get_next_item_container(gb_container, QUERY_ALL_ITEMS);
    }

    return NULL;
}


