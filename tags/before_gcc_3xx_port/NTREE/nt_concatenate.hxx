/*=======================================================================================*/
/*                                                                                       */
/*    File       : nt_concatenate.hxx                                                    */
/*    Purpose    : Header file for NT_concatenate.cxx                                    */
/*    Time-stamp : Thu Nov 21 2002                                                       */
/*    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                     */
/*    web site   : http://www.arb-home.de/                                               */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/

#define AWAR_CON_SEQUENCE_TYPE       "tmp/concat/sequence_type"
#define AWAR_CON_NEW_ALIGNMENT_NAME  "tmp/concat/new_alignment_name"
#define AWAR_CON_ALIGNMENT_SEPARATOR "tmp/concat/alignment_separator"
#define AWAR_CON_DB_ALIGNS           "tmp/concat/database_alignments"
#define AWAR_CON_CONCAT_ALIGNS       "tmp/concat/concatenating_alignments"
#define AWAR_CON_DUMMY               "tmp/concat/dummy"
#define AWAR_CON_MERGE_FIELD         "tmp/concat/merge_field"
#define AWAR_CON_STORE_SIM_SP_NO     "tmp/concat/store_sim_sp_no"

#define MERGE_SIMILAR_CONCATENATE_ALIGNMENTS 1
#define MOVE_DOWN  0
#define MOVE_UP    1

struct conAlignStruct{
    GBDATA            *gb_main;
    AW_window         *aws;
    AW_root           *awr;
    AW_selection_list *db_id;
    AW_selection_list *con_id;
    char              *seqType;
};

struct SPECIES_ConcatenateList {
    GBDATA *species;
    char   *species_name;
    struct SPECIES_ConcatenateList *next;
};
typedef struct SPECIES_ConcatenateList *speciesConcatenateList;

AW_window *NT_createConcatenationWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesAndConcatenateWindow(AW_root *aw_root, AW_CL cl_ntw);
void       NT_createConcatenationAwars(AW_root *aw_root, AW_default aw_def);




