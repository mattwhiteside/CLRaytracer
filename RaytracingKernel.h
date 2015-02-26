//
//  raytrace_kernel.h
//  CLRaytracer
//
//  Created by Matthew Whiteside on 9/22/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//

#ifndef CLRaytracer_raytrace_kernel_h
#define CLRaytracer_raytrace_kernel_h

#include <vector>
#include <stack>

#include "Transform.h"

#include "BaseCLKernel.h"
#include "types.h"
#include <sstream>
#include <fstream>


class RaytracingKernel : BaseCLKernel{
private:
  cl_mem kernelObjectData;
  cl_mem kernelLightData;
  cl_float attenuation[3] = {1,0,0};
  std::vector<Object> objects;
  Camera camera;
  std::vector<Light> lights;
  cl_float3 ambient, diffuse, specular, emission;
  cl_float shininess;
  unsigned maxdepth = 5;
  

  
  std::vector<cl_float3> vertices;

  void matransform (std::stack<mat4> &transfstack, Real * values) ;
  void rightmultiply (const mat4 &M, std::stack<mat4> &transfstack) ;
  bool readvals (std::stringstream &s, const int numvals, cl_float * values) ;
  void readfile (const char * filename, const float pWidth, const float pHeight) ;


public:
  RaytracingKernel(const string configFile, unsigned pWidth, unsigned pHeight);
  virtual int customRecompute();
  int init();
};

#endif
