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

#include <aw_msg.hxx>
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

static void refill_syncWithList_cb(AW_root *, AW_selection_list *sellst, AWT_canvas *ntw) {
    sellst->clear();
    NT_fill_canvas_selection_list(sellst, ntw);
    sellst->insert_default("<don't sync>", DONT_SYNC_SCROLLING);
    sellst->update();
}

enum SyncMode {
    SM_EXPLICIT, // explicit button press
    SM_IMPLICIT, // implicit (select new source)
    SM_AUTO,     // auto (source scrolled) // @@@ not impl
};

static void perform_scroll_sync_cb(UNFIXED, int tgt_idx, SyncMode mode) {
    // perform a scroll-sync in canvas 'tgt_idx'

    AW_root *awr     = AW_root::SINGLETON;
    int      src_idx = awr->awar(awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, tgt_idx))->read_int();

    if (src_idx == DONT_SYNC_SCROLLING) {
        if (mode == SM_EXPLICIT) aw_message("Please select a window to sync with");
    }
    else {
        AWT_canvas *tgt_ntw = NT_get_canvas_by_index(tgt_idx);
        AWT_canvas *src_ntw = NT_get_canvas_by_index(src_idx);

        nt_assert(tgt_ntw && src_ntw && tgt_ntw != src_ntw);

#if defined(DEBUG)
        fprintf(stderr, "DEBUG: sync triggered (mode=%i)\n", int(mode));
#endif
    }
}

static void toggle_autosync_cb(AW_root *awr, int ntw_idx) {
    int do_autosync = awr->awar(awarname(AWAR_TEMPL_AUTO_SYNCED, ntw_idx))->read_int();

    // @@@ install/uninstall callbacks to source-canvas (called after refresh)

    if (do_autosync) {                                      // autosync activated?
        perform_scroll_sync_cb(NULL, ntw_idx, SM_IMPLICIT); // sync scrolling once
    }
}

AW_window *NT_create_syncScroll_window(AW_root *awr, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;

    int ntw_idx = NT_get_canvas_idx(ntw);
    nt_assert(ntw_idx>=0);

    AW_awar *awar_sync_with = awr->awar_int(awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, ntw_idx), DONT_SYNC_SCROLLING, AW_ROOT_DEFAULT);
    AW_awar *awar_autosync  = awr->awar_int(awarname(AWAR_TEMPL_AUTO_SYNCED,        ntw_idx), 0,                   AW_ROOT_DEFAULT);

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
        aws->at("box");
        AW_selection_list *sellist = aws->create_selection_list(awar_sync_with->awar_name, true);

        // refill list when new main-windows are created:
        RootCallback refill_sellist_cb = makeRootCallback(refill_syncWithList_cb, sellist, ntw);
        awr->awar(AWAR_NTREE_MAIN_WINDOW_COUNT)->add_callback(refill_sellist_cb);
        refill_sellist_cb(awr);

        awar_sync_with->add_callback(makeRootCallback(perform_scroll_sync_cb, ntw_idx, SM_IMPLICIT)); // resync when source changes
    }

    aws->at("dosync");
    aws->callback(makeWindowCallback(perform_scroll_sync_cb, ntw_idx, SM_EXPLICIT));
    aws->create_autosize_button("SYNC_SCROLL", "Sync scroll");

    aws->at("autosync");
    aws->label("Auto-sync?");
    aws->create_toggle(awar_autosync->awar_name);

    awar_autosync->add_callback(makeRootCallback(toggle_autosync_cb, ntw_idx));

    return aws;
}


