#version 450 core
in vec2 uv;
in vec3 color;
in vec3 P;
in vec2 u;
out vec4 frag_data;
uniform sampler2DArray wave_sampler;
uniform sampler2D reflectance_texture;
uniform sampler2D transmittance_texture;
uniform sampler3D slope_variance_sampler;
uniform sampler2D irradiance_texture;
uniform vec3 ocean_camera_pos;
uniform float time;
in vec3 camera_dir;
uniform vec3 sun_dir;
uniform vec4 GRID_SIZES;
uniform vec2 iResolution;
const vec4 choppy_factor = vec4(2.3, 2.1, 1.3, 0.9);
const float ISUN = 15.0;
#define INSCATTER_NON_LINEAR
#define TRANSMITTANCE_NON_LINEAR
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
/***FRACTALS***/
float hash( float n )
{
    return fract(sin(n)*43758.5453);
}
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

//x3
vec3 noise3d( in vec3 x)
{
	return vec3( noise(x+vec3(123.456,.567,.37)),
				noise(x+vec3(.11,47.43,19.17)),
				noise(x) );
}

float bias(float x, float b) {
	return  x/((1./b-2.)*(1.-x)+1.);
}

float gain(float x, float g) {
	float t = (1./g-2.)*(1.-(2.*x));	
	return x<0.5 ? (x/(t+1.)) : (t-x)/(t-1.);
}


mat3 rotation(float angle, vec3 axis)
{
    float s = sin(-angle);
    float c = cos(-angle);
    float oc = 1.0 - c;
	vec3 sa = axis * s;
	vec3 oca = axis * oc;
    return mat3(	
		oca.x * axis + vec3(	c,	-sa.z,	sa.y),
		oca.y * axis + vec3( sa.z,	c,		-sa.x),		
		oca.z * axis + vec3(-sa.y,	sa.x,	c));	
}

vec3 fbm(vec3 x, float H, float L, int oc)
{
	vec3 v = vec3(0);
	float f = 1.;
	for (int i=0; i<10; i++)
	{
		if (i >= oc) break;
		float w = pow(f,-H);
		v += noise3d(x)*w;
		x *= L;
		f *= L;
	}
	return v;
}

vec3 smf(vec3 x, float H, float L, int oc, float off)
{
	vec3 v = vec3(1);
	float f = 1.;
	for (int i=0; i<10; i++)
	{
		if (i >= oc) break;
		v *= off + f*(noise3d(x)*2.-1.);
		f *= H;
		x *= L;
	}
	return v;	
}

float erfc(float x) {
	return 2.0 * exp(-x * x) / (2.319 * x + sqrt(4.0 + 1.52 * x * x));
}

float Lambda(float cosTheta, float sigmaSq) {
	float v = cosTheta / sqrt((1.0 - cosTheta * cosTheta) * (2.0 * sigmaSq));
    return max(0.0, (exp(-v * v) - v * sqrt(M_PI) * erfc(v)) / (2.0 * v * sqrt(M_PI)));
	//return (exp(-v * v)) / (2.0 * v * sqrt(M_PI)); // approximate, faster formula
}

// L, V, N, Tx, Ty in world space
float reflectedSunRadiance(vec3 L, vec3 V, vec3 N, vec3 Tx, vec3 Ty, vec2 sigmaSq) {
    vec3 H = normalize(L + V);
    float zetax = dot(H, Tx) / dot(H, N);
    float zetay = dot(H, Ty) / dot(H, N);

    float zL = dot(L, N); // cos of source zenith angle
    float zV = dot(V, N); // cos of receiver zenith angle
    float zH = dot(H, N); // cos of facet normal zenith angle
    float zH2 = zH * zH;

    float p = exp(-0.5 * (zetax * zetax / sigmaSq.x + zetay * zetay / sigmaSq.y)) / (2.0 * M_PI * sqrt(sigmaSq.x * sigmaSq.y));

    float tanV = atan(dot(V, Ty), dot(V, Tx));
    float cosV2 = 1.0 / (1.0 + tanV * tanV);
    float sigmaV2 = sigmaSq.x * cosV2 + sigmaSq.y * (1.0 - cosV2);

    float tanL = atan(dot(L, Ty), dot(L, Tx));
    float cosL2 = 1.0 / (1.0 + tanL * tanL);
    float sigmaL2 = sigmaSq.x * cosL2 + sigmaSq.y * (1.0 - cosL2);

    float fresnel = 0.02 + 0.98 * pow(1.0 - dot(V, H), 5.0);

    zL = max(zL, 0.01);
    zV = max(zV, 0.01);

    return fresnel * p / ((1.0 + Lambda(zL, sigmaL2) + Lambda(zV, sigmaV2)) * zV * zH2 * zH2 * 4.0);
}

// ---------------------------------------------------------------------------
// REFLECTED SKY RADIANCE
// ---------------------------------------------------------------------------


// V, N, Tx, Ty in world space
vec2 U(vec2 zeta, vec3 V, vec3 N, vec3 Tx, vec3 Ty) {
    vec3 f = normalize(vec3(-zeta.x, 1.0, -zeta.y)); // tangent space
    vec3 F = f.x * Tx + f.y * N + f.z * Ty; // world space
    vec3 R = 2.0 * dot(F, V) * F - V;
    return R.xz / (1.0 + R.y);
}

float meanFresnel(float cosThetaV, float sigmaV) {
	return pow(1.0 - cosThetaV, 5.0 * exp(-2.69 * sigmaV)) / (1.0 + 22.7 * pow(sigmaV, 1.5));
}

// V, N in world space
float meanFresnel(vec3 V, vec3 N, vec2 sigmaSq) {
    vec2 v = V.xz; // view direction in wind space
    vec2 t = v * v / (1.0 - V.y * V.y); // cos^2 and sin^2 of view direction
    float sigmaV2 = dot(t, sigmaSq); // slope variance in view direction
    return meanFresnel(dot(V, N), sqrt(sigmaV2));
}

// V, N, Tx, Ty in world space;
vec3 meanSkyRadiance(vec3 V, vec3 N, vec3 Tx, vec3 Ty, vec2 sigmaSq) {
    vec4 result = vec4(0.0);

    const float eps = 0.001;
    vec2 u0 = U(vec2(0.0), V, N, Tx, Ty);
    vec2 dux = 2.0 * (U(vec2(eps, 0.0), V, N, Tx, Ty) - u0) / eps * sqrt(sigmaSq.x);
    vec2 duy = 2.0 * (U(vec2(0.0, eps), V, N, Tx, Ty) - u0) / eps * sqrt(2.0);

    result = textureGrad(reflectance_texture, u0 * (0.5 / 1.1) + 0.5, dux * (0.5 / 1.1), duy * (0.5 / 1.1));


    return result.rgb;
	//return vec3(duy, 0.0);
}
vec2 getIrradianceUV(float r, float muS) {
    float uR = (r - Rg) / (Rt - Rg);
    float uMuS = (muS + 0.2) / (1.0 + 0.2);
    return vec2(uMuS, uR);
}
vec3 irradiance(sampler2D sampler, float r, float muS) {
    vec2 uv = getIrradianceUV(r, muS);
    return texture(sampler, uv).rgb;
}
vec3 skyIrradiance(float r, float muS) {
    return irradiance(irradiance_texture, r, muS) * ISUN;
}
vec2 getTransmittanceUV(float r, float mu) {
    float uR, uMu;
    uR = sqrt((r - Rg) / (Rt - Rg));
    uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
    return vec2(uMu, uR);
}
vec3 transmittance(float r, float mu) {
    vec2 uv = getTransmittanceUV(r, mu);
    return texture(transmittance_texture, uv).rgb;
}
vec3 transmittanceWithShadow(float r, float mu) {
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? vec3(0.0) : transmittance(r, mu);
}
vec3 sunRadiance(float r, float muS) {
    return transmittanceWithShadow(r, muS) * 4.0;
}
void sunRadianceAndSkyIrradiance(vec3 worldP, vec3 worldS, out vec3 sunL, out vec3 skyE)
{
    vec3 worldV = normalize(worldP); // vertical vector
    float r = length(worldP);
    float muS = dot(worldV, worldS);
    sunL = sunRadiance(r, muS) * ISUN;
    skyE = skyIrradiance(r, muS);
}
vec3 hdr(vec3 L) {
    L = L * exposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}
void main()
{
	vec3 V = normalize(ocean_camera_pos - P);
	vec3 worldSunDir = normalize(sun_dir);
	vec3 seaColor = vec3(10.0 / 255.0, 40.0 / 255.0, 120.0 / 255.0);



    vec2 slopes = texture(wave_sampler, vec3(u / GRID_SIZES.x, 1.0)).xy;
    slopes += texture(wave_sampler, vec3(u / GRID_SIZES.y, 1.0)).zw;
    slopes += texture(wave_sampler, vec3(u / GRID_SIZES.z, 2.0)).xy;
    slopes += texture(wave_sampler, vec3(u / GRID_SIZES.w, 2.0)).zw;
	vec4 lambda = choppy_factor;
	float jxx, jxy, jyy;
		// Jxx1..4 : partial Jxx
	float Jxx1 = texture(wave_sampler, vec3(u / GRID_SIZES.x, 5.0)).r;
	float Jxx2 = texture(wave_sampler, vec3(u / GRID_SIZES.y, 5.0)).g;
	float Jxx3 = texture(wave_sampler, vec3(u / GRID_SIZES.z, 5.0)).b;
	float Jxx4 = texture(wave_sampler, vec3(u / GRID_SIZES.w, 5.0)).a;
	jxx = dot((lambda), vec4(Jxx1,Jxx2,Jxx3,Jxx4));

		// Jyy1..4 : partial Jyy
	float Jyy1 = texture(wave_sampler, vec3(u / GRID_SIZES.x, 6.0)).r;
	float Jyy2 = texture(wave_sampler, vec3(u / GRID_SIZES.y, 6.0)).g;
	float Jyy3 = texture(wave_sampler, vec3(u / GRID_SIZES.z, 6.0)).b;
	float Jyy4 = texture(wave_sampler, vec3(u / GRID_SIZES.w, 6.0)).a;
	jyy = dot((lambda), vec4(Jyy1,Jyy2,Jyy3,Jyy4));

		slopes /= (1.0 + vec2(jxx, jyy));
    vec3 N = normalize(vec3(-slopes.x, 1.0, -slopes.y));
    if (dot(V, N) < 0.0) {
        N = reflect(N, V); // reflects backfacing normals
    }
		float height = P.y;
	float height_off = 0.0025 * height;
	height_off += 0.0025;
	height_off = max(0.0, height_off);
	height_off *= 2.5;
	float light_ray =  (pow(max(0.0, dot(normalize(vec3(sun_dir.xy, 0.0)), 1.0 - camera_dir)), 2.0) * height_off) * pow(max(1.0 - dot(worldSunDir, N), 0.0), 8.0);
	float sub_lighting = max(dot(camera_dir, N), 0.0) * (max((height * 0.01) + 1.0, 0.0) * (0.2 * 1.5));
	vec3 final_sss = (sub_lighting * (300.0 / (max((1.0 - camera_dir), 0.0) * (length(ocean_camera_pos - P) + 300.0)) + light_ray)) * 0.9;
	float Jxx = dFdx(u.x);
    float Jxy = dFdy(u.x);
    float Jyx = dFdx(u.y);
    float Jyy = dFdy(u.y);
    float A = Jxx * Jxx + Jyx * Jyx;
    float B = Jxx * Jxy + Jyx * Jyy;
    float C = Jxy * Jxy + Jyy * Jyy;
    const float SC = 10.0;
    float ua = pow(A / SC, 0.25);
    float ub = 0.5 + 0.5 * B / sqrt(A * C);
    float uc = pow(C / SC, 0.25);
    vec2 sigmaSq = texture(slope_variance_sampler, vec3(ua, ub, uc)).xw;

    sigmaSq = max(sigmaSq, 2e-5);

    vec3 Ty = normalize(vec3(0.0, N.y, -N.z)).xzy;
    vec3 Tx = cross(Ty, N);
	float fresnel = 0.02 + 0.98 * meanFresnel(V, N, sigmaSq);
	 vec3 Lsun;
    vec3 Esky;
    vec3 extinction;
    sunRadianceAndSkyIrradiance(vec3(0.0, ocean_camera_pos.y, 0.0) + earthPos, worldSunDir, Lsun, Esky);
	frag_data = vec4(0.0);

    frag_data.rgb += reflectedSunRadiance(worldSunDir, V, N, Tx, Ty, sigmaSq) * Lsun;

    frag_data.rgb += fresnel * meanSkyRadiance(V, N, Tx, Ty, sigmaSq);
	float slow = time*0.002;
	vec3 fractal_position = vec3(u + (1. + .5*slow*sin(slow*10.)), slow) * 0.075;
	vec3 axis = 4.0 * fbm(fractal_position, 0.5, 2.0, 8);
	vec3 colorvec = 0.5 * 5.0 * fbm(fractal_position * 0.3, 0.5, 2.0, 7);
	fractal_position += colorvec;
	vec3 colormod = 0.75e5 * smf(fractal_position, 0.7, 2.0, 8, 0.2);
	float height_scale = P.y;
	colorvec += colormod;
	colorvec = rotation(3.0 * length(axis), normalize(axis)) * colorvec;
	colorvec *= 0.05;
	colorvec = pow(colorvec, vec3(1./2.2));
	colorvec += (height_scale * 0.086);
   vec3 Lsea = (colorvec * 4.0) * Esky / M_PI;
   frag_data.rgb += (1.0 - fresnel) * Lsea;
	
    frag_data.rgb += 0.0001 * (colorvec + height_scale) * (Lsun * max(dot(N, worldSunDir), 0.0) + Esky) / M_PI;
	frag_data.rgb = hdr(frag_data.rgb);
//	frag_data.rgb = colorvec;
	frag_data.a = 1.0;

    //frag_data.rgb = hdr(frag_data.rgb);
	//frag_data = texture(wave_sampler, vec3(uv, 0.0));
	//frag_data = texture(spec_1_2, uv) * 256.0;
}
