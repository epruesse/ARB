#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_StructureData.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Graphics.hxx"

using namespace std;

GLRenderer::GLRenderer(void){
    fSkeletonSize = 0.5;
    iBackBone = iColorise = 0;
    ObjectSize = 8.0;
    iDisplayBases = iBaseMode = 0;
    iBaseHelix  = iBaseUnpairHelix  = iBaseNonHelix  = 0;
    iShapeHelix = iShapeUnpairHelix = iShapeNonHelix = 0;
    iDisplayHelix = iHelixMidPoint = iHelixBackBone = iHelixNrs = 0;
    iStartHelix  = 1;
    iEndHelix    = 50;
    fHelixSize = 1.0;
}

GLRenderer::~GLRenderer(void){
}

void GLRenderer::DisplayHelices(void){
    SetColor(RNA3D_GC_HELIX);
    glLineWidth(fHelixSize);
    for(int i = iStartHelix; i <= iEndHelix; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayHelixBackBone(void){
    SetColor(RNA3D_GC_HELIX_SKELETON);
    glLineWidth(0.5);
    for(int i = 1; i <= 50; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayPositions(void){
    glColor4f(0.8,0.8,0.8,1);
    glCallList(STRUCTURE_POS_ANCHOR);
    glCallList(STRUCTURE_POS);
}

void GLRenderer::DisplayHelixMidPoints(Texture2D *cImages){
    glPointSize(fHelixSize + 5); // size will be propotional to the Helix Thickness specified !!
    glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
    SetColor(RNA3D_GC_HELIX_MIDPOINT);
    glCallList(HELIX_NUMBERS_POINTS);
}

void GLRenderer::DisplayHelixNumbers(void){
    glColor4f(0.8,0.8,0.8,1);
    glCallList(HELIX_NUMBERS);
}

void GLRenderer::DoHelixMapping(void) {
    if (iDisplayHelix) {
        if (iHelixBackBone) {
            DisplayHelixBackBone();
        }
        if (iHelixNrs) {
            DisplayHelixNumbers();
        }
        DisplayHelices();
    }
}

void GLRenderer::DisplayMolecule(void) {
    glLineWidth(fSkeletonSize);
    if(iColorise){
        glCallList(STRUCTURE_BACKBONE_CLR);
    }
    else if (iBackBone) {
        SetColor(RNA3D_GC_MOL_BACKBONE);
        glCallList(STRUCTURE_BACKBONE);
    }
}

void GLRenderer::BeginTexturizer(){
   glDisable(GL_CULL_FACE);
   glDisable(GL_LIGHTING);
   
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_BLEND );
   glEnable(GL_POINT_SPRITE_ARB);
   glEnable(GL_TEXTURE_2D);
}

void GLRenderer::EndTexturizer(){
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void GLRenderer::DrawTexturizedStructure(Texture2D *cImages, int Structure ) {
    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(Structure);
    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(Structure+1);
    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(Structure+2);
    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(Structure+3);
}

void GLRenderer::SetColor(int gc) {
    float r, g, b;
    extern AW_window_menu_modes_opengl *awm;
    const char *ERROR = awm->GC_to_RGB_float(awm->get_device(AW_MIDDLE_AREA), gc, r, g, b);
    if (ERROR) {
        cout<<"Error retrieving RGB values for GC #"<<gc<<" : "<<ERROR
            <<endl<<"Setting the color to default white"<<endl;
        glColor4f(1,1,1,1);
    }
    else {
        glColor4f(r,g,b,1);
    }
}

void GLRenderer::TexturizeStructure(Texture2D *cImages) {
    glPointSize(ObjectSize);

    if (iDisplayBases) 
        {
            switch(iBaseMode) {
            case CHARACTERS:
                if(iBaseHelix) {
                    SetColor(RNA3D_GC_BASES_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(HELIX_U);
                }
                if(iBaseUnpairHelix) {
                    SetColor(RNA3D_GC_BASES_UNPAIRED_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(UNPAIRED_HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(UNPAIRED_HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(UNPAIRED_HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(UNPAIRED_HELIX_U);
                }
                if(iBaseNonHelix) {
                    SetColor(RNA3D_GC_BASES_NON_HELIX);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(NON_HELIX_A);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(NON_HELIX_G);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(NON_HELIX_C);
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(NON_HELIX_U);
                }
                break;

            case SHAPES:
                if(iBaseHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeHelix]);
                    SetColor(RNA3D_GC_BASES_HELIX);
                    glCallList(HELIX_A); glCallList(HELIX_G); glCallList(HELIX_C); glCallList(HELIX_U);
                }
                if(iBaseUnpairHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeUnpairHelix]);
                    SetColor(RNA3D_GC_BASES_UNPAIRED_HELIX);
                    glCallList(UNPAIRED_HELIX_A); glCallList(UNPAIRED_HELIX_G); glCallList(UNPAIRED_HELIX_C); glCallList(UNPAIRED_HELIX_U);
                }
                if(iBaseNonHelix) {
                    glBindTexture(GL_TEXTURE_2D, cImages->texture[iShapeNonHelix]);
                    SetColor(RNA3D_GC_BASES_NON_HELIX);
                    glCallList(NON_HELIX_A); glCallList(NON_HELIX_G); glCallList(NON_HELIX_C); glCallList(NON_HELIX_U);
                }
                break;
            }
        }

    if (iDisplayHelix && iHelixMidPoint) {
        DisplayHelixMidPoints(cImages);  // Draw circles at the midpoint of each Helix
    }
    
//     switch(mode) 
//         {
//         case BASES:        
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);
//             glColor4fv(GREEN);        
//             glCallList(STRUCTURE_BACKBONE_POINTS_A);
            
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);
//             glColor4fv(BLUE);        
//             glCallList(STRUCTURE_BACKBONE_POINTS_G); 
            
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);
//             glColor4fv(BLUE);        
//             glCallList(STRUCTURE_BACKBONE_POINTS_C);
            
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);
//             glColor4fv(GREEN);        
//             glCallList(STRUCTURE_BACKBONE_POINTS_U);
//             break;

//         case HELIX_MAPPING:
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[STAR]);
//             glColor4fv(WHITE);        
//             glCallList(NON_HELIX_BASES);
//             break;

//         case HELIX_MASK:
//             glPointSize(HelixWidth+5);
//             glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
//             glColor4fv(WHITE);        
//             glCallList(HELIX_NUMBERS_POINTS);
//             break;

//        case BASES_LOOP:
//            glColor4f(0, 1, 0,1);
//            DrawTexturizedStructure(cImages, HELIX_A);
//            glColor4fv(CYAN);
//            DrawTexturizedStructure(cImages, NON_HELIX_A);
//            glColor4fv(GREEN);
//            DrawTexturizedStructure(cImages, UNPAIRED_HELIX_A);

//            glPointSize(ObjectSize*1.75);
//            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
//            glColor4fv(BLUE);
//            glCallList(STRUCTURE_SEARCH_POINTS);

//            glPointSize(ObjectSize*2);
//            glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_DOWN]);
//            glColor4fv(WHITE);
//            glCallList(UNPAIRED_HELIX_A);glCallList(UNPAIRED_HELIX_U);
//            glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_UP]);
//            glColor4fv(RED);
//            glCallList(UNPAIRED_HELIX_G);glCallList(UNPAIRED_HELIX_C);

//            break;
//         }
}
