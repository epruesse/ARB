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
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ITEMS_H
#include <items.h>
#endif
#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif

#define is_assert(cond) arb_assert(cond)

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
    std::string awar_prefix; // empty means "awars not initialized yet"

    virtual void init_specific_awars(AW_root *awr) = 0;
public:
    ShaderPlugin(const std::string& id_, const std::string& description_) :
        id(id_),
        description(description_)
    {}
    virtual ~ShaderPlugin() {}

    const std::string& get_id() const { return id; }
    const std::string& get_description() const { return description; }

    const char *plugin_awar(const char *name) const;
    void init_awars(AW_root *awr, const char *awar_prefix_);

    virtual ShadedValue shade(GBDATA *gb_item) const = 0;
    
    bool overlay_marked() const; // true if shader-plugin currently likes to display marked species in marked color
    bool overlay_color_groups() const; // true if shader-plugin currently likes to display color groups

    virtual int get_dimension() const = 0; // returns (current) dimension of shader-plugin
};

typedef SmartPtr<ShaderPlugin> ShaderPluginPtr;

// --------------------
//      ItemShader

#define NO_PLUGIN_SELECTED ""

typedef void (*ReshadeCallback)();

class ItemShader {
    std::string id;
    std::string description;

protected:
    ReshadeCallback reshade_cb;
    ShaderPluginPtr active_plugin; // null means: no plugin active

public:
    ItemShader(const std::string& id_, const std::string& description_, ReshadeCallback rcb) :
        id(id_),
        description(description_),
        reshade_cb(rcb)
    {}
    virtual ~ItemShader() {}

    virtual void register_plugin(ShaderPluginPtr plugin) = 0;
    virtual bool activate_plugin(const std::string& id)  = 0; // returns 'true' on success
    bool deactivate_plugin() { return activate_plugin(NO_PLUGIN_SELECTED); }
    virtual std::string active_plugin_name() const = 0;

    virtual void init() = 0; // call once after register_plugin was called (activates plugin stored in AWAR)
    virtual void popup_config_window(AW_root *awr) = 0;

    const std::string& get_id() const { return id; }
    const std::string& get_description() const { return description; }

    bool active() const { return active_plugin.isSet(); }
    bool overlay_marked()       const { return !active() || active_plugin->overlay_marked(); }       // if true, caller should use marked-GC
    bool overlay_color_groups() const { return active() ? active_plugin->overlay_color_groups() : AW_color_groups_active(); } // if true, caller should use color-groups-GCs

    ShadedValue shade(GBDATA *gb_item) const {
        is_assert(active()); // don't call if no shader is active
        return active() ? active_plugin->shade(gb_item) : ValueTuple::undefined();
    }
};

// -----------------------------
//      ItemShader registry

ItemShader *registerItemShader(AW_root *awr, BoundItemSel& itemtype, const char *unique_id, const char *description, const char *help_id, ReshadeCallback reshade);
ItemShader *findItemShader(const char *id);
void        destroyAllItemShaders();

#else
#error item_shader.h included twice
#endif // ITEM_SHADER_H
