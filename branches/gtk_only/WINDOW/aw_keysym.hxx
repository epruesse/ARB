// =========================================================== //
//                                                             //
//   File      : aw_keysym.hxx                                 //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AW_KEYSYM_HXX
#define AW_KEYSYM_HXX

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>

enum AW_key_code {
    AW_KEY_NONE      = GDK_KEY_VoidSymbol,
    AW_KEY_ESCAPE    = GDK_KEY_Escape,
    AW_KEY_F1        = GDK_KEY_F1,
    AW_KEY_F2        = GDK_KEY_F2,
    AW_KEY_F3        = GDK_KEY_F3,
    AW_KEY_F4        = GDK_KEY_F4,
    AW_KEY_F5        = GDK_KEY_F5,
    AW_KEY_F6        = GDK_KEY_F6,
    AW_KEY_F7        = GDK_KEY_F7,
    AW_KEY_F8        = GDK_KEY_F8,
    AW_KEY_F9        = GDK_KEY_F9,
    AW_KEY_F10       = GDK_KEY_F10,
    AW_KEY_F11       = GDK_KEY_F11,
    AW_KEY_F12       = GDK_KEY_F12,
    AW_KEY_LEFT      = GDK_KEY_Left,
    AW_KEY_RIGHT     = GDK_KEY_Right,
    AW_KEY_UP        = GDK_KEY_Up,
    AW_KEY_DOWN      = GDK_KEY_Down,
    AW_KEY_DELETE    = GDK_KEY_Delete,
    AW_KEY_BACKSPACE = GDK_KEY_BackSpace,
    AW_KEY_INSERT    = GDK_KEY_Insert,
    AW_KEY_HELP      = GDK_KEY_Help,
    AW_KEY_HOME      = GDK_KEY_Home,
    AW_KEY_END       = GDK_KEY_End,
    AW_KEY_RETURN    = GDK_KEY_Return,
    AW_KEY_TAB       = GDK_KEY_Tab,
    AW_KEY_ASCII
};

#else
#error aw_keysym.hxx included twice
#endif // AW_KEYSYM_HXX
