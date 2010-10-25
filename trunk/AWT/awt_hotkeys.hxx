//  ==================================================================== //
//                                                                       //
//    File      : awt_hotkeys.hxx                                        //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_HOTKEYS_HXX
#define AWT_HOTKEYS_HXX

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#if defined(DEBUG)
#ifndef _GLIBCXX_CSTDIO
#include <cstdio>
#endif
#endif // DEBUG

//  --------------------------
//      class awt_hotkeys
//  --------------------------
// this class automatically creates hotkeys from strings
// use one instance for one set of hotkeys
class awt_hotkeys {
private:
    bool used[26];
    bool USED[26];
    char artificial;
    char current[2];

    awt_hotkeys(const awt_hotkeys& other); // copying not allowed
    awt_hotkeys& operator=(const awt_hotkeys& other); // assignment not allowed
public:
    awt_hotkeys() {
        for (int i = 0; i<26; ++i) {
            USED[i] = used[i] = false;
        }
        artificial  = '0';
        current[0] = current[1] = 0;
    }
    virtual ~awt_hotkeys() {}

    // return a unique hotkey (returns an empty string if no hotkey left)
    const char* artificial_hotkey();
    // return a unique hotkey for label (uses one character from label if possible)
    const char* hotkey(const std::string& label);
};



#else
#error awt_hotkeys.hxx included twice
#endif // AWT_HOTKEYS_HXX

