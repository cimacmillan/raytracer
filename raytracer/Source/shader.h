#include <stdint.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include "raymath.h"
#include "kdtree.h"

using namespace std;
using glm::vec3;
using glm::mat3;

const int MONTE_CARLO_DEPTH = 1;
const int MONTE_CARLO_BREADTH = 16;
mat4 MONTE_CARLO_MATRIX;

const int REFRACTION_DEPTH = 8;
const int LIGHT_SAMPLES = 1;

const int PHOTON_SAMPLES = 200000;
const float distance_falloff = 2;
const float distance_multiplier = 20;
const float radiance_size = 0.04f;
const float light_mult = 50.f;
std::vector<Photon> photon_map;
int tree_size = 0;
kdtree* photon_tree;

// Node* photon_tree = NULL;
//

vec3 MainShader(Scene &scene, const Intersection& intersection, vec4 origin, vec4 direction, int reflect_depth, int monte_carlo_depth);

vec3 getCausticValues(Scene &scene, const Intersection& intersect) {

  struct kdres *presults;
  double pt[3] = { intersect.position.x, intersect.position.y, intersect.position.z};

  presults = kd_nearest_range( photon_tree, pt, radiance_size );
  float numberInRange = kd_res_size(presults);

  return light_mult * (vec3(1, 1, 1) + intersect.properties.color) * (numberInRange) / ((float)tree_size + 1);


  // for(int i = 0; i < photon_map.size(); i++) {
  //   float dis = distance(photon_map[i].position, intersect.position);
  //   float falloff = 4.0f * M_PI * pow(dis * distance_multiplier, distance_falloff);
  //
  //   sum += (photon_map[i].energy / falloff);
  // }
  //
  // vec3 pos = vec3(intersect.position);
  //
  // float best = distance(photon_tree->point, pos);
  // Node* nearest = nearest_neighbour(photon_tree, best, pos);
  //
  // float dis = distance(nearest->point, pos);
  //
  // return light_mult * (vec3(1, 1, 1) + intersect.properties.color) / (dis * dis) / (float)(tree_size + 1);

  // float count_within = 0;
  // for(int i = 0; i < photon_map.size(); i++) {
  //   float dis = distance(photon_map[i].position, intersect.position);
  //   if (dis < radiance_size) count_within++;
  // }
  //
  // return light_mult * (vec3(1, 1, 1) + intersect.properties.color) * (count_within) / ((float)photon_map.size() + 1);
}

bool isObscured(Scene &scene, vec4 start, vec4 direction, float target) {
  Intersection obscureTest;
  bool intersect = ClosestIntersection(start, direction, scene, obscureTest);
  if(!intersect) return false;
  return glm::distance(glm::vec3(obscureTest.position), glm::vec3(start)) < target;
}

vec3 monteCarloSample(float m) {

  float u = drand48();
  float v = drand48();

  float theta_a = acos(pow(1 - u, 1 / (1 + m)));
  float theta_b = 2 * M_PI * v;

  return vec3(
    sin(theta_a) * cos(theta_b),
    cos(theta_a),
    sin(theta_a) * sin(theta_b)
  );
}

vec3 InDirectLightingValues(Scene &scene, const Intersection& intersect, vec4 origin, vec4 direction, int reflect_depth, int monte_carlo_depth) {

  vec3 summed_colors = vec3(0, 0, 0);
  vec3 specular_colors = vec3(0, 0, 0);

  int breadth = MONTE_CARLO_BREADTH;

  vec3 Nt, Nb;
  createCoordinateSystem(vec3(intersect.normal), Nt, Nb);

  vec3 reflected = reflect(vec3(direction), vec3(intersect.normal));
  vec3 Ntr, Nbr;
  createCoordinateSystem(reflected, Ntr, Nbr);

  for(int i = 0; i < breadth; i++) {

    if(intersect.properties.material_diffuse > 0) {

        vec3 sample = monteCarloSample(1);
        vec4 newDirection = vec4(
            sample.x * Nb.x + sample.y * intersect.normal.x + sample.z * Nt.x,
            sample.x * Nb.y + sample.y * intersect.normal.y + sample.z * Nt.y,
            sample.x * Nb.z + sample.y * intersect.normal.z + sample.z * Nt.z,
            1);

        vec4 start = intersect.position + (newDirection * 0.0001f);

        Intersection indirectRay;
        bool intersect = ClosestIntersection(start, newDirection, scene, indirectRay);
        if(intersect) {
          vec3 indirectColor = MainShader(scene, indirectRay, start, newDirection, reflect_depth, monte_carlo_depth + 1);
          summed_colors += (indirectColor);
        }
    }

    if(intersect.properties.material_specular > 0) {
      vec3 sample = monteCarloSample(intersect.properties.material_shininess);
      vec4 newDirection = vec4(
          sample.x * Nbr.x + sample.y * reflected.x + sample.z * Ntr.x,
          sample.x * Nbr.y + sample.y * reflected.y + sample.z * Ntr.y,
          sample.x * Nbr.z + sample.y * reflected.z + sample.z * Ntr.z,
          1);

      vec4 start = intersect.position + (newDirection * 0.0001f);

      Intersection indirectRay;
      bool intersect = ClosestIntersection(start, newDirection, scene, indirectRay);
      if(intersect) {
        vec3 indirectColor = MainShader(scene, indirectRay, start, newDirection, reflect_depth, monte_carlo_depth + 1);
        specular_colors += (indirectColor);
      }
    }

  }

  if(MONTE_CARLO_BREADTH == 0) return summed_colors;

  summed_colors = summed_colors * intersect.properties.material_diffuse;
  specular_colors = specular_colors * intersect.properties.material_specular;

  return ((summed_colors + specular_colors) / ((float)breadth));

}

vec3 DirectLightingValues(Scene &scene, const Intersection& i, vec4 origin, PointLight light) {

  vec3 difference = vec3(light.lightPos - i.position);
  float distance = length(difference);
  vec3 ambient = i.properties.color * light.component_ambient * i.properties.material_ambient;

  // Intersection shadowTest;
  const float offset = 0.0001f;
  vec4 start = i.position + (offset * i.normal);

  int light_samples = 0;

  for (int light_x = 0; light_x < LIGHT_SAMPLES; light_x++) {

    float grad_x = (((float)light_x) / ((float)LIGHT_SAMPLES)) - 0.5f;

    vec4 light_difference = vec4(difference, 1) + (light.plane_a * grad_x);

    for (int light_y = 0; light_y < LIGHT_SAMPLES; light_y++) {

        float grad_y = (((float)light_y) / ((float)LIGHT_SAMPLES)) - 0.5f;

        vec4 light_difference_x = light_difference + (light.plane_b * grad_y);

        if(!isObscured(scene, start, light_difference_x, distance)) light_samples++;
    }
  }
  if(light_samples == 0) {
    return ambient;
  }

  float multiplier = ((float)light_samples) / ((float) LIGHT_SAMPLES * LIGHT_SAMPLES);

  vec3 normal = vec3(i.normal);
  float dotProduct = max(dot(normal, difference) / (length(normal) * length(difference)), 0.f);
  vec3 camera_difference = vec3(i.position - origin);
  vec3 reflected = reflect(camera_difference, normal);
  float specularDotProduct = max(dot(difference, reflected) / (length(difference) * length(reflected)), 0.f);

  vec3 diffuse = dotProduct * i.properties.color * light.color * i.properties.material_diffuse * light.component_diffuse;
  vec3 specular = pow(specularDotProduct, i.properties.material_shininess) * light.color * i.properties.material_specular * light.component_specular;
  float attenuation = (((light.attenuation.z * distance) + light.attenuation.y) * distance) + light.attenuation.x;

  return (((diffuse + specular) * multiplier) / attenuation) + ambient;

}

vec3 MainShader(Scene &scene, const Intersection& intersection, vec4 origin, vec4 direction, int reflect_depth, int monte_carlo_depth) {

  vec3 color = vec3(0, 0, 0);

  for (int i = 0; i < (int)scene.scene_lights.size(); i++) {
    color += DirectLightingValues(scene, intersection, origin, scene.scene_lights[i]);
  }

  if(monte_carlo_depth < MONTE_CARLO_DEPTH) {
    color += ((InDirectLightingValues(scene, intersection, origin, direction, reflect_depth, monte_carlo_depth)));
  }

  if(reflect_depth < REFRACTION_DEPTH && (intersection.properties.reflectance > 0 || intersection.properties.refractance > 0)) {
    vec3 normal = vec3(intersection.normal);

    if(intersection.properties.reflectance > 0) {
        float offset = 0.0001f;
        vec3 reflected = reflect(vec3(direction), normal);
        vec4 start = intersection.position + (offset * intersection.normal);
        Intersection reflected_ray;
        bool intersect = ClosestIntersection(start, vec4(reflected, 1), scene, reflected_ray);
        if(intersect) {
          vec3 reflectColor = MainShader(scene, reflected_ray, start, vec4(reflected, 1), reflect_depth + 1, monte_carlo_depth);
          color += reflectColor * intersection.properties.reflectance;
        }
    }

    if(intersection.properties.refractance > 0) {

        float k = fresnel(vec3(direction), normal, intersection.properties.refractive_index);
        float facing = dot(direction, intersection.normal);
        float offset = facing > 1 ? 0.0001f : -0.0001f;
        vec4 start_refract = intersection.position + (offset * intersection.normal);
        vec4 start_reflect = intersection.position + ((-offset) * intersection.normal);
        vec3 reflected = reflect(vec3(direction), normal);
        vec3 refracted = myRefract(vec3(direction), normal, intersection.properties.refractive_index);

        float reflectRatio = k;
        float refractRatio = 1 - k;

        vec3 refract_color = vec3(0, 0, 0);

          //Total internal reflection
        Intersection reflected_ray;
        bool intersect = ClosestIntersection(start_reflect, vec4(reflected, 1), scene, reflected_ray);
        if(intersect) refract_color += (MainShader(scene, reflected_ray, start_reflect, vec4(reflected, 1), reflect_depth + 1, monte_carlo_depth) * reflectRatio);

        Intersection refracted_ray;
        intersect = ClosestIntersection(start_refract, vec4(refracted, 1), scene, refracted_ray);
        if(intersect) refract_color += (MainShader(scene, refracted_ray, start_refract, vec4(refracted, 1), reflect_depth + 1, monte_carlo_depth) * refractRatio);

        color += (refract_color * intersection.properties.refractance);

    }


  }

  if(monte_carlo_depth == 0 && reflect_depth == 0) {
    color += getCausticValues(scene, intersection);
  }

  return color;

}


Photon PropogatePhoton(Scene &scene, const Intersection& i, vec4 origin, vec4 direction, int depth) {

  if (depth >= REFRACTION_DEPTH) return Photon(vec3(1, 1, 1), i.position);

  vec3 normal = vec3(i.normal);

  if(i.properties.reflectance > 0) {
      float offset = 0.0001f;
      vec3 reflected = reflect(vec3(direction), normal);
      vec4 start = i.position + (offset * i.normal);
      Intersection reflected_ray;
      bool intersect = ClosestIntersection(start, vec4(reflected, 1), scene, reflected_ray);
      if(intersect) {
        return PropogatePhoton(scene, reflected_ray, start, vec4(reflected, 1), depth + 1);
      }
  }

  if(i.properties.refractance > 0) {


      float facing = dot(direction, i.normal);
      float offset = facing > 1 ? 0.0001f : -0.0001f;
      vec4 start_refract = i.position + (offset * i.normal);
      vec4 start_reflect = i.position + ((-offset) * i.normal);
      vec3 reflected = reflect(vec3(direction), normal);
      vec3 refracted = myRefract(vec3(direction), normal, i.properties.refractive_index);

      vec3 refract_color = vec3(0, 0, 0);

        //Total internal reflection
      Intersection reflected_ray;
      bool intersect = ClosestIntersection(start_reflect, vec4(reflected, 1), scene, reflected_ray);
      if(intersect) {
        //return PropogatePhoton(scene, reflected_ray, start_reflect, vec4(reflected, 1), depth + 1);
      }

      Intersection refracted_ray;
      intersect = ClosestIntersection(start_refract, vec4(refracted, 1), scene, refracted_ray);
      if(intersect) {
        return PropogatePhoton(scene, refracted_ray, start_refract, vec4(refracted, 1), depth + 1);
      }

  }

  return Photon(vec3(1, 1, 1), i.position);
}

vec3 Shade(Scene &scene, const Intersection& i, vec4 origin, vec4 direction) {
  return MainShader(scene, i, origin, direction, 0, 0);
}



void ConstructPhotonMap(Scene &scene) {

  photon_tree = kd_create(3);

  vec3 normal_down = vec3(0, 1, 0);
  vec3 normal_down_Nt;
  vec3 normal_down_Nb;
  createCoordinateSystem(normal_down, normal_down_Nt, normal_down_Nb);

  for(int i = 0; i < (int)scene.scene_lights.size(); i++){
    PointLight light = scene.scene_lights[i];
    vec4 start = light.lightPos;

    for(int s = 0; s < PHOTON_SAMPLES; s++) {

        vec3 sample = monteCarloSample(2);
        vec4 direction = vec4(
            sample.x * normal_down_Nb.x + sample.y * normal_down.x + sample.z * normal_down_Nt.x,
            sample.x * normal_down_Nb.y + sample.y * normal_down.y + sample.z * normal_down_Nt.y,
            sample.x * normal_down_Nb.z + sample.y * normal_down.z + sample.z * normal_down_Nt.z,
            1);

        Intersection photon;
        if(ClosestIntersection(start, direction, scene, photon)) {
          //if(photon.properties.refractance > 0 || photon.properties.reflectance > 0){
          if(photon.properties.refractance > 0){

            Photon p = PropogatePhoton(scene, photon, start, direction, 0);
            // photon_map.push_back(PropogatePhoton(scene, photon, start, direction));
            // photon_tree = insert(photon_tree, vec3(p.position));
            kd_insert3( photon_tree, p.position.x, p.position.y, p.position.z, 0);
            tree_size++;
          }
        }


    }

  }

  printf("Photon map size: %d\n", tree_size);

}
