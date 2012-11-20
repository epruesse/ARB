// =============================================================== //
//                                                                 //
//   File      : AW_xkey.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_keysym.hxx"
#include "aw_xkey.hxx"
#include "aw_msg.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_assert.hxx"

#include <arbdbt.h>
#include <arb_defs.h>

#include <gdk/gdkevents.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeys.h>
#include <string>


#if defined(DEBUG)
// #define DUMP_KEYEVENTS
#endif // DEBUG



#if defined(ASSERTION_USED)
static int mappedKeys = 0;
#endif // ASSERTION_USED



awKeymap gtk_key_2_awkey(const GdkEventKey& event) {
    GTK_PARTLY_IMPLEMENTED;
    
    awKeymap result;
    static awKeymap singlekey = {AW_KEYMODE_NONE, AW_KEY_NONE, "" };
    //no easy way to know if numlock was on in gtk
//    bool numlockwason = false;

//    if (event.state & AW_KEYMODE_NUMLOCK) {    // numlock is active
//        xkeyevent.state &= ~AW_KEYMODE_NUMLOCK;    // ignore NUMLOCK
//        numlockwason      = true;
//    }

    const int   BUFFERSIZE = 256;;
    static char buffer[BUFFERSIZE];
    std::string keysymStr(gdk_keyval_name(event.keyval));
//    KeySym      keysym;
//    int         count      = XLookupString(xkeyevent, buffer, BUFFERSIZE, &keysym, NULL);
//    buffer[count]          = 0;

//    if (!buffer[0] && count) {
//#if defined(DUMP_KEYEVENTS)
//        printf("special case: Ctrl-Space\n");
//#endif
//        aw_assert(keysym == XK_space);
//        buffer[0] = ' ';
//    }

#if defined(DUMP_KEYEVENTS)
    printf("state=%u keycode=%u name='%s' ", xkeyevent.state, xkeyevent.keycode, buffer);
#endif // DUMP_KEYEVENTS

    if (event.keyval >= GDK_space && event.keyval <= GDK_asciitilde) {
        singlekey.key = AW_KEY_ASCII;
        
        //forward ctrl and alt state
        if(event.state & GDK_CONTROL_MASK) singlekey.modifier = (AW_key_mod) (singlekey.modifier &  AW_KEYMODE_CONTROL);
        if(event.state & GDK_META_MASK) singlekey.modifier = (AW_key_mod) (singlekey.modifier &  AW_KEYMODE_ALT); //FIXME make sure meta is really alt
        
        singlekey.str = keysymStr;
        aw_assert(!singlekey.str.empty());

        result = singlekey;

//        if (numlockwason && (xkeyevent.state & AW_KEYMODE_ALT)) {
//            static bool warned = false;
//            if (!warned) {
//                aw_message("Warning: Accelerator keys only work if NUMLOCK is off!");
//                warned = true;
//            }
//        }

#if defined(DUMP_KEYEVENTS)
        printf("AW_KEY_ASCII:");
#endif // DUMP_KEYEVENTS
    }
//    else {
//        long ptr;
//
//        if (count && (ptr = GBS_read_hash(awxkeymap_string_2_key_hash, buffer))) {
//            result    = (awXKeymap*)ptr;
//
//#if defined(DUMP_KEYEVENTS)
//            printf("_awxkeymap_string_2_key_hash['%s']=", buffer);
//#endif // DUMP_KEYEVENTS
//        }
//        else if ((ptr = GBS_read_numhash(awxkeymap_xkey_2_key_hash, keysym))) {
//            result    = (awXKeymap*)ptr;
//
//#if defined(DUMP_KEYEVENTS)
//            printf("_awxkeymap_xkey_2_key_hash['%x']='%s'", (unsigned)keysym, result->xstr);
//#endif // DUMP_KEYEVENTS
//        }
//        else {
//            singlekey.awkey = AW_KEY_NONE;
//            singlekey.awmod = AW_KEYMODE_NONE;
//            singlekey.awstr = 0;
//
//            result = &singlekey;
//
//#if defined(DUMP_KEYEVENTS)
//            printf("Undefined key (keysym=%x)", (unsigned)keysym);
//            if (count) printf(" name='%s'", buffer);
//#endif // DUMP_KEYEVENTS
//        }
//    }
//#if defined(DUMP_KEYEVENTS)
//    printf(" key='%u' mod='%u' awstr='%s'\n", result->awkey, result->awmod, result->awstr);
//#endif // DUMP_KEYEVENTS

    return result;
}

