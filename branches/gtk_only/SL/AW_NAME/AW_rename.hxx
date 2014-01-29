// =============================================================== //
//                                                                 //
//   File      : AW_rename.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_RENAME_HXX
#define AW_RENAME_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif

#define AWAR_NAMESERVER_ADDID "nt/nameserver_addid"

// --------------------------------------------------------------------------------
#ifndef AW_RENAME_SKIP_GUI

void       AWTC_create_rename_awars(AW_root *root, AW_default db1);
AW_window *AWTC_create_rename_window(AW_root *root, GBDATA *gb_main);

void       AW_create_namesadmin_awars(AW_root *aw_root, GBDATA *gb_main);
AW_window *AW_create_namesadmin_window(AW_root *root, GBDATA *gb_main);

GB_ERROR AW_select_nameserver(GBDATA *gb_main, GBDATA *gb_other_main);

#endif // AW_RENAME_SKIP_GUI
// --------------------------------------------------------------------------------

char     *AWTC_create_numbered_suffix(GB_HASH *species_name_hash, const char *shortname, GB_ERROR& warning);
GB_ERROR  AWTC_pars_names(GBDATA *gb_main, bool *isWarning = 0);
GB_ERROR  AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, const char *addid, char*& new_name);
GB_ERROR  AWTC_recreate_name(GBDATA *gb_main);

// return name of additional field used for species identification
// (-> para 'addid' in AWTC_generate_one_name)
const char *AW_get_nameserver_addid(GBDATA *gb_main);

GB_ERROR AW_test_nameserver(GBDATA *gb_main); // create a test link to the nameserver

class UniqueNameDetector : virtual Noncopyable {
    // Note: If you add new items to the DB while one instance of this class exists,
    //       you have to call add_name() for these new species!

    GB_HASH *hash;
public:
    UniqueNameDetector(GBDATA *gb_item_data, int additionalEntries = 0);
    ~UniqueNameDetector();

    bool name_known(const char *name) { return GBS_read_hash(hash, name) == 1; }
    void add_name(const char *name) { GBS_write_hash(hash, name, 1); }
};

char *AWTC_makeUniqueShortName(const char *prefix, UniqueNameDetector& existingNames);
char *AWTC_generate_random_name(UniqueNameDetector& existingNames);

struct PersistentNameServerConnection {
    // create a PersistentNameServerConnection instance while calling AWTC_generate_one_name
    // to avoid repeated re-connections to name server
    bool dummy;
    PersistentNameServerConnection();
    ~PersistentNameServerConnection();
};

int AWTC_name_quality(const char *short_name);

#else
#error AW_rename.hxx included twice
#endif // AW_RENAME_HXX
