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
#include <limits>

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
    FieldReader() : fieldname(NULL) {}

    FieldReader(const char *fieldname_, float minVal, float maxVal) :
        fieldname(fieldname_),
        is_hkey(strchr(fieldname, '/')),
        min_value(minVal),
        max_value(maxVal)
    {
        is_assert(fieldname);
        calc_factor();
    }

    bool may_read() const { return fieldname != 0; } // false -> never will produce value

    float calc_value(GBDATA *gb_item) const {
        /*! reads one field from passed item.
         *
         * Returns NAN in the following cases:
         * - 'this' is a null-reader
         * - no item passed
         * - field is missing
         * - STRING field contains no numeric data
         *
         * Otherwise the value is scaled (but not limited) to the value range.
         */
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
                            return NAN;
                        }
                        break;
                    }
                }
                val = (val-min_value)*factor;
                return val;
            }
        }
        return NAN;
    }

};

class MultiFieldReader {
    FieldReader reader[3];
    int         dim;

public:
    MultiFieldReader() :
        dim(0)
    {}

    void add_reader(const FieldReader& added) {
        if (added.may_read()) {
            is_assert(dim<3); // hardcoded limit
            reader[dim++] = added;
        }
    }
    int get_dimension() const {
        return dim;
    }

    ShadedValue calc_value(GBDATA *gb_item) const {
        switch (dim) {
            case 0: return ValueTuple::undefined();
            case 1: return ValueTuple::make(reader[0].calc_value(gb_item));
            case 2: return ValueTuple::make(reader[0].calc_value(gb_item),
                                            reader[1].calc_value(gb_item));
            case 3: return ValueTuple::make(reader[0].calc_value(gb_item),
                                            reader[1].calc_value(gb_item),
                                            reader[2].calc_value(gb_item));
        }
        is_assert(0); // unsupported dimension
        return ShadedValue();
    }
};


typedef set<string> FieldSet;

class ItemFieldShader: public ShaderPlugin {
    BoundItemSel itemtype;
    FieldSet     hcbs_installed; // list of currently installed hierarchy callbacks

    RefPtr<AW_window> aw_config;

    mutable string                     item_dbpath;
    mutable SmartPtr<MultiFieldReader> reader;

    const char *get_fieldname(int dim) const {
        // returns configured fieldname (or NULL)
        AW_root *awr = AW_root::SINGLETON;
        if (awr && awr->awar(AWAR_DIM_ACTIVE(dim))->read_int()) {
            const char *fname = awr->awar(AWAR_FIELD(dim))->read_char_pntr();
            if (strcmp(fname, NO_FIELD_SELECTED) != 0) {
                return fname;
            }
        }
        return NULL;
    }

    FieldReader get_dimension_reader(int dim) const {
        AW_root *awr = AW_root::SINGLETON;
        if (awr) {
            const char *fieldname = get_fieldname(dim);
            if (fieldname)  {
                float minVal = atof(awr->awar(AWAR_VALUE_MIN(dim))->read_char_pntr());
                float maxVal = atof(awr->awar(AWAR_VALUE_MAX(dim))->read_char_pntr());
                return FieldReader(fieldname, minVal, maxVal);
            }
        }
        return FieldReader();
    }

    MultiFieldReader *make_multi_field_reader() const {
        MultiFieldReader *multi = new MultiFieldReader;
        for (int dim = 0; dim<get_max_dimension(); ++dim) {
            multi->add_reader(get_dimension_reader(dim));
        }
        return multi;
    }

    MultiFieldReader& get_field_reader() const {
        if (reader.isNull()) reader = make_multi_field_reader();
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
        const char *fname0 = get_fieldname(0);
        if (fname0) wanted.insert(fname0);
    }
    void setup_db_callbacks(bool install) {
        FieldSet wanted;
        if (install) add_used_fields(wanted); // otherwise uninstall all
        update_db_callbacks(wanted);
    }
    static void field_updated_in_DB_cb(UNFIXED, ItemFieldShader *shader) {
        shader->trigger_reshade_cb(SIMPLE_RESHADE);
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
        return get_field_reader().get_dimension();
    }
    int get_max_dimension() const {
        return 3;
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
        trigger_reshade_cb(CHECK_DIMENSION_CHANGE);
    }
    static void setup_changed_cb(AW_root*, ItemFieldShader *shader) {
        shader->setup_changed_cb();
    }

    void scan_value_range_cb(int dim);
    static void scan_value_range_cb(AW_window*, ItemFieldShader *shader, int dim) { shader->scan_value_range_cb(dim); }

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

            const int VALCOL = 9;
            aws->create_input_field(AWAR_VALUE_MIN(dim), VALCOL);
            aws->create_input_field(AWAR_VALUE_MAX(dim), VALCOL);

            aws->callback(makeWindowCallback(scan_value_range_cb, this, dim));
            aws->create_button(GBS_global_string("SCAN%i", dim), "SCAN", 0);

            aws->at_newline();
        }

        aw_config = aws;
    }
    aw_config->activate();
}

inline const char *make_limit_string(bool use_float, float f, int i) {
    if (use_float) {
        // cut off trailing '.0*':
        char *s = GBS_global_string_copy("%f", f);
        char *e = strchr(s, 0)-1;
        while (e>s) {
            char c = e[0];
            if (c == '.') {
                e[0] = 0;
                break;
            }
            if (c != '0') break;
            *e-- = 0;
        }
        const char *cs = GBS_static_string(s);
        free(s);
        return cs;
    }
    return GBS_global_string("%i", i);
}

void ItemFieldShader::scan_value_range_cb(int dim) {
    AW_root    *awr       = AW_root::SINGLETON;
    const char *fieldname = awr->awar(AWAR_FIELD(dim))->read_char_pntr();

    if (strcmp(fieldname, NO_FIELD_SELECTED) == 0) {
        aw_message("Select field to scan");
    }
    else {
        int   imin = numeric_limits<int>::max();
        int   imax = numeric_limits<int>::min();
        float fmin = numeric_limits<float>::max();
        float fmax = numeric_limits<float>::min();

        bool seen_field      = false;
        bool is_hierarchical = strchr(fieldname, '/');

        GB_transaction ta(itemtype.gb_main);
        for (GBDATA *gb_cont = itemtype.get_first_item_container(NULL, QUERY_ALL_ITEMS);
             gb_cont;
             gb_cont = itemtype.get_next_item_container(gb_cont, QUERY_ALL_ITEMS))
        {
            for (GBDATA *gb_item = itemtype.get_first_item(gb_cont, QUERY_ALL_ITEMS);
                 gb_item;
                 gb_item         = itemtype.get_next_item(gb_item, QUERY_ALL_ITEMS))
            {
                GBDATA *gb_field = is_hierarchical
                    ? GB_search(gb_item, fieldname, GB_FIND)
                    : GB_entry(gb_item, fieldname);

                if (gb_field) {
                    seen_field = true;

                    GB_TYPES field_type = GB_read_type(gb_field);

                    switch (field_type) {
                        case GB_INT: {
                            int i = GB_read_int(gb_field);
                            imin  = min(imin, i);
                            imax  = max(imax, i);
                            break;
                        }
                        case GB_FLOAT: {
                            float f = GB_read_float(gb_field);
                            fmin  = min(fmin, f);
                            fmax  = max(fmax, f);
                            break;
                        }
                        default: {
                            const char *s = GB_read_as_string(gb_field);

                            float f = atof(s);
                            int   i = atoi(s);

                            imin = min(imin, i);
                            imax = max(imax, i);

                            fmin = min(fmin, f);
                            fmax = max(fmax, f);
                            break;
                        }
                    }
                }
            }
        }

        if (seen_field) {
            // decide whether to use float or int limits
            bool seen_float = fmin<=fmax;
            bool seen_int   = imin<=imax;
            is_assert(seen_float || seen_int);

            bool use_float = seen_float && (!seen_int || imax<fmax);

            // @@@ avoid duplicate refresh:
            awr->awar(AWAR_VALUE_MIN(dim))->write_string(make_limit_string(use_float, fmin, imin));
            awr->awar(AWAR_VALUE_MAX(dim))->write_string(make_limit_string(use_float, fmax, imax));

            bool shading_useless = !(use_float ? fmin<fmax : imin<imax);
            if (shading_useless) {
                aw_message(GBS_global_string("Using field '%s' for shading is quite useless", fieldname));
            }
        }
    }
}

// ------------------
//      factory:

ShaderPluginPtr makeItemFieldShader(BoundItemSel& itemtype) {
    return new ItemFieldShader(itemtype);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#include <arbdbt.h>

#define TEST_READER_READS(reader,species,expected) TEST_EXPECT_EQUAL(ValueTuple::make((reader).calc_value(species))->inspect(), expected)
#define TEST_READER_UNDEF(reader,species)          TEST_READER_READS(reader, species, "<undef>")
#define TEST_MULTI_READS(reader,species,expected)  TEST_EXPECT_EQUAL((reader).calc_value(species)->inspect(), expected)
#define TEST_MULTI_UNDEF(reader,species)           TEST_MULTI_READS(reader,species, "<undef>")

void TEST_FieldReader() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("nosuch.arb", "c");
    TEST_REJECT_NULL(gb_main);

    GBDATA *gb_species, *gb_species2, *gb_species_outofbounds, *gb_species_no_field;

    const char *FIELD_FLOAT  = "float";
    const char *FIELD_INT    = "int";
    const char *FIELD_STRING = "string";
    {
        GB_transaction ta(gb_main);

        gb_species             = GBT_find_or_create_species(gb_main, "test");  TEST_REJECT_NULL(gb_species);
        gb_species2            = GBT_find_or_create_species(gb_main, "other"); TEST_REJECT_NULL(gb_species2);
        gb_species_no_field    = GBT_find_or_create_species(gb_main, "empty"); TEST_REJECT_NULL(gb_species_no_field);
        gb_species_outofbounds = GBT_find_or_create_species(gb_main, "outer"); TEST_REJECT_NULL(gb_species_outofbounds);

        GBDATA *gb_field;
        gb_field = GB_searchOrCreate_float (gb_species, FIELD_FLOAT,  0.25);        TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_int   (gb_species, FIELD_INT,    50);          TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_string(gb_species, FIELD_STRING, "200 units"); TEST_REJECT_NULL(gb_field);

        gb_field = GB_searchOrCreate_float (gb_species2, FIELD_FLOAT,  0.9);     TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_int   (gb_species2, FIELD_INT,    99);      TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_string(gb_species2, FIELD_STRING, "175.9"); TEST_REJECT_NULL(gb_field);

        gb_field = GB_searchOrCreate_float (gb_species_outofbounds, FIELD_FLOAT,  1.5);          TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_int   (gb_species_outofbounds, FIELD_INT,    9999);         TEST_REJECT_NULL(gb_field);
        gb_field = GB_searchOrCreate_string(gb_species_outofbounds, FIELD_STRING, "-12345.678"); TEST_REJECT_NULL(gb_field);
    }

    FieldReader nullReader;
    FieldReader missingReader("missing",    0,   1);
    FieldReader floatReader  (FIELD_FLOAT,  1,   0); // reverse value-range!
    FieldReader intReader    (FIELD_INT,    0,   100);
    FieldReader stringReader (FIELD_STRING, 100, 250);

    TEST_REJECT(nullReader.may_read());
    TEST_EXPECT(missingReader.may_read());
    TEST_EXPECT(stringReader.may_read());

    {
        GB_transaction ta(gb_main);

        // expect undef if no species - no matter what reader is used (eg. zombie in tree)
        TEST_READER_UNDEF(nullReader,    NULL);
        TEST_READER_UNDEF(missingReader, NULL);
        TEST_READER_UNDEF(floatReader,   NULL);
        TEST_READER_UNDEF(intReader,     NULL);
        TEST_READER_UNDEF(stringReader,  NULL);

        TEST_READER_UNDEF(nullReader,    gb_species); // null reader always undef
        TEST_READER_UNDEF(missingReader, gb_species); // expect undef if field is missing

        TEST_READER_READS(floatReader,   gb_species, "(0.750)");
        TEST_READER_READS(intReader,     gb_species, "(0.500)");
        TEST_READER_READS(stringReader,  gb_species, "(0.667)");

        TEST_READER_READS(floatReader,   gb_species2, "(0.100)");
        TEST_READER_READS(intReader,     gb_species2, "(0.990)");
        TEST_READER_READS(stringReader,  gb_species2, "(0.506)"); // 175 would be mid-range, 175.9 is a little bit above

        // if values are outside of value-range -> they are scaled to range-size, but not bounded:
        TEST_READER_READS(floatReader,   gb_species_outofbounds, "(-0.500)");
        TEST_READER_READS(intReader,     gb_species_outofbounds, "(99.990)");
        TEST_READER_READS(stringReader,  gb_species_outofbounds, "(-82.971)");

        TEST_READER_UNDEF(floatReader,   gb_species_no_field); // species is missing all fields -> always undef
        TEST_READER_UNDEF(intReader,     gb_species_no_field);
        TEST_READER_UNDEF(stringReader,  gb_species_no_field);
    }

    // @@@ tdd MultiFieldReader!

    MultiFieldReader multi;        TEST_EXPECT_EQUAL(multi.get_dimension(), 0);
    multi.add_reader(nullReader);  TEST_EXPECT_EQUAL(multi.get_dimension(), 0);
    multi.add_reader(floatReader); TEST_EXPECT_EQUAL(multi.get_dimension(), 1);

    {
        GB_transaction ta(gb_main);
        
        // only floatReader added yet -> should behave like floatReader did above:
        TEST_MULTI_READS(multi, gb_species,             "(0.750)");
        TEST_MULTI_READS(multi, gb_species2,            "(0.100)");
        TEST_MULTI_READS(multi, gb_species_outofbounds, "(-0.500)");
        TEST_MULTI_UNDEF(multi, gb_species_no_field);
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
