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

inline __ATTR__NORETURN void throwError(const char *msg) {
    throw string(msg);
}

static __ATTR__NORETURN void throwParseError(const char *msg, const LineReader& file) {
    fprintf(stderr, "\n%s:%zu: %s\n", file.getFilename().c_str(), file.getLineNumber(), msg);
    fflush(stderr);
    throwError(msg);
}

static __ATTR__NORETURN void throwItemError(const GmenuItem& i, const char *error, const LineReader& file) {
    char       *itemName = readableItemname(i);
    const char *msg      = GBS_global_string("[Above this line] Invalid item '%s' defined: %s", itemName, error);
    free(itemName);
    throwParseError(msg, file);
}

static void CheckItemConsistency(const GmenuItem *item, const LineReader& file) {
    // (incomplete) consistency check.
    // bailing out with ItemError() here, will make unit-test and arb-startup fail!

    if (item) {
        const GmenuItem& I = *item;
        if (I.seqtype != '-' && I.numinputs<1) {
            // Such an item would create a window where alignment/species selection is present,
            // but no sequence export will take place.
            //
            // Pressing 'GO' would result in failure or deadlock.
            throwItemError(I, "item defines seqtype ('seqtype:' <> '-'), but is lacking input-specification ('in:')", file);
        }
        if (I.seqtype == '-' && I.numinputs>0) {
            // Such an item would create a window where alignment/species selection has no GUI-elements,
            // but sequences are exported (generating a corrupt sequence file)
            //
            // Pressing 'GO' would result in failure.
            throwItemError(I, "item defines no seqtype ('seqtype:' = '-'), but defines input-specification ('in:')", file);
        }
    }
}

#define THROW_IF_NO(ptr,name) do { if (ptr == NULL) throwParseError(GBS_global_string("'%s' used w/o '" name "'", head), in); } while(0)

#define THROW_IF_NO_MENU()   THROW_IF_NO(thismenu, "menu")
#define THROW_IF_NO_ITEM()   THROW_IF_NO(thisitem, "item")
#define THROW_IF_NO_ARG()    THROW_IF_NO(thisarg, "arg")
#define THROW_IF_NO_INPUT()  THROW_IF_NO(thisinput, "in")
#define THROW_IF_NO_OUTPUT() THROW_IF_NO(thisoutput, "out")

static void ParseMenus(LineReader& in) {
    /*  parses menus via LineReader (contains ALL found menu-files) and
     *  assemble an internal representation of the menu/menu-item hierarchy.
     *
     *  please document changes in ../HELP_SOURCE/oldhelp/gde_menus.hlp
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

    bool thismenu_firstOccurance = true;

    int  j;
    char temp[GBUFSIZ];
    char head[GBUFSIZ];
    char tail[GBUFSIZ];

    string lineStr;

    while (in.getLine(lineStr)) {
        gde_assert(lineStr.length()<GBUFSIZ); // otherwise buffer usage may fail

        const char *in_line = lineStr.c_str();
        if (in_line[0] == '#' || (in_line[0] && in_line[1] == '#')) {
            ; // skip line
        }
        else if (only_whitespace(in_line)) {
            ; // skip line
        }
        else {
            splitEntry(in_line, head, temp);

            // menu: chooses menu to use (may occur multiple times with same label!)
            if (strcmp(head, "menu") == 0) {
                int curmenu = -1;
                for (j=0; j<num_menus && curmenu == -1; j++) {
                    if (strcmp(temp, menu[j].label) == 0) curmenu=j;
                }

                thismenu_firstOccurance = curmenu == -1;

                // If menu not found, make a new one
                if (thismenu_firstOccurance) {
                    curmenu               = num_menus++;
                    thismenu              = &menu[curmenu];
                    thismenu->label       = ARB_strdup(temp);
                    thismenu->numitems    = 0;
                    thismenu->active_mask = AWM_ALL;
                }
                else {
                    thismenu = &menu[curmenu];
                }

                CheckItemConsistency(thisitem, in);
                thisitem   = NULL;
                thisarg    = NULL;
                thisoutput = NULL;
                thisinput  = NULL;
            }
            else if (strcmp(head, "menumask") == 0) {
                THROW_IF_NO_MENU();
                AW_active wanted_mask = strcmp("expert", temp) == 0 ? AWM_EXP : AWM_ALL;
                if (!thismenu_firstOccurance && thismenu->active_mask != wanted_mask) {
                    throwParseError(GBS_global_string("menumask has inconsistent definitions (in different definitions of menu '%s')", thismenu->label), in);
                }
                thismenu->active_mask = wanted_mask;
            }
            else if (strcmp(head, "menumeta") == 0) {
                THROW_IF_NO_MENU();
                char wanted_meta = temp[0];
                if (!thismenu_firstOccurance && thismenu->meta != wanted_meta) {
                    if (wanted_meta != 0) {
                        if (thismenu->meta != 0) {
                            throwParseError(GBS_global_string("menumeta has inconsistent definitions (in different definitions of menu '%s')", thismenu->label), in);
                        }
                        else {
                            thismenu->meta = wanted_meta;
                        }
                    }
                }
                else {
                    thismenu->meta = wanted_meta;
                }
            }
            // item: chooses menu item to use
            else if (strcmp(head, "item") == 0) {
                CheckItemConsistency(thisitem, in);

                THROW_IF_NO_MENU();

                curarg    = -1;
                curinput  = -1;
                curoutput = -1;

                int curitem = thismenu->numitems++;

                // Resize the item list for this menu (add one item)
                if (curitem == 0) {
                    ARB_alloc(thismenu->item, 1);
                }
                else {
                    ARB_realloc(thismenu->item, thismenu->numitems);
                }

                thisitem = &(thismenu->item[curitem]);

                thisitem->numargs    = 0;
                thisitem->numoutputs = 0;
                thisitem->numinputs  = 0;
                thisitem->label      = ARB_strdup(temp);
                thisitem->method     = NULL;
                thisitem->input      = NULL;
                thisitem->output     = NULL;
                thisitem->arg        = NULL;
                thisitem->meta       = '\0';
                thisitem->seqtype    = '-';   // no default sequence export
                thisitem->aligned    = false;
                thisitem->help       = NULL;

                thisitem->parent_menu = thismenu;
                thisitem->aws         = NULL; // no window opened yet
                thisitem->active_mask = AWM_ALL;
                thisitem->popup       = NULL;

                for (int i = 0; i<curitem; ++i) {
                    if (strcmp(thismenu->item[i].label, thisitem->label) == 0) {
                        throwParseError(GBS_global_string("Duplicated item label '%s'", thisitem->label), in);
                    }
                }

                thisarg = NULL;
            }

            // itemmethod: generic command line generated by this item
            else if (strcmp(head, "itemmethod") == 0) {
                THROW_IF_NO_ITEM();
                ARB_calloc(thisitem->method, strlen(temp)+1);

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
            else if (strcmp(head, "itemhelp") == 0) {
                THROW_IF_NO_ITEM();
                thisitem->help = GBS_string_eval(temp, "*.help=agde_*1.hlp", 0);
            }
            // Meta key equiv
            else if (strcmp(head, "itemmeta") == 0) {
                THROW_IF_NO_ITEM();
                thisitem->meta = temp[0];
            }
            else if (strcmp(head, "itemmask") == 0) {
                THROW_IF_NO_ITEM();
                if (strcmp("expert", temp) == 0) thisitem->active_mask = AWM_EXP;
            }
            // Sequence type restriction
            else if (strcmp(head, "seqtype") == 0) {
                THROW_IF_NO_ITEM();
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

            else if (strcmp(head, "arg") == 0) {
                THROW_IF_NO_ITEM();

                curarg = thisitem->numargs++;
                ARB_recalloc(thisitem->arg, curarg, thisitem->numargs);

                thisarg = &(thisitem->arg[curarg]);

                thisarg->symbol      = ARB_strdup(temp);
                thisarg->type        = 0;
                thisarg->min         = 0.0;
                thisarg->max         = 0.0;
                thisarg->numchoices  = 0;
                thisarg->choice      = NULL;
                thisarg->textvalue   = NULL;
                thisarg->ivalue      = 0;
                thisarg->fvalue      = 0.0;
                thisarg->label       = 0;
                thisarg->active_mask = AWM_ALL;
            }
            // argtype: Defines the type of argument (menu,chooser, text, slider)
            else if (strcmp(head, "argtype") == 0) {
                THROW_IF_NO_ARG();
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
            else if (strcmp(head, "argtext") == 0) {
                THROW_IF_NO_ARG();
                freedup(thisarg->textvalue, temp);
            }
            /* arglabel: Text label displayed in the dialog box for
             *           this argument. It should be a descriptive label.
             */
            else if (strcmp(head, "arglabel") == 0) {
                THROW_IF_NO_ARG();
                thisarg->label = GBS_string_eval(temp, "\\\\n=\\n", 0);
            }
            /* Argument choice values use the following notation:
             *
             * argchoice:Displayed value:Method
             *
             * Where "Displayed value" is the label displayed in the dialog box
             * and "Method" is the value passed back on the command line.
             */
            else if (strcmp(head, "argchoice") == 0) {
                THROW_IF_NO_ARG();
                splitEntry(temp, head, tail);

                int curchoice = thisarg->numchoices++;
                ARB_recalloc(thisarg->choice, curchoice, thisarg->numchoices);

                thisarg->choice[curchoice].label  = ARB_strdup(head);
                thisarg->choice[curchoice].method = ARB_strdup(tail);
            }
            // argmin: Minimum value for a slider
            else if (strcmp(head, "argmin") == 0) {
                THROW_IF_NO_ARG();
                (void)sscanf(temp, "%lf", &(thisarg->min));
            }
            // argmax: Maximum value for a slider
            else if (strcmp(head, "argmax") == 0) {
                THROW_IF_NO_ARG();
                (void)sscanf(temp, "%lf", &(thisarg->max));
            }
            // argvalue: default value for a slider
            else if (strcmp(head, "argvalue") == 0) {
                THROW_IF_NO_ARG();
                if (thisarg->type == TEXT) {
                    freedup(thisarg->textvalue, temp);
                }
                else {
                    (void)sscanf(temp, "%lf", &(thisarg->fvalue));
                    thisarg->ivalue = (int) thisarg->fvalue;
                }
            }
            else if (strcmp(head, "argmask") == 0) {
                THROW_IF_NO_ARG();
                if (strcmp("expert", temp) == 0) thisarg->active_mask = AWM_EXP;
            }
            // in: Input file description
            else if (strcmp(head, "in") == 0) {
                THROW_IF_NO_ITEM();

                curinput = (thisitem->numinputs)++;
                ARB_recalloc(thisitem->input, curinput, thisitem->numinputs);

                thisinput = &(thisitem->input)[curinput];

                thisinput->save     = false;
                thisinput->format   = 0;
                thisinput->symbol   = ARB_strdup(temp);
                thisinput->name     = NULL;
                thisinput->typeinfo = BASIC_TYPEINFO;
            }
            else if (strcmp(head, "informat") == 0) {
                THROW_IF_NO_INPUT();
                if (Find(temp, "genbank")) thisinput->format   = GENBANK;
                else if (Find(temp, "flat")) thisinput->format = NA_FLAT;
                else throwParseError(GBS_global_string("Unknown informat '%s' (allowed 'genbank' or 'flat')", temp), in);
            }
            else if (strcmp(head, "insave") == 0) {
                THROW_IF_NO_INPUT();
                thisinput->save = true;
            }
            else if (strcmp(head, "intyped") == 0) {
                THROW_IF_NO_INPUT();
                if (Find(temp, "detailed")) thisinput->typeinfo   = DETAILED_TYPEINFO;
                else if (Find(temp, "basic")) thisinput->typeinfo = BASIC_TYPEINFO;
                else throwParseError(GBS_global_string("Unknown value '%s' for 'intyped' (known: 'detailed', 'basic')", temp), in);
            }
            // out: Output file description
            else if (strcmp(head, "out") == 0) {
                THROW_IF_NO_ITEM();

                curoutput = (thisitem->numoutputs)++;
                ARB_recalloc(thisitem->output, curoutput, thisitem->numoutputs);

                thisoutput = &(thisitem->output)[curoutput];

                thisoutput->save   = false;
                thisoutput->format = 0;
                thisoutput->symbol = ARB_strdup(temp);
                thisoutput->name   = NULL;
            }
            else if (strcmp(head, "outformat") == 0) {
                THROW_IF_NO_OUTPUT();
                if (Find(temp, "genbank")) thisoutput->format   = GENBANK;
                else if (Find(temp, "gde")) thisoutput->format  = GDE;
                else if (Find(temp, "flat")) thisoutput->format = NA_FLAT;
                else throwParseError(GBS_global_string("Unknown outformat '%s' (allowed 'genbank', 'gde' or 'flat')", temp), in);
            }
            else if (strcmp(head, "outaligned") == 0) {
                THROW_IF_NO_OUTPUT();
                if (Find(temp, "yes")) thisitem->aligned = true;
                else throwParseError(GBS_global_string("Unknown outaligned '%s' (allowed 'yes' or skip entry)", temp), in);
            }
            else if (strcmp(head, "outsave") == 0) {
                THROW_IF_NO_OUTPUT();
                thisoutput->save = true;
            }
            else {
                throwParseError(GBS_global_string("No known GDE-menu-command found (line='%s')", in_line), in);
            }
        }
    }

    CheckItemConsistency(thisitem, in);

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
        char *user_menu_dir = ARB_strdup(GB_path_in_arbprop("gde"));

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

bool Find(const char *target, const char *key) {
    // Search the target string for the given key
    return strstr(target, key) ? true : false;
}

int Find2(const char *target, const char *key) {
    /* Like Find(), but returns the index of the leftmost
     * occurrence, and -1 if not found.
     */
    const char *found = strstr(target, key);
    return found ? int(found-target) : -1;
}

// --------------------------------------------------------------------------------

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
        TEST_EXPECT_EQUAL(num_menus, 13);

        string menus;
        string menuitems;
        for (int m = 0; m<num_menus; ++m) {
            menus = menus + menu[m].label + ";";
            menuitems += GBS_global_string("%i;", menu[m].numitems);
        }

        TEST_EXPECT_EQUAL(menus,
                          "Import;Export;Print;Align;Network;SAI;Incremental phylogeny;Phylogeny Distance Matrix;"
                          "Phylogeny max. parsimony;Phylogeny max. Likelihood EXP;Phylogeny max. Likelihood;Phylogeny (Other);User;");
        TEST_EXPECT_EQUAL(menuitems, "3;1;2;10;1;1;1;3;2;1;8;5;0;");
    }
    TEST_EXPECT_EQUAL((void*)arb_test::fakeenv, (void*)GB_install_getenv_hook(old));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

