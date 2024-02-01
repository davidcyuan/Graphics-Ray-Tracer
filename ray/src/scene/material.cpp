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
  // YOUR CODE HERE

  // For now, this method just returns the diffuse color of the object.
  // This gives a single matte color for every distinct surface in the
  // scene, and that's it.  Simple, but enough to get you started.
  // (It's also inconsistent with the phong model...)

  // Your mission is to fill in this method with the rest of the phong
  // shading model, including the contributions of all the light sources.
  // You will need to call both distanceAttenuation() and
  // shadowAttenuation()
  // somewhere in your code in order to compute shadows and light falloff.
  //  if( debugMode )
  //    std::cout << "Debugging Phong code..." << std::endl;

  // When you're iterating through the lights,
  // you'll want to use code that looks something
  // like this:
  //
  // for ( const auto& pLight : scene->getAllLights() )
  // {
  //              // pLight has type Light*
  //    .
  //    .
  //    .
  // }

  //check all lights
  glm::dvec3 isect_point = r.getPosition() + r.getDirection() * (i.getT()-RAY_EPSILON);       //current intersect point
  glm::dvec3 pos = r.at(i.getT());
  glm::dvec3 color = ke(i); //Emissivity
  color += ka(i) * scene->ambient();

  for(const auto& pLight : scene->getAllLights()){
    glm::dvec3 light_direction = pLight->getDirection(isect_point);              //direction to light

    //check if light is blocked
    isect shadow_sect;
    ray shadow_ray(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::SHADOW);
    shadow_ray.setPosition(isect_point);
    shadow_ray.setDirection(glm::normalize(light_direction));
    bool is_shadow_sect = scene->intersect(shadow_ray, shadow_sect);  //check if the shadowray collides with something
    //check if the collision is before the light
    if(is_shadow_sect){
      double shadow_sect_dist = glm::length(shadow_ray.getDirection() * shadow_sect.getT());
      double light_dist = pLight->getDistance(shadow_ray.getPosition());
      if(light_dist > 0 && light_dist < shadow_sect_dist){// shadow ray hits light before closest object
        is_shadow_sect = false;
      }
    }
    if(is_shadow_sect){
      //something is blocking the light
    }
    else{
      glm::dvec3 dir = pLight->getDirection(pos);
      glm::dvec3 norm = i.getN(); 

      //distance attenuation
      double dist_atten = pLight->distanceAttenuation(pos);


      //diffusal
      glm::dvec3 diffusal = kd(i) * glm::max(glm::dot(norm, dir), 0.0) * pLight->getColor() * dist_atten;
      color+=diffusal;

      //specular
      glm::dvec3 viewDir = r.getDirection();
      glm::dvec3 reflectedDir = glm::reflect(dir, i.getN());
      double specAngle = max(glm::dot(viewDir, reflectedDir), 0.0);
      glm::dvec3 spec = ks(i) * pow(specAngle, shininess(i)) * pLight->getColor() * dist_atten;
      color += spec;
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
  if (0 != _textureMap)
    return _textureMap->getMappedValue(is.getUVCoordinates());
  else
    return _value;
}

double MaterialParameter::intensityValue(const isect &is) const {
  if (0 != _textureMap) {
    glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
    return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
  } else
    return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}