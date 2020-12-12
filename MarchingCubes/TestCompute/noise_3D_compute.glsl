#version 450 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
uniform sampler2D gradient_texture;
uniform sampler2D permutation_texture;

layout(r32f, binding = 1)	uniform writeonly image3D noise_map;

vec3 fade(vec3 t)
{
	return (t * t * t * (t * (t * 6.0 - 15.0) + 10.0));
}
float grad_perm(float x, vec3 p)
{
	return dot(vec3(texture(gradient_texture, vec2(x, 0)).xyz), p);
}
vec4 perm2d(vec2 p)
{
	return texture(permutation_texture, p);
}
float inoise(in vec3 p)
{
	vec3 P = mod(floor(p), 256.0);	
  	p -= floor(p);                      
	vec3 f = fade(p);                 
	P = P / 256.0;
	const float one = 1.0 / 256.0;  
	vec4 AA = perm2d(P.yx) + P.z;
  	return ((mix( mix( mix( grad_perm(AA.x, p ),  
                             grad_perm(AA.z, p + vec3(-1.0, 0.0, 0.0) ), f.x),
                       mix( grad_perm(AA.y, p + vec3(0.0, -1.0, 0.0) ),
                             grad_perm(AA.w, p + vec3(-1.0, -1.0, 0.0) ), f.x), f.y),
                             
                 mix( mix( grad_perm(AA.x+one, p + vec3(0.0, 0.0, -1.0) ),
                             grad_perm(AA.z+one, p + vec3(-1.0, 0.0, -1.0) ), f.x),
                       mix( grad_perm(AA.y+one, p + vec3(0.0, -1.0, -1.0) ),
                             grad_perm(AA.w+one, p + vec3(-1.0, -1.0, -1.0) ), f.x), f.y), f.z)) + 1.0) * 0.5;
}

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

float fbm(in vec3 p)
{
	float signal = 0.0;
	float amplitude = 1.5;
	float lacunarity = 1.61;
	float frequency = 1.0;
	float gain = 0.5;
	for (uint i = 0; i < 12; i++)
	{
		float n = inoise(p * frequency) * amplitude;
		signal += n;
		frequency *= lacunarity;
		amplitude *= gain;
	}
	return (signal + 1.0) / 2.0;
}
float rnoise(vec3 p) {
	float n = 1.0 - abs((noise(p)) * 2.0);
	return n*n - 0.5;
}

float rmf3D(vec3 p, int octaves, float frequency, float lacunarity, float gain) {
    float sum = 0.0;
    float amp = 1.0;
    for(int i = 0; i < octaves; i++) {
        float n = rnoise(p * frequency);
        sum += n * amp;
        frequency *= lacunarity;
        amp *= gain;
    }
    return sum;
}
void main()
{
	vec3 pos = (vec3(gl_GlobalInvocationID.xyz) / (vec3(gl_NumWorkGroups * gl_WorkGroupSize) * 4.0)) * 6.4324;
	float n = fbm(pos);
	//float n = 0.5* ((rmf3D(pos * 2.0, 12, 1.55, 1.31, 0.55)) + 1.0);
	//float n = (fbm(pos * 2.0)) - 1.0;
	//vec4 nn = texture(permutation_texture, vec2(gl_GlobalInvocationID.xy) / (vec2(gl_NumWorkGroups * gl_WorkGroupSize)));
	// = length(pos - 1.5) + ((1.0 - (n)) * 0.4);
	imageStore(noise_map, ivec3(gl_GlobalInvocationID.xyz),vec4(n));
}
