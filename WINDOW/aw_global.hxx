//  ==================================================================== // 
//                                                                       // 
//    File      : aw_global.hxx                                          // 
//    Purpose   : global functions from WINDOWS lib                      // 
//    Time-stamp: <Mon Jun/07/2004 13:38 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in June 2004             // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#ifndef AW_GLOBAL_HXX
#define AW_GLOBAL_HXX

void aw_create_selection_box_awars(AW_root *awr, const char *awar_base, const char *directory, const char *filter, const char *file_name, AW_default default_file = AW_ROOT_DEFAULT);

#else
#error aw_global.hxx included twice
#endif // AW_GLOBAL_HXX

