
class GLRenderer {
public:
    int StartHelix, EndHelix;
    float HelixWidth;
    float ObjectSize;

    GLRenderer(void);
    virtual ~GLRenderer(void);

    void DisplayHelices();
    void DisplayHelixBackBone(void);
    void DisplayHelixNumbers(void);
    void DisplayPositions(void);

    void BeginTexturizer();
    void EndTexturizer();
    void TexturizeStructure(int mode, Texture2D *cImages, OpenGLGraphics *cGraphics);
    void DrawTexturizedStructure(Texture2D *cImages, int Structure );
};
