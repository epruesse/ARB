// ================================================================ //
//                                                                  //
//   File      : map_viewer.h                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef MAP_VIEWER_H
#define MAP_VIEWER_H

#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

void launch_MapViewer_cb(GBDATA *gbd, enum AD_MAP_VIEWER_TYPE type);

#else
#error map_viewer.h included twice
#endif // MAP_VIEWER_H
