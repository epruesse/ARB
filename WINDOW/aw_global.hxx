//  ==================================================================== //
//                                                                       //
//    File      : aw_global.hxx                                          //
//    Purpose   : global functions from WINDOWS lib                      //
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

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

void aw_create_fileselection_awars(AW_root *awr, const char *awar_base,
                                   const char *directory, const char *filter, const char *file_name,
                                   AW_default default_file = AW_ROOT_DEFAULT, bool resetValues = false);

void aw_detect_text_size(const char *text, size_t& width, size_t& height);


#else
#error aw_global.hxx included twice
#endif // AW_GLOBAL_HXX

