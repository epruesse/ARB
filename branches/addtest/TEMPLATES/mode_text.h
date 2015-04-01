// ================================================================= //
//                                                                   //
//   File      : mode_text.hxx                                       //
//   Purpose   : generate infotext for modes                         //
//               (for use in AW_window_menu_modes)                   //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2013   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef MODE_TEXT_HXX
#define MODE_TEXT_HXX

#define MT_MODE(modeName) modeName " MODE"

#define MT_LEFT(leftInfo)     "  LEFT: " leftInfo
#define MT_RIGHT(rightInfo)   "  RIGHT: " rightInfo
#define MT_MIDDLE(middleInfo) "  MIDDLE: " middleInfo
#define MT_KEYS(keyInfo)      "  (" keyInfo ")"

#define KEYINFO_ABORT           "ESC=abort"
#define KEYINFO_RESET           "0=reset"
#define KEYINFO_ABORT_AND_RESET KEYINFO_ABORT " " KEYINFO_RESET

#define MODE_TEXT_1BUTTON(modeName,leftInfo)                       MT_MODE(modeName) MT_LEFT(leftInfo)
#define MODE_TEXT_2BUTTONS(modeName,leftInfo,rightInfo)            MT_MODE(modeName) MT_LEFT(leftInfo) MT_RIGHT(rightInfo)
#define MODE_TEXT_3BUTTONS(modeName,leftInfo,middleInfo,rightInfo) MT_MODE(modeName) MT_LEFT(leftInfo) MT_MIDDLE(middleInfo) MT_RIGHT(rightInfo)

#define MODE_TEXT_1BUTTON_KEYS(modeName,leftInfo,keyInfo)                       MT_MODE(modeName) MT_LEFT(leftInfo) MT_KEYS(keyInfo)
#define MODE_TEXT_2BUTTONS_KEYS(modeName,leftInfo,rightInfo,keyInfo)            MT_MODE(modeName) MT_LEFT(leftInfo) MT_RIGHT(rightInfo) MT_KEYS(keyInfo)
#define MODE_TEXT_3BUTTONS_KEYS(modeName,leftInfo,middleInfo,rightInfo,keyInfo) MT_MODE(modeName) MT_LEFT(leftInfo) MT_MIDDLE(middleInfo) MT_RIGHT(rightInfo) MT_KEYS(keyInfo)

#define MODE_TEXT_STANDARD_ZOOMMODE() MODE_TEXT_2BUTTONS_KEYS("ZOOM", "zoom in", "zoom out (click or drag)", KEYINFO_RESET)
#define MODE_TEXT_PLACEHOLDER()       MT_MODE("PLACEHOLDER") "  (reserved)"

inline const char *no_mode_text_defined() {
    arb_assert(0); // pleae define an infotext for current mode in caller
    return "No help for this mode available";
}

#else
#error mode_text.hxx included twice
#endif // MODE_TEXT_HXX
