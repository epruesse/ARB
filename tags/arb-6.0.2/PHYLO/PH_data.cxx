// ============================================================= //
//                                                               //
//   File      : PH_data.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "phwin.hxx"
#include "phylo.hxx"
#include <arbdbt.h>
#include <aw_awar.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>

char *PHDATA::unload() {
    PHENTRY *phentry;

    freenull(use);
    for (phentry=entries; phentry; phentry=phentry->next) {
        free(phentry->name);
        free(phentry->full_name);
        free(phentry);
    }
    entries = 0;
    nentries = 0;
    return 0;
}

char *PHDATA::load(char*& Use) {
    reassign(use, Use);
    last_key_number = 0;

    GBDATA *gb_main = get_gb_main();
    GB_push_transaction(gb_main);

    seq_len  = GBT_get_alignment_len(gb_main, use);
    entries  = NULL;
    nentries = 0;

    PHENTRY *tail = NULL;
    for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBDATA *gb_ali = GB_entry(gb_species, use);

        if (gb_ali) {                                     // existing alignment for this species
            GBDATA *gb_data = GB_entry(gb_ali, "data");

            if (gb_data) {
                PHENTRY *new_entry = new PHENTRY;

                new_entry->gb_species_data_ptr = gb_data;

                new_entry->key       = last_key_number++;
                new_entry->name      = strdup(GBT_read_name(gb_species));
                new_entry->full_name = GBT_read_string(gb_species, "full_name");

                new_entry->prev = tail;
                new_entry->next = NULL;

                if (!entries) {
                    tail = entries = new_entry;
                }
                else {
                    tail->next = new_entry;
                    tail       = new_entry;
                }
                nentries++;
            }
        }
    }

    GB_pop_transaction(gb_main);

    hash_elements = (PHENTRY **)calloc(nentries, sizeof(PHENTRY *));

    {
        PHENTRY *phentry = entries;
        for (unsigned int i = 0; i < nentries; i++) {
            hash_elements[i] = phentry;
            phentry = phentry->next;
        }
    }

    return 0;
}


GB_ERROR PHDATA::save(char *filename) {
    FILE *out;

    out = fopen(filename, "w");
    if (!out) {
        return "Cannot save your File";
    }
    unsigned row, col;
    fprintf(out, "%u\n", nentries);
    for (row = 0; row<nentries; row++) {
        fprintf(out, "%-13s", hash_elements[row]->name);
        for (col=0; col<=row; col++) {
            fprintf(out, "%7.4f ", matrix->get(row, col)*100.0);
        }
        fprintf(out, "\n");
    }
    fclose(out);
    return 0;
}

void PHDATA::print() {
    unsigned row, col;
    printf("    %u\n", nentries);
    for (row = 0; row<nentries; row++) {
        printf("%-10s ", hash_elements[row]->name);
        for (col=0; col<row; col++) {
            printf("%6f ", matrix->get(row, col));
        }
        printf("\n");
    }
    printf("\n");
}


GB_ERROR PHDATA::calculate_matrix(const char * /* cancel */, double /* alpha */, PH_TRANSFORMATION /* transformation */) {
    if (nentries<=1) return "There are not enough species selected";

    matrix = new AP_smatrix(nentries);

    long        i, j, column, reference_table[256];
    long        options_vector[OPT_COUNT];
    const char *real_chars, *low_chars, *rest_chars;
    char        all_chars[100], *sequence_bufferi, *sequence_bufferj;
    bool        compare[256];
    AP_FLOAT    number_of_comparisons;
    bool        bases_used = true;                      // rna oder dna sequence : nur zum testen und Entwicklung

    if (!PHDATA::ROOT) return "nothing loaded yet";

    arb_progress progress("Calculating matrix", matrix_halfsize(nentries, false));

    aw_root = PH_used_windows::windowList->phylo_main_window->get_root();
    if (bases_used) {
        real_chars="ACGTU";
        low_chars="acgtu";
        rest_chars="MRWSYKVHDBXNmrwsykvhdbxn";

        strcpy(all_chars, real_chars);
        strcat(all_chars, low_chars);
        strcat(all_chars, rest_chars);
    }
    else {
        real_chars="ABCDEFGHIKLMNPQRSTVWYZ";
        low_chars=0;
        rest_chars="X";

        strcpy(all_chars, real_chars);
        strcat(all_chars, rest_chars);
    }
    strcat(all_chars, ".-");

    // initialize variables

    options_vector[OPT_FILTER_POINT] = aw_root->awar(AWAR_PHYLO_MATRIX_POINT)->read_int();
    options_vector[OPT_FILTER_MINUS] = aw_root->awar(AWAR_PHYLO_MATRIX_MINUS)->read_int();
    options_vector[OPT_FILTER_AMBIG] = aw_root->awar(AWAR_PHYLO_MATRIX_REST)->read_int();
    options_vector[OPT_FILTER_LOWER] = aw_root->awar(AWAR_PHYLO_MATRIX_LOWER)->read_int();


    for (i=0; i<256; i++) compare[i]=false;
    for (i=0; i<long(strlen(real_chars)); i++) compare[(unsigned char)real_chars[i]]=true;
    for (i=0; i<long(strlen(all_chars)); i++) reference_table[(unsigned char)all_chars[i]]=i;

    // rna or dna sequence: set synonyms
    if (bases_used) {
        reference_table[(unsigned char)'U'] = reference_table[(unsigned char)'T']; // T=U
        reference_table[(unsigned char)'u'] = reference_table[(unsigned char)'t'];
        reference_table[(unsigned char)'N'] = reference_table[(unsigned char)'X'];
        reference_table[(unsigned char)'n'] = reference_table[(unsigned char)'x'];
    }

    distance_table = new AP_smatrix(strlen(all_chars));
    for (i=0; i<long(strlen(all_chars)); i++) {
        for (j=0; j<long(strlen(all_chars)); j++) {
            distance_table->set(i, j, (reference_table[i]==reference_table[j]) ? 0.0 : 1.0);
        }
    }

    if (bases_used) // set substitutions T = U ...
    {
        distance_table->set(reference_table[(unsigned char)'N'], reference_table[(unsigned char)'X'], 0.0);
        distance_table->set(reference_table[(unsigned char)'n'], reference_table[(unsigned char)'x'], 0.0);
        // @@@ why aren't opposite entries used?
    }
    distance_table->set(reference_table[(unsigned char)'.'], reference_table[(unsigned char)'-'], 0.0);

    char *filter = aw_root->awar(AWAR_PHYLO_FILTER_FILTER)->read_string();

    // set compare-table according to options_vector
    switch (options_vector[OPT_FILTER_POINT]) // '.' in column
    {
        case 0:  // forget pair
            // do nothing: compare stays false
            break;
        case 1:
            compare[(unsigned char)'.']=true;
            break;
    }
    switch (options_vector[OPT_FILTER_MINUS]) // '-' in column
    {
        case 0:  // forget pair
            // do nothing: compare stays false
            break;
        case 1:
            compare[(unsigned char)'-']=true;
            break;
    }
    switch (options_vector[OPT_FILTER_AMBIG]) // ambigious character in column
    {
        case 0:                 // forget pair
            // do nothing: compare stays false
            break;
        case 1:
            for (i=0; i<long(strlen(rest_chars)); i++) compare[(unsigned char)rest_chars[i]] = true;
            break;
    }
    if (bases_used) {
        switch (options_vector[OPT_FILTER_LOWER]) // lower char in column
        {
            case 0:             // forget pair
                // do nothing: compare stays false
                break;
            case 1:
                for (i=0; i<long(strlen(low_chars)); i++) compare[(unsigned char)low_chars[i]] = true;
                break;
        }
    }


    // counting routine
    sequence_bufferi = 0;
    sequence_bufferj = 0;
    GB_transaction ta(PHDATA::ROOT->get_gb_main());

    GB_ERROR error = 0;
    for (i = 0; i < long(nentries); i++) {
        delete sequence_bufferi;
        sequence_bufferi = GB_read_string(hash_elements[i]->gb_species_data_ptr);
        for (j = i + 1; j < long(nentries); j++) {
            number_of_comparisons = 0.0;
            delete sequence_bufferj;
            sequence_bufferj = GB_read_string(hash_elements[j]->gb_species_data_ptr);
            for (column = 0; column < seq_len; column++) {
                if (compare[(unsigned char)sequence_bufferi[column]] &&
                    compare[(unsigned char)sequence_bufferj[column]] &&
                    filter[column]) {
                    matrix->set(i, j,
                                matrix->get(i, j) +
                                distance_table->get(reference_table[(unsigned char)sequence_bufferi[column]],
                                                    reference_table[(unsigned char)sequence_bufferj[column]]));
                    number_of_comparisons++;
                }
            }
            if (number_of_comparisons) {
                matrix->set(i, j, (matrix->get(i, j) / number_of_comparisons));
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    delete sequence_bufferi;
    delete sequence_bufferj;

    if (error) {
        delete matrix;
        matrix = NULL;
    }

    free(filter);
    
    return error;
}
