

class OpenGLGraphics {
public:
    int screenXmax,screenYmax, mouseX, mouseY;
    bool displayGrid;

    OpenGLGraphics(void);
    virtual  ~OpenGLGraphics(void);

    void WinToScreenCoordinates(int x, int y, GLdouble  *screenPos);

    void ClickedPos(float x, float y, float z);
    void DrawCursor(int x, int y);
    void SetColor(char base);
    void SetBackGroundColor(int color);
    void HexaDecimalToRGB(char *color, float *RGB);
    void PrintString(float x, float y, float z, char *s, void *font);
    void DrawPoints(float x, float y, float z, char base);

    void DrawCircle(float radius, float x, float y);
    void DrawCube(float x, float y, float z, float radius);
    void DrawSphere(float radius, float x, float y, float z);

    void Draw3DSGrid();
    void DrawAxis(float x, float y, float z, float len);
};
