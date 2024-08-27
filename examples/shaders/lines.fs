#version 330 core
    
in vec4 theColor;
layout (location = 0) out vec4 finalColor;
    
void 
main () 
{
  finalColor = vec4 (theColor);
}
    
