
class Texture2D {
public:
    GLuint texture[SHAPE_MAX];  

    Texture2D(void);
    virtual ~Texture2D(void);

    char *GetImageFile(int ImageId);
    void LoadGLTextures(void);
};

