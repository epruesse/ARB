
struct Image {
    unsigned long sizeX;
    unsigned long sizeY;
    char *data;
};

class Texture2D {
public:
    GLuint texture[SHAPE_MAX];  

    Texture2D(void);
    virtual ~Texture2D(void);

    int ImageLoad(char *filename, Image *image);
    char *GetImageFile(int ImageId);
    void LoadGLTextures(int mode);
};

