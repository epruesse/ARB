#include "GDE_extglob.h"
#include "GDE_awars.h"

#include <awt_filter.hxx>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_file.hxx>
#include <arb_progress.h>
#include <AP_filter.hxx>

#include <set>
#include <string>

#include <unistd.h>

using namespace std;

extern adfiltercbstruct *agde_filter;

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
     *  is the general command line structure.  All arguments have two
     *  parts : a label and a value.  Values are the
     *  associated arguments that some flags require.  All symbols that
     *  require argvalue replacement should have a '$' infront of the symbol
     *  name in the itemmethod definition.
     *
     *  If '$symbol' is prefixed by '!' ARB_GDE does a label replacement, i.e. insert
     *  the value visible in GUI. Only works for argchoice arguments!
     *  This is intended for informational use (e.g. to write used settings
     *  into the comment of a generated tree).
     *
     *  An example command line replacement would be:
     *
     *       itemmethod=>        "lpr -P $arg1 $arg2"
     *
     *       arglabel arg1=>     "To printer?"
     *       argvalue arg1=>     "lw"
     *
     *       arglabel arg2=>     "File name?"
     *       argvalue arg2=>     "foobar"
     *
     *   final command line:
     *
     *       lpr -P lw foobar
     *
     */

    char       *textvalue  = 0;
    const char *labelvalue = 0;

    const GmenuItemArg& currArg = gmenuitem->arg[number];

    const char *symbol = currArg.symbol;
    int         type   = currArg.type;

    if (type == SLIDER) {
        char *awarname = GDE_makeawarname(gmenuitem, number);
        textvalue      = awr->awar(awarname)->read_as_string();
        free(awarname);
    }
    else if (type == FILE_SELECTOR) {
        char *awar_base = GDE_maketmpawarname(gmenuitem, number);
        textvalue  = AW_get_selected_fullname(awr, awar_base);
        free(awar_base);
    }
    else if (type == CHOOSER ||
             type == CHOICE_TREE ||
             type == CHOICE_SAI ||
             type == CHOICE_MENU ||
             type == CHOICE_LIST ||
             type == CHOICE_WEIGHTS ||
             type == TEXTFIELD)
    {
        char *awarname = GDE_makeawarname(gmenuitem, number);
        textvalue      = awr->awar(awarname)->read_string();

        if (currArg.choice) {
            for (int c = 0; c<currArg.numchoices && !labelvalue; ++c) {
                GargChoice& choice = currArg.choice[c];
                if (choice.method) {
                    if (strcmp(choice.method, textvalue) == 0) {
                        labelvalue = choice.label;
                    }
                }
            }
        }
    }

    if (textvalue == NULL)  textvalue = (char *)ARB_calloc(1, 1);
    if (symbol == NULL)     symbol    = "";

    set<string>warned_about;
    int conversion_warning = 0;
    
    for (int i, j = 0; (i=Find2(Action+j, symbol)) != -1;) {
        i += j;
        ++j;
        if (i>0 && Action[i-1] == '$') {
            const char *replaceBy = textvalue;
            int         skip      = 1;

            if (i>1 && Action[i-2] == '!') { // use label (if available)
                if (labelvalue) {
                    replaceBy = labelvalue;
                    skip = 2; // skip '!'
                }
                else {
                    aw_message(GBS_global_string("[ARB_GDE]: Cannot access label of '%s'\n", symbol));
                    return NULL; // @@@ ignores resources (should only occur during development)
                }
            }

            int   repLen = strlen(replaceBy);
            int   symLen = strlen(symbol);
            int   newlen = strlen(Action)-skip-symLen+repLen+1;
            char *temp   = (char*)ARB_calloc(newlen, 1);

            strncat(temp, Action, i-skip);
            strncat(temp, replaceBy, repLen);
            strcat(temp, &(Action[i+symLen]));
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
    return (Action);
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
        temp = (char *)ARB_calloc(newlen, 1);
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
        temp = (char *)ARB_calloc(newlen, 1);
        strncat(temp, Action, i);
        strncat(temp, method, strlen(method));
        strcat(temp, &(Action[i+strlen(symbol)]));
        freeset(Action, temp);
    }
    return (Action);
}

static void GDE_freesequ(NA_Sequence *sequ) {
    if (sequ) {
        freenull(sequ->comments);
        freenull(sequ->baggage);
        freenull(sequ->sequence);
    }
}

NA_Alignment::NA_Alignment(GBDATA *gb_main_)
    : id(NULL),
      description(NULL),
      authority(NULL),
      numelements(0),
      maxnumelements(0),
      maxlen(0),
      rel_offset(0),
      element(NULL),
      numgroups(0),
      group(NULL),
      format(0),
      gb_main(gb_main_)
{
    GB_transaction ta(gb_main);
    alignment_name = GBT_get_default_alignment(gb_main);
    alignment_type = GBT_get_alignment_type(gb_main, alignment_name);
}

NA_Alignment::~NA_Alignment() {
    free(id);
    free(description);
    free(authority);
    free(alignment_name);

    for (unsigned long i=0; i<numelements; i++) {
        GDE_freesequ(element+i);
    }
}

static GB_ERROR write_sequence_autoinc_alisize(GBDATA *gb_data, long& ali_len, const char *sequence, int seq_len) {
    /* writes sequence data.
     * Specials things done:
     * - cuts content beyond 'ali_len' if nothing relevant there
     * - increments alignment length (stored in DB and parameter)
     */

    GB_ERROR error    = 0;
    int      part_len = seq_len; // size that will be written
    if (seq_len > ali_len) { // sequence longer than alignment
        // check whether it can be cutoff w/o loosing anything relevant
        int oversize          = seq_len-ali_len;
        int irrelevant        = strspn(sequence+ali_len, "-.nN"); // @@@ this has to be different for AA!
        int relevant_oversize = oversize-irrelevant;

        part_len = ali_len+relevant_oversize;

        if (relevant_oversize) { // got some relevant data behind alignment length -> increase alignment length
            int         new_ali_len = part_len;
            GBDATA     *gb_main     = GB_get_root(gb_data);
            const char *ali_name    = GB_read_key_pntr(GB_get_father(gb_data));

            gde_assert(GBT_get_alignment_len(gb_main, ali_name) == ali_len);

            error   = GBT_set_alignment_len(gb_main, ali_name, new_ali_len);
            ali_len = new_ali_len;
        }
    }

    if (!error) {
        if (part_len<seq_len) {
            char *seq_part = ARB_strndup(sequence, part_len);
            error = GB_write_string(gb_data, seq_part);
            free(seq_part);
        }
        else {
            gde_assert(part_len == seq_len);
            error = GB_write_string(gb_data, sequence);
        }
    }

    return error;
}

inline bool isgap(char c) { return c == '.' || c == '-'; }
inline bool isTU(char c) { return c == 'T' || c == 'U'; }

inline char eatgaps(const char *seq, int& index) {
    /*! increments index forward to next base (or EOS)
     * @return first gap char seen or 0
     */
    if (isgap(seq[index])) {
        char gap = seq[index++];
        while (isgap(seq[index])) ++index;
        return gap;
    }
    return 0;
}

static char *fix_aligned_data(const char *old_seq, const char *new_seq, GB_alignment_type ali_type) {
    char *fixed = strdup(new_seq);

    int o = 0;
    int n = 0;
    int f = 0;

    bool fixTU = ali_type == GB_AT_RNA || ali_type == GB_AT_DNA;
    char TU    = ali_type == GB_AT_RNA ? 'U' : 'T';
    char tu    = tolower(TU);

    while (old_seq[o]) {
        char og = eatgaps(old_seq, o);
        char ng = eatgaps(new_seq, n);

        if (og && ng && og != ng) memset(fixed+f, og, n-f);
        f = n;

        char oc = old_seq[o++];
        char nc = new_seq[n++];
        if (!nc) break;

        char oC = toupper(oc);
        char nC = toupper(nc);

        if (fixTU && isTU(nC) && isTU(oC)) fixed[f] = (oc == oC) ? TU : tu;
        else if (oc != nc && oC == nC)     fixed[f] = oc;

        f++;
    }

    return fixed;
}

static void export_to_DB(NA_Alignment& dataset, size_t oldnumelements, bool aligned_data) {
    /*! (re-)import data into arb DB
     * @param dataset normally has been read from file (which was created by external tool)
     * @param oldnumelements start index into dataset
     * @param aligned_data if true => only import sequences; expect checksums did not change; repair some minor, unwanted changes (case, T<>U, gaptype)
     */
    if (dataset.numelements == oldnumelements) return;
    gde_assert(dataset.numelements > oldnumelements); // otherwise this is a noop

    GBDATA     *gb_main     = db_access.gb_main;
    GB_ERROR    error       = GB_begin_transaction(gb_main);
    const char *ali_name    = dataset.alignment_name;
    long        maxalignlen = GBT_get_alignment_len(gb_main, ali_name);

    if (maxalignlen <= 0 && !error) {
        error = GB_await_error();
    }

    long lotyp = 0;
    if (!error) {
        GB_alignment_type at = GBT_get_alignment_type(gb_main, ali_name);

        switch (at) {
            case GB_AT_DNA:     lotyp = DNA;     break;
            case GB_AT_RNA:     lotyp = RNA;     break;
            case GB_AT_AA:      lotyp = PROTEIN; break;
            case GB_AT_UNKNOWN: lotyp = DNA;     break;
        }
    }

    unsigned long i;
    const long    oldalignlen = maxalignlen;
    bool          auto_format = false;

    AW_repeated_question overwrite_question;
    AW_repeated_question checksum_change_question;

    arb_progress progress("importing", dataset.numelements-oldnumelements+1); // +1 avoids zero-progress
    for (i = oldnumelements; !error && i < dataset.numelements; i++) {
        NA_Sequence *sequ = dataset.element+i;
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

        const char *new_seq     = (const char *)sequ->sequence;
        int         new_seq_len = sequ->seqlen;

        gde_assert(new_seq[new_seq_len] == 0);
        gde_assert((int)strlen(new_seq) == new_seq_len);

        if (!issame) {          // save as extended
            GBDATA *gb_extended = GBT_find_or_create_SAI(gb_main, savename);

            if (!gb_extended) error = GB_await_error();
            else {
                sequ->gb_species = gb_extended;
                GBDATA *gb_data  = GBT_add_data(gb_extended, ali_name, "data", GB_STRING);

                if (!gb_data) error = GB_await_error();
                else {
                    error = write_sequence_autoinc_alisize(gb_data, maxalignlen, new_seq, new_seq_len);
                    if (new_seq_len<maxalignlen) auto_format = true;
                }
            }
        }
        else {                  // save as sequence
            GBDATA *gb_species_data     = GBT_get_species_data(gb_main);
            if (!gb_species_data) error = GB_await_error();
            else {
                GBDATA *gb_species       = GBT_find_species_rel_species_data(gb_species_data, savename);
                bool    fix_data_changes = false;

                GB_push_my_security(gb_main);

                if (gb_species) {   // new element that already exists !!!!
                    enum ReplaceMode { REPLACE_SPEC = 0, REIMPORT_SEQ = 1, SKIP_IMPORT  = 2 } replace_mode;

                    if (aligned_data) {
                        replace_mode = REIMPORT_SEQ;
                    }
                    else {
                        const char *question =
                            GBS_global_string("You are (re-)importing a species '%s'.\n"
                                              "That species already exists in your database!\n"
                                              "\n"
                                              "Possible actions:\n"
                                              "\n"
                                              "       - overwrite existing species (all fields)\n"
                                              "       - overwrite the sequence (does not change other fields)\n"
                                              "       - skip import of the species\n",
                                              savename);

                        replace_mode = (ReplaceMode)overwrite_question.get_answer("GDE_overwrite", question, "Overwrite species,Overwrite sequence only,Skip entry", "all", false);
                    }

                    switch (replace_mode) {
                        case SKIP_IMPORT:
                            gb_species = 0;
                            break;
                        case REPLACE_SPEC:
                            error      = GB_delete(gb_species);
                            gb_species = NULL;
                            if (error) break;
                            // fall-through
                        case REIMPORT_SEQ:
                            gb_species = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                            if (!gb_species) error = GB_await_error();
                            break;
                    }

                    fix_data_changes = replace_mode == REIMPORT_SEQ;
                }
                else {
                    if (aligned_data) {
                        aw_message(GBS_global_string("Warning: new species '%s' has been created (unexpected; possible naming problems)", savename));
                    }
                    gb_species             = GBT_find_or_create_species_rel_species_data(gb_species_data, savename);
                    if (!gb_species) error = GB_await_error();
                }

                if (gb_species) {
                    gde_assert(!error);
                    sequ->gb_species = gb_species;

                    GBDATA *gb_data     = GBT_add_data(gb_species, ali_name, "data", GB_STRING); // does only add if not already existing
                    if (!gb_data) error = GB_await_error();
                    else {
                        GBDATA *gb_old_data   = GBT_find_sequence(gb_species, ali_name);
                        bool    writeSequence = true;
                        if (gb_old_data) {          // we already have data -> compare checksums
                            const char *old_seq = GB_read_char_pntr(gb_old_data);

                            long old_checksum    = 0;
                            long new_checksum    = 0;
                            bool calcStdChecksum = true;
                            if (fix_data_changes) {
                                char *new_seq_fixed = fix_aligned_data(old_seq, new_seq, dataset.alignment_type);  // apply some fixes to (realigned) data

                                switch (dataset.alignment_type) {
                                    case GB_AT_DNA:
                                    case GB_AT_RNA: {
                                        char *old_TU = GBS_string_eval(old_seq,       ":T=U:t=u", NULL);
                                        char *new_TU = GBS_string_eval(new_seq_fixed, ":T=U:t=u", NULL);

                                        old_checksum = GBS_checksum(old_TU, 1, "-.");
                                        new_checksum = GBS_checksum(new_TU, 1, "-.");

                                        free(new_TU);
                                        free(old_TU);
                                        break;
                                    }
                                    case GB_AT_AA:
                                    case GB_AT_UNKNOWN:
                                        old_checksum = GBS_checksum(old_seq,       1, "-.");
                                        new_checksum = GBS_checksum(new_seq_fixed, 1, "-.");
                                        break;
                                }

                                if (new_checksum == old_checksum) { // fix succeeded
                                    free(sequ->sequence);
                                    sequ->sequence  = (NA_Base*)new_seq_fixed;
                                    new_seq         = new_seq_fixed;
                                    calcStdChecksum = false;
                                }
                                else {
                                    fprintf(stderr, "Checksum changed for '%s':\nold='%s'\nfix='%s' (failed)\nnew='%s'\n", savename, old_seq, new_seq_fixed, new_seq);
                                    free(new_seq_fixed);
                                }
                            }
                            if (calcStdChecksum) {
                                old_checksum = GBS_checksum(old_seq, 1, "-.");
                                new_checksum = GBS_checksum(new_seq, 1, "-.");
                            }

                            if (old_checksum != new_checksum) {
                                if (!fix_data_changes) { // already dumped above
                                    fprintf(stderr, "Checksum changed for '%s':\nold='%s'\nnew='%s'\n", savename, old_seq, new_seq);
                                }

                                char *question = GBS_global_string_copy("Warning: Sequence checksum of '%s' has changed!\n"
                                                                        "This should NOT happen if you aligned sequences!\n"
                                                                        "(see console for changes to sequence)", savename);

                                const char *questionID = aligned_data ? "GDE_accept_aligner_seqchange" : "GDE_accept_seqchange";

                                enum ChangeMode {
                                    ACCEPT_CHANGE = 0,
                                    REJECT_CHANGE = 1,
                                } change_mode = (ChangeMode)checksum_change_question.get_answer(questionID, question, "Accept change,Reject", "all", false);

                                if (change_mode == REJECT_CHANGE) writeSequence = false;

                                aw_message(GBS_global_string("Warning: Sequence checksum for '%s' has changed (%s)",
                                                             savename, writeSequence ? "accepted" : "rejected"));
                                free(question);
                            }
                        }
                        if (writeSequence) {
                            error = write_sequence_autoinc_alisize(gb_data, maxalignlen, new_seq, new_seq_len);
                            if (new_seq_len<maxalignlen) auto_format = true;
                        }
                    }
                }
                GB_pop_my_security(gb_main);
            }
        }
        free(savename);
        progress.inc_and_check_user_abort(error);
    }

    if (!auto_format) auto_format = oldalignlen != maxalignlen;

    if (auto_format) {
        if (db_access.format_ali) {
            GB_push_my_security(gb_main);
            error = db_access.format_ali(gb_main, ali_name);
            GB_pop_my_security(gb_main);
        }
    }

    progress.done();

    GB_end_transaction_show_error(db_access.gb_main, error, aw_message);
}

static char *preCreateTempfile(const char *name) {
    // creates a tempfile and returns heapcopy of fullpath
    // exits in case of error
    char *fullname = GB_create_tempfile(name);

    if (!fullname) aw_message(GBS_global_string("[ARB_GDE]: %s", GB_await_error()));
    return fullname;
}

void GDE_startaction_cb(AW_window *aw, GmenuItem *gmenuitem) {
    gde_assert(!GB_have_error());

    AW_root   *aw_root      = aw->get_root();
    GmenuItem *current_item = gmenuitem;

    GapCompression compress = static_cast<GapCompression>(aw_root->awar(AWAR_GDE_COMPRESSION)->read_int());
    arb_progress   progress(current_item->label);
    NA_Alignment   DataSet(db_access.gb_main);
    int            stop     = 0;

    if (current_item->numinputs>0) {
        TypeInfo typeinfo = UNKNOWN_TYPEINFO;
        {
            for (int j=0; j<current_item->numinputs; j++) {
                if (j == 0) { typeinfo = current_item->input[j].typeinfo; }
                else if (current_item->input[j].typeinfo != typeinfo) {
                    aw_message("'intyped' must be same for all inputs (config error in GDE menu file)");
                    stop = 1;
                }
            }
        }
        gde_assert(typeinfo != UNKNOWN_TYPEINFO);

        if (!stop) {
            AP_filter *filter2 = awt_get_filter(agde_filter);
            gde_assert(gmenuitem->seqtype != '-'); // inputs w/o seqtype? impossible!
            {
                GB_ERROR error = awt_invalid_filter(filter2);
                if (error) {
                    aw_message(error);
                    stop = 1;
                }
            }

            if (!stop) {
                GB_transaction ta(DataSet.gb_main);
                progress.subtitle("reading database");

                long cutoff_stop_codon = aw_root->awar(AWAR_GDE_CUTOFF_STOPCODON)->read_int();
                bool marked            = (aw_root->awar(AWAR_GDE_SPECIES)->read_int() != 0);

                if (db_access.get_sequences) {
                    stop = ReadArbdb2(DataSet, filter2, compress, cutoff_stop_codon, typeinfo);
                }
                else {
                    stop = ReadArbdb(DataSet, marked, filter2, compress, cutoff_stop_codon, typeinfo);
                }
            }
            delete filter2;
        }

        if (!stop && DataSet.numelements==0) {
            aw_message("no sequences selected");
            stop = 1;
        }
    }

    if (!stop) {
        int select_mode = (current_item->numinputs>0) ? ALL : NONE;
        int pid         = getpid();

        static int fileindx = 0;
        for (int j=0; j<current_item->numinputs; j++) {
            GfileFormat& gfile = current_item->input[j];

            char buffer[GBUFSIZ];
            sprintf(buffer, "gde%d_%d", pid, fileindx++);
            gfile.name = preCreateTempfile(buffer);

            switch (gfile.format) {
                case GENBANK: WriteGen    (DataSet, gfile.name, select_mode); break;
                case NA_FLAT: WriteNA_Flat(DataSet, gfile.name, select_mode); break;
                case GDE:     WriteGDE    (DataSet, gfile.name, select_mode); break;
                default: break;
            }
        }

        for (int j=0; j<current_item->numoutputs; j++) {
            char buffer[GBUFSIZ];
            sprintf(buffer, "gde%d_%d", pid, fileindx++);
            current_item->output[j].name = preCreateTempfile(buffer);
        }

        // Create the command line for external the function call
        char *Action = strdup(current_item->method);

        while (1) {
            char *oldAction = strdup(Action);

            for (int j=0; j<current_item->numargs; j++) Action = ReplaceArgs(aw_root, Action, gmenuitem, j);
            bool changed = strcmp(oldAction, Action) != 0;
            free(oldAction);

            if (!changed) break;
        }

        for(int j=0; j<current_item->numinputs;  j++) Action = ReplaceFile(Action, current_item->input[j]);
        for(int j=0; j<current_item->numoutputs; j++) Action = ReplaceFile(Action, current_item->output[j]);

        if (Find(Action, "$FILTER") == true) {
            char *filter_name = AWT_get_combined_filter_name(aw_root, AWAR_PREFIX_GDE_TEMP);
            Action            = ReplaceString(Action, "$FILTER", filter_name);
            free(filter_name);
        }

        // call and go...
        progress.subtitle("calling external program");
        aw_message_if(GBK_system(Action));
        free(Action);

        size_t oldnumelements = DataSet.numelements;

        for (int j=0; j<current_item->numoutputs; j++) {
            switch (current_item->output[j].format) {
                case GENBANK:
                case NA_FLAT:
                case GDE:
                    LoadData(current_item->output[j].name, DataSet);
                    break;
                default:
                    gde_assert(0);
                    break;
            }
        }
        for (int j=0; j<current_item->numoutputs; j++) {
            if (!current_item->output[j].save) {
                unlink(current_item->output[j].name);
            }
        }

        for (int j=0; j<current_item->numinputs; j++) {
            if (!current_item->input[j].save) {
                unlink(current_item->input[j].name);
            }
        }

        export_to_DB(DataSet, oldnumelements, current_item->aligned);
    }

    gde_assert(!GB_have_error());
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation fixed_as(GB_alignment_type ali_type, const char *old, const char *expected_fix, const char *aligned) {
    using namespace    arb_test;
    char              *fixed = fix_aligned_data(old, aligned, ali_type);
    match_expectation  e     = that(fixed).is_equal_to(expected_fix);
    free(fixed);
    return e;
}

#define TEST_FIX_ALIGNED(t,o,f,a)             TEST_EXPECTATION(fixed_as(t,o,f,a))
#define TEST_FIX_ALIGNED__BROKEN(t,o,fw,fg,a) TEST_EXPECTATION__BROKEN(fixed_as(t,o,fw,a), fixed_as(t,o,fg,a))

void TEST_fix_aligned_data() {
    TEST_FIX_ALIGNED(GB_AT_RNA,
                     "...A---CG..G--U.....", // old
                     "..AC--G..GU...",       // fixed: gaps corrected; T->U
                     "--AC--G--GT---");      // aligned

    TEST_FIX_ALIGNED(GB_AT_RNA,
                     "A---CG..G--U",         // old (no gaps at border)
                     "--AC--G..GU---",       // fixed: gaps corrected; T->U
                     "--AC--G--GT---");      // aligned

    TEST_FIX_ALIGNED(GB_AT_RNA,
                     "...A---CG..G--U.....", // old
                     "AC--G..GU",            // fixed: gaps corrected; T->U
                     "AC--G--GT");           // aligned (no gaps at border)

    TEST_FIX_ALIGNED(GB_AT_RNA,
                     "A---CG..G--U", // old
                     "AC-----GT",    // not fixed
                     "AC-----GT");   // aligned (bases changed!)

    TEST_FIX_ALIGNED(GB_AT_DNA,
                     "A---cTUu..G--t", // old
                     "AcT--Tt..Gt",    // fixed: case restored; U's convert to T's
                     "ACT--UT--GU");   // aligned

    TEST_FIX_ALIGNED(GB_AT_RNA,
                     "A---cTUu..G--t", // old
                     "AcU--Uu..Gu",    // fixed: case restored; T's convert to U's
                     "ACT--UT--GU");   // aligned
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
