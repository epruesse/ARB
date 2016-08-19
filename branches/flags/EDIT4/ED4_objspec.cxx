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

#define MAX_POSSIBLE_SPECIFIED_OBJECT_TYPES ((sizeof(ED4_level)*8)-1)

STATIC_ASSERT(SPECIFIED_OBJECT_TYPES <= MAX_POSSIBLE_SPECIFIED_OBJECT_TYPES);

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
    ED4_objspec *known_spec[SPECIFIED_OBJECT_TYPES];
    int          count;

    ED4_objspec_registry() : count(0) {
        for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) known_spec[i] = 0;
    }
    friend ED4_objspec_registry& get_objspec_registry();

public:

    void register_objspec(ED4_objspec *ospec) {
        int idx = level2index(ospec->level);

        e4_assert(ospec);
        e4_assert(idx<SPECIFIED_OBJECT_TYPES);
        e4_assert(!known_spec[idx]); // two object types have same level

        known_spec[idx] = ospec;
        count++;
    }

    int count_registered() const {
        return count;
    }

    const ED4_objspec *get_object_spec_at_index(int index) const {
        e4_assert(index >= 0 && index<SPECIFIED_OBJECT_TYPES);
        return known_spec[index];
    }
    const ED4_objspec& get_object_spec(ED4_level lev) const { return *get_object_spec_at_index(level2index(lev)); }

    bool has_manager_that_may_contain(ED4_level lev) const {
        for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) {
            ED4_objspec *spec = known_spec[i];
            if (spec && spec->is_manager() && spec->allowed_to_contain(lev))
                return true;
        }
        return false;
    }

    void init_object_specs() {
#if defined(DEBUG)
        for (int i = 1; i<SPECIFIED_OBJECT_TYPES; ++i) {
            if (known_spec[i]) {
                ED4_objspec& spec = *known_spec[i];
                e4_assert(has_manager_that_may_contain(spec.level));
            }
        }
#endif
    }
};

static ED4_objspec_registry& get_objspec_registry() {
    static ED4_objspec_registry objspec_registry;
    return objspec_registry;
}

bool ED4_objspec::object_specs_initialized = false;
bool ED4_objspec::descendants_uptodate     = false;

void ED4_objspec::init_object_specs() {
    e4_assert(!object_specs_initialized);
    get_objspec_registry().init_object_specs();
    object_specs_initialized = true;
}

void ED4_objspec::calc_descendants() const {
    if (possible_descendants == LEV_INVALID) {
        possible_descendants = LEV_NONE;
        allowed_descendants  = LEV_NONE;
        if (used_children || allowed_children) {
            possible_descendants = used_children;
            allowed_descendants  = allowed_children;

            ED4_objspec_registry& objspec_registry = get_objspec_registry();

            for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) {
                const ED4_objspec *child_spec = objspec_registry.get_object_spec_at_index(i);
                if (child_spec && child_spec != this) {
                    if ((used_children|allowed_children) & child_spec->level) {
                        child_spec->calc_descendants();
                        if (used_children    & child_spec->level) possible_descendants = ED4_level(possible_descendants|child_spec->possible_descendants);
                        if (allowed_children & child_spec->level) allowed_descendants  = ED4_level(allowed_descendants |child_spec->allowed_descendants);
                    }
                }
            }
            // dump(2);
        }
    }
}
void ED4_objspec::recalc_descendants() {
    ED4_objspec_registry& objspec_registry = get_objspec_registry();

    for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) {
        const ED4_objspec *spec = objspec_registry.get_object_spec_at_index(i);
        if (spec) {
            spec->possible_descendants = LEV_INVALID;
            spec->allowed_descendants  = LEV_INVALID;
        }
    }
    for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) {
        const ED4_objspec *spec = objspec_registry.get_object_spec_at_index(i);
        if (spec) spec->calc_descendants();
    }

    descendants_uptodate = true;
}

ED4_objspec::ED4_objspec(ED4_properties static_prop_, ED4_level level_, ED4_level allowed_children_, ED4_level handled_level_, ED4_level restriction_level_)
    : used_children(LEV_NONE),
      possible_descendants(LEV_INVALID),
      allowed_descendants(LEV_INVALID),
      static_prop(static_prop_),
      level(level_),
      allowed_children(allowed_children_),
      handled_level(handled_level_),
      restriction_level(restriction_level_)
{
    e4_assert(!object_specs_initialized); // specs must be instaciated before they are initialized

    switch (static_prop&(ED4_P_IS_MANAGER|ED4_P_IS_TERMINAL)) {
        case ED4_P_IS_MANAGER: // manager specific checks
            e4_assert(static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL));                                        // each manager has to be vertical or horizontal
            e4_assert((static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL)) != (ED4_P_HORIZONTAL|ED4_P_VERTICAL)); // but never both
            e4_assert(allowed_children != LEV_NONE); // managers need to allow children (what else should they manage)
            break;

        case ED4_P_IS_TERMINAL: // terminal specific checks
            e4_assert((static_prop & (ED4_P_HORIZONTAL|ED4_P_VERTICAL)) == 0); // terminals do not have orientation
            e4_assert(allowed_children == LEV_NONE); // terminals cannot have children
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
    TEST_EXPECT_EQUAL(level2index(ED4_level(0x1)), 0);
    TEST_EXPECT_EQUAL(level2index(ED4_level(0x2)), 1);
    TEST_EXPECT_EQUAL(level2index(ED4_level(0x4)), 2);
    TEST_EXPECT_EQUAL(level2index(ED4_level(0x10000)), 16);

    for (int i = 0; i<SPECIFIED_OBJECT_TYPES; ++i) {
        TEST_EXPECT_EQUAL(level2index(index2level(i)), i);
    }

    ED4_objspec_registry& objspec_registry = get_objspec_registry();
    TEST_EXPECT(objspec_registry.count_registered()>0);
    TEST_EXPECT_EQUAL(objspec_registry.count_registered(), SPECIFIED_OBJECT_TYPES);

    TEST_EXPECT_EQUAL(objspec_registry.get_object_spec(LEV_ROOT).allowed_children, LEV_ROOTGROUP);

    ED4_objspec::init_object_specs();

    const ED4_objspec& multi_seq = objspec_registry.get_object_spec(LEV_MULTI_SEQUENCE);
    const ED4_objspec& seq       = objspec_registry.get_object_spec(LEV_SEQUENCE);

    TEST_EXPECT_ZERO(seq.get_possible_descendants()       & LEV_SEQUENCE_STRING);
    TEST_EXPECT_ZERO(multi_seq.get_possible_descendants() & LEV_SEQUENCE_STRING);

    TEST_REJECT_ZERO(seq.get_allowed_descendants()       & LEV_SEQUENCE_STRING);
    TEST_REJECT_ZERO(multi_seq.get_allowed_descendants() & LEV_SEQUENCE_STRING);

    TEST_EXPECT_ZERO(multi_seq.get_allowed_descendants() & LEV_ROOTGROUP);

    // simulate adding sth in the hierarchy
    multi_seq.announce_added(LEV_SEQUENCE);
    seq.announce_added(LEV_SEQUENCE_STRING);

    TEST_REJECT_ZERO(seq.get_possible_descendants()       & LEV_SEQUENCE_STRING);
    TEST_REJECT_ZERO(multi_seq.get_possible_descendants() & LEV_SEQUENCE_STRING);

    TEST_EXPECT_ZERO(multi_seq.get_possible_descendants() & LEV_SEQUENCE_INFO);

    // add more (checks refresh)
    seq.announce_added(LEV_SEQUENCE_INFO);

    TEST_REJECT_ZERO(multi_seq.get_possible_descendants() & LEV_SEQUENCE_INFO);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
