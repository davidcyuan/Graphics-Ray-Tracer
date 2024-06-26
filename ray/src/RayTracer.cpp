// The main ray tracer.

#pragma warning(disable : 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/JsonParser.h"
#include "parser/Parser.h"
#include "parser/Tokenizer.h"
#include <json.hpp>

#include "ui/TraceUI.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <fstream>
#include <iostream>

using namespace std;
extern TraceUI *traceUI;


// Use this variable to decide if you want to print out debugging messages. Gets
// set in the "trace single ray" mode in TraceGLWindow, for example.
bool debugMode = false;

bool cel = false;

bool animation = true;
// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates
// (x,y), through the projection plane, and out into the scene. All we do is
// enter the main ray-tracing method, getting things started by plugging in an
// initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

//? Trace ray through windowsx, windowsy
glm::dvec3 RayTracer::trace(double x, double y) {
  // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) {
    scene->clearIntersectCache();
  }

  //anitaliasing

  ray r(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1),
        ray::VISIBILITY);
  scene->getCamera().rayThrough(x, y, r);
  isect isect_dummy;
  glm::dvec3 ret =
      traceRay(r, traceUI->getDepth(), isect_dummy, glm::dvec3(1, 1, 1));
  ret = glm::clamp(ret, 0.0, 1.0);
  return ret;
}

//? Trace ray through pixel i, pixel j, relative to buffer
glm::dvec3 RayTracer::tracePixel(int i, int j) {
	glm::dvec3 col(0, 0, 0);

	if (!sceneLoaded())
		return col;

	double x = double(i) / double(buffer_width);
	double y = double(j) / double(buffer_height);

	unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;

//  std::cout<<"get aa_switch : "<<traceUI->aaSwitch()<<std::endl;

	int ss_dim = 1;	//super sample dimension
  if(traceUI->aaSwitch()){
    ss_dim = traceUI->getSuperSamples();
  }
	//super sample unit lengths
	double ss_x_unit = 1/double(buffer_width) / double(ss_dim);
	double ss_y_unit = 1/double(buffer_height) / double(ss_dim);
	//shift center pixel to bottom left pixel
	double ss_offset_x = 0.0;
	double ss_offset_y = 0.0;

	if(ss_dim % 2 == 1){	//odd ss_dim
		//still in center of pixel
	}
	else{	//even ss_dim
		//shift from border to center of pixel
		ss_offset_x -= ss_x_unit / 2;
		ss_offset_y -= ss_y_unit / 2;
	}
	//shift from "center" pixel to bottom left pixel
	//1 = no shift, 2 = no shift, 3 = 1 unit, 4 = 1 unit;
	int offset_coeff = (ss_dim-1)/2;
	if(offset_coeff < 0){
		offset_coeff = 0;
	}
	ss_offset_x -= ss_x_unit * offset_coeff;
	ss_offset_y -= ss_y_unit * offset_coeff;

	for(int ss_i = 0;ss_i < ss_dim; ss_i++){
		for(int ss_j = 0;ss_j < ss_dim; ss_j++){
			double ss_x = x + ss_offset_x + ss_i*ss_x_unit;
			double ss_y = y + ss_offset_y + ss_j*ss_y_unit;
			//std::cout<<ss_x<<", "<<ss_y<<"  ";
			col += trace(ss_x, ss_y);
      /*if (glm::length(col) < glm::length(thresh)) {
        return col;
      }*/
		}
		//std::cout<<"\n";
	}
	col = col * (1 / double(ss_dim * ss_dim));

  pixel[0] = (int)(255.0 * col[0]);
  pixel[1] = (int)(255.0 * col[1]);
  pixel[2] = (int)(255.0 * col[2]);
  return col;
}

#define VERBOSE 0

// Do recursive ray tracing! You'll want to insert a lot of code here (or places
// called from here) to handle reflection, refraction, etc etc.

//? Actual ray trace
//dummy intersect object that gets filled with previous ray intersection in order to calculate distance in recursive traceRay calls
glm::dvec3 RayTracer::traceRay(ray &r, int depth,
                                isect &intersect, glm::dvec3 prev_kr) {
  isect i;
  glm::dvec3 colorC;
#if VERBOSE
  std::cerr << "== current depth: " << depth << std::endl;
#endif
  
  if (scene->intersect(r, i)) {
    intersect = i;
    // YOUR CODE HERE

    // An intersection occurred!  We've got work to do. For now, this code gets
    // the material for the surface that was intersected, and asks that material
    // to provide a color for the ray.

    // This is a great place to insert code for recursive ray tracing. Instead
    // of just returning the result of shade(), add some more steps: add in the
    // contributions from reflected and refracted rays.

    //phong illumination
    const Material &m = i.getMaterial();
    colorC = m.shade(scene.get(), r, i, cel);;

    if(depth > 0){
        
      glm::dvec3 ray_pos = r.at(i.getT());
		  glm::dvec3 ray_dir = r.getDirection();
		  glm::dvec3 norm = i.getN();


      // reflection
      //max reflect magnitude must be above threshold to be worth calculating
      double max_reflect_mag = glm::length(m.kr(i) * prev_kr);
     if(glm::length(m.kr(i)) != 0 && max_reflect_mag > traceUI->getThreshold()){
       glm::dvec3 opp = - r.getDirection();
         glm::dvec3 reflect = 2 * glm::dot(opp, norm) * norm - opp;
			    ray reflectRay = ray(ray_pos - RAY_EPSILON * ray_dir, reflect, glm::dvec3(1, 1, 1), ray::REFLECTION);
          isect dummy;
			    colorC += m.kr(i) * traceRay(reflectRay, depth - 1, dummy, prev_kr * m.kr(i));
      }

      //check if materical has non zero transmissive index
      if(m.Trans()){
            //get the refractive index
            double d_prod = glm::dot(ray_dir, norm);
            double index= m.index(i);
            //see if we are inside the object
            bool inside = false;
            if (d_prod > 0) {
              if(debugMode){
                std::cout<<"Sophia inside check"<<std::endl;
              }
              // Inside the object, invert the normal
              norm = -norm;
              inside = true;
           } else if (d_prod == 0){
            //case when ray parallel to surface
              index= 0;
              norm = {0.0, 0.0, 0.0};
            } else{
              if(debugMode){
                std::cout<<"Sophia oustide check"<<std::endl;
              }
              //going outside the object
              index= 1.0 / index;
            }
            double cos = (1.0 - (index* index* (1.0 - (d_prod * d_prod))));
            if(cos > 0){
                glm::dvec3 refrac = glm::refract(glm::normalize(ray_dir), glm::normalize(norm), index);
				        ray refract_ray = ray(ray_pos + RAY_EPSILON * refrac, refrac, glm::dvec3(1,1,1), ray::REFRACTION);
                isect dummy;
                glm::dvec3 col = glm::dvec3(0, 0, 0);

                // 0 depth trace ray to get attenuation
                ray atten_test_ray = refract_ray;
                isect atten_dummy = dummy;
                glm::dvec3 dummy_col = traceRay(atten_test_ray, 0, atten_dummy, prev_kr);
                
                //refracting from outside = no attenuation, so no thresholding
                if(inside){
                  col = traceRay(refract_ray, depth-1, dummy, prev_kr);
                  colorC += col;
                }
                //refracting through inside means yes attenuation
                else{
                  //must pass attenuation threshold
                  double refract_max_mag = glm::length(glm::pow(m.kt(i), glm::dvec3(dummy.getT())) * prev_kr);
                  if(refract_max_mag > traceUI->getThreshold()){
                    col = traceRay(refract_ray, depth-1, dummy, glm::pow(m.kt(i), glm::dvec3(dummy.getT())) * prev_kr);
                    colorC += col * glm::pow(m.kt(i), glm::dvec3(dummy.getT()));
                  }
                }
            } else{
                //total internal reflection
                glm::dvec3 opp = - r.getDirection();
                glm::dvec3 reflect = 2 * glm::dot(opp, norm) * norm - opp;
			          ray reflectRay = ray(ray_pos, reflect, glm::dvec3(1, 1, 1), ray::REFLECTION);
                isect dummy;
                if(glm::length(m.kr(i)) != 0 && max_reflect_mag > traceUI->getThreshold())
			            colorC += m.kr(i) * traceRay(reflectRay, depth - 1, dummy, m.kr(i) * prev_kr);
            }
      } 
          
    } else{
      return colorC;
    }
  } else {
    // No intersection. This ray travels to infinity, so we color
    // it according to the background color, which in this (simple)
    // case is just black.
    //
    // FIXME: Add CubeMap support here.
    // TIPS: CubeMap object can be fetched from
    // traceUI->getCubeMap();
    //       Check traceUI->cubeMap() to see if cubeMap is loaded
    //       and enabled.
    if(traceUI->cubeMap()){
			return traceUI->getCubeMap()->getColor(r);
		}
    colorC = glm::dvec3(0.0, 0.0, 0.0);
  }
#if VERBOSE
  std::cerr << "== depth: " << depth + 1 << " done, returning: " << colorC
            << std::endl;
#endif
if(debugMode){
  //std::cout<<colorC<<std::endl;
}
  //std::cout<<colorC<<std::endl;
  return colorC;
}

//? probably not important
RayTracer::RayTracer()
    : scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0),
      m_bBufferReady(false) {
}

//? wtf is this
RayTracer::~RayTracer() {}

//? wtf is buffer
void RayTracer::getBuffer(unsigned char *&buf, int &w, int &h) {
  buf = buffer.data();
  w = buffer_width;
  h = buffer_height;
}

//probably not important
double RayTracer::aspectRatio() {
  return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char *fn) {
  ifstream ifs(fn);
  if (!ifs) {
        string msg("Error: couldn't read scene file ");
    msg.append(fn);
    traceUI->alert(msg);
    return false;
  }

  // Check if fn ends in '.ray'
  bool isRay = false;
  const char *ext = strrchr(fn, '.');
  if (ext && !strcmp(ext, ".ray"))
    isRay = true;

  // Strip off filename, leaving only the path:
  string path(fn);
  if (path.find_last_of("\\/") == string::npos)
    path = ".";
  else
    path = path.substr(0, path.find_last_of("\\/"));

  if (isRay) {
    // .ray Parsing Path
   if(animation){
    Tokenizer tokenizer(ifs, false, threads);
    Parser parser(tokenizer, path);
    try {
      scene.reset(parser.parseScene());
    } catch (SyntaxErrorException &pe) {
      traceUI->alert(pe.formattedMessage());
      return false;
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (TextureMapException e) {
      string msg("Texture mapping exception: ");
      msg.append(e.message());
      traceUI->alert(msg);
      return false;
    }
   }
  } else {
    // JSON Parsing Path
    try {
      JsonParser parser(path, ifs);
      scene.reset(parser.parseScene());
    } catch (ParserException &pe) {
      string msg("Parser: fatal exception ");
      msg.append(pe.message());
      traceUI->alert(msg);
      return false;
    } catch (const json::exception &je) {
      string msg("Invalid JSON encountered ");
      msg.append(je.what());
      traceUI->alert(msg);
      return false;
    }
  }

  //generate scene bvh here
  //must re-load scene before using switch
  if(traceUI->kdSwitch()){
    int bvh_depth = traceUI->getMaxDepth();
    int bvh_leaf_stop_size = traceUI->getLeafSize();
    this->scene->generate_bvh(bvh_depth, bvh_leaf_stop_size);
  }

  if (!sceneLoaded())
    return false;

  return true;
}

void RayTracer::traceSetup(int w, int h) {
  size_t newBufferSize = w * h * 3;
  if (newBufferSize != buffer.size()) {
    bufferSize = newBufferSize;
    buffer.resize(bufferSize);
  }
  buffer_width = w;
  buffer_height = h;
  std::fill(buffer.begin(), buffer.end(), 0);
  m_bBufferReady = true;

  /*
   * Sync with TraceUI
   */

  threads = traceUI->getThreads();
  block_size = traceUI->getBlockSize();
  thresh = traceUI->getThreshold();
  samples = traceUI->getSuperSamples();
  aaThresh = traceUI->getAaThreshold();

  // YOUR CODE HERE
  // FIXME: Additional initializations
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */

void RayTracer::traceImage(int w, int h) {
  //I am going to use this to test functions.
  //std::cout<<test_vector<<std::endl;

  // Always call traceSetup before rendering anything.
  traceSetup(w, h);

  //lambda function for trace across 
  auto lambda = [this](int startX, int endX) {
    for(int x = startX; x < endX; x++) {
      for(int y = 0; y < buffer_height; y++) {
        tracePixel(x, y);
      }
    }
  };

   int numThreads = std::min(threads, (unsigned int)w);
   int width = w / numThreads;

  std::vector<std::thread> threadPool;

  for(int i = 0; i < numThreads; i++) {
      int start = i * width;
      int end;
      if (i == numThreads - 1) {
          end = w;
      } else {
          end = (i + 1) * width;
      }
      threadPool.emplace_back(lambda, start, end);
    }

  for(auto& thread : threadPool) {
    thread.join();
  }




  // YOUR CODE HERE
  // FIXME: Start one or more threads for ray tracing
  //
  // TIPS: Ideally, the traceImage should be executed asynchronously,
  //       i.e. returns IMMEDIATELY after working threads are launched.
  //
  //       An asynchronous traceImage lets the GUI update your results
  //       while rendering.

}

int RayTracer::aaImage() {
  // YOUR CODE HERE
  // FIXME: Implement Anti-aliasing here
  //
  // TIP: samples and aaThresh have been synchronized with TraceUI by
  //      RayTracer::traceSetup() function
  return 0;
}

bool RayTracer::checkRender() {
  // YOUR CODE HERE
  // FIXME: Return true if tracing is done.
  //        This is a helper routine for GUI.
  //
  // TIPS: Introduce an array to track the status of each worker thread.
  //       This array is maintained by the worker threads.
  return true;
}

void RayTracer::waitRender() {
  // YOUR CODE HERE
  // FIXME: Wait until the rendering process is done.
  //        This function is essential if you are using an asynchronous
  //        traceImage implementation.
  //
  // TIPS: Join all worker threads here.
}


glm::dvec3 RayTracer::getPixel(int i, int j) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
  return glm::dvec3((double)pixel[0] / 255.0, (double)pixel[1] / 255.0,
                    (double)pixel[2] / 255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color) {
  unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;

  pixel[0] = (int)(255.0 * color[0]);
  pixel[1] = (int)(255.0 * color[1]);
  pixel[2] = (int)(255.0 * color[2]);
}
