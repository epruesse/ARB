// ==================================================================== //
//                                                                      //
//   File      : db_scanner.hxx                                         //
//   Purpose   : ARB database scanner                                   //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef DB_SCANNER_HXX
#define DB_SCANNER_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

/* Database scanner boxes
 *
 * A scanner show all (recursive) information of a database entry.
 *
 * This information can be organized in two different ways:
 * 1. DB_SCANNER: Show exact all (filtered) information stored in the DB
 * 2. DB_VIEWER:  Create a list of all database fields (see FIELD INFORMATION)
 *                and if any information is stored under a field append it.
 *
 *    example:
 *      fields:
 *              name, full_name, acc, author
 *
 *      DB entries:
 *              name:e.coli
 *              full_name:esc.coli
 *              flag:test
 *
 *     ->
 *              name:       e.coli
 *              full_name:  esc.coli
 *              acc:
 *              author:
 */

typedef enum {
    DB_SCANNER,
    DB_VIEWER
} DB_SCANNERMODE;

struct ad_item_selector;

AW_CL create_db_scanner(GBDATA                 *gb_main,
                        AW_window              *aws,
                        const char             *box_pos_fig, /* the position for the box in the xfig file */
                        const char             *delete_pos_fig,
                        const char             *edit_pos_fig,
                        const char             *edit_enable_pos_fig,
                        DB_SCANNERMODE          scannermode,
                        const char             *rescan_pos_fig, // DB_VIEWER
                        const char             *mark_pos_fig,
                        long                    type_filter,
                        const ad_item_selector *selector);

void map_db_scanner(AW_CL arbdb_scanid, GBDATA *gb_pntr, const char *key_path);


GBDATA *get_db_scanner_main(AW_CL arbdb_scanid);

#else
#error db_scanner.hxx included twice
#endif // DB_SCANNER_HXX

