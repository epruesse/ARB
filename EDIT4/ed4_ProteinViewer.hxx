#ifndef ed4_ProteinViewer_hxx_included
#define ed4_ProteinViewer_hxx_included

// Define Awars
#define AWAR_PROTVIEW                            "protView/"
#define AWAR_PROTVIEW_FORWARD_TRANSLATION        AWAR_PROTVIEW "forward_translation" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_1      AWAR_PROTVIEW "forward_translation_1" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_2      AWAR_PROTVIEW "forward_translation_2" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_3      AWAR_PROTVIEW "forward_translation_3" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION        AWAR_PROTVIEW "reverse_translation" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_1      AWAR_PROTVIEW "reverse_translation_1" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_2      AWAR_PROTVIEW "reverse_translation_2" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_3      AWAR_PROTVIEW "reverse_translation_3" 
#define AWAR_PROTVIEW_DEFINED_FIELDS             AWAR_PROTVIEW "defined_fields" 

// Create Awars For Protein Viewer
void  PV_CreateAwars(AW_root *root, AW_default aw_def);

// Create All Terminals 
void PV_CreateAllTerminals(AW_root *root);
 
// Create ProteinViewer window
AW_window *ED4_CreateProteinViewer_window(AW_root *aw_root); 

#endif
