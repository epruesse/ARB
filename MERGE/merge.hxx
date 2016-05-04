//  ==================================================================== //
//                                                                       //
//    File      : merge.hxx                                              //
//    Purpose   : Local header for usage inside directory MERGE          //
//                                                                       //
//                                                                       //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef MERGE_HXX
#define MERGE_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

#define mg_assert(bed) arb_assert(bed)

#define AWAR_MERGE_SAV     "merge/"
#define AWAR_MERGE_SAV_SRC AWAR_MERGE_SAV "src/"
#define AWAR_MERGE_SAV_DST AWAR_MERGE_SAV "dst/"

#define AWAR_MERGE_TMP     "tmp/" AWAR_MERGE_SAV
#define AWAR_MERGE_TMP_SRC AWAR_MERGE_TMP "src/"
#define AWAR_MERGE_TMP_DST AWAR_MERGE_TMP "dst/"

#define AWAR_DB_SRC AWAR_MERGE_TMP_SRC "db"
#define AWAR_DB_DST AWAR_MERGE_TMP_DST "db"

inline const char *awar_name_tmp(int db_nr, const char *name) {
    mg_assert(db_nr == 1 || db_nr == 2); // @@@ replace these magics (throughout whole library)
    static char buffer[256];
    sprintf(buffer, "%s%s", db_nr == 1 ? AWAR_MERGE_TMP_SRC : AWAR_MERGE_TMP_DST, name);
    return buffer;
}

AW_window *MG_create_merge_alignment_window(AW_root *awr);
AW_window *MG_create_merge_names_window(AW_root *awr);
AW_window *MG_create_merge_species_window(AW_root *awr, bool dst_is_new);
AW_window *MG_create_preserves_selection_window(AW_root *awr);
AW_window *MG_create_merge_SAIs_window(AW_root *awr);
AW_window *MG_create_merge_trees_window(AW_root *awr);
AW_window *MG_create_merge_configs_window(AW_root *awr);

AW_window *create_mg_check_fields_window(AW_root *aw_root);

void MG_create_config_awar(AW_root *aw_root, AW_default aw_def);
void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def);
void MG_create_extendeds_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_alignment_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_species_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_rename_awars(AW_root *aw_root, AW_default aw_def);


void MG_create_db_dependent_rename_awars(AW_root *aw_root, GBDATA *gb_src, GBDATA *gb_dst);

void     MG_set_renamed(bool renamed, AW_root *aw_root, const char *reason);
GB_ERROR MG_expect_renamed();

int MG_copy_and_check_alignments();

// export of gene-species:

void       MG_create_gene_species_awars(AW_root *aw_root, AW_default aw_def);
AW_window *MG_gene_species_create_field_transfer_def_window(AW_root *aw_root);
GB_ERROR   MG_export_fields(AW_root *aw_root, GBDATA *gb_src, GBDATA *gb_dst, GB_HASH *error_suppressor, GB_HASH *source_organism_hash); // export defined fields

#define AWAR_REMAP_SPECIES_LIST AWAR_MERGE_SAV "remap_species_list"
#define AWAR_REMAP_ENABLE       AWAR_MERGE_SAV "remap_enable"

#define IS_QUERIED_SPECIES(gb_species) GB_user_flag(gb_species, GB_USERFLAG_QUERY)
int mg_count_queried(GBDATA *gb_main);

const char *MG_left_AWAR_SPECIES_NAME();

#ifndef MG_MERGE_HXX
#include "mg_merge.hxx"
#endif

inline GBDATA *get_gb_main(int db_nr) {
    mg_assert(db_nr == 1 || db_nr == 2);
    return db_nr == 1 ? GLOBAL_gb_src : GLOBAL_gb_dst;
}


#else
#error merge.hxx included twice
#endif // MERGE_HXX
