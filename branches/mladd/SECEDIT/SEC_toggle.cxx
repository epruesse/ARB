// ================================================================= //
//                                                                   //
//   File      : SEC_toggle.cxx                                      //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "SEC_toggle.hxx"
#include "SEC_graphic.hxx"
#include "SEC_defs.hxx"

#include <arbdbt.h>

#include <climits>


using namespace std;

GB_ERROR SEC_structure_toggler::store(GBDATA *gb_struct) {
    char     *data    = 0;
    char     *xstring = 0;
    GB_ERROR  error   = gfx->read_data_from_db(&data, &xstring);

    if (!error) error = GBT_write_string(gb_struct, "data", data);
    if (!error) error = GBT_write_string(gb_struct, "ref", xstring);

    free(xstring);
    free(data);

    return error;
}

GB_ERROR SEC_structure_toggler::restore(GBDATA *gb_struct) {
    char     *data    = 0;
    char     *xstring = 0;
    GB_ERROR  error   = 0;

    GBDATA *gb_data   = GB_search(gb_struct, "data", GB_FIND);
    if (gb_data) data = GB_read_string(gb_data);
    if (!data) error  = GB_await_error();

    if (!error) {
        GBDATA *gb_ref      = GB_search(gb_struct, "ref", GB_FIND);
        if (gb_ref) xstring = GB_read_string(gb_ref);
        if (!xstring) error = GB_await_error();
    }

    if (!error) {
        sec_assert(data && xstring);
        error = gfx->write_data_to_db(data, xstring);
    }

    free(xstring);
    free(data);
    return error;
}

int SEC_structure_toggler::current() {
    return *GBT_read_int(gb_structures, "current");
}

GB_ERROR SEC_structure_toggler::set_current(int idx) {
    GBDATA   *gb_num = GB_search(gb_structures, "current", GB_INT);
    GB_ERROR  error;

    if (!gb_num) error = GB_await_error();
    else {
        sec_assert(find(idx));                      // oops - nonexisting container
        error = GB_write_int(gb_num, idx);
    }
    return error;
}

GBDATA *SEC_structure_toggler::find(int num) {
    int     cnt      = 0;
    GBDATA *gb_found = GB_entry(gb_structures, "struct");
    while (gb_found && num>0) {
        cnt++;
        num--;
        gb_found = GB_nextEntry(gb_found);
    }
    if (!gb_found) Count = cnt;  // seen all -> set count
    return gb_found;
}

GBDATA *SEC_structure_toggler::create(const char *structure_name) {
    sec_assert(!st_error);
    if (st_error) return 0;

    GBDATA *gb_new        = GB_create_container(gb_structures, "struct");
    if (!gb_new) st_error = GB_await_error();
    else {
        st_error = setName(gb_new, structure_name);

        if (!st_error) st_error = store(gb_new);
        if (!st_error) st_error = set_current(Count);
        if (!st_error) {
            gb_current = gb_new;
            sec_assert(find(current()) == gb_current);
            Count++;
        }
    }

    return gb_new;
}

// --------------------------------------------------------------------------------
// public

SEC_structure_toggler::SEC_structure_toggler(GBDATA *gb_main, const char *ali_name, SEC_graphic *Gfx)
    : gfx(Gfx)
    , st_error(0)
    , Count(0)
{
    GB_transaction ta(gb_main);
    gb_structures = GB_search(gb_main, GBS_global_string("secedit/structs/%s", ali_name), GB_CREATE_CONTAINER);
    if (!gb_structures) {
        st_error   = GB_await_error();
        gb_current = 0;
    }
    else {
        find(INT_MAX); // sets Count
        if (Count == 0) { // init
            gb_current = create(ali_name);
            st_error   = set_current(0);
        }
        else {
            int curr = current();
            if (curr<Count) {
                gb_current = find(current());
            }
            else { // wrong -> reset
                st_error   = set_current(0);
                gb_current = find(0);
            }
        }
        sec_assert(gb_current);
    }
}

GB_ERROR SEC_structure_toggler::next() {
    GB_ERROR       error = 0;
    GB_transaction ta(gb_structures);

    if (Count<2) {
        error = "No other structure in DB";
    }
    else {
        int nextNum = current()+1;
        if (nextNum >= Count) nextNum = 0;

        sec_assert(find(current()) == gb_current);

        error             = store(gb_current);
        if (!error) error = set_current(nextNum);
        if (!error) {
            gb_current = find(nextNum);
            if (!gb_current) {
                error = GBS_global_string("Failed to find structure #%i", nextNum);
            }
            else {
                error = restore(gb_current);
            }
        }
    }
    return ta.close(error);
}

GB_ERROR SEC_structure_toggler::copyTo(const char *structure_name) {
    GB_transaction ta(gb_structures);

    sec_assert(find(current()) == gb_current);

    GB_ERROR error = store(gb_current);
    if (!error) {
        GBDATA *gb_new = create(structure_name);
        if (!gb_new) {
            sec_assert(st_error);
            error = st_error;
        }
        else {
            gb_current = gb_new;
        }
    }
    sec_assert(error || (find(current()) == gb_current));
    return ta.close(error);
}

GB_ERROR SEC_structure_toggler::remove() {
    GB_transaction ta(gb_structures);

    sec_assert(Count > 1);

    GBDATA   *gb_del = gb_current;
    int       del    = current();
    GB_ERROR  error  = next();

    if (!error) {
        int curr = current();
        error    = GB_delete(gb_del);
        if (!error) {
            Count--;
            if (curr >= del) error = set_current(curr-1);
        }
    }
    return ta.close(error);
}

const char *SEC_structure_toggler::name() {
    const char     *structure_name = 0;
    GB_transaction  ta(gb_structures);

    GBDATA *gb_name               = GB_search(gb_current, "name", GB_FIND);
    if (gb_name) structure_name   = GB_read_char_pntr(gb_name);
    if (!structure_name) st_error = GB_await_error();

    st_error = ta.close(st_error);
    return structure_name;
}

GB_ERROR SEC_structure_toggler::setName(GBDATA *gb_struct, const char *new_name) {
    return GBT_write_string(gb_struct, "name", new_name);
}

GB_ERROR SEC_structure_toggler::setName(const char *new_name) {
    GB_transaction ta(gb_structures);
    GB_ERROR       error = setName(gb_current, new_name);
    return ta.close(error);
}

