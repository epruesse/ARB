// ================================================================= //
//                                                                   //
//   File      : ptclean.cxx                                         //
//   Purpose   : prepare db for ptserver/ptpan                       //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2011   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "ptclean.h"
#include <arbdbt.h>

#define pt_assert(cond) arb_assert(cond)

// using namespace std;

class EntryTempMarker : virtual Noncopyable {
    // marks all entries of DB as 'temp', that are of no use for PTSERVER
    // (Note: 'temp' entries will not be saved)

    GBDATA         *gb_main;
    GB_transaction  ta;
    Servertype      type;
    char           *ali_name;

    enum Need {
        NONE,
        ALL,
        SOME_OF_ROOT,
        // SOME_OF_PRESETS,
        SOME_OF_SPECIES_DATA,
        SOME_OF_EXTENDED_DATA,
        SOME_OF_SPECIES,
        SOME_OF_EXTENDED,
        SOME_OF_ALI_CONTAINER, // below species or SAI
    };

    Need data_needed(GBDATA *gbd, const char *keyname, Need from) const {
        switch (from) {
            case SOME_OF_ROOT:
                if (strcmp(keyname, GB_SYSTEM_FOLDER)== 0) return ALL;
                if (strcmp(keyname, "genom_db")      == 0) return ALL;
                if (strcmp(keyname, "gene_map")      == 0) return ALL;
                if (strcmp(keyname, "species_data")  == 0) return SOME_OF_SPECIES_DATA;
                if (strcmp(keyname, "extended_data") == 0) return SOME_OF_EXTENDED_DATA;
                if (strcmp(keyname, "presets")       == 0) return ALL;
                break;

            case SOME_OF_SPECIES_DATA:
                if (strcmp(keyname, "species") == 0) return SOME_OF_SPECIES;
                break;

            case SOME_OF_EXTENDED_DATA:
                if (strcmp(keyname, "extended") == 0) {
                    const char *sainame = GBT_read_name(gbd);
                    if (strcmp(sainame, "ECOLI") == 0) return SOME_OF_EXTENDED; 
                    if (strcmp(sainame, "HELIX") == 0) return SOME_OF_EXTENDED;
                    if (strcmp(sainame, "HELIX_NR") == 0) return SOME_OF_EXTENDED;
                }
                break;

            case SOME_OF_SPECIES:
            case SOME_OF_EXTENDED:
                if (from == SOME_OF_SPECIES) {
                    if (strcmp(keyname, "abspos") == 0) return ALL;
                }
                if (strcmp(keyname, "name")      == 0) return ALL;
                if (strcmp(keyname, "acc")       == 0) return ALL;
                if (strcmp(keyname, "full_name") == 0) return ALL;
                if (strcmp(keyname, ali_name)    == 0) return SOME_OF_ALI_CONTAINER;
                break;

            case SOME_OF_ALI_CONTAINER:
                if (strcmp(keyname, "data") == 0) return ALL;
                break;

            case NONE: pt_assert(0); break;
            case ALL:  pt_assert(0); break;
        }

        return NONE;
    }

    GB_ERROR mark_child(GBDATA *gb_entry, const char *keyname, Need from) {
        GB_ERROR error = 0;
        Need     need  = data_needed(gb_entry, keyname, from);
        switch (need) {
            case NONE: error = GB_set_temporary(gb_entry); break;
            case ALL:  pt_assert(!GB_is_temporary(gb_entry)); break;
            default:   error = mark_subentries(gb_entry, need); break;
        }
        return error;
    }
    GB_ERROR mark_subentries(GBDATA *gb_father, Need from) {
        GB_ERROR error = 0;
        if (!GB_is_temporary(gb_father)) {
            for (GBDATA *gb_child = GB_child(gb_father); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
                const char *key = GB_read_key_pntr(gb_child);
                error           = mark_child(gb_child, key, from);
            }
        }
        return error;
    }

public:
    EntryTempMarker(Servertype type_, GBDATA *gb_main_)
        : gb_main(gb_main_),
          ta(gb_main),
          type(type_),
          ali_name(GBT_get_default_alignment(gb_main))
    {}
    ~EntryTempMarker() { free(ali_name); }

    GB_ERROR mark_unwanted_entries() { return mark_subentries(gb_main, SOME_OF_ROOT); }
};

inline GB_ERROR clean_ptserver_database(GBDATA *gb_main, Servertype type) {
    return EntryTempMarker(type, gb_main).mark_unwanted_entries();
}

GB_ERROR prepare_ptserver_database(GBDATA *gb_main, Servertype type) {
    GB_ERROR error = GB_request_undo_type(gb_main, GB_UNDO_NONE);
    if (!error) {
        GB_push_my_security(gb_main);
        error = clean_ptserver_database(gb_main, type);
        if (!error) {
            // @@@ calculate bp and checksums
        }
        GB_pop_my_security(gb_main);
    }
    return error;
}
// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include <arb_file.h>

void TEST_SLOW_ptclean() {
    GB_shell    shell;
    GBDATA     *gb_main = GB_open("TEST_pt_src.arb", "rw");
    const char *saveas  = "TEST_pt_cleaned.arb";

    TEST_EXPECT_NOTNULL(gb_main);
    TEST_EXPECT_NO_ERROR(prepare_ptserver_database(gb_main, PTSERVER));
    TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, saveas, "a"));
    GB_close(gb_main);

// #define TEST_AUTO_UPDATE
#if defined(TEST_AUTO_UPDATE)
    TEST_COPY_FILE("TEST_pt_cleaned.arb", "TEST_pt_cleaned_expected.arb");
#else
    TEST_EXPECT_TEXTFILES_EQUAL("TEST_pt_cleaned.arb", "TEST_pt_cleaned_expected.arb");
#endif
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(saveas));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

