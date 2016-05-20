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

	//for fixed pipeline stuff
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( view_matrix.m );

	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( projection_matrix.m );

	glMatrixMode( GL_MODELVIEW );

	extractFrustum();
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
	if (type == PERSPECTIVE)
		view_matrix.lookAt(eye, center, up);
	else
		view_matrix.setIdentity();

	view_projection_matrix = view_matrix * projection_matrix;

	extractFrustum();
}

// ******************************************

//Create a projection matrix
void Camera::updateProjectionMatrix()
{
	if (type == ORTHOGRAPHIC)
		projection_matrix.ortho(left, right, bottom, top, near_plane, far_plane);
	else
		projection_matrix.perspective(fov, aspect, near_plane, far_plane);

	view_projection_matrix = view_matrix * projection_matrix;

	extractFrustum();
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

void Camera::extractFrustum()
{
	float* proj = projection_matrix.m;
	float* modl = view_matrix.m;;
	float  clip[16];
	float   t;

	/* Combine the two matrices (multiply projection by modelview) */
	clip[0] = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
	clip[1] = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
	clip[2] = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
	clip[3] = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];

	clip[4] = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
	clip[5] = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
	clip[6] = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
	clip[7] = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];

	clip[8] = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
	clip[9] = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
	clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];

	clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];

	/* Extract the numbers for the RIGHT plane */
	frustum[0][0] = clip[3] - clip[0];
	frustum[0][1] = clip[7] - clip[4];
	frustum[0][2] = clip[11] - clip[8];
	frustum[0][3] = clip[15] - clip[12];

	/* Normalize the result */
	t = sqrt(frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2]);
	frustum[0][0] /= t;
	frustum[0][1] /= t;
	frustum[0][2] /= t;
	frustum[0][3] /= t;

	/* Extract the numbers for the LEFT plane */
	frustum[1][0] = clip[3] + clip[0];
	frustum[1][1] = clip[7] + clip[4];
	frustum[1][2] = clip[11] + clip[8];
	frustum[1][3] = clip[15] + clip[12];

	/* Normalize the result */
	t = sqrt(frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2]);
	frustum[1][0] /= t;
	frustum[1][1] /= t;
	frustum[1][2] /= t;
	frustum[1][3] /= t;

	/* Extract the BOTTOM plane */
	frustum[2][0] = clip[3] + clip[1];
	frustum[2][1] = clip[7] + clip[5];
	frustum[2][2] = clip[11] + clip[9];
	frustum[2][3] = clip[15] + clip[13];

	/* Normalize the result */
	t = sqrt(frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2]);
	frustum[2][0] /= t;
	frustum[2][1] /= t;
	frustum[2][2] /= t;
	frustum[2][3] /= t;

	/* Extract the TOP plane */
	frustum[3][0] = clip[3] - clip[1];
	frustum[3][1] = clip[7] - clip[5];
	frustum[3][2] = clip[11] - clip[9];
	frustum[3][3] = clip[15] - clip[13];

	/* Normalize the result */
	t = sqrt(frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2]);
	frustum[3][0] /= t;
	frustum[3][1] /= t;
	frustum[3][2] /= t;
	frustum[3][3] /= t;

	/* Extract the FAR plane */
	frustum[4][0] = clip[3] - clip[2];
	frustum[4][1] = clip[7] - clip[6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];

	/* Normalize the result */
	t = sqrt(frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2]);
	frustum[4][0] /= t;
	frustum[4][1] /= t;
	frustum[4][2] /= t;
	frustum[4][3] /= t;

	/* Extract the NEAR plane */
	frustum[5][0] = clip[3] + clip[2];
	frustum[5][1] = clip[7] + clip[6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];

	/* Normalize the result */
	t = sqrt(frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2]);
	frustum[5][0] /= t;
	frustum[5][1] /= t;
	frustum[5][2] /= t;
	frustum[5][3] /= t;
}

char Camera::pointInFrustum(float x, float y, float z)
{
	int p;

	for (p = 0; p < 6; p++)
		if (frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0)
			return OUTSIDE;
	return INSIDE;
}



char Camera::sphereInFrustum(float x, float y, float z, float radius)
{
	int p;

	for (p = 0; p < 6; p++)
	{
		float f = frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3];
		if (f < -radius)
			return OUTSIDE;
		if (f >= -radius && f <= +radius)
			return OVERLAP;
	}
	return INSIDE;
}

char Camera::AABBInFrustrum(const Vector3 &mins, const Vector3 &maxs)
{
	char ret = INSIDE;
	Vector3 vmin, vmax;

	for (int i = 0; i < 6; ++i)
	{
		// X axis
		if (frustum[i][0] > 0)
		{
			vmin.x = mins.x;
			vmax.x = maxs.x;
		}
		else
		{
			vmin.x = maxs.x;
			vmax.x = mins.x;
		}

		// Y axis
		if (frustum[i][1] > 0)
		{
			vmin.y = mins.y;
			vmax.y = maxs.y;
		}
		else
		{
			vmin.y = maxs.y;
			vmax.y = mins.y;
		}

		// Z axis
		if (frustum[i][2] > 0)
		{
			vmin.z = mins.z;
			vmax.z = maxs.z;
		}
		else
		{
			vmin.z = maxs.z;
			vmax.z = mins.z;
		}

		if (Vector3(frustum[i]).dot(vmin) > frustum[i][3])
			return OUTSIDE;

		if (Vector3(frustum[i]).dot(vmax) >= frustum[i][3])
			ret = OVERLAP;
	}

	return ret;
}