#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>
#include "shader.h"


// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel( std::vector<Triangle>& triangles )
{
	using glm::vec3;
	using glm::vec4;

	// Defines colors:
	vec3 red(    0.75f, 0.15f, 0.15f );
	vec3 yellow( 0.75f, 0.75f, 0.15f );
	vec3 green(  0.15f, 0.75f, 0.15f );
	vec3 cyan(   0.15f, 0.75f, 0.75f );
	vec3 blue(   0.15f, 0.15f, 0.75f );
	vec3 purple( 0.75f, 0.15f, 0.75f );
	vec3 white(  0.75f, 0.75f, 0.75f );


	ShaderProperties floor(green, 				0.2, 0.1,   1, 20, 0, 0, 1);
	ShaderProperties left_wall(purple, 		0.2, 1,     0, 1, 0, 0, 1);
	ShaderProperties right_wall(yellow, 	0, 0.2,     0.8, 2, 0, 0, 1);
	ShaderProperties ceiling(cyan, 				0.2, 0.5, 0.5, 20, 0, 0, 1);
	ShaderProperties back_wall(white, 		0.2, 0.5, 0.5, 8, 0, 0, 1);
	ShaderProperties short_block(red, 		0.2, 0,     1, 8, 0, 0, 1);
	ShaderProperties tall_block(blue, 		0.2, 1,     0, 20, 0, 0, 1);


	triangles.clear();
	triangles.reserve( 5*2*3 );

	// ---------------------------------------------------------------------------
	// Room

	float L = 555;			// Length of Cornell Box side.

	vec4 A(L,0,0,1);
	vec4 B(0,0,0,1);
	vec4 C(L,0,L,1);
	vec4 D(0,0,L,1);

	vec4 E(L,L,0,1);
	vec4 F(0,L,0,1);
	vec4 G(L,L,L,1);
	vec4 H(0,L,L,1);

	// Floor:
	triangles.push_back( Triangle( C, B, A, floor ) );
	triangles.push_back( Triangle( C, D, B, floor ) );

	// Left wall
	triangles.push_back( Triangle( A, E, C, left_wall ) );
	triangles.push_back( Triangle( C, E, G, left_wall ) );

	// Right wall
	triangles.push_back( Triangle( F, B, D, right_wall ) );
	triangles.push_back( Triangle( H, F, D, right_wall ) );

	// Ceiling
	triangles.push_back( Triangle( E, F, G, ceiling ) );
	triangles.push_back( Triangle( F, H, G, ceiling ) );

	// Back wall
	triangles.push_back( Triangle( G, D, C, back_wall ) );
	triangles.push_back( Triangle( G, H, D, back_wall ) );

	// ---------------------------------------------------------------------------
	// Short block

	A = vec4(290,0,114,1);
	B = vec4(130,0, 65,1);
	C = vec4(240,0,272,1);
	D = vec4( 82,0,225,1);

	E = vec4(290,165,114,1);
	F = vec4(130,165, 65,1);
	G = vec4(240,165,272,1);
	H = vec4( 82,165,225,1);

	// Front
	triangles.push_back( Triangle(E,B,A,short_block) );
	triangles.push_back( Triangle(E,F,B,short_block) );

	// Front
	triangles.push_back( Triangle(F,D,B,short_block) );
	triangles.push_back( Triangle(F,H,D,short_block) );

	// BACK
	triangles.push_back( Triangle(H,C,D,short_block) );
	triangles.push_back( Triangle(H,G,C,short_block) );

	// LEFT
	triangles.push_back( Triangle(G,E,C,short_block) );
	triangles.push_back( Triangle(E,A,C,short_block) );

	// TOP
	triangles.push_back( Triangle(G,F,E,short_block) );
	triangles.push_back( Triangle(G,H,F,short_block) );

	// ---------------------------------------------------------------------------
	// Tall block

	A = vec4(423,0,247,1);
	B = vec4(265,0,296,1);
	C = vec4(472,0,406,1);
	D = vec4(314,0,456,1);

	E = vec4(423,330,247,1);
	F = vec4(265,330,296,1);
	G = vec4(472,330,406,1);
	H = vec4(314,330,456,1);

	// Front
	triangles.push_back( Triangle(E,B,A,tall_block) );
	triangles.push_back( Triangle(E,F,B,tall_block) );

	// Front
	triangles.push_back( Triangle(F,D,B,tall_block) );
	triangles.push_back( Triangle(F,H,D,tall_block) );

	// BACK
	triangles.push_back( Triangle(H,C,D,tall_block) );
	triangles.push_back( Triangle(H,G,C,tall_block) );

	// LEFT
	triangles.push_back( Triangle(G,E,C,tall_block) );
	triangles.push_back( Triangle(E,A,C,tall_block) );

	// TOP
	triangles.push_back( Triangle(G,F,E,tall_block) );
	triangles.push_back( Triangle(G,H,F,tall_block) );


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for( size_t i=0; i<triangles.size(); ++i )
	{
		triangles[i].v0 *= 2/L;
		triangles[i].v1 *= 2/L;
		triangles[i].v2 *= 2/L;

		triangles[i].v0 -= vec4(1,1,1,1);
		triangles[i].v1 -= vec4(1,1,1,1);
		triangles[i].v2 -= vec4(1,1,1,1);

		triangles[i].v0.x *= -1;
		triangles[i].v1.x *= -1;
		triangles[i].v2.x *= -1;

		triangles[i].v0.y *= -1;
		triangles[i].v1.y *= -1;
		triangles[i].v2.y *= -1;

		triangles[i].v0.w = 1.0;
		triangles[i].v1.w = 1.0;
		triangles[i].v2.w = 1.0;

		triangles[i].ComputeNormal();
	}
}

#endif
