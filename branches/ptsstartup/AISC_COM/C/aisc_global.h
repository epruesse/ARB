// =============================================================== //
//                                                                 //
//   File      : aisc_global.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //
#ifndef AISC_GLOBAL_H
#define AISC_GLOBAL_H

#ifndef BYTESTRING_H
#include <bytestring.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define aisc_assert(cond) arb_assert(cond)

// type mask
#define AISC_TYPE_NONE   0x00000000
#define AISC_TYPE_INT    0x01000000
#define AISC_TYPE_DOUBLE 0x02000000
#define AISC_TYPE_STRING 0x03000000
#define AISC_TYPE_COMMON 0x04000000
#define AISC_TYPE_BYTES  0x05000000

#define AISC_VAR_TYPE_MASK 0xff000000
#define AISC_OBJ_TYPE_MASK 0x00ff0000
#define AISC_ATTR_MASK     0x0000ffff

#define AISC_INDEX       0x1ff0000
#define AISC_NO_ANSWER  -0x7fffffff

#define AISC_COMMON      0

union double_xfer { // workaround aliasing problems
    double as_double;
    int    as_int[2];
};

class AISC_Object : public Noncopyable {
    int  type_id;
    long remote_ptr; // this is a pointer to client-data in server-address-space (casted to long)

    void *operator&() { return 0; } // forbid error-prone idiom (AISC_Object just was 'long' in the past). Instead use as_result_param()

protected:

    void set(int IF_ASSERTION_USED(remoteType), long remotePtr) { aisc_assert(remoteType == type_id); remote_ptr = remotePtr; }

public:
    AISC_Object(int type_) : type_id(type_), remote_ptr(0) {}

    bool exists() const { return remote_ptr; }
    long get() const { return remote_ptr; }
    int type() const { return type_id; }

    void clear() { remote_ptr = 0; }
    void init(long remotePtr) { aisc_assert(!exists()); set(type_id, remotePtr); }

    long *as_result_param() { return &remote_ptr; }

};

#else
#error aisc_global.h included twice
#endif // AISC_GLOBAL_H

