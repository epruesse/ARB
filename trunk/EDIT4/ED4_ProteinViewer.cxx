/*============================================================================*/
//  DNA-Protein Viewer Module:
//    => Retrieving the protein gene (DNA) sequence
//    => Translating into aminoacid sequence ( 1 letter codon)
//    => Painting the aminoacid sequences along with the DNA sequence
/*============================================================================*/

#include <stream.h>
#include "pv_header.hxx"

void RetrieveDNAsequence();
void TranslateDNA();
void PaintAminoAcids();

extern GBDATA *gb_main;

void PV_AddCallBacks(AW_root *awr) {
// ***********Binding callback function to the AWARS*****************
//    awr->awar(AWAR_PROTVIEW_DB_FIELD)->add_callback(RefreshEDITOR);
}

static void  PV_CreateAwars(AW_root *root){
    // ******** Creating AWARS to be used by the PROTVIEW module***************

    root->awar_int(AWAR_PROTVIEW_DB_FIELD, 0, AW_ROOT_DEFAULT); 

    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_1, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_2, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_3, 0, AW_ROOT_DEFAULT); 

    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_1, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_2, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_3, 0, AW_ROOT_DEFAULT); 

    root->awar_int(AWAR_PROTVIEW_DEFINED_FIELDS, 0, AW_ROOT_DEFAULT); 
}

#define BUFFERSIZE 1000
#define NAME_BUFFERSIZE 100
char      *key_string;

void  AddNewTerminalToTheSequence(ED4_multi_sequence_manager *multi_sequence_manager,
                                  ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                  ED4_sequence_terminal      *ref_sequence_terminal,
                                  ED4_index                  *max_sequence_terminal_length)
//                                                       int                         count_too,
//                                  ED4_index                  *seq_coords)

{
    int count_too = 0;
    ED4_index *seq_coords;

    AW_device *device;
    int        j          = 0;
    int        pixel_length;
    long       string_length;
    char       namebuffer[NAME_BUFFERSIZE];

    sprintf( namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count_too++ );
    ED4_sequence_manager *seq_manager = new ED4_sequence_manager( namebuffer, 0, j*TERMINALHEIGHT, 0, 0, multi_sequence_manager );
    seq_manager->set_properties( ED4_P_MOVABLE );
    multi_sequence_manager->children->append_member( seq_manager );

    ED4_sequence_info_terminal *sequence_info_terminal =
        new ED4_sequence_info_terminal(key_string, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, seq_manager );
    sequence_info_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
    sequence_info_terminal->set_links( ref_sequence_info_terminal, ref_sequence_info_terminal );
    //         sequence_info_terminal->set_species_pointer(gb_alignment);    // segmentation fault
    seq_manager->children->append_member( sequence_info_terminal );

    ED4_text_terminal *text_terminal = 0;
    sprintf(namebuffer, "PureText_Term%ld.%d",ED4_counter, count_too++);
    text_terminal = new ED4_pure_text_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, seq_manager);

    text_terminal->set_properties( ED4_P_CURSOR_ALLOWED );
    text_terminal->set_links( ref_sequence_terminal, ref_sequence_terminal );
    seq_manager->children->append_member(text_terminal);
    //           text_terminal->set_species_pointer(gb_alignment);

    string_length = 100;

    device = ED4_ROOT->first_window->aww->get_device(AW_MIDDLE_AREA);
    pixel_length = device->get_string_size( ED4_G_SEQUENCES, NULL, string_length) + 100;

    *max_sequence_terminal_length = max(*max_sequence_terminal_length, pixel_length);
    text_terminal->extension.size[WIDTH] = pixel_length;

    if (MAXSEQUENCECHARACTERLENGTH < string_length) {
        MAXSEQUENCECHARACTERLENGTH = string_length;
        ref_sequence_terminal->extension.size[WIDTH] = pixel_length;
    }

    if (!ED4_ROOT->scroll_links.link_for_hor_slider) {
        ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
    }
    else if (*max_sequence_terminal_length > ED4_ROOT->scroll_links.link_for_hor_slider->extension.size[WIDTH]) {
        ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
    }

    *seq_coords += TERMINALHEIGHT;
}

void PV_CreateNewTerminal() {

    GB_transaction dummy(gb_main);
    int marked = GBT_count_marked_species(gb_main);

    if (marked) {
        GBDATA *gb_species = GBT_first_marked_species(gb_main);
        int count = 0;
        char *buffer = new char[BUFFERSIZE+1];
        char *bp = buffer;
        //	int inbuf = 0; // # of names in buffer
        ED4_multi_species_manager *insert_into_manager = ED4_new_species_multi_species_manager();
        ED4_group_manager *group_man = insert_into_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        int group_depth = insert_into_manager->calc_group_depth();
        int index = 0;
        ED4_index y = 0;
        ED4_index lot = 0;
        int inserted = 0;
        char *default_alignment = GBT_get_default_alignment(gb_main);

        aw_openstatus("ARB_EDIT4");
        aw_status("Loading species...");

        all_found         = 0;
        not_found_message = GBS_stropen(1000);
        GBS_strcat(not_found_message,"Species not found: ");

        while (gb_species) {
            count++;
            GB_status(double(count)/double(marked));

            char *name = GBT_read_name(gb_species);
            ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);

            if (name_term) {
                {
#if defined(DEVEL_YADHU)
                    extern bool bIsSpecies;
                    // its nearly always a very bad idea to declare something extern
                    
                    bIsSpecies = true;
#error Linker error :  undefined reference to bIsSpecies
#endif // DEVEL_YADHU

                    ED4_species_manager *species_manager = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
                    ED4_multi_sequence_manager *multiSeqManager = species_manager->search_spec_child_rek(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();

                    int namelen = strlen(name);
                    int buffree = BUFFERSIZE-int(bp-buffer);

                    if ((namelen+2)>buffree) {
                        *bp++ = 0;
                        ED4_ROOT->database->fill_species(insert_into_manager,
                                                         ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                                         buffer, &index, &y, 0, &lot, group_depth);
                        bp = buffer;
                        index = 0;
                    }

                    *bp++ = 1;
                    *bp++ = 'L';
                    memcpy(bp, name, namelen);
                    bp += namelen;
                }

                {
                    GBDATA *gbd = GBT_read_sequence(gb_species, default_alignment);

                    if (gbd) {
                        char *data = GB_read_string(gbd);
                        int len = GB_read_string_count(gbd);
                        group_man->table().add(data, len);
                    }
                }

                inserted++;
            }
            if (inserted ==1) name_term->kill_object();
            gb_species = GBT_next_marked_species(gb_species);
        }

        if (bp>buffer) {
            *bp++ = 0;
            ED4_ROOT->database->fill_species(insert_into_manager,
                                             ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                             buffer, &index, &y, 0, &lot, group_depth);
        }

        aw_closestatus();
        aw_message(GBS_global_string("Loaded %i of %i marked species.", inserted, marked));

        {
            char *out_message = GBS_strclose(not_found_message);
            not_found_message = 0;
            if (all_found != 0) aw_message(out_message);
            free(out_message);
        }

        if (inserted) {
            ED4_ROOT->main_manager->update_info.set_resize(1);
            ED4_ROOT->main_manager->resize_requested_by_parent();
        }

        delete buffer;
        delete default_alignment;
    }
    else {
        aw_message("No species marked.");
    }

    ED4_ROOT->refresh_all_windows(0);
}

// void PV_CreateNewTerminal() {

//     GB_transaction dummy(gb_main);
//     int marked = GBT_count_marked_species(gb_main);

//     if (marked) {
//         GBDATA *gb_species = GBT_first_marked_species(gb_main);
//         int count = 0;
//         int inserted = 0;

//         char *default_alignment = GBT_get_default_alignment(gb_main);

//         aw_openstatus("PROTEIN VIEWVER");
//         aw_status("Inserting NEW Terminal...");
//         cout<<"Terminals found for the following species....."<<endl;

//         while (gb_species) {
//             count++;
//             GB_status(double(count)/double(marked));

//             {
//                 GBDATA  *gb_alignment = GB_find(gb_species,ED4_ROOT->alignment_name,0,down_level);
//                 gb_alignment = GB_find(gb_alignment,0,0,down_level);
//                 key_string = GB_read_key( gb_alignment );
//             }

//             char *name = GBT_read_name(gb_species);
//             ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);

//             //            int name_len;
//             //            char *speciesName = name_term->resolve_pointer_to_string_copy(&name_len); // getting the name of the species from its terminal

//             if (name_term)  { // if species found create new terminal
//                 cout<<count<<". "<<name<<endl;
         
//                 ED4_move_info mi;
//                 ED4_selection_entry *selInfo = name_term->selection_info;

// //                 mi.object = name_term;
// //                 mi.end_x = name_term->lastXpos;
// //                 mi.end_y = name_term->lastYpos;
// //                 mi.mode = ED4_M_FREE;
// //                 mi.preferred_parent = ED4_L_NO_LEVEL;

// //                 name_term->parent->move_requested_by_child(&mi);
             
//                  ED4_species_manager *species_manager = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
//                  ED4_multi_sequence_manager *multiSeqManager = species_manager->search_spec_child_rek(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                
//                  AddNewTerminalToTheSequence(multiSeqManager, 
//                                              ED4_ROOT->ref_terminals.get_ref_sequence_info(), 
//                                              ED4_ROOT->ref_terminals.get_ref_sequence(), &max_seq_terminal_length);
                
//                 inserted++;
//             }
//             if (inserted>0) ED4_R_BREAK;
//             else gb_species = GBT_next_marked_species(gb_species);
//         }

//         aw_closestatus();
//         aw_message(GBS_global_string(" %i of %i terminals created.", inserted, marked));

//         if (inserted) {
//             ED4_ROOT->main_manager->update_info.set_resize(1);
//             ED4_ROOT->main_manager->resize_requested_by_parent();
//         }

//         delete default_alignment;
//     }
//     else {
//         aw_message("No species marked to create new terminal.");
//     }

//     ED4_ROOT->refresh_all_windows(0);
// }

#undef NAME_BUFFERSIZE
#undef BUFFERSIZE

void StartTranslation(AW_window *aww){
    PV_CreateNewTerminal();

    AW_root *aw_root = aww->get_root();
    GB_ERROR error;


//         GB_begin_transaction(gb_main);

// #if defined(DEBUG) && 0
//     test_AWT_get_codons();
// #endif

//     awt_pro_a_nucs_convert_init(gb_main);

//     char *ali_source    = aw_root->awar(AWAR_TRANSPRO_SOURCE)->read_string();
//     char *ali_dest      = aw_root->awar(AWAR_TRANSPRO_DEST)->read_string();
//     char *mode          = aw_root->awar(AWAR_TRANSPRO_MODE)->read_string();
//     int   startpos      = aw_root->awar(AWAR_TRANSPRO_POS)->read_int();
//     bool  save2fields   = aw_root->awar(AWAR_TRANSPRO_WRITE)->read_int();
//     bool  translate_all = aw_root->awar(AWAR_TRANSPRO_XSTART)->read_int();

//     error = arb_r2a(gb_main, strcmp(mode, "fields") == 0, save2fields, startpos, translate_all, ali_source, ali_dest);
//     if (error) {
//         GB_abort_transaction(gb_main);
//         aw_message(error);
//     }else{
//         GBT_check_data(gb_main,0);
//         GB_commit_transaction(gb_main);
//     }

//     free(mode);
//     free(ali_dest);
//     free(ali_source);
}

AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root) {
    GB_transaction dummy(gb_main);

    PV_CreateAwars (aw_root); // Creating AWARS to be used by the PROTVIEW module

    static AW_window_simple *aws = 0;
    if (aws) return aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "PROTEIN_VIEWER", "Protein Viewer");
    aws->load_xfig("proteinViewer.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"proteinViewer.hlp");
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP","#helpText.xpm");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE","#closeText.xpm");

    {
        aw_root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, gb_main);
        aws->at("table");
        aws->create_option_menu(AWAR_PROTEIN_TYPE);
        for (int code_nr=0; code_nr<AWT_CODON_TABLES; code_nr++) {
            aws->insert_option(AWT_get_codon_code_name(code_nr), "", code_nr);
        }
        aws->update_option_menu();

        aws->at("DBfield");
        aws->create_toggle(AWAR_PROTVIEW_DB_FIELD);

        aws->at("frwd");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_TRANSLATION);

        aws->at("f1");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_TRANSLATION_1);

        aws->at("f2");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_TRANSLATION_2);

        aws->at("f3");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_TRANSLATION_3);

        aws->at("rvrs");
        aws->create_toggle(AWAR_PROTVIEW_REVERSE_TRANSLATION);

        aws->at("r1");
        aws->create_toggle(AWAR_PROTVIEW_REVERSE_TRANSLATION_1);

        aws->at("r2");
        aws->create_toggle(AWAR_PROTVIEW_REVERSE_TRANSLATION_2);

        aws->at("r3");
        aws->create_toggle(AWAR_PROTVIEW_REVERSE_TRANSLATION_3);

        aws->at("defined");
        aws->create_toggle(AWAR_PROTVIEW_DEFINED_FIELDS);

        aws->at("go"); 
        aws->callback(StartTranslation);  // translation function
        aws->button_length(25);
        aws->create_button("START","TRANSLATE & DISPLAY", "T");
    }

    PV_AddCallBacks(aw_root);  // binding callback function to the AWARS

    aws->show();

    return (AW_window *)aws;
}
