#include "GlobalHeader.hxx"
#include "Menu.hxx"
//=========================== Menu Functions ========================================

void MakeMenu() {
    int Menu = glutCreateMenu(MenuFunction);
    glutSetMenu(Menu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutAddMenuEntry("Display STRUCTURE",1);
    glutAddMenuEntry("Display BACKBONE",2);
    glutAddMenuEntry("Display NON_HELIX_BASES",3);
    glutAddMenuEntry("Display BASES",4);
    glutAddMenuEntry("Display POSITIONS",5);
    glutAddMenuEntry("Display HELIX BACKBONE",6);
    glutAddMenuEntry("Display HELICES",7);
    glutAddMenuEntry("Display HELIX NUMBERS",8);
}

void MenuFunction(int value) {
    //    switch(value) {
//     case 1 : 
//         if(!iStructurize) iStructurize = TRUE;
//         else              iStructurize = FALSE;
//         break;
//     case 2 : 
//         if (!dispBackBone) dispBackBone = TRUE;
//         else               dispBackBone = FALSE;
//         break;
//     case 3:
//         if(!dispNonHelixBases) dispNonHelixBases = TRUE;
//         else                   dispNonHelixBases = FALSE;
//         break;
//     case 4 : 
//         if(!dispBases) dispBases = TRUE;
//         else           dispBases = FALSE;
//         break;
//     case 5 : 
//         if(!dispPositions) dispPositions = TRUE; 
//         else               dispPositions = FALSE; 
//         break;
//     case 6:
//         if(!dispHelixBackbone) dispHelixBackbone = TRUE;
//         else                   dispHelixBackbone = FALSE;
//         break;
//     case 7:
//         if(!dispHelices) dispHelices = TRUE;
//         else             dispHelices = FALSE;
//         break;
//     case 8 : 
//         if(!dispHelixNrs) dispHelixNrs = TRUE; 
//         else              dispHelixNrs = FALSE; 
//        break;
//    }
    glutPostRedisplay();
}

