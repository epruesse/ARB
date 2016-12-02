// =============================================================== //
//                                                                 //
//   File      : NT_sync_scroll.cxx                                //
//   Purpose   : Sync tree scrolling                               //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2016   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ScrollSynchronizer.h"
#include "NT_sync_scroll.h"

#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_select.hxx>

#include <downcast.h>

#define AWAR_TEMPL_SYNCED_WITH_WINDOW "tmp/sync%i/with"
#define AWAR_TEMPL_AUTO_SYNCED        "tmp/sync%i/auto"

#define MAX_AWARNAME_LENGTH (3+1+5+1+4)

static ScrollSynchronizer synchronizer;

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

static void refill_syncWithList_cb(AW_root *, AW_selection_list *sellst, TREE_canvas *ntw) {
    sellst->clear();
    NT_fill_canvas_selection_list(sellst, ntw);
    sellst->insert_default("<don't sync>", NO_SCROLL_SYNC);
    sellst->update();
}

static unsigned auto_refresh_cb(AW_root*) {
    synchronizer.auto_update();
    return 0; // do not call again
}

static void canvas_updated_cb(AWT_canvas *ntw, AW_CL /*cd*/) {
    TREE_canvas *tw = DOWNCAST(TREE_canvas*, ntw);
    synchronizer.announce_update(tw->get_index());

    AW_root *awr = ntw->awr;
    awr->add_timed_callback(250, makeTimedCallback(auto_refresh_cb));
}

static void sync_changed_cb(AW_root *awr, int slave_idx) {
    AW_awar *awar_sync_with = awr->awar(awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, slave_idx));
    AW_awar *awar_autosync  = awr->awar(awarname(AWAR_TEMPL_AUTO_SYNCED,        slave_idx));

    int  master_idx = awar_sync_with->read_int();
    bool autosync   = awar_autosync->read_int();

    if (valid_canvas_index(master_idx)) {
        TREE_canvas *master = NT_get_canvas_by_index(master_idx);
        master->at_screen_update_call(canvas_updated_cb, 0);
    }

    GB_ERROR error = synchronizer.define_dependency(slave_idx, master_idx, autosync);
    nt_assert(implicated(error, autosync)); // error may only occur if autosync is ON

    if (error) {
        aw_message(GBS_global_string("Auto-sync turned off (%s)", error));
        awar_autosync->write_int(0);
    }
    else {
        synchronizer.update_implicit(slave_idx);
    }
}

static void explicit_scroll_sync_cb(AW_window*, int tgt_idx) {
    aw_message_if(synchronizer.update_explicit(tgt_idx));
}

AW_window *NT_create_syncScroll_window(AW_root *awr, TREE_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;

    int ntw_idx = ntw->get_index();
    nt_assert(ntw_idx>=0);

    AW_awar *awar_sync_with = awr->awar_int(awarname(AWAR_TEMPL_SYNCED_WITH_WINDOW, ntw_idx), NO_SCROLL_SYNC, AW_ROOT_DEFAULT);
    AW_awar *awar_autosync  = awr->awar_int(awarname(AWAR_TEMPL_AUTO_SYNCED,        ntw_idx), 0,              AW_ROOT_DEFAULT);

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

    RootCallback sync_changed_rcb = makeRootCallback(sync_changed_cb, ntw_idx);
    {
        aws->at("box");
        AW_selection_list *sellist = aws->create_selection_list(awar_sync_with->awar_name, true);

        // refill list when new main-windows are created:
        RootCallback refill_sellist_cb = makeRootCallback(refill_syncWithList_cb, sellist, ntw);
        awr->awar(AWAR_NTREE_MAIN_WINDOW_COUNT)->add_callback(refill_sellist_cb);
        refill_sellist_cb(awr);

        awar_sync_with->add_callback(sync_changed_rcb); // resync when source changes
    }

    aws->at("dosync");
    aws->callback(makeWindowCallback(explicit_scroll_sync_cb, ntw_idx));
    aws->create_autosize_button("SYNC_SCROLL", "Sync scroll");

    aws->at("autosync");
    aws->label("Auto-sync?");
    aws->create_toggle(awar_autosync->awar_name);

    awar_autosync->add_callback(sync_changed_rcb); // resync when auto-sync changes (master may have been scrolled since last sync)

    return aws;
}


