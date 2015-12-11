//  ==================================================================== //
//                                                                       //
//    File      : AW_global_awars.cxx                                    //
//    Purpose   : Make some awars accessible from ALL arb applications   //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2003          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#define TEMP_DB_PATH "tmp/global_awars"

#include <arbdb.h>
#include <ad_cb.h>
#include <aw_global_awars.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_window.hxx>

static GBDATA *gb_main4awar = 0; // gb_main used for global awars

inline const char *get_db_path(const AW_awar *awar) {
    return GBS_global_string("%s/%s", TEMP_DB_PATH, awar->awar_name);
}

static bool in_global_awar_cb = false;

static void awar_updated_cb(AW_root*, AW_awar *awar) {
    if (!in_global_awar_cb) {
        char           *content = awar->read_as_string();
        const char     *db_path = get_db_path(awar);
        GB_transaction  ta(gb_main4awar);
        GBDATA         *gbd     = GB_search(gb_main4awar, db_path, GB_FIND);

        aw_assert(gbd);             // should exists

        LocallyModify<bool> flag(in_global_awar_cb, true);
        GB_write_string(gbd, content);
        free(content);
    }
}

static void db_updated_cb(GBDATA *gbd, AW_awar *awar) {
    if (!in_global_awar_cb) {
        GB_transaction      ta(gb_main4awar);
        LocallyModify<bool> flag(in_global_awar_cb, true);

        awar->write_as_string(GB_read_char_pntr(gbd));
    }
}

GB_ERROR AW_awar::make_global() {
#if defined(DEBUG)
    aw_assert(!is_global);      // don't make awars global twice!
    aw_assert(gb_main4awar);
    is_global = true;
#endif // DEBUG

    add_callback(makeRootCallback(awar_updated_cb, this));

    GB_transaction  ta(gb_main4awar);
    const char     *db_path = get_db_path(this);
    GBDATA         *gbd     = GB_search(gb_main4awar, db_path, GB_FIND);
    GB_ERROR        error   = 0;

    if (gbd) { // was already set by another ARB application
        // -> read db value and store in awar

        const char *content = GB_read_char_pntr(gbd);
        write_as_string(content);
    }
    else {
        // store awar value in db

        char *content   = read_as_string();
        gbd             = GB_search(gb_main4awar, db_path, GB_STRING);
        if (!gbd) error = GB_await_error();
        else  error     = GB_write_string(gbd, content);
        free(content);
    }

    if (!error) GB_add_callback(gbd, GB_CB_CHANGED, makeDatabaseCallback(db_updated_cb, this));
    return error;
}

static bool initialized = false;

bool ARB_global_awars_initialized() {
    return initialized;
}

bool ARB_in_expert_mode(AW_root *awr) {
    aw_assert(ARB_global_awars_initialized());
    int mask = awr->awar(AWAR_AWM_MASK)->read_int();
    return (mask == AWM_MASK_EXPERT);
}

static void AWAR_AWM_MASK_changed_cb(AW_root *awr) {
    int mask = awr->awar(AWAR_AWM_MASK)->read_int();
#if defined(DEBUG)
    printf("AWAR_AWM_MASK changed, calling apply_sensitivity(%i)\n", mask);
#endif
    awr->apply_sensitivity(mask);
}

static void AWAR_AW_FOCUS_FOLLOWS_MOUSE_changed_cb(AW_root *awr) {
    int focus = awr->awar(AWAR_AW_FOCUS_FOLLOWS_MOUSE)->read_int();
#if defined(DEBUG)
    printf("AWAR_AW_FOCUS_FOLLOWS_MOUSE changed, calling apply_focus_policy(%i)\n", focus);
#endif
    awr->apply_focus_policy(focus);
}

#if defined(DARWIN)
#define OPENURL "open"    
#else
#define OPENURL "xdg-open"
#endif // DARWIN

#define MAX_GLOBAL_AWARS 5

static AW_awar *declared_awar[MAX_GLOBAL_AWARS];
static int      declared_awar_count = 0;

inline void declare_awar_global(AW_awar *awar) {
    aw_assert(declared_awar_count<MAX_GLOBAL_AWARS);
    declared_awar[declared_awar_count++] = awar;
}

void ARB_declare_global_awars(AW_root *aw_root, AW_default aw_def) {
    aw_assert(!declared_awar_count);

    declare_awar_global(aw_root->awar_string(AWAR_WWW_BROWSER, OPENURL " \"$(URL)\"", aw_def));
    declare_awar_global(aw_root->awar_int(AWAR_AWM_MASK, AWM_MASK_UNKNOWN, aw_def)->add_callback(AWAR_AWM_MASK_changed_cb));
    declare_awar_global(aw_root->awar_string(AWAR_ARB_TREE_RENAMED, "", aw_def));

    AW_awar *awar_focus          = aw_root->awar_int(AWAR_AW_FOCUS_FOLLOWS_MOUSE, 0, aw_def);
    aw_root->focus_follows_mouse = awar_focus->read_int();
    awar_focus->add_callback(AWAR_AW_FOCUS_FOLLOWS_MOUSE_changed_cb);
    declare_awar_global(awar_focus);
}

GB_ERROR ARB_bind_global_awars(GBDATA *gb_main) {
    aw_assert(!initialized);                        // don't call twice!
    aw_assert(!gb_main4awar);
    aw_assert(declared_awar_count);                 // b4 call ARB_declare_global_awars!

    initialized  = true;
    gb_main4awar = gb_main;

    GB_ERROR error = NULL;
    for (int a = 0; a<declared_awar_count && !error; ++a) {
        error = declared_awar[a]->make_global();
    }

    return error;
}

GBDATA *get_globalawars_gbmain() {
    //! hack to access open ARB database (used for helpfile editing)
    return gb_main4awar;
}
