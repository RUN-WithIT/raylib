#include "raylib.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            120
#endif

#include <stdlib.h>             // Required for: NULL

#define MAX_LIGHTS  10           // Max dynamic lights supported by shader

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

static float
_ts_millis ()
{
  static double a_start = 0.0;
  double dtime = 0.0;
  struct timespec _t;

  /* offset time to bring millis into resolution */
  if (a_start == 0.0)
    {
      a_start = 
        clock_gettime (CLOCK_REALTIME, &_t);
      a_start = (double)((_t.tv_sec) + (_t.tv_nsec / 1e9));
      dtime = 0.0;
    }
  else
    {
      clock_gettime (CLOCK_REALTIME, &_t);
      dtime = (double)((_t.tv_sec) + (_t.tv_nsec / 1e9));
      dtime -= a_start;
    }
  dtime *= 1000.0;

  return ((float)dtime);
}

// Light type
typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

typedef struct
{
  float ts;
  Color color;
  Vector3 position;
  Vector3 target;
} LightFields;
    
typedef struct
{
  int enabled;
  LightFields start;
  LightFields end;
} LightAnim;
    
// Light data
typedef struct
{
  int type;
  int enabled;
  Vector3 position;
  Vector3 target;
  float color[4];
  float intensity;

  // Shader light parameters locations
  int typeLoc;
  int enabledLoc;
  int positionLoc;
  int targetLoc;
  int colorLoc;
  int intensityLoc;

  LightAnim light_anim;
} Light;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightCount = 0;     // Current number of dynamic lights that have been created

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
// Create a light and get shader locations
static Light CreateLight(int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader);

// Update light properties on shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light);

void
EnableLight (Light *light)
{
  light->enabled = 1;
}

void
DisableLight (Light *light)
{
  light->enabled = 0;
}

void
ToggleLight (Light *light)
{
  light->enabled = !(light->enabled); 
}

void
AnimateLight (Light *light)
{
  LightAnim *light_anim = &(light->light_anim);
  float
    ts = 0,
    ratio = 0;

  if (!(light_anim->enabled))
    return;

  ts = _ts_millis ();

  if (ts < light_anim->start.ts)
    return;

  // anim logic here
  ratio = (ts - light_anim->start.ts) / (light_anim->end.ts - light_anim->start.ts);

  if (ratio >= 1)
  {
    ratio = 1;
    light_anim->enabled = 0;
  }
    
  // color
  light->color[0] = (light_anim->start.color.r + ((light_anim->end.color.r - light_anim->start.color.r) * ratio)) / 255.0;
  light->color[1] = (light_anim->start.color.g + ((light_anim->end.color.g - light_anim->start.color.g) * ratio)) / 255.0;
  light->color[2] = (light_anim->start.color.b + ((light_anim->end.color.b - light_anim->start.color.b) * ratio)) / 255.0;
  light->color[3] = (light_anim->start.color.a + ((light_anim->end.color.a - light_anim->start.color.a) * ratio)) / 255.0;

  // position
  light->position.x = light_anim->start.position.x + ((light_anim->end.position.x - light_anim->start.position.x) * ratio);
  light->position.y = light_anim->start.position.y + ((light_anim->end.position.y - light_anim->start.position.y) * ratio);
  light->position.z = light_anim->start.position.z + ((light_anim->end.position.z - light_anim->start.position.z) * ratio);

  // target
  light->target.x = light_anim->start.target.x + ((light_anim->end.target.x - light_anim->start.target.x) * ratio);
  light->target.y = light_anim->start.target.y + ((light_anim->end.target.y - light_anim->start.target.y) * ratio);
  light->target.z = light_anim->start.target.z + ((light_anim->end.target.z - light_anim->start.target.z) * ratio);
}

// at
// 0 - relative, 1 - absolute
void
AddLightAnimation (Light *light, float delay, float duration,
		   Color color, Color color_at,
		   Vector3 position, Vector3 position_at,
		   Vector3 target, Vector3 target_at)
{
  LightAnim *light_anim = &(light->light_anim);
  float ts = _ts_millis ();

  memset (light_anim, 0, sizeof (LightAnim));

  ts += delay * 1000;
  duration *= 1000;

  light_anim->enabled = 1;

  // ts
  light_anim->start.ts = ts;
  light_anim->end.ts = ts + duration;

  // color
  light_anim->start.color.r = light->color[0] * 255.0;
  light_anim->start.color.g = light->color[1] * 255.0;
  light_anim->start.color.b = light->color[2] * 255.0;
  light_anim->start.color.a = light->color[3] * 255.0;

  if (color_at.r)
    light_anim->end.color.r = color.r;
  else
    light_anim->end.color.r = light_anim->start.color.r + color.r;

  if (color_at.g)
    light_anim->end.color.g = color.g;
  else
    light_anim->end.color.g = light_anim->start.color.g + color.g;

  if (color_at.b)
    light_anim->end.color.b = color.b;
  else
    light_anim->end.color.b = light_anim->start.color.b + color.b;

  if (color_at.a)
    light_anim->end.color.a = color.a;
  else
    light_anim->end.color.a = light_anim->start.color.a + color.a;

  // position
  light_anim->start.position = light->position;

  if (position_at.x)
    light_anim->end.position.x = position.x;
  else
    light_anim->end.position.x = light_anim->start.position.x + position.x;

  if (position_at.y)
    light_anim->end.position.y = position.y;
  else
    light_anim->end.position.y = light_anim->start.position.y + position.y;

  if (position_at.z)
    light_anim->end.position.z = position.z;
  else
    light_anim->end.position.z = light_anim->start.position.z + position.z;

  // target
  light_anim->start.target = light->target;

  if (target_at.x)
    light_anim->end.target.x = target.x;
  else
    light_anim->end.target.x = light_anim->start.target.x + target.x;

  if (target_at.y)
    light_anim->end.target.y = target.y;
  else
    light_anim->end.target.y = light_anim->start.target.y + target.y;

  if (target_at.z)
    light_anim->end.target.z = target.z;
  else
    light_anim->end.target.z = light_anim->start.target.z + target.z;
}

void
AnimateLights (Shader shader, Light lights[MAX_LIGHTS])
{
  int i = 0;

  for (i = 0; i < MAX_LIGHTS; i++)
  {
    AnimateLight (&(lights[i]));
    UpdateLight (shader, lights[i]);
  }
}

void
DrawLightSpheres (Light lights[MAX_LIGHTS])    
{
  int i = 0;
  Color color = BLACK;

  for (i = 0; i < MAX_LIGHTS; i++)
  {
    color = (Color) { lights[i].color[0] * 255, lights[i].color[1] * 255, lights[i].color[2] * 255, lights[i].color[3] * 255 };
		    
    if (lights[i].enabled)
      DrawSphereEx (lights[i].position, 0.2f, 8, 8, color);
    else
      DrawSphereWires (lights[i].position, 0.2f, 8, 8, ColorAlpha (color, 0.3f));
  }
}

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1600;
    const int screenHeight = 900;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "raylib [shaders] example - basic pbr");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ 2.0f, 2.0f, 6.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    // Load PBR shader and setup all required locations
    Shader shader = LoadShader(TextFormat("resources/shaders/glsl%i/pbr.vs", GLSL_VERSION),
                               TextFormat("resources/shaders/glsl%i/pbr.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    // WARNING: Metalness, roughness, and ambient occlusion are all packed into a MRA texture
    // They are passed as to the SHADER_LOC_MAP_METALNESS location for convenience,
    // shader already takes care of it accordingly
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    // WARNING: Similar to the MRA map, the emissive map packs different information 
    // into a single texture: it stores height and emission data
    // It is binded to SHADER_LOC_MAP_EMISSION location an properly processed on shader
    shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
    shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");

    // Setup additional required shader locations, including lights data
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int lightCountLoc = GetShaderLocation(shader, "numOfLights");
    int maxLightCount = MAX_LIGHTS;
    SetShaderValue(shader, lightCountLoc, &maxLightCount, SHADER_UNIFORM_INT);

    // Setup ambient color and intensity parameters
    float ambientIntensity = 0.02f;
    Color ambientColor = (Color){ 26, 32, 135, 255 };
    Vector3 ambientColorNormalized = (Vector3){ ambientColor.r/255.0f, ambientColor.g/255.0f, ambientColor.b/255.0f };
    SetShaderValue(shader, GetShaderLocation(shader, "ambientColor"), &ambientColorNormalized, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, GetShaderLocation(shader, "ambient"), &ambientIntensity, SHADER_UNIFORM_FLOAT);

    // Get location for shader parameters that can be modified in real time
    int emissiveIntensityLoc = GetShaderLocation(shader, "emissivePower");
    int emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
    int textureTilingLoc = GetShaderLocation(shader, "tiling");

    // Load old car model using PBR maps and shader
    // WARNING: We know this model consists of a single model.meshes[0] and
    // that model.materials[0] is by default assigned to that mesh
    // There could be more complex models consisting of multiple meshes and
    // multiple materials defined for those meshes... but always 1 mesh = 1 material
    Model car = LoadModel("resources/models/old_car_new.glb");

    // Assign already setup PBR shader to model.materials[0], used by models.meshes[0]
    car.materials[0].shader = shader;

    // Setup materials[0].maps default parameters
    car.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    car.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    car.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    car.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    car.materials[0].maps[MATERIAL_MAP_EMISSION].color = (Color){ 255, 162, 0, 255 };

    // Setup materials[0].maps default textures
    car.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/old_car_d.png");
    car.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/old_car_mra.png");
    car.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/old_car_n.png");
    car.materials[0].maps[MATERIAL_MAP_EMISSION].texture = LoadTexture("resources/old_car_e.png");
    
    // Load floor model mesh and assign material parameters
    // NOTE: A basic plane shape can be generated instead of being loaded from a model file
    Model floor = LoadModel("resources/models/plane.glb");
    //Mesh floorMesh = GenMeshPlane(10, 10, 10, 10);
    //GenMeshTangents(&floorMesh);      // TODO: Review tangents generation
    //Model floor = LoadModelFromMesh(floorMesh);

    // Assign material shader for our floor model, same PBR shader 
    floor.materials[0].shader = shader;
    
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[MATERIAL_MAP_EMISSION].color = BLACK;

    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/road_a.png");
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/road_mra.png");
    floor.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/road_n.png");

    // Models texture tiling parameter can be stored in the Material struct if required (CURRENTLY NOT USED)
    // NOTE: Material.params[4] are available for generic parameters storage (float)
    Vector2 carTextureTiling = (Vector2){ 0.5f, 0.5f };
    Vector2 floorTextureTiling = (Vector2){ 0.5f, 0.5f };

    // Create some lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){  2.0f, 1.0f,  1.0f }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color) {0,0,0,0} , 3.3f, shader);

    // Setup material texture maps usage in shader
    // NOTE: By default, the texture maps are always used
    int usage = 1;
    SetShaderValue(shader, GetShaderLocation(shader, "useTexAlbedo"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexNormal"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexMRA"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexEmissive"), &usage, SHADER_UNIFORM_INT);
    
    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    AddLightAnimation (&(lights[0]), 1, 5,
		       YELLOW, (Color) { 1, 1, 1, 1 },
		       (Vector3) { -3, 0.25, -3 }, (Vector3) { 0, 0, 0 },
		       (Vector3) { 0,  0,     0 }, (Vector3) { 0, 0, 0 });

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_ONE))  { ToggleLight (&(lights[0])); }

	AnimateLights (shader, lights);

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        
            ClearBackground(BLACK);
            
            BeginMode3D(camera);
                
                // Set floor model texture tiling and emissive color parameters on shader
                SetShaderValue(shader, textureTilingLoc, &floorTextureTiling, SHADER_UNIFORM_VEC2);
                Vector4 floorEmissiveColor = ColorNormalize(floor.materials[0].maps[MATERIAL_MAP_EMISSION].color);
                SetShaderValue(shader, emissiveColorLoc, &floorEmissiveColor, SHADER_UNIFORM_VEC4);
                
                DrawModel(floor, (Vector3){ 0.0f, 0.0f, 0.0f }, 5.0f, WHITE);   // Draw floor model

                // Set old car model texture tiling, emissive color and emissive intensity parameters on shader
                SetShaderValue(shader, textureTilingLoc, &carTextureTiling, SHADER_UNIFORM_VEC2);
                Vector4 carEmissiveColor = ColorNormalize(car.materials[0].maps[MATERIAL_MAP_EMISSION].color);
                SetShaderValue(shader, emissiveColorLoc, &carEmissiveColor, SHADER_UNIFORM_VEC4);
                float emissiveIntensity = 0.01f;
                SetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, SHADER_UNIFORM_FLOAT);
                
                DrawModel(car, (Vector3){ 0.0f, 0.0f, 0.0f }, 0.005f, WHITE);   // Draw car model

                // Draw spheres to show the lights positions
                
		DrawLightSpheres (lights);
		
            EndMode3D();
            
            DrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);

            DrawText("(c) Old Rusty Car model by Renafox (https://skfb.ly/LxRy)", screenWidth - 320, screenHeight - 20, 10, LIGHTGRAY);
            
            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unbind (disconnect) shader from car.material[0] 
    // to avoid UnloadMaterial() trying to unload it automatically
    car.materials[0].shader = (Shader){ 0 };
    UnloadMaterial(car.materials[0]);
    car.materials[0].maps = NULL;
    UnloadModel(car);
    
    floor.materials[0].shader = (Shader){ 0 };
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);
    
    UnloadShader(shader);       // Unload Shader
    
    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

// Create light with provided data
// NOTE: It updated the global lightCount and it's limited to MAX_LIGHTS
static Light CreateLight(int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader)
{
    Light light = { 0 };

    if (lightCount < MAX_LIGHTS)
    {
        light.enabled = 1;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color[0] = (float)color.r/255.0f;
        light.color[1] = (float)color.g/255.0f;
        light.color[2] = (float)color.b/255.0f;
        light.color[3] = (float)color.a/255.0f;
        light.intensity = intensity;
        
        // NOTE: Shader parameters names for lights must match the requested ones
        light.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightCount));
        light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightCount));
        light.positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", lightCount));
        light.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightCount));
        light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightCount));
        light.intensityLoc = GetShaderLocation(shader, TextFormat("lights[%i].intensity", lightCount));
        
        UpdateLight(shader, light);

        lightCount++;
    }

    return light;
}

// Send light properties to shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light)
{
    SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);
    
    // Send to shader light position values
    float position[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = { light.target.x, light.target.y, light.target.z };
    SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, light.colorLoc, light.color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, light.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
}
