#version 450 core

uniform sampler2D geometry_texture;
uniform sampler3D inscatter_texture;
uniform sampler2D transmittance_texture;
uniform vec3 camera_pos;
uniform vec3 sun_dir;
uniform float time;
const float ISUN = 100.0;
#define INSCATTER_NON_LINEAR
#define TRANSMITTANCE_NON_LINEAR
in vec2 uv;
in vec3 view_dir;
out vec4 frag_color;
const vec3 earthPos = vec3(0.0, 6360010.0, 0.0);
const float SCALE = 1000.0;
const float Rg = 6360.0 * SCALE;
const float Rt = 6420.0 * SCALE;
const float RL = 6421.0 * SCALE;

const float AVERAGE_GROUND_REFLECTANCE = 0.1;

// Rayleigh
const float HR = 8.0 * SCALE;
const vec3 betaR = vec3(5.8e-3, 1.35e-2, 3.31e-2) / SCALE;

// Mie
// DEFAULT
const float HM = 1.2 * SCALE;
const vec3 betaMSca = vec3(4e-3) / SCALE;
const vec3 betaMEx = betaMSca / 0.9;
const float mieG = 0.72;
const int TRANSMITTANCE_W = 256;
const int TRANSMITTANCE_H = 64;

const int SKY_W = 64;
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;
const float M_PI = 3.141592657;
const float exposure = 0.4;

float field(in vec3 p,float s) {
	float strength = 7. + .03 * log(1.e-6 + fract(sin(16.3141) * 4373.11));
	float accum = s/4.;
	float prev = 0.;
	float tw = 0.;
	for (int i = 0; i < 26; ++i) {
		float mag = dot(p, p);
		p = abs(p) / mag + vec3(-.5, -.4, -1.5);
		float w = exp(-float(i) / 7.);
		accum += w * exp(-strength * pow(abs(mag - prev), 2.2));
		tw += w;
		prev = mag;
	}
	return max(0., 5. * accum / tw - .7);
}

// Less iterations for second layer
float field2(in vec3 p, float s) {
	float strength = 7. + .03 * log(1.e-6 + fract(sin(16.3141) * 4373.11));
	float accum = s/4.;
	float prev = 0.;
	float tw = 0.;
	for (int i = 0; i < 18; ++i) {
		float mag = dot(p, p);
		p = abs(p) / mag + vec3(-.5, -.4, -1.5);
		float w = exp(-float(i) / 7.);
		accum += w * exp(-strength * pow(abs(mag - prev), 2.2));
		tw += w;
		prev = mag;
	}
	return max(0., 5. * accum / tw - .7);
}

vec3 nrand3( vec2 co )
{
	vec3 a = fract( cos( co.x*8.3e-3 + co.y )*vec3(1.3e5, 4.7e5, 2.9e5) );
	vec3 b = fract( sin( co.x*0.3e-3 + co.y )*vec3(8.1e5, 1.0e5, 0.1e5) );
	vec3 c = mix(a, b, 0.5);
	return c;
}

float phaseFunctionR(float mu) {
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) {
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
}

// approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
vec3 getMie(vec4 rayMie) { // rayMie.rgb=C*, rayMie.w=Cm,r
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
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
	vec2 c = getTransmittanceUV(r, mu);
    return texture(transmittance_texture, c).rgb;
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
vec3 sky_radiance(vec3 camera, vec3 viewdir, vec3 sundir, out vec3 extinction)
{
    vec3 result;
    float r = length(camera);
    float rMu = dot(camera, viewdir);
    float mu = rMu / r;
    float r0 = r;
    float mu0 = mu;

    float deltaSq = sqrt(rMu * rMu - r * r + Rt*Rt);
    float din = max(-rMu - deltaSq, 0.0);
    if (din > 0.0) {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r = Rt;
    }

    if (r <= Rt) {
        float nu = dot(viewdir, sundir);
        float muS = dot(camera, sundir) / r;

        vec4 inScatter = texture4D(inscatter_texture, r, rMu / r, muS, nu);
        extinction = transmittance(r, mu);

        vec3 inScatterM = getMie(inScatter);
        float phase = phaseFunctionR(nu);
        float phaseM = phaseFunctionM(nu);
        result = inScatter.rgb * phase + inScatterM * phaseM;
    } else {
        result = vec3(0.0);
        extinction = vec3(1.0);
    }

    return result * ISUN;
}
vec3 HDR(vec3 L) {
    L = L * exposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}
void main()
{
	vec3 v = normalize(view_dir);
	//fractals
	vec3 fv1 = 1.0 * v;
	vec3 fv2 = (fv1 * 1024.0) / 768.0;
	vec3 p = vec3(fv2 / 4.) + vec3(1.0, -1.3, -1.0);
	p += 0.2 * vec3(sin(time / 16.0), sin(time / 12.0), sin(time / 128.0));
	float freqs[4];
	freqs[0] = 0.25;
	freqs[1] = 0.167;
	freqs[2] = 0.56563;
	freqs[3] = 0.7156;
	float fractalt1 = field(p, freqs[2]);
	float fractalv1 = (1.0 - exp((abs(fv2.x) - 1.0) * 6.0)) * (1.0 - exp((abs(fv2.y) - 1.0) * 6.0)) * (1.0 - exp((abs(fv2.z) - 1.0) * 6.0));
	vec3 p2 = vec3(fv2 / (4.+sin(time*0.11)*0.2+0.2+sin(time*0.15)*0.3+0.4)) + vec3(2.0, -1.3, -1.0);
	p2 += 0.25 * vec3(sin(time / 16.), sin(time / 12.),  sin(time / 128.));
	float fractalt2 = field2(p2, freqs[3]);
	vec4 c2 = mix(.4, 1., fractalv1) * vec4(1.3 * fractalt2 * fractalt2 * fractalt2 ,1.8  * fractalt2 * fractalt2 , fractalt2* freqs[0], fractalt2);
	vec4 fractal_final = mix(freqs[3]-.3, 1., fractalv1) * vec4(1.5*freqs[2] * fractalt1 * fractalt1* fractalt1 , 1.2*freqs[1] * fractalt1 * fractalt1, freqs[3]*fractalt1, 1.0)+c2;
	vec2 seed = (p.xy + p.z) * 2.0;	
	seed = floor(seed * 768.0);
	vec3 rnd = nrand3( seed );
	vec4 starcolor = vec4(pow(rnd.y,40.0));
	
	//Second Layer
	vec2 seed2 = (p2.xy + p2.z) * 2.0;
	seed2 = floor(seed2 * 768.0);
	vec3 rnd2 = nrand3( seed2 );
	starcolor += vec4(pow(rnd2.y,40.0));
	fractal_final += starcolor;
	//scattering
	vec3 sun_color = vec3(step(cos(3.1415926 / 180.0), dot(v, normalize(sun_dir)))) * ISUN;
	vec3 extinction;
	vec3 inscatter = sky_radiance(camera_pos + earthPos, v, normalize(sun_dir), extinction);
	vec3 sky_color = inscatter + (sun_color * extinction);
	sky_color = HDR(sky_color + fractal_final.rgb);
	frag_color = vec4(sky_color, 1.0);
}