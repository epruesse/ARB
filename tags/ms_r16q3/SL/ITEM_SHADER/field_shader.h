// ============================================================ //
//                                                              //
//   File      : field_shader.h                                 //
//   Purpose   : shader plugin using DB fields                  //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef FIELD_SHADER_H
#define FIELD_SHADER_H

#ifndef ITEM_SHADER_H
#include "item_shader.h"
#endif

ShaderPluginPtr makeItemFieldShader(BoundItemSel& itemtype);

#else
#error field_shader.h included twice
#endif // FIELD_SHADER_H
