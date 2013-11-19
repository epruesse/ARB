// =============================================================== //
//                                                                 //
//   File      : gb_cb.h                                           //
//   Purpose   : gb_callback_list                                   //
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
        gb_assert(type&type_);
        gb_assert(!is_marked_for_removal());
        dbcb(gbd, type_);
    }
    void operator()(GBDATA *gbd) const { (*this)(gbd, type); }

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
};

struct gb_transaction_save;

struct gb_callback_list : virtual Noncopyable {
    // @@@ make members private
    gb_callback_list      *next;
    TypedDatabaseCallback  spec;
    gb_transaction_save   *old;
    GBDATA                *gbd;

    gb_callback_list(GBDATA *gbd_, gb_transaction_save *old_, const TypedDatabaseCallback& spec_)
        : next(NULL),
          spec(spec_),
          old(old_),
          gbd(gbd_)
    {
        gb_add_ref_gb_transaction_save(old);
    }
    ~gb_callback_list() {
        gb_assert(!next); // unlink before deleting
        gb_del_ref_gb_transaction_save(old);
    }
};

#else
#error gb_cb.h included twice
#endif // GB_CB_H

