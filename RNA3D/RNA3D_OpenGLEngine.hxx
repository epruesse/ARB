#include <Xm/Xm.h>

#define CREATED     1
#define NOT_CREATED 0

static int MODE = 3;

static int iStructurize      = 1;
static int dispBackBone      = 1;
static int dispNonHelixBases = 0;
static int dispBases         = 1;
static int dispHelices       = 1;
static int dispPositions     = 1;
static int dispHelixNrs      = 0;
static int dispHelixBackbone = 1;
static int showCursor        = 0;

enum {
    HELIX_1,HELIX_2,HELIX_3,HELIX_4,HELIX_5,HELIX_6,HELIX_7,HELIX_8,HELIX_9,HELIX_10,
    HELIX_11,HELIX_12,HELIX_13,HELIX_14,HELIX_15,HELIX_16,HELIX_17,HELIX_18,HELIX_19,HELIX_20,
    HELIX_21,HELIX_22,HELIX_23,HELIX_24,HELIX_25,HELIX_26,HELIX_27,HELIX_28,HELIX_29,HELIX_30,
    HELIX_31,HELIX_32,HELIX_33,HELIX_34,HELIX_35,HELIX_36,HELIX_37,HELIX_38,HELIX_39,HELIX_40,
    HELIX_41,HELIX_42,HELIX_43,HELIX_44,HELIX_45,HELIX_46,HELIX_47,HELIX_48,HELIX_49,HELIX_50
};

void ShowVendorInformation();
void ReshapeOpenGLWindow( GLint width, GLint height );
void InitializeOpenGLEngine(GLint width, GLint height );
void DrawStructure();
void RenderOpenGLScene(Widget w);
void InitializeOpenGLWindow( Widget w );
void RotateMolecule(int x, int y);
void CalculateRotationMatrix();
