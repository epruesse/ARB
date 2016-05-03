#define CHARACTERS 0
#define SHAPES     1

// the following macro is already defined in gmacros.h:
// #define MIN(a, b) ((a)<(b) ? (a) : (b))

class Texture2D;
class Structure3D;
class OpenGLGraphics;

struct GLRenderer : virtual Noncopyable {
    float ObjectSize;
    int iDisplayBases, iBaseMode;
    int iBaseHelix,  iBaseUnpairHelix,  iBaseNonHelix;
    int iShapeHelix, iShapeUnpairHelix, iShapeNonHelix;
    int iDisplayHelix, iHelixMidPoint, iHelixBackBone, iHelixNrs;
    int iDispTerInt;
    int iStartHelix, iEndHelix;
    float fHelixSize;
    float fSkeletonSize;
    int iColorise, iBackBone;
    int iDispPos;
    int iMapSpecies, iMapSpeciesBase, iMapSpeciesPos;
    int iMapSpeciesDels, iMapSpeciesMiss, iMapSpeciesIns, iMapSpeciesInsInfo;
    int iDispCursorPos;

    OpenGLGraphics *G;

    GLRenderer();
    virtual ~GLRenderer();

    void DisplayMolecule(Structure3D *cStr);
    void DisplayMoleculeName(int w, int h, Structure3D *cStr);
    void DisplayMoleculeMask(int w, int h);

    void DoHelixMapping();
    void DisplayHelices();
    void DisplayHelixBackBone();
    void DisplayHelixNumbers();
    void DisplayBasePositions();
    void DisplayMappedSpBasePositions();
    void DisplayMappedSpInsertions();
    void DisplayHelixMidPoints(Texture2D *cImages);

    void BeginTexturizer();
    void EndTexturizer();
    void TexturizeStructure(Texture2D *cImages, Structure3D *cStructure);
};
