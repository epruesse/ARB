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

#ifndef CB_BASE_H
#include "cb_base.h"
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
struct ForwardParamT {
    typedef typename IfThenElseType< TypeT<T>::IsClassT,
                                     typename TypeOp<T>::RefConstT,
                                     typename TypeOp<T>::ArgT >::ResultType Type;
};

template<typename T>
struct ForwardParamT<T*> {
    typedef typename TypeOp<T>::ArgT *Type;
};
template<> class ForwardParamT<void> { class Unused {}; public: typedef Unused Type; };

// ------------------------------
//      const parameter types

template<typename T>
struct ConstParamT {
    typedef typename IfThenElseType< TypeT<T>::IsClassT,
                                     typename TypeOp<T>::RefConstT,
                                     typename TypeOp<T>::ConstT >::ResultType Type;
};
template<typename T>
struct ConstParamT<T*> {
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
struct StrictlyTypedCallback {
    typedef typename Function<RT,P1,P2,P3>::Type FuncType;

private:
    typedef typename ForwardParamT<P1>::Type FP1;
    typedef typename ForwardParamT<P2>::Type FP2;
    typedef typename ForwardParamT<P3>::Type FP3;

    FuncType cb;

public:
    enum { NumParams = Function<RT,P1,P2,P3>::NumParams };

    StrictlyTypedCallback(FuncType CB) : cb(CB) {}

    RT operator()(FP1 p1, FP2 p2, FP3 p3) const { return cb(p1, p2, p3); }
    RT operator()(FP1 p1, FP2 p2)         const { return cb(p1, p2); }
    RT operator()(FP1 p1)                 const { return cb(p1); }
    RT operator()()                       const { return cb(); }

    bool equals(const StrictlyTypedCallback& other) const { return cb == other.cb; }

    AW_CL get_cb() const { return (AW_CL)cb; }
    static StrictlyTypedCallback make_cb(AW_CL cb_) { return StrictlyTypedCallback((FuncType)cb_); }

    bool is_set() const { return cb != 0; }

    bool operator <  (const StrictlyTypedCallback& other) const { return cb <  other.cb; }
    bool operator == (const StrictlyTypedCallback& other) const { return cb == other.cb; }
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
    bool operator <  (const CallbackData& other) const {
        return p1<other.p1 || (p1 == other.p1 && (p2< other.p2 || (p2 == other.p2 && dealloc<other.dealloc)));
    }
    bool operator == (const CallbackData& other) const {
        return p1 == other.p1 && p2 == other.p2 && dealloc == other.dealloc;
    }
};

typedef CallbackData<AW_CL,AW_CL> UntypedCallbackData;

// ------------------------------
//      casted callback types

template<typename RT, typename FIXED>
struct Callback_FVV { // FVV stands for arguments (FIXED, VARIABLE, VARIABLE)
    typedef StrictlyTypedCallback<RT,FIXED,AW_CL,AW_CL> Signature;

private:
    Signature                     cb;
    SmartPtr<UntypedCallbackData> cd;

public:
    Callback_FVV(Signature CB, AW_CL P1, AW_CL P2) : cb(CB), cd(new UntypedCallbackData(P1, P2)) {}
    Callback_FVV(Signature CB, UntypedCallbackData::CallbackDataDeallocator dealloc, AW_CL P1, AW_CL P2) : cb(CB), cd(new UntypedCallbackData(P1, P2, dealloc)) {}

    RT operator()(FIXED fixed) const { return cb(fixed, cd->p1, cd->p2); }

    bool operator <  (const Callback_FVV& other) const { return cb<other.cb || (cb == other.cb && *cd<*other.cd); }
    bool operator == (const Callback_FVV& other) const { return cb == other.cb && *cd == *other.cd; }

    bool same_function_as(const Callback_FVV& other) const { return cb == other.cb; }

    AW_CL callee() const { return cb.get_cb(); } // @@@ only intermediate - remove later
    AW_CL inspect_CD1() const { return cd->p1; } // @@@ only intermediate - remove later
    AW_CL inspect_CD2() const { return cd->p2; } // @@@ only intermediate - remove later
};

template<typename RT, typename F1, typename F2>
struct Callback_FFV { // FFV stands for arguments (FIXED, FIXED, VARIABLE)
    typedef StrictlyTypedCallback<RT,F1,F2,AW_CL> Signature;

private:
    typedef CallbackData<AW_CL, AW_CL> FFV_CallbackData; // 2nd AW_CL is unused

    Signature                  cb;
    SmartPtr<FFV_CallbackData> cd;

public:
    Callback_FFV(Signature CB, AW_CL P)
        : cb(CB),
          cd(new FFV_CallbackData(P, 0))
    {}

    RT operator()(F1 f1, F2 f2) const { return cb(f1, f2, cd->p1); }
};

template<typename RT, typename F1, typename F2>
struct Callback_FVF { // FVF stands for arguments (FIXED, VARIABLE, FIXED)
    typedef StrictlyTypedCallback<RT,F1,AW_CL,F2>  SigP1;
    typedef StrictlyTypedCallback<RT,F1,F2,void>   SigP0F12;
    typedef StrictlyTypedCallback<RT,F2,void,void> SigP0F2;

private:
    enum funtype { ST_P0F12, ST_P0F2, ST_P1 }; // Signature type
    typedef CallbackData<AW_CL,funtype> FVF_CallbackData;

    AW_CL                      cb;  // has one of the above Signatures
    SmartPtr<FVF_CallbackData> cd;  // cd->p2 cannot be used by clients and is used to select Signature of 'cb'

    funtype get_funtype() const { return cd->p2; }

public:
    Callback_FVF(SigP0F12 CB) : cb(CB.get_cb()), cd(new FVF_CallbackData(0, ST_P0F12)) {}
    Callback_FVF(SigP0F2 CB) : cb(CB.get_cb()), cd(new FVF_CallbackData(0, ST_P0F2)) {}
    Callback_FVF(SigP1 CB, AW_CL P1) : cb(CB.get_cb()), cd(new FVF_CallbackData(P1, ST_P1)) {}
    Callback_FVF(SigP1 CB, UntypedCallbackData::CallbackDataDeallocator dealloc, AW_CL P1)
        : cb(CB.get_cb()), cd(new FVF_CallbackData(P1, ST_P1, (typename FVF_CallbackData::CallbackDataDeallocator)dealloc)) {}

    RT operator()(F1 f1, F2 f2) const {
        funtype ft = get_funtype();
        if (ft == ST_P0F12) return SigP0F12::make_cb(cb)(f1, f2);
        if (ft == ST_P0F2) return SigP0F2::make_cb(cb)(f2);
        arb_assert(ft == ST_P1);
        return SigP1::make_cb(cb)(f1, cd->p1, f2);
    }

    bool operator <  (const Callback_FVF& other) const { return cb<other.cb || (cb == other.cb && *cd<*other.cd); }
    bool operator == (const Callback_FVF& other) const { return cb == other.cb && *cd == *other.cd; }

    bool same_function_as(const Callback_FVF& other) const { return cb == other.cb; }

    AW_CL callee() const { return cb; }          // @@@ only intermediate - remove later
    AW_CL inspect_CD1() const { return cd->p1; } // @@@ only intermediate - remove later
    AW_CL inspect_CD2() const { return cd->p2; } // @@@ only intermediate - remove later
};


// ---------------------------
//      convenience macros

#define CASTABLE_TO_AW_CL(TYPE)   (sizeof(TYPE) <= sizeof(AW_CL))
#define CAST_TO_AW_CL(TYPE,PARAM) AW_CL_castableType<TYPE>::cast_to_AW_CL(PARAM)

#define CONST_PARAM_T(T) typename ConstParamT<T>::Type 

#define CBTYPE_FVV_BUILDER_NP(BUILDER,CB,RESULT,FIXED,SIG)      \
    inline CB BUILDER(RESULT (*cb)(FIXED)) {                    \
        return CB((SIG)cb, 0, 0);                               \
    }


#define CBTYPE_FVV_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,P1fun)                                             \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun), P1 p1) {                                                      \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), 0);                                                            \
    }                                                                                                           \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun),                                                               \
                      void (*dealloc)(P1), P1 p1) {                                                             \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1), 0);     \
    }

#define CBTYPE_FVV_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,P1fun,P2fun) \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun, P2fun),                \
                      P1 p1, P2 p2) {                                   \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2));  \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }                                                                   \
    template<typename P1, typename P2>                                  \
    inline CB BUILDER(RESULT (*cb)(FIXED, P1fun, P2fun),                \
                      void (*dealloc)(P1,P2), P1 p1, P2 p2) {           \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1) && CASTABLE_TO_AW_CL(P2));  \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1), CAST_TO_AW_CL(P2,p2)); \
    }

#define CBTYPE_FVV_BUILDER_NP12(BUILDER,CB,RESULT,FIXED,SIG,P1,P2)                                      \
    CBTYPE_FVV_BUILDER_NP(BUILDER,CB,RESULT,FIXED,SIG);                                                 \
    CBTYPE_FVV_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,P1);                                           \
    CBTYPE_FVV_BUILDER_P1(BUILDER,CB,RESULT,FIXED,SIG,P1,CONST_PARAM_T(P1));                            \
    CBTYPE_FVV_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,P1,P2);                                   \
    CBTYPE_FVV_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,P1,CONST_PARAM_T(P2));                    \
    CBTYPE_FVV_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,CONST_PARAM_T(P1),P2);                    \
    CBTYPE_FVV_BUILDER_P1P2(BUILDER,CB,RESULT,FIXED,SIG,P1,P2,CONST_PARAM_T(P1),CONST_PARAM_T(P2))

#define CBTYPE_FVV_BUILDER_TEMPLATES(BUILDER,CB,RESULT,FIXED,SIG)                                       \
    inline CB BUILDER(RESULT (*cb)()) {                                                                 \
        return CB((SIG)cb, 0, 0);                                                                       \
    }                                                                                                   \
    CBTYPE_FVV_BUILDER_NP12(BUILDER,CB,RESULT,FIXED,SIG,P1,P2);                                         \
    CBTYPE_FVV_BUILDER_NP12(BUILDER,CB,RESULT,UNFIXED,SIG,P1,P2)

#define CBTYPE_FVV_BUILDER_P(BUILDER,CB,RESULT,F1,F2,SIG,P,Pfun)                                        \
    template<typename P>                                                                                \
    inline CB BUILDER(RESULT (*cb)(F1,F2,Pfun), P p) {                                                  \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P));                                                            \
        return CB((SIG)cb, CAST_TO_AW_CL(P,p));                                                         \
    }                                                                                                   \
    template<typename P>                                                                                \
    inline CB BUILDER(RESULT (*cb)(F1,F2,Pfun), void (*dealloc)(P), P p) {                              \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P));                                                            \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P,p));  \
    }

#define CBTYPE_FFV_BUILDER_TEMPLATES(BUILDER,CB,RESULT,F1,F2,SIG)          \
    inline CB BUILDER(RESULT (*cb)()) { return CB((SIG)cb, 0); }           \
    inline CB BUILDER(RESULT (*cb)(F1)) { return CB((SIG)cb, 0); }         \
    inline CB BUILDER(RESULT (*cb)(F1,F2)) { return CB((SIG)cb, 0); }      \
    CBTYPE_FVV_BUILDER_P(BUILDER,CB,RESULT,F1,F2,SIG,P,P);                 \
    CBTYPE_FVV_BUILDER_P(BUILDER,CB,RESULT,F1,F2,SIG,P,CONST_PARAM_T(P))

#define CBTYPE_FVF_BUILDER_P1_F1F2(BUILDER,CB,RESULT,F1,F2,SIG,P1,P1fun)                                        \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(F1, P1fun, F2), P1 p1) {                                                     \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1));                                                               \
    }                                                                                                           \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(F1, P1fun, F2),                                                              \
                      void (*dealloc)(P1), P1 p1) {                                                             \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1));        \
    }

#define CBTYPE_FVF_BUILDER_P1_F1(BUILDER,CB,RESULT,F1,SIG,P1,P1fun)                                             \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(F1, P1fun), P1 p1) {                                                         \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, CAST_TO_AW_CL(P1,p1));                                                               \
    }                                                                                                           \
    template<typename P1>                                                                                       \
    inline CB BUILDER(RESULT (*cb)(F1, P1fun),                                                                  \
                      void (*dealloc)(P1), P1 p1) {                                                             \
        STATIC_ASSERT(CASTABLE_TO_AW_CL(P1));                                                                   \
        return CB((SIG)cb, (UntypedCallbackData::CallbackDataDeallocator)dealloc, CAST_TO_AW_CL(P1,p1));        \
    }

#define CBTYPE_FVF_BUILDER_NP1(BUILDER,CB,RESULT,F1,F2,SIG,SIG01,P1)                    \
    inline CB BUILDER(RESULT (*cb)(F1)) { return CB((SIG01)cb); }                       \
    inline CB BUILDER(RESULT (*cb)(F1,F2)) { return CB((SIG01)cb); }                    \
    CBTYPE_FVF_BUILDER_P1_F1F2(BUILDER,CB,RESULT,F1,F2,SIG,P1,P1);                      \
    CBTYPE_FVF_BUILDER_P1_F1F2(BUILDER,CB,RESULT,F1,F2,SIG,P1,CONST_PARAM_T(P1));       \
    CBTYPE_FVF_BUILDER_P1_F1(BUILDER,CB,RESULT,F1,SIG,P1,P1);                           \
    CBTYPE_FVF_BUILDER_P1_F1(BUILDER,CB,RESULT,F1,SIG,P1,CONST_PARAM_T(P1))

#define CBTYPE_FVF_BUILDER_TEMPLATES(BUILDER,CB,RESULT,F1,F2,SIG,SIG01,SIG02)           \
    inline CB BUILDER(RESULT (*cb)()) { return CB((SIG01)cb); }                         \
    inline CB BUILDER(RESULT (*cb)(F2)) { return CB((SIG02)cb); }                       \
    CBTYPE_FVF_BUILDER_NP1(BUILDER,CB,RESULT,F1,F2,SIG,SIG01,P1);                       \
    CBTYPE_FVF_BUILDER_NP1(BUILDER,CB,RESULT,UNFIXED,F2,SIG,SIG01,P1)

// declares the callback type (CBTYPE) and the makeCBTYPE() templates needed to ensure
// typecheck between callback-signature and bound parameters

#define DECLARE_CBTYPE_FVV_AND_BUILDERS(CBTYPE,RESULT,FIXED)            \
    typedef Callback_FVV<RESULT, FIXED> CBTYPE;                         \
    CBTYPE_FVV_BUILDER_TEMPLATES(make##CBTYPE,CBTYPE,RESULT,FIXED,      \
                                 CBTYPE::Signature::FuncType)

#define DECLARE_CBTYPE_FFV_AND_BUILDERS(CBTYPE,RESULT,F1,F2)            \
    typedef Callback_FFV<RESULT,F1,F2> CBTYPE;                          \
    CBTYPE_FFV_BUILDER_TEMPLATES(make##CBTYPE,CBTYPE,RESULT,F1,F2,      \
                                 CBTYPE::Signature::FuncType)

#define DECLARE_CBTYPE_FVF_AND_BUILDERS(CBTYPE,RESULT,F1,F2)            \
    typedef Callback_FVF<RESULT,F1,F2> CBTYPE;                          \
    CBTYPE_FVF_BUILDER_TEMPLATES(make##CBTYPE,CBTYPE,RESULT,F1,F2,      \
                                 CBTYPE::SigP1::FuncType,               \
                                 CBTYPE::SigP0F12::FuncType,            \
                                 CBTYPE::SigP0F2::FuncType)

#else
#error cbtypes.h included twice
#endif // CBTYPES_H
