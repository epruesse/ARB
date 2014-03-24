#include "GDE_proto.h"

#include <aw_window.hxx>
#include <MultiFileReader.h>
#include <arb_file.h>

#include <cctype>

/*
  Copyright (c) 1989, University of Illinois board of trustees.  All rights
  reserved.  Written by Steven Smith at the Center for Prokaryote Genome
  Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
  Carl Woese.

  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  All rights reserved.

  Changed to fit into ARB by ARB development team.
*/


inline bool only_whitespace(const char *line) {
    size_t white = strspn(line, " \t");
    return line[white] == 0; // only 0 after leading whitespace
}

static char *readableItemname(const GmenuItem& i) {
    return GBS_global_string_copy("%s/%s", i.parent_menu->label, i.label);
}

__ATTR__NORETURN static void throwItemError(const GmenuItem& i, const char *error) {
    char       *itemName = readableItemname(i);
    const char *msg      = GBS_global_string("Invalid item '%s' in arb.menu (Reason: %s)", itemName, error); // @@@ use currently processed filename here
    free(itemName);
    throwError(msg);
}

static void CheckItemConsistency() {
    // (incomplete) consistency check.
    // bailing out with ItemError() here, will make unit-test and arb-startup fail!

    for (int m = 0; m<num_menus; ++m) {
        const Gmenu& M = menu[m];
        for (int i = 0; i<M.numitems; ++i) {
            const GmenuItem& I = M.item[i];

            if (I.seqtype != '-' && I.numinputs<1) {
                // Such an item would create a window where alignment/species selection has no GUI-elements.
                // Pressing 'GO' would result in failure or deadlock.
                throwItemError(I, "item defines seqtype ('seqtype:' != '-'), but is lacking input-specification ('in:')");
            }
        }
    }
}

static __ATTR__NORETURN void throwParseError(const char *msg, const LineReader& file) {
    fprintf(stderr, "\n%s:%li: ", file.getFilename().c_str(), file.getLineNumber());
    throwError(msg);
}

static void ParseMenus(LineReader& in) {
    /*  parses menus via LineReader (contains ALL found menu-files) and
     *  assemble an internal representation of the menu/menu-item hierarchy.
     */

    memset((char*)&menu[0], 0, sizeof(Gmenu)*GDEMAXMENU);

    int curarg    = 0;
    int curinput  = 0;
    int curoutput = 0;

    Gmenu        *thismenu   = NULL;
    GmenuItem    *thisitem   = NULL;
    GmenuItemArg *thisarg    = NULL;
    GfileFormat  *thisinput  = NULL;
    GfileFormat  *thisoutput = NULL;

    int  j;
    char temp[GBUFSIZ];
    char head[GBUFSIZ];
    char tail[GBUFSIZ];

    char *resize;

    string lineStr;

    while (in.getLine(lineStr)) {
        // const char *in_line = lineStr.c_str(); // @@@ preferred, but parser modifies line :/
        char in_line[GBUFSIZ];
        gde_assert(lineStr.length()<GBUFSIZ);
        strcpy(in_line, lineStr.c_str());

        if (in_line[0] == '#' || (in_line[0] && in_line[1] == '#')) {
            ; // skip line
        }
        else if (only_whitespace(in_line)) {
            ; // skip line
        }
        // menu: chooses menu to use
        else if (Find(in_line, "menu:")) { // @@@ should test for 'menu' and 'lmenu'
            splitEntry(in_line, head, temp);
            int curmenu = -1;
            for (j=0; j<num_menus; j++) {
                if (Find(temp, menu[j].label)) curmenu=j;
            }
            // If menu not found, make a new one
            if (curmenu == -1) {
                curmenu         = num_menus++;
                thismenu        = &menu[curmenu];
                thismenu->label = (char*)calloc(strlen(temp)+1, sizeof(char));

                (void)strcpy(thismenu->label, temp);
                thismenu->numitems = 0;
                thismenu->active_mask = AWM_ALL;
            }
            else {
                thismenu = &menu[curmenu];
            }
        }
        else if (Find(in_line, "menumask:")) {
            splitEntry(in_line, head, temp);
            if (strcmp("expert", temp) == 0) thismenu->active_mask = AWM_EXP;
        }
        // item: chooses menu item to use
        else if (Find(in_line, "item:")) {
            if (thismenu == NULL) throwParseError("'item' used w/o 'menu'", in);

            curarg    = -1;
            curinput  = -1;
            curoutput = -1;

            splitEntry(in_line, head, temp);

            int curitem = thismenu->numitems++;

            // Resize the item list for this menu (add one item);
            if (curitem == 0) {
                resize = (char*)GB_calloc(1, sizeof(GmenuItem)); // @@@ calloc->GB_calloc avoids (rui)
            }
            else {
                resize = (char *)realloc((char *)thismenu->item, thismenu->numitems*sizeof(GmenuItem));
            }

            thismenu->item = (GmenuItem*)resize;

            thisitem              = &(thismenu->item[curitem]);
            thisitem->label       = strdup(temp);
            thisitem->meta        = '\0';
            thisitem->seqtype     = '-';
            thisitem->numinputs   = 0;
            thisitem->numoutputs  = 0;
            thisitem->numargs     = 0;
            thisitem->help        = NULL;
            thisitem->parent_menu = thismenu;
            thisitem->aws         = NULL; // no window opened yet
            thisitem->active_mask = AWM_ALL;
        }

        // itemmethod: generic command line generated by this item
        else if (Find(in_line, "itemmethod:")) {
            splitEntry(in_line, head, temp);
            thisitem->method = (char*)calloc(strlen(temp)+1, sizeof(char));

            {
                char *to = thisitem->method;
                char *from = temp;
                char last = 0;
                char c;

                do {
                    c = *from++;
                    if (c == '@' && last == '@') {
                        // replace "@@" with "'"
                        // [WHY_USE_DOUBLE_AT]
                        // - cant use 1 single quote  ("'"). Things inside will not be preprocessed correctly.
                        // - cant use 2 single quotes ("''") any longer. clang fails on OSX.
                        to[-1] = '\'';
                    }
                    else {
                        *to++ = c;
                    }
                    last = c;
                }
                while (c!=0);
            }

        }
        // Help file
        else if (Find(in_line, "itemhelp:")) {
            splitEntry(in_line, head, temp);
            thisitem->help = GBS_string_eval(temp, "*.help=agde_*1.hlp", 0);
        }
        // Meta key equiv
        else if (Find(in_line, "itemmeta:")) {
            splitEntry(in_line, head, temp);
            thisitem->meta = temp[0];
        }
        else if (Find(in_line, "itemmask:")) {
            splitEntry(in_line, head, temp);
            if (strcmp("expert", temp) == 0) thisitem->active_mask = AWM_EXP;
        }
        else if (Find(in_line, "menumeta:")) {
            if (thismenu == NULL) throwParseError("'menumeta' used w/o 'menu' or 'lmenu'", in);
            splitEntry(in_line, head, temp);
            thismenu->meta = temp[0];
        }
        // Sequence type restriction
        else if (Find(in_line, "seqtype:")) {
            splitEntry(in_line, head, temp);
            thisitem->seqtype = toupper(temp[0]);
            /* 'A' -> amino acids,
             * 'N' -> nucleotides,
             * '-' -> don't select sequences,
             * otherwise any alignment
             */
        }
        /* arg: defines the symbol for a command line argument.
         *      this is used for substitution into the itemmethod
         *      definition.
         */

        else if (Find(in_line, "arg:")) {
            splitEntry(in_line, head, temp);
            curarg=thisitem->numargs++;
            if (curarg == 0) resize = (char*)calloc(1, sizeof(GmenuItemArg));
            else resize = (char *)realloc((char *)thisitem->arg, thisitem->numargs*sizeof(GmenuItemArg));

            memset((char *)resize + (thisitem->numargs-1)*sizeof(GmenuItemArg), 0, sizeof(GmenuItemArg));

            (thisitem->arg) = (GmenuItemArg*)resize;
            thisarg         = &(thisitem->arg[curarg]);
            thisarg->symbol = (char*)calloc(strlen(temp)+1, sizeof(char));
            (void)strcpy(thisarg->symbol, temp);

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
            thisarg->active_mask= AWM_ALL;
        }
        // argtype: Defines the type of argument (menu,chooser, text, slider)
        else if (Find(in_line, "argtype:")) {
            splitEntry(in_line, head, temp);

            int arglen = -1;
            if (strncmp(temp, "text", (arglen = 4)) == 0) {
                thisarg->type         = TEXTFIELD;
                freedup(thisarg->textvalue, "");

                if (temp[arglen] == 0) thisarg->textwidth = TEXTFIELDWIDTH; // only 'text'
                else {
                    if (temp[arglen] != '(' || temp[strlen(temp)-1] != ')') {
                        sprintf(head, "Unknown argtype '%s' -- syntax: text(width) e.g. text(20)", temp);
                        throwParseError(head, in);
                    }
                    thisarg->textwidth = atoi(temp+arglen+1);
                    if (thisarg->textwidth<1) {
                        sprintf(head, "Illegal textwidth specified in '%s'", temp);
                        throwParseError(head, in);
                    }
                }
            }
            else if (strcmp(temp, "choice_list") == 0) thisarg->type = CHOICE_LIST;
            else if (strcmp(temp, "choice_menu") == 0) thisarg->type = CHOICE_MENU;
            else if (strcmp(temp, "chooser")     == 0) thisarg->type = CHOOSER;
            else if (strcmp(temp, "filename")    == 0) {
                thisarg->type = FILE_SELECTOR;
                freedup(thisarg->textvalue, "");
            }
            else if (strcmp(temp, "sai")         == 0) thisarg->type = CHOICE_SAI;
            else if (strcmp(temp, "slider")      == 0) thisarg->type = SLIDER;
            else if (strcmp(temp, "tree")        == 0) thisarg->type = CHOICE_TREE;
            else if (strcmp(temp, "weights")     == 0) thisarg->type = CHOICE_WEIGHTS;
            else {
                sprintf(head, "Unknown argtype '%s'", temp);
                throwParseError(head, in);
            }
        }
        /* argtext: The default text value of the symbol.
         *          $argument is replaced by this value if it is not
         *           changed in the dialog box by the user.
         */
        else if (Find(in_line, "argtext:")) {
            splitEntry(in_line, head, temp);
            freedup(thisarg->textvalue, temp);
        }
        /* arglabel: Text label displayed in the dialog box for
         *           this argument. It should be a descriptive label.
         */
        else if (Find(in_line, "arglabel:")) {
            splitEntry(in_line, head, temp);
            thisarg->label = GBS_string_eval(temp, "\\\\n=\\n", 0);
        }
        /* Argument choice values use the following notation:
         *
         * argchoice:Displayed value:Method
         *
         * Where "Displayed value" is the label displayed in the dialog box
         * and "Method" is the value passed back on the command line.
         */
        else if (Find(in_line, "argchoice:")) {
            splitEntry(in_line, head, temp);
            splitEntry(temp, head, tail);

            int curchoice = thisarg->numchoices++;
            if (curchoice == 0) resize = (char*)calloc(1, sizeof(GargChoice));
            else                resize = (char*)realloc((char *)thisarg->choice, thisarg->numchoices*sizeof(GargChoice));

            thisarg->choice = (GargChoice*)resize;

            (thisarg->choice[curchoice].label)  = NULL;
            (thisarg->choice[curchoice].method) = NULL;
            (thisarg->choice[curchoice].label)  = (char*)calloc(strlen(head)+1, sizeof(char));
            (thisarg->choice[curchoice].method) = (char*)calloc(strlen(tail)+1, sizeof(char));

            (void)strcpy(thisarg->choice[curchoice].label, head);
            (void)strcpy(thisarg->choice[curchoice].method, tail);
        }
        // argmin: Minimum value for a slider
        else if (Find(in_line, "argmin:")) {
            splitEntry(in_line, head, temp);
            (void)sscanf(temp, "%lf", &(thisarg->min));
        }
        // argmax: Maximum value for a slider
        else if (Find(in_line, "argmax:")) {
            splitEntry(in_line, head, temp);
            (void)sscanf(temp, "%lf", &(thisarg->max));
        }
        /* argmethod: Command line flag associated with this argument.
         *            Replaces argument in itemmethod description.
         */
        else if (Find(in_line, "argmethod:")) {
            splitEntry(in_line, head, temp);
            thisarg->method = (char*)calloc(GBUFSIZ, strlen(temp));
            (void)strcpy(thisarg->method, tail);
        }
        // argvalue: default value for a slider
        else if (Find(in_line, "argvalue:")) {
            splitEntry(in_line, head, temp);
            if (thisarg->type == TEXT) {
                freedup(thisarg->textvalue, temp);
            }
            else {
                (void)sscanf(temp, "%lf", &(thisarg->fvalue));
                thisarg->ivalue = (int) thisarg->fvalue;
            }
        }
        // argoptional: Flag specifying that an argument is optional
        else if (Find(in_line, "argoptional:")) {
            gde_assert(thisarg);
            thisarg->optional = TRUE;
        }
        else if (Find(in_line, "argmask:")) {
            splitEntry(in_line, head, temp);
            if (strcmp("expert", temp) == 0) thisarg->active_mask = AWM_EXP;
        }
        // in: Input file description
        else if (Find(in_line, "in:")) {
            splitEntry(in_line, head, temp);
            curinput                 = (thisitem->numinputs)++;
            if (curinput == 0) resize = (char*)calloc(1, sizeof(GfileFormat));
            else resize              = (char *)realloc((char *)thisitem->input, (thisitem->numinputs)*sizeof(GfileFormat));

            thisitem->input      = (GfileFormat*)resize;
            thisinput            = &(thisitem->input)[curinput];
            thisinput->save      = FALSE;
            thisinput->overwrite = FALSE;
            thisinput->maskable  = FALSE;
            thisinput->format    = 0;
            thisinput->symbol    = strdup(temp);
            thisinput->name      = NULL;
            thisinput->select    = SELECTED;
            thisinput->typeinfo  = BASIC_TYPEINFO;
        }
        else if (Find(in_line, "informat:")) {
            if (thisinput == NULL) throwParseError("'informat' used w/o 'in'", in);
            splitEntry(in_line, head, tail);

            if (Find(tail, "genbank")) thisinput->format   = GENBANK;
            else if (Find(tail, "flat")) thisinput->format = NA_FLAT;
            else fprintf(stderr, "Warning, unknown file format %s\n", tail);
        }
        else if (Find(in_line, "insave:")) {
            if (thisinput == NULL) throwParseError("'insave' used w/o 'in'", in);
            thisinput->save = TRUE;
        }
        else if (Find(in_line, "intyped:")) {
            if (thisinput == NULL) throwParseError("'intyped' used w/o 'in'", in);
            splitEntry(in_line, head, tail);

            if (Find(tail, "detailed")) thisinput->typeinfo   = DETAILED_TYPEINFO;
            else if (Find(tail, "basic")) thisinput->typeinfo = BASIC_TYPEINFO;
            else throwParseError("Unknown value for 'intyped' (known: 'detailed', 'basic')", in);
        }
        else if (Find(in_line, "inselect:")) {
            if (thisinput == NULL) throwParseError("'inselect' used w/o 'in'", in);
            splitEntry(in_line, head, tail);

            if (Find(tail, "one")) thisinput->select         = SELECT_ONE;
            else if (Find(tail, "region")) thisinput->select = SELECT_REGION;
            else if (Find(tail, "all")) thisinput->select    = ALL;
        }
        else if (Find(in_line, "inmask:")) {
            if (thisinput == NULL) throwParseError("'inmask' used w/o 'in'", in);
            thisinput->maskable = TRUE;
        }
        // out: Output file description
        else if (Find(in_line, "out:")) {
            splitEntry(in_line, head, temp);
            gde_assert(thisitem);
            curoutput = (thisitem->numoutputs)++;

            if (curoutput == 0) resize = (char*)calloc(1, sizeof(GfileFormat));
            else resize               = (char *)realloc((char *)thisitem->output, (thisitem->numoutputs)*sizeof(GfileFormat));

            thisitem->output      = (GfileFormat*)resize;
            thisoutput            = &(thisitem->output)[curoutput];
            thisoutput->save      = FALSE;
            thisoutput->overwrite = FALSE;
            thisoutput->format    = 0;
            thisoutput->symbol    = strdup(temp);
            thisoutput->name      = NULL;
        }
        else if (Find(in_line, "outformat:")) {
            if (thisoutput == NULL) throwParseError("'outformat' used w/o 'out'", in);
            splitEntry(in_line, head, tail);

            if (Find(tail, "genbank")) thisoutput->format   = GENBANK;
            else if (Find(tail, "gde")) thisoutput->format  = GDE;
            else if (Find(tail, "flat")) thisoutput->format = NA_FLAT;
            else fprintf(stderr, "Warning, unknown file format %s\n", tail);
        }
        else if (Find(in_line, "outsave:")) {
            if (thisoutput == NULL) throwParseError("'outsave' used w/o 'out'", in);
            thisoutput->save = TRUE;
        }
        else if (Find(in_line, "outoverwrite:")) {
            if (thisoutput == NULL) throwParseError("'outoverwrite' used w/o 'out'", in);
            thisoutput->overwrite = TRUE;
        }
        else {
            throwParseError(GBS_global_string("No known GDE-menu-command found (line='%s')", in_line), in);
        }
    }

    CheckItemConsistency();

    gde_assert(num_menus>0); // if this fails, the file arb.menu contained no menus (maybe file has zero size)
}

GB_ERROR LoadMenus() {
    /*! Load menu config files
     *
     * loads all '*.menu' from "$ARBHOME/lib/gde" and "$ARB_PROP/gde"
     */

    GB_ERROR error = NULL;
    StrArray files;
    {
        char *user_menu_dir = strdup(GB_path_in_arbprop("gde"));

        if (!GB_is_directory(user_menu_dir)) {
            error = GB_create_directory(user_menu_dir);
        }
        gde_assert(!GB_have_error());

        GBS_read_dir(files, user_menu_dir, "/\\.menu$/");
        GBS_read_dir(files, GB_path_in_ARBLIB("gde"), "/\\.menu$/");

        if (GB_have_error()) error = GB_await_error();

        free(user_menu_dir);
    }

    if (!error) {
        MultiFileReader menus(files);
        error = menus.get_error();
        if (!error) {
            try {
                ParseMenus(menus);
            }
            catch (const string& err) {
                error = GBS_static_string(err.c_str());
            }
        }
    }

    if (error) error = GBS_global_string("Error while loading menus: %s", error);
    return error;
}

int Find(const char *target, const char *key) {
    // Search the target string for the given key
    return strstr(target, key) ? TRUE : FALSE;
}

int Find2(const char *target, const char *key) {
    /* Like Find(), but returns the index of the leftmost
     * occurrence, and -1 if not found.
     */
    const char *found = strstr(target, key);
    return found ? int(found-target) : -1;
}

// --------------------------------------------------------------------------------

void throwError(const char *msg) {
    // goes to header: __ATTR__NORETURN
    throw string(msg);
}

inline void trim(char *str) {
    int s = 0;
    int d = 0;

    while (isspace(str[s])) ++s;
    while (str[s]) str[d++] = str[s++];

    str[d] = 0;
    while (d>0 && isspace(str[d-1])) str[--d] = 0;
}

void splitEntry(const char *input, char *head, char *tail) {
    /*! Split "this:that[:the_other]" into: "this" and "that[:the_other]"
     */
    const char *colon = strchr(input, ':');
    if (colon) {
        int len   = colon-input;
        memcpy(head, input, len);
        head[len] = 0;

        strcpy(tail, colon+1);

        trim(tail);
    }
    else {
        strcpy(head, input);
    }
    trim(head);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_load_menu() {
    // very basic test: just detects failing assertions, crashes and errors

    gb_getenv_hook old = GB_install_getenv_hook(arb_test::fakeenv);
    {
        // ../UNIT_TESTER/run/homefake/

        TEST_EXPECT_NO_ERROR(LoadMenus());

        // basic check of loaded data (needs to be adapted if menus change):
        TEST_EXPECT_EQUAL(num_menus, 12);

        string menus;
        string menuitems;
        for (int m = 0; m<num_menus; ++m) {
            menus = menus + menu[m].label + ";";
            menuitems += GBS_global_string("%i;", menu[m].numitems);
        }

        TEST_EXPECT_EQUAL(menus,
                          "Import;Export;Print;Align;User;SAI;Incremental phylogeny;Phylogeny Distance Matrix;"
                          "Phylogeny max. parsimony;Phylogeny max. Likelyhood EXP;Phylogeny max. Likelyhood;Phylogeny (Other);");
        TEST_EXPECT_EQUAL(menuitems, "3;1;1;11;1;1;1;3;2;1;7;5;");
    }
    TEST_EXPECT_EQUAL((void*)arb_test::fakeenv, (void*)GB_install_getenv_hook(old));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

