#include <Xm/Xm.h>

#define CREATED     1
#define NOT_CREATED 0

void ReshapeOpenGLWindow(GLint width, GLint height);
void InitializeOpenGLEngine(GLint width, GLint height);
void RenderOpenGLScene(Widget w);
void InitializeOpenGLWindow(Widget w);
void ComputeRotationXY(int x, int y);
void MapDisplayParameters(AW_root *aw_root);
void DisplayPostionsIntervalChanged_CB(AW_root *awr);
void MapSelectedSpeciesChanged_CB(AW_root *awr);
void CursorPositionChanged_CB(AW_root *awr);
void DisplayHelixNrsChanged_CB(AW_root *awr);
void MapSaiToEcoliTemplateChanged_CB(AW_root *awr);
void MapSearchStringsToEcoliTemplateChanged_CB(AW_root *awr);
void WinToScreenCoordinates(int x, int y, GLdouble *screenPos);
