/*=======================================================================================*/
/*                                                                                       */
/*    File       : ED4_visualizeSAI.cxx                                                  */
/*    Purpose    : To Visualise the Sequence Associated Information (SAI) in the Editor  */
/*    Time-stamp : Tue Apr 1 2003                                                        */
/*    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                     */
/*    web site   : http://www.arb-home.de/                                               */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_preset.hxx>

#include "ed4_class.hxx"
#include "ed4_visualizeSAI.hxx"

extern GBDATA *gb_main;

static bool clrDefinitionsChanged       = false;
static bool inCallback                  = false; // used to avoid multiple refreshs
static bool in_colorDefChanged_callback = false; // used to avoid colorDef correction

#define BUFSIZE 100
static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];

    strcpy(buf, AWAR_SAI_CLR);
    (strchr(buf, 0)-1)[0] = '0'+awarNo;

    return buf;
}

static const char *getClrDefAwar(const char *awarName) {
    static char buf[BUFSIZE];

    e4_assert(awarName);
    e4_assert(awarName[0]); // empty awar is bad

#ifdef DEBUG
    int size =
#endif
    sprintf(buf,AWAR_SAI_CLR_DEF "%s", awarName);
    e4_assert(size<BUFSIZE);
    return buf;
}
#undef BUFSIZE

/* --------------------------------------------------------- */

static void setVisualizeSAI_cb(AW_root *awr) {
    ED4_ROOT->visualizeSAI = awr->awar(AWAR_SAI_ENABLE)->read_int();
    ED4_ROOT->refresh_all_windows(1); //refresh editor
}

static void setVisualizeSAI_options_cb(AW_root *awr) {
    ED4_ROOT->visualizeSAI_allSpecies = awr->awar(AWAR_SAI_ALL_SPECIES)->read_int();
    ED4_ROOT->refresh_all_windows(1); //refresh editor
}

static bool colorTransTable_exists(AW_root *awr, const char *name) {
    char       *tableNames = awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->read_string();
    const char *searchFrom = tableNames;
    int         len        = strlen(name);

    while (searchFrom) {
        const char *found = strstr(searchFrom, name);

        if (found) {
            if ((found == tableNames || found[-1] == '\n') && // found start of entry
                (found[len] == '\n' || found[len] == 0)) // avoid partial entry
            {
                break; // exists!
            }
            else {              // search next match
                searchFrom = found+1;
            }
        }
        else {
            searchFrom = 0;
        }
    }

    free(tableNames);
    return searchFrom != 0;
}

static void colorDefChanged_callback(AW_root *awr, AW_CL cl_awarNo) {
    clrDefinitionsChanged = true;

    if (!in_colorDefChanged_callback) { // this callback is special, because it may change all other color defs
        in_colorDefChanged_callback = true;

        bool old_inCallback = inCallback;
        inCallback          = true;

        char *clrTabName = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
        if (clrTabName[0]) {
            unsigned char charUsed[256]; memset(charUsed,255,256);

            {
                for (int i=0; i<10 ; i++){
                    char *awarString_next = awr->awar_string(getAwarName(i))->read_string();
                    for(int c=0; awarString_next[c]; ++c){
                        charUsed[(unsigned char)awarString_next[c]] = i;
                    }
                    free(awarString_next);
                }

                int   awarNo     = (int)cl_awarNo;
                char *awarString = awr->awar_string(getAwarName(awarNo))->read_string();
                for(int c=0; awarString[c]; ++c){
                    charUsed[(unsigned char)awarString[c]] = awarNo;
                }
                free(awarString);
            }

            typedef unsigned char mystr[256];
            mystr s[10];
            for (int i=0; i<10; i++)  s[i][0]=0; //initializing the strings

            for (int i=0; i<256; i++) {
                int table = charUsed[i];
                if (table != 255) {
                    char *eos = strchr((char *)s[table],0); // get pointer to end of string
                    eos[0] = char(i);
                    eos[1] = 0;
                }
            }

            {
                void *clrDefStr = GBS_stropen(500);            /* create output stream */
                for(int i=0; i<10; i++){
                    awr->awar_string(getAwarName(i))->write_string((char *)s[i]);

                    char *escaped = GBS_escape_string((char*)s[i], ";", '&');
                    GBS_strcat(clrDefStr, escaped);
                    free(escaped);
                    GBS_chrcat(clrDefStr, ';');
                }

                char    *colorDef = GBS_strclose(clrDefStr);
                AW_awar *awar_def = awr->awar_string(getClrDefAwar(clrTabName), "", AW_ROOT_DEFAULT);
                awar_def->write_string(colorDef); // writing clr defnition to clr trans table awar
                free(colorDef);
            }
        }
        else {
            if (!old_inCallback) { // only warn when user really changed the setting
                aw_message("Please select a VALID Color Translation Table to EDIT.");
            }
        }
        free(clrTabName);
        inCallback = old_inCallback;

        if (!inCallback) {
            ED4_ROOT->refresh_all_windows(1); //refresh editor
        }

        in_colorDefChanged_callback = false;
    }
}

static void colorDefTabNameChanged_callback(AW_root *awr) {
    char *clrTabName     = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
    bool  old_inCallback = inCallback;
    inCallback           = true;
    {
        bool old_in_colorDefChanged_callback = in_colorDefChanged_callback;
        in_colorDefChanged_callback          = true; // avoid correction

        // clear current translation table definition
        for(int i=0; i<10 ; i++) {
            AW_awar *transDef_awar = awr->awar_string(getAwarName(i), "", AW_ROOT_DEFAULT);
            transDef_awar->write_string("");
        }

        if (clrTabName[0]) {
            AW_awar *clrTabDef_awar = awr->awar_string(getClrDefAwar(clrTabName), "", AW_ROOT_DEFAULT);
            char    *clrTabDef      = clrTabDef_awar->read_string();

            if (clrTabDef[0]) {
                int i        = 0;
                int tokStart = 0;

                for (int si = 0; clrTabDef[si]; ++si) {
                    if (clrTabDef[si] == ';') {
                        e4_assert(i >= 0 && i<10);
                        AW_awar *awar = awr->awar(getAwarName(i));

                        if (tokStart == si) { // empty definition
                            awar->write_string("");
                        }
                        else {
                            int toklen = si-tokStart;

                            e4_assert(toklen > 0);
                            e4_assert(clrTabDef[tokStart+toklen] == ';');
                            clrTabDef[tokStart+toklen] = 0;

                            char *unescaped = GBS_unescape_string(clrTabDef+tokStart, ";", '&');
                            awar->write_string(unescaped);
                            free(unescaped);

                            clrTabDef[tokStart+toklen] = ';';
                        }
                        ++i;
                        tokStart = si+1;
                    }
                }
                e4_assert(i == 10);
            }
            free(clrTabDef);
        }
        in_colorDefChanged_callback = old_in_colorDefChanged_callback;
        colorDefChanged_callback(awr, 0); // correct first def manually
    }
    {
        // store the selected tabel as default for this SAI:
        char *saiName = awr->awar(AWAR_SAI_SELECT)->read_string();
        if (saiName[0]) {
            char buf[100];
            sprintf(buf, AWAR_SAI_CLR_TRANS_TAB_REL "%s", saiName);
            awr->awar_string(buf, "", AW_ROOT_DEFAULT); // create an AWAR for the selected SAI and
            awr->awar(buf)->write_string(clrTabName); // write the existing clr trans table names to the same
        }
        free(saiName);
    }
    inCallback = old_inCallback;
    free(clrTabName);

    if (!inCallback && clrDefinitionsChanged) {
        ED4_ROOT->refresh_all_windows(1); //refresh editor
    }
}

static void refresh_display_cb(GBDATA *, int *, GB_CB_TYPE cb_type) {
    if ((cb_type & GB_CB_CHANGED) &&
        ED4_ROOT->aw_root->awar(AWAR_SAI_ENABLE)->read_int())
    {
        clrDefinitionsChanged = 1;
        ED4_ROOT->refresh_all_windows(1); //refresh editor when current SAI is changed
    }
}

static void saiChanged_callback(AW_root *awr) {
    bool old_inCallback = inCallback;
    inCallback          = true;
    char *saiName       = 0;
    {
        static GBDATA *gb_last_SAI = 0;

        if (gb_last_SAI) {
            GB_transaction dummy(gb_main);
            GB_remove_callback(gb_last_SAI, GB_CB_CHANGED, refresh_display_cb, 0);
            gb_last_SAI = 0;
        }

        saiName = awr->awar(AWAR_SAI_SELECT)->read_string();
        char *transTabName = 0;

        if (saiName[0]) {
            char  buf[100];
            sprintf(buf, AWAR_SAI_CLR_TRANS_TAB_REL "%s", saiName);
            awr->awar_string(buf, "", AW_ROOT_DEFAULT);
            transTabName = awr->awar(buf)->read_string();
        }

        {
            GB_transaction dummy(gb_main);
            gb_last_SAI = GBT_find_SAI(gb_main, saiName);
            if (gb_last_SAI) {
                GB_add_callback(gb_last_SAI, GB_CB_CHANGED, refresh_display_cb, 0);
            }
        }
        awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->write_string(transTabName ? transTabName : "");
        free(transTabName);

        clrDefinitionsChanged = true; // SAI changed -> update needed
    }
    inCallback = old_inCallback;

    if (!inCallback && clrDefinitionsChanged) {
        ED4_ROOT->refresh_all_windows(1); //refresh editor
        // SAI changed notify Global SAI Awar AWAR_SAI_GLOBAL
        awr->awar(AWAR_SAI_GLOBAL)->write_string(saiName); 
    }
    free(saiName);
}

static void update_ClrTransTabNamesList_cb(AW_root *awr, AW_CL cl_aws, AW_CL cl_id) {
    AW_window         *aws              = (AW_window*)cl_aws;
    AW_selection_list *id               = (AW_selection_list*)cl_id;
    char              *clrTransTabNames = awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->read_string();

    aws->clear_selection_list(id);

    for (char *tok = strtok(clrTransTabNames,"\n"); tok; tok = strtok(0,"\n")) {
        aws->insert_selection(id,tok,tok);
    }
    aws->insert_default_selection(id, "????", "");
    aws->update_selection_list(id);

    free(clrTransTabNames);
}

static void autoselect_cb(AW_root *aw_root) {
    char *curr_sai = aw_root->awar(AWAR_SAI_NAME)->read_string();
#if defined(DEBUG)
    printf("curr_sai='%s'\n", curr_sai);
#endif // DEBUG
    aw_root->awar(AWAR_SAI_SELECT)->write_string(curr_sai);
    free(curr_sai);
}

static void set_autoselect_cb(AW_root *aw_root) {
    static bool callback_active = false;

    if (aw_root->awar(AWAR_SAI_AUTO_SELECT)->read_int()) { // auto select is avtivated
        aw_root->awar(AWAR_SAI_NAME)->add_callback(autoselect_cb);
        callback_active = true;
    }
    else {
        if (callback_active) { // only remove if added
            aw_root->awar(AWAR_SAI_NAME)->remove_callback(autoselect_cb);
            callback_active = false;
        }
    }
}

static void addNewTransTable(AW_root *aw_root, const char *newClrTransTabName, const char *defaultDefinition, bool autoselect) {
    AW_awar *table_def_awar = aw_root->awar_string(getClrDefAwar(newClrTransTabName), defaultDefinition , AW_ROOT_DEFAULT);
    table_def_awar->write_string(defaultDefinition);

    e4_assert(!colorTransTable_exists(aw_root, newClrTransTabName));

    AW_awar *names_awar = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
    char    *old_names  = names_awar->read_string();
    names_awar->write_string(old_names[0]
                             ? GBS_global_string("%s\n%s", old_names, newClrTransTabName)
                             : newClrTransTabName); // add new name
    free(old_names);

    if (autoselect) {
        aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->write_string(newClrTransTabName); // select new
    }
}

static void addDefaultTransTable(AW_root *aw_root, const char *newClrTransTabName, const char *defaultDefinition) {
    if (!colorTransTable_exists(aw_root, newClrTransTabName)) {
        addNewTransTable(aw_root, newClrTransTabName, defaultDefinition, false);
    }
}

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def) {  // --- Creating and initializing AWARS -----
    aw_root->awar_int   (AWAR_SAI_ENABLE,                 0,  aw_def);
    aw_root->awar_int   (AWAR_SAI_ALL_SPECIES,            0,  aw_def);
    aw_root->awar_int   (AWAR_SAI_AUTO_SELECT,            0,  aw_def);
    aw_root->awar_string(AWAR_SAI_SELECT,                 "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TABLE,        "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME, "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TAB_NAMES,    "", aw_def);

    for (int i=0;i<10;i++){   // initialising 10 color definition string AWARS
       AW_awar *def_awar = aw_root->awar_string(getAwarName(i),"",aw_def);
       def_awar->add_callback(colorDefChanged_callback, (AW_CL)i);
    }
    aw_root->awar(AWAR_SAI_ENABLE)->add_callback(setVisualizeSAI_cb);
    aw_root->awar(AWAR_SAI_ALL_SPECIES)->add_callback(setVisualizeSAI_options_cb);
    aw_root->awar(AWAR_SAI_AUTO_SELECT)->add_callback(set_autoselect_cb);
    aw_root->awar(AWAR_SAI_SELECT)->add_callback(saiChanged_callback);
    aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->add_callback(colorDefTabNameChanged_callback);

    ED4_ROOT->visualizeSAI            = aw_root->awar(AWAR_SAI_ENABLE)->read_int();
    ED4_ROOT->visualizeSAI_allSpecies = aw_root->awar(AWAR_SAI_ALL_SPECIES)->read_int();

    // create some defaults:
    AW_awar *awar_defaults_created = aw_root->awar_int(AWAR_SAI_CLR_DEFAULTS_CREATED, 0,  aw_def);

    if (awar_defaults_created->read_int() == 0) {
        addDefaultTransTable(aw_root, "numeric", "0;1;2;3;4;5;6;7;8;9;");
        addDefaultTransTable(aw_root, "binary", ".;+;;;;;;;;;");
        addDefaultTransTable(aw_root, "xstring", ";x;;;;;;;;;");
        addDefaultTransTable(aw_root, "consensus", "=ACGTU;;acgtu;.;;;;;;;");

        awar_defaults_created->write_int(1);
    }

    inCallback = true;          // avoid refresh
    saiChanged_callback(aw_root);
    colorDefTabNameChanged_callback(aw_root); // init awars for selected table
    set_autoselect_cb(aw_root);
    inCallback = false;
}

static void createCopyClrTransTable(AW_window *aws, AW_CL cl_mode) {
    // mode = ED4_VIS_COPY   copies the selected color translation table
    // mode = ED4_VIS_CREATE creates a new (empty) color translation table

    int      mode               = (int)cl_mode;
    AW_root *aw_root            = aws->get_root();
    char    *newClrTransTabName = 0;
    char    *clrTabSourceName   = 0;

    switch(mode){
    case ED4_VIS_CREATE:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME)->read_string());

        if (strcmp(newClrTransTabName, "__") == 0) { // user entered nothing
            aw_message("Please enter a translation table name");
        }
        else if (colorTransTable_exists(aw_root, newClrTransTabName)) {
            aw_message(GBS_global_string("Color translation table '%s' already exists.", newClrTransTabName));
        }
        else {
            addNewTransTable(aw_root, newClrTransTabName, "", true);
        }
        break;

    case ED4_VIS_COPY:
        newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME)->read_string());
        clrTabSourceName   = aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();

        if (!clrTabSourceName[0]) {
            aw_message("Please select a valid Color Translation Table to COPY!");
        }
        else if (colorTransTable_exists(aw_root, newClrTransTabName)) {
            aw_message(GBS_global_string("Color Translation Table \"%s\" EXISTS! Please enter a different name.", newClrTransTabName));
        }
        else {
            char *old_def = aw_root->awar(getClrDefAwar(clrTabSourceName))->read_string();
            addNewTransTable(aw_root, newClrTransTabName, old_def, true);
            free(old_def);
        }
        break;

    default:
        break;
    }
    free(clrTabSourceName);
    free(newClrTransTabName);
}

static void deleteColorTranslationTable(AW_window *aws){
    AW_root *aw_root = aws->get_root();

    int answer = aw_message("Are you sure you want to delete the selected COLOR TRANSLATION TABLE?","OK,CANCEL");
    if (answer) return;

    char *clrTabName = aw_root->awar_string(AWAR_SAI_CLR_TRANS_TABLE)->read_string();

    if (clrTabName[0]) {
        AW_awar *awar_tabNames    = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
        char    *clrTransTabNames = awar_tabNames->read_string();
        void    *newTransTabName  = GBS_stropen(strlen(clrTransTabNames));

        for (const char *tok = strtok(clrTransTabNames,"\n"); tok; tok = strtok(0,"\n")) {
            if (strcmp(clrTabName, tok) != 0) { // merge all not to delete
                GBS_strcat(newTransTabName, tok);
                GBS_strcat(newTransTabName, "\n");
            }
        }

        aw_root->awar_string(getClrDefAwar(clrTabName))->write_string("");
        char *new_name = GBS_strclose(newTransTabName);
        awar_tabNames->write_string(new_name); // updates selection list
        free(new_name);

        free(clrTransTabNames);
    }
    else {
        aw_message("Selected Color Translation Table is not VALID and cannot be DELETED!");
    }
    free(clrTabName);
}

static AW_selection_list *buildClrTransTabNamesList(AW_window *aws) {
    AW_root           *awr = aws->get_root();
    AW_selection_list *id  = aws->create_selection_list(AWAR_SAI_CLR_TRANS_TABLE);

    update_ClrTransTabNamesList_cb(awr, (AW_CL)aws, (AW_CL)id);

    return id;
}

const char *getSaiColorString(AW_root *awr, int start, int end) {
    static int   seqBufferSize = 0;
    static char *saiColors     = 0;
    static int   lastStart     = -1;
    static int   lastEnd       = -1;
    static bool  lastVisualize = false;

    e4_assert(start<=end);

    if(lastStart==start && lastEnd==end  && !clrDefinitionsChanged && lastVisualize) {
        return saiColors-start;    //if start and end positions are same as the previous positions and no settings
    }                              //were changed return the last calculated saiColors string

    lastStart = start; lastEnd = end; clrDefinitionsChanged = false; // storing start and end positions

    int seqSize = end-start+1;

    if(seqSize>seqBufferSize){
        free(saiColors);
        seqBufferSize = seqSize;
        saiColors =  (char*)GB_calloc(seqBufferSize, sizeof(char));
    }
    else memset(saiColors,0,sizeof(char)*seqSize);

    char *saiSelected = awr->awar(AWAR_SAI_SELECT)->read_string();

    GB_push_transaction(gb_main);
    char   *alignment_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_extended    = GBT_find_SAI(gb_main, saiSelected);
    bool    visualize      = false; // set to true if all goes fine

    if (gb_extended) {
        GBDATA *gb_ali = GB_find(gb_extended, alignment_name, 0, down_level);
        if (gb_ali) {
            const char *saiData      = 0;
            bool        free_saiData = false;

            {
                GBDATA *saiSequence = GB_find(gb_ali, "data", 0, down_level); // search "data" entry (normal SAI)
                if (saiSequence) {
                    saiData = GB_read_char_pntr(saiSequence); // not allocated
                }
                else {
                    saiSequence = GB_find(gb_ali, "bits", 0, down_level); // search "bits" entry (binary SAI)
                    if (saiSequence) {
                        saiData      = GB_read_as_string(saiSequence); // allocated
                        free_saiData = true; // free saiData below
                    }
                }
            }

            if (saiData) {
                char trans_table[256];
                {
                    // build the translation table:
                    memset(trans_table, 0, 256);
                    for (int i = 0; i<AWAR_SAI_CLR_COUNT; ++i) {
                        char *def      = awr->awar(getAwarName(i))->read_string();
                        int   clrRange = ED4_G_CBACK_0 + i;

                        for (char *d = def; *d; ++d) {
                            trans_table[(unsigned char)*d] = clrRange;
                        }
                        free(def);
                    }
                }

                // translate SAI to colors
                for (int i = start; i <= end; ++i) {
                    saiColors[i-start] = trans_table[(unsigned char)saiData[i]];
                }

                visualize = true;
            }

            if (free_saiData) {
                free(const_cast<char*>(saiData)); // in this case saiData is allocated (see above)
            }
        }
    }
    free(alignment_name);
    free(saiSelected);
    GB_pop_transaction(gb_main);

    lastVisualize = visualize;
    if (visualize) {
        e4_assert(saiColors);
        return saiColors-start;
    }

    return 0; // don't visualize (sth went wrong)
}


/* -------------------- Creating Windows and Display dialogs -------------------- */

static AW_window *create_copyColorTranslationTable_window(AW_root *aw_root){  // creates copy color tranlation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "COPY_CLR_TR_TABLE", "Copy Color Translation Table");
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the new name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME,15);

    aws->at("ok");
    aws->callback(createCopyClrTransTable, (AW_CL)ED4_VIS_COPY);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

static AW_window *create_createColorTranslationTable_window(AW_root *aw_root){ // creates create color tranlation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "CREATE_CLR_TR_TABLE", "Create Color Translation Table");
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME,15);

    aws->at("ok");
    aws->callback(createCopyClrTransTable, (AW_CL)ED4_VIS_CREATE);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

static void reverseColorTranslationTable(AW_window *aww) {
    AW_root *aw_root = aww->get_root();

    int j = AWAR_SAI_CLR_COUNT-1;
    for (int i = 0; i<AWAR_SAI_CLR_COUNT/2+1; ++i, --j) {
        if (i<j) {
            AW_awar *aw_i = aw_root->awar(getAwarName(i));
            AW_awar *aw_j = aw_root->awar(getAwarName(j));

            char *ci = aw_i->read_string();
            char *cj = aw_j->read_string();

            aw_i->write_string(cj);
            aw_j->write_string(ci);

            free(ci);
            free(cj);
        }
    }
}

static AW_window *create_editColorTranslationTable_window(AW_root *aw_root){  // creates edit color tranlation table window
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "EDIT_CTT", "Color Translation Table");
    aws->load_xfig("saiColorRange.fig");

    char at_name[] = "rangex";
    char *dig      = strchr(at_name, 0)-1;

    for (int i = 0; i<AWAR_SAI_CLR_COUNT; ++i) {
        dig[0] = '0'+i;
        aws->at(at_name);
        aws->create_input_field(getAwarName(i), 20);
    }

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("reverse");
    aws->callback(reverseColorTranslationTable);
    aws->create_button("REVERSE", "Reverse", "R");

    aws->at("colors");
    aws->callback(AW_POPUP,(AW_CL)AW_create_gc_window, (AW_CL)ED4_ROOT->aw_gc_manager);
    aws->button_length(0);
    aws->create_button("COLORS","#colors.xpm");

    return (AW_window *)aws;
}

static AW_window *ED4_openSelectSAI_window(AW_root *aw_root){
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_SAI", "SELECT SAI");
    aws->load_xfig("selectSAI.fig");

    aws->at("selection");
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_extendeds(gb_main,(AW_window *)aws,AWAR_SAI_SELECT);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

AW_window *ED4_createVisualizeSAI_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "VISUALIZE_SAI", "VISUALIZE SAI");
    aws->load_xfig("visualizeSAI.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"visualizeSAI.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("enable");
    aws->create_toggle(AWAR_SAI_ENABLE);

    aws->at("sai");
    aws->callback(AW_POPUP,(AW_CL)ED4_openSelectSAI_window,(AW_CL)0);
    aws->button_length(25);
    aws->create_button("SELECT_SAI", AWAR_SAI_SELECT);

    aws->at("auto_select");
    aws->create_toggle(AWAR_SAI_AUTO_SELECT);

    aws->at("clrTrList");
    AW_selection_list *clrTransTableLst = buildClrTransTabNamesList(aws);

    aws->at("edit");
    aws->button_length(10);
    aws->callback(AW_POPUP,(AW_CL)create_editColorTranslationTable_window,0);
    aws->create_button("EDIT","EDIT");

    aws->at("create");
    aws->callback(AW_POPUP,(AW_CL)create_createColorTranslationTable_window,0);
    aws->create_button("CREATE","CREATE");

    aws->at("copy");
    aws->callback(AW_POPUP,(AW_CL)create_copyColorTranslationTable_window,0);
    aws->create_button("COPY","COPY");

    aws->at("delete");
    aws->callback((AW_CB1)deleteColorTranslationTable,0);
    aws->create_button("DELETE","DELETE");

    aws->at("marked");
    aws->create_toggle_field(AWAR_SAI_ALL_SPECIES,1);
    aws->insert_toggle("MARKED SPECIES", "M", 0);
    aws->insert_toggle("ALL SPECIES", "A", 1);
    aws->update_toggle_field();

    AW_awar *trans_tabs = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
    trans_tabs->add_callback(update_ClrTransTabNamesList_cb, (AW_CL)aws, (AW_CL)clrTransTableLst);
    trans_tabs->touch();        // force update

    aws->show();

    return (AW_window *)aws;
}

