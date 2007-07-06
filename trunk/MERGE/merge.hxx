//  ==================================================================== //
//                                                                       //
//    File      : merge.hxx                                              //
//    Purpose   : Local header for usage inside directory MERGE          //
//    Time-stamp: <Thu Jul/05/2007 16:47 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
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
void MG_create_alignment_awars(AW_root *aw_root,AW_default aw_def);
void MG_create_species_awars(AW_root *aw_root, AW_default aw_def);
void MG_create_rename_awars(AW_root *aw_root, AW_default aw_def);


void MG_create_db_dependent_rename_awars(AW_root *aw_root, GBDATA *gb_merge, GBDATA *gb_dest);

void     MG_set_renamed(bool renamed, AW_root *aw_root, const char *reason);
GB_ERROR MG_expect_renamed();

int MG_check_alignment(AW_window *aww, int fast = 0);

// export of gene-species:

void       MG_create_gene_species_awars(AW_root *aw_root, AW_default aw_def);
AW_window *MG_gene_species_create_field_transfer_def_window(AW_root *aw_root);
GB_ERROR   MG_export_fields(AW_root *aw_root, GBDATA *gb_source, GBDATA *gb_dest, GB_HASH *error_suppressor, GB_HASH *source_organism_hash); // export defined fields

#define AWAR_REMAP_SPECIES_LIST "merge/remap_species_list"
#define AWAR_REMAP_ENABLE       "merge/remap_enable"

#define AWAR_MERGE_GENE_SPECIES_BASE "merge/gene_species/"

const char *MG_left_AWAR_SPECIES_NAME();
const char *MG_right_AWAR_SPECIES_NAME();

class MG_remap {
    int in_length;
    int out_length;
    int *remap_tab;
    int *soft_remap_tab;
    int compiled;
public:
    MG_remap();
    ~MG_remap();
    GB_ERROR set(const char *in_reference, const char *out_reference); // returns only warnings
    GB_ERROR compile();		// after last set
    char *remap(const char *sequence); // returns 0 on error, else copy of sequence
};

class AW_root;

class MG_remaps {
public:
    int n_remaps;
    char **alignment_names;
    MG_remap **remaps;
    MG_remaps(GBDATA *gb_left,GBDATA *gb_right,AW_root *awr);
    ~MG_remaps();
};

extern GBDATA *gb_merge;
extern GBDATA *gb_dest;
extern GBDATA *gb_main;
