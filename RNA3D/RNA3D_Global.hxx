#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum {
    CIRCLE,  
    RECTANGLE, 
    RECTANGLE_ROUND, 
    POLYGON,
    STAR,
    STAR_SMOOTH, 
    DIAMOND,
    CONE_UP,
    CONE_DOWN,
    FREEFORM_1,
    LETTER_A,
    LETTER_G,
    LETTER_C,
    LETTER_U,
    SHAPE_MAX
};

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

