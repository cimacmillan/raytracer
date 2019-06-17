
#include <glm/glm.hpp>
#include <vector>

using glm::vec4;
using glm::vec3;

class Photon {
public:
	vec3 energy;
	vec4 position;

	Photon(vec3 energy, vec4 position)
	: energy(energy), position(position)
	{

	}

};

class PointLight {
public:
	vec4 lightPos;
	vec3 color;
	float component_ambient;
	float component_diffuse;
	float component_specular;
	vec3 attenuation;
	vec4 plane_a, plane_b;

	PointLight(vec4 lightPos, vec3 color, float component_ambient, float component_diffuse, float component_specular, vec3 attenuation, vec4 plane_a, vec4 plane_b)
		: lightPos(lightPos), color(color), component_ambient(component_ambient), component_diffuse(component_diffuse), component_specular(component_specular), attenuation(attenuation), plane_a(plane_a), plane_b(plane_b)
	{

	}

};

class ShaderProperties {
public:
	glm::vec3 color;
	float material_ambient;
	float material_diffuse;
	float material_specular;
	float material_shininess;
	float reflectance;
	float refractance;
	float refractive_index;

  ShaderProperties(glm::vec3 color, float material_ambient, float material_diffuse, float material_specular, float material_shininess, float reflectance, float refractance, float refractive_index)
    : color(color), material_ambient(material_ambient), material_diffuse(material_diffuse), material_specular(material_specular), material_shininess(material_shininess), reflectance(reflectance), refractance(refractance), refractive_index(refractive_index)
  {

  }

  ShaderProperties(){}

};

// Used to describe a triangular surface:
class Triangle
{
public:
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec4 normal;
	ShaderProperties properties;

	Triangle( glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, ShaderProperties properties )
		: v0(v0), v1(v1), v2(v2), properties(properties)
	{
		ComputeNormal();
	}

	void ComputeNormal()
	{
	  glm::vec3 e1 = glm::vec3(v1.x-v0.x,v1.y-v0.y,v1.z-v0.z);
	  glm::vec3 e2 = glm::vec3(v2.x-v0.x,v2.y-v0.y,v2.z-v0.z);
	  glm::vec3 normal3 = glm::normalize( glm::cross( e2, e1 ) );
	  normal.x = normal3.x;
	  normal.y = normal3.y;
	  normal.z = normal3.z;
	  normal.w = 1.0;
	}
};

class Sphere
{
public:
    glm::vec4 origin;
    float radius;
    float radius2;
    ShaderProperties properties;

    Sphere(glm::vec4 origin, float radius, ShaderProperties properties)
      : origin(origin), radius(radius), properties(properties)
    {
      radius2 = radius * radius;
    }

};

class Scene {
public:
	std::vector<Triangle> scene_triangles;
	std::vector<Sphere> scene_spheres;
	std::vector<PointLight> scene_lights;
};

void injectCustom(Scene &scene) {

	// ShaderProperties(color, material_ambient, material_diffuse, material_specular, material_shininess, reflectance, refractance, refractive_index)


	scene.scene_spheres.push_back(
		Sphere(vec4(0, 0, -0.7, 1), 0.2f, ShaderProperties(vec3(1, 1, 1), 0.01f, 0.f, 0.f, 8, 0.f, 1.0f, 1.5f))
	);

	scene.scene_spheres.push_back(
		Sphere(vec4(0.4, -0.2, 0, 1), 0.15f, ShaderProperties(vec3(1.0f, 1.0f, 1.0f), 0.1, 0.1, 1.f, 10.0f, 0.9f, 0, 1))
	);

	float light_size = 0.1f;
	scene.scene_lights.push_back(
		PointLight(vec4( 0, -0.5, -0.7, 1.0), vec3( 1, 1, 1 ), 0.1f, 14.0f, 14.0f, vec3(0, 0, 12.5), vec4(light_size, 0, 0, 1.0), vec4(0, 0, light_size, 1.0))
	);
	// scene.scene_lights.push_back(
	// 	PointLight(vec4( -0.7, 0.7, -0.7, 1.0), vec3( 1, 0, 0 ), 0.1f, 7.0f, 7.0f, vec3(0, 0, 12.5), vec4(light_size, 0, 0, 1.0), vec4(0, 0, light_size, 1.0))
	// );

	// scene.scene_lights.push_back(
	// 	PointLight(vec4( 0.7, 0.5, -0.7, 1.0), vec3( 1, 0.1, 0.2 ), 0.1f, 4.0f, 0.2f, vec3(0, 0, 32.0))
	// );

	// scene.scene_lights.push_back(
	// 	PointLight(vec4( 0, 0.5, -0.7, 1.0), vec3( 0, 0, 0 ), 0.5f, 14.0f, 14.0f, vec3(0, 0, 12.0))
	// );

	// scene.scene_lights.push_back(
	// 	PointLight(vec4( -0.8, -0.5, -0.7, 1.0), vec3( 1, 1, 1 ), 0.5f, 14.0f, 14.0f, vec3(0, 0, 12.0))
	// );

}
