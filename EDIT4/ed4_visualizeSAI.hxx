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

#define AWAR_SAI_CLR_TAB             "saicolors/"
#define AWAR_SAI_SELECT              AWAR_SAI_CLR_TAB "select"
#define AWAR_SAI_CLR_DEF             AWAR_SAI_CLR_TAB "clr_trans_tab/"
#define AWAR_SAI_CLR_TRANS_TAB_NAMES AWAR_SAI_CLR_TAB "clr_trans_tab_names"

#define AWAR_SAI_ENABLE                "tmp/sai/enable"
#define AWAR_SAI_CLR_TRANS_TABLE       "tmp/sai/clr_trans_table"
#define AWAR_SAI_CLR_TRANS_TAB_CREATE  "tmp/sai/clr_trans_tab_create"
#define AWAR_SAI_CLR_TRANS_TAB_COPY    "tmp/sai/clr_trans_tab_copy"

#define AWAR_SAI_CLR    "tmp/sai/color_"
#define AWAR_SAI_CLR_0  AWAR_SAI_CLR "0"
#define AWAR_SAI_CLR_1  AWAR_SAI_CLR "1"
#define AWAR_SAI_CLR_2  AWAR_SAI_CLR "2"
#define AWAR_SAI_CLR_3  AWAR_SAI_CLR "3"
#define AWAR_SAI_CLR_4  AWAR_SAI_CLR "4"
#define AWAR_SAI_CLR_5  AWAR_SAI_CLR "5"
#define AWAR_SAI_CLR_6  AWAR_SAI_CLR "6"
#define AWAR_SAI_CLR_7  AWAR_SAI_CLR "7"
#define AWAR_SAI_CLR_8  AWAR_SAI_CLR "8"
#define AWAR_SAI_CLR_9  AWAR_SAI_CLR "9"

#define CREATE_CLR_TR_TABLE  1
#define COPY_CLR_TR_TABLE    0

AW_window *ED4_createVisualizeSAI_window(AW_root *aw_root);
char      *getSaiColorString(int start, int end);   //used in ED4_text_terminal.cxx & SEC_paint.cxx to paint the SAI in the background
int        checkSai(const char *species_name);      //used in ED4_text_terminal.cxx  to validate SAI with respect to species 

