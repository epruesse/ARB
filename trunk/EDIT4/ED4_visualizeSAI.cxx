#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>

#include "../NTREE/ad_ext.hxx"
#include "ed4_class.hxx"
#include "ed4_visualizeSAI.hxx"

extern GBDATA *gb_main;

/*--------------------------Creating and initializing AWARS----------------------------------------*/

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int( AWAR_SAI_ENABLE,                   0 , aw_def);
    aw_root->awar_string( AWAR_SAI_SELECT,                   "SAI not selected" , aw_def);
    aw_root->awar_string( AWAR_SAI_COLOR_TRANS_TABLE,        "" , aw_def);
    aw_root->awar_string( AWAR_SAI_COLOR_TRANS_TABLE_EDIT,   "" , aw_def);   
    aw_root->awar_string( AWAR_SAI_COLOR_TRANS_TABLE_COPY,   "" , aw_def); 
    aw_root->awar_string( AWAR_SAI_COLOR_TRANS_TABLE_DELETE, "" , aw_def);
}

void createColorTranslationTable(AW_root *aw_root) {
    ;
}

AW_window *create_editColorTranslationTableWindow(AW_root *aw_root){
;
}

AW_window *create_copyColorTranslationTableWindow(AW_root *aw_root){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "COPY_CLR_TR_TABLE", "Copy Color Translation Table", 10,10 );
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_button(0,"Please enter the new name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_COLOR_TRANS_TABLE_COPY,15);

    aws->at("ok");
    //    aws->callback(copyColorTranslationTable);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *create_createColorTranslationTableWindow(AW_root *aw_root){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "CREATE_CLR_TR_TABLE", "Create Color Translation Table", 10,10 );
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_button(0,"Please enter the name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_COLOR_TRANS_TABLE_CREATE,15);

    aws->at("ok");
    //    aws->callback(createColorTranslationTable,0);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void deleteColorTranslationTable(AW_root *aw_root){
    int answer = aw_message("Are you sure to delete the selected COLOR TRANLATION TABLE?","OK,CANCEL");
    if(answer) return;
}


void visualizeSAI(AW_window *aws){
;
}


AW_window *ED4_openSelectSAI_window(AW_root *aw_root){
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_SAI", "SELECT SAI", 400, 200 );
    aws->load_xfig("selectSAI.fig");

    aws->at("selection");
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_extendeds(gb_main,(AW_window *)aws,AWAR_SAI_SELECT);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

AW_window *ED4_createVisualizeSAI_window(AW_root *aw_root/*, AW_CL cl_ntw*/){

    ED4_createVisualizeSAI_Awars(aw_root,AW_ROOT_DEFAULT);
    AW_window_simple *aws = new AW_window_simple;

    aws->init( aw_root, "VISUALIZE_SAI", "VISUALIZE SAI", 150, 150 );
    aws->load_xfig("visualizeSAI.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"visualizeSAI.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("enable");
    aws->create_toggle(AWAR_SAI_ENABLE);

    aws->at("sai");
    aws->callback(AW_POPUP,(AW_CL)ED4_openSelectSAI_window,(AW_CL)0);
    aws->button_length(25);
    aws->create_button("SELECT_SAI", AWAR_SAI_SELECT);

    aws->at("clrTrList");
    AW_selection_list*  clrTransTableLst = aws->create_selection_list(AWAR_SAI_COLOR_TRANS_TABLE);

    aws->at("edit");
    aws->button_length(10);
    aws->callback(AW_POPUP,(AW_CL)create_editColorTranslationTableWindow,0);
    aws->create_button("EDIT","EDIT");
 
   aws->at("create");
   aws->callback(AW_POPUP,(AW_CL)create_createColorTranslationTableWindow,0);
    aws->create_button("CREATE","CREATE");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_copyColorTranslationTableWindow,0);
    aws->create_button("COPY","COPY");

    aws->at("delete");
    aws->callback((AW_CB1)deleteColorTranslationTable,0);
    aws->create_button("DELETE","DELETE");

    aws->at("visualize");
    aws->callback((AW_CB1)visualizeSAI,0);
    aws->create_button("VISUALIZE","VISUALIZE","V");

    aws->show();
    return (AW_window *)aws;
}

