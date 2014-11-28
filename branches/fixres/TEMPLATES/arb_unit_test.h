// =============================================================== //
//                                                                 //
//   File      : arb_unit_test.h                                   //
//   Purpose   : useful high level code used in multiple units     //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_UNIT_TEST_H
#define ARB_UNIT_TEST_H

#ifdef UNIT_TESTS

namespace arb_unit_test {

    struct test_alignment_data {
        int         mark;
        const char *name;
        const char *data;
    };

    inline void test_insert_SAI_data(GBDATA *gb_main, ARB_ERROR& error, const char *test_aliname, test_alignment_data *ali_data, int sai_count) {
        for (int sp = 0; sp<sai_count && !error; ++sp) {
            test_alignment_data&  sai    = ali_data[sp];
            GBDATA               *gb_species = GBT_find_or_create_SAI(gb_main, sai.name);
            if (!gb_species) error           = GB_await_error();
            else {
                GB_write_flag(gb_species, sai.mark);

                GBDATA *gb_data     = GBT_add_data(gb_species, test_aliname, "data", GB_STRING);
                if (!gb_data) error = GB_await_error();
                else    error       = GB_write_string(gb_data, sai.data);
            }
        }
    }

    inline GBDATA *test_create_DB(ARB_ERROR& error, const char *test_aliname, test_alignment_data *ali_data, int species_count, bool use_compression) {
        GBDATA *gb_main = GB_open("nodb.arb", "crw");
        error           = GB_push_transaction(gb_main);

        GB_allow_compression(gb_main, use_compression);

        if (!error) {
            GBDATA *gb_species_data     = GBT_get_species_data(gb_main);
            if (!gb_species_data) error = GB_await_error();
            else {
                long    ali_len          = strlen(ali_data[0].data);
                GBDATA *gb_alignment     = GBT_create_alignment(gb_main, test_aliname, ali_len, 1, 0, "rna");
                if (!gb_alignment) error = GB_await_error();
                else {
                    for (int sp = 0; sp<species_count && !error; ++sp) {
                        test_alignment_data&  species    = ali_data[sp];
                        GBDATA               *gb_species = GBT_find_or_create_species(gb_main, species.name);
                        if (!gb_species) error           = GB_await_error();
                        else {
                            GB_write_flag(gb_species, species.mark);

                            GBDATA *gb_data     = GBT_add_data(gb_species, test_aliname, "data", GB_STRING);
                            if (!gb_data) error = GB_await_error();
                            else    error       = GB_write_string(gb_data, species.data);
                        }
                    }
                }
                if (!error) error = GBT_set_default_alignment(gb_main, test_aliname);
            }
        }

        error = GB_pop_transaction(gb_main);

        return gb_main;
    }

#define TEST_SPECIES_COUNT(test_ali_data) ARRAY_ELEMS(test_ali_data)

#define TEST_CREATE_DB(error,test_aliname,test_ali_data,compress)               \
    arb_unit_test::test_create_DB(error, test_aliname, test_ali_data,           \
                                  TEST_SPECIES_COUNT(test_ali_data), compress)

#define TEST_DB_INSERT_SAI(gb_main, error,test_aliname,test_ali_data)                   \
    arb_unit_test::test_insert_SAI_data(gb_main, error, test_aliname, test_ali_data,    \
                                        TEST_SPECIES_COUNT(test_ali_data))

};

#endif // UNIT_TESTS

#else
#error arb_unit_test.h included twice
#endif // ARB_UNIT_TEST_H
