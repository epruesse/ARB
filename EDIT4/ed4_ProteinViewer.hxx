#ifndef ed4_ProteinViewer_hxx_included
#define ed4_ProteinViewer_hxx_included
/*=======================================================================================*/
//
//    File            : ed4_ProteinViewer.hxx                               
//    Purpose      : Global Header File For Protein Viewer Module
//    Time-stamp : Thu Sep 14 2006                                                       
//    Author        : Yadhu Kumar (yadhu@arb-home.de)
//    web site      : http://www.arb-home.de/                                              
//                                                                                      
//        Copyright Department of Microbiology (Technical University Munich)            
//                                                                                      
/*=======================================================================================*/

// Define Awars
#define AWAR_PROTVIEW                                           "protView/"
#define AWAR_PV_DISPLAY_ALL                        AWAR_PROTVIEW "display_all" 
#define AWAR_PROTVIEW_FORWARD_STRAND_1             AWAR_PROTVIEW "forward_strand_1" 
#define AWAR_PROTVIEW_FORWARD_STRAND_2             AWAR_PROTVIEW "forward_strand_2" 
#define AWAR_PROTVIEW_FORWARD_STRAND_3             AWAR_PROTVIEW "forward_strand_3" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1   AWAR_PROTVIEW "complementary_strand_1" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2   AWAR_PROTVIEW "complementary_strand_2" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3   AWAR_PROTVIEW "complementary_strand_3" 
#define AWAR_PROTVIEW_DEFINED_FIELDS                   AWAR_PROTVIEW "defined_fields" 
#define AWAR_PROTVIEW_DISPLAY_OPTIONS                AWAR_PROTVIEW "display_options" 

// Awars for different display configurations 
#define AWAR_PV_SELECTED         AWAR_PROTVIEW "selected" 
#define AWAR_PV_SELECTED_DB      AWAR_PROTVIEW "selected_db" 
#define AWAR_PV_SELECTED_FS_1    AWAR_PROTVIEW "selected_fs_1" 
#define AWAR_PV_SELECTED_FS_2    AWAR_PROTVIEW "selected_fs_2" 
#define AWAR_PV_SELECTED_FS_3    AWAR_PROTVIEW "selected_fs_3" 
#define AWAR_PV_SELECTED_CS_1    AWAR_PROTVIEW "selected_cs_1" 
#define AWAR_PV_SELECTED_CS_2    AWAR_PROTVIEW "selected_cs_2" 
#define AWAR_PV_SELECTED_CS_3    AWAR_PROTVIEW "selected_cs_3" 

#define AWAR_PV_MARKED         AWAR_PROTVIEW "marked" 
#define AWAR_PV_MARKED_DB      AWAR_PROTVIEW "marked_db" 
#define AWAR_PV_MARKED_FS_1    AWAR_PROTVIEW "marked_fs_1" 
#define AWAR_PV_MARKED_FS_2    AWAR_PROTVIEW "marked_fs_2" 
#define AWAR_PV_MARKED_FS_3    AWAR_PROTVIEW "marked_fs_3" 
#define AWAR_PV_MARKED_CS_1    AWAR_PROTVIEW "marked_cs_1" 
#define AWAR_PV_MARKED_CS_2    AWAR_PROTVIEW "marked_cs_2" 
#define AWAR_PV_MARKED_CS_3    AWAR_PROTVIEW "marked_cs_3" 

#define AWAR_PV_CURSOR         AWAR_PROTVIEW "cursor" 
#define AWAR_PV_CURSOR_DB      AWAR_PROTVIEW "cursor_db" 
#define AWAR_PV_CURSOR_FS_1    AWAR_PROTVIEW "cursor_fs_1" 
#define AWAR_PV_CURSOR_FS_2    AWAR_PROTVIEW "cursor_fs_2" 
#define AWAR_PV_CURSOR_FS_3    AWAR_PROTVIEW "cursor_fs_3" 
#define AWAR_PV_CURSOR_CS_1    AWAR_PROTVIEW "cursor_cs_1" 
#define AWAR_PV_CURSOR_CS_2    AWAR_PROTVIEW "cursor_cs_2" 
#define AWAR_PV_CURSOR_CS_3    AWAR_PROTVIEW "cursor_cs_3" 

// Create Awars For Protein Viewer
void  PV_CreateAwars(AW_root *root, AW_default aw_def);

// Create All Terminals 
void PV_CallBackFunction(AW_root *root);
 
// Create ProteinViewer window
AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root); 

// callback function to update sequence change in EDITOR
void PV_AA_SequenceUpdate_CB(GB_CB_TYPE gbtype);

// refresh all AA sequene terminals
void PV_RefreshWindow(AW_root *root);

// Creates new AA sequence terminals for new species added to the editor ** called from ED4_cursor.cxx **
void PV_AddCorrespondingAAseqTerminals(ED4_species_name_terminal *spNameTerm);

// Creates new AA sequence terminals when newly marked species is loaded
void PV_AddAAseqTerminalsToLoadedSpecies();

// flags to use in display options
enum {
    PV_AA_CODE = 0,
    PV_AA_NAME,
    PV_AA_BOX
};

#endif
