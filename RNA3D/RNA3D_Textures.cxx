#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_Textures.hxx"

using namespace std;

Texture2D::Texture2D(void){
}

Texture2D::~Texture2D(void){
}

char* Texture2D::GetImageFile(int ImageId){
    char Path[10] = "images"; //@@FIX PATH ??? - yadhuGL
    char ImageFileName[50];

    switch (ImageId)
     {
     case CIRCLE:
         sprintf(ImageFileName, "%s/Circle.png",Path);
         break;
     case RECTANGLE:
         sprintf(ImageFileName, "%s/Rectangle.png",Path);
         break;
     case RECTANGLE_ROUND:
         sprintf(ImageFileName, "%s/RectangleRound.png",Path);
         break;
     case POLYGON:
         sprintf(ImageFileName, "%s/Polygon.png",Path);
         break;
     case STAR:
         sprintf(ImageFileName, "%s/Star.png",Path);
         break;
     case STAR_SMOOTH:
         sprintf(ImageFileName, "%s/StarSmooth.png",Path);
         break;
     case DIAMOND:
         sprintf(ImageFileName, "%s/Diamond.png",Path);
         break;
     case CONE_UP:
         sprintf(ImageFileName, "%s/ConeUp.png",Path);
         break;
     case CONE_DOWN:
         sprintf(ImageFileName, "%s/ConeDown.png",Path);
         break;
     case FREEFORM_1:
         sprintf(ImageFileName, "%s/FreeForm_1.png",Path);
         break;
     case LETTER_A:
         sprintf(ImageFileName, "%s/LetterA.png",Path);
         break;
     case LETTER_G:
         sprintf(ImageFileName, "%s/LetterG.png",Path);
         break;
     case LETTER_C:
         sprintf(ImageFileName, "%s/LetterC.png",Path);
         break;
     case LETTER_U:
         sprintf(ImageFileName, "%s/LetterU.png",Path);
         break;
     }
    return strdup(ImageFileName);
}

// Load Bitmaps And Convert To Textures
void Texture2D::LoadGLTextures(void) {	
    
    for (int i = 0; i < SHAPE_MAX; i++) 
        {
            const char *ImageFile = GetImageFile(i);
            pngInfo info;

            // Using pngLoadAndBind to set texture parameters automatically.
            texture[i] = pngBind(ImageFile, PNG_NOMIPMAP, PNG_ALPHA, &info, GL_CLAMP, GL_NEAREST, GL_NEAREST);
             
            if (texture[i] == 0) {
                cout<<"Error loading file : "<<ImageFile<<" !!!"<<endl;
                exit(0);
            }
            cout<<ImageFile<<" : Size = "<<info.Width<<" x "<<info.Height <<", Depth = "
                <<info.Depth<<", Alpha = "<<info.Alpha<<endl;
        }
}

