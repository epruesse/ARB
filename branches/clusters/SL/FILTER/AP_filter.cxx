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
#include <inline.h>


// ------------------
//      AP_filter

AP_filter::AP_filter()
    : filter_mask(NULL)
    , filter_len(0)
    , real_len(0)
    , update(0)
    , simplify_type(AWT_FILTER_SIMPLIFY_NOT_INITIALIZED)
    , bootstrap(NULL)
    , filterpos_2_seqpos(NULL)
{
}

AP_filter::AP_filter(const AP_filter& other)
    : filter_mask(new bool[other.filter_len])
    , filter_len(other.filter_len)
    , real_len(other.real_len)
    , update(other.update)
    , simplify_type(other.simplify_type)
    , bootstrap(NULL)
    , filterpos_2_seqpos(NULL)
{
    memcpy(filter_mask, other.filter_mask, filter_len*sizeof(*filter_mask));
    memcpy(simplify, other.simplify, sizeof(simplify)*sizeof(*simplify));
    if (other.bootstrap) {
        bootstrap = new size_t[filter_len];
        memcpy(bootstrap, other.bootstrap, filter_len*sizeof(*bootstrap));
    }
    if (other.filterpos_2_seqpos) {
        filterpos_2_seqpos = new size_t[real_len];
        memcpy(filterpos_2_seqpos, other.filterpos_2_seqpos, real_len*sizeof(*filterpos_2_seqpos));
    }
}


AP_filter::~AP_filter(void) {
    delete [] bootstrap;
    delete [] filter_mask;
    delete [] filterpos_2_seqpos;
}

void AP_filter::resize(size_t newLen) {
    delete [] filter_mask;
    filter_mask = new bool[newLen];
    filter_len  = newLen;

    delete [] bootstrap;                bootstrap          = NULL;
    delete [] filterpos_2_seqpos;       filterpos_2_seqpos = NULL;
}

void AP_filter::init(const char *ifilter, const char *zerobases, size_t size) {
    if (!ifilter || !*ifilter) {                    // select all
        init(size);
    }
    else {
        resize(size);

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
        update = AP_timer();
    }
}



void AP_filter::init(size_t size) {
    resize(size);
    real_len = filter_len;
    for (size_t i = 0; i < size; i++) filter_mask[i] = true;
    update = AP_timer();
}


char *AP_filter::to_string() const {
    char *data = new char[filter_len+1];

    for (size_t i=0; i<filter_len; ++i) {
        data[i] = "01"[filter_mask[i]];
    }
    data[filter_len] = 0;
    
    return data;
}


void AP_filter::enable_simplify(AWT_FILTER_SIMPLIFY type) {
    if (type != simplify_type) {
        int i;
        for (i=0;i<32;i++) {
            simplify[i] = '.';
        }
        for (;i<256;i++){
            simplify[i] = i;
        }
        switch (type){
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
            default :
                af_assert(0);
                break;
        }

        simplify_type = type;
    }
}

void AP_filter::calc_filterpos_2_seqpos(){
    delete filterpos_2_seqpos;
    filterpos_2_seqpos = new size_t[real_len];
    size_t i, j;
    for (i=j=0; i<filter_len; ++i) {
        if (filter_mask[i]) {
            filterpos_2_seqpos[j++] = i;
        }
    }
}

void AP_filter::enable_bootstrap(){
    delete [] bootstrap;
    bootstrap = new size_t[real_len];

    af_assert(filter_len < RAND_MAX);

    for (size_t i = 0; i<real_len; ++i) {
        int r = GB_random(filter_len);
        af_assert(r >= 0);     // otherwise overflow in random number generator
        bootstrap[i] = r;
    }
}

// -------------------
//      AP_weights

AP_weights::AP_weights() {
    memset(this, 0, sizeof(AP_weights));
}

AP_weights::AP_weights(const AP_weights& other)
    : weights(new GB_UINT4[other.weight_len])
    , weight_len(other.weight_len)
    , alloc_len(other.weight_len)
    , update(other.update)
{
    memcpy(weights, other.weights, weight_len*sizeof(*weights));
}

AP_weights::~AP_weights() {
    delete [] weights;
}

void AP_weights::resize(size_t newLen) {
    af_assert(newLen>0);
    if (newLen>alloc_len) {
        delete [] weights;
        weights   = new GB_UINT4[newLen];
        alloc_len = newLen;
    }
    weight_len = newLen;
}

void AP_weights::init(const AP_filter *fil) {
    if (fil->get_timestamp() > update) {
        resize(fil->get_filtered_length());
        for (size_t i=0; i<weight_len; ++i) {
            weights[i] = 1;
        }
        update = fil->get_timestamp();
    }
}

void AP_weights::init(GB_UINT4 *w, size_t len, const AP_filter *fil) {
    af_assert(len == fil->get_length());

    if (fil->get_timestamp() > this->update) {
        resize(fil->get_filtered_length());

        size_t i,j;
        for (j=i=0; i<weight_len; ++j) {
            if (fil->use_position(j)) {
                weights[i++] = w[j];
            }
        }
        af_assert(j <= fil->get_length());
        af_assert(i == fil->get_filtered_length());
        update = fil->get_timestamp();
    }
}

long AP_timer(void) {
    static long time = 0;
    return ++time;
}


