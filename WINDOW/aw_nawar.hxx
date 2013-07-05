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

#ifndef _GLIBCXX_CSTDDEF
#include <cstddef>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class AW_root_cblist : virtual Noncopyable {
    RootCallback    callback;
    AW_root_cblist *next;

    AW_root_cblist(AW_root_cblist *next_, const RootCallback& cb) : callback(cb), next(next_) {}
    ~AW_root_cblist() { delete next; }

    AW_root_cblist *unlink(const RootCallback& cb, AW_root_cblist*& found) {
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
        callback(root);
    }
    
public:

    static void add(AW_root_cblist*& listhead, const RootCallback& cb) {
        remove(listhead, cb); // first remove duplicated callbacks
        listhead = new AW_root_cblist(listhead, cb);
    }
    static void remove(AW_root_cblist*& listhead, const RootCallback& cb) {
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

    bool contains(const RootCallback& cb) const {
        return (cb == callback) || (next && next->contains(cb));
    }
    static bool contains(const AW_root_cblist*& listhead, const RootCallback& cb) {
        return listhead && listhead->contains(cb);
    }
};

#else
#error aw_nawar.hxx included twice
#endif // AW_NAWAR_HXX

