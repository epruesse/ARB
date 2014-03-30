#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <malloc.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>

#include "gde.hxx"
#include "GDE_menu.h"
#include "GDE_def.h"
#include "GDE_extglob.h"

#define META "M"


AW_CL agde_filtercd;

extern void GDE_startaction_cb(AW_window *aw,AWwindowinfo *AWinfo,AW_CL cd);

Gmenu menu[GDEMAXMENU];
int num_menus = 0,repeat_cnt = 0;
//Frame frame,pframe,infoframe;
//Panel popup,infopanel;
//Panel_item left_foot,right_foot;
//Canvas EditCan,EditNameCan;
int DisplayType;
GmenuItem *current_item;
NA_Alignment *DataSet = NULL;
//NA_Alignment *Clipboard = NULL;
//char **TextClip;
//int TextClipSize = 0,TextClipLength = 0;

char GDEBLANK[] = "\0";

enum {
	SLIDERWIDTH = 5, // input field for numbers
	TEXTFIELDWIDTH = 15
	};

char *GDE_makeawarname(AWwindowinfo *AWinfo,long i);

int GDE_odd(long a)
{
	if(((a/2)*2)==a) return 0;
	return 1;
}

void GDE_showhelp_cb(AW_window *aw,AWwindowinfo *AWinfo,AW_CL cd)
{
	AWUSE(cd);
	AW_root *awr=aw->get_root();
	static AW_window_simple *helpwindow=0;

	char helpfile[1024], *help_file;
	char *helptext=0;	

	help_file=AWinfo->gmenuitem->help;
	if(!(help_file)) awr->awar("tmp/gde/helptext")->write_string("no help\0");
	else 
	{
		sprintf(helpfile,"%s/GDEHELP/%s",GB_getenvARBHOME(),help_file);

		helptext=GB_read_file(helpfile);
		if(helptext) 
		{
			awr->awar("tmp/gde/helptext")->write_string(helptext);
			delete helptext;
		}
		else awr->awar("tmp/gde/helptext")->write_string("can not find help file\0");
	}
	
	if(helpwindow) { helpwindow->show() ; return; }
	
	helpwindow=new AW_window_simple;
		helpwindow->init(awr,"GDE_HELP", "GDE HELP",200,10);
	
	helpwindow->button_length(10);
	helpwindow->at(10,10);helpwindow->callback((AW_CB0)AW_POPDOWN);
	helpwindow->create_button("CLOSE", "CLOSE","C");

	helpwindow->at(10,40);
	//helpwindow->button_length(80);
	//helpwindow->label_length(80);
	helpwindow->create_text_field("tmp/gde/helptext",80,20);

	helpwindow->window_fit();	
	helpwindow->show();
	
}

char *GDE_makeawarname(AWwindowinfo *AWinfo,long i)
{
	char name[GB_KEY_LEN_MAX*4+5];
	char *gmenu = GBS_string_2_key(AWinfo->gmenu->label);
	char *gmenuitem = GBS_string_2_key(AWinfo->gmenuitem->label);
	char *arg = GBS_string_2_key(AWinfo->gmenuitem->arg[i].symbol);

	sprintf(name,"gde/%s/%s/%s",gmenu,gmenuitem,arg);
	delete gmenu;
	delete gmenuitem;
	delete arg;

	return(strdup(name));
}

void GDE_decreaseawar_cb(AW_window *aws,char *awar,AW_CL cd)
{
	AWUSE(cd);
	AW_root *awr=aws->get_root();
	awr->awar(awar)->write_int(awr->awar(awar)->read_int()-1);
}

void GDE_increaseawar_cb(AW_window *aws,char *awar,AW_CL cd)
{
	AWUSE(cd);
	AW_root *awr=aws->get_root();
	awr->awar(awar)->write_int(awr->awar(awar)->read_int()+1);
}


void GDE_create_infieldwithpm(AW_window *aws,char *newawar,long width)
{
	char *awar=strdup(newawar);
	aws->create_input_field(newawar,(int)width);
	if (aws->get_root()->awar(newawar)->get_type() == AW_INT) {
	    aws->button_length(3);
	    aws->callback((AW_CB2)GDE_decreaseawar_cb,(AW_CL)awar,0);
	    aws->create_button(0,"-","-");
	    aws->callback((AW_CB2)GDE_increaseawar_cb,(AW_CL)awar,0);
	    aws->create_button(0,"+","+");
	}
}

char *gde_filter_weights(GBDATA *gb_sai,AW_CL ){
    char *ali_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_ali = GB_find(gb_sai,ali_name,0,down_level);
    delete ali_name;
    if (!gb_ali) return 0;
    GBDATA *gb_type = GB_find(gb_ali, "_TYPE",0,down_level);
    if (!gb_type) return 0;
    char *type = GB_read_char_pntr(gb_type);
    if (GBS_string_cmp( type,"PV?:*",0)) {
	return 0;
    }
    
    char *name = GBT_read_name(gb_sai);
    const char *res = GBS_global_string("%s: %s",name,type);
    delete name;
    return strdup(res);
}


		  
AW_window *GDE_menuitem_cb(AW_root *aw_root,AWwindowinfo *AWinfo) {
#define BUFSIZE 200
    char bf[BUFSIZE+1]; 
#if defined(DEBUG)
    int printed =
#endif // DEBUG
        sprintf(bf,"GDE / %s / %s",AWinfo->gmenu->label,AWinfo->gmenuitem->label); 
    gb_assert(printed<=BUFSIZE);
    
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root,bf,bf,10,10);
    
    switch (gde_cgss.wt) {
        case CGSS_WT_DEFAULT:
            aws->load_xfig("gdeitem.fig");
            break;
        case CGSS_WT_EDIT:
            aws->load_xfig("gde2item.fig");
            break;
        case CGSS_WT_EDIT4:
            aws->load_xfig("gde3item.fig");
            break;
        default:
            gde_assert(0);
    }
    
    aws->set_window_size(1000,2000);
    aws->button_length(10);
    aws->at(10,10);
    aws->auto_space(0,10);

    aws->at("help");
    aws->callback((AW_CB2)GDE_showhelp_cb,(AW_CL)AWinfo,0);
    aws->create_button("GDE_HELP","HELP...","H");

    aws->at("start");
    aws->callback((AW_CB2)GDE_startaction_cb,(AW_CL)AWinfo,0);
    aws->create_button("GO", "GO","O");

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");


    if (AWinfo->gmenuitem->numinputs>0) {
        switch (gde_cgss.wt) {
            case CGSS_WT_DEFAULT:
                aws->at("which_alignment");
                awt_create_selection_list_on_ad(gb_main, (AW_window *)aws,"tmp/gde/alignment","*=");
                aws->at( "which_species" );
                aws->create_toggle_field( "gde/species" );
                aws->insert_toggle( "all", "a", 0 );
                aws->insert_default_toggle( "marked",  "m", 1 );
                aws->update_toggle_field();
                break;
            case CGSS_WT_EDIT:
                aws->at("bottom"); aws->create_toggle("gde/bottom_area");
                aws->at("bottomsai"); aws->create_toggle("gde/bottom_area_sai");
                aws->at("bottomh"); aws->create_toggle("gde/bottom_area_helix");
                goto both_edits;
            case CGSS_WT_EDIT4:
                aws->at("topk"); aws->create_toggle("gde/top_area_kons");
                aws->at("middlek"); aws->create_toggle("gde/middle_area_kons");
                aws->at("topr"); aws->create_toggle("gde/top_area_remark");
                aws->at("middler"); aws->create_toggle("gde/middle_area_remark");
                goto both_edits;
        both_edits:
                aws->at("top"); aws->create_toggle("gde/top_area");
                aws->at("topsai"); aws->create_toggle("gde/top_area_sai");
                aws->at("toph"); aws->create_toggle("gde/top_area_helix");
                aws->at("middle"); aws->create_toggle("gde/middle_area");
                aws->at("middlesai"); aws->create_toggle("gde/middle_area_sai");
                aws->at("middleh"); aws->create_toggle("gde/middle_area_helix");
                break;
        }
	
        aws->at("compression");
        aws->create_toggle_field( "gde/compression" );
        aws->insert_toggle( "none", "n",0 ); 
        aws->insert_default_toggle( "vertical gaps", "v", 1 );
        aws->insert_toggle( "all gaps",  "g", 2 );
        aws->update_toggle_field();

        aws->button_length(12);
        aws->at("filtername");
        agde_filtercd = awt_create_select_filter(aws->get_root(),gb_main,"gde/filter/name");
        aws->callback((AW_CB2)AW_POPUP,(AW_CL)awt_create_select_filter_win,agde_filtercd);
        aws->create_button("SELECT_FILTER", "gde/filter/name");

        aws->at("paramsb");
    }
    else {
        aws->at("paramsb");
    }

	
    long labellength=0,lablen;
    //char *help;
    labellength=1;
    long i;
    for(i=0;i<AWinfo->gmenuitem->numargs;i++)
    {
        //help=AWinfo->gmenuitem->help;
        if(!(AWinfo->gmenuitem->arg[i].label)) AWinfo->gmenuitem->arg[i].label=GDEBLANK;
        lablen=strlen(AWinfo->gmenuitem->arg[i].label);	
        if(lablen>labellength) labellength=lablen;
    }
    aws->label_length((int)labellength);
    aws->auto_space(0,0);

    for(i=0;i<AWinfo->gmenuitem->numargs;i++)
    {
        GmenuItemArg itemarg=AWinfo->gmenuitem->arg[i];

        if(itemarg.type==SLIDER)
        {
            char *newawar=GDE_makeawarname(AWinfo,i);
            if ( int(AWinfo->gmenuitem->arg[i].fvalue) == AWinfo->gmenuitem->arg[i].fvalue &&
                 int(AWinfo->gmenuitem->arg[i].min) == AWinfo->gmenuitem->arg[i].min &&
                 int(AWinfo->gmenuitem->arg[i].max) == AWinfo->gmenuitem->arg[i].max){
                aw_root->awar_int(newawar,(long)AWinfo->gmenuitem->arg[i].fvalue,AW_ROOT_DEFAULT);
            }else{
                aw_root->awar_float(newawar,AWinfo->gmenuitem->arg[i].fvalue,AW_ROOT_DEFAULT);
            }
            aw_root->awar(newawar)
                ->set_minmax(AWinfo->gmenuitem->arg[i].min,AWinfo->gmenuitem->arg[i].max);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            GDE_create_infieldwithpm(aws,newawar,SLIDERWIDTH);
            // maybe bound checking //
            delete newawar;
        }

        else if(itemarg.type==CHOOSER)
        {
            char *defopt=itemarg.choice[0].method;
            char *newawar=GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            if (	(! GBS_string_cmp(itemarg.choice[0].label,"no",1)) ||
                    (! GBS_string_cmp(itemarg.choice[0].label,"yes",1))){
                aws->create_toggle_field(newawar,1);
            }else{
                aws->create_toggle_field(newawar);
            }
            for(long j=0;j<itemarg.numchoices;j++)
            {
                if (!j) {
                    aws->insert_default_toggle(itemarg.choice[j].label,"1",
                                               itemarg.choice[j].method);
                }else{
                    aws->insert_toggle(itemarg.choice[j].label,"1",
                                       itemarg.choice[j].method);
                }
            }
            aws->update_toggle_field();
            delete newawar;
        }
        else if(itemarg.type==CHOICE_MENU)
        {
            char *defopt=itemarg.choice[itemarg.ivalue].method;
            char *newawar=GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            aws->create_option_menu(newawar,NULL,"");

            for(long j=0;j<itemarg.numchoices;j++)
            {
                aws->insert_option(itemarg.choice[j].label,"1",
                                   itemarg.choice[j].method);
            }
            aws->update_option_menu();
            delete newawar;
        }else if(itemarg.type==TEXTFIELD){
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);			
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            aws->create_input_field(newawar,TEXTFIELDWIDTH);
            delete newawar;
        }else if(itemarg.type==CHOICE_TREE){
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);			
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);		    
            awt_create_selection_list_on_trees(gb_main,aws,newawar);
            delete newawar;
        }else if(itemarg.type==CHOICE_SAI){
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);			
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);		    
            awt_create_selection_list_on_extendeds(gb_main,aws,newawar);
            delete newawar;
        }else if(itemarg.type==CHOICE_WEIGHTS){
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);			
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);		    
            void *id = awt_create_selection_list_on_extendeds(gb_main,aws,newawar,gde_filter_weights);
            delete newawar;
            aw_root->awar("tmp/gde/alignment")
                ->add_callback((AW_RCB1)awt_create_selection_list_on_extendeds_update,(AW_CL)id);
        }
        if((AWinfo->gmenuitem->numargs<8) || GDE_odd(i)) aws->at_newline();
        else aws->at_shift( 50,0 );
    }
    aws->at_newline();

    aws->window_fit();	
    return (AW_window *)aws;
#undef BUFSIZE    
}


void GDE_load_menu(AW_window *awm,const char *menulabel,const char *menuitemlabel)
{
//	AW_root *aw_root=awm->get_root();
	char	buffer[1024];
	char	*help;
	long nitem,num_items;
	GmenuItem *menuitem;
	for(long nmenu=0;nmenu<num_menus;nmenu++)
	{
		char *manuname=GBS_string_2_key(menu[nmenu].label);
		if (menulabel){
			if (strcmp(menulabel,manuname)) {
				delete manuname;
				continue;
			}
		}else{
			awm->insert_sub_menu(0,manuname,META);
		}
		delete manuname;
		num_items=menu[nmenu].numitems;
		for(nitem=0;nitem<num_items;nitem++)
		{
			menuitem=&menu[nmenu].item[nitem];
			if (!menuitemlabel || !GBS_string_cmp(menuitem->label,menuitemlabel,0)){
				AWwindowinfo *AWinfo=new AWwindowinfo;
				AWinfo->gmenu=&menu[nmenu];
				AWinfo->gmenuitem=menuitem;
				if (menuitem->help) {
					sprintf(buffer,"GDEHELP/%s",menuitem->help);
					help = strdup(buffer);
				}else{
					help = 0;
				}
				awm->insert_menu_topic(0,menuitem->label,&(menuitem->meta),
					help,AWM_ALL,
					AW_POPUP, (AW_CL)GDE_menuitem_cb, (AW_CL)AWinfo );
			}
		}
	//	if(!menulabel && nmenu==0)
	//		awm->insert_menu_topic(0,"Close","C",0,(AW_active) -1, (AW_CB)AW_POPDOWN, 0, 0);
		if (!menulabel){
			awm->close_sub_menu();
		}
	}
}

struct choose_get_sequence_struct gde_cgss = { 0,CGSS_WT_DEFAULT};

void create_gde_var(AW_root  *aw_root, AW_default aw_def,
		    char *(*get_sequences)(void *THIS, GBDATA **&the_species, 
					   uchar **&the_names,
					   uchar **&the_sequences,
					   long &numberspecies,long &maxalignlen),
		    gde_cgss_window_type wt,
		    void *THIS)
{
    gde_cgss.get_sequences= get_sequences;
    gde_cgss.wt = wt;
    gde_cgss.THIS= THIS;

    aw_root->awar_string("tmp/gde/helptext","help",aw_def);
    aw_root->awar_string( "tmp/gde/alignment","" ,aw_def );	
    
    switch (gde_cgss.wt)
    {
	case CGSS_WT_EDIT4:
	    aw_root->awar_int("gde/top_area_kons",1,aw_def);
	    aw_root->awar_int("gde/top_area_remark",1,aw_def);
	    aw_root->awar_int("gde/middle_area_kons",1,aw_def);
	    aw_root->awar_int("gde/middle_area_remark",1,aw_def);
	case CGSS_WT_EDIT:
	    aw_root->awar_int("gde/top_area",1,aw_def);
	    aw_root->awar_int("gde/top_area_sai",1,aw_def);
	    aw_root->awar_int("gde/top_area_helix",1,aw_def);
	    aw_root->awar_int("gde/middle_area",1,aw_def);
	    aw_root->awar_int("gde/middle_area_sai",1,aw_def);
	    aw_root->awar_int("gde/middle_area_helix",1,aw_def);
	    aw_root->awar_int("gde/bottom_area",1,aw_def);
	    aw_root->awar_int("gde/bottom_area_sai",1,aw_def);
	    aw_root->awar_int("gde/bottom_area_helix",1,aw_def);
	default:
	    break;
    }
    
    aw_root->awar_string( "presets/use","" ,gb_main );	
    aw_root->awar_string( "gde/filter/name","",aw_def);
    aw_root->awar_string( "gde/filter/filter","",aw_def);
    aw_root->awar_int( "gde/species",1,aw_def);
    aw_root->awar_int( "gde/compression",1,aw_def);
    aw_root->awar_string( "gde/filter/alignment","",aw_def);
    aw_root->awar("tmp/gde/alignment")->map("presets/use");
    aw_root->awar("gde/filter/alignment")->map("presets/use");

    DataSet = (NA_Alignment *) Calloc(1,sizeof(NA_Alignment));
    DataSet->rel_offset = 0;
    ParseMenu();	
}

AW_window *AP_open_gde_window(AW_root *aw_root)
{
	AW_window_menu_modes *awm = new AW_window_menu_modes;
		awm->init(aw_root,"GDE","GDE",700,30,0,0);
	awm->at(10,100);awm->callback((AW_CB0)AW_POPDOWN);
	awm->create_button("CLOSE", "CLOSE","C");

	GDE_load_menu(awm);

	return (AW_window *)awm;
}