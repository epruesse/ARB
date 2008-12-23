#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>
#include "awt_nds.hxx"
#include "awt_config_manager.hxx"
#include "awt_changekey.hxx"
#include "awt_sel_boxes.hxx"

#define NDS_PER_PAGE 10         // number of NDS definitions on each config-page
#define NDS_PAGES     6         // how many config-pages (each has NDS_PER_PAGE definitions)

#define NDS_COUNT (NDS_PER_PAGE*NDS_PAGES)    // overall number of NDS definitions

#define AWAR_NDS_PAGE "tmp/viewkeys/page"

#if defined(DEBUG)
#define NDS_STRING_SIZE 200
#else
#define NDS_STRING_SIZE 4000
#endif // DEBUG

struct make_node_text_struct {
    char  buf[NDS_STRING_SIZE]; // buffer used to generate output
    char *bp;
    int   space_left;

    long count;
    int  show_errors;           // how many errors to show

    long  lengths[NDS_COUNT];   // length of generated string
    char *dkeys[NDS_COUNT];     // database field name (may be empty)
    bool  rek[NDS_COUNT];       // 1->key is hierarchical (e.g. 'ali_16s/data')
    char *parsing[NDS_COUNT];   // ACI/SRT program
    bool  at_group[NDS_COUNT];  // whether string shall appear at group NDS entries
    bool  at_leaf[NDS_COUNT];   // whether string shall appear at leaf NDS entries

    void init_buffer() {
        bp         = buf;
        space_left = NDS_STRING_SIZE-1;
    }
    char *get_buffer() {
        bp[0] = 0;
        return buf;
    }

    void insert_overflow_warning() {
        awt_assert(space_left >= 0); // <0 means 'already warned'
        while (space_left) {
            *bp++ = ' ';
            space_left--;
        }

        static const char *warning  = "..<truncated>";
        static int         warn_len = 0;
        if (!warn_len) warn_len = strlen(warning);

        strcpy(buf+(NDS_STRING_SIZE-warn_len-1), warning);
        space_left = -1;
    }

    void append(char c) {
        if (space_left >= 1) {
            *bp++ = c;
            space_left--;
        }
        else if (!space_left) {
            insert_overflow_warning();
        }
    }
    void append(const char *str, int length = -1) {
        awt_assert(str);
        if (length == -1) length = strlen(str);
        awt_assert(int(strlen(str)) == length);

        if (space_left >= length) {
            strcpy(bp, str);
            bp         += length;
            space_left -= length;
        }
        else { // space_left < length
            if (space_left >= 0) {
                if (space_left>0) {
                    memcpy(bp, str, space_left);
                    bp         += space_left;
                    space_left  = 0;
                }
                insert_overflow_warning();
            }
        }
    }

} *awt_nds_ms = 0;

inline const char *viewkeyAwarName(int i, const char *name) {
    awt_assert(i >= 0 && i < NDS_PER_PAGE);
    return GBS_global_string("tmp/viewkeys/viewkey_%i/%s", i, name);
}

inline AW_awar *viewkeyAwar(AW_root *aw_root,AW_default awdef,int i, const char *name, bool string_awar) {
    const char *awar_name = viewkeyAwarName(i, name);
    AW_awar    *awar      = 0;
    if (string_awar) awar = aw_root->awar_string(awar_name, "", awdef);
    else        awar      = aw_root->awar_int(awar_name, 0, awdef);
    return awar;
}

static void map_viewkey(AW_root *aw_root, AW_default awdef, int i, GBDATA *gb_viewkey)  {
    // maps one NDS key data to one line of the config window

    GBDATA *gb_key_text = GB_entry(gb_viewkey, "key_text");
    GBDATA *gb_pars     = GB_entry(gb_viewkey, "pars");
    GBDATA *gb_len1     = GB_entry(gb_viewkey, "len1");
    GBDATA *gb_group    = GB_entry(gb_viewkey, "group");
    GBDATA *gb_leaf     = GB_entry(gb_viewkey, "leaf");

    awt_assert(gb_key_text);
    awt_assert(gb_pars);
    awt_assert(gb_len1);
    awt_assert(gb_group);
    awt_assert(gb_leaf);

    AW_awar *Awar;
    Awar = viewkeyAwar(aw_root, awdef, i, "key_text", true ); Awar->map((void*)gb_key_text);
    Awar = viewkeyAwar(aw_root, awdef, i, "pars",     true ); Awar->map((void*)gb_pars    );
    Awar = viewkeyAwar(aw_root, awdef, i, "len1",     false); Awar->map((void*)gb_len1    );
    Awar = viewkeyAwar(aw_root, awdef, i, "group",    false); Awar->map((void*)gb_group   );
    Awar = viewkeyAwar(aw_root, awdef, i, "leaf",     false); Awar->map((void*)gb_leaf    );
}

static void map_viewkeys(AW_root *aw_root, AW_CL cl_awdef, AW_CL cl_gb_main) {
    // map visible viewkeys to internal db entries
    static bool  initialized        = false;
    AW_default   awdef              = (AW_default)cl_awdef;
    GBDATA      *gb_main            = (GBDATA*)cl_gb_main;
    AW_awar     *awar_selected_page = 0;

    if (!initialized) {
        awar_selected_page = aw_root->awar_int(AWAR_NDS_PAGE, 0);
        awar_selected_page->add_callback(map_viewkeys, cl_awdef, cl_gb_main); // bind to self
        initialized        = true;
    }
    else {
        awar_selected_page = aw_root->awar(AWAR_NDS_PAGE);
    }

    int page = awar_selected_page->read_int();
#if defined(DEBUG)
    printf("map_viewkeys to page %i\n", page);
#endif // DEBUG
    if (page<NDS_PAGES) {
        GB_transaction ta(gb_main);

        GBDATA *gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
        GBDATA *gb_viewkey     = 0;
        
        int i1 = page*NDS_PER_PAGE;
        int i2 = i1+NDS_PER_PAGE-1;

        for (int i = 0; i <= i2; i++) {
            gb_viewkey = !gb_viewkey ? GB_entry(gb_arb_presets,"viewkey") : GB_nextEntry(gb_viewkey);
            awt_assert(i<NDS_COUNT);
            awt_assert(gb_viewkey);
            if (i >= i1) map_viewkey(aw_root, awdef, i-i1, gb_viewkey);
        }
    }
}

void create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main) {
    GB_push_transaction(gb_main);

    GBDATA *gb_viewkey     = 0;
    GBDATA *gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);

    for (int i = 0; i<NDS_COUNT; ++i) {
        gb_viewkey = !gb_viewkey ? GB_entry(gb_arb_presets,"viewkey") : GB_nextEntry(gb_viewkey);

        if (!gb_viewkey) gb_viewkey = GB_create_container(gb_arb_presets,"viewkey");

        {
            int  group             = 0;
            int  leaf              = 0;
            bool was_group_name    = false;
            int  default_len       = 30;

            GBDATA *gb_key_text = GB_entry(gb_viewkey, "key_text");
            if (!gb_key_text) {
                gb_key_text        = GB_create(gb_viewkey,"key_text",GB_STRING);
                const char *wanted = "";
                switch(i){
                    case 0: wanted = "name"; default_len = 12; leaf = 1; break;
                    case 1: wanted = "full_name"; leaf = 1; break;
                    case 2: wanted = ""; was_group_name = true; break;
                    case 3: wanted = "acc"; default_len = 20; leaf = 1; break;
                    case 4: wanted = "date"; break;
                }
                GB_write_string(gb_key_text, wanted);
            }

            if (strcmp(GB_read_char_pntr(gb_key_text), "group_name") == 0) {
                GB_write_string(gb_key_text, "");
                was_group_name = true; // means: change group/leaf + add 'taxonomy(1)' to ACI
            }

            GBDATA *gb_len1  = GB_entry(gb_viewkey, "len1");
            GBDATA *gb_pars  = GB_entry(gb_viewkey, "pars");

            if (!gb_len1)  { gb_len1 = GB_create(gb_viewkey, "len1",  GB_INT   ); GB_write_int(gb_len1, default_len); }
            if (!gb_pars ) { gb_pars  = GB_create(gb_viewkey, "pars",  GB_STRING); GB_write_string(gb_pars, ""); }

            if (was_group_name) {
                group = 1;
                leaf  = 0;

                const char *pars = GB_read_char_pntr(gb_pars);

                if (pars[0] == 0) pars        = "taxonomy(1)"; // empty ACI/SRT
                else if (pars[0] == ':') pars = GBS_global_string("taxonomy(1)|%s", pars); // was an SRT -> unsure what to do
                else if (pars[0] == '|') pars = GBS_global_string("taxonomy(1)%s", pars); // was an ACI -> prefix taxonomy
                else pars                     = GBS_global_string("taxonomy(1)|%s", pars); // other ACIs -> same

                GB_write_string(gb_pars, pars);
            }

            {
                GBDATA *gb_flag1 = GB_entry(gb_viewkey, "flag1");
                if (gb_flag1) {
                    if (GB_read_int(gb_flag1)) { // obsolete
                        leaf = 1;
                    }
                    GB_ERROR error = GB_delete(gb_flag1);
                    if (error) aw_message(error);
                }
            }

            {
                GBDATA *gb_inherit = GB_entry(gb_viewkey, "inherit");
                if (gb_inherit) { // 'inherit' is old NDS style -> convert & delete
                    if (was_group_name && GB_read_int(gb_inherit)) leaf = 1;
                    GB_ERROR error = GB_delete(gb_inherit);
                    if (error) aw_message(error);
                }
            }

            GBDATA *gb_group = GB_entry(gb_viewkey, "group");
            GBDATA *gb_leaf  = GB_entry(gb_viewkey, "leaf");
            if (!gb_group) { gb_group = GB_create(gb_viewkey, "group", GB_INT); GB_write_int(gb_group, group ); }
            if (!gb_leaf)  { gb_leaf = GB_create(gb_viewkey, "leaf",  GB_INT); GB_write_int(gb_leaf,  leaf ); }
        }
    }

    aw_root->awar_string("tmp/viewkeys/key_text_select","",awdef);
    GB_pop_transaction(gb_main);

    map_viewkeys(aw_root, (AW_CL)awdef, (AW_CL)gb_main); // call once
}

void awt_pre_to_view(AW_root *aw_root){
    char *str = aw_root->awar(AWAR_SELECT_ACISRT_PRE)->read_string();
    char *brk = strchr(str,'#');
    if (brk) {
        *(brk++) = 0;
        aw_root->awar(AWAR_SELECT_ACISRT)->write_string(brk);
    }else{
        aw_root->awar(AWAR_SELECT_ACISRT)->write_string(str);
    }
    free(str);
}
void AWT_create_select_srtaci_window(AW_window *aww,AW_CL awar_acisrt,AW_CL awar_short)
{
    AWUSE(awar_short);
    static AW_window *win = 0;

    if (!win) {
        AW_root *aw_root = aww->get_root();
        aw_root->awar_string(AWAR_SELECT_ACISRT);
        aw_root->awar_string(AWAR_SELECT_ACISRT_PRE);
        aw_root->awar(AWAR_SELECT_ACISRT)->map((char *)awar_acisrt);
        
        AW_window_simple *aws = new AW_window_simple;
        aws->init( aw_root, "SRT_ACI_SELECT", "SRT_ACI_SELECT");
        aws->load_xfig("awt/srt_select.fig");
        aws->button_length(13);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        aws->callback( AW_POPUP_HELP,(AW_CL)"acisrt.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP","H");

        aws->at("box");
        AW_selection_list*  id = aws->create_selection_list(AWAR_SELECT_ACISRT_PRE);
        char *filename = AWT_unfold_path("lib/sellists/srt_aci*.sellst","ARBHOME");
        GB_ERROR error = aws->load_selection_list(id,filename);
        free(filename);
        if (error) aw_message(error);

        aws->at("field");
        aws->create_text_field(AWAR_SELECT_ACISRT);

        aw_root->awar(AWAR_SELECT_ACISRT_PRE)->add_callback(awt_pre_to_view);
        awt_pre_to_view(aw_root);
        win =  (AW_window*)aws;
    }
    win->show();
}

static void nds_init_config(AWT_config_definition& cdef) {
    for (int i = 0; i<NDS_PER_PAGE; ++i) {
        cdef.add(viewkeyAwarName(i, "leaf"), "leaf", i);
        cdef.add(viewkeyAwarName(i, "group"), "group", i);
        cdef.add(viewkeyAwarName(i, "key_text"), "key_text", i);
        cdef.add(viewkeyAwarName(i, "len1"), "len1", i);
        cdef.add(viewkeyAwarName(i, "pars"), "pars", i);
    }
}

static char *nds_store_config(AW_window *aww, AW_CL, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    nds_init_config(cdef);
    return cdef.read();
}

static void nds_restore_config(AW_window *aww, const char *stored, AW_CL, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    nds_init_config(cdef);

    AWT_config parsedCfg(stored);
    if (parsedCfg.has_entry("inherit0")) {
        aw_message("Converting stored config to new NDS format -- consider saving it again.");
        // Note: The conversion applied here is also done in create_nds_vars()

        GB_ERROR error = 0;

        for (int i = 0; !error && i<NDS_COUNT; ++i) {
            bool was_group_name = false;
            {
                const char *key_text_key = GBS_global_string("key_text%i", i);
                const char *key_text     = parsedCfg.get_entry(key_text_key);
                if (strcmp(key_text, "group_name") == 0) {
                    was_group_name = true;
                    parsedCfg.set_entry(key_text_key, "");
                }
            }

            bool leaf    = false;
            bool group   = false;
            int  inherit = 0;

            {
                const char *inherit_key   = GBS_global_string("inherit%i", i);
                const char *inherit_value = parsedCfg.get_entry(inherit_key);

                if (inherit_value) {
                    inherit = atoi(inherit_value);
                    parsedCfg.delete_entry(inherit_key);
                }
                else {
                    error = GB_export_error("Expected entry '%s' in saved config", inherit_key);
                }
            }

            if (was_group_name) {
                if (!error) {
                    leaf  = inherit;
                    group = true;

                    char       *aci_key = GBS_global_string_copy("pars%i", i);
                    const char *aci     = parsedCfg.get_entry(aci_key);
                    char       *new_aci = 0;

                    if      (aci[0] == 0)   { new_aci = strdup("taxonomy(1)"); }
                    else if (aci[0] == '|') { new_aci = GBS_global_string_copy("taxonomy(1)%s", aci); }
                    else                    { new_aci = GBS_global_string_copy("taxonomy(1)|%s", aci); }

                    parsedCfg.set_entry(aci_key, new_aci);

                    free(new_aci);
                    free(aci_key);
                }
            }
            else {
                leaf = true;
            }

            if (!error) {
                const char *flag1_key   = GBS_global_string("active%i", i);
                const char *flag1_value = parsedCfg.get_entry(flag1_key);
                if (flag1_value) {
                    int flag1 = atoi(flag1_value);
                    if (flag1 == 0) { leaf = group = false; }
                    parsedCfg.delete_entry(flag1_key);
                }
                else {
                    error = GB_export_error("Expected entry '%s' in saved config", flag1_key);
                }
            }

            if (!error) {
                const char *leaf_key  = GBS_global_string("leaf%i", i);
                parsedCfg.set_entry(leaf_key, GBS_global_string("%i", int(leaf)));
                const char *group_key = GBS_global_string("group%i", i);
                parsedCfg.set_entry(group_key, GBS_global_string("%i", int(group)));
            }
        }

        if (!error) {
            char *converted_cfg_str = parsedCfg.config_string();
            cdef.write(converted_cfg_str);
            free(converted_cfg_str);
        }
        else {
            aw_message(error);
        }
    }
    else {
        cdef.write(stored);
    }
}

AW_window *AWT_create_nds_window(AW_root *aw_root,AW_CL cgb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init( aw_root, "NDS_PROPS", "NDS");
        aws->load_xfig("awt/nds.fig");
        aws->auto_space(10,5);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP,(AW_CL)"props_nds.hlp");
        aws->create_button("HELP", "HELP","H");

        aws->at("page");
        aws->create_option_menu(AWAR_NDS_PAGE, "", "");
        for (int p = 0; p < NDS_PAGES; p++) {
            const char *text = GBS_global_string("Entries %i - %i", p*NDS_PER_PAGE+1, (p+1)*NDS_PER_PAGE);
            aws->insert_option(text, "", p);
        }
        aws->update_option_menu();

        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "nds", nds_store_config, nds_restore_config, 0, 0);

        // --------------------

        aws->button_length(13);
        int dummy,closey;
        aws->at_newline();
        aws->get_at_position( &dummy,&closey );

        aws->create_button(0,"K");

        aws->at_newline();

        int leafx, groupx, fieldselectx, fieldx, columnx, srtx, srtux;

        aws->auto_space(10,0);

        int i;
        // for (i=0;i<NDS_COUNT; i++) {
        for (i=0;i<NDS_PER_PAGE; i++) {
            aws->get_at_position( &leafx,&dummy );
            aws->create_toggle(viewkeyAwarName(i, "leaf"));

            aws->get_at_position( &groupx,&dummy );
            aws->create_toggle(viewkeyAwarName(i, "group"));

            {
                const char *awar_name = viewkeyAwarName(i, "key_text");

                aws->button_length(20);
                aws->get_at_position( &fieldx,&dummy );
                aws->create_input_field(awar_name,15);

                aws->button_length(0);
                aws->callback(AWT_popup_select_species_field_window, (AW_CL)strdup(awar_name), cgb_main);
                aws->get_at_position( &fieldselectx,&dummy );
                aws->create_button("SELECT_NDS","S");
            }

            aws->get_at_position( &columnx,&dummy );
            aws->create_input_field(viewkeyAwarName(i, "len1"),4);

            {
                const char *awar_name = viewkeyAwarName(i, "pars");

                aws->get_at_position( &srtx,&dummy );
                aws->button_length(0);
                aws->callback(AWT_create_select_srtaci_window,(AW_CL)strdup(awar_name),0);
                aws->create_button("SELECT_SRTACI", "S","S");

                aws->get_at_position( &srtux,&dummy );
                aws->at_set_to(AW_TRUE, AW_FALSE, -7, 30);
                aws->create_input_field(awar_name,40);
            }

            aws->at_unset_to();
            aws->at_newline();
        }

        aws->at(leafx,closey);

        aws->at_x(leafx);
        aws->create_button(0,"LEAF");
        aws->at_x(groupx);
        aws->create_button(0,"GRP.");

        aws->at_x(fieldx);
        aws->create_button(0,"FIELD");

        aws->at_x(fieldselectx);
        aws->create_button(0,"SEL");

        aws->at_x(columnx);
        aws->create_button(0,"WIDTH");

        aws->at_x(srtx);
        aws->create_button(0,"SRT");

        aws->at_x(srtux);
        aws->create_button(0,"ACI/SRT PROGRAM");
    }

    return aws;
}



void make_node_text_init(GBDATA *gb_main){
    GBDATA *gbz,*gbe;
    int     count;

    if (!awt_nds_ms) awt_nds_ms = (struct make_node_text_struct *) GB_calloc(sizeof(struct make_node_text_struct),1);

    GBDATA *gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    count                  = 0;

    for (gbz = GB_entry(gb_arb_presets, "viewkey"); gbz; gbz  = GB_nextEntry(gbz)) {
        /* toggle set ? */
        bool at_leaf = GB_read_int(GB_entry(gbz, "leaf"));
        bool at_group = GB_read_int(GB_entry(gbz, "group"));


        if (at_leaf || at_group) {
            if (awt_nds_ms->dkeys[count]) free(awt_nds_ms->dkeys[count]);
            awt_nds_ms->dkeys[count] = GB_read_string(GB_entry(gbz, "key_text"));

            awt_nds_ms->rek[count]      = (GB_first_non_key_char(awt_nds_ms->dkeys[count]) != 0);
            awt_nds_ms->lengths[count]  = GB_read_int(GB_entry(gbz, "len1"));
            awt_nds_ms->at_leaf[count]  = at_leaf;
            awt_nds_ms->at_group[count] = at_group;

            gbe = GB_entry(gbz, "pars");
            if (awt_nds_ms->parsing[count]) {
                free(awt_nds_ms->parsing[count]);
                awt_nds_ms->parsing[count] = 0;
            }
            if (gbe && GB_read_string_count(gbe)>1 ) awt_nds_ms->parsing[count] = GB_read_string(gbe);
            count++;
        }
    }
    awt_nds_ms->show_errors = 10;
    awt_nds_ms->count       = count;
}

enum { MNTN_COMPRESSED = 0, MNTN_SPACED = 1, MNTN_TABBED = 2 };

#if defined(DEBUG)
// #define QUOTE_NDS_STRING
#endif // DEBUG

const char *make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode, GBT_TREE *species, const char *tree_name)
{
    // mode == MNTN_COMPRESSED      compress info (no tabbing, seperate single fields by komma)
    // mode == MNTN_SPACED          format info (using spaces)
    // mode == MNTN_TABBED          format info (using 1 tab per column - for easy import into star-calc, excel, etc. )

    awt_nds_ms->init_buffer();

    if (!gbd) {
        if (!species) return "<internal error: no tree-node, no db-entry>";
        if (!species->name) return "<internal error: node w/o name>";
        sprintf(awt_nds_ms->buf,"<%s>",species->name); // zombie
        return awt_nds_ms->buf;
    }

#if defined(QUOTE_NDS_STRING)
    awt_nds_ms->append('\'');
#endif // QUOTE_NDS_STRING

    bool    field_was_printed = false;
    GB_BOOL is_leaf           = species ? species->is_leaf : GB_TRUE;

    for (int i = 0; i < awt_nds_ms->count; i++) {
        if (is_leaf) { if (!awt_nds_ms->at_leaf[i]) continue; }
        else         { if (!awt_nds_ms->at_group[i]) continue; }

        char *str        = 0;   // the generated string
        bool  apply_aci  = false; // whether aci shall be applied
        bool  align_left = true; // otherwise align right

        {
            const char *field_output = "";
            const char *field_name   = awt_nds_ms->dkeys[i];

            if (field_name[0] == 0) { // empty field_name -> only do ACI/SRT
                apply_aci = true;
            }
            else { // non-empty field_name
                GBDATA *gbe;
                if (awt_nds_ms->rek[i]) {       /* hierarchical key */
                    gbe = GB_search(gbd,awt_nds_ms->dkeys[i],0);
                }
                else {              /* flat entry */
                    gbe = GB_entry(gbd, awt_nds_ms->dkeys[i]);
                }
                // silently ignore missing fields (and leave apply_aci false!)
                if (gbe) {
                    apply_aci = true;
                    switch (GB_read_type(gbe)) {
                        case GB_INT: field_output  = GBS_global_string("%li", GB_read_int(gbe)); align_left = false; break;
                        case GB_BYTE: field_output = GBS_global_string("%i", GB_read_byte(gbe)); align_left = false; break;

                        case GB_FLOAT: {
                            const char *format = "%5.4f";
                            if (mode == MNTN_TABBED) { // '.' -> ','
                                char *dotted  = GBS_global_string_copy(format, GB_read_float(gbe));
                                char *dot     = strchr(dotted, '.');
                                if (dot) *dot = ',';
                                field_output  = GBS_global_string("%s", dotted);
                                free(dotted);
                            }
                            else {
                                field_output = GBS_global_string(format, GB_read_float(gbe));
                            }
                            align_left = false;
                            break;
                        }
                        case GB_STRING:
                            field_output = GB_read_char_pntr(gbe);
                            break;

                        default : {
                            char *as_string = GB_read_as_string(gbe);
                            field_output    = GBS_global_string("%s", as_string);
                            free(as_string);
                        }
                    }
                }
            }
            str = strdup(field_output);
        }

        // apply ACI/SRT program

        GB_ERROR error = 0;
        if (apply_aci) {
            const char *aci_srt = awt_nds_ms->parsing[i];
            if (aci_srt) {
                char *aci_result = GB_command_interpreter(gb_main, str, aci_srt, gbd, tree_name);
                if (aci_result) {
                    free(str);
                    str = aci_result;
                }
                else {          // error
                    error = GB_get_error();
                    free(str);
                    str   = GBS_global_string_copy("<error: %s>", error);
                }
            }
        }

        bool skip_display = (mode == MNTN_COMPRESSED && str[0] == 0);
        if (!skip_display) {
            switch (mode) {
                case MNTN_COMPRESSED:
                    if (!field_was_printed) break; // no komma no space if nothing printed yet
                    awt_nds_ms->append(','); // seperate single fields by komma in compressed mode
                    // fall-through
                case MNTN_SPACED:
                    awt_nds_ms->append(' '); // print at least one space if not using tabs
                    break;
                    
                case MNTN_TABBED:
                    if (i != 0) awt_nds_ms->append('\t'); // tabbed output for star-calc/excel/...
                    break;
                    
                default :
                    awt_assert(0);
                    break;
            }

            field_was_printed = true;

            int str_len = strlen(str);
            int nds_len = awt_nds_ms->lengths[i];
            if (str_len>nds_len) { // string is too long -> shorten
                str[nds_len] = 0;
                str_len      = nds_len;
            }

            if (mode == MNTN_SPACED) { // may need alignment
                const char *spaced = GBS_global_string((align_left ? "%-*s" : "%*s"), nds_len, str);
                awt_nds_ms->append(spaced, nds_len);
            }
            else {
                awt_nds_ms->append(str, str_len);
            }
        }

        // show first XXX errors
        if (error && awt_nds_ms->show_errors>0) {
            awt_nds_ms->show_errors--;
            aw_message(error);
        }

        free(str);
    }

#if defined(QUOTE_NDS_STRING)
    awt_nds_ms->append('\'');
#endif // QUOTE_NDS_STRING

    return awt_nds_ms->get_buffer();
}

char *make_node_text_list(GBDATA * gbd, FILE *fp)
{
    /* if mode==0 screen else file */
    char       *bp;
    const char *p;
    GBDATA     *gbe;
    long        i;
    long        cp;
    char        c = 0;
    char        fieldname[50];

    bp = awt_nds_ms->buf;
    if (!gbd) {
        *bp = 0;
        return awt_nds_ms->buf;
    }

    fprintf(fp, "\n------------------- %s\n", GBT_read_name(gbd));

    for (i = 0; i < awt_nds_ms->count; i++) {
        if (awt_nds_ms->rek[i]) {       /* hierarchical key */
            gbe = GB_search(gbd,awt_nds_ms->dkeys[i],0);
        }else{              /* flat entry */
            gbe = GB_entry(gbd, awt_nds_ms->dkeys[i]);
        }
        if (!gbe) continue;
        /*** get field info ***/
        switch (GB_read_type(gbe)) {
            case GB_INT:
                sprintf(bp, "%li", GB_read_int(gbe));
                break;
            case GB_STRING:
                p = GB_read_char_pntr(gbe);
                sprintf(bp,"%s", p);
                break;
            case GB_FLOAT:
                sprintf(bp, "%4.4f", GB_read_float(gbe));
                break;
            default:
                sprintf(bp,"'default:' make_node_text_list!");
                                 break;
        }/*switch*/

        /*** get fieldname ***/
        strcpy(fieldname, awt_nds_ms->dkeys[i]);

        /*** print fieldname+begin of line ***/
        cp = strlen (bp);
        if (cp>=60) {
            c = bp[60];
            bp[60] = 0;
        }
        fprintf(fp,"%18s: %s\n",fieldname+1, bp);
        if (cp>=60) bp[60] = c;

        while(cp > 60) {
            cp -= 60;
            bp += 60;
            if (cp>=60) {
                c = bp[60];
                bp[60] = 0;
            }
            fprintf(fp,"%18s  %s\n","", bp);
            if (cp>=60) bp[60] = c;
        }
    }
    *bp = 0;
    return awt_nds_ms->buf;
}
