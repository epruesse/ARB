#include "GlobalHeader.hxx"
#include "Globals.hxx"
//=========================== Mouse Functions ====================================
void MouseMain(int btn, int state, int x, int y);

//================================================================================

void MouseMain(int btn, int state, int x, int y) 
{
    if (btn == GLUT_MIDDLE_BUTTON) {
        switch (state) {
            extern bool enableZoom;
        case GLUT_DOWN : enableZoom = true;  break;
        case GLUT_UP   : enableZoom = false; break;
        }
    }
    if (btn == GLUT_LEFT_BUTTON) {
        switch (state) {
            extern bool rotateMolecule;
            extern GLfloat saved_x;
            extern GLfloat saved_y;
        case GLUT_DOWN : 
            rotateMolecule = true;   
            saved_x = x;
            saved_y = y;
            break;
        case GLUT_UP   : 
            rotateMolecule = false; 
            break;
        }
    }     
}
