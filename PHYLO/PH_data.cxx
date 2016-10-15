// ============================================================= //
//                                                               //
//   File      : PH_data.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "phylo.hxx"
#include <arbdbt.h>

char *PHDATA::unload() { // @@@ never called (PHDATA never destructed)
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

