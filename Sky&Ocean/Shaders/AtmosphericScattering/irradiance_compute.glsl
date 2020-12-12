#version 450 core
layout(local_size_x = 8, local_size_y = 2, local_size_z = 1) in;
layout(rgba16f, binding = 0) uniform restrict writeonly image2D irradiance_tex;
layout(location = 0) uniform sampler2D transmittance_tex;
#define TRANSMITTANCE_NON_LINEAR
const float Rg = 6360.0;
const float Rt = 6420.0;
const int SKY_W = 64;
const int SKY_H = 16;

vec2 getTransmittanceUV(float r, float mu) {
    float uR, uMu;
#ifdef TRANSMITTANCE_NON_LINEAR
	uR = sqrt((r - Rg) / (Rt - Rg));
	uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
#else
	uR = (r - Rg) / (Rt - Rg);
	uMu = (mu + 0.15) / (1.0 + 0.15);
#endif
    return vec2(uMu, uR);
}

vec3 transmittance(float r, float mu) {
	vec2 uv = getTransmittanceUV(r, mu);
    return texture(transmittance_tex, uv).rgb;
}
void getIrradianceRMuS(out float r, out float muS)
{
    r = Rg + (float(gl_GlobalInvocationID.y)) / (float(SKY_H) - 1.0) * (Rt - Rg);
    muS = -0.2 + (float(gl_GlobalInvocationID.x)) / (float(SKY_W) - 1.0) * (1.0 + 0.2);
}
void main()
{
    float r, muS;
    getIrradianceRMuS(r, muS);
	imageStore(irradiance_tex, ivec2(gl_GlobalInvocationID.xy), vec4(transmittance(r, muS) * max(muS, 0.0), 0.0));
}