//  ==================================================================== //
//                                                                       //
//    File      : arbtools.h                                             //
//    Purpose   : small general purpose helper classes                   //
//    Time-stamp: <Fri Aug/08/2003 07:36 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2003           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef ARBTOOLS_H
#define ARBTOOLS_H

//  Base class for classes that may not be copied, neither via copy
//  constructor or assignment operator.

class Noncopyable {
    Noncopyable(const Noncopyable&);
    Noncopyable& operator=(const Noncopyable&);
public:
    Noncopyable() {}
};


#else
#error arbtools.h included twice
#endif // ARBTOOLS_H

