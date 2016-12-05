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
#ifndef ARB_MSG_H
#include <arb_msg.h>
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
#ifndef _GLIBCXX_STRING
#include <string>
#endif

#define is_assert(cond) arb_assert(cond)

class GBDATA;
class AW_gc_manager;

// ----------------
//      Phaser

class Phaser {
    float frequency; // 1.0 => [0.0 .. 1.0] gets mapped to whole color range;
                     // 2.0 => [0.0 .. 0.5] and [0.5 .. 1.0] each gets mapped to whole color range

    float preshift;  // allowed values [0.0 .. 1.0]; applied BEFORE frequency mapping
    float postshift; // allowed values [0.0 .. 1.0]; applied AFTER  frequency mapping

    bool alternate;  // if frequency>1.0 => use alternate mapping direction

    float result_lower_bound; // = result of rephase(0.0)
    float result_upper_bound; // = result of rephase(1.0)

    static inline bool CONSTEXPR_RETURN is_normalized(const float& f) {
        return f>=0.0 && f<=1.0;
    }
    static inline float CONSTEXPR_RETURN shift_and_wrap(const float& val, const float& shift, float wrapto) {
        return shift>val ? val-shift+wrapto : val-shift;
    }

    float rephase_inbound(float f) const {
        is_assert(f>=0.0 && f<=1.0);

        const float preshifted(shift_and_wrap(f, preshift, 1.0));
        const float blow(preshifted*frequency);

        int   phase(blow);
        float rest(blow-phase);
        if (rest <= 0.0 && phase>0) {
            rest  = 1.0;
            phase--;
        }

        rest += postshift;
        if (rest>1.0) {
            rest -= 1.0;
            phase++;
        }
        is_assert(is_normalized(rest));

        const bool  do_alter(alternate && (phase&1));
        const float altered(do_alter ? 1.0-rest : rest);

#if defined(DEBUG) && 0
        fprintf(stderr,
                "f=%f preshifted=%f blow=%f postshifted=%f phase=%i rest=%f do_alter=%i altered=%f\n",
                f, preshifted, blow, postshifted, phase, rest, do_alter, altered);
#endif

        is_assert(is_normalized(altered));
        return altered;
    }

    void calc_bound_results() {
        result_lower_bound = rephase_inbound(0.0);
        result_upper_bound = rephase_inbound(1.0);
    }

public:
    Phaser() :  // this Phaser does "nothing"
        frequency(1.0),
        preshift(0.0),
        postshift(0.0),
        alternate(false),
        result_lower_bound(0.0),
        result_upper_bound(1.0)
    {}

    Phaser(float frequency_, bool alternate_, float preshift_, float postshift_) :
        frequency(frequency_),
        preshift(preshift_),
        postshift(postshift_),
        alternate(alternate_)
    {
        is_assert(is_normalized(preshift));
        is_assert(is_normalized(postshift));

        calc_bound_results();
    }

    float rephase(float f) const {
        return
            f<=0.0
            ? result_lower_bound
            : (f>=1.0
               ? result_upper_bound
               : rephase_inbound(f));
    }
};

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
    typedef SmartPtr<ValueTuple> ShadedValue;

    virtual ~ValueTuple() {}

    virtual bool is_defined() const   = 0;
    virtual ShadedValue clone() const = 0;
    virtual int range_offset(const Phaser&) const  = 0; // returns int-offset into range [0 .. AW_RANGE_COLORS[

#if defined(UNIT_TESTS)
    virtual const char *inspect() const = 0;
#endif

    // ValueTuple factory:
    static ShadedValue undefined();
    static ShadedValue make(float f);
    static ShadedValue make(float f1, float f2);
    static ShadedValue make(float f1, float f2, float f3);

    // mix interface (main function + reverse visitors):
    virtual ShadedValue mix(float my_ratio, const ValueTuple& other) const = 0;
    virtual ShadedValue reverse_mix(float /*other_ratio*/, const class NoTuple& /*other*/)      const { return undefined_reverse_mix(); }
    virtual ShadedValue reverse_mix(float /*other_ratio*/, const class LinearTuple& /*other*/)  const { return undefined_reverse_mix(); }
    virtual ShadedValue reverse_mix(float /*other_ratio*/, const class PlanarTuple& /*other*/)  const { return undefined_reverse_mix(); }
    virtual ShadedValue reverse_mix(float /*other_ratio*/, const class SpatialTuple& /*other*/) const { return undefined_reverse_mix(); }
};

typedef ValueTuple::ShadedValue ShadedValue;

inline ShadedValue mix(const ShadedValue& val1, float val1_ratio, const ShadedValue& val2) {
    return val1->mix(val1_ratio, *val2);
}

// --------------------------
//      ShaderPlugin

class ItemShader;

enum ReshadeMode {
    SIMPLE_RESHADE         = 0,
    CHECK_DIMENSION_CHANGE = 1,
};

class ShaderPlugin {
    RefPtr<ItemShader> plugged_into;

    std::string id;
    std::string description;
    std::string awar_prefix; // empty means "awars not initialized yet"

    virtual void init_specific_awars(AW_root *awr) = 0;
public:
    ShaderPlugin(const std::string& id_, const std::string& description_) :
        plugged_into(NULL),
        id(id_),
        description(description_)
    {
        /*! construct a shader plugin
         * @param id_           a unique id
         * @param description_  description of plugin (ia. used as title of config window)
         */
    }
    virtual ~ShaderPlugin() {}

    void announce_shader(ItemShader *shader) { plugged_into = shader; }

    const std::string& get_id() const { return id; }
    const std::string& get_description() const { return description; }

    inline const char *get_shader_local_id() const;

    void init_awars(AW_root *awr, const char *awar_prefix_);
    const char *plugin_awar(const char *name) const {
        is_assert(!awar_prefix.empty()); // forgot to call init_awars?
        return GBS_global_string("%s/%s", awar_prefix.c_str(), name);
    }
    const char *dimension_awar(int dim, const char *name) const {
        is_assert(!awar_prefix.empty()); // forgot to call init_awars?
        is_assert(dim>=0 && dim<3); // invalid dimension specified
        return GBS_global_string("%s/%s_%i", awar_prefix.c_str(), name, dim);
    }

    bool overlay_marked() const;       // true if shader-plugin currently likes to display marked species in marked color
    bool overlay_color_groups() const; // true if shader-plugin currently likes to display color groups

    virtual ShadedValue shade(GBDATA *gb_item) const = 0;

    virtual int get_dimension() const = 0; // returns (current) dimension of shader-plugin

    virtual bool customizable() const    = 0;
    virtual void customize(AW_root *awr) = 0;

    virtual char *store_config() const  = 0;
    virtual void load_or_reset_config(const char *cfgstr) = 0;

    virtual void activate(bool on) = 0; // called with true when plugin gets activated, with false when it gets deactivated

    void trigger_reshade_if_active_cb(ReshadeMode mode);
};

typedef SmartPtr<ShaderPlugin> ShaderPluginPtr;

// --------------------
//      ItemShader

#define NO_PLUGIN_SELECTED ""

typedef void (*ReshadeCallback)();

class ItemShader {
    std::string     id;
    std::string     description;
    ReshadeCallback reshade_cb;

    int undefined_gc;

    mutable int  reshade_delay_level;
    mutable bool reshade_was_suppressed;

    void delay_reshade_callbacks(bool suppress) const {
        reshade_delay_level += suppress ? 1 : -1;
        is_assert(reshade_delay_level>=0);

        if (!reshade_delay_level) { // switched off delay
            if (reshade_was_suppressed) {
                reshade_cb();
                reshade_was_suppressed = false;
            }
        }

#if defined(ASSERTION_USED)
        bool start_of_delay = reshade_delay_level == 1 && suppress;
        is_assert(implicated(start_of_delay, !reshade_was_suppressed));
#endif
    }
    friend class DelayReshade;

protected:
    ShaderPluginPtr active_plugin;  // null means: no plugin active
    int             first_range_gc; // has to be set by init()!
    Phaser          phaser;

public:
    ItemShader(const std::string& id_, const std::string& description_, ReshadeCallback rcb, int undefined_gc_) :
        id(id_),
        description(description_),
        reshade_cb(rcb),
        undefined_gc(undefined_gc_),
        reshade_delay_level(0),
        reshade_was_suppressed(false),
        first_range_gc(-1)
    {}
    virtual ~ItemShader() {}

    virtual void register_plugin(ShaderPluginPtr plugin) = 0;
    virtual bool activate_plugin(const std::string& id)  = 0; // returns 'true' on success
    bool deactivate_plugin() { return activate_plugin(NO_PLUGIN_SELECTED); }
    virtual std::string active_plugin_name() const = 0;

    bool is_active_plugin(const ShaderPlugin& plugin) const {
        return active_plugin_name() == plugin.get_id();
    }

    virtual void init() = 0; // call once after register_plugin was called (activates plugin stored in AWAR)
    virtual void popup_config_window(AW_root *awr) = 0;

    virtual void check_dimension_change() = 0;

    const std::string& get_id() const { return id; }
    const std::string& get_description() const { return description; }

    bool active() const { return active_plugin.isSet(); }
    bool overlay_marked()       const { return !active() || active_plugin->overlay_marked(); }       // if true, caller should use marked-GC
    bool overlay_color_groups() const { return active() ? active_plugin->overlay_color_groups() : AW_color_groups_active(); } // if true, caller should use color-groups-GCs

    ShadedValue shade(GBDATA *gb_item) const {
        is_assert(active()); // don't call if no shader is active
        return active() ? active_plugin->shade(gb_item) : ValueTuple::undefined();
    }
    int to_GC(const ShadedValue& val) const {
        is_assert(first_range_gc>0);
        if (val->is_defined()) {
            return first_range_gc + val->range_offset(phaser);
        }
        return undefined_gc;
    }

    void trigger_reshade_callback(ReshadeMode mode) {
        if (mode == CHECK_DIMENSION_CHANGE) check_dimension_change();

        if (reshade_delay_level) reshade_was_suppressed = true;
        else                     reshade_cb();
    }
};

class DelayReshade : virtual Noncopyable {
    const ItemShader *shader;
public:
    DelayReshade(const ItemShader *shader_) :
        shader(shader_)
    {
        shader->delay_reshade_callbacks(true);
    }
    ~DelayReshade() {
        shader->delay_reshade_callbacks(false);
    }
};

inline const char *ShaderPlugin::get_shader_local_id() const {
    is_assert(plugged_into);
    return GBS_global_string("%s_%s", plugged_into->get_id().c_str(), get_id().c_str());
}
inline void ShaderPlugin::trigger_reshade_if_active_cb(ReshadeMode mode) {
    if (plugged_into && plugged_into->is_active_plugin(*this)) {
        plugged_into->trigger_reshade_callback(mode);
    }
}

// -----------------------------
//      ItemShader registry

ItemShader       *registerItemShader(AW_root *awr, AW_gc_manager *gcman, BoundItemSel& itemtype, const char *unique_id, const char *description, const char *help_id, ReshadeCallback reshade, int undef_gc);
const ItemShader *findItemShader(const char *id);
void              destroyAllItemShaders();

#else
#error item_shader.h included twice
#endif // ITEM_SHADER_H
