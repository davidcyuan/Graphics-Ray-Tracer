#include <cmath>

#include "../ui/TraceUI.h"
#include "kdTree.h"
#include "light.h"
#include "scene.h"
#include <glm/gtx/extended_min_max.hpp>
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;

extern bool debugMode;

bool Geometry::intersect(ray &r, isect &i) const {
  double tmin, tmax;
  if (hasBoundingBoxCapability() && !(bounds.intersect(r, tmin, tmax)))
    return false;
  // Transform the ray into the object's local coordinate space
  glm::dvec3 pos = transform.globalToLocalCoords(r.getPosition());
  glm::dvec3 dir =
      transform.globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
  double length = glm::length(dir);
  dir = glm::normalize(dir);
  // Backup World pos/dir, and switch to local pos/dir
  glm::dvec3 Wpos = r.getPosition();
  glm::dvec3 Wdir = r.getDirection();
  r.setPosition(pos);
  r.setDirection(dir);
  bool rtrn = false;
  if (intersectLocal(r, i)) {
    // Transform the intersection point & normal returned back into
    // global space.
    i.setN(transform.localToGlobalCoordsNormal(i.getN()));
    i.setT(i.getT() / length);
    rtrn = true;
  }
  // Restore World pos/dir
  r.setPosition(Wpos);
  r.setDirection(Wdir);
  return rtrn;
}

bool Geometry::hasBoundingBoxCapability() const {
  // by default, primitives do not have to specify a bounding box. If this
  // method returns true for a primitive, then either the ComputeBoundingBox()
  // or the ComputeLocalBoundingBox() method must be implemented.

  // If no bounding box capability is supported for an object, that object will
  // be checked against every single ray drawn. This should be avoided whenever
  // possible, but this possibility exists so that new primitives will not have
  // to have bounding boxes implemented for them.
  return false;
}

void Geometry::ComputeBoundingBox() {
  // take the object's local bounding box, transform all 8 points on it,
  // and use those to find a new bounding box.

  BoundingBox localBounds = ComputeLocalBoundingBox();

  glm::dvec3 min = localBounds.getMin();
  glm::dvec3 max = localBounds.getMax();

  glm::dvec4 v, newMax, newMin;

  v = transform.localToGlobalCoords(glm::dvec4(min[0], min[1], min[2], 1));
  newMax = v;
  newMin = v;
  v = transform.localToGlobalCoords(glm::dvec4(max[0], min[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], max[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], max[1], min[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], min[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], min[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(min[0], max[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);
  v = transform.localToGlobalCoords(glm::dvec4(max[0], max[1], max[2], 1));
  newMax = glm::max(newMax, v);
  newMin = glm::min(newMin, v);

  bounds.setMax(glm::dvec3(newMax));
  bounds.setMin(glm::dvec3(newMin));
  bounds.set_parent(this);
}

Scene::Scene() { ambientIntensity = glm::dvec3(0, 0, 0); }

Scene::~Scene() {
  for (auto &obj : objects)
    delete obj;
  for (auto &light : lights)
    delete light;
}


//add bounding boxes here
void Scene::add(Geometry *obj) {
  obj->ComputeBoundingBox();
  obj->generate_bvh();
  sceneBounds.merge(obj->getBoundingBox());
  objects.emplace_back(obj);
  this->bvh.add(&obj->getBoundingBox());

}

void Scene::add(Light *light) { lights.emplace_back(light); }


// Get any intersection with an object.  Return information about the
// intersection through the reference parameter.
// bool Scene::intersect(ray &r, isect &i) const {
//   bool have_one = false;
//   for (const auto &obj : objects) {
//     isect cur;
//     if (obj->intersect(r, cur)) {
//       if (!have_one || (cur.getT() < i.getT())) {
//         i = cur;
//         have_one = true;
//       }
//     }
//   }
//   if (!have_one)
//     i.setT(1000.0);
//   // if debugging,
//   if (TraceUI::m_debug) {
//     addToIntersectCache(std::make_pair(new ray(r), new isect(i)));
//   }
//   return have_one;
// }

//still need to check non-box objects
bool Scene::intersect(ray &r, isect &i) const {
  bool have_one = false;
  for(const BoundingBox *atom_box : this->bvh.get_atom_boxes()){
    double tmin = 0.0;
    double tmax = 0.0;
    if(atom_box->intersect(r, tmin, tmax)){
      Geometry *obj = atom_box->get_geometry_parent();
      isect cur;
      if(obj->intersect(r, cur)){
        if(!have_one || cur.getT()<i.getT()){
          i = cur;
          have_one = true;
        }
      }
    }
  }
  if(!have_one){
    i.setT(1000.0);
  }
  if (TraceUI::m_debug) {
    addToIntersectCache(std::make_pair(new ray(r), new isect(i)));
  }
  return have_one;
}

//r is with backed by epsilon point
std::vector<isect> Scene::intersect_list(ray &r, Light * pLight) const{
  //array of isect objects
  std::vector<isect> intersections;
  //time elapsed
  double time = 0.0;
  bool hitwall = false;
  bool hitlight = false;
  bool hit_opaque_obj = false;
  ray cur_ray = r;
  //keep shooting rays till we hit light or wall
  //!hitwall && !hitlight
  while(!hitwall && !hitlight&&!hit_opaque_obj){
    if(debugMode){
      //std::cout<<"intersect_list time: "<<time<<std::endl;
    }
    isect cur_isect;
    bool hitobject = this->intersect(cur_ray, cur_isect);
    if(hitobject == false){
      if(debugMode){
        //std::cout<<"Didn't hit any objects"<<std::endl;
      }
      hitwall = true;
    }
    else{
      double shadow_sect_dist = glm::length(r.getDirection() * cur_isect.getT());
      double light_dist = pLight->getDistance(r.getPosition());
      //shadow ray hits light before closest object
      if(light_dist > 0 && light_dist < shadow_sect_dist){
        if(debugMode){
          //std::cout<<"Hit light first"<<std::endl;
        }
        hitlight = true;
      }
      //shadow ray hits opaque object
      else if(cur_isect.getMaterial().Trans()==false){
        if(debugMode){
          //std::cout<<"Hit Opaque object"<<std::endl;
        }
        hit_opaque_obj = true;
        //the time it took to travel the new intersection, added onto original time;
        time = time + cur_isect.getT();
        cur_isect.setT(time);
        intersections.emplace_back(cur_isect);
      }
      //shadow ray hits trans object
      else{
        if(debugMode){
          //std::cout<<"Hit trans object"<<std::endl;
        }
        //the time it took to travel the new intersection, added onto original time;
        time = time + cur_isect.getT();
        cur_isect.setT(time);
        intersections.emplace_back(cur_isect);
        //set position to new position
        cur_ray.setPosition(cur_ray.at(cur_isect.getT()+RAY_EPSILON));
      }
    }
  }
  //debug
  if (TraceUI::m_debug) {
    isect i;
    if(intersections.empty()==false){
      i = intersections[0];
    }
    addToIntersectCache(std::make_pair(new ray(r), new isect(i)));
  }
  //should be a list of trans isects, and one opaque at the end
  return intersections;
}

TextureMap *Scene::getTexture(string name) {
  auto itr = textureCache.find(name);
  if (itr == textureCache.end()) {
    textureCache[name].reset(new TextureMap(name));
    return textureCache[name].get();
  }
  return itr->second.get();
}
