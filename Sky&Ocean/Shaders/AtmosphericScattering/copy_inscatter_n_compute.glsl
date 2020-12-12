#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform image3D inscatter_tex;
layout(rgba16f, binding = 1) uniform restrict readonly image3D delta_s_tex;

const float Rg = 6360.0;
const float Rt = 6420.0;
const float RL = 6421.0;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;
const float M_PI = 3.141592657;

#define TRANSMITTANCE_NON_LINEAR
#define INSCATTER_NON_LINEAR
void set_layer(out float r, out vec4 dhdH)
{
	int layer = int(gl_GlobalInvocationID.z);
    r = layer / (RES_R - 1.0);
    r = r * r;
    r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
    float dmin = Rt - r;
    float dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
    float dminp = r - Rg;
    float dmaxp = sqrt(r * r - Rg * Rg);
    dhdH = vec4(float(dmin), float(dmax), float(dminp), float(dmaxp));
}
float phaseFunctionR(float mu) {
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}
void getMuMuSNu(float r, vec4 dhdH, out float mu, out float muS, out float nu) {
    float x = float(gl_GlobalInvocationID.x);
    float y = float(gl_GlobalInvocationID.y);
#ifdef INSCATTER_NON_LINEAR
    if (y < float(RES_MU) / 2.0) {
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999);
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d);
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001);
    } else {
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999);
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d);
    }
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
    // paper formula
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
    // better formula
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#else
    mu = -1.0 + 2.0 * y / (float(RES_MU) - 1.0);
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
    muS = -0.2 + muS * 1.2;
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
#endif
}
void main() {
    float mu, muS, nu;
	float r;
	vec4 dhdH;
	set_layer(r, dhdH);
    getMuMuSNu(r, dhdH, mu, muS, nu);
	int layer = int(gl_GlobalInvocationID.z);
    vec3 uvw = vec3(gl_GlobalInvocationID.xy, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
	vec4 addition = imageLoad(inscatter_tex, ivec3(gl_GlobalInvocationID.xyz));
	imageStore(inscatter_tex, ivec3(gl_GlobalInvocationID), vec4(imageLoad(delta_s_tex, ivec3(gl_GlobalInvocationID)).rgb / phaseFunctionR(nu), 0.0) + addition);
}