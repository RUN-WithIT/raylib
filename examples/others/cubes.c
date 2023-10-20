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

#define RSTRDUP(x) strdup(x)

typedef struct Vertex_s Vertex_t;
typedef struct PointCloud_s PointCloud_t;
typedef struct Obj_s Obj_t;

struct Vertex_s
{
  float x, y, z, r, g, b, a;
};

struct PointCloud_s
{
  Shader shader;
  unsigned int cube_count;
  unsigned int vertex_count;
  Vertex_t *vertices;
  GLuint vao; // vertex array object
  GLuint vbo; // vertex buffer object
};

struct Obj_s
{
  int hidden_flag;
  unsigned int vertice_index;
  Vector3 pos;
  Vector3 dim;
  Color color;
};

char *
_obj_free (Obj_t *obj)
{
  char *retval = NULL;

  if (obj) free (obj);
  goto CLEANUP;

CLEANUP:
  return retval;
}

char *
_obj_alloc (unsigned int vertice_index, Vector3 pos, Vector3 dim, Color color, Obj_t **obj)
{
  char *retval = NULL;
  Obj_t *_obj = NULL;

  _obj = calloc (sizeof (Obj_t), 1);
  if (_obj == NULL)
  {
    retval = RSTRDUP ("failed to alloc Obj_t");
    goto CLEANUP;
  }

  _obj->vertice_index = vertice_index;
  _obj->pos = pos;
  _obj->dim = dim;
  _obj->color = color;

  *obj = _obj;
  _obj = NULL;

CLEANUP:
  if (_obj) _obj_free (_obj);
  return retval;
}

void
point_cloud_free (PointCloud_t *pc)
{
  if (pc != NULL)
  {
    UnloadShader (pc->shader);
    glDeleteBuffers (1, &(pc->vbo));
    glDeleteVertexArrays (1, &(pc->vao));
    free (pc->vertices);
    free (pc);
  }
}

char *
point_cloud_alloc (unsigned int cube_count, PointCloud_t **pc)
{
  char *retval = NULL;
  char
    *vs = NULL,
    *fs = NULL;
  PointCloud_t *_pc = NULL;
  
  _pc = calloc (sizeof (PointCloud_t), 1);
  if (_pc == NULL)
  {
    retval = RSTRDUP ("failed to alloc PointCloud_t");
    goto CLEANUP;
  }

  vs = RSTRDUP ("#version 330 core\n \n layout (location = 0) in vec3 vertexPosition;\n layout (location = 1) in vec4 vertexColor;\n \n uniform mat4 mvp;\n \n out vec4 theColor;\n \n void\n main ()\n {\n vec3 pos = vertexPosition;\n \n if (vertexColor.a == 0)\n pos.y += 1000;\n \n gl_Position = mvp * vec4 (pos, 1.0);\n theColor = vertexColor;\n}");

  fs = RSTRDUP ("#version 330 core\n \n in vec4 theColor;\n layout (location = 0) out vec4 finalColor;\n \n void\n main ()\n {\n finalColor = vec4 (theColor);\n }");

  // load shader
  _pc->shader = LoadShaderFromMemory (vs, fs);

  // generate vertex array
  glGenVertexArrays (1, &(_pc->vao));

  // generated buffer
  glGenBuffers (1, &(_pc->vbo));

  _pc->cube_count = cube_count;
  _pc->vertex_count = cube_count * 36;

  _pc->vertices = calloc (sizeof (Vertex_t), _pc->vertex_count);
  if (_pc->vertices == NULL)
  {
    retval = RSTRDUP ("failed to alloc Vertex_t");
    goto CLEANUP;
  }

  *pc = _pc;
  _pc = NULL;

CLEANUP:
  if (_pc) point_cloud_free (_pc);
  return retval;
}

char *
point_cloud_update_cube (PointCloud_t *pc, unsigned int index, Vector3 pos, Vector3 dim, Color color)
{
  char *retval = NULL;
  float
    r = color.r,
    g = color.g,
    b = color.b,
    a = color.a;
  float
    width = dim.x,
    height = dim.y,
    depth = dim.z;
  Vertex_t
    tl = (Vertex_t) {pos.x - (width / 2), pos.y + (height / 2), pos.z - (depth / 2), r, g, b, a},
    tr = (Vertex_t) {pos.x + (width / 2), pos.y + (height / 2), pos.z - (depth / 2), r, g, b, a},
    br = (Vertex_t) {pos.x + (width / 2), pos.y - (height / 2), pos.z - (depth / 2), r, g, b, a},
    bl = (Vertex_t) {pos.x - (width / 2), pos.y - (height / 2), pos.z - (depth / 2), r, g, b, a},
    rtl = (Vertex_t) {pos.x - (width / 2), pos.y + (height / 2), pos.z + (depth / 2), r, g, b, a},
    rtr = (Vertex_t) {pos.x + (width / 2), pos.y + (height / 2), pos.z + (depth / 2), r, g, b, a},
    rbr = (Vertex_t) {pos.x + (width / 2), pos.y - (height / 2), pos.z + (depth / 2), r, g, b, a},
    rbl = (Vertex_t) {pos.x - (width / 2), pos.y - (height / 2), pos.z + (depth / 2), r, g, b, a};

  if (index + 35 >= pc->vertex_count)
  {
    retval = RSTRDUP ("not enough space to add cube");
    goto CLEANUP;
  }

  // top
  pc->vertices[index + 0] = tl;
  pc->vertices[index + 1] = rtl;
  pc->vertices[index + 2] = tr;
  pc->vertices[index + 3] = rtl;
  pc->vertices[index + 4] = rtr;
  pc->vertices[index + 5] = tr;

  // bottom
  pc->vertices[index + 6] = bl;
  pc->vertices[index + 7] = br;
  pc->vertices[index + 8] = rbl;
  pc->vertices[index + 9] = br;
  pc->vertices[index + 10] = rbr;
  pc->vertices[index + 11] = rbl;

  // back
  pc->vertices[index + 12] = bl;
  pc->vertices[index + 13] = rbl;
  pc->vertices[index + 14] = rtl;
  pc->vertices[index + 15] = bl;
  pc->vertices[index + 16] = rtl;
  pc->vertices[index + 17] = tl;

  // front
  pc->vertices[index + 18] = br;
  pc->vertices[index + 19] = tr;
  pc->vertices[index + 20] = rtr;
  pc->vertices[index + 21] = br;
  pc->vertices[index + 22] = rtr;
  pc->vertices[index + 23] = rbr;

  // left
  pc->vertices[index + 24] = bl;
  pc->vertices[index + 25] = tl;
  pc->vertices[index + 26] = tr;
  pc->vertices[index + 27] = bl;
  pc->vertices[index + 28] = tr;
  pc->vertices[index + 29] = br;

  // right
  pc->vertices[index + 30] = rbl;
  pc->vertices[index + 31] = rtr;
  pc->vertices[index + 32] = rtl;
  pc->vertices[index + 33] = rbl;
  pc->vertices[index + 34] = rbr;
  pc->vertices[index + 35] = rtr;

CLEANUP:
  return retval;
}

// used for filter ?
char *
point_cloud_show_cube (PointCloud_t *pc, unsigned int index)
{
  char *retval = NULL;
  unsigned int i = 0;

  if (index + 35 >= pc->vertex_count)
  {
    retval = RSTRDUP ("not enough space to add cube");
    goto CLEANUP;
  }

  for (i = index; i < index + 36; i++)
    pc->vertices[i].a = 1;

CLEANUP:
  return retval;
}

// used for filter ?
char *
point_cloud_hide_cube (PointCloud_t *pc, unsigned int index)
{
  char *retval = NULL;
  unsigned int i = 0;

  if (index + 35 >= pc->vertex_count)
  {
    retval = RSTRDUP ("not enough space to add cube");
    goto CLEANUP;
  }

  for (i = index; i < index + 36; i++)
    pc->vertices[i].a = 0;

CLEANUP:
  return retval;
}

char *
point_cloud_upload_all_data (PointCloud_t *pc)
{
  char *retval = NULL;

  // bind vertex array
  glBindVertexArray (pc->vao);

  // bind buffer
  glBindBuffer (GL_ARRAY_BUFFER, pc->vbo);

  // load data
  glBufferData (GL_ARRAY_BUFFER, pc->vertex_count * sizeof (Vertex_t), pc->vertices, GL_STATIC_DRAW);

  // load x,y,z into layout = 0
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof (float), (void *) 0);
  glEnableVertexAttribArray (0);

  // load r,g,b,a into layout = 1
  glVertexAttribPointer (1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof (float), (void *) (sizeof (float) * 3));
  glEnableVertexAttribArray (1);

  // unbind buffer
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  // unbind vertex array
  glBindVertexArray (0);

  goto CLEANUP;
CLEANUP:
  return retval;
}

char *
point_cloud_draw (PointCloud_t *pc)
{
  char *retval = NULL;
  // model view projection
  Matrix mvp = { 0 };

  rlDrawRenderBatchActive ();
  glUseProgram (pc->shader.id);

  mvp = MatrixMultiply (rlGetMatrixModelview (), rlGetMatrixProjection ());
  glUniformMatrix4fv (pc->shader.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat (mvp));

  glBindVertexArray (pc->vao);
  glDrawArrays (GL_TRIANGLES, 0, pc->vertex_count);
  glBindVertexArray (0);

  glUseProgram (0);

  goto CLEANUP;
CLEANUP:
  return retval;
}

int
main (int argc, char *argv[])
{
  char *retval = NULL;
  int shader_flag = 1;
  unsigned int
    cube_count = 100,
    i = 0,
    j = 0,
    screenWidth = 1600,
    screenHeight = 900;
  Camera3D camera = { 0 };
  Vector3
    pos = { 0 },
    dim = { 0 };
  Color color = { 0 };
  PointCloud_t *pc = NULL;
  Obj_t **objs = NULL;

  if (argc >= 2)
    cube_count = atoi (argv[1]);

  if (argc >= 3)
    shader_flag = atoi (argv[2]);
  
  objs = calloc (sizeof (Obj_t *), cube_count);
  if (objs == NULL)
  {
    retval = RSTRDUP ("failed to alloc Obj_t");
    goto CLEANUP;
  }

  // populate objs
  for (i = 0; i < cube_count; i++)
  {
    color = (Color) { 0, 0, 0, 255};
    j = GetRandomValue (0, 4);

    switch (j)
    {
    case 0:
      color.r = 255;
      break;

    case 1:
      color.g = 255;
      break;

    case 3:
      color.b = 255;
      break;
    }

    pos = (Vector3) { GetRandomValue (-10000, 10000)/100.0, GetRandomValue (-1000, 1000)/100.0, GetRandomValue (-10000,10000)/100.0};
    dim = (Vector3) { 3, 3, 3 };

    retval = _obj_alloc (i * 36, pos, dim, color, &(objs[i]));
    if (retval != NULL) { goto CLEANUP; }
  }

  InitWindow (screenWidth, screenHeight, "cube shaders");

  camera.position = (Vector3) { 60.0f, 45.0, 60.0f };
  camera.target = (Vector3) { 0.0f, 0.0f, 0.0f };
  camera.up = (Vector3) { 0.0f, 1.0f, 0.0f };
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // alloc point cloud (must be after InitWindow)
  retval = point_cloud_alloc (cube_count, &pc);
  if (retval != NULL) { goto CLEANUP; }
  
  for (i = 0; i < pc->cube_count; i++)
  {
    retval = point_cloud_update_cube (pc, objs[i]->vertice_index, objs[i]->pos, objs[i]->dim, objs[i]->color);
    if (retval != NULL) { goto CLEANUP; }
  }

  // upload all data to GPU
  retval = point_cloud_upload_all_data (pc);
  if (retval != NULL) { goto CLEANUP; }

  glEnable (GL_PROGRAM_POINT_SIZE);

  //SetTargetFPS (60);

  while (!WindowShouldClose ())
  {
    UpdateCamera (&camera, CAMERA_THIRD_PERSON);

    BeginDrawing ();
    BeginMode3D (camera);

    ClearBackground (WHITE);

    if (shader_flag)
    {
      // draw cubes (formed by vertices) (must be inside Mode3D)
      retval = point_cloud_draw (pc);
      if (retval != NULL) { goto CLEANUP; }
    }
    else
    {
      for (i = 0; i < pc->cube_count; i++)
	DrawCubeWires (objs[i]->pos, objs[i]->dim.x, objs[i]->dim.y, objs[i]->dim.z, objs[i]->color);
    }

    EndMode3D ();

    // draw fps
    DrawRectangle (screenWidth - 120, 0, 110, 30, BLACK);
    DrawFPS (screenWidth - 100, 5);

    // draw cube count
    DrawRectangle (10, 0, 220, 30, BLACK);
    DrawText (TextFormat ("%zu cubes drawn", pc->cube_count), 15, 5, 20, GREEN);
    
    EndDrawing ();
  }

CLEANUP:
  CloseWindow ();
  if (pc) point_cloud_free (pc);
  if (objs)
  {
    for (i = 0; i < cube_count; i++)
      _obj_free (objs[i]);
    free (objs);
  }
  if (retval != NULL)
  {
    printf ("ERROR: %s\n", retval);
    free (retval);
  }
  return 0;
}
