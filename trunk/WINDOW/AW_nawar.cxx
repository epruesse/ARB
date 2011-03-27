// =============================================================== //
//                                                                 //
//   File      : AW_nawar.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_nawar.hxx"
#include "aw_awar.hxx"
#include "aw_detach.hxx"
#include "aw_msg.hxx"
#include <arbdb.h>
#include <sys/stat.h>

#define AWAR_EPS 0.00000001

#if defined(DEBUG)
// uncomment next line to dump all awar-changes to stderr
// #define DUMP_AWAR_CHANGES
#endif // DEBUG

AW_var_target::AW_var_target(void* pntr, AW_var_target *nexti) {
    next    = nexti;
    pointer = pntr;
}

void AW_var_gbdata_callback_delete_intern(GBDATA *gbd, int *cl) {
    AW_awar *awar = (AW_awar *)cl;

    if (awar->gb_origin == gbd) {
        // make awar zombie 
        awar->gb_var    = NULL;
        awar->gb_origin = NULL;
    }
    else {
        aw_assert(awar->gb_var == gbd);             // forgot to remove a callback ?
        awar->gb_var = awar->gb_origin;             // unmap
    }

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

char *AW_awar::read_as_string() {
    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_as_string(gb_var);
}

char *AW_awar::read_string() {
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return strdup("");
    GB_transaction ta(gb_var);
    return GB_read_string(gb_var);
}

const char *AW_awar::read_char_pntr() {
    aw_assert(variable_type == AW_STRING);

    if (!gb_var) return "";
    GB_transaction ta(gb_var);
    return GB_read_char_pntr(gb_var);
}

long AW_awar::read_int() {
    if (!gb_var) return 0;
    GB_transaction ta(gb_var);
    return (long)GB_read_int(gb_var);
}
double AW_awar::read_float() {
    if (!gb_var) return 0.0;
    GB_transaction ta(gb_var);
    return GB_read_float(gb_var);
}

void *AW_awar::read_pointer() {
    if (!gb_var) return NULL;
    GB_transaction ta(gb_var);
    return GB_read_pointer(gb_var);
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
    WRITE_SKELETON(write_as_string, const char*, "%s", GB_write_as_string) // defines rewrite_as_string
    WRITE_SKELETON(write_pointer, void*, "%p", GB_write_pointer) // defines rewrite_pointer

#undef WRITE_SKELETON
#undef concat
#undef AWAR_CHANGE_DUMP


    void AW_awar::touch() {
    if (!gb_var) {
        return;
    }
    GB_transaction dummy(gb_var);
    GB_touch(gb_var);
}

// for string
AW_awar *AW_root::awar_string(const char *var_name, const char *default_value, AW_default default_file) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_STRING, var_name, default_value, 0, default_file, this);
        GBS_write_hash(hash_table_for_variables, (char *)var_name, (long)vs);
    }
    return vs;
}


// for int
AW_awar *AW_root::awar_int(const char *var_name, long default_value, AW_default default_file) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_INT, var_name, (char *)default_value, 0, default_file, this);
        GBS_write_hash(hash_table_for_variables, (char *)var_name, (long)vs);
    }
    return vs;
}


// for float
AW_awar *AW_root::awar_float(const char *var_name, float default_value, AW_default default_file) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_FLOAT, var_name, "", (double)default_value, default_file, this);
        GBS_write_hash(hash_table_for_variables, (char *)var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_pointer(const char *var_name, void *default_value, AW_default default_file) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (!vs) {
        default_file = check_properties(default_file);
        vs           = new AW_awar(AW_POINTER, var_name, (const char *)default_value, NULL, default_file, this);
        GBS_write_hash(hash_table_for_variables, (char *)var_name, (long)vs);
    }
    return vs;
}

AW_awar *AW_root::awar_no_error(const char *var_name) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    return vs;
}

AW_awar *AW_root::awar(const char *var_name) {
    AW_awar *vs = (AW_awar *)GBS_read_hash(hash_table_for_variables, (char *)var_name);
    if (vs) return vs;  /* already defined */
    AW_ERROR("AWAR %s not defined", var_name);
    return this->awar_string(var_name);
}


AW_awar *AW_awar::add_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::add(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar::add_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) { return add_callback((AW_RCB)f, cd1, 0); }
AW_awar *AW_awar::add_callback(void (*f)(AW_root*)) { return add_callback((AW_RCB)f, 0, 0); }

AW_awar *AW_awar::remove_callback(AW_RCB value_changed_cb, AW_CL cd1, AW_CL cd2) {
    AW_root_cblist::remove(callback_list, AW_root_callback(value_changed_cb, cd1, cd2));
    return this;
}

AW_awar *AW_awar::remove_callback(void (*f)(AW_root*, AW_CL), AW_CL cd1) { return remove_callback((AW_RCB) f, cd1, 0); }
AW_awar *AW_awar::remove_callback(void (*f)(AW_root*)) { return remove_callback((AW_RCB) f, 0, 0); }

void AW_awar::remove_all_callbacks() {
    AW_root_cblist::clear(callback_list);
}

// --------------------------------------------------------------------------------

inline bool member_of_DB(GBDATA *gbd, GBDATA *gb_main) {
    return gbd && GB_get_root(gbd) == gb_main;;
}

void AW_awar::unlink() {
    aw_assert(this->is_valid());
    remove_all_callbacks();
    remove_all_target_vars();
    gb_origin = NULL;                               // make zombie awar
    unmap();                                        // unmap to nothing
    aw_assert(is_valid());
}

bool AW_awar::unlink_from_DB(GBDATA *gb_main) {
    bool make_zombie  = false;
    bool mapped_to_DB = member_of_DB(gb_var, gb_main);
    bool origin_in_DB = member_of_DB(gb_origin, gb_main);

    aw_assert(is_valid());
    
    if (mapped_to_DB) {
        if (origin_in_DB) make_zombie = true;
        else unmap();
    }
    else if (origin_in_DB) {
        // origin is in DB, current mapping is not
        // -> remap permanentely
        gb_origin = gb_var;
    }

    if (make_zombie) unlink();

    aw_assert(is_valid());
    
    return make_zombie;
}

long AW_unlink_awar_from_DB(const char */*key*/, long cl_awar, void *cl_gb_main) {
    AW_awar *awar    = (AW_awar*)cl_awar;
    GBDATA  *gb_main = (GBDATA*)cl_gb_main;

    awar->unlink_from_DB(gb_main);
    return cl_awar;
}

void AW_root::unlink_awars_from_DB(AW_default database) {
    GBDATA *gb_main = (GBDATA*)database;

    aw_assert(GB_get_root(gb_main) == gb_main);

    GB_transaction ta(gb_main); // needed in awar-callbacks during unlink
    GBS_hash_do_loop(hash_table_for_variables, AW_unlink_awar_from_DB, gb_main);
}

// --------------------------------------------------------------------------------

GB_ERROR AW_awar::toggle_toggle() {
    char *var = this->read_as_string();
    GB_ERROR    error = 0;
    if (var[0] == '0' || var[0] == 'n') {
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("yes"); break;
            case AW_INT:        error = this->write_int(1); break;
            case AW_FLOAT:      error = this->write_float(1.0); break;
            default: break;
        }
    }
    else {
        switch (this->variable_type) {
            case AW_STRING:     error = this->write_string("no"); break;
            case AW_INT:        error = this->write_int(0); break;
            case AW_FLOAT:      error = this->write_float(0.0); break;
            default: break;
        }
    }
    free(var);
    return error;
}



AW_awar *AW_awar::set_minmax(float min, float max) {
    if (min>max || variable_type == AW_STRING) {
        AW_ERROR("ERROR: set MINMAX for AWAR '%s' invalid", awar_name);
    }
    else {
        pp.f.min = min;
        pp.f.max = max;
        update(); // corrects wrong default value
    }
    return this;
}


AW_awar *AW_awar::add_target_var(char **ppchr) {
    if (variable_type != AW_STRING) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE", awar_name);
    }
    else {
        target_list = new AW_var_target((void *)ppchr, target_list);
        update_target(target_list);
    }
    return this;
}

AW_awar *AW_awar::add_target_var(float *pfloat) {
    if (variable_type != AW_FLOAT) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE", awar_name);
    }
    else {
        target_list = new AW_var_target((void *)pfloat, target_list);
        update_target(target_list);
    }
    return this;
}

AW_awar *AW_awar::add_target_var(long *pint) {
    if (variable_type != AW_INT) {
        AW_ERROR("Cannot set target awar '%s', WRONG AWAR TYPE", awar_name);
    }
    else {
        target_list = new AW_var_target((void *)pint, target_list);
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
        AW_ERROR("ERROR: set SRT for AWAR '%s' invalid", awar_name);
    }
    else {
        pp.srt = srt;
    }
    return this;
}

AW_awar *AW_awar::map(AW_default gbd) {
    if (gb_var) {                                   // remove old mapping
        GB_remove_callback((GBDATA *)gb_var, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this);
        if (gb_var != gb_origin) {                  // keep callback if pointing to origin!
            GB_remove_callback((GBDATA *)gb_var, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this);
        }
        gb_var = NULL;
    }

    if (!gbd) {                                     // attempt to remap to NULL -> unmap
        gbd = gb_origin;
    }

    if (gbd) {
        GB_transaction ta(gbd);

        GB_ERROR error = GB_add_callback((GBDATA *) gbd, GB_CB_CHANGED, (GB_CB)AW_var_gbdata_callback, (int *)this);
        if (!error && gbd != gb_origin) {           // do not add delete callback if mapping to origin
            error = GB_add_callback((GBDATA *) gbd, GB_CB_DELETE, (GB_CB)AW_var_gbdata_callback_delete, (int *)this);
        }
        if (error) aw_message(error);

        gb_var = gbd;
        update();
    }
    else {
        update();
    }

    aw_assert(is_valid()); 

    return this;
}

AW_awar *AW_awar::map(AW_awar *dest) {
    return this->map(dest->gb_var);
}
AW_awar *AW_awar::map(const char *awarn) {
    return map(root->awar(awarn));
}

AW_awar *AW_awar::unmap() {
    return this->map(gb_origin);
}

AW_VARIABLE_TYPE AW_awar::get_type() {
    return this->variable_type;
}

void AW_awar::update() {
    bool out_of_range = false;

    aw_assert(is_valid()); 
    
    if (gb_var && ((pp.f.min != pp.f.max) || pp.srt)) {
        float fl;
        char *str;
        switch (variable_type) {
            case AW_INT: {
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
                    aw_assert(is_valid()); 
                    return;
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
                    this->write_float(fl); 
                    aw_assert(is_valid()); 
                    return;
                }
                break;

            case AW_STRING:
                str = this->read_string();
                char *n;
                n = GBS_string_eval(str, pp.srt, 0);
                if (!n) AW_ERROR("SRT ERROR %s %s", pp.srt, GB_await_error());
                else {
                    if (strcmp(n, str)) {
                        this->write_string(n);
                        free(n);
                        free(str);
                        aw_assert(is_valid()); 
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

    aw_assert(is_valid()); 
}

void AW_awar::run_callbacks() {
    AW_root_cblist::call(callback_list, root);
}

void AW_awar::update_target(AW_var_target *pntr) {
    // send data to all variables
    if (!pntr->pointer) return;
    switch (variable_type) {
        case AW_STRING: this->get((char **)pntr->pointer); break;
        case AW_FLOAT:  this->get((float *)pntr->pointer); break;
        case AW_INT:    this->get((long *)pntr->pointer); break;
        default:
            aw_assert(0);
            GB_warning("Unknown awar type");
            break;
    }
}

void AW_awar::update_targets() {
    // send data to all variables
    AW_var_target*pntr;
    for (pntr = target_list; pntr; pntr = pntr->next) {
        update_target(pntr);
    }
}

AW_awar::AW_awar(AW_VARIABLE_TYPE var_type, const char *var_name, const char *var_value, double var_double_value, AW_default default_file, AW_root *rooti) {
    memset((char *)this, 0, sizeof(AW_awar));
    GB_transaction dummy((GBDATA *)default_file);

    aw_assert(var_name && var_name[0] != 0);

#if defined(DEBUG)
    GB_ERROR err = GB_check_hkey(var_name);
    aw_assert(!err);
#endif // DEBUG

    this->awar_name = strdup(var_name);
    this->root      = rooti;
    GBDATA *gb_def  = GB_search((GBDATA *)default_file, var_name, GB_FIND);

    GB_TYPES wanted_gbtype = (GB_TYPES)var_type;

    if (gb_def) {                                   // belege Variable mit Datenbankwert
        GB_TYPES gbtype = GB_read_type(gb_def);

        if (gbtype != wanted_gbtype) {
            GB_warningf("Existing awar '%s' has wrong type (%i instead of %i) - recreating\n",
                        var_name, int(gbtype), int(wanted_gbtype));
            GB_delete(gb_def);
            gb_def = 0;
        }
    }

    if (!gb_def) {                                  // belege Variable mit Programmwert
        gb_def = GB_search((GBDATA *)default_file, var_name, wanted_gbtype);

        switch (var_type) {
            case AW_STRING:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_string '%s' with default value '%s'\n", var_name, (char*)var_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_string(gb_def, (char *)var_value);
                break;

            case AW_INT:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_int '%s' with default value '%li'\n", var_name, (long)var_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_int(gb_def, (long)var_value);
                break;

            case AW_FLOAT:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_float '%s' with default value '%f'\n", var_name, (double)var_double_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_float(gb_def, (double)var_double_value);
                break;

            case AW_POINTER:
#if defined(DUMP_AWAR_CHANGES)
                fprintf(stderr, "creating awar_pointer '%s' with default value '%p'\n", var_name, (void*)var_value);
#endif // DUMP_AWAR_CHANGES
                GB_write_pointer(gb_def, (void*)var_value);
                break;

            default:
                GB_warningf("AWAR '%s' cannot be created because of disallowed type", var_name);
                break;
        }
    }

    variable_type   = var_type;
    this->gb_origin = gb_def;
    this->map(gb_def);

    aw_assert(is_valid());
}

AW_awar::~AW_awar() {
    unlink();
    untie_all_widgets();
    free(awar_name);
}

const char *AW_root::property_DB_fullname(const char *default_name) {
    const char *home = GB_getenvHOME();
    return GBS_global_string("%s/%s", home, default_name);
}

bool AW_root::property_DB_exists(const char *default_name) {
    return GB_is_regularfile(property_DB_fullname(default_name));
}

AW_default AW_root::load_properties(const char *default_name) {
    GBDATA *gb_default = GB_open(default_name, "rwcD");

    if (gb_default) {
        GB_no_transaction(gb_default);

        GBDATA *gb_tmp = GB_search(gb_default, "tmp", GB_CREATE_CONTAINER);
        GB_set_temporary(gb_tmp);
    }
    else {
        GB_ERROR    error           = GB_await_error();
        const char *shown_name      = strrchr(default_name, '/');
        if (!shown_name) shown_name = default_name;

        GBK_terminatef("Error loading properties '%s': %s", shown_name, error);
    }
    return (AW_default) gb_default;
}

GB_ERROR AW_root::save_properties(const char *filename) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_main = application_database;

    if (!gb_main) {
        error = "No properties loaded - won't save";
    }
    else {
        error = GB_push_transaction(gb_main);
        if (!error) {
            aw_update_awar_window_geometry(this);
            error = GB_pop_transaction(gb_main);
            if (!error) error = GB_save_in_home(gb_main, filename, "a");
        }
    }

    return error;
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

void AW_create_fileselection_awars(AW_root *awr, const char *awar_base,
                                   const char *directory, const char *filter, const char *file_name,
                                   AW_default default_file, bool resetValues)
{
    int   base_len  = strlen(awar_base);
    bool  has_slash = awar_base[base_len-1] == '/';
    char *awar_name = new char[base_len+30]; // use private buffer, because caller will most likely use GBS_global_string for arguments

    sprintf(awar_name, "%s%s", awar_base, "/directory"+int(has_slash));
    AW_awar *awar_dir = awr->awar_string(awar_name, directory, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/filter"   + int(has_slash));
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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static int test_cb1_called;
static int test_cb2_called;

static void test_cb1(AW_root *, AW_CL cd1, AW_CL cd2) { test_cb1_called += (cd1+cd2); }
static void test_cb2(AW_root *, AW_CL cd1, AW_CL cd2) { test_cb2_called += (cd1+cd2); }

#define TEST_ASSERT_CBS_CALLED(cbl, c1,c2)              \
    do {                                                \
        test_cb1_called = test_cb2_called = 0;          \
        AW_root_cblist::call(cbl, NULL);                \
        TEST_ASSERT_EQUAL(test_cb1_called, c1);         \
        TEST_ASSERT_EQUAL(test_cb2_called, c2);         \
    } while(0)

void TEST_AW_root_cblist() {
    AW_root_cblist *cb_list = NULL;

    AW_root_callback tcb1(test_cb1, 1, 0);
    AW_root_callback tcb2(test_cb2, 0, 1);
    AW_root_callback wrong_tcb2(test_cb2, 1, 0);

    AW_root_cblist::add(cb_list, tcb1); TEST_ASSERT_CBS_CALLED(cb_list, 1, 0);
    AW_root_cblist::add(cb_list, tcb2); TEST_ASSERT_CBS_CALLED(cb_list, 1, 1);

    AW_root_cblist::remove(cb_list, tcb1);       TEST_ASSERT_CBS_CALLED(cb_list, 0, 1);
    AW_root_cblist::remove(cb_list, wrong_tcb2); TEST_ASSERT_CBS_CALLED(cb_list, 0, 1);
    AW_root_cblist::remove(cb_list, tcb2);       TEST_ASSERT_CBS_CALLED(cb_list, 0, 0);

    AW_root_cblist::add(cb_list, tcb1);
    AW_root_cblist::add(cb_list, tcb1); // add callback twice
    TEST_ASSERT_CBS_CALLED(cb_list, 1, 0);  // should only be called once

    AW_root_cblist::add(cb_list, tcb2);
    AW_root_cblist::clear(cb_list);
    TEST_ASSERT_CBS_CALLED(cb_list, 0, 0); // list clear - nothing should be called
}

#endif // UNIT_TESTS
