//  ==================================================================== //
//                                                                       //
//    File      : ps_tools.hxx                                           //
//    Purpose   :                                                        //
//    Time-stamp: <Mon Oct/04/2004 17:49 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2004          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef PS_TOOLS_HXX
#define PS_TOOLS_HXX

struct tms;

void PS_print_time_diff( const struct tms *_since, const char *_before = 0, const char *_after = 0 );

#else
#error ps_tools.hxx included twice
#endif // PS_TOOLS_HXX

