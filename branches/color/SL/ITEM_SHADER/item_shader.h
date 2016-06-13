// ============================================================ //
//                                                              //
//   File      : item_shader.h                                  //
//   Purpose   : external interface of ITEM_SHADER              //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef ITEM_SHADER_H
#define ITEM_SHADER_H

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif

class GBDATA;

// --------------------
//      ValueTuple

class ValueTuple {
    /*! Contains a value-tuple (used for shading items).
     *
     * Properties:
     * - variable tuple-size (0-3)
     * - values may be defined or not
     * - values are normalized to [0.0 .. 1.0]
     * - values can be mixed (using weights)
     */

public:
    virtual ~ValueTuple() {}
};
typedef SmartPtr<ValueTuple> ShadedValue;

// --------------------
//      ItemShader

class ItemShader {
public:
    virtual ~ItemShader() {}

    virtual ShadedValue shade(GBDATA *gb_item) const                                                  = 0;
    virtual ShadedValue mix(const ShadedValue& val1, float proportion, const ShadedValue& val2) const = 0;
    virtual int to_GC(const ShadedValue& val) const                                                   = 0;
};

// -----------------------------
//      ItemShader registry

ItemShader *registerItemShader(const char *unique_id);
ItemShader *findItemShader(const char *id);
void        destroyAllItemShaders();

#else
#error item_shader.h included twice
#endif // ITEM_SHADER_H
