/* 
	Framework developed by    Javi Agenjo 2010  
	Universitat Pompeu Fabra  javi.agenjo@gmail.com
*/

#ifndef MATH_INC //macros to ensure the code is included once
#define MATH_INC

#include <vector>
#include <cmath>

#define DEG2RAD 0.0174532925
#define RAD2DEG 57.2957795

#ifndef M_PI
	#define M_PI 3.14159265
#endif

#define MYRAND(x) ((rand() / (double)RAND_MAX) * 2 * x - x)

class Vector2
{
public:
	union
	{
		struct { float x,y; };
		float value[2];
	};

	Vector2() { x = y = 0.0f; }
	Vector2(float x, float y) { this->x = x; this->y = y; }

	double length() { return sqrt(x*x + y*y); }
	double length() const { return sqrt(x*x + y*y); }

	float dot( const Vector2& v );
	float perpdot( const Vector2& v );

	void set(float x, float y) { this->x = x; this->y = y; }

	Vector2& normalize() { *this *= (float)length(); return *this; }

	float distance(const Vector2& v);
	void random(float range);
	void parseFromText(const char* text);

	void operator *= (float v) { x*=v; y*=v; }
	void operator *= (const Vector2& v) { x*=v.x; y*=v.y; }
};

Vector2 operator * (const Vector2& a, float v);
Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator - (const Vector2& a, const Vector2& b);


class Vector3u
{
public:
	union
	{
		struct { unsigned int x;
				 unsigned int y;
				 unsigned int z; };
		unsigned int v[3];
	};
	Vector3u() { x = y = z = 0; }
	Vector3u(unsigned int x, unsigned int y, unsigned int z) { this->x = x; this->y = y; this->z = z; }
};

//*********************************

class Vector3
{
public:
	union
	{
		struct { float x,y,z; };
		float v[3];
	};

	Vector3() { x = y = z = 0.0f; }
	Vector3(float x, float y, float z) { this->x = x; this->y = y; this->z = z;	}
	Vector3(float* v) { x = v[0]; y = v[1]; z = v[2]; }

	double length();
	double length() const;
	double length2() const;

	Vector3& set(float x, float y, float z) { this->x = x; this->y = y; this->z = z; return *this; }

	Vector3& normalize();
	Vector3& random(float range);
	Vector3& random(Vector3 range);
	void setMin(Vector3 v);
	void setMax(Vector3 v);

	float distance(const Vector3& v) const;

	Vector3 cross( const Vector3& v ) const;
	float dot( const Vector3& v ) const;

	void operator += (const Vector3& v);
	bool operator == (const Vector3& v);
	Vector3 operator * (const Vector3& v) { return Vector3(x*v.x, y*v.y, z*v.z);}
	void operator *= (const Vector3& v) { x*=v.x; y*=v.y; z*=v.z; }

	void parseFromText(const char* text, const char separator = ',');

	static const Vector3 zero;
};

class Vector4
{
public:
	union
	{
		struct { float x,y,z,w; };
		struct { float r,g,b,a; };
		float v[4];
	};

	Vector4() { x = y = z = w = 0.0f; }
	void set(float x, float y, float z, float w = 0.0) { this->x = x; this->y = y; this->z = z; this->w = w; }
	void operator = (const Vector3& v) { x = v.x; y = v.y; z = v.z; } 
	Vector3 xyz() { return Vector3(x,y,z); }
};

//****************************
//Matrix44 class
class Matrix44
{
	public:

		//This matrix works in 
		union { //allows to access the same var using different ways
			struct
			{
				float        _11, _12, _13, _14;
				float        _21, _22, _23, _24;
				float        _31, _32, _33, _34;
				float        _41, _42, _43, _44;
			};
			float M[4][4]; //[row][column]
			float m[16];
		};

		Matrix44();
		Matrix44(const float* v);

		void set();
		void clear();
		void setIdentity();
		void transpose();

		Vector3 rightVector() { return Vector3(m[0],m[1],m[2]); }
		Vector3 topVector() { return Vector3(m[4],m[5],m[6]); }
		Vector3 frontVector() { return Vector3(m[8],m[9],m[10]); }

		bool inverse();
		void setUpAndOrthonormalize(Vector3 up);
		void setFrontAndOrthonormalize(Vector3 front);
		void setOrthoProjection(float ortho_left, float ortho_right, float ortho_bottom, float ortho_top, float z_near, float z_far);
		void setProjection(float fov, float aspect, float near_plane, float far_plane);

		void copyOrientationTo( Matrix44& mat ) const;

		//rotate only
		Vector3 rotateVector( const Vector3& v) const;

		//translate using world coordinates
		void translate(float x, float y, float z);
		void rotate( float angle_in_rad, const Vector3& axis  );
		void translateLocal(float x, float y, float z);
		void rotateLocal( float angle_in_rad, const Vector3& axis  );
		void scaleLocal( const Vector3& scale );

		//create a transformation matrix from scratch
		Matrix44& setTranslation(const Vector3& v);
		Matrix44& setTranslation(float x, float y, float z);
		Matrix44& setRotation( float angle_in_rad, const Vector3& axis );
		void setScale(const Vector3& scale);

		void removeTranslation();

		bool getXYZ(float* euler) const;
		Vector3 getTranslation() const;

		Vector3 project2D(const Vector3& position);
		Matrix44 getRotationMatrix() const; //returns the matrix without translation

		Matrix44 operator * (const Matrix44& matrix) const;

		//camera methods
};

class AABB
{
public:
	Vector3 center;
	Vector3 halfsize;
	AABB() {}
	AABB(Vector3 center, Vector3 halfsize) { this->center = center; this->halfsize = halfsize; };
};

//Operators, they are our friends
//Matrix44 operator * ( const Matrix44& a, const Matrix44& b );
Vector3 operator * (const Matrix44& matrix, const Vector3& v);
Vector3 operator + (const Vector3& a, const Vector3& b);
Vector3 operator - (const Vector3& a, const Vector3& b);
Vector3 operator * (const Vector3& a, float v);

float ComputeSignedAngle( Vector2 a, Vector2 b);
Vector3 RayPlaneCollision( const Vector3& plane_pos, const Vector3& plane_normal, const Vector3& ray_origin, const Vector3& ray_dir );
Vector3 normalize( Vector3 n );
int raySphereTest(Vector3 start, Vector3 dir, const Vector3& sphereCenter, float r, float& t0, float& t1);

#endif