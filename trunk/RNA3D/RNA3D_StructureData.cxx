#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_StructureData.hxx"

using namespace std;

inline float _min(float a, float b) { return (a>b)? b:a; }
inline float _max(float a, float b) { return (a<b)? b:a; }

Vector3 strCen = Vector3(0.0, 0.0, 0.0);

OpenGLGraphics *GRAPHICS = new OpenGLGraphics();

Structure3D::Structure3D(void) {
    xCenter = yCenter = zCenter = 0.0;
    xStrPoint = yStrPoint = zStrPoint = 0.0;
}

Structure3D::~Structure3D(void) {
}

Struct3Dinfo *start3D = NULL;

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

void Structure3D::ReadCoOrdinateFile(Vector3 *sCen) {
    const char 
        DataFile[] = "data/Ecoli_1M5G_16SrRNA.pdb",
        ErrorMsg[] = "\n *** Error Opening File : ";
    char buf[256];

    float X, Y, Z;
    unsigned int pos;
    char Base;

    ifstream readData;
    readData.open(DataFile, ios::in);
    if (!readData.is_open()) {
        cerr<<ErrorMsg<<DataFile<<endl;
        exit(1);
    }

    int cntr = 0;

    while (!readData.eof()) {
        readData.getline(buf,100);  
        string tmp, atom, line = string(buf);

        if (line.find("ATOM") != string::npos ) {
            atom = (line.substr(77,2)).c_str();
            if (atom.find("P") != string::npos) {
                tmp = (line.substr(18,3)).c_str();
                Base = tmp[1];
                pos  = atoi((line.substr(22,4)).c_str());
                X    = atof((line.substr(31,8)).c_str());
                Y    = atof((line.substr(39,8)).c_str());
                Z    = atof((line.substr(47,8)).c_str());
                StoreCoordinates(X,Y,Z,Base,pos);
                
                strCen.x += X; strCen.y += Y; strCen.z += Z;
                cntr++;
            }
        }
    }
    strCen.x = strCen.x/cntr; strCen.y = strCen.y/cntr; strCen.z = strCen.z/cntr;
    sCen->x = strCen.x; sCen->y = strCen.y; sCen->z = strCen.z;

    readData.close();
}

Struct2Dinfo  *start2D = NULL;

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

void Structure3D::GetSecondaryStructureInfo(void) {
    const char 
        DataFile[] = "data/ECOLI_SECONDARY_STRUCTURE_INFO",
        ErrorMsg[] = "\n *** Error Opening File : ";
    char buf[256];

    int pos, helixNr, lastHelixNr; lastHelixNr = 0;
    char info[4]; info[4] = '\0';
    bool insideHelix = false;
    bool skip = false;

    ifstream readData;
    readData.open(DataFile, ios::in);
    if (!readData.is_open()) {
        cerr<<ErrorMsg<<DataFile<<endl;
        exit(1);
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
                Store2Dinfo(info, pos, lastHelixNr); skip = true;
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
}

Struct2Dplus3D *start2D3D = NULL;

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

void Structure3D::Combine2Dand3DstructureInfo(void) {
    Struct3Dinfo *temp3D;
    Struct2Dinfo *temp2D;

    temp3D = start3D;    
    temp2D = start2D;
    while ((temp3D != NULL) &&  (temp2D != NULL)) { 
        if (temp2D->pos == temp3D->pos) {
            Store2D3Dinfo(temp2D, temp3D);
        }
        temp3D = temp3D->next;
        temp2D = temp2D->next;
    }
}

void GenerateNonHelixBaseDispList(void) {
    Struct2Dplus3D *temp;

    glNewList(NON_HELIX_BASES, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        temp = start2D3D;
        while (temp != NULL) { 
            if ((temp->helixNr >= 0) && (temp->mask == '.')) { 
                glVertex3f(temp->x, temp->y, temp->z);
            }
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();
}

HelixNrInfo *start = NULL;

void StoreHelixNrInfo(float x, float y, float z, int helixNr) {
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

void PositionsToCoordinatesDispList(int listID, int *pos, int len){
    Struct2Dplus3D *t;
    int tmpPos = 0;

    glNewList(listID, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        for(int i = 0; i < len; i++) 
            {
                tmpPos = pos[i];
                t = start2D3D;
                while (t != NULL) {
                    if (t->pos == tmpPos) {
                        glVertex3f(t->x, t->y, t->z); break;
                    }
                    t = t->next;
                }
            }
        glEnd();
    }
    glEndList();
}

void GenerateSecStructureNonHelixRegions(void) {
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

void GenerateSecStructureHelixRegions(void) {
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

void GenerateSecStructureUnpairedHelixRegions(void) {
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

void Structure3D::GenerateHelixDispLists(int HELIX_NR_ID, int HELIX_NR) {
    Struct3Dinfo *temp3D;
    Struct2Dinfo *temp2D;
    Struct2Dplus3D *temp2D3D;

    const int MAX = 200;

    int thisStrandPos[MAX], otherStrandPos[MAX];
    int i, j; i = j = 0;
    {
        temp3D = start3D;    
        temp2D = start2D;
        while ((temp3D != NULL) &&  (temp2D != NULL)) { 
            if (temp2D->helixNr == HELIX_NR) {
                if ((temp2D->mask == '[') || (temp2D->mask == '<'))
                    thisStrandPos[i++]  = temp2D->pos;
                if ((temp2D->mask == ']') || (temp2D->mask == '>'))
                    otherStrandPos[j++] = temp2D->pos;
            }
            temp3D = temp3D->next;
            temp2D = temp2D->next;
        }
    }

    int tempPos = 0;
    float x1,x2,y1,y2,z1,z2; x1=x2=y1=y2=z1=z2=0.0;

    glNewList(HELIX_NR_ID, GL_COMPILE);
    {
        for (int k = 0, l = j-1; k < i && l >= 0; k++, l--)
            {
                tempPos = thisStrandPos[k];
                {
                    temp2D3D = start2D3D;
                    while (temp2D3D != NULL) { 
                        if (temp2D3D->pos == tempPos) { 
                            glVertex3f(temp2D3D->x, temp2D3D->y, temp2D3D->z);
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
                            glVertex3f(temp2D3D->x, temp2D3D->y, temp2D3D->z);
                            x2=temp2D3D->x; y2=temp2D3D->y; z2=temp2D3D->z;
                            break;
                        }
                        temp2D3D = temp2D3D->next;
                    }
                }
                x1 = (x1+x2)/2; y1 = (y1+y2)/2; z1 = (z1+z2)/2;
                StoreHelixNrInfo(x1,y1,z1,HELIX_NR);
            }
    }
    glEndList();
}

void Generate2DmaskDispLists(int dispLstID, char MASK) {
    Struct3Dinfo *temp3D;
    Struct2Dinfo *temp2D;

    glNewList(dispLstID, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        temp3D = start3D;    
        temp2D = start2D;
        while ((temp3D != NULL) &&  (temp2D != NULL)) {
            if (temp2D->mask == MASK) {
                glVertex3f(temp3D->x, temp3D->y, temp3D->z);
            }
            temp3D = temp3D->next;
            temp2D = temp2D->next;
        }
        glEnd();
    }
    glEndList();
}

void Structure3D::BuildSecondaryStructureMask(void){
    Generate2DmaskDispLists(STRUCTURE_2D_MASK_HELIX_START, '[');
    Generate2DmaskDispLists(STRUCTURE_2D_MASK_HELIX_END, ']');
    Generate2DmaskDispLists(STRUCTURE_2D_MASK_HELIX_FORWARD_STRAND, '<');
    Generate2DmaskDispLists(STRUCTURE_2D_MASK_HELIX_BACKWARD_STRAND, '>');
    Generate2DmaskDispLists(STRUCTURE_2D_MASK_NON_HELIX, '.');


    for (int i = 1; i <= 50;i++) {
        GenerateHelixDispLists(i, i);
    }

    GenerateNonHelixBaseDispList();
    GenerateSecStructureNonHelixRegions();
    GenerateSecStructureHelixRegions();
    GenerateSecStructureUnpairedHelixRegions();

    glNewList(HELIX_NUMBERS, GL_COMPILE);
    {
        char POS[50];
        HelixNrInfo *t = start;    
        while (t != NULL) {
            sprintf(POS, "%d", t->helixNr);
            GRAPHICS->PrintString(t->x, t->y, t->z, POS, GLUT_BITMAP_HELVETICA_10);
            t = t->next;
        }
    }
    glEndList();

    glNewList(HELIX_NUMBERS_POINTS, GL_COMPILE);
    {
        HelixNrInfo *t = start;    
        glBegin(GL_POINTS);
        while (t != NULL) {
            glVertex3f(t->x, t->y, t->z);
            t = t->next;
        }
        glEnd();
    }
    glEndList();
}

void GenerateDisplayLists(int dispLstID, char BASE){
    Struct3Dinfo *temp;

    glNewList(dispLstID, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        temp = start3D;    
        while (temp != NULL) {
            if(temp->base == BASE) {
                glVertex3f(temp->x, temp->y, temp->z);
            }
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();
}

void Structure3D::PrepareStructureSkeleton(void){
    Struct3Dinfo *temp;
    // drawing structure 

    glNewList(STRUCTURE_BACKBONE_POINTS, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        temp = start3D;    
        while (temp != NULL) {
            glVertex3f(temp->x, temp->y, temp->z);
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();

    glNewList(STRUCTURE_BACKBONE_POINTS_CLR, GL_COMPILE);
    {
        glBegin(GL_POINTS);
        temp = start3D;    
        while (temp != NULL) {
            GRAPHICS->DrawPoints(temp->x, temp->y, temp->z, temp->base);
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();

    glNewList(STRUCTURE_BACKBONE, GL_COMPILE);
    {   
        glBegin(GL_LINE_STRIP);
        temp = start3D;    
        while (temp != NULL) {
            glVertex3f(temp->x, temp->y, temp->z);
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();

    glNewList(STRUCTURE_BACKBONE_CLR, GL_COMPILE);
    {   
        glBegin(GL_LINE_STRIP);
        temp = start3D;    
        while (temp != NULL) {
            GRAPHICS->DrawPoints(temp->x, temp->y, temp->z, temp->base);
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();

    GenerateDisplayLists(STRUCTURE_BACKBONE_POINTS_A, 'A');
    GenerateDisplayLists(STRUCTURE_BACKBONE_POINTS_G, 'G');
    GenerateDisplayLists(STRUCTURE_BACKBONE_POINTS_C, 'C');
    GenerateDisplayLists(STRUCTURE_BACKBONE_POINTS_U, 'U');

    DrawStructureInfo();
}

void Structure3D::DrawStructureInfo(void){
    Struct3Dinfo *temp;
    // drawing position numbers
    char POS[50];
    float spacer = 1.5;//0.5;
    int posSkip = 25;

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

    glNewList(STRUCTURE_SEARCH_POINTS, GL_COMPILE);
    {   
        glBegin(GL_POINTS);
        temp = start3D;    
        while (temp != NULL) {
            if(temp->pos >= startPos && temp->pos <= endPos) {
                glVertex3f(temp->x, temp->y, temp->z);
            }
            temp = temp->next;
        }
        glEnd();
    }
    glEndList();

    glNewList(STRUCTURE_SEARCH, GL_COMPILE);
    {   
        temp = start3D;    
        while (temp != NULL) {
            if(temp->pos >= startPos && temp->pos <= endPos) {
                glVertex3f(temp->x, temp->y, temp->z);
                if(temp->pos == ((startPos+endPos)/2)) {
                    xStrPoint = temp->x; yStrPoint = temp->y; zStrPoint = temp->z;
                }
            }
            temp = temp->next;
        }
    }
    glEndList();

}

void Structure3D::PrepareSecondaryStructureInfo(void) {
    const char 
        outFile[]      = "data/test.data",
        EcoliFile[]    = "data/ECOLI_GAPS",
        HelixNrFile[]  = "data/HELIX_NR",
        HelixGapFile[] = "data/HELIX_GAP",
        ErrorMsg[] = "\n *** Error Opening File : ";

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

//     {
//         {
//             inFile.open(BaseFile, ios::binary);
//             if(!inFile.is_open())   cerr<<ErrorMsg<<HelixGapFile<<endl;
    
//             inFile.seekg (0, ios::end);  // get length of file
//             fileLen = inFile.tellg();
//             inFile.seekg (0, ios::beg);
    
//             BaseBuf = new char[fileLen];    // allocate memory:

//             inFile.read (BaseBuf,fileLen);     // read data as a block:
//             inFile.close();
//         }

//         {
//             inFile.open(HelixFile, ios::binary);
//             if(!inFile.is_open())   cerr<<ErrorMsg<<HelixNrFile<<endl;
    
//             inFile.seekg (0, ios::end);  // get length of file
//             fileLen = inFile.tellg();
//             inFile.seekg (0, ios::beg);
    
//             HelixBuf = new char[fileLen];    // allocate memory:

//             inFile.read (HelixBuf,fileLen);     // read data as a block:
//             inFile.close();
//         }

//         int pos, skip; pos = skip = 0;

//         for(unsigned int i = 0; i < strlen(HelixBuf); i++) 
//             {
//                 if (HelixBuf[i] == '\n')    skip++;
//                 else {
//                     pos = (i - skip) + 1; if(pos>1542) break;
//                     cout<<pos<<"  "<<BaseBuf[i]<<"\t"<<HelixBuf[i]<<"\t";
//                     switch (HelixBuf[i]) 
//                         {
//                         case '.' : cout<<"N"; break;
//                         case '[' : cout<<"S"; break;
//                         case ']' : cout<<"E"; break;
//                         case '<' : cout<<"H"; break;
//                         case '>' : cout<<"H"; break;
//                         }
//                     cout<<endl;
//                 }
//             }
//     }

}


