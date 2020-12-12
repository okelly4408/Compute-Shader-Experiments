#version 450 core

layout(location = 0) in vec4 position;

out vec2 U;

void main()
{
	gl_Position = vec4(position.xy, 0.0, 1.0);
	U = position.xy * 1.1;
}