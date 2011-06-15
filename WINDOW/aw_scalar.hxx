// ============================================================ //
//                                                              //
//   File      : aw_scalar.hxx                                  //
//   Purpose   : Scalar variables (similar to perl scalars)     //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AW_SCALAR_HXX
#define AW_SCALAR_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif


#ifndef aw_assert
# define aw_assert(cond) arb_assert(cond)
#endif

class AW_scalar {
    union {
        char  *s;
        int    i;
        float  f;
        void  *p;
    } value;

    enum { INT, FLOAT, STR, PNTR } type;
public:
    explicit AW_scalar(int I) : type(INT) { value.i = I; }
    explicit AW_scalar(float F) : type(FLOAT) { value.f = F; }
    explicit AW_scalar(const char *S) : type(STR) { value.s = strdup(S); }
    explicit AW_scalar(void *P) : type(PNTR) { value.p = P; }
    explicit AW_scalar(class AW_awar *awar);

    ~AW_scalar() { if (type == STR) { free(value.s); } }

    int get_int() const { aw_assert(type == INT); return value.i; }
    float get_float() const { aw_assert(type == FLOAT); return value.f; }
    const char *get_string() const { aw_assert(type == STR); return value.s; }
    void *get_pointer() const { aw_assert(type == PNTR); return value.p; }

    bool operator == (const AW_scalar& other) const {
        aw_assert(type == other.type); // type mismatch!
        bool equal = false;
        switch (type) {
            case INT:   equal = (value.i == other.value.i); break;
            case FLOAT: equal = fabs(value.f-other.value.f)<0.000001; break;
            case STR:   equal = strcmp(value.s, other.value.s) == 0; break;
            case PNTR:  equal = (value.p == other.value.p); break;
        }
        return equal;
    }
    bool operator != (const AW_scalar& other) const { return !(*this == other); }
};


#else
#error aw_scalar.hxx included twice
#endif // AW_SCALAR_HXX
