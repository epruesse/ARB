// ================================================================= //
//                                                                   //
//   File      : ad_cb.cxx                                           //
//   Purpose   : callbacks on DB entries                             //
//                                                                   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "ad_cb.h"
#include "ad_hcb.h"
#include "gb_compress.h"
#include "gb_ta.h"
#include "gb_ts.h"

#include <arb_strarray.h>
#include <arb_strbuf.h>

static gb_triggered_callback *currently_called_back = NULL; // points to callback during callback; NULL otherwise
static GB_MAIN_TYPE          *inside_callback_main  = NULL; // points to DB root during callback; NULL otherwise

gb_hierarchy_location::gb_hierarchy_location(GBDATA *gb_main, const char *db_path) {
    invalidate();
    if (db_path) {
        GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
        GB_test_transaction(Main);

        ConstStrArray keys;
        GBT_split_string(keys, db_path, '/');

        size_t size = keys.size();
        if (size>1 && !keys[0][0]) { // db_path starts with '/' and contains sth
            size_t q = 0;
            for (size_t offset = size-1; offset>0; --offset, ++q) {
                if (!keys[offset][0]) { // empty key
                    invalidate();
                    return;
                }
                quark[q] = gb_find_or_create_quark(Main, keys[offset]);
                if (quark[q]<1) { // unknown/invalid key
                    invalidate();
                    return;
                }
            }
            quark[size-1] = 0;
            gb_assert(is_valid());
        }
    }
}

char *gb_hierarchy_location::get_db_path(GBDATA *gb_main) const {
    GBS_strstruct  out(MAX_HIERARCHY_DEPTH*20);
    GB_MAIN_TYPE  *Main = GB_MAIN(gb_main);

    int offset = 0;
    while (quark[offset]) ++offset;
    while (offset-->0) {
        out.put('/');
        out.cat(quark2key(Main, quark[offset]));
    }
    return out.release();
}

void gb_pending_callbacks::call_and_forget(GB_CB_TYPE allowedTypes) {
#if defined(ASSERTION_USED)
    const gb_triggered_callback *tail = get_tail();
#endif

    for (itertype cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        currently_called_back = &*cb;
        gb_assert(currently_called_back);
        currently_called_back->spec(cb->gbd, allowedTypes);
        currently_called_back = NULL;
    }

    gb_assert(tail == get_tail());

    callbacks.clear();
}

void GB_MAIN_TYPE::call_pending_callbacks() {
    inside_callback_main = this;

    deleteCBs.pending.call_and_forget(GB_CB_DELETE);         // first all delete callbacks:
    changeCBs.pending.call_and_forget(GB_CB_ALL_BUT_DELETE); // then all change callbacks:

    inside_callback_main = NULL;
}

inline void GB_MAIN_TYPE::callback_group::forget_hcbs() {
    delete hierarchy_cbs;
    hierarchy_cbs = NULL;
}

void GB_MAIN_TYPE::forget_hierarchy_cbs() {
    changeCBs.forget_hcbs();
    deleteCBs.forget_hcbs();
}

static void dummy_db_cb(GBDATA*, GB_CB_TYPE) { gb_assert(0); } // used as marker for deleted callbacks
DatabaseCallback TypedDatabaseCallback::MARKED_DELETED = makeDatabaseCallback(dummy_db_cb);

GB_MAIN_TYPE *gb_get_main_during_cb() {
    /* if inside a callback, return the DB root of the DB element, the callback was called for.
     * if not inside a callback, return NULL.
     */
    return inside_callback_main;
}

NOT4PERL bool GB_inside_callback(GBDATA *of_gbd, GB_CB_TYPE cbtype) {
    GB_MAIN_TYPE *Main   = gb_get_main_during_cb();
    bool          inside = false;

    if (Main) {                 // inside a callback
        gb_assert(currently_called_back);
        if (currently_called_back->gbd == of_gbd) {
            GB_CB_TYPE curr_cbtype;
            if (Main->has_pending_delete_callback()) { // delete callbacks were not all performed yet
                                                       // => current callback is a delete callback
                curr_cbtype = GB_CB_TYPE(currently_called_back->spec.get_type() & GB_CB_DELETE);
            }
            else {
                gb_assert(Main->has_pending_change_callback());
                curr_cbtype = GB_CB_TYPE(currently_called_back->spec.get_type() & (GB_CB_ALL-GB_CB_DELETE));
            }
            gb_assert(curr_cbtype != GB_CB_NONE); // wtf!? are we inside callback or not?

            if ((cbtype&curr_cbtype) != GB_CB_NONE) {
                inside = true;
            }
        }
    }

    return inside;
}

GBDATA *GB_get_gb_main_during_cb() {
    GBDATA       *gb_main = NULL;
    GB_MAIN_TYPE *Main    = gb_get_main_during_cb();

    if (Main) {                 // inside callback
        if (!GB_inside_callback(Main->gb_main(), GB_CB_DELETE)) { // main is not deleted
            gb_main = Main->gb_main();
        }
    }
    return gb_main;
}

static GB_CSTR gb_read_pntr_ts(GBDATA *gbd, gb_transaction_save *ts) {
    int         type = GB_TYPE_TS(ts);
    const char *data = GB_GETDATA_TS(ts);
    if (data) {
        if (ts->flags.compressed_data) {    // uncompressed data return pntr to database entry
            long size = GB_GETSIZE_TS(ts) * gb_convert_type_2_sizeof[type] + gb_convert_type_2_appendix_size[type];
            data = gb_uncompress_data(gbd, data, size);
        }
    }
    return data;
}

NOT4PERL const void *GB_read_old_value() {
    // get last array value in callbacks
    char *data;

    if (!currently_called_back) {
        GB_export_error("You cannot call GB_read_old_value outside a ARBDB callback");
        return NULL;
    }
    if (!currently_called_back->old) {
        GB_export_error("No old value available in GB_read_old_value");
        return NULL;
    }
    data = GB_GETDATA_TS(currently_called_back->old);
    if (!data) return NULL;

    return gb_read_pntr_ts(currently_called_back->gbd, currently_called_back->old);
}
long GB_read_old_size() {
    // same as GB_read_old_value for size
    if (!currently_called_back) {
        GB_export_error("You cannot call GB_read_old_size outside a ARBDB callback");
        return -1;
    }
    if (!currently_called_back->old) {
        GB_export_error("No old value available in GB_read_old_size");
        return -1;
    }
    return GB_GETSIZE_TS(currently_called_back->old);
}

inline char *cbtype2readable(GB_CB_TYPE type) {
    ConstStrArray septype;

#define appendcbtype(cbt) do {                  \
        if (type&cbt) {                         \
            type = GB_CB_TYPE(type-cbt);        \
            septype.put(#cbt);                  \
        }                                       \
    } while(0)

    appendcbtype(GB_CB_DELETE);
    appendcbtype(GB_CB_CHANGED);
    appendcbtype(GB_CB_SON_CREATED);

    gb_assert(type == GB_CB_NONE);

    return GBT_join_strings(septype, '|');
}

char *TypedDatabaseCallback::get_info() const {
    const char *readable_fun    = GBS_funptr2readable((void*)dbcb.callee(), true);
    char       *readable_cbtype = cbtype2readable((GB_CB_TYPE)dbcb.inspect_CD2());
    char       *result          = GBS_global_string_copy("func='%s' type=%s clientdata=%p",
                                                         readable_fun, readable_cbtype, (void*)dbcb.inspect_CD1());

    free(readable_cbtype);

    return result;
}

char *GB_get_callback_info(GBDATA *gbd) {
    // returns human-readable information about callbacks of 'gbd' or 0
    char *result = 0;
    if (gbd->ext) {
        gb_callback_list *cbl = gbd->get_callbacks();
        if (cbl) {
            for (gb_callback_list::itertype cb = cbl->callbacks.begin(); cb != cbl->callbacks.end(); ++cb) {
                char *cb_info = cb->spec.get_info();
                if (result) {
                    char *new_result = GBS_global_string_copy("%s\n%s", result, cb_info);
                    free(result);
                    free(cb_info);
                    result = new_result;
                }
                else {
                    result = cb_info;
                }
            }
        }
    }

    return result;
}

#if defined(ASSERTION_USED)
template<typename CB>
bool CallbackList<CB>::contains_unremoved_callback(const CB& like) const {
    for (const_itertype cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (cb->spec.is_equal_to(like.spec) &&
            !cb->spec.is_marked_for_removal())
        {
            return true;
        }
    }
    return false;
}
template<>
bool CallbackList<gb_hierarchy_callback>::contains_unremoved_callback(const gb_hierarchy_callback& like) const {
    for (const_itertype cb = callbacks.begin(); cb != callbacks.end(); ++cb) {
        if (cb->spec.is_equal_to(like.spec)   &&
            !cb->spec.is_marked_for_removal() &&
            cb->get_location() == like.get_location()) // if location differs, accept duplicate callback
        {
            return true;
        }
    }
    return false;
}
#endif

template <typename PRED>
inline void gb_remove_callbacks_that(GBDATA *gbd, PRED shallRemove) {
#if defined(ASSERTION_USED)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: gb_remove_callback called inside delete-callback of gbd (gbd may already be freed)\n");
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
        return;
    }
#endif // DEBUG

    if (gbd->ext) {
        gb_callback_list *cbl = gbd->get_callbacks();
        if (cbl) cbl->remove_callbacks_that(shallRemove);
    }
}

struct ShallBeDeleted {
    bool operator()(const gb_callback& cb) const { return cb.spec.is_marked_for_removal(); }
};
void gb_remove_callbacks_marked_for_deletion(GBDATA *gbd) {
    gb_remove_callbacks_that(gbd, ShallBeDeleted());
}

struct IsCallback : private TypedDatabaseCallback {
    IsCallback(GB_CB func_, GB_CB_TYPE type_) : TypedDatabaseCallback(makeDatabaseCallback((GB_CB)func_, (int*)NULL), type_) {}
    bool operator()(const gb_callback& cb) const { return sig_is_equal_to(cb.spec); }
};
struct IsSpecificCallback : private TypedDatabaseCallback {
    IsSpecificCallback(const TypedDatabaseCallback& cb) : TypedDatabaseCallback(cb) {}
    bool operator()(const gb_callback& cb) const { return is_equal_to(cb.spec); }
};
struct IsSpecificHierarchyCallback : private TypedDatabaseCallback {
    gb_hierarchy_location loc;
    IsSpecificHierarchyCallback(const gb_hierarchy_location& loc_, const TypedDatabaseCallback& cb)
        : TypedDatabaseCallback(cb),
          loc(loc_)
    {}
    bool operator()(const gb_callback& cb) const {
        const gb_hierarchy_callback& hcb = static_cast<const gb_hierarchy_callback&>(cb);
        return is_equal_to(cb.spec) && hcb.get_location() == loc;
    }
};

inline void add_to_callback_chain(gb_callback_list*& head, const TypedDatabaseCallback& cbs) {
    if (!head) head = new gb_callback_list;
    head->add(gb_callback(cbs));
}
inline void add_to_callback_chain(gb_hierarchy_callback_list*& head, const TypedDatabaseCallback& cbs, const gb_hierarchy_location& loc) {
    if (!head) head = new gb_hierarchy_callback_list;
    head->add(gb_hierarchy_callback(cbs, loc));
}

inline GB_ERROR gb_add_callback(GBDATA *gbd, const TypedDatabaseCallback& cbs) {
    /* Adds a callback to a DB entry.
     *
     * Be careful when writing GB_CB_DELETE callbacks, there is a severe restriction:
     *
     * - the DB element may already be freed. The pointer is still pointing to the original
     *   location, so you can use it to identify the DB element, but you cannot dereference
     *   it under all circumstances.
     *
     * ARBDB internal delete-callbacks may use gb_get_main_during_cb() to access the DB root.
     * See also: GB_get_gb_main_during_cb()
     */

#if defined(DEBUG)
    if (GB_inside_callback(gbd, GB_CB_DELETE)) {
        printf("Warning: add_priority_callback called inside delete-callback of gbd (gbd may already be freed)\n");
#if defined(DEVEL_RALF)
        gb_assert(0); // fix callback-handling (never modify callbacks from inside delete callbacks)
#endif // DEVEL_RALF
    }
#endif // DEBUG

    GB_test_transaction(gbd); // may return error
    gbd->create_extended();
    add_to_callback_chain(gbd->ext->callback, cbs);
    return 0;
}

GB_ERROR GB_add_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    return gb_add_callback(gbd, TypedDatabaseCallback(dbcb, type));
}

void GB_remove_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    // remove specific callback; 'type' and 'dbcb' have to match
    gb_remove_callbacks_that(gbd, IsSpecificCallback(TypedDatabaseCallback(dbcb, type)));
}
void GB_remove_all_callbacks_to(GBDATA *gbd, GB_CB_TYPE type, GB_CB func) {
    // removes all callbacks 'func' bound to 'gbd' with 'type'
    gb_remove_callbacks_that(gbd, IsCallback(func, type));
}

inline void GB_MAIN_TYPE::callback_group::add_hcb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb) {
    add_to_callback_chain(hierarchy_cbs, dbcb, loc);
}
inline void GB_MAIN_TYPE::callback_group::remove_hcb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb) {
    if (hierarchy_cbs) {
        hierarchy_cbs->remove_callbacks_that(IsSpecificHierarchyCallback(loc, dbcb));
    }
}

#define CHECK_HIER_CB_CONDITION()                                                                       \
    if (get_transaction_level()<0) return "no hierarchy callbacks allowed in NO_TRANSACTION_MODE";      \
    if (!loc.is_valid()) return "invalid hierarchy location"

GB_ERROR GB_MAIN_TYPE::add_hierarchy_cb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb) {
    CHECK_HIER_CB_CONDITION();

    GB_CB_TYPE type = dbcb.get_type();
    if (type & GB_CB_DELETE) {
        deleteCBs.add_hcb(loc, dbcb.with_type_changed_to(GB_CB_DELETE));
    }
    if (type & GB_CB_ALL_BUT_DELETE) {
        changeCBs.add_hcb(loc, dbcb.with_type_changed_to(GB_CB_TYPE(type&GB_CB_ALL_BUT_DELETE)));
    }
    return NULL;
}

GB_ERROR GB_MAIN_TYPE::remove_hierarchy_cb(const gb_hierarchy_location& loc, const TypedDatabaseCallback& dbcb) {
    CHECK_HIER_CB_CONDITION();

    GB_CB_TYPE type = dbcb.get_type();
    if (type & GB_CB_DELETE) {
        deleteCBs.remove_hcb(loc, dbcb.with_type_changed_to(GB_CB_DELETE));
    }
    if (type & GB_CB_ALL_BUT_DELETE) {
        changeCBs.remove_hcb(loc, dbcb.with_type_changed_to(GB_CB_TYPE(type&GB_CB_ALL_BUT_DELETE)));
    }
    return NULL;
}

#undef CHECK_HIER_CB_CONDITION

GB_ERROR GB_add_hierarchy_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    /*! bind callback to ALL entries which are at the same DB-hierarchy as 'gbd'.
     *
     * Hierarchy callbacks are triggered before normal callbacks (added by GB_add_callback or GB_ensure_callback).
     * Nevertheless delete callbacks take precedence over change callbacks
     * (i.e. a normal delete callback is triggered before a hierarchical change callback).
     *
     * Hierarchy callbacks cannot be installed and will NOT be triggered in NO_TRANSACTION_MODE
     * (i.e. it will not work in ARBs property DBs)
     *
     * @return error if in NO_TRANSACTION_MODE or if hierarchy location is invalid
     */
    return GB_MAIN(gbd)->add_hierarchy_cb(gb_hierarchy_location(gbd), TypedDatabaseCallback(dbcb, type));
}

GB_ERROR GB_remove_hierarchy_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    //! remove callback added with GB_add_hierarchy_callback()
    return GB_MAIN(gbd)->remove_hierarchy_cb(gb_hierarchy_location(gbd), TypedDatabaseCallback(dbcb, type));
}

GB_ERROR GB_add_hierarchy_callback(GBDATA *gb_main, const char *db_path, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    //! same as overloaded GB_add_hierarchy_callback(), but using db_path instead of GBDATA to define hierarchy location
    return GB_MAIN(gb_main)->add_hierarchy_cb(gb_hierarchy_location(gb_main, db_path), TypedDatabaseCallback(dbcb, type));
}
GB_ERROR GB_remove_hierarchy_callback(GBDATA *gb_main, const char *db_path, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    //! same as overloaded GB_remove_hierarchy_callback(), but using db_path instead of GBDATA to define hierarchy location
    return GB_MAIN(gb_main)->remove_hierarchy_cb(gb_hierarchy_location(gb_main, db_path), TypedDatabaseCallback(dbcb, type));
}

GB_ERROR GB_ensure_callback(GBDATA *gbd, GB_CB_TYPE type, const DatabaseCallback& dbcb) {
    TypedDatabaseCallback newcb(dbcb, type);
    gb_callback_list *cbl = gbd->get_callbacks();
    if (cbl) {
        for (gb_callback_list::itertype cb = cbl->callbacks.begin(); cb != cbl->callbacks.end(); ++cb) {
            if (cb->spec.is_equal_to(newcb) && !cb->spec.is_marked_for_removal()) {
                return NULL; // already in cb list
            }
        }
    }
    return gb_add_callback(gbd, newcb);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <string> // needed b4 test_unit.h!
#include <arb_file.h>

#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif


// -----------------------
//      test callbacks

static void test_count_cb(GBDATA *, int *counter) {
    fprintf(stderr, "test_count_cb: var.add=%p old.val=%i ", counter, *counter);
    (*counter)++;
    fprintf(stderr, "new.val=%i\n", *counter);
    fflush(stderr);
}

static void remove_self_cb(GBDATA *gbe, GB_CB_TYPE cbtype) {
    GB_remove_callback(gbe, cbtype, makeDatabaseCallback(remove_self_cb));
}

static void re_add_self_cb(GBDATA *gbe, int *calledCounter, GB_CB_TYPE cbtype) {
    ++(*calledCounter);

    DatabaseCallback dbcb = makeDatabaseCallback(re_add_self_cb, calledCounter);
    GB_remove_callback(gbe, cbtype, dbcb);

    ASSERT_NO_ERROR(GB_add_callback(gbe, cbtype, dbcb));
}

void TEST_db_callbacks_ta_nota() {
    GB_shell shell;

    enum TAmode {
        NO_TA   = 1, // no transaction mode
        WITH_TA = 2, // transaction mode

        BOTH_TA_MODES = (NO_TA|WITH_TA)
    };

    for (TAmode ta_mode = NO_TA; ta_mode <= WITH_TA; ta_mode = TAmode(ta_mode+1)) {
        GBDATA   *gb_main = GB_open("no.arb", "c");
        GB_ERROR  error;

        TEST_ANNOTATE(ta_mode == NO_TA ? "NO_TA" : "WITH_TA");
        if (ta_mode == NO_TA) {
            error = GB_no_transaction(gb_main); TEST_EXPECT_NO_ERROR(error);
        }

        // create some DB entries
        GBDATA *gbc;
        GBDATA *gbe1;
        GBDATA *gbe2;
        GBDATA *gbe3;
        {
            GB_transaction ta(gb_main);

            gbc  = GB_create_container(gb_main, "cont");
            gbe1 = GB_create(gbc, "entry", GB_STRING);
            gbe2 = GB_create(gb_main, "entry", GB_INT);
        }

        // counters to detect called callbacks
        int e1_changed    = 0;
        int e2_changed    = 0;
        int c_changed     = 0;
        int c_son_created = 0;

        int e1_deleted = 0;
        int e2_deleted = 0;
        int e3_deleted = 0;
        int c_deleted  = 0;

#define CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc)       \
        that(e1_changed).is_equal_to(e1c),              \
            that(e2_changed).is_equal_to(e2c),          \
            that(c_changed).is_equal_to(cc),            \
            that(c_son_created).is_equal_to(csc)

#define DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd)       \
        that(e1_deleted).is_equal_to(e1d),              \
            that(e2_deleted).is_equal_to(e2d),          \
            that(e3_deleted).is_equal_to(e3d),          \
            that(c_deleted).is_equal_to(cd)

#define TEST_EXPECT_CHCB_COUNTERS(e1c,e2c,cc,csc,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION(all().of(CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc))); }while(0)
#define TEST_EXPECT_CHCB___WANTED(e1c,e2c,cc,csc,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION__WANTED(all().of(CHCB_COUNTERS_EXPECTATION(e1c,e2c,cc,csc))); }while(0)

#define TEST_EXPECT_DLCB_COUNTERS(e1d,e2d,e3d,cd,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION(all().of(DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd))); }while(0)
#define TEST_EXPECT_DLCB___WANTED(e1d,e2d,e3d,cd,tam) do{ if (ta_mode & (tam)) TEST_EXPECTATION__WANTED(all().of(DLCB_COUNTERS_EXPECTATION(e1d,e2d,e3d,cd))); }while(0)

#define TEST_EXPECT_COUNTER(tam,cnt,expected)             do{ if (ta_mode & (tam)) TEST_EXPECT_EQUAL(cnt, expected); }while(0)
#define TEST_EXPECT_COUNTER__BROKEN(tam,cnt,expected,got) do{ if (ta_mode & (tam)) TEST_EXPECT_EQUAL__BROKEN(cnt, expected, got); }while(0)

#define RESET_CHCB_COUNTERS()   do{ e1_changed = e2_changed = c_changed = c_son_created = 0; }while(0)
#define RESET_DLCB_COUNTERS()   do{ e1_deleted = e2_deleted = e3_deleted = c_deleted = 0; }while(0)
#define RESET_ALL_CB_COUNTERS() do{ RESET_CHCB_COUNTERS(); RESET_DLCB_COUNTERS(); }while(0)

        // install some DB callbacks
        {
            GB_transaction ta(gb_main);
            GB_add_callback(gbe1, GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &e1_changed));
            GB_add_callback(gbe2, GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &e2_changed));
            GB_add_callback(gbc,  GB_CB_CHANGED,     makeDatabaseCallback(test_count_cb, &c_changed));
            GB_add_callback(gbc,  GB_CB_SON_CREATED, makeDatabaseCallback(test_count_cb, &c_son_created));
        }

        // check callbacks were not called yet
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES);

        // trigger callbacks
        {
            GB_transaction ta(gb_main);

            error = GB_write_string(gbe1, "hi"); TEST_EXPECT_NO_ERROR(error);
            error = GB_write_int(gbe2, 666);     TEST_EXPECT_NO_ERROR(error);

            TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, NO_TA);   // callbacks triggered instantly in NO_TA mode
            TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, WITH_TA); // callbacks delayed until transaction is committed

        } // [Note: callbacks happen here in ta_mode]

        // test that GB_CB_SON_CREATED is not triggered here:
        TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, NO_TA);
        TEST_EXPECT_CHCB_COUNTERS(1, 1, 1, 0, WITH_TA);

        // really create a son
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            gbe3 = GB_create(gbc, "e3", GB_STRING);
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, NO_TA); // broken
        TEST_EXPECT_CHCB___WANTED(0, 0, 1, 1, NO_TA);
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 1, WITH_TA);

        // change that son
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            error = GB_write_string(gbe3, "bla"); TEST_EXPECT_NO_ERROR(error);
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 0, BOTH_TA_MODES);


        // test delete callbacks
        RESET_CHCB_COUNTERS();
        {
            GB_transaction ta(gb_main);

            GB_add_callback(gbe1, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e1_deleted));
            GB_add_callback(gbe2, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e2_deleted));
            GB_add_callback(gbe3, GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &e3_deleted));
            GB_add_callback(gbc,  GB_CB_DELETE, makeDatabaseCallback(test_count_cb, &c_deleted));
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES); // adding callbacks does not trigger existing change-callbacks
        {
            GB_transaction ta(gb_main);

            error = GB_delete(gbe3); TEST_EXPECT_NO_ERROR(error);
            error = GB_delete(gbe2); TEST_EXPECT_NO_ERROR(error);

            TEST_EXPECT_DLCB_COUNTERS(0, 1, 1, 0, NO_TA);
            TEST_EXPECT_DLCB_COUNTERS(0, 0, 0, 0, WITH_TA);
        }

        TEST_EXPECT_CHCB_COUNTERS(0, 0, 1, 0, WITH_TA); // container changed by deleting a son (gbe3); no longer triggers via GB_SON_CHANGED
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, NO_TA);   // change is not triggered in NO_TA mode (error?)
        TEST_EXPECT_CHCB___WANTED(0, 0, 1, 1, NO_TA);

        TEST_EXPECT_DLCB_COUNTERS(0, 1, 1, 0, BOTH_TA_MODES);

        RESET_ALL_CB_COUNTERS();
        {
            GB_transaction ta(gb_main);
            error = GB_delete(gbc);  TEST_EXPECT_NO_ERROR(error); // delete the container containing gbe1 and gbe3 (gbe3 alreay deleted)
        }
        TEST_EXPECT_CHCB_COUNTERS(0, 0, 0, 0, BOTH_TA_MODES); // deleting the container does not trigger any change callbacks
        TEST_EXPECT_DLCB_COUNTERS(1, 0, 0, 1, BOTH_TA_MODES); // deleting the container does also trigger the delete callback for gbe1

        // --------------------------------------------------------------------------------
        // document that a callback now can be removed while it is running
        // (in NO_TA mode; always worked in WITH_TA mode)
        {
            GBDATA *gbe;
            {
                GB_transaction ta(gb_main);
                gbe = GB_create(gb_main, "new_e1", GB_INT); // recreate
                GB_add_callback(gbe, GB_CB_CHANGED, makeDatabaseCallback(remove_self_cb));
            }
            { GB_transaction ta(gb_main); GB_touch(gbe); }
        }

        // test that a callback may remove and re-add itself
        {
            GBDATA *gbn1;
            GBDATA *gbn2;

            int counter1 = 0;
            int counter2 = 0;

            {
                GB_transaction ta(gb_main);
                gbn1 = GB_create(gb_main, "new_e1", GB_INT);
                gbn2 = GB_create(gb_main, "new_e2", GB_INT);
                GB_add_callback(gbn1, GB_CB_CHANGED, makeDatabaseCallback(re_add_self_cb, &counter1));
            }

            TEST_EXPECT_COUNTER(NO_TA,         counter1, 0); // no callback triggered (trigger happens BEFORE call to GB_add_callback in NO_TA mode!)
            TEST_EXPECT_COUNTER(WITH_TA,       counter1, 1); // callback gets triggered by GB_create
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            counter1 = 0; counter2 = 0;

            // test no callback is triggered by just adding a callback
            {
                GB_transaction ta(gb_main);
                GB_add_callback(gbn2, GB_CB_CHANGED, makeDatabaseCallback(re_add_self_cb, &counter2));
            }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 0);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            { GB_transaction ta(gb_main); GB_touch(gbn1); }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 0);

            { GB_transaction ta(gb_main); GB_touch(gbn2); }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 1);

            { GB_transaction ta(gb_main);  }
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter1, 1);
            TEST_EXPECT_COUNTER(BOTH_TA_MODES, counter2, 1);
        }

        GB_close(gb_main);
    }
}

struct calledWith {
    RefPtr<GBDATA> gbd;
    GB_CB_TYPE     type;
    int            time_called;

    static int timer;

    calledWith(GBDATA *gbd_, GB_CB_TYPE type_) : gbd(gbd_), type(type_), time_called(++timer) {}
};

using std::string;
using std::list;

class callback_trace;

class ct_registry {
    static ct_registry *SINGLETON;

    typedef list<callback_trace*> ctl;

    ctl traces;
public:
    ct_registry() {
        gb_assert(!SINGLETON);
        SINGLETON = this;
    }
    ~ct_registry() {
        gb_assert(this == SINGLETON);
        SINGLETON = NULL;
        gb_assert(traces.empty());
    }

    static ct_registry *instance() { gb_assert(SINGLETON); return SINGLETON; }

    void add(callback_trace *ct) { traces.push_back(ct); }
    void remove(callback_trace *ct) { traces.remove(ct); }

    arb_test::match_expectation expect_all_calls_checked();
};
ct_registry *ct_registry::SINGLETON = NULL;

int calledWith::timer = 0;

class callback_trace {
    typedef list<calledWith> calledList;
    typedef calledList::iterator  calledIter;

    calledList called;
    string     name;

    calledIter find(GBDATA *gbd) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->gbd == gbd) break;
            ++c;
        }
        return c;
    }
    calledIter find(GB_CB_TYPE exp_type) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->type&exp_type) break;
            ++c;
        }
        return c;
    }
    calledIter find(GBDATA *gbd, GB_CB_TYPE exp_type) {
        calledIter c = called.begin();
        while (c != called.end()) {
            if (c->gbd == gbd && (c->type&exp_type)) break;
            ++c;
        }
        return c;
    }

    bool removed(calledIter c) {
        if (c == called.end()) return false;
        called.erase(c);
        return true;
    }

public:
    callback_trace(const char *name_)
        : name(name_)
    {
        called.clear();
        ct_registry::instance()->add(this);
    }
    ~callback_trace() {
        if (was_called()) TEST_EXPECT_EQUAL(name, "has unchecked calls in dtor"); // you forgot to check some calls using TEST_EXPECT_..._TRIGGERED
        ct_registry::instance()->remove(this);
    }

    const string& get_name() const { return name; }

    void set_called_by(GBDATA *gbd, GB_CB_TYPE type) { called.push_back(calledWith(gbd, type)); }

    bool was_called_by(GBDATA *gbd) { return removed(find(gbd)); }
    bool was_called_by(GB_CB_TYPE exp_type) { return removed(find(exp_type)); }
    bool was_called_by(GBDATA *gbd, GB_CB_TYPE exp_type) { return removed(find(gbd, exp_type)); }

    int call_time(GBDATA *gbd, GB_CB_TYPE exp_type) {
        calledIter found = find(gbd, exp_type);
        if (found == called.end()) return -1;

        int t = found->time_called;
        removed(found);
        return t;
    }

    bool was_not_called() const { return called.empty(); }
    bool was_called() const { return !was_not_called(); }
};

arb_test::match_expectation ct_registry::expect_all_calls_checked() {
    using namespace   arb_test;
    expectation_group expected;

    // add failing expectations for all traces with unchecked calls
    for (ctl::iterator t = traces.begin(); t != traces.end(); ++t) {
        callback_trace *ct = *t;
        if (ct->was_called()) {
            expectation_group bad_trace;
            bad_trace.add(that(ct->was_called()).is_equal_to(false));

            const char *failing_trace = ct->get_name().c_str();
            bad_trace.add(that(failing_trace).does_differ_from(failing_trace)); // display failing_trace in failing expectation

            expected.add(all().ofgroup(bad_trace));
        }
    }

    return all().ofgroup(expected);
}


static void some_cb(GBDATA *gbd, callback_trace *trace, GB_CB_TYPE cbtype) {
    trace->set_called_by(gbd, cbtype);
}

#define TRACESTRUCT(ELEM,FLAVOR)           trace_##ELEM##_##FLAVOR
#define HIERARCHY_TRACESTRUCT(ELEM,FLAVOR) traceHier_##ELEM##_##FLAVOR

#define CONSTRUCT(name)                       name(#name) // pass instance-name to callback_trace-ctor as char* 
#define TRACECONSTRUCT(ELEM,FLAVOR)           CONSTRUCT(TRACESTRUCT(ELEM,FLAVOR))
#define HIERARCHY_TRACECONSTRUCT(ELEM,FLAVOR) CONSTRUCT(HIERARCHY_TRACESTRUCT(ELEM,FLAVOR))

#define ADD_CHANGED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed))))
#define ADD_DELETED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted))))
#define ADD_NWCHILD_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild))))

#define ADD_CHANGED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,changed))))
#define ADD_DELETED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,deleted))))
#define ADD_NWCHILD_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,newchild))))

#define ENSURE_CHANGED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed))))
#define ENSURE_DELETED_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted))))
#define ENSURE_NWCHILD_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_ensure_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild))))

#define REMOVE_CHANGED_CALLBACK(elem) GB_remove_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,changed)))
#define REMOVE_DELETED_CALLBACK(elem) GB_remove_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,deleted)))
#define REMOVE_NWCHILD_CALLBACK(elem) GB_remove_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &TRACESTRUCT(elem,newchild)))

#define REMOVE_CHANGED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_remove_hierarchy_callback(elem, GB_CB_CHANGED,     makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,changed))))
#define REMOVE_DELETED_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_remove_hierarchy_callback(elem, GB_CB_DELETE,      makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,deleted))))
#define REMOVE_NWCHILD_HIERARCHY_CALLBACK(elem) TEST_EXPECT_NO_ERROR(GB_remove_hierarchy_callback(elem, GB_CB_SON_CREATED, makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(elem,newchild))))

#define INIT_CHANGED_CALLBACK(elem) callback_trace TRACECONSTRUCT(elem,changed);  ADD_CHANGED_CALLBACK(elem)
#define INIT_DELETED_CALLBACK(elem) callback_trace TRACECONSTRUCT(elem,deleted);  ADD_DELETED_CALLBACK(elem)
#define INIT_NWCHILD_CALLBACK(elem) callback_trace TRACECONSTRUCT(elem,newchild); ADD_NWCHILD_CALLBACK(elem)

#define INIT_CHANGED_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACECONSTRUCT(elem,changed);  ADD_CHANGED_HIERARCHY_CALLBACK(elem)
#define INIT_DELETED_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACECONSTRUCT(elem,deleted);  ADD_DELETED_HIERARCHY_CALLBACK(elem)
#define INIT_NWCHILD_HIERARCHY_CALLBACK(elem) callback_trace HIERARCHY_TRACECONSTRUCT(elem,newchild); ADD_NWCHILD_HIERARCHY_CALLBACK(elem)

#define ENSURE_ENTRY_CALLBACKS(entry)    ENSURE_CHANGED_CALLBACK(entry); ENSURE_DELETED_CALLBACK(entry)
#define ENSURE_CONTAINER_CALLBACKS(cont) ENSURE_CHANGED_CALLBACK(cont);  ENSURE_NWCHILD_CALLBACK(cont); ENSURE_DELETED_CALLBACK(cont)

#define REMOVE_ENTRY_CALLBACKS(entry)    REMOVE_CHANGED_CALLBACK(entry); REMOVE_DELETED_CALLBACK(entry)
#define REMOVE_CONTAINER_CALLBACKS(cont) REMOVE_CHANGED_CALLBACK(cont);  REMOVE_NWCHILD_CALLBACK(cont); REMOVE_DELETED_CALLBACK(cont)

#define INIT_ENTRY_CALLBACKS(entry)    INIT_CHANGED_CALLBACK(entry); INIT_DELETED_CALLBACK(entry)
#define INIT_CONTAINER_CALLBACKS(cont) INIT_CHANGED_CALLBACK(cont);  INIT_NWCHILD_CALLBACK(cont); INIT_DELETED_CALLBACK(cont)

#define TRIGGER_CHANGE(gbd) do {                \
        GB_initial_transaction ta(gb_main);     \
        if (ta.ok()) GB_touch(gbd);             \
        TEST_EXPECT_NO_ERROR(ta.close(NULL));   \
    } while(0)

#define TRIGGER_2_CHANGES(gbd1, gbd2) do {      \
        GB_initial_transaction ta(gb_main);     \
        if (ta.ok()) {                          \
            GB_touch(gbd1);                     \
            GB_touch(gbd2);                     \
        }                                       \
        TEST_EXPECT_NO_ERROR(ta.close(NULL));   \
    } while(0)

#define TRIGGER_DELETE(gbd) do {                \
        GB_initial_transaction ta(gb_main);     \
        GB_ERROR error = NULL;                  \
        if (ta.ok()) error = GB_delete(gbd);    \
        TEST_EXPECT_NO_ERROR(ta.close(error));  \
    } while(0)

#define TEST_EXPECT_CB_TRIGGERED(TRACE,GBD,TYPE)         TEST_EXPECT(TRACE.was_called_by(GBD, TYPE))
#define TEST_EXPECT_CB_TRIGGERED_AT(TRACE,GBD,TYPE,TIME) TEST_EXPECT_EQUAL(TRACE.call_time(GBD, TYPE), TIME)

#define TEST_EXPECT_CHANGE_TRIGGERED(GBD) TEST_EXPECT_CB_TRIGGERED(TRACESTRUCT(GBD, changed),  GBD, GB_CB_CHANGED)
#define TEST_EXPECT_DELETE_TRIGGERED(GBD) TEST_EXPECT_CB_TRIGGERED(TRACESTRUCT(GBD, deleted),  GBD, GB_CB_DELETE)
#define TEST_EXPECT_NCHILD_TRIGGERED(GBD) TEST_EXPECT_CB_TRIGGERED(TRACESTRUCT(GBD, newchild), GBD, GB_CB_SON_CREATED)

#define TEST_EXPECT_CHANGE_HIER_TRIGGERED(NAME,GBD) TEST_EXPECT_CB_TRIGGERED(HIERARCHY_TRACESTRUCT(NAME, changed),  GBD, GB_CB_CHANGED)
#define TEST_EXPECT_DELETE_HIER_TRIGGERED(NAME,GBD) TEST_EXPECT_CB_TRIGGERED(HIERARCHY_TRACESTRUCT(NAME, deleted),  GBD, GB_CB_DELETE)
#define TEST_EXPECT_NCHILD_HIER_TRIGGERED(NAME,GBD) TEST_EXPECT_CB_TRIGGERED(HIERARCHY_TRACESTRUCT(NAME, newchild), GBD, GB_CB_SON_CREATED)


#define TEST_EXPECT_CHANGE_TRIGGERED_AT(TRACE,GBD,TIME) TEST_EXPECT_CB_TRIGGERED_AT(TRACE, GBD, GB_CB_CHANGED, TIME)
#define TEST_EXPECT_DELETE_TRIGGERED_AT(TRACE,GBD,TIME) TEST_EXPECT_CB_TRIGGERED_AT(TRACE, GBD, GB_CB_DELETE, TIME)

#define TEST_EXPECT_TRIGGERS_CHECKED() TEST_EXPECTATION(trace_registry.expect_all_calls_checked())

void TEST_db_callbacks() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("new.arb", "c");

    // create some data
    GB_begin_transaction(gb_main);

    GBDATA *cont_top = GB_create_container(gb_main,  "cont_top"); TEST_REJECT_NULL(cont_top);
    GBDATA *cont_son = GB_create_container(cont_top, "cont_son"); TEST_REJECT_NULL(cont_son);

    GBDATA *top       = GB_create(gb_main,  "top",       GB_STRING); TEST_REJECT_NULL(top);
    GBDATA *son       = GB_create(cont_top, "son",       GB_INT);    TEST_REJECT_NULL(son);
    GBDATA *grandson  = GB_create(cont_son, "grandson",  GB_STRING); TEST_REJECT_NULL(grandson);
    GBDATA *ograndson = GB_create(cont_son, "ograndson", GB_STRING); TEST_REJECT_NULL(ograndson);

    GB_commit_transaction(gb_main);

    // install callbacks
    GB_begin_transaction(gb_main);

    ct_registry trace_registry;
    INIT_CONTAINER_CALLBACKS(cont_top);
    INIT_CONTAINER_CALLBACKS(cont_son);
    INIT_ENTRY_CALLBACKS(top);
    INIT_ENTRY_CALLBACKS(son);
    INIT_ENTRY_CALLBACKS(grandson);
    INIT_ENTRY_CALLBACKS(ograndson);

    GB_commit_transaction(gb_main);

    TEST_EXPECT_TRIGGERS_CHECKED();

    // trigger change callbacks via change
    GB_begin_transaction(gb_main);
    GB_write_string(top, "hello world");
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    GB_begin_transaction(gb_main);
    GB_write_string(top, "hello world"); // no change
    GB_commit_transaction(gb_main);
    TEST_EXPECT_TRIGGERS_CHECKED();

#if 0
    // code is wrong (cannot set terminal entry to "marked")
    GB_begin_transaction(gb_main);
    GB_write_flag(son, 1);                                  // only change "mark"
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(son);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGER__UNWANTED(trace_cont_top_newchild); // @@@ modifying son should not trigger newchild callback
    TEST_EXPECT_TRIGGERS_CHECKED();
#else
    // @@@ add test code similar to wrong section above
#endif

    GB_begin_transaction(gb_main);
    GB_touch(grandson);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(grandson);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // trigger change- and soncreate-callbacks via create

    GB_begin_transaction(gb_main);
    GBDATA *son2 = GB_create(cont_top, "son2", GB_INT); TEST_REJECT_NULL(son2);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    GB_begin_transaction(gb_main);
    GBDATA *grandson2 = GB_create(cont_son, "grandson2", GB_STRING); TEST_REJECT_NULL(grandson2);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_son);
    TEST_EXPECT_NCHILD_TRIGGERED(cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // trigger callbacks via delete

    TRIGGER_DELETE(son2);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TRIGGER_DELETE(grandson2);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_son);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TEST_EXPECT_NO_ERROR(GB_request_undo_type(gb_main, GB_UNDO_UNDO));

    TRIGGER_DELETE(top);
    TEST_EXPECT_DELETE_TRIGGERED(top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TRIGGER_DELETE(grandson);
    TEST_EXPECT_DELETE_TRIGGERED(grandson);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_son);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TRIGGER_DELETE(cont_son);
    TEST_EXPECT_DELETE_TRIGGERED(ograndson);
    TEST_EXPECT_DELETE_TRIGGERED(cont_son);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // trigger callbacks by undoing last 3 delete transactions

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of cont_son
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of grandson
    // cont_son callbacks are not triggered (they are not restored by undo)
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    TEST_EXPECT_NO_ERROR(GB_undo(gb_main, GB_UNDO_UNDO)); // undo delete of top
    TEST_EXPECT_TRIGGERS_CHECKED();

    // reinstall callbacks that were removed by deletes

    GB_begin_transaction(gb_main);
    ENSURE_CONTAINER_CALLBACKS(cont_top);
    ENSURE_CONTAINER_CALLBACKS(cont_son);
    ENSURE_ENTRY_CALLBACKS(top);
    ENSURE_ENTRY_CALLBACKS(son);
    ENSURE_ENTRY_CALLBACKS(grandson);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // trigger callbacks which will be removed

    TRIGGER_CHANGE(son);
    TEST_EXPECT_CHANGE_TRIGGERED(son);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    GB_begin_transaction(gb_main);
    GBDATA *son3 = GB_create(cont_top, "son3", GB_INT); TEST_REJECT_NULL(son3);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_CHANGE_TRIGGERED(cont_top);
    TEST_EXPECT_NCHILD_TRIGGERED(cont_top);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // test remove callback

    GB_begin_transaction(gb_main);
    REMOVE_ENTRY_CALLBACKS(son);
    REMOVE_CONTAINER_CALLBACKS(cont_top);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // "trigger" removed callbacks

    TRIGGER_CHANGE(son);
    TEST_EXPECT_TRIGGERS_CHECKED();

    GB_begin_transaction(gb_main);
    GBDATA *son4 = GB_create(cont_top, "son4", GB_INT); TEST_REJECT_NULL(son4);
    GB_commit_transaction(gb_main);
    TEST_EXPECT_TRIGGERS_CHECKED();

    // avoid that GB_close calls delete callbacks (@@@ which might be an error in GB_close!)
    // by removing remaining callbacks
    REMOVE_ENTRY_CALLBACKS(grandson);
    REMOVE_ENTRY_CALLBACKS(top);
    REMOVE_CONTAINER_CALLBACKS(cont_son);

    GB_close(gb_main);
}

void TEST_hierarchy_callbacks() {
    GB_shell    shell;
    const char *DBNAME  = "tmp_hier_cb.arb";

    for (int pass = 1; pass<=2; ++pass) {
        bool creating = pass == 1;
        TEST_ANNOTATE(GBS_global_string("%s database", creating ? "created" : "reloaded"));

        GBDATA *gb_main = pass == 1 ? GB_open(DBNAME, "cw") : GB_open(DBNAME, "r");
        TEST_REJECT_NULL(gb_main);

        // create some data
        GB_begin_transaction(gb_main);

        GBDATA *cont_top1 = creating ? GB_create_container(gb_main, "cont_top") : GB_entry(gb_main, "cont_top"); TEST_REJECT_NULL(cont_top1);
        GBDATA *cont_top2 = creating ? GB_create_container(gb_main, "cont_top") : GB_nextEntry(cont_top1);       TEST_REJECT_NULL(cont_top2);

        GBDATA *cont_son11 = creating ? GB_create_container(cont_top1, "cont_son") : GB_entry(cont_top1, "cont_son"); TEST_REJECT_NULL(cont_son11);
        GBDATA *cont_son21 = creating ? GB_create_container(cont_top2, "cont_son") : GB_entry(cont_top2, "cont_son"); TEST_REJECT_NULL(cont_son21);
        GBDATA *cont_son22 = creating ? GB_create_container(cont_top2, "cont_son") : GB_nextEntry(cont_son21);        TEST_REJECT_NULL(cont_son22);

        GBDATA *top1 = creating ? GB_create(gb_main, "top", GB_STRING) : GB_entry(gb_main, "top"); TEST_REJECT_NULL(top1);
        GBDATA *top2 = creating ? GB_create(gb_main, "top", GB_STRING) : GB_nextEntry(top1);       TEST_REJECT_NULL(top2);

        GBDATA *son11 = creating ? GB_create(cont_top1, "son", GB_INT) : GB_entry(cont_top1, "son"); TEST_REJECT_NULL(son11);
        GBDATA *son12 = creating ? GB_create(cont_top1, "son", GB_INT) : GB_nextEntry(son11);        TEST_REJECT_NULL(son12);
        GBDATA *son21 = creating ? GB_create(cont_top2, "son", GB_INT) : GB_entry(cont_top2, "son"); TEST_REJECT_NULL(son21);

        GBDATA *grandson111 = creating ? GB_create(cont_son11, "grandson", GB_STRING) : GB_entry(cont_son11, "grandson"); TEST_REJECT_NULL(grandson111);
        GBDATA *grandson112 = creating ? GB_create(cont_son11, "grandson", GB_STRING) : GB_nextEntry(grandson111);        TEST_REJECT_NULL(grandson112);
        GBDATA *grandson211 = creating ? GB_create(cont_son21, "grandson", GB_STRING) : GB_entry(cont_son21, "grandson"); TEST_REJECT_NULL(grandson211);
        GBDATA *grandson221 = creating ? GB_create(cont_son22, "grandson", GB_STRING) : GB_entry(cont_son22, "grandson"); TEST_REJECT_NULL(grandson221);
        GBDATA *grandson222 = creating ? GB_create(cont_son22, "grandson", GB_STRING) : GB_nextEntry(grandson221);        TEST_REJECT_NULL(grandson222);

        // create some entries at uncommon hierarchy locations (compared to entries created above)
        GBDATA *ctop_top = creating ? GB_create          (cont_top2, "top", GB_STRING)      : GB_entry(cont_top2, "top");      TEST_REJECT_NULL(ctop_top);
        GBDATA *top_son  = creating ? GB_create          (gb_main,   "son", GB_INT)         : GB_entry(gb_main,   "son");      TEST_REJECT_NULL(top_son);
        GBDATA *cson     = creating ? GB_create_container(gb_main,   "cont_son")            : GB_entry(gb_main,   "cont_son"); TEST_REJECT_NULL(cson);
        GBDATA *cson_gs  = creating ? GB_create          (cson,      "grandson", GB_STRING) : GB_entry(cson,      "grandson"); TEST_REJECT_NULL(cson_gs);

        GB_commit_transaction(gb_main);

        // test gb_hierarchy_location
        {
            GB_transaction ta(gb_main);

            gb_hierarchy_location loc_top(top1);
            gb_hierarchy_location loc_son(son11);
            gb_hierarchy_location loc_grandson(grandson222);

            TEST_EXPECT(loc_top.matches(top1));
            TEST_EXPECT(loc_top.matches(top2));
            TEST_EXPECT(!loc_top.matches(cont_top1));
            TEST_EXPECT(!loc_top.matches(son12));
            TEST_EXPECT(!loc_top.matches(cont_son22));
            TEST_EXPECT(!loc_top.matches(ctop_top));

            TEST_EXPECT(loc_son.matches(son11));
            TEST_EXPECT(loc_son.matches(son21));
            TEST_EXPECT(!loc_son.matches(top1));
            TEST_EXPECT(!loc_son.matches(grandson111));
            TEST_EXPECT(!loc_son.matches(cont_son22));
            TEST_EXPECT(!loc_son.matches(top_son));

            TEST_EXPECT(loc_grandson.matches(grandson222));
            TEST_EXPECT(loc_grandson.matches(grandson111));
            TEST_EXPECT(!loc_grandson.matches(son11));
            TEST_EXPECT(!loc_grandson.matches(top1));
            TEST_EXPECT(!loc_grandson.matches(cont_son22));
            TEST_EXPECT(!loc_grandson.matches(cson_gs));
            TEST_EXPECT(!loc_grandson.matches(gb_main)); // nothing matches gb_main

            gb_hierarchy_location loc_ctop_top(ctop_top);
            TEST_EXPECT(loc_ctop_top.matches(ctop_top));
            TEST_EXPECT(!loc_ctop_top.matches(top1));

            gb_hierarchy_location loc_top_son(top_son);
            TEST_EXPECT(loc_top_son.matches(top_son));
            TEST_EXPECT(!loc_top_son.matches(son11));
            TEST_EXPECT(!loc_top_son.matches(gb_main)); // nothing matches gb_main

            gb_hierarchy_location loc_gs(cson_gs);
            TEST_EXPECT(loc_gs.matches(cson_gs));
            TEST_EXPECT(!loc_gs.matches(grandson211));

            gb_hierarchy_location loc_root(gb_main);
            TEST_REJECT(loc_root.is_valid()); // deny binding hierarchy callback to gb_main
            TEST_EXPECT(!loc_root.matches(gb_main)); // nothing matches gb_main
            TEST_EXPECT(!loc_root.matches(cont_top1)); // nothing matches an invalid location

            // test location from/to path
            {
                char *path_grandson = loc_grandson.get_db_path(gb_main);
                TEST_EXPECT_EQUAL(path_grandson, "/cont_top/cont_son/grandson");

                gb_hierarchy_location loc_grandson2(gb_main, path_grandson);
                TEST_EXPECT(loc_grandson2.is_valid());
                TEST_EXPECT(loc_grandson == loc_grandson2);

                char *path_grandson2 = loc_grandson2.get_db_path(gb_main);
                TEST_EXPECT_EQUAL(path_grandson, path_grandson2);

                free(path_grandson2);
                free(path_grandson);
            }

            gb_hierarchy_location loc_invalid(gb_main, "");
            TEST_REJECT(loc_invalid.is_valid());
            TEST_REJECT(loc_invalid == loc_invalid); // invalid locations equal nothing

            TEST_EXPECT(gb_hierarchy_location(gb_main, "/grandson/cont_top/son").is_valid()); // non-existing path with existing keys is valid
            TEST_EXPECT(gb_hierarchy_location(gb_main, "/no/such/path").is_valid());          // non-existing keys generate key-quarks on-the-fly
            TEST_EXPECT(gb_hierarchy_location(gb_main, "/grandson/missing/son").is_valid());  // non-existing keys generate key-quarks on-the-fly

            // test some pathological locations:
            TEST_REJECT(gb_hierarchy_location(gb_main, "/")    .is_valid());
            TEST_REJECT(gb_hierarchy_location(gb_main, "//")   .is_valid());
            TEST_REJECT(gb_hierarchy_location(gb_main, "  /  ").is_valid());
            TEST_REJECT(gb_hierarchy_location(gb_main, NULL)   .is_valid());
        }

        if (pass == 1) {
            TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, DBNAME, "wb"));
        }

        // instanciate callback_trace data and install hierarchy callbacks
        GBDATA *anySon = son11;

        GBDATA *anySonContainer     = cont_son11;
        GBDATA *anotherSonContainer = cont_son22;

        GBDATA *anyGrandson     = grandson221;
        GBDATA *anotherGrandson = grandson112;
        GBDATA *elimGrandson    = grandson222;
        GBDATA *elimGrandson2   = grandson111;
        GBDATA *newGrandson     = NULL;

        ct_registry    trace_registry;
        callback_trace HIERARCHY_TRACECONSTRUCT(anyElem,changed); // no CB added yet
        INIT_CHANGED_HIERARCHY_CALLBACK(anyGrandson);
        INIT_DELETED_HIERARCHY_CALLBACK(anyGrandson);
        INIT_NWCHILD_HIERARCHY_CALLBACK(anySonContainer);

        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger change-callback using same DB entry
        TRIGGER_CHANGE(anyGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anyGrandson);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger change-callback using another DB entry (same hierarchy)
        TRIGGER_CHANGE(anotherGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anotherGrandson);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // check nothing is triggered by an element at different hierarchy
        TRIGGER_CHANGE(anySon);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger change-callback using both DB entries (in two TAs)
        TRIGGER_CHANGE(anyGrandson);
        TRIGGER_CHANGE(anotherGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anyGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anotherGrandson);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger change-callback using both DB entries (in one TA)
        TRIGGER_2_CHANGES(anyGrandson, anotherGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anyGrandson);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, anotherGrandson);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger son-created-callback
        {
            GB_initial_transaction ta(gb_main);
            if (ta.ok()) {
                GBDATA *someson = GB_create(anySonContainer, "someson", GB_STRING); TEST_REJECT_NULL(someson);
            }
            TEST_EXPECT_NO_ERROR(ta.close(NULL));
        }
        TEST_EXPECT_NCHILD_HIER_TRIGGERED(anySonContainer, anySonContainer);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger 2 son-created-callbacks (for 2 containers) and one change-callback (for a newly created son)
        {
            GB_initial_transaction ta(gb_main);
            if (ta.ok()) {
                newGrandson     = GB_create(anotherSonContainer, "grandson", GB_STRING); TEST_REJECT_NULL(newGrandson);
                GBDATA *someson = GB_create(anySonContainer,     "someson",  GB_STRING); TEST_REJECT_NULL(someson);
            }
            TEST_EXPECT_NO_ERROR(ta.close(NULL));
        }
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyGrandson, newGrandson);
        TEST_EXPECT_NCHILD_HIER_TRIGGERED(anySonContainer, anotherSonContainer);
        TEST_EXPECT_NCHILD_HIER_TRIGGERED(anySonContainer, anySonContainer);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // trigger delete-callback
        {
            GB_initial_transaction ta(gb_main);
            TEST_EXPECT_NO_ERROR(GB_delete(elimGrandson));
            TEST_EXPECT_NO_ERROR(ta.close(NULL));
        }
        TEST_EXPECT_DELETE_HIER_TRIGGERED(anyGrandson, elimGrandson);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // bind normal (non-hierarchical) callbacks to entries which trigger hierarchical callbacks and ..
        calledWith::timer = 0;
        GB_begin_transaction(gb_main);

        INIT_CHANGED_CALLBACK(anotherGrandson);
        INIT_DELETED_CALLBACK(elimGrandson2);

        GB_commit_transaction(gb_main);

        TEST_EXPECT_TRIGGERS_CHECKED();

        {
            GB_initial_transaction ta(gb_main);
            if (ta.ok()) {
                GB_touch(anotherGrandson);
                GB_touch(elimGrandson2);
                TEST_EXPECT_NO_ERROR(GB_delete(elimGrandson2));
            }
        }

        // .. test call-order (delete before change, hierarchical before normal):
        TEST_EXPECT_DELETE_TRIGGERED_AT(traceHier_anyGrandson_deleted, elimGrandson2,   1);
        TEST_EXPECT_DELETE_TRIGGERED_AT(trace_elimGrandson2_deleted,   elimGrandson2,   2);
        TEST_EXPECT_CHANGE_TRIGGERED_AT(traceHier_anyGrandson_changed, anotherGrandson, 3);
        TEST_EXPECT_CHANGE_TRIGGERED_AT(trace_anotherGrandson_changed, anotherGrandson, 4);

        TEST_EXPECT_TRIGGERS_CHECKED();

        // test removed hierarchy callbacks stop to trigger
        REMOVE_CHANGED_HIERARCHY_CALLBACK(anyGrandson);
        REMOVE_DELETED_HIERARCHY_CALLBACK(anyGrandson);
        TRIGGER_CHANGE(anyGrandson);
        {
            GB_initial_transaction ta(gb_main);
            if (ta.ok()) TEST_EXPECT_NO_ERROR(GB_delete(anyGrandson));
        }
        TEST_EXPECT_TRIGGERS_CHECKED();

        GBDATA *anyElem;

        // bind SAME callback to different hierarchy locations
        anyElem = top1;  ADD_CHANGED_HIERARCHY_CALLBACK(anyElem); // binds      hierarchy cb to "/top"
        anyElem = son11; ADD_CHANGED_HIERARCHY_CALLBACK(anyElem); // binds SAME hierarchy cb to "/cont_top/son"

        // - check both trigger independently and together
        TRIGGER_CHANGE(top1);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, top1);
        TEST_EXPECT_TRIGGERS_CHECKED();

        TRIGGER_CHANGE(son11);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, son11);
        TEST_EXPECT_TRIGGERS_CHECKED();

        TRIGGER_2_CHANGES(top1, son11);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, top1);
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, son11);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // - check removing one does not disable the other
        anyElem = son11;  REMOVE_CHANGED_HIERARCHY_CALLBACK(anyElem); // remove hierarchy cb from "/cont_top/son"

        TRIGGER_2_CHANGES(top1, son11);
        // son11 no longer triggers -> ok
        TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, top1);
        TEST_EXPECT_TRIGGERS_CHECKED();

        // test add/remove hierarchy cb by path
        {
            const char       *locpath = "/cont_top/son";
            DatabaseCallback  dbcb    = makeDatabaseCallback(some_cb, &HIERARCHY_TRACESTRUCT(anyElem,changed));

            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(gb_main, locpath, GB_CB_CHANGED, dbcb));
            }

            // now both should trigger again
            TRIGGER_2_CHANGES(top1, son11);
            TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, top1);
            TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, son11);
            TEST_EXPECT_TRIGGERS_CHECKED();

            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_NO_ERROR(GB_remove_hierarchy_callback(gb_main, locpath, GB_CB_CHANGED, dbcb));
            }

            TRIGGER_2_CHANGES(top1, son11);
            // son11 no longer triggers -> ok
            TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, top1);
            TEST_EXPECT_TRIGGERS_CHECKED();

            // check some failing binds
            const char *invalidPath = "//such";
            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_ERROR_CONTAINS(GB_add_hierarchy_callback(gb_main, invalidPath,   GB_CB_CHANGED, dbcb), "invalid hierarchy location");
                TEST_EXPECT_ERROR_CONTAINS(GB_add_hierarchy_callback(gb_main,                GB_CB_CHANGED, dbcb), "invalid hierarchy location");
            }

            // bind a hierarchy callback to a "not yet existing" path (i.e. path containing yet unused keys),
            // then create an db-entry at that path and test that callback is trigger
            const char *unknownPath = "/unknownPath/unknownEntry";
            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_NO_ERROR(GB_add_hierarchy_callback(gb_main, unknownPath, GB_CB_CHANGED, dbcb));
            }
            TEST_EXPECT_TRIGGERS_CHECKED();

            GBDATA *gb_unknown;
            {
                GB_transaction ta(gb_main);
                TEST_EXPECT_RESULT__NOERROREXPORTED(gb_unknown = GB_search(gb_main, unknownPath, GB_STRING));
            }
            TEST_EXPECT_TRIGGERS_CHECKED(); // creating an entry does not trigger callback (could call a new callback-type)

            TRIGGER_CHANGE(gb_unknown);
            TEST_EXPECT_CHANGE_HIER_TRIGGERED(anyElem, gb_unknown);
            TEST_EXPECT_TRIGGERS_CHECKED();
        }

        // cleanup
        GB_close(gb_main);
    }

    GB_unlink(DBNAME);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
