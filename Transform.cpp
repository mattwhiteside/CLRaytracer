// Transform.cpp: implementation of the Transform class.

// Note: when you construct a matrix using mat4() or mat3(), it will be COLUMN-MAJOR
// Keep this in mind in readfile.cpp and display.cpp
// See FAQ for more details or if you're having problems.

#include "Transform.h"

// Helper rotation function.  Please implement this.  
mat3 Transform::rotate(const Real degrees, const vec3& axis) 
{
    vec3 v = glm::normalize(axis);
    double radians = glm::radians(degrees);
    Real _cos = cos(radians);
    Real cosp = 1.0f - _cos;
    Real _sin = sin(radians);
    
    mat3 ret(_cos + cosp * v[0] * v[0],
             cosp * v[0] * v[1] + v[2] * _sin,
             cosp * v[0] * v[2] - v[1] * _sin,
             
             cosp * v[0] * v[1] - v[2] * _sin,
             _cos + cosp * v[1] * v[1],
             cosp * v[1] * v[2] + v[0] * _sin,
             
             cosp * v[0] * v[2] + v[1] * _sin,
             cosp * v[1] * v[2] - v[0] * _sin,
             _cos + cosp * v[2] * v[2]);
    
    return ret;
}

// Transforms the camera left around the "crystal ball" interface
void Transform::left(Real degrees, vec3& eye, vec3& up) {
    //vec3 axis = glm::cross(up, eye);
    const mat3 rotor = Transform::rotate(degrees, up);
    eye = rotor * eye;
    up = rotor * up;
}

// Transforms the camera up around the "crystal ball" interface
void Transform::up(Real degrees, vec3& eye, vec3& up) {
    //vec3 left = glm::cross(up, eye);
    vec3 axis = glm::cross(eye,up);
    const mat3 rotor = Transform::rotate(degrees, axis);
    eye = rotor * eye;
    up = rotor * up;
}

mat4 Transform::lookAt(const vec3 &eye, const vec3 &center, const vec3 &up) 
{
    
    vec3 f = glm::normalize(center - eye);
    vec3 _up = glm::normalize(up);
    vec3 s = glm::cross(f, _up);
    vec3 u = glm::cross(s, f);
    
    
    mat4 ret(
             s[0], s[1], s[2], 0.0f,
             u[0], u[1], u[2], 0.0f,
             -f[0], -f[1], -f[2], 0.0f,
             0,0,0, 1
             );
    
    mat4 translation(
                     1,0,0,-eye.x,
                     0,1,0,-eye.y,
                     0,0,1,-eye.z,
                     0,0,0,1
                     );
    
    return glm::transpose(translation * ret);
}

mat4 Transform::perspective(Real fovy, Real aspect, Real zNear, Real zFar)
{
    Real fovyRadians = glm::radians(fovy);
    Real cotan = 1.0f / tanf(fovyRadians / 2.0f);
    
    mat4 ret( cotan / aspect, 0.0f, 0.0f, 0.0f,
              0.0f, cotan, 0.0f, 0.0f,
              0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), -1.0f,
              0.0f, 0.0f, (2.0f * zFar * zNear) / (zNear - zFar), 0.0f );

    return ret;
}

mat4 Transform::scale(const Real &sx, const Real &sy, const Real &sz) 
{
    mat4 ret(sx,0,0,0,
             0,sy,0,0,
             0,0,sz,0,
             0,0,0,1);
    return glm::transpose(ret);
}

mat4 Transform::translate(const Real &tx, const Real &ty, const Real &tz) 
{
    mat4 ret(1,0,0,tx,
             0,1,0,ty,
             0,0,1,tz,
             0,0,0,1);
    return glm::transpose(ret);
}

// To normalize the up direction and construct a coordinate frame.  
// As discussed in the lecture.  May be relevant to create a properly 
// orthogonal and normalized up. 
// This function is provided as a helper, in case you want to use it. 
// Using this function (in readfile.cpp or display.cpp) is optional.  

vec3 Transform::upvector(const vec3 &up, const vec3 & zvec) 
{
  vec3 x = glm::cross(up,zvec); 
  vec3 y = glm::cross(zvec,x); 
  vec3 ret = glm::normalize(y); 
  return ret;
}


Transform::Transform()
{

}

Transform::~Transform()
{

}
