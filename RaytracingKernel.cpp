//
//  raytrace_kernel.cpp
//  CLRaytracer
//
//  Created by Matthew Whiteside on 9/22/13.
//  Copyright (c) 2013 Matt Whiteside. All rights reserved.
//

#include <stdio.h>
#include "RaytracingKernel.h"



// Basic includes to get this file to work.
#include <iostream>
#include <string>
#include <deque>
#include <stack>
#include "Transform.h"


using namespace std;

RaytracingKernel::RaytracingKernel(const string configFile, unsigned pWidth, unsigned pHeight) : BaseCLKernel("",pWidth, pHeight){
  int err = 0;
  std::ifstream t("types.h");
  std::stringstream buffer;
  buffer << t.rdbuf();
  string sourceFile = "raytracer.cl";
  //string sourceFile = "row_based_raytracer.cl";
  std::ifstream r(sourceFile);
  std::stringstream buff2;
  buff2 << r.rdbuf();
  const string source = buff2.str();
  
  string typesFileSource = buffer.str();
  string target = "#include <OpenCL/OpenCL.h>";
  size_t i = typesFileSource.find(target);
  typesFileSource.replace(i, target.length(), "");
  char* preprocess = (char*)malloc(source.length() + typesFileSource.length() + 1024);
  readfile(configFile.c_str(), width, height);
  sprintf(preprocess, "\ntypedef float cl_float; typedef float3 cl_float3; typedef float16 cl_float16; typedef bool cl_bool;\n\n#define NUM_OBJECTS (%lu)\n#define NUM_LIGHTS (%lu)\n#define WIDTH (%u)\n#define HEIGHT (%u)\n%s\n\n%s", objects.size(), lights.size(), width, height, typesFileSource.c_str(), source.c_str());

  BaseCLKernel::setupComputeKernel(preprocess);
  err = BaseCLKernel::createComputeResult();
  if(err != CL_SUCCESS)
  {
    printf ("Failed to create compute result! Error %d\n", err);
    exit (err);
  }

  
}


int RaytracingKernel::customRecompute(){
  int err = 0;
  kernelObjectData = clCreateBuffer(computeContext,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    sizeof(Object) * objects.size(),
                                    &objects[0],
                                    NULL);
  
  
  
  
  kernelLightData = clCreateBuffer(computeContext,
                                   CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(Light) * lights.size(),
                                   &lights[0],
                                   NULL);
  
  
  err = CL_SUCCESS;
  
  
  err |= clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &computeResult);
  err |= clSetKernelArg(computeKernel, 1, sizeof(cl_mem), &kernelObjectData);
  err |= clSetKernelArg(computeKernel, 2, sizeof(cl_mem), &kernelLightData);
  err |= clSetKernelArg(computeKernel, 3, sizeof(cl_float3), attenuation);
  err |= clSetKernelArg(computeKernel, 4, sizeof(Camera), &camera);
  
  return err;
  
}

// You may not need to use the following two functions, but it is provided
// here for convenience

// The function below applies the appropriate transform to a 4-vector
void RaytracingKernel::matransform(stack<mat4> &transfstack, cl_float* values)
{
  mat4 transform = transfstack.top();
  vec4 valvec = vec4(values[0],values[1],values[2],values[3]);
  vec4 newval = transform * valvec;
  for (int i = 0; i < 4; i++) values[i] = newval[i];
}

void RaytracingKernel::rightmultiply(const mat4 & M, stack<mat4> &transfstack)
{
  mat4 &T = transfstack.top();
  T = T * M;
}

// Function to read the input data values
// Use is optional, but should be very helpful in parsing.
bool RaytracingKernel::readvals(stringstream &s, const int numvals, cl_float* values)
{
  for (int i = 0; i < numvals; i++) {
    s >> values[i];
    if (s.fail()) {
      cout << "Failed reading value " << i << " will skip\n";
      return false;
    }
  }
  return true;
}

void RaytracingKernel::readfile(const char* filename, const float pWidth, const float pHeight)
{
  string str, cmd;
  ifstream in;
  in.open(filename);
  if (in.is_open()) {
    
    // I need to implement a matrix stack to store transforms.
    // This is done using standard STL Templates
    stack <mat4> transfstack;
    transfstack.push(mat4(1.0));  // identity
    
    getline (in, str);
    const unsigned numLightComponents = 3;
    while (in) {
      if ((str.find_first_not_of(" \t\r\n") != string::npos) && (str[0] != '#')) {
        // Ruled out comment and blank lines
        
        stringstream s(str);
        s >> cmd;
        int i;
        cl_float values[10]; // Position and color for light, colors for others
                             // Up to 10 params for cameras.
        bool validinput; // Validity of input
        
        // Process the light, add it to database.
        // Lighting Command
        
        if (cmd == "point") {
          validinput = readvals(s, 6, values); // Position/color for lts.
          if (validinput) {
            Light l = {{values[0],values[1], values[2]},{values[3],values[4],values[5]},false};
            lights.push_back(l);
          }
          
        } else if (cmd == "directional"){
          validinput = readvals(s, 6, values); // Position/color for lts.
          if (validinput) {
            Light l = {{values[0],values[1], values[2]},{values[3],values[4],values[5]},true};
            lights.push_back(l);
          }
        } else if (cmd == "attenuation"){
          validinput = readvals(s, 3, values);
          if (validinput) {
            attenuation[0] = values[0];
            attenuation[1] = values[1];
            attenuation[2] = values[2];
          }
        }
        
        // Material Commands
        // Ambient, diffuse, specular, shininess properties for each object.
        // Filling this in is pretty straightforward, so I've left it in
        // the skeleton, also as a hint of how to do the more complex ones.
        // Note that no transforms/stacks are applied to the colors.
        
        else if (cmd == "ambient") {
          validinput = readvals(s, numLightComponents, values); // colors
          if (validinput) {
            ambient.s[0] = values[0];
            ambient.s[1] = values[1];
            ambient.s[2] = values[2];
          }
        } else if (cmd == "diffuse") {
          validinput = readvals(s, numLightComponents, values);
          if (validinput) {
            diffuse.s[0] = values[0];
            diffuse.s[1] = values[1];
            diffuse.s[2] = values[2];
            
          }
        } else if (cmd == "specular") {
          validinput = readvals(s, numLightComponents, values);
          if (validinput) {
            specular.s[0] = values[0];
            specular.s[1] = values[1];
            specular.s[2] = values[2];
          }
        } else if (cmd == "emission") {
          validinput = readvals(s, numLightComponents, values);
          if (validinput) {
            for (i = 0; i < numLightComponents; i++) {
              emission.s[i] = values[i];
            }
          }
        } else if (cmd == "shininess") {
          validinput = readvals(s, 1, values);
          if (validinput) {
            shininess = values[0];
          }
        } else if (cmd == "camera") {
          validinput = readvals(s,10,values); // 10 values eye cen up fov
          if (validinput) {
            vec3 position(values[0],values[1],values[2]);
            vec3 lookAt(values[3],values[4],values[5]);
            vec3 upVector(values[6],values[7],values[8]);
            
            for (unsigned i = 0; i < 3; i++) {
              camera.position.s[i] = values[i];
              camera.lookAt.s[i] = values[i+3];
              camera.upVector.s[i] = values[i+6];
            }
            
            camera.fovy = glm::radians(values[9]);
            camera.fovx = 2*atanf(tanf(camera.fovy/2)*(pWidth/pHeight));
            const vec3 w = glm::normalize(position - lookAt);
            const vec3 u = glm::normalize(glm::cross(upVector, w));
            const vec3 v = glm::cross(w, u);
            
            for (unsigned i = 0; i < 3; i++) {
              camera.u.s[i] = u[i];
              camera.v.s[i] = v[i];
              camera.w.s[i] = w[i];
            }
            
            
          }
        }
        
        else if (cmd == "vertex"){
          validinput = readvals(s, 3, values);
          if (validinput) {
            vec4 vert4 = transfstack.top() * vec4(values[0],values[1],values[2],1);
            cl_float3 vert = {static_cast<cl_float>(vert4.x),static_cast<cl_float>(vert4.y),static_cast<cl_float>(vert4.z)};
            
            vertices.push_back(vert);
          }
          
          
        } else if (cmd == "vertexnormal"){
          validinput = readvals(s, 6, values);
          if (validinput) {
            //                VertexNormal vert = {
            //                    vec3(values[0],values[1],values[2]),
            //                    vec3(values[3],values[4],values[5])
            //                };
            //                vertexnormals.push_back(vert);
            //never needed this in the homework, so didnt implement
          }
          
        } else if (cmd == "tri"){
          validinput = readvals(s, 3, values);
          if (validinput) {
            Object obj;
            obj.geometry = TRIANGLE_T;

            obj.ambient = ambient;
            obj.diffuse = diffuse;
            obj.specular = specular;
            obj.emissive = emission;
            obj.shininess = shininess;
            
            for (unsigned i = 0; i < 3; i++) {
              cl_float3 vert = vertices[values[i]];
              auto v = vec3( transfstack.top() * vec4(vert.s0, vert.s1, vert.s2,1));
              for (unsigned j = 0; j < 3; j++) {
                obj.privateData[i*3 + j] = v[j];
              }
              
            }
            objects.push_back(obj);
          }
          
        } else if (cmd == "trinormal"){
          validinput = readvals(s, 3, values);
          if (validinput) {
            //never needed this in the homework, so didnt implement
          }
          
        } else if (cmd == "maxverts"){
          validinput = readvals(s, 1, values);
          if (validinput) {
            
          }
          
        } else if (cmd == "maxvertnorms"){
          validinput = readvals(s, 1, values);
          if (validinput) {
            //never needed this in the homework, so didnt implement
          }
          
        } else if (cmd == "maxdepth"){
          validinput = readvals(s, 1, values);
          if (validinput) {
            maxdepth = 5;
          }
        }
        
        else if (cmd == "sphere") {
          
          validinput = readvals(s, 4, values);
          if (validinput) {
            
            
            Object obj;
            
            obj.geometry = SPHERE_T;
            obj.ambient = ambient;
            obj.diffuse = diffuse;
            obj.specular = specular;
            obj.emissive = emission;
            obj.shininess = shininess;
            
            mat4 transf = transfstack.top();
            
            auto inv = glm::inverse(transf);
            for (unsigned col = 0; col < 4; col++) {
              for (unsigned row = 0; row < 4; row++) {
                obj.transform.s[4 * col + row] = transf[col][row];
                obj.invTransform.s[4 * col + row] = inv[col][row];
              }
            }
            
            
            
            vec3 center(values[0],values[1],values[2]);
            for (unsigned j = 0; j < 3; j++) {
              obj.privateData[j] = center[j];
            }
            float radius = values[3];
            obj.privateData[3] = radius * radius;
            
            objects.push_back(obj);
          }
        }
        
        else if (cmd == "translate") {
          validinput = readvals(s,3,values);
          if (validinput) {
            const mat4 M = Transform::translate(values[0], values[1], values[2]);
            rightmultiply(M, transfstack);
            
          }
        }
        else if (cmd == "scale") {
          validinput = readvals(s,3,values);
          if (validinput) {
            
            // YOUR CODE FOR HW 2 HERE.
            // Think about how the transformation stack is affected
            // You might want to use helper functions on top of file.
            // Also keep in mind what order your matrix is!
            const mat4 M = Transform::scale(values[0], values[1], values[2]);
            rightmultiply(M, transfstack);
          }
        }
        else if (cmd == "rotate") {
          validinput = readvals(s,4,values);
          if (validinput) {
            
            // YOUR CODE FOR HW 2 HERE.
            // values[0..2] are the axis, values[3] is the angle.
            // You may want to normalize the axis (or in Transform::rotate)
            // See how the stack is affected, as above.
            // Note that rotate returns a mat3.
            // Also keep in mind what order your matrix is!
            vec3 axis(values[0],values[1],values[2]);
            mat3 M = Transform::rotate(values[3],axis);
            const mat4 rotor(M[0][0],M[0][1],M[0][2],0,
                             M[1][0],M[1][1],M[1][2],0,
                             M[2][0],M[2][1],M[2][2],0,
                             0,0,0,1);
            rightmultiply(rotor, transfstack);
            
          }
        }
        
        // I include the basic push/pop code for matrix stacks
        else if (cmd == "pushTransform") {
          transfstack.push(transfstack.top());
        } else if (cmd == "popTransform") {
          if (transfstack.size() <= 1) {
            cerr << "Stack has no elements.  Cannot Pop\n";
          } else {
            transfstack.pop();
          }
        }
        else if (cmd == "output"){
          validinput = readvals(s,20,values);
          //outfile = string(&values);
        }
        else {
          cerr << "Unknown Command: " << cmd << " Skipping \n";
        }
      }
      getline (in, str);
    }
    
    // Set up initial position for eye, up and amount
    // As well as booleans
    
    
    //glEnable(GL_DEPTH_TEST);
  } else {
    cerr << "Unable to Open Input Data File " << filename << "\n";
    throw 2; 
  }
}
