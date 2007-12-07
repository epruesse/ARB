//  =============================================================== //
//                                                                  //
//    File      : NT_MAUS.cxx                                       //
//    Purpose   :                                                   //
//    Time-stamp: <Wed Jan/05/2005 12:55 MET Coder@ReallySoft.de>   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in July 2004      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //


#include <stdlib.h>
#include <stdio.h>

#include <arbdb.h>
#include <awt.hxx>

#define AWAR_MAUS_INPUTNAME     "MAUS/inputname"
#define AWAR_MAUS_EXCLUDED_ACCS "MAUS/excl_acc"

extern GBDATA *gb_main;

void NT_create_MAUS_awars(AW_root *aw_root, AW_default aw_def, AW_default gb_def) {
    aw_root->awar_string(AWAR_MAUS_INPUTNAME, "", aw_def) ;
    aw_root->awar_string(AWAR_MAUS_EXCLUDED_ACCS, "", gb_def);
}

static void exclude_marked_species(AW_window*) {

}

static void MAUS(AW_window *aww) {
    AW_root *aw_root = aww->get_root();;

    char *inputname = aw_root->awar(AWAR_MAUS_INPUTNAME)->read_string();

    // system maus.pl


    free(inputname);
}


AW_window *NT_create_MAUS_window(AW_root *aw_root, AW_CL)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init( aw_root, "MAUS", "MAUS" );
    aws->load_xfig("maus.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"MAUS.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws, AWAR_MAUS_INPUTNAME);

    aws->at("acc_exclude");
    aws->callback(exclude_marked_species);
    aws->create_autosize_button("EXCLUDE","Exclude marked species");

    aws->at("filter");
    aws->callback(MAUS);
    aws->create_button("Filter","Filter","F");

    return (AW_window *)aws;
}




