#include <cstdio>
#include <iostream.h>
#include <memory.h>
#include <cstring>
#include <arbdb.h>
#include <arbdbt.h>
#include <cmath>
#include <servercntrl.h>
#include <cstdlib>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_display.hxx>
#include "AP_buffer.hxx"
#include "ap_main.hxx"
#include "ap_tree_nlen.hxx"
#include "GA_genetic.hxx"
#include <phwin.hxx>
#include <ntcanvas.hxx>

void start_genetic(GBDATA *);
void quit_genetic();
AP_ERR * make_start_population(GBDATA *gbmain, AP_tree *tree);
extern FILE *GAout;
extern GA_genetic *GAgenetic;

void start_genetic_cb(AW_window *, AW_CL) {
    start_genetic(ap_main->gb_main);
}

void savetree_genetic_cb(AW_window */*aww*/, AW_CL /*cld1*/) {
    // AW_root *aw_root = aww->get_root();

}

void test_genetic_cb(AW_window */*aww*/, AW_CL /*cld1*/) {
    // AW_root *aw_root = aww->get_root();
}

void save_genetic_cb(AW_window *aww, AW_CL /*cld1*/) {
    AW_root *aw_root = aww->get_root();
    int cluster = (int)aw_root->awar("genetic/presets/curCluster")->read_int();
    GAgenetic->put_start_tree(ap_main->tree_root, 0, cluster);
    return;
}

void quit_genetic_cb(AW_window */*aww*/, AW_CL /*cld1*/) {
    quit_genetic();
    // AW_root *aw_root = aww->get_root();
}

void create_genetic_variables(AW_root *aw_root, AW_default def)
{

    aw_root->awar_int("genetic/presets/max_jobs", 50, def);
    aw_root->awar_int("genetic/presets/max_cluster", 4, def);
    aw_root->awar_int("genetic/presets/jobOpt", 2, def);
    aw_root->awar_int("genetic/presets/jobCross", 2, def);
    aw_root->awar_int("genetic/presets/jobOther", 2, def);
    aw_root->awar_int("genetic/presets/curCluster", 0, def);
    aw_root->awar_float("genetic/presets/bestTree", 0, def);
    aw_root->awar_int("genetic/presets/jobCount", 0, def);
    aw_root->awar_int("genetic/presets/maxTree", 100, def);
}

AW_window *create_genetic_window(AW_root *aw_root, AW_display *awd)
{

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "GENETIC_MAIN", "Kernighan", 10, 10);
    aws->load_xfig("ph_gen.fig");
    aws->button_length(10);

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");


    aws->at("start");
    aws->callback((AW_CB1)start_genetic_cb, (AW_CL)awd);
    aws->create_button("START", "START", "S");

    aws->at("save");
    aws->callback((AW_CB1)save_genetic_cb, (AW_CL)aw_root);
    aws->create_button("SAVE", "SAVE", "S");

    aws->at("quit");
    aws->callback((AW_CB1)quit_genetic_cb, (AW_CL)aw_root);
    aws->create_button("QUIT", "QUIT", "Q");

    aws->at("savetree");
    aws->callback((AW_CB1)savetree_genetic_cb, (AW_CL)aw_root);
    aws->create_button("SAVETREE", "SaveTree", "Q");

    aws->at("jobs");
    aws->create_input_field("genetic/presets/max_jobs", 8);
    aws->at("cluster");
    aws->create_input_field("genetic/presets/max_cluster", 8);
    aws->at("jobOpt");
    aws->create_input_field("genetic/presets/jobOpt", 8);
    aws->at("jobCross");
    aws->create_input_field("genetic/presets/jobCross", 8);
    aws->at("jobOther");
    aws->create_input_field("genetic/presets/jobOther", 8);
    aws->at("curCluster");
    aws->create_input_field("genetic/presets/curCluster", 8);
    aws->at("maxTree");
    aws->create_input_field("genetic/presets/maxTree", 8);

    aws->at("jobCount");
    aws->create_label("genetic/presets/jobCount", 8);

    return (AW_window *)aws;

}
