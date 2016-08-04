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
#include <cfloat>

#define ap_assert(cond) arb_assert(cond)

// --------------------
//      AP_smatrix

AP_smatrix::AP_smatrix(size_t si)
    : Size(si)
{
    size_t elements = Size*(Size+1)/2;
    size_t headsize = Size * sizeof(*m);
    size_t datasize = elements * sizeof(*(m[0]));

    m    = (AP_FLOAT**)calloc(1, headsize+datasize);
    m[0] = (AP_FLOAT*)(((char*)m)+headsize);

    for (size_t i=1; i<si; i++) {
        m[i] = m[i-1]+i;
    }
}

AP_smatrix::~AP_smatrix() {
    free(m);
}

AP_FLOAT AP_smatrix::get_max_value() const { // O(n*2)
    AP_FLOAT max = -FLT_MAX;
    for (size_t i=0; i<Size; i++) {
        for (size_t j=0; j<i; j++) {
            if (m[i][j] > max) max = m[i][j];
        }
    }
    return max;
}

// ------------------
//      AP_matrix

AP_matrix::AP_matrix(long si) {
    m = (AP_FLOAT **)calloc(sizeof(AP_FLOAT *), (size_t)si);
    for (long i=0; i<si; i++) {
        m[i] = (AP_FLOAT *)calloc(sizeof(AP_FLOAT), (size_t)(si));
    }
    size = si;
}

AP_matrix::~AP_matrix() {
    for (long i=0; i<size; i++) {
        free(m[i]);
    }
    free(m);
}

// ---------------------------
//      AP_userdef_matrix

void AP_userdef_matrix::set_desc(char**& which_desc, int idx, const char *desc) {
    if (!which_desc) which_desc = (char**)ARB_calloc(sizeof(char*), get_size());
    which_desc[idx] = strdup(desc);
}

void AP_userdef_matrix::create_awars(AW_root *awr) {
    char buffer[1024];
    int x, y;
    for (x = 0; x<get_size(); x++) {
        if (x_description[x]) {
            for (y = 0; y<get_size(); y++) {
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
void AP_userdef_matrix::update_from_awars(AW_root *awr) {
    char buffer[1024];
    int x, y;
    for (x = 0; x<get_size(); x++) {
        if (x_description[x]) {
            for (y = 0; y<get_size(); y++) {
                if (y_description[y]) {
                    sprintf(buffer, "%s/B%s/B%s", awar_prefix, x_description[x], y_description[y]);
                    this->set(x, y, awr->awar(buffer)->read_float());
                }
            }
        }
    }
}

void AP_userdef_matrix::create_input_fields(AW_window *aww) {
    char buffer[1024];
    int x, y;
    aww->create_button(0, "    ");
    for (x = 0; x<get_size(); x++) {
        if (x_description[x]) {
            aww->create_button(0, x_description[x]);
        }
    }
    aww->at_newline();
    for (x = 0; x<get_size(); x++) {
        if (x_description[x]) {
            aww->create_button(0, x_description[x]);
            for (y = 0; y<get_size(); y++) {
                if (y_description[y]) {
                    sprintf(buffer, "%s/B%s/B%s", awar_prefix, x_description[x], y_description[y]);
                    aww->create_input_field(buffer, 4);
                }
            }
            aww->at_newline();
        }
    }
}

void AP_userdef_matrix::normize() { // set values so that average of non diag elems == 1.0
    int x, y;
    double sum = 0.0;
    double elems = 0.0;
    for (x = 0; x<get_size(); x++) {
        if (x_description[x]) {
            for (y = 0; y<get_size(); y++) {
                if (y!=x && y_description[y]) {
                    sum += this->get(x, y);
                    elems += 1.0;
                }
            }
        }
    }
    if (sum == 0.0) return;
    sum /= elems;
    for (x = 0; x<get_size(); x++) {
        for (y = 0; y<get_size(); y++) {
            this->set(x, y, get(x, y)/sum);
        }
    }
}

AP_userdef_matrix::~AP_userdef_matrix() {
    for (long i=0; i<get_size(); i++) {
        if (x_description) free(x_description[i]);
        if (y_description) free(y_description[i]);
    }
    free(x_description);
    free(y_description);
    free(awar_prefix);
}

