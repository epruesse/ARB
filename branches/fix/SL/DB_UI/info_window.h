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
#ifndef ARB_UNORDERED_MAP_H
#include <arb_unordered_map.h>
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

    AW_root *get_root() const { return aww->get_root(); }

    void update_window_title() const {
        if (is_detached()) { // only change window titles of detached windows
            char *title         = NULL;
            char *mapped_item = get_id_of_item_mapped_in(scanner);
            if (mapped_item) {
                title = GBS_global_string_copy("%s information (%s)", mapped_item, itemname());
                free(mapped_item);
            }
            else {
                title = GBS_global_string_copy("Press 'Update' to attach selected %s", itemname());
            }

            arb_assert(title);
            aww->set_window_title(title);
            free(title);
        }
    }

public:

    static const int MAIN_WINDOW = 0;
    typedef SmartPtr<InfoWindow> Ptr;
    typedef void (*detached_uppopper)(AW_window*, const InfoWindow*);

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
    int getDetachID() const { return detach_id; }

    GBDATA *get_gbmain() const { return get_db_scanner_main(scanner); }

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
    bool handlesSameItemtypeAs(const InfoWindow& other) const {
        QUERY_ITEM_TYPE type = itemtype();
        if (type == other.itemtype()) {
            return type != QUERY_ITEM_SPECIES || mapsOrganism() == other.mapsOrganism();
        }
        return false;
    }

    GBDATA *get_selected_item() const {
        GBDATA         *gb_main = get_gbmain();
        GB_transaction  ta(gb_main);
        return getSelector().get_selected_item(gb_main, get_root());
    }

    void map_item(GBDATA *gb_item) const {
        map_db_scanner(scanner, gb_item, getSelector().change_key_path);
    }
    void map_selected_item() const { map_item(get_selected_item()); }
    void attach_selected_item() const {
        map_selected_item();
        update_window_title();
    }
    void reuse() const {
        set_used(true);
        aww->activate();
        attach_selected_item();
    }

    void bind_to_selected_item() const;
    void add_detachOrGet_button(detached_uppopper popup_detached_cb) const;

    void display_selected_item() const;
    void detach_selected_item(detached_uppopper popup_detached_cb) const;
};

class InfoWindowRegistry {
    typedef arb_unordered_map<AW_window*, InfoWindow::Ptr> WinMap;
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
            if (info->handlesSameItemtypeAs(other)) {
                maxUsedID = std::max(maxUsedID, info->getDetachID());
            }
        }
        return maxUsedID+1;
    }
    const InfoWindow *find_reusable_of_same_type_as(const InfoWindow& other) {
        for (WinMap::iterator i = win.begin(); i != win.end(); ++i) {
            const InfoWindow::Ptr& info = i->second;
            if (info->handlesSameItemtypeAs(other) && !info->is_used()) return &*info;
        }
        return NULL;
    }

    static InfoWindowRegistry infowin;
};

#else
#error info_window.h included twice
#endif // INFO_WINDOW_H
