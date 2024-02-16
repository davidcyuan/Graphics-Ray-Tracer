#pragma once

#include <glm/vec3.hpp>
#include <list>
class ray;
class Geometry;
class TrimeshFace;
class isect;

class BoundingBox {
  bool bEmpty;
  bool dirty;
  glm::dvec3 bmin;
  glm::dvec3 bmax;
  double bArea = 0.0;
  double bVolume = 0.0;
  bool parent_is_geometry = false;
  bool parent_is_TrimeshFace = false;
  Geometry *geometry_parent;
  TrimeshFace *TrimeshFace_parent;

public:
  BoundingBox();
  BoundingBox(glm::dvec3 bMin, glm::dvec3 bMax);

  glm::dvec3 getMin() const { return bmin; }
  glm::dvec3 getMax() const { return bmax; }
  bool isEmpty() { return bEmpty; }
  void setEmpty() { bEmpty = true; }

  void setMin(glm::dvec3 bMin) {
    bmin = bMin;
    dirty = true;
    bEmpty = false;
  }
  void setMax(glm::dvec3 bMax) {
    bmax = bMax;
    dirty = true;
    bEmpty = false;
  }

  void setMin(int i, double val) {
    if (i >= 0 && i <= 2) {
      bmin[i] = val;
      bEmpty = false;
    }
  }

  void setMax(int i, double val) {
    if (i >= 0 && i <= 2) {
      bmax[i] = val;
      bEmpty = false;
    }
  }

  void set_parent(Geometry* parent){
    this->parent_is_geometry = true;
    this->geometry_parent = parent;
  }

  void set_parent(TrimeshFace* parent){
    this->parent_is_TrimeshFace = true;
    this->TrimeshFace_parent = parent;
  }

  bool is_geometry_parent()const{
    return this->parent_is_geometry;
  }
  Geometry* get_geometry_parent()const{
    return this->geometry_parent;
  }

  bool is_TrimeshFace_parent()const{
    return this->parent_is_TrimeshFace;
  }
  TrimeshFace* get_TrimeshFace_parent()const{
    return this->TrimeshFace_parent;
  }

  // Does this bounding box intersect the target?
  bool intersects(const BoundingBox &target) const;

  // does the box contain this point?
  bool intersects(const glm::dvec3 &point) const;

  // if the ray hits the box, put the "t" value of the intersection closest to
  // the origin in tMin and the "t" value of the far intersection in tMax and
  // return true, else return false.
  bool intersect(const ray &r, double &tMin, double &tMax) const;

  double area();
  double volume();
  void merge(const BoundingBox &bBox);
};

class BVH {
  public:
    void add(const BoundingBox *atom_box);
    void print_objects_length(){
      // std::cout<<"testing: "<<this->objects.size()<<std::endl;
    }
    int bvh_length(){
      return atom_boxes.size();
    }
    std::list<const BoundingBox *> get_atom_boxes() const{
      return this->atom_boxes;
    }
    void generate_children(int depth, int bvh_leaf_stop_size);
    void generate_children_atom_boxes(int depth, int bvh_leaf_stop_size);
    bool intersect(ray &r, isect &i) const;
  private:
    BoundingBox bvh_box;
    std::list<const BoundingBox *> atom_boxes;
    bool has_children = false;
    BVH *left_child;
    BVH *right_child;

    static bool compare_boxes_x(const BoundingBox *box_one, const BoundingBox *box_two){
      return box_one->getMin()[0] < box_two->getMin()[0];
    }
    static bool compare_boxes_y(const BoundingBox *box_one, const BoundingBox *box_two){
      return box_one->getMin()[1] < box_two->getMin()[1];
    }
    static bool compare_boxes_z(const BoundingBox *box_one, const BoundingBox *box_two){
      return box_one->getMin()[2] < box_two->getMin()[2];
    }
};