typedef struct ColorRGBf {
public:
    float red, green, blue;
    ColorRGBf() {}
	ColorRGBf(float r, float g, float b) { red = r; green = g; blue = b; }

	bool operator==(ColorRGBf c) { 
        if((red == c.red) && (green == c.green) && (blue == c.blue)) {
            return true;
        }
        else {
            return false;
        }
    }
};

class OpenGLGraphics {
public:
    int       screenXmax,screenYmax, mouseX, mouseY;
    bool      displayGrid;
    ColorRGBf ApplicationBGColor;

    OpenGLGraphics(void);
    virtual  ~OpenGLGraphics(void);

    void WinToScreenCoordinates(int x, int y, GLdouble  *screenPos);
    void ScreenToWinCoordinates(int x, int y, GLdouble *winPos);

    void PrintString(float x, float y, float z, char *s, void *font);
    void PrintComment(float x, float y, float z, char *s);

    void init_font(GLuint base, char* f);
    void print_string(GLuint base, char* s);
    void InitMainFont(char* f);

    void SetOpenGLBackGroundColor();    
    ColorRGBf ConvertGCtoRGB(int gc);
    void SetColor(int gc);
    ColorRGBf GetColor(int gc);

    void DrawBox(float x, float y, float width, float height);
};

