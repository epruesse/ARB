// ============================================================== //
//                                                                //
//   File      : ttypes.h                                         //
//   Purpose   : template type inspection                         //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2011   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef TTYPES_H
#define TTYPES_H

// according to C++ templates by Vandevoorde/Josuttis
// thank you for that great book

#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#define NOisnotYES() enum { No = !Yes }

// --------------------------
//      fundamental types

template<typename T> class IsFundaT { public: enum { Yes = 0,  No = 1 }; };
#define DECL_FUNDA_TYPE(t) template<> class IsFundaT<t> { public: enum { Yes = 1,  No = 0 }; };
#define DECL_FUNDA_TYPE_SIGNED(t) DECL_FUNDA_TYPE(signed t); DECL_FUNDA_TYPE(unsigned t)

DECL_FUNDA_TYPE(bool);
DECL_FUNDA_TYPE(char);
DECL_FUNDA_TYPE_SIGNED(char);
DECL_FUNDA_TYPE_SIGNED(int);
DECL_FUNDA_TYPE_SIGNED(long);
DECL_FUNDA_TYPE_SIGNED(long long);
DECL_FUNDA_TYPE(float);
DECL_FUNDA_TYPE(double);
DECL_FUNDA_TYPE(long double);

#undef DECL_FUNDA_TYPE_SIGNED
#undef DECL_FUNDA_TYPE

// -----------------------
//      function types

template<typename T>
class IsFunctionT {
private:
    typedef char yes;
    typedef struct { char a[2]; } no;
public:
    template<typename U> static yes test(...);
    template<typename U> static no  test(U (*)[1]);
    enum { Yes = sizeof(test<T>(0)) == sizeof(yes) };
    NOisnotYES();
};

#define NO_FUNCTION_TYPE(t) class IsFunctionT<t> { public: enum { Yes = 0 }; NOisnotYES(); }

template<typename T> NO_FUNCTION_TYPE(T&);
template<> NO_FUNCTION_TYPE(void);
template<> NO_FUNCTION_TYPE(void const);
template<> NO_FUNCTION_TYPE(void volatile);
template<> NO_FUNCTION_TYPE(void const volatile);

#undef NO_FUNCTION_TYPE

// -----------------------
//      compound types

template<typename T>
struct CompountT { // primary template
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 0, IsFuncT = IsFunctionT<T>::Yes, IsPtrMemT = 0 };
    typedef T BaseT;
    typedef T BottomT;
    typedef CompountT<void> ClassT;
};
template<typename T>
struct CompountT<T*> { // specialization for pointers
    enum { IsPtrT = 1, IsRefT = 0, IsArrayT = 0, IsFuncT = 0, IsPtrMemT = 0 };
    typedef T BaseT;
    typedef typename CompountT<T>::BottomT BottomT;
    typedef CompountT<void> ClassT;
};
template<typename T>
struct CompountT<T&> { // specialization for references
    enum { IsPtrT = 0, IsRefT = 1, IsArrayT = 0, IsFuncT = 0, IsPtrMemT = 0 };
    typedef T BaseT;
    typedef typename CompountT<T>::BottomT BottomT;
    typedef CompountT<void> ClassT;
};
template<typename T, size_t N>
struct CompountT<T[N]> { // specialization for arrays
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 1, IsFuncT = 0, IsPtrMemT = 0 };
    typedef T BaseT;
    typedef typename CompountT<T>::BottomT BottomT;
    typedef CompountT<void> ClassT;
};
template<typename T>
struct CompountT<T[]> { // specialization for empty arrays
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 1, IsFuncT = 0, IsPtrMemT = 0 };
    typedef T BaseT;
    typedef typename CompountT<T>::BottomT BottomT;
    typedef CompountT<void> ClassT;
};
template<typename T, typename C>
struct CompountT<T C::*> { // specialization for pointers to members
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 0, IsFuncT = 0, IsPtrMemT = 1 };
    typedef T BaseT;
    typedef typename CompountT<T>::BottomT BottomT;
    typedef C ClassT;
};

template<typename R>
struct CompountT<R()> { // specialization for functions ()
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 0, IsFuncT = 1, IsPtrMemT = 0 };
    typedef R BaseT();
    typedef R BottomT();
    typedef CompountT<void> ClassT;
};
template<typename R, typename P1>
struct CompountT<R(P1)> { // specialization for functions (P1)
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 0, IsFuncT = 1, IsPtrMemT = 0 };
    typedef R BaseT(P1);
    typedef R BottomT(P1);
    typedef CompountT<void> ClassT;
};
template<typename R, typename P1>
struct CompountT<R(P1, ...)> { // specialization for functions (P1, ...)
    enum { IsPtrT = 0, IsRefT = 0, IsArrayT = 0, IsFuncT = 1, IsPtrMemT = 0 };
    typedef R BaseT(P1);
    typedef R BottomT(P1);
    typedef CompountT<void> ClassT;
};

// -------------------
//      enum types

struct SizeOverOne { char c[2]; };

template<typename T, bool convert_possible = !CompountT<T>::IsFuncT && !CompountT<T>::IsArrayT>
class ConsumeUDC { public: operator T() const; };

template<typename T> class ConsumeUDC<T, false> {}; // conversion to function types not possible
template<bool convert_possible> class ConsumeUDC<void, convert_possible> {}; // conversion to void not possible

#define enum_check_SIGNED(t) char enum_check(signed t); char enum_check(unsigned t)

char enum_check(bool);
enum_check_SIGNED(char);
char enum_check(wchar_t);
enum_check_SIGNED(short);
enum_check_SIGNED(int);
enum_check_SIGNED(long);
char enum_check(float);
char enum_check(double);
char enum_check(long double);
char enum_check(long long);

#undef enum_check_SIGNED

SizeOverOne enum_check(...); // catch all

template<typename T>
struct IsEnumT {
    enum {
        Yes = IsFundaT<T>::No    &&
        !CompountT<T>::IsPtrT    &&
        !CompountT<T>::IsRefT    &&
        !CompountT<T>::IsPtrMemT &&
        sizeof(enum_check(ConsumeUDC<T>())) == 1
    };
    NOisnotYES();
};

// --------------------
//      class types

template<typename T>
struct IsClassT {
    enum {
        Yes = IsFundaT<T>::No    &&
        IsEnumT<T>::No           &&
        !CompountT<T>::IsPtrT    &&
        !CompountT<T>::IsRefT    &&
        !CompountT<T>::IsArrayT  &&
        !CompountT<T>::IsPtrMemT &&
        !CompountT<T>::IsFuncT
    };
    NOisnotYES();
};

// ---------------------------
//      generic type check

template<typename T>
struct TypeT {
    enum {
        IsFundaT  = IsFundaT<T>::Yes,
        IsPtrT    = CompountT<T>::IsPtrT,
        IsRefT    = CompountT<T>::IsRefT,
        IsArrayT  = CompountT<T>::IsArrayT,
        IsFuncT   = CompountT<T>::IsFuncT,
        IsPtrMemT = CompountT<T>::IsPtrMemT,
        IsEnumT   = IsEnumT<T>::Yes,
        IsClassT  = IsClassT<T>::Yes
    };
};


// ----------------------
//      modify types 


template<typename T>
struct TypeOp  { // primary template
    typedef T         ArgT;
    typedef T         BareT;
    typedef T const   ConstT;
    typedef T &       RefT;
    typedef T &       RefBareT;
    typedef T const & RefConstT;
};
template<typename T>
struct TypeOp<T const>  { // partial specialization for const types
    typedef T const   ArgT;
    typedef T         BareT;
    typedef T const   ConstT;
    typedef T const & RefT;
    typedef T &       RefBareT;
    typedef T const & RefConstT;
};
template<typename T>
struct TypeOp<T&>  { // partial specialization for references
    typedef T &                         ArgT;
    typedef typename TypeOp<T>::BareT   BareT;
    typedef T const                     ConstT;
    typedef T &                         RefT;
    typedef typename TypeOp<T>::BareT & RefBareT;
    typedef T const &                   RefConstT;
};
template<>
struct TypeOp<void>  { // full specialization for void
    typedef void       ArgT;
    typedef void       BareT;
    typedef void const ConstT;
    typedef void       RefT;
    typedef void       RefBareT;
    typedef void       RefConstT;
};

// -------------------------
//      conditional type

template<bool B, typename T, typename F> class IfThenElseType;
template<typename T, typename F> class IfThenElseType<true, T, F> { public: typedef T ResultType; };
template<typename T, typename F> class IfThenElseType<false, T, F> { public: typedef F ResultType; };

#undef NOisnotYES

#else
#error ttypes.h included twice
#endif // TTYPES_H
