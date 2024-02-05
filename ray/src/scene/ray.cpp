#include "ray.h"
#include "../ui/TraceUI.h"
#include "material.h"
#include "scene.h"
#include <iostream>
extern bool debugMode;

const Material &isect::getMaterial() const {
  //std::cout << typeid(obj).name() << "\n";
  /*
  if(debugMode){
    if(material){
      std::cout<<"yes material";
    }
    else{
      Material test_material = obj->getMaterial();
      //if(test_material!=nullptr){
        //std::cout<<m
      //}
    }
    std::cout<<std::endl;
  }
  */
  return material ? *material : obj->getMaterial();
}

ray::ray(const glm::dvec3 &pp, const glm::dvec3 &dd, const glm::dvec3 &w,
         RayType tt)
    : p(pp), d(dd), atten(w), t(tt) {
  TraceUI::addRay(ray_thread_id);
}

ray::ray(const ray &other) : p(other.p), d(other.d), atten(other.atten) {
  TraceUI::addRay(ray_thread_id);
}

ray::~ray() {}

ray &ray::operator=(const ray &other) {
  p = other.p;
  d = other.d;
  atten = other.atten;
  t = other.t;
  return *this;
}

glm::dvec3 ray::at(const isect &i) const { return at(i.getT()); }

thread_local unsigned int ray_thread_id = 0;