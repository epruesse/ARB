// ==================================================================== //
//                                                                      //
//   File      : awt_macro.hxx                                          //
//   Purpose   :                                                        //
//   Time-stamp: <Wed May/25/2005 17:31 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AWT_MACRO_HXX
#define AWT_MACRO_HXX

AW_window *awt_open_macro_window(AW_root *aw_root,const char *application_id);

#else
#error awt_macro.hxx included twice
#endif // AWT_MACRO_HXX

