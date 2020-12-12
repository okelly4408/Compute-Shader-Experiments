#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
uniform mat4 view;
uniform mat4 projection;
in vec3 n[];
in VS_OUT
{
	vec3 color;
	vec3 normal;
} vs_out[];

out GEOM_OUT
{
	vec3 position;
	vec3 normal;
} geom_out;
void main()
{
	gl_Position = projection * view * gl_in[0].gl_Position;
	geom_out.normal =  vs_out[0].normal;
	geom_out.position = gl_in[0].gl_Position.xyz;
	EmitVertex();

	gl_Position = projection * view * gl_in[1].gl_Position;
	geom_out.normal =  vs_out[1].normal;
	geom_out.position = gl_in[1].gl_Position.xyz;
	EmitVertex();

	gl_Position = projection * view * gl_in[2].gl_Position;
	geom_out.normal =  vs_out[2].normal;
	geom_out.position = gl_in[2].gl_Position.xyz;
	EmitVertex();
	EndPrimitive();
}