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
static AW_selection_list *clrTransTableLst;

void setVisualizeSAI_cb(AW_root *awr){
    int set = awr->awar(AWAR_SAI_ENABLE)->read_int();
    
    if(set)  ED4_ROOT->visualizeSAI = 1; 
    else     ED4_ROOT->visualizeSAI = 0; 
}

void awarColorDefTable_callback(AW_root *aw_root){
    char *clrTransTableName = aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
    char *clrDefName =    aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_DEF)->read_string();
    printf("%s\t%s\n", clrTransTableName,clrDefName);
    free(clrDefName);
    free(clrTransTableName);
}


#define BUFSIZE 30

static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];
#ifdef DEBUG
    int size =
#endif
    sprintf(buf,AWAR_SAI_CLR "%i", awarNo);
    e4_assert(size<BUFSIZE);
    return buf;
}

#undef BUFSIZE

/*--------------------------Creating and initializing AWARS----------------------------------------*/

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(    AWAR_SAI_ENABLE,                  0 , aw_def);
    aw_root->awar_string( AWAR_SAI_SELECT,                 "" , aw_def);
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TABLE,        "" , aw_def);
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_CREATE,   "" , aw_def);   
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_COPY,     "" , aw_def); 
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_DEF,      "" , aw_def);

    for (int i=0;i<10;i++){   // initialising 10 color definition string AWARS
       aw_root->awar_string(getAwarName(i),"",aw_def);
   }
    aw_root->awar(AWAR_SAI_ENABLE)->add_callback(setVisualizeSAI_cb);
    //    aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->add_callback(awarColorDefTable_callback);
}

void generateColorDefinitionString(AW_window *aws){
    AW_root *awr    = aws->get_root();
    void *clrDefStr = GBS_stropen(400);            /* create output stream */

    for (int i=0;i<10;i++){
        GBS_strcat(clrDefStr, awr->awar(getAwarName(i))->read_string());
        GBS_strcat(clrDefStr, ";");
    }
    awr->awar(AWAR_SAI_CLR_TRANS_TAB_DEF)->write_string(GBS_strclose(clrDefStr, 0));
    printf("\n%s\n",awr->awar(AWAR_SAI_CLR_TRANS_TAB_DEF)->read_string());
}

static bool inCallback = false;

void colorDefChange_callback(AW_window *aws, long int awarNo){
    if(!inCallback){
        inCallback   = true;
        AW_root *awr = aws->get_root();
        unsigned char charUsed[256]; memset(charUsed,255,256);

        {
            for (int i=0; i<10 ; i++){
                char *awarString_next = awr->awar_string(getAwarName(i))->read_string();
                for(int c=0; awarString_next[c]; ++c){
                    charUsed[awarString_next[c]] = i;
                }
                free(awarString_next);
            }
            
            char *awarString = awr->awar_string(getAwarName(awarNo))->read_string();
            for(int c=0; awarString[c]; ++c){
                charUsed[awarString[c]] = awarNo;
            }
            free(awarString);
        }

        typedef unsigned char mystr[256];
        mystr s[10];
        for (int i=0; i<10; i++)  s[i][0]=0; //initializing the strings
     
        for (int i=0; i<256; i++) {
            int table = charUsed[i];
            if (table != 255) {
                char *eos = strchr((char *)s[table],0); // get pointer to end of string
                eos[0] = char(i);
                eos[1] = 0;
            }
        }
        
        for(int i=0; i<10; i++){
            awr->awar_string(getAwarName(i))->write_string((char *)s[i]);
        }
        inCallback = false;
    }
}

void defineClrTransTable(AW_window *aws){
    generateColorDefinitionString(aws);
}

void createCopyClrTransTable(AW_window *aws, long int mode) {
    AW_root    *aw_root = aws->get_root();
    const char *newClrTransTabName;

    switch(mode){
    case CREATE_CLR_TR_TABLE:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_CREATE)->read_string());
        aws->insert_selection(clrTransTableLst, newClrTransTabName,newClrTransTabName);
        aws->update_selection_list(clrTransTableLst);
        break;
    case COPY_CLR_TR_TABLE:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_COPY)->read_string());
        aws->insert_selection(clrTransTableLst, newClrTransTabName, newClrTransTabName);
        aws->update_selection_list(clrTransTableLst);
        break;
    default:
        break;
    }
}

AW_window *create_copyColorTranslationTable_window(AW_root *aw_root){  // creates copy color tranlation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "COPY_CLR_TR_TABLE", "Copy Color Translation Table", 10,10 );
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_button(0,"Please enter the new name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_COPY,15);

    aws->at("ok");
    aws->callback(createCopyClrTransTable,0); // pass 0 as argument to copy color translation table
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

AW_window *create_createColorTranslationTable_window(AW_root *aw_root){ // creates create color tranlation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "CREATE_CLR_TR_TABLE", "Create Color Translation Table", 10,10 );
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_button(0,"Please enter the name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_CREATE,15);

    aws->at("ok");
    aws->callback(createCopyClrTransTable,1); //pass 1 as argument to create new color translation table
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

void deleteColorTranslationTable(AW_root *aw_root){
    int answer = aw_message("Are you sure to delete the selected COLOR TRANLATION TABLE?","OK,CANCEL");
    if(answer) return;
}

AW_window *create_editColorTranslationTable_window(AW_root *aw_root){  // creates edit color tranlation table window
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;
  
    aws = new AW_window_simple;
    aws->init( aw_root, "EDIT_CTT", "Color Translation Table", 400, 200 );
    aws->load_xfig("saiColorRange.fig");

    aws->at("range0");
    aws->callback(colorDefChange_callback,0);
    aws->create_input_field(AWAR_SAI_CLR_0,20);

    aws->at("range1");
    aws->callback(colorDefChange_callback,1);
    aws->create_input_field(AWAR_SAI_CLR_1,20);

    aws->at("range2");
    aws->callback(colorDefChange_callback,2);
    aws->create_input_field(AWAR_SAI_CLR_2,20);

    aws->at("range3");
    aws->callback(colorDefChange_callback,3);
    aws->create_input_field(AWAR_SAI_CLR_3,20);

    aws->at("range4");
    aws->callback(colorDefChange_callback,4);
    aws->create_input_field(AWAR_SAI_CLR_4,20);

    aws->at("range5");
    aws->callback(colorDefChange_callback,5);
    aws->create_input_field(AWAR_SAI_CLR_5,20);

    aws->at("range6");
    aws->callback(colorDefChange_callback,6);
    aws->create_input_field(AWAR_SAI_CLR_6,20);

    aws->at("range7");
    aws->callback(colorDefChange_callback,7);
    aws->create_input_field(AWAR_SAI_CLR_7,20);

    aws->at("range8");
    aws->callback(colorDefChange_callback,8);
    aws->create_input_field(AWAR_SAI_CLR_8,20);

    aws->at("range9");
    aws->callback(colorDefChange_callback,9);
    aws->create_input_field(AWAR_SAI_CLR_9,20);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    return (AW_window *)aws;
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
    clrTransTableLst = aws->create_selection_list(AWAR_SAI_CLR_TRANS_TABLE);
    aws->insert_default_selection(clrTransTableLst, "????", "????");
    aws->update_selection_list(clrTransTableLst);

    aws->at("edit");
    aws->button_length(10);
    aws->callback(AW_POPUP,(AW_CL)create_editColorTranslationTable_window,0);
    aws->create_button("EDIT","EDIT");
 
   aws->at("create");
   aws->callback(AW_POPUP,(AW_CL)create_createColorTranslationTable_window,0);
    aws->create_button("CREATE","CREATE");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_copyColorTranslationTable_window,0);
    aws->create_button("COPY","COPY");

    aws->at("delete");
    aws->callback((AW_CB1)deleteColorTranslationTable,0);
    aws->create_button("DELETE","DELETE");

    aws->show();
    return (AW_window *)aws;
}

