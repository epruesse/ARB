// ============================================================ //
//                                                              //
//   File      : item_shader.cxx                                //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "item_shader.h"

#define is_assert(cond) arb_assert(cond)

// --------------------------------------------------------------------------------
// test my interface

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_shader_interface() {
    const char *SHADY  = "lady";
    ItemShader *shader = registerItemShader(SHADY);
    TEST_REJECT_NULL(shader);

    ItemShader *unknown = findItemShader("unknown");
    TEST_EXPECT_NULL(unknown);

    ItemShader *found = findItemShader(SHADY);
    TEST_EXPECT_EQUAL(found, shader);

    destroyAllItemShaders();
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
// implementation

#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// -------------------------
//      ItemShader_impl

class ItemShader_impl : public ItemShader { // @@@ move to own header?
    string id;
public:
    ItemShader_impl(const string& id_) : id(id_) {}

    const string& get_id() const { return id; }

    ShadedValue shade(GBDATA */*gb_item*/) const OVERRIDE { return NULL; } // @@@ fake
    ShadedValue mix(const ShadedValue& /*val1*/, float /*proportion*/, const ShadedValue& /*val2*/) const OVERRIDE { return NULL; } // @@@ fake
    int to_GC(const ShadedValue& /*val*/) const OVERRIDE { return 0; } // @@@ fake
};


// -------------------------
//      shader registry

typedef vector<ItemShader_impl> Shaders;

static Shaders registered;

ItemShader *registerItemShader(const char *unique_id) { // @@@ return const shader?
    if (findItemShader(unique_id)) {
        is_assert(0); // duplicate shader id
        return NULL;
    }

    registered.push_back(ItemShader_impl(unique_id));
    return &registered.back();
}

struct has_id {
    string id;
    has_id(const char *id_) : id(id_) {}
    bool operator()(const ItemShader_impl& shader) { return shader.get_id() == id; }
};
ItemShader *findItemShader(const char *id) { // @@@ return const shader?
    Shaders::iterator found = find_if(registered.begin(), registered.end(), has_id(id));
    return found == registered.end() ? NULL : &*found;
}

void destroyAllItemShaders() {
    registered.clear();
}


