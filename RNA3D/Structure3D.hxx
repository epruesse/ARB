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
