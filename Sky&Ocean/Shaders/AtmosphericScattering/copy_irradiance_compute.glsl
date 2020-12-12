#version 450 core

layout(local_size_x = 8, local_size_y = 2, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform image2D irradiance_tex;
layout(rgba16f, binding = 1) uniform restrict readonly image2D delta_e_tex;
layout(location = 0) uniform float k;
layout(location = 1) uniform int add;
void main()
{
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(64.0, 16.0);
	vec4 addition = imageLoad(irradiance_tex, ivec2(gl_GlobalInvocationID.xy));
	imageStore(irradiance_tex, ivec2(gl_GlobalInvocationID.xy), k * (imageLoad(delta_e_tex, ivec2(gl_GlobalInvocationID.xy)) + addition));
}