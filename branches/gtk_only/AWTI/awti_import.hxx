// =========================================================== //
//                                                             //
//   File      : awti_import.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWTI_IMPORT_HXX
#define AWTI_IMPORT_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef CB_H
#include <cb.h>
#endif

// more awars defined at awti_imp_local.hxx@AWAR_IMPORT
#define AWAR_IMPORT_PREFIX     "import/"
#define AWAR_IMPORT_TMP_PREFIX "tmp/import/"

#define AWAR_IMPORT_GENOM_DB AWAR_IMPORT_TMP_PREFIX "genom_db"
#define AWAR_IMPORT_AUTOCONF AWAR_IMPORT_PREFIX "autoconf"

enum AWTI_ImportType { IMP_GENOME_FLATFILE, IMP_PLAIN_SEQUENCE };

void AWTI_import_set_ali_and_type(AW_root *awr, const char *ali_name, const char *ali_type, GBDATA *gbmain);

void AWTI_open_import_window(AW_root *awr, const char *defname, bool do_exit, GBDATA *gb_main, const RootCallback& after_import_cb);

GBDATA *AWTI_peek_imported_DB();
void    AWTI_cleanup_importer();

GBDATA *AWTI_take_imported_DB_and_cleanup_importer();

#else
#error awti_import.hxx included twice
#endif // AWTI_IMPORT_HXX
