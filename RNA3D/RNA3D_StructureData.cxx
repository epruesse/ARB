#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_StructureData.hxx"

#include <cerrno>
#include <string>
#include <iostream>
#include <fstream>

#include <arbdbt.h>
#include <aw_awars.hxx>
#include <ed4_extern.hxx>
#include <BI_helix.hxx>


#define COLORLINK (ED4_G_SBACK_0 - RNA3D_GC_SBACK_0)  // to link to the colors defined in primary editor ed4_defs.hxx
#define SAICOLORS (ED4_G_CBACK_0 - RNA3D_GC_CBACK_0)

using namespace std;

static Struct3Dinfo   *start3D                 = NULL;
static Struct2Dinfo   *start2D                 = NULL;
static Struct2Dplus3D *start2D3D               = NULL;
static HelixNrInfo    *start                   = NULL;
static CurrSpecies    *startSp                 = NULL;
static bool            bOldSpecesDataExists    = false;
static Insertions     *startIns                = NULL;
static bool            bOldInsertionDataExists = false;
static bool            bStartPosStored         = false;
static bool            bEndPosStored           = false;

static char *find_data_file(const char *name) {
    char *fname = GBS_find_lib_file(name, "rna3d/", 0);
    if (!fname) throw string("file not found: ")+name;
    return fname;
}

static void throw_IO_error(const char *filename) __attribute__((noreturn));
static void throw_IO_error(const char *filename) {
    string error = string("IO-Error: ")+strerror(errno)+" ('"+filename+"')";
    throw error;
}

GBDATA *Structure3D::gb_main = 0;

Structure3D::Structure3D(void) {
    strCen          = new Vector3(0.0, 0.0, 0.0);
    GRAPHICS        = new OpenGLGraphics();
    ED4_SeqTerminal = 0;
    iInterval       = 25;
    iMapSAI         = 0;
    iMapSearch      = 0;
    iMapEnable      = 0;
    iEColiStartPos  = 0;
    iEColiEndPos    = 0;
    iStartPos       = 0;
    iEndPos         = 0;
    iTotalSubs      = 0;
    iTotalDels      = 0;
    iTotalIns       = 0;
    LSU_molID       = 3;  // default molecule to load in case of 23S rRNA : 1C2W
    HelixBase       = 500;
}

Structure3D::~Structure3D(void) {
    delete GRAPHICS;
    delete strCen;
}


void Structure3D::StoreCoordinates(float x, float y, float z, char base, unsigned int pos){
    Struct3Dinfo *data, *temp;
    data = new Struct3Dinfo;
    data->x    = x;
    data->y    = y;
    data->z    = z;
    data->base = base; 
    data->pos  = pos;
    data->next = NULL;

    if (start3D == NULL)
        start3D = data;
    else {
        temp = start3D;
        // We know this is not NULL - list not empty!
        while (temp->next != NULL) {
            temp = temp->next;  // Move to next link in chain
        }
        temp->next = data;
    }
}

//------------------Selecting rRNA type--------------------------//

int Structure3D::FindTypeOfRNA(){
    int rnaType = 0;       
    GB_push_transaction(gb_main);  //opening a transaction 

    GBDATA *gbTemplate = GBT_find_SAI(gb_main,"ECOLI");
    if (!gbTemplate) {
        aw_message("SAI:ECOLI not found");
    }
    else {
        char   *ali_name          = GBT_get_default_alignment(gb_main);
        GBDATA *gbAlignment       = GB_find(gbTemplate, ali_name, 0, down_level);
        GBDATA *gbTemplateSeqData = gbAlignment ? GB_find(gbAlignment, "data", 0, down_level) : 0;
                 
        if (!gbTemplateSeqData) {
          aw_message("Could not find species in the database!");
        }
        else {
            const char *pTemplateSeqData  = GB_read_char_pntr(gbTemplateSeqData);
            int iSeqLen = strlen(pTemplateSeqData); 
            int iBaseCount = 0;  
            for(int i = 0; i<iSeqLen; i++) {
                if((pTemplateSeqData[i] != '.') && (pTemplateSeqData[i] != '-')){
                    iBaseCount++;
                }
            }

            if (iBaseCount > 2000)      rnaType = LSU_23S;
            else if (iBaseCount > 300)  rnaType = SSU_16S;
            else                        rnaType = LSU_5S;
        }
    }
    GB_pop_transaction(gb_main);
    
    return rnaType;
}

//----------Delete old molecule data-------------------------------// 
// Struct2Dinfo, Struct2Dplus3D, Struct3Dinfo, HelixNrInfo, Insertions

void Structure3D::DeleteOldMoleculeData(){
    // Struct2Dinfo -> start2D
    {   
        Struct2Dinfo *tmp, *data;
        for(data = start2D; data != NULL; data = tmp) {
            tmp = data->next;
            delete data;
        }
        start2D = NULL; 
    }

    // Struct2Dplus3D ->start2D3D
    {
        Struct2Dplus3D *tmp, *data;
        for(data = start2D3D; data != NULL; data = tmp) {
            tmp = data->next;
            delete data;
        }
        start2D3D = NULL; 
    }

    // Struct3Dinfo ->start3D
    {
        Struct3Dinfo *tmp, *data;
        for(data = start3D; data != NULL; data = tmp) {
            tmp = data->next;
            delete data;
        }
        start3D = NULL; 
    }

    // HelixNrInfo -> start
    {
        HelixNrInfo *tmp, *data;
        for(data = start; data != NULL; data = tmp) {
            tmp = data->next;
            delete data;
        }
        start = NULL; 
    }

    // Delete insertions data
    DeleteOldInsertionData();

    //Delelte mapped species data
    DeleteOldSpeciesData ();
}

//=========== Reading 3D Coordinates from PDB file ====================//

void Structure3D::ReadCoOrdinateFile() {
    static char *DataFile = 0;  
    int rnaType    = FindTypeOfRNA();
    
    RNA3D->bDisplayComments = true; // displaying comments in main window

    switch (rnaType) {
    case LSU_23S:
        switch(LSU_molID){
        case _1PNU: 
            DataFile = find_data_file("Ecoli_1PNU_23S_rRNA.pdb");
            sprintf(globalComment, "The 3D molecule rendered from PDB entry : 1PNU at 8.7 Angstrom."); 
            break;
        case _1VOR: 
            DataFile = find_data_file("Ecoli_1VOR_23S_rRNA.pdb");
            sprintf(globalComment,"The 3D molecule is rendered from PDB entry [1VOR] with 11.5 Angstrom resolution."); 
            break;
        case _1C2W: 
            DataFile = find_data_file("Ecoli_1C2W_23S_rRNA.pdb");
            sprintf(globalComment,"The 3D molecule is rendered from PDB entry [1C2W] with 7.5 Angstrom resolution."); 
            break;
        }
        break;
    case LSU_5S: 
        DataFile = find_data_file("Ecoli_1C2X_5S_rRNA.pdb");
        sprintf(globalComment,"The 3D molecule is rendered from PDB entry [1C2X] with 7.5 Angstrom resolution."); 
        break;
    case SSU_16S:
        DataFile = find_data_file("Ecoli_1M5G_16S_rRNA.pdb");
        sprintf(globalComment, "The 3D molecule is rendered from PDB entry [1M5G] with 11.5 Angstrom resolution."); 
        break;
    }
    
    char      buf[256];

    float X, Y, Z;
    unsigned int pos;
    char Base;
 
    ifstream readData;
    readData.open(DataFile, ios::in);
    if (!readData.is_open()) {
        throw_IO_error(DataFile);
    }

    int cntr = 0;
    unsigned int last3Dpos = 0; 

    static bool bEColiStartPosStored = false;

    while (!readData.eof()) {
        readData.getline(buf,100);  
        string tmp, atom, line = string(buf);

        if ((line.find("ATOM") != string::npos ) || (line.find("HETATM") != string::npos )) {
            atom = (line.substr(77,2)).c_str();
            if (atom.find("P") != string::npos) {
                tmp = (line.substr(18,3)).c_str();
                Base = tmp[1];
                pos  = atoi((line.substr(22,4)).c_str());
                X    = atof((line.substr(31,8)).c_str());
                Y    = atof((line.substr(39,8)).c_str());
                Z    = atof((line.substr(47,8)).c_str());
       
                // special filter for 23S rRNA structure (IVOR/IPNU)
                // IVOR/IPNU contains artifacts and are not mentioned in any of the 
                // remarks of PDB file 
                if (last3Dpos != pos && !(pos >= 3093)) {       
                    StoreCoordinates(X,Y,Z,Base,pos);
                    last3Dpos = pos;
                    strCen->x += X; strCen->y += Y; strCen->z += Z;
                    cntr++;
                }
                if(!bEColiStartPosStored) { 
                    iEColiStartPos = pos;
                    bEColiStartPosStored = true;
                }
            }
        }
    }
    iEColiEndPos = pos; 

    cout<<"--------------------------------------------------"<<endl
        <<globalComment<<endl
        <<"Structure contains "<<iEColiEndPos-iEColiStartPos<<"( "<<iEColiStartPos<<" - "
        <<iEColiEndPos<<" ) positions."<<endl
        <<"---------------------------------------------------"<<endl;

    strCen->x = strCen->x/cntr; 
    strCen->y = strCen->y/cntr;
    strCen->z = strCen->z/cntr;

    readData.close();
    free(DataFile);
}


void Structure3D::Store2Dinfo(char *info, int pos, int helixNr){
    Struct2Dinfo *data, *temp;
    data = new Struct2Dinfo;
    data->base    = info[0];
    data->mask    = info[1];
    data->code    = info[2];
    data->pos     = pos;
    data->helixNr = helixNr;

    data->next = NULL;

    if (start2D == NULL)
        start2D = data;
    else {
        temp = start2D;
        // We know this is not NULL - list not empty!
        while (temp->next != NULL) {
            temp = temp->next;  // Move to next link in chain
        }
        temp->next = data;
    }
}

//=========== Reading Secondary Structure Data from Ecoli Secondary Structure Mask file ====================//

void Structure3D::GetSecondaryStructureInfo(void) {
    static char *DataFile = 0;  
    int rnaType    = FindTypeOfRNA();

     switch (rnaType) {
     case LSU_23S: 
         DataFile = find_data_file("SecondaryStructureModel_23SrRNA.data");
         break;
     case LSU_5S: 
         DataFile = find_data_file("SecondaryStructureModel_5SrRNA.data");
         break;
     case SSU_16S:
         DataFile = find_data_file("SecondaryStructureModel_16SrRNA.data");
         break;
     }

    char  buf[256];

    int pos, helixNr, lastHelixNr; lastHelixNr = 0;
    char info[4]; info[4] = '\0';
    bool insideHelix = false;
    bool skip = false;

    ifstream readData;
    readData.open(DataFile, ios::in);
    if (!readData.is_open()) {
        throw_IO_error(DataFile);
    }

    while (!readData.eof()) {
        readData.getline(buf,100);  
        char *tmp;
        tmp = strtok(buf, " ");
        for (int i = 0; tmp != NULL; tmp = strtok(NULL, " "), i++) 
            {
                switch (i) {
                case 0 : pos = atoi(tmp);     break;
                case 1 : info[0] = tmp[0];    break;
                case 2 : info[1] = tmp[0];    break;
                case 3 : info[2] = tmp[0];    break;
                case 4 : helixNr = atoi(tmp); break;
                }
            }
        if (((info[2] == 'S') || (info[2] == 'E')) && (helixNr > 0))  lastHelixNr = helixNr;

        if ((info[2] == 'S') || (info[2] == 'E')) {
            if (!insideHelix) insideHelix = true;
            else {
                Store2Dinfo(info, pos, lastHelixNr); 
                skip        = true;
                insideHelix = false;
            }
        }
        if (insideHelix) Store2Dinfo(info, pos, lastHelixNr);
        else {
            if (skip) skip = false;
            else Store2Dinfo(info, pos, 0);
        }
    }
    readData.close();
    free(DataFile);
}

void Structure3D::Store2D3Dinfo(Struct2Dinfo *s2D, Struct3Dinfo *s3D) {
    Struct2Dplus3D *data, *temp;
    data = new Struct2Dplus3D;
    data->base    = s2D->base;
    data->mask    = s2D->mask;
    data->code    = s2D->code;
    data->pos     = s2D->pos;;
    data->helixNr = s2D->helixNr;;
    data->x       = s3D->x;
    data->y       = s3D->y;
    data->z       = s3D->z;
    data->next    = NULL;

    if (start2D3D == NULL)
        start2D3D = data;
    else {
        temp = start2D3D;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = data;
    }
}

//=========== Combining Secondary Structrue Data with 3D Coordinates =======================//

void Structure3D::Combine2Dand3DstructureInfo(void) {
    Struct3Dinfo *temp3D;
    Struct2Dinfo *temp2D;
    int cntr = 0;

    cout<<"Missing Base Positions : "<<endl;

    temp3D = start3D;    
    temp2D = start2D;
    while ((temp3D != NULL) &&  (temp2D != NULL)) { 
        if (temp2D->pos == temp3D->pos) {
            Store2D3Dinfo(temp2D, temp3D);
        }
        else {
            while (temp2D->pos != temp3D->pos) {
                cout<<temp2D->pos<<", "; // missing base positions
                cntr++;
                temp2D = temp2D->next;
            }
            Store2D3Dinfo(temp2D, temp3D);
        }
        
        temp3D = temp3D->next;
        temp2D = temp2D->next;
    }
    cout<<endl<<"Total No. of bases missing = "<<cntr<<endl;
    
    // printing comments in the main window
    {
        RNA3D->bDisplayComments = true;
        char buf[256];
        sprintf(buf, "Total No. of bases missing = %d. See the console messages for the actual missing base positions.",cntr);
        strcat(globalComment,buf);
    }
}

void Structure3D::PointsToQuads(float x, float y, float z) {

    if (RNA3D->bPointSpritesSupported) {
        glVertex3f(x, y, z);
    }
    else {
        glBegin(GL_QUADS);
        // Front Face
        glTexCoord2f(0,0); glVertex3f(x - 1, y + 1, z + 1);
        glTexCoord2f(1,0); glVertex3f(x + 1, y + 1, z + 1);
        glTexCoord2f(1,1); glVertex3f(x + 1, y - 1, z + 1);
        glTexCoord2f(0,1); glVertex3f(x - 1, y - 1, z + 1);

        // Back Face
        glTexCoord2f(0,0); glVertex3f(x + 1, y + 1, z - 1);
        glTexCoord2f(1,0); glVertex3f(x - 1, y + 1, z - 1);
        glTexCoord2f(1,1); glVertex3f(x - 1, y - 1, z - 1);
        glTexCoord2f(0,1); glVertex3f(x + 1, y - 1, z - 1);

        // Top Face
        glTexCoord2f(0,0); glVertex3f(x + 1, y + 1, z + 1);
        glTexCoord2f(1,0); glVertex3f(x - 1, y + 1, z + 1);
        glTexCoord2f(1,1); glVertex3f(x - 1, y + 1, z - 1);
        glTexCoord2f(0,1); glVertex3f(x + 1, y + 1, z - 1);

        // Bottom Face
        glTexCoord2f(0,0); glVertex3f(x + 1, y - 1, z - 1);
        glTexCoord2f(1,0); glVertex3f(x - 1, y - 1, z - 1);
        glTexCoord2f(1,1); glVertex3f(x - 1, y - 1, z + 1);
        glTexCoord2f(0,1); glVertex3f(x + 1, y - 1, z + 1);

        // Left Face
        glTexCoord2f(0,0); glVertex3f(x + 1, y + 1, z + 1);
        glTexCoord2f(1,0); glVertex3f(x + 1, y + 1, z - 1);
        glTexCoord2f(1,1); glVertex3f(x + 1, y - 1, z - 1);
        glTexCoord2f(0,1); glVertex3f(x + 1, y - 1, z + 1);

        // Right Face
        glTexCoord2f(0,0); glVertex3f(x - 1, y + 1, z - 1);
        glTexCoord2f(1,0); glVertex3f(x - 1, y + 1, z + 1);
        glTexCoord2f(1,1); glVertex3f(x - 1, y - 1, z + 1);
        glTexCoord2f(0,1); glVertex3f(x - 1, y - 1, z - 1);

        glEnd();
    }
}

void Structure3D::PositionsToCoordinatesDispList(int listID, int *pos, int len){
    Struct2Dplus3D *t;
    int tmpPos = 0;

    glNewList(listID, GL_COMPILE);
    {   
        if (RNA3D->bPointSpritesSupported) {
            glBegin(GL_POINTS);
        }
        for(int i = 0; i < len; i++) 
            {
                tmpPos = pos[i];
                t = start2D3D;
                while (t != NULL) {
                    if (t->pos == tmpPos) {
                        PointsToQuads(t->x, t->y, t->z);
                        break;
                    }
                    t = t->next;
                }
            }
        if (RNA3D->bPointSpritesSupported){
            glEnd();
        }
    }
    glEndList();
}

void Structure3D::GenerateSecStructureNonHelixRegions(void) {
    Struct2Dplus3D *t;
    const int MAX_BASE = 1000;
    int baseA[MAX_BASE], baseG[MAX_BASE], baseC[MAX_BASE], baseU[MAX_BASE];
    int a,g,c,u; a=g=c=u=0;

    {
        t = start2D3D;
        while (t != NULL) {
            if (t->helixNr == 0) {
                switch (t->base) 
                    {
                    case 'A' : baseA[a++] = t->pos; break;
                    case 'G' : baseG[g++] = t->pos; break;
                    case 'C' : baseC[c++] = t->pos; break;
                    case 'U' : baseU[u++] = t->pos; break;
                    }
            }
            t = t->next;
        }
    }

    PositionsToCoordinatesDispList(NON_HELIX_A, baseA, a);
    PositionsToCoordinatesDispList(NON_HELIX_G, baseG, g);
    PositionsToCoordinatesDispList(NON_HELIX_C, baseC, c);
    PositionsToCoordinatesDispList(NON_HELIX_U, baseU, u);
}

void Structure3D::GenerateSecStructureHelixRegions(void) {
    Struct2Dplus3D *t;
    const int MAX_BASE = 1000;
    int baseA[MAX_BASE], baseG[MAX_BASE], baseC[MAX_BASE], baseU[MAX_BASE];
    int a,g,c,u; a=g=c=u=0;

    {
        t = start2D3D;
        while (t != NULL) {
            if (t->helixNr > 0) {
                if ((t->mask == '[') || (t->mask == ']') || 
                    (t->mask == '<') || (t->mask == '>') ) 
                    {
                        switch (t->base) 
                            {
                            case 'A' : baseA[a++] = t->pos; break;
                            case 'G' : baseG[g++] = t->pos; break;
                            case 'C' : baseC[c++] = t->pos; break;
                            case 'U' : baseU[u++] = t->pos; break;
                            }
                    }
            }
            t = t->next;
        }
    }

    PositionsToCoordinatesDispList(HELIX_A, baseA, a);
    PositionsToCoordinatesDispList(HELIX_G, baseG, g);
    PositionsToCoordinatesDispList(HELIX_C, baseC, c);
    PositionsToCoordinatesDispList(HELIX_U, baseU, u);
}

void Structure3D::GenerateSecStructureUnpairedHelixRegions(void) {
    Struct2Dplus3D *t;
    const int MAX_BASE = 500;
    int baseA[MAX_BASE], baseG[MAX_BASE], baseC[MAX_BASE], baseU[MAX_BASE];
    int a,g,c,u; a=g=c=u=0;

    {
        t = start2D3D;
        while (t != NULL) {
            if (t->helixNr > 0) {
                if (t->mask == '.')
                    {
                        switch (t->base) 
                            {
                            case 'A' : baseA[a++] = t->pos; break;
                            case 'G' : baseG[g++] = t->pos; break;
                            case 'C' : baseC[c++] = t->pos; break;
                            case 'U' : baseU[u++] = t->pos; break;
                            }
                    }
            }
            t = t->next;
        }
    }

    PositionsToCoordinatesDispList(UNPAIRED_HELIX_A, baseA, a);
    PositionsToCoordinatesDispList(UNPAIRED_HELIX_G, baseG, g);
    PositionsToCoordinatesDispList(UNPAIRED_HELIX_C, baseC, c);
    PositionsToCoordinatesDispList(UNPAIRED_HELIX_U, baseU, u);
}

//==============================================================================
// Tertiary Interactions of 16S ribosomal RNA model of E.coli. 
// Reference : http://www.rna.icmb.utexas.edu/
// Year of Last Update : 2001.
// Pseudoknots and Triple Base pairs are extracted and displayed in 
// the 3D model.
//==============================================================================

void Structure3D::GenerateTertiaryInteractionsDispLists(){
    Struct2Dplus3D *t;
    static char *DataFile = 0;  
    int rnaType    = FindTypeOfRNA();

    switch (rnaType) {
    case LSU_23S: 
        DataFile = find_data_file("TertiaryInteractions_23SrRNA.data");
        break;
    case LSU_5S: 
        cout<<"There are no tertiary interactions observed in 5S rRNA model!"<<endl;
        return;
    case SSU_16S:
        DataFile = find_data_file("TertiaryInteractions_16SrRNA.data");
        break;
    }
    
    cout<<"Tertiary Interactions are fetched from comparative RNA website [http://www.rna.icmb.utexas.edu]."<<endl
        <<"The same are located in the file \""<<DataFile<<"\"."<<endl;

    char  buf[256];

    ifstream readData;
    readData.open(DataFile, ios::in);
    if (!readData.is_open()) {
        throw_IO_error(DataFile);
    }

    int K[100];
    int R[100];
    int k, r; k = r = 0;

    while (!readData.eof()) {
        readData.getline(buf,100);  
        char *tmp;
        tmp = strtok(buf, " ");
        if (tmp != NULL) 
            {
                if (strcmp(tmp, "KNOT") == 0) {
                    tmp = strtok(NULL, ":" );
                    while (tmp != NULL) {
                        K[k++] = atoi(tmp); 
                        tmp = strtok(NULL, ":" );
                    }
                }
                else if (strcmp(tmp,"TRIPLE") == 0) {
                    tmp = strtok(NULL, ":" );
                    while (tmp != NULL) {
                        R[r++] = atoi(tmp);
                        tmp = strtok(NULL, ":" );
                    }
                }
            }
    }
    readData.close();

    glNewList(ECOLI_TERTIARY_INTRACTION_PSEUDOKNOTS, GL_COMPILE);
    {   
        for(int i = 0; i < k; ) {
            glBegin(GL_LINES);
            for(int j = 0; j < 2; j++) 
                {
                    t = start2D3D;
                    while (t != NULL) {
                        if (t->pos == K[i]) {
                            glVertex3f(t->x, t->y, t->z);
                            i++;
                            break;
                        }
                        t = t->next;
                    }
                }
            glEnd();
        }
    }
    glEndList();

    glNewList(ECOLI_TERTIARY_INTRACTION_TRIPLE_BASES, GL_COMPILE);
    {   
        for(int i = 0; i < r; ) {
            glBegin(GL_LINE_STRIP);
            for(int j = 0; j < 3; j++) 
                {
                    t = start2D3D;
                    while (t != NULL) {
                        if (t->pos == R[i]) {
                            glVertex3f(t->x, t->y, t->z);
                            i++;
                            break;
                        }
                        t = t->next;
                    }
                }
            glEnd();
        }
    }
    glEndList();
    free(DataFile);
}

//==============================================================================

void Structure3D::StoreHelixNrInfo(float x, float y, float z, int helixNr) {
    HelixNrInfo *data, *temp;
    data = new HelixNrInfo;
    data->helixNr = helixNr;;
    data->x       = x;
    data->y       = y;
    data->z       = z;
    data->next    = NULL;

    if (start == NULL)
        start = data;
    else {
        temp = start;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = data;
    }
}

void Structure3D::GenerateHelixDispLists(int HELIX_NR_ID, int HELIX_NR) {
    Struct2Dinfo *temp2D;
    Struct2Dplus3D *temp2D3D;

    const int MAX = 500;

    int thisStrandPos[MAX], otherStrandPos[MAX];
    int i, j; i = j = 0;
    {
        temp2D = start2D;
        while (temp2D != NULL) { 
            if (temp2D->helixNr == HELIX_NR) {
                if ((temp2D->mask == '[') || (temp2D->mask == '<'))
                    thisStrandPos[i++]  = temp2D->pos;
                if ((temp2D->mask == ']') || (temp2D->mask == '>'))
                    otherStrandPos[j++] = temp2D->pos;
            }
            temp2D = temp2D->next;
        }
    }

    int tempPos = 0;
    float x1,x2,y1,y2,z1,z2; x1=x2=y1=y2=z1=z2=0.0;

    bool bThisStrand, bOtherStrand;
    bThisStrand = bOtherStrand = false;

    bool bMissingBasePair = false;

    glNewList(HELIX_NR_ID, GL_COMPILE);
    {
        for (int k = 0, l = j-1; k < i && l >= 0; k++, l--)
            {
                tempPos = thisStrandPos[k];
                {
                    temp2D3D = start2D3D;
                    while (temp2D3D != NULL) { 
                        if (temp2D3D->pos == tempPos) { 
                            bThisStrand = true;
                            x1=temp2D3D->x; y1=temp2D3D->y; z1=temp2D3D->z;
                            break;
                        }
                        temp2D3D = temp2D3D->next;
                    }
                }
                tempPos = otherStrandPos[l];
                {
                    temp2D3D = start2D3D;
                    while (temp2D3D != NULL) { 
                        if (temp2D3D->pos == tempPos) { 
                            bOtherStrand = true;
                            x2=temp2D3D->x; y2=temp2D3D->y; z2=temp2D3D->z;
                            break;
                        }
                        temp2D3D = temp2D3D->next;
                    }
                }
                //if bases present in both the strands then draw a bond
                //and store the helix number information
                if (bThisStrand && bOtherStrand){
                    glVertex3f(x1, y1, z1);
                    glVertex3f(x2, y2, z2);
                    
                    x1 = (x1+x2)/2; y1 = (y1+y2)/2; z1 = (z1+z2)/2;
                    StoreHelixNrInfo(x1,y1,z1,HELIX_NR);

                    bThisStrand = bOtherStrand = false;
                }
                else {
                    bMissingBasePair = true;
                    cout<<thisStrandPos[k]<<"-"<<tempPos<<"("<<HELIX_NR_ID<<"), ";
                }
            }
        if (bMissingBasePair) cout<<endl;
    }
    glEndList();
}

void Structure3D::GenerateHelixNrDispList(int startHx, int endHx) {
    HelixNrInfo *t;

    glNewList(HELIX_NUMBERS, GL_COMPILE);
    {
        char POS[50];
        for(int i = startHx; i <= endHx; i++) {
            t = start;    
            while (t != NULL) {
                if (t->helixNr == i) {
                    sprintf(POS, "%d", t->helixNr);
                    GRAPHICS->PrintString(t->x, t->y, t->z, POS, GLUT_BITMAP_8_BY_13);
                }
                t = t->next;
            }
        }
    }
    glEndList();

    glNewList(HELIX_NUMBERS_POINTS, GL_COMPILE);
    {
        if (RNA3D->bPointSpritesSupported) {
            glBegin(GL_POINTS);
        }
        for(int i = startHx; i <= endHx; i++) {
            t = start;    
            while (t != NULL) {
                if (t->helixNr == i) {
                    PointsToQuads(t->x, t->y, t->z);
                }
                t = t->next;
            }
        }
        if (RNA3D->bPointSpritesSupported) {
            glEnd();
        }
    }
    glEndList();
}

void Structure3D::GenerateDisplayLists(void){

    GenerateMoleculeSkeleton();
    ComputeBasePositions();
    
    int rnaType = FindTypeOfRNA();
    switch (rnaType) {
    case SSU_16S:
        if (glIsList(HelixBase) != GL_TRUE) 
            {
                for (int i = 1; i <= 50;i++) {
                    GenerateHelixDispLists(HelixBase+i, i);
                }
            }
        break;
    case LSU_23S:
        cout<<"=========================================================="<<endl
            <<"Missing base pairs (bracket number indicates helix number) "<<endl;
        if (glIsList(HelixBase) != GL_TRUE) 
            {
                for (int i = 1; i <= 101;i++) {
                    GenerateHelixDispLists(HelixBase+i, i);
                }
            }
        cout<<"=========================================================="<<endl;
        break;
    case LSU_5S:
        if (glIsList(HelixBase) != GL_TRUE) 
            {
                for (int i = 1; i <= 5;i++) {
                    GenerateHelixDispLists(HelixBase+i, i);
                }
            }
        break;
    }

    GenerateSecStructureHelixRegions();
    GenerateSecStructureNonHelixRegions();
    GenerateSecStructureUnpairedHelixRegions();

    GenerateTertiaryInteractionsDispLists();
}

void Structure3D::GenerateMoleculeSkeleton(void){
    Struct2Dplus3D *t;

    glNewList(STRUCTURE_BACKBONE, GL_COMPILE);
    {   
        glBegin(GL_LINE_STRIP);
        t = start2D3D;    
        while (t != NULL) {
            glVertex3f(t->x, t->y, t->z);
            t = t->next;
        }
        glEnd();
    }
    glEndList();

    glNewList(STRUCTURE_BACKBONE_CLR, GL_COMPILE);
    {   
        glBegin(GL_LINE_STRIP);
        t = start2D3D;    
        while (t != NULL) 
            {
                if (t->helixNr > 0) {
                    if ((t->mask == '[') || (t->mask == ']') || 
                        (t->mask == '<') || (t->mask == '>') ) 
                        {
                            GRAPHICS->SetColor(RNA3D_GC_BASES_HELIX);
                            glVertex3f(t->x, t->y, t->z);
                        }
                    if (t->mask == '.') {
                        GRAPHICS->SetColor(RNA3D_GC_BASES_UNPAIRED_HELIX);
                        glVertex3f(t->x, t->y, t->z);
                    }
                }
                 if (t->helixNr == 0) {
                     GRAPHICS->SetColor(RNA3D_GC_BASES_NON_HELIX);
                     glVertex3f(t->x, t->y, t->z);
                 }
                 t = t->next;
            }
        glEnd();
    }
    glEndList();
}

void Structure3D::GenerateCursorPositionDispList(long pos){
    Struct3Dinfo *temp;

    glNewList(ECOLI_CURSOR_POSITION, GL_COMPILE); 
    {
        glBegin(GL_POINTS);
        temp = start3D;    
        while (temp != NULL) {
            if(temp->pos == pos) {
#ifdef DEBUG
                cout<<"Cursor Position : "<<pos<<endl;
#endif
                glVertex3f(temp->x, temp->y, temp->z);
                break;
            }
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();
}

void Structure3D::ComputeBasePositions(){
    Struct3Dinfo *temp;

    char POS[50];
    float spacer = 1.5;
    int posSkip = iInterval;
    if (posSkip <= 0) posSkip = 25; // default interval value

    glNewList(STRUCTURE_POS, GL_COMPILE);
    {
        temp = start3D;    
        while (temp != NULL) {
            if(temp->pos%posSkip == 0) {
                sprintf(POS, "%d", temp->pos);
                GRAPHICS->PrintString(temp->x-spacer, temp->y, temp->z-spacer, POS, GLUT_BITMAP_HELVETICA_10);
            }
            temp = temp->next;
        }
    }
    glEndList();

    glNewList(STRUCTURE_POS_ANCHOR, GL_COMPILE);
    {  
        glLineWidth(0.5);
        glBegin(GL_LINES);
        temp = start3D;    
        while (temp != NULL) {
            if(temp->pos%posSkip == 0) {
                glVertex3f(temp->x, temp->y, temp->z);
                glVertex3f(temp->x-spacer, temp->y, temp->z-spacer);
            }
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();
}

void Structure3D::PrepareSecondaryStructureData(void) {
//     const char 
//         outFile[]      = "/nfshome/yadhu/DataBase/Struct3D/model16SrRNA/test.data",
//         EcoliFile[]    = "/nfshome/yadhu/DataBase/Struct3D/model16SrRNA/ECOLI_GAPS",
//         HelixNrFile[]  = "/nfshome/yadhu/DataBase/Struct3D/model16SrRNA/HELIX_NR",
//         HelixGapFile[] = "/nfshome/yadhu/DataBase/Struct3D/model16SrRNA/HELIX_GAP",
//         ErrorMsg[] = "\n *** Error Opening File : ";

    const char 
        outFile[]      = "/home/Yadhu/DataBase/Struct3D/model23SrRNA/test.data",
        EcoliFile[]    = "/home/Yadhu/DataBase/Struct3D/model23SrRNA/Ecoli_23SrRNA_with_gaps.seq",
        HelixNrFile[]  = "/home/Yadhu/DataBase/Struct3D/model23SrRNA/HelixNumbers.msk",
        HelixGapFile[] = "/home/Yadhu/DataBase/Struct3D/model23SrRNA/SecStructureMask_with_gaps.msk",
        ErrorMsg[] = "\n *** Error Opening File : ";

//     const char 
//         outFile[]      = "/home/Yadhu/DataBase/Struct3D/model5SrRNA/test.data",
//         EcoliFile[]    = "/home/Yadhu/DataBase/Struct3D/model5SrRNA/Ecoli_5SrRNA_with_gaps.seq",
//         HelixNrFile[]  = "/home/Yadhu/DataBase/Struct3D/model5SrRNA/HelixNumbers.msk",
//         HelixGapFile[] = "/home/Yadhu/DataBase/Struct3D/model5SrRNA/SecStructureMask_with_gaps.msk",
//         ErrorMsg[] = "\n *** Error Opening File : ";

    int fileLen = 0;
    char *helixNrBuf, *helixGapBuf, *ecoliBuf;

    ofstream out;
    out.open(outFile, ios::out);
    if(!out.is_open())   cerr<<ErrorMsg<<outFile<<endl;

    ifstream inFile;
    {
        {
            inFile.open(EcoliFile, ios::binary);
            if(!inFile.is_open())   cerr<<ErrorMsg<<HelixGapFile<<endl;
    
            inFile.seekg (0, ios::end);  // get length of file
            fileLen = inFile.tellg();
            inFile.seekg (0, ios::beg);
    
            ecoliBuf = new char[fileLen];    // allocate memory:

            inFile.read (ecoliBuf,fileLen);     // read data as a block:
            inFile.close();
        }

        {
            inFile.open(HelixNrFile, ios::binary);
            if(!inFile.is_open())   cerr<<ErrorMsg<<HelixGapFile<<endl;
    
            inFile.seekg (0, ios::end);  // get length of file
            fileLen = inFile.tellg();
            inFile.seekg (0, ios::beg);
    
            helixNrBuf = new char[fileLen];    // allocate memory:

            inFile.read (helixNrBuf,fileLen);     // read data as a block:
            inFile.close();
        }

        {
            inFile.open(HelixGapFile, ios::binary);
            if(!inFile.is_open())   cerr<<ErrorMsg<<HelixNrFile<<endl;
    
            inFile.seekg (0, ios::end);  // get length of file
            fileLen = inFile.tellg();
            inFile.seekg (0, ios::beg);
    
            helixGapBuf = new char[fileLen];    // allocate memory:

            inFile.read (helixGapBuf,fileLen);     // read data as a block:
            inFile.close();
        }

        char helixNr[4]; helixNr[3] = '\0';
        int pos, skip, gaps, k;
        pos = skip = gaps = k = 0;

        for(unsigned int i = 0; i < strlen(ecoliBuf); i++) 
            {
                if (ecoliBuf[i] == '\n')    skip++;
                else {
                    if ((ecoliBuf[i] == '.') || (ecoliBuf[i] == '-'))  gaps++;
                    else 
                        {
                            pos = (i - (skip + gaps)) + 1; 
                            out<<pos<<"  "<<ecoliBuf[i]<<"  "<<helixGapBuf[i]<<"  ";
                            switch (helixGapBuf[i]) 
                                {
                                case '.' : out<<"N  "; break;
                                case '[' : out<<"S  "; break;
                                case ']' : out<<"E  "; break;
                                case '<' : out<<"H  "; break;
                                case '>' : out<<"H  "; break;
                                }
                            for(unsigned int j = i; helixNrBuf[j] != '.'; j++) {
                                helixNr[k++] = helixNrBuf[j];
                                //                                    out<<helixNrBuf[j];
                            }
                            for(int l = k; l < 4; l++)   helixNr[l] = '\0';
                            k = 0;
                            if ((helixGapBuf[i] == '[') || (helixGapBuf[i] == ']')){
                                out<<helixNr;
                            }
                            out<<endl;
                        }
                }
            }
        delete [] helixNrBuf;
        delete [] ecoliBuf;
        delete [] helixGapBuf;
        out.close();
    }
}

void Structure3D::StoreCurrSpeciesDifference(char base, int pos){
    Struct2Dplus3D *st;
    st = start2D3D;
    while (st != NULL) {
        if(st->pos == pos) {
            CurrSpecies *data, *temp;
            data = new CurrSpecies;
            data->base = base; 
            data->pos  = pos;
            data->x = st->x;
            data->y = st->y;
            data->z = st->z;
            data->next = NULL;

            if (startSp == NULL){
                startSp = data;
                bOldSpecesDataExists = true;
            }
            else {
                temp = startSp;
                while (temp->next != NULL) {
                    temp = temp->next;
                }
                temp->next = data;
            }
            break;
        }
        st = st->next;
    }
}

void Structure3D::DeleteOldSpeciesData(){
    CurrSpecies *tmp, *data;

    for(data = startSp; data != NULL; data = tmp) {
        tmp = data->next;
        delete data;
    }
    startSp = NULL; 

    bOldSpecesDataExists = false;
}

void Structure3D::GenerateBaseDifferencePositionDisplayList(){
    CurrSpecies *t;
    Struct3Dinfo *temp;

    char POS[50];
    float spacer = 1.5;
    
    glNewList(MAP_SPECIES_BASE_DIFFERENCE_POS, GL_COMPILE);
    {
        for(t = startSp; t != NULL; t = t->next) 
            {
                for(temp = start3D; temp != NULL; temp = temp->next) 
                    {
                        if(temp->pos == t->pos) {
                            sprintf(POS, "%d", temp->pos);
                            GRAPHICS->PrintString(temp->x-spacer, temp->y, temp->z-spacer, POS, GLUT_BITMAP_8_BY_13);
                        }
                    }
            }
    }
    glEndList();

    glNewList(MAP_SPECIES_BASE_DIFFERENCE_POS_ANCHOR, GL_COMPILE);
    {
        glLineWidth(1.0);
        glBegin(GL_LINES);

        for(t = startSp; t != NULL; t = t->next) 
            {
                for(temp = start3D; temp != NULL; temp = temp->next) 
                    {
                        if(temp->pos == t->pos) {
                            glVertex3f(temp->x, temp->y, temp->z);
                            glVertex3f(temp->x-spacer, temp->y, temp->z-spacer);
                        }
                    }
            }
        glEnd();
    }
    glEndList();
}

void Structure3D::BuildDisplayList(int listID, int *pos, int len){
    CurrSpecies *t;
    int tmpPos = 0;

    glNewList(listID, GL_COMPILE);
    {   
        if (RNA3D->bPointSpritesSupported) {
            glBegin(GL_POINTS);
        }
        for(int i = 0; i < len; i++) 
            {
                tmpPos = pos[i];
                t = startSp;
                while (t != NULL) {
                    if (t->pos == tmpPos) {
                        PointsToQuads(t->x, t->y, t->z);
                        break;
                    }
                    t = t->next;
                }
            }
        if (RNA3D->bPointSpritesSupported){
            glEnd();
        }
    }
    glEndList();
}

void Structure3D::GenerateBaseDifferenceDisplayList(){
    CurrSpecies *t;

    glNewList(MAP_SPECIES_BASE_DIFFERENCE, GL_COMPILE);
    {   
        if (RNA3D->bPointSpritesSupported) {
            glBegin(GL_POINTS);
        }
        t = startSp;
        while (t != NULL) {
            PointsToQuads(t->x, t->y, t->z);
            t = t->next;
        }
        if (RNA3D->bPointSpritesSupported){
            glEnd();
        }
    }
    glEndList();

    const int MAX_BASE = 400;
    int baseA[MAX_BASE], baseG[MAX_BASE], baseC[MAX_BASE], 
        baseU[MAX_BASE], deletion[MAX_BASE], miss[MAX_BASE];
    int a,g,c,u,d,m; a=g=c=u=d=m=0;

    {
        t = startSp;
        while (t != NULL) {
            switch (t->base) 
                {
                case 'A' : baseA[a++]    = t->pos; break;
                case 'G' : baseG[g++]    = t->pos; break;
                case 'C' : baseC[c++]    = t->pos; break;
                case 'U' : baseU[u++]    = t->pos; break;
                case '-' : deletion[d++] = t->pos; break;
                case '.' : miss[m++] = t->pos; break;
                }
            t = t->next;
        }
        BuildDisplayList(MAP_SPECIES_BASE_A, baseA, a);
        BuildDisplayList(MAP_SPECIES_BASE_U, baseU, u);
        BuildDisplayList(MAP_SPECIES_BASE_G, baseG, g);
        BuildDisplayList(MAP_SPECIES_BASE_C, baseC, c);
        BuildDisplayList(MAP_SPECIES_DELETION, deletion, d);
        BuildDisplayList(MAP_SPECIES_MISSING, miss, m);
        GenerateBaseDifferencePositionDisplayList();

        iTotalSubs = a+g+c+u;  // Summing up the substitutions/mutations
        iTotalDels = d;        // Storing total number of deletions
#ifdef DEBUG
        cout<<"Substitutions = "<<iTotalSubs<<endl;
        cout<<"Deletions     = "<<iTotalDels<<endl;
#endif
    }
}

void Structure3D::StoreInsertions(char base, int pos){
    Insertions *data, *temp;
    data = new Insertions;
    data->base = base; 
    data->pos  = pos;
    data->next = NULL;
    
    if (startIns == NULL){
        startIns = data;
        bOldInsertionDataExists = true;
    }
    else {
        temp = startIns;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = data;
    }
}

void Structure3D::DeleteOldInsertionData(){
    Insertions *tmp, *data;

    for(data = startIns; data != NULL; data = tmp) {
        tmp = data->next;
        delete data;
    }
    startIns = NULL; 

    bOldInsertionDataExists = false;
}

void Structure3D::GenerateInsertionDisplayList(){
    Insertions   *ins;
    Struct3Dinfo *str;
    char inserts[500];
    int i, cntr;
    float spacer = 2.0;
    iTotalIns = 0;

    glNewList(MAP_SPECIES_INSERTION_BASES, GL_COMPILE);
    {   
        for(str = start3D; str != NULL; str = str->next) 
        {
            i = cntr = 0;
            for(ins = startIns; ins != NULL; ins = ins->next) 
                {
                    if(str->pos == ins->pos) {
                        inserts[i++] = ins->base;
                        cntr++; 
                    }
                }
            if(cntr>0) {
                inserts[i] = '\0';
                char buffer[strlen(inserts) + 10];
                sprintf(buffer, "%d:%s", cntr, inserts);
                GRAPHICS->PrintString(str->x, str->y+spacer, str->z, 
                                      buffer, GLUT_BITMAP_8_BY_13);

                iTotalIns += cntr; // Summing up the insertions
            }
        }
    }
    glEndList();

#ifdef DEBUG
    cout<<"Insertions = "<<iTotalIns<<endl;
#endif

    glNewList(MAP_SPECIES_INSERTION_BASES_ANCHOR, GL_COMPILE);
    {   
        glLineWidth(1.0);
        glBegin(GL_LINES);

        for(str = start3D; str != NULL; str = str->next) 
        {
            for(ins = startIns; ins != NULL; ins = ins->next) 
                {
                    if(str->pos == ins->pos) {
                        glVertex3f(str->x, str->y, str->z);          
                        glVertex3f(str->x, str->y+spacer, str->z);          
                    }
                }
        }
        glEnd();
    }
    glEndList();

    glNewList(MAP_SPECIES_INSERTION_POINTS, GL_COMPILE);
    {   
        if (RNA3D->bPointSpritesSupported) {
            glBegin(GL_POINTS);
        }
        for(str = start3D; str != NULL; str = str->next) 
        {
            for(ins = startIns; ins != NULL; ins = ins->next) 
                {
                    if(str->pos == ins->pos) {
                        PointsToQuads(str->x, str->y, str->z);
                        break;
                    }
                }
        }
        if (RNA3D->bPointSpritesSupported){
            glEnd();
        }
    }
    glEndList();
}

void Structure3D::MapCurrentSpeciesToEcoliTemplate(AW_root *awr){

    GB_push_transaction(gb_main);

    GBDATA *gbTemplate = GBT_find_SAI(gb_main,"ECOLI");

    if (!gbTemplate) {
        aw_message("SAI:ECOLI not found");
    }
    else {
        char *pSpeciesName = awr->awar(AWAR_SPECIES_NAME)->read_string();
        if (pSpeciesName) {
            ED4_SeqTerminal   = ED4_find_seq_terminal(pSpeciesName); // initializing the seqTerminal to get the current terminal
            GBDATA *gbSpecies = GBT_find_species(gb_main, pSpeciesName);
            if(gbSpecies) {
                char   *ali_name          = GBT_get_default_alignment(gb_main);
                GBDATA *gbAlignment       = GB_find(gbTemplate, ali_name, 0, down_level);
                GBDATA *gbTemplateSeqData = gbAlignment ? GB_find(gbAlignment, "data", 0, down_level) : 0;
                
                if (!gbTemplateSeqData) {
                    aw_message(GBS_global_string("Mapping impossible, since SAI:ECOLI has no data in alignment '%s'", ali_name));
                }
                else {
                    const char *pTemplateSeqData  = GB_read_char_pntr(gbTemplateSeqData);

                    if(!RNA3D->bEColiRefInitialised) {
                        EColiRef = new BI_ecoli_ref();
                        EColiRef->init(gb_main);
                        RNA3D->bEColiRefInitialised = true;
                    }

                    char buf[100];
                    char *pSpFullName = GB_read_string(GB_find(gbSpecies, "full_name", 0, down_level));
                    sprintf(buf, "%s : %s", pSpeciesName, pSpFullName);
                    awr->awar(AWAR_3D_SELECTED_SPECIES)->write_string(buf); 

                    GBDATA *gbSeqData    = GBT_read_sequence(gbSpecies, ali_name);
                    const char *pSeqData = GB_read_char_pntr(gbSeqData); 
                    int iSeqCount = 0;

                    if (pSeqData && pTemplateSeqData) {
                        int iSeqLen = strlen(pTemplateSeqData); 

                        if(bOldSpecesDataExists) {
                            DeleteOldSpeciesData();
                        }

                        for(int i = 0; i<iSeqLen; i++) {
                            if((pTemplateSeqData[i] != '.') && (pTemplateSeqData[i] != '-'))
                            { 
                                if (!bStartPosStored) {
                                    iStartPos = i; 
                                    bStartPosStored = true;
                                }
                                if(pTemplateSeqData[i] != pSeqData[i]) {
                                    StoreCurrSpeciesDifference(pSeqData[i],iSeqCount);
                                }
                                iSeqCount++;
                            }
                        }

                        for(int i = iSeqLen; i>0; i--) {
                            if((pTemplateSeqData[i] != '.') && (pTemplateSeqData[i] != '-')){ 
                                if (!bEndPosStored) {
                                    iEndPos = i; 
                                    bEndPosStored = true;
                                    break;
                                }
                            }
                        }

                        if(bOldInsertionDataExists) {
                            DeleteOldInsertionData();
                        }

                        for(int i = iStartPos, iSeqCount = 0; i < iEndPos; i++) {
                            if((pTemplateSeqData[i] != '.') && 
                               (pTemplateSeqData[i] != '-'))
                                { // Store EColi base positions : Insertion point !
                                    iSeqCount++;                                    
                                }

                            if((pTemplateSeqData[i] == '-') &&
                               (pSeqData[i]         != '-') &&
                               (pSeqData[i]         != '.'))
                                { 
                                    StoreInsertions(pSeqData[i],iSeqCount);
                                }
                        }
                    }
                    free(pSpFullName);
                }
                free(ali_name);
            }
            GenerateBaseDifferenceDisplayList();
            GenerateInsertionDisplayList();
        }
        free(pSpeciesName);
    }
    GB_pop_transaction(gb_main);
}

static bool ValidSearchColor(int iColor, int mode) {
    bool isValid = false;

    switch (mode) {
        case SAI:    isValid = (iColor >= RNA3D_GC_CBACK_0) && (iColor < RNA3D_GC_MAX); break;
        case SEARCH: isValid = (iColor >= RNA3D_GC_SBACK_0) && (iColor < RNA3D_GC_MAX); break;
    }
    return isValid;
}

void Structure3D::MapSearchStringsToEcoliTemplate(AW_root * /*awr*/){
    if (ED4_SeqTerminal) { 
        const char *pSearchColResults = 0;

        if (iMapSearch) {
            // buildColorString builds the background color of each base
            pSearchColResults = ED4_buildColorString(ED4_SeqTerminal, iStartPos, iEndPos); 
        }
        
        if(pSearchColResults) {
            int iColor = 0;

            glNewList(MAP_SEARCH_STRINGS_TO_STRUCTURE, GL_COMPILE);
            {
                if (RNA3D->bPointSpritesSupported) {
                    glBegin(GL_POINTS);
                }
                for (int i = iStartPos; i < iEndPos; i++) {
                    if(RNA3D->bEColiRefInitialised) {
                        long absPos   = (long) i;
                        long EColiPos = EColiRef->abs_2_rel(absPos);

                        for(Struct3Dinfo *t = start3D; t != NULL; t = t->next)
                        {
                            if ((t->pos == EColiPos) && (pSearchColResults[i] >= 0))
                                    {
                                        iColor = pSearchColResults[i] - COLORLINK;

                                        if(ValidSearchColor(iColor, SEARCH)) {
                                            RNA3D->cGraphics->SetColor(iColor);
                                            PointsToQuads(t->x, t->y, t->z);
                                        }
                                        break;
                                    }
                            }
                    }
                }
                if (RNA3D->bPointSpritesSupported) {
                    glEnd();
                }
            }
            glEndList();

            glNewList(MAP_SEARCH_STRINGS_BACKBONE, GL_COMPILE);
            {
                int iLastClr = 0; int iLastPos = 0; Vector3 vLastPt;
                glBegin(GL_LINES);
                for (int i = iStartPos; i < iEndPos; i++) {
                    if(RNA3D->bEColiRefInitialised) {
                        long absPos   = (long) i;
                        long EColiPos = EColiRef->abs_2_rel(absPos);

                        for(Struct3Dinfo *t = start3D; t != NULL; t = t->next)
                        {
                            if ((t->pos == EColiPos) && (pSearchColResults[i] >= 0))
                                    {
                                        iColor = pSearchColResults[i] - COLORLINK;

                                        if(ValidSearchColor(iColor, SEARCH)) {
                                            if ((iLastClr == iColor) && (iLastPos == EColiPos-1)) {
                                                RNA3D->cGraphics->SetColor(iColor);
                                                glVertex3f(vLastPt.x, vLastPt.y, vLastPt.z);
                                                glVertex3f(t->x, t->y, t->z);
                                            }
                                            iLastPos = EColiPos;
                                            iLastClr = iColor;
                                            vLastPt.x = t->x; vLastPt.y = t->y; vLastPt.z = t->z;
                                        }
                                        break;
                                    }
                            }
                    }
                }
                glEnd();
            }
            glEndList();

            RNA3D->bMapSearchStringsDispListCreated = true;
        }
        else cout<<"BuildColorString did not get the colors : SAI cannot be Visualized!"<<endl;                                                     
    }
    else cout<<"Problem with initialization : SAI cannot be Visualized!"<<endl;  
}

void Structure3D::MapSaiToEcoliTemplate(AW_root *awr){
    if (ED4_SeqTerminal) { 
        const char *pSearchColResults = 0;

        if (iMapSAI && ED4_SAIs_visualized()) {
            pSearchColResults = ED4_getSaiColorString(awr, iStartPos, iEndPos); // returns 0 if sth went wrong
        }

        if(pSearchColResults) {
            int iColor = 0;

            glNewList(MAP_SAI_TO_STRUCTURE, GL_COMPILE);
            {
                if (RNA3D->bPointSpritesSupported) {
                    glBegin(GL_POINTS);
                }
                for (int i = iStartPos; i < iEndPos; i++) {
                    if(RNA3D->bEColiRefInitialised) {
                        long absPos   = (long) i;
                        long EColiPos = EColiRef->abs_2_rel(absPos);

                        for(Struct3Dinfo *t = start3D; t != NULL; t = t->next)
                        {
                            if ((t->pos == EColiPos) && (pSearchColResults[i] >= 0))
                                    {
                                        iColor = pSearchColResults[i-1] - SAICOLORS;

                                        if(ValidSearchColor(iColor, SAI)) {
                                            RNA3D->cGraphics->SetColor(iColor);
                                            PointsToQuads(t->x, t->y, t->z);
                                        }
                                        break;
                                    }
                            }
                    }
                }
                if (RNA3D->bPointSpritesSupported) {
                    glEnd();
                }
            }
            glEndList();

            RNA3D->bMapSaiDispListCreated = true;
        }
        else cout<<"ED4_getSaiColorString did not get the colors : SAI cannot be Visualized!"<<endl;                                                     
    }
    else cout<<"Problem with initialization : SAI cannot be Visualized!"<<endl;  
}
