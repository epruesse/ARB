//  ==================================================================== // 
//                                                                       // 
//    File      : mo_assert.h                                            // 
//    Purpose   : defines assert macro                                   // 
//    Time-stamp: <Thu May/06/2004 12:18 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2004              // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#ifndef MO_ASSERT_H
#define MO_ASSERT_H

#include "../../INCLUDE/arb_assert.h"
#define mo_assert(cond) arb_assert(cond)

#else
#error mo_assert.h included twice
#endif // MO_ASSERT_H

