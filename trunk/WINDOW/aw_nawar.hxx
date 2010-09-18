// =============================================================== //
//                                                                 //
//   File      : aw_nawar.hxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_NAWAR_HXX
#define AW_NAWAR_HXX

#ifndef CB_H
#include <cb.h>
#endif
#ifndef _CPP_CSTDDEF
#include <cstddef>
#endif

class AW_root_callback {
    AW_RCB cb;
    AW_CL  cd1;
    AW_CL  cd2;
public:
    AW_root_callback(AW_RCB cb_, AW_CL cd1_, AW_CL cd2_) : cb(cb_), cd1(cd1_), cd2(cd2_) {}

    void call(AW_root *root) const { cb(root, cd1, cd2); }
    bool equals(const AW_root_callback& other) const {
        return cb == other.cb && cd1 == other.cd1 && cd2 == other.cd2;
    }
};

inline bool operator == (const AW_root_callback& cb1, const AW_root_callback& cb2) { return cb1.equals(cb2); }

class AW_root_cblist {
    AW_root_callback  callback;
    AW_root_cblist   *next;

    AW_root_cblist(AW_root_cblist *next_, const AW_root_callback& cb) : callback(cb), next(next_) {}
    ~AW_root_cblist() { delete next; }

    AW_root_cblist *unlink(const AW_root_callback& cb, AW_root_cblist*& found) {
        if (callback == cb) {
            AW_root_cblist *rest = next;
            
            found = this;
            next  = NULL;

            return rest;
        }

        if (next) next = next->unlink(cb, found);
        return this;
    }

    void call(AW_root *root) {          // runs the whole list in FIFO order
        if (next) next->call(root);
        callback.call(root);
    }
    
public:

    static void add(AW_root_cblist*& listhead, const AW_root_callback& cb) {
        remove(listhead, cb); // first remove duplicated callbacks
        listhead = new AW_root_cblist(listhead, cb);
    }
    static void remove(AW_root_cblist*& listhead, const AW_root_callback& cb) {
        AW_root_cblist *found = NULL;
        if (listhead) listhead = listhead->unlink(cb, found);
        delete found;
    }
    static void clear(AW_root_cblist*& listhead) {
        if (listhead) {
            delete listhead;
            listhead = NULL;
        }
    }
    static void call(AW_root_cblist*& listhead, AW_root *root) {
        if (listhead) listhead->call(root);
    }
};

struct AW_var_target {
    AW_var_target(void *pntr, AW_var_target *next);

    void          *pointer;
    AW_var_target *next;
};

void aw_update_awar_window_geometry(AW_root *awr);

#else
#error aw_nawar.hxx included twice
#endif // AW_NAWAR_HXX

