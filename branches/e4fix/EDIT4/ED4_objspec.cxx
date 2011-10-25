// =============================================================== //
//                                                                 //
//   File      : ED4_objspec.cxx                                   //
//   Purpose   : hierarchy object specification                    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"
#include <static_assert.h>

#define MAX_POSSIBLE_SPECIFIED_OBJECT_TYPES ((sizeof(ED4_level)*8)-1)

COMPILE_ASSERT(MAX_SPECIFIED_OBJECT_TYPES <= MAX_POSSIBLE_SPECIFIED_OBJECT_TYPES);

inline int level2index(ED4_level lev) {
    int index = 0;
    e4_assert(lev);
    while (lev) {
        if (lev&1) {
            e4_assert(lev == 1);
            return index;
        }
        lev = ED4_level(lev>>1);
        index++;
    }
    e4_assert(0); // invalid level
    return -1U;
}
inline ED4_level index2level(int index) { return ED4_level(1<<index); }


static class ED4_objspec_registry& get_objspec_registry();

class ED4_objspec_registry : virtual Noncopyable {
    ED4_objspec *known_spec[MAX_SPECIFIED_OBJECT_TYPES];
    int count;

    ED4_objspec_registry()
        : count(0)
    {
        memset(known_spec, 0, sizeof(*known_spec)*MAX_SPECIFIED_OBJECT_TYPES);
    }
    friend ED4_objspec_registry& get_objspec_registry();
public:

    void register_objspec(ED4_objspec *ospec) {
        int idx = level2index(ospec->level);

        e4_assert(ospec);
        e4_assert(idx<MAX_SPECIFIED_OBJECT_TYPES);
        e4_assert(!known_spec[idx]); // two object types have same level

        known_spec[idx] = ospec;
        count++;
    }

    int count_registered() const {
        return count;
    }

    const ED4_objspec& get_object_spec(ED4_level lev) const {
        ED4_objspec *spec = known_spec[level2index(lev)];
        e4_assert(spec);
        return *spec;
    }

    bool has_manager_that_may_contain(ED4_level lev) const {
        for (int i = 0; i<MAX_SPECIFIED_OBJECT_TYPES; ++i) {
            ED4_objspec *spec = known_spec[i];
            if (spec && spec->is_manager() && spec->allowed_to_contain(lev))
                return true;
        }
        return false;
    }

    void init_object_specs() {
        for (int i = 1; i<MAX_SPECIFIED_OBJECT_TYPES; ++i) {
            if (known_spec[i]) {
                ED4_objspec& spec = *known_spec[i];

                if (!has_manager_that_may_contain(spec.level)) {
                    printf("\nDid not find manager allowed to contain:\n");
                    spec.dump(2);
                }
                // e4_assert(has_manager_that_may_contain(spec.level));
            }
        }

    }
};

static ED4_objspec_registry& get_objspec_registry() {
    static ED4_objspec_registry objspec_registry;
    return objspec_registry;
}

bool ED4_objspec::object_specs_initialized = false;
void ED4_objspec::init_object_specs() {
    e4_assert(!object_specs_initialized);
    get_objspec_registry().init_object_specs();
    object_specs_initialized = true;
}

ED4_objspec::ED4_objspec(ED4_properties static_prop_, ED4_level level_, ED4_level allowed_children_, ED4_level handled_level_, ED4_level restriction_level_, float justification_)
    : static_prop(static_prop_),
      level(level_),
      allowed_children(allowed_children_),
      handled_level(handled_level_),
      restriction_level(restriction_level_),
      justification(justification_)
{
    e4_assert(!object_specs_initialized); // specs must be instaciated before they are initialized
    e4_assert(justification == 0); // @@@ always 0 -> remove

    switch (static_prop&(ED4_P_IS_MANAGER|ED4_P_IS_TERMINAL)) {
        case ED4_P_IS_MANAGER: // manager specific checks
            e4_assert(static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL));                                        // each manager has to be vertical or horizontal
            e4_assert((static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL)) != (ED4_P_HORIZONTAL|ED4_P_VERTICAL)); // but never both
            e4_assert(allowed_children != ED4_L_NO_LEVEL); // managers need to allow children (what else should they manage)
            break;

        case ED4_P_IS_TERMINAL: // terminal specific checks
            e4_assert((static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL)) == 0); // terminals do not have orientation
            e4_assert(allowed_children == ED4_L_NO_LEVEL); // terminals cannot have children
            break;

        default :
            e4_assert(0);
            break;
    }

    get_objspec_registry().register_objspec(this);
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif


void TEST_objspec_registry() {
    TEST_ASSERT(level2index(ED4_level(0x1)) == 0);
    TEST_ASSERT(level2index(ED4_level(0x2)) == 1);
    TEST_ASSERT(level2index(ED4_level(0x4)) == 2);
    TEST_ASSERT(level2index(ED4_level(0x10000)) == 16);

    for (int i = 0; i<MAX_SPECIFIED_OBJECT_TYPES; ++i) {
        TEST_ASSERT_EQUAL(level2index(index2level(i)), i);
    }

    ED4_objspec_registry& objspec_registry = get_objspec_registry();
    TEST_ASSERT(objspec_registry.count_registered()>0);
    TEST_ASSERT_EQUAL(objspec_registry.count_registered(), MAX_SPECIFIED_OBJECT_TYPES);

    TEST_ASSERT(objspec_registry.get_object_spec(ED4_L_ROOT).allowed_children == ED4_L_ROOTGROUP);

    MISSING_TEST(log);
    ED4_objspec::init_object_specs();
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
