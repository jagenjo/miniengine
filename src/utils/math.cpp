#include "math.h"

#include "../includes.h"
#include <cassert>
#include <cmath> //for sqrt (square root) function
#include <math.h> //atan2



#define M_PI_2 1.57079632679489661923

//**************************************
float Vector2::distance(const Vector2& v)
{
	return (v - *this).length();
}

float Vector2::dot( const Vector2& v )
{
	return x * v.x + y * v.y;
}

float Vector2::perpdot( const Vector2& v )
{
	return y * v.x + -x * v.y;
}

void Vector2::random(float range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	y = (rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
}


Vector2 operator * (const Vector2& a, float v) { return Vector2(a.x * v, a.y * v); }
Vector2 operator + (const Vector2& a, const Vector2& b) { return Vector2(a.x + b.x, a.y + b.y); }
Vector2 operator - (const Vector2& a, const Vector2& b) { return Vector2(a.x - b.x, a.y - b.y); }

void Vector2::parseFromText(const char* text)
{
	int pos = 0;
	char num[255];
	const char* start = text;
	const char* current = text;

	while(1)
	{
		if (*current == ',' || (*current == '\0' && current != text) )
		{
			strncpy(num, start, current - start);
			num[current - start] = '\0';
			start = current + 1;
			switch(pos)
			{
				case 0: x = (float)atof(num); break;
				case 1: y = (float)atof(num); break;
				default: return; break;
			}
			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
}


// **************************************
const Vector3 Vector3::zero(0.0,0.0,0.0);

double Vector3::length() 
{
	return sqrt(x*x + y*y + z*z);
}

double Vector3::length() const
{
	return sqrt(x*x + y*y + z*z);
}

double Vector3::length2() const
{
	return x*x + y*y + z*z;
}

Vector3& Vector3::normalize()
{
	double len = length();
	x /= len;
	y /= len;
	z /= len;
	return *this;
}

float Vector3::distance(const Vector3& v) const
{
	return (v - *this).length();
}

Vector3 Vector3::cross( const Vector3& b ) const
{
	return Vector3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
}

float Vector3::dot( const Vector3& v ) const
{
	return x*v.x + y*v.y + z*v.z;
}

Vector3& Vector3::random(float range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	y = (rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	z = (rand() / (double)RAND_MAX) * 2 * range - range; //value between -range and range
	return *this;
}

Vector3& Vector3::random(Vector3 range)
{
	//rand returns a value between 0 and RAND_MAX
	x = (rand() / (double)RAND_MAX) * 2 * range.x - range.x; //value between -range and range
	y = (rand() / (double)RAND_MAX) * 2 * range.y - range.y; //value between -range and range
	z = (rand() / (double)RAND_MAX) * 2 * range.z - range.z; //value between -range and range
	return *this;
}

void Vector3::operator += (const Vector3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}


bool Vector3::operator == (const Vector3& v)
{
	return x == v.x && y == v.y && z == v.z;
}

void Vector3::setMin(Vector3 v)
{
	if (v.x < x) x = v.x;
	if (v.y < y) y = v.y;
	if (v.z < z) z = v.z;
}

void Vector3::setMax(Vector3 v)
{
	if (v.x > x) x = v.x;
	if (v.y > y) y = v.y;
	if (v.z > z) z = v.z;
}

void Vector3::parseFromText(const char* text, const char separator)
{
	int pos = 0;
	char num[255];
	const char* start = text;
	const char* current = text;

	while(1)
	{
		if (*current == separator || (*current == '\0' && current != text) )
		{
			strncpy(num, start, current - start);
			num[current - start] = '\0';
			start = current + 1;
			if (num[0] != 'x') //¿?
				switch(pos)
				{
					case 0: x = (float)atof(num); break;
					case 1: y = (float)atof(num); break;
					case 2: z = (float)atof(num); break;
					default: return; break;
				}

			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
};


//*********************************
Matrix44::Matrix44()
{
	setIdentity();
}

void Matrix44::set()
{
	glMatrixMode( GL_MODELVIEW );
	glMultMatrixf(m);
}

void Matrix44::clear()
{
	memset(m, 0, 16*sizeof(float));
}

void Matrix44::setIdentity()
{
	m[0]=1; m[4]=0; m[8]=0; m[12]=0;
	m[1]=0; m[5]=1; m[9]=0; m[13]=0;
	m[2]=0; m[6]=0; m[10]=1; m[14]=0;
	m[3]=0; m[7]=0; m[11]=0; m[15]=1;
}

void Matrix44::transpose()
{
   std::swap(m[1],m[4]); std::swap(m[2],m[8]); std::swap(m[3],m[12]);
   std::swap(m[6],m[9]); std::swap(m[7],m[13]); std::swap(m[11],m[14]);
}

void Matrix44::translate(float x, float y, float z)
{
	Matrix44 T;
	T.setTranslation(x, y, z);
	*this = *this * T;
}

void Matrix44::rotate( float angle_in_rad, const Vector3& axis )
{
	Matrix44 R;
	R.setRotation(angle_in_rad, axis);
	*this = *this * R;
}

Vector3 Matrix44::rotateVector(const Vector3& v) const
{
	Matrix44 temp = *this;
	temp.m[12] = 0.0;
	temp.m[13] = 0.0;
	temp.m[14] = 0.0;
	return temp * v;
}

void Matrix44::translateLocal(float x, float y, float z)
{
	Matrix44 T;
	T.setTranslation(x, y, z);
	*this = T * *this;
}

void Matrix44::rotateLocal( float angle_in_rad, const Vector3& axis )
{
	Matrix44 R;
	R.setRotation(angle_in_rad, axis);
	*this = R * *this;
}

void Matrix44::scaleLocal( const Vector3& scale )
{
	Matrix44 S;
	S.setScale(scale);
	*this = S * *this;
}

//To create a translation matrix
Matrix44& Matrix44::setTranslation(const Vector3& v)
{
	setIdentity();
	m[12] = v.x;
	m[13] = v.y;
	m[14] = v.z;
	return *this;
}

Matrix44& Matrix44::setTranslation(float x, float y, float z)
{
	setIdentity();
	m[12] = x;
	m[13] = y;
	m[14] = z;
	return *this;
}

void Matrix44::removeTranslation()
{
	m[12] = m[13] = m[14] = 0.0;
}


Vector3 Matrix44::getTranslation() const
{
	return Vector3( m[12],m[13],m[14]);
}

//To create a rotation matrix
Matrix44& Matrix44::setRotation( float angle_in_rad, const Vector3& axis  )
{
	clear();
	Vector3 axis_n = axis;
	axis_n.normalize();

	float c = cos( angle_in_rad );
	float s = sin( angle_in_rad );
	float t = 1 - c;

	M[0][0] = t * axis.x * axis.x + c;
	M[0][1] = t * axis.x * axis.y - s * axis.z;
	M[0][2] = t * axis.x * axis.z + s * axis.y;

	M[1][0] = t * axis.x * axis.y + s * axis.z;
	M[1][1] = t * axis.y * axis.y + c;
	M[1][2] = t * axis.y * axis.z - s * axis.x;

	M[2][0] = t * axis.x * axis.z - s * axis.y;
	M[2][1] = t * axis.y * axis.z + s * axis.x;
	M[2][2] = t * axis.z * axis.z + c;

	M[3][3] = 1.0f;
	return *this;
}

void Matrix44::setScale(const Vector3& scale)
{
	setIdentity();
	m[0] = scale.x;
	m[5] = scale.y;
	m[10] = scale.z;
}

bool Matrix44::getXYZ(float* euler) const
{
// Code adapted from www.geometrictools.com
//	Matrix3<Real>::EulerResult Matrix3<Real>::ToEulerAnglesXYZ 
    // +-           -+   +-                                        -+
    // | r00 r01 r02 |   |  cy*cz           -cy*sz            sy    |
    // | r10 r11 r12 | = |  cz*sx*sy+cx*sz   cx*cz-sx*sy*sz  -cy*sx |
    // | r20 r21 r22 |   | -cx*cz*sy+sx*sz   cz*sx+cx*sy*sz   cx*cy |
    // +-           -+   +-                                        -+
    if (_13 < 1.0f)
    {
        if (_13 > - 1.0f)
        {
            // y_angle = asin(r02)
            // x_angle = atan2(-r12,r22)
            // z_angle = atan2(-r01,r00)
            euler[1] = asinf(_13);
            euler[0] = atan2f(-_23,_33);
            euler[2] = atan2f(-_12,_11);
            return true;
        }
        else
        {
            // y_angle = -pi/2
            // z_angle - x_angle = atan2(r10,r11)
            // WARNING.  The solution is not unique.  Choosing z_angle = 0.
            euler[1] = (float)-M_PI_2;
            euler[0] = -atan2f(_21,_22);
            euler[2] = 0.0f;
            return false;
        }
    }
    else
    {
        // y_angle = +pi/2
        // z_angle + x_angle = atan2(r10,r11)
        // WARNING.  The solutions is not unique.  Choosing z_angle = 0.
        euler[1] = (float)M_PI_2;
        euler[0] = atan2f(_21,_22);
        euler[2] = 0.0f;
    }
	return false;
}

void Matrix44::copyOrientationTo( Matrix44& mat ) const
{
	memcpy( &mat.m[0], &m[0], sizeof(float) * 3);
	memcpy( &mat.m[4], &m[4], sizeof(float) * 3);
	memcpy( &mat.m[8], &m[8], sizeof(float) * 3);
}

//Multiply a matrix by another and returns the result
Matrix44 Matrix44::operator*(const Matrix44& matrix) const
{
	Matrix44 ret;

	unsigned int i,j,k;
	for (i=0;i<4;i++) 	
	{
		for (j=0;j<4;j++) 
		{
			ret.M[i][j]=0.0;
			for (k=0;k<4;k++) 
				ret.M[i][j] += M[i][k] * matrix.M[k][j];
		}
	}

	return ret;
}


Vector3 Matrix44::project2D(const Vector3& v)
{
   float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12]; 
   float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13]; 
   float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
   float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
   return Vector3(x / w,y / w,z / w);
}

Matrix44 Matrix44::getRotationMatrix() const
{
	Matrix44 tmp = *this;
	tmp.m[12] = 0.0;
	tmp.m[13] = 0.0;
	tmp.m[14] = 0.0;
	return tmp;
}

//it allows to add two vectors
Vector3 operator + (const Vector3& a, const Vector3& b) 
{
	return Vector3(a.x + b.x, a.y + b.y, a.z + b.z );
}

//it allows to add two vectors
Vector3 operator - (const Vector3& a, const Vector3& b) 
{
	return Vector3(a.x - b.x, a.y - b.y, a.z - b.z );
}

Vector3 operator * (const Vector3& a, float v) 
{
	return Vector3(a.x * v, a.y * v, a.z * v);
}


//Multiplies a vector by a matrix and returns the new vector
Vector3 operator * (const Matrix44& matrix, const Vector3& v) 
{   
   float x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12]; 
   float y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13]; 
   float z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14];
   return Vector3(x,y,z);
}

void Matrix44::setUpAndOrthonormalize(Vector3 up)
{
	up.normalize();

	//put the up vector in the matrix
	m[4] = up.x;
	m[5] = up.y;
	m[6] = up.z;

	//orthonormalize
	Vector3 right,front;
	right = rightVector();

	if ( abs(right.dot( up )) < 0.99998 )
	{
		right = up.cross( frontVector() );
		front = right.cross( up );
	}
	else
	{
		front = Vector3(right).cross( up );
		right = up.cross( front );
	}

	right.normalize();
	front.normalize();

	m[8] = front.x;
	m[9] = front.y;
	m[10] = front.z;

	m[0] = right.x;
	m[1] = right.y;
	m[2] = right.z;
}

void Matrix44::setFrontAndOrthonormalize(Vector3 front)
{
	front.normalize();

	//put the up vector in the matrix
	m[8] = front.x;
	m[9] = front.y;
	m[10] = front.z;

	//orthonormalize
	Vector3 right,up;
	right = rightVector();

	if ( abs(right.dot( front )) < 0.99998 )
	{
		right = topVector().cross( front  );
		up = front.cross( right );
	}
	else
	{
		up = front.cross( right );
		right = up.cross( front );
	}

	right.normalize();
	up.normalize();

	m[4] = up.x;
	m[5] = up.y;
	m[6] = up.z;

	m[0] = right.x;
	m[1] = right.y;
	m[2] = right.z;
	
}

bool Matrix44::inverse()
{
   unsigned int i, j, k, swap;
   float t;
   Matrix44 temp, final;
   final.setIdentity();

   temp = (*this);

   unsigned int m,n;
   m = n = 4;
	
   for (i = 0; i < m; i++)
   {
      // Look for largest element in column

      swap = i;
      for (j = i + 1; j < m; j++)// m or n
	  {
		 if ( fabs(temp.M[j][i]) > fabs( temp.M[swap][i]) )
            swap = j;
	  }
   
      if (swap != i)
      {
         // Swap rows.
         for (k = 0; k < n; k++)
         {
			 std::swap( temp.M[i][k],temp.M[swap][k]);
			 std::swap( final.M[i][k], final.M[swap][k]);
         }
      }

      // No non-zero pivot.  The CMatrix is singular, which shouldn't
      // happen.  This means the user gave us a bad CMatrix.


#define MATRIX_SINGULAR_THRESHOLD 0.00001 //change this if you experience problems with matrices

      if ( fabsf(temp.M[i][i]) <= MATRIX_SINGULAR_THRESHOLD)
	  {
		  final.setIdentity();
         return false;
	  }
#undef MATRIX_SINGULAR_THRESHOLD

      t = 1.0f/temp.M[i][i];

      for (k = 0; k < n; k++)//m or n
      {
         temp.M[i][k] *= t;
         final.M[i][k] *= t;
      }

      for (j = 0; j < m; j++) // m or n
      {
         if (j != i)
         {
            t = temp.M[j][i];
            for (k = 0; k < n; k++)//m or n
            {
               temp.M[j][k] -= (temp.M[i][k] * t);
               final.M[j][k] -= (final.M[i][k] * t);
            }
         }
      }
   }

   *this = final;

   return true;
}

void Matrix44::setOrthoProjection(float ortho_left, float ortho_right, float ortho_bottom, float ortho_top, float z_near, float z_far)
{
	const float width = ortho_right - ortho_left;
	const float height = ortho_top - ortho_bottom;
	const float depth = z_far - z_near;
	Vector3 translation;
	
	memset(m, 0, 16*sizeof(float)); //clear

	_11 = 2.0f / width;
	_22 = 2.0f / height;
	_33 = -2.0f / depth;

	translation.set(
		-(ortho_right + ortho_left)/width,
		-(ortho_top + ortho_bottom)/height,
		-(z_far + z_near)/depth
		);
	setTranslation(translation);
	_44 = 1;
}

void Matrix44::setProjection(float fov, float aspect, float near_plane, float far_plane)
{
	float cotg = 1.0f/tanf( fov * DEG2RAD * 0.5f );
	float zs = near_plane - far_plane;
	if (fabsf(zs) < 0.00001f)
	{
		assert(0 && "compute projection matrix, division by zero"); //division by zero
		memset(m, 0, 16*sizeof(float)); //clear
	}

	zs = 1.0f/zs;
	memset(m, 0, 16*sizeof(float)); //clear
	_11 = cotg / aspect;
	_22 = cotg;
	_33 = (far_plane+near_plane)*zs;
	_43 = (2*far_plane*near_plane)*zs;
	_34 = -1;
}

float ComputeSignedAngle( Vector2 a, Vector2 b)
{
	a.normalize();
	b.normalize();
	return atan2(a.perpdot(b), a.dot(b) );
}

Vector3 RayPlaneCollision( const Vector3& plane_pos, const Vector3& plane_normal, const Vector3& ray_origin, const Vector3& ray_dir )
{
    double D = plane_pos.dot(plane_normal);
    double numer = plane_normal.dot(ray_origin) + D;
    double denom = plane_normal.dot(ray_dir);
    double t = -(numer / denom);
	return ray_origin + ray_dir * t;
}

Vector3 normalize( Vector3 n )
{
	return n.normalize();
}

bool quadraticFormula
(
	const float a,
	const float b,
	const float c,
	float& r1, //first
	float& r2 //and second roots
)
{
	const float q = b*b - 4*a*c; 
	if( q >= 0.0f )
	{
		const float sq = sqrt(q);
		const float d = 1 / (2*a);
		r1 = ( -b + sq ) * d;
		r2 = ( -b - sq ) * d;
		return true;//real roots
	}
	else
	{
		return false;//complex roots
	}
}

int raySphereTest(Vector3 start, Vector3 dir, const Vector3& sphereCenter, float r, float& t0, float& t1)
{
	// sphere equation (centered at origin) x2+y2+z2=r2
	// ray equation x(t) = p0 + t*dir
	// substitute x(t) into sphere equation
	// solution below:

	// transform ray origin into sphere local coordinates
	Vector3 orig = start - sphereCenter;
	float a = dir.x*dir.x + dir.y*dir.y + dir.z*dir.z;
	float b = 2*orig.x*dir.x + 2*orig.y*dir.y + 2*orig.z*dir.z;
	float c = 
		orig.x*orig.x + orig.y*orig.y + orig.z*orig.z - 
		r*r;

	return quadraticFormula(a,b,c,t0,t1) ? 2 : 0;
}