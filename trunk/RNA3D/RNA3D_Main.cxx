#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_Main.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;

extern GBDATA *gb_main;

static void  CreateRNA3DAwars(AW_root *root){
    // Display Base Section
    root->awar_int(AWAR_3D_DISPLAY_BASES, 0, AW_ROOT_DEFAULT); 
    root->awar_float(AWAR_3D_DISPLAY_SIZE, 10, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_BASES_MODE, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_BASES_HELIX, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_BASES_UNPAIRED_HELIX, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_BASES_NON_HELIX, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_SHAPES_HELIX, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_SHAPES_UNPAIRED_HELIX, 3, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_SHAPES_NON_HELIX, 1, AW_ROOT_DEFAULT); 

    //Display Helix Section
    root->awar_int(AWAR_3D_DISPLAY_HELIX, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_HELIX_BACKBONE, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_HELIX_MIDPOINT, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_HELIX_FROM, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_HELIX_TO, 50, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_HELIX_NUMBER, 0, AW_ROOT_DEFAULT); 
    root->awar_float(AWAR_3D_HELIX_SIZE, 0.5, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_DISPLAY_TERTIARY_INTRACTIONS, 0, AW_ROOT_DEFAULT); 

    // Genral Molecule Display Section
    root->awar_int(AWAR_3D_MOL_BACKBONE, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MOL_COLORIZE, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SPECIES, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SPECIES_DISP_BASE, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SPECIES_DISP_POS, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SPECIES_DISP_DELETIONS, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SPECIES_DISP_MISSING, 1, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_CURSOR_POSITION, 0, AW_ROOT_DEFAULT); 
    root->awar_float(AWAR_3D_MOL_SIZE, 0.5, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MOL_DISP_POS, 0, AW_ROOT_DEFAULT);
    root->awar_int(AWAR_3D_MOL_ROTATE, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MOL_POS_INTERVAL, 25, AW_ROOT_DEFAULT); 
    root->awar_string(AWAR_3D_SELECTED_SPECIES, "", AW_ROOT_DEFAULT); 

    //Display SAI Section
    root->awar_int(AWAR_3D_MAP_SAI, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_ENABLE, 0, AW_ROOT_DEFAULT); 
    root->awar_int(AWAR_3D_MAP_SEARCH_STRINGS, 0, AW_ROOT_DEFAULT); 
}

void RNA3D_StartApplication(AW_root *awr){

    CreateRNA3DAwars(awr);

    // Creating and Initialising Motif/OpenGL window and 
    // rendering the structure
    {
        static AW_window *aw_3D = 0;

        if (!aw_3D) { // do not open window twice
            aw_3D = CreateRNA3DMainWindow(awr);
            if (!aw_3D) {
                GB_ERROR err = GB_get_error();
                aw_message(GBS_global_string("Couldn't start Ribosomal RNA 3D Structure Tool.\nReason: %s", err));
                return ;
            }
        }
        aw_3D->show();
    }

}
