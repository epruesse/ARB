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

/*****************************************************************************************
 ************           Filter                      **********
 *****************************************************************************************/

AP_filter::AP_filter(void){
    memset ((char *)this,0,sizeof(*this));
    int i;
    for (i=0;i<256;i++){
        simplify[i] = i;
    }
}

GB_ERROR AP_filter::init(const char *ifilter, const char *zerobases, long size)
{
    int             i;
    if (!ifilter || !*ifilter) {        // select all
        return this->init(size);
    }

    delete  filter_mask;
    filter_mask = new char[size];
    filter_len = size;
    real_len = 0;
    int slen = strlen(ifilter);
    if (slen>size) slen = size;
    for (i = 0; i < slen; i++) {
        if (zerobases) {
            if (strchr(zerobases, ifilter[i])) {
                filter_mask[i] = 0;
            } else {
                filter_mask[i] = 1;
                real_len++;
            }
        } else {
            if (ifilter[i]) {
                filter_mask[i] = 1;
                real_len++;
            } else {
                filter_mask[i] = 0;
            }
        }
    }
    for (; i < size; i++) {
        filter_mask[i] = 1;
        real_len++;
    }
    update = AP_timer();
    return 0;
}



GB_ERROR AP_filter::init(long size)
{
    int             i;
    delete  filter_mask;
    filter_mask = new char[size];
    real_len = filter_len = size;
    for (i = 0; i < size; i++) {
        filter_mask[i] = 1;
    }
    update = AP_timer();
    return 0;
}

AP_filter::~AP_filter(void){
    delete [] bootstrap;
    delete [] filter_mask;
    delete filterpos_2_seqpos;
}


char *AP_filter::to_string(){
    char *data = new char[filter_len+1];
    data[filter_len] = 0;
    int i;
    for (i=0;i<filter_len;i++){
        if (filter_mask[i]){
            data[i] = '1';
        }else{
            data[i] = '0';
        }
    }
    return data;
}


void AP_filter::enable_simplify(AWT_FILTER_SIMPLIFY type){
    int i;
    for (i=0;i<32;i++){
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
            af_assert(0);
            break;
        case AWT_FILTER_SIMPLIFY_NONE:
            break;
    }
}

void AP_filter::calc_filter_2_seq(){
    delete filterpos_2_seqpos;
    filterpos_2_seqpos = new int[real_len];
    int i;
    int j = 0;
    for (i=0;i<filter_len;i++){
        if (filter_mask[i]){
            filterpos_2_seqpos[j++] = i;
        }
    }
}

void AP_filter::enable_bootstrap(){
    delete [] bootstrap;
    bootstrap = new int[real_len];

    af_assert(filter_len < RAND_MAX);

    for (int i = 0; i<this->real_len; i++){
        int r = GB_random(filter_len);
        af_assert(r >= 0);     // otherwise overflow in random number generator
        bootstrap[i] = r;
    }
}

/*****************************************************************************************
 ************           Weights                         **********
 *****************************************************************************************/

AP_weights::AP_weights(void) {
    memset ((char *)this,0,sizeof(AP_weights));
}

char *AP_weights::init(const AP_filter *fil)
{
    int i;
    if (fil->update<= this->update) return 0;

    weight_len = fil->real_len;
    delete weights;
    weights = new GB_UINT4[weight_len];
    for (i=0;i<weight_len;i++) {
        weights[i] = 1;
    }
    this->dummy_weights = 1;
    this->update = fil->update;
    return 0;
}

char *AP_weights::init(GB_UINT4 *w, const AP_filter *fil)
{
    int i,j;
    if (fil->update<= this->update) return 0;

    weight_len = fil->real_len;
    delete weights;
    weights = new GB_UINT4[weight_len];
    for (j=i=0;i<weight_len;j++) {
        if (fil->filter_mask[j]){
            weights[i++] = w[j];
        }
    }
    this->update = fil->update;
    return 0;}

AP_weights::~AP_weights(void)
{
    delete [] weights;
}

long AP_timer(void) {
    static long time = 0;
    return ++time;
}


