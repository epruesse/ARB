#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define zoomFactor	1.0f     // scaling factor in z-axis (ZOOM)

#define TRUE        1
#define FALSE       0

#define SKELETON_SIZE  1.0f
#define ZOOM_FACTOR    0.0005f

#define AWAR_SAI_SELECTED "RNA3D/sai_selected"

struct Vector3 {
public:
    float x, y, z;						

	Vector3() {}  	// A default constructor
	Vector3(float X, float Y, float Z) { x = X; y = Y; z = Z; }

	// Overloading Operator(+,-,*,/) functions

    // adding 2 vectors    
	Vector3 operator+(Vector3 v) { return Vector3(v.x + x, v.y + y, v.z + z); }

    // substracting 2 vectors
	Vector3 operator-(Vector3 v) { return Vector3(x - v.x, y - v.y, z - v.z); }

	//multiply by scalars
	Vector3 operator*(float num) { return Vector3(x * num, y * num, z * num); }

    // divide by scalars
	Vector3 operator/(float num) { return Vector3(x / num, y / num, z / num); }
};

