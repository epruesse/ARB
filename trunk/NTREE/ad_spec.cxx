// =============================================================== //
//                                                                 //
//   File      : ad_spec.cxx                                       //
//   Purpose   : Create and modify species and SAI.                //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "map_viewer.h"

#include <dbui.h>

#include <awt_www.hxx>
#include <awt_canvas.hxx>

#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <arb_progress.h>
#include <arb_defs.h>

#include <cctype>

static const char * const SAI_COUNTED_CHARS = "COUNTED_CHARS";

void NT_count_different_chars(AW_window*, GBDATA *gb_main) {
    // @@@ extract algorithm and call extracted from testcode
    ARB_ERROR error = GB_begin_transaction(gb_main); // open transaction

    char *alignment_name = GBT_get_default_alignment(gb_main);  // info about sequences
    int   alignment_len  = GBT_get_alignment_len(gb_main, alignment_name);
    int   is_amino       = GBT_is_alignment_protein(gb_main, alignment_name);

    if (!error) {
        const int MAXLETTER   = 256;
        const int FIRSTLETTER = 0; // cppcheck-suppress variableScope

        typedef bool letterOccurs[MAXLETTER];

        letterOccurs *occurs = ARB_alloc<letterOccurs>(alignment_len);
        for (int i = 0; i<MAXLETTER; ++i) {
            for (int p = 0; p<alignment_len; ++p) {
                occurs[p][i] = false;
            }
        }

        // loop over all marked species
        {
            int          all_marked = GBT_count_marked_species(gb_main);
            arb_progress progress("Counting different characters", all_marked);

            for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
                 gb_species && !error;
                 gb_species = GBT_next_marked_species(gb_species))
            {
                GBDATA *gb_ali = GB_entry(gb_species, alignment_name); // search the sequence database entry ( ali_xxx/data )
                if (gb_ali) {
                    GBDATA *gb_data = GB_entry(gb_ali, "data");
                    if (gb_data) {
                        const char * const seq = GB_read_char_pntr(gb_data);
                        if (seq) {
                            for (int i=0; i< alignment_len; ++i) {
                                unsigned char c = seq[i];
                                if (!c) break;

                                occurs[i][c-FIRSTLETTER] = true;
                            }
                        }
                    }
                }
                progress.inc_and_check_user_abort(error);
            }
        }

        if (!error) {

            char filter[256];
            if (is_amino) for (int c = 0; c<256; ++c) filter[c] = isupper(c) && (strchr("BJOUZ", c) == 0);
            else          for (int c = 0; c<256; ++c) filter[c] = (strchr("ACGTU", c) != 0);

            char result[alignment_len+1];
            for (int i=0; i<alignment_len; i++) {
                int sum = 0;
                for (int c = 'A'; c < 'Z'; ++c) {
                    if (filter[c]) {
                        sum += (occurs[i][c] || occurs[i][tolower(c)]);
                    }
                }
                result[i] = sum<10 ? '0'+sum : 'A'-10+sum;
            }
            result[alignment_len] = 0;

            {
                GBDATA *gb_sai     = GBT_find_or_create_SAI(gb_main, SAI_COUNTED_CHARS);
                if (!gb_sai) error = GB_await_error();
                else {
                    GBDATA *gb_data     = GBT_add_data(gb_sai, alignment_name, "data", GB_STRING);
                    if (!gb_data) error = GB_await_error();
                    else    error       = GB_write_string(gb_data, result);
                }
            }
        }

        free(occurs);
    }

    free(alignment_name);

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

void NT_create_sai_from_pfold(AW_window *aww, AWT_canvas *ntw) {
    /*! \brief Creates an SAI from protein secondary structure of a selected species.
     *
     *  \param[in] aww AW_window
     *  \param[in] ntw AWT_canvas
     *
     *  The function takes the currently selected species and searches for the field
     *  "sec_struct". A new SAI is created using the data in this field. A simple input
     *  window allows the user to change the default name ([species name]_pfold) for
     *  the new SAI.
     *
     *  \note The import filter "dssp_all.ift" allows for importing the amino acid sequence
     *        as well as the protein secondary structure from a dssp file and the structure
     *        is stored in the field "sec_struct". That way, secondary structure can be
     *        aligned along with the sequence manually and can later be extracted to create
     *        an SAI.
     *
     *  \attention The import filter "dssp_2nd_struct.ift" extracts only the protein
     *             secondary structure which is stored as alignment data. SAIs can simply
     *             be created from these species via move_species_to_extended().
     */

    GB_ERROR  error      = 0;
    GB_begin_transaction(GLOBAL.gb_main);
    char     *sai_name   = 0;
    char     *sec_struct = 0;
    bool      canceled   = false;

    // get the selected species
    char *species_name = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = 0;
    if (!strcmp(species_name, "") || !(gb_species = GBT_find_species(GLOBAL.gb_main, species_name))) {
        error = "Please select a species first.";
    }
    else {
        // search for the field "sec_struct"
        GBDATA *gb_species_sec_struct = GB_entry(gb_species, "sec_struct");
        if (!gb_species_sec_struct) {
            error = "Field \"sec_struct\" not found or empty. Please select another species.";
        }
        else if (!(sec_struct = GB_read_string(gb_species_sec_struct))) {
            error = "Couldn't read field \"sec_struct\". Is it empty?";
        }
        else {
            // generate default name and name input field for the new SAI
            {
                char *sai_default_name = GBS_global_string_copy("%s%s", species_name, strstr(species_name, "_pfold") ? "" : "_pfold");
                sai_name         = aw_input("Name of SAI to create:", sai_default_name);
                free(sai_default_name);
            }

            if (!sai_name) {
                canceled = true;
            }
            else if (strspn(sai_name, " ") == strlen(sai_name)) {
                error = "Name of SAI is empty. Please enter a valid name.";
            }
            else {
                GBDATA *gb_sai_data = GBT_get_SAI_data(GLOBAL.gb_main);
                GBDATA *gb_sai      = GBT_find_SAI_rel_SAI_data(gb_sai_data, sai_name);
                char   *ali_name    = GBT_get_default_alignment(GLOBAL.gb_main);

                if (gb_sai) {
                    error = "SAI with the same name already exists. Please enter another name.";
                }
                else {
                    // create SAI container and copy fields from the species to the SAI
                    gb_sai                   = GB_create_container(gb_sai_data, "extended");
                    GBDATA *gb_species_field = GB_child(gb_species);

                    while (gb_species_field && !error) {
                        char   *key          = GB_read_key(gb_species_field);
                        GBDATA *gb_sai_field = GB_search(gb_sai, GB_read_key(gb_species_field), GB_read_type(gb_species_field));

                        if (strcmp(key, "name") == 0) { // write the new name
                            error = GB_write_string(gb_sai_field, sai_name);
                        }
                        else if (strcmp(key, "sec_struct") == 0) { // write contents from the field "sec_struct" to the alignment data
                            GBDATA *gb_sai_ali     = GB_search(gb_sai, ali_name, GB_CREATE_CONTAINER);
                            if (!gb_sai_ali) error = GB_await_error();
                            else    error          = GBT_write_string(gb_sai_ali, "data", sec_struct);
                        }
                        else if (strcmp(key, "acc") != 0 && strcmp(key, ali_name) != 0) { // don't copy "acc" and the old alignment data
                            error = GB_copy(gb_sai_field, gb_species_field);
                        }
                        gb_species_field = GB_nextChild(gb_species_field);
                        free(key);
                    }

                    // generate accession number and delete field "sec_struct" from the SAI
                    if (!error) {
                        // TODO: is it necessary that a new acc is generated here?
                        GBDATA *gb_sai_acc = GB_search(gb_sai, "acc", GB_FIND);
                        if (gb_sai_acc) {
                            GB_delete(gb_sai_acc);
                            GBT_gen_accession_number(gb_sai, ali_name);
                        }
                        GBDATA *gb_sai_sec_struct = GB_search(gb_sai, "sec_struct", GB_FIND);
                        if (gb_sai_sec_struct) GB_delete(gb_sai_sec_struct);
                        aww->get_root()->awar(AWAR_SAI_NAME)->write_string(sai_name);
                    }
                }
            }
        }
    }

    if (canceled) error = "Aborted by user";

    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);

    if (!error) {
        AW_window *sai_info = NT_create_extendeds_window(aww->get_root());
        // TODO: why doesn't info box show anything on first startup? proper refresh needed?
        sai_info->activate();
        ntw->refresh(); // refresh doesn't work, I guess...
    }

    free(species_name);
    free(sai_name);
    free(sec_struct);
}

void launch_MapViewer_cb(GBDATA *gbd, AD_MAP_VIEWER_TYPE type) {
    GB_ERROR error = GB_push_transaction(GLOBAL.gb_main);

    if (!error) {
        const char *species_name    = "";
        GBDATA     *gb_species_data = GBT_get_species_data(GLOBAL.gb_main);

        if (gbd && GB_get_father(gbd) == gb_species_data) {
            species_name = GBT_read_name(gbd);
        }

        AW_root::SINGLETON->awar(AWAR_SPECIES_NAME)->write_string(species_name);
    }

    if (!error && gbd && type == ADMVT_WWW) {
        GBDATA *gb_name       = GB_entry(gbd, "name");
        if (!gb_name) gb_name = GB_entry(gbd, "group_name"); // bad hack, should work

        const char *name = gb_name ? GB_read_char_pntr(gb_name) : "noname";
        error = awt_openURL_by_gbd(GLOBAL.aw_root, GLOBAL.gb_main, gbd, name);
    }

    error = GB_end_transaction(GLOBAL.gb_main, error);
    if (error) aw_message(error);
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <arb_unit_test.h>

static uint32_t counted_chars_checksum(GBDATA *gb_main)  {
    GB_transaction ta(gb_main);

    GBDATA *gb_sai;
    GBDATA *gb_ali;
    GBDATA *gb_counted_chars;

    char *ali_name = GBT_get_default_alignment(gb_main);

    TEST_EXPECT_RESULT__NOERROREXPORTED(gb_sai = GBT_expect_SAI(gb_main, SAI_COUNTED_CHARS));
    TEST_EXPECT_RESULT__NOERROREXPORTED(gb_ali = GB_entry(gb_sai, ali_name));
    TEST_EXPECT_RESULT__NOERROREXPORTED(gb_counted_chars = GB_entry(gb_ali, "data"));

    const char *data = GB_read_char_pntr(gb_counted_chars);

    free(ali_name);

    return GBS_checksum(data, 0, NULL);
}

void TEST_count_chars() {
    // calculate SAI for test DBs

    arb_suppress_progress silence;
    GB_shell              shell;

    for (int prot = 0; prot<2; ++prot) {
        GBDATA *gb_main;
        TEST_EXPECT_RESULT__NOERROREXPORTED(gb_main = GB_open(prot ? "TEST_prot.arb" : "TEST_nuc.arb", "rw"));

        GBT_mark_all(gb_main, 1);
        NT_count_different_chars(NULL, gb_main);

        uint32_t expected = prot ? 0x9cad14cc : 0xefb05e4e;
        TEST_EXPECT_EQUAL(counted_chars_checksum(gb_main), expected);

        GB_close(gb_main);
    }
}
void TEST_SLOW_count_chars() {
    // calculate a real big test alignment
    //
    // the difference to TEST_count_chars() is just in size of alignment.
    // NT_count_different_chars() crashes for big alignments when running in gdb
    arb_suppress_progress silence;
    GB_shell              shell;
    {
        arb_unit_test::test_alignment_data data_source[] = {
            { 1, "s1", "ACGT" },
            { 1, "s2", "ACGTN" },
            { 1, "s3", "NANNAN" },
            { 1, "s4", "GATTACA" },
        };

        const int alilen = 50000;
        const int count  = ARRAY_ELEMS(data_source);

        char *longSeq[count];
        for (int c = 0; c<count; ++c) {
            char *dest = longSeq[c] = ARB_alloc<char>(alilen+1);

            const char *source = data_source[c].data;
            int         len    = strlen(source);

            for (int p = 0; p<alilen; ++p) {
                dest[p] = source[p%len];
            }
            dest[alilen] = 0;
            
            data_source[c].data = dest;
        }

        ARB_ERROR  error;
        GBDATA    *gb_main = TEST_CREATE_DB(error, "ali_test", data_source, false);

        TEST_EXPECT_NO_ERROR(error.deliver());

        NT_count_different_chars(NULL, gb_main);

        // TEST_EXPECT_EQUAL(counted_chars_checksum(gb_main), 0x1d34a14f);
        // TEST_EXPECT_EQUAL(counted_chars_checksum(gb_main), 0x609d788b);
        TEST_EXPECT_EQUAL(counted_chars_checksum(gb_main), 0xccdfa527);

        for (int c = 0; c<count; ++c) {
            free(longSeq[c]);
        }

        GB_close(gb_main);
    }
}

#endif // UNIT_TESTS

