struct POINT {
public:
    int x, y;
    POINT(){ x = y = 0; }
};

// This is our basic 3D point/vector struct

class CCamera {

private:
	Vector3 m_vPosition;		// The camera's position
	Vector3 m_vView;			// The camera's View
	Vector3 m_vUpVector;		// The camera's UpVector

public:

	CCamera();	

	// These are the data access functions for camera's private data
	Vector3 Position() {	return m_vPosition;		}
	Vector3 View()		{	return m_vView;			}
	Vector3 UpVector() {	return m_vUpVector;		}
    
    POINT MousePos;

    void GetCursorPos(POINT *pos)   { pos->x = MousePos.x; pos->y = MousePos.y; }
    void SetCursorPos(int x, int y) { MousePos.x = x; MousePos.y = y; }

	// This uses gluLookAt() to tell OpenGL where to look
    void Look();	

	// This changes the position, view, and up vector of the camera.
	// This is primarily used for initialization
	void PositionCamera(float positionX, float positionY, float positionZ,
			 		    float viewX,     float viewY,     float viewZ,
						float upVectorX, float upVectorY, float upVectorZ);

	// This rotates the camera's view around the position using axis-angle rotation
	void RotateView(float angle, float X, float Y, float Z);

	// This rotates the camera around a point (I.E. your character).
	// vCenter is the point that we want to rotate the position around.
	// Like the other rotate function, the x, y and z is the axis to rotate around.
	void RotateAroundPoint(Vector3 vCenter, float angle, float x, float y, float z);

	// This will move the camera forward or backward depending on the speed
	void MoveCamera(float speed);

    // This updates the camera's view and other data (Should be called each frame)
	void Update();
};
