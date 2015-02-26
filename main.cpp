  //
//  main.c
//  CLRaytracer
//
//  Created by Matt Whiteside on 6/23/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//


#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/CGLDevice.h>

#include <GLFW/glfw3.h>



#include <stack>
#include <algorithm>
#include "Transform.h"
#include "RaytracingKernel.h"

using namespace std;
#define MAINPROGRAM

#include <mach/mach_time.h>

#include <sstream>
#include <fstream>


static const unsigned Height = 480, Width = 640;

static BaseCLKernel* kern = nullptr;


static int FrameCount                   = 0;



static bool hasRun = false;






static void
Display(void)
{
  FrameCount++;
  //uint64_t uiStartTime = GetCurrentTime();
  
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glClear (GL_COLOR_BUFFER_BIT);
  
  int err = 0;
  if (!hasRun) {

    const uint64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t s_timebase_info;
    
    if (s_timebase_info.denom == 0) {
      mach_timebase_info(&s_timebase_info);
    }

    
    auto start = ((mach_absolute_time() * s_timebase_info.numer) / (kOneMillion * s_timebase_info.denom));
    err = kern->recompute();
    auto end = ((mach_absolute_time() * s_timebase_info.numer) / (kOneMillion * s_timebase_info.denom));
    auto total = end - start;
    // mach_absolute_time() returns billionth of seconds,
    // so divide by one million to get milliseconds
    
    printf("total time: %llu ms\n",total);
    hasRun = true;
  }
  
  if (err != 0)
  {
    printf("Error %d from Recompute!\n", err);
    exit(1);
  }
  
  kern->renderTexture();

}



int main(int argc, char** argv)
{
  // Parse command line options
  //
  
  int i;
  int use_gpu = 0;
  for( i = 0; i < argc && argv; i++)
  {
    if(!argv[i])
    continue;
    
    if(strstr(argv[i], "cpu"))
    use_gpu = 0;
    
    else if(strstr(argv[i], "gpu"))
    use_gpu = 1;
  }


  GLFWwindow* window;
  if (!glfwInit())
    return -1;
  
  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(Width, Height, "Hello World", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }
  
  /* Make the window's context current */
  glfwMakeContextCurrent(window);
  
  kern = (BaseCLKernel*)(new RaytracingKernel(argv[1],Width*2,Height*2));

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window))
  {
    /* Render here */
    Display();
    /* Swap front and back buffers */
    glfwSwapBuffers(window);
    
    /* Poll for and process events */
    glfwPollEvents();
  }
  
  glfwTerminate();
  
  return 0;
}