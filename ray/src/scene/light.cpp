#include <cmath>
#include <iostream>
#include <algorithm>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3 &) const {
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const ray &r,
                                               const glm::dvec3 &p) const {
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  return glm::dvec3(1.0, 1.0, 1.0);
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

glm::dvec3 PointLight::shadowAttenuation(const ray &r,
                                         const glm::dvec3 &p) const {
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  return glm::dvec3(1, 1, 1);
}

#define VERBOSE 0