#include "Globals.hxx"
#include "Camera.hxx"
#include "Structure3D.hxx"
#include "OPENGL/OpenGLGraphics.hxx"
#include "OPENGL/TextureMapping.hxx"
#include "OPENGL/Renderer.hxx"


// Global Variables
bool rotateStruct    = false;
bool rotateClockwise = false;
bool enableZoom   = false;

int iStructurize      = 0;
int dispBackBone      = 1;
int dispNonHelixBases = 0;
int dispBases         = 0;
int dispHelices       = 0;
int dispPositions     = 0;
int dispHelixNrs      = 0;
int dispHelixBackbone = 0;
int showCursor        = 0;

static int MODE = 0;

#define ROTATION_SPEED  0.3
float rot_x = 0.0, rot_y = 0.0;

float Angle = 0.0;    // Angle for rotation of structure
float scale = 1.0f;   // scale is used to pass the value to gScalef() for scaling purpose

double IdMtx[16] = { 1.0, 0.0, 0.0, 0.0,  
                     0.0, 1.0, 0.0, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0 };// Identity matrix 

// Definitions

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
#define TITLE  "3D Structure of rRNA !"



