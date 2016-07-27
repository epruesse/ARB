// =============================================================== //
//                                                                 //
//   File      : AP_filter.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_filter.hxx"
#include <arbdb.h>
#include <arb_mem.h>


// ------------------
//      AP_filter

void AP_filter::init(size_t size) {
    filter_mask        = new bool[size];
    filter_len         = size;
    update             = AP_timer();
    simplify_type      = AWT_FILTER_SIMPLIFY_NOT_INITIALIZED;
    simplify[0]        = 0; // silence cppcheck-warning
    bootstrap          = NULL;
    filterpos_2_seqpos = NULL;
#if defined(ASSERTION_USED)
    checked_for_validity = false;
#endif
}


AP_filter::AP_filter(size_t size) {
    make_permeable(size);
}

AP_filter::AP_filter(const AP_filter& other)
    : filter_mask(new bool[other.filter_len]),
      filter_len(other.filter_len),
      real_len(other.real_len),
      update(other.update),
      simplify_type(other.simplify_type),
      bootstrap(NULL),
      filterpos_2_seqpos(NULL)
{
    memcpy(filter_mask, other.filter_mask, filter_len*sizeof(*filter_mask));
    memcpy(simplify, other.simplify, sizeof(simplify)*sizeof(*simplify));
    if (other.bootstrap) {
        bootstrap = new size_t[real_len];
        memcpy(bootstrap, other.bootstrap, real_len*sizeof(*bootstrap));
    }
    if (other.filterpos_2_seqpos) {
        filterpos_2_seqpos = new size_t[real_len];
        memcpy(filterpos_2_seqpos, other.filterpos_2_seqpos, real_len*sizeof(*filterpos_2_seqpos));
    }
#if defined(ASSERTION_USED)
    checked_for_validity = other.checked_for_validity;
#endif
}

AP_filter::~AP_filter() {
    delete [] bootstrap;
    delete [] filter_mask;
    delete [] filterpos_2_seqpos;
}

AP_filter::AP_filter(const char *ifilter, const char *zerobases, size_t size) {
    if (!ifilter || !*ifilter) {
        make_permeable(size);
    }
    else {
        init(size);

        bool   char2mask[256];
        size_t i;

        for (i = 0; i<256; ++i) char2mask[i] = true;
        if (zerobases) {
            for (i = 0; zerobases[i]; ++i) char2mask[safeCharIndex(zerobases[i])] = false;
        }
        else {
            char2mask['0'] = false;
        }

        real_len = 0;
        for (i = 0; i < size && ifilter[i]; ++i) {
            real_len += int(filter_mask[i] = char2mask[safeCharIndex(ifilter[i])]);
        }
        for (; i < size; i++) {
            filter_mask[i] = true;
            real_len++;
        }
    }
}

void AP_filter::make_permeable(size_t size) {
    init(size);
    real_len = filter_len;
    for (size_t i = 0; i < size; i++) filter_mask[i] = true;
}

char *AP_filter::to_string() const {
    af_assert(checked_for_validity);

    char *data = (char*)malloc(filter_len+1);

    for (size_t i=0; i<filter_len; ++i) {
        data[i] = "01"[filter_mask[i]];
    }
    data[filter_len] = 0;

    return data;
}


void AP_filter::enable_simplify(AWT_FILTER_SIMPLIFY type) {
    if (type != simplify_type) {
        int i;
        for (i=0; i<32; i++) {
            simplify[i] = '.';
        }
        for (; i<256; i++) {
            simplify[i] = i;
        }
        switch (type) {
            case AWT_FILTER_SIMPLIFY_DNA:
                simplify[(unsigned char)'g'] = 'a';
                simplify[(unsigned char)'G'] = 'A';
                simplify[(unsigned char)'u'] = 'c';
                simplify[(unsigned char)'t'] = 'c';
                simplify[(unsigned char)'U'] = 'C';
                simplify[(unsigned char)'T'] = 'C';
                break;
            case AWT_FILTER_SIMPLIFY_PROTEIN:
                af_assert(0);                           // not implemented or impossible!?
                break;
            case AWT_FILTER_SIMPLIFY_NONE:
                break;
            default:
                af_assert(0);
                break;
        }

        simplify_type = type;
    }
}

void AP_filter::calc_filterpos_2_seqpos() {
    af_assert(checked_for_validity);
    af_assert(real_len>0);

    delete [] filterpos_2_seqpos;
    filterpos_2_seqpos = new size_t[real_len];
    size_t i, j;
    for (i=j=0; i<filter_len; ++i) {
        if (filter_mask[i]) {
            filterpos_2_seqpos[j++] = i;
        }
    }
}

void AP_filter::enable_bootstrap() {
    af_assert(checked_for_validity);
    af_assert(real_len>0);

    delete [] bootstrap;
    bootstrap = new size_t[real_len];

    af_assert(filter_len < RAND_MAX);

    for (size_t i = 0; i<real_len; ++i) {
        int r = GB_random(real_len);
        af_assert(r >= 0);     // otherwise overflow in random number generator
        bootstrap[i] = r;
    }
}

char *AP_filter::blowup_string(char *filtered_string, char fillChar) const {
    /*! blow up 'filtered_string' to unfiltered length
     * by inserting 'fillChar' at filtered positions
     */
    af_assert(checked_for_validity);

    char   *blownup = (char*)malloc(filter_len+1);
    size_t  f       = 0;

    for (size_t i = 0; i<filter_len; ++i) {
        blownup[i] = use_position(i) ? filtered_string[f++] : fillChar;
    }

    return blownup;
}


// -------------------
//      AP_weights

AP_weights::AP_weights(const AP_filter *fil)
    : len(fil->get_filtered_length()),
      weights(NULL)
{
}

AP_weights::AP_weights(const GB_UINT4 *w, size_t wlen, const AP_filter *fil)
    : len(fil->get_filtered_length()),
      weights(NULL)
{
    arb_alloc_aligned(weights, len);

    af_assert(wlen == fil->get_length());

    size_t i, j;
    for (j=i=0; j<wlen; ++j) {
        if (fil->use_position(j)) {
            weights[i++] = w[j];
        }
    }
    af_assert(j <= fil->get_length());
    af_assert(i == fil->get_filtered_length());
}

AP_weights::AP_weights(const AP_weights& other)
    : len(other.len),
      weights(NULL)
{
    if (other.weights != NULL) {
        arb_alloc_aligned(weights, len);
        memcpy(weights, other.weights, len*sizeof(*weights));
    }
}

AP_weights::~AP_weights() {
    free(weights);
}

long AP_timer() {
    static long time = 0;
    return ++time;
}


