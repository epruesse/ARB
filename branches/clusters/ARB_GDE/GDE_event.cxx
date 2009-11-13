#include "GDE_extglob.h"
#include "GDE_awars.h"

#include <awt_filter.hxx>
#include <aw_window.hxx>
#include <AP_filter.hxx>

#include <set>
#include <string>

using namespace std;

#define DEFAULT_COLOR 8
extern adfiltercbstruct *agde_filtercd;

/*
  ReplaceArgs():
  Replace all command line arguments with the appropriate values
  stored for the chosen menu item.

  Copyright (c) 1989-1990, University of Illinois board of trustees.  All
  rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
  Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
  Carl Woese.

  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  All rights reserved.

*/


static char *ReplaceArgs(AW_root *awr,char *Action,GmenuItem *gmenuitem,int number)
{
    /*
     *  The basic idea is to replace all of the symbols in the method
     *  string with the values picked in the dialog box.  The method
     *  is the general command line structure.  All arguments have three
     *  parts, a label, a method, and a value.  The method never changes, and
     *  is used to represent '-flag's for a given function.  Values are the
     *  associated arguments that some flags require.  All symbols that
     *  require argvalue replacement should have a '$' infront of the symbol
     *  name in the itemmethod definition.  All symbols without the '$' will
     *  be replaced by their argmethod.  There is currently no way to do a label
     *  replacement, as the label is considered to be for use in the dialog
     *  box only.  An example command line replacement would be:
     *
     *       itemmethod=>        "lpr arg1 $arg1 $arg2"
     *
     *       arglabel arg1=>     "To printer?"
     *       argmethod arg1=>    "-P"
     *       argvalue arg1=>     "lw"
     *
     *       arglabel arg2=>     "File name?"
     *       argvalue arg2=>     "foobar"
     *       argmethod arg2=>    ""
     *
     *   final command line:
     *
     *       lpr -P lw foobar
     *
     *   At this point, the chooser dialog type only supports the arglabel and
     *   argmethod field.  So if an argument is of type chooser, and
     *   its symbol is "this", then "$this" has no real meaning in the
     *   itemmethod definition.  Its format in the .GDEmenu file is slighty
     *   different as well.  A choice from a chooser field looks like:
     *
     *       argchoice:Argument_label:Argument_method
     *
     *
     */
    const char *symbol=0;
    char *method=0;
    char *textvalue=0;
    char *temp;
    int i,newlen,type;
    symbol = gmenuitem->arg[number].symbol;
    //method = gmenuitem->arg[number]->method;
    //textvalue = gmenuitem->arg[number]->textvalue;
    type = gmenuitem->arg[number].type;
    if( (type == SLIDER) )
    {
        char *awarname = GDE_makeawarname(gmenuitem,number);
        textvalue      = (char*)malloc(GBUFSIZ);
        char *awalue   = awr->awar(awarname)->read_as_string();
        sprintf(textvalue,"%s",awalue);
        free(awalue);
    }
    else if((type == CHOOSER) ||
            (type == CHOICE_TREE) ||
            (type == CHOICE_SAI) ||
            (type == CHOICE_MENU) ||
            (type == CHOICE_LIST) ||
            (type == CHOICE_WEIGHTS) ||
            (type == TEXTFIELD))
    {
        char *awarname=GDE_makeawarname(gmenuitem,number);
        method=awr->awar(awarname)->read_string();
        textvalue=awr->awar(awarname)->read_string();
    }

    if(textvalue == NULL)   textvalue=(char *)calloc(1,sizeof(char));
    if(method == NULL)      method=(char *)calloc(1,sizeof(char));
    if(symbol == NULL)      symbol="";

    set<string>warned_about;
    int conversion_warning        = 0;
    int j                         = 0;

    for(; (i=Find2(Action+j,symbol)) != -1;)
    {
        i += j;
        ++j;
        if(i>0 && Action[i-1] == '$' )
        {
            newlen = strlen(Action)-strlen(symbol)
                +strlen(textvalue);
            temp = (char *)calloc(newlen,1);
            if (temp == NULL)
                Error("ReplaceArgs():Error in calloc");
            strncat(temp,Action,i-1);
            strncat(temp,textvalue,strlen(textvalue));
            strcat( temp,&(Action[i+strlen(symbol)]) );
            freeset(Action, temp);
        }
        else {
            if (warned_about.find(symbol) == warned_about.end()) {
                fprintf(stderr,
                        "old arb version converted '%s' to '%s' (now only '$%s' is converted)\n",
                        symbol, textvalue, symbol);
                conversion_warning++;
                warned_about.insert(symbol);
            }
        }
    }

    if (conversion_warning) {
        fprintf(stderr,
                "Conversion warnings occurred in Action:\n'%s'\n",
                Action);
    }

    free(textvalue);
    free(method);
    return(Action);
}



static long LMAX(long a,long b)
{
    if(a>b) return a;
    return b;
}

static void GDE_free(void **p) {
    freeset(*p, NULL);
}

static char *ReplaceFile(char *Action,GfileFormat file)
{
    char *symbol,*method,*temp;
    int i,newlen;
    symbol = file.symbol;
    method = file.name;

    for(; (i=Find2(Action,symbol)) != -1;)
    {
        newlen = strlen(Action)-strlen(symbol) + strlen(method)+1;
        temp = (char *)calloc(newlen,1);
        if (temp == NULL)
            Error("ReplaceFile():Error in calloc");
        strncat(temp,Action,i);
        strncat(temp,method,strlen(method));
        strcat( temp,&(Action[i+strlen(symbol)]) );
        freeset(Action, temp);
    }
    return(Action);
}

static char *ReplaceString(char *Action,const char *old,const char *news)
{
    const char *symbol;
    const char *method;
    char *temp;
    int i,newlen;

    symbol = old;
    method = news;

    for(; (i=Find2(Action,symbol)) != -1;)
    {
        newlen = strlen(Action)-strlen(symbol) + strlen(method)+1;
        temp = (char *)calloc(newlen,1);
        if (temp == NULL)
            Error("ReplaceFile():Error in calloc");
        strncat(temp,Action,i);
        strncat(temp,method,strlen(method));
        strcat( temp,&(Action[i+strlen(symbol)]) );
        freeset(Action, temp);
    }
    return(Action);
}


static void GDE_freesequ(NA_Sequence *sequ) {
    if (sequ) {
        GDE_free((void**)&sequ->comments);
        GDE_free((void**)&sequ->cmask);
        GDE_free((void**)&sequ->baggage);
        GDE_free((void**)&sequ->sequence);
    }
}

static void GDE_freeali(NA_Alignment *dataset) {
    if (dataset) {
        GDE_free((void**)&dataset->id);
        GDE_free((void**)&dataset->description);
        GDE_free((void**)&dataset->authority);
        GDE_free((void**)&dataset->cmask);
        GDE_free((void**)&dataset->selection_mask);
        GDE_free((void**)&dataset->alignment_name);

        for (unsigned long i=0;i<dataset->numelements;i++) {
            GDE_freesequ(dataset->element+i);
        }
    }
}

static void GDE_export(NA_Alignment *dataset, char *align, long oldnumelements) {
    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);
    
    long maxalignlen    = GBT_get_alignment_len(GLOBAL_gb_main, align);
    long isdefaultalign = 0;

    if (maxalignlen <= 0 && !error) {
        align             = GBT_get_default_alignment(GLOBAL_gb_main);
        if (!align) error = GB_await_error();
        else {
            isdefaultalign = 1;
            maxalignlen    = GBT_get_alignment_len(GLOBAL_gb_main, align);
        }
    }

    long lotyp = 0;
    if (!error) {
        GB_alignment_type at = GBT_get_alignment_type(GLOBAL_gb_main, align);

        switch (at) {
            case GB_AT_DNA:     lotyp = DNA;     break;
            case GB_AT_RNA:     lotyp = RNA;     break;
            case GB_AT_AA:      lotyp = PROTEIN; break;
            case GB_AT_UNKNOWN: lotyp = DNA;     break;
        }
    }

    unsigned long i;
    int           load_all = 0;

    for (i = oldnumelements; !error && i < dataset->numelements; i++) {
        NA_Sequence *sequ = dataset->element+i;
        int seqtyp, issame = 0;

        seqtyp = sequ->elementtype;
        if ((seqtyp == lotyp) || ((seqtyp == DNA) && (lotyp == RNA)) || ((seqtyp == RNA) && (lotyp == DNA))) {
            issame = 1;
        }
        else {
            aw_message(GBS_global_string("Warning: sequence type of species '%s' changed", sequ->short_name));
        }

        if (sequ->tmatrix) {
            for (long j = 0; j < sequ->seqlen; j++) {
                sequ->sequence[j] = (char)sequ->tmatrix[sequ->sequence[j]];
            }
        }

        char *savename = GBS_string_2_key(sequ->short_name);

        sequ->gb_species = 0;

        if (!issame) {          /* save as extended */
            GBDATA *gb_extended = GBT_find_or_create_SAI(GLOBAL_gb_main, savename);

            if (!gb_extended) error = GB_await_error();
            else {
                sequ->gb_species    = gb_extended;
                GBDATA *gb_data     = GBT_add_data(gb_extended, align, "data", GB_STRING);
                if (!gb_data) error = GB_await_error();
                else    error       = GBT_write_sequence(gb_data, align, maxalignlen, (char *)sequ->sequence);
            }
        }
        else {                  /* save as sequence */
            GBDATA *gb_species_data     = GB_search(GLOBAL_gb_main, "species_data", GB_CREATE_CONTAINER);
            if (!gb_species_data) error = GB_await_error();
            else {
                GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, savename);

                GB_push_my_security(GLOBAL_gb_main);

                if (gb_species) {   /* new element that already exists !!!! */
                    int select_mode;
                    const char *msg =
                        GBS_global_string("You are (re-)importing a species '%s'.\n"
                                          "That species already exists in your database!\n"
                                          "\n"
                                          "Possible actions:\n"
                                          "\n"
                                          "       - delete and overwrite the existing species\n"
                                          "       - skip - don't change the species\n"
                                          "       - reimport only the sequence of the existing species\n"
                                          "       - reimport all sequences (don't ask again)\n"
                                          "\n"
                                          "Note: After aligning it's recommended to choose 'reimport all'.",
                                          savename);

                    if (!load_all) {
                        select_mode = aw_question(msg,
                                                  "delete existing,"   // 0
                                                  "skip,"              // 1
                                                  "reimport sequence," // 2
                                                  "reimport all"       // 3
                                                  );

                        if (select_mode == 3) {     // load all sequences
                            load_all = 1;
                        }
                    }

                    if (load_all) select_mode = 2;    // reimport sequence
                    gde_assert(select_mode >= 0 && select_mode <= 2);

                    switch (select_mode) {
                        case 1:    // skip
                            gb_species = 0;
                            break;       // continue with next species

                        case 0:    // delete existing species
                            GB_delete(gb_species);
                            // fall-through!
                        case 2:    // reimport sequence
                            gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                            break;
                    }
                }
                else {
                    gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                }
                if (gb_species) {
                    sequ->gb_species = gb_species;

                    GBDATA *gb_data     = GBT_add_data(gb_species, align, "data", GB_STRING); // does only add if not already existing
                    if (!gb_data) error = GB_await_error();
                    else {
                        GBDATA *gb_old_data = GBT_read_sequence(gb_species, align);
                        if (gb_old_data) {  // we already have data -> compare checksums
                            const char *old_seq      = GB_read_char_pntr(gb_old_data);
                            long        old_checksum = GBS_checksum(old_seq, 1, "-.");
                            long        new_checksum = GBS_checksum((char *)sequ->sequence, 1, "-.");

                            if (old_checksum != new_checksum) {
                                aw_message(GBS_global_string("Warning: Sequence checksum for '%s' differs", savename));
                            }
                        }
                        error = GBT_write_sequence(gb_data, align, maxalignlen, (char *)sequ->sequence);
                    }
                }
                GB_pop_my_security(GLOBAL_gb_main);
            }
        }
        free(savename);
    }

    /* colormasks */
    for (i = 0; !error && i < dataset->numelements; i++) {
        NA_Sequence *sequ = &(dataset->element[i]);

        if (sequ->cmask) {
            maxalignlen     = LMAX(maxalignlen, sequ->seqlen);
            char *resstring = (char *)calloc((unsigned int)maxalignlen + 1, sizeof(char));
            char *dummy     = resstring;

            for (long j = 0; j < maxalignlen - sequ->seqlen; j++) *resstring++ = DEFAULT_COLOR;
            for (long k = 0; k < sequ->seqlen; k++)               *resstring++ = (char)sequ->cmask[i];
            *resstring = '\0';

            GBDATA *gb_ali     = GB_search(sequ->gb_species, align, GB_CREATE_CONTAINER);
            if (!gb_ali) error = GB_await_error();
            else {
                GBDATA *gb_color     = GB_search(gb_ali, "colmask", GB_BYTES);
                if (!gb_color) error = GB_await_error();
                else    error        = GB_write_bytes(gb_color, dummy, maxalignlen);
            }
            free(dummy);
        }
    }

    if (!error && dataset->cmask) {
        maxalignlen     = LMAX(maxalignlen, dataset->cmask_len);
        char *resstring = (char *)calloc((unsigned int)maxalignlen + 1, sizeof(char));
        char *dummy     = resstring;
        long  k;

        for (k = 0; k < maxalignlen - dataset->cmask_len; k++) *resstring++ = DEFAULT_COLOR;
        for (k = 0; k < dataset->cmask_len; k++)               *resstring++ = (char)dataset->cmask[k];
        *resstring = '\0';

        GBDATA *gb_extended     = GBT_find_or_create_SAI(GLOBAL_gb_main, "COLMASK");
        if (!gb_extended) error = GB_await_error();
        else {
            GBDATA *gb_color     = GBT_add_data(gb_extended, align, "colmask", GB_BYTES);
            if (!gb_color) error = GB_await_error();
            else    error        = GB_write_bytes(gb_color, dummy, maxalignlen);
        }

        free(dummy);
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    if (isdefaultalign) free(align);
}

static char *preCreateTempfile(const char *name) {
    // creates a tempfile and returns heapcopy of fullpath
    // exits in case of error
    char *fullname = GB_create_tempfile(name);

    if (!fullname) Error(GBS_global_string("preCreateTempfile: %s", GB_await_error())); // exits
    return fullname;
}

void GDE_startaction_cb(AW_window *aw,GmenuItem *gmenuitem,AW_CL cd)
{
    long oldnumelements=0;
    AWUSE(cd);
    AW_root *aw_root=aw->get_root();

    GapCompression  compress          = static_cast<GapCompression>(aw_root->awar(AWAR_GDE_COMPRESSION)->read_int());
    AP_filter      *filter2           = awt_get_filter(aw_root,agde_filtercd);
    char           *filter_name       = 0; /* aw_root->awar(AWAR_GDE_FILTER_NAME)->read_string() */
    char           *alignment_name    = strdup("ali_unknown");
    bool            marked            = (aw_root->awar(AWAR_GDE_SPECIES)->read_int() != 0);
    long            cutoff_stop_codon = aw_root->awar(AWAR_GDE_CUTOFF_STOPCODON)->read_int();

    GmenuItem *current_item;
    current_item=gmenuitem;

    aw_openstatus(current_item->label);
    aw_status((double)0);

    int   j;
    bool  flag;
    char *Action,buffer[GBUFSIZ];

    static int fileindx    = 0;
    int        select_mode = 0;
    int        stop        = 0;

    if (current_item->numinputs>0) {
        DataSet->gb_main = GLOBAL_gb_main;
        GB_begin_transaction(DataSet->gb_main);
        freeset(DataSet->alignment_name, GBT_get_default_alignment(DataSet->gb_main));
        freedup(alignment_name, DataSet->alignment_name);

        aw_status("reading database");
        if (gde_cgss.get_sequences) {
            stop = ReadArbdb2(DataSet, filter2, compress, cutoff_stop_codon);
        }
        else {
            stop = ReadArbdb(DataSet, marked, filter2, compress, cutoff_stop_codon);
        }
        GB_commit_transaction(DataSet->gb_main);

        if (!stop && DataSet->numelements==0) {
            aw_message("no sequences selected");
            stop = 1;
        }
    }

    if (!stop) {
        flag = false;
        for(j=0;j<current_item->numinputs;j++) {
            if(current_item->input[j].format != STATUS_FILE) {
                flag = true;
            }
        }
        if(flag && DataSet) select_mode = ALL; // TestSelection();

        int pid = getpid();

        for(j=0;j<current_item->numinputs;j++) {
            GfileFormat& gfile = current_item->input[j];

            sprintf(buffer,"gde%d_%d", pid, fileindx++);
            gfile.name = preCreateTempfile(buffer);

            switch(gfile.format) {
                case COLORMASK:   WriteCMask  (DataSet, gfile.name, select_mode, gfile.maskable); break;
                case GENBANK:     WriteGen    (DataSet, gfile.name, select_mode, gfile.maskable); break;
                case NA_FLAT:     WriteNA_Flat(DataSet, gfile.name, select_mode, gfile.maskable); break;
                case STATUS_FILE: WriteStatus (DataSet, gfile.name, select_mode)                ; break;
                case GDE:         WriteGDE    (DataSet, gfile.name, select_mode, gfile.maskable); break;
                default: break;
            }
        }

        for(j=0;j<current_item->numoutputs;j++) {
            sprintf(buffer,"gde%d_%d", pid, fileindx++);
            current_item->output[j].name = preCreateTempfile(buffer);
        }

        /*
         *  Create the command line for external the function call
         */
        Action = (char*)strdup(current_item->method);
        if(Action == NULL) Error("DO(): Error in duplicating method string");

        while (1) {
            char *oldAction = strdup(Action);

            for(j=0;j<current_item->numargs;j++) Action = ReplaceArgs(aw_root,Action,gmenuitem,j);
            bool changed = strcmp(oldAction, Action) != 0;
            free(oldAction);

            if (!changed) break;
        }

        for(j=0;j<current_item->numinputs;j++) Action = ReplaceFile(Action,current_item->input[j]);
        for(j=0;j<current_item->numoutputs;j++) Action = ReplaceFile(Action,current_item->output[j]);

        filter_name = AWT_get_combined_filter_name(aw_root, "gde");
        Action = ReplaceString(Action,"$FILTER",filter_name);

        /*
         *  call and go...
         */

        aw_status("calling external program");
        printf("Action: %s\n",Action);
        system(Action);
        free(Action);

        oldnumelements=DataSet->numelements;

        BlockInput = false;

        for(j=0;j<current_item->numoutputs;j++)
        {
            if(current_item->output[j].overwrite)
            {
                if(current_item->output[j].format == GDE)
                    OVERWRITE = true;
                else
                    Warning("Overwrite mode only available for GDE format");
            }
            switch(current_item->output[j].format)
            {
                /*
                 *     The LoadData routine must be reworked so that
                 *     OpenFileName uses it, and so I can remove the
                 *     major kluge in OpenFileName().
                 */
                case GENBANK:
                case NA_FLAT:
                case GDE:
                    /*ARB-change:*/
                    /*OpenFileName(current_item->output[j].name,NULL);*/
                    LoadData(current_item->output[j].name);
                    break;
                case COLORMASK:
                    ReadCMask(current_item->output[j].name);
                    break;
                case STATUS_FILE:
                    ReadStatus(current_item->output[j].name);
                    break;
                default:
                    break;
            }
            OVERWRITE = false;
        }
        for(j=0;j<current_item->numoutputs;j++)
        {
            if(!current_item->output[j].save)
            {
                unlink(current_item->output[j].name);
            }
        }

        for(j=0;j<current_item->numinputs;j++)
        {
            if(!current_item->input[j].save)
            {
                unlink(current_item->input[j].name);
            }
        }

        aw_closestatus();
        GDE_export(DataSet,alignment_name,oldnumelements);
    }
    else {
        aw_closestatus();
    }

    free(alignment_name);
    delete filter2;
    free(filter_name);

    GDE_freeali(DataSet);
    freeset(DataSet, (NA_Alignment *)Calloc(1,sizeof(NA_Alignment)));
    DataSet->rel_offset = 0;
}

