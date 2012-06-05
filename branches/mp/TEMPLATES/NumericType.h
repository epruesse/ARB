// ============================================================ //
//                                                              //
//   File      : NumericType.h                                  //
//   Purpose   : Numeric types w/o auto-cast to base type       //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2012   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef NUMERICTYPE_H
#define NUMERICTYPE_H

template <typename T, int I> 
class NumericType {
    // if you like to have NumericTypes which are not castable into each other,
    // but are based on the same base type T, use different values for I

    T N;

public:
    explicit NumericType(T n) : N(n) {}
    
    const T& base() const { return N; } // explicit conversion (e.g. to pass via ellipse (...))

    operator T() const { return N; }

    bool operator == (const NumericType& other) const { return N == other.N; }
    bool operator != (const NumericType& other) const { return N != other.N; }
    bool operator >= (const NumericType& other) const { return N >= other.N; }
    bool operator <= (const NumericType& other) const { return N <= other.N; }
    bool operator >  (const NumericType& other) const { return N >  other.N; }
    bool operator <  (const NumericType& other) const { return N <  other.N; }

    NumericType& operator++() { ++N; return *this; }
    NumericType& operator--() { --N; return *this; }
};

#else
#error NumericType.h included twice
#endif // NUMERICTYPE_H
