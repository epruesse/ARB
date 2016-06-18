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

using namespace std;


class ItemFieldShader: public ShaderPlugin {
    BoundItemSel& itemtype;


public:
    ItemFieldShader(BoundItemSel& itemtype_) :
        ShaderPlugin("field", "Database field shader"),
        itemtype(itemtype_)
    {}

    ShadedValue shade(GBDATA */*gb_item*/) const OVERRIDE {
        return ValueTuple::undefined(); // @@@ fake
    }

    int get_dimension() const OVERRIDE {
        // returns (current) dimension of shader-plugin
        return 0; // @@@ fake
    }
    void init_specific_awars(AW_root *awr) OVERRIDE {
        // @@@ add shader specific awars
    }
};

ShaderPluginPtr makeItemFieldShader(BoundItemSel& itemtype) {
    return new ItemFieldShader(itemtype);
}
