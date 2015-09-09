// ============================================================ //
//                                                              //
//   File      : itemtools.cxx                                  //
//   Purpose   : item-specific toolkit                          //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "items.h"
#include <arb_msg.h>
#include <aw_window.hxx>

inline const char *itemTypeSpecificWindowID(const ItemSelector& selector, const char *window_id) {
    if (selector.type == QUERY_ITEM_SPECIES) {
        return window_id; // for species return given window id
    }
    const char *item_type_id = NULL;
    switch (selector.type) {
        case QUERY_ITEM_GENES:       item_type_id = "GENE";       break;
        case QUERY_ITEM_EXPERIMENTS: item_type_id = "EXPERIMENT"; break;

        case QUERY_ITEM_SPECIES:
        case QUERY_ITEM_TYPES: it_assert(0); break;
    }
    it_assert(item_type_id);
    return GBS_global_string("%s_%s", window_id, item_type_id);
}

void init_itemType_specific_window(AW_root *aw_root, AW_window_simple *aws, const ItemSelector& itemType, const char *id, const char *title_format, bool plural) {
    const char *s_id    = itemTypeSpecificWindowID(itemType, id);
    char       *s_title = GBS_global_string_copy(title_format, plural ? itemType.items_name : itemType.item_name);

    aws->init(aw_root, s_id, s_title);
    free(s_title);
}

