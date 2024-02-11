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
  glm::dvec3 isect_point = r.at(i.getT()-RAY_EPSILON*2);  
  glm::dvec3 pos = r.at(i.getT());
  glm::dvec3 color = ke(i); //Emissivity
  //check if current point is inside or outside object
  bool is_inside = false;
  if(glm::dot(i.getN(), r.getDirection())>0){
    if(debugMode){
      std::cout<<"Inside Object!"<<std::endl;
      std::cout<<"dot product: "<<glm::dot(i.getN(), r.getDirection())<<std::endl;
    }
    is_inside = true;
  }
  else{
    if(debugMode){
      std::cout<<"Outside Object"<<std::endl;
    }
  }
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
    //this is with a backed up point
    std::vector<isect> shadow_sects = scene->intersect_list(shadow_ray, (Light *)pLight);
    //if inside object, then we need to add current intersection in
    if(is_inside){
      isect temp_i = i;
      //if inside, intersection should never be empty
      if(shadow_sects.empty()){
        //std::cout<<"Error: shadow_sects empty while inside object"<<std::endl;
        //std::cout<<isect_point<<std::endl;
        isect temp_i2 = i;
        shadow_sects.emplace_back(temp_i);
      }
      shadow_sects.insert(shadow_sects.begin(), temp_i);
    }
    if(debugMode){
      //std::cout<<"shadow_sects.size(): "<<shadow_sects.size()<<std::endl;
    }
    //no intersections, path to light clear
    //ignore check
    glm::dvec3 dir = pLight->getDirection(pos);
    glm::dvec3 norm = i.getN(); 

    //distance attenuation
    double dist_atten = pLight->distanceAttenuation(pos);
    //shadow attenuation
    glm::dvec3 atten_light = pLight->shadowAttenuation(shadow_sects);


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

  double x = coord[0] * (width - 1);
	double y = coord[1] * (height - 1);

	int x1 = (int)x;
	int x2 = x1 + 1;
	int y1 = (int)y;
	int y2 = y1 + 1;

  glm::dvec3 x1y1 = getPixelAt(x1, y1);
  glm::dvec3 x1y2 = getPixelAt(x1, y2);
  glm::dvec3 x2y1 = getPixelAt(x2, y1);
  glm::dvec3 x2y2 = getPixelAt(x2, y2);

  glm::dvec3 color = x1y1 * (double(x2) - x) * (double(y2) - y) +
										 x1y2 * (x - double(x1)) * (double(y2) - y) +
										 x2y1 * (double(x2) - x) * (y - double(y1)) +
										 x2y2 * (x - double(x1)) * (y - double(y1));


  return color;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const {
  // YOUR CODE HERE
  //
  // In order to add texture mapping support to the
  // raytracer, you need to implement this function.

  //what is relationship between x and y and its position in data
  //what if x and y are out of bounds
    if(data.size() < 3 * width * height){
        return glm::dvec3(0,0,0);
    }

   if( x > width - 1){
     x = width - 1;
   } 

   if (y > height - 1){
     y = height - 1;
   }

   if(x < 0){
    x = 0;
   }

   if(y < 0){
    y = 0;
   }

    int place = (y * width + x) * 3;
    //std::cout<<"point: "<<x << " , "<<y;
    if(data.size() <= place + 2){
      std::cout<<"data out of bounds";
    }
    double r = data[place] / 255.0;
    double g = data[place + 1] / 255.0;
    double b = data[place + 2] / 255.0;
    //std::cout<<" color: "<<glm::dvec3(r,g,b)<<"\n";
    return glm::dvec3(r,g,b);
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