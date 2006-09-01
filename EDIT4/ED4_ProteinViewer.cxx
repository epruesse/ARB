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

// Global Variables
extern GBDATA *gb_main;
static bool gTerminalsCreated = false;
static int PV_AA_Terminals4Species = 0;
AW_root *gRoot = 0;
static int gMissingTransTable  = 0;
static int gMissingCodonStart = 0;
static int giLastTranlsationTable = -1;

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
GBDATA *TranslateGeneToAminoAcidSequence(AW_root *root, char *speciesName, int startPos4Translation, int translationMode);
AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root);
void PV_DisplayAminoAcidNames(AW_root *root);
void PV_PrintMissingDBentryInformation(void);
void PV_RefreshWindow(AW_root *root);

// --------------------------------------------------------------------------------
//        Binding callback function to the AWARS
// --------------------------------------------------------------------------------
void PV_AddCallBacks(AW_root *awr) {

    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->add_callback(PV_CallBackFunction);

    awr->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->add_callback(PV_CallBackFunction);
    awr->awar(AWAR_PROTVIEW_DISPLAY_AA)->add_callback(PV_DisplayAminoAcidNames);
    awr->awar(AWAR_PROTVIEW_DISPLAY_OPTIONS)->add_callback(PV_RefreshWindow);
    
    {
        GB_transaction dummy(gb_main);
        GBDATA *spContainer = GB_search(gb_main, "species_data", GB_FIND);
        // callback if sequence alignment changes
        GB_add_callback(spContainer, (GB_CB_TYPE)GB_CB_CHANGED, (GB_CB)PV_AA_SequenceUpdate_CB, 0); 
    }
}

// --------------------------------------------------------------------------------
//        Creating AWARS to be used by the PROTVIEW module
// --------------------------------------------------------------------------------
void PV_CreateAwars(AW_root *root, AW_default aw_def){

    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_1, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_2, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_FORWARD_STRAND_3, 0, aw_def); 

    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3, 0, aw_def); 

    root->awar_int(AWAR_PROTVIEW_DEFINED_FIELDS, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_DISPLAY_AA, 0, aw_def); 
    root->awar_int(AWAR_PROTVIEW_DISPLAY_OPTIONS, 0, aw_def); 
}

bool PV_LookForNewTerminals(AW_root *root){
    bool bTerminalsFound = true;

    int frwdStrand  = root->awar(AWAR_PROTVIEW_FORWARD_STRAND)->read_int();
    int frwdStrand1 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->read_int();
    int frwdStrand2 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->read_int();
    int frwdStrand3 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->read_int();
    int complStrand   = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND)->read_int();
    int complStrand1  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->read_int();
    int complStrand2  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->read_int();
    int complStrand3  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->read_int();
    int defFields     = root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();
    
    // Test whether any of the options are selected or not? 
    if((frwdStrand && (frwdStrand1 || frwdStrand2 || frwdStrand3)) ||
       (complStrand && (complStrand1 || complStrand2 || complStrand3)) ||
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

void PV_RefreshWindow(AW_root *root){
    AWUSE(root);
    ED4_ROOT->refresh_all_windows(0);
    ED4_refresh_window(0, 0, 0);
}

void PV_CallBackFunction(AW_root *root) {
    // Create New Terminals If Aminoacid Sequence Terminals Are Not Created 
    if(!gTerminalsCreated){
        // AWAR_PROTEIN_TYPE is not initilized if called from ED4_main before creating proteinViewer window
        // so, initilize it here
        root->awar_int(AWAR_PROTEIN_TYPE, AWAR_PROTEIN_TYPE_bacterial_code_index, gb_main);
        PV_CreateAllTerminals(root);
    }
    // Manage the terminals showing only those selected by the user
    if(gTerminalsCreated) {
        PV_ManageTerminals(root);
    }
    PV_RefreshWindow(root);
}

void PV_HideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal) {
    ED4_sequence_manager *seqManager = aaSeqTerminal->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->hide_children();
}

void PV_UnHideTerminal(ED4_AA_sequence_terminal *aaSeqTerminal) {
    ED4_sequence_manager *seqManager = aaSeqTerminal->get_parent(ED4_L_SEQUENCE)->to_sequence_manager();
    seqManager->make_children_visible();
}

void PV_DisplayAminoAcidNames(AW_root *root) {
    GB_transaction dummy(gb_main);

    ED4_terminal *terminal = 0;
    for( terminal = ED4_ROOT->root_group_man->get_first_terminal();
         terminal;  
         terminal = terminal->get_next_terminal())
        {
            if(terminal->is_sequence_terminal()) {
                ED4_sequence_terminal *seqTerminal = terminal->to_sequence_terminal();
                char                          *speciesName = seqTerminal->species_name; 
                int translationMode = 0;

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
                                        // we are in AA sequence terminal
                                        int aaSeqFlag = int(aaSeqTerminal->GET_aaSeqFlag()); 
                                        // retranslate the genesequence and store it to the AA_sequnce_terminal
                                        int startPos;
                                        if (i<FORWARD_STRANDS) {
                                            startPos = aaSeqFlag-1;
                                            translationMode = FORWARD_STRAND;
                                        }
                                        else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS){
                                            startPos = (aaSeqFlag-FORWARD_STRANDS)-1;
                                            translationMode = COMPLEMENTARY_STRAND;
                                        }
                                        else {
                                            startPos = 0;
                                            translationMode = DB_FIELD_STRAND;
                                        }
                                        aaSeqTerminal->aaSequence = TranslateGeneToAminoAcidSequence(gRoot, speciesName, startPos, translationMode);
                                        aaSeqTerminal->set_species_pointer(aaSeqTerminal->aaSequence);
                                    }
                                }
                            }
                    }
            }
        }
    // Print missing DB entries
    PV_PrintMissingDBentryInformation();
}

void PV_ManageTerminals(AW_root *root){
    int frwdStrand  = root->awar(AWAR_PROTVIEW_FORWARD_STRAND)->read_int();
    int frwdStrand1 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_1)->read_int();
    int frwdStrand2 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_2)->read_int();
    int frwdStrand3 = root->awar(AWAR_PROTVIEW_FORWARD_STRAND_3)->read_int();
    int complStrand   = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND)->read_int();
    int complStrand1  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1)->read_int();
    int complStrand2  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2)->read_int();
    int complStrand3  = root->awar(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3)->read_int();

    int useDBentries =  root->awar(AWAR_PROTVIEW_DEFINED_FIELDS)->read_int();

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
                                                (frwdStrand && frwdStrand1)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 2: 
                                                (frwdStrand && frwdStrand2)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 3: 
                                                (frwdStrand && frwdStrand3)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 4: 
                                                (complStrand && complStrand1)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 5: 
                                                (complStrand && complStrand2)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 6: 
                                                (complStrand && complStrand3)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            case 7: 
                                                (useDBentries)? PV_UnHideTerminal(aaSeqTerminal):PV_HideTerminal(aaSeqTerminal);
                                                    break;
                                            }
                                    }
                                }
                            }
                    }
            }
        }
}

PV_ERROR PV_ComplementarySequence(char *sequence)
{
    char T_or_U;
    GB_ERROR error  = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "complement");
    if (error) {
        cout<<error<<endl;
        return PV_FAILED;
    }

    int seqLen = strlen(sequence);
    char *complementarySeq  = GBT_complementNucSequence((const char*) sequence, seqLen, T_or_U);
    strcpy(sequence, complementarySeq);
    return PV_SUCCESS;
}

GBDATA *TranslateGeneToAminoAcidSequence(AW_root *root, char *speciesName, int startPos4Translation, int translationMode){
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

    GB_ERROR error = 0;
    PV_ERROR pvError;
    int selectedTable = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE);
    int translationTable = 0;

    switch(translationMode) 
        {
        case FORWARD_STRAND:
            translationTable  = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE);
            break;
        case COMPLEMENTARY_STRAND:
            translationTable  = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE);
            // for complementary strand - get the complementary sequence and then perform translation
            pvError =  PV_ComplementarySequence(str_SeqData);
            if (pvError == PV_FAILED) {
                aw_message("Getting complementary strand failed!!");
                exit(0);
            }
            break;
        case DB_FIELD_STRAND:
            // for use database field options - fetch codon start and translation table from the respective species data     
            GBDATA *gb_translTable = GB_find(gb_species, "transl_table", 0, down_level);
            if (gb_translTable) {
                int sp_embl_table = atoi(GB_read_char_pntr(gb_translTable));
                translationTable      = AWT_embl_transl_table_2_arb_code_nr(sp_embl_table);
            }
            else {   // use selected translation table as default (if 'transl_table' field is missing)
                gMissingTransTable++;
                translationTable  = GBT_read_int(gb_main,AWAR_PROTEIN_TYPE);
            }
            GBDATA *gb_codonStart = GB_find(gb_species, "codon_start", 0, down_level);
            if (gb_codonStart) {
                startPos4Translation = atoi(GB_read_char_pntr(gb_codonStart))-1;
                if (startPos4Translation<0 || startPos4Translation>2) {
                    error = GB_export_error("'%s' has invalid codon_start entry %i (allowed: 1..3)",
                                            speciesName, startPos4Translation+1);
                    break;
                }
            }
            else {
                gMissingCodonStart++;                
                startPos4Translation = 0;
            }
            break;
        }
    // set wanted tranlation code table and initialize
    // if the current table is different from the last table then set new table and initilize the same 
    if (translationTable != giLastTranlsationTable) {
        GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, translationTable);// set wanted protein table
        awt_pro_a_nucs_convert_init(gb_main); // (re-)initialize codon tables for current translation table
        giLastTranlsationTable = translationTable;  // store the current table 
    }
    //AWT_pro_a_nucs_convert(char *seqdata, long seqlen, int startpos, bool translate_all)
    int stops = AWT_pro_a_nucs_convert(str_SeqData, len, startPos4Translation, "false"); 

    GBDATA *gb_ProSeqData = 0;
    char buf[100];
    // Create temporary alignment data to store the translated sequence 
    switch(translationMode) {
    case FORWARD_STRAND:           sprintf(buf, "ali_temp_aa_seq_%ld_%ld",FORWARD_STRAND, startPos4Translation); break;
    case COMPLEMENTARY_STRAND: sprintf(buf, "ali_temp_aa_seq_%ld_%ld",COMPLEMENTARY_STRAND, startPos4Translation); break;
    case DB_FIELD_STRAND:            sprintf(buf, "ali_temp_aa_seq_%ld_%ld",DB_FIELD_STRAND, startPos4Translation); break;
    }
    gb_ProSeqData = GBT_add_data(gb_species,buf,"data", GB_STRING);
    if (!gb_ProSeqData) {
        cout<<"ERROR : "<<GB_get_error()<<endl;
        exit(0);
    }

    char *s = (char*)malloc(len+1);
    int i,j;
    char spChar = ' ';
    {
        int displayAminoAcid = root->awar(AWAR_PROTVIEW_DISPLAY_AA)->read_int();
        if (displayAminoAcid) 
            {
                for( i=0, j=0; i<len && j<len; ) {
                    // start from the start pos of aa sequence
                    for (;i<startPos4Translation;) s[i++] = spChar;
                    char base = str_SeqData[j++];
                    const char *AAname = 0;
                    if(base>='A' && base<='Z') {
                        AAname = AWT_get_protein_name(base);
                    }
                    else if (base=='*') {
                        AAname = "End";
                    }
                    else {
                        for(int m =0;m<3 && i<len ;m++) 
                            s[i++]=base;
                    }
                    if (AAname) {
                        for(int n =0; n<strlen(AAname) && i<len ;n++) {
                            s[i++]=AAname[n];
                        }
                    }
                }
            }
        else 
            {
                int k = startPos4Translation;
                for(i=0, j=0; i<len; i++) {
                    if((k==i) && (j<len)) {
                        s[i]=str_SeqData[j++];
                        k+=3;
                    }
                    else 
                        s[i] = spChar;
                }
            }
        s[i]='\0'; // close the string
    }

    error  = GB_write_string(gb_ProSeqData,s);

    free(s);
    free(str_SeqData);

    GBT_write_int(gb_main, AWAR_PROTEIN_TYPE, selectedTable);// restore the selected table

    if (error) {
        cout<<"error during writing translated data.....!!!"<<endl;
        exit(0);
    }

    return gb_ProSeqData;
}

void PV_PrintMissingDBentryInformation(void){
    aw_message(GBS_global_string("WARNING:  'codon start' entry not found in %d of %d species! Using 1st base as codon start...",gMissingCodonStart,  GBT_count_marked_species(gb_main)));
    aw_message(GBS_global_string("WARNING:  'translation table' entry not found in %d of %d species! Using selected translation table  as a default table...",gMissingTransTable,  GBT_count_marked_species(gb_main)));
    gMissingCodonStart = gMissingTransTable = 0;
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
                    int translationMode = 0;

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
                                    if (i<FORWARD_STRANDS) {
                                        startPos = aaSeqFlag-1;
                                        translationMode = FORWARD_STRAND;
                                    }
                                    else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS){
                                        startPos = (aaSeqFlag-FORWARD_STRANDS)-1;
                                        translationMode = COMPLEMENTARY_STRAND;
                                    }
                                    else {
                                        startPos = 0;
                                        translationMode = DB_FIELD_STRAND;
                                    }
                                    aaSeqTerminal->aaSequence = TranslateGeneToAminoAcidSequence(gRoot, speciesName, startPos, translationMode);
                                    aaSeqTerminal->set_species_pointer(aaSeqTerminal->aaSequence);
                                }
                            }
                        }
                    // Print missing DB entries
                    PV_PrintMissingDBentryInformation();
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
    int aaTerminalsToBeCreated = FORWARD_STRANDS + COMPLEMENTARY_STRANDS + DB_FIELD_STRANDS;
    PV_AA_Terminals4Species = aaTerminalsToBeCreated;
    int  translationMode = 0;
                                        
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
                                        if (i<FORWARD_STRANDS) 
                                            sprintf(namebuffer, "F%dProtienInfo_Term%ld.%d",i+1,ED4_counter, count++);
                                         else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS)  
                                             sprintf(namebuffer, "C%dProtienInfo_Term%ld.%d",(i-FORWARD_STRANDS)+1,ED4_counter, count++);
                                         else
                                             sprintf(namebuffer, "DBProtienInfo_Term%ld.%d",ED4_counter, count++);
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
                                        if (i<FORWARD_STRANDS){
                                            startPos = i;
                                            translationMode = FORWARD_STRAND;
                                        }
                                        else if ((i-FORWARD_STRANDS)<COMPLEMENTARY_STRANDS){
                                            startPos = i-FORWARD_STRANDS;
                                            translationMode = COMPLEMENTARY_STRAND;
                                        }
                                        else {
                                            startPos = 0;
                                            translationMode = DB_FIELD_STRAND;
                                        }
                                        AA_SeqTerminal->aaSequence = TranslateGeneToAminoAcidSequence(root, speciesName, startPos, translationMode);
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

    // Print missing DB entries
    PV_PrintMissingDBentryInformation();

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
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND);

        aws->at("f1");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_1);

        aws->at("f2");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_2);

        aws->at("f3");
        aws->create_toggle(AWAR_PROTVIEW_FORWARD_STRAND_3);

        aws->at("cmpl");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND);

        aws->at("c1");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1);

        aws->at("c2");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2);

        aws->at("c3");
        aws->create_toggle(AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3);

        aws->at("defined");
        aws->create_toggle(AWAR_PROTVIEW_DEFINED_FIELDS);

        aws->at("aaName");
        aws->create_toggle(AWAR_PROTVIEW_DISPLAY_AA);

        aws->at("dispOption");
        aws->create_toggle_field(AWAR_PROTVIEW_DISPLAY_OPTIONS,1);
        aws->insert_toggle("TEXT", "T", 0);
        aws->insert_toggle("BOX", "B", 1);
        aws->update_toggle_field();
    }

    // binding callback function to the AWARS
    PV_AddCallBacks(aw_root);  

    // Create All Terminals 
    PV_CreateAllTerminals(aw_root);

    aws->show();

    return (AW_window *)aws;
}

#undef BUFFERSIZE
