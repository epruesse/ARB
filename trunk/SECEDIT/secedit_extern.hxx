// =============================================================== //
//                                                                 //
//   File      : secedit_extern.hxx                                //
//   Purpose   : external interface                                //
//   Time-stamp: <Wed Sep/12/2007 09:38 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SECEDIT_EXTERN_HXX
#define SECEDIT_EXTERN_HXX

AW_window *SEC_create_main_window(AW_root *awr, GBDATA *gb_main);


#else
#error secedit_extern.hxx included twice
#endif // SECEDIT_EXTERN_HXX
