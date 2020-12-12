#version 450 core
layout(location = 0) in vec4 position;
uniform mat4 inv_view;
uniform mat4 inv_persp;
out vec2 uv;
out vec3 view_dir;
void main()
{
	uv = position.zw;
	view_dir = (inv_view * vec4((inv_persp * vec4(position.xy, 0.0, 1.0)).xyz, 0.0)).xyz;
	gl_Position = vec4(position.xy, 0.9999999, 1.0);
}