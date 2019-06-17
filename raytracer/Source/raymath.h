#include <glm/glm.hpp>
#include <stdbool.h>

#include "geometry.h"

using namespace std;
using glm::vec4;
using glm::mat4;
using glm::vec3;
using glm::mat3;

struct Intersection
{
  vec4 position;
  vec4 normal;
  float distance;
  ShaderProperties properties;
};

void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb)
{
    if (std::fabs(N.x) > std::fabs(N.y))
        Nt = vec3(N.z, 0, -N.x) / sqrtf(N.x * N.x + N.z * N.z);
    else
        Nt = vec3(0, -N.z, N.y) / sqrtf(N.y * N.y + N.z * N.z);
    Nb = cross(N, Nt);
}

float clamp(float a, float b, float c) {
  if(c < a) return a;
  if(c > b) return b;
  return c;
}

float fresnel(const vec3 &I, const vec3 &N, const float &ior)
{
    float cosi = clamp(-1, 1, dot(I, N));
    float etai = 1, etat = ior;
    float kr;
    if (cosi > 0) { std::swap(etai, etat); }
    // Compute sini using Snell's law
    float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
    // Total internal reflection
    if (sint >= 1) {
        kr = 1;
    }
    else {
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
    return kr;
}

vec3 myRefract(const vec3 &I, const vec3 &N, const float &ior)
{
  float cosi = clamp(-1, 1, dot(I, N));
  float etai = 1, etat = ior;
  vec3 n = N;
  if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
  float eta = etai / etat;
  float k = 1 - eta * eta * (1 - cosi * cosi);
  //return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
  return eta * I + (eta * cosi - sqrtf(k)) * n;
}

bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
            -0.5 * (b + sqrt(discr)) :
            -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);

    return true;
}

bool getIntersectionSphere (vec4 s, vec4 d, Sphere sphere, Intersection& intersection) {
  // analytic solution
  vec3 origin = vec3(sphere.origin);
  vec3 L = vec3(s) - origin;
  vec3 dir = vec3(d);

  float a = dot(dir, dir);
  float b = 2 * dot(dir, L);
  float c = dot(L, L) - sphere.radius2;
  float t0, t1, t;
  if (!solveQuadratic(a, b, c, t0, t1)) return false;

  if (t0 > t1) std::swap(t0, t1);

  if (t0 < 0) {
      t0 = t1; // if t0 is negative, let's use t1 instead
      if (t0 < 0) return false; // both t0 and t1 are negative
  }

  t = t0;

  vec3 position = vec3(s + (d * t));
  vec3 normal = glm::normalize(position - origin);

  intersection.position = vec4(position, 1);
  intersection.normal = vec4(normal, 1);
  intersection.distance = t;
  intersection.properties = sphere.properties;

  return true;
}

bool getIntersectionTriangle(vec4 s, vec4 d, Triangle triangle, Intersection& intersection) {


  vec3 v0 = vec3(triangle.v0);
  vec3 v1 = vec3(triangle.v1);
  vec3 v2 = vec3(triangle.v2);
  vec3 e1 = v1 - v0;
  vec3 e2 = v2 - v0;
  vec3 b = vec3(s) - v0;
  mat3 A( -vec3(d), e1, e2 );

  //Could be a faster way around this
  float detA = glm::determinant(A);
  float detA1 = glm::determinant(mat3(b, A[1], A[2]));
  float detA2 = glm::determinant(mat3(A[0], b, A[2]));
  float detA3 = glm::determinant(mat3(A[0], A[1], b));
  //vec3 x = glm::inverse( A ) * b;
  vec3 x = vec3(detA1, detA2, detA3) / detA; //Cramers Rule

  float ray_length = x.x;
  float u_coord = x.y;
  float v_coord = x.z;

  //Intersection test
  if(ray_length > 0 && u_coord > 0 && v_coord > 0 && (u_coord + v_coord) < 1) {

    vec3 position = v0 + (u_coord * e1) + (v_coord * e2);

    intersection.position = vec4(position, 1.0);
    intersection.normal = triangle.normal;
    intersection.distance = ray_length;
    intersection.properties = triangle.properties;

    return true;
  }

  return false;
}

bool ClosestIntersection(vec4 s, vec4 d, Scene &scene, Intersection& closestIntersection) {


  closestIntersection.distance = -1;

  for (long unsigned int i = 0; i < scene.scene_triangles.size(); i++){

    Intersection intersection;
    Triangle triangle = scene.scene_triangles[i];
    if(getIntersectionTriangle(s, d, triangle, intersection)) {
      if((closestIntersection.distance < 0 || intersection.distance < closestIntersection.distance)) {
        closestIntersection = intersection;
      }
    }
  }

  for (long unsigned int i = 0; i < scene.scene_spheres.size(); i++){

    Intersection intersection;
    Sphere sphere = scene.scene_spheres[i];
    if(getIntersectionSphere(s, d, sphere, intersection)) {
      if((closestIntersection.distance < 0 || intersection.distance < closestIntersection.distance)) {
        closestIntersection = intersection;
      }
    }
  }

  return (closestIntersection.distance > 0);
}

bool anIntersection(vec4 s, vec4 d, Scene &scene, Intersection& closestIntersection) {
  for (long unsigned int i = 0; i < scene.scene_triangles.size(); i++){

    Intersection intersection;
    Triangle triangle = scene.scene_triangles[i];
    if(getIntersectionTriangle(s, d, triangle, intersection)) {
      return true;
    }
  }

  for (long unsigned int i = 0; i < scene.scene_spheres.size(); i++){

    Intersection intersection;
    Sphere sphere = scene.scene_spheres[i];
    if(getIntersectionSphere(s, d, sphere, intersection)) {
      return true;
    }
  }

  return false;
}

void InterpolateVector( vec3 a, vec3 b, vector<vec3>& result ) {

  for (int i = 0; i < (int)result.size(); i++) {

    float grad = ((float)i) / ((float)result.size() - 1);

    result[i].x = ((1 - grad) * a.x) + (grad * b.x);
    result[i].y = ((1 - grad) * a.y) + (grad * b.y);
    result[i].z = ((1 - grad) * a.z) + (grad * b.z);

  }

}

float InterpolateFloat(float a, float b, float grad) {
  return ((1 - grad) * a) + (grad * b);
}

float RandomFloat() {
    return float(rand()) / float(RAND_MAX);
}

void UpdateRotationMatrix(float pitch, float yaw, float roll, mat4& target) {
  mat4 pitchMatrix = mat4(
    vec4(1, 0, 0, 0),
    vec4(0, cos(pitch), -sin(pitch), 0),
    vec4(0, sin(pitch), cos(pitch), 0),
    vec4(0, 0, 0, 1)
  );
  mat4 yawMatrix = mat4(
    vec4(cos(yaw), 0, sin(yaw), 0),
    vec4(0, 1, 0, 0),
    vec4(-sin(yaw), 0, cos(yaw), 0),
    vec4(0, 0, 0, 1)
  );
  mat4 rollMatrix = mat4(
    vec4(cos(roll), -sin(roll), 0, 0),
    vec4(sin(roll), cos(roll), 0, 0),
    vec4(0, 0, 1, 0),
    vec4(0, 0, 0, 1)
  );

  target = pitchMatrix * yawMatrix * rollMatrix;
}
