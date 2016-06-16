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
#ifndef _GLIBCXX_STRING
#include <string>
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

    ValueTuple *undefined_reverse_mix() const { arb_assert(0); return NULL; }

public:
    virtual ~ValueTuple() {}

    virtual bool is_defined() const   = 0;
    virtual ValueTuple *clone() const = 0;
    virtual int range_offset() const  = 0; // returns int-offset into range [0 .. AW_RANGE_COLORS[

#if defined(UNIT_TESTS)
    virtual const char *inspect() const = 0;
#endif

    // ValueTuple factory:
    static ValueTuple *undefined();
    static ValueTuple *make(float f); // @@@ add builders for 2 and 3 floats

    // mix interface (main function + reverse visitors):
    virtual ValueTuple *mix(float my_ratio, const ValueTuple& other) const = 0;
    virtual ValueTuple *reverse_mix(float /*other_ratio*/, const class NoTuple& /*other*/)     const { return undefined_reverse_mix(); }
    virtual ValueTuple *reverse_mix(float /*other_ratio*/, const class LinearTuple& /*other*/) const { return undefined_reverse_mix(); }
    virtual ValueTuple *reverse_mix(float /*other_ratio*/, const class PlanarTuple& /*other*/) const { return undefined_reverse_mix(); }
    virtual ValueTuple *reverse_mix(float /*other_ratio*/, const class CubicTuple& /*other*/)  const { return undefined_reverse_mix(); }
};

typedef SmartPtr<ValueTuple> ShadedValue;

inline ShadedValue mix(const ShadedValue& val1, float val1_ratio, const ShadedValue& val2) {
    return val1->mix(val1_ratio, *val2);
}

// --------------------------
//      ShaderPlugin

class ShaderPlugin {
    std::string id;
    std::string description;

public:
    ShaderPlugin(const std::string& id_, const std::string& description_) :
        id(id_),
        description(description_)
    {}
    virtual ~ShaderPlugin() {}

    const std::string& get_id() const { return id; }
    const std::string& get_description() const { return description; }

    virtual ShadedValue shade(GBDATA *gb_item) const = 0;
    virtual bool shades_marked() const               = 0; // true if shader-plugin likes to shade marked species
};

typedef SmartPtr<ShaderPlugin> ShaderPluginPtr;

// --------------------
//      ItemShader

class ItemShader {
public:
    virtual ~ItemShader() {}

    virtual void register_plugin(ShaderPluginPtr plugin) = 0;
    virtual bool activate_plugin(const std::string& id)  = 0;            // returns 'true' on success

    virtual bool active() const = 0;
    virtual bool shades_marked() const = 0; // if true, caller should not use marked-GC

    virtual ShadedValue shade(GBDATA *gb_item) const = 0;
};

// -----------------------------
//      ItemShader registry

ItemShader *registerItemShader(const char *unique_id);
ItemShader *findItemShader(const char *id);
void        destroyAllItemShaders();

#else
#error item_shader.h included twice
#endif // ITEM_SHADER_H
