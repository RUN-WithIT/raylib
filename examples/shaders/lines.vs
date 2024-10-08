#version 330 core
    
layout (location = 0) in vec3 vertexPosition; 
    
uniform mat4 mvp; 
out vec4 theColor; 
    
void 
main () 
{
  vec3 pos = vertexPosition;
  vec4 color = vec4 (1.0, 0.0, 0.0, 1.0);
  gl_Position = mvp * vec4 (pos, 1.0);
  theColor = color;
}
