
// DisplayLists
enum { 
    CIRCLE_LIST = 51,     
    STRUCTURE_BACKBONE,   
    STRUCTURE_BACKBONE_CLR,
    STRUCTURE_BACKBONE_POINTS,     
    STRUCTURE_BACKBONE_POINTS_CLR, 
    STRUCTURE_BACKBONE_POINTS_A,     
    STRUCTURE_BACKBONE_POINTS_G,  
    STRUCTURE_BACKBONE_POINTS_C,  
    STRUCTURE_BACKBONE_POINTS_U,
    STRUCTURE_SEARCH,
    STRUCTURE_SEARCH_POINTS,
    STRUCTURE_POS,
    STRUCTURE_POS_ANCHOR,
    STRUCTURE_2D_MASK,
    STRUCTURE_2D_MASK_HELIX_START,
    STRUCTURE_2D_MASK_HELIX_END,
    STRUCTURE_2D_MASK_HELIX_FORWARD_STRAND,
    STRUCTURE_2D_MASK_HELIX_BACKWARD_STRAND,
    STRUCTURE_2D_MASK_NON_HELIX,
    NON_HELIX_BASES,
    HELIX_NUMBERS,
    HELIX_NUMBERS_POINTS,
    LOOP_A, LOOP_G, LOOP_C, LOOP_U,
    HELIX_A, HELIX_G, HELIX_C, HELIX_U,
    UNPAIRED_HELIX_A, UNPAIRED_HELIX_G, UNPAIRED_HELIX_C, UNPAIRED_HELIX_U,
    NON_HELIX_A, NON_HELIX_G, NON_HELIX_C, NON_HELIX_U
};

struct Struct2Dplus3D {
    char base;
    char mask;
    char code;
    int pos;
    int helixNr;
    float x;
    float y;
    float z;
    struct Struct2Dplus3D *next;
};

struct Struct2Dinfo {
    char base;    // Nucleotide 
    char mask;    // Helix mask [..<<<..>>>]
    char code;    // Helix code (H)elix (N)on-helix (S)tart (E)nd 
    int  pos;     // Base Position
    int  helixNr;  // Helix Number
    struct Struct2Dinfo *next;
};

struct Struct3Dinfo {
    float x;
    float y;
    float z;
    char base;
    int pos;
    struct Struct3Dinfo *next;
};

struct HelixNrInfo {
    float x,y,z;
    int helixNr;
    struct HelixNrInfo *next;
};

struct Vector3;

class Structure3D {
public:
    float xCenter, yCenter, zCenter;
    float xStrPoint, yStrPoint, zStrPoint;
    
    Structure3D(void);
    virtual  ~Structure3D(void);

    void PrepareStructureSkeleton(void);
    void DrawStructureInfo(void);
    void StoreCoordinates(float x, float y, float z, char base, unsigned int pos);
    void ReadCoOrdinateFile(Vector3 *sCen);

    void GetSecondaryStructureInfo(void);
    void Store2Dinfo(char *info, int pos, int helixNr);
    void BuildSecondaryStructureMask(void);

    void Combine2Dand3DstructureInfo(void);
    void Store2D3Dinfo(Struct2Dinfo *s2D, Struct3Dinfo *s3D);
    void GenerateHelixDispLists(int HELIX_NR_ID, int HELIX_NR);

    void PrepareSecondaryStructureInfo(void);
};
