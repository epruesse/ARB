//c++ header files

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cmath>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

// openGL header files

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glpng.h>

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

