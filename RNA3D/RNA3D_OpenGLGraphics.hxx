
const int startPos = 0;
const int endPos   = 150;

// color conversion from Hexadecimal to floating point value 
enum HEXA_DECIMAL_VALUES {
    HEXA_00, HEXA_01, HEXA_02, HEXA_03, HEXA_04, HEXA_05, HEXA_06, HEXA_07, HEXA_08, HEXA_09, HEXA_0A, HEXA_0B, HEXA_0C, HEXA_0D, HEXA_0E, HEXA_0F, 
    HEXA_10, HEXA_11, HEXA_12, HEXA_13, HEXA_14, HEXA_15, HEXA_16, HEXA_17, HEXA_18, HEXA_19, HEXA_1A, HEXA_1B, HEXA_1C, HEXA_1D, HEXA_1E, HEXA_1F, 
    HEXA_20, HEXA_21, HEXA_22, HEXA_23, HEXA_24, HEXA_25, HEXA_26, HEXA_27, HEXA_28, HEXA_29, HEXA_2A, HEXA_2B, HEXA_2C, HEXA_2D, HEXA_2E, HEXA_2F, 
    HEXA_30, HEXA_31, HEXA_32, HEXA_33, HEXA_34, HEXA_35, HEXA_36, HEXA_37, HEXA_38, HEXA_39, HEXA_3A, HEXA_3B, HEXA_3C, HEXA_3D, HEXA_3E, HEXA_3F, 
    HEXA_40, HEXA_41, HEXA_42, HEXA_43, HEXA_44, HEXA_45, HEXA_46, HEXA_47, HEXA_48, HEXA_49, HEXA_4A, HEXA_4B, HEXA_4C, HEXA_4D, HEXA_4E, HEXA_4F, 
    HEXA_50, HEXA_51, HEXA_52, HEXA_53, HEXA_54, HEXA_55, HEXA_56, HEXA_57, HEXA_58, HEXA_59, HEXA_5A, HEXA_5B, HEXA_5C, HEXA_5D, HEXA_5E, HEXA_5F, 
    HEXA_60, HEXA_61, HEXA_62, HEXA_63, HEXA_64, HEXA_65, HEXA_66, HEXA_67, HEXA_68, HEXA_69, HEXA_6A, HEXA_6B, HEXA_6C, HEXA_6D, HEXA_6E, HEXA_6F, 
    HEXA_70, HEXA_71, HEXA_72, HEXA_73, HEXA_74, HEXA_75, HEXA_76, HEXA_77, HEXA_78, HEXA_79, HEXA_7A, HEXA_7B, HEXA_7C, HEXA_7D, HEXA_7E, HEXA_7F, 
    HEXA_80, HEXA_81, HEXA_82, HEXA_83, HEXA_84, HEXA_85, HEXA_86, HEXA_87, HEXA_88, HEXA_89, HEXA_8A, HEXA_8B, HEXA_8C, HEXA_8D, HEXA_8E, HEXA_8F, 
    HEXA_90, HEXA_91, HEXA_92, HEXA_93, HEXA_94, HEXA_95, HEXA_96, HEXA_97, HEXA_98, HEXA_99, HEXA_9A, HEXA_9B, HEXA_9C, HEXA_9D, HEXA_9E, HEXA_9F, 
    HEXA_A0, HEXA_A1, HEXA_A2, HEXA_A3, HEXA_A4, HEXA_A5, HEXA_A6, HEXA_A7, HEXA_A8, HEXA_A9, HEXA_AA, HEXA_AB, HEXA_AC, HEXA_AD, HEXA_AE, HEXA_AF, 
    HEXA_B0, HEXA_B1, HEXA_B2, HEXA_B3, HEXA_B4, HEXA_B5, HEXA_B6, HEXA_B7, HEXA_B8, HEXA_B9, HEXA_BA, HEXA_BB, HEXA_BC, HEXA_BD, HEXA_BE, HEXA_BF, 
    HEXA_C0, HEXA_C1, HEXA_C2, HEXA_C3, HEXA_C4, HEXA_C5, HEXA_C6, HEXA_C7, HEXA_C8, HEXA_C9, HEXA_CA, HEXA_CB, HEXA_CC, HEXA_CD, HEXA_CE, HEXA_CF, 
    HEXA_D0, HEXA_D1, HEXA_D2, HEXA_D3, HEXA_D4, HEXA_D5, HEXA_D6, HEXA_D7, HEXA_D8, HEXA_D9, HEXA_DA, HEXA_DB, HEXA_DC, HEXA_DD, HEXA_DE, HEXA_DF, 
    HEXA_E0, HEXA_E1, HEXA_E2, HEXA_E3, HEXA_E4, HEXA_E5, HEXA_E6, HEXA_E7, HEXA_E8, HEXA_E9, HEXA_EA, HEXA_EB, HEXA_EC, HEXA_ED, HEXA_EE, HEXA_EF, 
    HEXA_F0, HEXA_F1, HEXA_F2, HEXA_F3, HEXA_F4, HEXA_F5, HEXA_F6, HEXA_F7, HEXA_F8, HEXA_F9, HEXA_FA, HEXA_FB, HEXA_FC, HEXA_FD, HEXA_FE, HEXA_FF
};

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

