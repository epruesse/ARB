#define SAI    0
#define SEARCH 1

// DisplayLists
enum { 
    CIRCLE_LIST = 51,     
    STRUCTURE_BACKBONE,   
    STRUCTURE_BACKBONE_CLR,   
    HELIX_NUMBERS,
    HELIX_NUMBERS_POINTS,
    HELIX_A, HELIX_G, HELIX_C, HELIX_U,
    UNPAIRED_HELIX_A, UNPAIRED_HELIX_G, UNPAIRED_HELIX_C, UNPAIRED_HELIX_U,
    NON_HELIX_A, NON_HELIX_G, NON_HELIX_C, NON_HELIX_U,
    STRUCTURE_POS,
    STRUCTURE_POS_ANCHOR,
    MAP_SPECIES_BASE_DIFFERENCE, 
    MAP_SPECIES_BASE_DIFFERENCE_POS, MAP_SPECIES_BASE_DIFFERENCE_POS_ANCHOR,
    MAP_SPECIES_BASE_A, MAP_SPECIES_BASE_G, MAP_SPECIES_BASE_C, MAP_SPECIES_BASE_U,
    MAP_SPECIES_DELETION, MAP_SPECIES_MISSING,
    MAP_SAI_TO_STRUCTURE, 
    MAP_SEARCH_STRINGS_TO_STRUCTURE,MAP_SEARCH_STRINGS_BACKBONE,
    ECOLI_CURSOR_POSITION, 
    ECOLI_TERTIARY_INTRACTION_PSEUDOKNOTS,
    ECOLI_TERTIARY_INTRACTION_TRIPLE_BASES
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

struct CurrSpecies {
    float x;
    float y;
    float z;
    int pos;
    char base;
    struct CurrSpecies *next;
};

struct Vector3;

class ED4_sequence_terminal; 

class Structure3D {
public:
    Vector3 *strCen;

    int iInterval;
    int iMapSAI;
    int iMapSearch;
    int iMapEnable;
    int iStartPos, iEndPos;
    int iEColiStartPos, iEColiEndPos;
    
    BI_ecoli_ref *EColiRef;
    ED4_sequence_terminal *ED4_SeqTerminal;
    
    Structure3D(void);
    virtual  ~Structure3D(void);

    void ReadCoOrdinateFile();
    void StoreCoordinates(float x, float y, float z, char base, unsigned int pos);

    void GetSecondaryStructureInfo(void);
    void Store2Dinfo(char *info, int pos, int helixNr);
    void PrepareSecondaryStructureData(void);

    void Combine2Dand3DstructureInfo(void);
    void Store2D3Dinfo(Struct2Dinfo *s2D, Struct3Dinfo *s3D);

    void GenerateMoleculeSkeleton(void);
    void ComputeBasePositions();

    void PositionsToCoordinatesDispList(int listID, int *pos, int len);
    void PointsToQuads(float x, float y, float z);
    void StoreHelixNrInfo(float x, float y, float z, int helixNr);
    
    void GenerateDisplayLists(void);
    void GenerateHelixDispLists(int HELIX_NR_ID, int HELIX_NR);
    void GenerateHelixNrDispList(int startHx, int endHx);
    void GenerateSecStructureHelixRegions(void);
    void GenerateSecStructureNonHelixRegions(void);
    void GenerateSecStructureUnpairedHelixRegions(void);

    void GenerateTertiaryInteractionsDispLists(void);

    void MapCurrentSpeciesToEcoliTemplate(AW_root *awr);
    void StoreCurrSpeciesDifference(char base, int pos);
    void DeleteOldSpeciesData();
    void BuildDisplayList(int listID, int *pos, int len);
    void GenerateBaseDifferenceDisplayList();
    void GenerateBaseDifferencePositionDisplayList();

    void GenerateCursorPositionDispList(long pos);

    void MapSaiToEcoliTemplate(AW_root *awr);
    void MapSearchStringsToEcoliTemplate(AW_root *awr);
    int  ValidateSearchColor(int iColor, int mode);
};
