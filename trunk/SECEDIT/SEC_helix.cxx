// =============================================================== //
//                                                                 //
//   File      : SEC_helix.cxx                                     //
//   Purpose   : helix related things                              //
//   Time-stamp: <Fri Sep/07/2007 09:01 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cstring>

#include "SEC_root.hxx"
#include "SEC_helix.hxx"

using namespace std;

const size_t *SEC_root::getHelixPositions(const char *helixNr) const {
    sec_assert(helixNr);
    const BI_helix *helix  = get_helix();
    long            start1 = helix->first_position(helixNr);

    if (start1 == -1) return 0;

    size_t        start2 = helix->last_position(helixNr);
    static size_t pos[4];

    if (size_t(start1)<start2) {
        pos[0] = size_t(start1);
        pos[1] = start2;
    }
    else {
        pos[0] = start2;
        pos[1] = size_t(start1);
    }

    pos[2] = helix->opposite_position(pos[1]);
    pos[3] = helix->opposite_position(pos[0]);

    return pos;
}

// -------------------------------------------
// current way to save folded helices:
//
// save a list of helix-numbers (with sign) seperated by ';'
// strand start-positions are saved as 'num'
// segment start-positions are saved as '!num'

char *SEC_xstring_to_foldedHelixList(const char *x_string, size_t xlength, const BI_helix *helix) {
    void *out = GBS_stropen(1000);

    bool next_is_start = true;

    for (size_t pos = 0; pos<xlength; ++pos) {
        if (x_string[pos] == 'x') {
            BI_PAIR_TYPE  pairType = helix->pairtype(pos);
            const char   *helixNr  = helix->helixNr(pos);

            if (next_is_start) {
                sec_assert(pairType != HELIX_NONE);
                GBS_strcat(out, helixNr);
            }
            else {
                bool is_end_and_start = pairType != HELIX_NONE;
                
                sec_assert(helix->pairtype(pos-1) != HELIX_NONE);
                GBS_strnprintf(out, 20, "!%s", helix->helixNr(pos-1));

                if (is_end_and_start) {
                    GBS_chrcat(out, ';');
                    GBS_strcat(out, helixNr);
                    next_is_start = !next_is_start;
                }
            }
            next_is_start = !next_is_start;
            GBS_chrcat(out, ';');
        }
    }

    return GBS_strclose(out);
}

char *SEC_foldedHelixList_to_xstring(const char *foldedHelices, size_t xlength, const BI_helix *helix, GB_ERROR& error) {
    // example of foldedHelices: '-5;!-5;-8;!-8;-9;!-9;9;!9;8;!8;5;!5;-18;!-18;18;!18;-19a;!-19a;19a;!19a;-21;!-21;-24;!-24;24;!24;21;!21;-27;!-27;27;!27;-31;!-31;-43;!-43;43;!43;-46;!-46;46;!46;31;!31;-50;!-50;50;!50;'
    char *xstring    = (char*)malloc(xlength+1);
    memset(xstring, '.', xlength);
    xstring[xlength] = 0;

    sec_assert(error == 0); 
    error = 0;

    while (!error) {
        const char *semi = strchr(foldedHelices, ';');
        if (!semi) break;

        char       *taggedHelixNr = GB_strpartdup(foldedHelices, semi-1);
        const char *helixNr       = 0;
        long        xpos          = -1;

        if (taggedHelixNr[0] == '!') { // position behind end of helix
            helixNr = taggedHelixNr+1;
            xpos    = helix->last_position(helixNr);
            if (xpos >= 0) ++xpos;
        }
        else { // position at start of helix
            helixNr = taggedHelixNr;
            xpos    = helix->first_position(helixNr);
        }

        if (xpos == -1) {
            error = GBS_global_string("Can't find helix '%s'", helixNr);
        }
        else {
            sec_assert(xpos >= 0 && size_t(xpos) < xlength);
            xstring[xpos] = 'x';
        }

        free(taggedHelixNr);

        foldedHelices = semi+1;
    }

    return xstring;
}

// ------------------------------------------------------------------------
//      xstring was made helix-relative, when saved to old version file
// ------------------------------------------------------------------------


char *old_decode_xstring_rel_helix(GB_CSTR rel_helix, size_t xlength, const BI_helix *helix, int *no_of_helices_ptr)
{
    const char *start_helix_nr = 0;
    int         no_of_helices  = 0;
    int         start_helix    = 0;
    int         end_helix      = -1;
    int         rel_pos        = 0;
    size_t      lastpos        = helix->size()-1;

    char *x_buffer    = (char*)malloc(xlength+1);
    memset(x_buffer, '.', xlength);
    x_buffer[xlength] = 0;

    for (size_t pos=0; ; pos++) {
        const char   *helix_nr = 0;
        BI_PAIR_TYPE  pairType = helix->pairtype(pos);

        if (pairType!=HELIX_NONE) {
            helix_nr = helix->helixNr(pos);

            if (helix_nr==start_helix_nr) { // same helix as last
                end_helix = pos;
            }
            else { // new helix nr
                if (start_helix_nr) { // not first helix -> write last to xstring
                insert_helix:
                    helix_nr = helix->helixNr(pos); // re-init (needed in case of goto insert_helix)
                    char flag = rel_helix[rel_pos++];

                    no_of_helices++;
                    if (flag=='1') {
                        sec_assert(end_helix!=-1);
                        sec_assert(size_t(start_helix)<xlength);
                        sec_assert(size_t(end_helix+1)<xlength);

                        x_buffer[start_helix] = 'x';
                        x_buffer[end_helix+1] = 'x';
                    }
                    else if (flag!='0') {
                        if (flag==0) break; // eos

                        sec_assert(0); // illegal character
                        
                        break;
                    }
                    if (pos==lastpos) {
                        break;
                    }
                }
                start_helix = pos;
                end_helix = pos;
                start_helix_nr = helix_nr;
            }
        }
        if (pos==lastpos) {
            if (start_helix_nr) {
                goto insert_helix;
            }
        }
    }

    *no_of_helices_ptr = no_of_helices;
    return x_buffer;
}
