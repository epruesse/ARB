#define CHARACTERS 0
#define SHAPES     1

enum {
    BASES,
    BASES_COLOR,
    BASES_LOOP,
    BASES_HELIX,
    BASES_NON_HELIX,
    BASES_UNPAIRED_HELIX,
    HELIX_MASK,
    HELIX_MAPPING
};

const float DEG2RAD = 3.14159/180;   // Used in circle drawing

// Colors
const GLfloat BLACK[4]   = { 0.0, 0.0, 0.0, 1.0 };
const GLfloat RED[4]     = { 1.0, 0.0, 0.0, 1.0 };
const GLfloat GREEN[4]   = { 0.0, 1.0, 0.0, 1.0 };
const GLfloat BLUE[4]    = { 0.0, 0.0, 1.0, 1.0 };
const GLfloat CYAN[4]    = { 0.0, 1.0, 1.0, 1.0 };
const GLfloat YELLOW[4]  = { 1.0, 1.0, 0.0, 1.0 };
const GLfloat MAGENTA[4] = { 1.0, 0.0, 1.0, 1.0 };
const GLfloat WHITE[4]   = { 1.0, 1.0, 1.0, 1.0 };

const GLfloat DEFAULT[4] = { 0.5, 0.5, 0.5, 1.0 }; // grey

class Texture2D;

class GLRenderer {
public:
    float ObjectSize;
    int iDisplayBases, iBaseMode;
    int iBaseHelix,  iBaseUnpairHelix,  iBaseNonHelix;
    int iShapeHelix, iShapeUnpairHelix, iShapeNonHelix;
    int iDisplayHelix, iHelixMidPoint, iHelixBackBone, iHelixNrs;
    int iStartHelix, iEndHelix;
    float fHelixSize;
    float fSkeletonSize;
    int iColorise, iBackBone;

    GLRenderer(void);
    virtual ~GLRenderer(void);

    void DisplayMolecule(void);
    void DoHelixMapping(void);
    void DisplayHelices();
    void DisplayHelixBackBone(void);
    void DisplayHelixNumbers(void);
    void DisplayPositions(void);
    void DisplayHelixMidPoints(Texture2D *cImages);

    void BeginTexturizer();
    void EndTexturizer();
    void TexturizeStructure(Texture2D *cImages);
    void DrawTexturizedStructure(Texture2D *cImages, int Structure );
    void SetColor(int gc);
};
