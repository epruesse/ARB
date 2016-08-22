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

#include <aw_window.hxx>


AW_window *create_kernighan_window(AW_root *aw_root) {

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

    aws->at("nodes");
    aws->create_input_field("genetic/kh/nodes");
    aws->at("maxdepth");
    aws->create_input_field("genetic/kh/maxdepth");
    aws->at("incdepth");
    aws->create_input_field("genetic/kh/incdepth");


    aws->at("static");
    aws->create_toggle("genetic/kh/static/enable");

    aws->button_length(4);
    aws->at("input_6");
    aws->create_input_field("genetic/kh/static/depth0");
    aws->at("input_7");
    aws->create_input_field("genetic/kh/static/depth1");
    aws->at("input_8");
    aws->create_input_field("genetic/kh/static/depth2");
    aws->at("input_9");
    aws->create_input_field("genetic/kh/static/depth3");
    aws->at("input_10");
    aws->create_input_field("genetic/kh/static/depth4");


    aws->at("dynamic");
    aws->create_toggle("genetic/kh/dynamic/enable");

    aws->button_length(4);
    aws->at("start");
    aws->create_input_field("genetic/kh/dynamic/start");
    aws->at("maxx");
    aws->create_input_field("genetic/kh/dynamic/maxx");
    aws->at("maxy");
    aws->create_input_field("genetic/kh/dynamic/maxy");

#if 0
    aws->at("button_4");
    aws->create_option_menu("genetic/kh/function_type");
    aws->insert_option("start_x^2", "d", AP_QUADRAT_START);
    aws->insert_option("max_x^2", "d", AP_QUADRAT_MAX);
    aws->insert_default_option("???", "?", AP_QUADRAT_START);
    aws->update_option_menu();
#endif
    return (AW_window *)aws;
}

