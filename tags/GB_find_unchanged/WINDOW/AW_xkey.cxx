#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "aw_keysym.hxx"
#include "aw_xkey.hxx"
#include <arbdb.h>

#ifndef aw_assert
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define aw_assert(bed) arb_assert(bed)
#endif

// for keysyms see /usr/include/X11/keysymdef.h


// each entry in awxkeymap_modfree generates 9 entries : 2*SHIFT, 2*ALT, 2*META, 2*CTRL, 1 unMODified
// see awModDef below
static awXKeymap_modfree awxkeymap_modfree[] = {
    { XK_Left,   "Left",   AW_KEY_LEFT },
    { XK_Right,  "Right",  AW_KEY_RIGHT },
    { XK_Up,     "Up",     AW_KEY_UP },
    { XK_Down,   "Down",   AW_KEY_DOWN },
    { XK_Home,   "Home",   AW_KEY_HOME },
    { XK_End,    "End",    AW_KEY_END },
    { XK_Delete, "Delete", AW_KEY_DELETE }, // was same as BackSpace in the past -- 2007/11/15
    
    { 0, 0, AW_KEY_NONE }, 
};

// manual key defs
// keys where all (or most) modifiers dont work should go here
static awXKeymap awxkeymap[] = {
    // sun keypad ? 
    { XK_Shift_R, XK_R10, "Shift-Left",  AW_KEYMODE_SHIFT, AW_KEY_LEFT,  0},
    { XK_Shift_R, XK_R12, "Shift-Right", AW_KEYMODE_SHIFT, AW_KEY_RIGHT, 0},
    { XK_Shift_L, XK_R10, "Shift-Left",  AW_KEYMODE_SHIFT, AW_KEY_LEFT,  0},
    { XK_Shift_L, XK_R12, "Shift-Right", AW_KEYMODE_SHIFT, AW_KEY_RIGHT, 0},
    { 0,          XK_R7,  "Home",        AW_KEYMODE_NONE,  AW_KEY_HOME,  0},
    { 0,          XK_R13, "End",         AW_KEYMODE_NONE,  AW_KEY_END,   0},

    // functions keys
    { 0, XK_F1,  0, AW_KEYMODE_NONE, AW_KEY_F1,  0},
    { 0, XK_F2,  0, AW_KEYMODE_NONE, AW_KEY_F2,  0},
    { 0, XK_F3,  0, AW_KEYMODE_NONE, AW_KEY_F3,  0},
    { 0, XK_F4,  0, AW_KEYMODE_NONE, AW_KEY_F4,  0},
    { 0, XK_F5,  0, AW_KEYMODE_NONE, AW_KEY_F5,  0},
    { 0, XK_F6,  0, AW_KEYMODE_NONE, AW_KEY_F6,  0},
    { 0, XK_F7,  0, AW_KEYMODE_NONE, AW_KEY_F7,  0},
    { 0, XK_F8,  0, AW_KEYMODE_NONE, AW_KEY_F8,  0},
    { 0, XK_F9,  0, AW_KEYMODE_NONE, AW_KEY_F9,  0},
    { 0, XK_F10, 0, AW_KEYMODE_NONE, AW_KEY_F10, 0},
    { 0, XK_F11, 0, AW_KEYMODE_NONE, AW_KEY_F11, 0},
    { 0, XK_F12, 0, AW_KEYMODE_NONE, AW_KEY_F12, 0},

    // other
    { 0, XK_BackSpace, "BackSpace", AW_KEYMODE_NONE, AW_KEY_BACKSPACE, 0},
    { 0, XK_Help,      0,           AW_KEYMODE_NONE, AW_KEY_HELP,      0},
    { 0, XK_Escape,    0,           AW_KEYMODE_NONE, AW_KEY_ESCAPE,    0},
    { 0, XK_Return,    0,           AW_KEYMODE_NONE, AW_KEY_RETURN,    0},
    { 0, XK_Tab,       0,           AW_KEYMODE_NONE, AW_KEY_RETURN,    0}, // done to accept input-field-changes via TAB, also disables inserting tabs

    { 0, 0, (char*)1, AW_KEYMODE_NONE, AW_KEY_NONE, 0}
};

static GB_HASH *awxkeymap_string_2_key_hash;
static long     awxkeymap_xkey_2_key_hash;

const int KEYMAX = 100; // size for hashes

#if defined(DEBUG)
static int mappedKeys = 0;
#endif

static void map_awXKey(Display *display, const awXKeymap *awxk) {
    if (awxk->xstr) {
        KeySym modlist[2];
        int    modsize;

        modlist[0] = awxk->xmod;
        modsize    = modlist[0] ? 1 : 0;

        XRebindKeysym(display, awxk->xkey, modlist, modsize,
                      (unsigned char*)awxk->xstr, strlen(awxk->xstr));

        GBS_write_hash(awxkeymap_string_2_key_hash, awxk->xstr, (long)awxk);
    }
    GBS_write_hashi(awxkeymap_xkey_2_key_hash, awxk->xkey, (long)awxk);

#if defined(DEBUG)
    ++mappedKeys;
#endif // DEBUG
}

void aw_install_xkeys(Display *display) {
    int i;
    
    awxkeymap_string_2_key_hash = GBS_create_hash(KEYMAX, 0);
    awxkeymap_xkey_2_key_hash   = GBS_create_hashi(KEYMAX);

    // auto-generate all key/modifier combinations for keys in awxkeymap_modfree
    for (i=0;;++i) {
        if (awxkeymap_modfree[i].xstr_suffix == 0) break;
    }

    const int MODS = 9;         // (2*SHIFT, 2*ALT, 2*META, 2*CTRL, 1 unMODified)

    struct awModDef {
        int         xmod;
        const char *xstr_prefix;
        AW_key_mod  awmod;
    } 
    moddef[MODS] = {
        { XK_Shift_L,   "Shift",   AW_KEYMODE_SHIFT },
        { XK_Shift_R,   "Shift",   AW_KEYMODE_SHIFT },
        { XK_Meta_L,    "Meta",    AW_KEYMODE_ALT },     // handle Meta as Alt
        { XK_Meta_R,    "Meta",    AW_KEYMODE_ALT },
        { XK_Alt_L,     "Alt",     AW_KEYMODE_ALT },
        { XK_Alt_R,     "Alt",     AW_KEYMODE_ALT },
        { XK_Control_L, "Control", AW_KEYMODE_CONTROL },
        { XK_Control_R, "Control", AW_KEYMODE_CONTROL },
        { 0,            0,         AW_KEYMODE_NONE },
    };

    int modfree = i;
    int gensize = modfree*MODS;

    static awXKeymap *generated = 0;

    aw_assert(generated == 0);  // oops - called twice
    generated = (awXKeymap*)GB_calloc(gensize, sizeof(*generated)); // never freed (cause hashes refers to it)

    for (i=0;i<modfree;++i) {
        const awXKeymap_modfree *mf = awxkeymap_modfree+i;
        for (int j = 0; j<MODS; ++j) {
            awXKeymap *km = generated+i*MODS+j;
            awModDef  *md = moddef+j;

            km->xmod  = md->xmod;
            km->xkey  = mf->xkey;
            km->xstr  = md->xstr_prefix ? GBS_global_string_copy("%s-%s", md->xstr_prefix, mf->xstr_suffix) : mf->xstr_suffix;
            km->awmod = md->awmod;
            km->awkey = mf->awkey;
            km->awstr = 0;

            map_awXKey(display, km);
        }
    }

    // add manually defined keys
    for (i=0;;++i) {
        if (awxkeymap[i].xstr== (char*)1) break;
        map_awXKey(display, awxkeymap+i);
    }

    aw_assert(mappedKeys<KEYMAX);
}

#if defined(DEBUG)
//#define DUMP_KEYEVENTS
#endif // DEBUG


const awXKeymap *aw_xkey_2_awkey(XKeyEvent *xkeyevent) {
    awXKeymap *result;
    static awXKeymap singlekey = { 0,0,0,AW_KEYMODE_NONE,AW_KEY_NONE,0};

    xkeyevent->state &= ~AW_KEYMODE_NUMLOCK; // ignore NUMLOCK

    static char    buffer[256];
    KeySym         keysym;
    XComposeStatus compose;
    int            count = XLookupString(xkeyevent,buffer,256,&keysym,&compose);
    buffer[count]        = 0;

#if defined(DUMP_KEYEVENTS)
    printf("state=%u keycode=%u name='%s' ", xkeyevent->state, xkeyevent->keycode, buffer);
#endif // DUMP_KEYEVENTS

    if( keysym >= XK_space && keysym <= XK_asciitilde ) {
        singlekey.awkey = AW_KEY_ASCII;
        singlekey.awmod = AW_KEYMODE_NONE;
        singlekey.awstr = buffer;

        result = &singlekey;
        
#if defined(DUMP_KEYEVENTS)
        printf("AW_KEY_ASCII:");
#endif // DUMP_KEYEVENTS
    }
    else {
        long ptr;

        if (count && (ptr = GBS_read_hash(awxkeymap_string_2_key_hash,buffer))){
            result    = (awXKeymap*)ptr;

#if defined(DUMP_KEYEVENTS)
            printf("_awxkeymap_string_2_key_hash['%s']=", buffer);
#endif // DUMP_KEYEVENTS
        }
        else if ( (ptr = GBS_read_hashi(awxkeymap_xkey_2_key_hash,keysym))){
            result    = (awXKeymap*)ptr;
            
#if defined(DUMP_KEYEVENTS)
            printf("_awxkeymap_xkey_2_key_hash['%x']='%s'", (unsigned)keysym, result->xstr);
#endif // DUMP_KEYEVENTS
        }
        else {
            singlekey.awkey = AW_KEY_NONE;
            singlekey.awmod = AW_KEYMODE_NONE;
            singlekey.awstr = 0;

            result = &singlekey;

#if defined(DUMP_KEYEVENTS)
            printf("Undefined key (keysym=%x)", (unsigned)keysym);
            if (count) printf(" name='%s'", buffer);
#endif // DUMP_KEYEVENTS
        }
    }
#if defined(DUMP_KEYEVENTS)
    printf(" key='%u' mod='%u' awstr='%s'\n", result->awkey, result->awmod, result->awstr);
#endif // DUMP_KEYEVENTS

    return result;
}

