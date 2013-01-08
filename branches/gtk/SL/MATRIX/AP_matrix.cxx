// =============================================================== //
//                                                                 //
//   File      : AP_matrix.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_matrix.hxx"

#include <arbdbt.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>

#define ap_assert(cond) arb_assert(cond)

// --------------------
//      AP_smatrix

AP_smatrix::AP_smatrix(long si)
{
    m = (AP_FLOAT **)calloc(sizeof(AP_FLOAT *), (size_t)si);
    long i;
    for (i=0; i<si; i++) {
        m[i] = (AP_FLOAT *)calloc(sizeof(AP_FLOAT), (size_t)(i+1));
    }

    size = si;
}

AP_smatrix::~AP_smatrix() {
    for (long i=0; i<size; i++) free(m[i]);
    free(m);
}

// ------------------
//      AP_matrix

void AP_matrix::set_desc(char**& which_desc, int idx, const char *desc) {
    if (!which_desc) which_desc = (char**)GB_calloc(sizeof(char*), size);
    which_desc[idx] = strdup(desc);
}

void AP_matrix::create_awars(AW_root *awr, const char *awar_prefix) {
    char buffer[1024];
    int x, y;
    for (x = 0; x<size; x++) {
        if (x_description[x]) {
            for (y = 0; y<size; y++) {
                if (y_description[y]) {
                    sprintf(buffer, "%s/B%s/B%s", awar_prefix, x_description[x], y_description[y]);
                    if (x==y) {
                        awr->awar_float(buffer, 0)->set_minmax(0.0, 2.0);
                    }
                    else {
                        awr->awar_float(buffer, 1.0)->set_minmax(0.0, 2.0);
                    }
                }

            }
        }
    }
}
void AP_matrix::read_awars(AW_root *awr, const char *awar_prefix) {
    char buffer[1024];
    int x, y;
    for (x = 0; x<size; x++) {
        if (x_description[x]) {
            for (y = 0; y<size; y++) {
                if (y_description[y]) {
                    sprintf(buffer, "%s/B%s/B%s", awar_prefix, x_description[x], y_description[y]);
                    this->set(x, y, awr->awar(buffer)->read_float());
                }
            }
        }
    }
}

void AP_matrix::create_input_fields(AW_window *aww, const char *awar_prefix) {
    char buffer[1024];
    int x, y;
    aww->create_button(0, "    ");
    for (x = 0; x<size; x++) {
        if (x_description[x]) {
            aww->create_button(0, x_description[x]);
        }
    }
    aww->at_newline();
    for (x = 0; x<size; x++) {
        if (x_description[x]) {
            aww->create_button(0, x_description[x]);
            for (y = 0; y<size; y++) {
                if (y_description[y]) {
                    sprintf(buffer, "%s/B%s/B%s", awar_prefix, x_description[x], y_description[y]);
                    aww->create_input_field(buffer, 4);
                }
            }
            aww->at_newline();
        }
    }
}

void AP_matrix::normize() { // set values so that average of non diag elems == 1.0
    int x, y;
    double sum = 0.0;
    double elems = 0.0;
    for (x = 0; x<size; x++) {
        if (x_description[x]) {
            for (y = 0; y<size; y++) {
                if (y!=x && y_description[y]) {
                    sum += this->get(x, y);
                    elems += 1.0;
                }
            }
        }
    }
    if (sum == 0.0) return;
    sum /= elems;
    for (x = 0; x<size; x++) {
        for (y = 0; y<size; y++) {
            this->set(x, y, get(x, y)/sum);
        }
    }
}

AP_matrix::AP_matrix(long si)
    : x_description(NULL),
      y_description(NULL)
{
    m = (AP_FLOAT **)calloc(sizeof(AP_FLOAT *), (size_t)si);
    for (long i=0; i<si; i++) {
        m[i] = (AP_FLOAT *)calloc(sizeof(AP_FLOAT), (size_t)(si));
    }
    size = si;
}

AP_matrix::~AP_matrix() {
    for (long i=0; i<size; i++) {
        free(m[i]);
        if (x_description) free(x_description[i]);
        if (y_description) free(y_description[i]);
    }
    free(x_description);
    free(y_description);
    free(m);
}

