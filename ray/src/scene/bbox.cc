#include "bbox.h"
#include "ray.h"
#include "scene.h"
#include "../SceneObjects/trimesh.h"

BoundingBox::BoundingBox() : bEmpty(true) {}

BoundingBox::BoundingBox(glm::dvec3 bMin, glm::dvec3 bMax)
    : bEmpty(false), dirty(true), bmin(bMin), bmax(bMax) {}

bool BoundingBox::intersects(const BoundingBox &target) const {
  return ((target.getMin()[0] - RAY_EPSILON <= bmax[0]) &&
          (target.getMax()[0] + RAY_EPSILON >= bmin[0]) &&
          (target.getMin()[1] - RAY_EPSILON <= bmax[1]) &&
          (target.getMax()[1] + RAY_EPSILON >= bmin[1]) &&
          (target.getMin()[2] - RAY_EPSILON <= bmax[2]) &&
          (target.getMax()[2] + RAY_EPSILON >= bmin[2]));
}

bool BoundingBox::intersects(const glm::dvec3 &point) const {
  return ((point[0] + RAY_EPSILON >= bmin[0]) &&
          (point[1] + RAY_EPSILON >= bmin[1]) &&
          (point[2] + RAY_EPSILON >= bmin[2]) &&
          (point[0] - RAY_EPSILON <= bmax[0]) &&
          (point[1] - RAY_EPSILON <= bmax[1]) &&
          (point[2] - RAY_EPSILON <= bmax[2]));
}

bool BoundingBox::intersect(const ray &r, double &tMin, double &tMax) const {
  /*
   * Kay/Kajiya algorithm.
   */
  glm::dvec3 R0 = r.getPosition();
  glm::dvec3 Rd = r.getDirection();
  tMin = -1.0e308; // 1.0e308 is close to infinity... close enough
                   // for us!
  tMax = 1.0e308;
  double ttemp;

  for (int currentaxis = 0; currentaxis < 3; currentaxis++) {
    double vd = Rd[currentaxis];
    // if the ray is parallel to the face's plane (=0.0)
    if (vd == 0.0)
      continue;
    double v1 = bmin[currentaxis] - R0[currentaxis];
    double v2 = bmax[currentaxis] - R0[currentaxis];
    // two slab intersections
    double t1 = v1 / vd;
    double t2 = v2 / vd;
    if (t1 > t2) { // swap t1 & t2
      ttemp = t1;
      t1 = t2;
      t2 = ttemp;
    }
    if (t1 > tMin)
      tMin = t1;
    if (t2 < tMax)
      tMax = t2;
    if (tMin > tMax)
      return false; // box is missed
    if (tMax < RAY_EPSILON)
      return false; // box is behind ray
  }
  return true; // it made it past all 3 axes.
}

double BoundingBox::area() {
  if (bEmpty)
    return 0.0;
  else if (dirty) {
    bArea = 2.0 * ((bmax[0] - bmin[0]) * (bmax[1] - bmin[1]) +
                   (bmax[1] - bmin[1]) * (bmax[2] - bmin[2]) +
                   (bmax[2] - bmin[2]) * (bmax[0] - bmin[0]));
    dirty = false;
  }
  return bArea;
}

double BoundingBox::volume() {
  if (bEmpty)
    return 0.0;
  else if (dirty) {
    bVolume = ((bmax[0] - bmin[0]) * (bmax[1] - bmin[1]) * (bmax[2] - bmin[2]));
    dirty = false;
  }
  return bVolume;
}

void BoundingBox::merge(const BoundingBox &bBox) {
  if (bBox.bEmpty)
    return;
  for (int axis = 0; axis < 3; axis++) {
    if (bEmpty || bBox.bmin[axis] < bmin[axis])
      bmin[axis] = bBox.bmin[axis];
    if (bEmpty || bBox.bmax[axis] > bmax[axis])
      bmax[axis] = bBox.bmax[axis];
  }
  dirty = true;
  bEmpty = false;
}

void BVH::add(const BoundingBox *atom_box){
  this->atom_boxes.push_back(atom_box);
  this->bvh_box.merge(*atom_box);
}

void BVH::generate_children(int depth, int bvh_leaf_stop_size){
  //no children
  if(this->atom_boxes.size()<= bvh_leaf_stop_size || depth <= 0){
    return;
  }
  this->has_children = true;
  BVH *lefty = new BVH();
  BVH *righty = new BVH();

  glm::dvec3 bvh_box_dimensions = this->bvh_box.getMax() - this->bvh_box.getMin();
  double x_dist = bvh_box_dimensions[0];
  double y_dist = bvh_box_dimensions[1];
  double z_dist = bvh_box_dimensions[2];
  
  //split dimension
  if(x_dist > y_dist && x_dist > z_dist){
    this->atom_boxes.sort(BVH::compare_boxes_x);
  }
  else if(y_dist > x_dist && y_dist > z_dist){
    this->atom_boxes.sort(BVH::compare_boxes_y);
  }
  else{
    this->atom_boxes.sort(BVH::compare_boxes_z);
  }

  //split
  int atom_boxes_size = this->atom_boxes.size();
  for(int x = 0; x<atom_boxes_size; x++){
    const BoundingBox *popped_box = this->atom_boxes.front();
    this->atom_boxes.pop_front();
    if(x < atom_boxes_size/2){
      lefty->add(popped_box);
    }
    else{
      righty->add(popped_box);
    }
  }

  //assign
  this->left_child = lefty;
  this->right_child = righty;

  //recurse
  //std::cout<<"got to recurse"<<std::endl;
  this->left_child->generate_children(depth - 1, bvh_leaf_stop_size);
  this->right_child->generate_children(depth -1, bvh_leaf_stop_size);
}
void BVH::generate_children_atom_boxes(int depth, int bvh_leaf_stop_size){
  for(const BoundingBox *atom_box : this->atom_boxes){
    if(atom_box->is_geometry_parent()){
      atom_box->get_geometry_parent()->generate_bvh(depth, bvh_leaf_stop_size);
    }
  }
}

bool BVH::intersect(ray &r, isect &i) const{
  //std::cout<<"starting intersect"<<std::endl;
  bool have_one = false;

  double tmin = 0.0;
  double tmax = 0.0;
  if(this->bvh_box.intersect(r, tmin, tmax)){
    if(this->has_children){
      isect left_isect;
      bool left_have_one = false;
      isect right_isect;
      bool right_have_one = false;

      left_have_one = this->left_child->intersect(r, left_isect);
      right_have_one = this->right_child->intersect(r, right_isect);
      if(left_have_one){
        if(!have_one||left_isect.getT()<i.getT()){
          i = left_isect;
          have_one = true;
        }
      }
      if(right_have_one){
        if(!have_one||right_isect.getT()<i.getT()){
          i = right_isect;
          have_one = true;
        }
      }
    }
    else{
      //std::cout<<"no children"<<std::endl;
      for(const BoundingBox *atom_box : this->get_atom_boxes()){
        double tmin = 0.0;
        double tmax = 0.0;
        if(atom_box->intersect(r, tmin, tmax)){
          isect cur;
          if(atom_box->is_geometry_parent()){
            Geometry *obj = atom_box->get_geometry_parent();
            isect geometry_isect;
            if(obj->intersect(r, geometry_isect)){
              if(!have_one || geometry_isect.getT()<i.getT()){
                i = geometry_isect;
                have_one = true;
              }
            }
          }
          else if(atom_box->is_TrimeshFace_parent()){
            TrimeshFace *face = atom_box->get_TrimeshFace_parent();
            isect trimeshFace_isect;
            if(face->intersectLocal(r, trimeshFace_isect)){
              if(!have_one||(trimeshFace_isect.getT()<i.getT())){
                i = trimeshFace_isect;
                have_one = true;
              }
            }
          }
        }
      }
    }
  }
  if(!have_one){
    i.setT(1000.0);
  }
  return have_one;
}