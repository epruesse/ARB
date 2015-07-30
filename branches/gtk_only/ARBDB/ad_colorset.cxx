// ============================================================ //
//                                                              //
//   File      : ad_colorset.cxx                                //
//   Purpose   : item-colors and colorsets                      //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "ad_colorset.h"
#include "arbdbt.h"

#include <arb_strbuf.h>

long GBT_get_color_group(GBDATA *gb_item) {
    /*! return color group of item (special, gene, ...)
     * @param gb_item the item
     * @return color group if defined (0 if none)
     */
    GBDATA *gb_cgroup = GB_entry(gb_item, GB_COLORGROUP_ENTRY);
    return gb_cgroup ? GB_read_int(gb_cgroup) : 0;
}
GB_ERROR GBT_set_color_group(GBDATA *gb_item, long color_group) {
    /*! set color group of item
     * @param gb_item the item
     * @param color_group the color group [1...]
     * @return error if sth went wrong
     */

    if (color_group) return GBT_write_int(gb_item, GB_COLORGROUP_ENTRY, color_group);

    // do not store 0
    GBDATA *gb_cgroup = GB_entry(gb_item, GB_COLORGROUP_ENTRY);
    return gb_cgroup ? GB_delete(gb_cgroup) : NULL;
}

GBDATA *GBT_colorset_root(GBDATA *gb_main, const char *itemsname) {
    /*! return root container for colorsets
     * @param gb_main database
     * @param itemsname name of items ( == ItemSelector::items_name)
     */
    GBDATA *gb_colorsets = GB_search(gb_main, "colorsets", GB_CREATE_CONTAINER);
    GBDATA *gb_item_root = gb_colorsets ? GB_search(gb_colorsets, itemsname, GB_CREATE_CONTAINER) : NULL;
    return gb_item_root;
}

void GBT_get_colorset_names(ConstStrArray& colorsetNames, GBDATA *gb_colorset_root) {
    /*! retrieve names of existing colorsets from DB
     * @param colorsetNames result-param: will be filled with colorset names
     * @param gb_colorset_root result of GBT_colorset_root()
     */
    for (GBDATA *gb_colorset = GB_entry(gb_colorset_root, "colorset");
         gb_colorset;
         gb_colorset = GB_nextEntry(gb_colorset))
    {
        const char *name = GBT_read_name(gb_colorset);
        colorsetNames.put(name);
    }
}

GBDATA *GBT_find_colorset(GBDATA *gb_colorset_root, const char *name) {
    /*! lookup (existing) colorset
     * @param gb_colorset_root result of GBT_colorset_root()
     * @param name colorset name
     * @return colorset DB entry (or NULL, in which case an error MAY be exported)
     */
    return GBT_find_item_rel_item_data(gb_colorset_root, "name", name);
}

GBDATA *GBT_find_or_create_colorset(GBDATA *gb_colorset_root, const char *name) {
    /*! create a new colorset
     * @param gb_colorset_root result of GBT_colorset_root()
     * @param name colorset name
     * @return colorset DB entry (or NULL, in which case an error IS exported)
     */
    return GBT_find_or_create_item_rel_item_data(gb_colorset_root, "colorset", "name", name, false);
}

GB_ERROR GBT_load_colorset(GBDATA *gb_colorset, ConstStrArray& colorsetDefs) {
    /*! load a colorset
     * @param gb_colorset result of GBT_find_colorset()
     * @param colorsetDefs result-param: will be filled with colorset-entries ("itemname=color")
     * @return error if sth went wrong
     */

    GB_ERROR  error    = NULL;
    char     *colorset = GBT_read_string(gb_colorset, "color_set");
    if (!colorset) {
        error = GB_have_error() ? GB_await_error() : "Missing 'color_set' entry";
        error = GBS_global_string("Failed to read colorset (Reason: %s)", error);
    }
    else {
        GBT_splitNdestroy_string(colorsetDefs, colorset, ';');
    }
    return error;
}

GB_ERROR GBT_save_colorset(GBDATA *gb_colorset, CharPtrArray& colorsetDefs) {
    /*! saves a colorset
     * @param gb_colorset result of GBT_find_colorset() or GBT_create_colorset()
     * @param colorsetDefs contains colorset-entries ("itemname=color")
     * @return error if sth went wrong
     */
    GBS_strstruct buffer(colorsetDefs.size()*(8+1+2+1));
    for (size_t d = 0; d<colorsetDefs.size(); ++d) {
        buffer.cat(colorsetDefs[d]);
        buffer.put(';');
    }
    buffer.cut_tail(1); // remove trailing ';'

    return GBT_write_string(gb_colorset, "color_set", buffer.get_data());
}


