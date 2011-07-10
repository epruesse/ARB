//  ==================================================================== //
//                                                                       //
//    File      : AWT_hotkeys.cxx                                        //
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

#include "awt_hotkeys.hxx"
#include <cctype>


using namespace std;

const char* awt_hotkeys::artificial_hotkey() {
    if (artificial <= '9') {
        current[0] = artificial++;
    }
    else {
        int i;
        for (i = 25; i >= 0; --i) {
            if (!used[i]) {
                current[0] = 'a'+i;
                used[i]    = true;
                break;
            }
            if (!USED[i]) {
                current[0] = 'A'+i;
                USED[i]    = true;
                break;
            }
        }

        if (i == 26) current[0] = 0;
    }

    return current;
}

const char* awt_hotkeys::hotkey(const string& label) {
    if (label.length()) {
        for (string::const_iterator ch = label.begin(); ch != label.end(); ++ch) {
            if (islower(*ch)) {
                if (!used[*ch-'a']) {
                    used[*ch-'a'] = true;
                    current[0] = *ch;
                    return current;
                }
            }
            else if (isupper(*ch)) {
                if (!USED[*ch-'A']) {
                    USED[*ch-'A'] = true;
                    current[0]    = *ch;
                    return current;
                }
            }
        }
    }
    return artificial_hotkey();
}




