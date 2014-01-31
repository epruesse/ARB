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

struct gb_callback : virtual Noncopyable {
    // @@@ make members private
    gb_callback           *next;
    TypedDatabaseCallback  spec;

    short priority;
    short running; // only used in no-transaction mode

    gb_callback(const TypedDatabaseCallback& spec_, short priority_)
        : next(NULL),
          spec(spec_),
          priority(priority_),
          running(0)
    {}

    ~gb_callback() {
        gb_assert(!next); // unlink before deleting
    }

    bool may_be_removed() const { return !running && spec.is_marked_for_removal(); }

    bool call(GBDATA *with, GB_CB_TYPE typemask) {
        /*! call all matching callbacks in chain. only done in NO_TRANSACTION_MODE
         * @param with database entry passed to callback
         * @param typemask call only if callback-type and typemask have common bits
         * @return true if some callback in chain has to be removed afterwards
         */
        {
            GB_CB_TYPE matchingType = GB_CB_TYPE(spec.get_type() & typemask);
            if (matchingType && !spec.is_marked_for_removal()) {
                ++running;
#if defined(ASSERTION_USED)
                gb_callback *oldnext = next;
#endif
                spec(with, matchingType);
                gb_assert(oldnext == next);
                --running;
            }
        }
        return (next && next->call(with, typemask)) || may_be_removed();
    }
};

struct gb_transaction_save;

struct gb_triggered_callback : virtual Noncopyable {
    // @@@ make members private
    gb_triggered_callback *next;
    TypedDatabaseCallback  spec;
    gb_transaction_save   *old;
    GBDATA                *gbd;

    gb_triggered_callback(GBDATA *gbd_, gb_transaction_save *old_, const TypedDatabaseCallback& spec_)
        : next(NULL),
          spec(spec_),
          old(old_),
          gbd(gbd_)
    {
        gb_add_ref_gb_transaction_save(old);
    }
    ~gb_triggered_callback() {
        gb_assert(!next); // unlink before deleting
        gb_del_ref_gb_transaction_save(old);
    }
};

class gb_pending_callbacks {
    gb_triggered_callback *head, *tail;
public:
    gb_pending_callbacks() : head(NULL), tail(NULL) {}

    void append(gb_triggered_callback *cbl) { tail = (head ? tail->next : head) = cbl; }

    bool pending() const { return head; }
    const gb_triggered_callback *get_tail() const { return tail; }

    void call_and_forget(GB_CB_TYPE allowedTypes);
};

#else
#error gb_cb.h included twice
#endif // GB_CB_H


