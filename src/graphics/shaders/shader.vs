#version 330

//layout (location = 0) 
in vec3 position;

uniform mat4 gWVP;

out vec4 Color;

void main()
{
    gl_Position = gWVP * vec4(position, 1.0);
    Color = vec4(clamp(position, 0.0, 1.0), 1.0);
}
