/*============================================================================*/
//  DNA-Protein Viewer Module:
//    => Retrieving the protein gene (DNA) sequence
//    => Translating into aminoacid sequence ( 1 letter codon)
//    => Painting the aminoacid sequences along with the DNA sequence
/*============================================================================*/

#include <iostream>
#include "pv_header.hxx"
#include "ed4_ProteinViewer.hxx"

using namespace std;

void RetrieveDNAsequence();
void TranslateDNA();
void PaintAminoAcids();
void PV_CreateAllTerminals();

extern GBDATA *gb_main;
static bool hide = false;
static bool gTerminalsCreated = false;
#define BUFFERSIZE 1000

// --------------------------------------------------------------------------------
//        Binding callback function to the AWARS
// --------------------------------------------------------------------------------
void PV_AddCallBacks(AW_root *awr) {

    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_1)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_2)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_3)->add_callback(PV_CreateAllTerminals);

    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_1)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_2)->add_callback(PV_CreateAllTerminals);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_3)->add_callback(PV_CreateAllTerminals);

    awr->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->add_callback(PV_CreateAllTerminals);
}

// --------------------------------------------------------------------------------
//        Creating AWARS to be used by the PROTVIEW module
// --------------------------------------------------------------------------------
void  PV_CreateAwars(AW_root *root, AW_default aw_def){

    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_1, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_2, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_TRANSLATION_3, 0, aw_def); 

    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_1, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_2, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_REVERSE_TRANSLATION_3, 0, aw_def); 

    root->awar_int(AWAR_PROTVIEW_DEFINED_FIELDS, 0, aw_def); 
}

bool LookForNewTerminals(AW_root *root){
    bool bTerminalsFound = false;

    if(gTerminalsCreated) bTerminalsFound = true;
    else {
        int frwdTrans  = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION)->read_int();
        int frwdTrans1 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_1)->read_int();
        int frwdTrans2 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_2)->read_int();
        int frwdTrans3 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_3)->read_int();
        int rvrsTrans  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION)->read_int();
        int rvrsTrans1 = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_1)->read_int();
        int rvrsTrans2 = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_2)->read_int();
        int rvrsTrans3 = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_3)->read_int();
        int defFields  = root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();
        
        if((frwdTrans && (frwdTrans1 || frwdTrans2 || frwdTrans3)) &&
           (rvrsTrans && (rvrsTrans1 || rvrsTrans2 || rvrsTrans3)) &&
           (defFields)){
            bTerminalsFound = false; 
        }
    }

    return bTerminalsFound;
}

void PV_CreateAllTerminals(AW_root *root) {
    // Look for already created terminals, if found do nothing, otherwise, create
    // new terminals and set gTerminalsCreated to true

    bool bTerminalsFound = LookForNewTerminals(root);
    if(bTerminalsFound) return;

    // Idea: 
    // 1. Get the species terminal pointer
    // 2. Append the second terminal
    // 3. resize the multi-species manager
    // 4. refresh all the terminals including new appended terminals

    GB_transaction dummy(gb_main);
    int marked = GBT_count_marked_species(gb_main);

    if (marked) {
        int appended = 0;

        GBDATA *gb_species = GBT_first_marked_species(gb_main);
        char *default_alignment = GBT_get_default_alignment(gb_main);

        aw_openstatus("PROTEIN VIEWVER");
        aw_status("Inserting NEW Terminal...");
        cout<<"New Terminals are created and appended for the following species....."<<endl;

        while (gb_species) {

            char *speciesName = GBT_read_name(gb_species);
            ED4_species_name_terminal *speciesNameTERMINAL = ED4_find_species_name_terminal(speciesName);

            if (speciesNameTERMINAL->is_species_name_terminal()) {

                ED4_group_manager               *curr_GroupMANAGER    = 0;
                ED4_species_manager             *curr_SpeciesMANAGER  = 0;
                ED4_multi_sequence_manager  *curr_MultiSeqMANAGER = 0;
                ED4_sequence_manager          *curr_SeqMANAGER       = 0;
                ED4_sequence_terminal           *curr_SeqTERMINAL      = 0;
                ED4_sequence_info_terminal    *curr_SeqInfoTERMINAL = 0;

                curr_GroupMANAGER    = speciesNameTERMINAL->get_parent(ED4_L_GROUP)->to_group_manager();
                curr_SpeciesMANAGER  = speciesNameTERMINAL->get_parent(ED4_L_SPECIES)->to_species_manager();
                curr_MultiSeqMANAGER = curr_SpeciesMANAGER->search_spec_child_rek(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                curr_SeqMANAGER       = curr_SpeciesMANAGER->search_spec_child_rek(ED4_L_SEQUENCE)->to_sequence_manager();
                curr_SeqTERMINAL       = curr_SpeciesMANAGER->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();
                curr_SeqInfoTERMINAL = curr_SpeciesMANAGER->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();
     
                char namebuffer[BUFFERSIZE];
                int count = 1;
                sprintf( namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count++);

                ED4_sequence_manager *new_SeqMANAGER = new ED4_sequence_manager(namebuffer, 0, 0, 0, 0, curr_MultiSeqMANAGER);
                new_SeqMANAGER->set_properties(ED4_P_MOVABLE);
                curr_MultiSeqMANAGER->children->append_member(new_SeqMANAGER);

                ED4_sequence_info_terminal *ref_sequence_info_terminal = ED4_ROOT->ref_terminals.get_ref_sequence_info();

                ED4_sequence_info_terminal *new_SeqInfoTERMINAL = 0;
                sprintf(namebuffer, "PRotienInfo_Term%ld.%d",ED4_counter, count++);
                new_SeqInfoTERMINAL = new ED4_sequence_info_terminal(namebuffer, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, new_SeqMANAGER );
                new_SeqInfoTERMINAL->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
                new_SeqInfoTERMINAL->set_links( curr_SeqInfoTERMINAL, curr_SeqTERMINAL );
                new_SeqMANAGER->children->append_member( new_SeqInfoTERMINAL );

                GBDATA *gb_SeqData = GBT_read_sequence(gb_species, default_alignment);
                char *str_SeqData = 0;
                int len = 0;
                if (gb_SeqData) {
                    str_SeqData = GB_read_string(gb_SeqData);
                    len = GB_read_string_count(gb_SeqData);
                }

                //                for(int i =0;i<len;i++) { if(str_SeqData[i]!='.' && str_SeqData[i]!='-') cout<<str_SeqData[i];} cout<<endl;

                GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, 0);// set wanted protein table
                awt_pro_a_nucs_convert_init(gb_main); // (re-)initialize codon tables for current translation table
                int stops = AWT_pro_a_nucs_convert(str_SeqData, len, 1, "true");

                GB_ERROR error = 0;
                GBDATA *gb_ProSeqData = 0;
                gb_ProSeqData = GBT_add_data(gb_species,"ali_protein_seq","data", GB_STRING);
                if (!gb_ProSeqData) {cout<<"ERROR : "<<GB_get_error()<<endl;exit(0);}
                //              error        = GB_write_string(gb_ProSeqData,str_SeqData);

                char *s = (char*)malloc(len+1);
                int j=0, k=0;
                for(int i =0;i<len;i++) {
                    if((k==i) && (j<len)) {
                        s[i]=str_SeqData[j++];
                        k+=3;
                    }
                    else 
                        s[i]='~';
                }

                error  = GB_write_string(gb_ProSeqData,s);
                free(str_SeqData);

                if (error) {cout<<"error during writing data.....!!!"<<endl;exit(0);}

                ED4_AA_sequence_terminal *new_SeqTERMINAL = 0;
                sprintf(namebuffer, "Sequence_Term%ld.%d",ED4_counter, count++);
                new_SeqTERMINAL = new ED4_AA_sequence_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, new_SeqMANAGER);
                //                new_SeqTERMINAL->set_properties( ED4_P_CURSOR_ALLOWED );
                new_SeqTERMINAL->set_links( curr_SeqTERMINAL, curr_SeqTERMINAL );
                //                new_SeqTERMINAL->species_name = curr_SeqTERMINAL->get_name_of_species();
                new_SeqTERMINAL->set_species_pointer(gb_ProSeqData);
                new_SeqMANAGER->children->append_member(new_SeqTERMINAL);

//                 ED4_text_terminal *new_TextTERMINAL = 0;
//                 sprintf(namebuffer, "PureText_Term%ld.%d",ED4_counter, count++);
//                 new_TextTERMINAL = new ED4_pure_text_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, new_SeqMANAGER);
//                 //                new_TextTERMINAL->set_properties( ED4_P_CURSOR_ALLOWED );
//                 new_TextTERMINAL->set_links( curr_SeqTERMINAL, curr_SeqTERMINAL );
//                 new_TextTERMINAL->set_species_pointer(gb_SeqData);
//                 new_SeqMANAGER->children->append_member(new_TextTERMINAL);

                ED4_counter++;

                new_SeqMANAGER->resize_requested_by_child();

//                 if(hide) new_SeqMANAGER->hide_children();
//                 else {new_SeqMANAGER->make_children_visible();hide=!hide;}
//                ED4_base proteinSeqTerminal = curr_MultiSeqMANAGER->search_ID("ProteinInfo_Term");

                appended++;
                cout<<appended<<" : "<<speciesName<<endl;
            }
            else {
                cout<<"ERROR: Failed to find the species terminal for the species \""<<speciesName<<"\" !"<<endl
                    <<"New sequence terminals are not created for the same!"<<endl;
            }
            gb_species = GBT_next_marked_species(gb_species);
        } 
        aw_closestatus();
        aw_message(GBS_global_string("Created %i terminals and appended to the respective species.", appended));

        if (appended) {
            ED4_ROOT->main_manager->update_info.set_resize(1);
            ED4_ROOT->main_manager->resize_requested_by_parent();
        }
    }
    else {
        aw_message("No species marked to create new terminals!!");
    }

    gTerminalsCreated = true;

    ED4_ROOT->refresh_all_windows(0);
}

void StartTranslation(AW_window *aww){

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

//         aws->at("go"); 
//         aws->callback(StartTranslation);  // translation function
//         aws->button_length(25);
//         aws->create_button("START","DISPLAY", "D");
    }

    PV_AddCallBacks(aw_root);  // binding callback function to the AWARS

    // Create All Terminals 
    PV_CreateAllTerminals(aw_root);

    aws->show();

    return (AW_window *)aws;
}

// char      *key_string;
// void  AddNewTerminalToTheSequence(ED4_multi_sequence_manager *multi_sequence_manager,
//                                   ED4_sequence_info_terminal *ref_sequence_info_terminal,
//                                   ED4_sequence_terminal      *ref_sequence_terminal,
//                                   ED4_index                  *max_sequence_terminal_length)
// //                                                       int                         count_too,
// //                                  ED4_index                  *seq_coords)

// {
//     int count_too = 0;
//     ED4_index *seq_coords;

//     AW_device *device;
//     int        j          = 0;
//     int        pixel_length;
//     long       string_length;
//     char       namebuffer[BUFFERSIZE];

//     sprintf( namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count_too++ );
//     ED4_sequence_manager *seq_manager = new ED4_sequence_manager( namebuffer, 0, j*TERMINALHEIGHT, 0, 0, multi_sequence_manager );
//     seq_manager->set_properties( ED4_P_MOVABLE );
//     multi_sequence_manager->children->append_member( seq_manager );

//     ED4_sequence_info_terminal *sequence_info_terminal =
//         new ED4_sequence_info_terminal(key_string, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, seq_manager );
//     sequence_info_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
//     sequence_info_terminal->set_links( ref_sequence_info_terminal, ref_sequence_info_terminal );
//     //         sequence_info_terminal->set_species_pointer(gb_alignment);    // segmentation fault
//     seq_manager->children->append_member( sequence_info_terminal );

//     ED4_text_terminal *text_terminal = 0;
//     sprintf(namebuffer, "PureText_Term%ld.%d",ED4_counter, count_too++);
//     text_terminal = new ED4_pure_text_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, seq_manager);

//     text_terminal->set_properties( ED4_P_CURSOR_ALLOWED );
//     text_terminal->set_links( ref_sequence_terminal, ref_sequence_terminal );
//     seq_manager->children->append_member(text_terminal);
//     //           text_terminal->set_species_pointer(gb_alignment);

//     string_length = 100;

//     device = ED4_ROOT->first_window->aww->get_device(AW_MIDDLE_AREA);
//     pixel_length = device->get_string_size( ED4_G_SEQUENCES, NULL, string_length) + 100;

//     *max_sequence_terminal_length = max(*max_sequence_terminal_length, pixel_length);
//     text_terminal->extension.size[WIDTH] = pixel_length;

//     if (MAXSEQUENCECHARACTERLENGTH < string_length) {
//         MAXSEQUENCECHARACTERLENGTH = string_length;
//         ref_sequence_terminal->extension.size[WIDTH] = pixel_length;
//     }

//     if (!ED4_ROOT->scroll_links.link_for_hor_slider) {
//         ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
//     }
//     else if (*max_sequence_terminal_length > ED4_ROOT->scroll_links.link_for_hor_slider->extension.size[WIDTH]) {
//         ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
//     }

//     *seq_coords += TERMINALHEIGHT;
// }

#undef BUFFERSIZE
