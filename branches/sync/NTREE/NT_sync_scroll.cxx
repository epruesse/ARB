// =============================================================== //
//                                                                 //
//   File      : NT_sync_scroll.cxx                                //
//   Purpose   : Sync tree scrolling                               //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2016   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_sync_scroll.h"
#include "NT_local_proto.h"
#include "NT_local.h"

#include <arb_msg.h>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_select.hxx>


#define AWAR_TEMPL_SYNCED_WITH_WINDOW "tmp/sync%i/with"
#define AWAR_TEMPL_AUTO_SYNCED        "tmp/sync%i/auto"

#define MAX_AWARNAME_LENGTH (3+1+5+1+4)

#define DONT_SYNC_SCROLLING (-1)

inline const char *awarname(const char *awarname_template, int idx) {
    nt_assert(idx>=0 && idx<MAX_NT_WINDOWS);
    static char buffer[MAX_AWARNAME_LENGTH+1];

#if defined(ASSERTION_USED)
    int printed =
#endif
        sprintf(buffer, awarname_template, idx);
    nt_assert(printed<=MAX_AWARNAME_LENGTH);

    return buffer;
}

static void create_sync_awars(AW_root *awr, int ntw_idx) {
    awr->awar_int(awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, ntw_idx), DONT_SYNC_SCROLLING, AW_ROOT_DEFAULT);
    awr->awar_int(awarname(AWAR_TEMPL_AUTO_SYNCED,        ntw_idx), 0,                   AW_ROOT_DEFAULT);
}

static void refill_syncWithList_cb(AW_root *awr, AW_selection_list *sellst, AWT_canvas *ntw) {
    sellst->clear();
    NT_fill_canvas_selection_list(sellst, ntw);
    sellst->insert_default("<don't sync>", DONT_SYNC_SCROLLING);
    sellst->update();
}

AW_window *NT_create_syncScroll_window(AW_root *awr, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;

    int ntw_idx = NT_get_canvas_idx(ntw);
    nt_assert(ntw_idx>=0);

    create_sync_awars(awr, ntw_idx);

    {
        char *wid = GBS_global_string_copy("SYNC_TREE_SCROLL_%i", ntw_idx);
        aws->init(awr, wid,
                  ntw_idx>0
                  ? GBS_global_string("Synchronize tree scrolling (%i)", ntw_idx)
                  : "Synchronize tree scrolling");
        free(wid);
    }
    aws->load_xfig("syncscroll.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("syncscroll.hlp"));
    aws->create_button("HELP", "HELP", "H");

    {
        const char *awar_sync_with = awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, ntw_idx); // @@@ call resync on change

        aws->at("box");
        AW_selection_list *sellist = aws->create_selection_list(awar_sync_with, true);

        // refill list when new main-windows are created:
        RootCallback refill_sellist_cb = makeRootCallback(refill_syncWithList_cb, sellist, ntw);
        awr->awar(AWAR_NTREE_MAIN_WINDOW_COUNT)->add_callback(refill_sellist_cb);
        refill_sellist_cb(awr);
    }

    // @@@ add resync button
    // @@@ add auto-resync toggle

    return aws;
}


