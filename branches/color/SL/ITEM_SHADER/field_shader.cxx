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

#include <item_sel_list.h>

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>

#include <ad_cb_prot.h>

#include <arb_global_defs.h>
#include <arb_str.h>

#include <set>

using namespace std;

#define AWAR_DIM_ACTIVE(dim) dimension_awar(dim, "active")
#define AWAR_FIELD(dim)      dimension_awar(dim, "field")
#define AWAR_VALUE_MIN(dim)  dimension_awar(dim, "min")
#define AWAR_VALUE_MAX(dim)  dimension_awar(dim, "max")

class FieldReader {
    RefPtr<const char> fieldname;
    bool is_hkey; // true if fieldname is hierarchical

    float min_value, max_value;
    float factor;

    static bool safe_atof(const char *strval, float& res) {
        // returns true if at least some characters have been converted
        char *end;
        res = strtof(strval, &end);
        return end != strval;
    }

    void calc_factor() {
        float span = max_value-min_value;
        factor     = span != 0.0 ? 1.0/span : 1/0.00001;
    }
public:
    FieldReader(const char *fieldname_) :
        fieldname(fieldname_),
        is_hkey(fieldname && strchr(fieldname, '/')),
        min_value(0),
        max_value(1)
    {
        calc_factor();
    }

    void set_value_range(const float& min_val, const float& max_val) {
        min_value = min_val;
        max_value = max_val;
        calc_factor();
    }

    bool can_read() const { return fieldname != 0; }
    const char *get_fieldname() const { return fieldname; }

    ShadedValue calc_value(GBDATA *gb_item) const {
        if (fieldname && gb_item) {
            GBDATA *gb_field = is_hkey
                ? GB_search(gb_item, fieldname, GB_FIND)
                : GB_entry(gb_item, fieldname);

            if (gb_field) {
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
                val = (val-min_value)*factor;
                return ValueTuple::make(val);
            }
        }
        return ValueTuple::undefined();
    }
};

typedef set<string> FieldSet;

class ItemFieldShader: public ShaderPlugin {
    BoundItemSel itemtype;
    FieldSet     hcbs_installed; // list of currently installed hierarchy callbacks

    RefPtr<AW_window> aw_config;

    mutable string                item_dbpath;
    mutable SmartPtr<FieldReader> reader;

    const char *get_fieldname(int dim) const {
        // returns configured fieldname (or NULL)
        AW_root *awr = AW_root::SINGLETON;
        if (awr->awar(AWAR_DIM_ACTIVE(dim))->read_int()) {
            const char *fname = awr->awar(AWAR_FIELD(dim))->read_char_pntr();
            if (strcmp(fname, NO_FIELD_SELECTED) != 0) {
                return fname;
            }
        }
        return NULL;
    }

    FieldReader *make_field_reader(int dim) const {
        AW_root *awr = AW_root::SINGLETON;
        if (awr) {
            const char  *fieldname = get_fieldname(dim);
            FieldReader *fr        = new FieldReader(fieldname);
            if (fieldname) {
                fr->set_value_range(atof(awr->awar(AWAR_VALUE_MIN(dim))->read_char_pntr()),
                                    atof(awr->awar(AWAR_VALUE_MAX(dim))->read_char_pntr()));
            }
            return fr;
        }
        return NULL;
    }

    FieldReader& get_field_reader() const {
        if (reader.isNull()) reader = make_field_reader(0);
        return *reader;
    }

    bool knows_item_dbpath() const {
        if (item_dbpath.empty()) {
            GBDATA *gb_item = itemtype.get_any_item();
            if (gb_item) {
                const char *ipath = GB_get_db_path(gb_item);
                if (ipath) {
                    const int PREFIXLEN = 9;
#if defined(ASSERTION_USED)
                    const char *PREFIX        = "/<gbmain>";
                    is_assert(ARB_strBeginsWith(ipath, PREFIX));
                    is_assert(strlen(PREFIX) == PREFIXLEN);
#endif
                    item_dbpath = string(ipath+PREFIXLEN);
                }
            }
        }
        return !item_dbpath.empty();
    }

    void update_db_callbacks(const FieldSet& wanted) {
        if (knows_item_dbpath()) {
            DatabaseCallback dbcb = makeDatabaseCallback(ItemFieldShader::field_updated_in_DB_cb, this);

            GB_ERROR  error   = NULL;
            GBDATA   *gb_main = itemtype.gb_main;

            GB_transaction ta(gb_main);

            // uninstall unwanted callbacks:
            for (FieldSet::const_iterator installed = hcbs_installed.begin(); installed != hcbs_installed.end(); ++installed) {
                if (wanted.find(*installed) == wanted.end()) { // 'installed' is not in 'wanted'
                    string field_db_path = item_dbpath + '/' + *installed;
                    error                = GB_remove_hierarchy_callback(gb_main, field_db_path.c_str(), GB_CB_CHANGED_OR_DELETED, dbcb);
                }
            }
            // install missing callbacks:
            for (FieldSet::const_iterator missing = wanted.begin(); missing != wanted.end(); ++missing) {
                if (hcbs_installed.find(*missing) == hcbs_installed.end()) { // 'missing' is not in 'hcbs_installed'
                    string field_db_path = item_dbpath + '/' + *missing;
                    error                = GB_add_hierarchy_callback(gb_main, field_db_path.c_str(), GB_CB_CHANGED_OR_DELETED, dbcb);
                }
            }
            hcbs_installed = wanted; // store current state
            aw_message_if(error);
        }
    }
    void add_used_fields(FieldSet& wanted) const {
        FieldReader *freader = make_field_reader(0);
        if (freader && freader->can_read()) wanted.insert(freader->get_fieldname());
        delete freader;
    }
    void setup_db_callbacks(bool install) {
        FieldSet wanted;
        if (install) add_used_fields(wanted); // otherwise uninstall all
        update_db_callbacks(wanted);
    }
    static void field_updated_in_DB_cb(UNFIXED, const ItemFieldShader *shader) {
        shader->trigger_reshade_cb();
    }

public:
    explicit ItemFieldShader(const BoundItemSel& itemtype_) :
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

    void activate(bool on) OVERRIDE {
        // called with true when plugin gets activated, with false when it gets deactivated
        setup_db_callbacks(on);
    }


    void setup_changed_cb() {
        setup_db_callbacks(true); // @@@ does this also happen if plugin is NOT ACTIVE? shouldn't!

        reader.SetNull();
        trigger_reshade_cb();
    }
    static void setup_changed_cb(AW_root*, ItemFieldShader *shader) {
        shader->setup_changed_cb();
    }
};

void ItemFieldShader::init_specific_awars(AW_root *awr) {
    for (int dim = 0; dim<get_max_dimension(); ++dim) {
        RootCallback FieldSetup_changed_cb = makeRootCallback(ItemFieldShader::setup_changed_cb, this);

        awr->awar_int(AWAR_DIM_ACTIVE(dim), dim == 0)->add_callback(FieldSetup_changed_cb);
        awr->awar_string(AWAR_FIELD(dim), NO_FIELD_SELECTED)->add_callback(FieldSetup_changed_cb);
        awr->awar_string(AWAR_VALUE_MIN(dim), "0")->add_callback(FieldSetup_changed_cb);
        awr->awar_string(AWAR_VALUE_MAX(dim), "1")->add_callback(FieldSetup_changed_cb);
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

            const int VALCOL = 8;
            aws->create_input_field(AWAR_VALUE_MIN(dim), VALCOL);
            aws->create_input_field(AWAR_VALUE_MAX(dim), VALCOL);

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

