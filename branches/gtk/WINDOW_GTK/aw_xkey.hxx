#pragma once

#include "AW_gtk_forward_declarations.hxx"
#include <string>
//@depricated DO NOT USE, remove later
struct awXKeymap {
    int          xmod;
    int          xkey;
    const char  *xstr;
    AW_key_mod   awmod;
    AW_key_code  awkey;
    char        *awstr;
};

struct awKeymap {
    /**Bitmask containing infos about pressed modifier keys */
    AW_key_mod   modifier; 
    
    /** Contains what kind of key was pressed.
     * @note if the value is AW_KEY_ASCII str contains the exact key. */
    AW_key_code  key; 
    
    std::string  str;   
};



struct awXKeymap_modfree { // automatically defines key with SHIFT, ALT(META) and CTRL
    int          xkey;
    const char  *xstr_suffix;
    AW_key_code  awkey;
};



/**
 * Converts a gdk key event into an awkey event
 * @param event
 * @return 
 */
awKeymap gtk_key_2_awkey(const GdkEventKey& event);
