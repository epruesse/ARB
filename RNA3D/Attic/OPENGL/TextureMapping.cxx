#include "GlobalHeader.hxx"
#include "Globals.hxx"
#include "TextureMapping.hxx"

Texture2D::Texture2D(void){
}

Texture2D::~Texture2D(void){
}

char* Texture2D::GetImageFile(int ImageId){
    char Path[10] = "images";
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
void Texture2D::LoadGLTextures(int mode) {	
    switch(mode)
     {
     case PNG:
         {
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
         break;
         
     case BMP:
         {    // Load Texture
             Image *image1;
             
             // allocate space for texture
             image1 = (Image *) malloc(sizeof(Image));
             if (image1 == NULL) {
                 printf("Error allocating space for image");
                 exit(0);
             }

             if (!ImageLoad("images/Circle.bmp", image1)) {
                 exit(1);
             }        

             // Create Texture	
             glGenTextures(1, &texture[0]);
             glBindTexture(GL_TEXTURE_2D, texture[0]);   // 2d texture (x and y size)

             glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
             glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture

             // 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
             // border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
             glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);
         }
         break;
     }
}

int Texture2D::ImageLoad(char *filename, Image *image) {
    FILE *file;
    unsigned long size;                 // size of the image in bytes.
    unsigned long i;                    // standard counter.
    unsigned short int planes;          // number of planes in image (must be 1) 
    unsigned short int bpp;             // number of bits per pixel (must be 24)
    char temp;                          // temporary color storage for bgr-rgb conversion.

    // make sure the file is there.
    if ((file = fopen(filename, "rb"))==NULL)
    {
	printf("File Not Found : %s\n",filename);
	return 0;
    }
    
    // seek through the bmp header, up to the width/height:
    fseek(file, 18, SEEK_CUR);

    // read the width
    if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
	printf("Error reading width from %s.\n", filename);
	return 0;
    }
    printf("Width of %s: %lu\n", filename, image->sizeX);
    
    // read the height 
    if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
	printf("Error reading height from %s.\n", filename);
	return 0;
    }
    printf("Height of %s: %lu\n", filename, image->sizeY);
    
    // calculate the size (assuming 24 bits or 3 bytes per pixel).
    size = image->sizeX * image->sizeY * 3;

    // read the planes
    if ((fread(&planes, 2, 1, file)) != 1) {
	printf("Error reading planes from %s.\n", filename);
	return 0;
    }
    if (planes != 1) {
	printf("Planes from %s is not 1: %u\n", filename, planes);
	return 0;
    }

    // read the bpp
    if ((i = fread(&bpp, 2, 1, file)) != 1) {
	printf("Error reading bpp from %s.\n", filename);
	return 0;
    }
    if (bpp != 24) {
	printf("Bpp from %s is not 24: %u\n", filename, bpp);
	return 0;
    }
	
    // seek past the rest of the bitmap header.
    fseek(file, 24, SEEK_CUR);

    // read the data. 
    image->data = (char *) malloc(size);
    if (image->data == NULL) {
	printf("Error allocating memory for color-corrected image data");
	return 0;	
    }

    if ((i = fread(image->data, size, 1, file)) != 1) {
	printf("Error reading image data from %s.\n", filename);
	return 0;
    }

    for (i=0;i<size;i+=3) { // reverse all of the colors. (bgr -> rgb)
	temp = image->data[i];
	image->data[i] = image->data[i+2];
	image->data[i+2] = temp;
    }
    
    // we're done.
    return 1;
}
