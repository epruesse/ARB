// ================================================================= //
//                                                                   //
//   File      : SEC_toggle.cxx                                      //
//   Purpose   :                                                     //
//   Time-stamp: <Fri Dec/05/2008 18:01 MET Coder@ReallySoft.de>     //
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
    
    GBDATA *gb_new = GB_create_container(gb_structures, "struct");

    if (gb_new) {
        st_error = setName(gb_new, structure_name);

        if (!st_error) st_error = store(gb_new);
        if (!st_error) {
            set_current(Count);
            gb_current = gb_new;
            
            sec_assert(find(current()) == gb_current);
            Count++;
        }
    }
    else st_error = GB_get_error();

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
        st_error   = GB_get_error();
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
        int nextNum = current()+1;
        if (nextNum >= Count) nextNum = 0;

        sec_assert(find(current()) == gb_current);
        
        error = store(gb_current);

        if (!error) {
            set_current(nextNum);
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
    GB_ERROR error = 0;
    GB_transaction ta(gb_structures);

    sec_assert(find(current()) == gb_current);
    
    error = store(gb_current);

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
    return ta.close(error);
}

const char *SEC_structure_toggler::name() {
    const char     *structure_name = 0;
    GB_transaction  ta(gb_structures);

    GBDATA *gb_name               = GB_search(gb_current, "name", GB_FIND);
    if (gb_name) structure_name   = GB_read_char_pntr(gb_name);
    if (!structure_name) st_error = GB_get_error();

    st_error = ta.close(st_error);
    return structure_name;
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
    return ta.close(error);
}

