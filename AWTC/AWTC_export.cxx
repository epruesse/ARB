#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include "awtc_export.hxx"
#include "awtc_exp_local.hxx"

export_format_struct::export_format_struct(void){
	memset((char *)this,0,sizeof(export_format_struct));
}

export_format_struct::~export_format_struct(void)
	{
	struct export_format_struct *efo= this;
	delete efo->system;
	delete efo->new_format;
	delete efo->suffix;
}


char *awtc_read_export_format(export_format_struct * efo,char *file){
	char *fullfile = AWT_unfold_path(file,"ARBHOME");

	FILE *in = fopen(fullfile,"r");
	delete fullfile;
	sprintf(AW_ERROR_BUFFER,"Form %s not readable (select a form or check permissions)",
			file);
	if (!in) return AW_ERROR_BUFFER;
	char *s1=0,*s2=0;

	while (!awtc_read_string_pair(in,s1,s2)){
		if (!s1) {
			continue;
		}else if (!strcmp(s1,"SYSTEM")) {
			efo->system = s2; s2 = 0;
		}else if (!strcmp(s1,"PRE_FORMAT")) {
			efo->new_format = s2; s2 = 0;
		}else if (!strcmp(s1,"SUFFIX")) {
			efo->suffix = s2; s2 = 0;
		}else if (!strcmp(s1,"BEGIN")) {
			break;
		}else{
			fprintf(stderr,"Unknown command in import format file: %s\n",s1);
		}
	}

	fclose(in);
	return 0;
}

GB_ERROR AWTC_export_format(GBDATA *gb_main, char *formname, char *outname, int	multiple, 
		int openstatus )
	{
	char *fullformname = AWT_unfold_path(formname,"ARBHOME");
	FILE	*out = 0;
	GB_ERROR error = 0;
	int	count = 0;

	char *form = GB_read_file(fullformname);
	if (!form) {
		error = "Form not found";
	}else{
		char *form2 = GBS_string_eval(form,"*\nBEGIN*\n*=*3:\\==\\\\\\=:*=\\*\\=*1:\\:=\\\\\\:",0);
		if (!form2) error = (char *)GB_get_error();
		delete form;
		form = form2;
	}

	if (openstatus) aw_openstatus("Exporting Data");

	export_format_struct *efo = new export_format_struct;
	error = awtc_read_export_format(efo,formname);

	if (!error) {
		if (efo->system && !efo->new_format) {
			error = "Format File Error: You can only use the command SYSTEM "
				"when you use the command PRE_FORMAT";
		}else if (efo->new_format) {
			if (efo->system) {
				char intermediate[1024];
				char srt[1024];
				sprintf(intermediate,"/tmp/%s_%i",outname,getpid());
				error =
 				 AWTC_export_format(gb_main,efo->new_format, intermediate,0,0);
				if (!error) {
					sprintf(srt,"$<=%s:$>=%s",intermediate, outname);
					char *sys = GBS_string_eval(efo->system,srt,0);
					sprintf(AW_ERROR_BUFFER,"exec '%s'",efo->system);
					aw_status(AW_ERROR_BUFFER);
					if (system(sys)) {
						sprintf(AW_ERROR_BUFFER,"Error in '%s'",sys);
						delete sys; error = AW_ERROR_BUFFER;
					}
					delete sys;
				}
			}else{
				error =
				 AWTC_export_format(gb_main,efo->new_format, outname,multiple,0);
			}
			goto end_of_AWTC_export_format;
		}
	}


	GBDATA *gb_species;

	count = 0;
	char	buffer[1024];

	if (!error && !multiple) {
		out = fopen(outname,"w");
		if (!out) {
			sprintf(AW_ERROR_BUFFER,"Error: I Cannot write to file %s",outname);
			error = AW_ERROR_BUFFER;
		}
	}


	for (gb_species = GBT_first_marked_species(gb_main);
		!error && gb_species;
		gb_species = GBT_next_marked_species(gb_species)){
		if (multiple) {
			char buf[1024];
			char *name = GBT_read_name(gb_species);
			sprintf(buf,"%s_%s",outname,name);
			delete name;
			out = fopen(buf,"w");
			if (!out){
				error = "Cannot open Outfile";
				break;
			}
		}
		if (count % 10 == 0) {
			char *name = GBT_read_name(gb_species);
			sprintf(buffer,"%s: %i",name, count);
			if (aw_status(buffer)) break;
			delete name;
		}
		char *pars = GBS_string_eval(" ",form,gb_species);
		if (!pars) {
			error = GB_get_error();
			break;
		}
		char *p;
		char *o = pars;
		while ( (p = GBS_find_string(o,"$$DELETE_LINE$$",0)) ) {
			char *l,*r;
			for (l = p; l>o;l--) if (*l=='\n') break;
			r = strchr(p,'\n'); if (!r) r = p +strlen(p);
			fwrite(o,1,l-o,out);
			o = r;
		}
		fprintf(out,"%s",o);
		delete pars;

		if (multiple) {
			if (out) fclose(out);
			out = 0;
		}
		count ++;
	}

	if (out) fclose(out);
	end_of_AWTC_export_format:

	if (openstatus) aw_closestatus();

	delete fullformname;
	delete form;
	delete efo;
	return error;
}


void AWTC_export_go_cb(AW_window *aww,GBDATA *gb_main){


	GB_transaction dummy(gb_main);

	AW_root *awr = aww->get_root();
	char	*formname = awr->awar(AWAR_EXPORT_FORM"/file_name")->read_string();
	int	multiple = (int)awr->awar(AWAR_EXPORT_MULTIPLE_FILES)->read_int();
	GB_ERROR error = 0;
	
	char *outname = awr->awar(AWAR_EXPORT_FILE"/file_name")->read_string();
	error = AWTC_export_format(gb_main, formname, outname, multiple,1);
	aww->get_root()->awar(AWAR_EXPORT_FILE"/directory")->touch();
	delete outname;
	delete formname;
	if (error) aw_message(error);
}

AW_window *open_AWTC_export_window(AW_root *awr,GBDATA *gb_main)
	{
	static AW_window_simple *aws = 0;
	if (aws) return (AW_window *) aws;


	awr->awar_string(AWAR_EXPORT_FORM"/directory","lib/export",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_FORM"/filter",".eft",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_FORM"/file_name","",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_FILE"/directory","",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_FILE"/filter","",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_FILE"/file_name","noname",AW_ROOT_DEFAULT);
	awr->awar_string(AWAR_EXPORT_ALI,"16s",AW_ROOT_DEFAULT);
	awr->awar_int(AWAR_EXPORT_MULTIPLE_FILES,0,AW_ROOT_DEFAULT);

	aws = new AW_window_simple;

	aws->init( awr, "ARB_EXPORT", "ARB EXPORT", 400, 100 );
	aws->load_xfig("awt/export_db.fig");

	aws->at("close");
	aws->callback(AW_POPDOWN);
	aws->create_button("CLOSE", "CLOSE","C");

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"arb_export.hlp");
	aws->create_button("HELP", "HELP","H");

	awt_create_selection_box(aws,AWAR_EXPORT_FILE,"f" );

	awt_create_selection_box(aws,AWAR_EXPORT_FORM,"","ARBHOME", AW_FALSE );

	aws->at("multiple");
	aws->create_toggle(AWAR_EXPORT_MULTIPLE_FILES);

	aws->at("go");
	aws->highlight();
	aws->callback((AW_CB1)AWTC_export_go_cb,(AW_CL)gb_main);
	aws->create_button("GO", "GO","G");

	return (AW_window *) aws;
}
