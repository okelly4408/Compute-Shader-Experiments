#version 450 core
layout(location = 0) in vec3 position;
out vec2 uv;
void main(void)
{
	uv = (position.xy + 1.0) / 2.0;
	gl_Position = vec4(position, 1.0);
}