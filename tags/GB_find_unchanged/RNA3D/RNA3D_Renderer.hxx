#define CHARACTERS 0
#define SHAPES     1

#define MIN(a,b) ((a)<(b)?(a):(b))

class Texture2D;
class Structure3D;
class OpenGLGraphics;

class GLRenderer {
public:
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

    GLRenderer(void);
    virtual ~GLRenderer(void);

    void DisplayMolecule(Structure3D *cStr);
    void DisplayMoleculeName(int w, int h, Structure3D *cStr);
    void DisplayMoleculeMask(int w, int h);

    void DoHelixMapping(void);
    void DisplayHelices();
    void DisplayHelixBackBone(void);
    void DisplayHelixNumbers(void);
    void DisplayBasePositions(void);
    void DisplayMappedSpBasePositions(void);
    void DisplayMappedSpInsertions(void);
    void DisplayHelixMidPoints(Texture2D *cImages);

    void BeginTexturizer();
    void EndTexturizer();
    void TexturizeStructure(Texture2D *cImages, Structure3D *cStructure);
};
