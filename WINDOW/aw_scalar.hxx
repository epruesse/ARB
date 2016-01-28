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

#pragma once

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif

#include "aw_assert.hxx"

class AW_awar;

class AW_scalar {
private:
    union {
        char    *s;
        int32_t  i;
        float    f;
        GBDATA  *p;
    } value;

    GB_TYPES type;
    
public:
    explicit AW_scalar(int32_t I)     : type(GB_INT)   { value.i = I; }
    explicit AW_scalar(float F)       : type(GB_FLOAT) { value.f = F; }
    explicit AW_scalar(const char *S) : type(GB_STRING)   { value.s = strdup(S); }
    explicit AW_scalar(GBDATA *P)     : type(GB_POINTER)  { value.p = P; }
    explicit AW_scalar(AW_awar *awar);

    explicit AW_scalar(const AW_scalar& other)
        : value(other.value),
          type(other.type)
    {
        if (type == GB_STRING) value.s = strdup(value.s);
    }
    ~AW_scalar() { if (type == GB_STRING) { free(value.s); } }

    DECLARE_ASSIGNMENT_OPERATOR(AW_scalar);

    int32_t     get_int()     const { aw_assert(type == GB_INT);   return value.i; }
    float       get_float()   const { aw_assert(type == GB_FLOAT); return value.f; }
    const char *get_string()  const { aw_assert(type == GB_STRING);   return value.s; }
    GBDATA     *get_pointer() const { aw_assert(type == GB_POINTER);  return value.p; }

    void set_int(int32_t I)        { aw_assert(type == GB_INT);   value.i = I; }
    void set_float(float F)        { aw_assert(type == GB_FLOAT); value.f = F; }
    void set_string(const char *S) { aw_assert(type == GB_STRING);   freedup(value.s, S); }
    void set_pointer(GBDATA *P)    { aw_assert(type == GB_POINTER);  value.p = P; }

    GB_ERROR write_to(AW_awar *awar);

    bool operator == (const AW_scalar& other) const {
        // sync with AW_scalar.cxx@op_equal
        aw_assert(type == other.type); // type mismatch!
        bool equal = false;
        switch (type) {
            case GB_INT:      equal = (value.i == other.value.i);             break;
            case GB_FLOAT:    equal = fabsf(value.f-other.value.f)<0.000001;  break;
            case GB_STRING:   equal = strcmp(value.s, other.value.s) == 0;    break;
            case GB_POINTER:  equal = (value.p == other.value.p);             break;
            default: aw_assert(false);
        }
        return equal;
    }
    bool operator != (const AW_scalar& other) const { return !(*this == other); }

    bool operator == (const AW_awar* const & awar) const;
    bool operator == (AW_awar*& awar) const {
        return operator == (const_cast<const AW_awar*&>(awar));
    }
};
