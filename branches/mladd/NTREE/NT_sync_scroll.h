// =============================================================== //
//                                                                 //
//   File      : NT_sync_scroll.h                                  //
//   Purpose   : Sync tree scrolling                               //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2016   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_SYNC_SCROLL_H
#define NT_SYNC_SCROLL_H

#ifndef AWT_CANVAS_HXX
#include <awt_canvas.hxx>
#endif

AW_window *NT_create_syncScroll_window(AW_root *awr, AWT_canvas *ntw);

#else
#error NT_sync_scroll.h included twice
#endif // NT_SYNC_SCROLL_H
