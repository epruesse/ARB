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

inline bool AW_IS_IMAGEREF(const char* label) { // @@@ move into AW_window.cxx aftermerge
    //! return true if 'label' is an image reference
    return label[0] == '#';
}

enum AW_key_code {
    AW_KEY_NONE,
    AW_KEY_ESCAPE,
    AW_KEY_F1,
    AW_KEY_F2,
    AW_KEY_F3,
    AW_KEY_F4,
    AW_KEY_F5,
    AW_KEY_F6,
    AW_KEY_F7,
    AW_KEY_F8,
    AW_KEY_F9,
    AW_KEY_F10,
    AW_KEY_F11,
    AW_KEY_F12,
    AW_KEY_LEFT,
    AW_KEY_RIGHT,
    AW_KEY_UP,
    AW_KEY_DOWN,
    AW_KEY_DELETE,
    AW_KEY_BACKSPACE,
    AW_KEY_INSERT,
    AW_KEY_HELP,
    AW_KEY_HOME,
    AW_KEY_END,
    AW_KEY_RETURN,
    AW_KEY_TAB,
    AW_KEY_ASCII
};

enum AW_key_mod {
    AW_KEYMODE_NONE    = 0,
    AW_KEYMODE_SHIFT   = 2,
    AW_KEYMODE_CONTROL = 4,
    AW_KEYMODE_ALT     = 8, // Alt or Meta key
    AW_KEYMODE_NUMLOCK = 16,
};

// define some inline functions to avoid comparing apples and oranges:
inline bool operator==(AW_key_code, char);
inline bool operator!=(AW_key_code, char);
inline bool operator==(char, AW_key_code);
inline bool operator!=(char, AW_key_code);

inline bool operator==(AW_key_mod, char);
inline bool operator!=(AW_key_mod, char);
inline bool operator==(char, AW_key_mod);
inline bool operator!=(char, AW_key_mod);

#else
#error aw_keysym.hxx included twice
#endif // AW_KEYSYM_HXX
