#ifndef AW_XKEY_HXX
#define AW_XKEY_HXX

struct awXKeymap {
    int          xmod;
    int          xkey;
    const char  *xstr;
    AW_key_mod   awmod;
    AW_key_code  awkey;
    char        *awstr;
};

struct awXKeymap_modfree { // automatically defines key with SHIFT, ALT(META) and CTRL
    int          xkey;
    const char  *xstr_suffix;
    AW_key_code  awkey;
};

void aw_install_xkeys(Display *display);
const awXKeymap* aw_xkey_2_awkey(XKeyEvent *xkeyevent);

#else
#error aw_xkey.hxx included twice
#endif
