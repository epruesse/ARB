// ============================================================== //
//                                                                //
//   File      : cbtypes.h                                        //
//   Purpose   : generic cb types                                 //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2011   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef CBTYPES_H
#define CBTYPES_H

#ifndef CB_H
#include <cb.h>
#endif
#ifndef TTYPES_H
#include <ttypes.h>
#endif
#ifndef STATIC_ASSERT_H
#include <static_assert.h>
#endif


// ---------------------------------
//      function type inspection

template<typename RT, typename P1, typename P2, typename P3>
struct Function {
    enum { NumParams = 3 };
    typedef RT (*Type)(P1,P2,P3);
    typedef RT ResultType;
};
template<typename RT, typename P1, typename P2>
struct Function<RT, P1, P2, void> {
    enum { NumParams = 2 };
    typedef RT (*Type)(P1,P2);
};
template<typename RT, typename P1>
struct Function<RT, P1, void, void> {
    enum { NumParams = 1 };
    typedef RT (*Type)(P1);
};
template<typename RT>
struct Function<RT, void, void, void> {
    enum { NumParams = 0 };
    typedef RT (*Type)();
};

// ---------------------------
//      forward parameters

template<typename T>
class ForwardParamT {
public:
    typedef typename IfThenElseType< TypeT<T>::IsClassT,
                                     typename TypeOp<T>::RefConstT,
                                     typename TypeOp<T>::ArgT >::ResultType Type;
};

template<typename T>
class ForwardParamT<T*> {
public:
    typedef typename TypeOp<T>::ArgT *Type;
};
template<> class ForwardParamT<void> { class Unused {}; public: typedef Unused Type; };

// ------------------------------
//      const parameter types

template<typename T>
class ConstParamT {
public:
    typedef typename IfThenElseType< TypeT<T>::IsClassT,
                                     typename TypeOp<T>::RefConstT,
                                     typename TypeOp<T>::ConstT >::ResultType Type;
};
template<typename T>
class ConstParamT<T*> {
public:
    typedef typename TypeOp<T>::ConstT *Type;
};
template<> class ConstParamT<void> { class Unused {}; public: typedef Unused Type; };

// ------------------------------------
//      forbid some parameter types

template<typename T> class AW_CL_castableType { public: static AW_CL cast_to_AW_CL(const T& t) { return (AW_CL)t; } };

#define INVALID_CB_PARAM_TYPE(TYPE) template<> class AW_CL_castableType<TYPE> { }

INVALID_CB_PARAM_TYPE(void);
INVALID_CB_PARAM_TYPE(double);
INVALID_CB_PARAM_TYPE(float);

#undef INVALID_CB_PARAM_TYPE

// -----------------------
//      typed callback

template<typename RT, typename P1 = void, typename P2 = void, typename P3 = void>
class StrictlyTypedCallback {
protected: 
    typedef typename Function<RT,P1,P2,P3>::Type FuncType;

    typedef typename ForwardParamT<P1>::Type FP1;
    typedef typename ForwardParamT<P2>::Type FP2;
    typedef typename ForwardParamT<P3>::Type FP3;

private:
    FuncType cb;

public:
    enum { NumParams = Function<RT,P1,P2,P3>::NumParams };

    StrictlyTypedCallback(FuncType CB) : cb(CB) {}

    RT operator()(FP1 p1, FP2 p2, FP3 p3) const { return cb(p1, p2, p3); }
    RT operator()(FP1 p1, FP2 p2)         const { return cb(p1, p2); }
    RT operator()(FP1 p1)                 const { return cb(p1); }
    RT operator()()                       const { return cb(); }
};

// ------------------------
//      casted callback

template<typename RT, typename FIXED>
class CallbackGroup {
public:
    typedef StrictlyTypedCallback<RT,FIXED,AW_CL,AW_CL> Signature;

private:
    Signature cb;
    AW_CL     p1, p2;

public:
    CallbackGroup(Signature CB, AW_CL P1, AW_CL P2) : cb(CB), p1(P1), p2(P2) {}
    RT operator()(FIXED fixed) const { return cb(fixed, p1, p2); }
};

// ---------------------------
//      convenience macros

#define CASTABLE_TO_AW_CL(TYPE) (sizeof(TYPE) <= sizeof(AW_CL))
#define CAST_TO_AW_CL(TYPE,PARAM) AW_CL_castableType<TYPE>::cast_to_AW_CL(PARAM)

#define CBTYPE_BUILDER_TEMPLATES(BUILDER,CB,RESULT,FIXED,SIG)           \
    inline CB BUILDER(RESULT (*cb)(FIXED)) {                            \
        return CB((SIG)cb, 0, 0);                                       \
    }                                                                   \
    template<typename P1>                                               \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1), P1 p1) { \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1));                          \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), 0);                    \
    }                                                                   \
    template<typename P1>                                               \
    inline CB BUILDER(RESULT (*cb)(FIXED, typename ConstParamT<P1>::Type), P1 p1) { \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1));                          \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), 0);                    \
    }                                                                   \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1, P2), P1 p1, P2 p2) {      \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2)); \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }                                                                   \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, typename ConstParamT<P1>::Type, typename ConstParamT<P2>::Type), P1 p1, P2 p2) { \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2)); \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }

// declares the callback type (CBTYPE) and the makeCBTYPE() templates needed to ensure
// typecheck between callback-signature and bound parameters
#define DECLARE_CBTYPE_AND_BUILDERS(CBTYPE,RESULT,FIXED,SIG)            \
    typedef CallbackGroup<RESULT, FIXED> CBTYPE;                        \
    CBTYPE_BUILDER_TEMPLATES(make##CBTYPE, CBTYPE, RESULT, FIXED, SIG);



#else
#error cbtypes.h included twice
#endif // CBTYPES_H
