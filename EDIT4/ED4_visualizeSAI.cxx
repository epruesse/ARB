/*=======================================================================================*/
/*                                                                                       */
/*    File       : ED4_visualizeSAI.cxx                                                  */
/*    Purpose    : To Visualise the Sequence Associated Information (SAI) in the Editor  */
/*    Time-stamp : Tue Apr 1 2003                                                        */
/*    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                     */
/*    web site   : http://www.arb-home.de/                                               */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/

#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>

#include "../NTREE/ad_ext.hxx"
#include "ed4_class.hxx"
#include "ed4_defs.hxx"
#include "ed4_visualizeSAI.hxx"

extern GBDATA *gb_main;
static AW_selection_list *clrTransTableLst;
AW_window_simple *aws = new AW_window_simple;
static int seqBufferSize = 100;
static char *saiColors   = (char*)GB_calloc(seqBufferSize, sizeof(char));

/* --------------------------------------------------------- */
#define BUFSIZE 100

static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];
#ifdef DEBUG
    int size =
#endif
    sprintf(buf,AWAR_SAI_CLR "%i", awarNo);
    e4_assert(size<BUFSIZE);
    return buf;
}

static const char *getClrDefAwar(char *awarName) {
    static char buf[BUFSIZE];
#ifdef DEBUG
    int size =
#endif
    sprintf(buf,AWAR_SAI_CLR_DEF "%s", awarName);
    e4_assert(size<BUFSIZE);
    return buf;
}

#undef BUFSIZE
/* --------------------------------------------------------- */

void setVisualizeSAI_cb(AW_root *awr) {
    int set = awr->awar(AWAR_SAI_ENABLE)->read_int();
    
    if(set)  ED4_ROOT->visualizeSAI = 1; 
    else     ED4_ROOT->visualizeSAI = 0; 

    //refresh editor
    ED4_ROOT->refresh_all_windows(1);
}

void setVisualizeSAI_options_cb(AW_root *awr) {
    int set = awr->awar(AWAR_SAI_ALL_SPECIES)->read_int();

    if(set)  ED4_ROOT->visualizeSAI_allSpecies = 1; 
    else     ED4_ROOT->visualizeSAI_allSpecies = 0; 

    //refresh editor
    ED4_ROOT->refresh_all_windows(1);
}

static bool inCallback = false;

void colorDefTabNameChanged_callback(AW_root *awr) {
    char *clrTabName = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();

    if (strcmp(clrTabName,"") != 0){
        inCallback = true;
        {
            for(int i=0; i<10 ; i++)  awr->awar_string(getAwarName(i))->write_string("");

            if(clrTabName[0]!='?') {
                char *clrTabDef  = awr->awar(getClrDefAwar(clrTabName))->read_string();
                char *tok; int i = 0;

                if(strcmp(clrTabDef,"")!=0) {
                    for (tok = strtok(clrTabDef,";"); tok; tok = strtok(0,";"),i++) {
                        awr->awar_string(getAwarName(i))->write_string(tok);
                    }
                } else for(int i=0; i<10 ; i++)  awr->awar_string(getAwarName(i))->write_string("");
                free(clrTabDef);
            }
        }
        inCallback = false;
     
        char *saiName = awr->awar(AWAR_SAI_SELECT)->read_string();
        char buf[100];  sprintf(buf, AWAR_SAI_CLR_TAB "%s", saiName);  
        awr->awar_string(buf, "", AW_ROOT_DEFAULT);                 //creating a AWAR for the selected SAI and 
        awr->awar(buf)->write_string(clrTabName);                   // writing the existing clr trans table names to the same
        free(saiName);
        
        {
            void *transTabNamesStr = GBS_stropen(500);            
            const char *transTabName = clrTransTableLst->first_element();
            while(transTabName){
                GBS_strcat(transTabNamesStr,transTabName);
                GBS_strcat(transTabNamesStr,"\n");
                transTabName = clrTransTableLst->next_element();
            }
            awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->write_string(GBS_strclose(transTabNamesStr, 0));            
        }
    }
    free(clrTabName);
}

void saiChanged_callback(AW_root *awr) {
    char *saiName = awr->awar(AWAR_SAI_SELECT)->read_string();

    char buf[100];  sprintf(buf, AWAR_SAI_CLR_TAB "%s", saiName);                  
    awr->awar_string(buf, "???", AW_ROOT_DEFAULT);
    char *transTabName = awr->awar(buf)->read_string();

    int index = aws->get_index_of_element(clrTransTableLst, transTabName); // save old position
    aws->select_index(clrTransTableLst, AWAR_SAI_CLR_TRANS_TABLE, index);
    aws->update_selection_list(clrTransTableLst);

    free(transTabName);
    free(saiName);

    //refresh editor
    ED4_ROOT->refresh_all_windows(1);
}

void colorDefChange_callback(AW_window *aws, long int awarNo){
    if(!inCallback){
        inCallback   = true;
        AW_root *awr = aws->get_root();
        unsigned char charUsed[256]; memset(charUsed,255,256);
        void *clrDefStr = GBS_stropen(500);            /* create output stream */

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
            GBS_strcat(clrDefStr, (char *)s[i]);
            GBS_strcat(clrDefStr, " ;");
        }
        char *clrTabName = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
        if(clrTabName[0]!='?') awr->awar(getClrDefAwar(clrTabName))->write_string(GBS_strclose(clrDefStr, 0));
        free(clrTabName);
        
        inCallback = false;
    }
    //refresh editor
    ED4_ROOT->refresh_all_windows(1);
}

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def) {  // --- Creating and initializing AWARS -----
    aw_root->awar_int(    AWAR_SAI_ENABLE,                  0 , aw_def);
    aw_root->awar_int(    AWAR_SAI_ALL_SPECIES,             0 , aw_def);
    aw_root->awar_string( AWAR_SAI_SELECT,                 "" , aw_def);
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TABLE,        "" , aw_def);
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_CREATE,   "" , aw_def);   
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_COPY,     "" , aw_def); 
    aw_root->awar_string( AWAR_SAI_CLR_TRANS_TAB_NAMES,  "???" , aw_def); 

    for (int i=0;i<10;i++){   // initialising 10 color definition string AWARS
       aw_root->awar_string(getAwarName(i),"",aw_def);
    }
    aw_root->awar(AWAR_SAI_ENABLE)->add_callback(setVisualizeSAI_cb);
    aw_root->awar(AWAR_SAI_ALL_SPECIES)->add_callback(setVisualizeSAI_options_cb);
    aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->add_callback(colorDefTabNameChanged_callback);
    aw_root->awar(AWAR_SAI_SELECT)->add_callback(saiChanged_callback);
}

void createCopyClrTransTable(AW_window *aws, long int mode) {
    AW_root *aw_root = aws->get_root();
    char    *newClrTransTabName = 0;
    char    *clrTabSourceName   = 0;

    switch(mode){
    case CREATE_CLR_TR_TABLE:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_CREATE)->read_string());
        aw_root->awar_string(getClrDefAwar(newClrTransTabName), "" , AW_ROOT_DEFAULT);        
        aws->insert_selection(clrTransTableLst, newClrTransTabName,newClrTransTabName);
        aws->update_selection_list(clrTransTableLst);
        break;
    case COPY_CLR_TR_TABLE:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_COPY)->read_string());
        clrTabSourceName   = aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
        if(clrTabSourceName[0]!='?') {
            aw_root->awar_string(getClrDefAwar(newClrTransTabName), aw_root->awar(getClrDefAwar(clrTabSourceName))->read_string(), AW_ROOT_DEFAULT);            
        }
        aws->insert_selection(clrTransTableLst, newClrTransTabName, newClrTransTabName);
        aws->update_selection_list(clrTransTableLst);
        break;
    default:
        break;
    }
    free(clrTabSourceName);
    free(newClrTransTabName);
}

void deleteColorTranslationTable(AW_window *aws){
    AW_root *aw_root = aws->get_root();

    int answer = aw_message("Are you sure to delete the selected COLOR TRANLATION TABLE?","OK,CANCEL");
    if(answer) return;

    char *clrTabName = aw_root->awar_string(AWAR_SAI_CLR_TRANS_TABLE)->read_string();

    if(clrTabName[0]!='?') {
        aws->delete_selection_from_list(clrTransTableLst, clrTabName);
        aws->insert_default_selection(clrTransTableLst, "????", "????");
        aws->update_selection_list(clrTransTableLst);
    }

    aw_root->awar_string(getClrDefAwar(clrTabName))->write_string("");
    free(clrTabName);

    //refresh editor
    ED4_ROOT->refresh_all_windows(1);
}

AW_selection_list *buildClrTransTabNamesList(AW_window *aws, AW_selection_list *clrTransTabNamesList){
    AW_root *awr           = aws->get_root();
    clrTransTabNamesList   = aws->create_selection_list(AWAR_SAI_CLR_TRANS_TABLE);
    char *clrTransTabNames = awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->read_string();
    char *tok;
   
    if(strcmp(clrTransTabNames,"???")!=0) {
        for (tok = strtok(clrTransTabNames,"\n"); tok; tok = strtok(0,"\n")) {
            awr->awar_string(getClrDefAwar(tok), "" , AW_ROOT_DEFAULT);         
            aws->insert_selection(clrTransTabNamesList,tok,tok);
        }
    }
    aws->insert_default_selection(clrTransTabNamesList, "????", "????");
    aws->update_selection_list(clrTransTabNamesList);

    free(clrTransTabNames);

    return clrTransTabNamesList;
}

int checkSai(const char *species_name) {
    int matchesSai = 0;
    
    for( GBDATA *gb_extended = GBT_first_SAI(gb_main); 
         gb_extended;
         gb_extended = GBT_next_SAI(gb_extended)){
        GBDATA *gb_name = GB_find(gb_extended,"name",0,down_level);
        if(gb_name){
            char *saiName = GB_read_char_pntr(gb_name);
            if(strcmp(species_name,saiName)==0) matchesSai = 1;
        }
    }
    return matchesSai;
}

char *getSaiColorString(int start, int end) {

    e4_assert(start<=end);

    int seqSize = end-start+1;

    if(seqSize>seqBufferSize){
        free(saiColors);
        seqBufferSize = seqSize;
        saiColors =  (char*)GB_calloc(seqBufferSize, sizeof(char));    
    }
    else memset(saiColors,0,sizeof(char)*seqSize);

    AW_root *awr      = aws->get_root();
    char *saiSelected = awr->awar(AWAR_SAI_SELECT)->read_string();

    GB_push_transaction(gb_main);
    char *alignment_name     = GBT_get_default_alignment(gb_main);
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_extended      = GBT_find_SAI_rel_exdata(gb_extended_data, saiSelected);

    if (gb_extended) {
        GBDATA *saiSequence = GBT_read_sequence(gb_extended,alignment_name);
        if (saiSequence) {
            char *saiData = GB_read_string(saiSequence);
            
            char buf[100];  sprintf(buf, AWAR_SAI_CLR_TAB "%s", saiSelected);                  
            awr->awar_string(buf, "", AW_ROOT_DEFAULT);
            char *clrTransTabName = awr->awar(buf)->read_string();

            if (strcmp(clrTransTabName,"") != 0 && clrTransTabName[0] != '?') {
                char *clrTransTabString = awr->awar(getClrDefAwar(clrTransTabName))->read_string(); 
                char *tok; int clrRange = ED4_G_CBACK_0;        //clrRange is defined from RANGE 0 ... RANGE 9 as ED4_G_CBACK_0 .. ED4_G_CBACK_9 
                                                                // and stores values from ED4_G_CBACK_0 to +10 for the matched character in SAI to saiColors  
                if (strcmp(clrTransTabString,"") != 0) {        // and stores 0 (ED4_G_STANDARD) for the unmatched character
                    for (tok = strtok(clrTransTabString,";"); tok; tok = strtok(0,";"), clrRange++) {
                        int j = 0; 
                        while(tok[j]) {
                            for (int i = start; i<=end; i++) {
                                if(tok[j]==saiData[i])
                                    saiColors[i] = clrRange;
                            }
                            j++;
                        }
                    }
                }
                free(clrTransTabString);
            }
            free(clrTransTabName); free(saiData);
        }
    }
    free(alignment_name); free(saiSelected);
    GB_pop_transaction(gb_main);

    if (saiColors){
        for (int i = end-start; i>=0; i--) {                      // checking for the negative values 
            if(saiColors[i]<0) saiColors[i] = ED4_G_STANDARD;     // setting Background color as a default for the negative values
        }
        return saiColors;
    }
    else  return 0;
}


/* -------------------- Creating Windows and Display dialogs -------------------- */

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

AW_window *ED4_createVisualizeSAI_window(AW_root *aw_root){

    ED4_createVisualizeSAI_Awars(aw_root,AW_ROOT_DEFAULT);

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
    clrTransTableLst = buildClrTransTabNamesList(aws,clrTransTableLst);
    if (const char *transTabName = clrTransTableLst->first_element()) {
        saiChanged_callback(aw_root);
    }

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

    aws->at("marked");
    aws->create_toggle_field(AWAR_SAI_ALL_SPECIES,1);
    aws->insert_toggle("MARKED SPECIES", "M", 0);
    aws->insert_toggle("ALL SPECIES", "A", 1);
    aws->update_toggle_field();

    aws->show();
    return (AW_window *)aws;
}

