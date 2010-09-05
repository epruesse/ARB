
enum {
    CIRCLE,  
    DIAMOND,
    POLYGON,
    STAR,
    RECTANGLE, 
    RECTANGLE_ROUND, 
    STAR_SMOOTH, 
    CONE_UP,
    CONE_DOWN,
    CROSS,
    QUESTION,
    DANGER,
    HEXAGON,
    LETTER_A,
    LETTER_G,
    LETTER_C,
    LETTER_U,
    SHAPE_MAX
};

class Texture2D {
public:
    GLuint texture[SHAPE_MAX];  

    Texture2D(void);
    virtual ~Texture2D(void);

    // char *GetImageFile(int ImageId);
    void LoadGLTextures(void);
};

