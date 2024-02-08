#include "cubeMap.h"
#include "../scene/material.h"
#include "../ui/TraceUI.h"
#include "ray.h"
extern TraceUI *traceUI;

glm::dvec3 CubeMap::getColor(ray r) const {
  // YOUR CODE HERE
  // FIXME: Implement Cube Map here 
  glm::dvec3 ray_dir = r.getDirection();
  double x = ray_dir[0];
  double y = ray_dir[1];
  double z = ray_dir[2];
  double max, u, v;
	int face;
	if(fabs(x) >= fabs(y) && fabs(x) >= fabs(z)){
    v = y;
		max = fabs(x);
    if(x < 0){
        u = z;
        face = 1;
    } else{
      u = -z;
      face = 0;
    }
	} else if (fabs(y) >= fabs(x) && fabs(y) >= fabs(z)) {
    max = fabs(y);
    u = x;
    if(y < 0){
      v = z;
      face = 3;
    } else{
      v = -z;
      face = 2;
    }
  } else if (fabs(z) >= fabs(x) && fabs(z) >= fabs(y)) {
    max = fabs(z);
    v = y;
    if(z < 0){
      u = -x;
      face = 5;
    } else{
      u = x;
      face = 4;
    }
  }
  //scales and moves u and v so that it falls on scale [0,1]
    u = 0.5 * (u / max + 1.0);
	  v = 0.5 * (v / max + 1.0);
    glm::dvec2 coord = glm::dvec2(u, v);
	  return tMap[face] -> getMappedValue(coord);

  return glm::dvec3();
}

CubeMap::CubeMap() {}

CubeMap::~CubeMap() {}

void CubeMap::setNthMap(int n, TextureMap *m) {
  if (m != tMap[n].get())
    tMap[n].reset(m);
}
