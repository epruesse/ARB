#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <assert.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>

#include "gde.hxx"
#include "GDE_menu.h"
#include "GDE_def.h"
#include "GDE_global.h"
#include "GDE_extglob.h"

/*
ParseMenus(): Read in the menu config file, and generate the internal
menu structures used by the window system.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/



void ParseMenu()
{
	int           j,curmenu  = -1,curitem = 0;
	int           curchoice  = 0 ,curarg = 0,curinput = 0, curoutput = 0;
	char          in_line[GBUFSIZ],temp[GBUFSIZ],head[GBUFSIZ];
	char          tail[GBUFSIZ];
	const char   *home;
	Gmenu        *thismenu   = NULL;
	GmenuItem    *thisitem   = NULL;
	GmenuItemArg *thisarg    = NULL;
	GfileFormat  *thisinput  = NULL;
	GfileFormat  *thisoutput = NULL;
	FILE         *file;
	char         *resize;

    /*
     *	Open the menu configuration file "$ARBHOME/GDEHELP/ARB_GDEmenus"
     *	First search the local directory, then the home directory.
     */
	memset((char*)&menu[0],0,sizeof(Gmenu)*GDEMAXMENU);
	home = GB_getenvARBHOME();

	strcpy(temp,home);
	strcat(temp,"/GDEHELP/ARB_GDEmenus");
	file=fopen(temp,"r");
	if(file == NULL) Error("ARB_GDEmenus file not in the home, local, or $ARBHOME/GDEHELP directory");

    /*
     *	Read the ARB_GDEmenus file, and assemble an internal representation
     *	of the menu/menu-item hierarchy.
     */

	for(;getline(file,in_line) != EOF;)
	{
        /*
         *	menu: chooses menu to use
         */
		if(in_line[0] == '#' || (in_line[0] && in_line[1] == '#')){
			;
		}else if(Find(in_line,"menu:"))
		{
			crop(in_line,head,temp);
			curmenu = -1;
			for(j=0;j<num_menus;j++) {
				if(Find(temp,menu[j].label)) curmenu=j;
            }
            /*
             *	If menu not found, make a new one
             */
			if(curmenu == -1)
			{
				curmenu         = num_menus++;
				thismenu        = &menu[curmenu];
				thismenu->label = (char*)calloc(strlen(temp)+1,sizeof(char));

                if(thismenu->label == NULL) Error("Calloc");
				(void)strcpy(thismenu->label,temp);
                thismenu->numitems = 0;
			}
		}
        /*
         *	item: chooses menu item to use
         */
		else if(Find(in_line,"item:"))
		{
			curarg    = -1;
			curinput  = -1;
			curoutput = -1;
			crop(in_line,head,temp);
			curitem   = thismenu->numitems++;

            /*
             *	Resize the item list for this menu (add one item);
             */
			if(curitem == 0) {
			    resize = (char*)GB_calloc(1,sizeof(GmenuItem)); // @@@ calloc->GB_calloc avoids (rui)
			}
			else {
			    resize = (char *)realloc((char *)thismenu->item, thismenu->numitems*sizeof(GmenuItem));
			}

			if (resize == NULL) Error ("Calloc");
			thismenu->item =(GmenuItem*)resize;

			thisitem             = &(thismenu->item[curitem]);
			thisitem->label      = strdup(temp);
			thisitem->meta       = '\0';
			thisitem->numinputs  = 0;
			thisitem->numoutputs = 0;
			thisitem->numargs    = 0;
			//thisitem->X        = 0;
			thisitem->help       = NULL;
		}

        /*
         *	itemmethod: generic command line generated by this item
         */
		else if(Find(in_line,"itemmethod:"))
		{
			crop(in_line,head,temp);
			thisitem->method =
                (char*)calloc(strlen(temp)+1,sizeof(char));
			if(thisitem->method == NULL) Error("Calloc");

            {
                char *to = thisitem->method;
                char *from = temp;
                char last = 0;
                char c;

                do {
                    c = *from++;
                    if (c!=last || c!='\'') { /* replace '' with ' */
                        *to++ = c;
                        last = c;
                    }
                } while (c!=0);
            }

			//strcpy(thisitem->method,temp);
		}
        /*
         *	Help file
         */
		else if(Find(in_line,"itemhelp:"))
		{
			crop(in_line,head,temp);
			thisitem->help = (char*)calloc(strlen(temp)+1,sizeof(char));
			if(thisitem->method == NULL) Error("Calloc");
			(void)strcpy(thisitem->help,temp);
		}
        /*
         *		Meta key equiv
         */
		else if(Find(in_line,"itemmeta:"))
		{
			crop(in_line,head,temp);
			thisitem->meta = temp[0];
		}
        /*
         *	arg: defines the symbol for a command line arguement.
         *		this is used for substitution into the itemmethod
         *		definition.
         */

		else if(Find(in_line,"arg:"))
		{
			crop(in_line,head,temp);
			curarg=thisitem->numargs++;
			if(curarg == 0) resize = (char*)calloc(1,sizeof(GmenuItemArg));
			else resize = (char *)realloc((char *)thisitem->arg, thisitem->numargs*sizeof(GmenuItemArg) );
            memset((char *)resize + (thisitem->numargs-1)*sizeof(GmenuItemArg),0, sizeof(GmenuItemArg));


			if(resize == NULL) Error("arg: Realloc");

			(thisitem->arg) = (GmenuItemArg*)resize;
			thisarg         = &(thisitem->arg[curarg]);
			thisarg->symbol = (char*)calloc(strlen(temp)+1, sizeof(char));
			if(thisarg->symbol == NULL) Error("Calloc");
			(void)strcpy(thisarg->symbol,temp);

			thisarg->optional   = FALSE;
			thisarg->type       = 0;
			thisarg->min        = 0.0;
			thisarg->max        = 0.0;
			thisarg->numchoices = 0;
			thisarg->choice     = NULL;
			thisarg->textvalue  = NULL;
			thisarg->ivalue     = 0;
			thisarg->fvalue     = 0.0;
			thisarg->label      = 0;
		}
        /*
         *	argtype: Defines the type of argument (menu,chooser, text, slider)
         */
		else if(Find(in_line,"argtype:"))
		{
			crop(in_line,head,temp);
			if(strcmp(temp,"text")==0)
			{
				thisarg->type      = TEXTFIELD;
				thisarg->textvalue =
					(char*)calloc(GBUFSIZ,sizeof(char));
				if(thisarg->textvalue == NULL)
					Error("Calloc");
			}
			else if(strcmp(temp,"choice_list") == 0) thisarg->type=CHOICE_LIST;
			else if(strcmp(temp,"choice_menu") == 0) thisarg->type=CHOICE_MENU;
			else if(strcmp(temp,"chooser")     == 0) thisarg->type=CHOOSER;
			else if(strcmp(temp,"tree")        == 0) thisarg->type=CHOICE_TREE;
			else if(strcmp(temp,"sai")         == 0) thisarg->type=CHOICE_SAI;
			else if(strcmp(temp,"weights")     == 0) thisarg->type=CHOICE_WEIGHTS;
			else if(strcmp(temp,"slider")      == 0) thisarg->type=SLIDER;
			else{
				sprintf(head,"Unknown argtype %s",temp);
				Error(head);
			}
		}
        /*
         *	argtext: The default text value of the symbol.
         *		$argument is replaced by this value if it is not
         *		changed in the dialog box by the user.
         */
		else if(Find(in_line,"argtext:"))
		{
			crop(in_line,head,temp);
			(void)strcpy(thisarg->textvalue,temp);
		}
        /*
         *	arglabel: Text label displayed in the dialog box for
         *		this argument.  It should be a discriptive label.
         */
		else if(Find(in_line,"arglabel:"))
		{
			crop(in_line,head,temp);
			thisarg->label=(char*)calloc(strlen(temp)+1, sizeof(char));
			if(thisarg->label == NULL) Error("Calloc");
			(void)strcpy(thisarg->label,temp);
		}
        /*
         *	Argument choice values use the following notation:
         *
         *	argchoice:Displayed value:Method
         *
         *	Where "Displayed value" is the label displayed in the dialog box
         *	and "Method" is the value passed back on the command line.
         */
		else if(Find(in_line,"argchoice:"))
		{
			crop(in_line,head,temp);
			crop(temp,head,tail);
			curchoice = thisarg->numchoices++;

			if(curchoice == 0) resize = (char*)calloc(1,sizeof(GargChoice));
			else resize               = (char *)realloc((char *)thisarg->choice, thisarg->numchoices*sizeof(GargChoice));
			if(resize == NULL) Error("argchoice: Realloc");
			thisarg->choice           = (GargChoice*)resize;

			(thisarg->choice[curchoice].label)  = NULL;
			(thisarg->choice[curchoice].method) = NULL;
			(thisarg->choice[curchoice].label)  = (char*)calloc(strlen(head)+1,sizeof(char));
			(thisarg->choice[curchoice].method) = (char*)calloc(strlen(tail)+1,sizeof(char));

			if(thisarg->choice[curchoice].method == NULL || thisarg->choice[curchoice].label == NULL) Error("Calloc");

			(void)strcpy(thisarg->choice[curchoice].label,head);
			(void)strcpy(thisarg->choice[curchoice].method,tail);
		}
        /*
         *	argmin: Minimum value for a slider
         */
		else if(Find(in_line,"argmin:"))
		{
			crop(in_line,head,temp);
			(void)sscanf(temp,"%lf",&(thisarg->min));
		}
        /*
         *	argmax: Maximum value for a slider
         */
		else if(Find(in_line,"argmax:"))
		{
			crop(in_line,head,temp);
			(void)sscanf(temp,"%lf",&(thisarg->max));
		}
        /*
         *	argmethod: Command line flag associated with this argument.
         *		Replaces argument in itemmethod description.
         */
		else if(Find(in_line,"argmethod:"))
		{
			crop(in_line,head,temp);
			thisarg->method = (char*)calloc(GBUFSIZ,strlen(temp));
            if(thisarg->method == NULL) Error("Calloc");
			(void)strcpy(thisarg->method,tail);
		}
        /*
         *	argvalue: default value for a slider
         */
		else if(Find(in_line,"argvalue:"))
		{
			crop(in_line,head,temp);
			if(thisarg->type == TEXT){
				strcpy(thisarg->textvalue,temp);
			}else{
				(void)sscanf(temp,"%lf",&(thisarg->fvalue));
				thisarg->ivalue = (int) thisarg->fvalue;
			}
		}
        /*
         *	argoptional: Flag specifying that an arguement is optional
         */
		else if(Find(in_line,"argoptional:"))
			thisarg->optional = TRUE;
        /*
         *	in: Input file description
         */
		else if(Find(in_line,"in:"))
		{
			crop(in_line,head,temp);
			curinput                 = (thisitem->numinputs)++;
			if(curinput == 0) resize = (char*)calloc(1,sizeof(GfileFormat));
			else resize              = (char *)realloc((char *)thisitem->input, (thisitem->numinputs)*sizeof(GfileFormat));
			if(resize == NULL) Error("in: Realloc");

			thisitem->input      = (GfileFormat*)resize;
			thisinput            = &(thisitem->input)[curinput];
			thisinput->save      = FALSE;
			thisinput->overwrite = FALSE;
			thisinput->maskable  = FALSE;
			thisinput->format    = 0;
			thisinput->symbol    = strdup(temp);
			thisinput->name      = NULL;
			thisinput->select    = SELECTED;
		}

        /*
         *	out: Output file description
         */

		else if(Find(in_line,"out:"))
		{
			crop(in_line,head,temp);
			curoutput = (thisitem->numoutputs)++;

			if(curoutput == 0) resize = (char*)calloc(1,sizeof(GfileFormat));
			else resize               = (char *)realloc((char *)thisitem->output, (thisitem->numoutputs)*sizeof(GfileFormat));
			if(resize == NULL) Error("out: Realloc");

			thisitem->output      = (GfileFormat*)resize;
			thisoutput            = &(thisitem->output)[curoutput];
			thisitem->output      = (GfileFormat*)resize;
			thisoutput            = &(thisitem->output)[curoutput];
			thisoutput->save      = FALSE;
			thisoutput->overwrite = FALSE;
			thisoutput->format    = 0;
			thisoutput->symbol    = strdup(temp);
			thisoutput->name      = NULL;
		}
		else if(Find(in_line,"informat:")) {
			if(thisinput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			crop(in_line,head,tail);

			if(Find(tail,"genbank")) thisinput->format        = GENBANK;
			else if(Find(tail,"gde")) thisinput->format       = GDE;
			else if(Find(tail,"na_flat")) thisinput->format   = NA_FLAT;
			else if(Find(tail,"colormask")) thisinput->format = COLORMASK;
			else if(Find(tail,"flat")) thisinput->format      = NA_FLAT;
			else if(Find(tail,"status")) thisinput->format    = STATUS_FILE;
			else fprintf(stderr,"Warning, unknown file format %s\n" ,tail);
		}
		else if(Find(in_line,"insave:")) {
			if(thisinput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			thisinput->save = TRUE;
		}
		else if(Find(in_line,"inselect:")) {
			if(thisinput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
            crop(in_line,head,tail);

            if(Find(tail,"one")) thisinput->select         = SELECT_ONE;
            else if(Find(tail,"region")) thisinput->select = SELECT_REGION;
            else if(Find(tail,"all")) thisinput->select    = ALL;
		}
		else if(Find(in_line,"inmask:")) {
			if(thisinput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			thisinput->maskable = TRUE;
		}
		else if(Find(in_line,"outformat:")) {
			if(thisoutput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			crop(in_line,head,tail);

			if(Find(tail,"genbank")) thisoutput->format        = GENBANK;
			else if(Find(tail,"gde")) thisoutput->format       = GDE;
			else if(Find(tail,"na_flat")) thisoutput->format   = NA_FLAT;
			else if(Find(tail,"flat")) thisoutput->format      = NA_FLAT;
			else if(Find(tail,"status")) thisoutput->format    = STATUS_FILE;
			else if(Find(tail,"colormask")) thisoutput->format = COLORMASK;
			else fprintf(stderr,"Warning, unknown file format %s\n" ,tail);
		}
		else if(Find(in_line,"outsave:")) {
			if(thisoutput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			thisoutput->save = TRUE;
		}
		else if(Find(in_line,"outoverwrite:")) {
			if(thisoutput == NULL) Error("Problem with $ARBHOME/GDEHELP/ARB_GDEmenus");
			thisoutput->overwrite = TRUE;
		}
	}

    assert(num_menus>0); // if this fails, the file ARB_GDEmenus contained no menus (maybe file has zero size)

	return;
}



/*
Find(): Search the target string for the given key
*/
int Find(const char *target,const char *key)
{
	int i,j,len1,dif,flag = FALSE;
	dif = (strlen(target)) - (len1 = strlen(key)) +1;

	if(len1>0)
		for(j=0;j<dif && flag == FALSE;j++)
		{
			flag = TRUE;
			for(i=0;i<len1 && flag;i++)
				flag = (key[i] == target[i+j])?TRUE:FALSE;

		}
	return(flag);
}


int Find2(const char *target,const char *key)
/*
*	Like find, but returns the index of the leftmost
*	occurence, and -1 if not found.
*/
{
    int i,j,len1,dif,flag = FALSE;
    dif = (strlen(target)) - (len1 = strlen(key)) +1;

    if(len1>0)
    {
	for(j=0;j<dif && flag == FALSE;j++)
	{
	    flag = TRUE;
	    for(i=0;i<len1 && flag;i++)
		flag = (key[i] == target[i+j])?TRUE:FALSE;

	}

	return(flag?j-1:-1);
    }

    return -1;
}


void Error(const char *msg)
{
	(void)fprintf(stderr,"%s\n",msg);
	exit(1);
}


int getline(FILE *file,char *string)
{
	int c;
	int i;
	for(i=0;(c=getc(file))!='\n';i++){
		if (c== EOF) break;
		if (i>= GBUFSIZ -2) break;
		string[i]=c;
	}
	string[i] = '\0';
	if (i==0 && c==EOF) return (EOF);
	else return (0);
}

/*
Crop():
	Split "this:that[:the_other]"
	into: "this" and "that[:the_other]"
*/

void crop(char *input,char *head,char *tail)
{
/*
*	Crop needs to be fixed so that whitespace is compressed off the end
*	of tail
*/
	int offset,end,i,j,length;

	length=strlen(input);
	for(offset=0;offset<length && input[offset] != ':';offset++)
		head[offset]=input[offset];
	head[offset++] = '\0';
	for(;offset<length && isspace(input[offset]);offset++);
	for(end=length-1;isspace(input[end]) && end>offset;end--);

	for(j=0,i=offset;i<=end;i++,j++)
		tail[j]=input[i];
	tail[j] = '\0';
	return;
}
