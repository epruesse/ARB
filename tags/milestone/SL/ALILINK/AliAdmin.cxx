// ============================================================== //
//                                                                //
//   File      : AliAdmin.cxx                                     //
//   Purpose   : Provide alignment admin functionality            //
//               to NTREE and MERGE                               //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "AliAdmin.h"
#include <cctype>
#include <aw_window.hxx>

static struct {
    const char *id, *title;
} ALIGNMENT[ALI_ADMIN_TYPES] = {
    { "ALIGNMENT",        "alignment" },
    { "SOURCE_ALIGNMENT", "source alignment" },
    { "TARGET_ALIGNMENT", "target alignment" },
};

void AliAdmin::window_init(AW_window_simple *aw, const char *id_templ, const char *title_templ) const {
    char *id    = GBS_global_string_copy(id_templ,    ALIGNMENT[type].id);
    char *title = GBS_global_string_copy(title_templ, ALIGNMENT[type].title);

    title[0] = toupper(title[0]);

    aw->init(AW_root::SINGLETON, id, title);

    free(title);
    free(id);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void NOTEST_dummy() {
    TEST_REJECT(true);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

