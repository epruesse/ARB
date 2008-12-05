// =============================================================== //
//                                                                 //
//   File      : arb_test.cxx                                      //
//   Purpose   : small test programm (not part of ARB)             //
//               If you need to test sth, you may completely       // 
//               overwrite and checkin this.                       //
//                                                                 //
//   Time-stamp: <Fri Dec/05/2008 19:47 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2008       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <arbdb.h>

static GB_ERROR create_n_write(GBDATA *gb_father, const char *field, const char *content) {
    GB_ERROR  error = 0;
    GBDATA   *gbd   = GB_create(gb_father, field, GB_STRING);
    
    if (!gbd) {
        error = GB_expect_error();
    }
    else {
        error = GB_write_string(gbd, content);
    }

    return error;
}

static GB_ERROR find_n_print(GBDATA *gb_father, const char *field) {
    GB_ERROR  error = 0;
    GBDATA   *gbd   = GB_entry(gb_father, field);

    if (!gbd) {
        printf("No such field '%s'\n", field);
    }
    else {
        char *content = GB_read_string(gbd);
        if (!content) {
            error = GB_expect_error();
        }
        else {
            printf("field '%s' has content '%s'\n", field, content);
            free(content);
        }
    }

    return error;
}

static GB_ERROR dump_fields(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    GB_ERROR       error;
    
    error             = find_n_print(gb_main, "case");
    if (!error) error = find_n_print(gb_main, "CASE");
    if (!error) error = find_n_print(gb_main, "Case");

    return error;
}

static GB_ERROR dump_key_data(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    GB_ERROR       error = 0;

    GBDATA *gb_system = GB_entry(gb_main, "__SYSTEM__");
    if (!gb_system) {
        error = "can't find '__SYSTEM__'";
    }
    else {
        GBDATA *gb_key_data = GB_entry(gb_system, "@key_data");
        if (!gb_key_data) {
            error = "can't find '@key_data'";
        }
        else {
            int count = 0;

            for (GBDATA *gb_key = GB_entry(gb_key_data, "@key"); gb_key; gb_key = GB_nextEntry(gb_key)) {
                GBDATA *gb_name = GB_entry(gb_key, "@name");
                if (!gb_name) {
                    error = "@key w/o @name entry";
                }
                else {
                    char *name = GB_read_string(gb_name);

                    if (!name) {
                        error = GB_expect_error();
                    }
                    else {
                        GBDATA *gb_cm = GB_entry(gb_key, "compression_mask");
                        
                        if (!gb_cm)  {
                            error = "@key w/o compression_mask";
                        }
                        else {
                            int cm = GB_read_int(gb_cm);

                            printf("key %i name='%s' compression_mask=%i\n", count, name, cm);
                        }

                        free(name);
                    }
                }
                count++;
            }
            printf("%i keys seen\n", count);
        }
    }

    return error;
}

static GB_ERROR create_db(const char *filename) {
    GB_ERROR  error   = 0;
    GBDATA   *gb_main = GB_open(filename, "wc");

    if (!gb_main) error = GB_expect_error();

    // write two fields with same case
    if (!error) {
        GB_transaction ta(gb_main);

        error             = create_n_write(gb_main, "case", "lower case");
        if (!error) error = create_n_write(gb_main, "CASE", "upper case");

        error = ta.close(error);
    }

    // search for fields
    if (!error) error = dump_fields(gb_main);
    if (!error) error = dump_key_data(gb_main);
    if (!error) error = GB_save(gb_main, filename, "b");
    
    GB_close(gb_main);

    return error;
}

static GB_ERROR test_db(const char *filename) {
    GB_ERROR  error   = 0;
    GBDATA   *gb_main = GB_open(filename, "r");

    if (!gb_main) error = GB_expect_error();
    if (!error) error   = dump_fields(gb_main);
    if (!error) error   = dump_key_data(gb_main);
    
    GB_close(gb_main);

    return error;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "arb_test -- database test program\n"
                "Syntax: arb_test database\n");
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    GB_ERROR    error    = 0;

    printf("Creating DB '%s'..\n", filename);
    error = create_db(filename);
    
    if (!error) {
        printf("Testing DB '%s'..\n", filename);
        error = test_db(filename);
    }

    if (error) fprintf(stderr, "Error in arb_test: %s\n", error);
    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}


