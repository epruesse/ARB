#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define rna3d_assert(cond) arb_assert(cond)

// Awars for SAI
#define AWAR_3D_SAI_SELECTED "rna3d/sai_selected"

// Awars for Helix 
#define AWAR_3D_DISPLAY_HELIX   "rna3d/display_helix"
#define AWAR_3D_HELIX_BACKBONE  "rna3d/helix_backbone"
#define AWAR_3D_HELIX_MIDPOINT  "rna3d/helix_midpoint"
#define AWAR_3D_HELIX_SIZE      "rna3d/helix_size"
#define AWAR_3D_HELIX_FROM      "rna3d/helix_from"
#define AWAR_3D_HELIX_TO        "rna3d/helix_to"
#define AWAR_3D_HELIX_NUMBER    "rna3d/helix_number"

#define AWAR_3D_DISPLAY_TERTIARY_INTRACTIONS    "rna3d/tertiary_interactions"

// Awars for Bases
#define AWAR_3D_DISPLAY_BASES         "rna3d/display_bases"
#define AWAR_3D_DISPLAY_SIZE          "rna3d/display_size"
#define AWAR_3D_BASES_MODE            "rna3d/bases_mode"
#define AWAR_3D_BASES_HELIX           "rna3d/bases_helix"
#define AWAR_3D_BASES_UNPAIRED_HELIX  "rna3d/bases_unpaired_helix"
#define AWAR_3D_BASES_NON_HELIX       "rna3d/bases_non_helix"
#define AWAR_3D_SHAPES_HELIX          "rna3d/shapes_helix"
#define AWAR_3D_SHAPES_UNPAIRED_HELIX "rna3d/shapes_unpaired_helix"
#define AWAR_3D_SHAPES_NON_HELIX      "rna3d/shapes_non_helix"

// Awars for General Molecule
#define AWAR_3D_MOL_BACKBONE                 "rna3d/mol_backbone"
#define AWAR_3D_MOL_COLORIZE                 "rna3d/mol_colorize"
#define AWAR_3D_MOL_SIZE                     "rna3d/mol_size"
#define AWAR_3D_MOL_DISP_POS                 "rna3d/mol_disp_pos"
#define AWAR_3D_MOL_POS_INTERVAL             "rna3d/mol_pos_interval"
#define AWAR_3D_MOL_ROTATE                   "rna3d/mol_rotate"

#define AWAR_3D_MAP_SPECIES                      "rna3d/mol_map_species"
#define AWAR_3D_MAP_SPECIES_DISP_BASE            "rna3d/mol_map_species_base"
#define AWAR_3D_MAP_SPECIES_DISP_POS             "rna3d/mol_map_species_pos"
#define AWAR_3D_MAP_SPECIES_DISP_DELETIONS       "rna3d/mol_map_species_deletions"
#define AWAR_3D_MAP_SPECIES_DISP_MISSING         "rna3d/mol_map_species_missing"
#define AWAR_3D_MAP_SPECIES_DISP_INSERTIONS      "rna3d/mol_map_species_insertions"
#define AWAR_3D_MAP_SPECIES_DISP_INSERTIONS_INFO "rna3d/mol_map_species_insertions_info"

#define AWAR_3D_SELECTED_SPECIES             "rna3d/selected_species"
#define AWAR_3D_CURSOR_POSITION              "rna3d/cursor_postion"

#define AWAR_3D_MAP_ENABLE          "rna3d/map_enable"
#define AWAR_3D_MAP_SAI             "rna3d/map_sai"
#define AWAR_3D_MAP_SEARCH_STRINGS  "rna3d/map_search_strings"

#define AWAR_3D_23S_RRNA_MOL  "rna3d/rrna_mol_23s"

typedef struct Vector3 {
public:
    float x, y, z;						

	Vector3() {}  	// A default constructor
	Vector3(float X, float Y, float Z) { x = X; y = Y; z = Z; }

	// Overloading Operator(+,-,*,/) functions

    // adding 2 vectors    
	Vector3 operator+(Vector3 v) { return Vector3(v.x + x, v.y + y, v.z + z); }

    // substracting 2 vectors
	Vector3 operator-(Vector3 v) { return Vector3(x - v.x, y - v.y, z - v.z); }

	//multiply by scalars
	Vector3 operator*(float num) { return Vector3(x * num, y * num, z * num); }

    // divide by scalars
	Vector3 operator/(float num) { return Vector3(x / num, y / num, z / num); }
};


// Class to store all the GLOBAL variables that are used in the application

class OpenGLGraphics;
class Structure3D;
class Texture2D;
class GLRenderer;

class RNA3D_Global {
public:
    Widget      glw;
    AW_root    *root;
    AWT_canvas *gl_Canvas;

    int OpenGLEngineState;
    int iRotateMolecule;

    bool bPointSpritesSupported;
    bool bAutoRotate;
    bool bRotateMolecule;
    bool bDisplayMask;
    bool bDisplayComments;

    bool bEColiRefInitialised;
    bool bMapSearchStringsDispListCreated;
    bool bMapSaiDispListCreated;

    float ROTATION_SPEED;
    float saved_x, saved_y;
    float scale;

    OpenGLGraphics *cGraphics;
    Structure3D    *cStructure;
    Texture2D      *cTexture;
    GLRenderer     *cRenderer;

    Vector3 Viewer;
    Vector3 Center;
    Vector3 Up;

    RNA3D_Global();
    ~RNA3D_Global();
};

// global variables

extern RNA3D_Global *RNA3D;
extern GBDATA       *gb_main;

extern char globalComment[1000];

void RNA3D_init_global_data();

