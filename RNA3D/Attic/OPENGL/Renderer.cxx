#include "GlobalHeader.hxx"
#include "Globals.hxx"
#include "TextureMapping.hxx"
#include "OpenGLGraphics.hxx"
#include "Renderer.hxx"

GLRenderer::GLRenderer(void){
    StartHelix  = 1;
    EndHelix    = 50;
    HelixWidth = 1.5;
    ObjectSize = 8.0;
}

GLRenderer::~GLRenderer(void){
}

void GLRenderer::DisplayHelices(void){
    glColor4fv(RED);
    glLineWidth(HelixWidth);
    for(int i = StartHelix; i <= EndHelix; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayHelixBackBone(void){
    glColor4fv(DEFAULT);
    glLineWidth(0.5);
    for(int i = 1; i <= 50; i++) {
        glBegin(GL_LINES);
        glCallList(i);
        glEnd();
    }
}

void GLRenderer::DisplayPositions(void){
    glColor4fv(DEFAULT);
    glCallList(STRUCTURE_POS_ANCHOR);
    glColor4f(0.8,0.8,0.8,1);
    glCallList(STRUCTURE_POS);
}

void GLRenderer::DisplayHelixNumbers(void){
    glColor4f(0.8,0.8,0.8,1);
    glCallList(HELIX_NUMBERS);
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


void GLRenderer::TexturizeStructure(int mode, Texture2D *cImages, OpenGLGraphics *cGraphics ) {
    glPointSize(ObjectSize);
    switch(mode) 
        {
        case BASES:        
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);
            glColor4fv(GREEN);        
            glCallList(STRUCTURE_BACKBONE_POINTS_A);
            
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);
            glColor4fv(BLUE);        
            glCallList(STRUCTURE_BACKBONE_POINTS_G);
            
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);
            glColor4fv(BLUE);        
            glCallList(STRUCTURE_BACKBONE_POINTS_C);
            
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);
            glColor4fv(GREEN);        
            glCallList(STRUCTURE_BACKBONE_POINTS_U);
            break;

        case HELIX_MAPPING:
            glBindTexture(GL_TEXTURE_2D, cImages->texture[STAR]);
            glColor4fv(WHITE);        
            glCallList(NON_HELIX_BASES);
            break;

        case HELIX_MASK:
            glPointSize(HelixWidth+5);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
            glColor4fv(WHITE);        
            glCallList(HELIX_NUMBERS_POINTS);
            break;

        case BASES_HELIX:
            float RGB[3];
            cGraphics->HexaDecimalToRGB("FFFF00", RGB);
            glColor4f(RGB[0], RGB[1], RGB[2],1);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(HELIX_A);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(HELIX_G);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(HELIX_C);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(HELIX_U);
            break;

        case BASES_UNPAIRED_HELIX:
            glColor4fv(GREEN);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(UNPAIRED_HELIX_A);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(UNPAIRED_HELIX_G);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(UNPAIRED_HELIX_C);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(UNPAIRED_HELIX_U);

//             glBindTexture(GL_TEXTURE_2D, cImages->texture[FREEFORM_1]);
//             glPointSize(ObjectSize+8);
//             glCallList(UNPAIRED_HELIX_A); glCallList(UNPAIRED_HELIX_G); 
//             glCallList(UNPAIRED_HELIX_C); glCallList(UNPAIRED_HELIX_U);
            break;

        case BASES_NON_HELIX:
            glColor4fv(CYAN);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_A]);  glCallList(NON_HELIX_A);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_G]);  glCallList(NON_HELIX_G);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_C]);  glCallList(NON_HELIX_C);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[LETTER_U]);  glCallList(NON_HELIX_U);
            break;

        case BASES_COLOR:
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
            glColor4fv(RED);
            glCallList(HELIX_A); glCallList(HELIX_G); glCallList(HELIX_C); glCallList(HELIX_U);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_UP]);
            glColor4fv(YELLOW);
            glCallList(UNPAIRED_HELIX_A); glCallList(UNPAIRED_HELIX_G); glCallList(UNPAIRED_HELIX_C); glCallList(UNPAIRED_HELIX_U);
            glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_DOWN]);
            glColor4fv(CYAN);
            glCallList(NON_HELIX_A); glCallList(NON_HELIX_G); glCallList(NON_HELIX_C); glCallList(NON_HELIX_U);
            break;

       case BASES_LOOP:
           cGraphics->HexaDecimalToRGB("FFFF00", RGB);
           glColor4f(RGB[0], RGB[1], RGB[2],1);
           DrawTexturizedStructure(cImages, HELIX_A);
           glColor4fv(CYAN);
           DrawTexturizedStructure(cImages, NON_HELIX_A);
           glColor4fv(GREEN);
           DrawTexturizedStructure(cImages, UNPAIRED_HELIX_A);

           glPointSize(ObjectSize*1.75);
           glBindTexture(GL_TEXTURE_2D, cImages->texture[CIRCLE]);
           glColor4fv(BLUE);
           glCallList(STRUCTURE_SEARCH_POINTS);

           glPointSize(ObjectSize*2);
           glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_DOWN]);
           glColor4fv(WHITE);
           glCallList(UNPAIRED_HELIX_A);glCallList(UNPAIRED_HELIX_U);
           glBindTexture(GL_TEXTURE_2D, cImages->texture[CONE_UP]);
           glColor4fv(RED);
           glCallList(UNPAIRED_HELIX_G);glCallList(UNPAIRED_HELIX_C);

           break;
        }
}
