// =========================================================== //
//                                                             //
//   File      : awtlocal.hxx                                  //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWTLOCAL_HXX
#define AWTLOCAL_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


#define AWAR_TABLE_FIELD_REORDER_SOURCE_TEMPLATE "tmp/table/%s/field/reorder_source"
#define AWAR_TABLE_FIELD_REORDER_DEST_TEMPLATE  "tmp/table/%s/field/reorder_dest"
#define AWAR_TABLE_FIELD_NEW_NAME_TEMPLATE      "tmp/table/%s/new_name"
#define AWAR_TABLE_FIELD_REM_TEMPLATE           "tmp/table/%s/rem"
#define AWAR_TABLE_FIELD_NEW_TYPE_TEMPLATE      "tmp/table/%s/new_type"
#define AWAR_TABLE_SELECTED_FIELD_TEMPLATE      "tmp/table/%s/selected_field"

struct awt_table : virtual Noncopyable {
    GBDATA *gb_main;
    char   *table_name;
    char   *awar_field_reorder_source;
    char   *awar_field_reorder_dest;
    char   *awar_field_new_name;
    char   *awar_field_new_type;
    char   *awar_field_rem;
    char   *awar_selected_field;
    awt_table(GBDATA *gb_main, AW_root *awr, const char *table_name);
    ~awt_table();
};

#define AWAR_TABLE_NAME   "tmp/ad_table/table_name"
#define AWAR_TABLE_DEST   "tmp/ad_table/table_dest"
#define AWAR_TABLE_REM    "tmp/ad_table/table_rem"
#define AWAR_TABLE_EXPORT "tmp/ad_table/export_table"
#define AWAR_TABLE_IMPORT "tmp/ad_table/import_table"


#else
#error awtlocal.hxx included twice
#endif // AWTLOCAL_HXX


