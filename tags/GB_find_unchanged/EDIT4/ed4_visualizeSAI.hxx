#ifndef ed4_visualizeSAI_hxx_included
#define ed4_visualizeSAI_hxx_included
/*=======================================================================================*/
/*                                                                                       */
/*    File       : ed4_visualizeSAI.hxx                                                  */
/*    Purpose    : To Visualise the Sequence Associated Information (SAI) in the Editor  */
/*    Time-stamp : Tue Apr 1 2003                                                        */
/*    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                     */
/*    web site   : http://www.arb-home.de/                                               */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/


/* --------------------------------------------------------- */

#define AWAR_SAI_CLR_TAB                "saicolors/"
#define AWAR_SAI_SELECT                 AWAR_SAI_CLR_TAB "select" // current visualized SAI
#define AWAR_SAI_CLR_DEF                AWAR_SAI_CLR_TAB "clr_trans_tab/" // container for definitions
#define AWAR_SAI_ENABLE                 AWAR_SAI_CLR_TAB "enable" // global enable of visualization
#define AWAR_SAI_ALL_SPECIES            AWAR_SAI_CLR_TAB "all_species" // 1 = all / 0 = marked
#define AWAR_SAI_AUTO_SELECT            AWAR_SAI_CLR_TAB "auto_select" // 1 = auto select / 0 = manual select
#define AWAR_SAI_CLR_TRANS_TABLE        AWAR_SAI_CLR_TAB "clr_trans_table" // current translation table
#define AWAR_SAI_CLR_TRANS_TAB_NAMES    AWAR_SAI_CLR_TAB "clr_trans_tab_names" // ;-seperated list of existing translation tables
#define AWAR_SAI_CLR_TRANS_TAB_REL      AWAR_SAI_CLR_TAB "sai_relation/" // container to store trans tables for each SAI
#define AWAR_SAI_CLR_DEFAULTS_CREATED   AWAR_SAI_CLR_TAB "defaults_created" // whether defaults have been created (create only once)

#define AWAR_SAI_CLR_TRANS_TAB_NEW_NAME "tmp/sai/clr_trans_tab_new_name" // textfield to enter translation table name
#define AWAR_SAI_CLR                    "tmp/sai/color_0" // the definition of the current translation table (number runs from 0 to 9)
#define AWAR_SAI_CLR_COUNT              10

#define ED4_VIS_CREATE  1
#define ED4_VIS_COPY    0

/* --------------------------------------------------------- */

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def); // create awars
AW_window  *ED4_createVisualizeSAI_window(AW_root *aw_root);

#endif
