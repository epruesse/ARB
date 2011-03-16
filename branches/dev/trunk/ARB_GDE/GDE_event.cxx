#include "GDE_extglob.h"
#include "GDE_awars.h"

#include <awt_filter.hxx>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_file.hxx>
#include <arb_progress.h>
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


static char *ReplaceArgs(AW_root *awr, char *Action, GmenuItem *gmenuitem, int number)
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
    int i, newlen, type;
    symbol = gmenuitem->arg[number].symbol;
    type = gmenuitem->arg[number].type;
    if ((type == SLIDER)) {
        char *awarname = GDE_makeawarname(gmenuitem, number);
        textvalue      = awr->awar(awarname)->read_as_string();
        free(awarname);
    }
    else if (type == FILE_SELECTOR) {
        char *awar_base = GDE_maketmpawarname(gmenuitem, number);
        textvalue  = AW_get_selected_fullname(awr, awar_base);
        free(awar_base);
    }
    else if ((type == CHOOSER) ||
             (type == CHOICE_TREE) ||
             (type == CHOICE_SAI) ||
            (type == CHOICE_MENU) ||
            (type == CHOICE_LIST) ||
            (type == CHOICE_WEIGHTS) ||
            (type == TEXTFIELD))
    {
        char *awarname=GDE_makeawarname(gmenuitem, number);
        method=awr->awar(awarname)->read_string();
        textvalue=awr->awar(awarname)->read_string();
    }

    if (textvalue == NULL)  textvalue=(char *)calloc(1, sizeof(char));
    if (method == NULL)     method=(char *)calloc(1, sizeof(char));
    if (symbol == NULL)     symbol="";

    set<string>warned_about;
    int conversion_warning        = 0;
    int j                         = 0;

    for (; (i=Find2(Action+j, symbol)) != -1;)
    {
        i += j;
        ++j;
        if (i>0 && Action[i-1] == '$')
        {
            newlen = strlen(Action)-strlen(symbol)
                +strlen(textvalue);
            temp = (char *)calloc(newlen, 1);
            if (temp == NULL)
                Error("ReplaceArgs():Error in calloc");
            strncat(temp, Action, i-1);
            strncat(temp, textvalue, strlen(textvalue));
            strcat(temp, &(Action[i+strlen(symbol)]));
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
    return (Action);
}



static long LMAX(long a, long b)
{
    if (a>b) return a;
    return b;
}

static void GDE_free(void **p) {
    freenull(*p);
}

static char *ReplaceFile(char *Action, GfileFormat file)
{
    char *symbol, *method, *temp;
    int i, newlen;
    symbol = file.symbol;
    method = file.name;

    for (; (i=Find2(Action, symbol)) != -1;)
    {
        newlen = strlen(Action)-strlen(symbol) + strlen(method)+1;
        temp = (char *)calloc(newlen, 1);
        if (temp == NULL)
            Error("ReplaceFile():Error in calloc");
        strncat(temp, Action, i);
        strncat(temp, method, strlen(method));
        strcat(temp, &(Action[i+strlen(symbol)]));
        freeset(Action, temp);
    }
    return (Action);
}

static char *ReplaceString(char *Action, const char *old, const char *news)
{
    const char *symbol;
    const char *method;
    char *temp;
    int i, newlen;

    symbol = old;
    method = news;

    for (; (i=Find2(Action, symbol)) != -1;)
    {
        newlen = strlen(Action)-strlen(symbol) + strlen(method)+1;
        temp = (char *)calloc(newlen, 1);
        if (temp == NULL)
            Error("ReplaceFile():Error in calloc");
        strncat(temp, Action, i);
        strncat(temp, method, strlen(method));
        strcat(temp, &(Action[i+strlen(symbol)]));
        freeset(Action, temp);
    }
    return (Action);
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

        for (unsigned long i=0; i<dataset->numelements; i++) {
            GDE_freesequ(dataset->element+i);
        }
    }
}

static void GDE_export(NA_Alignment *dataset, char *align, long oldnumelements) {
    GBDATA   *gb_main = db_access.gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    long maxalignlen    = GBT_get_alignment_len(gb_main, align);
    long isdefaultalign = 0;

    if (maxalignlen <= 0 && !error) {
        GB_clear_error(); // clear "alignment not found" error
    
        align             = GBT_get_default_alignment(gb_main);
        if (!align) error = GB_await_error();
        else {
            isdefaultalign = 1;
            maxalignlen    = GBT_get_alignment_len(gb_main, align);
        }
    }

    long lotyp = 0;
    if (!error) {
        GB_alignment_type at = GBT_get_alignment_type(gb_main, align);

        switch (at) {
            case GB_AT_DNA:     lotyp = DNA;     break;
            case GB_AT_RNA:     lotyp = RNA;     break;
            case GB_AT_AA:      lotyp = PROTEIN; break;
            case GB_AT_UNKNOWN: lotyp = DNA;     break;
        }
    }

    unsigned long i;

    enum ReplaceMode {
        REPLACE_SPEC      = 0,
        REIMPORT_SEQ      = 1,
        SKIP_IMPORT       = 2,
        REPLACE_SPEC_ALL  = 3,
        REIMPORT_SEQ_ALL  = 4,
        SKIP_IMPORT_ALL   = 5
    } replace_mode = REPLACE_SPEC;

    enum ChangeMode {
        ACCEPT_CHANGE     = 0,
        REJECT_CHANGE     = 1,
        ACCEPT_CHANGE_ALL = 2,
        REJECT_CHANGE_ALL = 3
    } change_mode = ACCEPT_CHANGE;

    arb_progress progress("importing", dataset->numelements-oldnumelements+1); // +1 avoids zero-progress
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
            sequ->sequence[sequ->seqlen] = 0;
        }

        char *savename = GBS_string_2_key(sequ->short_name);

        sequ->gb_species = 0;

        const char *new_seq = (const char *)sequ->sequence;
        gde_assert(new_seq[sequ->seqlen] == 0);
        gde_assert((int)strlen(new_seq) == sequ->seqlen);

        if (!issame) {          /* save as extended */
            GBDATA *gb_extended = GBT_find_or_create_SAI(db_access.gb_main, savename);

            if (!gb_extended) error = GB_await_error();
            else {
                sequ->gb_species = gb_extended;
                GBDATA *gb_data  = GBT_add_data(gb_extended, align, "data", GB_STRING);

                if (!gb_data) error = GB_await_error();
                else error          = GBT_write_sequence(gb_data, align, maxalignlen, new_seq);
            }
        }
        else {                  /* save as sequence */
            GBDATA *gb_species_data     = GB_search(db_access.gb_main, "species_data", GB_CREATE_CONTAINER);
            if (!gb_species_data) error = GB_await_error();
            else {
                GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, savename);

                GB_push_my_security(db_access.gb_main);

                if (gb_species) {   /* new element that already exists !!!! */
                    if (replace_mode != REPLACE_SPEC_ALL &&
                        replace_mode != REIMPORT_SEQ_ALL &&
                        replace_mode != SKIP_IMPORT_ALL)
                    {
                        const char *question =
                            GBS_global_string("You are (re-)importing a species '%s'.\n"
                                              "That species already exists in your database!\n"
                                              "\n"
                                              "Possible actions:\n"
                                              "\n"
                                              "       - overwrite existing species (all fields)\n"
                                              "       - overwrite the sequence (does not change other fields)\n"
                                              "       - skip import of the species\n"
                                              "\n"
                                              "Note: After aligning it's recommended to choose 'overwrite sequence'.",
                                              savename);

                        replace_mode = (ReplaceMode)aw_question(question,
                                                                "Overwrite species,Overwrite sequence only,Skip entry,"
                                                                "^Overwrite ALL species,Overwrite ALL sequences,Skip ALL entries");
                    }

                    switch (replace_mode) {
                        default:
                            gde_assert(0);
                            // fall-through
                        case SKIP_IMPORT:
                        case SKIP_IMPORT_ALL:
                            gb_species = 0;
                            break;

                        case REPLACE_SPEC:
                        case REPLACE_SPEC_ALL:
                            error      = GB_delete(gb_species);
                            gb_species = NULL;
                            if (error) break;
                            // fall-through
                        case REIMPORT_SEQ:
                        case REIMPORT_SEQ_ALL:
                            gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                            if (!gb_species) error = GB_await_error();
                            break;
                    }
                }
                else {
                    gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                    if (!gb_species) error = GB_await_error();
                }

                if (gb_species) {
                    gde_assert(!error);
                    sequ->gb_species = gb_species;

                    GBDATA *gb_data     = GBT_add_data(gb_species, align, "data", GB_STRING); // does only add if not already existing
                    if (!gb_data) error = GB_await_error();
                    else {
                        GBDATA *gb_old_data   = GBT_read_sequence(gb_species, align);
                        bool    writeSequence = true;
                        if (gb_old_data) {          // we already have data -> compare checksums
                            const char *old_seq      = GB_read_char_pntr(gb_old_data);
                            long        old_checksum = GBS_checksum(old_seq, 1, "-.");
                            long        new_checksum = GBS_checksum(new_seq, 1, "-.");

                            if (old_checksum != new_checksum) {
                                if (change_mode != ACCEPT_CHANGE_ALL &&
                                    change_mode != REJECT_CHANGE_ALL)
                                {
                                    const char *question = GBS_global_string("Warning: Sequence checksum of '%s' has changed\n", savename);

                                    change_mode = (ChangeMode)aw_question(question,
                                                                          "Accept change,Reject (do not import),"
                                                                          "^Accept ALL,Reject ALL");
                                }

                                if (change_mode == REJECT_CHANGE || change_mode == REJECT_CHANGE_ALL) {
                                    writeSequence = false;
                                }
                                aw_message(GBS_global_string("Warning: Sequence checksum for '%s' has changed (%s)",
                                                             savename, writeSequence ? "accepted" : "rejected"));
                            }
                        }
                        if (writeSequence) {
                            error = GBT_write_sequence(gb_data, align, maxalignlen, new_seq);
                        }
                    }
                }
                GB_pop_my_security(db_access.gb_main);
            }
        }
        free(savename);
        progress.inc_and_check_user_abort(error);
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

        GBDATA *gb_extended     = GBT_find_or_create_SAI(db_access.gb_main, "COLMASK");
        if (!gb_extended) error = GB_await_error();
        else {
            GBDATA *gb_color     = GBT_add_data(gb_extended, align, "colmask", GB_BYTES);
            if (!gb_color) error = GB_await_error();
            else    error        = GB_write_bytes(gb_color, dummy, maxalignlen);
        }

        free(dummy);
    }

    progress.done();

    GB_end_transaction_show_error(db_access.gb_main, error, aw_message);
    if (isdefaultalign) free(align);
}

static char *preCreateTempfile(const char *name) {
    // creates a tempfile and returns heapcopy of fullpath
    // exits in case of error
    char *fullname = GB_create_tempfile(name);

    if (!fullname) Error(GBS_global_string("preCreateTempfile: %s", GB_await_error())); // exits
    return fullname;
}

void GDE_startaction_cb(AW_window *aw, GmenuItem *gmenuitem, AW_CL /*cd*/) {
    long oldnumelements=0;
    AW_root *aw_root=aw->get_root();

    GapCompression  compress          = static_cast<GapCompression>(aw_root->awar(AWAR_GDE_COMPRESSION)->read_int());
    AP_filter      *filter2           = awt_get_filter(aw_root, agde_filtercd);
    char           *filter_name       = 0; /* aw_root->awar(AWAR_GDE_FILTER_NAME)->read_string() */
    char           *alignment_name    = strdup("ali_unknown");
    bool            marked            = (aw_root->awar(AWAR_GDE_SPECIES)->read_int() != 0);
    long            cutoff_stop_codon = aw_root->awar(AWAR_GDE_CUTOFF_STOPCODON)->read_int();
    GmenuItem      *current_item      = gmenuitem;
    arb_progress    progress(current_item->label);

    int   j;
    bool  flag;
    char *Action, buffer[GBUFSIZ];

    static int fileindx    = 0;
    int        select_mode = 0;
    int        stop        = 0;

    if (current_item->numinputs>0) {
        DataSet->gb_main = db_access.gb_main;
        GB_begin_transaction(DataSet->gb_main);
        freeset(DataSet->alignment_name, GBT_get_default_alignment(DataSet->gb_main));
        freedup(alignment_name, DataSet->alignment_name);

        progress.subtitle("reading database");
        if (db_access.get_sequences) {
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
        for (j=0; j<current_item->numinputs; j++) {
            if (current_item->input[j].format != STATUS_FILE) {
                flag = true;
            }
        }
        if (flag && DataSet) select_mode = ALL;

        int pid = getpid();

        for (j=0; j<current_item->numinputs; j++) {
            GfileFormat& gfile = current_item->input[j];

            sprintf(buffer, "gde%d_%d", pid, fileindx++);
            gfile.name = preCreateTempfile(buffer);

            switch (gfile.format) {
                case COLORMASK:   WriteCMask  (DataSet, gfile.name, select_mode, gfile.maskable); break;
                case GENBANK:     WriteGen    (DataSet, gfile.name, select_mode, gfile.maskable); break;
                case NA_FLAT:     WriteNA_Flat(DataSet, gfile.name, select_mode, gfile.maskable); break;
                case STATUS_FILE: WriteStatus (DataSet, gfile.name, select_mode);                 break;
                case GDE:         WriteGDE    (DataSet, gfile.name, select_mode, gfile.maskable); break;
                default: break;
            }
        }

        for (j=0; j<current_item->numoutputs; j++) {
            sprintf(buffer, "gde%d_%d", pid, fileindx++);
            current_item->output[j].name = preCreateTempfile(buffer);
        }

        // Create the command line for external the function call
        Action = (char*)strdup(current_item->method);
        if (Action == NULL) Error("DO(): Error in duplicating method string");

        while (1) {
            char *oldAction = strdup(Action);

            for (j=0; j<current_item->numargs; j++) Action = ReplaceArgs(aw_root, Action, gmenuitem, j);
            bool changed = strcmp(oldAction, Action) != 0;
            free(oldAction);

            if (!changed) break;
        }

        for (j=0; j<current_item->numinputs; j++) Action = ReplaceFile(Action, current_item->input[j]);
        for (j=0; j<current_item->numoutputs; j++) Action = ReplaceFile(Action, current_item->output[j]);

        filter_name = AWT_get_combined_filter_name(aw_root, "gde");
        Action = ReplaceString(Action, "$FILTER", filter_name);

        // call and go...
        progress.subtitle("calling external program");
        printf("Action: %s\n", Action);
        system(Action);
        free(Action);

        oldnumelements=DataSet->numelements;

        BlockInput = false;

        for (j=0; j<current_item->numoutputs; j++)
        {
            if (current_item->output[j].overwrite)
            {
                if (current_item->output[j].format == GDE)
                    OVERWRITE = true;
                else
                    Warning("Overwrite mode only available for GDE format");
            }
            switch (current_item->output[j].format)
            {
                /* The LoadData routine must be reworked so that
                 * OpenFileName uses it, and so I can remove the
                 * major kluge in OpenFileName().
                 */
                case GENBANK:
                case NA_FLAT:
                case GDE:
                    LoadData(current_item->output[j].name);
                    break;
                case COLORMASK:
                    ReadCMask(current_item->output[j].name);
                    break;
                default:
                    break;
            }
            OVERWRITE = false;
        }
        for (j=0; j<current_item->numoutputs; j++)
        {
            if (!current_item->output[j].save)
            {
                unlink(current_item->output[j].name);
            }
        }

        for (j=0; j<current_item->numinputs; j++)
        {
            if (!current_item->input[j].save)
            {
                unlink(current_item->input[j].name);
            }
        }

        GDE_export(DataSet, alignment_name, oldnumelements);
    }

    free(alignment_name);
    delete filter2;
    free(filter_name);

    GDE_freeali(DataSet);
    freeset(DataSet, (NA_Alignment *)Calloc(1, sizeof(NA_Alignment)));
    DataSet->rel_offset = 0;
}

