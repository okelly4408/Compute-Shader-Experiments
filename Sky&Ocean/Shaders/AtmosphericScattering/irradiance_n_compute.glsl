#version 450 core

layout(local_size_x = 8, local_size_y = 2, local_size_z = 1) in;
layout(rgba16f, binding = 0) uniform restrict writeonly image2D delta_e_tex;
layout(location = 0) uniform float first;
layout(location = 1) uniform sampler3D delta_sr_sampler;
layout(location = 2) uniform sampler3D delta_sm_sampler;
const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
const float M_PI = 3.141592657;
const float dphi = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES);
const float dtheta = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES);
const float Rg = 6360.0;
const float Rt = 6420.0;
const float RL = 6421.0;

const int SKY_W = 64;
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;
const float mieG = 0.8;
#define INSCATTER_NON_LINEAR

float phaseFunctionR(float mu) {
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) {
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
}
void getIrradianceRMuS(out float r, out float muS) {
    r = Rg + (float(gl_GlobalInvocationID.y)) / (float(SKY_H) - 1.0) * (Rt - Rg);
    muS = -0.2 + (float(gl_GlobalInvocationID.x)) / (float(SKY_W) - 1.0) * (1.0 + 0.2);
}
vec4 texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
    float H = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(r * r - Rg * Rg);
#ifdef INSCATTER_NON_LINEAR
    float rmu = r * mu;
    float delta = rmu * rmu - r * r + Rg * Rg;
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
    // paper formula
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
    // better formula
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));
#else
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
    float uMu = 0.5 / float(RES_MU) + (mu + 1.0) / 2.0 * (1.0 - 1.0 / float(RES_MU));
    float uMuS = 0.5 / float(RES_MU_S) + max(muS + 0.2, 0.0) / 1.2 * (1.0 - 1.0 / float(RES_MU_S));
#endif
    float lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
    float uNu = floor(lerp);
    lerp = lerp - uNu;
    return texture(table, vec3((uNu + uMuS) / float(RES_NU), uMu, uR)) * (1.0 - lerp) +
           texture(table, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)) * lerp;
}
void main() {
    float r, muS;
    getIrradianceRMuS(r, muS);
    vec3 s = vec3(max(sqrt(1.0 - muS * muS), 0.0), 0.0, muS);

    vec3 result = vec3(0.0);
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15)
    for (int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi) {
        float phi = (float(iphi) + 0.5) * dphi;
        for (int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta) {
            float theta = (float(itheta) + 0.5) * dtheta;
            float dw = dtheta * dphi * sin(theta);
            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            float nu = dot(s, w);
            if (first == 1.0) {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = phaseFunctionR(nu);
                float pm1 = phaseFunctionM(nu);
                vec3 ray1 = texture4D(delta_sr_sampler, r, w.z, muS, nu).rgb;
                vec3 mie1 = texture4D(delta_sm_sampler, r, w.z, muS, nu).rgb;
                result += (ray1 * pr1 + mie1 * pm1) * w.z * dw;
            } else {
                result = texture4D(delta_sr_sampler, r, w.z, muS, nu).rgb * w.z * dw;
            }
        }
    }
   imageStore(delta_e_tex, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0.0));
//	imageStore(delta_e_tex, ivec2(gl_GlobalInvocationID.xy), imageLoad(delta_sr_tex, ivec3(vec2(256.0, 128.0) * (vec2(gl_GlobalInvocationID.xy) / vec2(64.0, 16.0)), 0)));
//	imageStore(delta_e_tex, ivec2(gl_GlobalInvocationID.xy), vec4(vec2(gl_GlobalInvocationID.xy) / vec2(64.0, 16.0), 0.0, 1.0));
}