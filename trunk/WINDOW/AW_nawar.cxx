#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// #include <fcntl.h>

#include <arbdb.h>
#include "aw_root.hxx"
#include "aw_nawar.hxx"
#include "awt.hxx"
#define AWAR_EPS 0.00000001

#if defined(DEBUG)
// uncomment next line to dump all awar-changes to stderr
// #define DUMP_AWAR_CHANGES
#endif // DEBUG

AW_var_target::AW_var_target(void* pntr, AW_var_target *nexti){
    next    = nexti;
    pointer = pntr;
}

AW_var_callback::AW_var_callback( void (*vc_cb)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2, AW_var_callback *nexti ) {
    value_changed_cb     = vc_cb;
    value_changed_cb_cd1 = cd1;
    value_changed_cb_cd2 = cd2;
    next                 = nexti;
}


void AW_var_callback::run_callback(AW_root *root) {
    if (this->next) this->next->run_callback(root);     // callback the whole list
    if (!this->value_changed_cb) return;
    this->value_changed_cb(root,this->value_changed_cb_cd1,this->value_changed_cb_cd2);
}

void AW_var_gbdata_callback_delete_intern(GBDATA *, int *cl) {
    AW_awar *awar = (AW_awar *)cl;
    awar->gb_var = 0;
    awar->update();
}

extern "C"
void AW_var_gbdata_callback(GBDATA *, int *cl, GB_CB_TYPE) {
    AW_awar *awar = (AW_awar *)cl;
    awar->update();
}

extern "C"
void AW_var_gbdata_callback_delete(GBDATA *gbd, int *cl, GB_CB_TYPE) {
    AW_var_gbdata_callback_delete_intern(gbd, cl);
}

GB_ERROR AW_MSG_UNMAPPED_AWAR = "Error (unmapped AWAR):\n"
    "You cannot write to this field because it is either deleted or\n"
    "unmapped. Try to select a different item, reselect this and retry.";

char *AW_awar::read_as_string( void ) {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

char *AW_awar::read_string(){
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

const char *AW_awar::read_char_pntr(){
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}

long AW_awar::read_int(){
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int( gb_var );
}
double AW_awar::read_float(){
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}

#if defined(DUMP_AWAR_CHANGES)
#define AWAR_CHANGE_DUMP(name, where, format) fprintf(stderr, "change awar '%s' " where "(" format ")\n", name, para)
#else
#define AWAR_CHANGE_DUMP(name, where, format)
#endif // DEBUG

#define concat(x, y) x##y

#define WRITE_SKELETON(self, type, format, func)        \
    GB_ERROR AW_awar::self(type para) {                 \
        if (!gb_var) return AW_MSG_UNMAPPED_AWAR;       \
        GB_transaction ta(gb_var);                      \
        AWAR_CHANGE_DUMP(awar_name, #self, format);     \
        return func(gb_var, para);                      \
    }                                                   \
    GB_ERROR AW_awar::concat(re, self)(type para) {     \
        if (!gb_var) return AW_MSG_UNMAPPED_AWAR;       \
        GB_transaction ta(gb_var);                      \
        AWAR_CHANGE_DUMP(awar_name, #self, format);     \
        GB_ERROR       error = func(gb_var, para);      \
        GB_touch(gb_var);                               \
        return error;                                   \
    }

WRITE_SKELETON(write_string, const char*, "%s", GB_write_string) // defines rewrite_string
    WRITE_SKELETON(write_int, long, "%li", GB_write_int) // defines rewrite_int
    WRITE_SKELETON(write_float, double, "%f", GB_write_float) // defines rewrite_float
    WRITE_SKELETON(write_as_string, const char *, "%s", GB_write_as_string) // defines rewrite_as_string

#undef WRITE_SKELETON
#undef concat
#undef AWAR_CHANGE_DUMP


    void AW_awar::touch( void ) {
    if (!gb_var) {
        return;
    }
    GB_transaction dummy(gb_var);
    GB_touch( gb_var );
}

AW_default aw_main_root_default = (AW_default) "this is a dummy text asfasf asfd";

AW_default aw_check_default_file(AW_default root_default, AW_default default_file,const char *varname)
{
    if (default_file == aw_main_root_default)  return root_default;
    if (default_file == NULL) {
        AW_ERROR("Creating variable '%s' with zero default file\n",varname);
        return root_default;
    }
    return default_file;
}


// for string
AW_awar *AW_root::awar_string( const char *var_name, const char *default_value, AW_default default_file ) {
    AW_awar *vs;
    vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (vs) return vs;  /* already defined */
    default_file = aw_check_default_file(this->application_database,default_file,var_name);

    vs = new AW_awar( AW_STRING, var_name, default_value, 0, default_file, this );
    GBS_write_hash( hash_table_for_variables, (char *)var_name, (long)vs );
    return vs;
}


// for int
AW_awar *AW_root::awar_int( const char *var_name, long default_value, AW_default default_file ) {
    AW_awar *vs;
    vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (vs) return vs;  /* already defined */
    default_file = aw_check_default_file(this->application_database,default_file,var_name);

    vs = new AW_awar( AW_INT, var_name, (char *)default_value, 0, default_file, this );
    GBS_write_hash( hash_table_for_variables, (char *)var_name, (long)vs );
    return vs;
}


// for float
AW_awar *AW_root::awar_float( const char *var_name, float default_value, AW_default default_file ) {
    AW_awar *vs;
    vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (vs) return vs;  /* already defined */
    default_file = aw_check_default_file(this->application_database,default_file,var_name);

    vs = new AW_awar( AW_FLOAT, var_name, "", (double)default_value, default_file, this );
    GBS_write_hash( hash_table_for_variables, (char *)var_name, (long)vs );
    return vs;
}

AW_awar *AW_root::awar_no_error( const char *var_name){
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    return vs;
}

AW_awar *AW_root::awar( const char *var_name){
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (vs) return vs;  /* already defined */
    AW_ERROR("AWAR %s not defined",var_name);
    return this->awar_string(var_name);
}


AW_awar *AW_awar::add_callback( void (*f)(class AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ) {
    callback_list = new AW_var_callback(f,cd1,cd2,callback_list);
    return this;
}

AW_awar *AW_awar::add_callback( void (*f)(AW_root*,AW_CL), AW_CL cd1 ) { return add_callback((AW_RCB)f,cd1,0); }
AW_awar *AW_awar::add_callback( void (*f)(AW_root*)){ return add_callback((AW_RCB)f,0,0); }

AW_awar *AW_awar::remove_callback( void (*f)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ){
    // remove a callback, please set unused AW_CL to (AW_CL)0
    AW_var_callback *prev = 0;
    for (AW_var_callback *vc = callback_list; vc; vc = vc->next){
        if (vc->value_changed_cb== f &&
            vc->value_changed_cb_cd1 == cd1 &&
            vc->value_changed_cb_cd2 == cd2){
            if (prev) {
                prev->next = vc->next;
            }else{
                callback_list = vc->next;
            }
            delete vc;
            break;
        }
        prev = vc;
    }
    return this;
}

AW_awar *AW_awar::remove_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) { return remove_callback((AW_RCB) f, cd1, 0); }
AW_awar *AW_awar::remove_callback(void (*f)(AW_root*)) { return remove_callback((AW_RCB) f, 0, 0); }

void AW_awar::remove_all_callbacks() {
    while (callback_list) {
        AW_var_callback *del = callback_list;
        callback_list        = del->next;
        delete del;
    }
}


// --------------------------------------------------------------------------------

bool AW_awar::unlink_from_DB(GBDATA *gb_main) {
    bool make_zombie = false;
    if (gb_origin == gb_var) {                      // not mapped
        if (gb_var) {                               // no zombie awar
            if (GB_get_root(gb_var) == gb_main) {   // awar is in questionable DB
                make_zombie = true;
            }
        }
    }
    else {
        if (GB_get_root(gb_var) == gb_main) {
            if (GB_get_root(gb_origin) == gb_main) {
                // mapped and origin in DB
                make_zombie = true;
            }
            else {
                // origin is in other DB -> just unmap
                unmap();
            }
        }
        else {
            if (GB_get_root(gb_origin) == gb_main) {
                // origin is in DB, current mapping is not
                // -> remap permanentely
                gb_origin = gb_var;
            }
            // else both are in other DB -> nothing to do
        }
    }

    if (make_zombie) {
        remove_all_callbacks();
        remove_all_target_vars();
        map(AW_default(NULL));                      // map to nothing
        gb_origin = NULL;                           // make zombie awar (will not unmap)
    }

    return make_zombie;
}

long AW_unlink_awar_from_DB(const char *key, long cl_awar, void *cl_gb_main) {
    AW_awar *awar    = (AW_awar*)cl_awar;
    GBDATA  *gb_main = (GBDATA*)cl_gb_main;

#if defined(DEBUG) && 0
    bool is_zombie = awar->unlink_from_DB(gb_main);
    if (is_zombie) printf("Unlinked awar '%s' from DB\n", key);
#else
    AWUSE(key);
    awar->unlink_from_DB(gb_main);
#endif // DEBUG
    return cl_awar;
}

void AW_root::unlink_awars_from_DB(AW_default database) {
    GBDATA *gb_main = (GBDATA*)database;
    
    aw_assert(GB_get_root(gb_main) == gb_main);

    GB_transaction ta(gb_main); // needed in awar-callbacks during unlink 
    GBS_hash_do_loop(hash_table_for_variables, AW_unlink_awar_from_DB, gb_main);
}

// --------------------------------------------------------------------------------

GB_ERROR AW_awar::toggle_toggle(){
    char *var = this->read_as_string();
    GB_ERROR    error =0;
    if (var[0] == '0' || var[0] == 'n') {
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("yes");break;
            case AW_INT:        error = this->write_int(1);break;
            case AW_FLOAT:      error = this->write_float(1.0);break;
            default: break;
        }
    }else{
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("no");break;
            case AW_INT:        error = this->write_int(0);break;
            case AW_FLOAT:      error = this->write_float(0.0);break;
            default: break;
        }
    }
    free(var);
    return error;
}



AW_awar *AW_awar::set_minmax(float min, float max){
    if (min>max || variable_type == AW_STRING) {
        AW_ERROR("ERROR: set MINMAX for AWAR '%s' invalid",awar_name);
    }else{
        pp.f.min = min;
        pp.f.max = max;
        update(); // corrects wrong default value
    }
    return this;
}


AW_awar *AW_awar::add_target_var( char **ppchr){
    if (variable_type != AW_STRING) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE",awar_name);
    }else{
        target_list = new AW_var_target((void *)ppchr,target_list);
        update_target(target_list);
    }
    return this;
}

AW_awar *AW_awar::add_target_var( float *pfloat){
    if (variable_type != AW_FLOAT) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE",awar_name);
    }else{
        target_list = new AW_var_target((void *)pfloat,target_list);
        update_target(target_list);
    }
    return this;
}

AW_awar *AW_awar::add_target_var( long *pint){
    if (variable_type != AW_INT) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE",awar_name);
    }else{
        target_list = new AW_var_target((void *)pint,target_list);
        update_target(target_list);
    }
    return this;
}

void AW_awar::remove_all_target_vars() {
    while (target_list) {
        AW_var_target *tar = target_list;
        target_list        = tar->next;
        delete tar;
    }
}


AW_awar *AW_awar::set_srt(const char *srt)
{
    if (variable_type != AW_STRING) {
        AW_ERROR("ERROR: set SRT for AWAR '%s' invalid",awar_name);
    }else{
        pp.srt = srt;
    }
    return this;
}


AW_awar *AW_awar::map(AW_default gbd) {
    if (gbd) GB_push_transaction((GBDATA *)gbd);
    if (gb_var) {               /* old map */
        GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this);
        GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this);
    }
    if (gbd){
        GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this );
        GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this );
    }
    gb_var = (GBDATA *)gbd;
    this->update();
    if (gbd) GB_pop_transaction((GBDATA *)gbd);
    return this;
}

AW_awar *AW_awar::map( AW_awar *dest) {
    return this->map(dest->gb_var);
}

AW_awar *AW_awar::unmap( ) {
    return this->map(gb_origin);
}

AW_VARIABLE_TYPE AW_awar::get_type(){
    return this->variable_type;
}

void AW_awar::update(void)
{
    bool out_of_range = false;
    if (gb_var && ((pp.f.min != pp.f.max) || pp.srt) ) {
        float fl;
        char *str;
        switch (variable_type) {
            case AW_INT:{
                long lo;

                lo = this->read_int();
                if (lo < pp.f.min -.5) {
                    out_of_range = true;
                    lo = (int)(pp.f.min + 0.5);
                }
                if (lo>pp.f.max + .5) {
                    out_of_range = true;
                    lo = (int)(pp.f.max + 0.5);
                }
                if (out_of_range) {
                    if (root) root->changer_of_variable = 0;
                    this->write_int(lo);
                    return;             // returns update !!!!
                }
                break;
            }
            case AW_FLOAT:
                fl = this->read_float();
                if (fl < pp.f.min) {
                    out_of_range = true;
                    fl = pp.f.min+AWAR_EPS;
                }
                if (fl>pp.f.max) {
                    out_of_range = true;
                    fl = pp.f.max-AWAR_EPS;
                }
                if (out_of_range) {
                    if (root) root->changer_of_variable = 0;
                    this->write_float(fl);              // returns update !!!!
                    return;
                }
                break;

            case AW_STRING:
                str = this->read_string();
                char *n;
                n = GBS_string_eval(str,pp.srt,0);
                if (!n) AW_ERROR("SRT ERROR %s %s", pp.srt, GB_await_error());
                else{
                    if (strcmp(n,str)) {
                        this->write_string(n);
                        free(n);
                        free(str);
                        return;
                    }
                    free(n);
                }
                free(str);
                break;
            default:
                break;
        }
    }
    this->update_targets();
    this->run_callbacks();
}

void AW_awar::run_callbacks(){
    if (callback_list) callback_list->run_callback(root);

}

// send data to all variables
void AW_awar::update_target(AW_var_target*pntr){
    if (!pntr->pointer) return;
    switch(variable_type) {
        case AW_STRING:
            this->get((char **)pntr->pointer);break;
        case AW_FLOAT:
            this->get((double *)pntr->pointer);break;
        case AW_INT:
            this->get((long *)pntr->pointer);break;
        default:
            GB_warning("Unknown awar type");
    }
}

// send data to all variables
void AW_awar::update_targets(void){
    AW_var_target*pntr;
    for (pntr = target_list; pntr; pntr = pntr->next){
        update_target(pntr);
    }
}

AW_awar::AW_awar(AW_VARIABLE_TYPE var_type, const char *var_name, const char *var_value, double var_double_value, AW_default default_file, AW_root *rooti){
    memset((char *)this,0,sizeof(AW_awar));
    GB_transaction dummy((GBDATA *)default_file);

    aw_assert(var_name && var_name[0] != 0);

#if defined(DEBUG)
    GB_ERROR err = GB_check_hkey(var_name);
    aw_assert(!err);
#endif // DEBUG

    this->awar_name = strdup(var_name);
    this->root = rooti;
    GBDATA *gb_def = GB_search((GBDATA *)default_file, var_name,GB_FIND);
    if ( gb_def ) {                                                  // belege Variable mit Datenbankwert
        AW_VARIABLE_TYPE gbtype;
        gbtype = (AW_VARIABLE_TYPE) GB_read_type( gb_def );
        if ( gbtype != var_type ) {
            GB_warning("Wrong Awar type %s\n",var_name);
            GB_delete( gb_def );
            gb_def = 0;
        }
    }
    if (!gb_def) {             // belege Variable mit Programmwert
        gb_def = GB_search( (GBDATA *)default_file, var_name, (GB_TYPES)var_type);

        switch ( var_type ) {
            case AW_STRING:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_string '%s' with default value '%s'\n", var_name, (char*)var_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_string( gb_def, (char *)var_value );
                break;
            case AW_INT:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_int '%s' with default value '%li'\n", var_name, (long)var_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_int( gb_def, (long)var_value );
                break;
            case AW_FLOAT:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_float '%s' with default value '%f'\n", var_name, (double)var_double_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_float( gb_def, (double)var_double_value );
                break;
            default:
                GB_warning("AWAR '%s' cannot be created because of inallowed type",var_name);
                break;
        }
    }
    variable_type               = var_type;
    this->gb_origin = gb_def;
    this->map(gb_def);
}



AW_default AW_root::open_default(const char *default_name, bool create_if_missing)
{
    if (!create_if_missing) { // used to check for existing specific properties
        const char *home   = GB_getenvHOME();
        char       *buffer = (char *)GB_calloc(sizeof(char),strlen(home)+ strlen(default_name) + 2);

        sprintf(buffer,"%s/%s", home, default_name);

        struct stat st;
        bool        found = stat(buffer, &st) == 0;

        free(buffer);

        if (!found) return 0;
    }

#if defined(DEVEL_RALF)
#warning gb_default is never closed
    // close it somewhere and call AWT_browser_forget_db as well
#endif // DEVEL_RALF

    GBDATA *gb_default = GB_open(default_name, "rwcD");

    if (gb_default) {
        GB_no_transaction(gb_default);
#if defined(DEBUG)
        AWT_announce_db_to_browser(gb_default, GBS_global_string("Properties (%s)", default_name));
#endif // DEBUG
    }
    else {
        GB_ERROR    error           = GB_await_error();
        const char *shown_name      = strrchr(default_name, '/');
        if (!shown_name) shown_name = default_name;

        GBK_terminate(GBS_global_string("Error loading properties '%s': %s", shown_name, error));
    }
    return (AW_default) gb_default;
}


AW_error *AW_root::save_default( const char *var_name ) {
    return save_default(var_name, NULL);
}

AW_error *AW_root::save_default( const char *var_name, const char *file_name) {
    AW_awar *vs;
    if ( (vs = this->awar( var_name ))  ) {
        AW_root::save_default((AW_default)vs->gb_var, file_name);
        return 0;
    }else {
        AW_ERROR("AW_root::save_default: Variable %s not defined", var_name);
    }
    return 0;

}

AW_error *AW_root::save_default(AW_default aw_default, const char *file_name)
{
    GBDATA *gb_tmp;
    GBDATA *gb_main = GB_get_root((GBDATA *)aw_default);
    GB_push_transaction(gb_main);
    gb_tmp = GB_entry(gb_main,"tmp");
    if (gb_tmp) GB_set_temporary(gb_tmp);
    aw_update_awar_window_geometry(this);
    GB_pop_transaction(gb_main);
    GB_save_in_home(gb_main,file_name,"a");
    return 0;
}

AW_default AW_root::get_default(const char *varname) {
    GBDATA      *gbd;
    AW_awar *vs;
    if ( (vs = this->awar( varname )) ) {
        gbd = vs->gb_var;
        return (AW_default)GB_get_root(gbd);
    }else {
        AW_ERROR("AW_root::get_default: Variable %s not defined", varname);
    }
    return 0;
}

AW_default AW_root::get_gbdata( const char *varname) {
    GBDATA      *gbd;
    AW_awar *vs;
    if ( (vs = this->awar( varname )) ) {
        gbd = vs->gb_var;
        return (AW_default)gbd;
    }else {
        AW_ERROR("AW_root::get_gbdata: Variable %s not defined", varname);
    }
    return 0;
}

AW_awar *AW_root::label_is_awar(const char *label) {
    AW_awar *awar_exists = NULL;
    size_t   off         = strcspn(label, "/ ");

    if (label[off] == '/') {                        // contains '/' and no space before first '/'
        awar_exists = awar_no_error(label);
    }
    return awar_exists;
}

// ---------------------------
//      Awar_Callback_Info
// ---------------------------

void Awar_Callback_Info::remap(const char *new_awar) {
    if (strcmp(awar_name, new_awar) != 0) {
        remove_callback();
        freedup(awar_name, new_awar);
        add_callback();
    }
}
void Awar_Callback_Info::init(AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_) {
    awr           = awr_;
    callback      = callback_;
    cd1           = cd1_;
    cd2           = cd2_;
    awar_name     = strdup(awar_name_);
    org_awar_name = strdup(awar_name_);
}

void aw_create_selection_box_awars(AW_root *awr, const char *awar_base,
                                   const char *directory, const char *filter, const char *file_name,
                                   AW_default default_file, bool resetValues)
{
    int   base_len  = strlen(awar_base);
    bool  has_slash = awar_base[base_len-1] == '/';
    char *awar_name = new char[base_len+30]; // use private buffer, because caller will most likely use GBS_global_string for arguments

    sprintf(awar_name, "%s%s", awar_base, "/directory"+int(has_slash));
    AW_awar *awar_dir = awr->awar_string(awar_name, directory, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/filter"   +int(has_slash));
    AW_awar *awar_filter = awr->awar_string(awar_name, filter, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/file_name"+int(has_slash));
    AW_awar *awar_filename = awr->awar_string(awar_name, file_name, default_file);

    if (resetValues) {
        awar_dir->write_string(directory);
        awar_filter->write_string(filter);
        awar_filename->write_string(file_name);
    }
    else {
        char *stored_directory = awar_dir->read_string();
#if defined(DEBUG)
        if (strncmp(awar_base, "tmp/", 4) == 0) { // non-saved awar
            if (directory[0] != 0) { // accept empty dir (means : use current ? )
                aw_assert(GB_is_directory(directory)); // default directory does not exist
            }
        }
#endif // DEBUG

        if (strcmp(stored_directory, directory) != 0) { // does not have default value
#if defined(DEBUG)
            const char *arbhome    = GB_getenvARBHOME();
            int         arbhomelen = strlen(arbhome);

            if (strncmp(directory, arbhome, arbhomelen) == 0) { // default points into $ARBHOME
                aw_assert(resetValues); // should be called with resetValues == true
                // otherwise it's possible, that locations from previously installed ARB versions are used
            }
#endif // DEBUG

            if (!GB_is_directory(stored_directory)) {
                awar_dir->write_string(directory);
                fprintf(stderr,
                        "Warning: Replaced reference to non-existing directory '%s'\n"
                        "         by '%s'\n"
                        "         (Save properties to make this change permanent)\n",
                        stored_directory, directory);
            }
        }

        free(stored_directory);
    }

    char *dir = awar_dir->read_string();
    if (dir[0] && !GB_is_directory(dir)) {
        if (aw_ask_sure(GBS_global_string("Directory '%s' does not exist. Create?", dir))) {
            GB_ERROR error = GB_create_directory(dir);
            if (error) aw_message(GBS_global_string("Failed to create directory '%s' (Reason: %s)", dir, error));
        }
    }
    free(dir);

    delete [] awar_name;
}
