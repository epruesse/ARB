#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "aw_keysym.hxx"
#include "aw_xkey.hxx"
#include <arbdb.h>

// for keysyms see /usr/include/X11/keysymdef.h

struct awxkeymap_struct awxkeymap[] =
{
    {   XK_Shift_R, XK_Left,    "Shift-Left",   AW_KEYMODE_SHIFT,   AW_KEY_LEFT,    0},
    {   XK_Shift_R, XK_Right,   "Shift-Right",  AW_KEYMODE_SHIFT,   AW_KEY_RIGHT,   0},
    {   XK_Shift_R, XK_Up,      "Shift-Up",     AW_KEYMODE_SHIFT,   AW_KEY_UP,      0},
    {   XK_Shift_R, XK_Down,    "Shift-Down",   AW_KEYMODE_SHIFT,   AW_KEY_DOWN,    0},
    {   XK_Shift_R, XK_R10,     "Shift-Left",   AW_KEYMODE_SHIFT,   AW_KEY_LEFT,    0},
    {   XK_Shift_R, XK_R12,     "Shift-Right",  AW_KEYMODE_SHIFT,   AW_KEY_RIGHT,   0},

    {   XK_Shift_L, XK_Left,    "Shift-Left",   AW_KEYMODE_SHIFT,   AW_KEY_LEFT,    0},
    {   XK_Shift_L, XK_Right,   "Shift-Right",  AW_KEYMODE_SHIFT,   AW_KEY_RIGHT,   0},
    {   XK_Shift_L, XK_Up,      "Shift-Up",     AW_KEYMODE_SHIFT,   AW_KEY_UP,      0},
    {   XK_Shift_L, XK_Down,    "Shift-Down",   AW_KEYMODE_SHIFT,   AW_KEY_DOWN,    0},
    {   XK_Shift_L, XK_R10,     "Shift-Left",   AW_KEYMODE_SHIFT,   AW_KEY_LEFT,    0},
    {   XK_Shift_L, XK_R12,     "Shift-Right",  AW_KEYMODE_SHIFT,   AW_KEY_RIGHT,   0},

    {   XK_Meta_R,  XK_Left,    "Meta-Left",    AW_KEYMODE_ALT,     AW_KEY_LEFT,    0}, // treat meta and alt the same
    {   XK_Meta_R,  XK_Right,   "Meta-Right",   AW_KEYMODE_ALT,     AW_KEY_RIGHT,   0},
    {   XK_Meta_R,  XK_Up,      "Meta-Up",      AW_KEYMODE_ALT,     AW_KEY_UP,      0},
    {   XK_Meta_R,  XK_Down,    "Meta-Down",    AW_KEYMODE_ALT,     AW_KEY_DOWN ,   0},

    {   XK_Meta_L,  XK_Left,    "Meta-Left",    AW_KEYMODE_ALT,     AW_KEY_LEFT,    0},
    {   XK_Meta_L,  XK_Right,   "Meta-Right",   AW_KEYMODE_ALT,     AW_KEY_RIGHT,   0},
    {   XK_Meta_L,  XK_Up,      "Meta-Up",      AW_KEYMODE_ALT,     AW_KEY_UP,      0},
    {   XK_Meta_L,  XK_Down,    "Meta-Down",    AW_KEYMODE_ALT,     AW_KEY_DOWN ,   0},

    {   XK_Alt_R,   XK_Left,    "Alt-Left",     AW_KEYMODE_ALT,     AW_KEY_LEFT ,   0},
    {   XK_Alt_R,   XK_Right,   "Alt-Right",    AW_KEYMODE_ALT,     AW_KEY_RIGHT,   0},
    {   XK_Alt_R,   XK_Up,      "Alt-Up",       AW_KEYMODE_ALT,     AW_KEY_UP,      0},
    {   XK_Alt_R,   XK_Down,    "Alt-Down",     AW_KEYMODE_ALT,     AW_KEY_DOWN,    0},

    {   XK_Alt_L,   XK_Left,    "Alt-Left",     AW_KEYMODE_ALT,     AW_KEY_LEFT ,   0},
    {   XK_Alt_L,   XK_Right,   "Alt-Right",    AW_KEYMODE_ALT,     AW_KEY_RIGHT,   0},
    {   XK_Alt_L,   XK_Up,      "Alt-Up",       AW_KEYMODE_ALT,     AW_KEY_UP,      0},
    {   XK_Alt_L,   XK_Down,    "Alt-Down",     AW_KEYMODE_ALT,     AW_KEY_DOWN,    0},

    {   XK_Control_R, XK_Left,  "Control-Left", AW_KEYMODE_CONTROL, AW_KEY_LEFT,    0},
    {   XK_Control_R, XK_Right, "Control-Right",AW_KEYMODE_CONTROL, AW_KEY_RIGHT,   0},
    {   XK_Control_R, XK_Up,    "Control-Up",   AW_KEYMODE_CONTROL, AW_KEY_UP,      0},
    {   XK_Control_R, XK_Down,  "Control-Down", AW_KEYMODE_CONTROL, AW_KEY_DOWN,    0},

    {   XK_Control_L, XK_Left,  "Control-Left", AW_KEYMODE_CONTROL, AW_KEY_LEFT,    0},
    {   XK_Control_L, XK_Right, "Control-Right",AW_KEYMODE_CONTROL, AW_KEY_RIGHT,   0},
    {   XK_Control_L, XK_Up,    "Control-Up",   AW_KEYMODE_CONTROL, AW_KEY_UP,      0},
    {   XK_Control_L, XK_Down,  "Control-Down", AW_KEYMODE_CONTROL, AW_KEY_DOWN,    0},

	{	0,	XK_Escape,	0,		AW_KEYMODE_NONE,	AW_KEY_ESCAPE , 0},

	{	0,	XK_Left,	"Left",		AW_KEYMODE_NONE,	AW_KEY_LEFT , 0},
	{	0,	XK_Right,	"Right",	AW_KEYMODE_NONE,	AW_KEY_RIGHT , 0},
	{	0,	XK_Up,		"Up",		AW_KEYMODE_NONE,	AW_KEY_UP , 0},
	{	0,	XK_Down,	"Down",		AW_KEYMODE_NONE,	AW_KEY_DOWN , 0},

	{	0, XK_R7,       "Home",	AW_KEYMODE_NONE, AW_KEY_HOME , 0},
	{	0, XK_R13,      "End",	AW_KEYMODE_NONE, AW_KEY_END , 0},

	{	0,	XK_F1,		0,		AW_KEYMODE_NONE,	AW_KEY_F1 , 0},
	{	0,	XK_F2,		0,		AW_KEYMODE_NONE,	AW_KEY_F2 , 0},
	{	0,	XK_F3,		0,		AW_KEYMODE_NONE,	AW_KEY_F3 , 0},
	{	0,	XK_F4,		0,		AW_KEYMODE_NONE,	AW_KEY_F4 , 0},
	{	0,	XK_F5,		0,		AW_KEYMODE_NONE,	AW_KEY_F5 , 0},
	{	0,	XK_F6,		0,		AW_KEYMODE_NONE,	AW_KEY_F6 , 0},
	{	0,	XK_F7,		0,		AW_KEYMODE_NONE,	AW_KEY_F7 , 0},
	{	0,	XK_F8,		0,		AW_KEYMODE_NONE,	AW_KEY_F8 , 0},
	{	0,	XK_F9,		0,		AW_KEYMODE_NONE,	AW_KEY_F9 , 0},
	{	0,	XK_F10,		0,		AW_KEYMODE_NONE,	AW_KEY_F10 , 0},
	{	0,	XK_F11,		0,		AW_KEYMODE_NONE,	AW_KEY_F11 , 0},
	{	0,	XK_F12,		0,		AW_KEYMODE_NONE,	AW_KEY_F12 , 0},

	{	0,	XK_BackSpace,	0,		AW_KEYMODE_NONE,	AW_KEY_BACKSPACE , 0},
	{	0,	XK_Delete,	0,		AW_KEYMODE_NONE,	AW_KEY_BACKSPACE , 0},

	{	0,	XK_Help,	0,		AW_KEYMODE_NONE,	AW_KEY_HELP , 0},
	{	0,	XK_Home,	0,		AW_KEYMODE_NONE,	AW_KEY_HOME , 0},
	{	0,	XK_End,		0,		AW_KEYMODE_NONE,	AW_KEY_END , 0},
	{	0,	XK_Return,	0,		AW_KEYMODE_NONE,	AW_KEY_RETURN , 0},
	{	0,	XK_Tab,		0,		AW_KEYMODE_NONE,	AW_KEY_RETURN , 0},

	{	0,	0,		(char*)1,	AW_KEYMODE_NONE,	AW_KEY_NONE,	0}
	};

GB_HASH *_awxkeymap_string_2_key_hash;
long _awxkeymap_xkey_2_key_hash;

void aw_install_xkeys(Display *display)
{
    int i;
    KeySym modlist[2];
    int modsize;
    _awxkeymap_string_2_key_hash = GBS_create_hash(50,0);
    _awxkeymap_xkey_2_key_hash = GBS_create_hashi(50);
    for (i=0;;i++) {
        if (awxkeymap[i].xstr== (char*)1) break;
        modlist[0] = awxkeymap[i].xmod;
        if (modlist[0]) modsize = 1;
        else modsize = 0;
        if (awxkeymap[i].xstr) {
            XRebindKeysym(display, awxkeymap[i].xkey,modlist,modsize,
                (unsigned char *)awxkeymap[i].xstr, strlen(awxkeymap[i].xstr) );
            GBS_write_hash(_awxkeymap_string_2_key_hash, awxkeymap[i].xstr, i+1);
        }
        GBS_write_hashi(_awxkeymap_xkey_2_key_hash,awxkeymap[i].xkey,i+1);
    }
}

#if defined(DEBUG)
// #define DUMP_KEYEVENTS
#endif // DEBUG


const struct awxkeymap_struct *aw_xkey_2_awkey(XKeyEvent *xkeyevent) {
    struct awxkeymap_struct *result;
	static struct awxkeymap_struct singlekey = { 0,0,0,AW_KEYMODE_NONE,AW_KEY_NONE,0};

    xkeyevent->state &= ~AW_KEYMODE_NUMLOCK; // ignore NUMLOCK

	static char    buffer[256];
	KeySym         keysym;
	XComposeStatus compose;
    int            count = XLookupString(xkeyevent,buffer,256,&keysym,&compose);
    buffer[count]        = 0;

#if defined(DUMP_KEYEVENTS)
    printf("state=%u keycode=%u ", xkeyevent->state, xkeyevent->keycode);
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
        long ind;

        if (count && (ind = GBS_read_hash(_awxkeymap_string_2_key_hash,buffer))){
            ind--;
            result = &awxkeymap[ind];
            
#if defined(DUMP_KEYEVENTS)
            printf("_awxkeymap_string_2_key_hash['%s']=", buffer);
#endif // DUMP_KEYEVENTS
        }
        else if ( (ind = GBS_read_hashi(_awxkeymap_xkey_2_key_hash,keysym))){
            ind--;
            result = &awxkeymap[ind];
            
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


//     if (count == 0) { // detect modifier presses
//         // printf("special key event: state=%i keycode=%i keysym=%u\n", xkeyevent->state, xkeyevent->keycode, (unsigned int)keysym);
//         if (keysym == XK_Meta_R || keysym == XK_Meta_L) {
//             // printf("single Meta detected\n");
//             singlekey.awmod = AW_KEYMODE_META;
//         }
//         else if (keysym == XK_Alt_R || keysym == XK_Alt_L) {
//             // printf("single Alt detected\n");
//             singlekey.awmod = AW_KEYMODE_ALT;
//         }
//         else if (keysym == XK_Control_R || keysym == XK_Control_L) {
//             // printf("single Control detected\n");
//             singlekey.awmod = AW_KEYMODE_CONTROL;
//         }
//         else if (keysym == XK_Shift_R || keysym == XK_Shift_L) {
//             // printf("single Shift detected\n");
//             singlekey.awmod = AW_KEYMODE_SHIFT;
//         }
//     }
