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

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

#define mg_assert(bed) arb_assert(bed)

#define AWAR_MERGE_DB "tmp/merge1/db"
#define AWAR_MAIN_DB  "tmp/merge2/db"

AW_window *MG_merge_alignment_cb(AW_root *awr);
AW_window *MG_merge_names_cb(AW_root *awr);
AW_window *MG_merge_species_cb(AW_root *awr);
AW_window *MG_select_preserves_cb(AW_root *awr);
AW_window *MG_merge_extendeds_cb(AW_root *awr);
AW_window *MG_merge_trees_cb(AW_root *awr);
AW_window *MG_merge_configs_cb(AW_root *awr);
AW_window *create_mg_check_fields(AW_root *aw_root);

void MG_create_config_awar(AW_root *aw_root, AW_default aw_def);
void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def);
void MG_create_extendeds_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_alignment_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_species_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_rename_awars(AW_root *aw_root, AW_default aw_def);


void MG_create_db_dependent_rename_awars(AW_root *aw_root, GBDATA *gb_merge, GBDATA *gb_dest);

void     MG_set_renamed(bool renamed, AW_root *aw_root, const char *reason);
GB_ERROR MG_expect_renamed();

int MG_copy_and_check_alignments(AW_window *aww);

// export of gene-species:

void       MG_create_gene_species_awars(AW_root *aw_root, AW_default aw_def);
AW_window *MG_gene_species_create_field_transfer_def_window(AW_root *aw_root);
GB_ERROR   MG_export_fields(AW_root *aw_root, GBDATA *gb_source, GBDATA *gb_dest, GB_HASH *error_suppressor, GB_HASH *source_organism_hash); // export defined fields

#define AWAR_REMAP_SPECIES_LIST "merge/remap_species_list"
#define AWAR_REMAP_ENABLE       "merge/remap_enable"

#define AWAR_MERGE_GENE_SPECIES_BASE "merge/gene_species/"

#define IS_QUERIED_SPECIES(gb_species) (1 & GB_read_usr_private(gb_species))
int mg_count_queried(GBDATA *gb_main);

const char *MG_left_AWAR_SPECIES_NAME();
const char *MG_right_AWAR_SPECIES_NAME();

#ifndef MG_MERGE_HXX
#include "mg_merge.hxx"
#endif

#else
#error merge.hxx included twice
#endif // MERGE_HXX
