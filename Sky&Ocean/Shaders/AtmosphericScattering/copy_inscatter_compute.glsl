#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform restrict writeonly image3D inscatter_tex;
layout(rgba16f, binding = 1) uniform restrict readonly image3D delta_sr_tex;
layout(rgba16f, binding = 2) uniform restrict readonly image3D delta_sm_tex;

void main()
{
	vec3 uvw = vec3(gl_GlobalInvocationID.xy, float(gl_GlobalInvocationID.z)) / vec3(256.0, 128.0, 32.0);
	vec4 ray = imageLoad(delta_sr_tex, ivec3(gl_GlobalInvocationID.xyz));
	vec4 mie = imageLoad(delta_sm_tex, ivec3(gl_GlobalInvocationID.xyz));
	imageStore(inscatter_tex, ivec3(gl_GlobalInvocationID.xyz), vec4(ray.rgb, mie.r));
}