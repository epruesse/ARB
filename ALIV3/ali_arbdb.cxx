#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>

#include "ali_arbdb.hxx"


#define HELIX_PAIRS "helix_pairs"
#define HELIX_LINE  "helix_line"


ALI_ARBDB::~ALI_ARBDB(void)
{
    if (gb_main)
        GB_close(gb_main);
    if (alignment)
        free ((char *) alignment);
}

int ALI_ARBDB::open(char *name, char *use_alignment)
{
   gb_main = GB_open(name, "rt");
   if (!gb_main) {
      GB_print_error();
      return 1;
   }

   GB_begin_transaction(gb_main);

   if (use_alignment)
      alignment = strdup(use_alignment);
   else
      alignment = GBT_get_default_alignment(gb_main);

   GB_commit_transaction(gb_main);

    return 0;
}

void ALI_ARBDB::close(void)
{
   GB_close(gb_main);
   if (alignment)
      free(alignment);
}

char *ALI_ARBDB::get_sequence_string(char *name,int and_mark)
{
   char *sequence = 0;
   GBDATA *gb_species_data;
   GBDATA *gb_seq;

   gb_species_data = GB_search(gb_main, "species_data", GB_FIND);

   gb_seq = GB_find(gb_species_data, "name", name, down_2_level);
   if (gb_seq) {
        if (and_mark) 
         GB_write_flag(GB_get_father(gb_seq),1);
      gb_seq = GB_find(gb_seq, alignment, 0, this_level);
      if (gb_seq) {
         gb_seq = GB_find(gb_seq, "data", 0, down_level);
         if (gb_seq) 
            sequence = GB_read_string(gb_seq);
      }
   }

   if (sequence == 0)
        return 0;

    return sequence;
}

ALI_SEQUENCE *ALI_ARBDB::get_sequence(char *name,int and_mark)
{
   ALI_SEQUENCE *ali_seq;
   char *sequence = 0;
   GBDATA *gb_species_data;
   GBDATA *gb_seq;

   gb_species_data = GB_search(gb_main, "species_data", GB_FIND);

   gb_seq = GB_find(gb_species_data, "name", name, down_2_level);
   if (gb_seq) {
        if (and_mark) 
         GB_write_flag(GB_get_father(gb_seq),1);
      gb_seq = GB_find(gb_seq, alignment, 0, this_level);
      if (gb_seq) {
         gb_seq = GB_find(gb_seq, "data", 0, down_level);
         if (gb_seq) 
            sequence = GB_read_string(gb_seq);
      }
   }

   if (sequence == 0)
        return 0;

   ali_seq = new ALI_SEQUENCE(name,sequence);

    return ali_seq;
}

char *ALI_ARBDB::get_extended(char *name)
{
   char             *extended = 0;
   GBDATA         *gb_extended_data;
   GBDATA         *gb_ex;

   gb_extended_data = GB_search(gb_main, "extended_data", GB_FIND);

   gb_ex = GB_find(gb_extended_data, "name", name, down_2_level);
   if (gb_ex) {
      gb_ex = GB_find(gb_ex, alignment, 0, this_level);
      if (gb_ex) {
         gb_ex = GB_find(gb_ex, "data", 0, down_level);
         if (gb_ex)
            extended = GB_read_string(gb_ex);
      }
   }

   return extended;
}


int ALI_ARBDB::put_sequence_string(char *name, char *sequence, char *info)
{
    GBDATA         *gb_species_data;
    GBDATA         *gb_seq;
    GBDATA         *gb_ali;
    GBDATA         *gb_data;
    GBDATA        *gb_mark;
    int             sequence_len = strlen(sequence);

    GB_change_my_security(gb_main,6,"passwd");
    gb_species_data = GB_search(gb_main, "species_data", GB_FIND);

    gb_seq = GB_find(gb_species_data, "name", name, down_2_level);
    if (gb_seq) {
        gb_ali = GB_find(gb_seq, alignment, 0, this_level);
        if (gb_ali) {
            gb_data = GB_search(gb_ali, "data", GB_STRING);
            GB_write_string(gb_data,sequence);
            free((char *) sequence);
            if (info){
                gb_mark = GB_search(gb_ali, "mark", GB_BITS);
                GB_write_bits(gb_mark, info, sequence_len, '.');
            }
        }
    }

    return 0;
}

int ALI_ARBDB::put_sequence(char *name, ALI_SEQUENCE *sequence, char *info)
{
    GBDATA         *gb_species_data;
    GBDATA         *gb_seq;
    GBDATA         *gb_ali;
    GBDATA         *gb_data;
    GBDATA        *gb_mark;
   char           *string;

    GB_change_my_security(gb_main,6,"passwd");
    gb_species_data = GB_search(gb_main, "species_data", GB_FIND);

    gb_seq = GB_find(gb_species_data, "name", name, down_2_level);
    if (gb_seq) {
        gb_ali = GB_find(gb_seq, alignment, 0, this_level);
        if (gb_ali) {
            gb_data = GB_search(gb_ali, "data", GB_STRING);
            string = sequence->string();
            GB_write_string(gb_data,string);
            free((char *) string);
            if (info){
                gb_mark = GB_search(gb_ali, "mark", GB_BITS);
                GB_write_bits(gb_mark, info, sequence->length() ,'.' );
            }
        }
    }

    return 0;
}


int ALI_ARBDB::put_extended(char *name, char *sequence, char *info)
{
    GBDATA         *gb_extended_data;
    GBDATA          *gb_extended;
    GBDATA         *gb_data;
    GBDATA          *gb_mark;
 
    GB_change_my_security(gb_main,6,"passwd");
 
    gb_extended = GBT_create_SAI(gb_main,name);
    gb_data = GBT_add_data(gb_extended,alignment,"data",GB_STRING);
    GB_write_string(gb_data,sequence);
    if (info){
    gb_mark = GB_find(gb_data,"mark",0,this_level);
    if (!gb_mark)
        gb_mark = GB_create(GB_get_father(gb_data),"mark");
    GB_write_bits(gb_mark,info,strlen(sequence),'.');
    }
 
    return 0;
}
