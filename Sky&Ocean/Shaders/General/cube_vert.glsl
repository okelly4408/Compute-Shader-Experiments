#version 450 core
layout(location = 0) in vec4 position;
uniform mat4 projection;
uniform mat4 view;
out vec3 vert_color;
void main()
{
	vert_color = position.xyz;
	gl_Position = projection * view * vec4(position.x, position.y + 10.0, position.z, 1.0);
}