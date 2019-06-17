#define RENDER_SCREEN 1

#include <iostream>
#include <glm/glm.hpp>

#if RENDER_SCREEN
#include <SDL.h>
#include "SDLauxiliary.h"
#endif

#include "TestModelH.h"
#include "lodepng.h"
#include <stdint.h>
#include <omp.h>

using namespace std;
using glm::vec3;
using glm::mat3;

// #define SCREEN_WIDTH 160
// #define SCREEN_HEIGHT 120
// #define SCREEN_WIDTH 320
// #define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FULLSCREEN_MODE false

#define DRAW_WIDTH 16
#define DRAW_HEIGHT 12

#define ANTI_ALIASING 1

struct png_obj {
  uint8_t* png_buffer;
};

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES
//System                                                    */
int t;
bool running = true;

//Object
Scene scene;
// vector<Triangle> triangles;

//Camera
float yaw = 0, pitch = 0, roll = 0;
mat4 rotationMatrix;
vec4 cameraPos(0, 0, -1.8, 1.0);
float f = 1.0;

int draw_x = 0, draw_y = 0;
/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

#if (!RENDER_SCREEN)
typedef struct screen;
#endif

void Init();
void Update();
void Draw(screen* screen);

int main( int argc, char* argv[] )
{

#if RENDER_SCREEN

    screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE, WINDOW_WIDTH, WINDOW_HEIGHT);
    t = SDL_GetTicks();	/*Set start value for timer.*/
    Init();

    //Clear the screen
    memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));

    while(running) {
      Draw(screen);
      Update();
      SDL_Renderframe(screen);
    }

    SDL_SaveImage( screen, "screenshot.bmp" );

    KillSDL(screen);
    return 0;

#endif
    Init();
    Draw(NULL);
}

// #define STAR_COUNT 1000
// vector<vec3> stars( STAR_COUNT );

void Init() {

  vector<Triangle> triangles;
  LoadTestModel(triangles);
  scene.scene_triangles.insert(scene.scene_triangles.end(), triangles.begin(), triangles.end());
  injectCustom(scene);

  vec4 sum = vec4(0, 0, 0, 0);
  for (int i = 0; i < (int)triangles.size(); i++) {
    Triangle triangle = triangles[i];
    sum += ((triangle.v0 + triangle.v1 + triangle.v2) / 3.0f);
  }
  vec4 center = sum / ((float)triangles.size());
  cameraPos.x = center.x;
  cameraPos.y = center.y;

  printf("Contrusting Photon Map \n");
  ConstructPhotonMap(scene);
  printf("Constructed\n");

}

float max(float a, float b) {
  if (a > b) {
    return a;
  }
  return b;
}

void PutPixelBCP(png_obj* png, int x, int y, glm::vec3 colour)
{
  if(x<0 || x>=SCREEN_WIDTH || y<0 || y>=SCREEN_HEIGHT)
    {
      std::cout << "apa" << std::endl;
      return;
    }
  uint32_t r = uint32_t( glm::clamp( 255*colour.r, 0.f, 255.f ) );
  uint32_t g = uint32_t( glm::clamp( 255*colour.g, 0.f, 255.f ) );
  uint32_t b = uint32_t( glm::clamp( 255*colour.b, 0.f, 255.f ) );

  int NewPos = (y * SCREEN_WIDTH + x) * 4;

  png->png_buffer[NewPos + 0] = r; //B is offset 2
  png->png_buffer[NewPos + 1] = g; //G is offset 1
  png->png_buffer[NewPos + 2] = b; //R is offset 0
  png->png_buffer[NewPos + 3] = 0xFF; //A is offset 3
}


/*Place your drawing here*/
void Draw(screen* screen)
{

  float aspect_ratio = ((float)SCREEN_HEIGHT) / ((float)SCREEN_WIDTH);
  const float samples = ANTI_ALIASING * ANTI_ALIASING;
  const float x_change = (2.0 / ((float)SCREEN_WIDTH)) / ANTI_ALIASING;
  const float y_change = ((2.0 / ((float)SCREEN_HEIGHT)) / ANTI_ALIASING) * aspect_ratio;

  int draw_height = DRAW_HEIGHT;
  int draw_width = DRAW_WIDTH;


#if (!RENDER_SCREEN)
  draw_height = SCREEN_HEIGHT;
  draw_width = SCREEN_WIDTH;
  png_obj png;
  png.png_buffer = (uint8_t*)malloc(sizeof(uint8_t) * SCREEN_WIDTH * SCREEN_HEIGHT * 4);
#endif

  #pragma omp parallel for
  for (int y = draw_y; y < draw_y + draw_height; y++) {

      float yDir = (((2 * y) / ((float)SCREEN_HEIGHT)) - 1.0) * aspect_ratio;

      #pragma omp simd
      for (int x = draw_x; x < draw_x + draw_width; x++) {

        float xDir = ((2 * x) / ((float)SCREEN_WIDTH)) - 1.0;

        vec3 colour = vec3(0, 0, 0);
        for(float xA = 0; xA < ANTI_ALIASING; xA++) {
          for(float yA = 0; yA < ANTI_ALIASING; yA++) {
            vec4 direction = rotationMatrix * vec4(xDir + (xA*x_change), yDir + (yA*y_change), f, 1.0);
            Intersection closest;
            bool doesIntersect = ClosestIntersection(cameraPos, direction, scene, closest);
            if(doesIntersect) {
              colour += Shade(scene, closest, cameraPos, direction);
            }
          }
        }

#if RENDER_SCREEN
          PutPixelSDL(screen, x, y, (colour / samples));
#endif

#if (!RENDER_SCREEN)
          PutPixelBCP(&png, x, y, (colour / samples));
#endif

        // vec4 direction = rotationMatrix * vec4(xDir, yDir, f, 1.0);
        //
        // Intersection closest;
        // bool doesIntersect = ClosestIntersection(cameraPos, direction, scene, closest);
        //
        // if (doesIntersect) {
        //   PutPixelSDL(screen, x, y, Shade(scene, closest, cameraPos, direction));
        // }

    }
  }

#if (!RENDER_SCREEN)
  std::vector<std::uint8_t> ImageBuffer;
  lodepng::encode(ImageBuffer, png_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
  lodepng::save_file(ImageBuffer, "render_64.png");
#endif

#if RENDER_SCREEN
  draw_x += DRAW_WIDTH;
  if(draw_x >= SCREEN_WIDTH) {
    draw_x = 0;
    draw_y += DRAW_HEIGHT;
  }
  if(draw_y >= SCREEN_HEIGHT) {
    draw_y = 0;
    int t2 = SDL_GetTicks();
    float dt = float(t2-t);
    t = t2;

    printf("Frame time: %f \n", dt);
  }
#endif

}

void updateRotationMatrix(){

  UpdateRotationMatrix(pitch, yaw, roll, rotationMatrix);

}

#if RENDER_SCREEN

/*Place updates of parameters here*/
void Update()
{
  /* Compute frame time */

  SDL_Event e;
  while (SDL_PollEvent(&e)) {

    float speed = 0.04f;
    float rotationSpeed = 0.04f;

    if(e.type == SDL_KEYDOWN) {

      switch(e.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;

        case SDLK_i:
          cameraPos.z += speed;
            break;

        case SDLK_k:
          cameraPos.z -= speed;
          break;

        case SDLK_j:
          cameraPos.x += speed;
          break;

        case SDLK_l:
          cameraPos.x -= speed;
          break;

        case SDLK_u:
          cameraPos.y += speed;
          break;

        case SDLK_m:
          cameraPos.y -= speed;
          break;

        case SDLK_LEFT:
          yaw -= rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_RIGHT:
          yaw += rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_UP:
          pitch -= rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_DOWN:
          pitch += rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_o:
          roll -= rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_p:
          roll += rotationSpeed;
          updateRotationMatrix();
          break;

        case SDLK_w:
          scene.scene_lights[0].lightPos.z += speed;
          break;

        case SDLK_s:
          scene.scene_lights[0].lightPos.z -= speed;
          break;

        case SDLK_d:
          scene.scene_lights[0].lightPos.x += speed;
          break;

        case SDLK_a:
          scene.scene_lights[0].lightPos.x -= speed;
          break;

        case SDLK_z:
          scene.scene_lights[0].lightPos.y += speed;
          break;

        case SDLK_q:
          scene.scene_lights[0].lightPos.y -= speed;
          break;

      }

    }

    if( e.type == SDL_QUIT )
    {
      running = false;
    }

  }
}
#endif
