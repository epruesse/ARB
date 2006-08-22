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
// Definitions used
#define BUFFERSIZE            1000
#define FORWARD_STRAND        3
#define COMPLEMENTARY_STRAND  3
#define DB_FIELD_STRAND       1

// Global Variables
extern GBDATA *gb_main;
static bool gTerminalsCreated = false;
static int PV_AA_Terminals4Species = 0;
AW_root *gRoot = 0;

// Functions 
void PV_AddCallBacks(AW_root *awr);
void PV_CreateAwars(AW_root *root, AW_default aw_def);
bool PV_LookForNewTerminals(AW_root *root);
void PV_CallBackFunction(AW_root *root);
//void PV_AA_SequenceUpdate_CB(AW_root *root);
void PV_AA_SequenceUpdate_CB(GBDATA *gb_species_data, int */*cl*/, GB_CB_TYPE gbtype);
void PV_CreateAllTerminals(AW_root *root);
void PV_ManageTerminals(AW_root *root);
void PV_HideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal);
void PV_UnHideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal);
GBDATA *TranslateGeneToAminoAcidSequence(AW_root *root, char *speciesName, int startPos4Translation, bool complementaryStrand);
AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root);

// --------------------------------------------------------------------------------
//        Binding callback function to the AWARS
// --------------------------------------------------------------------------------
void PV_AddCallBacks(AW_root *awr) {

    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->add_callback(PV_CallBackFunction);
    
    {
        GB_transaction dummy(gb_main);
        GBDATA *spContainer = GB_search(gb_main, "species_data", GB_FIND);
        // callback if sequence alignment changes
        GB_add_callback(spContainer, (GB_CB_TYPE)GB_CB_CHANGED, (GB_CB)PV_AA_SequenceUpdate_CB, 0); 
    }
    
    //    awr->awar(SPECIES_NAME)->add_callback(PV_AA_SequenceUpdate_CB);
}

// --------------------------------------------------------------------------------
//        Creating AWARS to be used by the PROTVIEW module
// --------------------------------------------------------------------------------
void PV_CreateAwars(AW_root *root, AW_default aw_def){

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

bool PV_LookForNewTerminals(AW_root *root){
    bool bTerminalsFound = true;

    int frwdTrans  = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION)->read_int();
    int frwdTrans1 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_1)->read_int();
    int frwdTrans2 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_2)->read_int();
    int frwdTrans3 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_3)->read_int();
    int rvrsTrans   = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION)->read_int();
    int rvrsTrans1  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_1)->read_int();
    int rvrsTrans2  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_2)->read_int();
    int rvrsTrans3  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_3)->read_int();
    int defFields     = root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();
    
    // Test whether any of the options are selected or not? 
    if((frwdTrans && (frwdTrans1 || frwdTrans2 || frwdTrans3)) ||
       (rvrsTrans && (rvrsTrans1 || rvrsTrans2 || rvrsTrans3)) ||
       (defFields))
        {
            // if so, then test whether the terminals are created already or not?
            if(gTerminalsCreated) {
                // if yes, then set the flag to true
                bTerminalsFound = true;
            } 
            else {
                //if not, then new terminals has to be created
                bTerminalsFound = false;
            }
        }
    return bTerminalsFound;
}

void PV_CallBackFunction(AW_root *root) {
    // Create New Terminals If Aminoacid Sequence Terminals Are Not Created 
    if(!gTerminalsCreated){
        PV_CreateAllTerminals(root);
    }
    // Manage the terminals showing only those selected by the user
    if(gTerminalsCreated) {
        PV_ManageTerminals(root);
    }
    ED4_ROOT->refresh_all_windows(0);
}

void PV_HideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal) {
    ED4_sequence_manager *seqManager = aaSeqTerminal->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->hide_children();
}

void PV_UnHideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal) {
    ED4_sequence_manager *seqManager = aaSeqTerminal->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->make_children_visible();
}

void PV_ManageTerminals(AW_root *root){
    int frwdTrans  = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION)->read_int();
    int frwdTrans1 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_1)->read_int();
    int frwdTrans2 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_2)->read_int();
    int frwdTrans3 = root->awar(AWAR_PROTVIEW_FORWARD_TRANSLATION_3)->read_int();
    int rvrsTrans   = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION)->read_int();
    int rvrsTrans1  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_1)->read_int();
    int rvrsTrans2  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_2)->read_int();
    int rvrsTrans3  = root->awar(AWAR_PROTVIEW_REVERSE_TRANSLATION_3)->read_int();

    ED4_terminal *terminal = 0;
    for( terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;  
         terminal = terminal->get_next_terminal())
        {
            if(terminal->is_sequence_terminal()) {
                ED4_sequence_terminal *seqTerminal = terminal->to_sequence_terminal();
                ED4_species_manager *speciesManager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
                if (speciesManager && !speciesManager->flag.is_consensus && !speciesManager->flag.is_SAI) 
                    {
                        // we are in the sequence terminal section of a species
                        // walk through all the corresponding AA sequence terminals for the speecies and 
                        // hide or unhide the terminals based on the display options set by the user
                        for(int i=0; i<PV_AA_Terminals4Species; i++) 
                            {
                                // get the corresponding AA_sequence_terminal skipping sequence_info terminal
                                // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                                terminal = terminal->get_next_terminal()->get_next_terminal(); 
                                if (terminal->is_aa_sequence_terminal()) {
                                    ED4_AA_sequence_terminal *aaSeqTerminal = terminal->to_aa_sequence_terminal();
                                    // Check AliType to make sure it is AA sequence terminal
                                    GB_alignment_type AliType = aaSeqTerminal->GetAliType();
                                    if (AliType && (AliType==GB_AT_AA)) {
                                        // Get the AA sequence flag - says which strand we are in 
                                        int aaSeqFlag = int(aaSeqTerminal->GET_aaSeqFlag()); 
                                        // Check the display options and make visible or hide the AA seq terminal
                                        switch (aaSeqFlag) 
                                            {
                                            case 1: 
                                                (frwdTrans && frwdTrans1)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 2: 
                                                (frwdTrans && frwdTrans2)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 3: 
                                                (frwdTrans && frwdTrans3)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 4: 
                                                (rvrsTrans && rvrsTrans1)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 5: 
                                                (rvrsTrans && rvrsTrans2)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 6: 
                                                (rvrsTrans && rvrsTrans3)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            }
                                    }
                                }
                            }
                    }
            }
        }
}

void PV_ComplementarySequence(char *sequence)
{
    int seqLen = strlen(sequence);
    char *temp_str = (char*)malloc(seqLen+1);
    int i;

    for(i=0; i<seqLen; i++)
    {
        if ((sequence[i]=='A') || (sequence[i]=='a') ||
            (sequence[i]=='G') || (sequence[i]=='g') ||
            (sequence[i]=='C') || (sequence[i]=='c') ||
            (sequence[i]=='U') || (sequence[i]=='u') ||
            (sequence[i]=='T') || (sequence[i]=='t'))
            {
                switch(sequence[i])
                    {
                    case 'A':
                        temp_str[i] = 'U';
                        break;
                    case 'a':
                        temp_str[i] = 'u';
                        break;
                    case 'C':
                        temp_str[i] = 'G';
                        break;
                    case 'c':
                        temp_str[i] = 'g';
                        break;
                    case 'G':
                        temp_str[i] = 'C';
                        break;
                    case 'g':
                        temp_str[i] = 'c';
                        break;
                    case 'T':
                    case 'U':
                        temp_str[i] = 'A';
                        break;
                    case 't':
                    case 'u':
                        temp_str[i] = 'a';
                        break;
                    }
            }
        else {
            temp_str[i] = sequence[i];
        }
    }
    temp_str[i] = '\0';
    strcpy(sequence, temp_str);
    if(temp_str != NULL)
    {
        free(temp_str);
        temp_str = NULL;
    }
}

GBDATA *TranslateGeneToAminoAcidSequence(AW_root *root, char *speciesName, int startPos4Translation, bool complementaryStrand){
    GBDATA    *gb_species = GBT_find_species(gb_main, speciesName);
    char *defaultAlignment = GBT_get_default_alignment(gb_main);
    GBDATA  *gb_SeqData = GBT_read_sequence(gb_species, defaultAlignment);

    e4_assert(startPos4Translation>=0 && startPos4Translation<=2);

    char *str_SeqData = 0;
    int len = 0;
    if (gb_SeqData) {
        str_SeqData = GB_read_string(gb_SeqData);
        len = GB_read_string_count(gb_SeqData);
    }

    int translationTable  = root->awar(AWAR_PROTEIN_TYPE)->read_int();

    GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, translationTable);// set wanted protein table
    awt_pro_a_nucs_convert_init(gb_main); // (re-)initialize codon tables for current translation table
    //AWT_pro_a_nucs_convert(char *seqdata, long seqlen, int startpos, bool translate_all)
    if(complementaryStrand) PV_ComplementarySequence(str_SeqData);
    int stops = AWT_pro_a_nucs_convert(str_SeqData, len, startPos4Translation, "true");

    GB_ERROR error = 0;
    GBDATA *gb_ProSeqData = 0;
    char buf[100];
    if(complementaryStrand)  sprintf(buf, "ali_temp_aa_seq_%ld",startPos4Translation+FORWARD_STRAND);
    else                     sprintf(buf, "ali_temp_aa_seq_%ld",startPos4Translation);
    gb_ProSeqData = GBT_add_data(gb_species,buf,"data", GB_STRING);
    if (!gb_ProSeqData) {
        cout<<"ERROR : "<<GB_get_error()<<endl;
        exit(0);
    }

    bool skipStartX = true;
    char *s = (char*)malloc(len+1);
    int i, j=0, k=0;
    switch (startPos4Translation)
        {
        case START_POS_1:
            for(i = 0; i<len; i++) {
                if((k==i) && (j<len)) {
                    s[i]=str_SeqData[j++];
                    k+=3;
                }
                else 
                    s[i]='_';
            }
            break;
        case START_POS_2:
            k=1;
            for(i = 0; i<len; i++) {
//                 if(skipStartX) {
//                     if (str_SeqData[j]=='X') j += 3;
//                     skipStartX = false;
//                 }
//                 else {
                    if((k==i) && (j<len)) {
                        s[i]=str_SeqData[j++];
                        k+=3;
                    }
                    else 
                        s[i]='_';
                }
            //            }
            break;

        case START_POS_3:
            k=2;
            for(i = 0; i<len; i++) {
                if((k==i) && (j<len)) {
                    s[i]=str_SeqData[j++];
                    k+=3;
                }
                else 
                    s[i]='_';
            }
            break;
        }
    s[i]='\0';

    error  = GB_write_string(gb_ProSeqData,s);
    free(str_SeqData);
    if (error) {
        cout<<"error during writing translated data.....!!!"<<endl;
        exit(0);
    }
    return gb_ProSeqData;
}

void PV_AA_SequenceUpdate_CB(GBDATA */*gb_species_data*/, int */*cl*/, GB_CB_TYPE gbtype)
{
    if (gbtype==GB_CB_CHANGED) {
        GB_transaction dummy(gb_main);
        
        ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
        if (cursor->owner_of_cursor) {
            ED4_terminal *cursorTerminal = cursor->owner_of_cursor->to_terminal();
            
            if (!cursorTerminal->parent->parent->flag.is_consensus) 
                {
                    ED4_sequence_terminal *seqTerminal = cursorTerminal->to_sequence_terminal();
                    char                  *speciesName = seqTerminal->species_name; 
                    bool complementaryStrand;

                    for(int i=0; i<PV_AA_Terminals4Species; i++) 
                        {
                            // get the corresponding AA_sequence_terminal skipping sequence_info terminal
                            // $$$$$ sequence_terminal->sequence_info_terminal->aa_sequence_terminal $$$$$$
                            cursorTerminal = cursorTerminal->get_next_terminal()->get_next_terminal(); 
                            if (cursorTerminal->is_aa_sequence_terminal()) {
                                ED4_AA_sequence_terminal *aaSeqTerminal = cursorTerminal->to_aa_sequence_terminal();
                                // Check AliType to make sure it is AA sequence terminal
                                GB_alignment_type AliType = aaSeqTerminal->GetAliType();
                                if (AliType && (AliType==GB_AT_AA)) {
                                    // Get the AA sequence flag - says which strand we are in 
                                    int aaSeqFlag = int(aaSeqTerminal->GET_aaSeqFlag()); 
                                    // retranslate the genesequence and store it to the AA_sequnce_terminal
                                    e4_assert (speciesName);
                                    int startPos;
                                    if(i<FORWARD_STRAND) {
                                        startPos = aaSeqFlag-1;
                                        complementaryStrand = false;
                                    }
                                    else {
                                        startPos = (aaSeqFlag-FORWARD_STRAND)-1;
                                        complementaryStrand = true;
                                    }
                                    aaSeqTerminal->aaSequence = TranslateGeneToAminoAcidSequence(gRoot, speciesName, startPos, complementaryStrand);
                                    aaSeqTerminal->set_species_pointer(aaSeqTerminal->aaSequence);
                                    
                                }
                            }
                        }
                }
        }
    }
}

void PV_CreateAllTerminals(AW_root *root) {
    // Idea: 1
    // 1. Get the species terminal pointer
    // 2. Append the second terminal
    // 3. resize the multi-species manager
    // 4. refresh all the terminals including new appended terminals

    // Look for already created terminals, if found do nothing, otherwise, create
    // new terminals and set gTerminalsCreated to true
    bool bTerminalsFound = PV_LookForNewTerminals(root);
    // if terminals are already created then do nothing exit the function
    if(bTerminalsFound) return;

    GB_transaction dummy(gb_main);

    // Number of AA sequence terminals to be created = 3 forward strands + 3 complementary strands + 1 DB field strand
    // totally 7 strands has to be created
    int aaTerminalsToBeCreated = FORWARD_STRAND + COMPLEMENTARY_STRAND;
    PV_AA_Terminals4Species = aaTerminalsToBeCreated;
    bool complementaryStrand;
                                        
    ED4_terminal *terminal = 0;
    for( terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;  
         terminal = terminal->get_next_terminal())
        {
            if(terminal->is_sequence_terminal()) {
                ED4_sequence_terminal *seqTerminal = terminal->to_sequence_terminal();
                if(seqTerminal->species_name) 
                    {
                        ED4_species_manager *speciesManager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
                        if (speciesManager && !speciesManager->flag.is_consensus && !speciesManager->flag.is_SAI) 
                            {
                                char namebuffer[BUFFERSIZE];
                                for(int i = 0; i<aaTerminalsToBeCreated; i++)
                                    {
                                        int count = 1;
                                        int startPos = 0;

                                        sprintf( namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count++);
                                        ED4_multi_sequence_manager *multiSeqManager = speciesManager->search_spec_child_rek(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                                        ED4_sequence_manager         *new_SeqManager = new ED4_sequence_manager(namebuffer, 0, 0, 0, 0, multiSeqManager);
                                        new_SeqManager->set_properties(ED4_P_MOVABLE);
                                        multiSeqManager->children->append_member(new_SeqManager);

                                        ED4_sequence_info_terminal *new_SeqInfoTerminal = 0;
                                        if (i<FORWARD_STRAND) sprintf(namebuffer, "F%dProtienInfo_Term%ld.%d",i+1,ED4_counter, count++);
                                        else                  sprintf(namebuffer, "R%dProtienInfo_Term%ld.%d",(i-FORWARD_STRAND)+1,ED4_counter, count++);
                                        new_SeqInfoTerminal = new ED4_sequence_info_terminal(namebuffer, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, new_SeqManager );
                                        new_SeqInfoTerminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
                                        ED4_sequence_info_terminal *seqInfoTerminal = speciesManager->search_spec_child_rek(ED4_L_SEQUENCE_INFO)->to_sequence_info_terminal();
                                        new_SeqInfoTerminal->set_links( seqInfoTerminal, seqTerminal );
                                        new_SeqManager->children->append_member( new_SeqInfoTerminal );

                                        ED4_AA_sequence_terminal *AA_SeqTerminal = 0;
                                        sprintf(namebuffer, "AA_Sequence_Term%ld.%d",ED4_counter, count++);
                                        AA_SeqTerminal = new ED4_AA_sequence_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, new_SeqManager);
                                        //                new_SeqTERMINAL->set_properties( ED4_P_CURSOR_ALLOWED );
                                        AA_SeqTerminal->set_links( seqTerminal, seqTerminal );
                                        char       *speciesName    = seqTerminal->species_name; 
                                        if (i<FORWARD_STRAND){
                                            startPos = i;
                                            complementaryStrand = false;
                                        }
                                        else {
                                            startPos = i-FORWARD_STRAND;
                                            complementaryStrand = true;
                                        }
                                        AA_SeqTerminal->aaSequence = TranslateGeneToAminoAcidSequence(root, speciesName, startPos, complementaryStrand);
                                        AA_SeqTerminal->set_species_pointer(AA_SeqTerminal->aaSequence);
                                        AA_SeqTerminal->SET_aaSeqFlag(i+1);
                                        new_SeqManager->children->append_member(AA_SeqTerminal);

                                        ED4_counter++;

                                        new_SeqManager->resize_requested_by_child();
                                    }
                            }
                    }
            }
        }
    ED4_ROOT->main_manager->update_info.set_resize(1);
    ED4_ROOT->main_manager->resize_requested_by_parent();

    gTerminalsCreated = true;
    //    ED4_ROOT->refresh_all_windows(0);
}

AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root) {
    GB_transaction dummy(gb_main);
    gRoot = aw_root;

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
    }

    // binding callback function to the AWARS
    PV_AddCallBacks(aw_root);  

    // Create All Terminals 
    PV_CreateAllTerminals(aw_root);

    aws->show();

    return (AW_window *)aws;
}

#undef BUFFERSIZE


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

