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
#ifndef SMARTPTR_H
#include <smartptr.h>
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
    
    bool equals(const StrictlyTypedCallback& other) const { return cb == other.cb; }
};

// ---------------------
//      CallbackData

template<typename P1, typename P2>
struct CallbackData {
    P1 p1;
    P2 p2;

    typedef void (*CallbackDataDeallocator)(P1 p1, P2 p2);
    CallbackDataDeallocator dealloc;

    CallbackData(P1 p1_, P2 p2_) : p1(p1_), p2(p2_), dealloc(NULL) {}
    CallbackData(P1 p1_, P2 p2_, CallbackDataDeallocator dealloc_) : p1(p1_), p2(p2_), dealloc(dealloc_) {}
    ~CallbackData() { if (dealloc) dealloc(p1, p2); }
    bool equals(const CallbackData& other) const { return p1 == other.p1 && p2 == other.p2 && dealloc == other.dealloc; }
};

template<typename P1, typename P2>
inline bool operator == (const CallbackData<P1,P2>& cd1, const CallbackData<P1,P2>& cd2) { return cd1.equals(cd2); }

typedef CallbackData<AW_CL,AW_CL> UntypedCallbackData;

// ------------------------
//      casted callback

template<typename RT, typename FIXED>
class CallbackGroup {
public:
    typedef StrictlyTypedCallback<RT,FIXED,AW_CL,AW_CL> Signature;

private:
    Signature                     cb;
    SmartPtr<UntypedCallbackData> cd;

public:
    CallbackGroup(Signature CB, AW_CL P1, AW_CL P2) : cb(CB), cd(new UntypedCallbackData(P1, P2)) {}
    CallbackGroup(Signature CB, UntypedCallbackData::CallbackDataDeallocator dealloc, AW_CL P1, AW_CL P2) : cb(CB), cd(new UntypedCallbackData(P1, P2, dealloc)) {}
    RT operator()(FIXED fixed) const { return cb(fixed, cd->p1, cd->p2); }
    bool equals(const CallbackGroup& other) const { return cb.equals(other.cb) && *cd == *other.cd; }
};

template<typename RT, typename FIXED>
inline bool operator == (const CallbackGroup<RT,FIXED>& cb1, const CallbackGroup<RT,FIXED>& cb2) { return cb1.equals(cb2); }

// ---------------------------
//      convenience macros

#define CASTABLE_TO_AW_CL(TYPE)   (sizeof(TYPE) <= sizeof(AW_CL))
#define CAST_TO_AW_CL(TYPE,PARAM) AW_CL_castableType<TYPE>::cast_to_AW_CL(PARAM)

#define CONST_PARAM_T(T) typename ConstParamT<T>::Type 

#define CBTYPE_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,P1fun)         \
    template<typename P1>                                               \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun), P1 p1) {              \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1));                          \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), 0);                    \
    }                                                                   \
    template<typename P1>                                               \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun),                       \
                      void (*dealloc)(P1), P1 p1) {                     \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1));                          \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1), 0); \
    }                                                                   \

#define CBTYPE_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,P1fun,P2fun) \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun, P2fun),                \
                      P1 p1, P2 p2) {                                   \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2)); \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }                                                                   \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun, P2fun),                \
                      void (*dealloc)(P1,P2), P1 p1, P2 p2) {           \
        COMPILE_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2)); \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }                                                                   \

#define CBTYPE_BUILDER_TEMPLATES(BUILDER,CB,RESULT,FIXED,SIG)           \
    inline CB BUILDER(RESULT (*cb)(FIXED)) {                            \
        return CB((SIG)cb, 0, 0);                                       \
    }                                                                   \
    CBTYPE_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,P1);               \
    CBTYPE_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,CONST_PARAM_T(P1)); \
    CBTYPE_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,P1,P2);       \
    CBTYPE_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,CONST_PARAM_T(P1), CONST_PARAM_T(P2)); \

// declares the callback type (CBTYPE) and the makeCBTYPE() templates needed to ensure
// typecheck between callback-signature and bound parameters
#define DECLARE_CBTYPE_AND_BUILDERS(CBTYPE,RESULT,FIXED,SIG)            \
    typedef CallbackGroup<RESULT, FIXED> CBTYPE;                        \
    CBTYPE_BUILDER_TEMPLATES(make##CBTYPE, CBTYPE, RESULT, FIXED, SIG);



#else
#error cbtypes.h included twice
#endif // CBTYPES_H
