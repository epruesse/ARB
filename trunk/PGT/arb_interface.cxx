// COPYRIGHT (C) 2004 - 2005 KAI BADER <BADERK@IN.TUM.DE>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// CVS REVISION TAG  --  $Revision$

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "arb_interface.hxx"
#include "xm_defs.hxx"

#define MAX_ENTRY_NAMES_COUNTER 10

/****************************************************************************
*  GLOBAL VARIABLES, ESSENTIAL FOR THE ARB CONNECTION
****************************************************************************/
GBDATA *global_gbData= NULL;
GBDATA *global_gbConfig= NULL;
int global_ARB_connections= 0;
bool global_ARB_lock= false;
bool global_ARB_available= false;
bool global_CONFIG_available= false;


/****************************************************************************
*  ESTABLISH A CONNECTION TO THE ARB DATABASE
****************************************************************************/
int ARB_connect(char *s)
{
    global_gbData= NULL; // ABFRAGE !!!
    global_ARB_connections= 0;
    global_ARB_lock= false;
    global_ARB_available= false;

    const char *server= ":";
    if(s)
    {
        server= s;
    }

    global_gbData= GB_open(server, "r");
    if (!global_gbData)
    {
        return 0;
    }

    global_ARB_available= true;

    return 1;
}


/****************************************************************************
*  CLOSE THE ARB DATABASE CONNECTION
****************************************************************************/
int ARB_disconnect()
{
    if(global_gbData) GB_close(global_gbData);

    global_ARB_available= false;
    global_gbData= NULL;

    return 1;
}


/****************************************************************************
*  ESTABLISH A CONNECTION TO THE ARB CONFIGURATION
****************************************************************************/
int CONFIG_connect()
{
    global_gbConfig= NULL; // ABFRAGE !!!
    global_CONFIG_available= false;

    // GET ARB HOME
    const char *home= GB_getenvHOME();
    char *buffer= (char *)malloc(1025 * sizeof(char));
    sprintf(buffer,"%s/%s", home, PGT_CONFIG_FILE);

    // OPEN CONFIG FILE
    global_gbConfig= GB_open(buffer, "rwc");
    if(!global_gbConfig)
    {
        free(buffer);
        return 0;
    }

    // DISABLE TRANSACTIONS
    GB_no_transaction(global_gbConfig);

    global_CONFIG_available= true;

    free(buffer);
    return 1;
}


/****************************************************************************
*  CLOSE THE ARB CONFIGURATION
****************************************************************************/
int CONFIG_disconnect()
{
    if(global_gbConfig)
    {
        // GET ARB HOME
        const char *home= GB_getenvHOME();
        char *buffer= (char *)malloc(1025 * sizeof(char));
        sprintf(buffer,"%s/%s", home, PGT_CONFIG_FILE);

        GB_save(global_gbConfig, buffer, "a");

        GB_close(global_gbConfig);

        free(buffer);
    }

    global_CONFIG_available= false;
    global_gbConfig= NULL;

    return 1;
}


/****************************************************************************
*  BEGIN AN ARB TRANSACTION
****************************************************************************/
bool ARB_begin_transaction()
{
    // INIT LOCK
    if(global_ARB_lock || !global_ARB_available) return false;
    else global_ARB_lock= true;

    // INCREASE CONNECTION COUNTER
    global_ARB_connections++;

    // FIRST TRANSACTION REQUEST?
    if(global_ARB_connections == 1)
        GB_begin_transaction(global_gbData);

    // REMOVE LOCK
    global_ARB_lock= false;

    // RETURN TRUE
    return true;
}


/****************************************************************************
*  COMMIT AN ARB TRANSACTION
****************************************************************************/
bool ARB_commit_transaction()
{
    // INIT LOCK
    if(global_ARB_lock || !global_ARB_available) return false;
    else global_ARB_lock= true;

    // CHECK CONNECTION COUNTER
    if(global_ARB_connections > 0)
    {
        // DECREASE CONNECTION COUNTER
        global_ARB_connections--;

        if(global_ARB_connections == 0)
        {
            // COMMIT ARB TRANSACTION
            GB_commit_transaction(global_gbData);
        }
    }

    // REMOVE LOCK
    global_ARB_lock= false;

    // RETURN TRUE
    return true;
}


/****************************************************************************
*  REQUEST ARB CONNECTION STATE
****************************************************************************/
bool ARB_connected() { return global_ARB_available; }


/****************************************************************************
*  SMALL DEBUG FUNCTION -- DUMPS ALL ARB DB ENTRIES -- HELPER
****************************************************************************/
void ARB_dump_helper(GBDATA *gb_level, int tabcount)
{
    char *key;

    GBDATA *gb_next_level= GB_find(gb_level, 0, 0, down_level);

    while(gb_next_level)
    {
        for(int i=0; i < tabcount; i++) printf("  ");

        key= GB_read_key(gb_next_level);

        if(key) printf("[%s]\n", key);
        else printf("[-]\n");

        ARB_dump_helper(gb_next_level, tabcount + 1);

        gb_next_level= GB_find(gb_next_level, 0, 0, this_level|search_next);
    }
}


/****************************************************************************
*  SMALL DEBUG FUNCTION -- DUMPS ALL ARB DB ENTRIES -- FUNCTION
****************************************************************************/
void ARB_dump(GBDATA *gb_main)
{
    ARB_begin_transaction();

    ARB_dump_helper(gb_main, 0);

    ARB_commit_transaction();
}


/****************************************************************************
*  RETURN CURRENT GBDATA ENTRY
****************************************************************************/
GBDATA *get_gbData() { return global_gbData; }


/****************************************************************************
*  FIND ARB SPECIES ENTRY BY SPECIES NAME (CHAR *)
****************************************************************************/
GBDATA *find_species(char *name)
{
    bool found= false;
    GBDATA *gb_sp= NULL;
    GBDATA *gb_sp_name;
    char *sp_name;

    if(!name) return NULL;

    ARB_begin_transaction();

    gb_sp= GB_find(GBT_get_species_data(global_gbData), "species", 0, down_level);
    while(gb_sp && !found)
    {
        gb_sp_name= GB_find(gb_sp, "name", 0, down_level);

        if(gb_sp_name)
        {
            sp_name= GB_read_string(gb_sp_name);
            if(!strcmp(sp_name, name))
            {
                found= true;
            }
            else
            {
                gb_sp= GB_find(gb_sp, "species", 0, this_level|search_next);
            }
        }
    }

    ARB_commit_transaction();

    return gb_sp;
}


/****************************************************************************
*  FIND GENOME ENTRY BY SPECIES NAME AND EXPERIMENT NAME
****************************************************************************/
GBDATA *find_genome(char *sp_name)
{
    if(!sp_name) return NULL;

    GBDATA *gb_sp_entry= find_species(sp_name);
    if(!gb_sp_entry) return NULL;

    return find_genome(gb_sp_entry);
}


/****************************************************************************
*  FIND GENOME ENTRY BY SPECIES NAME AND EXPERIMENT NAME
****************************************************************************/
GBDATA *find_genome(GBDATA *gb_sp_entry)
{
    GBDATA *gb_gene_data= NULL;

    if(!gb_sp_entry) return NULL;

    ARB_begin_transaction();

    gb_gene_data= GB_find(gb_sp_entry, "gene_data", 0, down_level);

    ARB_commit_transaction();

    return gb_gene_data;
}


/****************************************************************************
*  FIND ARB EXPERIMENT ENTRY BY SPECIES NAME AND EXPERIMENT NAME
****************************************************************************/
GBDATA *find_experiment(char *sp_name, char *name)
{
    GBDATA *gb_sp_entry= find_species(sp_name);
    if(!gb_sp_entry) return NULL;

    return find_experiment(gb_sp_entry, name);
}


/****************************************************************************
*  FIND ARB EXPERIMENT ENTRY BY ARB SPECIES ENTRY AND EXPERIMENT NAME
****************************************************************************/
GBDATA *find_experiment(GBDATA *gb_sp_entry, char *name)
{
    bool found= false;
    GBDATA *gb_exp= NULL;
    GBDATA *gb_exp_data= NULL;
    GBDATA *gb_exp_name;
    char *exp_name;

    if(!name) return NULL;

    ARB_begin_transaction();

    gb_exp_data= GB_find(gb_sp_entry, "experiment_data", 0, down_level);
    if(gb_exp_data)
    {
        gb_exp= GB_find(gb_exp_data, "experiment", 0, down_level);
        while(gb_exp && !found)
        {
            gb_exp_name= GB_find(gb_exp, "name", 0, down_level);

            if(gb_exp_name)
            {
                exp_name= GB_read_string(gb_exp_name);
                if(!strcmp(exp_name, name))
                {
                    found= true;
                }
                else
                {
                    gb_exp= GB_find(gb_exp, "experiment", 0, this_level|search_next);
                }
            }
        }
    }

    ARB_commit_transaction();

    return gb_exp;
}


/****************************************************************************
*  FIND ARB PROTEOME ENTRY BY SPECIES, EXPERIMENT AND PROTEOME NAME
****************************************************************************/
GBDATA *find_proteome(char *sp_name, char *exp_name, char *prot_name)
{
    GBDATA *gb_exp_entry= find_experiment(sp_name, exp_name);
    if(!gb_exp_entry) return NULL;

    return find_proteome(gb_exp_entry, prot_name);
}


/****************************************************************************
*  FIND ARB PROTEOME ENTRY BY ARB EXPERIMENT ENTRY AND PROTEOME NAME
****************************************************************************/
GBDATA *find_proteome(GBDATA *gb_exp_entry, char *name)
{
    bool found= false;
    GBDATA *gb_prot= NULL;
    GBDATA *gb_prot_data= NULL;
    GBDATA *gb_prot_name;
    char *prot_name;

    if(!name) return NULL;

    ARB_begin_transaction();

    gb_prot_data= GB_find(gb_exp_entry, "proteome_data", 0, down_level);
    if(gb_prot_data)
    {
        gb_prot= GB_find(gb_prot_data, "proteome", 0, down_level);
        while(gb_prot && !found)
        {
            gb_prot_name= GB_find(gb_prot, "name", 0, down_level);

            if(gb_prot_name)
            {
                prot_name= GB_read_string(gb_prot_name);
                if(!strcmp(prot_name, name))
                {
                    found= true;
                }
                else
                {
                    gb_prot= GB_find(gb_prot, "proteome", 0, this_level|search_next);
                }
            }
        }
    }

    ARB_commit_transaction();

    return gb_prot;
}


/****************************************************************************
*  APPEND ARB SPECIES ENTRIES TO A LIST WIDGET
****************************************************************************/
void getSpeciesList(Widget list, bool clear= false)
{
    // LOCAL VARIABLES
    GBDATA *gb_sp;
    GBDATA *gb_sp_name;
    char *sp_name;
    int pos;

    if(clear) // CLEAR LIST, IF FLAG IST SET (POS = 0)
    {
        XmListDeleteAllItems(list);
        pos= 0;
    }
    else // FIND LAST ITEM POSITION TO APPEND
    {
        XtVaGetValues(list, XmNitems, &pos, NULL);
    }

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // FETCH SPECIES ENTRIES
    gb_sp= GB_find(GBT_get_species_data(global_gbData), "species", 0, down_level);
    while(gb_sp)
    {
        // FETCH SPECIES NAME ENTRY
        gb_sp_name= GB_find(gb_sp, "name", 0, down_level);

        // IF A NAME IS AVAILABLE, ...
        if(gb_sp_name)
        {
            // GET CHAR* OF SPECIES NAME
            sp_name= GB_read_string(gb_sp_name);

            // ADD ITEM TO LIST WIDGET
            XmListAddItemUnselected(list, PGT_XmStringCreateLocalized(sp_name), pos);

            // INCREASE POSITION COUNTER
            pos++;
        }
        gb_sp= GB_find(gb_sp, "species", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();
}


/****************************************************************************
*  APPEND ARB EXPERIMENT ENTRIES TO A LIST WIDGET
****************************************************************************/
void getExperimentList(Widget list, char *species, bool clear= false)
{
    // LOCAL VARIABLES
    GBDATA *gb_sp;
    GBDATA *gb_exp;
    GBDATA *gb_exp_data;
    GBDATA *gb_exp_name;
    char *exp_name;
    int pos;

    if(clear) // CLEAR LIST, IF FLAG IST SET (POS = 0)
    {
        XmListDeleteAllItems(list);
        pos= 0;
    }
    else // FIND LAST ITEM POSITION TO APPEND
    {
        XtVaGetValues(list, XmNitems, &pos, NULL);
    }

    // FETCH SPECIES ENTRY
    gb_sp= find_species(species);
    if(!gb_sp) return;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // FETCH EXPERIMENT DATA ENTRY
    gb_exp_data= GB_find(gb_sp, "experiment_data", 0, down_level);

    if(gb_exp_data)
    {
        // BROWSE EXPERIMENTS AND FETCH ENTRY NAMES
        gb_exp= GB_find(gb_exp_data, "experiment", 0, down_level);

        while(gb_exp)
        {
            // FETCH SPECIES NAME ENTRY
            gb_exp_name= GB_find(gb_exp, "name", 0, down_level);

            // IF A NAME IS AVAILABLE, ...
            if(gb_exp_name)
            {
                // GET CHAR* OF SPECIES NAME
                exp_name= GB_read_string(gb_exp_name);

                // ADD ITEM TO LIST WIDGET
                XmListAddItemUnselected(list, PGT_XmStringCreateLocalized(exp_name), pos);

                // INCREASE POSITION COUNTER
                pos++;
            }
            gb_exp= GB_find(gb_exp, "experiment", 0, this_level|search_next);
        }
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();
}


/****************************************************************************
*  APPEND ARB PROTEOME ENTRIES TO A LIST WIDGET
****************************************************************************/
void getProteomeList(Widget list, char *species, char *experiment, bool clear= false)
{
    // LOCAL VARIABLES
    GBDATA *gb_exp;
    GBDATA *gb_prot;
    GBDATA *gb_prot_data;
    GBDATA *gb_prot_name;
    char *prot_name;
    int pos;

    if(clear) // CLEAR LIST, IF FLAG IST SET (POS = 0)
    {
        XmListDeleteAllItems(list);
        pos= 0;
    }
    else // FIND LAST ITEM POSITION TO APPEND
    {
        XtVaGetValues(list, XmNitems, &pos, NULL);
    }

    // FETCH EXPERIMENT ENTRY
    gb_exp= find_experiment(species, experiment);
    if(!gb_exp) return;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // FETCH EXPERIMENT DATA ENTRY
    gb_prot_data= GB_find(gb_exp, "proteome_data", 0, down_level);

    if(gb_prot_data)
    {
        // BROWSE EXPERIMENTS AND FETCH ENTRY NAMES
        gb_prot= GB_find(gb_prot_data, "proteome", 0, down_level);
        while(gb_prot)
        {
            // FETCH SPECIES NAME ENTRY
            gb_prot_name= GB_find(gb_prot, "name", 0, down_level);

            // IF A NAME IS AVAILABLE, ...
            if(gb_prot_name)
            {
                // GET CHAR* OF SPECIES NAME
                prot_name= GB_read_string(gb_prot_name);

                // ADD ITEM TO LIST WIDGET
                XmListAddItemUnselected(list, PGT_XmStringCreateLocalized(prot_name), pos);

                // INCREASE POSITION COUNTER
                pos++;
            }
            gb_prot= GB_find(gb_prot, "proteome", 0, this_level|search_next);
        }
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();
}


/****************************************************************************
*  TRAVERSE THE ARB DATABASE, SEARCH ALL PROTEIN ENTRIES AND
*  APPEND THEM TO A GIVEN LIST WIDGET (UNIQUE LIST)
****************************************************************************/
void getEntryNamesList(Widget list, bool clear= false)
{
    // LOCAL VARIABLES
    GBDATA *gb_sp, *gb_exp, *gb_prot, *gb_protein, *gb_keys, *gb_prot_data;
    char *str;

    // CLEAR LIST, IF FLAG IST SET (POS = 0)
    if(clear) XmListDeleteAllItems(list);

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // FETCH SPECIES ENTRIES
    gb_sp= GB_find(GBT_get_species_data(global_gbData), "species", 0, down_level);
    while(gb_sp)
    {
        // FIND EXPERIMENT
        gb_exp= GB_find(gb_sp, "experiment", 0, down_2_level);
        while(gb_exp)
        {
            // FIND PROTEIN
            gb_prot_data= GB_find(gb_exp, "proteome_data", 0, down_level);

            // FIND PROTEOME
            gb_prot= GB_find(gb_prot_data, "proteome", 0, down_level);
            while(gb_prot)
            {
                int entry_name_counter= MAX_ENTRY_NAMES_COUNTER;

                // FIND PROTEIN
                gb_protein= GB_find(gb_prot, "protein", 0, down_level);
                while(gb_protein && entry_name_counter)
                {
                    // TRAVERSE ALL ENTRIES
                    gb_keys= GB_find(gb_protein, 0, 0, down_level);

                    while(gb_keys)
                    {
                        // GET KEY NAME
                        str= GB_read_key(gb_keys);
                        XmString xm_str = PGT_XmStringCreateLocalized(str);

                        // ADD ITEM TO LIST WIDGET
                        if(!XmListItemExists(list, xm_str))
                            XmListAddItemUnselected(list, xm_str, 0);

                        gb_keys= GB_find(gb_keys, 0, 0, this_level|search_next);
                    }

                    entry_name_counter--;

                    gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
                }

                gb_prot= GB_find(gb_prot, "proteome", 0, this_level|search_next);
            }

            gb_exp= GB_find(gb_exp, "experiment", 0, this_level|search_next);
        }

        gb_sp= GB_find(gb_sp, "species", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();
}


/****************************************************************************
*  CHECK AWAR PATH AND CREATE MISSING ENTRIES
****************************************************************************/
bool check_create_AWAR(GBDATA *gb_data, const char *AWAR_path, bool transaction)
{
    // DO WE HAVE A STRING?
    if(AWAR_path == NULL) return false;

    // CREATE BUFFER
    char *buf= (char *)malloc(1024 * sizeof(char));
    strncpy(buf, AWAR_path, 1023);

    if(!gb_data) return false;

    // FIRST COUNT TOKENS
    int count= 0;
    for(strtok(buf, "/"); (strtok(NULL, "/")) != NULL; ++count){}

    if(transaction) ARB_begin_transaction();

    GBDATA *gb_search= NULL, *gb_create= NULL;
    char *str= NULL;

    strncpy(buf, AWAR_path, 1023);
    str= strtok(buf, "/");
    while(str && count)
    {
        // DO WE HAVE THE PART OF THE PATH?
        gb_search= GB_search(gb_data, str, GB_FIND);

        // IF THE SEARCH FAILED TRY TO CREATE THE CONTAINER
        if(!gb_search)
        {
            // CREATE CONTAINER
            gb_create= GB_create_container(gb_data, str);

            // DID AN ERROR OCCUR?
            if(!gb_create)
            {
                if(transaction) ARB_commit_transaction();
                free(buf);
                return false;
            }

            gb_data= gb_create;
        }
        else
        {
            gb_data= gb_search;
        }

        str= strtok(NULL, "/");

        count--;
    }

    // OKAY, WE WENT THROUGH THE PATH, WHAT ABOUT THE AWAR ITSELF?
    gb_search= GB_search(gb_data, str, GB_FIND);

    // CREATE IT IF NECESSARY
    if(!gb_search)
    {
        // CREATE CONTAINER
        gb_create= GB_create(gb_data, str, GB_STRING);

        // DID AN ERROR OCCUR?
        if(!gb_create)
        {
            if(transaction) ARB_commit_transaction();
            free(buf);
            return false;
        }
        else GB_write_string(gb_create, "");
    }

    if(transaction) ARB_commit_transaction();

    free(buf);

    return true;
}


/****************************************************************************
*  UPDATES THE CONTENT OF AN AWAR
****************************************************************************/
void set_AWAR(const char *AWAR_path, char *content)
{
    GBDATA *gb_data= get_gbData();

    check_create_AWAR(gb_data, AWAR_path, true);

    ARB_begin_transaction();

    GBDATA *gb_awar= GB_search(gb_data, AWAR_path, GB_FIND);

    if(gb_awar) GB_write_string(gb_awar, content);

    ARB_commit_transaction();
}


/****************************************************************************
*  FETCH THE CONTENT OF AN AWAR
****************************************************************************/
char *get_AWAR(const char *AWAR_path)
{
    char *content;

    GBDATA *gb_data= get_gbData();

    check_create_AWAR(gb_data, AWAR_path, true);

    ARB_begin_transaction();

    GBDATA *gb_awar= GB_search(gb_data, AWAR_path, GB_FIND);
    content= GB_read_string(gb_awar);

    ARB_commit_transaction();

    return content;
}


/****************************************************************************
*  UPDATES THE PGT CONFIG
****************************************************************************/
void set_CONFIG(const char *CONFIG_path, const char *content)
{
    if(!global_CONFIG_available) return;

    check_create_AWAR(global_gbConfig, CONFIG_path, false);

    GBDATA *gb_awar= GB_search(global_gbConfig, CONFIG_path, GB_FIND);

    if(gb_awar) GB_write_string(gb_awar, content);
}


/****************************************************************************
*  FETCH THE CONTENT OF AN CONFIG ENTRY
****************************************************************************/
char *get_CONFIG(const char *CONFIG_path)
{
    if(!global_CONFIG_available) return NULL;

    char *content= NULL;

    check_create_AWAR(global_gbConfig, CONFIG_path, false);

    GBDATA *gb_awar= GB_search(global_gbConfig, CONFIG_path, GB_FIND);
    content= GB_read_string(gb_awar);

    return content;
}


/****************************************************************************
*  VARIOUS GET + SET AWAR FUNCTIONS
****************************************************************************/
void set_species_AWAR(char *content) { set_AWAR(AWAR_SPECIES_NAME, content); }
void set_experiment_AWAR(char *content) { set_AWAR(AWAR_EXPERIMENT_NAME, content); }
void set_proteom_AWAR(char *content) { set_AWAR(AWAR_PROTEOM_NAME, content); }
void set_protein_AWAR(char *content) { set_AWAR(AWAR_PROTEIN_NAME, content); }
void set_gene_AWAR(char *content) { set_AWAR(AWAR_GENE_NAME, content); }
void set_config_AWAR(char *content) { set_AWAR(AWAR_CONFIG_CHANGED, content); }

char *get_species_AWAR() { return get_AWAR(AWAR_SPECIES_NAME); }
char *get_experiment_AWAR() { return get_AWAR(AWAR_EXPERIMENT_NAME); }
char *get_proteom_AWAR() { return get_AWAR(AWAR_PROTEOM_NAME); }
char *get_protein_AWAR() { return get_AWAR(AWAR_PROTEIN_NAME); }
char *get_gene_AWAR() { return get_AWAR(AWAR_GENE_NAME); }
char *get_config_AWAR() { return get_AWAR(AWAR_CONFIG_CHANGED); }


/****************************************************************************
*  ADD A CALLBACK TO AN ARB CONTAINER
****************************************************************************/
void add_callback(const char *ARB_path, GB_CB callback, void *caller)
{
    GBDATA *gb_data= get_gbData();

    ARB_begin_transaction();

    GBDATA *gb_field= GB_search(gb_data, ARB_path, GB_FIND);

    if(gb_field) GB_add_callback(gb_field, GB_CB_ALL, callback, (int *) caller);

    ARB_commit_transaction();
}


/****************************************************************************
*  VARIOUS CALLBACKS
****************************************************************************/
void add_species_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_SPECIES_NAME, callback, caller); }
void add_experiment_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_EXPERIMENT_NAME, callback, caller); }
void add_proteom_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_PROTEOM_NAME, callback, caller); }
void add_protein_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_PROTEIN_NAME, callback, caller); }
void add_gene_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_GENE_NAME, callback, caller); }
void add_config_callback(GB_CB callback, void *caller)
    { add_callback(AWAR_CONFIG_CHANGED, callback, caller); }


/****************************************************************************
*  CREATE & CHECK THE AWARS (SET DEFAULT IF THE AWARS ARE EMPTY)
****************************************************************************/
void checkCreateAWARS()
{
    // GET AWARS (IF AVAILABLE)
    char *crosshair_color_CONFIG= get_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR);
    char *unmarked_color_CONFIG=  get_CONFIG(CONFIG_PGT_COLOR_UNMARKED);
    char *marked_color_CONFIG=    get_CONFIG(CONFIG_PGT_COLOR_MARKED);
    char *selected_color_CONFIG=  get_CONFIG(CONFIG_PGT_COLOR_SELECTED);
    char *text_color_CONFIG=      get_CONFIG(CONFIG_PGT_COLOR_TEXT);
    char *id_protein_CONFIG=      get_CONFIG(CONFIG_PGT_ID_PROTEIN);
    char *id_gene_CONFIG=         get_CONFIG(CONFIG_PGT_ID_GENE);
    char *info_protein_CONFIG=    get_CONFIG(CONFIG_PGT_INFO_PROTEIN);
    char *info_gene_color_CONFIG= get_CONFIG(CONFIG_PGT_INFO_GENE);

    // SET DEFAULT VALUES IF THE CONFIG IS EMPTY
    if(!crosshair_color_CONFIG || (strlen(crosshair_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR, DEFAULT_COLOR_CROSSHAIR);

    if(!unmarked_color_CONFIG || (strlen(unmarked_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_COLOR_UNMARKED, DEFAULT_COLOR_UNMARKED);

    if(!marked_color_CONFIG || (strlen(marked_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_COLOR_MARKED, DEFAULT_COLOR_MARKED);

    if(!selected_color_CONFIG || (strlen(selected_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_COLOR_SELECTED, DEFAULT_COLOR_SELECTED);

    if(!text_color_CONFIG || (strlen(text_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_COLOR_TEXT, DEFAULT_COLOR_TEXT);

    if(!id_protein_CONFIG || (strlen(id_protein_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_ID_PROTEIN, DEFAULT_ID_PROTEIN);

    if(!id_gene_CONFIG || (strlen(id_gene_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_ID_GENE, DEFAULT_ID_GENE);

    if(!info_protein_CONFIG || (strlen(info_protein_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_INFO_PROTEIN, DEFAULT_INFO_PROTEIN);

    if(!info_gene_color_CONFIG || (strlen(info_gene_color_CONFIG) == 0))
        set_CONFIG(CONFIG_PGT_INFO_GENE, DEFAULT_INFO_GENE);

//     free(...);
}
