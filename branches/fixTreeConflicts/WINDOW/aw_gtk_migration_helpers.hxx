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

//#define GTK_SILENT
#ifdef GTK_SILENT
#define GTK_PARTLY_IMPLEMENTED
#if defined(DEVEL_RALF)
#define FIXME
#define DUMP_CALLBACK
#else // !defined(DEVEL_RALF)
#define FIXME(str)
#endif
#else
#define FIXME(str)  printf("FIXME: %s %s\n", __PRETTY_FUNCTION__, str)
#endif

