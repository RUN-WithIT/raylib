#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

#define GL_SILENCE_DEPRECATION
#include "rlgl.h"
#if defined (__APPLE__)
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include "glad.h"
#endif
#include "raymath.h"

#if defined (PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else
#define GLSL_VERSION            100
#endif

#define VERTEX_COUNT 10
#define ORTHO_VERTEX_COUNT VERTEX_COUNT * 3

Vector3 vertices[VERTEX_COUNT];
Vector3 vertices_ortho [ORTHO_VERTEX_COUNT];

Shader shader;
GLuint vao;
GLuint vbo;

void
draw ()
{
  Matrix mvp = { 0 };

  rlDrawRenderBatchActive ();
  glUseProgram (shader.id);

  mvp = MatrixMultiply (rlGetMatrixModelview (), rlGetMatrixProjection ());
  glUniformMatrix4fv (shader.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat (mvp));

  glBindVertexArray (vao);
  //glDrawArrays (GL_TRIANGLE_STRIP, 0, ORTHO_VERTEX_COUNT);
  //glDrawArrays (GL_LINES, 0, ORTHO_VERTEX_COUNT);
  glDrawArrays (GL_TRIANGLES, 0, ORTHO_VERTEX_COUNT);
  glBindVertexArray (0);

  glUseProgram (0);
}

void
ortho (Vector2 p0, Vector2 p1, Vector2 *p2, Vector2 *p3, float line_width, int debug)    
{
  Vector2 offset = { p0.x - 0, p0.y - 0 };
  int left = 0;
  float
    dy = 0, dx = 0,
    distance = 0, h = 0,
    m = 0, b = 0,
    a = 0, _b = 0, c = 0,
    x = 0, y = 0;

  if (debug)
    printf ("p0 { %f, %f } p1 { %f, %f }\n", p0.x, p0.y, p1.x, p1.y);

  p0.x -= offset.x;
  p0.y -= offset.y;
  p1.x -= offset.x;
  p1.y -= offset.y;

  dy = p1.y - p0.y;
  dx = p1.x - p0.x;

  distance = sqrtf (powf (dy, 2) + powf (dx, 2));
  h = sqrtf (powf (distance, 2) + powf (line_width, 2));

  if (debug)
    printf ("distance: %f, h: %f\n", distance, h);

  if ((p1.x > p0.x && p1.y < p0.y) || (p1.x < p0.x && p1.y < p0.y))
    left++;

  if (p0.y == p1.y)
  {
    // horizontal line
    p3->x = p1.x;
    p3->y = p1.y;

    if (p1.x >= p0.x)
      p3->y -= line_width;
    else
      p3->y += line_width;
  }
  else if (p0.x == p1.x)
  {
    // vertical line
    p3->x = p1.x;
    p3->y = p1.y;

    if (p1.y >= p0.x)
      p3->x += line_width;
    else
      p3->y -= line_width;
  }
  else
  {
    m = dy / dx;
    b = p1.y - m * p1.x;

    if (debug)
      printf ("m: %f, b: %f\n", m, b);

    m = -1 / m;
    b = p1.y - m * p1.x;

    if (debug)
      printf ("m: %f, b: %f\n", m, b);

    a = powf (m, 2) + 1;
    _b = 2 * m * b;
    c = powf (b, 2) - powf (h, 2);

    if (debug)
      printf ("a: %f, _b: %f, c: %f\n", a, _b, c);

    if (left)
      x = (-1 * _b) - sqrtf (powf (_b, 2) - 4 * a * c);
    else
      x = (-1 * _b) + sqrtf (powf (_b, 2) - 4 * a * c);

    x = x / (2 * a);

    y = m * x + b;

    p3->x = x;
    p3->y = y;
  }

  p0.x += offset.x;
  p0.y += offset.y;

  p1.x += offset.x;
  p1.y += offset.y;

  p3->x += offset.x;
  p3->y += offset.y;

  offset.x = p3->x - p1.x;
  offset.y = p3->y - p1.y;

  p2->x = p0.x + offset.x;
  p2->y = p0.y + offset.y;

  if (debug)
  {
    dy = p0.y - p2->y;
    dx = p0.x - p2->x;
    distance = sqrtf (powf (dy, 2) + powf (dx, 2));
    printf ("distance: %f, expected distance: %f\n", distance, line_width);

    dy = p1.y - p3->y;
    dx = p1.x - p3->x;
    distance = sqrtf (powf (dy, 2) + powf (dx, 2));
    printf ("distance: %f, expected distance: %f\n", distance, line_width);

    printf ("p2 { %f, %f } p3 { %f, %f }\n", p2->x, p2->y, p3->x, p3->y);
  }
}

void
vec2_to_vec3 (Vector2 v2, Vector3 *v3)
{
  v3->x = v2.x;
  v3->y = 0;
  v3->z = v2.y;
}

int
main (int argc, char *argv[])
{
  unsigned int screenWidth = 1600, screenHeight = 900;
  // anti aliasing ?
  SetConfigFlags (FLAG_MSAA_4X_HINT);
  InitWindow (screenWidth, screenHeight, "line_width");

  Camera3D camera = { 0 };
  camera.position = (Vector3){ 5.0f, 10.0f, 5.0f };
  camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
  camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  SetTargetFPS (60);

  vertices[0] = (Vector3) {0,0,0};
  vertices[1] = (Vector3) {2,0,2};
  vertices[2] = (Vector3) {2,0,2};
  vertices[3] = (Vector3) {4,0,2};
  vertices[4] = (Vector3) {4,0,2};
  vertices[5] = (Vector3) {5,0,-1};
  vertices[6] = (Vector3) {5,0,-1};
  vertices[7] = (Vector3) {0,0,-2};
  vertices[8] = (Vector3) {0,0,-2};
  vertices[9] = (Vector3) {0,0,0};

  int j = 0;
  for (int i = 0; i < VERTEX_COUNT - 1; i += 2)
  {
    Vector2
      p0 = (Vector2) { vertices[i].x, vertices[i].z },
      p1 = (Vector2) { vertices[i + 1].x, vertices[i + 1].z },
      p2 = { 0 },
      p3 = { 0 };

    printf ("---\n");
    ortho (p0, p1, &p2, &p3, 0.045, 1);
    printf ("---\n");

    vec2_to_vec3 (p2, &(vertices_ortho[j])); j++;
    vec2_to_vec3 (p0, &(vertices_ortho[j])); j++;
    vec2_to_vec3 (p3, &(vertices_ortho[j])); j++;

    vec2_to_vec3 (p0, &(vertices_ortho[j])); j++;
    vec2_to_vec3 (p1, &(vertices_ortho[j])); j++;
    vec2_to_vec3 (p3, &(vertices_ortho[j])); j++;
  }

  for (int i = 0; i < ORTHO_VERTEX_COUNT; i++)
    printf ("vertices_ortho[%d] = { %f, %f }\n", i, vertices_ortho[i].x, vertices_ortho[i].z);

  // init
  shader = LoadShader ("lines.vs", "lines.fs");
  glGenVertexArrays (1, &vao);
  glGenBuffers (1, &vbo);

  // bind
  glBindVertexArray (vao);
  glBindBuffer (GL_ARRAY_BUFFER, vbo);

  // push
  glBufferData (GL_ARRAY_BUFFER, ORTHO_VERTEX_COUNT * sizeof (Vector3), vertices_ortho, GL_STATIC_DRAW);

  // push
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void *) 0);
  glEnableVertexAttribArray (0);

  // unbind
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);

  while (!WindowShouldClose ())
  {
    UpdateCamera (&camera, CAMERA_FREE);
    BeginDrawing ();
    ClearBackground (WHITE);
    BeginMode3D (camera);

    DrawGrid (10, 1.0f);
    //DrawCube ((Vector3) { 0, 0, 0 }, 0.5f, 0.5f, 0.5f, BLACK);

    //for (int i = 0; i < VERTEX_COUNT - 1; i++) DrawLine3D (vertices[i], vertices[i + 1], BLACK);
    draw ();

    EndMode3D ();
    EndDrawing ();
  }

  CloseWindow ();
  return 0;
}
