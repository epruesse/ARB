// ================================================================= //
//                                                                   //
//   File      : SEC_toggle.cxx                                      //
//   Purpose   :                                                     //
//   Time-stamp: <Fri Sep/14/2007 17:01 MET Coder@ReallySoft.de>     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "SEC_toggle.hxx"
#include "SEC_graphic.hxx"
#include "SEC_defs.hxx"

#include <cstdlib>
#include <climits>


using namespace std;

GB_ERROR SEC_structure_toggler::store(GBDATA *gb_struct) {
    char     *data    = 0;
    char     *xstring = 0;
    GB_ERROR  error   = gfx->read_data_from_db(&data, &xstring);

    if (!error) {
        GBDATA *gb_data = GB_search(gb_struct, "data", GB_STRING);
        if (!gb_data) error = GB_get_error();
        else error = GB_write_string(gb_data, data);
    }

    if (!error) {
        GBDATA *gb_ref = GB_search(gb_struct, "ref", GB_STRING);
        if (!gb_ref) error = GB_get_error();
        else error = GB_write_string(gb_ref, xstring);
    }

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
    if (!data) error  = GB_get_error();

    if (!error) {
        GBDATA *gb_ref      = GB_search(gb_struct, "ref", GB_FIND);
        if (gb_ref) xstring = GB_read_string(gb_ref);
        if (!xstring) error = GB_get_error();
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
    GBDATA *gb_num = GB_search(gb_structures, "current", GB_INT);
    return GB_read_int(gb_num);
}
void SEC_structure_toggler::set_current(int idx) {
    GBDATA *gb_num = GB_search(gb_structures, "current", GB_INT);
    sec_assert(find(idx)); // oops - nonexisting container
    GB_write_int(gb_num, idx);
}

GBDATA *SEC_structure_toggler::find(int num) {
    int     cnt      = 0;
    GBDATA *gb_found = GB_find(gb_structures, "struct", 0, down_level);
    while (gb_found && num>0) {
        cnt++;
        num--;
        gb_found = GB_find(gb_found, "struct", 0, this_level|search_next);
    }
    if (!gb_found) Count = cnt;  // seen all -> set count
    return gb_found;
}

GBDATA *SEC_structure_toggler::create(const char *name) {
    sec_assert(!error);
    if (error) return 0;
    
    GBDATA *gb_new = GB_create_container(gb_structures, "struct");

    if (gb_new) {
        error = setName(gb_new, name);

        if (!error) error = store(gb_new);
        if (!error) {
            set_current(Count);
            gb_current = gb_new;
            
            sec_assert(find(current()) == gb_current);
            Count++;
        }
    }
    else error = GB_get_error();

    return gb_new;
}

// --------------------------------------------------------------------------------
// public

SEC_structure_toggler::SEC_structure_toggler(GBDATA *gb_main, const char *ali_name, SEC_graphic *Gfx)
    : gfx(Gfx)
    , error(0)
    , Count(0)
{
    GB_transaction ta(gb_main);
    gb_structures = GB_search(gb_main, GBS_global_string("secedit/structs/%s", ali_name), GB_CREATE_CONTAINER);
    if (!gb_structures) {
        error      = GB_get_error();
        gb_current = 0;
    }
    else {
        find(INT_MAX); // sets Count
        if (Count == 0) { // init
            gb_current = create(ali_name);
            set_current(0);
        }
        else {
            int curr = current();
            if (curr<Count) {
                gb_current = find(current());
            }
            else { // wrong -> reset
                set_current(0);
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
        int next = current()+1;
        if (next >= Count) next = 0;

        sec_assert(find(current()) == gb_current);
        
        error = store(gb_current);

        if (!error) {
            set_current(next);
            gb_current = find(next);
            if (!gb_current) {
                error = GBS_global_string("Failed to find structure #%i", next);
            }
            else {
                error = restore(gb_current);
            }
        }
    }
    if (error) ta.abort();
    return error;
}

GB_ERROR SEC_structure_toggler::copyTo(const char *name) {
    GB_ERROR err = 0;
    GB_transaction ta(gb_structures);

    sec_assert(find(current()) == gb_current);
    
    err = store(gb_current);

    if (!err) {
        GBDATA *gb_new = create(name);
        if (!gb_new) {
            sec_assert(error);
            err = error;
        }
        else {
            gb_current = gb_new;
        }
    }
    sec_assert(err || (find(current()) == gb_current));
    if (err) ta.abort();
    return err;
}

GB_ERROR SEC_structure_toggler::remove() {
    GB_ERROR       error = 0;
    GB_transaction ta(gb_structures);

    sec_assert(Count > 1);

    GBDATA *gb_del = gb_current;
    int     del    = current();
        
    error = next();

    if (!error) {
        int curr = current();
        error    = GB_delete(gb_del);

        if (!error) {
            Count--;
            if (curr >= del) set_current(curr-1);
        }
    }
    if (error) ta.abort();
    return error;
}

const char *SEC_structure_toggler::name() {
    const char     *name = 0;
    GB_transaction  ta(gb_structures);

    GBDATA *gb_name   = GB_search(gb_current, "name", GB_FIND);
    if (gb_name) name = GB_read_char_pntr(gb_name);
    if (!name) error  = GB_get_error();

    if (error) ta.abort();
    return name;
}

GB_ERROR SEC_structure_toggler::setName(GBDATA *gb_struct, const char *new_name) {
    GB_ERROR  error    = 0;
    GBDATA   *gb_name  = GB_search(gb_struct, "name", GB_STRING);
    if (gb_name) error = GB_write_string(gb_name, new_name);
    else      error    = GB_get_error();

    return error;
    
}

GB_ERROR SEC_structure_toggler::setName(const char *new_name) {
    GB_transaction ta(gb_structures);
    GB_ERROR       error = setName(gb_current, new_name);
    if (error) ta.abort();
    return error;
}

