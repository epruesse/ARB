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
#include <aw_window.hxx>


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


