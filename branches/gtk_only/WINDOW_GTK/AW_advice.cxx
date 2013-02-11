// ============================================================= //
//                                                               //
//   File      : AW_advice.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //





#include "aw_advice.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_root.hxx"

#define AWAR_ADVICE_TMP "/tmp/advices/"

#define AWAR_ADVICE_TEXT       AWAR_ADVICE_TMP "text"
#define AWAR_ADVICE_UNDERSTOOD AWAR_ADVICE_TMP "understood"

// -------------------------
//      internal data :

static bool     initialized = false;
static AW_root *advice_root = 0;

void init_Advisor(AW_root *awr) {
    if (!initialized) {
        advice_root  = awr;

        advice_root->awar_string(AWAR_ADVICE_TEXT, "<no advice>");
        advice_root->awar_int(AWAR_ADVICE_UNDERSTOOD, 0);

        initialized = true;
    }
}

void AW_advice(const char */*message*/,
               int type /*= AW_ADVICE_SIMPLE*/,
               const char */*title*/ /*= 0*/,
               const char */*corresponding_help*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}
