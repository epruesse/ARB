/*=======================================================================================*/
/*                                                                                       */
/*    File      : nt_concatenate.hxx                                                     */
/*    Purpose   : Header file for NT_concatenate.cxx                                     */
/*    Time-stamp: <Wed Dec/17/2008 17:33 MET Coder@ReallySoft.de>                        */
/*    Author    : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                      */
/*    web site  : http://www.arb-home.de/                                                */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/

#ifndef NT_CONCATENATE_HXX
#define NT_CONCATENATE_HXX

AW_window *NT_createConcatenationWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root, AW_CL cl_ntw);
AW_window *NT_createMergeSimilarSpeciesAndConcatenateWindow(AW_root *aw_root, AW_CL cl_ntw);
void       NT_createConcatenationAwars(AW_root *aw_root, AW_default aw_def);

#else
#error nt_concatenate.hxx included twice
#endif // NT_CONCATENATE_HXX
