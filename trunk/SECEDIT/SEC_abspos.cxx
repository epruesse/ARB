// =============================================================== //
//                                                                 //
//   File      : SEC_abspos.cxx                                    //
//   Purpose   : Encapsulates helix position access                //
//   Time-stamp: <Fri Sep/07/2007 09:03 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cstdlib>
#include <cstring>

#include "SEC_abspos.hxx"

using namespace std;

void XString::set_length(size_t len) {
    if (number_found && x_string_len<len) {
        free(number_found);
        number_found = 0;
    }
    x_string_len = len;
    initialized  = false;
}

XString::XString(size_t ali_length)
    : abspos(0)
    , number_found(0)
{
    int len = ali_length+1; // need one more (cause 'x's are written behind position)
    x_string = (char*)malloc((len+1)*sizeof(*x_string));
    memset(x_string, '.', len);
    x_string[len] = 0;
    set_length(len);
}

XString::XString(const char *saved_x_string, size_t saved_length, size_t ali_length) 
    : abspos(0)
    , number_found(0)
{
    size_t xlen = ali_length+1;
    
    sec_assert(saved_length == strlen(saved_x_string));
    sec_assert(saved_length == xlen || saved_length == xlen-1);

    x_string = (char*)malloc((xlen+1)*sizeof(*x_string));
    memcpy(x_string, saved_x_string, saved_length+1);

    if (saved_length == xlen-1) { // normal case
        x_string[xlen-1] = '.'; // additional position is a gap (SAI 'HELIX' should have a gap at end)
        x_string[xlen]   = 0;   // (see also comments in get_x_string())
    }

    set_length(xlen);
    initialize();
}

void XString::initialize()
{
    // detect number of 'x's in x_string :
    {
        size_t len = 0;
        int    x   = 0;
        
        while (char c = x_string[len]) {
            if (c == 'x') x++;
            len++;
        }

        sec_assert(len == x_string_len);

        if (abspos) { // re-initialization
            if (x_count<x) { // abspos array too small
                free(abspos);
                abspos = 0;
            }
        }
        x_count = x;
    }

    if (!abspos)       abspos       = (size_t*)malloc(x_count * sizeof(*abspos));
    if (!number_found) number_found = (int*)malloc(x_string_len * sizeof(*number_found));

    // init abspos and number_found : 
    {
        int pos = 0;
        int x   = 0;

        while (char c = x_string[pos]) {
            number_found[pos] = x;
            if (c == 'x') {
                abspos[x] = pos;
                x++;
            }
            pos++;
        }
    }

    initialized = true;
}

XString::~XString()
{
    free(x_string);
    free(abspos);
    free(number_found);
}

size_t XString::getAbsPos(int x) const
{
    sec_assert(initialized);
    sec_assert(x >= 0 && x<x_count);
    size_t pos = abspos[x];
    sec_assert(pos<x_string_len);
    return pos;
}

int XString::getXleftOf(size_t pos) const
{
    sec_assert(initialized);
    sec_assert(pos<x_string_len);
    int x = number_found[pos];
    sec_assert(x >= 0 && x<x_count);
    return x;
}

const char *XString::get_x_string() const {
    sec_assert(initialized);

    static char   *copy       = 0;
    static size_t  copy_alloc = 0;

    size_t bufsize = x_string_len+1; 

    if (copy && copy_alloc<bufsize) {
        free(copy);
        copy = 0;
    }

    if (!copy) {
        copy       = (char*)malloc(bufsize);
        copy_alloc = bufsize;
    }

    memcpy(copy, x_string, x_string_len+1);

    int add_pos = x_string_len-1;

    if (copy[add_pos] == '.') { // can be removed - added again after reload
        copy[add_pos] = 0; // hide internal additional position
    }
    else {
        // happens only if there's a helix on last alignment position.
        // In this case we save the additional position to DB, which will
        // lead the user to reformat his alignment (which will add a gap
        // at end of SAI HELIX)
    }
    return copy;
}

