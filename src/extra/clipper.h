#ifndef _CLIPPER_
#define _CLIPPER_

#include <math.h>
//#include <GL/glut.h>
#include "../includes.h"
#include "../utils/math.h"

class Clipper
{
public:
	enum { OUTSIDE, OVERLAP, INSIDE };

	float frustum[6][4];

	void ExtractFrustum();
	bool PointInFrustum( float x, float y, float z );
	bool SphereInFrustum( float x, float y, float z, float radius );
	int AABBInFrustrum( const Vector3 &mins, const Vector3 &maxs );
};

#endif