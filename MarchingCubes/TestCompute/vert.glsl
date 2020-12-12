#version 450 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
out VS_OUT
{
	vec3 color;
	vec3 normal;
} vs_out;
void main(void)
{
	vec3 n = normalize(position.xyz);
	vs_out.color = n;
	vs_out.normal = normal.xyz;
	gl_Position = vec4(position.xyz, 1.0);
}