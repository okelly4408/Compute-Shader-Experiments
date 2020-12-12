#version 450 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 4) in;

layout(location = 0) uniform sampler2D butterfly_sampler;
layout(location = 1) uniform int p;
layout(location = 2) uniform float pass;
layout(rgba16f, binding = 0) uniform image2DArray fft_a;
layout(rgba16f, binding = 1) uniform image2DArray fft_b;

vec4 fft2(int l, vec2 i, vec2 w, vec2 uv, layout(rgba16f) image2DArray fft_re) {
//   vec4 input1 = textureLod(img_sampler, vec3(uv.x, i.x, layer), 0.0);
//   vec4 input2 = textureLod(img_sampler, vec3(uv.x, i.y, layer), 0.0);
	vec4 input1 = imageLoad(fft_re, ivec3(uv.x * 256.0, i.x * 256.0, l));
	vec4 input2 = imageLoad(fft_re, ivec3(uv.x * 256.0, i.y * 256.0, l));
    float res1x = w.x * input2.x - w.y * input2.y;
    float res1y = w.y * input2.x + w.x * input2.y;
    float res2x = w.x * input2.z - w.y * input2.w;
    float res2y = w.y * input2.z + w.x * input2.w;
    return input1 + vec4(res1x, res1y, res2x, res2y);
}

void main()
{
	int layer = int(gl_GlobalInvocationID.z);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / 256.0;
	vec4 data = textureLod(butterfly_sampler, vec2(uv.y, pass), 0.0);
	vec2 i = data.xy;
	vec2 w = data.zw;
	if (mod(p, 2) == 0)
	{
		imageStore(fft_b, ivec3(gl_GlobalInvocationID.xyz), fft2(layer, i, w, uv, fft_a));
	} else
	{
		imageStore(fft_a, ivec3(gl_GlobalInvocationID.xyz), fft2(layer, i, w, uv, fft_b));
	}
}