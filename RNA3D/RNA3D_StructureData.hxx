// =============================================================== //
//                                                                 //
//   File      : RNA3D_StructureData.hxx                           //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef RNA3D_STRUCTUREDATA_HXX
#define RNA3D_STRUCTUREDATA_HXX

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#define SAI    0
#define SEARCH 1

#define SSU_16S 1
#define LSU_23S 2
#define LSU_5S  3

#define _1PNU  1
#define _1VOR  2
#define _1C2W  3

// DisplayLists
enum { 
    STRUCTURE_BACKBONE = 300,   
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
    MAP_SPECIES_INSERTION_POINTS, MAP_SPECIES_INSERTION_BASES, MAP_SPECIES_INSERTION_BASES_ANCHOR,
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

struct Insertions {
    int pos;
    char base;
    struct Insertions *next;
};

struct Vector3;

class ED4_sequence_terminal; 
class BI_ecoli_ref; 

class Structure3D {
public:
    Vector3 *strCen;

    int iInterval;
    int iMapSAI;
    int iMapSearch;
    int iMapEnable;
    int iStartPos, iEndPos;
    int iEColiStartPos, iEColiEndPos;
    int iTotalSubs, iTotalDels, iTotalIns;
    int LSU_molID;
    int HelixBase; // to create display lists storing helix information

    static GBDATA *gb_main;
    BI_ecoli_ref *EColiRef;
    ED4_sequence_terminal *ED4_SeqTerminal;

    OpenGLGraphics *GRAPHICS; // not really a good place - it better should be passed from callers

    Structure3D(void);
    virtual  ~Structure3D(void);

    void ReadCoOrdinateFile();
    void StoreCoordinates(float x, float y, float z, char base, unsigned int pos);

    void GetSecondaryStructureInfo(void);
    void Store2Dinfo(char *info, int pos, int helixNr);
    void PrepareSecondaryStructureData(void);

    void Combine2Dand3DstructureInfo(void);
    void Store2D3Dinfo(Struct2Dinfo *s2D, Struct3Dinfo *s3D);

    static int FindTypeOfRNA();
    void DeleteOldMoleculeData();

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
    void StoreInsertions(char base, int pos);
    void DeleteOldInsertionData();
    void GenerateInsertionDisplayList();

    void GenerateCursorPositionDispList(long pos);

    void MapSaiToEcoliTemplate(AW_root *awr);
    void MapSearchStringsToEcoliTemplate(AW_root *awr);
};

#else
#error RNA3D_StructureData.hxx included twice
#endif
