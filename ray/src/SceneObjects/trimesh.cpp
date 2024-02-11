#include "trimesh.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <float.h>
#include <string.h>
#include "../ui/TraceUI.h"
#include <iostream>
extern TraceUI *traceUI;
extern TraceUI *traceUI;

using namespace std;

Trimesh::~Trimesh() {
  for (auto f : faces)
    delete f;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex(const glm::dvec3 &v) { vertices.emplace_back(v); }

void Trimesh::addNormal(const glm::dvec3 &n) { normals.emplace_back(n); }

void Trimesh::addColor(const glm::dvec3 &c) { vertColors.emplace_back(c); }

void Trimesh::addUV(const glm::dvec2 &uv) { uvCoords.emplace_back(uv); }

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace(int a, int b, int c) {
  int vcnt = vertices.size();

  if (a >= vcnt || b >= vcnt || c >= vcnt)
    return false;

  TrimeshFace *newFace = new TrimeshFace(this, a, b, c);
  if (!newFace->degen)
    faces.push_back(newFace);
  else
    delete newFace;

  // Don't add faces to the scene's object list so we can cull by bounding
  // box
  return true;
}

// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
const char *Trimesh::doubleCheck() {
  if (!vertColors.empty() && vertColors.size() != vertices.size())
    return "Bad Trimesh: Wrong number of vertex colors.";
  if (!uvCoords.empty() && uvCoords.size() != vertices.size())
    return "Bad Trimesh: Wrong number of UV coordinates.";
  if (!normals.empty() && normals.size() != vertices.size())
    return "Bad Trimesh: Wrong number of normals.";

  return 0;
}

bool Trimesh::intersectLocal(ray &r, isect &i) const {
  bool have_one = false;
  for (auto face : faces) {
    isect cur;
    if (face->intersectLocal(r, cur)) {
      if (!have_one || (cur.getT() < i.getT())) {
        i = cur;
        have_one = true;
      }
    }
  }
  if (!have_one)
    i.setT(1000.0);
  return have_one;
}

bool TrimeshFace::intersect(ray &r, isect &i) const {
  return intersectLocal(r, i);
}


// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray &r, isect &i) const {
  // YOUR CODE HERE
  //
  /* To determine the color of an intersection, use the following rules:
     - If the parent mesh has non-empty `uvCoords`, barycentrically interpolate
       the UV coordinates of the three vertices of the face, then assign it to
       the intersection using i.setUVCoordinates().
     - Otherwise, if the parent mesh has non-empty `vertexColors`,
       barycentrically interpolate the colors from the three vertices of the
       face. Create a new material by copying the parent's material, set the
       diffuse color of this material to the interpolated color, and then 
       assign this material to the intersection.
     - If neither is true, assign the parent's material to the intersection.
  */
  glm::dvec3 q_dav = parent->vertices[ids[0]];
  glm::dvec3 o_dav = r.getPosition();
  glm::dvec3 n_dav = normal;
  glm::dvec3 v_dav = r.getDirection();
  double denom_dav = glm::dot(v_dav, n_dav);
  double t_dav = -1;
  if(abs(denom_dav) > 0.0000001){
    t_dav = glm::dot((q_dav - o_dav), n_dav) / denom_dav;
  }
  if(t_dav <0){
    return false;
  }



  const glm::dvec3 &A = parent->vertices[ids[0]];
  const glm::dvec3 &B = parent->vertices[ids[1]];
  const glm::dvec3 &C = parent->vertices[ids[2]];

  glm::dvec3 Ba = B - A;
  glm::dvec3 Ac = A - C;
  glm::dvec3 Cb = C - B;
  glm::dvec3 Ca = C - A;
  glm::dvec3 position = r.getPosition() + t_dav * v_dav;

  glm::dvec3 position_A = position - A;
  glm::dvec3 position_B = position - B;
  glm::dvec3 position_C = position - C;

  glm::dvec3 check_normal = glm::normalize(glm::cross(Ba, Ca));

  double u =glm::dot(glm::cross(Ba, position_A), check_normal);
  double v =glm::dot(glm::cross(Cb, position_B), check_normal);
  double w =glm::dot(glm::cross(Ac, position_C), check_normal);

  //there is an intersection, color it
  if (u >= 0.0 && v >= 0.0 && w >= 0.0) {
    i.setN(normal);
    i.setObject(this->parent);
    i.setT(t_dav);

    //color with uv coords
    if(!parent->uvCoords.empty()){
      //bay coords
      glm::dvec3 bay_coords = bay_coordinate(A, B, C, position);
      glm::dvec2 vert_uv_0 = parent->uvCoords[ids[0]];
      glm::dvec2 vert_uv_1 = parent->uvCoords[ids[1]];
      glm::dvec2 vert_uv_2 = parent->uvCoords[ids[2]];
      glm::dvec2 mixed_uv = vert_uv_0 * bay_coords[0] + vert_uv_1 * bay_coords[1] + vert_uv_2 * bay_coords[2];

      i.setUVCoordinates(mixed_uv);

    }
    //color with vertice colors
    if(!this->parent->vertColors.empty()){
      glm::dvec3 bay_coords = bay_coordinate(A, B, C, position);
      //color mixing
      glm::dvec3 vert_color_0 = parent->vertColors[ids[0]];
      glm::dvec3 vert_color_1 = parent->vertColors[ids[1]];
      glm::dvec3 vert_color_2 = parent->vertColors[ids[2]];
      glm::dvec3 mixed_color = vert_color_0 * bay_coords[0] + vert_color_1 * bay_coords[1] + vert_color_2 * bay_coords[2];

      Material material_copy = i.getMaterial();
      material_copy.setDiffuse(mixed_color);
      i.setMaterial(material_copy);

    }
    //mix normals
    if(!this->parent->normals.empty()){
      glm::dvec3 bay_coords = bay_coordinate(A, B, C, position);
      //norm mixing
      glm::dvec3 vert_normal_0 = parent->normals[ids[0]];
      glm::dvec3 vert_normal_1 = parent->normals[ids[1]];
      glm::dvec3 vert_normal_2 = parent->normals[ids[2]];
      glm::dvec3 mixed_normal = vert_normal_0 * bay_coords[0] + vert_normal_1 * bay_coords[1] + vert_normal_2 * bay_coords[2];
      mixed_normal = glm::normalize(mixed_normal);
      i.setN(mixed_normal);

     
    }

    return true;
  } else{
    return false;
  }
}
glm::dvec3 TrimeshFace::bay_coordinate(glm::dvec3 p1, glm::dvec3 p2, glm::dvec3 p3, glm::dvec3 c) const{
  //we want to end up with [a_1 \n a_2] = [b2, b3 \n c2, c3] x [m2 \n m3]
  //then use the inverse to get m2 and m3
  double a_1 = glm::dot(c - p1, p2 - p1);
  double a_2 = glm::dot(c - p1, p3 - p1);
  double b_2 = glm::dot(p2 - p1, p2 - p1);
  double b_3 = glm::dot(p3 - p1, p2 - p1);
  double c_2 = glm::dot(p2 - p1, p3 - p1);
  double c_3 = glm::dot(p3 - p1, p3 - p1);
  glm::dmat2x2 bay_matrix = glm::dmat2x2(b_2, c_2, b_3, c_3);
  glm::dmat2x2 inverse_bay_matrix = glm::inverse(bay_matrix);
  glm::dvec2 bay_coord = inverse_bay_matrix * glm::dvec2(a_1, a_2);
  return glm::dvec3(1 - bay_coord[0] - bay_coord[1], bay_coord[0], bay_coord[1]);
}
// Once all the verts and faces are loaded, per vertex normals can be
// generated by averaging the normals of the neighboring faces.
void Trimesh::generateNormals() {
  int cnt = vertices.size();
  normals.resize(cnt);
  std::vector<int> numFaces(cnt, 0);

  for (auto face : faces) {
    glm::dvec3 faceNormal = face->getNormal();

    for (int i = 0; i < 3; ++i) {
      normals[(*face)[i]] += faceNormal;
      ++numFaces[(*face)[i]];
    }
  }

  for (int i = 0; i < cnt; ++i) {
    if (numFaces[i])
      normals[i] /= numFaces[i];
  }

  vertNorms = true;
}

