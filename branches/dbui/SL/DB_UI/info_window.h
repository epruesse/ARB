// ============================================================ //
//                                                              //
//   File      : info_window.h                                  //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2013   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef DB_SCANNER_HXX
#include <db_scanner.hxx>
#endif
#ifndef ARB_MSG_H
#include <arb_msg.h>
#endif
#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif

// --------------------
//      InfoWindow

class InfoWindow : virtual Noncopyable {
    AW_window *aww;
    DbScanner *scanner;
    int        detach_id;

    mutable bool used; // window in use (i.e. not yet popped down)

    const ItemSelector& getSelector() const { return get_itemSelector_of(scanner); }
    QUERY_ITEM_TYPE itemtype() const { return getSelector().type; }
    const char *itemname() const { return getSelector().item_name; }

    void update_window_title() const {
        if (is_detached()) { // only change window titles of detached windows
            char *title         = NULL;
            char *mapped_item = get_id_of_item_mapped_in(scanner);
            if (mapped_item) {
                title = GBS_global_string_copy("%s information (%s)", mapped_item, itemname());
                free(mapped_item);
            }
            else {
                title = GBS_global_string_copy("Press GET to attach selected %s", itemname());
            }

            arb_assert(title);
            aww->set_window_title(title);
            free(title);
        }
    }

public:

    static const int MAIN_WINDOW = 0;
    typedef SmartPtr<InfoWindow> Ptr;

    InfoWindow(AW_window *aww_, DbScanner *scanner_, int detach_id_)
        : aww(aww_),
          scanner(scanner_),
          detach_id(detach_id_),
          used(true)
    {}
    ~InfoWindow() {
        // @@@ should free 'scanner'
    }

    bool is_detached() const { return detach_id>MAIN_WINDOW; }
    bool is_maininfo() const { return detach_id == MAIN_WINDOW; }

    GBDATA *get_gbmain() const { return get_db_scanner_main(scanner); }
    AW_window *get_aww() const { return aww; }
    AW_root *get_root() const { return get_aww()->get_root(); }

    bool is_used() const { return used; }
    void set_used(bool used_) const {
        arb_assert(!is_maininfo()); // cannot change for maininfo window (it is never reused!)
        used = used_;
    }

    bool mapsOrganism() const {
        const ItemSelector& sel = getSelector();

        return
            (sel.type == QUERY_ITEM_SPECIES) &&
            (strcmp(sel.item_name, "organism") == 0);
    }

    int getDetachID() const { return detach_id; }
    bool mapsSameItemsAs(const InfoWindow& other) const {
        QUERY_ITEM_TYPE type = itemtype();
        if (type == other.itemtype()) {
            return type != QUERY_ITEM_SPECIES || mapsOrganism() == other.mapsOrganism();
        }
        return false;
    }

    void map_current_item() const {
        GBDATA         *gb_main = get_gbmain();
        GB_transaction  ta(gb_main);
        ItemSelector&   itemT   = getSelector();
        GBDATA         *gb_item = itemT.get_selected_item(gb_main, get_root());

        if (gb_item) {
            map_db_scanner(scanner, gb_item, itemT.change_key_path);
        }
    }

    void attach_currently_selected_item() const {
        map_current_item();
        update_window_title();
    }

    void reuse() const {
        set_used(true);
        attach_currently_selected_item();
        get_aww()->activate();
    }

    void bind_to_selected_item() const;
};

class InfoWindowRegistry {
    typedef std::map<AW_window*, InfoWindow::Ptr> WinMap;
    WinMap win;

    InfoWindowRegistry(){} // InfoWindowRegistry is a singleton and always exists

public:
    const InfoWindow& registerInfoWindow(AW_window *aww, DbScanner *scanner, int detach_id) {
        // registers a new instance of an InfoWindow
        // returned reference is persistent
        arb_assert(aww && scanner);
        arb_assert(detach_id >= InfoWindow::MAIN_WINDOW);

        arb_assert(!find(aww));

        win[aww] = new InfoWindow(aww, scanner, detach_id);
        return *win[aww];
    }

    const InfoWindow* find(AW_window *aww) {
        // returns InfoWindow for 'aww' (or NULL if no such InfoWindow)
        WinMap::iterator found = win.find(aww);
        return found == win.end() ? NULL : &*found->second;
    }

    int allocate_detach_id(const InfoWindow& other) {
        arb_assert(other.is_maininfo());

        int maxUsedID = InfoWindow::MAIN_WINDOW;
        for (WinMap::iterator i = win.begin(); i != win.end(); ++i) {
            const InfoWindow::Ptr& info = i->second;
            if (info->mapsSameItemsAs(other)) {
                maxUsedID = std::max(maxUsedID, info->getDetachID());
            }
        }
        return maxUsedID+1;
    }
    const InfoWindow *find_reusable_of_same_type_as(const InfoWindow& other) {
        for (WinMap::iterator i = win.begin(); i != win.end(); ++i) {
            const InfoWindow::Ptr& info = i->second;
            if (info->mapsSameItemsAs(other) && !info->is_used()) return &*info;
        }
        return NULL;
    }

    static InfoWindowRegistry infowin;
};

// callbacks needed while creating item-infowindows:
void store_unused_detached_info_window_cb(AW_window *aw_detached);
void sync_detached_window_cb(AW_window *, const InfoWindow *infoWin);

#else
#error info_window.h included twice
#endif // INFO_WINDOW_H
