// ============================================================ //
//                                                              //
//   File      : info_window.cxx                                //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2013   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "info_window.h"
#include <dbui.h>
#include <arb_str.h>
#include <aw_msg.hxx>

InfoWindowRegistry InfoWindowRegistry::infowin;

// ------------------------------------------------
//      item-independent info window callbacks

static void map_item_cb(AW_root *, const InfoWindow *infoWin) {
    infoWin->map_selected_item();
}

static void store_unused_detached_info_window_cb(AW_window *aw_detached) {
    const InfoWindow *infoWin = InfoWindowRegistry::infowin.find(aw_detached);
    arb_assert(infoWin); // forgot to registerInfoWindow?
    if (infoWin) {
        arb_assert(infoWin->is_used());
        infoWin->set_used(false);
        infoWin->map_item(NULL);
    }
}

void DBUI::init_info_window(AW_root *aw_root, AW_window_simple_menu *aws, const ItemSelector& itemType, int detach_id) {
    char *window_id;
    char *window_title;
    {
        const char *itemname = itemType.item_name;
        char       *ITEMNAME = ARB_strupper(strdup(itemname));

        if (detach_id == InfoWindow::MAIN_WINDOW) {
            char *Itemname = strdup(itemname); Itemname[0] = ITEMNAME[0];
            window_id      = GBS_global_string_copy("%s_INFORMATION", ITEMNAME);
            window_title   = GBS_global_string_copy("%s information", Itemname);
            free(Itemname);
        }
        else {
            window_id    = GBS_global_string_copy("%s_INFODETACH_%i", ITEMNAME, detach_id);
            window_title = strdup("<detaching>");
        }
        free(ITEMNAME);
    }

    aws->init(aw_root, window_id, window_title);

    free(window_title);
    free(window_id);

    if (!detach_id == InfoWindow::MAIN_WINDOW) {
        aws->on_hide(store_unused_detached_info_window_cb);
    }
}

void InfoWindow::bind_to_selected_item() const {
    arb_assert(is_maininfo());
    getSelector().add_selection_changed_cb(get_root(), makeRootCallback(map_item_cb, this));
}

void InfoWindow::display_selected_item() const {
    arb_assert(is_detached());
    attach_selected_item();
    if (!get_selected_item()) {
        aw_message(GBS_global_string("Please select a %s, afterwards retry to attach it here", itemname()));
    }
}

void InfoWindow::detach_selected_item(detached_uppopper popup_detached_cb) const {
    if (get_selected_item()) {
        popup_detached_cb(aww, this);
    }
    else {
        aw_message(GBS_global_string("Please select a %s, afterwards retry to detach it", itemname()));
    }
}

static void detach_selected_item_cb(AW_window *, InfoWindow::detached_uppopper popup_detached_cb, const InfoWindow *infoWin) {
    infoWin->detach_selected_item(popup_detached_cb);
}
static void display_selected_item_cb(AW_window *, const InfoWindow *infoWin) {
    infoWin->display_selected_item();
}

void InfoWindow::add_detachOrGet_button(detached_uppopper popup_detached_cb) const {
    if (is_maininfo()) {
        bind_to_selected_item();

        aww->callback(makeWindowCallback(detach_selected_item_cb, popup_detached_cb, this));
        aww->create_autosize_button("DETACH", "New window...", "S");
    }
    else {
        aww->callback(makeWindowCallback(display_selected_item_cb, this));
        aww->create_autosize_button("GET", "Update", "U");
    }
}
