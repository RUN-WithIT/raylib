#version 330 core 

layout (location = 0) in vec3 vertexPosition; 
layout (location = 1) in vec4 vertexColor; 
layout (location = 2) in vec3 deltaPosition; 
layout (location = 3) in vec4 deltaColor; 
layout (location = 4) in uint start_ts; 
layout (location = 5) in uint end_ts; 

uniform mat4 mvp; 
uniform uint ts; 

out vec4 theColor; 

void 
main () 
{
  vec3 pos = vertexPosition;
  vec4 color = vertexColor;
  float m0 = 0;

  if (start_ts <= ts && ts <= end_ts)
  {
    m0 = float (ts - start_ts) / float (end_ts - start_ts);
    m0 = float (1) - m0;
    // m0 = sin (m0 * 10);

    pos.x += (m0 * deltaPosition.x);
    pos.y += (m0 * deltaPosition.y);
    pos.z += (m0 * deltaPosition.z);

    color.r += (m0 * deltaColor.r);
    color.g += (m0 * deltaColor.g);
    color.b += (m0 * deltaColor.b);
    color.a += (m0 * deltaColor.a);
  }
 
  if (color.a == 0)
    pos.y += 1000;
 
  gl_Position = mvp * vec4 (pos, 1.0);
  theColor = color;
}
