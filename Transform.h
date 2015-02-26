// Transform header file to define the interface. 
// The class is all static for simplicity
// You need to implement left, up and lookAt
// Rotate is a helper function

// Include the helper glm library, including matrix transform extensions
#ifndef __MATTS_TRANSFORM
#define __MATTS_TRANSFORM

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// glm provides vector, matrix classes like glsl
// Typedefs to make code more readable 
//#define GLM_PRECISION_HIGHP_FLOAT
//typedef glm::highp_mat3 mat3 ;
//typedef glm::highp_mat4 mat4 ;
//typedef glm::highp_vec3 vec3 ;
//typedef glm::highp_vec4 vec4 ;
//typedef glm::highp_float_t Real;

typedef glm::mediump_mat3 mat3 ;
typedef glm::mediump_mat4 mat4 ;
typedef glm::mediump_vec3 vec3 ;
typedef glm::mediump_vec4 vec4 ;
typedef glm::mediump_float_t Real;

const double pi = 3.14159265 ; // For portability across platforms


class Transform  
{
public:
	Transform();
	virtual ~Transform();
	static void left(Real degrees, vec3& eye, vec3& up);
	static void up(Real degrees, vec3& eye, vec3& up);
	static mat4 lookAt(const vec3& eye, const vec3 &center, const vec3& up);
	static mat4 perspective(Real fovy, Real aspect, Real zNear, Real zFar);
        static mat3 rotate(const Real degrees, const vec3& axis) ;
        static mat4 scale(const Real &sx, const Real &sy, const Real &sz) ;
        static mat4 translate(const Real &tx, const Real &ty, const Real &tz);
        static vec3 upvector(const vec3 &up, const vec3 &zvec) ; 
};

#endif