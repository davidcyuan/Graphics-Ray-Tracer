#ifndef __LIGHT_H__
#define __LIGHT_H__

#ifndef _WIN32
#include <algorithm>
using std::max;
using std::min;
#endif

#include "../ui/TraceUI.h"
#include "scene.h"
#include <FL/gl.h>

class Light : public SceneElement {
public:
  virtual glm::dvec3 shadowAttenuation(const std::vector<isect> &intersect_list) const = 0;
  virtual double distanceAttenuation(const glm::dvec3 &P) const = 0;
  virtual glm::dvec3 getColor() const = 0;
  virtual glm::dvec3 getDirection(const glm::dvec3 &P) const = 0;
  virtual double getDistance(const glm::dvec3 &P) const = 0;


protected:
  Light(Scene *scene, const glm::dvec3 &col)
      : SceneElement(scene), color(col) {}

  glm::dvec3 color;

public:
  virtual void glDrawLight([[maybe_unused]] GLenum lightID) const {}
  virtual void glDrawLight() const {}
};

class DirectionalLight : public Light {
public:
  DirectionalLight(Scene *scene, const glm::dvec3 &orien,
                   const glm::dvec3 &color)
      : Light(scene, color), orientation(glm::normalize(orien)) {}
  virtual glm::dvec3 shadowAttenuation(const std::vector<isect> &intersect_list) const;
  virtual double distanceAttenuation(const glm::dvec3 &P) const;
  virtual glm::dvec3 getColor() const;
  virtual glm::dvec3 getDirection(const glm::dvec3 &P) const;
  virtual double getDistance(const glm::dvec3 &P) const;

protected:
  glm::dvec3 orientation;

public:
  void glDrawLight(GLenum lightID) const;
  void glDrawLight() const;
};

class PointLight : public Light {
public:
  PointLight(Scene *scene, const glm::dvec3 &pos, const glm::dvec3 &color,
             float constantAttenuationTerm, float linearAttenuationTerm,
             float quadraticAttenuationTerm)
      : Light(scene, color), position(pos),
        constantTerm(constantAttenuationTerm),
        linearTerm(linearAttenuationTerm),
        quadraticTerm(quadraticAttenuationTerm) {}

  virtual glm::dvec3 shadowAttenuation(const std::vector<isect> &intersect_list) const;
  virtual double distanceAttenuation(const glm::dvec3 &P) const;
  virtual glm::dvec3 getColor() const;
  virtual glm::dvec3 getDirection(const glm::dvec3 &P) const;
  //get Distance from P to position
  virtual double getDistance(const glm::dvec3 &P) const;

  void setAttenuationConstants(float a, float b, float c) {
    constantTerm = a;
    linearTerm = b;
    quadraticTerm = c;
  }

protected:
  glm::dvec3 position;

  // These three values are the a, b, and c in the distance attenuation function
  // (from the slide labelled "Intensity drop-off with distance"):
  //    f(d) = min( 1, 1/( a + b d + c d^2 ) )
  float constantTerm;  // a
  float linearTerm;    // b
  float quadraticTerm; // c

public:
  void glDrawLight(GLenum lightID) const;
  void glDrawLight() const;

protected:
};

#endif // __LIGHT_H__
