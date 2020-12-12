#version 450 core
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

uniform sampler2D spectrum_1_2_Sampler;
uniform sampler2D spectrum_3_4_Sampler;
uniform float FFT_SIZE;
uniform vec4 INVERSE_GRID_SIZES;
uniform float t;

layout(rgba16f, binding = 0) restrict writeonly uniform image2DArray init_output;

vec2 getSpectrum(float k, vec2 s0, vec2 s0c)
{
    float w = sqrt(9.81 * k * (1.0 + k * k / (370.0 * 370.0)));
    float c = cos(w * t);
    float s = sin(w * t);
    return vec2((s0.x + s0c.x) * c - (s0.y + s0c.y) * s, (s0.x - s0c.x) * s + (s0.y - s0c.y) * c);
}
vec2 i(vec2 z)
{
    return vec2(-z.y, z.x); // returns i times z (complex number)
}
void main()
{
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / 256.0;
    vec2 st = floor(uv * FFT_SIZE) / FFT_SIZE;
    float x = uv.x > 0.5 ? st.x - 1.0 : st.x;
    float y = uv.y > 0.5 ? st.y - 1.0 : st.y;

    vec4 s12 = textureLod(spectrum_1_2_Sampler, uv, 0.0)*1.414213562;
    vec4 s34 = textureLod(spectrum_3_4_Sampler, uv, 0.0)*1.414213562;
    vec4 s12c = textureLod(spectrum_1_2_Sampler, vec2(1.0 + 0.5 / FFT_SIZE) - st, 0.0)*1.414213562;
    vec4 s34c = textureLod(spectrum_3_4_Sampler, vec2(1.0 + 0.5 / FFT_SIZE) - st, 0.0)*1.414213562;

    vec2 k1 = vec2(x, y) * INVERSE_GRID_SIZES.x;
    vec2 k2 = vec2(x, y) * INVERSE_GRID_SIZES.y;
    vec2 k3 = vec2(x, y) * INVERSE_GRID_SIZES.z;
    vec2 k4 = vec2(x, y) * INVERSE_GRID_SIZES.w;

    float K1 = length(k1);
    float K2 = length(k2);
    float K3 = length(k3);
    float K4 = length(k4);

    float IK1 = K1 == 0.0 ? 0.0 : 1.0 / K1;
    float IK2 = K2 == 0.0 ? 0.0 : 1.0 / K2;
    float IK3 = K3 == 0.0 ? 0.0 : 1.0 / K3;
    float IK4 = K4 == 0.0 ? 0.0 : 1.0 / K4;

    vec2 h1 = getSpectrum(K1, s12.xy, s12c.xy);
    vec2 h2 = getSpectrum(K2, s12.zw, s12c.zw);
    vec2 h3 = getSpectrum(K3, s34.xy, s34c.xy);
    vec2 h4 = getSpectrum(K4, s34.zw, s34c.zw);

    vec4 l0 = vec4(h1 + i(h2), h3 + i(h4));
    vec4 l1 = vec4(i(k1.x * h1) - k1.y * h1, i(k2.x * h2) - k2.y * h2);
    vec4 l2 = vec4(i(k3.x * h3) - k3.y * h3, i(k4.x * h4) - k4.y * h4);
    vec4 l3 = l1 * vec4(IK1, IK1, IK2, IK2);
    vec4 l4 = l2 * vec4(IK3, IK3, IK4, IK4);
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 0), l0);
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 1), l1);
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 2), l2);
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 3), l3);
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 4), l4); 
	vec4 IK = vec4(IK1,IK2,IK3,IK4);
	vec2 k1Squared = k1*k1;
	vec2 k2Squared = k2*k2;
	vec2 k3Squared = k3*k3;
	vec2 k4Squared = k4*k4;

	// 5: d(Dx(X,t))/dx 	Tes01 eq30
	// 6: d(Dy(X,t))/dy 	Tes01 eq30
	// 7: d(Dx(X,t))/dy 	Tes01 eq30
	vec4 tmp = vec4(h1.x, h2.x, h3.x, h4.x);

	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 5), -tmp * (vec4(k1Squared.x, k2Squared.x, k3Squared.x, k4Squared.x) * IK));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 6), -tmp * (vec4(k1Squared.y, k2Squared.y, k3Squared.y, k4Squared.y) * IK));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 7), -tmp * (vec4(k1.x*k1.y, k2.x*k2.y, k3.x*k3.y, k4.x*k4.y) * IK));
/*	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 0), vec4(1.0));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 1), vec4(1.0));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 2), vec4(1.0));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 3), vec4(1.0));
	imageStore(init_output, ivec3(gl_GlobalInvocationID.xy, 4), vec4(1.0)); */
}