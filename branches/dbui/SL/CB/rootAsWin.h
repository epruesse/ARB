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

class RootAsWindowCallback {
    // Allows to use any RootCallback where a WindowCallback is expected.
    //
    // It's not recommended to use this widely, mainly this is a helper for refactoring.
    // Normally you should better adapt your callback-type.
    //
    // Use it in cases where a user-provided callback has to be used
    // both as RootCallback and as WindowCallback.

    RootCallback rcb;
    void call(AW_window *aww) const { rcb(aww->get_root()); }
    RootAsWindowCallback(const RootCallback& rcb_) : rcb(rcb_) {}

    static void window_cb(AW_window *aww, const RootAsWindowCallback *rawcb) { rawcb->call(aww); }
    static void delete_raw_cb(RootAsWindowCallback *raw_cb) { delete raw_cb; }

public:

    static WindowCallback reuse(const RootCallback& rcb) {
        return makeWindowCallback(window_cb, delete_raw_cb, new RootAsWindowCallback(rcb));
    }
};

#else
#error rootAsWin.h included twice
#endif // ROOTASWIN_H
