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

void        ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def); // create awars
AW_window  *ED4_createVisualizeSAI_window(AW_root *aw_root);
const char *getSaiColorString(AW_root *aw_root, int start, int end); // used in ED4_text_terminal.cxx & SEC_paint.cxx to paint the SAI in the background

