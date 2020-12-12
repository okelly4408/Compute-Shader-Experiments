#version 450 core
uniform sampler2D screen_texture;
uniform sampler2D perm_texture;
uniform sampler2D grad_texture;
uniform sampler3D noise_map;
uniform vec3 up;
uniform vec3 right;
uniform vec3 forward;
uniform vec3 pos;
uniform mat4 perspective;
in vec2 uv;
out vec4 frag_color;


vec3 fade(vec3 t)
{
	return (t * t * t * (t * (t * 6.0 - 15.0) + 10.0));
}
float grad_perm(float x, vec3 p)
{
	return dot(vec3(texture(grad_texture, vec2(x, 0.0)).xyz), p);
}
vec4 perm2d(vec2 p)
{
	return texture(perm_texture, p);
}
float inoise(in vec3 p)
{
	vec3 P = mod(floor(p), 256.0);	
  	p -= floor(p);                      
	vec3 f = fade(p);                 
	P = P / 256.0;
	const float one = 1.0 / 256.0;  
	vec4 AA = perm2d(P.yx) + P.z;
  	return mix( mix( mix( grad_perm(AA.x, p ),  
                             grad_perm(AA.z, p + vec3(-1.0, 0.0, 0.0) ), f.x),
                       mix( grad_perm(AA.y, p + vec3(0.0, -1.0, 0.0) ),
                             grad_perm(AA.w, p + vec3(-1.0, -1.0, 0.0) ), f.x), f.y),
                             
                 mix( mix( grad_perm(AA.x+one, p + vec3(0.0, 0.0, -1.0) ),
                             grad_perm(AA.z+one, p + vec3(-1.0, 0.0, -1.0) ), f.x),
                       mix( grad_perm(AA.y+one, p + vec3(0.0, -1.0, -1.0) ),
                             grad_perm(AA.w+one, p + vec3(-1.0, -1.0, -1.0) ), f.x), f.y), f.z);
							/* return  grad_perm(AA.x, p + vec3(-1.0, 0.0, 0.0));*/

}




float sphere(vec3 p, float r)
{
	return length(p) - r;
}
void main(void)
{
	vec4 pixel = texture(screen_texture, uv);
	vec3 rd = normalize(right * uv.x + up * uv.y + forward);
	vec3 ro = pos;
/*	float d = 0.0;
	float precis = 0.3;
	vec3 col = vec3(0.0);
	for (int i = 0; i < 16; i++)
	{
		vec3 p = ro + rd * d;
		vec3 pp = (perspective * vec4(p, 1.0)).xyz;
		float t = length(pp) - 0.5;
		if(abs(d) < precis)
		{
			col = vec3(1.0);
			break;
		}
		d += t;
	}*/
	//frag_color = texture(noise_map, vec3(uv, 0.0));
	//float noisey = (inoise(vec3(uv, 0.0) * 8.0) + 1.0) / 2.0;
	//frag_color = vec4(noisey);
	frag_color = texture(screen_texture, uv);

}