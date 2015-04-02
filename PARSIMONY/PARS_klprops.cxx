// =============================================================== //
//                                                                 //
//   File      : PARS_klprops.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pars_klprops.hxx"
#include "pars_awars.h"

#include <aw_window.hxx>


AW_window *create_kernighan_properties_window(AW_root *aw_root) {

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "OPTIMIZATION_PROPS", "Kernighan Lin Properties");
    aws->load_xfig("pars/kernlin.fig");
    aws->button_length(10);

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("kernlin.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->button_length(6);

    aws->at("maxdepth");
    aws->create_input_field(AWAR_KL_MAXDEPTH);
    aws->at("incdepth");
    aws->create_input_field(AWAR_KL_INCDEPTH);


    aws->at("static");
    aws->create_toggle(AWAR_KL_STATIC_ENABLED);

    aws->button_length(4);
    aws->at("sred1"); aws->create_input_field(AWAR_KL_STATIC_DEPTH1);
    aws->at("sred2"); aws->create_input_field(AWAR_KL_STATIC_DEPTH2);
    aws->at("sred3"); aws->create_input_field(AWAR_KL_STATIC_DEPTH3);
    aws->at("sred4"); aws->create_input_field(AWAR_KL_STATIC_DEPTH4);
    aws->at("sred5"); aws->create_input_field(AWAR_KL_STATIC_DEPTH5);


    aws->at("dynamic");
    aws->create_toggle(AWAR_KL_DYNAMIC_ENABLED);

    aws->button_length(4);
    aws->at("start");
    aws->create_input_field(AWAR_KL_DYNAMIC_START);
    aws->at("maxx");
    aws->create_input_field(AWAR_KL_DYNAMIC_MAXX);
    aws->at("maxy");
    aws->create_input_field(AWAR_KL_DYNAMIC_MAXY);

#if 0
    aws->at("button_4");
    aws->create_option_menu(AWAR_KL_FUNCTION_TYPE);
    aws->insert_option("start_x^2", "d", AP_QUADRAT_START);
    aws->insert_option("max_x^2", "d", AP_QUADRAT_MAX);
    aws->insert_default_option("???", "?", AP_QUADRAT_START);
    aws->update_option_menu();
#endif
    return (AW_window *)aws;
}

