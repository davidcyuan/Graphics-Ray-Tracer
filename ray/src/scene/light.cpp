#include <cmath>
#include <iostream>
#include <algorithm>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

using namespace std;
extern bool debugMode;

double DirectionalLight::distanceAttenuation(const glm::dvec3 &) const {
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const std::vector<isect> &intersect_list) const {
  glm::dvec3 atten_color = this->getColor();
  for(int isect_index = 0; isect_index < intersect_list.size();isect_index+=2){
    isect cur_isect = intersect_list[isect_index];
    //if current material is not transparent, it should be the last, do nothing and exist loop
    if(cur_isect.getMaterial().Trans() == false){
      //the opaque intersection better be the last!
      if(isect_index != intersect_list.size()-1){
        //std::cout<<"Error! There is an opaque intersection before the end of the intersection list"<<std::endl;
      }
      //hitting an opaque object means no light gets through
      atten_color = glm::dvec3(0, 0, 0);
    }
    //if material is transparent, get the coeeficient
    else{
      //if this transparent intersect does not have a followup, we have an issue
      isect next_isect;
      if(isect_index + 1 >= intersect_list.size()){
        //std::cout<<"Error! This transparent intersection does not have a followup!"<<std::endl;
        next_isect = cur_isect;
      }
      else{
        next_isect = intersect_list[isect_index + 1];
      }
      
      //these two intersections better have the same trans value!
      if(cur_isect.getMaterial().kt(cur_isect) != next_isect.getMaterial().kt(next_isect)){
        //std::cout<<"Error! This transparent intersection's followup has a different kt value!"<<std::endl;
      }
      glm::dvec3 kt = cur_isect.getMaterial().kt(cur_isect);
      //assume shadowray is normalized, so time diff = dist diff
      double cur_time = cur_isect.getT();
      double next_time = next_isect.getT();
      double dist = next_time - cur_time;
      glm::dvec3 atten_coeff = glm::dvec3(pow(kt[0], dist), pow(kt[1], dist), pow(kt[2], dist));
      atten_color = atten_color * atten_coeff;
    }
  }
  return atten_color;
}

glm::dvec3 DirectionalLight::getColor() const { return color; }

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3 &) const {
  return -orientation;
}
//should never be called?
double DirectionalLight::getDistance(const glm::dvec3 &) const {
  return -1.0;
}

double PointLight::distanceAttenuation(const glm::dvec3 &P) const {
  // YOUR CODE HERE

  // You'll need to modify this method to attenuate the intensity
  // of the light based on the distance between the source and the
  // point P.  For now, we assume no attenuation and just return 1.0
  glm::dvec3 dist = P - position;
  double d = glm::length(dist);
  double dist_atten = 1/(constantTerm + linearTerm*d + quadraticTerm*d*d);
  if(dist_atten > 1){
    dist_atten = 1;
  }
  return dist_atten;
}

glm::dvec3 PointLight::getColor() const { return color; }

glm::dvec3 PointLight::getDirection(const glm::dvec3 &P) const {
  return glm::normalize(position - P);
}

double PointLight::getDistance(const glm::dvec3 &P) const{
  return glm::length(position - P);
}

glm::dvec3 PointLight::shadowAttenuation(const std::vector<isect> &intersect_list) const {
  glm::dvec3 atten_color = this->getColor();
  for(int isect_index = 0; isect_index < intersect_list.size();isect_index+=2){
    isect cur_isect = intersect_list[isect_index];
    //if current material is not transparent, it should be the last, do nothing and exist loop
    if(cur_isect.getMaterial().Trans() == false){
      //the opaque intersection better be the last!
      if(isect_index != intersect_list.size()-1){
        //std::cout<<"Error! There is an opaque intersection before the end of the intersection list"<<std::endl;
      }
      //hitting an opaque object means no light gets through
      atten_color = glm::dvec3(0, 0, 0);
    }
    //if material is transparent, get the coeeficient
    else{
      //if this transparent intersect does not have a followup, we have an issue
      isect next_isect;
      if(isect_index + 1 >= intersect_list.size()){
        //std::cout<<"Error! This transparent intersection does not have a followup!"<<std::endl;
        next_isect = cur_isect;
      }
      else{
        next_isect = intersect_list[isect_index + 1];
      }
      
      //these two intersections better have the same trans value!
      if(cur_isect.getMaterial().kt(cur_isect) != next_isect.getMaterial().kt(next_isect)){
        //std::cout<<"Error! This transparent intersection's followup has a different kt value!"<<std::endl;
      }
      glm::dvec3 kt = cur_isect.getMaterial().kt(cur_isect);
      //assume shadowray is normalized, so time diff = dist diff
      double cur_time = cur_isect.getT();
      double next_time = next_isect.getT();
      double dist = next_time - cur_time;
      glm::dvec3 atten_coeff = glm::dvec3(pow(kt[0], dist), pow(kt[1], dist), pow(kt[2], dist));
      atten_color = atten_color * atten_coeff;
    }
  }
  return atten_color;
}

#define VERBOSE 0