#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <GEN.hxx>
#include "awtc_import.hxx"
#include "awtc_rename.hxx"
#include "awtc_imp_local.hxx"

struct awtcig_struct awtcig;
#define MAX_COMMENT_LINES 2000

AW_BOOL  awtc_read_string_pair(FILE *in, char *&s1,char *&s2)
{
	const int BUFSIZE = 8000;
	char buffer[BUFSIZE];
	char *res = fgets(&buffer[0], BUFSIZE-1, in);
	delete s1;
	delete s2;
	s1 = 0;
	s2 = 0;
	if (!res) return AW_TRUE;

	char *p = buffer;
	while (*p == ' ' || *p == '\t') p++;
	if (*p == '#') return AW_FALSE;

	int len = strlen(p)-1;
	while  (len >= 0 && (
                         p[len] == '\n' || p[len] == ' ' || p[len] == '\t' || p[len] == 13) ) p[len--] = 0;

	if (!*p) return AW_FALSE;
	char *e = 0;
	e = strpbrk(p," \t");

	if (e) {
		*(e++) = 0;
		s1 = strdup(p);
		while (*e == ' ' || *e == '\t') e++;
		if (*e == '"') {
			char *k = strrchr(e,'"');
			if (k!=e) {
				e++;
				*k=0;
			}
		}
		s2 = strdup(e);
	}else{
		s1 = strdup(p);
	}

	return AW_FALSE;
}


GB_ERROR awtc_read_import_format(char *file)
{
	char *fullfile = AWT_unfold_path(file,"ARBHOME");

	FILE *in = fopen(fullfile,"r");
	delete fullfile;
	if (!in) return "Form not readable (select a form or check permissions)";
	struct input_format_struct *ifo;
	struct input_format_per_line *pl = 0;
	ifo = awtcig.ifo = new input_format_struct;
	char *s1=0,*s2=0;

	while (!awtc_read_string_pair(in,s1,s2)){
		if (!s1) {
			continue;
		}else if (!strcmp(s1,"AUTODETECT")) {
			ifo->autodetect = s2; s2 = 0;
		}else if (!strcmp(s1,"SYSTEM")) {
			ifo->system = s2; s2 = 0;
		}else if (!strcmp(s1,"NEW_FORMAT")) {
			ifo->new_format = s2; s2 = 0;
		}else if (!strcmp(s1,"KEYWIDTH")) {
			ifo->tab = atoi(s2);
		}else if (!strcmp(s1,"BEGIN")) {
			ifo->begin = s2; s2 = 0;
		}else if (!strcmp(s1,"SEQUENCESTART")) {
			ifo->sequencestart = s2; s2 = 0;
			ifo->read_this_sequence_line_too = 1;
		}else if (!strcmp(s1,"SEQUENCEAFTER")) {
			ifo->sequencestart = s2; s2 = 0;
			ifo->read_this_sequence_line_too = 0;
		}else if (!strcmp(s1,"FILETAG")) {
			ifo->filetag = s2; s2 = 0;
		}else if (!strcmp(s1,"SEQUENCESRT")) {
			ifo->sequencesrt = s2; s2 = 0;
		}else if (!strcmp(s1,"SEQUENCEACI")) {
			ifo->sequenceaci = s2; s2 = 0;
		}else if (!strcmp(s1,"SEQUENCECOLUMN")) {
			ifo->sequencecolumn = atoi(s2);
		}else if (!strcmp(s1,"SEQUENCEEND")) {
			ifo->sequenceend = s2; s2 = 0;
		}else if (!strcmp(s1,"END")) {
			ifo->end = s2; s2 = 0;
		}else if (!strcmp(s1,"CREATE_ACC_FROM_SEQUENCE")) {
			ifo->autocreateacc = 1;
		}else if (!strcmp(s1,"DONT_GEN_NAMES")) {
			ifo->noautonames = 1;
		}else if (!strcmp(s1,"MATCH")) {

			pl = (struct input_format_per_line *)GB_calloc(1,
                                                           sizeof(struct input_format_per_line));
			pl->next = ifo->pl;
			ifo->pl = pl;
			pl->match = GBS_remove_escape(s2); free(s2); s2 = 0;
		}else if (pl && !strcmp(s1,"SRT")) {
			pl->srt = s2;s2= 0;
		}else if (pl && !strcmp(s1,"ACI")) {
			pl->aci = s2;s2= 0;
		}else if (pl && !strcmp(s1,"WRITE")) {
			pl->write = s2;s2= 0;
		}else if (pl && !strcmp(s1,"APPEND")) {
			pl->append = s2;s2= 0;
		}else if (pl && !strcmp(s1,"TAG")) {
			pl->tag = GBS_string_2_key(s2);
			if (!strlen(pl->tag)){
			    delete pl->tag;
			    pl->tag = 0;
			}
		}else{
			aw_message(GBS_global_string("Unknown command in import format file '%s'  command '%s'\n",fullfile,s1));
		}
	}

	fclose(in);
	return 0;
}

input_format_struct::input_format_struct(void){
	memset((char *)this,0,sizeof(input_format_struct));
}

input_format_struct::~input_format_struct(void)
{
	struct input_format_struct *ifo= this;
	struct input_format_per_line *pl1 = 0;
	struct input_format_per_line *pl2 = 0;

	for (pl1 = ifo->pl; pl1; pl1=pl2){
		pl2 = pl1->next;
		delete pl1->match;
		delete pl1->srt;
		delete pl1->aci;
		delete pl1->tag;
		delete pl1->append;
		delete pl1->write;
		delete pl1;
	}

	delete ifo->autodetect;
	delete ifo->system;
	delete ifo->new_format;
	delete ifo->begin;
	delete ifo->sequencestart;
	delete ifo->sequenceend;
	delete ifo->sequencesrt;
	delete ifo->sequenceaci;
	delete ifo->end;
	delete ifo->b1;
	delete ifo->b2;
}

void awtc_check_input_format(AW_window *aww)
{
	AW_root *root = aww->get_root();
	char **files = GBS_read_dir(0,"import/*.ift");
	char **file;
	char	buffer[AWTC_IMPORT_CHECK_BUFFER_SIZE+10];
	for (file = files; *file; file++) {
		char *fname = 	AWT_fold_path(*file,"ARBHOME");
		root->awar(AWAR_FORM"/file_name")->write_string(fname);

		GB_ERROR error = awtc_read_import_format(fname);
		if (error) {
			aw_message(error);
		}
		if (!awtcig.ifo->autodetect) continue;		// not detectable
		char *autodetect = GBS_remove_escape(awtcig.ifo->autodetect);
		delete awtcig.ifo; awtcig.ifo = 0;

		FILE *in;
		{
			char com[1024];
			char *f = root->awar(AWAR_FILE)->read_string();
			sprintf(com,"cat %s 2>/dev/null",f);
			in = popen(com,"r");
			if (!in) aw_message( "Cannot open any input file");
		}
		if (in) {
			int size = fread(buffer,1,AWTC_IMPORT_CHECK_BUFFER_SIZE,in);
			pclose(in);
			if (size>=0){
				buffer[size] = 0;
				if (!GBS_string_cmp(buffer,autodetect,0)){	// format found
					delete autodetect;
					break;
				}
			}
		}
	}
	if (!*file) {		// nothing found
		root->awar(AWAR_FORM"/file_name")->write_string("unknown.ift");
	}

	GBT_free_names(files);
}

static const char *awtc_next_file(void){
	if (awtcig.in) fclose(awtcig.in);
	if (!awtcig.current_file) awtcig.current_file = awtcig.filenames;
	while (*awtcig.current_file) {
		char *origin_file_name;
		origin_file_name = *(awtcig.current_file++);
		char *sorigin= strrchr(origin_file_name,'/');
        if (!sorigin) sorigin= origin_file_name;
        else sorigin++;
		char mid_file_name[1024];
		char dest_file_name[1024];
		char srt[1024];

		if (awtcig.ifo2 && awtcig.ifo2->system){
			sprintf(mid_file_name,"/tmp/%s_%i",sorigin, getpid());
			sprintf(srt,"$<=%s:$>=%s",origin_file_name, mid_file_name);
			char *sys = GBS_string_eval(awtcig.ifo2->system,srt,0);
			sprintf(AW_ERROR_BUFFER,"exec '%s'",awtcig.ifo2->system);
			aw_status(AW_ERROR_BUFFER);
			if (system(sys)) {
				sprintf(AW_ERROR_BUFFER,"Error in '%s'",sys);
				aw_message();
				delete sys; continue;
			}
			delete sys;
			origin_file_name = mid_file_name;
		}else{
			mid_file_name[0] = 0;
		}

		if (awtcig.ifo->system){
			sprintf(dest_file_name,"/tmp/%s_2_%i",sorigin, getpid());
			sprintf(srt,"$<=%s:$>=%s",origin_file_name, dest_file_name);
			char *sys = GBS_string_eval(awtcig.ifo->system,srt,0);
			sprintf(AW_ERROR_BUFFER,"Converting File %s",awtcig.ifo->system);
			aw_status(AW_ERROR_BUFFER);
			if (system(sys)) {
				sprintf(AW_ERROR_BUFFER,"Error in '%s'",sys);
				aw_message();
				delete sys; continue;
			}
			delete sys;
			origin_file_name = dest_file_name;
		}else{
			dest_file_name[0] = 0;
		}

		awtcig.in = fopen(origin_file_name,"r");
		if (strlen(mid_file_name)) unlink(mid_file_name);
		if (strlen(dest_file_name)) unlink(dest_file_name);

		if (awtcig.in) return 0;
		sprintf(AW_ERROR_BUFFER,"Error: Cannot open file %s\n",awtcig.current_file[-1]);
		aw_message();
	}
	return "last file reached";
}

char *awtc_read_line(int tab,char *sequencestart, char *sequenceend){
	/* two modes:	tab == 0 -> read single lines,
       different files are seperated by sequenceend,
       tab != 0	join lines that start after position tab,
       joined lines are seperated by '|'
       except lines that match sequencestart
       (they may be part of sequence if read_this_sequence_line_too = 1 */

    static char *in_queue = 0;		// read data
    static int b2offset = 0;
    const int BUFSIZE = 8000;
    const char *SEPERATOR = "|";	// line seperator
    struct input_format_struct *ifo;
    ifo = awtcig.ifo;
    char *p;

    if (!ifo->b1) ifo->b1 = (char*)calloc(BUFSIZE,1);
    if (!ifo->b2) ifo->b2 = (char*)calloc(BUFSIZE,1);
    if (!awtcig.in){
        if (awtc_next_file()) {
            if (in_queue) {
                char *s = in_queue;
                in_queue = 0;
                return s;
            }
            return 0;
        }
    }


	if (!tab) {
		if (in_queue) {
			char *s = in_queue;
			in_queue = 0;
			return s;
		}
		p = fgets(ifo->b1, BUFSIZE-3, awtcig.in);
		if (!p){
			sprintf(ifo->b1,"%s",sequenceend);
			if (awtcig.in) fclose(awtcig.in);awtcig.in= 0;
			p = ifo->b1;
		}
		int len = strlen(p)-1;
		while (len>=0){
		    if (p[len] =='\n' || p[len] == 13) p[len--] = 0;
		    else break;
		}
		return ifo->b1;
	}

	b2offset = 0;
	ifo->b2[0] = 0;

	if (in_queue) {
		b2offset = 0;
		strncpy(ifo->b2+b2offset, in_queue, BUFSIZE - 4- b2offset);
		b2offset += strlen(ifo->b2+b2offset);
		in_queue = 0;
		if (!GBS_string_cmp(ifo->b2,sequencestart,0)) return ifo->b2;
	}
	while (1) {
		p = fgets(ifo->b1, BUFSIZE-3, awtcig.in);
		if (!p){
			if (awtcig.in) fclose(awtcig.in);awtcig.in= 0;
			break;
		}
		int len = strlen(p)-1;
		while (len>=0){
		    if (p[len] =='\n' || p[len] == 13) p[len--] = 0;
		    else break;
		}


		if (!GBS_string_cmp(ifo->b1,sequencestart,0)){
			in_queue = ifo->b1;
			return ifo->b2;
		}

		int i;
		for (i=0;i<=tab;i++) if (ifo->b1[i] != ' ') break;

		if (i < tab) {
			in_queue = ifo->b1;
			return ifo->b2;
		}
		strncpy(ifo->b2+b2offset, SEPERATOR, BUFSIZE - 4- b2offset);
		b2offset += strlen(ifo->b2+b2offset);

		p = ifo->b1;
		if (b2offset>0) while (*p==' ') p++;	// delete spaces in second line

		strncpy(ifo->b2+b2offset, p, BUFSIZE - 4- b2offset);
		b2offset += strlen(ifo->b2+b2offset);
	}
	in_queue = 0;
	return ifo->b2;
}

static void awtc_write_entry(GBDATA *gbd,const char *key,char *str,const char *tag,int append)
{
	GBDATA *gbk;
	int len;
	char *strin,*buf;
	if (!gbd) return;
	while (str[0] == ' '|| str[0] == '\t'|| str[0] == '|') str++;
	len = strlen(str) -1;
	while (len >=0 && (
                       str[len] == ' '|| str[len] == '\t'|| str[len] == '|' || str[len] == 13)) len--;
	str[len+1] = 0;
	if (!str[0]) return;

	gbk = GB_find(gbd,key,NULL,down_level);
	if (!gbk || !append){
		if (!gbk) gbk=GB_create(gbd,key,GB_STRING);

		if (tag){
		    void *s = GBS_stropen(10000);
		    GBS_chrcat(s,'[');
		    GBS_strcat(s,tag);
		    GBS_strcat(s,"] ");
		    GBS_strcat(s,str);
		    char *val = GBS_strclose(s,0);
		    GB_write_string(gbk,val);
		    free(val);
		}else{
		    if (!strcmp(key,"name")){
                char *nstr = GBT_create_unique_species_name(awtcig.gb_main,str);
                GB_write_string(gbk,nstr);
                delete nstr;
		    }else{
                GB_write_string(gbk,str);
		    }
		}
		return;
	}

	strin = GB_read_char_pntr(gbk);
	len = strlen(str) + strlen(strin);
	buf = (char *)GB_calloc(sizeof(char),len+2);
	sprintf(buf,"%s %s",strin,str);
	GB_write_string(gbk,buf);
	free(buf);
	return;
}

GB_ERROR awtc_read_data(char *ali_name)
{
    char num[6];
    char text[100];
    static int	counter = 0;
    GBDATA *gb_species_data = GB_search(GB_MAIN,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species;
    char *p;
    struct input_format_struct *ifo;
    struct input_format_per_line *pl = 0;
    ifo = awtcig.ifo;

    while (1){				// go to the start
        p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
        if (!p || !ifo->begin || !GBS_string_cmp(p,ifo->begin,0)) break;
    }
    if (!p) return "Cannot find start of file: Wrong format or empty file";

    // lets start !!!!!
    while(p){
        counter++;
        sprintf(text,"Reading species %i",counter);
        if (counter %10 == 0){
            if (aw_status(text)) break;
        }

        gb_species = GB_create_container(gb_species_data,"species");
        sprintf(text,"spec%i",counter);
        free(GBT_read_string2(gb_species,"name",text));
        if ( awtcig.filenames[1] ) {	// multiple files !!!
            char *f= strrchr(awtcig.current_file[-1],'/');
            if (!f) f= awtcig.current_file[-1];
            else f++;
            awtc_write_entry(gb_species,"file",f,ifo->filetag,0);
        }
        int line;
        int max_line = MAX_COMMENT_LINES;
        for(line=0;line<=max_line;line++){
            sprintf(num,"%i  ",line);
            if (line == max_line){
                char *file = 0;
                if (awtcig.current_file[0]) file = awtcig.current_file[0];
                GB_ERROR msg = GB_export_error("A database entry in file '%s' is longer than %i lines.\n"
                                               "	This might be the result of a wrong input format\n"
                                               "	or a long comment in a sequence\n",file,line);
                if (!aw_message(msg,"Continue Reading,Abort")) max_line *= 2;
            }
            if (strlen(p) > ifo->tab){
                for (pl = ifo->pl; pl; pl=pl->next) {
                    if (!GBS_string_cmp(p,pl->match,0)){
                        char *dup = p+ifo->tab;
                        while (*dup == ' ' || *dup == '\t') dup++;

                        char *s = 0;
                        char *dele = 0;
                        if (pl->srt){
                            dele = s =
                                GBS_string_eval(dup,pl->srt,gb_species);
                            if (!s) return GB_get_error();
                        }else{
                            s = dup;
                        }

                        if (pl->aci){
                            dup = dele;
                            dele = s = GB_command_interpreter(GB_MAIN,
                                                              s,pl->aci,gb_species);
                            delete dup;
                            if (!s) return GB_get_error();
                        }

                        if (pl->append) {
                            awtc_write_entry(gb_species,pl->append,s,pl->tag,1);
                        }else if (pl->write) {
                            awtc_write_entry(gb_species,pl->write,s,pl->tag,0);
                        }
                        delete dele;
                    }
                }
            }

            if (!GBS_string_cmp(p,ifo->sequencestart,0)) goto read_sequence;

            p = awtc_read_line(ifo->tab,ifo->sequencestart,ifo->sequenceend);
            if (!p) break;
        }
        return GB_export_error("No Start of Sequence found (%i lines read)",
                               MAX_COMMENT_LINES);

    read_sequence:
        {
            char *sequence;
            void *strstruct = GBS_stropen(5000);
            int linecnt;
            for(linecnt = 0;;linecnt++) {
                if (linecnt || !ifo->read_this_sequence_line_too){
                    p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
                }
                if (!p) break;
                if (ifo->sequenceend && !GBS_string_cmp(p,ifo->sequenceend,0)) break;
                if (strlen(p) <= ifo->sequencecolumn) continue;
                GBS_strcat(strstruct,p+ifo->sequencecolumn);
            }
            sequence = GBS_strclose(strstruct,0);

            GBDATA *gb_data = GBT_add_data(gb_species,ali_name,"data", GB_STRING);
            if (ifo->sequencesrt) {
                char *h = GBS_string_eval(sequence,ifo->sequencesrt,gb_species);
                if (!h) return GB_get_error();
                delete sequence;
                sequence = h;
            }

            if (ifo->sequenceaci) {
                char *h = GB_command_interpreter(GB_MAIN,
                                                 sequence,ifo->sequenceaci,gb_species);
                delete sequence;
                if (!h) return GB_get_error();
                sequence = h;
            }

            GB_write_string(gb_data,sequence);
            GB_write_security_write(gb_data,4);

            GBDATA *gb_acc = GB_search(gb_species,"acc",GB_FIND);
            if (!gb_acc && ifo->autocreateacc) {
                char buf[100];
                long id = GBS_checksum(sequence,1,".-");
                sprintf(buf,"ARB_%lX",id);
                gb_acc = GB_search(gb_species,"acc",GB_STRING);
                GB_write_string(gb_acc,buf);
            }
            delete sequence;
        }
        while (1){				// go to the start of an species
            if (!p || !ifo->begin || !GBS_string_cmp(p,ifo->begin,0)) break;
            p = awtc_read_line(0,ifo->sequencestart,ifo->sequenceend);
        }
    }
    return 0;
}

void AWTC_import_go_cb(AW_window *aww)
{
    char buffer[1024];
    AW_root *awr = aww->get_root();
    bool is_genom_db;
    {
        bool read_genom_db = awr->awar(AWAR_READ_GENOM_DB)->read_int();
        is_genom_db = awr->awar_int(AWAR_GENOM_DB, read_genom_db)->read_int();

        if (read_genom_db!=is_genom_db) {
            if (is_genom_db) {
                aw_message("You can only import whole genom sequences (genbank-format) into a genom database.");
            }
            else {
                aw_message("You can't import whole genom sequences (genbank-format) into a non-genom ARB database.");
            }
            return;
        }
    }

    GB_ERROR error = 0;
    char *file = 0;
    if (!is_genom_db) {
        file = awr->awar(AWAR_FORM"/file_name")->read_string();
        if (!strlen(file)) {
            delete file;
            aw_message("Please select a form");
            return;
        }

        error = awtc_read_import_format(file);
        if (error) {
            delete awtcig.ifo; awtcig.ifo = 0;
            aw_message(error);
            return;
        }

        if (awtcig.ifo->new_format) {
            awtcig.ifo2 = awtcig.ifo;
            awtcig.ifo = 0;
            error = awtc_read_import_format(awtcig.ifo2->new_format);
            if (error) {
                delete awtcig.ifo; awtcig.ifo = 0;
                delete awtcig.ifo2; awtcig.ifo2 = 0;
                aw_message(error);
                return;
            }
        }
    }

    GB_change_my_security(GB_MAIN,6,"");
    GB_begin_transaction(GB_MAIN);
    char *ali_name;
    {
        char *ali = awr->awar(AWAR_ALI)->read_string();
        ali_name = GBS_string_eval(ali,"*=ali_*1:ali_ali_=ali_",0);
        delete ali;
    }

    error = GBT_check_alignment_name(ali_name);

    if (!error) {
        char *ali_type;
        ali_type = awr->awar(AWAR_ALI_TYPE)->read_string();

        if (is_genom_db && strcmp(ali_type, "dna")!=0) {
            error = "You must set the alignment type to dna when importing genom sequences.";
        }
        else {
            GBT_create_alignment(GB_MAIN,ali_name,2000,0,4,ali_type);
        }
        delete ali_type;
    }

    bool noautonames = false; // strange name!
    if (!error) {
        if (is_genom_db) {
            char *fname = awr->awar(AWAR_FILE)->read_string();
            error = GEN_read(GB_MAIN, fname, ali_name);
            delete fname;
        }
        else {
            char *f = awr->awar(AWAR_FILE)->read_string();
            awtcig.filenames = GBS_read_dir(f,0);
            delete f;

            if (awtcig.filenames[0] == 0){
                error = GB_export_error("Cannot find selected file");
            }

            aw_openstatus("Reading input files");
            if (!error) {
                error = awtc_read_data(ali_name);
                if (error) {
                    sprintf(buffer,"Error: %s reading file %s",error,awtcig.current_file[-1]);
                    error = buffer;
                }
            }

            noautonames = awtcig.ifo->noautonames;
            if (awtcig.ifo2) noautonames += awtcig.ifo2->noautonames;

            delete awtcig.ifo; awtcig.ifo = 0;
            delete awtcig.ifo2; awtcig.ifo2 = 0;

            if (awtcig.in)	fclose(awtcig.in);
            awtcig.in = 0;
            GBT_free_names(awtcig.filenames);
            awtcig.filenames = 0;
            awtcig.current_file = 0;

            aw_closestatus();
        }
    }
    delete ali_name;

    if (error) {
        aw_message(error);
        GB_abort_transaction(GB_MAIN);
        return;
    }

    aww->hide();

    aw_openstatus("Checking and Scanning database");
    aw_status("First Pass: Check entries");
    awt_selection_list_rescan(GB_MAIN,AWT_NDS_FILTER);
    GBT_mark_all(GB_MAIN,1);
    sleep(1);
    aw_status("Second Pass: Check sequence lengths");
    GBT_check_data(GB_MAIN,0);
    sleep(1);

    GB_commit_transaction(GB_MAIN);

    if (!noautonames) {
        if (aw_message("You may generate short names using the full_name and accession entry of the species",
                       "Generate new short names,use old names")==0)
        {
            aw_status("Third Pass: Generate unique names");
            error = AWTC_pars_names(GB_MAIN,1);
        }
    }

    aw_closestatus();
    if (error) aw_message(error);

    GB_begin_transaction(GB_MAIN);
    GB_change_my_security(GB_MAIN,0,"");
    GB_commit_transaction(GB_MAIN);

    if (!is_genom_db) awtcig.func(awr, awtcig.cd1,awtcig.cd2);
}

//  ------------------------------------------------------
//      static void genom_flag_changed(AW_root *root)
//  ------------------------------------------------------
static void genom_flag_changed(AW_root *awr) {
    if (awr->awar(AWAR_READ_GENOM_DB)->read_int()) {
        awr->awar(AWAR_ALI)->write_string("genom");
        awr->awar(AWAR_ALI_TYPE)->write_string("dna");
        awr->awar_string(AWAR_FORM"/filter",".fit");        // dummy to hide normal import filters
    }
    else {
        awr->awar_string(AWAR_FORM"/filter",".ift");
    }
}

GBDATA *open_AWTC_import_window(AW_root *awr,const char *defname, int do_exit,AWTC_RCB(func), AW_CL cd1, AW_CL cd2)
{
    static AW_window_simple *aws = 0;

    awtcig.gb_main = GB_open("noname.arb","wc");
    awtcig.func = func;
    awtcig.cd1 = cd1;
    awtcig.cd2 = cd2;

    awr->awar_string(AWAR_FILE,defname);
    awr->awar_string(AWAR_FORM"/directory","lib/import");
    awr->awar_string(AWAR_FORM"/filter",".ift");
    awr->awar_string(AWAR_FORM"/file_name","");
    awr->awar_string(AWAR_ALI,"16s");
    awr->awar_string(AWAR_ALI_TYPE,"rna");
    awr->awar_int(AWAR_READ_GENOM_DB, 0)->add_callback(genom_flag_changed);

    if (aws){
        aws->show();
        return GB_MAIN;
    }

    aws = new AW_window_simple;

    aws->init( awr, "ARB_IMPORT","ARB IMPORT", 400, 100 );
    aws->load_xfig("awt/import_db.fig");

    aws->at("close");
    if (do_exit){
        aws->callback((AW_CB0)exit);
    }else{
        aws->callback(AW_POPDOWN);
    }
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"arb_import.hlp");
    aws->create_button("HELP", "HELP","H");

    aws->at("pattern");
    aws->create_input_field(AWAR_FILE,4);

    awt_create_selection_box(aws,AWAR_FORM,"","ARBHOME", AW_FALSE );

    aws->at("auto");
    aws->callback(awtc_check_input_format);
    aws->create_button("AUTO_DETECT", "AUTO DETECT","A");


    aws->at("ali");
    aws->create_input_field(AWAR_ALI,4);

    aws->at("type");
    aws->create_option_menu(AWAR_ALI_TYPE);
    aws->insert_option("dna","d","dna");
    aws->insert_option("rna","r","rna");
    aws->insert_option("protein","p","ami");
    aws->update_option_menu();

    aws->at("genom");
    aws->create_toggle(AWAR_READ_GENOM_DB);

    aws->at("go");
    aws->callback(AWTC_import_go_cb);
    aws->highlight();
    aws->create_button("GO", "GO","G");

    aws->show();
    return GB_MAIN;
}
