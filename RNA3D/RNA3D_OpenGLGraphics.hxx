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
    int screenXmax,screenYmax, mouseX, mouseY;
    bool displayGrid;

    OpenGLGraphics(void);
    virtual  ~OpenGLGraphics(void);

    void WinToScreenCoordinates(int x, int y, GLdouble  *screenPos);
    void ScreenToWinCoordinates(int x, int y, GLdouble *winPos);

    void DrawCursor(int x, int y);
    void SetBackGroundColor(int color);
    void PrintString(float x, float y, float z, char *s, void *font);
    void PrintCharacter(float x, float y, float z, char c, void *font);
    void SetColor(int gc);
    ColorRGBf GetColor(int gc);

    void DrawCircle(float radius, float x, float y);
    void DrawCube(float x, float y, float z, float radius);
    void DrawSphere(float radius, float x, float y, float z);

    void Draw3DSGrid();
    void DrawAxis(float x, float y, float z, float len);
};

