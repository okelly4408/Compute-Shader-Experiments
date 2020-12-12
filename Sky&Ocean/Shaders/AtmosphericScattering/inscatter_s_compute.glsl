#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform restrict writeonly image3D delta_j_tex;
layout(location = 0) uniform sampler2D delta_e_tex;
layout(location = 1) uniform sampler2D transmittance_tex;
layout(location = 2) uniform float first;
layout(location = 3) uniform sampler3D delta_sr_sampler;
layout(location = 4) uniform sampler3D delta_sm_sampler;

const float AVERAGE_GROUND_REFLECTANCE = 0.1;
// Rayleigh
const float HR = 8.0;
const vec3 betaR = vec3(5.8e-3, 1.35e-2, 3.31e-2);

// Mie
// DEFAULT
const float HM = 1.2;
const vec3 betaMSca = vec3(4e-3);
const vec3 betaMEx = betaMSca / 0.9;
const float mieG = 0.8;

const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

const float Rg = 6360.0;
const float Rt = 6420.0;
const float RL = 6421.0;

const int TRANSMITTANCE_W = 256;
const int TRANSMITTANCE_H = 64;

const int SKY_W = 64;
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;
const float M_PI = 3.141592657;

const float dphi = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);
const float dtheta = M_PI / float(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);
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
vec2 getIrradianceUV(float r, float muS) {
    float uR = (r - Rg) / (Rt - Rg);
    float uMuS = (muS + 0.2) / (1.0 + 0.2);
    return vec2(uMuS, uR);
}
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
vec3 transmittance(float r, float mu, float d) {
    vec3 result;
    float r1 = sqrt(r * r + d * d + 2.0 * r * mu * d);
    float mu1 = (r * mu + d) / r1;
    if (mu > 0.0) {
        result = min(transmittance(r, mu) / transmittance(r1, mu1), 1.0);
    } else {
        result = min(transmittance(r1, -mu1) / transmittance(r, -mu), 1.0);
    }
    return result;
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
vec3 irradiance(sampler2D sampler, float r, float muS) {
    vec2 uv = getIrradianceUV(r, muS);
    return texture(sampler, uv).rgb;
}

// Rayleigh phase function
float phaseFunctionR(float mu) {
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) {
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
}

void inscatter(float r, float mu, float muS, float nu, out vec3 raymie) {
    r = clamp(r, Rg, Rt);
    mu = clamp(mu, -1.0, 1.0);
    muS = clamp(muS, -1.0, 1.0);
    float var = sqrt(1.0 - mu * mu) * sqrt(1.0 - muS * muS);
    nu = clamp(nu, muS * mu - var, muS * mu + var);

    float cthetamin = -sqrt(1.0 - (Rg / r) * (Rg / r));

    vec3 v = vec3(sqrt(1.0 - mu * mu), 0.0, mu);
    float sx = v.x == 0.0 ? 0.0 : (nu - muS * mu) / v.x;
    vec3 s = vec3(sx, sqrt(max(0.0, 1.0 - sx * sx - muS * muS)), muS);

    raymie = vec3(0.0);

    // integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7)
    for (int itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta) {
        float theta = (float(itheta) + 0.5) * dtheta;
        float ctheta = cos(theta);

        float greflectance = 0.0;
        float dground = 0.0;
        vec3 gtransp = vec3(0.0);
        if (ctheta < cthetamin) { // if ground visible in direction w
            // compute transparency gtransp between x and ground
            greflectance = AVERAGE_GROUND_REFLECTANCE / M_PI;
            dground = -r * ctheta - sqrt(r * r * (ctheta * ctheta - 1.0) + Rg * Rg);
            gtransp = transmittance(Rg, -(r * ctheta + dground) / Rg, dground);
        }

        for (int iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi) {
            float phi = (float(iphi) + 0.5) * dphi;
            float dw = dtheta * dphi * sin(theta);
            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), ctheta);

            float nu1 = dot(s, w);
            float nu2 = dot(v, w);
            float pr2 = phaseFunctionR(nu2);
            float pm2 = phaseFunctionM(nu2);

            // compute irradiance received at ground in direction w (if ground visible) =deltaE
            vec3 gnormal = (vec3(0.0, 0.0, r) + dground * w) / Rg;
            vec3 girradiance = irradiance(delta_e_tex, Rg, dot(gnormal, s));

            vec3 raymie1; // light arriving at x from direction w

            // first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE
            raymie1 = greflectance * girradiance * gtransp;

            // second term = inscattered light, =deltaS
            if (first == 1.0) {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = phaseFunctionR(nu1);
                float pm1 = phaseFunctionM(nu1);
                vec3 ray1 = texture4D(delta_sr_sampler, r, w.z, muS, nu1).rgb;
                vec3 mie1 = texture4D(delta_sm_sampler, r, w.z, muS, nu1).rgb;
                raymie1 += ray1 * pr1 + mie1 * pm1;
            } else {
                raymie1 += texture4D(delta_sr_sampler, r, w.z, muS, nu1).rgb;
            }

            // light coming from direction w and scattered in direction v
            // = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction)
            // see Eq (7)
            raymie += raymie1 * (betaR * exp(-(r - Rg) / HR) * pr2 + betaMSca * exp(-(r - Rg) / HM) * pm2) * dw;
        }
    }

    // output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1)
}

void main()
{
    vec3 raymie;
	float r;
	vec4 dhdH;
	set_layer(r, dhdH);
    float mu, muS, nu;
    getMuMuSNu(r, dhdH, mu, muS, nu);
    inscatter(r, mu, muS, nu, raymie);
	imageStore(delta_j_tex, ivec3(gl_GlobalInvocationID), vec4(raymie, 0.0));
}