#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_StructureData.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Graphics.hxx"

using namespace std;

OpenGLGraphics *G = new OpenGLGraphics();
ColorRGBf ApplicationBGColor = ColorRGBf(0,0,0);

GLRenderer::GLRenderer(void){
    fSkeletonSize = 0.5;
    iBackBone = iColorise = 0;
    ObjectSize = 8.0;
    iDisplayBases = iBaseMode = 0;
    iBaseHelix  = iBaseUnpairHelix  = iBaseNonHelix  = 0;
    iShapeHelix = iShapeUnpairHelix = iShapeNonHelix = 0;
    iDisplayHelix = iHelixMidPoint = iHelixBackBone = iHelixNrs = 0;
    iDispTerInt = 0;
    iStartHelix  = 1;
    iEndHelix    = 50;
    fHelixSize = 1.0;
    iDispPos = 0;
    iDispCursorPos = 0;
    iMapSpecies = iMapSpeciesBase = iMapSpeciesPos = 0;
    iMapSpeciesDels = iMapSpeciesMiss = 0;
} 

GLRenderer::~GLRenderer(void){
}

void GLRenderer::DisplayHelices(void){
    G->SetColor(RNA3D_GC_HELIX);
    glLineWidth(fHelixSize);
    for(int i = iStartHelix; i <= iEndHelix; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayHelixBackBone(void){
    G->SetColor(RNA3D_GC_HELIX_SKELETON);
    glLineWidth(0.5);
    for(int i = 1; i <= 50; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayBasePositions(void){

    G->SetColor(RNA3D_GC_MOL_BACKBONE);
    glCallList(STRUCTURE_POS_ANCHOR);

    G->SetColor(RNA3D_GC_MOL_POS);
    glCallList(STRUCTURE_POS);

}

void GLRenderer::DisplayMappedSpBasePositions(void){
    
    G->SetColor(RNA3D_GC_MOL_BACKBONE);
    glCallList(MAP_SPECIES_BASE_DIFFERENCE_POS_ANCHOR);

    G->SetColor(RNA3D_GC_MAPPED_SPECIES);
    glCallList(MAP_SPECIES_BASE_DIFFERENCE_POS);
}

void GLRenderer::DisplayHelixMidPoints(Texture2D *cImages){
    glPointSize(fHelixSize + 5); // size will be propotional to the Helix Thickness specified !!
    glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
    G->SetColor(RNA3D_GC_HELIX_MIDPOINT);
    glCallList(HELIX_NUMBERS_POINTS);
}

void GLRenderer::DisplayHelixNumbers(void){
    G->SetColor(RNA3D_GC_FOREGROUND);
    glCallList(HELIX_NUMBERS);
}

void GLRenderer::DoHelixMapping(void) {
    if (iDisplayHelix) {
        if (iHelixNrs) {
            DisplayHelixNumbers();
        }
        if (iHelixBackBone) {
            DisplayHelixBackBone();
        }
        DisplayHelices();
    }
    // Displaying Tertiary Interactions of E.coli 16S ribosomal RNA 
    if(iDispTerInt) {
        G->SetColor(RNA3D_GC_PSEUDOKNOT);
        glCallList(ECOLI_TERTIARY_INTRACTION_PSEUDOKNOTS);
        G->SetColor(RNA3D_GC_TRIPLE_BASE);
        glCallList(ECOLI_TERTIARY_INTRACTION_TRIPLE_BASES);
    }
}

void GLRenderer::DisplayMoleculeName(int w, int h){
    extern AW_root *gAwRoot;
    char *pSpeciesName;

    if(iMapSpecies) {
        pSpeciesName = gAwRoot->awar(AWAR_3D_SELECTED_SPECIES)->read_string();
    }
    else {
        pSpeciesName = "Eschericia Coli : Master Template"; 
    }

    float x_ratio = (float) w/ (float)h; 
    float y_ratio = (float) h/ (float)w; 
    float x, y; 
    int factor = 190;
    
    glPushMatrix(); 
    glScalef(0.01, 0.01, 0.01);

    x = x_ratio*factor;
    if(x_ratio < y_ratio)  y = x_ratio*factor;
    else                   y = y_ratio*factor;

    G->SetColor(RNA3D_GC_FOREGROUND);
    G->PrintString(x, y, 0, pSpeciesName, GLUT_BITMAP_8_BY_13);

    glPopMatrix();
}

void GLRenderer::DisplayMolecule(Structure3D *cStr) {
    glLineWidth(fSkeletonSize);

    static ColorRGBf HelixOldColor         = G->GetColor(RNA3D_GC_BASES_HELIX); 
    static ColorRGBf UnpairedHelixOldColor = G->GetColor(RNA3D_GC_BASES_UNPAIRED_HELIX); 
    static ColorRGBf NonHelixOldColor      = G->GetColor(RNA3D_GC_BASES_NON_HELIX); 

    if(iColorise){
        ColorRGBf HelixNewColor         = G->GetColor(RNA3D_GC_BASES_HELIX); 
        ColorRGBf UnpairedHelixNewColor = G->GetColor(RNA3D_GC_BASES_UNPAIRED_HELIX); 
        ColorRGBf NonHelixNewColor      = G->GetColor(RNA3D_GC_BASES_NON_HELIX); 
        
        if ((HelixOldColor == HelixNewColor) && 
            (UnpairedHelixOldColor == UnpairedHelixNewColor) &&
            (NonHelixOldColor == NonHelixNewColor))
            {
                glCallList(STRUCTURE_BACKBONE_CLR);
            }
        else {
            HelixOldColor         = HelixNewColor;
            UnpairedHelixOldColor = UnpairedHelixNewColor;
            NonHelixOldColor      = NonHelixNewColor;

            glDeleteLists(STRUCTURE_BACKBONE_CLR,1);
            cStr->GenerateMoleculeSkeleton();
            glCallList(STRUCTURE_BACKBONE_CLR);
        }
    }
    else if (iBackBone) {
        G->SetColor(RNA3D_GC_MOL_BACKBONE);
        glCallList(STRUCTURE_BACKBONE);
    }

    if(iDispPos) {
        DisplayBasePositions();
    } 

    if(cStr->iMapEnable) {
        if (cStr->iMapSearch) {
            glLineWidth(ObjectSize/3);
            glCallList(MAP_SEARCH_STRINGS_BACKBONE);
        }
        if(iMapSpecies && iMapSpeciesPos) {
            DisplayMappedSpBasePositions();
        }
    }
}

void GLRenderer::BeginTexturizer(){
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_POINT_SMOOTH);

   extern bool bPointSpritesSupported;

   if (bPointSpritesSupported) {
       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

       float quadratic[] =  { 0.0f, 0.0f, 1.0f };
       glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, quadratic );
       
       // Query for the max point size supported by the hardware
       float maxSize = 0.0f;
       glGetFloatv( GL_POINT_SIZE_MAX_EXT, &maxSize );
       glPointSize(MIN(ObjectSize,maxSize) );
       
       glPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, 1.0f );
       glPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, MIN(65,maxSize));
       
       glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
       glEnable(GL_POINT_SPRITE_ARB);
   }

   glEnable(GL_TEXTURE_2D);
}

void GLRenderer::EndTexturizer(){
    glDisable(GL_TEXTURE_2D);

    extern bool bPointSpritesSupported;

    if (bPointSpritesSupported) {
        float defaultAttenuation[3] = { 1.0f, 0.0f, 0.0f };
        glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, defaultAttenuation );

        glDisable(GL_POINT_SPRITE_ARB);
        glDisable(GL_BLEND);
    }
    glEnable(GL_POINT_SMOOTH);
}

void GLRenderer::TexturizeStructure(Texture2D *cImages) {
    extern Structure3D *cStructure;

    if (cStructure->iMapEnable ) {
        glPointSize(ObjectSize*2);
        if (cStructure->iMapSAI) {
            glBindTexture(GL_TEXTURE_2D, cImages->texture[HEXAGON]);  
            glCallList(MAP_SAI_TO_STRUCTURE);
        }
        if (cStructure->iMapSearch) {
            glPointSize(ObjectSize*1.5);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);  
            glCallList(MAP_SEARCH_STRINGS_TO_STRUCTURE);
        }
    }

    if(iDispCursorPos) {
        glPointSize(ObjectSize+8);
        G->SetColor(RNA3D_GC_CURSOR_POSITION);
        glBindTexture(GL_TEXTURE_2D, cImages->texture[DIAMOND]);  
        glCallList(ECOLI_CURSOR_POSITION);
    }

    glPointSize(ObjectSize);
    if (iDisplayBases) 
        {
            switch(iBaseMode) {
            case CHARACTERS:
                if(iBaseHelix) {
                    glColor4f(ApplicationBGColor.red, ApplicationBGColor.green, ApplicationBGColor.blue, 1);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);    
                    glCallList(HELIX_A); glCallList(HELIX_G); glCallList(HELIX_C); glCallList(HELIX_U); 

                    G->SetColor(RNA3D_GC_BASES_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(HELIX_U);
                }

                if(iBaseUnpairHelix) {
                    glColor4f(ApplicationBGColor.red, ApplicationBGColor.green, ApplicationBGColor.blue, 1);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);    
                    glCallList(UNPAIRED_HELIX_A); glCallList(UNPAIRED_HELIX_G); glCallList(UNPAIRED_HELIX_C); glCallList(UNPAIRED_HELIX_U); 

                    G->SetColor(RNA3D_GC_BASES_UNPAIRED_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(UNPAIRED_HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(UNPAIRED_HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(UNPAIRED_HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(UNPAIRED_HELIX_U);
                }

                if(iBaseNonHelix) {
                    glColor4f(ApplicationBGColor.red, ApplicationBGColor.green, ApplicationBGColor.blue, 1);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);    
                    glCallList(NON_HELIX_A); glCallList(NON_HELIX_G); glCallList(NON_HELIX_C); glCallList(NON_HELIX_U); 

                    G->SetColor(RNA3D_GC_BASES_NON_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(NON_HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(NON_HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(NON_HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(NON_HELIX_U);
                }
                break;

            case SHAPES:
                if(iBaseHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeHelix]);
                    G->SetColor(RNA3D_GC_BASES_HELIX);
                    glCallList(HELIX_A); glCallList(HELIX_G); glCallList(HELIX_C); glCallList(HELIX_U);
                }
                if(iBaseUnpairHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeUnpairHelix]);
                    G->SetColor(RNA3D_GC_BASES_UNPAIRED_HELIX);
                    glCallList(UNPAIRED_HELIX_A); glCallList(UNPAIRED_HELIX_G); glCallList(UNPAIRED_HELIX_C); glCallList(UNPAIRED_HELIX_U);
                }
                if(iBaseNonHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeNonHelix]);
                    G->SetColor(RNA3D_GC_BASES_NON_HELIX);
                    glCallList(NON_HELIX_A); glCallList(NON_HELIX_G); glCallList(NON_HELIX_C); glCallList(NON_HELIX_U);
                }
                break;
            }
        }

    if (iDisplayHelix && iHelixMidPoint) {
        DisplayHelixMidPoints(cImages);  // Draw circles at the midpoint of each Helix
    }

    if (cStructure->iMapEnable && iMapSpecies) {
        if(iMapSpeciesBase) { 
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);  

            glPointSize(ObjectSize+4);
            G->SetColor(RNA3D_GC_MAPPED_SPECIES);
            glCallList(MAP_SPECIES_BASE_A); glCallList(MAP_SPECIES_BASE_U);
            glCallList(MAP_SPECIES_BASE_G); glCallList(MAP_SPECIES_BASE_C);

            glPointSize(ObjectSize);
            G->SetColor(RNA3D_GC_FOREGROUND);
            glCallList(MAP_SPECIES_BASE_A); glCallList(MAP_SPECIES_BASE_U);
            glCallList(MAP_SPECIES_BASE_G); glCallList(MAP_SPECIES_BASE_C);

            glPointSize(ObjectSize);
            G->SetColor(RNA3D_GC_MAPPED_SPECIES);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(MAP_SPECIES_BASE_A);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(MAP_SPECIES_BASE_G);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(MAP_SPECIES_BASE_C);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(MAP_SPECIES_BASE_U);
        }

        if(iMapSpeciesMiss) {
            glPointSize(ObjectSize);
            G->SetColor(RNA3D_GC_MAPPED_SPECIES);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);  
            glCallList(MAP_SPECIES_MISSING);
            glPointSize(ObjectSize-2);
            G->SetColor(RNA3D_GC_FOREGROUND);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[QUESTION]);  
            glCallList(MAP_SPECIES_MISSING);
        }

        if(iMapSpeciesDels) {
            glPointSize(ObjectSize);
            G->SetColor(RNA3D_GC_DELETION);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);  
            glCallList(MAP_SPECIES_DELETION);
            glPointSize(ObjectSize-2);
            G->SetColor(RNA3D_GC_FOREGROUND);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[DANGER]);  
            glCallList(MAP_SPECIES_DELETION);
        }
    }
}
