// ============================================================ //
//                                                              //
//   File      : rootAsWin.h                                    //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2013   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ROOTASWIN_H
#define ROOTASWIN_H

#ifndef CB_H
#include <cb.h>
#endif

class RootAsWindowCallback {
    // Allows to use any RootCallback where a WindowCallback is expected.

    RootCallback rcb;
    void call(AW_window *aww) const { rcb(aww->get_root()); }
    RootAsWindowCallback(const RootCallback& rcb_) : rcb(rcb_) {}

    static void window_cb(AW_window *aww, const RootAsWindowCallback *rawcb) { rawcb->call(aww); }
    static void delete_raw_cb(RootAsWindowCallback *raw_cb) { delete raw_cb; }

    template<typename T>
    static void call_root_as_win(AW_window *aww, void (*root_cb)(AW_root*, T), T t) { root_cb(aww->get_root(), t); }

    static void call_root_as_win(AW_window *aww, void (*root_cb)(AW_root*)) { root_cb(aww->get_root()); }

public:

    static WindowCallback reuse(const RootCallback& rcb) {
        // It's not recommended to use this widely, mainly this is a helper for refactoring.
        // Normally you should better adapt your callback-type.
        //
        // Use it in cases where a user-provided callback has to be used
        // both as RootCallback and as WindowCallback.

        return makeWindowCallback(window_cb, delete_raw_cb, new RootAsWindowCallback(rcb));
    }

    template<typename T>
    static WindowCallback simple(void (*root_cb)(AW_root*, T), T t) {
        // usable for simple root callbacks with only one argument
        // (automatically creates one general wrapper function for each type T)
        return makeWindowCallback(call_root_as_win, root_cb, t);
    }

    static WindowCallback simple(void (*root_cb)(AW_root*)) {
        // usable for simple root callbacks with no argument
        return makeWindowCallback(call_root_as_win, root_cb);
    }
};

#else
#error rootAsWin.h included twice
#endif // ROOTASWIN_H
