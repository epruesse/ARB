#include "KeyBoard.hxx"
#include "GlobalHeader.hxx"
#include "Camera.hxx"
#include "Structure3D.hxx"
#include "OPENGL/Renderer.hxx"

void KeyBoardFunction(unsigned char key, Structure3D *cStructure, GLRenderer *cRenderer, CCamera cCamera ) {
    switch(key) {
    case '=' : if ((cRenderer->StartHelix > 0) && (cRenderer->StartHelix < 50))  cRenderer->StartHelix++; break;
    case '-' : if ((cRenderer->StartHelix > 1) && (cRenderer->StartHelix <= 50)) cRenderer->StartHelix--; break;
    case '_' : if ((cRenderer->EndHelix > 1)   && (cRenderer->EndHelix <= 50))   cRenderer->EndHelix--;   break;
    case '+' : if ((cRenderer->EndHelix > 0)   && (cRenderer->EndHelix < 50))    cRenderer->EndHelix++;   break;
    case 27  : exit (0);                 break;
    case 'g' : glShadeModel (GL_SMOOTH); break;
    case 'f' : glShadeModel (GL_FLAT);   break;
    case 'q' : glPolygonMode(GL_FRONT_AND_BACK,GL_POINT); break;
    case 'w' : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
    case 'e' : glPolygonMode(GL_BACK,GL_FILL);  break;
    case 'T' : cRenderer->HelixWidth += 0.5; if(cRenderer->HelixWidth>10)  cRenderer->HelixWidth = 10; break;
    case 't' : cRenderer->HelixWidth -= 0.5; if(cRenderer->HelixWidth<0.5) cRenderer->HelixWidth = 0.5; break;
    case 'O' : cRenderer->ObjectSize += 0.5; break;
    case 'o' : cRenderer->ObjectSize -= 0.5; break;
    case 'r' : 
        if(!rotateStruct) rotateStruct = true;
        else              rotateStruct = false;
        break;
    case 'R' : 
        if(!rotateClockwise) rotateClockwise = true;
        else                 rotateClockwise = false;
        break;
    case 'c': 
        cCamera.PositionCamera(0,0,-2, 
                               cStructure->xCenter, 
                               cStructure->yCenter, 
                               cStructure->zCenter, 
                               0, 1, 0 ); 
        break;
    case 'x': 
        cCamera.PositionCamera(0,200,0,
                               cStructure->xCenter, 
                               cStructure->yCenter, 
                               cStructure->zCenter, 
                               0, 1, 0 ); 
        break;

    case '1' : iStructurize      = (!iStructurize)?      TRUE : FALSE; break;
    case '2' : dispBackBone      = (!dispBackBone)?      TRUE : FALSE; break;
    case '3' : dispBases         = (!dispBases)?         TRUE : FALSE; break;
    case '4' : dispHelixBackbone = (!dispHelixBackbone)? TRUE : FALSE; break;
    case '5' : dispHelices       = (!dispHelices)?       TRUE : FALSE; break;
    case '6' : dispHelixNrs      = (!dispHelixNrs)?      TRUE : FALSE; break;
    case '7' : dispNonHelixBases = (!dispNonHelixBases)? TRUE : FALSE; break;
    case '8' : dispPositions     = (!dispPositions)?     TRUE : FALSE; break;
    case '9' : showCursor        = (!showCursor)?        TRUE : FALSE; break;
    case 'H' : MODE = BASES_HELIX;          break;
    case 'N' : MODE = BASES_NON_HELIX;      break;
    case 'U' : MODE = BASES_UNPAIRED_HELIX; break;
    case 'L' : MODE = BASES_LOOP;           break;
    case 'b' : MODE = BASES;                break;
    case 'B' : MODE = BASES_COLOR;          break;
    }
}

void SpecialKey(int key, CCamera cCamera) {
    Vector3 vView = cCamera.View();
    switch (key) {
    case GLUT_KEY_LEFT:
        // We want to rotate around the Y axis so we pass in a positive Y speed
        cCamera.RotateAroundPoint(vView, kSpeed, 0, 1, 0);	
        //        rotY += 0.5;
        break;

    case GLUT_KEY_RIGHT:
		// Use a negative Y speed to rotate around the Y axis
        cCamera.RotateAroundPoint(vView, -kSpeed, 0, 1, 0);	
        //        rotY -= 0.5;
        break;

    case GLUT_KEY_UP:
        //        cCamera.RotateView(Angle, 1, 0, 0);
        cCamera.RotateAroundPoint(vView, kSpeed, 1, 0, 0);	
        //        rotX += 0.5; 
        break;

    case GLUT_KEY_DOWN:
        //        cCamera.RotateView(-Angle, 1, 0, 0);
        cCamera.RotateAroundPoint(vView, kSpeed, 0, 0, 1);	
        //        rotX -= 0.5;
        break;

    case GLUT_KEY_PAGE_UP:
        cCamera.MoveCamera(zoomFactor);
        break;

    case GLUT_KEY_PAGE_DOWN:
        cCamera.MoveCamera(-zoomFactor);
        break;
    }
    glutPostRedisplay();
}
