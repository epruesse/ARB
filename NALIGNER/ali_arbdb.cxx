// =============================================================== //
//                                                                 //
//   File      : ali_arbdb.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ali_sequence.hxx"
#include "ali_arbdb.hxx"


#define HELIX_PAIRS     "helix_pairs"
#define HELIX_LINE      "helix_line"


ALI_ARBDB::~ALI_ARBDB()
{
    if (gb_main) GB_close(gb_main);
    freenull(alignment);
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

void ALI_ARBDB::close()
{
    GB_close(gb_main);
    freenull(alignment);
}

char *ALI_ARBDB::get_sequence_string(char *name, int and_mark)
{
    char *sequence = 0;
    GBDATA *gb_species_data;
    GBDATA *gb_seq;

    gb_species_data = GBT_get_species_data(gb_main);

    gb_seq = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (gb_seq) {
        if (and_mark) GB_write_flag(GB_get_father(gb_seq), 1);
        gb_seq = GB_brother(gb_seq, alignment);
        if (gb_seq) {
            gb_seq = GB_entry(gb_seq, "data");
            if (gb_seq)
                sequence = GB_read_string(gb_seq);
        }
    }

    if (sequence == 0)
        return 0;

    return sequence;
}

ALI_SEQUENCE *ALI_ARBDB::get_sequence(char *name, int and_mark)
{
    ALI_SEQUENCE *ali_seq;
    char *sequence = 0;
    GBDATA *gb_species_data;
    GBDATA *gb_seq;

    gb_species_data = GBT_get_species_data(gb_main);

    gb_seq = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (gb_seq) {
        if (and_mark) GB_write_flag(GB_get_father(gb_seq), 1);
        gb_seq = GB_brother(gb_seq, alignment);
        if (gb_seq) {
            gb_seq = GB_entry(gb_seq, "data");
            if (gb_seq)
                sequence = GB_read_string(gb_seq);
        }
    }

    if (sequence == 0)
        return 0;

    ali_seq = new ALI_SEQUENCE(name, sequence);

    return ali_seq;
}

char *ALI_ARBDB::get_SAI(char *name) {
    char   *extended    = 0;
    GBDATA *gb_sai_data = GBT_get_SAI_data(gb_main);
    GBDATA *gb_sai      = GB_find_string(gb_sai_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

    if (gb_sai) {
        gb_sai = GB_brother(gb_sai, alignment);
        if (gb_sai) {
            gb_sai = GB_entry(gb_sai, "data");
            if (gb_sai)
                extended = GB_read_string(gb_sai);
        }
    }

    return extended;
}


int ALI_ARBDB::put_sequence_string(char *name, char *sequence) {
    GB_change_my_security(gb_main, 6);
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);

    GBDATA *gb_seq = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (gb_seq) {
        GBDATA *gb_ali = GB_brother(gb_seq, alignment);
        if (gb_ali) {
            GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
            GB_write_string(gb_data, sequence);
            free(sequence);
        }
    }

    return 0;
}

int ALI_ARBDB::put_sequence(char *name, ALI_SEQUENCE *sequence) {
    GB_change_my_security(gb_main, 6);
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);

    GBDATA *gb_seq = GB_find_string(gb_species_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    if (gb_seq) {
        GBDATA *gb_ali = GB_brother(gb_seq, alignment);
        if (gb_ali) {
            GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
            char *String = sequence->string();
            GB_write_string(gb_data, String);
            free(String);
        }
    }

    return 0;
}


int ALI_ARBDB::put_SAI(const char *name, char *sequence) {
    GB_change_my_security(gb_main, 6);

    GBDATA *gb_extended = GBT_find_or_create_SAI(gb_main, name);
    GBDATA *gb_data     = GBT_add_data(gb_extended, alignment, "data", GB_STRING);
    GB_write_string(gb_data, sequence);

    return 0;
}
