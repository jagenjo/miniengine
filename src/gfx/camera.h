#ifndef CAMERA_H
#define CAMERA_H

#include "../utils/math.h"
#include "../includes.h"

enum { CAM_GLOBAL, CAM_BACK, CAM_SIDE, CAM_FRONT, CAM_TOP, CAM_INSIDE, CAM_ALL };

class Camera
{
public:
	enum { PERSPECTIVE, ORTHOGRAPHIC };

	static int window_width;
	static int window_height;

	char type; //camera type

	//vectors to define the orientation of the camera
	Vector3 eye;
	Vector3 center;
	Vector3 up;

	//properties of the projection of the camera
	float fov;			//view angle
	float tan_fov;		//tangent of the view angle 
	float aspect;		//aspect ratio
	float near_plane;	//near plane
	float far_plane;	//far plane

	//for orthogonal projection
	float left,right,top,bottom;

	//matrices
	Matrix44 view_matrix;
	Matrix44 projection_matrix;
	Matrix44 view_projection_matrix;

	//frustrum culling
	enum { OUTSIDE, OVERLAP, INSIDE };

	float frustum[6][4];

	void extractFrustum();
	char pointInFrustum(float x, float y, float z);
	char sphereInFrustum(float x, float y, float z, float radius);
	char AABBInFrustrum(const Vector3 &mins, const Vector3 &maxs);

	Camera();
	void set();

	//move in camera space
	void move(Vector3 delta);

	void orbit( const Vector3& center, float angle, const Vector3& axis );
	void rotate(float angle, const Vector3& axis);

	Vector3 getLocalVector(const Vector3& v);

	void setPerspective(float fov, float aspect, float near_plane, float far_plane);
	void setOrthographic(float left, float right, float top, float bottom, float near_plane, float far_plane);
	void lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);

	Vector3 project2D(Vector3 pos, float screen_width, float screen_height);

	void updateViewMatrix();
	void updateProjectionMatrix();
	void updateViewProjectionMatrix() { view_projection_matrix = view_matrix * projection_matrix; }

	std::string toString();
};


#endif