// =============================================================== //
//                                                                 //
//   File      : ScrollSynchronizer.h                              //
//   Purpose   : synchronize TREE_canvas scrolling                 //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2016   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SCROLLSYNCHRONIZER_H
#define SCROLLSYNCHRONIZER_H

#ifndef NT_LOCAL_H
#include "NT_local.h"
#endif
#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef _GLIBCXX_SET
#include <set>
#endif

#define NO_SCROLL_SYNC (-1)

inline bool valid_canvas_index(int idx) { return idx>=0 && idx<MAX_NT_WINDOWS; }

class timestamp {
    unsigned t;
public:
    explicit timestamp(unsigned i) : t(i) {}
    operator unsigned() const { return t; }

    static timestamp before(const timestamp& t) { return timestamp(t-1); }
    static timestamp after(const timestamp& t) { return timestamp(t+1); }

    void setNewerThan(const timestamp& other) { *this = after(other); }
    void setOlderThan(const timestamp& other) { *this = before(other); }

    bool newer_than(const timestamp& other) const { return t>other; }
    bool older_than(const timestamp& other) const { return t<other; }

};

typedef std::set< RefPtr<GBDATA> > SpeciesSet;

typedef SmartPtr<SpeciesSet> SpeciesSetPtr;

inline bool operator == (const SpeciesSetPtr& spec1, const SpeciesSetPtr& spec2) {
    if (spec1.isNull()) return spec2.isNull() ? true : false;
    if (spec2.isNull()) return false;
    if (spec1.sameObject(spec2)) return true;
    return *spec1 == *spec2;
}
inline bool operator != (const SpeciesSetPtr& spec1, const SpeciesSetPtr& spec2) {
    return !(spec1 == spec2);
}

class CanvasRef {
    int                  index;
    mutable TREE_canvas *canvas;
public:
    CanvasRef() :
        index(-1),
        canvas(NULL)
    {}

    void define_canvas_index(int i) {
        nt_assert(index == -1);
        nt_assert(valid_canvas_index(i));

        index = i;
    }

    TREE_canvas *get_canvas() const {
        if (!canvas) {
            nt_assert(valid_canvas_index(index));
            canvas = NT_get_canvas_by_index(index);
            nt_assert(canvas);
        }
        return canvas;
    }
};

class MasterCanvas : public CanvasRef {
    timestamp last_Refresh;      // passive (last refresh of canvas)
    timestamp last_DisplayTrack; // last tracking of displayed species (=last update species-set)
    timestamp last_SetChange;    // last CHANGE of species-set

    SpeciesSetPtr species;

    SpeciesSetPtr track_displayed_species();

    void update_SpeciesSet() {
        if (last_Refresh.newer_than(last_SetChange)) {

            if (last_Refresh.newer_than(last_DisplayTrack)) {
                SpeciesSetPtr current_species = track_displayed_species();
                last_DisplayTrack             = last_Refresh;
#if defined(DEBUG)
                fprintf(stderr, "DEBUG: MasterCanvas tracking species (this=%p, last_DisplayTrack=%u, species count=%zu)\n", this, unsigned(last_DisplayTrack), current_species->size());
#endif

                if (species != current_species) { // set of species changed?
                    species        = current_species;
                    last_SetChange = last_DisplayTrack;
#if defined(DEBUG)
                    fprintf(stderr, "DEBUG: MasterCanvas::SpeciesSet changed/updated (this=%p, last_SetChange=%u, species count=%zu)\n", this, unsigned(last_SetChange), species->size());
#endif
                }
            }
        }
    }

public:
    MasterCanvas() :
        last_Refresh(8),
        last_DisplayTrack(7),
        last_SetChange(6)
    {}

    void announce_update() { last_Refresh.setNewerThan(last_DisplayTrack); }

    const timestamp& get_updated_SpeciesSet(SpeciesSetPtr& specset) {
        update_SpeciesSet();
        specset = species;
        return last_SetChange;
    }
};

class SlaveCanvas : public CanvasRef {
    RefPtr<MasterCanvas> last_master;
    timestamp            last_master_change;

    bool need_SetUpdate;
    bool need_PositionTrack;
    bool need_ScrollZoom;
    bool need_Refresh;

    SpeciesSetPtr species;

    void update_set_from_master(MasterCanvas& master) {
        bool update_only_if_master_changed = false;

        if (!need_SetUpdate) {
            if (!last_master) need_SetUpdate                = true;
            else if (last_master != &master) need_SetUpdate = true;
            else update_only_if_master_changed = true;
        }

        if (need_SetUpdate || update_only_if_master_changed) {
            SpeciesSetPtr master_spec;
            timestamp     master_stamp = master.get_updated_SpeciesSet(master_spec);

            if (update_only_if_master_changed && !need_SetUpdate) {
                need_SetUpdate = master_stamp.newer_than(last_master_change);
            }
            if (need_SetUpdate) {
                last_master        = &master;
                last_master_change = master_stamp;

                if (master_spec != species) {
                    species            = master_spec;
                    need_PositionTrack = true;

#if defined(DEBUG)
                    fprintf(stderr, "DEBUG: updating SlaveCanvas::SpeciesSet (this=%p, species count=%zu)\n", this, species->size());
#endif
                }
                need_SetUpdate = false;
            }
        }
    }

    void update_tracked_positions() {
        nt_assert(!need_SetUpdate);
        if (need_PositionTrack) {
#if defined(DEBUG)
            fprintf(stderr, "DEBUG: SlaveCanvas tracks positions (this=%p)\n", this);
#endif
            need_ScrollZoom    = true; // @@@ fake (only set of positions changed? they may always change)
            need_PositionTrack = false;
        }
    }
    void update_scroll_zoom() {
        nt_assert(!need_PositionTrack);
        if (need_ScrollZoom) {
#if defined(DEBUG)
            fprintf(stderr, "DEBUG: SlaveCanvas updates scroll/zoom (this=%p)\n", this);
#endif
            need_Refresh    = true; // @@@ fake (only do if scroll/zoom did change)
            need_ScrollZoom = false;
        }
    }

public:
    SlaveCanvas() :
        last_master(NULL),
        last_master_change(0),
        need_SetUpdate(true),
        need_PositionTrack(true),
        need_ScrollZoom(true),
        need_Refresh(true)
    {}

    void request_Refresh() {
        need_Refresh = true;
    }
    void request_SetUpdate() {
        need_SetUpdate = true;
    }

    void refresh_if_needed(MasterCanvas& master) {
        update_set_from_master(master);
        update_tracked_positions();
        update_scroll_zoom();

        nt_assert(!need_ScrollZoom);
        if (need_Refresh) {
#if defined(DEBUG)
            fprintf(stderr, "DEBUG: SlaveCanvas does refresh (this=%p)\n", this);
#endif
            need_Refresh = false;
        }
    }
};



class ScrollSynchronizer {
    MasterCanvas source[MAX_NT_WINDOWS];
    SlaveCanvas  dest[MAX_NT_WINDOWS];

    int  master_index[MAX_NT_WINDOWS];       // index = slave-canvas
    bool autosynced[MAX_NT_WINDOWS];         // true if canvas is autosynced

    bool autosynced_with(int canvas, int with) const {
        nt_assert(valid_canvas_index(canvas));
        nt_assert(valid_canvas_index(with));

        if (autosynced[canvas]) {
            int master = master_index[canvas];
            if (master == with) return true;
            if (valid_canvas_index(master)) return autosynced_with(master, with);
        }
        return false;
    }

    int autosync_master(int slave) const {
        return autosynced[slave] ? master_index[slave] : NO_SCROLL_SYNC;
    }

public:
    ScrollSynchronizer() {
        for (int i = 0; i<MAX_NT_WINDOWS; ++i) {
            source[i].define_canvas_index(i);
            dest[i].define_canvas_index(i);
        }
    }

    GB_ERROR define_dependency(int slave_idx, int master_idx, bool auto_sync) {
        /*! defines master/slave dependency between TREE_canvas'es
         * @param slave_idx index of slave canvas [0..MAX_NT_WINDOWS-1]
         * @param master_idx index of master canvas (or NO_SCROLL_SYNC)
         * @param auto_sync true -> automatically synchronize scrolling
         * @return error (only if auto_sync is impossible because of sync-loop => auto_sync is ignored!)
         */

        nt_assert(valid_canvas_index(slave_idx));
        nt_assert(valid_canvas_index(master_idx) || master_idx == NO_SCROLL_SYNC);

        master_index[slave_idx] = master_idx;
        autosynced[slave_idx]   = false;

        GB_ERROR error = NULL;
        if (auto_sync && master_idx != NO_SCROLL_SYNC) {
            if (autosynced_with(master_idx, slave_idx)) {
                error = "dependency loop detected";
            }
            else {
                autosynced[slave_idx] = true;
            }
        }
        return error;
    }

    void announce_update(int canvas_idx) {
        nt_assert(valid_canvas_index(canvas_idx));
        source[canvas_idx].announce_update();
#if defined(DEBUG)
        fprintf(stderr, "DEBUG: announce_update(canvas_idx=%i)\n", canvas_idx);
#endif
    }

    // @@@ add announce_resize (master AND slave!)
    // @@@ add announce_tree_modified (master AND slave!); shall also handle change to other tree
    // @@@ add announce_tree_type_changed (master AND slave!)

    GB_ERROR update_explicit(int slave_idx) {
        /*! perform an unconditional update
         * (ie. refresh of slave is forced even if internal data was up-to-date)
         * @param slave_idx index of canvas to update
         * @return error (eg. if no master defined)
         */

        int master_idx = master_index[slave_idx];

        GB_ERROR error = NULL;
        if (valid_canvas_index(master_idx)) {
#if defined(DEBUG)
            fputs("------------------------------\n", stderr);
            fprintf(stderr, "DEBUG: update_explicit(slave_idx=%i) from master_idx=%i\n", slave_idx, master_idx);
#endif
            MasterCanvas& master = source[master_idx];
            SlaveCanvas&  slave  = dest[slave_idx];

            slave.request_Refresh();
            slave.refresh_if_needed(master);
        }
        else {
            error = "No master-window defined";
        }
        return error;
    }

    void update_implicit(int slave_idx) {
        /*! perform an implicit update
         *
         * @param slave_idx index of canvas to update
         * @return error (eg. if no master defined)
         */

        int master_idx = master_index[slave_idx];
        if (valid_canvas_index(master_idx)) {
#if defined(DEBUG)
            fputs("------------------------------\n", stderr);
            fprintf(stderr, "DEBUG: update_implicit(slave_idx=%i) from master_idx=%i\n", slave_idx, master_idx);
#endif
            MasterCanvas& master = source[master_idx];
            SlaveCanvas&  slave  = dest[slave_idx];

            slave.request_SetUpdate();
            slave.refresh_if_needed(master);
        }
    }

    void auto_update() {
        /*! update all auto-synchronized canvases */

#if defined(DEBUG)
        fputs("------------------------------\n"
              "DEBUG: auto_update\n", stderr);
#endif

        bool check_update[MAX_NT_WINDOWS];
        for (int i = 0; i<MAX_NT_WINDOWS; ++i) {
            check_update[i] = autosynced[i];
        }

        bool need_check = true;
        while (need_check) {
            need_check = false;

            for (int slave_idx = 0; slave_idx<MAX_NT_WINDOWS; ++slave_idx) {
                if (check_update[slave_idx]) {
                    int master_idx = autosync_master(slave_idx);
                    if (valid_canvas_index(master_idx)) {
                        if (check_update[master_idx]) {
                            // delay (first update master)
                            need_check = true; // need another loop
                        }
                        else {
#if defined(DEBUG)
                            fprintf(stderr, "DEBUG: auto_update(slave_idx=%i) from master_idx=%i\n", slave_idx, master_idx);
#endif
                            MasterCanvas& master = source[master_idx];
                            SlaveCanvas&  slave  = dest[slave_idx];

                            slave.refresh_if_needed(master);

                            check_update[slave_idx] = false;
                        }
                    }
                }
            }
        }
    }
};


#else
#error ScrollSynchronizer.h included twice
#endif // SCROLLSYNCHRONIZER_H
