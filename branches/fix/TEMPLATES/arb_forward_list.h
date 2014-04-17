// ================================================================ //
//                                                                  //
//   File      : arb_forward_list.h                                 //
//   Purpose   : compatibility wrapper for forward_list             //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2014   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARB_FORWARD_LIST_H
#define ARB_FORWARD_LIST_H

// this header can be eliminated when arb EXPECTS Cxx11

// arb_forward_list can only use those members common
// between Non-Cxx11-std:list and Cxx11-std::forward_list
// (i.e. nearly nothing)

#ifdef Cxx11
# include <forward_list>
# define arb_forward_list std::forward_list
#else
# include <list>
# define arb_forward_list std::list
#endif

#else
#error arb_forward_list.h included twice
#endif // ARB_FORWARD_LIST_H
