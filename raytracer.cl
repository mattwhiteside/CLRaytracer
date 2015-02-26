//
// File:       raytracer.cl
//


__constant float EPSILON = 1e-5f;

__constant unsigned NUM_PIXELS = HEIGHT * WIDTH;


typedef struct{
  float3 origin, direction, color;
} Ray;


typedef struct{
  __global Object* objects;
  __global Light* lights;
  const float3 attenuation;
} Scene;

typedef struct {
  float3 position, normal, eyeDirn;
  __global Object* object;
  float distance;
} Intersection;


__constant unsigned maxdepth = 5;


float16 Matrix4Invert(float16 m);

inline float4 Matrix4MultiplyVector4(float16 m, float4 v){
  //this is assuming float16 is column-major
  return (float4) (
                   dot(m.s048C, v),
                   dot(m.s159D, v),
                   dot(m.s26AE, v),
                   dot(m.s37BF, v)
                   );
}


inline float16 Matrix4Transpose(float16 m){
  float16 ret;
  ret.s0123 = m.s048C;
  ret.s4567 = m.s159D;
  ret.s89AB = m.s26AE;
  ret.sCDEF = m.s37BF;
  return ret;
}


inline float16 Matrix4Multiply(float16 _matrixLeft, float16 _matrixRight){
  float* matrixLeft = (float*)&_matrixLeft;
  float* matrixRight = (float*)&_matrixRight;
  float m[16];
  m[0]  = matrixLeft[0] * matrixRight[0]  + matrixLeft[4] * matrixRight[1]  + matrixLeft[8] * matrixRight[2]   + matrixLeft[12] * matrixRight[3];
  m[4]  = matrixLeft[0] * matrixRight[4]  + matrixLeft[4] * matrixRight[5]  + matrixLeft[8] * matrixRight[6]   + matrixLeft[12] * matrixRight[7];
  m[8]  = matrixLeft[0] * matrixRight[8]  + matrixLeft[4] * matrixRight[9]  + matrixLeft[8] * matrixRight[10]  + matrixLeft[12] * matrixRight[11];
  m[12] = matrixLeft[0] * matrixRight[12] + matrixLeft[4] * matrixRight[13] + matrixLeft[8] * matrixRight[14]  + matrixLeft[12] * matrixRight[15];
  
  m[1]  = matrixLeft[1] * matrixRight[0]  + matrixLeft[5] * matrixRight[1]  + matrixLeft[9] * matrixRight[2]   + matrixLeft[13] * matrixRight[3];
  m[5]  = matrixLeft[1] * matrixRight[4]  + matrixLeft[5] * matrixRight[5]  + matrixLeft[9] * matrixRight[6]   + matrixLeft[13] * matrixRight[7];
  m[9]  = matrixLeft[1] * matrixRight[8]  + matrixLeft[5] * matrixRight[9]  + matrixLeft[9] * matrixRight[10]  + matrixLeft[13] * matrixRight[11];
  m[13] = matrixLeft[1] * matrixRight[12] + matrixLeft[5] * matrixRight[13] + matrixLeft[9] * matrixRight[14]  + matrixLeft[13] * matrixRight[15];
  
  m[2]  = matrixLeft[2] * matrixRight[0]  + matrixLeft[6] * matrixRight[1]  + matrixLeft[10] * matrixRight[2]  + matrixLeft[14] * matrixRight[3];
  m[6]  = matrixLeft[2] * matrixRight[4]  + matrixLeft[6] * matrixRight[5]  + matrixLeft[10] * matrixRight[6]  + matrixLeft[14] * matrixRight[7];
  m[10] = matrixLeft[2] * matrixRight[8]  + matrixLeft[6] * matrixRight[9]  + matrixLeft[10] * matrixRight[10] + matrixLeft[14] * matrixRight[11];
  m[14] = matrixLeft[2] * matrixRight[12] + matrixLeft[6] * matrixRight[13] + matrixLeft[10] * matrixRight[14] + matrixLeft[14] * matrixRight[15];
  
  m[3]  = matrixLeft[3] * matrixRight[0]  + matrixLeft[7] * matrixRight[1]  + matrixLeft[11] * matrixRight[2]  + matrixLeft[15] * matrixRight[3];
  m[7]  = matrixLeft[3] * matrixRight[4]  + matrixLeft[7] * matrixRight[5]  + matrixLeft[11] * matrixRight[6]  + matrixLeft[15] * matrixRight[7];
  m[11] = matrixLeft[3] * matrixRight[8]  + matrixLeft[7] * matrixRight[9]  + matrixLeft[11] * matrixRight[10] + matrixLeft[15] * matrixRight[11];
  m[15] = matrixLeft[3] * matrixRight[12] + matrixLeft[7] * matrixRight[13] + matrixLeft[11] * matrixRight[14] + matrixLeft[15] * matrixRight[15];
  
  return (float16)(
                   m[0],m[1],m[2],m[3],
                   m[4],m[5],m[6],m[7],
                   m[8],m[9],m[10],m[11],
                   m[12],m[13],m[14],m[15]
                   );
}


inline float3 Matrix4MultiplyVector3(float16 m, float3 v){
  float4 tmp = Matrix4MultiplyVector4(m,(float4)(v.x,v.y,v.z,0.0f));
  return (float3)(tmp.x,tmp.y,tmp.z);
}


inline float4 Vector4MakeWithVector3(float3 v, float w){
  return (float4)(v.x,v.y,v.z,w);
}


inline Matrix3 Matrix4GetMatrix3(float16 m)
{
  Matrix3 ret;
  ret.m00 = m.s0;
  ret.m01 = m.s1;
  ret.m02 = m.s2;
  
  
  ret.m10 = m.s4;
  ret.m11 = m.s5;
  ret.m12 = m.s6;
  
  ret.m20 = m.s8;
  ret.m21 = m.s9;
  ret.m22 = m.sA;
  
  return ret;
}

inline Matrix3 Matrix3Transpose(Matrix3 m)
{
  Matrix3 ret;
  ret.m[0] = m.m[0];
  ret.m[1] = m.m[3];
  ret.m[2] = m.m[6];
  
  ret.m[3] = m.m[1];
  ret.m[4] = m.m[4];
  ret.m[5] = m.m[7];
  
  ret.m[6] = m.m[2];
  ret.m[7] = m.m[5];
  ret.m[8] = m.m[8];
  return ret;
}


inline float3 Matrix3MultiplyVector3(Matrix3 matrixLeft, float3 vectorRight){
  float3 v = { matrixLeft.m[0] * vectorRight.x + matrixLeft.m[3] * vectorRight.y + matrixLeft.m[6] * vectorRight.z,
    matrixLeft.m[1] * vectorRight.x + matrixLeft.m[4] * vectorRight.y + matrixLeft.m[7] * vectorRight.z,
    matrixLeft.m[2] * vectorRight.x + matrixLeft.m[5] * vectorRight.y + matrixLeft.m[8] * vectorRight.z };
  return v;
  
}

//need function prototypes to silence compiler warnings
bool Intersect(Ray* r, Intersection* inter, __global Object* pObject, float t1);
float3 ShadePixel(Intersection* inter, Scene* scene, unsigned depth);
void Trace(Ray* ray, Scene* scene);

bool Intersect(Ray* r, Intersection* inter, __global Object* pObject, float t1) {
  float t0 = 0.0f;
  float t = INFINITY;

  if (pObject->geometry == TRIANGLE_T) {
    
    float3 d_ = r->direction;
    float3 e_ = r->origin;
    
    __global float* verts = pObject->privateData;
    
    float3 a_ = (float3)(verts[0],verts[1],verts[2]);
    float3 b_ = (float3)(verts[3],verts[4],verts[5]);
    float3 c_ = (float3)(verts[6],verts[7],verts[8]);
    
    const float a = a_.x - b_.x;
    const float b = a_.y - b_.y;
    const float c = a_.z - b_.z;
    const float d = a_.x - c_.x;
    const float e = a_.y - c_.y;
    const float f = a_.z - c_.z;
    const float g = d_.x;
    const float h = d_.y;
    const float i = d_.z;
    const float j = a_.x - e_.x;
    const float k = a_.y - e_.y;
    const float l = a_.z - e_.z;
    
    const float M = a*(e * i - h * f) + b*(g*f - d*i) + c*(d*h -e*g);
    
    t = -1 * (f * (a*k - j*b) + e*(j*c - a*l) + d * (b*l - k*c))/M;
    if ((t < t0) || (t > t1) || (t < 0)){
      return false;
    }

    float gamma = (i * (a*k - j*b) + h*(j*c -a*l) + g*(b*l - k*c))/M;
    if ((gamma < 0) || (gamma > 1)){
      return false;
    }

    
    float beta = (j*(e*i - h*f) + k*(g*f - d*i) + l * (d*h - e*g))/M;
    
    if ((beta < 0) || (beta > 1 - gamma)){
      return false;
    }
    float3 edge1 = b_ - a_;
    float3 edge2 = c_ - a_;
    inter->normal = cross(edge1, edge2);
    
  } else{

    float16 inverseTransf = pObject->invTransform;

    float3 d_ = Matrix4MultiplyVector3(inverseTransf,r->direction);
    
    float4 tmp = Matrix4MultiplyVector4(inverseTransf, Vector4MakeWithVector3(r->origin,1));
    float3 e_ = (float3)(tmp.x,tmp.y,tmp.z);

    float3 c = (float3)(pObject->privateData[0],pObject->privateData[1],pObject->privateData[2]);
    float r_2 = pObject->privateData[3];//radius squared
    

    float discriminant = powf(dot(d_, e_ - c),2) - dot(d_, d_) * (dot(e_ - c, e_ - c) - r_2);

    if (discriminant <= 0) {
      return false;
    } else{
      float p1 = (-1*dot(d_, e_ - c) + sqrt(discriminant))/dot(d_, d_);
      float p2 = (-1*dot(d_, e_ - c) - sqrt(discriminant))/dot(d_, d_);
      t = fmin(p1,p2);
      if (t < 0 || t > t1) {
        return false;//behind the camera or the light between the intersection
      }
      float3 transformedPosn = e_ + t * d_;
      float3 normal = transformedPosn - c;
      Matrix3 _3by3 = Matrix4GetMatrix3(inverseTransf);
      
      Matrix3 invTransf3x3 = Matrix3Transpose(_3by3);
      
      inter->normal = Matrix3MultiplyVector3(invTransf3x3,normal);
    }
  }

  inter->distance = t;

  return true;
  
}



float3 ShadePixel(Intersection* inter, Scene* scene, unsigned depth){
  float3 ret = (float3)(0,0,0);
  
  __global Object* obj = inter->object;
  
  if (depth == 0) {
    for (unsigned i = 0; i < 3; i++) {
      ret[i] += obj->ambient[i] + obj->emissive[i];
    }
  }
  
  
  for (unsigned i = 0; i < NUM_LIGHTS; i++) {
    float3 directionToLight;
    float distToLight = INFINITY;

    if (scene->lights[i].isDirectional) {
      directionToLight = fast_normalize(scene->lights[i].origin);
    } else {
      directionToLight = scene->lights[i].origin - inter->position;
      distToLight = fast_length(directionToLight);
      directionToLight = fast_normalize(directionToLight);
    }
    Ray r = {inter->position + EPSILON * directionToLight, directionToLight};
    bool blocked = false;
    Intersection _inter;
    

    unsigned j = 0;
    while (j < NUM_OBJECTS && !blocked) {
      __global Object* _obj = &scene->objects[j];
      if (_obj != inter->object && Intersect(&r, &_inter, _obj, distToLight)) {
        blocked = true;        
      }
      j++;
    }

    if (!blocked) {
      float3 directionBackToCamera = -inter->eyeDirn;
      float3 halfAngle = fast_normalize(directionToLight + directionBackToCamera) ;
      
      float _atten = 1;
      if (!scene->lights[i].isDirectional) {
          //attenuation doesnt apply to directional lights, which are infinitely far away
        float dist = fast_length(scene->lights[i].origin - inter->position);
        _atten = scene->attenuation[0] + scene->attenuation[1] * dist + scene->attenuation[2] * dist * dist;
      }
      float nDotL = fmax(dot(directionToLight , inter->normal),0);
      float nDotH = fmax(dot(halfAngle, inter->normal),0);
      ret += (scene->lights[i].color / _atten) * ((obj->diffuse * nDotL) + obj->specular * pow(nDotH,obj->shininess));


    }

  
  }


  return ret;
}


void Trace(Ray* ray, Scene* scene){
  ray->color = (float3)(0,0,0);

  
  unsigned depth = 0;
  
  float specularDecayFactor[3] = {1.00000f, 1.00000f, 1.00000f};
  bool rayHasEscapedScene = false;
  do{
    
    Intersection inter;
    
    
    float minDist = inter.distance = INFINITY;

    bool hitAnything = false;
    
    for (unsigned i = 0; i < NUM_OBJECTS; i++) {      
      bool hit = false;
      __global Object* _object = &scene->objects[i];

      hit = Intersect(ray, &inter, _object, minDist);

      if (hit && inter.distance < minDist) {
        hitAnything = true;
        minDist = inter.distance;
        inter.position = ray->origin + inter.distance * ray->direction;
        inter.object = _object;
        inter.eyeDirn = ray->direction;
      }
    }
    
    if (hitAnything){
      inter.normal = fast_normalize(inter.normal);
      float3 color = ShadePixel(&inter,scene,depth);

      color.x *= specularDecayFactor[0];
      color.y *= specularDecayFactor[1];
      color.z *= specularDecayFactor[2];

      for (unsigned i = 0; i < 3; i++){
        specularDecayFactor[i] *= inter.object->specular[i];
      }


      
      ray->color += color;
      float3 reflDir = inter.eyeDirn - 2.0f * dot(inter.eyeDirn, inter.normal) * inter.normal;
      ray->origin = inter.position  + EPSILON * reflDir;
      ray->direction = fast_normalize(reflDir);

    } else{
      rayHasEscapedScene = true;
    }


    depth++;
  } while(depth < maxdepth && !rayHasEscapedScene);
}


__kernel void
kernelMain(__global uchar4 *result,
          __global Object* _objects,
          __global Light* _lights,
          const float3 _attenuation,
          const Camera camera
){
  
  int tx = get_global_id(0);
  
  int ty = get_global_id(1);

  int index = NUM_PIXELS - (ty * WIDTH + (WIDTH - tx));

  bool valid = (tx < WIDTH) && (ty < HEIGHT);
  
  
  if(valid)
  {

    Scene scene = {_objects, _lights, _attenuation};
    
    float beta = tan(camera.fovy/2.0f)*(
                                     ((HEIGHT/2.0f) - (ty + 0.5f)) / (HEIGHT/2.0f)
                                     );
    
    float alpha = tan(camera.fovx/2.0f)*(
                                       ((tx + 0.5f) - (WIDTH/2.0f))/(WIDTH/2.0f)
                                      );

    float3 direction = alpha * camera.u  + beta * camera.v - camera.w;
    direction = fast_normalize(direction);
    Ray ray = {camera.position, direction};
    Trace(&ray, &scene);

    float4 color = (float4)(ray.color.x, ray.color.y, ray.color.z, 1);
    uchar4 output = convert_uchar4_sat_rte(color * 255.0f);

    result[index] = output;
  }

}