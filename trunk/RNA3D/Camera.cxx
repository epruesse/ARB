#include "GlobalHeader.hxx"
#include "Camera.hxx"
#include "Structure3D.hxx"

//=========================================================================
//   Camera class constructor
//=========================================================================

CCamera::CCamera()
{
	Vector3 vZero = Vector3(0.0, 0.0, 0.0);		// Init a vVector to 0 0 0 for camera position
	Vector3 vView = Vector3(0.0, 1.0, 0.5);		// Init a starting view vVector (looking up and out the screen) 
	Vector3 vUp   = Vector3(0.0, 0.0, 1.0);		// Init a standard up vVector (Rarely ever changes)

	m_vPosition	= vZero;					// Init the position to zero
	m_vView		= vView;					// Init the view to a std starting view
	m_vUpVector	= vUp;						// Init the UpVector
}

//=========================================================================
//	This returns a normalize vector (A vector exactly of length 1)
//=========================================================================
Vector3 Normalize(Vector3 vVector)
{
	// Equation:  magnitude = sqrt(V.x^2 + V.y^2 + V.z^2) : Where V is the vector
	float magnitude = sqrt( (vVector.x * vVector.x) + 
                            (vVector.y * vVector.y) + 
                            (vVector.z * vVector.z) );

	// Divide vector by the magnitude => vector a total length of 1.  
	vVector = vVector / magnitude;		
	
	// Return normalized vector
	return vVector;										
}

//=========================================================================
//	This function sets the camera's position and view and up vVector.
//=========================================================================

void CCamera::PositionCamera(float positionX, float positionY, float positionZ,
				  		     float viewX,     float viewY,     float viewZ,
							 float upVectorX, float upVectorY, float upVectorZ)
{
	Vector3 vPosition	= Vector3(positionX, positionY, positionZ);
	Vector3 vView		= Vector3(viewX, viewY, viewZ);
	Vector3 vUpVector	= Vector3(upVectorX, upVectorY, upVectorZ);

	m_vPosition = vPosition;					// Assign the position
	m_vView     = vView;						// Assign the view
	m_vUpVector = vUpVector;					// Assign the up vector
}

//=================================================================================
//	This updates the camera according to the camera position set in Init function
//=================================================================================

void CCamera::Look()
{
	// camera position, camera view, camera up vector
	gluLookAt(m_vPosition.x, m_vPosition.y, m_vPosition.z,	
			  m_vView.x,	 m_vView.y,     m_vView.z,	
			  m_vUpVector.x, m_vUpVector.y, m_vUpVector.z);
}

//=========================================================================
//	This rotates the view around the position using an axis-angle rotation
//=========================================================================

void CCamera::RotateView(float angle, float x, float y, float z)
{
	Vector3 vNewView;

	// Get the view vector (The direction we are facing)
	Vector3 vView = m_vView - m_vPosition;		

	// Calculate the sine and cosine of the angle once
	float cosTheta = (float)cos(angle);
	float sinTheta = (float)sin(angle);

	// Find the new x position for the new rotated point
	vNewView.x  = (cosTheta + (1 - cosTheta) * x * x)		* vView.x;
	vNewView.x += ((1 - cosTheta) * x * y - z * sinTheta)	* vView.y;
	vNewView.x += ((1 - cosTheta) * x * z + y * sinTheta)	* vView.z;

	// Find the new y position for the new rotated point
	vNewView.y  = ((1 - cosTheta) * x * y + z * sinTheta)	* vView.x;
	vNewView.y += (cosTheta + (1 - cosTheta) * y * y)		* vView.y;
	vNewView.y += ((1 - cosTheta) * y * z - x * sinTheta)	* vView.z;

	// Find the new z position for the new rotated point
	vNewView.z  = ((1 - cosTheta) * x * z - y * sinTheta)	* vView.x;
	vNewView.z += ((1 - cosTheta) * y * z + x * sinTheta)	* vView.y;
	vNewView.z += (cosTheta + (1 - cosTheta) * z * z)		* vView.z;

	// Now add the newly rotated vector to the position to set
	// the new rotated view of the camera.
	m_vView = m_vPosition + vNewView;
}

//=========================================================================
/////	This rotates the position around a given point
//=========================================================================

void CCamera::RotateAroundPoint(Vector3 vCenter, float angle, float x, float y, float z)
{
	Vector3 vNewPosition;			

	// To rotate our position around a point, what we need to do is find
	// a vector from our position to the center point we will be rotating around.
	// Once we get this vector, then we rotate it along the specified axis with
	// the specified degree.  Finally the new vector is added center point that we
	// rotated around (vCenter) to become our new position.  That's all it takes.

	// Get the vVector from our position to the center we are rotating around
	Vector3 vPos = m_vPosition - vCenter;

	// Calculate the sine and cosine of the angle once
	float cosTheta = (float)cos(angle);
	float sinTheta = (float)sin(angle);

	// Find the new x position for the new rotated point
	vNewPosition.x  = (cosTheta + (1 - cosTheta) * x * x)		* vPos.x;
	vNewPosition.x += ((1 - cosTheta) * x * y - z * sinTheta)	* vPos.y;
	vNewPosition.x += ((1 - cosTheta) * x * z + y * sinTheta)	* vPos.z;

	// Find the new y position for the new rotated point
	vNewPosition.y  = ((1 - cosTheta) * x * y + z * sinTheta)	* vPos.x;
	vNewPosition.y += (cosTheta + (1 - cosTheta) * y * y)		* vPos.y;
	vNewPosition.y += ((1 - cosTheta) * y * z - x * sinTheta)	* vPos.z;

	// Find the new z position for the new rotated point
	vNewPosition.z  = ((1 - cosTheta) * x * z - y * sinTheta)	* vPos.x;
	vNewPosition.z += ((1 - cosTheta) * y * z + x * sinTheta)	* vPos.y;
	vNewPosition.z += (cosTheta + (1 - cosTheta) * z * z)		* vPos.z;

	// Now we just add the newly rotated vector to our position to set
	// our new rotated position of our camera.
	m_vPosition = vCenter + vNewPosition;
}

//============================================================================
//	This will move the camera forward or backward depending on the speed
//============================================================================

void CCamera::MoveCamera(float speed)
{
	// Get our view vector (The direciton we are facing)
	Vector3 vVector = m_vView - m_vPosition;

	//  normalize the view vector when moving throughout the world. 
    vVector = Normalize(vVector);

    // Add acceleration to Position and View
    m_vPosition.x += vVector.x * speed;		
	m_vPosition.z += vVector.z * speed;		
	m_vView.x     += vVector.x * speed;			
	m_vView.z     += vVector.z * speed;			
}


