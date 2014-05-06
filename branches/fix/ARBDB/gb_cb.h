// =============================================================== //
//                                                                 //
//   File      : gb_cb.h                                           //
//   Purpose   : database callback types                           //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_CB_H
#define GB_CB_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif

class TypedDatabaseCallback {
    DatabaseCallback dbcb;
    GB_CB_TYPE       type;

    static DatabaseCallback MARKED_DELETED;

public:
    TypedDatabaseCallback(const DatabaseCallback& cb, GB_CB_TYPE type_)
        : dbcb(cb),
          type(type_)
    {}

    TypedDatabaseCallback with_type_changed_to(GB_CB_TYPE type_) const { return TypedDatabaseCallback(dbcb, type_); }

    GB_CB_TYPE get_type() const { return type; }

    void operator()(GBDATA *gbd, GB_CB_TYPE type_) const {
        GB_CB_TYPE effType = GB_CB_TYPE(type&type_);
        gb_assert(effType);
        gb_assert(!is_marked_for_removal());
        dbcb(gbd, effType);
    }

    bool sig_is_equal_to(const TypedDatabaseCallback& other) const { // ignores 'clientdata'
        return type == other.type && dbcb.same_function_as(other.dbcb);
    }
    bool is_equal_to(const TypedDatabaseCallback& other) const {
        return type == other.type && dbcb == other.dbcb;
    }

    void mark_for_removal() { dbcb = MARKED_DELETED; }
    bool is_marked_for_removal() const { return dbcb == MARKED_DELETED; }

    char *get_info() const;
};

template<typename CB>
struct CallbackList {
    typedef CB                                cbtype;
    typedef typename std::list<cbtype>        listtype; // (when you change container, see also arbdb.cxx@CBLISTNODE_TYPE)
    typedef typename listtype::iterator       itertype;
    typedef typename listtype::const_iterator const_itertype;

    listtype callbacks;

#if defined(ASSERTION_USED)
    bool contains_unremoved_callback(const cbtype& like) const;
#endif

    bool empty() const { return callbacks.empty(); }
    void add_unchecked(const CB& cb) { callbacks.push_back(cb); }
    void add(const CB& newcb) {
        gb_assert(!contains_unremoved_callback(newcb));
        add_unchecked(newcb);
    }

    const CB *get_tail() const { return empty() ? NULL : &callbacks.back(); }

};

struct gb_callback {
    // @@@ make members private
    TypedDatabaseCallback spec;
    short                 running; // only used in no-transaction mode

    explicit gb_callback(const TypedDatabaseCallback& spec_)
        : spec(spec_),
          running(0)
    {}
    gb_callback(const gb_callback& other)
        : spec(other.spec),
          running(other.running)
    {
        gb_assert(!running); // appears pathological - does it ever happen?
    }
    DECLARE_ASSIGNMENT_OPERATOR(gb_callback);

    bool may_be_removed() const { return !running && spec.is_marked_for_removal(); }

    bool call(GBDATA *with, GB_CB_TYPE typemask) {
        /*! call if matching. only done in NO_TRANSACTION_MODE
         * @param with database entry passed to callback
         * @param typemask call only if callback-type and typemask have common bits
         * @return true if callback has to be removed afterwards
         */
        {
            GB_CB_TYPE matchingType = GB_CB_TYPE(spec.get_type() & typemask);
            if (matchingType && !spec.is_marked_for_removal()) {
                ++running;
                spec(with, matchingType);
                --running;
            }
        }
        return may_be_removed();
    }
};

struct gb_callback_list : public CallbackList<gb_callback> {
    bool call(GBDATA *with, GB_CB_TYPE typemask) {
        /*! call all matching callbacks. only done in NO_TRANSACTION_MODE
         * @param with database entry passed to callback
         * @param typemask call only if callback-type and typemask have common bits
         * @return true if some callback in list has to be removed afterwards
         */

        bool need_del = false;
        for (itertype cb = callbacks.begin(); cb != callbacks.end(); ) {
            itertype next = cb; advance(next, 1);
            need_del = cb->call(with, typemask) || need_del;
            cb = next;
        }
        return need_del;
    }
};

struct gb_transaction_save;

struct gb_triggered_callback { // callbacks that will be called during commit
    // @@@ make members private
    TypedDatabaseCallback  spec;
    gb_transaction_save   *old;
    GBDATA                *gbd;

    gb_triggered_callback(GBDATA *gbd_, gb_transaction_save *old_, const TypedDatabaseCallback& spec_)
        : spec(spec_),
          old(old_),
          gbd(gbd_)
    {
        gb_add_ref_gb_transaction_save(old);
    }
    gb_triggered_callback(const gb_triggered_callback& other)
        : spec(other.spec),
          old(other.old),
          gbd(other.gbd)
    {
        gb_add_ref_gb_transaction_save(old);
    }
    DECLARE_ASSIGNMENT_OPERATOR(gb_triggered_callback);
    ~gb_triggered_callback() {
        gb_del_ref_gb_transaction_save(old);
    }
};

struct gb_pending_callbacks : public CallbackList<gb_triggered_callback> {
    bool pending() const { return !empty(); }
    void call_and_forget(GB_CB_TYPE allowedTypes);
};

#else
#error gb_cb.h included twice
#endif // GB_CB_H
