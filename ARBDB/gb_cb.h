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

class gb_cb_spec {
    GB_CB       func;
    GB_CB_TYPE  type;
    int        *clientdata;

public:
    gb_cb_spec(GB_CB func_, GB_CB_TYPE type_, int *clientdata_)
        : func(func_),
          type(type_),
          clientdata(clientdata_)
    {}

    gb_cb_spec with_type_changed_to(GB_CB_TYPE type_) const { return gb_cb_spec(func, type_, clientdata); }

    GB_CB_TYPE get_type() const { return type; }

    void operator()(GBDATA *gbd, GB_CB_TYPE type_) const {
        gb_assert(type&type_);
        gb_assert(!is_marked_for_removal());
        func(gbd, clientdata, type_);
    }
    void operator()(GBDATA *gbd) const { (*this)(gbd, type); }

    bool sig_is_equal_to(const gb_cb_spec& other) const { // ignores 'clientdata'
        return func == other.func && type == other.type;
    }
    bool is_equal_to(const gb_cb_spec& other) const {
        return sig_is_equal_to(other) && clientdata == other.clientdata;
    }

    void mark_for_removal() { func = NULL; }
    bool is_marked_for_removal() const { return func == NULL; }

    char *get_info() const;
};

struct gb_callback {
    gb_callback *next;
    gb_cb_spec   spec;
    short        priority;
    short        running; // @@@ only used in no-transaction mode
};

struct gb_transaction_save;

struct gb_callback_list {
    gb_callback_list    *next;
    gb_cb_spec           spec;
    gb_transaction_save *old;
    GBDATA              *gbd;
};

#else
#error gb_cb.h included twice
#endif // GB_CB_H

