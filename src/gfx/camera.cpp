#include "camera.h"

#include "../includes.h"
#include <iostream>
#include "../utils/utils.h"
#include <cassert>

int Camera::window_width = 800;
int Camera::window_height = 600;

Camera::Camera()
{
	view_matrix.setIdentity();
	eye.set(0,0,1);
	center.set(0,0,0);
	up.set(0,1,0);
	setOrthographic(-100,100,100,-100,-100,100);
}

void Camera::set()
{
	updateViewMatrix();
	updateProjectionMatrix();

	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( view_matrix.m );

	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( projection_matrix.m );

	glMatrixMode( GL_MODELVIEW );

	clipper.ExtractFrustum();
}

Vector3 Camera::getLocalVector(const Vector3& v)
{
	Matrix44 iV = view_matrix;
	if (iV.inverse() == false)
		std::cout << "Matrix Inverse error" << std::endl;
	Vector3 result = iV.rotateVector(v);
	return result;
}

void Camera::move(Vector3 delta)
{
	Vector3 localDelta = getLocalVector(delta);
	eye = eye - localDelta;
	center = center - localDelta;
	updateViewMatrix();
}

void Camera::orbit( const Vector3& center, float angle, const Vector3& axis )
{
	Matrix44 R;
	R.setRotation(angle,axis);
	Matrix44 T;
	T.setTranslation( center.x, center.y, center.z );
	Matrix44 M = T * R;
	T.setTranslation( -center.x, -center.y, -center.z );
	M = M * T;

	this->center = M * this->center;
	this->eye = M * this->eye;
	updateViewMatrix();
}

void Camera::rotate(float angle, const Vector3& axis)
{
	Matrix44 R;
	R.setRotation(angle,axis);
	Vector3 new_front = R * (center - eye);
	center = eye + new_front;
	updateViewMatrix();
}

void Camera::setOrthographic(float left, float right, float top, float bottom, float near_plane, float far_plane)
{
	type = ORTHOGRAPHIC;

	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	updateProjectionMatrix();
}

void Camera::setPerspective(float fov, float aspect, float near_plane, float far_plane)
{
	type = PERSPECTIVE;

	this->fov = fov;
	this->aspect = aspect;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	//update projection
	updateProjectionMatrix();
}

void Camera::lookAt(const Vector3& eye, const Vector3& center, const Vector3& up)
{
	this->eye = eye;
	this->center = center;
	this->up = up;

	updateViewMatrix();
}

void Camera::updateViewMatrix()
{
	//if (type != PERSPECTIVE) return;
	assert( up.length() > 0.0 );

	//We activate the matrix we want to work: modelview
	glMatrixMode(GL_MODELVIEW);

	//We set it as identity
	glLoadIdentity();
	
	//We find the look at matrix
	gluLookAt( eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);

	//We get the matrix and store it in our app
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix.m );

	assert( checkGLErrors() );

	updateViewProjectionMatrix();
}

// ******************************************

//Create a projection matrix
void Camera::updateProjectionMatrix()
{
	this->tan_fov = fabs( tan(fov * DEG2RAD) );

	//We activate the matrix we want to work: projection
	glMatrixMode(GL_PROJECTION);

	//We set it as identity
	glLoadIdentity();

	if (type == PERSPECTIVE)
		gluPerspective(fov, aspect, near_plane, far_plane);
	else
		glOrtho(left,right,bottom,top,near_plane,far_plane);

	//upload to hardware
	glGetFloatv(GL_PROJECTION_MATRIX, projection_matrix.m );

	glMatrixMode(GL_MODELVIEW);

	updateViewProjectionMatrix();
}

Vector3 Camera::project2D(Vector3 pos, float screen_width, float screen_height)
{
	Vector3 p = view_projection_matrix.project2D(pos);
	p = p * 0.5 + Vector3(0.5,0.5,0.0);
	return Vector3(p.x * screen_width, p.y * screen_height, p.z * 2.0);
}

std::string Camera::toString() 
{ 
	char temp[1024];
	sprintf (temp,"Camera: Eye:%f,%f,%f\n", eye.x, eye.y, eye.z);
	return temp;
}

