// ==================================================================== //
//                                                                      //
//   File      : db_scanner.hxx                                         //
//   Purpose   : ARB database scanner                                   //
//   Time-stamp: <Tue May/24/2005 13:22 MET Coder@ReallySoft.de>        //
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

/**************************************************************************
 *********************   Various Database SCANNER Boxes  *******************
 ***************************************************************************/
/*  A scanner show all (rekursiv) information of a database entry:
    This information can be organized in two different ways:
    1. AWT_SCANNER: Show exact all (filtered) information stored in the DB
    2. AWT_VIEWER:  Create a list of all database fields (see FIELD INFORMATIONS)
    and if any information is stored under a field append it.
    example: fields:    name, full_name, acc, author
    DB entries:    name:e.coli full_name:esc.coli flag:test
    ->  name:       e.coli
    full_name:  esc.coli
    acc:
    author:
*/

typedef enum {
    AWT_SCANNER,
    AWT_VIEWER
} AWT_SCANNERMODE;


AW_CL awt_create_arbdb_scanner(GBDATA                 *gb_main, AW_window *aws,
                               const char             *box_pos_fig, /* the position for the box in the xfig file */
                               const char             *delete_pos_fig,
                               const char             *edit_pos_fig,
                               const char             *edit_enable_pos_fig,
                               AWT_SCANNERMODE         scannermode,
                               const char             *rescan_pos_fig, // AWT_VIEWER
                               const char             *mark_pos_fig,
                               long                    type_filter,
                               const ad_item_selector *selector);

void awt_map_arbdb_scanner(AW_CL arbdb_scanid, GBDATA *gb_pntr, int show_only_marked_flag, const char *key_path);

#else
#error db_scanner.hxx included twice
#endif // DB_SCANNER_HXX

