#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>

#include "nt_tipsAndTricks.hxx"
#include "../MULTI_PROBE/SoTl.hxx"

List<char> *tipsList;
static bool bNoMoreTips      = false;
static bool bNewTips         = false;
static bool bEditTips        = false;
static char *pt_cExistingTip = "NULL";

void buildTipsAndTricksList(AW_root *awr) {

    char *pt_cTok; int i = 0;
    char *pt_cTipsAndTricksDataBase = awr->awar(AWAR_TIPS_AND_TRICKS_DATABASE)->read_string();
    
    tipsList = new List<char>;

    for (pt_cTok = strtok(pt_cTipsAndTricksDataBase,";"); 
         pt_cTok; 
         pt_cTok = strtok(0,";"),i++) 
        {    
            tipsList->insert_as_last(strdup(pt_cTok));
        }

    awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string(tipsList->get_first());

    free(pt_cTipsAndTricksDataBase);
}

void saveTipsAndTricksDatabase(AW_root *awr){

    char *pt_cTipsAndTricks         = awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->read_string();
    char *pt_cTipsAndTricksDataBase = awr->awar(AWAR_TIPS_AND_TRICKS_DATABASE)->read_string();

    List<char> *tempList = new List<char>;    
    void *buf = GBS_stropen(strlen(pt_cTipsAndTricksDataBase) + strlen(pt_cTipsAndTricks) + 10);

    if ((strcmp(pt_cTipsAndTricks,"") != 0) && 
        (strcmp(pt_cTipsAndTricks,"No more TIPS! :-(") != 0)) 
        {
            if (bNewTips) {
                tipsList->insert_as_last(pt_cTipsAndTricks);
            }
            if (bEditTips && (strcmp(pt_cTipsAndTricks,pt_cExistingTip) != 0)) {
                char *temp = tipsList->get_first();
                while (temp) {
                    if (strcmp(temp,pt_cExistingTip) == 0) {
                        tempList->insert_as_last(pt_cTipsAndTricks);
                    }
                    else {
                        tempList->insert_as_last(temp);
                    }
                    temp = tipsList->get_next();
                }
            }
            char *tips = 0;
            if (bNewTips)       tips = tipsList->get_first();
            else if (bEditTips) tips = tempList->get_first();

            while(tips) {
                GBS_strcat(buf,tips); 
                GBS_strcat(buf,";");
                if (bNewTips)       tips = tipsList->get_next();
                else if (bEditTips) tips = tempList->get_next();
            }
            if(bNewTips || bEditTips)
                awr->awar(AWAR_TIPS_AND_TRICKS_DATABASE)->write_string(GBS_strclose(buf, 0)); // finally writing to the database AWAR
        }

    delete tempList;  // no more required
    delete tipsList;  // delete the tipsList and the same will be created in buildTipsAndTricksList();
    buildTipsAndTricksList(awr);

    bNewTips = bEditTips = false;
    free(pt_cTipsAndTricksDataBase);
    free(pt_cTipsAndTricks);
}

void tipsAndTricksAdmin(AW_window *aws, long int mode) {

    AW_root *awr = aws->get_root();
    char *pt_cTipsAndTricks = awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->read_string();

    switch (mode){
    case TIPS_NEW: //add a new tip/trick to the tipsDatabase
        bNewTips = true;
        awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string("");
        break;

    case TIPS_EDIT: //edit the existing tip/trick in the tipsDatabase
        bEditTips = true;
        pt_cExistingTip = strdup(pt_cTipsAndTricks);
        aw_message("You can edit this tip by making your changes\n to the displayed tip!");
        break;

    case TIPS_SAVE: //save respective properties files to save to tipsDatabase
        saveTipsAndTricksDatabase(awr);
        aw_message("Save Properties inorder to save your Tips\n for future sessions!");
        break;

    case TIPS_NEXT: //display next tip in the tipsDatabase
        if ( (strcmp(pt_cTipsAndTricks,"") == 0) || bNoMoreTips) {
            awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string(tipsList->get_first());
            bNoMoreTips = false;
        }
        else {
            awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string(tipsList->get_next());
            free(pt_cTipsAndTricks);
            pt_cTipsAndTricks =  awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->read_string();
            if (strcmp(pt_cTipsAndTricks,"") == 0) {
                awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string("No more TIPS! :-(");
                bNoMoreTips = true;
            }
        }
        break;

    case TIPS_PREVIOUS: //display previous tip in the tipsDatabase
        if ( (strcmp(pt_cTipsAndTricks,"") == 0) || bNoMoreTips ) {
            awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string(tipsList->get_last());
            bNoMoreTips = false;
        }
        else {
            awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string(tipsList->get_prev());
            free(pt_cTipsAndTricks);
            pt_cTipsAndTricks =  awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->read_string();
            if (strcmp(pt_cTipsAndTricks,"") == 0) {
                awr->awar(AWAR_TIPS_AND_TRICKS_DISP)->write_string("No more TIPS! :-(");
                bNoMoreTips = true;
            }
        }
        break;

    default:
        break; 
    }
    free(pt_cTipsAndTricks);
}

/* ------------------------TIPS AND TRICKS WINDOW ---------------------- */

AW_window *createTipsAndTricks_window(AW_root *root, AW_default aw_def) {

    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "TIPS_AND_TRICKS", "Tips and Tricks in ARB", 500, 400);
    aws->load_xfig("tipsAndTricks.fig");

    root->awar_string(AWAR_TIPS_AND_TRICKS_DISP);
    root->awar_string(AWAR_TIPS_AND_TRICKS_DATABASE);
    root->awar_string(AWAR_TIPS_AND_TRICKS_LIST);

    buildTipsAndTricksList(root);

    aws->button_length(10);
    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("new");
    aws->callback(tipsAndTricksAdmin,0);
    aws->create_button("NEW","NEW","N");

    aws->at("edit");
    aws->callback(tipsAndTricksAdmin,1);
    aws->create_button("EDIT","EDIT","D");

    aws->at("save");
    aws->callback(tipsAndTricksAdmin,2);
    aws->create_button("SAVE","SAVE","S");

    aws->at("next");
    aws->callback(tipsAndTricksAdmin,3);
    aws->create_button("NEXT","NEXT >>","X");

    aws->button_length(11);
    aws->at("previous");
    aws->callback(tipsAndTricksAdmin,4);
    aws->create_button("PREVIOUS","<< PREVIOUS","P");

    aws->at("tips");
    aws->create_text_field(AWAR_TIPS_AND_TRICKS_DISP);

    return (AW_window *)aws;
}

