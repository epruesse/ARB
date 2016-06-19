// ============================================================ //
//                                                              //
//   File      : field_shader.cxx                               //
//   Purpose   : a shader based on 1 to 3 item fields           //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "field_shader.h"

#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <item_sel_list.h>
#include <arb_global_defs.h>

using namespace std;

#define AWAR_DIM_ACTIVE(dim) dimension_awar(dim, "active")
#define AWAR_FIELD(dim)      dimension_awar(dim, "field")

class FieldReader {
    RefPtr<const char> fieldname;

    static bool safe_atof(const char *strval, float& res) {
        // returns true if at least some characters have been converted
        char *end;
        res = strtof(strval, &end);
        return end != strval;
    }

public:
    FieldReader(const char *dim0_awarname) :
        fieldname(AW_root::SINGLETON->awar(dim0_awarname)->read_char_pntr())
    {}

    ShadedValue calc_value(GBDATA *gb_item) const {
        GBDATA *gb_field = GB_entry(gb_item, fieldname);
        if (!gb_field) return ValueTuple::undefined();

        float val = 0.0;
        switch (GB_read_type(gb_field)) {
            case GB_INT: val = GB_read_int(gb_field); break;
            case GB_FLOAT: val = GB_read_float(gb_field); break;
            default: {
                if (!safe_atof(GB_read_as_string(gb_field), val)) {
                    return ValueTuple::undefined();
                }
                break;
            }
        }
        return ValueTuple::make(val);
    }
};

class ItemFieldShader: public ShaderPlugin {
    BoundItemSel      itemtype;
    RefPtr<AW_window> aw_config;

    mutable SmartPtr<FieldReader> reader;

    FieldReader& get_field_reader() const {
        if (reader.isNull()) reader = new FieldReader(AWAR_FIELD(0));
        return *reader;
    }

public:
    ItemFieldShader(const BoundItemSel& itemtype_) :
        ShaderPlugin("field", "Database field shader"),
        itemtype(itemtype_),
        aw_config(NULL)
    {}

    ShadedValue shade(GBDATA *gb_item) const OVERRIDE {
        return get_field_reader().calc_value(gb_item);
    }

    int get_dimension() const OVERRIDE {
        // returns (current) dimension of shader-plugin
        return 1; // @@@ fake
    }
    int get_max_dimension() const {
        return 1; // @@@ shall be 3
    }
    void init_specific_awars(AW_root *awr) OVERRIDE;

    bool customizable() const OVERRIDE { return true; }
    void customize(AW_root *awr) OVERRIDE;

    void setup_changed_cb() const {
        reader.SetNull();
        trigger_reshade_cb();
    }
    static void setup_changed_cb(AW_root*, const ItemFieldShader *shader) {
        shader->setup_changed_cb();
    }
};

void ItemFieldShader::init_specific_awars(AW_root *awr) {
    for (int dim = 0; dim<get_max_dimension(); ++dim) {
        awr->awar_int(AWAR_DIM_ACTIVE(dim), dim == 0);

        RootCallback FieldSetup_changed_cb = makeRootCallback(ItemFieldShader::setup_changed_cb, this);
        awr->awar_string(AWAR_FIELD(dim), NO_FIELD_SELECTED)->add_callback(FieldSetup_changed_cb);
    }
}
void ItemFieldShader::customize(AW_root *awr) {
    if (!aw_config) {
        AW_window_simple *aws = new AW_window_simple;
        {
            string wid   = GBS_global_string("%s_cfg", get_shader_local_id());
            string title = GBS_global_string("Configure %s", get_description().c_str());
            aws->init(awr, wid.c_str(), title.c_str());
        }

        aws->auto_space(5, 5);

        aws->button_length(8);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "O");

        aws->callback(makeHelpCallback("field_shader.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at_newline();

        for (int dim = 0; dim<get_max_dimension(); ++dim) {
            aws->create_toggle(AWAR_DIM_ACTIVE(dim));

            FieldSelDef def(AWAR_FIELD(dim), itemtype.gb_main, itemtype.selector, FIELD_FILTER_STRING_READABLE);
            create_itemfield_selection_button(aws, def, NULL);

            // @@@ add fields defining value-range
            // @@@ add autoscan for value-range

            aws->at_newline();
        }

        aw_config = aws;
    }
    aw_config->activate();
}

// ------------------
//      factory:

ShaderPluginPtr makeItemFieldShader(BoundItemSel& itemtype) {
    return new ItemFieldShader(itemtype);
}

