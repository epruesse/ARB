// ============================================================ //
//                                                              //
//   File      : AWT_misc.cxx                                   //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "awt_misc.hxx"

#include <arb_strarray.h>

#include <arbdb.h>

#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_advice.hxx>

AW_window *AWT_create_IUPAC_info_window(AW_root *aw_root) {
    // Open window showing IUPAC tables
    static AW_window_simple *aws = NULL;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "SHOW_IUPAC", "IUPAC info");
        aws->load_xfig("iupac.fig");

        aws->button_length(7);

        aws->at("ok");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "O");
    }
    return aws;
}

void AWT_insert_DBsaveType_selector(AW_window *aww, const char *awar_name) {
    aww->get_root()->awar_string(awar_name, "b"); // default=save binary database

    aww->label("Type ");
    aww->create_option_menu(awar_name, true);
    aww->insert_default_option("Binary",          "B", "b");
    aww->insert_option        ("Bin (+FastLoad)", "F", "bm");
    aww->insert_option        ("Ascii",           "A", "a");
    aww->update_option_menu();
}

static void warn_compression_type(AW_root*, AW_awar *awar_compression) {
    if (awar_compression->read_char_pntr()[0]) { // not empty
        AW_advice("Using extra compression is not compatible with older\n"
                  "arb-versions (prior to arb 6.1)!\n"
                  "\n"
                  "Databases will fail to load there..",
                  AW_ADVICE_TOGGLE,
                  "Extra database compression warning",
                  NULL);
    }
}

void AWT_insert_DBcompression_selector(AW_window *aww, const char *awar_name) {
    AW_awar *awar_compression = aww->get_root()->awar_string(awar_name, ""); // default = do not use extra compression

    aww->label("Compression ");

    ConstStrArray compressor;
    GBT_split_string(compressor, GB_get_supported_compression_flags(true), "=, ", true);
    arb_assert((compressor.size()%2) == 0); // should contain alternating flag/name entries

    aww->create_option_menu(awar_name, true);
    aww->insert_default_option("none", "N", "");
    for (int c = 0; compressor[c]; c += 2) {
        const char *flag = compressor[c];
        const char *name = compressor[c+1];
        arb_assert(strlen(flag) == 1);
        aww->insert_option(name, NULL, flag);
    }
    aww->update_option_menu();

    awar_compression->add_callback(makeRootCallback(warn_compression_type, awar_compression));
}
