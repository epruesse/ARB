// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>

#include "a3_arbdb.hxx"

// -----------------------------------------------------------------------------
    A3Arbdb::~A3Arbdb ( void )
// -----------------------------------------------------------------------------
{
    if (gb_main)    GB_close(gb_main);
    if (alignment)  free ((char *) alignment);
}

// -----------------------------------------------------------------------------
    int A3Arbdb::open ( char *name,
                        char *use_alignment )
// -----------------------------------------------------------------------------
{
   gb_main = GB_open(name,"rt");

   if (!gb_main)
   {
      GB_print_error();
      return 1;
   }

   GB_begin_transaction(gb_main);

   if (use_alignment)   alignment = strdup(use_alignment);
   else                 alignment = GBT_get_default_alignment(gb_main);

   GB_commit_transaction(gb_main);

   return 0;
}

// -----------------------------------------------------------------------------
void A3Arbdb::close ( void )
// -----------------------------------------------------------------------------
{
    GB_close(gb_main);
    freeset(alignment, 0);
}

// -----------------------------------------------------------------------------
    char *A3Arbdb::get_sequence_string ( const char *name,
                                         int     and_mark )
// -----------------------------------------------------------------------------
{
   char   *sequence = NULL;
   GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
   GBDATA *gb_seq          = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

   if (gb_seq)
   {
       if (and_mark) GB_write_flag(GB_get_father(gb_seq),1);

       gb_seq = GB_brother(gb_seq,alignment);

       if (gb_seq)
       {
           gb_seq = GB_entry(gb_seq,"data");

           if (gb_seq) sequence = GB_read_string(gb_seq);
       }
   }

   if (sequence == 0) return 0;

   return sequence;
}

int A3Arbdb::put_sequence_string(char *name, char *sequence) {
    GB_change_my_security(gb_main,6,"passwd");

    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
    GBDATA *gb_seq = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

    if (gb_seq) {
        GBDATA *gb_ali = GB_brother(gb_seq,alignment);

        if (gb_ali) {
            GBDATA *gb_data = GB_search(gb_ali,"data",GB_STRING);

            GB_write_string(gb_data,sequence);
            free((char *) sequence);
        }
    }

    return 0;
}
