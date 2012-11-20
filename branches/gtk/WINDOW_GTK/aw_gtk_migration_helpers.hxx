//  ==================================================================== //
//                                                                       //
//    File      : aw_gtk_migration_helpers.hxx                           //
//    Purpose   : Helper functions to ease the migration from motif      //
//                to gtk                                                 //
//                                                                       //
//  Coded by Arne Boeckmann (aboeckma@mpi-bremen.de)           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#pragma once

#include <cstdio>

#define GTK_NOT_IMPLEMENTED printf("NOT IMPLEMENTED %s\n",  __PRETTY_FUNCTION__)
//#define GTK_PARTLY_IMPLEMENTED printf("PARTLY IMPLEMENTED %s\n",  __PRETTY_FUNCTION__)
#define GTK_PARTLY_IMPLEMENTED

//#define FIXME(str)  printf("FIXME: [%s] in %s\n", str, __PRETTY_FUNCTION__)
#define FIXME(str)
