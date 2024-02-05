#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI *traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material() {}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray &r, const isect &i) const {
  //get initial position and color
  glm::dvec3 isect_point = r.at(i.getT()-RAY_EPSILON);  
  glm::dvec3 pos = r.at(i.getT());
  glm::dvec3 color = ke(i); //Emissivity
  //ambient light
  color += ka(i) * scene->ambient();

  //loop through all lights
  for(const auto& pLight : scene->getAllLights()){
    glm::dvec3 light_direction = pLight->getDirection(isect_point);

    //create shadow ray
    //isect shadow_sect;
    ray shadow_ray(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::SHADOW);
    shadow_ray.setPosition(isect_point);
    shadow_ray.setDirection(glm::normalize(light_direction));
    //get all shadow ray collisions, including last opaque object (if it exists)
    std::vector<isect> shadow_sects = scene->intersect_list(shadow_ray, (Light *)pLight);
    if(debugMode){
      std::cout<<"shadow_sects.size(): "<<shadow_sects.size()<<std::endl;
    }
    //no intersections, path to light clear
    //ignore check
    if(true){
      glm::dvec3 dir = pLight->getDirection(pos);
      glm::dvec3 norm = i.getN(); 

      //distance attenuation
      double dist_atten = pLight->distanceAttenuation(pos);
      //shadow attenuation
      glm::dvec3 atten_light = pLight->shadowAttenuation(shadow_ray, shadow_sects);


      //diffusal
      glm::dvec3 diffusal = kd(i) * glm::max(glm::dot(norm, dir), 0.0) * atten_light * dist_atten;
      color+=diffusal;
      if(debugMode){
        //std::cout<<"kd: "<<kd(i)<<"|| angle coeff: "<<glm::max(glm::dot(norm, dir), 0.0)<<"|| light color: "<<
        //  pLight->getColor()<<"|| dist_atten: "<<dist_atten<<std::endl;
      }

      //specular
      glm::dvec3 viewDir = r.getDirection();
      glm::dvec3 reflectedDir = glm::reflect(dir, i.getN());
      double specAngle = max(glm::dot(viewDir, reflectedDir), 0.0);
      glm::dvec3 spec = ks(i) * pow(specAngle, shininess(i)) * atten_light * dist_atten;
      color += spec;
      if(debugMode){
        //std::cout<<"ks: "<<ks(i)<<"|| spec coeff: "<<pow(specAngle, shininess(i))<<std::endl;
      }
    }

  }
return color;
}

TextureMap::TextureMap(string filename) {
  data = readImage(filename.c_str(), width, height);
  if (data.empty()) {
    width = 0;
    height = 0;
    string error("Unable to load texture map '");
    error.append(filename);
    error.append("'.");
    throw TextureMapException(error);
  }
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2 &coord) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.
  // What this function should do is convert from
  // parametric space which is the unit square
  // [0, 1] x [0, 1] in 2-space to bitmap coordinates,
  // and use these to perform bilinear interpolation
  // of the values.

  return glm::dvec3(1, 1, 1);
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.

  return glm::dvec3(1, 1, 1);
}

glm::dvec3 MaterialParameter::value(const isect &is) const {
  if (0 != _textureMap){
    return _textureMap->getMappedValue(is.getUVCoordinates());
  }
  else{
    return _value;
  }
}

double MaterialParameter::intensityValue(const isect &is) const {
  if (0 != _textureMap) {
    glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
    return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
  } else
    return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}