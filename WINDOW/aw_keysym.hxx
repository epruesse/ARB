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

#ifdef GDK_KEY_VoidSymbol
#   define GDK_KEY(x) GDK_KEY_##x
#else
#   define GDK_KEY(x) GDK_##x
#endif

enum AW_key_code {
    AW_KEY_NONE      = GDK_KEY(VoidSymbol),
    AW_KEY_ESCAPE    = GDK_KEY(Escape),
    AW_KEY_F1        = GDK_KEY(F1),
    AW_KEY_F2        = GDK_KEY(F2),
    AW_KEY_F3        = GDK_KEY(F3),
    AW_KEY_F4        = GDK_KEY(F4),
    AW_KEY_F5        = GDK_KEY(F5),
    AW_KEY_F6        = GDK_KEY(F6),
    AW_KEY_F7        = GDK_KEY(F7),
    AW_KEY_F8        = GDK_KEY(F8),
    AW_KEY_F9        = GDK_KEY(F9),
    AW_KEY_F10       = GDK_KEY(F10),
    AW_KEY_F11       = GDK_KEY(F11),
    AW_KEY_F12       = GDK_KEY(F12),
    AW_KEY_LEFT      = GDK_KEY(Left),
    AW_KEY_RIGHT     = GDK_KEY(Right),
    AW_KEY_UP        = GDK_KEY(Up),
    AW_KEY_DOWN      = GDK_KEY(Down),
    AW_KEY_DELETE    = GDK_KEY(Delete),
    AW_KEY_BACKSPACE = GDK_KEY(BackSpace),
    AW_KEY_INSERT    = GDK_KEY(Insert),
    AW_KEY_HELP      = GDK_KEY(Help),
    AW_KEY_HOME      = GDK_KEY(Home),
    AW_KEY_END       = GDK_KEY(End),
    AW_KEY_RETURN    = GDK_KEY(Return),
    AW_KEY_TAB       = GDK_KEY(Tab),
    AW_KEY_ASCII
};

// define some inline functions to avoid comparing apples and oranges:
inline bool operator==(AW_key_code, char);
inline bool operator!=(AW_key_code, char);
inline bool operator==(char, AW_key_code);
inline bool operator!=(char, AW_key_code);

#else
#error aw_keysym.hxx included twice
#endif // AW_KEYSYM_HXX
