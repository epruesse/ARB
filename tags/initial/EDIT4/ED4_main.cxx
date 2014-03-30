#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <awt_seq_colors.hxx>
#include <awt_map_key.hxx>
#include <awt.hxx>
#define _USE_AW_WINDOW
#include <BI_helix.hxx>
#include <st_window.hxx>
#include <gde.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_defs.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_nds.hxx"

#include "edit_naligner.hxx"

AW_HEADER_MAIN

ED4_root 	*ED4_ROOT;
GBDATA 		*gb_main = NULL;
ED4_database	*main_db;
long 		global_width = 100;
	
int 			TERMINALHEIGHT;				// this variable replaces the define
int			MAXLETTERDESCENT;			// Important for drawing text on the screen
int			MAXSEQUENCECHARACTERLENGTH;		// greatest # of characters in a sequence string terminal
int			MAXSPECIESWIDTH;
int			MAXINFOWIDTH;				// # of characters used to display sequence info ("CONS", "4data", etc.)
int			MAXCHARWIDTH;
int			MARGIN;					// sets margin for cursor moves in characters
long			ED4_counter = 0;
long			all_found;				// nr of species which haven't been found
long			species_read;				// nr of species read; important during loading
void 			*not_found_message;
long			max_seq_terminal_length;		// global maximum of sequence terminal length
ED4_EDITMODI		awar_edit_modus;
long			awar_edit_direction;
bool			move_cursor;				// only needed for editing in consensus
bool			DRAW;				
bool			last_window_reached;			// needed for refreshing all windows (if TRUE refresh/...-flags will be cleared) 

double			status_add_count;			// only needed for loading configuration
double			status_total_count;
bool			loading;
//long 			last_used_timestamp;

int ED4_font_info::max_width = 0;
int ED4_font_info::max_height = 0;
int ED4_font_info::max_ascent = 0;
int ED4_font_info::max_descent = 0;

void ED4_config_change_cb(AW_root *)
{
}

inline void replaceChars(char *s, char o, char n)
{
    while (1) {
        char c = *s++;
        if (!c) {
            break;
        }
        if (c==o) {
            s[-1] = n;
        }
    }
}

inline void set_and_realloc_gde_array(uchar **&the_names, uchar **&the_sequences, long &allocated, long &numberspecies, long &maxalign,
                                      const char *name, int name_len, const char *seq, int seq_len)
{
    if (allocated==numberspecies)
    {
        long new_allocated = (allocated*3)/2;
	
        the_names = (uchar**)GB_recalloc(the_names, allocated, new_allocated, sizeof(*the_names));
        the_sequences = (uchar**)GB_recalloc(the_sequences, allocated, new_allocated, sizeof(*the_sequences));
        allocated = new_allocated;
    }
    
    the_names[numberspecies] = (uchar*)GB_calloc(name_len+1, sizeof(char));
    memcpy(the_names[numberspecies], name, name_len);
    the_names[numberspecies][name_len] = 0;
    replaceChars((char*)the_names[numberspecies], ' ', '_'); 
    
    the_sequences[numberspecies] = (uchar*)GB_calloc(seq_len+1, sizeof(char));
    memcpy(the_sequences[numberspecies], seq, seq_len);
    the_sequences[numberspecies][seq_len] = 0;
    
    if (seq_len>maxalign) {
        maxalign = seq_len;
    }
    
    numberspecies++;
}

#ifndef NDEBUG

inline const char *id(ED4_base *base)
{
    return base->id ? base->id : "???";
}

static void childs(ED4_manager *manager)
{
    int i;
    int anz = manager->children->members();
    
    if (anz) {
        printf("(");
        for (i=0; i<anz; i++) {
            if (i) printf(", ");
            printf("%s", id(manager->children->member(i)));
        }
        printf(")");
    }
}

void baum(ED4_base *base)
{
    static int level;
    
    level++;
    
    if (base->parent) {
        baum(base->parent);
    }
    
    printf("[%2i] %-30s", level, id(base));
    if (base->is_manager()) {
        childs(base->to_manager());
    }
    printf("\n");
    
    level--;
}

void baum(const char *id)
{
    baum(ED4_ROOT->root_group_man->search_ID(id));
}

#endif

static char *add_area_for_gde(ED4_area_manager *area_man, uchar **&the_names, uchar **&the_sequences,
                              long &allocated, long &numberspecies, long &maxalign,
                              int show_sequence, int show_SAI, int show_helix, int show_consensus, int show_remark)
{
    ED4_terminal *terminal = area_man->get_first_terminal(),
        *last = area_man->get_last_terminal();
    //    ED4_multi_species_manager *last_multi = 0;
    
    for (;terminal;) {
        if (terminal->is_species_name_terminal()) {
            ED4_species_manager *species_manager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
            ED4_species_name_terminal *species_name = species_manager->search_spec_child_rek(ED4_L_SPECIES_NAME)->to_species_name_terminal();
            int name_len;
            char *name = species_name->resolve_pointer_to_string(&name_len);
            ED4_sequence_terminal *sequence_terminal;
	    
            {
                ED4_base *sequence_term  = species_manager->search_spec_child_rek(ED4_L_SEQUENCE_STRING);
                if (!sequence_term) goto end_of_loop;
                sequence_terminal = sequence_term->to_sequence_terminal();
            }
	    
            int show = -1;
            int is_consensus = 0;
            int is_SAI = 0;
	    
            if (sequence_terminal->parent) {
                ED4_manager *cons_man = sequence_terminal->parent->parent;
                if (cons_man && cons_man->flag.is_consensus) { // consensus
                    show = show_consensus;
                    is_consensus = 1;
                }
            }
	    
            if (show==-1) {
                if (species_manager->flag.is_SAI) {
                    show = show_SAI;
                    is_SAI = 1;
                }
                else {
                    show = show_sequence;
                }
            }
	    
            e4_assert(show!=-1);
	    
            if (show) {
                int seq_len;
                char *seq = 0;
		
                if (is_consensus) {
                    ED4_group_manager *group_manager = sequence_terminal->get_parent(ED4_L_GROUP)->to_group_manager();
                    int size = group_manager->table().size();
			
                    seq = GB_give_buffer(size+1);
                    seq[size] = 0;
                    seq = group_manager->table().build_consensus_string(0, size-1, seq);
                    seq_len = size;
                    e4_assert(strlen(seq)==size_t(seq_len));
			
                    ED4_group_manager *folded_group_man = sequence_terminal->is_in_folded_group();
			
                    if (folded_group_man) { // we are in a folded group
                        if (folded_group_man==group_manager) { // we are the consensus of the folded group
                            if (folded_group_man->is_in_folded_group()) { // a folded group inside a folded group -> do not show
                                seq = 0;
                            }
                            else { // group folded but consensus shown -> add '-' before name
                                char *new_name = GB_give_buffer2(name_len+2);
#ifndef NDEBUG				
                                memset(new_name, 0, name_len+2); // avoids (rui)
#endif				
                                sprintf(new_name, "-%s", name); 
                                name = new_name;
                                name_len++;
                            }
                        }
                        else { // we are really inside a folded group -> don't show
                            seq = 0;
                        }
                    }
                }
                else { // sequence 
                    if (!sequence_terminal->is_in_folded_group()) {
                        seq = sequence_terminal->resolve_pointer_to_string(&seq_len);
                    }
                }
		
                if (seq) {
                    set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, seq, seq_len);
                    if (show_helix && !is_SAI && ED4_ROOT->helix->size) {
                        char *helix = ED4_ROOT->helix->seq_2_helix(seq, '.');
                        set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, helix, seq_len);
                    }
                    if (show_remark && !is_consensus) {
                        ED4_multi_sequence_manager *ms_man = sequence_terminal->get_parent(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                        ED4_base *remark_name_term = ms_man->search_ID("remark");
                        if (remark_name_term) {
                            ED4_base *remark_term = remark_name_term->get_next_terminal();
                            e4_assert(remark_term);
			    
                            int remark_len;
                            char *remark = remark_term->resolve_pointer_to_string(&remark_len);
			    
                            replaceChars(remark, ' ', '_');
                            set_and_realloc_gde_array(the_names, the_sequences, allocated, numberspecies, maxalign, name, name_len, remark, remark_len);
                        }
                    }
                }
            }
        }
    end_of_loop:
        if (terminal==last) break;
        terminal = terminal->get_next_terminal();
    }
    
    return 0;
}

char *ED4_create_sequences_for_gde(void*, GBDATA **&the_species, uchar **&the_names, uchar **&the_sequences, long &numberspecies, long &maxalign)
{
    int top = ED4_ROOT->aw_root->awar("gde/top_area")->read_int();		
    int tops = ED4_ROOT->aw_root->awar("gde/top_area_sai")->read_int();
    int toph = ED4_ROOT->aw_root->awar("gde/top_area_helix")->read_int();
    int topk = ED4_ROOT->aw_root->awar("gde/top_area_kons")->read_int();
    int topr = ED4_ROOT->aw_root->awar("gde/top_area_remark")->read_int();
    int middle = ED4_ROOT->aw_root->awar("gde/middle_area")->read_int();
    int middles = ED4_ROOT->aw_root->awar("gde/middle_area_sai")->read_int();
    int middleh = ED4_ROOT->aw_root->awar("gde/middle_area_helix")->read_int();
    int middlek = ED4_ROOT->aw_root->awar("gde/middle_area_kons")->read_int();
    int middler = ED4_ROOT->aw_root->awar("gde/middle_area_remark")->read_int();
    
    numberspecies = 0;
    maxalign = 0;
    
    long allocated = 100;
    the_species = 0;
    the_names = (uchar**)GB_calloc(allocated, sizeof(*the_names));
    the_sequences = (uchar**)GB_calloc(allocated, sizeof(*the_sequences));
    
    char *err = add_area_for_gde(ED4_ROOT->top_area_man, the_names, the_sequences, allocated, numberspecies, maxalign, top, tops, toph, topk, topr);
    if (!err) {
        err = add_area_for_gde(ED4_ROOT->middle_area_man, the_names, the_sequences, allocated, numberspecies, maxalign, middle, middles, middleh, middlek, middler);
    }
    
    if (allocated!=(numberspecies+1)) {
        the_names = (uchar**)GB_recalloc(the_names, allocated, numberspecies+1, sizeof(*the_names));
        the_sequences = (uchar**)GB_recalloc(the_sequences, allocated, numberspecies+1, sizeof(*the_sequences));
    }
    
    return err;
}

static void ED4_gap_chars_changed(AW_root *root) {
    char *gap_chars = root->awar_string(ED4_AWAR_GAP_CHARS)->read_string();
    
    ED4_init_is_align_character(gap_chars);
}

void ED4_create_global_awars(AW_root *root) {
    root->awar_string(AWAR_ITARGET_STRING, "", gb_main);    
    ED4_create_search_awars(root);
}

void ED4_create_awars(AW_root *root, const char *config_name) { // cursor awars are created in window constructor
    
    ED4_create_global_awars(root);
    create_naligner_variables(root, AW_ROOT_DEFAULT);
    
    root->awar_int(ED4_AWAR_SPECIES_NAME_WIDTH,20) ->add_target_var(&global_width) ->set_minmax(10,50) ->add_callback(ED4_config_change_cb);
    awar_edit_modus = AD_ALIGN;
    
    int def_sec_level = 0;
#ifdef DEBUG
    def_sec_level = 6; // don't nag developers
#endif	
    root->awar_int( AWAR_EDIT_SECURITY_LEVEL, def_sec_level,AW_ROOT_DEFAULT);

    root->awar_int( AWAR_EDIT_SECURITY_LEVEL_ALIGN, def_sec_level,AW_ROOT_DEFAULT)->add_callback(ed4_changesecurity,(AW_CL) def_sec_level);
    root->awar_int( AWAR_EDIT_SECURITY_LEVEL_CHANGE, def_sec_level,AW_ROOT_DEFAULT)->add_callback(ed4_changesecurity,(AW_CL) def_sec_level);
    
    //    root->awar_int( AWAR_EDIT_MODE, (long) awar_edit_modus )->add_target_var((long *) &awar_edit_modus)->add_callback(ed4_changesecurity,(AW_CL) 0);    
    
    root->awar_int( AWAR_EDIT_MODE,   0)->add_callback(ed4_change_edit_mode, (AW_CL)0);
    root->awar_int( AWAR_INSERT_MODE, 1)->add_callback(ed4_change_edit_mode, (AW_CL)0);
    
    root->awar_int( AWAR_EDIT_DIRECTION, 1)->add_target_var(&awar_edit_direction);
    root->awar_int( AWAR_EDIT_HELIX_SPACING, 6)->add_target_var(&ED4_ROOT->helix_spacing);
    root->awar_int( AWAR_EDIT_TITLE_MODE, 0);
    
    root->awar_int(AWAR_CURSOR_POSITION, 1, gb_main); 
    root->awar_int(AWAR_SET_CURSOR_POSITION, 1, gb_main)->add_callback(ED4_remote_set_cursor_cb, 0, 0); 
    
    root->awar_string(AWAR_FIELD_CHOSEN, "", gb_main);
    
    root->awar_string(AWAR_SPECIES_NAME, "", gb_main)->add_callback(ED4_selected_species_changed_cb);		
    root->awar_string(AWAR_EDIT_CONFIGURATION,config_name,gb_main);
    ed4_changesecurity(root,0);

    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_GAPS,0)->add_callback(ED4_compression_toggle_changed_cb, AW_CL(0), 0);
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_HIDE,0)->add_callback(ED4_compression_toggle_changed_cb, AW_CL(1), 0);
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_TYPE,0)->add_callback(ED4_compression_changed_cb);
    root->awar_int(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT,1)->add_callback(ED4_compression_changed_cb)->set_minmax(1,99);
    root->awar_int(ED4_AWAR_COMPRESS_LEFT_COLUMN,0);
    
    root->awar_int(ED4_AWAR_DIGITS_AS_REPEAT, 0);
    root->awar_int(ED4_AWAR_FAST_CURSOR_JUMP, 0);
    root->awar_int(ED4_AWAR_CURSOR_TYPE, (int)ED4_RIGHT_ORIENTED_CURSOR);
    
    ED4_create_consensus_awars(root);
    ED4_create_NDS_awars(root);
    
    root->awar_string(ED4_AWAR_REP_SEARCH_PATTERN, ".");
    root->awar_string(ED4_AWAR_REP_REPLACE_PATTERN, "-");
    
    root->awar_int( ED4_AWAR_SCROLL_SPEED_X, 40);
    root->awar_int( ED4_AWAR_SCROLL_SPEED_Y, 20);
    
    root->awar_string(ED4_AWAR_SPECIES_TO_CREATE, "");
    
    root->awar_string(ED4_AWAR_GAP_CHARS, ".-~?")->add_callback(ED4_gap_chars_changed);
    ED4_gap_chars_changed(root);
    
    create_gde_var(ED4_ROOT->aw_root, ED4_ROOT->db, ED4_create_sequences_for_gde, CGSS_WT_EDIT4, 0);
    
    root->awar_string(ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL, "-");
    root->awar_string(ED4_AWAR_CREATE_FROM_CONS_REPL_POINT, "?");
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_CREATE_POINTS, 1);
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER, 1);
    root->awar_int(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE, 0);
}

int main(int argc,char **argv)   
{
    const char *data_path = ":";
    const char *err = NULL;
    char *config_name = NULL;
     
    if (argc > 1 && strcmp(argv[1],"-c") == 0) {
        config_name = new char[strlen(argv[2])+1];
        strcpy(config_name,argv[2]);
        argc -= 2; argv += 2;
    }
    else { // load 'default_configuration' if no command line is given
        config_name = strdup("default_configuration");
        err = "Using 'default_configuration'";
#ifndef NDEBUG	
        err = 0;
#endif	
    }

    if (argc>1)
    {
        data_path = argv[1];
        argc--; argv++;
    }
    
    gb_main = GB_open(data_path, "rwt");
    if (!gb_main)
    {
        GB_print_error();
        exit (-1);
    }

    ED4_ROOT = new ED4_root();

    ED4_ROOT->db = ED4_ROOT->aw_root->open_default( ".arb_prop/edit4.arb" );	// open default-database
    ED4_ROOT->aw_root->init_variables( ED4_ROOT->db);				// pass defaults
    ED4_ROOT->aw_root->init( "ARB_EDIT4" );					// initialize window-system 

    ED4_ROOT->database = new EDB_root_bact;
    ED4_ROOT->init_alignment();  
    ED4_create_awars(ED4_ROOT->aw_root, config_name);

    ED4_ROOT->st_ml = new_ST_ML(gb_main);
    ED4_ROOT->sequence_colors = new AWT_seq_colors((GBDATA *)ED4_ROOT->aw_root->application_database,(int)ED4_G_SEQUENCES, ED4_refresh_window, 0,0);

    ED4_ROOT->edk = new ed_key;
    ED4_ROOT->edk->create_awars(ED4_ROOT->aw_root);
    
    ED4_ROOT->sequence_colors->aww = ED4_ROOT->create_new_window();// create first editor window

    ED4_ROOT->helix = new BI_helix(ED4_ROOT->aw_root);

    if (err){
        aw_message(err);
        err = 0;
    }
    
    err = ED4_ROOT->helix->init(gb_main);
    if (err) aw_message(err);
    
    {
        int found_config = 0;

        if (config_name)
        {
            GB_begin_transaction(gb_main);
            GBDATA *gb_configuration = GBT_find_configuration(gb_main, config_name);

            if (gb_configuration) {
                GBDATA *gb_middle_area = GB_search(gb_configuration, "middle_area", GB_FIND);
                GBDATA *gb_top_area = GB_search(gb_configuration, "top_area", GB_FIND);
                char *config_data_middle = GB_read_as_string(gb_middle_area);
                char *config_data_top	= GB_read_as_string(gb_top_area);
	    
                ED4_ROOT->create_hierarchy(config_data_middle, config_data_top); // create internal hierarchy
                delete config_data_middle;
                delete config_data_top;

                found_config = 1;
            }
            else {
                aw_message(GBS_global_string("Could not find configuration '%s'", config_name));
            }
	    
            GB_commit_transaction(gb_main);
        }

        if (!found_config) {
            // create internal hierarchy
	    
            char *as_mid = ED4_ROOT->database->make_string();
            char *as_top = ED4_ROOT->database->make_top_bot_string();
	    
            ED4_ROOT->create_hierarchy(as_mid, as_top);	    
	    
            delete as_mid;
            delete as_top;
        }
    }
    
    {
        GB_transaction dummy(gb_main);
        GBDATA *species_container = GB_search(gb_main, "species_data", GB_FIND);
        GB_add_callback(species_container, (GB_CB_TYPE)GB_CB_CHANGED, (GB_CB)ED4_species_container_changed_cb, 0); // callback if species_data changes	
	
        ED4_elements_in_species_container = GB_rescan_number_of_subentries(species_container); // store # of species 
#if defined(DEBUG) && 0
        printf("Species container contains %i species (at startup)\n", ED4_elements_in_species_container);	
#endif
    }
    
    ED4_ROOT->aw_root->main_loop(); // enter main-loop
}

void AD_map_viewer(GBDATA *,AD_MAP_VIEWER_TYPE)
{
}

// --------------------------------------------------------------------------------
//	Debug-Routine dump() -- displays ED4_base-Tree (use 'p base->dump()' in debugger)
// --------------------------------------------------------------------------------

#ifdef IMPLEMENT_DUMP

static inline void spaces(int anz) {
    while (anz--) {
        fputc('|', stdout);
        fputc(' ', stdout);
    }
}

void ED4_base::dump(int depth) const {
    if (is_terminal()) {
        to_terminal()->dump(depth);
    }
    else {
        to_manager()->dump(depth);
    }
}

static void printLevel(ED4_level level) {
#define plev(tag) if (level&ED4_L_##tag) printf(#tag"|")
    
    plev(ROOT);
    plev(DEVICE);
    plev(AREA);
    plev(MULTI_SPECIES);
    plev(SPECIES);
    plev(MULTI_SEQUENCE);
    plev(SEQUENCE);
    plev(TREE);
    plev(SPECIES_NAME);
    plev(SEQUENCE_INFO);
    plev(SEQUENCE_STRING);
    plev(SPACER);
    plev(LINE);
    plev(MULTI_NAME);
    plev(NAME_MANAGER);
    plev(GROUP);
    plev(BRACKET);
    plev(PURE_TEXT);
    plev(COL_STAT);
	
#undef plev	
}

void ED4_terminal::dump(int depth) const {
    spaces(depth);
    printf("terminal ");
    printLevel(spec->level);
    printf(" id='%s' ((ED4_terminal*)0x%p)\n", id ? id : "(nil)", this);
}
void ED4_manager::dump(int depth) const {
    spaces(depth);
    printf("manager  ");
    printLevel(spec->level);
    printf(" id='%s' ((ED4_terminal*)0x%p)\n", id ? id : "(nil)", this);
    
    children->dump(depth+1);
}
void ED4_members::dump(int depth) const {
    ED4_index i;
    
    for (i=0; i<members(); i++) {
        member(i)->dump(depth);
    }
}

#endif