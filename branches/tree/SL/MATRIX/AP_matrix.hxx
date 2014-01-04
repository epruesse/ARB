// =============================================================== //
//                                                                 //
//   File      : AP_matrix.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AP_MATRIX_HXX
#define AP_MATRIX_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ap_assert(cond) arb_assert(cond)

typedef double AP_FLOAT;

class AW_root;
class AW_window;

class AP_smatrix : virtual Noncopyable {
    // Symmetrical Matrix (upper triangular matrix)

    size_t     Size;
    AP_FLOAT **m;       // m[i][j]  i <= j !!!!

public:

    explicit AP_smatrix(size_t si);
    ~AP_smatrix();

    void     set(size_t i, size_t j, AP_FLOAT val) { if (i>j) m[i][j] = val; else m[j][i] = val; };

    AP_FLOAT fast_get(size_t i, size_t j) const { ap_assert(i>=j); return m[i][j]; };
    AP_FLOAT get(size_t i, size_t j) const { if (i>j) return m[i][j]; else return m[j][i]; };

    AP_FLOAT get_max_value() const;

    size_t size() const { return Size; }
};

class AP_matrix : virtual Noncopyable {
    AP_FLOAT **m;
    long       size;
    char     **x_description;                                                     // optional description, strdupped
    char     **y_description;

    void set_desc(char**& which_desc, int idx, const char *desc);

public:
    AP_matrix(long si);
    ~AP_matrix();

    void create_awars(AW_root *awr, const char *awar_prefix);
    void read_awars(AW_root *awr, const char *awar_prefix);
    void normize();                                                                     // set average non diag element to 1.0 (only for described elements)
    void create_input_fields(AW_window *aww, const char *awar_prefix);

    void set_x_description(int idx, const char *desc) { set_desc(x_description, idx, desc); }
    void set_y_description(int idx, const char *desc) { set_desc(y_description, idx, desc); }
    void set_descriptions(int idx, const char *desc) { set_x_description(idx, desc); set_y_description(idx, desc); }

    void     set(int i, int j, AP_FLOAT val) { m[i][j] = val; };
    AP_FLOAT get(int i, int j) { return m[i][j]; };
};

#else
#error AP_matrix.hxx included twice
#endif // AP_MATRIX_HXX
