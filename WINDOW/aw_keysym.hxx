#ifndef AW_KEYSYM_HXX
#define AW_KEYSYM_HXX

// #ifndef _AW_KEY_CODES_INCLUDED
// #define _AW_KEY_CODES_INCLUDED

typedef enum {
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

} AW_key_code;


typedef enum {
	AW_KEYMODE_NONE    = 0,
	AW_KEYMODE_SHIFT   = 2,
	AW_KEYMODE_CONTROL = 4,
	AW_KEYMODE_ALT     = 8, // Alt or Meta key
	AW_KEYMODE_NUMLOCK = 16,
} AW_key_mod;

// #endif

#else
#error aw_keysym.hxx included twice
#endif


