#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Main.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;


void RNA3D_StartApplication(AW_root *awr){

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
