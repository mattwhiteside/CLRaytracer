//
//  types.h
//  CLRaytracer
//
//  Created by Matt Whiteside on 7/7/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//

#ifndef CLRaytracer_types_h
#define CLRaytracer_types_h


#include <OpenCL/OpenCL.h>

typedef enum{
  SPHERE_T,
  TRIANGLE_T
} Geometry;



typedef struct{
  cl_float3 origin, color;
  cl_bool isDirectional;
} Light;



typedef struct {
  
  Geometry geometry;
  cl_float16 transform;
  cl_float16 invTransform;
  cl_float3 ambient, diffuse, emissive, specular;
  cl_float shininess;
  cl_float privateData[9];
  
} Object;//16



typedef struct {
  cl_float3 position, lookAt, upVector;
  cl_float3 u,v,w;//u,v,w are the unit frame vectors;
  cl_float fovy, fovx;
  //  Real fovx(Real pWidth, Real pHeight) const{
  //    return 2*atan(tanf(fovy/2)*(pWidth/pHeight));
  //  }
  
  //  mat3 frame() const {
  //    const vec3 w = glm::normalize(lookFrom - lookAt);
  //    const vec3 u = glm::normalize(glm::cross(upVector, w));
  //    const vec3 v = glm::cross(w, u);
  //    return mat3(u,v,w);
  //  }
} Camera;



union _Matrix3
{
  struct
  {
    cl_float m00, m01, m02;
    cl_float m10, m11, m12;
    cl_float m20, m21, m22;
  };
  cl_float m[9];
};
typedef union _Matrix3 Matrix3;



#endif
