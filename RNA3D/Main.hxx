#include "Globals.hxx"
#include "Camera.hxx"
#include "Structure3D.hxx"
#include "OPENGL/OpenGLGraphics.hxx"
#include "OPENGL/TextureMapping.hxx"
#include "OPENGL/Renderer.hxx"

// Global Class declarations 
OpenGLGraphics *cGraphics  = new OpenGLGraphics();
Structure3D    *cStructure = new Structure3D();
Texture2D      *cTexture   = new Texture2D();
GLRenderer     *cRenderer  = new GLRenderer();
CCamera         cCamera;

// Global Variables
bool rotateMolecule = true;
bool enableZoom     = false;

int iStructurize      = 0;
int dispBackBone      = 1;
int dispNonHelixBases = 0;
int dispBases         = 0;
int dispHelices       = 0;
int dispPositions     = 0;
int dispHelixNrs      = 0;
int dispHelixBackbone = 0;
int showCursor        = 0;

Vector3 Viewer = Vector3(0.0, 0.0, -2);
Vector3 Center = Vector3(0.0, 0.0, 0.0);
Vector3 Up     = Vector3(0.0, 1.0, 0.0);

static Vector3 sCen;
static bool autoRotate = false;
static int MODE = 0;
static float scale = 0.01f;   // scale is used to pass the value to gScalef() for scaling purpose
static float ZOOM  = 0.0005;

/* 
 * Window properties 
 */
#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   600
#define WINDOW_X        200
#define WINDOW_Y        200
#define TITLE    "3D Structure of Ribosomal RNA"

/* 
 * Perspective properties 
 */
#define FOV_ANGLE  90
#define CLIP_NEAR  0.5f
#define CLIP_FAR   10000

static float ROTATION_SPEED = 0.5;
static GLfloat rot_x = 0.0, rot_y = 0.0;
GLfloat saved_x, saved_y;

static GLfloat rotation_matrix[16];
