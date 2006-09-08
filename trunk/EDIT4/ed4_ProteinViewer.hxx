#ifndef ed4_ProteinViewer_hxx_included
#define ed4_ProteinViewer_hxx_included

// Define Awars
#define AWAR_PROTVIEW                                           "protView/"
#define AWAR_PROTVIEW_FORWARD_STRAND                AWAR_PROTVIEW "forward_strand" 
#define AWAR_PROTVIEW_FORWARD_STRAND_1             AWAR_PROTVIEW "forward_strand_1" 
#define AWAR_PROTVIEW_FORWARD_STRAND_2             AWAR_PROTVIEW "forward_strand_2" 
#define AWAR_PROTVIEW_FORWARD_STRAND_3             AWAR_PROTVIEW "forward_strand_3" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND      AWAR_PROTVIEW "complementary_strand" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_1   AWAR_PROTVIEW "complementary_strand_1" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_2   AWAR_PROTVIEW "complementary_strand_2" 
#define AWAR_PROTVIEW_COMPLEMENTARY_STRAND_3   AWAR_PROTVIEW "complementary_strand_3" 
#define AWAR_PROTVIEW_DEFINED_FIELDS                   AWAR_PROTVIEW "defined_fields" 
#define AWAR_PROTVIEW_DISPLAY_AA                         AWAR_PROTVIEW "display_aa"
#define AWAR_PROTVIEW_DISPLAY_OPTIONS                AWAR_PROTVIEW "display_options" 
#define AWAR_PROTVIEW_DISPLAY_MODE                    AWAR_PROTVIEW "display_mode" 
 
// Create Awars For Protein Viewer
void  PV_CreateAwars(AW_root *root, AW_default aw_def);

// Create All Terminals 
void PV_CallBackFunction(AW_root *root);
 
// Create ProteinViewer window
AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root); 

#endif
