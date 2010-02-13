// ================================================================ //
//                                                                  //
//   File      : map_viewer.hxx                                     //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef MAP_VIEWER_HXX
#define MAP_VIEWER_HXX

#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

void MapViewer_set_default_root(AW_root *aw_root);
void launch_MapViewer_cb(GBDATA *gbd, enum AD_MAP_VIEWER_TYPE type);

#else
#error map_viewer.hxx included twice
#endif // MAP_VIEWER_HXX
