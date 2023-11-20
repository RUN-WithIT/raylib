#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include <time.h>
#include <math.h>
#include <unistd.h>

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
typedef struct Cbd_s Cbd_t;

struct Vertex_s
{
  float x, y, z, r, g, b, a;
  float dx, dy, dz, dr, dg, db, da;
  unsigned int start_ts, end_ts, mode;
  float cx, cy, cz;
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
  unsigned int age;
  unsigned int vertice_index;
  Vector3 pos;
  Vector3 dim;
  Color color;
  Vector3 _orig_pos;
  Vector3 _orig_dim;
  Color _orig_color;
};

struct Cbd_s
{
  Obj_t *obj;
  Vector3 pos, dim, pos_at, dim_at;
  Color color, color_at;
  unsigned int start_ts, end_ts, mode;
  Vertex_t **vs;
  int reset, animate;
};

unsigned int
_ts (unsigned int offset_sec)
{
  struct timespec _t;
  clock_gettime (CLOCK_REALTIME, &_t);
  return (_t.tv_sec + offset_sec) * 1000 + lround (_t.tv_nsec / 1e6);
}

char *
_color_print (Color c)
{
  char *str = NULL;
  asprintf (&str, "{r:%d, g:%d, b:%d, a:%d}\n", c.r, c.g, c.b, c.a);
  return str;
}

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
  _obj->age = GetRandomValue (0, 100);
  _obj->hidden_flag = 0;

  _obj->_orig_pos = pos;
  _obj->_orig_dim = dim;
  _obj->_orig_color = color;

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

  vs = LoadFileText (TextFormat ("resources/shaders/glsl%i/cubes.vs", GLSL_VERSION));
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

static char *
_reset (Obj_t *obj)
{
  char *retval = NULL;

  obj->pos.x = obj->_orig_pos.x;
  obj->pos.y = obj->_orig_pos.y;
  obj->pos.z = obj->_orig_pos.z;

  obj->dim.x = obj->_orig_dim.x;
  obj->dim.y = obj->_orig_dim.y;
  obj->dim.z = obj->_orig_dim.z;

  obj->color.r = obj->_orig_color.r;
  obj->color.g = obj->_orig_color.g;
  obj->color.b = obj->_orig_color.b;
  obj->color.a = obj->_orig_color.a;

  goto CLEANUP;

CLEANUP:
  return retval;
}

static char *
_animate (Cbd_t *cbd)
{
  char *retval = NULL;
  float
    dx = 0, dy = 0, dz = 0, dr = 0, dg = 0, db = 0, da = 0,
    r = 0, g = 0, b = 0, a = 0, x = 0, y = 0, z = 0;
  unsigned int
    start_ts = cbd->start_ts,
    end_ts = cbd->end_ts;
  Obj_t *obj = cbd->obj;
  Vector3
    pos = cbd->pos,
    dim = cbd->dim,
    pos_at = cbd->pos_at,
    dim_at = cbd->dim_at,
    old_pos = obj->pos,
    old_dim = obj->dim;
  Color
    color = cbd->color,
    color_at = cbd->color_at,
    old_color = obj->color;
  Vertex_t **vs = cbd->vs;


  if (!(pos_at.x)) pos.x += old_pos.x;

  if (!(pos_at.y)) pos.y += old_pos.y;

  if (!(pos_at.z)) pos.z += old_pos.z;

  if (!(dim_at.x)) dim.x += old_dim.x;

  if (!(dim_at.y)) dim.y += old_dim.y;

  if (!(dim_at.z)) dim.z += old_dim.z;

  if (!(color_at.r)) color.r += old_color.r;

  if (!(color_at.g)) color.g += old_color.g;

  if (!(color_at.b)) color.b += old_color.b;

  if (!(color_at.a)) color.a += old_color.a;

  r = color.r / 255.0;
  g = color.g / 255.0;
  b = color.b / 255.0;
  a = color.a / 255.0;

  dr = (old_color.r - color.r) / 255.0;
  dg = (old_color.g - color.g) / 255.0;
  db = (old_color.b - color.b) / 255.0;
  da = (old_color.a - color.a) / 255.0;

  obj->pos = pos;
  obj->dim = dim;
  obj->color = color;

  x = pos.x - (dim.x / 2);
  y = pos.y + (dim.y / 2);
  z = pos.z - (dim.z / 2);

  dx = (old_pos.x - (old_dim.x / 2)) - x;
  dy = (old_pos.y + (old_dim.y / 2)) - y;
  dz = (old_pos.z - (old_dim.z / 2)) - z;
  *vs[0] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x + (dim.x / 2);
  y = pos.y + (dim.y / 2);
  z = pos.z - (dim.z / 2);

  dx = (old_pos.x + (old_dim.x / 2)) - x;
  dy = (old_pos.y + (old_dim.y / 2)) - y;
  dz = (old_pos.z - (old_dim.z / 2)) - z;
  *vs[1] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x + (dim.x / 2);
  y = pos.y - (dim.y / 2);
  z = pos.z - (dim.z / 2);

  dx = (old_pos.x + (old_dim.x / 2)) - x;
  dy = (old_pos.y - (old_dim.y / 2)) - y;
  dz = (old_pos.z - (old_dim.z / 2)) - z;
  *vs[2] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x - (dim.x / 2);
  y = pos.y - (dim.y / 2);
  z = pos.z - (dim.z / 2);

  dx = (old_pos.x - (old_dim.x / 2)) - x;
  dy = (old_pos.y - (old_dim.y / 2)) - y;
  dz = (old_pos.z - (old_dim.z / 2)) - z;
  *vs[3] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x - (dim.x / 2);
  y = pos.y + (dim.y / 2);
  z = pos.z + (dim.z / 2);

  dx = (old_pos.x - (old_dim.x / 2)) - x;
  dy = (old_pos.y + (old_dim.y / 2)) - y;
  dz = (old_pos.z + (old_dim.z / 2)) - z;
  *vs[4] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x + (dim.x / 2);
  y = pos.y + (dim.y / 2);
  z = pos.z + (dim.z / 2);

  dx = (old_pos.x + (old_dim.x / 2)) - x;
  dy = (old_pos.y + (old_dim.y / 2)) - y;
  dz = (old_pos.z + (old_dim.z / 2)) - z;
  *vs[5] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x + (dim.x / 2);
  y = pos.y - (dim.y / 2);
  z = pos.z + (dim.z / 2);

  dx = (old_pos.x + (old_dim.x / 2)) - x;
  dy = (old_pos.y - (old_dim.y / 2)) - y;
  dz = (old_pos.z + (old_dim.z / 2)) - z;
  *vs[6] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

  x = pos.x - (dim.x / 2);
  y = pos.y - (dim.y / 2);
  z = pos.z + (dim.z / 2);

  dx = (old_pos.x - (old_dim.x / 2)) - x;
  dy = (old_pos.y - (old_dim.y / 2)) - y;
  dz = (old_pos.z + (old_dim.z / 2)) - z;
  *vs[7] = (Vertex_t) { x, y, z, r, g, b, a, dx, dy, dz, dr, dg, db, da, start_ts, end_ts, cbd->mode };

CLEANUP:
  return retval;
}

char *
point_cloud_update_cube (PointCloud_t *pc, Obj_t *obj, Cbd_t *cbd)
{
  char *retval = NULL;
  Vector3 pos, dim;
  Color color;
  float r, g, b, a, width, height, depth;
  unsigned int index = obj->vertice_index;
  Vertex_t tl, tr, br, bl, rtl, rtr, rbr, rbl;
  Vertex_t *vs[8] = { &tl, &tr, &br, &bl, &rtl, &rtr, &rbr, &rbl };

  /*
   * do not interupt ongoing animation
   */
  if (cbd->start_ts <= pc->vertices[index].end_ts)
  {
    goto CLEANUP;
  }

  /*
   * reset the object (if not hidden) to its original pos, dim, color
   */
  if (cbd->reset && !(obj->hidden_flag))
  {
    _reset (obj);
  }

  pos = obj->pos;
  dim = obj->dim;
  color = obj->color;

  r = color.r;
  g = color.g;
  b = color.b;
  a = color.a;
  width = dim.x;
  height = dim.y;
  depth = dim.z;
  float cx = pos.x, cy = pos.y, cz = pos.z;

  tl =  (Vertex_t) { pos.x - (width / 2), pos.y + (height / 2), pos.z - (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  tr =  (Vertex_t) { pos.x + (width / 2), pos.y + (height / 2), pos.z - (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  br =  (Vertex_t) { pos.x + (width / 2), pos.y - (height / 2), pos.z - (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  bl =  (Vertex_t) { pos.x - (width / 2), pos.y - (height / 2), pos.z - (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  rtl = (Vertex_t) { pos.x - (width / 2), pos.y + (height / 2), pos.z + (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  rtr = (Vertex_t) { pos.x + (width / 2), pos.y + (height / 2), pos.z + (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  rbr = (Vertex_t) { pos.x + (width / 2), pos.y - (height / 2), pos.z + (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};
  rbl = (Vertex_t) { pos.x - (width / 2), pos.y - (height / 2), pos.z + (depth / 2), r, g, b, a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cx, cy, cz};

  cbd->vs = (Vertex_t **) &vs;
  cbd->obj = obj;

  /*
   * animate the obj only if it is not hidden
   */
  if (!(cbd->reset) && !(obj->hidden_flag) && cbd->animate)
  {
    retval = _animate (cbd);

    if (retval != NULL)
    {
      retval = RSTRDUP ("failed to animate");
      goto CLEANUP;
    }
  }

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
  float size = sizeof (float) * 14 + sizeof (unsigned int) * 3 + sizeof (float) * 3;

  // bind vertex array
  glBindVertexArray (pc->vao);

  // bind buffer
  glBindBuffer (GL_ARRAY_BUFFER, pc->vbo);

  // load data
  glBufferData (GL_ARRAY_BUFFER, pc->vertex_count * sizeof (Vertex_t), pc->vertices, GL_STATIC_DRAW);

  // load x,y,z into layout = 0
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, size, (void *) 0);
  glEnableVertexAttribArray (0);

  // load r,g,b,a into layout = 1
  glVertexAttribPointer (1, 4, GL_FLOAT, GL_FALSE, size, (void *) (sizeof (float) * 3));
  glEnableVertexAttribArray (1);

  // load dx,dy,dz into layout = 2
  glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, size, (void *) (sizeof (float) * 7));
  glEnableVertexAttribArray (2);

  // load dr,dg,db,da into layout = 3
  glVertexAttribPointer (3, 4, GL_FLOAT, GL_FALSE, size, (void *) (sizeof (float) * 10));
  glEnableVertexAttribArray (3);

  // load start_ts into layout = 4
  glVertexAttribPointer (4, 1, GL_UNSIGNED_INT, GL_FALSE, size, (void *) (sizeof (float) * 14));
  glEnableVertexAttribArray (4);

  // load end_ts into layout = 5
  glVertexAttribPointer (5, 1, GL_UNSIGNED_INT, GL_FALSE, size, (void *) (sizeof (float) * 14) + sizeof (unsigned int));
  glEnableVertexAttribArray (5);

  // load end_ts into layout = 6
  glVertexAttribPointer (6, 1, GL_UNSIGNED_INT, GL_FALSE, size, (void *) (sizeof (float) * 14) + sizeof (unsigned int) * 2);
  glEnableVertexAttribArray (6);

  // load cx,cy,cz
  glVertexAttribPointer (7, 3, GL_FLOAT, GL_FALSE, size, (void *) (sizeof (float) * 14) + sizeof (unsigned int) * 3);
  glEnableVertexAttribArray (7);

  // unbind buffer
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  // unbind vertex array
  glBindVertexArray (0);

  goto CLEANUP;

CLEANUP:
  return retval;
}

char *
point_cloud_draw (PointCloud_t *pc, Camera3D camera)
{
  char *retval = NULL;
  // model view projection
  Matrix mvp = { 0 };
  GLint location;
  unsigned int ts = 0;

  rlDrawRenderBatchActive ();
  glUseProgram (pc->shader.id);

  mvp = MatrixMultiply (rlGetMatrixModelview (), rlGetMatrixProjection ());
  glUniformMatrix4fv (pc->shader.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat (mvp));

  ts = _ts (0);
  location = glGetUniformLocation (pc->shader.id, "ts");

  glUniform1ui (location, ts);

  location = glGetUniformLocation (pc->shader.id, "cam");

  glUniform3f (location,
	       camera.position.x,
	       camera.position.y,
	       camera.position.z);
	       

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
    cube_count = 175 * 1000,
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
  Cbd_t cbd;

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

    pos = (Vector3) { GetRandomValue (-10000, 10000)/200.0, GetRandomValue (-1000, 1000)/100.0, GetRandomValue (-10000,10000)/200.0};
    dim = (Vector3) { 0.25, 0.25, 0.25 };

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
  //glDepthFunc(GL_LESS);

  // alloc point cloud (must be after InitWindow)
  retval = point_cloud_alloc (cube_count, &pc);
  if (retval != NULL) { goto CLEANUP; }

  memset (&cbd, '\0', sizeof (Cbd_t));
  cbd.pos = (Vector3) { 0, 100, 0 };
  cbd.dim = (Vector3) { 0, 0, 0 };
  cbd.color = (Color) { 0, 0, 0, 0 };
  cbd.pos_at = (Vector3) { 0, 0, 0};
  cbd.dim_at = (Vector3) { 0, 0, 0 };
  cbd.color_at = (Color) { 0, 0, 0, 0 };
  cbd.start_ts = _ts (0);
  cbd.end_ts = _ts (10);
  cbd.reset = 0;
  cbd.animate = 0;
  cbd.mode = 1;
  
  for (i = 0; i < pc->cube_count; i++)
  {
    retval = point_cloud_update_cube (pc, objs[i], &cbd);
    if (retval != NULL) { goto CLEANUP; }
  }

  // upload all data to GPU
  retval = point_cloud_upload_all_data (pc);
  if (retval != NULL) { goto CLEANUP; }

  glEnable (GL_PROGRAM_POINT_SIZE);

  //SetTargetFPS (60);
  //printf ("vertex_count %u\n", pc->vertex_count);

  unsigned int fc = 0;

  while (!WindowShouldClose ())
  {
    fc++;
    UpdateCamera (&camera, CAMERA_THIRD_PERSON);

    BeginDrawing ();
    BeginMode3D (camera);

    ClearBackground (WHITE);

    if (shader_flag)
    {
      // draw cubes (formed by vertices) (must be inside Mode3D)
      retval = point_cloud_draw (pc, camera);
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
    if (fc % 150 == 0)
    {
    printf ("camera -> {pos: {x:%f,y:%f,z:%f}, target: {x:%f,y:%f,z:%f}\n",
	    camera.position.x,
	    camera.position.y,
	    camera.position.z,
	    camera.target.x,
	    camera.target.y,
	    camera.target.z);
      fc = 0;
    }

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
