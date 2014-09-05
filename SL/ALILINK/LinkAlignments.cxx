// ============================================================== //
//                                                                //
//   File      : LinkAlignments.cxx                               //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "LinkAlignments.h"
#include <arbdbt.h>

#define ali_assert(cond) arb_assert(cond)

static GBDATA *get_ali_link(GBDATA *gb_main, const char *ali, bool createIfMissing, GB_ERROR& error) {
    ali_assert(!error);

    GBDATA *gb_linked  = NULL;
    GBDATA *gb_ali     = GBT_get_alignment(gb_main, ali);
    if (!gb_ali) error = GB_await_error();
    else {
        gb_linked = GB_entry(gb_ali, "linked");
        if (!gb_linked && createIfMissing) {
            gb_linked             = GB_create(gb_ali, "linked", GB_STRING);
            if (!gb_linked) error = GB_await_error();
        }
    }

    ali_assert(!(error && gb_linked));
    return gb_linked;
}

const char *ALI_is_linked(GBDATA *gb_main, const char *ali) {
    //! @return name of alignment linked to alignment 'ali' or NULL if none
    const char *result    = NULL;
    GB_ERROR    error     = NULL;
    GBDATA     *gb_linked = get_ali_link(gb_main, ali, false, error);
    if (gb_linked) result = GB_read_char_pntr(gb_linked);
    ali_assert(!GB_have_error());
    return result;
}

bool ALI_are_linked(GBDATA *gb_main, const char *ali1, const char *ali2) {
    const char *linked1 = ALI_is_linked(gb_main, ali1);
    if (linked1 && strcmp(linked1, ali2) == 0) {
        const char *linked2 = ALI_is_linked(gb_main, ali2);
        return linked2 && strcmp(linked2, ali1) == 0;
    }
    return false;
}

GB_ERROR ALI_link(GBDATA *gb_main, const char *ali1, const char *ali2) {
    /*! links the alignments 'ali1' and 'ali2'.
     * One of them has to be a protein, the other a dna alignment.
     * @return error if something goes wrong (e.g. wrong ali type, ...)
     * Silently does nothing, if already linked.
     * Silently fix , if link exists in one direction, but not vv.
     */

    GB_ERROR error = NULL;

    if (strcmp(ali1, ali2) == 0) error = GBS_global_string("Cannot link '%s' to itself", ali1);
    else {
        GBDATA *gb_linked1 = get_ali_link(gb_main, ali1, false, error);
        GBDATA *gb_linked2 = error ? NULL : get_ali_link(gb_main, ali2, false, error);

        if (!error && (gb_linked1 || gb_linked2)) {
            const char *linked1 = gb_linked1 ? GB_read_char_pntr(gb_linked1) : NULL;
            const char *linked2 = gb_linked2 ? GB_read_char_pntr(gb_linked2) : NULL;

            bool wrong_linked1 = linked1 && strcmp(linked1, ali2) != 0;
            bool wrong_linked2 = linked2 && strcmp(linked2, ali1) != 0;

            if      (wrong_linked1) error = GBS_global_string("Alignment '%s' is already linked with '%s' (cannot link with '%s')", ali1, linked1, ali2);
            else if (wrong_linked2) error = ALI_link(gb_main, ali2, ali1);

            ali_assert(correlated(error, wrong_linked1||wrong_linked2));
        }

        if (!error && (!gb_linked1 || !gb_linked2)) { // need link?
            // check ali types
            {
                GB_alignment_type type1 = GBT_get_alignment_type(gb_main, ali1);
                GB_alignment_type type2 = GBT_get_alignment_type(gb_main, ali2);

                bool compat_types        = (type1 == GB_AT_DNA && type2 == GB_AT_AA) || (type2 == GB_AT_DNA && type1 == GB_AT_AA);
                if (!compat_types) error = GBS_global_string("Cannot link '%s' and '%s' (incompatible alignment types)", ali1, ali2);
            }
            if (!error && !gb_linked1) gb_linked1 = get_ali_link(gb_main, ali1, true, error);
            if (!error && !gb_linked2) gb_linked2 = get_ali_link(gb_main, ali2, true, error);

            if (!error) error = GB_write_string(gb_linked1, ali2);
            if (!error) error = GB_write_string(gb_linked2, ali1);
        }
    }
    return error;
}

GB_ERROR ALI_unlink(GBDATA *gb_main, const char *ali1, const char *ali2) {
    //! Unlinks alignments 'ali1' from 'ali2' (and vv)

    GB_ERROR  error      = NULL;
    GBDATA   *gb_linked1 = get_ali_link(gb_main, ali1, false, error);
    GBDATA   *gb_linked2 = error ? NULL : get_ali_link(gb_main, ali2, false, error);

    if (!error) {
        const char *linked1 = gb_linked1 ? GB_read_char_pntr(gb_linked1) : NULL;
        const char *linked2 = gb_linked2 ? GB_read_char_pntr(gb_linked2) : NULL;

        bool were_linked = false;
        if (linked1 && linked2) {
            if (strcmp(ali1, linked2) == 0 && strcmp(ali2, linked1) == 0) {
                were_linked       = true;
                error             = GB_delete(gb_linked1);
                if (!error) error = GB_delete(gb_linked2);
            }
        }

        if (!were_linked) {
            ali_assert(!error);
            error = GBS_global_string("Cannot unlink '%s' from '%s' (alignments are not linked)", ali1, ali2);
        }
    }
    return error;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

struct alipair {
    const char *dna;
    const char *pro;
};
struct seqdata {
    const char *dna;
    const char *pro;

    seqdata(const char *d, const char *p) : dna(d), pro(p) {}
};

static arb_test::match_expectation species_has_data(GBDATA *gb_species, const alipair& ali, const seqdata& expected) {
    using namespace    arb_test;
    expectation_group  wanted;
    GB_transaction     ta(gb_species);
    GBDATA            *gb_ali_dna = GBT_find_sequence(gb_species, ali.dna);
    GBDATA            *gb_ali_pro = GBT_find_sequence(gb_species, ali.pro);

    if (gb_ali_dna) {
        const char *curr_dna = GB_read_char_pntr(gb_ali_dna);
        wanted.add(that(curr_dna).is_equal_to(expected.dna));
    }
    else {
        wanted.add(that(gb_ali_dna).does_differ_from(NULL));
    }
    if (gb_ali_pro) {
        const char *curr_pro = GB_read_char_pntr(gb_ali_pro);
        wanted.add(that(curr_pro).is_equal_to(expected.pro));
    }
    else {
        wanted.add(that(gb_ali_pro).does_differ_from(NULL));
    }

    return all().ofgroup(wanted);
}

static GB_ERROR write_ali_sequence(GBDATA *gb_species, const char *ali_name, const char *sequence) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_data = GBT_find_sequence(gb_species, ali_name);
    if (!gb_data) {
        error = GB_have_error() ? GB_await_error() : "species has no entry in alignment";
    }
    else {
        error = GB_write_string(gb_data, sequence);
    }
    return error;
}


#define TEST_EXPECT_DATA(gb_species,apair,sdata)                          TEST_EXPECTATION(species_has_data(gb_species,apair,sdata))
#define TEST_EXPECT_DATA__BROKEN(gb_species,apair,sdata_wanted,sdata_got) TEST_EXPECTATION__BROKEN(species_has_data(gb_species,apair,sdata_wanted), species_has_data(gb_species,apair,sdata_got))

void TEST_linkAli() {
    GB_shell    shell;
    GBDATA     *gb_main = GB_open("TEST_prot_mini.arb", "r");
    const char *ali_rna = "ali_rna";

    GBDATA *gb_TaxOcell;

    alipair ali1 = { "ali_dna1", "ali_pro1"};
    alipair ali2 = { "ali_dna2", "ali_pro2"};

    seqdata org   ("------GCTAACAAAGGTCTGGCCGCT---------AAGCGTGACTTC---TCCTCGATCGACAACGCTCCGGAGGAAAAAGAGCGCGGTATCACCATCAACACCGCTCACGTT.........", "--ANKGLAA---KRDF-SSIDNAPEEKERGITINTAHV...");
    seqdata state1("------GCTAACAAAGGTCTGGCCGCT------AAG---CGTGACTTC---TCCTCGATCGACAACGCTCCGGAGGAAAAAGAGCGCGGTATCACCATCAACACCGCTCACGTT.........", "--ANKGLAA--K-RDF-SSIDNAPEEKERGITINTAHV...");
    seqdata state2("------GCTAACAAAGGTCTGGCCGCT---------AAGCGTGACTTCTCC---TCGATCGACAACGCTCCGGAGGAAAAAGAGCGCGGTATCACCATCAACACCGCTCACGTT.........", "--ANKGLAA---KRDFS-SIDNAPEEKERGITINTAHV...");

    {
        GB_transaction ta(gb_main);

        gb_TaxOcell = GBT_find_species(gb_main, "TaxOcell");
        TEST_REJECT_NULL(gb_TaxOcell);

        TEST_EXPECT_NULL(ALI_is_linked(gb_main, "bla")); // non-existing alignment is not linked

        // existing ali1 are NOT linked
        TEST_EXPECT_NULL(ALI_is_linked(gb_main, ali1.dna));
        TEST_EXPECT_NULL(ALI_is_linked(gb_main, ali1.pro));

        // existing ali2 are linked
        TEST_EXPECT_EQUAL(ALI_is_linked(gb_main, ali2.dna), ali2.pro);
        TEST_EXPECT_EQUAL(ALI_is_linked(gb_main, ali2.pro), ali2.dna);
        TEST_EXPECT(ALI_are_linked(gb_main, ali2.dna, ali2.pro));

        // test original seq content
        TEST_EXPECT_DATA(gb_TaxOcell, ali1, org);
        TEST_EXPECT_DATA(gb_TaxOcell, ali2, org);
    }

    // test changing dna seq
    {
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_NO_ERROR(write_ali_sequence(gb_TaxOcell, ali1.dna, state1.dna));
            TEST_EXPECT_NO_ERROR(write_ali_sequence(gb_TaxOcell, ali2.dna, state1.dna));
        }
        seqdata written1(state1.dna, org.pro);
        TEST_EXPECT_DATA(gb_TaxOcell, ali1, written1);                 // nothing retranslated
        TEST_EXPECT_DATA__BROKEN(gb_TaxOcell, ali2, state1, written1); // protein should get retranslated
    }

    // test changing protein seq
    {
        {
            GB_transaction ta(gb_main);

            TEST_EXPECT_NO_ERROR(write_ali_sequence(gb_TaxOcell, ali1.pro, state2.pro));
            TEST_EXPECT_NO_ERROR(write_ali_sequence(gb_TaxOcell, ali2.pro, state2.pro));
        }
        seqdata written2(state1.dna, state2.pro);
        TEST_EXPECT_DATA(gb_TaxOcell, ali1, written2);                 // nothing realigned
        TEST_EXPECT_DATA__BROKEN(gb_TaxOcell, ali2, state2, written2); // dna should get realigned
    }

    {
        GB_transaction ta(gb_main);
        // test linking already linked alignment fails
        TEST_EXPECT_NO_ERROR(ALI_link(gb_main, ali2.dna, ali2.pro));

        // test re-linking already linked alignment fails
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali2.dna, ali1.pro), "Alignment 'ali_dna2' is already linked with 'ali_pro2' (cannot link with 'ali_pro1')");
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali1.dna, ali2.pro), "Alignment 'ali_pro2' is already linked with 'ali_dna2' (cannot link with 'ali_dna1')");

        // test linking non-existant alignments fails
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali1.dna, "nosuch"), "ARB ERROR: alignment 'nosuch' not found");
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, "nosuch", ali2.pro), "ARB ERROR: alignment 'nosuch' not found");
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, "nosuch1", "nosuch2"), "ARB ERROR: alignment 'nosuch1' not found");

        // test linking alignment to itself fails
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali1.pro, ali1.pro), "Cannot link 'ali_pro1' to itself");
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali2.dna, ali2.dna), "Cannot link 'ali_dna2' to itself");

        TEST_EXPECT_NO_ERROR(ALI_link(gb_main, ali1.dna, ali1.pro)); // link ali1
        // now ali1 should be linked
        TEST_EXPECT_EQUAL(ALI_is_linked(gb_main, ali1.dna), ali1.pro);
        TEST_EXPECT_EQUAL(ALI_is_linked(gb_main, ali1.pro), ali1.dna);

    }

    // @@@ test changing seqs has an effect (both directions, both alis)

    {
        GB_transaction ta(gb_main);

        // test unlinking not-linked alignments
        TEST_EXPECT_ERROR_CONTAINS(ALI_unlink(gb_main, ali1.dna, ali2.pro), "Cannot unlink 'ali_dna1' from 'ali_pro2' (alignments are not linked)");

        // test unlinking linked alignments
        TEST_EXPECT_NO_ERROR(ALI_unlink(gb_main, ali1.dna, ali1.pro));
        TEST_EXPECT_NULL(ALI_is_linked(gb_main, ali1.dna));
        TEST_EXPECT_NULL(ALI_is_linked(gb_main, ali1.pro));
        TEST_EXPECT(!ALI_are_linked(gb_main, ali1.pro, ali1.dna));

        // @@@ test changing seqs has no effect

        // test linking wrong ali types
        TEST_EXPECT_NO_ERROR(ALI_unlink(gb_main, ali2.dna, ali2.pro)); // first unlink ali2
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali1.dna, ali2.dna), "Cannot link 'ali_dna1' and 'ali_dna2' (incompatible alignment types)");
        TEST_EXPECT_ERROR_CONTAINS(ALI_link(gb_main, ali_rna,  ali1.pro), "Cannot link 'ali_rna' and 'ali_pro1' (incompatible alignment types)");

    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



