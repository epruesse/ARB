/*=======================================================================================*/
/*                                                                                       */
/*    File       : NT_concatenate.cxx                                                    */
/*    Purpose    : 1.Concatenatenation of sequences or alignments                        */
/*                 2.Merging the fields of similar species and creating a new species    */
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

#define MERGE_SIMILAR_CONCATENATE_ALIGNMENTS 1

struct conAlignStruct{
    GBDATA            *gb_main; 
    AW_window         *aws;
	AW_root           *awr;
    AW_selection_list *db_id;
    AW_selection_list *con_id;
    char	          *seqType;
};

struct SPECIES_ConcatenateList {
    GBDATA *species;
    char   *species_name;
    struct SPECIES_ConcatenateList *next;
};
typedef struct SPECIES_ConcatenateList *speciesConcatenateList;

AW_window *NT_createConcatenationWindow(AW_root *aw_root, AW_default def);
AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root, int option);
void       createConcatenationAwars(AW_root *aw_root, AW_default aw_def);




