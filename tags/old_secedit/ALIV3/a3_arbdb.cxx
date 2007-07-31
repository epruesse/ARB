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
    if (gb_main)    GB_exit(gb_main);
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
   GB_exit(gb_main);

   if (alignment) free(alignment);
   alignment = 0;
}

// -----------------------------------------------------------------------------
    char *A3Arbdb::get_sequence_string ( const char *name,
                                         int     and_mark )
// -----------------------------------------------------------------------------
{
   char   *sequence = NULL;
   GBDATA *gb_species_data;
   GBDATA *gb_seq;

   gb_species_data = GB_search(gb_main,"species_data",GB_FIND);

   gb_seq = GB_find(gb_species_data,"name",name,down_2_level);

   if (gb_seq)
   {
       if (and_mark) GB_write_flag(GB_get_father(gb_seq),1);

       gb_seq = GB_find(gb_seq,alignment,0,this_level);

       if (gb_seq)
       {
           gb_seq = GB_find(gb_seq,"data",0,down_level);

           if (gb_seq) sequence = GB_read_string(gb_seq);
       }
   }

   if (sequence == 0) return 0;

   return sequence;
}

// -----------------------------------------------------------------------------
    int A3Arbdb::put_sequence_string ( char *name,
                                       char *sequence,
                                       char *info )
// -----------------------------------------------------------------------------
{
    GBDATA  *gb_species_data;
    GBDATA  *gb_seq;
    GBDATA  *gb_ali;
    GBDATA  *gb_data;
    GBDATA  *gb_mark;
    int      sequence_len = strlen(sequence);

    GB_change_my_security(gb_main,6,"passwd");

    gb_species_data = GB_search(gb_main,"species_data",GB_FIND);

    gb_seq = GB_find(gb_species_data,"name",name,down_2_level);

    if (gb_seq)
    {
        gb_ali = GB_find(gb_seq,alignment,0,this_level);

        if (gb_ali)
        {
            gb_data = GB_search(gb_ali,"data",GB_STRING);

            GB_write_string(gb_data,sequence);

            free((char *) sequence);

            if (info)
            {
                gb_mark = GB_search(gb_ali,"mark",GB_BITS);

                GB_write_bits(gb_mark,info,sequence_len,".");
            }
        }
    }

    return 0;
}
