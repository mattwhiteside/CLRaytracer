//
//  BaseCLKernel.h
//  CLRaytracer
//
//  Created by Matthew Whiteside on 9/23/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//

#ifndef __CLRaytracer__BaseCLKernel__
#define __CLRaytracer__BaseCLKernel__

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0


#include <iostream>
#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/CGLDevice.h>
#include <GLUT/glut.h>
#include <sys/types.h>

using namespace std;

class BaseCLKernel {
private:
  cl_command_queue                  computeCommands;

  cl_program                        computeProgram = 0;
  cl_device_id                      computeDeviceId;
  cl_device_type                    computeDeviceType;

  cl_mem                            computeImage = 0;

  size_t                            MaxWorkGroupSize = 0;
  unsigned long                     WorkGroupSize[2];
  int                               WorkGroupItems = 32;
  
  /////////////////////////////////////////////////////////////////////////
  
  
  
  uint textureId                   = 0;
  uint textureTarget               = GL_TEXTURE_2D;
  uint textureInternal             = GL_RGBA;
  uint textureFormat               = GL_RGBA;
  uint textureType                 = GL_UNSIGNED_BYTE;
  uint textureWidth;
  uint textureHeight;
  size_t textureTypeSize           = sizeof(char);
  uint activeTextureUnit           = GL_TEXTURE1_ARB;
  void* hostImageBuffer            = 0;
  
  static float TexCoords[4][2];


  static size_t DivideUp(size_t a, size_t b)
  {
    return ((a % b) != 0) ? (a / b + 1) : (a / b);
  }
  
  
  int setupComputeDevices(int gpu);
  
  int setupGraphics();
  void createTexture();
  
  //virtual const char* getKernelSource() = 0;
  
protected:
  bool update                      = true;
  virtual int customRecompute() = 0;
  cl_context                        computeContext;
  cl_kernel                         computeKernel = 0;
  cl_mem                            computeResult = 0;
  int setupComputeKernel(const char* kernelSrc);
  int createComputeResult();
  
public:
  static float VertexPos[4][2];

  BaseCLKernel(std::string sourceFilePath, unsigned pWidth, unsigned pHeight);
  const unsigned width;
  const unsigned height;
  void renderTexture();
  //virtual int init();
  int recompute();
  ~BaseCLKernel();
  
  
};

#endif /* defined(__CLRaytracer__BaseCLKernel__) */
