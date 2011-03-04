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

typedef double AP_FLOAT;

class AW_root;
class AW_window;

class AP_smatrix {      // Symmetrical Matrix (upper triangular matrix)
public:
    AP_FLOAT **m;       // m[i][j]  i<= j !!!!
    long    size;
    AP_smatrix(long si);
    ~AP_smatrix();
    void    set(long i, long j, AP_FLOAT val) { if (i>j) m[i][j] = val; else m[j][i] = val; };
    AP_FLOAT get(long i, long j) { if (i>j) return m[i][j]; else return m[j][i]; };
};

class AP_matrix {       // Matrix
public:
    AP_FLOAT **m;
    char    **x_description; // optional description, strdupped
    char    **y_description;
    long    size;
    AP_matrix(long si);
    ~AP_matrix();
    void create_awars(AW_root *awr, const char *awar_prefix);
    void read_awars(AW_root *awr, const char *awar_prefix);
    void normize();     // set average non diag element to 1.0 (only for described elements)
    void create_input_fields(AW_window *aww, const char *awar_prefix);
    void set_description(const char *xstring, const char *ystring);
    void    set(int i, int j, AP_FLOAT val) { m[i][j] = val; };
    AP_FLOAT get(int i, int j) { return m[i][j]; };
};

#else
#error AP_matrix.hxx included twice
#endif // AP_MATRIX_HXX
