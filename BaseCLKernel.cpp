//
//  BaseCLKernel.cpp
//  CLRaytracer
//
//  Created by Matthew Whiteside on 9/23/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//
#include "BaseCLKernel.h"
#define USE_GL_ATTACHMENTS 1

float BaseCLKernel::VertexPos[4][2] = { { -1.0f, -1.0f },
  { +1.0f, -1.0f },
  { +1.0f, +1.0f },
  { -1.0f, +1.0f } };

//not really sure if TexCoords is needed anymore
float BaseCLKernel::TexCoords[4][2] = { { -1.0f, -1.0f },
  { +1.0f, -1.0f },
  { +1.0f, +1.0f },
  { -1.0f, +1.0f } };

BaseCLKernel::BaseCLKernel(std::string sourceFilePath, unsigned pWidth, unsigned pHeight) : height(pHeight), width(pWidth), textureWidth(pWidth), textureHeight(pHeight){
  
  int err;
  err = setupGraphics();
  if (err != GL_NO_ERROR)
  {
    printf ("Failed to setup OpenGL state!");
    exit (err);
  }
  int gpu = 1;
  err = setupComputeDevices(gpu);
  if(err != CL_SUCCESS)
  {
    printf ("Failed to connect to compute device! Error %d\n", err);
    exit (err);
  }
  
  cl_bool image_support;
  err = clGetDeviceInfo(computeDeviceId, CL_DEVICE_IMAGE_SUPPORT,
                        sizeof(image_support), &image_support, NULL);
  if (err != CL_SUCCESS) {
    printf("Unable to query device for image support");
    exit(err);
  }
  if (image_support == CL_FALSE) {
    printf("Qjulia requires images: Images not supported on this device.");
    //return CL_IMAGE_FORMAT_NOT_SUPPORTED;
  }
  
//  err = setupComputeKernel();
//  if (err != CL_SUCCESS)
//  {
//    printf ("Failed to setup compute kernel! Error %d\n", err);
//    exit (err);
//  }
//  

  

  
  
}

int BaseCLKernel::setupGraphics(){
  createTexture();
  
  glClearColor (0.0, 0.0, 0.0, 0.0);
  
  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  TexCoords[3][0] = 0.0f;
  TexCoords[3][1] = 0.0f;
  TexCoords[2][0] = width;
  TexCoords[2][1] = 0.0f;
  TexCoords[1][0] = width;
  TexCoords[1][1] = height;
  TexCoords[0][0] = 0.0f;
  TexCoords[0][1] = height;

  
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, VertexPos);
  glClientActiveTexture(GL_TEXTURE0);
  glTexCoordPointer(2, GL_FLOAT, 0, TexCoords);
  return GL_NO_ERROR;
}

void BaseCLKernel::createTexture(){
  if(textureId)
    glDeleteTextures(1, &textureId);
  textureId = 0;
  
  printf("Creating Texture %d x %d...\n", width, height);
  
  textureWidth = width;
  textureHeight = height;
  
  glActiveTextureARB(activeTextureUnit);
  glGenTextures(1, &textureId);
  glBindTexture(textureTarget, textureId);
  glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(textureTarget, 0, textureInternal, textureWidth, textureHeight, 0,
               textureFormat, textureType, 0);
  glBindTexture(textureTarget, 0);

}

void BaseCLKernel::renderTexture( )
{
  glDisable( GL_LIGHTING );
  
  glViewport( 0, 0, width, height );
  
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluOrtho2D( -1.0, 1.0, -1.0, 1.0 );
  
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  
  glMatrixMode( GL_TEXTURE );
  glLoadIdentity();
  
  glEnable( textureTarget );
  glBindTexture( textureTarget, textureId );
  
  if(hostImageBuffer)
    glTexSubImage2D(textureTarget, 0, 0, 0, textureWidth, textureHeight,
                    textureFormat, textureType, hostImageBuffer);
  
  glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
  glBegin( GL_QUADS );
  {
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f( 0.0f, 0.0f );
    glVertex3f( -1.0f, -1.0f, 0.0f );
    
    glTexCoord2f( 0.0f, 1.0f );
    glVertex3f( -1.0f, 1.0f, 0.0f );
    
    glTexCoord2f( 1.0f, 1.0f );
    glVertex3f( 1.0f, 1.0f, 0.0f );
    
    glTexCoord2f( 1.0f, 0.0f );
    glVertex3f( 1.0f, -1.0f, 0.0f );
  }
  glEnd();
  glBindTexture( textureTarget, 0 );
  glDisable( textureTarget );
}

int BaseCLKernel::recompute(){
  if(!computeKernel || !computeResult)
    return CL_SUCCESS;
  
  
  size_t global[2];
  size_t local[2];
  
  int err = 0;
  
  
  if(update)
  {
    update = false;
    
    err = customRecompute();
    
  }
  
  size_t size_x = WorkGroupSize[0];
  size_t size_y = WorkGroupSize[1];
  
  global[0] = DivideUp(textureWidth, size_x) * size_x;
  global[1] = DivideUp(textureHeight, size_y) * size_y;

  local[0] = size_x;
  local[1] = size_y;
  
#if (DEBUG_INFO)
  if(FrameCount <= 1)
    printf("Global[%4d %4d] Local[%4d %4d]\n",
           (int)global[0], (int)global[1],
           (int)local[0], (int)local[1]);
#endif
  
  err = clEnqueueNDRangeKernel(computeCommands, computeKernel, 2, NULL, global, local, 0, NULL, NULL);
  if (err)
  {
    std::cout << "Failed to enqueue kernel! " << err << std::endl;
    return err;
  }
  
#if (USE_GL_ATTACHMENTS)
  
  err = clEnqueueAcquireGLObjects(computeCommands, 1, &computeImage, 0, 0, 0);
  if (err != CL_SUCCESS)
  {
    printf("Failed to acquire GL object! %d\n", err);
    return EXIT_FAILURE;
  }
  
  size_t origin[] = { 0, 0, 0 };
  size_t region[] = { textureWidth, textureHeight, 1 };
  err = clEnqueueCopyBufferToImage(computeCommands, computeResult, computeImage,
                                   0, origin, region, 0, NULL, 0);
  
  if(err != CL_SUCCESS)
  {
    printf("Failed to copy buffer to image! %d\n", err);
    return EXIT_FAILURE;
  }
  
  err = clEnqueueReleaseGLObjects(computeCommands, 1, &computeImage, 0, 0, 0);
  if (err != CL_SUCCESS)
  {
    printf("Failed to release GL object! %d\n", err);
    return EXIT_FAILURE;
  }
  
#else
  
  err = clEnqueueReadBuffer( computeCommands, computeResult, CL_TRUE, 0, width * height * textureTypeSize * 4, hostImageBuffer, 0, NULL, NULL );
  if (err != CL_SUCCESS)
  {
    printf("Failed to read buffer! %d\n", err);
    return EXIT_FAILURE;
  }
  
#endif
  
  return CL_SUCCESS;
}

int BaseCLKernel::createComputeResult(){
  int err;
	//size_t returned_size;
  //ComputeDeviceType = gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU;

  
#if (USE_GL_ATTACHMENTS)
  
  if(computeImage)
    clReleaseMemObject(computeImage);
  computeImage = 0;
  
  printf("Allocating compute result image in device memory...\n");
  computeImage = clCreateFromGLTexture(computeContext, CL_MEM_WRITE_ONLY, textureTarget, 0, textureId, &err);
  if (!computeImage || err != CL_SUCCESS)
  {
    printf("Failed to create OpenGL texture reference! %d\n", err);
    return -1;
  }
  
#else
  
  if (hostImageBuffer)
    free(hostImageBuffer);
  
  printf("Allocating compute result image in host memory...\n");
  hostImageBuffer = malloc(textureWidth * textureHeight * textureTypeSize * 4);
  if(!hostImageBuffer)
  {
    printf("Failed to create host image buffer!\n");
    return -1;
  }
  
  memset(hostImageBuffer, 0, textureWidth * textureHeight * textureTypeSize * 4);
  
#endif
  
  if(computeResult)
    clReleaseMemObject(computeResult);
  computeResult = 0;
  
  computeResult = clCreateBuffer(computeContext, CL_MEM_WRITE_ONLY, textureTypeSize * 4 * textureWidth * textureHeight, NULL, NULL);
  if (!computeResult)
  {
    printf("Failed to create OpenCL array!\n");
    return -1;
  }
  
  return CL_SUCCESS;

}


int BaseCLKernel::setupComputeDevices(int gpu)
{
  int err;
	size_t returned_size;
  computeDeviceType = (gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU);
  
#if (USE_GL_ATTACHMENTS)
  
  //printf(SEPARATOR);
  printf("Using active OpenGL context...\n");
  
  CGLContextObj kCGLContext = CGLGetCurrentContext();
  CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
  
  cl_context_properties properties[] = {
    CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
    (cl_context_properties)kCGLShareGroup, 0
  };
  
  // Create a context from a CGL share group
  //
  computeContext = clCreateContext(properties, 0, 0, clLogMessagesToStdoutAPPLE, 0, 0);
  if (!computeContext)
  {
    printf("Error: Failed to create a compute context!\n");
    return EXIT_FAILURE;
  }
  
#else
  
  // Locate a compute device
  //
  cl_uint platformCount;
  cl_platform_id* platforms;
  cl_uint deviceCount;
  cl_device_id* devices;
  clGetPlatformIDs(0, NULL, &platformCount);
  platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
  clGetPlatformIDs(platformCount, platforms, NULL);
  
  clGetDeviceIDs(platforms[0], computeDeviceType, 0, NULL, &deviceCount);
  devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
  err = clGetDeviceIDs(platforms[0], computeDeviceType, deviceCount, &computeDeviceId, NULL);
  
  //    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, &deviceCount, &ComputeDeviceId, NULL);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to locate compute device!\n");
    return EXIT_FAILURE;
  }
  
  // Create a context containing the compute device(s)
  //
  computeContext = clCreateContext(0, deviceCount, &computeDeviceId, clLogMessagesToStdoutAPPLE, NULL, &err);
  if (!computeContext)
  {
    printf("Error: Failed to create a compute context!\n");
    return EXIT_FAILURE;
  }
  
#endif
  
  unsigned long device_count;
  cl_device_id device_ids[16];
  
  err = clGetContextInfo(computeContext, CL_CONTEXT_DEVICES, sizeof(device_ids), device_ids, &returned_size);
  if(err)
  {
    printf("Error: Failed to retrieve compute devices for context!\n");
    return EXIT_FAILURE;
  }
  
  device_count = returned_size / sizeof(cl_device_id);
  
  int i = 0;
  int device_found = 0;
  cl_device_type device_type;
  for(i = 0; i < device_count; i++)
  {
    clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
    if(device_type == computeDeviceType)
    {
      computeDeviceId = device_ids[i];
      device_found = 1;
      break;
    }
  }
  
  if(!device_found)
  {
    printf("Error: Failed to locate compute device!\n");
    return EXIT_FAILURE;
  }
  
  // Create a command queue
  //
  computeCommands = clCreateCommandQueue(computeContext, computeDeviceId, 0, &err);
  if (!(computeCommands))
  {
    printf("Error: Failed to create a command queue!\n");
    return EXIT_FAILURE;
  }
  
  // Report the device vendor and device name
  //
  cl_char vendor_name[1024] = {0};
  cl_char device_name[1024] = {0};
  err = clGetDeviceInfo(computeDeviceId, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
  err|= clGetDeviceInfo(computeDeviceId, CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to retrieve device info!\n");
    return EXIT_FAILURE;
  }
  
  printf("do what now--------\n");
  printf("Connecting to %s %s...\n", vendor_name, device_name);
  
  return CL_SUCCESS;
}

int BaseCLKernel::setupComputeKernel(const char* kernelSource)
{

  
  if(computeKernel)
    clReleaseKernel(computeKernel);
  computeKernel = 0;
  
  if(computeProgram)
    clReleaseProgram(computeProgram);
  computeProgram = 0;
  
 
  
  

  
  
#if (DEBUG_INFO)
//  printf("%s", preprocess);
#endif
  int err;
  // Create the compute program from the source buffer
  //
  computeProgram = clCreateProgramWithSource(computeContext, 1, (const char **) & kernelSource, NULL, &err);
  if (!computeProgram || err != CL_SUCCESS)
  {
    printf("Error: Failed to create compute program!\n");
    return EXIT_FAILURE;
  }
  free((void*)kernelSource);

  
  // Build the program executable
  //
  err = clBuildProgram(computeProgram, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    size_t len;
    char buffer[16000];
    
    printf("Error: Failed to build program executable!\n");
    clGetProgramBuildInfo(computeProgram,computeDeviceId, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    printf("%s\n", buffer);
    return EXIT_FAILURE;
  }
  
  
  computeKernel = clCreateKernel(computeProgram, "kernelMain", &err);
  if (!computeKernel || err != CL_SUCCESS)
  {
    printf("Error: Failed to create compute kernel!\n");
    return EXIT_FAILURE;
  }
  
  // Get the maximum work group size for executing the kernel on the device
  //
  err = clGetKernelWorkGroupInfo(computeKernel, computeDeviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &MaxWorkGroupSize, NULL);
  MaxWorkGroupSize = 1;
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to retrieve kernel work group info! %d\n", err);
    exit(1);
  }
  
#if (DEBUG_INFO)
  printf("MaxWorkGroupSize: %d\n", MaxWorkGroupSize);
  printf("WorkGroupItems: %d\n", WorkGroupItems);
#endif
  
  WorkGroupSize[0] = (MaxWorkGroupSize > 1) ? (MaxWorkGroupSize / WorkGroupItems) : MaxWorkGroupSize;
  WorkGroupSize[1] = MaxWorkGroupSize / WorkGroupSize[0];
  //WorkGroupSize[0] = WorkGroupSize[1] = 1;
  
  //printf(SEPARATOR);
  
  return CL_SUCCESS;
  
}

BaseCLKernel::~BaseCLKernel(){
  clFinish(computeCommands);
  clReleaseKernel(computeKernel);
  clReleaseProgram(computeProgram);
  clReleaseCommandQueue(computeCommands);
  clReleaseMemObject(computeResult);
  if (computeImage) {
    clReleaseMemObject(computeImage);
  }
  
  clReleaseContext(computeContext);
  
  computeCommands = 0;
  computeKernel = 0;
  computeProgram = 0;
  computeResult = 0;
  computeImage = 0;
  computeContext = 0;
}

