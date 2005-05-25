#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <ctype.h>

#include <list>
#include <string>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <aw_color_groups.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"
#include "awt_config_manager.hxx"
#include "awt_changekey.hxx"
#include "awt_sel_boxes.hxx"

#include "GEN.hxx"

using namespace std;

/***************** Create the database query box and functions *************************/

#define AWAR_COLORIZE "tmp/arbdb_query_all/colorize"

static void awt_query_create_global_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_COLORIZE, 0, aw_def);
}



enum AWT_EXT_QUERY_TYPES {
    AWT_EXT_QUERY_NONE,
    AWT_EXT_QUERY_COMPARE_LINES,
    AWT_EXT_QUERY_COMPARE_WORDS
};

awt_query_struct::awt_query_struct(void){
    memset((char *)this,0,sizeof(awt_query_struct));
    select_bit = 1;
}

long awt_count_queried_species(struct adaqbsstruct *cbs){
    long    count      = 0;
    GBDATA *gb_species;
    for (   gb_species = GBT_first_species(cbs->gb_main);
            gb_species;
            gb_species = GBT_next_species(gb_species)){
        if (IS_QUERIED(gb_species,cbs)) {
            count ++;
        }
    }
    return count;
}

long awt_query_update_list(void *dummy, struct adaqbsstruct *cbs)
{
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);
    //     char buffer[BUFSIZE+1],*p;
    int count;
    char    *key = cbs->aws->get_root()->awar(cbs->awar_keys[0])->read_string();

    cbs->aws->clear_selection_list(cbs->result_id);
    count = 0;

    AW_root         *aw_root = cbs->aws->get_root();
    AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
    {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item       = cbs->selector->get_next_item(gb_item))
            {
                if (IS_QUERIED(gb_item,cbs)) {
                    if (count < AWT_MAX_QUERY_LIST_LEN ) {
                        char *name = cbs->selector->generate_item_id(cbs->gb_main, gb_item);
                        if (!name) continue; // a guy w/o name

                        char flag                       = ' ';
                        if (GB_read_flag(gb_item)) flag = '*';

                        char *data = 0;
                        {
                            GBDATA *gb_key = GB_search(gb_item,key,GB_FIND);
                            if (gb_key) {
                                data = GB_read_as_string(gb_key);
                            }
                        }
                        const char *line = GBS_global_string("%c %-*s :%s",
                                                             flag,
                                                             cbs->selector->item_name_length, name,
                                                             data ? data : "<no data>");

                        cbs->aws->insert_selection( cbs->result_id, line, name );
                        free(name);
                        free(data);
                    }
                    else if (count == AWT_MAX_QUERY_LIST_LEN) {
                        cbs->aws->insert_selection( cbs->result_id,
                                                    "********* List truncated *********","" );
                    }
                    count ++;
                }
            }
    }

    cbs->aws->insert_default_selection( cbs->result_id, "End of list", "" );
    cbs->aws->update_selection_list( cbs->result_id );
    free(key);
    cbs->aws->get_root()->awar(cbs->awar_count)->write_int( (long)count);
    GB_pop_transaction(cbs->gb_main);
    return count;
}
// Mark listed species
// mark = 1 -> mark listed
// mark | 8 -> don'd change rest
void awt_do_mark_list(void *dummy, struct adaqbsstruct *cbs, long mark)
{
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) {
                        GB_write_flag(gb_item, mark&1);
                    }
                    else if ((mark&8) == 0) {
                        GB_write_flag(gb_item, 1-(mark&1));
                    }
                }
        }

    awt_query_update_list(0,cbs);
    GB_pop_transaction(cbs->gb_main);
}

void awt_unquery_all(void *dummy, struct adaqbsstruct *cbs){
    AWUSE(dummy);
    GB_push_transaction(cbs->gb_main);
    GBDATA *gb_species;
    for (       gb_species = GBT_first_species(cbs->gb_main);
                gb_species;
                gb_species = GBT_next_species(gb_species)){
        CLEAR_QUERIED(gb_species,cbs);
    }
    awt_query_update_list(0,cbs);
    GB_pop_transaction(cbs->gb_main);
}

void awt_delete_species_in_list(void *dummy, struct adaqbsstruct *cbs)
{
    GB_begin_transaction(cbs->gb_main);
    long cnt = 0;

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item, cbs)) cnt++;
                }
        }

    sprintf(AW_ERROR_BUFFER,"Are you sure to delete %li %s",cnt, cbs->selector->items_name);
    if (aw_message(AW_ERROR_BUFFER,"OK,CANCEL")){
        GB_abort_transaction(cbs->gb_main);
        return;
    }

    GB_ERROR error = 0;

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item,cbs)) {
                        error = GB_delete(gb_item);
                    }
                }
        }

    if (error) {
        GB_abort_transaction(cbs->gb_main);
        aw_message(error);
    }else{
        awt_query_update_list(dummy,cbs);
        GB_commit_transaction(cbs->gb_main);
    }
}

GB_HASH *awt_generate_species_hash(GBDATA *gb_main, char *key,int split)
{
    GB_HASH *hash = GBS_create_hash(GBT_get_species_hash_size(gb_main),1);
    GBDATA  *gb_species;
    GBDATA  *gb_name;
    char    *keyas;

    for (   gb_species = GBT_first_species(gb_main);
            gb_species;
            gb_species = GBT_next_species(gb_species))
    {
        gb_name = GB_search(gb_species,key,GB_FIND);
        if (!gb_name) continue;
        keyas = GB_read_as_string(gb_name);
        if (keyas && strlen(keyas)) {
            if (split){
                char *t;
                for (t = strtok(keyas," ");
                     t;
                     t = strtok(0," "))
                    {
                        if (strlen(t) ){
                            GBS_write_hash(hash,t,(long)gb_species);
                        }
                    }
            }else{
                GBS_write_hash(hash,keyas,(long)gb_species);
            }
            delete keyas;
        }
    }
    return hash;
}

//  ------------------------
//      class awt_query
//  ------------------------
typedef enum { ILLEGAL, AND, OR } awt_query_operator;

class awt_query {
private:

    awt_query_operator  op;
    char               *key;
    char               *query;
    AW_BOOL             Not;
    awt_query          *next;

    AW_BOOL rek;
    AW_BOOL all_fields;

    GBQUARK keyquark;

    void init() {
        op    = ILLEGAL;
        key   = 0;
        query = 0;
        Not   = AW_FALSE;
        next  = 0;

        rek        = AW_FALSE;
        all_fields = AW_FALSE;

        keyquark = 0;
    }

public:

    awt_query() { init(); }
    awt_query(struct adaqbsstruct *cbs);

    virtual ~awt_query() {
        free(key);
        free(query);
        delete next;
    }

    awt_query_operator getOperator() const { return op; }
    const char *getKey() const { return key; }
    const char *getQuery() const { return query; }
    AW_BOOL getNot() const { return Not; }

    awt_query *getNext() { return next; }

    void initForContainer(GBDATA *gb_item_container) {
        rek      = 0;
        keyquark = -1;

        if (GB_first_non_key_char(key)) {
            if (strcmp(key, ALL_FIELDS_PSEUDO_FIELD) == 0) {
                all_fields = 1;
            }
            else {
                rek = 1;
            }
        }
        else {
            keyquark = GB_key_2_quark(gb_item_container, key);
        }

        if (next) next->initForContainer(gb_item_container);
    }

    AW_BOOL is_rek() const { return rek; }
    AW_BOOL query_all_fields() const { return all_fields; }
    GBQUARK getKeyquark() const { return keyquark; }
};

//  -------------------------------------------------------
//      awt_query::awt_query(struct adaqbsstruct *cbs)
//  -------------------------------------------------------
awt_query::awt_query(struct adaqbsstruct *cbs) {
    init(); // set defaults

    AW_root *aw_root = cbs->aws->get_root();

    op    = OR; // before hit is false
    key   = aw_root->awar(cbs->awar_keys[0])->read_string();
    query = aw_root->awar(cbs->awar_queries[0])->read_string();
    Not   = aw_root->awar(cbs->awar_not[0])->read_int() != 0;
    next  = 0;

    awt_query *tail = this;
    for (size_t keyidx = 1; keyidx<AWT_QUERY_SEARCHES; ++keyidx) {
        char *opstr = aw_root->awar(cbs->awar_operator[keyidx])->read_string();

        if (strcmp(opstr, "ign") != 0) { // not ignore
            awt_query_operator next_op = ILLEGAL;

            if (strcmp(opstr, "and")      == 0) next_op = AND;
            else if (strcmp(opstr, "or")  == 0) next_op = OR;
            else aw_assert(0);

            if (next_op != ILLEGAL) {
                awt_query *next_query = new awt_query();

                next_query->op    = next_op;
                next_query->key   = aw_root->awar(cbs->awar_keys[keyidx])->read_string();
                next_query->query = aw_root->awar(cbs->awar_queries[keyidx])->read_string();
                next_query->Not   = aw_root->awar(cbs->awar_not[keyidx])->read_int() != 0;

                tail->next = next_query;
                tail       = tail->next;
            }
        }
        free(opstr);
    }
}

//  --------------------------------------------------------------------------------
//      void awt_do_query(void *dummy, struct adaqbsstruct *cbs,AW_CL ext_query)
//  --------------------------------------------------------------------------------
void awt_do_query(void *dummy, struct adaqbsstruct *cbs,AW_CL ext_query)
{
    AWUSE(dummy);
    AW_root   *aw_root = cbs->aws->get_root();
    awt_query  query(cbs);

    //  char *key = aw_root->awar(cbs->awar_keys[0])->read_string();

    //  if (!strlen(key)) {
    //         free(key);
    //      aw_message("ERROR: To perfom a query you have to select a field and enter a search string");
    //      return;
    //  }

    char    *first_key = aw_root->awar(cbs->awar_keys[0])->read_string();
    char    *first_query = aw_root->awar(cbs->awar_queries[0])->read_string();
    GB_push_transaction(cbs->gb_main);
    if (cbs->gb_ref && !strlen(first_query)){
        if (!ext_query) ext_query = AWT_EXT_QUERY_COMPARE_LINES;
    }

    AWT_QUERY_MODES mode = (AWT_QUERY_MODES)aw_root->awar(cbs->awar_ere)->read_int();
    AWT_QUERY_RANGE range = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();
    AWT_QUERY_TYPES type = (AWT_QUERY_TYPES)aw_root->awar(cbs->awar_by)->read_int();

    //  int      hit;
    GB_HASH *ref_hash = 0;

    if (cbs->gb_ref && ( ext_query == AWT_EXT_QUERY_COMPARE_LINES || ext_query == AWT_EXT_QUERY_COMPARE_WORDS)) {
        GB_push_transaction(cbs->gb_ref);
        ref_hash = awt_generate_species_hash(cbs->gb_ref,first_key, ext_query == AWT_EXT_QUERY_COMPARE_WORDS);
    }

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
         gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            //         int     rek      = 0;
            //         GBQUARK keyquark = -1;
            //
            //         if (GB_first_non_key_char(key)) rek = 1;
            //         else keyquark                       = GB_key_2_quark(gb_item_container,key);

            query.initForContainer(gb_item_container);

            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    switch(mode) {
                    case    AWT_QUERY_GENERATE: CLEAR_QUERIED(gb_item,cbs); break;
                    case    AWT_QUERY_ENLARGE: if (IS_QUERIED(gb_item,cbs)) continue; break;
                    case    AWT_QUERY_REDUCE:   if (!IS_QUERIED(gb_item,cbs)) continue; break;
                    }

                    AW_BOOL hit = 0;

                    switch(type) {
                    case AWT_QUERY_MARKED: {
                        hit = GB_read_flag(gb_item);
                        break;
                    }
                    case AWT_QUERY_MATCH:
                    case AWT_QUERY_DONT_MATCH: {

                        for (awt_query *this_query = &query; this_query;  this_query = this_query ? this_query->getNext() : 0) { // iterate over all single queries
                            AW_BOOL  this_hit   = 0;
                            GBDATA  *gb_key     = 0;
                            AW_BOOL  abortQuery = 0;
                            AW_BOOL  all_fields = 0;

                            if (this_query->is_rek()) {
                                gb_key = GB_search(gb_item,this_query->getKey(),GB_FIND);
                            }
                            else if (this_query->query_all_fields()) {
                                gb_key     = GB_find(gb_item, 0, 0, down_level);
                                all_fields = 1;
                            }
                            else {
                                gb_key = GB_find_sub_by_quark(gb_item,this_query->getKeyquark(),0,0);
                            }

                            while (gb_key) {
                                switch(ext_query) {
                                case AWT_EXT_QUERY_NONE: {
                                    const char *query_string = this_query->getQuery();
                                    if (gb_key) {
                                        char *data = GB_read_as_string(gb_key);
                                        awt_assert(data || all_fields);
                                        if (data) {
                                            switch (query_string[0]) {
                                            case '>': if (atoi(data)> atoi(query_string+1)) this_hit   = 1; break;
                                            case '<': if (atoi(data) < atoi(query_string+1)) this_hit  = 1; break;
                                            default: if (GBS_string_cmp(data,query_string,1) == 0) this_hit = 1; break;
                                            }
                                            free(data);
                                        }
                                    }
                                    else {
                                        this_hit = (strlen(query_string) == 0);
                                    }
                                    break;
                                }
                                case AWT_EXT_QUERY_COMPARE_LINES:
                                case AWT_EXT_QUERY_COMPARE_WORDS: {
                                    if (gb_key) {
                                        char   *data        = GB_read_as_string(gb_key);
                                        GBDATA *gb_ref_pntr = 0;

                                        if (!data || !data[0]) {
                                            delete data;
                                            break;
                                        }
                                        if (ext_query == AWT_EXT_QUERY_COMPARE_WORDS){
                                            for (char *t = strtok(data," "); t; t = strtok(0," ")) {
                                                gb_ref_pntr =   (GBDATA *)GBS_read_hash(ref_hash,t);
                                                if (gb_ref_pntr){
                                                    if (cbs->look_in_ref_list) {
                                                        if (IS_QUERIED(gb_ref_pntr,cbs)) this_hit = 1;
                                                    }
                                                    else {
                                                        this_hit = 1;
                                                    }
                                                }
                                            }
                                        }else{
                                            gb_ref_pntr =   (GBDATA *)GBS_read_hash(ref_hash,data);
                                            if (gb_ref_pntr){
                                                if (cbs->look_in_ref_list) {
                                                    if (IS_QUERIED(gb_ref_pntr,cbs)) this_hit = 1;
                                                }else{
                                                    this_hit = 1;
                                                }
                                            }
                                        }
                                        delete data;
                                    }
                                    abortQuery = true;
                                    hit        = this_hit;
                                    break;
                                }
                                }

                                if (abortQuery) break;

                                if (hit) {
                                    if (all_fields) {
                                        // @@@ store field for display?
                                    }
                                    break;
                                }

                                if (all_fields) {
                                    gb_key = GB_find(gb_key, 0, 0, this_level|search_next);
                                }
                                else {
                                    gb_key = 0;
                                }
                            }

                            if (abortQuery) {
                                this_query = 0;
                            }
                            else {
                                if (this_query->getNot()) this_hit = !this_hit;

                                // calculate result
                                switch (this_query->getOperator()) {
                                case AND: hit = hit && this_hit; break;
                                case OR: hit  = hit || this_hit; break;
                                default : awt_assert(0); break;
                                }
                            }
                        }

                        if (type == AWT_QUERY_DONT_MATCH) hit = !hit;
                        break;
                    }
                    }
                    if (hit) SET_QUERIED(gb_item,cbs);
                    else    CLEAR_QUERIED(gb_item,cbs);
                }
        }

    //  delete key;
    free(first_query);
    free(first_key);

    awt_query_update_list(0,cbs);

    GB_pop_transaction(cbs->gb_main);
    if (ref_hash){
        GBS_free_hash(ref_hash);
        GB_pop_transaction(cbs->gb_ref);
    }
}

void awt_copy_selection_list_2_queried_species(struct adaqbsstruct *cbs, AW_selection_list* id){
    GB_transaction dummy(cbs->gb_main);
    GBDATA *gb_species;
    for (gb_species = GBT_first_species(cbs->gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        CLEAR_QUERIED(gb_species,cbs);
    }
    GBDATA_SET *set = cbs->aws->selection_list_to_species_set(cbs->gb_main,id);
    if (set){
        int i;
        for (i=0;i<set->nitems;i++){
            SET_QUERIED(set->items[i],cbs);
        }
        GB_delete_set(set);
    }
    awt_query_update_list(0,cbs);
}


void awt_search_equal_entries(AW_window *,struct adaqbsstruct *cbs,int tokenize){
    char    *key = cbs->aws->get_root()->awar(cbs->awar_keys[0])->read_string();
    if (!strlen(key)) {
        delete key;
        aw_message("ERROR: To perfom a query you have to select a field and enter a search string");
        return;
    }
    GB_transaction dumy(cbs->gb_main);

    GBDATA          *gb_species_data = GB_search(cbs->gb_main,  "species_data",GB_CREATE_CONTAINER);
    long             hashsize;
    GB_ERROR         error           = 0;
    AWT_QUERY_RANGE  range           = AWT_QUERY_ALL_SPECIES;

    switch (cbs->selector->type) {
    case AWT_QUERY_ITEM_SPECIES:  {
        hashsize = GB_number_of_subentries(gb_species_data);
        break;
    }
    case AWT_QUERY_ITEM_EXPERIMENTS:
    case AWT_QUERY_ITEM_GENES: {
        // handle species sub-items
        hashsize = 0;

        for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), range);
             !error && gb_item_container;
             gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                hashsize += GB_number_of_subentries(gb_item_container);
            }

        break;
    }
    default: {
        awt_assert(0);
        hashsize = 0;
        break;
    }
    }

    if (hashsize) {
        GB_HASH *hash    = GBS_create_hash(hashsize, 1);
        AW_root *aw_root = cbs->aws->get_root();

        for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
             gb_item_container;
             gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     gb_item;
                     gb_item       = cbs->selector->get_next_item(gb_item))
                    {
                        CLEAR_QUERIED(gb_item,cbs);
                        GB_write_flag(gb_item,0);

                        GBDATA *gb_key = GB_search(gb_item,key,GB_FIND);    if (!gb_key) continue;
                        char   *data   = GB_read_as_string(gb_key);         if (!data) continue;

                        if (tokenize){
                            char *s;
                            for (s=strtok(data,",; \t."); s ; s = strtok(0,",; \t.")){
                                GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash,s);
                                if (gb_old){
                                    SET_QUERIED(gb_old,cbs);
                                    SET_QUERIED(gb_item,cbs);
                                    GB_write_flag(gb_item,1);
                                }else{
                                    GBS_write_hash(hash,s,(long)gb_item);
                                }
                            }
                        }else{
                            GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash,data);
                            if (gb_old){
                                SET_QUERIED(gb_old,cbs);
                                SET_QUERIED(gb_item,cbs);
                                GB_write_flag(gb_item,1);
                            }else{
                                GBS_write_hash(hash,data,(long)gb_item);
                            }
                        }
                        free(data);
                    }
            }

        GBS_free_hash(hash);
    }

    free(key);
    awt_query_update_list(0,cbs);
}

/***************** Pars fields *************************/

void awt_do_pars_list(void *dummy, struct adaqbsstruct *cbs)
{
    AWUSE(dummy);
    GB_ERROR  error = 0;
    char     *key   = cbs->aws->get_root()->awar(cbs->awar_parskey)->read_string();

    if (!strcmp(key,"name")){
        switch (cbs->selector->type)  {
            case AWT_QUERY_ITEM_SPECIES: {
                if (aw_message("WARNING WARNING WARNING!!! You now try to rename the species\n"
                               "    The name is used to link database entries and trees\n"
                               "    ->  ALL TREES WILL BE LOST\n"
                               "    ->  The new name MUST be UNIQUE"
                               "        if not you will corrupt the database!" ,
                               "Let's Go,Cancel")) return;
                break;
            }
            case AWT_QUERY_ITEM_GENES: {
                if (aw_message("WARNING! You now try to rename the gene\n"
                               "    ->  Pseudo-species will loose their link to the gene"
                               "    ->  The new name MUST be UNIQUE"
                               "        if not you will corrupt the database!" ,
                               "Let's Go,Cancel")) return;
                break;
            }
            case AWT_QUERY_ITEM_EXPERIMENTS: {
                if (aw_message("WARNING! You now try to rename the experiment\n"
                               "    ->  The new name MUST be UNIQUE"
                               "        if not you will corrupt the database!" ,
                               "Let's Go,Cancel")) return;
                break;
            }
            default: {
                awt_assert(0);
                return;
            }
        }
    }


    if (!strlen(key)) error = "Please select a valid key";

    char *command = cbs->aws->get_root()->awar(cbs->awar_parsvalue)->read_string();
    if (!error && !strlen(command)) {
        error = "Please enter your command";
    }

    if (error) {
        if (strlen(error)) aw_message(error);
        free(command);
        free(key);
        return;
    }

    GB_begin_transaction(cbs->gb_main);
    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key_name;

    while (!(gb_key_name  = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level))) {
        sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
        if (aw_message(AW_ERROR_BUFFER,"Create Field (Type STRING),Cancel")){
            GB_abort_transaction(cbs->gb_main);
            free(command);
            free(key);
            return;
        }
        GB_ERROR error2 = awt_add_new_changekey_to_keypath(cbs->gb_main,key,GB_STRING, cbs->selector->change_key_path);
        if (error2){
            aw_message(error2);
            GB_abort_transaction(cbs->gb_main);
            delete command;
            delete key;
            return;
        }
    }

    GBDATA *gb_key_type = GB_find(gb_key_name,CHANGEKEY_TYPE,0,this_level);
    bool aborted = false;
    {
        if (GB_read_int(gb_key_type)!=GB_STRING) {
            if (aw_message("Writing to a non-STRING database field may lead to conversion problems.", "Abort,Continue")==0) {
                aborted = true;
            }
        }
    }

    if (!aborted) {
        long  count  = 0;
        long  ncount = cbs->aws->get_root()->awar(cbs->awar_count)->read_int();
        char *deftag = cbs->aws->get_root()->awar(cbs->awar_deftag)->read_string();
        char *tag    = cbs->aws->get_root()->awar(cbs->awar_tag)->read_string();

        {
            long use_tag = cbs->aws->get_root()->awar(cbs->awar_use_tag)->read_int();
            if (!use_tag || !strlen(tag)) {
                free(tag);
                tag = 0;
            }
        }
        int double_pars = cbs->aws->get_root()->awar(cbs->awar_double_pars)->read_int();

        aw_openstatus("Pars Fields");

        AW_root         *aw_root = cbs->aws->get_root();
        AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

        for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
             !error && gb_item_container;
             gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
            {
                for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                     !error && gb_item;
                     gb_item       = cbs->selector->get_next_item(gb_item))
                    {
                        if (IS_QUERIED(gb_item,cbs)) {
                            if (aw_status((count++)/(double)ncount)) {
                                error = "Operation aborted";
                                break;
                            }
                            GBDATA *gb_new = GB_search(gb_item, key,GB_FIND);
                            char *str = 0;
                            if (gb_new) {
                                str = GB_read_as_string(gb_new);
                            }else{
                                str = strdup("");
                            }
                            char *parsed = 0;
                            if (double_pars){
                                char *com2 = 0;
                                parsed = 0;
                                com2 = GB_command_interpreter(cbs->gb_main, str,command,gb_item, cbs->tree_name);
                                if (com2){
                                    if (tag){
                                        parsed = GBS_string_eval_tagged_string(cbs->gb_main, "",deftag,tag,0,com2,gb_item);
                                    }else{
                                        parsed = GB_command_interpreter(cbs->gb_main, "",com2,gb_item, cbs->tree_name);
                                    }
                                }
                                delete com2;
                            }else{
                                if (tag){
                                    parsed = GBS_string_eval_tagged_string(cbs->gb_main, str,deftag,tag,0,command,gb_item);
                                }else{
                                    parsed = GB_command_interpreter(cbs->gb_main, str,command,gb_item, cbs->tree_name);
                                }
                            }
                            if (!parsed) {
                                error = GB_get_error();
                                free(str);
                                break;
                            }
                            if (!strcmp(parsed,str)){   // nothing changed
                                free(str);
                                free(parsed);
                                continue;
                            }

                            if (gb_new && !strlen(parsed) ) {
                                error = GB_delete(gb_new);
                            }else {
                                if (!gb_new) {
                                    gb_new = GB_search(gb_item, key, GB_read_int(gb_key_type));
                                    if (!gb_new) error = GB_get_error();
                                }
                                if (!error) {
                                    error = GB_write_as_string(gb_new,parsed);
                                }
                            }
                            free(str);
                            free(parsed);
                        }
                    }
            }


        //         GBDATA *gb_species;
        //         for (gb_species = GBT_first_species(cbs->gb_main);
        //              gb_species;
        //              gb_species = GBT_next_species(gb_species)){

        //             if (IS_QUERIED(gb_species,cbs)) {
        //                 if (aw_status((count++)/(double)ncount)) {
        //                     error = "Operation aborted";
        //                     break;
        //                 }
        //                 GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
        //                 char *str = 0;
        //                 if (gb_new) {
        //                     str = GB_read_as_string(gb_new);
        //                 }else{
        //                     str = strdup("");
        //                 }
        //                 char *parsed = 0;
        //                 if (double_pars){
        //                     char *com2 = 0;
        //                     parsed = 0;
        //                     com2 = GB_command_interpreter(cbs->gb_main, str,command,gb_species);
        //                     if (com2){
        //                         if (tag){
        //                             parsed = GBS_string_eval_tagged_string(cbs->gb_main, "",deftag,tag,0,com2,gb_species);
        //                         }else{
        //                             parsed = GB_command_interpreter(cbs->gb_main, "",com2,gb_species);
        //                         }
        //                     }
        //                     delete com2;
        //                 }else{
        //                     if (tag){
        //                         parsed = GBS_string_eval_tagged_string(cbs->gb_main, str,deftag,tag,0,command,gb_species);
        //                     }else{
        //                         parsed = GB_command_interpreter(cbs->gb_main, str,command,gb_species);
        //                     }
        //                 }
        //                 if (!parsed) {
        //                     error = GB_get_error();
        //                     delete str;
        //                     break;
        //                 }
        //                 if (!strcmp(parsed,str)){    // nothing changed
        //                     delete str;
        //                     delete parsed;
        //                     continue;
        //                 }

        //                 if (gb_new && !strlen(parsed) ) {
        //                     error = GB_delete(gb_new);
        //                 }else {
        //                     if (!gb_new) {
        //                         gb_new = GB_search(gb_species, key, GB_read_int(gb_key_type));
        //                         if (!gb_new) error = GB_get_error();
        //                     }
        //                     if (!error) {
        //                         error = GB_write_as_string(gb_new,parsed);
        //                     }
        //                 }
        //                 delete str;
        //                 delete parsed;
        //             }
        //             if (error) break;
        //         }

        aw_closestatus();
        delete tag;
        free(deftag);
    }

    if (error) {
        GB_abort_transaction(cbs->gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(cbs->gb_main);
    }
    free(key);
    free(command);
}

void awt_predef_prg(AW_root *aw_root, struct adaqbsstruct *cbs){
    char *str = aw_root->awar(cbs->awar_parspredefined)->read_string();
    char *brk = strchr(str,'#');
    if (brk) {
        *(brk++) = 0;
        char *kv = str;
        if (!strcmp(str,"ali_*/data")){
            GB_transaction valid_transaction(cbs->gb_main);
            char *use = GBT_get_default_alignment(cbs->gb_main);
            kv = GBS_global_string_copy("%s/data",use);
            free(use);
        }
        aw_root->awar(cbs->awar_parskey)->write_string( kv);
        if (kv != str) free(kv);
        aw_root->awar(cbs->awar_parsvalue)->write_string( brk);
    }else{
        aw_root->awar(cbs->awar_parsvalue)->write_string( str);
    }
    free(str);
}

char *AWT_get_item_id(GBDATA *gb_main, ad_item_selector *sel, GBDATA *gb_item) {
    return sel->generate_item_id(gb_main, gb_item);
}

GBDATA *AWT_get_item_with_id(GBDATA *gb_main, const ad_item_selector *sel, const char *id) {
    return sel->find_item_by_id(gb_main, id);
}

static void awt_colorize_listed(AW_window */*dummy*/, AW_CL cl_cbs) {
    struct adaqbsstruct *cbs         = (struct adaqbsstruct *)cl_cbs;
    GB_transaction       trans_dummy(cbs->gb_main);
    GB_ERROR             error       = 0;
    AW_root             *aw_root     = cbs->aws->get_root();
    int                  color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE      range       = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item,cbs)) {
                        error = AW_set_color_group(gb_item, color_group);
                    }
                }
        }

    if (error) GB_export_error(error);
}

struct color_mark_data {
    const ad_item_selector *sel;
    GBDATA                 *gb_main;
};

static void awt_colorize_marked(AW_window *aww, AW_CL cl_cmd) {
    const color_mark_data  *cmd         = (struct color_mark_data *)cl_cmd;
    const ad_item_selector *sel         = cmd->sel;
    GB_transaction          trans_dummy(cmd->gb_main);
    GB_ERROR                error       = 0;
    AW_root                *aw_root     = aww->get_root();
    int                     color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE         range       = AWT_QUERY_ALL_SPECIES; // @@@ FIXME: make customizable

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = sel->get_next_item(gb_item))
                {
                    if (GB_read_flag(gb_item)) {
                        error = AW_set_color_group(gb_item, color_group);
                    }
                }
        }

    if (error) GB_export_error(error);
}

// @@@ awt_mark_colored is obsolete! (will be replaced by dynamic coloring in the future)
static void awt_mark_colored(AW_window *aww, AW_CL cl_cmd, AW_CL cl_mode)
{
    const color_mark_data  *cmd         = (struct color_mark_data *)cl_cmd;
    const ad_item_selector *sel         = cmd->sel;
    int                     mode        = int(cl_mode); // 0 = unmark 1 = mark 2 = invert
    GB_ERROR                error       = 0;
    AW_root                *aw_root     = aww->get_root();
    int                     color_group = aw_root->awar(AWAR_COLORIZE)->read_int();
    AWT_QUERY_RANGE         range       = AWT_QUERY_ALL_SPECIES; // @@@ FIXME: make customizable

    GB_transaction trans_dummy(cmd->gb_main);

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = sel->get_next_item(gb_item))
                {
                    long my_color = AW_find_color_group(gb_item, true);
                    if (my_color == color_group) {
                        bool marked = GB_read_flag(gb_item);

                        switch (mode) {
                        case 0: marked = 0; break;
                        case 1: marked = 1; break;
                        case 2: marked = !marked; break;
                        default : awt_assert(0); break;
                        }

                        error = GB_write_flag(gb_item, marked);
                    }
                }
        }

    if (error) GB_export_error(error);
}

// --------------------------------------------------------------------------------
// color sets

struct color_save_data {
    color_mark_data   *cmd;
    const char        *items_name;
    AW_window         *aww; // window of selection list
    AW_selection_list *sel_id;
};

#define AWT_COLOR_LOADSAVE_NAME "tmp/colorset/name"

static GBDATA *get_colorset_root(const color_save_data *csd) {
    GBDATA *gb_main      = csd->cmd->gb_main;
    GBDATA *gb_colorsets = GB_search(gb_main, "colorsets", GB_CREATE_CONTAINER);
    GBDATA *gb_item_root = GB_search(gb_colorsets, csd->items_name, GB_CREATE_CONTAINER);

    awt_assert(gb_item_root);
    return gb_item_root;
}

static void update_colorset_selection_list(const color_save_data *csd) {
    GB_transaction  ta(csd->cmd->gb_main);
    AW_window      *aww = csd->aww;

    aww->clear_selection_list(csd->sel_id);
    GBDATA *gb_item_root = get_colorset_root(csd);

    for (GBDATA *gb_colorset = GB_find(gb_item_root, "colorset", 0, down_level);
         gb_colorset;
         gb_colorset = GB_find(gb_colorset, "colorset", 0, this_level|search_next))
    {
        GBDATA *gb_name = GB_find(gb_colorset, "name", 0, down_level);
        if (gb_name) {
            const char *name = GB_read_char_pntr(gb_name);
            aww->insert_selection(csd->sel_id, name, name);
        }
    }
    aww->insert_default_selection(csd->sel_id, "<new colorset>", "");

    aww->update_selection_list(csd->sel_id);
}

static void colorset_changed_cb(GBDATA*, int *cl_csd, GB_CB_TYPE cbt) {
    if (cbt&GB_CB_CHANGED) {
        const color_save_data *csd = (const color_save_data*)cl_csd;
        update_colorset_selection_list(csd);
    }
}

static char *create_colorset_representation(const color_save_data *csd, GB_ERROR& error) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    AWT_QUERY_RANGE         range   = AWT_QUERY_ALL_SPECIES;
    AW_root                *aw_root = csd->aww->get_root();
    GBDATA                 *gb_main = cmd->gb_main;

    typedef list<string> ColorList;
    ColorList             cl;

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = sel->get_next_item(gb_item))
        {
            long        color     = AW_find_color_group(gb_item, AW_TRUE);
            char       *id        = sel->generate_item_id(gb_main, gb_item);
            const char *color_def = GBS_global_string("%s=%li", id, color);
            cl.push_front(color_def);
            free(id);
        }
    }

    char *result = 0;
    if (cl.empty()) {
        error = GBS_global_string("Could not find any %s", sel->items_name);
    }
    else {
        string res;
        for (ColorList::iterator ci = cl.begin(); ci != cl.end(); ++ci) {
            res += (*ci) + ';';
        }

        result               = (char*)malloc(res.length());
        memcpy(result, res.c_str(), res.length()-1); // skip trailing ';'
        result[res.length()] = 0;
    }
    return result;
}

static GB_ERROR clear_all_colors(const color_save_data *csd) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    AWT_QUERY_RANGE         range   = AWT_QUERY_ALL_SPECIES;
    AW_root                *aw_root = csd->aww->get_root();
    GB_ERROR                error   = 0;

    for (GBDATA *gb_item_container = sel->get_first_item_container(cmd->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = sel->get_next_item_container(gb_item_container, range))
    {
        for (GBDATA *gb_item = sel->get_first_item(gb_item_container);
             !error && gb_item;
             gb_item = sel->get_next_item(gb_item))
        {
            AW_set_color_group(gb_item, 0); // clear colors
        }
    }

    return error;
}

static void awt_clear_all_colors(AW_window *, AW_CL cl_csd) {
    const color_save_data *csd   = (const color_save_data*)cl_csd;
    GB_transaction         ta(csd->cmd->gb_main);
    GB_ERROR               error = clear_all_colors(csd);

    if (error) {
        ta.abort();
        aw_message(error);
    }
}

static GB_ERROR restore_colorset_representation(const color_save_data *csd, const char *colorset) {
    const color_mark_data  *cmd     = csd->cmd;
    const ad_item_selector *sel     = cmd->sel;
    GBDATA                 *gb_main = cmd->gb_main;

    int   buffersize = 200;
    char *buffer     = (char*)malloc(buffersize);

    while (colorset) {
        const char *equal = strchr(colorset, '=');
        awt_assert(equal);
        const char *semi  = strchr(equal, ';');

        int size = equal-colorset;
        if (size >= buffersize) {
            buffersize = int(size*1.5);
            free(buffer);
            buffer     = (char*)malloc(buffersize);
        }

        awt_assert(buffer && buffersize>size);
        memcpy(buffer, colorset, size);
        buffer[size] = 0;       // now buffer contains the item id

        GBDATA *gb_item = AWT_get_item_with_id(gb_main, sel, buffer);
        if (!gb_item) {
            aw_message(GBS_global_string("No such %s: '%s'", sel->item_name, buffer));
        }
        else {
            int color_group = atoi(equal+1);
            AW_set_color_group(gb_item, color_group);
        }
        colorset = semi ? semi+1 : 0;
    }

    free(buffer);
    return 0;
}

static void awt_loadsave_colorset(AW_window *aws, AW_CL cl_csd, AW_CL cl_mode) {
    const color_save_data *csd     = (const color_save_data*)cl_csd;
    int              mode          = (int)cl_mode; // 0 = save; 1 = load; 2 = overlay; 3 = delete;
    AW_root               *aw_root = aws->get_root();
    char                  *name    = aw_root->awar(AWT_COLOR_LOADSAVE_NAME)->read_string();
    GB_ERROR               error   = 0;

    if (name[0] == 0) {
        error = "Please enter a name for the colorset.";
    }
    else {
        GB_transaction ta(csd->cmd->gb_main);
        GBDATA *gb_colorset_root = get_colorset_root(csd);
        awt_assert(gb_colorset_root);

        GBDATA *gb_colorset_name = GB_find(gb_colorset_root, "name", name, down_2_level);
        GBDATA *gb_colorset      = gb_colorset_name ? GB_get_father(gb_colorset_name) : 0;
        if (mode == 0) {        // save-mode
            if (!gb_colorset) { // create new (otherwise overwrite w/o comment)
                gb_colorset             = GB_create_container(gb_colorset_root, "colorset");
                if (!gb_colorset) error = GB_get_error();
                else {
                    GBDATA *gb_name = GB_create(gb_colorset, "name", GB_STRING);
                    if (!gb_name) error = GB_get_error();
                    else error = GB_write_string(gb_name, name);
                }
            }
            if (!error) {
                char *colorset = create_colorset_representation(csd, error);
                if (colorset) {
                    GBDATA *gb_set     = GB_search(gb_colorset, "color_set", GB_STRING);
                    if (!gb_set) error = GB_get_error();
                    else    error      = GB_write_string(gb_set, colorset);
                    free(colorset);
                }
            }
        }
        else { // load/delete
            if (!gb_colorset) {
                error = GBS_global_string("Colorset '%s' not found", name);
            }
            else {
                if (mode == 1 || mode == 2) { // load- and overlay-mode
                    GBDATA *gb_set = GB_find(gb_colorset, "color_set", 0, down_level);
                    if (!gb_set) {
                        error = "colorset is empty (oops)";
                    }
                    else {
                        const char *colorset = GB_read_char_pntr(gb_set);
                        if (!colorset) error = GB_get_error();
                        else {
                            if (mode == 1) { // load => clear all colors
                                error = clear_all_colors(csd);
                            }
                            if (!error) {
                                error = restore_colorset_representation(csd, colorset);
                            }
                        }
                    }
                }
                else {
                    awt_assert(mode == 3); // delete-mode
                    error = GB_delete(gb_colorset);
                }
            }
        }

        if (error) ta.abort();
    }
    free(name);

    if (error) aw_message(error);
}

static AW_window *awt_create_loadsave_colored_window(AW_root *aw_root, AW_CL cl_csd) {
    color_save_data *csd = (color_save_data*)cl_csd;

    // init data
    aw_root->awar_string(AWT_COLOR_LOADSAVE_NAME, "", AW_ROOT_DEFAULT);

    // init window
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "colorset_loadsave", GBS_global_string("Load/Save %s colorset", csd->items_name));
    aws->load_xfig("color_loadsave.fig");

    aws->at("close");
    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"color_loadsave.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("name");
    aws->create_input_field(AWT_COLOR_LOADSAVE_NAME, 60);

    awt_assert(csd->sel_id == 0);
    aws->at("list");
    csd->sel_id = aws->create_selection_list(AWT_COLOR_LOADSAVE_NAME, 0, 0, 0, 0);
    csd->aww    = aws;

    update_colorset_selection_list(csd);

    aws->at("save");
    aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)0); // 0 = save
    aws->create_button("save", "Save", "S");

    aws->at("load");
    aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)1); // 1 = load
    aws->create_button("load", "Load", "L");

    aws->at("overlay");
    aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)2); // 2 = overlay
    aws->create_button("overlay", "Overlay", "O");

    aws->at("delete");
    aws->callback(awt_loadsave_colorset, (AW_CL)csd, (AW_CL)3); // 3 = delete
    aws->create_button("delete", "Delete", "D");

    aws->at("reset");
    aws->callback(awt_clear_all_colors, (AW_CL)csd);
    aws->create_button("reset", "Reset", "R");

    // callbacks
    {
        GB_transaction  ta(csd->cmd->gb_main);
        GBDATA         *gb_colorset = get_colorset_root(csd);
        GB_add_callback(gb_colorset, GB_CB_CHANGED, colorset_changed_cb, (int*)csd);
    }

    return aws;
}

static const char *color_group_name(int color_group) {
    static char buf[30];

    if (color_group) {
        sprintf(buf, "color group %i", color_group);
    }
    else {
        strcpy(buf, "no color group");
    }

    return buf;
}
// -------------------------------------------------------------------------------------------------------------------------------------------------
//      static AW_window *create_awt_colorizer_window(AW_root *aw_root, GBDATA *gb_main, struct adaqbsstruct *cbs, const ad_item_selector *sel)
// -------------------------------------------------------------------------------------------------------------------------------------------------
// invoked by   'colorize listed'                   (sel != 0)
// and          'colorize marked/mark colored'      (cbs != 0)
//
static AW_window *create_awt_colorizer_window(AW_root *aw_root, GBDATA *gb_main, struct adaqbsstruct *cbs, const ad_item_selector *sel) {
    enum { AWT_COL_INVALID, AWT_COL_COLORIZE_LISTED, AWT_COL_COLORIZE_MARKED } mode = AWT_COL_INVALID;

    awt_query_create_global_awars(aw_root, AW_ROOT_DEFAULT);

    AW_window_simple *aws = new AW_window_simple;

    if (cbs) {
        awt_assert(mode == AWT_COL_INVALID);
        mode = AWT_COL_COLORIZE_LISTED;
    }
    if (sel) {
        awt_assert(mode == AWT_COL_INVALID);
        mode = AWT_COL_COLORIZE_MARKED;
    }
    awt_assert(!(mode == AWT_COL_INVALID));

    const ad_item_selector *Sel  = mode == AWT_COL_COLORIZE_LISTED ? cbs->selector : sel;
    const char             *what = mode == AWT_COL_COLORIZE_LISTED ? "listed" : "marked";

    {
        char *macro_name  = GBS_global_string_copy("COLORIZE_%s", Sel->items_name);
        char *window_name = GBS_global_string_copy("Colorize %s %s", what, Sel->items_name);

        aws->init( aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("colorize.fig");

    aws->auto_space(10, 10);

    aws->at("close");
    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)(mode == AWT_COL_COLORIZE_LISTED ? "set_color_of_listed.hlp" : "colorize.hlp"));
    aws->create_button("HELP","HELP","H");

    aws->at("colorize");

    color_mark_data *cmd = new color_mark_data; // do not free!
    cmd->sel             = (mode == AWT_COL_COLORIZE_MARKED) ? sel : cbs->selector;
    cmd->gb_main = gb_main;

    if (mode == AWT_COL_COLORIZE_LISTED)    aws->callback(awt_colorize_listed, (AW_CL)cbs);
    else                                    aws->callback(awt_colorize_marked, (AW_CL)cmd);

    aws->create_autosize_button("COLORIZE", GB_strdup(GBS_global_string("Set color of %s %s to ...", what, Sel->items_name)), "S", 2);

    {
        int color_group;

        aws->create_option_menu(AWAR_COLORIZE);
        aws->insert_default_option(color_group_name(0), "none", 0);
        for (color_group = 1; color_group <= AW_COLOR_GROUPS; ++color_group) {
            aws->insert_option(color_group_name(color_group), "", color_group);
        }
        aws->update_option_menu();
    }

    color_save_data *csd = new color_save_data; // do not free!
    csd->cmd             = cmd;
    csd->items_name      = Sel->items_name;
    csd->aww             = 0;
    csd->sel_id          = 0;

    aws->at("loadsave");
    aws->callback(AW_POPUP, (AW_CL)awt_create_loadsave_colored_window, (AW_CL)csd);
    aws->create_autosize_button("LOADSAVE_COLORED", "Load/Save", "L");

    if (mode == AWT_COL_COLORIZE_MARKED) {
        aws->at("mark");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)1);
        aws->create_autosize_button("MARK_COLORED", GB_strdup(GBS_global_string("Mark all %s of ...", Sel->items_name)), "M", 2);

        aws->at("unmark");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)0);
        aws->create_autosize_button("UNMARK_COLORED", GB_strdup(GBS_global_string("Unmark all %s of ...", Sel->items_name)), "U", 2);

        aws->at("invert");
        aws->callback(awt_mark_colored, (AW_CL)cmd, (AW_CL)2);
        aws->create_autosize_button("INVERT_COLORED", GB_strdup(GBS_global_string("Invert all %s of ...", Sel->items_name)), "I", 2);
    }

    aws->at_newline();
    aws->window_fit();

    return aws;
}

AW_window *create_awt_listed_items_colorizer(AW_root *aw_root, struct adaqbsstruct *cbs) {
    return create_awt_colorizer_window(aw_root, cbs->gb_main, cbs, 0);
}

AW_window *awt_create_item_colorizer(AW_root *aw_root, GBDATA *gb_main, const ad_item_selector *sel) {
    return create_awt_colorizer_window(aw_root, gb_main, 0, sel);
}

//     AW_window_simple *aws = new AW_window_simple;

//     {
//         char *macro_name = GB_strdup(GBS_global_string("COLORIZE_%s", sel->items_name));
//         char *title      = GB_strdup(GBS_global_string("Colorize and mark %s", sel->items_name));

//         aws->init(aw_root, macro_name, title, 100, 100);

//         free(title);
//         free(macro_name);
//     }

//     aws->load_xfig("colorize.fig");

//     aws->at("close");
//     aws->callback((AW_CB0)AW_POPDOWN);
//     aws->create_button("CLOSE","CLOSE","C");

//     aws->at("help");
//     aws->callback( AW_POPUP_HELP,(AW_CL)"colorize_items.hlp");
//     aws->create_button("HELP","HELP","H");

// #if 0
//     aws->at("color");
//     aws->create_option_menu(AWAR_CURRENT_COLOR);
//     for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
//         aws->insert_option(AW_get_color_group_name(aw_root, i), 0, i);
//     }
//     aws->update_option_menu();
// #endif

//     return aws;
// }


AW_window *create_awt_open_parser(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws                   = new AW_window_simple;

    {
        char *macro_name = GBS_global_string_copy("MODIFY_DATABASE_FIELD_%s", cbs->selector->items_name);
        char *window_name = GBS_global_string_copy("MODIFY DATABASE FIELD of listed %s", cbs->selector->items_name);

        aws->init( aw_root, macro_name, window_name);

        free(window_name);
        free(macro_name);
    }

    aws->load_xfig("awt/parser.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mod_field_list.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("helptags");
    aws->callback(AW_POPUP_HELP,(AW_CL)"tags.hlp");
    aws->create_button("HELP_TAGS", "HELP TAGS","H");

    aws->at("usetag");  aws->create_toggle(cbs->awar_use_tag);
    aws->at("deftag");  aws->create_input_field(cbs->awar_deftag);
    aws->at("tag");     aws->create_input_field(cbs->awar_tag);

    aws->at("double");  aws->create_toggle(cbs->awar_double_pars);

    awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_parskey,AWT_PARS_FILTER, "field",0, cbs->selector, 20, 10);

    aws->at("go");
    aws->callback((AW_CB1)awt_do_pars_list,(AW_CL)cbs);
    aws->create_button("GO", "GO","G");

    aws->at("parser");
    aws->create_text_field(cbs->awar_parsvalue);

    aws->at("pre");
    AW_selection_list *id = aws->create_selection_list(cbs->awar_parspredefined);

    char *filename = 0;
    switch (cbs->selector->type) {
    case AWT_QUERY_ITEM_SPECIES:
        filename = AWT_unfold_path("lib/sellists/mod_fields*.sellst","ARBHOME");
        break;
    case AWT_QUERY_ITEM_GENES:
        filename = AWT_unfold_path("lib/sellists/mod_gene_fields*.sellst","ARBHOME");
        break;
    case AWT_QUERY_ITEM_EXPERIMENTS:
        filename = AWT_unfold_path("lib/sellists/mod_experiment_fields*.sellst","ARBHOME");
        break;
    default:
        gb_assert(0);
        break;
    }

    GB_ERROR error = filename ? aws->load_selection_list(id,filename) : "No default selection list for query-type";
    free(filename);
    if (error) {
        aw_message(error);
    }
    else{
        aws->get_root()->awar(cbs->awar_parspredefined)->add_callback((AW_RCB1)awt_predef_prg,(AW_CL)cbs);
    }
    return (AW_window *)aws;
}


/***************** Multi set fields *************************/

void awt_do_set_list(void *dummy, struct adaqbsstruct *cbs, long append)
{
    AWUSE(dummy);

    GB_ERROR error = 0;

    char *key = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();
    if (!strcmp(key,"name")) error = "You cannot set the name field";

    char *value = cbs->aws->get_root()->awar(cbs->awar_setvalue)->read_string();
    if (!strlen(value)) {
        free(value);
        value = 0;
    }


    GB_begin_transaction(cbs->gb_main);
    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key_name = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level);
    if (!gb_key_name) {
        sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
        aw_message();
        delete value;
        free(key);
        GB_commit_transaction(cbs->gb_main);
        return;
    }

    GBDATA *gb_key_type = GB_find(gb_key_name,CHANGEKEY_TYPE,0,this_level);


    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, cbs->aws->get_root(), AWT_QUERY_ALL_SPECIES);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, AWT_QUERY_ALL_SPECIES))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item,cbs)) {
                        GBDATA *gb_new = GB_search(gb_item, key,GB_FIND);
                        if (gb_new) {
                            if (value){
                                if (append){
                                    char *old = GB_read_as_string(gb_new);
                                    if (old){
                                        void *strstr = GBS_stropen(strlen(old)+strlen(value)+2);
                                        GBS_strcat(strstr,old);
                                        GBS_strcat(strstr,value);
                                        char *v = GBS_strclose(strstr);
                                        error = GB_write_as_string(gb_new,v);
                                        free(v);
                                    }else{
                                        char *name = GBT_read_string(gb_item,"name");
                                        error = GB_export_error("Field '%s' of %s '%s' has incombatible type", key, cbs->selector->item_name, name);
                                        free(name);
                                    }
                                }else{
                                    error = GB_write_as_string(gb_new,value);
                                }
                            }else{
                                if (!append){
                                    error = GB_delete(gb_new);
                                }
                            }
                        }else {
                            gb_new = GB_search(gb_item, key, GB_read_int(gb_key_type));
                            if (!gb_new) error = GB_get_error();
                            if (!error) error = GB_write_as_string(gb_new,value);
                        }
                    }
                }
        }

    //     GBDATA         *gb_species;
    //     for (gb_species = GBT_first_species(cbs->gb_main);
    //          !error &&  gb_species;
    //          gb_species = GBT_next_species(gb_species)){

    //         if (IS_QUERIED(gb_species,cbs)) {
    //             GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
    //             if (gb_new) {
    //                 if (value){
    //                     if (append){
    //                         char *old = GB_read_as_string(gb_new);
    //                         if (old){
    //                             void *strstr = GBS_stropen(1024);
    //                             GBS_strcat(strstr,old);
    //                             GBS_strcat(strstr,value);
    //                             char *v = GBS_strclose(strstr);
    //                             error = GB_write_as_string(gb_new,v);
    //                             delete v;
    //                         }else{
    //                             char *name = GBT_read_string(gb_species,"name");
    //                             error = GB_export_error("Field '%s' of species '%s' has incombatible type",key,name);
    //                             delete name;
    //                         }
    //                     }else{

    //                         error = GB_write_as_string(gb_new,value);
    //                     }
    //                 }else{
    //                     if (!append){
    //                         error = GB_delete(gb_new);
    //                     }
    //                 }
    //             }else {
    //                 gb_new = GB_search(gb_species, key, GB_read_int(gb_key_type));
    //                 if (!gb_new){
    //                     error = GB_get_error();
    //                 }

    //                 if (!error) {
    //                     error = GB_write_as_string(gb_new,value);
    //                 }
    //             }
    //         }
    //     }

    if (error) {
        GB_abort_transaction(cbs->gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(cbs->gb_main);
    }
    free(key);
    free(value);
}

AW_window *create_awt_do_set_list(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws = new AW_window_simple;
    aws->init( aw_root, "SET_DATABASE_FIELD_OF_LISTED","SET MANY FIELDS");
    aws->load_xfig("ad_mset.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"write_field_list.hlp");
    aws->create_button("HELP", "HELP","H");

    awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_setkey, AWT_NDS_FILTER, "box",0, cbs->selector, 20, 10);
    aws->at("create");
    aws->callback((AW_CB)awt_do_set_list,(AW_CL)cbs,0);
    aws->create_button("SET_SINGLE_FIELD_OF_LISTED","WRITE");
    aws->at("do");
    aws->callback((AW_CB)awt_do_set_list,(AW_CL)cbs,1);
    aws->create_button("APPEND_SINGLE_FIELD_OF_LISTED","APPEND");

    aws->at("val");
    aws->create_text_field(cbs->awar_setvalue,2,2);
    return (AW_window *)aws;

}

/***************** Multi set fields *************************/

void awt_do_set_protection(void *dummy, struct adaqbsstruct *cbs)
{
    AWUSE(dummy);

    char *key = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();

    GB_begin_transaction(cbs->gb_main);
    GBDATA *gb_key_data = GB_search(cbs->gb_main, cbs->selector->change_key_path, GB_CREATE_CONTAINER);
    GBDATA *gb_key_name = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level);
    if (!gb_key_name) {
        sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
        aw_message();
        delete key;
        GB_commit_transaction(cbs->gb_main);
        return;
    }
    int              level   = cbs->aws->get_root()->awar(cbs->awar_setprotection)->read_int();
    GB_ERROR         error   = 0;
    AW_root         *aw_root = cbs->aws->get_root();
    AWT_QUERY_RANGE  range   = (AWT_QUERY_RANGE)aw_root->awar(cbs->awar_where)->read_int();

    for (GBDATA *gb_item_container = cbs->selector->get_first_item_container(cbs->gb_main, aw_root, range);
         !error && gb_item_container;
         gb_item_container = cbs->selector->get_next_item_container(gb_item_container, range))
        {
            for (GBDATA *gb_item = cbs->selector->get_first_item(gb_item_container);
                 !error && gb_item;
                 gb_item       = cbs->selector->get_next_item(gb_item))
                {
                    if (IS_QUERIED(gb_item,cbs)) {
                        GBDATA *gb_new = GB_search(gb_item, key,GB_FIND);
                        if (!gb_new) continue;
                        error = GB_write_security_delete(gb_new,level);
                        error = GB_write_security_write(gb_new,level);
                    }
                }
        }


    //  GBDATA *gb_species;

    //  for (gb_species = GBT_first_species(cbs->gb_main);
    //       gb_species;
    //       gb_species = GBT_next_species(gb_species)){

    //      if (IS_QUERIED(gb_species,cbs)) {
    //          GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
    //          if (!gb_new) continue;
    //          error = GB_write_security_delete(gb_new,level);
    //          error = GB_write_security_write(gb_new,level);
    //      }
    //      if (error) break;
    //  }

    if (error) {
        GB_abort_transaction(cbs->gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(cbs->gb_main);
    }
    free(key);
}

AW_window *create_awt_set_protection(AW_root *aw_root, struct adaqbsstruct *cbs)
{
    AW_window_simple *aws = 0;
    aws = new AW_window_simple;
    aws->init( aw_root, "SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTIONS OF FIELDS");
    aws->load_xfig("awt/set_protection.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"set_protection.hlp");
    aws->create_button("HELP", "HELP","H");


    aws->at("prot");
    aws->create_toggle_field(cbs->awar_setprotection,0);
    aws->insert_toggle("0 Temporary","0",0);
    aws->insert_toggle("1 Checked","1",1);
    aws->insert_toggle("2","2",2);
    aws->insert_toggle("3","3",3);
    aws->insert_toggle("4 normal","4",4);
    aws->insert_toggle("5 ","5",5);
    aws->insert_toggle("6 the truth","5",6);

    awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_setkey, AWT_NDS_FILTER, "list",0, cbs->selector, 20, 10);

    aws->at("go");
    aws->callback((AW_CB1)awt_do_set_protection,(AW_CL)cbs);
    aws->create_button("SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTION");

    return (AW_window *)aws;
}

void awt_toggle_flag(AW_window *aww, struct adaqbsstruct *cbs) {
    GB_transaction dummy(cbs->gb_main);
    GBDATA *gb_item = cbs->selector->get_selected_item(cbs->gb_main, aww->get_root());
    if (gb_item) {
        long flag = GB_read_flag(gb_item);
        GB_write_flag(gb_item, 1-flag);
    }
    awt_query_update_list(aww,cbs);

    //  GB_transaction dummy(cbs->gb_main);
    //  char *sname = aww->get_root()->awar(cbs->species_name)->read_string();
    //  if (sname[0]){
    //      GBDATA *gb_species = GBT_find_species(cbs->gb_main,sname);
    //      if (gb_species) {
    //          long flag = GB_read_flag(gb_species);
    //          GB_write_flag(gb_species,1-flag);
    //      }
    //  }

    //  delete sname;
    //  awt_query_update_list(aww,cbs);
}

//  -----------------------------------------------------
//      struct ad_item_selector AWT_species_selector
//  -----------------------------------------------------

static GBDATA *awt_get_first_species_data(GBDATA *gb_main, AW_root *, AWT_QUERY_RANGE) {
    return GBT_get_species_data(gb_main);
}
static GBDATA *awt_get_next_species_data(GBDATA *, AWT_QUERY_RANGE) {
    return 0; // there is only ONE species_data
}

static void awt_select_species(GBDATA* , AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES_NAME)->write_string(item_name);
}

static GBDATA* awt_get_selected_species(GBDATA *gb_main, AW_root *aw_root) {
    char   *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species   = 0;
    if (species_name[0]) {
        gb_species = GBT_find_species(gb_main, species_name);
    }
    free(species_name);
    return gb_species;
}

static char* awt_get_species_id(GBDATA *, GBDATA *gb_species) {
    // awt_get_species_id creates the label that occurs in the search and query result list
    GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
    if (!gb_name) return 0;     // species w/o name -> skip
    return GB_read_as_string(gb_name);
}

static GBDATA *awt_find_species_by_id(GBDATA *gb_main, const char *id) {
    return GBT_find_species(gb_main, id); // id is 'name' field
}

struct ad_item_selector AWT_species_selector = {
    AWT_QUERY_ITEM_SPECIES,
    awt_select_species,
    awt_get_species_id,
    awt_find_species_by_id,
    (AW_CB)awt_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "species",
    "species",
    awt_get_first_species_data,
    awt_get_next_species_data,
    GBT_first_species_rel_species_data,
    GBT_next_species,
    awt_get_selected_species,
    0
};

struct ad_item_selector AWT_organism_selector = {
    AWT_QUERY_ITEM_SPECIES,
    awt_select_species,
    awt_get_species_id,
    awt_find_species_by_id,
    (AW_CB)awt_selection_list_update_cb,
    12,
    CHANGE_KEY_PATH,
    "organism",
    "organism",
    awt_get_first_species_data,
    awt_get_next_species_data,
    GBT_first_species_rel_species_data,
    GBT_next_species,
    awt_get_selected_species,
    0
};

static void awt_new_selection_made(AW_root *aw_root, AW_CL cl_awar_selection, AW_CL cl_cbs) {
    const char          *awar_selection = (const char *)cl_awar_selection;
    struct adaqbsstruct *cbs            = (struct adaqbsstruct *)cl_cbs;

    char *item_name = aw_root->awar(awar_selection)->read_as_string();
    cbs->selector->update_item_awars(cbs->gb_main, aw_root, item_name);
    free(item_name);
}

static void query_box_init_config(AWT_config_definition& cdef, struct adaqbsstruct *cbs) { // this defines what is saved/restored to/from configs
    //  don't save these
    //     cdef.add(cbs->awar_ere, "action");
    //     cdef.add(cbs->awar_where, "range");
    //     cdef.add(cbs->awar_by, "by");

    for (int key_id = 0; key_id<AWT_QUERY_SEARCHES; ++key_id) {
        cdef.add(cbs->awar_keys[key_id], "key", key_id);
        cdef.add(cbs->awar_queries[key_id], "query", key_id);
        cdef.add(cbs->awar_not[key_id], "not", key_id);
        cdef.add(cbs->awar_operator[key_id], "operator", key_id);
    }
}
static char *query_box_store_config(AW_window *aww, AW_CL cl_cbs, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    query_box_init_config(cdef, (struct adaqbsstruct *)cl_cbs);
    return cdef.read();
}
static void query_box_restore_config(AW_window *aww, const char *stored, AW_CL cl_cbs, AW_CL ) {
    AWT_config_definition cdef(aww->get_root());
    query_box_init_config(cdef, (struct adaqbsstruct *)cl_cbs);
    cdef.write(stored);
}


typedef AW_window *(*window_generator)(AW_root *);

static void query_box_popup_view_window(AW_window *aww, AW_CL cl_create_window, AW_CL) {
    window_generator  create_window = (window_generator)cl_create_window;
    AW_window        *aw_viewer     = create_window(aww->get_root());
    aw_viewer->show();
}

/***************** Create the database query box and functions *************************/
struct adaqbsstruct *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs) // create the query box
{
    static int query_id = 0;

    char                 buffer[256];
    AW_root             *aw_root = aws->get_root();
    GBDATA              *gb_main = awtqs->gb_main;
    struct adaqbsstruct *cbs     = (struct adaqbsstruct *)calloc(1, sizeof(struct adaqbsstruct));

    cbs->gb_main          = awtqs->gb_main;
    cbs->aws              = aws;
    cbs->gb_ref           = awtqs->gb_ref;
    cbs->look_in_ref_list = awtqs->look_in_ref_list;
    cbs->select_bit       = awtqs->select_bit;
    cbs->species_name     = strdup(awtqs->species_name);
    cbs->tree_name        = awtqs->tree_name ? aw_root->awar(awtqs->tree_name)->read_string() : 0;
    cbs->selector         = awtqs->selector;

    GB_push_transaction(gb_main);
    /*************** Create local AWARS *******************/

    awt_query_create_global_awars(aw_root, AW_ROOT_DEFAULT);

    {
        const char *default_key[AWT_QUERY_SEARCHES+1] = { "name", "name", "name", 0};

        for (int key_id = 0; key_id<AWT_QUERY_SEARCHES; ++key_id) {
            sprintf(buffer,"tmp/arbdb_query_%i/key_%i",query_id, key_id);
            cbs->awar_keys[key_id] = strdup(buffer);
            awt_assert(default_key[key_id] != 0);
            aw_root->awar_string( cbs->awar_keys[key_id], default_key[key_id], AW_ROOT_DEFAULT);

            sprintf(buffer,"tmp/arbdb_query_%i/query_%i",query_id, key_id);
            cbs->awar_queries[key_id] = strdup(buffer);
            aw_root->awar_string( cbs->awar_queries[key_id], "*", AW_ROOT_DEFAULT);

            sprintf(buffer,"tmp/arbdb_query_%i/not_%i",query_id, key_id);
            cbs->awar_not[key_id] = strdup(buffer);
            aw_root->awar_int( cbs->awar_not[key_id], 0, AW_ROOT_DEFAULT);

            sprintf(buffer,"tmp/arbdb_query_%i/operator_%i",query_id, key_id);
            cbs->awar_operator[key_id] = strdup(buffer);
            aw_root->awar_string( cbs->awar_operator[key_id], "ign", AW_ROOT_DEFAULT);
        }
    }

    sprintf(buffer,"tmp/arbdb_query_%i/ere",query_id);
    cbs->awar_ere = strdup(buffer);
    aw_root->awar_int( cbs->awar_ere, AWT_QUERY_GENERATE);

    sprintf(buffer,"tmp/arbdb_query_%i/where",query_id);
    cbs->awar_where = strdup(buffer);
    aw_root->awar_int( cbs->awar_where, AWT_QUERY_ALL_SPECIES);

    sprintf(buffer,"tmp/arbdb_query_%i/count",query_id);
    cbs->awar_count = strdup(buffer);
    aw_root->awar_int( cbs->awar_count, 0);

    sprintf(buffer,"tmp/arbdb_query_%i/by",query_id);
    cbs->awar_by = strdup(buffer);
    aw_root->awar_int( cbs->awar_by, AWT_QUERY_MATCH);

    if (awtqs->ere_pos_fig){
        aws->at(awtqs->ere_pos_fig);
        aws->create_toggle_field(cbs->awar_ere,"","");

        aws->insert_toggle(GBS_global_string("Search %s", cbs->selector->items_name),"G",(int)AWT_QUERY_GENERATE);
        aws->insert_toggle(GBS_global_string("Add %s", cbs->selector->items_name),"E",(int)AWT_QUERY_ENLARGE);
        aws->insert_toggle(GBS_global_string("Keep %s", cbs->selector->items_name),"R",(int)AWT_QUERY_REDUCE);

        aws->update_toggle_field();
    }
    if (awtqs->where_pos_fig) {
        aws->at(awtqs->where_pos_fig);
        aws->create_toggle_field(cbs->awar_where,"","");
        aws->insert_toggle("of current organism","C",(int)AWT_QUERY_CURRENT_SPECIES);
        aws->insert_toggle("of marked organisms","M",(int)AWT_QUERY_MARKED_SPECIES);
        aws->insert_toggle("of all organisms","A",(int)AWT_QUERY_ALL_SPECIES);
        aws->update_toggle_field();

    }
    if (awtqs->by_pos_fig){
        aws->at(awtqs->by_pos_fig);
        aws->create_toggle_field(cbs->awar_by,"","");
        aws->insert_toggle("that match the query","M",(int)AWT_QUERY_MATCH);
        aws->insert_toggle("that dont match the q.","D",(int)AWT_QUERY_DONT_MATCH);
        aws->insert_toggle("that are marked","A",(int)AWT_QUERY_MARKED);
        aws->update_toggle_field();
    }

    // distances for multiple queries :

#define KEY_Y_OFFSET 32

    int xpos_calc[3] = { -1, -1, -1 }; // X-positions for elements in search expressions

    if (awtqs->qbox_pos_fig){
        AW_at_size at_size;
        int        xpos, ypos;

        aws->auto_space(1, 1);

        aws->at(awtqs->qbox_pos_fig);
        aws->store_at_size_and_attach(&at_size);
        aws->get_at_position(&xpos, &ypos);

        int ypos_dummy;

        for (int key = AWT_QUERY_SEARCHES-1;  key >= 0; --key) {
            if (key) {
                aws->at(xpos, ypos+key*KEY_Y_OFFSET);
                aws->create_option_menu(cbs->awar_operator[key], 0, "");
                aws->insert_option("and", "", "and");
                aws->insert_option("or", "", "or");
                aws->insert_option("ign", "", "ign");
                aws->update_option_menu();

                if (xpos_calc[0] == -1) aws->get_at_position(&xpos_calc[0], &ypos_dummy);
            }

            aws->at(xpos_calc[0], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_size_and_attach(&at_size);
            awt_create_selection_list_on_scandb(gb_main,aws,cbs->awar_keys[key], AWT_NDS_FILTER,
                                                0, awtqs->rescan_pos_fig,
                                                awtqs->selector, 22, 20, true, true);

            if (xpos_calc[1] == -1) aws->get_at_position(&xpos_calc[1], &ypos_dummy);

            aws->at(xpos_calc[1], ypos+key*KEY_Y_OFFSET);
            aws->create_toggle(cbs->awar_not[key], "matches.bitmap", "not.bitmap");

            if (xpos_calc[2] == -1) aws->get_at_position(&xpos_calc[2], &ypos_dummy);
        }
    }
    if (awtqs->key_pos_fig){
        aws->at(awtqs->key_pos_fig);
        aws->create_input_field(cbs->awar_keys[0],12);
    }

    if (awtqs->query_pos_fig){
        aws->at(awtqs->query_pos_fig);

        AW_at_size at_size;
        int        xpos, ypos;
        aws->store_at_size_and_attach(&at_size);
        aws->get_at_position(&xpos, &ypos);

        for (int key = 0; key<AWT_QUERY_SEARCHES; ++key) {
            aws->at(xpos_calc[2], ypos+key*KEY_Y_OFFSET);
            aws->restore_at_size_and_attach(&at_size);
            aws->d_callback((AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_NONE); // enable ENTER in searchfield to start search
            aws->create_input_field(cbs->awar_queries[key],12);
        }
    }

    if (awtqs->result_pos_fig){
        aws->at(awtqs->result_pos_fig);
        if(awtqs->create_view_window) {
            aws->callback(query_box_popup_view_window,(AW_CL)awtqs->create_view_window, 0);
        }
        aws->d_callback((AW_CB1)awt_toggle_flag,(AW_CL)cbs);

        {
            char    *this_awar_name = GBS_global_string_copy("tmp/arbdb_query_%i/select", query_id);
            AW_awar *awar           = aw_root->awar_string( this_awar_name, "", AW_ROOT_DEFAULT);

            cbs->result_id = aws->create_selection_list(this_awar_name,"","",5,5);
            awar->add_callback(awt_new_selection_made, (AW_CL)this_awar_name, (AW_CL)cbs);

            //free(this_awar_name); do not free this, cause it's passed to awt_new_selection_made
        }
        aws->insert_default_selection( cbs->result_id, "end of list", "" );
        aws->update_selection_list( cbs->result_id );
    }
    if (awtqs->count_pos_fig){
        aws->at(awtqs->count_pos_fig);
        aws->label("Hits:");
        aws->create_button(0,cbs->awar_count);
    }

    if (awtqs->config_pos_fig){
        aws->button_length(0);
        aws->at(awtqs->config_pos_fig);
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "query_box", query_box_store_config, query_box_restore_config, (AW_CL)cbs, 0);
    }
    aws->button_length(19);
    if (awtqs->do_query_pos_fig){
        aws->at(awtqs->do_query_pos_fig);
        aws->callback((AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_NONE);
        aws->highlight();
        aws->create_button("SEARCH", "SEARCH","Q");
    }
    if (awtqs->do_refresh_pos_fig && !GB_NOVICE){
        aws->at(awtqs->do_refresh_pos_fig);
        aws->callback((AW_CB1)awt_query_update_list,(AW_CL)cbs);
        aws->create_button("REFRESH_HITLIST", "REFRESH","R");
    }
    if (awtqs->do_mark_pos_fig){
        aws->at(awtqs->do_mark_pos_fig);
        aws->help_text("mark_list.hlp");
        aws->callback((AW_CB)awt_do_mark_list,(AW_CL)cbs,1);
        aws->create_button("MARK_LISTED_UNMARK_REST", "MARK LISTED\nUNMARK REST","M");
    }
    if (awtqs->do_unmark_pos_fig){
        aws->at(awtqs->do_unmark_pos_fig);
        aws->help_text("unmark_list.hlp");
        aws->callback((AW_CB)awt_do_mark_list,(AW_CL)cbs,0);
        aws->create_button("UNMARK_LISTED_MARK_REST","UNMARK LISTED\nMARK REST","U");
    }
    if (awtqs->do_delete_pos_fig){
        aws->at(awtqs->do_delete_pos_fig);
        aws->help_text("del_list.hlp");
        aws->callback((AW_CB)awt_delete_species_in_list,(AW_CL)cbs,0);
        aws->create_button("DELETE_LISTED","DELETE LISTED","D");
    }
    if (awtqs->do_set_pos_fig && !GB_NOVICE){
        sprintf(buffer,"tmp/arbdb_query_%i/set_key",query_id);
        cbs->awar_setkey = strdup(buffer);
        aw_root->awar_string( cbs->awar_setkey);

        sprintf(buffer,"tmp/arbdb_query_%i/set_protection",query_id);
        cbs->awar_setprotection = strdup(buffer);
        aw_root->awar_int( cbs->awar_setprotection, 4);

        sprintf(buffer,"tmp/arbdb_query_%i/set_value",query_id);
        cbs->awar_setvalue = strdup(buffer);
        aw_root->awar_string( cbs->awar_setvalue);

        aws->at(awtqs->do_set_pos_fig);
        aws->help_text("mod_field_list.hlp");
        aws->callback(AW_POPUP,(AW_CL)create_awt_do_set_list,(AW_CL)cbs);
        aws->create_button("WRITE_TO_FIELDS_OF_LISTED", "WRITE TO FIELDS\nOF LISTED","S");
    }

    char *Items = strdup(cbs->selector->items_name);
    Items[0]    = toupper(Items[0]);

    if ( ( awtqs->use_menu || awtqs->open_parser_pos_fig) && !GB_NOVICE){
        sprintf(buffer,"tmp/arbdb_query_%i/tag",query_id);      cbs->awar_tag = strdup(buffer);     aw_root->awar_string( cbs->awar_tag);
        sprintf(buffer,"tmp/arbdb_query_%i/use_tag",query_id);      cbs->awar_use_tag = strdup(buffer); aw_root->awar_int( cbs->awar_use_tag);
        sprintf(buffer,"tmp/arbdb_query_%i/deftag",query_id);       cbs->awar_deftag = strdup(buffer);  aw_root->awar_string( cbs->awar_deftag);
        sprintf(buffer,"tmp/arbdb_query_%i/double_pars",query_id);  cbs->awar_double_pars = strdup(buffer); aw_root->awar_int( cbs->awar_double_pars);
        sprintf(buffer,"tmp/arbdb_query_%i/parskey",query_id);      cbs->awar_parskey = strdup(buffer); aw_root->awar_string( cbs->awar_parskey);
        sprintf(buffer,"tmp/arbdb_query_%i/parsvalue",query_id);    cbs->awar_parsvalue = strdup(buffer);   aw_root->awar_string( cbs->awar_parsvalue);
        sprintf(buffer,"tmp/arbdb_query_%i/awar_parspredefined",query_id);cbs->awar_parspredefined = strdup(buffer);aw_root->awar_string( cbs->awar_parspredefined);

        if (awtqs->use_menu){
            sprintf(buffer, "Modify Fields of Listed %s", Items); aws->insert_menu_topic("mod_fields_of_listed",buffer,"F","mod_field_list.hlp",-1,AW_POPUP,(AW_CL)create_awt_open_parser,(AW_CL)cbs);
        }else{
            aws->at(awtqs->open_parser_pos_fig);
            aws->callback(AW_POPUP,(AW_CL)create_awt_open_parser,(AW_CL)cbs);
            aws->create_button("MODIFY_FIELDS_OF_LISTED", "MODIFY FIELDS\nOF LISTED","F");
        }
    }
    if (awtqs->use_menu && !GB_NOVICE){
        sprintf(buffer, "Set Protection of Fields of Listed %s", Items); aws->insert_menu_topic("s_prot_of_listed",buffer,"P","set_protection.hlp",-1,AW_POPUP,(AW_CL)create_awt_set_protection,(AW_CL)cbs);
        aws->insert_separator();
        sprintf(buffer, "Mark Listed %s, don't Change Rest", Items);    aws->insert_menu_topic("mark_listed", buffer,"M","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)1 | 8);
        sprintf(buffer, "Mark Listed %s, Unmark Rest", Items);          aws->insert_menu_topic("mark_listed_unmark_rest", buffer, "L","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)1);
        sprintf(buffer, "Unmark Listed %s, don't Change Rest", Items);  aws->insert_menu_topic("unmark_listed", buffer,"U","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)0 | 8);
        sprintf(buffer, "Unmark Listed %s, Mark Rest", Items);          aws->insert_menu_topic("unmark_listed_mark_rest", buffer,"R","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)0);
        aws->insert_separator();


        sprintf(buffer, "Set Color of Listed %s", Items);    aws->insert_menu_topic("set_color_of_listed", buffer,"C","set_color_of_listed.hlp",-1,AW_POPUP, (AW_CL)create_awt_listed_items_colorizer, (AW_CL)cbs);

        if (cbs->gb_ref){
            awt_assert(cbs->selector->type == AWT_QUERY_ITEM_SPECIES); // stuff below works only for species
            aws->insert_separator();
            if (cbs->look_in_ref_list){
                aws->insert_menu_topic("search_equal_fields_and_listed_in_I",   "Search Species with an Entry Which Exists in Both Databases and are Listed in the Database I hitlist","S",
                                       "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_LINES);
                aws->insert_menu_topic("search_equal_words_and_listed_in_I","Search Species with an Entry Word Which Exists in Both Databases and are Listed in the Database I hitlist","W",
                                       "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_WORDS);
            }else{
                aws->insert_menu_topic("search_equal_field_in_both_db", "Search Species with an Entry Which Exists in Both Databases","S",
                                       "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_LINES);
                aws->insert_menu_topic("search_equal_word_in_both_db","Search Species with an Entry Word Which Exists in Both Databases","W",
                                       "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_WORDS);
            }
        }
    }

    free(Items);

    query_id++;
    GB_pop_transaction(gb_main);
    return cbs;
}
