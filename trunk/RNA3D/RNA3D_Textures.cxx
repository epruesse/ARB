#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_OpenGLEngine.hxx"

using namespace std;

Texture2D::Texture2D(void){
}

Texture2D::~Texture2D(void){
}

static char* GetImageFile(int ImageId){
    const char *imageName = 0;

    switch (ImageId) {
        case CIRCLE:          imageName = "Circle.png"; break;
        case DIAMOND:         imageName = "Diamond.png"; break;
        case POLYGON:         imageName = "Polygon.png"; break;
        case STAR:            imageName = "Star.png"; break;
        case RECTANGLE:       imageName = "Rectangle.png"; break;
        case RECTANGLE_ROUND: imageName = "RectangleRound.png"; break;
        case STAR_SMOOTH:     imageName = "StarSmooth.png"; break;
        case CONE_UP:         imageName = "ConeUp.png"; break;
        case CONE_DOWN:       imageName = "ConeDown.png"; break;
        case CROSS:           imageName = "Cross.png"; break;
        case QUESTION:        imageName = "Question.png"; break;
        case DANGER:          imageName = "Danger.png"; break;
        case HEXAGON:         imageName = "Hexagon.png"; break;
        case LETTER_A:        imageName = "LetterA.png"; break;
        case LETTER_G:        imageName = "LetterG.png"; break;
        case LETTER_C:        imageName = "LetterC.png"; break;
        case LETTER_U:        imageName = "LetterU.png"; break;
    }

    if (!imageName) {
        throw string(GBS_global_string("Illegal image id %i", ImageId));
    }

    char *fname = GBS_find_lib_file(imageName, "rna3d/images/", 0);
    if (!fname) {
        throw string("File not found: ")+imageName;
    }
    return fname;
}

// Load Bitmaps And Convert To Textures
void Texture2D::LoadGLTextures(void) {	

    for (int i = 0; i < SHAPE_MAX; i++)
    {
        char    *ImageFile = GetImageFile(i);
        pngInfo  info;

        // Using pngLoadAndBind to set texture parameters automatically.
        texture[i] = pngBind(ImageFile, PNG_NOMIPMAP, PNG_ALPHA, &info, GL_CLAMP, GL_NEAREST, GL_NEAREST);

        if (texture[i] == 0) {
            throw string(GBS_global_string("Error loading %s", ImageFile));
        }

    if (!GLOBAL->bPointSpritesSupported) {
       glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
    }
    else {
       glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    }

#ifdef DEBUG
        cout<<ImageFile<<" : Size = "<<info.Width<<" x "<<info.Height <<", Depth = "
            <<info.Depth<<", Alpha = "<<info.Alpha<<endl;
#endif

        free(ImageFile);
    }

}

